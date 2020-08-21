/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "debug.h"
#include "file.h"
#include "dir.h"
#include "disk_cache.h"
#include "fileobj_mgr.h"
#include "dircache_redirect.h"

/**
 * Manages file and directory streams on all volumes
 *
 * Intended for internal use by disk, file and directory code
 */


/* there will always be enough of these for all user handles, thus these
   functions don't return failure codes */
#define MAX_FILEOBJS (MAX_OPEN_HANDLES + AUX_FILEOBJS)

/* describes the file as an image on the storage medium */
static struct fileobj_binding
{
    struct file_base_binding bind;    /* base info list item (first!) */
    uint16_t                 flags;   /* F(D)(O)_* bits of this file/dir */
    uint16_t                 writers; /* number of writer streams */
    struct filestr_cache     cache;   /* write mode shared cache */
    file_size_t              size;    /* size of this file */
    struct ll_head           list;    /* open streams for this file/dir */
} fobindings[MAX_FILEOBJS];
static struct mutex stream_mutexes[MAX_FILEOBJS] SHAREDBSS_ATTR;
static struct ll_head free_bindings;
static struct ll_head busy_bindings[NUM_VOLUMES];

#define BUSY_BINDINGS(volume) \
    (&busy_bindings[IF_MV_VOL(volume)])

#define BASEBINDING_LIST(bindp) \
    (BUSY_BINDINGS(BASEBINDING_VOL(bindp)))

#define FREE_BINDINGS() \
    (&free_bindings)

