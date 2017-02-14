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
#include <errno.h>
#include "system.h"
#include "debug.h"
#include "file_internal.h"
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
    struct fileinfo info;    /* basic file info (first!) */
    unsigned int refcount;   /* file reference count */
    struct ll_head openstrs; /* open streams for this file/dir */
} fobindings[MAX_FILEOBJS];
static struct mutex stream_mutexes[MAX_FILEOBJS] SHAREDBSS_ATTR;
static struct ll_head free_bindings;

#define BUSY_BINDINGS(_bpb) \
    ({ struct bpb *__bpb = (_bpb); \
       &__bpb->openfiles; })

#define BINDING_LIST(fip) \
    ({ struct fileinfo *__fip = (fip); \
       BUSY_BINDINGS(__fip->bpb); })

#define FREE_BINDINGS() \
    (&free_bindings)

#define BINDING_FIRST(type, bpb...) \
    ((struct fileobj_binding *)type##_BINDINGS(bpb)->head)

#define BINDING_NEXT(fobp) \
    ({ struct fileobj_binding *__fobp = (fobp); \
       (struct fileobj_binding *)__fobp->info.node.next; })

#define FOR_EACH_BINDING(bpb, fobp) \
    for (struct fileobj_binding *fobp = BINDING_FIRST(BUSY, bpb); \
         fobp; fobp = BINDING_NEXT(fobp))

#define STREAM_FIRST(fobp) \
    ({ struct fileobj_binding *__fobp = (fobp); \
       (struct filestr *)__fobp->openstrs.head; })

/* see if this file has open streams and return that fileobj_binding if so */
static struct fileobj_binding *
    binding_search(const struct fileinfo *srcinfop)
{
    FOR_EACH_BINDING(srcinfop->bpb, fobp)
    {
        if (compare_fileinfo_internal(srcinfop, &fobp->info) == CMPFI_EQ)
            return fobp;
    }

    return NULL;
}

/* assign a binding: existing one if file is already open */
static struct fileobj_binding *
    binding_assign(const struct fileinfo *srcinfop)
{
    struct fileobj_binding *fobp = binding_search(srcinfop);

    if (!fobp)
    {
        /* not found - allocate anew */
        fobp = BINDING_FIRST(FREE);
        ll_remove_first(FREE_BINDINGS());
        fileinfo_hdr_init(NULL, 0, &fobp->info);
        fobp->refcount = 0;
        ll_init(&fobp->openstrs);
    }

    return fobp;
}

/* mark descriptor as unused and return to the free list */
static void binding_add_to_free_list(struct fileinfo *infop)
{
    ll_insert_last(FREE_BINDINGS(), &infop->node);
}

/** File and directory internal interface **/

void file_binding_insert_last(struct fileinfo *infop)
{
    ll_insert_last(BINDING_LIST(infop), &infop->node);
}

void file_binding_remove(struct fileinfo *infop)
{
    ll_remove(BINDING_LIST(infop), &infop->node);
}

#ifdef HAVE_DIRCACHE
void file_binding_insert_first(struct fileinfo *infop)
{
    ll_insert_first(BINDING_LIST(infop), &infop->node);
}

void file_binding_remove_next(struct fileinfo *prevp,
                              struct fileinfo *infop)
{
    ll_remove_next(BINDING_LIST(infop), &prevp->node);
    (void)infop;
}
#endif /* HAVE_DIRCACHE */

/* opens common info for a file */
void fileobj_fileop_open(struct fileinfo *srcinfop,
                         unsigned int callflags,
                         struct fileinfo **infopp)
{
    struct fileobj_binding *fobp = binding_assign(srcinfop);

    struct fileinfo *infop = &fobp->info;
    *infopp = infop;

    if (++fobp->refcount > 1)
    {
        /* once a file/directory, always a file/directory; such a change
           is a bug */
        if ((callflags ^ infop->flags) & FO_DIRECTORY)
        {
            DEBUGF("%s - FO_DIRECTORY flag does not match: %p %X\n",
                   __func__, srcinfop, callflags);
        }

        return;
    }

    /* cache information on first reference */
    infop->flags = FDO_BUSY | (callflags & (FO_OPEN_MASK|FF_INFO_MASK));
    fileinfo_copy_fsinfo(infop, srcinfop);
    fileobj_bind_file(infop);
}

/* closes common info for a file */
void fileobj_fileop_close(struct fileinfo *infop)
{
    if (!(infop->flags & FDO_BUSY))
    {
        fileinfo_hdr_destroy(infop);
        return;
    }

    struct fileobj_binding *fobp = (struct fileobj_binding *)infop;
    if (--fobp->refcount == 0)
    {
       /* close everything */
        fileobj_unbind_file(infop);
        binding_add_to_free_list(infop);
    }
}

unsigned int fileobj_get_refcount(struct fileinfo *infop)
{
    if (!infop->flags)
        return 0;

    return ((struct fileobj_binding *)infop)->refcount;
}

/* opens the stream for use with read and write APIs
 * str->infop must be one allocated by fileobj_fileop_open
 */
void fileobj_fileop_open_filestr(unsigned int callflags,
                                 struct filestr *str)
{
    struct fileobj_binding *fobp = (struct fileobj_binding *)str->infop;

    /* initiate the new stream into the enclave */
    str->flags = FDO_BUSY |
                 (callflags & (FD_OPEN_MASK|FF_STR_MASK));
    str->mtx   = &stream_mutexes[fobp - fobindings];

    /* add stream to this file's list */
    ll_insert_last(&fobp->openstrs, &str->node);
    fobp->refcount++;
}

/* close the stream and free associated resources */
void fileobj_fileop_close_filestr(struct filestr *str)
{
    if (!(str->flags & FDO_BUSY))
    {
        /* most likely force closed by unmount */
        filestr_hdr_destroy(str);
        return;
    }

    struct fileobj_binding *fobp = (struct fileobj_binding *)str->infop;
    ll_remove(&fobp->openstrs, &str->node);
    filestr_hdr_destroy(str);
    fobp->refcount--;
}

/* mark all open streams on a device as "nonexistant" */
void fileobj_mgr_unmount(IF_MV_NONVOID(int volume))
{
    FOR_EACH_VOLUME(volume, v)
    {
        struct bpb *bpb = get_vol_bpb(IF_MV_VOL(v));
        if (!(bpb && bpb->mounted))
            continue;

        struct fileobj_binding *fobp;
        while ((fobp = BINDING_FIRST(BUSY, bpb)))
        {
            struct filestr *s;
            while ((s = STREAM_FIRST(fobp)))
            {
                /* last ditch effort to preserve FS integrity; we could still
                   be alive (soft unmount, maybe); we get informed early  */
                close_filestr_internal(s);
                /* keep it "busy" to avoid races; any valid file/directory
                   descriptor returned by an open call should always be
                   closed by whoever opened it (of course!) */
                s->flags = (s->flags & ~FDO_BUSY) | FD_NONEXIST;
            }

            close_file_internal(&fobp->info);
        }
    }
}

/* one-time init at startup */
void fileobj_mgr_init(void)
{
    ll_init(FREE_BINDINGS());
    for (unsigned int i = 0; i < MAX_FILEOBJS; i++)
    {
        mutex_init(&stream_mutexes[i]);
        binding_add_to_free_list(&fobindings[i].info);
    }
}