#define BINDING_FIRST(type, volume...) \
    ((struct fileobj_binding *)type##_BINDINGS(volume)->head)

#define BINDING_NEXT(fobp) \
    ((struct fileobj_binding *)(fobp)->bind.node.next)

#define FOR_EACH_BINDING(volume, fobp) \
    for (struct fileobj_binding *fobp = BINDING_FIRST(BUSY, volume); \
         fobp; fobp = BINDING_NEXT(fobp))

#define STREAM_FIRST(fobp) \
    ((struct filestr_base *)(fobp)->list.head)

#define STREAM_NEXT(s) \
    ((struct filestr_base *)(s)->node.next)

#define STREAM_THIS(s) \
    (s)

#define FOR_EACH_STREAM(what, start, s) \
    for (struct filestr_base *s = STREAM_##what(start); \
         s; s = STREAM_NEXT(s))


/* syncs information for the stream's old and new parent directory if any are
   currently opened */
static void fileobj_sync_parent(const struct file_base_info *infop[],
                                int count)
{
    FOR_EACH_BINDING(infop[0]->volume, fobp)
    {
        if ((fobp->flags & (FO_DIRECTORY|FO_REMOVED)) != FO_DIRECTORY)
            continue; /* not directory or removed can't be parent of anything */

        struct filestr_base *parentstrp = STREAM_FIRST(fobp);
        struct fat_file *parentfilep = &parentstrp->infop->fatfile;

        for (int i = 0; i < count; i++)
        {
            if (!fat_dir_is_parent(parentfilep, &infop[i]->fatfile))
                continue;

            /* discard scan/read caches' parent dir info */
            FOR_EACH_STREAM(THIS, parentstrp, s)
                filestr_discard_cache(s);
        }
    }
}

/* see if this file has open streams and return that fileobj_binding if so,
   else grab a new one from the free list; returns true if this stream is
   the only open one */
static bool binding_assign(const struct file_base_info *srcinfop,
                           struct fileobj_binding **fobpp)
{
    FOR_EACH_BINDING(srcinfop->fatfile.volume, fobp)
    {
        if (fobp->flags & FO_REMOVED)
            continue;

        if (fat_file_is_same(&srcinfop->fatfile, &fobp->bind.info.fatfile))
        {
            /* already has open streams */
            *fobpp = fobp;
            return false;
        }
    }

    /* not found - allocate anew */
    *fobpp = BINDING_FIRST(FREE);
    ll_remove_first(FREE_BINDINGS());
    ll_init(&(*fobpp)->list);
    return true;
}

/* mark descriptor as unused and return to the free list */
static void binding_add_to_free_list(struct fileobj_binding *fobp)
{
    fobp->flags = 0;
    ll_insert_last(FREE_BINDINGS(), &fobp->bind.node);
}

/** File and directory internal interface **/

void file_binding_insert_last(struct file_base_binding *bindp)
{
    ll_insert_last(BASEBINDING_LIST(bindp), &bindp->node);
}

void file_binding_remove(struct file_base_binding *bindp)
{
    ll_remove(BASEBINDING_LIST(bindp), &bindp->node);
}

#ifdef HAVE_DIRCACHE
void file_binding_insert_first(struct file_base_binding *bindp)
{
    ll_insert_first(BASEBINDING_LIST(bindp), &bindp->node);
}

void file_binding_remove_next(struct file_base_binding *prevp,
                              struct file_base_binding *bindp)
{
    ll_remove_next(BASEBINDING_LIST(bindp), &prevp->node);
    (void)bindp;
}
#endif /* HAVE_DIRCACHE */

/* opens the file object for a new stream and sets up the caches;
 * the stream must already be opened at the FS driver level and *stream
 * initialized.
 *
 * NOTE: switches stream->infop to the one kept in common for all streams of
 *       the same file, making a copy for only the first stream
 */
void fileobj_fileop_open(struct filestr_base *stream,
                         const struct file_base_info *srcinfop,
                         unsigned int callflags)
{
    struct fileobj_binding *fobp;
    bool first = binding_assign(srcinfop, &fobp);

    /* add stream to this file's list */
    ll_insert_last(&fobp->list, &stream->node);

    /* initiate the new stream into the enclave */
    stream->flags = FDO_BUSY |
                    (callflags & (FF_MASK|FD_WRITE|FD_WRONLY|FD_APPEND));
    stream->infop = &fobp->bind.info;
    stream->fatstr.fatfilep = &fobp->bind.info.fatfile;
    stream->bindp = &fobp->bind;
    stream->mtx   = &stream_mutexes[fobp - fobindings];

    if (first)
    {
        /* first stream for file */
        fobp->bind.info = *srcinfop;
        fobp->flags     = FDO_BUSY | FO_SINGLE |
                          (callflags & (FO_DIRECTORY|FO_TRUNC));
        fobp->writers   = 0;
        fobp->size      = 0;

        fileobj_bind_file(&fobp->bind);
    }
    else
    {
        /* additional stream for file */
        fobp->flags &= ~FO_SINGLE;
        fobp->flags |= callflags & FO_TRUNC;

        /* once a file/directory, always a file/directory; such a change
           is a bug */
        if ((callflags ^ fobp->flags) & FO_DIRECTORY)
        {
            DEBUGF("%s - FO_DIRECTORY flag does not match: %p %u\n",
                   __func__, stream, callflags);
        }
    }

    if ((callflags & FD_WRITE) && ++fobp->writers == 1)
    {
        /* first writer */
        file_cache_init(&fobp->cache);
        FOR_EACH_STREAM(FIRST, fobp, s)
            filestr_assign_cache(s, &fobp->cache);
    }
    else if (fobp->writers)
    {
        /* already writers present */
        filestr_assign_cache(stream, &fobp->cache);
    }
    else
    {
        /* another reader and no writers present */
        file_cache_reset(stream->cachep);
    }
}

/* close the stream and free associated resources */
void fileobj_fileop_close(struct filestr_base *stream)
{
    if (!(stream->flags & FDO_BUSY))
    {
        /* not added to manager or forced-closed by unmounting */
        filestr_base_destroy(stream);
        return;
    }

    struct fileobj_binding *fobp = (struct fileobj_binding *)stream->bindp;
    unsigned int foflags = fobp->flags;

    ll_remove(&fobp->list, &stream->node);

    if (foflags & FO_SINGLE)
    {
       /* last stream for file; close everything */
        fileobj_unbind_file(&fobp->bind);

        if (fobp->writers)
            file_cache_free(&fobp->cache);

        binding_add_to_free_list(fobp); 
    }
    else
    {
        if ((stream->flags & FD_WRITE) && --fobp->writers == 0)
        {
            /* only readers remain; switch back to stream-local caching */
            FOR_EACH_STREAM(FIRST, fobp, s)
                filestr_copy_cache(s, &fobp->cache);

            file_cache_free(&fobp->cache);
        }

        if (fobp->list.head == fobp->list.tail)
            fobp->flags |= FO_SINGLE; /* only one open stream remaining */
    }

    filestr_base_destroy(stream);
}

/* informs manager that file has been created */
void fileobj_fileop_create(struct filestr_base *stream,
                           const struct file_base_info *srcinfop,
                           unsigned int callflags)
{
    fileobj_fileop_open(stream, srcinfop, callflags);
    fileobj_sync_parent((const struct file_base_info *[]){ stream->infop }, 1);
}

/* informs manager that file has been removed */
void fileobj_fileop_remove(struct filestr_base *stream,
                           const struct file_base_info *oldinfop)
{
    ((struct fileobj_binding *)stream->bindp)->flags |= FO_REMOVED;
    fileobj_sync_parent((const struct file_base_info *[]){ oldinfop }, 1);
}

/* informs manager that file has been renamed */
void fileobj_fileop_rename(struct filestr_base *stream,
                           const struct file_base_info *oldinfop)
{
    /* if there is old info then this was a move and the old parent has to be
       informed */
    int count = oldinfop ? 2 : 1;
    fileobj_sync_parent(&(const struct file_base_info *[])
                            { oldinfop, stream->infop }[2 - count],
                        count);
}

/* informs manager than directory entries have been updated */
void fileobj_fileop_sync(struct filestr_base *stream)
{
    if (((struct fileobj_binding *)stream->bindp)->flags & FO_REMOVED)
        return; /* no dir to sync */

    fileobj_sync_parent((const struct file_base_info *[]){ stream->infop }, 1);
}

/* query for the pointer to the size storage for the file object */
file_size_t * fileobj_get_sizep(const struct filestr_base *stream)
{
    if (!stream->bindp)
        return NULL;

    return &((struct fileobj_binding *)stream->bindp)->size;
}

/* iterate the list of streams for this stream's file */
struct filestr_base * fileobj_get_next_stream(const struct filestr_base *stream,
                                              const struct filestr_base *s)
{
    if (!stream->bindp)
        return NULL;

    return s ? STREAM_NEXT(s) : STREAM_FIRST((struct fileobj_binding *)stream->bindp);
}

/* query manager bitflags for the file object */
unsigned int fileobj_get_flags(const struct filestr_base *stream)
{
    if (!stream->bindp)
        return 0;

    return ((struct fileobj_binding *)stream->bindp)->flags;
}

/* change manager bitflags for the file object (permitted only) */
void fileobj_change_flags(struct filestr_base *stream,
                          unsigned int flags, unsigned int mask)
{
    struct fileobj_binding *fobp = (struct fileobj_binding *)stream->bindp;
    if (!fobp)
        return;

    mask &= FDO_CHG_MASK; 
    fobp->flags = (fobp->flags & ~mask) | (flags & mask);
}

/* mark all open streams on a device as "nonexistant" */
void fileobj_mgr_unmount(IF_MV_NONVOID(int volume))
{
    FOR_EACH_VOLUME(volume, v)
    {
        struct fileobj_binding *fobp;
        while ((fobp = BINDING_FIRST(BUSY, v)))
        {
            struct filestr_base *s;
            while ((s = STREAM_FIRST(fobp)))
            {
                /* last ditch effort to preserve FS integrity; we could still
                   be alive (soft unmount, maybe); we get informed early  */
                fileop_onunmount_internal(s);

                if (STREAM_FIRST(fobp) == s)
                    fileop_onclose_internal(s); /* above didn't close it */

                /* keep it "busy" to avoid races; any valid file/directory
                   descriptor returned by an open call should always be
                   closed by whoever opened it (of course!) */
                s->flags = (s->flags & ~FDO_BUSY) | FD_NONEXIST;
            }
        }
    }
}

/* one-time init at startup */
void fileobj_mgr_init(void)
{
    for (unsigned int i = 0; i < NUM_VOLUMES; i++)
        ll_init(BUSY_BINDINGS(i));

    ll_init(FREE_BINDINGS());
    for (unsigned int i = 0; i < MAX_FILEOBJS; i++)
    {
        mutex_init(&stream_mutexes[i]);
        binding_add_to_free_list(&fobindings[i]);
    }
}
