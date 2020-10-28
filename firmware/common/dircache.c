/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
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
#include <stdio.h>
#include <errno.h>
#include "string-extra.h"
#include <stdbool.h>
#include <stdlib.h>
#include "debug.h"
#include "system.h"
#include "logf.h"
#include "fileobj_mgr.h"
#include "pathfuncs.h"
#include "dircache.h"
#include "thread.h"
#include "kernel.h"
#include "usb.h"
#include "file.h"
#include "core_alloc.h"
#include "dir.h"
#include "storage.h"
#include "audio.h"
#include "rbpaths.h"
#include "linked_list.h"
#include "crc32.h"

/**
 * Cache memory layout:
 * x - array of struct dircache_entry
 * r - reserved buffer
 * d - name buffer for the name entry of the struct dircache_entry
 * 0 - zero bytes to assist free name block sentinel scanning (not 0xfe or 0xff)
 * |xxxxxx|rrrrrrrrr|0|dddddd|0|
 *
 * Subsequent x are allocated from the front, d are allocated from the back,
 * using the reserve buffer for entries added after initial scan.
 *
 * After a while the cache may look like:
 * |xxxxxxxx|rrrrr|0|dddddddd|0|
 *
 * After a reboot, the reserve buffer is restored in it's size, so that the
 * total allocation size grows:
 * |xxxxxxxx|rrrrrrrrr|0|dddddddd|0|
 *
 *
 * Cache structure:
 * Format is memory position independent and uses only indexes as links. The
 * buffer pointers are offset back by one entry to make the array 1-based so
 * that an index of 0 may be considered an analog of a NULL pointer.
 *
 * Entry elements are linked together analagously to the filesystem directory
 * structure with minor variations that are helpful to the cache's algorithms.
 * Each volume has a special root structure in the dircache structure, not an
 * entry in the cache, comprising a forest of volume trees which facilitates
 * mounting or dismounting of specified volumes on the fly.
 *
 * Indexes identifying a volume are computed as: index = -volume - 1
 * Returning the volume from these indexes is thus: volume = -index - 1
 * Such indexes are used in root binding and as the 'up' index for an entry
 * who's parent is the root directory.
 *
 * Open files list:
 * When dircache is made it is the maintainer of the main volume open files
 * lists, even when it is off. Any files open before dircache is enabled or
 * initialized must be bound to cache entries by the scan and build operation.
 * It maintains these lists in a special way.
 *
 * Queued (unresolved) bindings are at the back and resolved at the front.
 * A pointer to the first of each kind of binding is provided to skip to help
 * iterating one sublist or another.
 *
 * r0->r1->r2->q0->q1->q2->NULL
 * ^resolved0  ^queued0
 */

#ifdef DIRCACHE_NATIVE
#define dircache_lock()   file_internal_lock_WRITER()
#define dircache_unlock() file_internal_unlock_WRITER()

/* scan and build parameter data */
struct sab_component;
struct sab
{
    struct filestr_base   stream;    /* scan directory stream */
    struct file_base_info info;      /* scanned entry info */
    bool volatile         quit;      /* halt all scanning */
    struct sab_component  *stackend; /* end of stack pointer */
    struct sab_component  *top;      /* current top of stack */
    struct sab_component
    {
        int volatile      idx;       /* cache index of directory */
        int               *downp;    /* pointer to ce->down */
        int *volatile     prevp;     /* previous item accessed */
    } stack[];                       /* "recursion" stack */
};

#else /* !DIRCACHE_NATIVE */

#error need locking scheme
#define FILESYS_WRITER_LOCK()
#define FILESYS_WRITER_UNLOCK()

struct sab_component
{
};

struct sab
{
#ifdef HAVE_MUTLIVOLUME
    int volume;
#endif /* HAVE_MULTIVOLUME */
    char path[MAX_PATH];
    unsigned int append;
};
#endif /* DIRCACHE_NATIVE */

enum
{
    FRONTIER_SETTLED = 0x0, /* dir entry contents are complete */
    FRONTIER_NEW     = 0x1, /* dir entry contents are in-progress */
    FRONTIER_ZONED   = 0x2, /* frontier entry permanent mark (very sticky!) */
    FRONTIER_RENEW   = 0x4, /* override FRONTIER_ZONED sticky (not stored) */
};

enum
{
    DCM_BUILD,              /* build a volume */
    DCM_PROCEED,            /* merged DCM_BUILD messages */
    DCM_FIRST = DCM_BUILD,
    DCM_LAST  = DCM_PROCEED,
};

#define MAX_TINYNAME      sizeof (uint32_t)
#define NAMELEN_ADJ       (MAX_TINYNAME+1)
#define DC_MAX_NAME       (UINT8_MAX+NAMELEN_ADJ)
#define CE_NAMESIZE(len)  ((len)+NAMELEN_ADJ)
#define NAMESIZE_CE(size) ((size)-NAMELEN_ADJ)

/* Throw some warnings if about the limits if things may not work */
#if MAX_COMPNAME > UINT8_MAX+5
#warning Need more than 8 bits in name length bitfield
#endif

#if DIRCACHE_LIMIT > ((1 << 24)-1+5)
#warning Names may not be addressable with 24 bits
#endif

/* data structure used by cache entries */
struct dircache_entry
{
    int         next;              /* next at same level */
    union {
    int         down;              /* first at child level (if directory) */
    file_size_t filesize;          /* size of file in bytes (if file) */
    };
    int         up;                /* parent index (-volume-1 if root) */
    union {
    struct {
    uint32_t    name         : 24; /* indirect storage (.tinyname == 0) */
    uint32_t    namelen      :  8; /* length of name - (MAX_TINYNAME+1) */
    };
    unsigned char namebuf[MAX_TINYNAME]; /* direct storage (.tinyname == 1) */
    };
    uint32_t    direntry     : 16; /* entry # in parent - max 0xffff */
    uint32_t    direntries   :  5; /* # of entries used - max 21 */
    uint32_t    tinyname     :  1; /* if == 1, name fits in .namebuf */
    uint32_t    frontier     :  2; /* (FRONTIER_* bitflags) */
    uint32_t    attr         :  8; /* entry file attributes */
#ifdef DIRCACHE_NATIVE
    long        firstcluster;      /* first file cluster - max 0x0ffffff4 */
    uint16_t    wrtdate;           /* FAT write date */
    uint16_t    wrttime;           /* FAT write time */
#else
    time_t      mtime;             /* file last-modified time */
#endif
    dc_serial_t serialnum;         /* entry serial number */
};

/* spare us some tedium */
#define ENTRYSIZE   (sizeof (struct dircache_entry))

/* thread and kernel stuff */
static struct event_queue dircache_queue SHAREDBSS_ATTR;
static uintptr_t dircache_stack[DIRCACHE_STACK_SIZE / sizeof (uintptr_t)];
static const char dircache_thread_name[] = "dircache";

/* struct that is both used during run time and for persistent storage */
static struct dircache
{
    /* cache-wide data */
    int          free_list;           /* first index of free entry list */
    size_t       size;                /* total size of data (including holes) */
    size_t       sizeused;            /* bytes of .size bytes actually used */
    union {
    unsigned int numentries;          /* entry count (including holes) */
#ifdef HAVE_EEPROM_SETTINGS
    size_t       sizeentries;         /* used when persisting */
#endif
    };
    int          names;               /* index of first name in name block */
    size_t       sizenames;           /* size of all names (including holes) */
    size_t       namesfree;           /* amount of wasted name space */
    int          nextnamefree;        /* hint of next free name in buffer */
    /* per-volume data */
    struct dircache_volume            /* per volume cache data */
    {
        uint32_t       status   : 2;  /* cache status of this volume */
        uint32_t       frontier : 2;  /* (FRONTIER_* bitflags) */
        dc_serial_t    serialnum;     /* dircache serial number of root */
        int            root_down;     /* index of first entry of volume root */
        union {
        long           start_tick;    /* when did scan start (scanning) */
        long           build_ticks;   /* how long to build volume? (ready) */
        };
    } dcvol[NUM_VOLUMES];
    /* these remain unchanged between cache resets */
    size_t       last_size;           /* last reported size at boot */
    size_t       reserve_used;        /* reserved used at last build */
    dc_serial_t  last_serialnum;      /* last serialnumber generated */
} dircache;

/* struct that is used only for the cache in main memory */
static struct dircache_runinfo
{
    /* cache setting and build info */
    int          suspended;        /* dircache suspend count */
    bool         enabled;          /* dircache master enable switch */
    unsigned int thread_id;        /* current/last thread id */
    bool         thread_done;      /* thread has exited */
    /* cache buffer info */
    int          handle;           /* buflib buffer handle */
    size_t       bufsize;          /* size of buflib allocation - 1 */
    int          buflocked;        /* don't move due to other allocs */
    union {
    void                  *p;      /* address of buffer - ENTRYSIZE */
    struct dircache_entry *pentry; /* alias of .p to assist entry resolution */
    unsigned char         *pname;  /* alias of .p to assist name resolution */
    };
    struct buflib_callbacks ops;   /* buflib ops callbacks */
    /* per-volume data */
    struct dircache_runinfo_volume
    {
        struct file_base_binding *resolved0; /* first resolved binding in list */
        struct file_base_binding *queued0;   /* first queued binding in list */
        struct sab               *sabp;      /* if building, struct sab in use */
    } dcrivol[NUM_VOLUMES];
} dircache_runinfo;

#define BINDING_NEXT(bindp) \
    ((struct file_base_binding *)(bindp)->node.next)

#define FOR_EACH_BINDING(start, p) \
    for (struct file_base_binding *p = (start); p; p = BINDING_NEXT(p))

#define FOR_EACH_CACHE_ENTRY(ce) \
    for (struct dircache_entry *ce = &dircache_runinfo.pentry[1],  \
                               *_ceend = ce + dircache.numentries; \
         ce < _ceend; ce++) if (ce->serialnum)

#define FOR_EACH_SAB_COMP(sabp, p) \
    for (struct sab_component *p = (sabp)->top; p < (sabp)->stackend; p++)

/* "overloaded" macros to get volume structures */
#define DCVOL_i(i)               (&dircache.dcvol[i])
#define DCVOL_volume(volume)     (&dircache.dcvol[volume])
#define DCVOL_infop(infop)       (&dircache.dcvol[BASEINFO_VOL(infop)])
#define DCVOL_dirinfop(dirinfop) (&dircache.dcvol[BASEINFO_VOL(dirinfop)])
#define DCVOL(x)                 DCVOL_##x(x)

#define DCRIVOL_i(i)             (&dircache_runinfo.dcrivol[i])
#define DCRIVOL_infop(infop)     (&dircache_runinfo.dcrivol[BASEINFO_VOL(infop)])
#define DCRIVOL_bindp(bindp)     (&dircache_runinfo.dcrivol[BASEBINDING_VOL(bindp)])
#define DCRIVOL(x)               DCRIVOL_##x(x)

/* reserve over 75% full? */
#define DIRCACHE_STUFFED(reserve_used) \
    ((reserve_used) > 3*DIRCACHE_RESERVE / 4)

#ifdef HAVE_EEPROM_SETTINGS
/**
 * remove the snapshot file
 */
static int remove_dircache_file(void)
{
    return remove(DIRCACHE_FILE);
}

/**
 * open or create the snapshot file
 */
static int open_dircache_file(int oflag)
{
    return open(DIRCACHE_FILE, oflag, 0666);
}
#endif /* HAVE_EEPROM_SETTINGS */

#ifdef DIRCACHE_DUMPSTER
/**
 * clean up the memory allocation to make viewing in a hex editor more friendly
 * and highlight problems
 */
static inline void dumpster_clean_buffer(void *p, size_t size)
{
    memset(p, 0xAA, size);
}
#endif /* DIRCACHE_DUMPSTER */

/**
 * relocate the cache when the buffer has moved
 */
static int move_callback(int handle, void *current, void *new)
{
    if (dircache_runinfo.buflocked)
        return BUFLIB_CB_CANNOT_MOVE;

    dircache_runinfo.p = new - ENTRYSIZE;

    return BUFLIB_CB_OK;
    (void)handle; (void)current;
}

/**
 * add a "don't move" lock count
 */
static inline void buffer_lock(void)
{
    dircache_runinfo.buflocked++;
}

/**
 * remove a "don't move" lock count
 */
static inline void buffer_unlock(void)
{
    dircache_runinfo.buflocked--;
}


/** Open file bindings management **/

/* compare the basic file information and return 'true' if they are logically
   equivalent or the same item, else return 'false' if not */
static inline bool binding_compare(const struct file_base_info *infop1,
                                   const struct file_base_info *infop2)
{
#ifdef DIRCACHE_NATIVE
    return fat_file_is_same(&infop1->fatfile, &infop2->fatfile);
#else
    #error hey watch it!
#endif
}

/**
 * bind a file to the cache; "queued" or "resolved" depending upon whether or
 * not it has entry information
 */
static void binding_open(struct file_base_binding *bindp)
{
    struct dircache_runinfo_volume *dcrivolp = DCRIVOL(bindp);
    if (bindp->info.dcfile.serialnum)
    {
        /* already resolved */
        dcrivolp->resolved0 = bindp;
        file_binding_insert_first(bindp);
    }
    else
    {
        if (dcrivolp->queued0 == NULL)
            dcrivolp->queued0 = bindp;

        file_binding_insert_last(bindp);
    }
}

/**
 * remove a binding from the cache
 */
static void binding_close(struct file_base_binding *bindp)
{
    struct dircache_runinfo_volume *dcrivolp = DCRIVOL(bindp);

    if (bindp == dcrivolp->queued0)
        dcrivolp->queued0 = BINDING_NEXT(bindp);
    else if (bindp == dcrivolp->resolved0)
    {
        struct file_base_binding *nextp = BINDING_NEXT(bindp);
        dcrivolp->resolved0 = (nextp == dcrivolp->queued0) ? NULL : nextp;
    }

    file_binding_remove(bindp);
    /* no need to reset it */
}

/**
 * resolve a queued binding with the information from the given source file
 */
static void binding_resolve(const struct file_base_info *infop)
{
    struct dircache_runinfo_volume *dcrivolp = DCRIVOL(infop);

    /* quickly check the queued list to see if it's there */
    struct file_base_binding *prevp = NULL;
    FOR_EACH_BINDING(dcrivolp->queued0, p)
    {
        if (!binding_compare(infop, &p->info))
        {
            prevp = p;
            continue;
        }

        if (p == dcrivolp->queued0)
        {
            dcrivolp->queued0 = BINDING_NEXT(p);
            if (dcrivolp->resolved0 == NULL)
                dcrivolp->resolved0 = p;
        }
        else
        {
            file_binding_remove_next(prevp, p);
            file_binding_insert_first(p);
            dcrivolp->resolved0 = p;
        }

        /* srcinfop may be the actual one */
        if (&p->info != infop)
            p->info.dcfile = infop->dcfile;

        break;
    }
}

/**
 * dissolve a resolved binding on its volume
 */
static void binding_dissolve(struct file_base_binding *prevp,
                             struct file_base_binding *bindp)
{
    struct dircache_runinfo_volume *dcrivolp = DCRIVOL(bindp);

    if (bindp == dcrivolp->resolved0)
    {
        struct file_base_binding *nextp = BINDING_NEXT(bindp);
        dcrivolp->resolved0 = (nextp == dcrivolp->queued0) ? NULL : nextp;
    }

    if (dcrivolp->queued0 == NULL)
        dcrivolp->queued0 = bindp;

    file_binding_remove_next(prevp, bindp);
    file_binding_insert_last(bindp);

    dircache_dcfile_init(&bindp->info.dcfile);
}

/**
 * dissolve all resolved bindings on a given volume
 */
static void binding_dissolve_volume(struct dircache_runinfo_volume *dcrivolp)
{
    if (!dcrivolp->resolved0)
        return;

    FOR_EACH_BINDING(dcrivolp->resolved0, p)
    {
        if (p == dcrivolp->queued0)
            break;

        dircache_dcfile_init(&p->info.dcfile);
    }

    dcrivolp->queued0 = dcrivolp->resolved0;
    dcrivolp->resolved0 = NULL;
}


/** Dircache buffer management **/

/**
 * allocate the cache's memory block
 */
static int alloc_cache(size_t size)
{
    /* pad with one extra-- see alloc_name() and free_name() */
    return core_alloc_ex("dircache", size + 1, &dircache_runinfo.ops);
}

/**
 * put the allocation in dircache control
 */
static void set_buffer(int handle, size_t size)
{
    void *p = core_get_data(handle);

#ifdef DIRCACHE_DUMPSTER
    dumpster_clean_buffer(p, size);
#endif /* DIRCACHE_DUMPSTER */

    /* set it up as a 1-based array */
    dircache_runinfo.p = p - ENTRYSIZE;

    if (dircache_runinfo.handle != handle)
    {
        /* new buffer */
        dircache_runinfo.handle = handle;
        dircache_runinfo.bufsize = size;
        dircache.names = size + ENTRYSIZE;
        dircache_runinfo.pname[dircache.names - 1] = 0;
        dircache_runinfo.pname[dircache.names    ] = 0;
    }
}

/**
 * remove the allocation from dircache control and return the handle
 */
static int reset_buffer(void)
{
    int handle = dircache_runinfo.handle;
    if (handle > 0)
    {
        /* don't mind .p; it might get changed by the callback even after
           this call; buffer presence is determined by the following: */
        dircache_runinfo.handle  = 0;
        dircache_runinfo.bufsize = 0;
    }

    return handle;
}

/**
 * return the number of bytes remaining in the buffer
 */
static size_t dircache_buf_remaining(void)
{
    if (!dircache_runinfo.handle)
        return 0;

    return dircache_runinfo.bufsize - dircache.size;
}

/**
 * return the amount of reserve space used
 */
static size_t reserve_buf_used(void)
{
    size_t remaining = dircache_buf_remaining();
    return (remaining < DIRCACHE_RESERVE) ?
                DIRCACHE_RESERVE - remaining : 0;
}


/** Internal cache structure control functions **/

/**
 * generate the next serial number in the sequence
 */
static dc_serial_t next_serialnum(void)
{
    dc_serial_t serialnum = MAX(dircache.last_serialnum + 1, 1);
    dircache.last_serialnum = serialnum;
    return serialnum;
}

/**
 * return the dircache volume pointer for the special index
 */
static struct dircache_volume * get_idx_dcvolp(int idx)
{
    if (idx >= 0)
        return NULL;

    return &dircache.dcvol[IF_MV_VOL(-idx - 1)];
}

/**
 * return the cache entry referenced by idx (NULL if outside buffer)
 */
static struct dircache_entry * get_entry(int idx)
{
    if (idx <= 0 || (unsigned int)idx > dircache.numentries)
        return NULL;

    return &dircache_runinfo.pentry[idx];
}

/**
 * return the index of the cache entry (0 if outside buffer)
 */
static int get_index(const struct dircache_entry *ce)
{
    if (!PTR_IN_ARRAY(dircache_runinfo.pentry + 1, ce,
                      dircache.numentries + 1))
    {
        return 0;
    }

    return ce - dircache_runinfo.pentry;
}

/**
 * return the frontier flags for the index
 */
static uint32_t get_frontier(int idx)
{
    if (idx == 0)
        return UINT32_MAX;
    else if (idx > 0)
        return get_entry(idx)->frontier;
    else /* idx < 0 */
        return get_idx_dcvolp(idx)->frontier;
}

/**
 *  return the sublist down pointer for the sublist that contains entry 'idx'
 */
static int * get_downidxp(int idx)
{
    /* NOTE: 'idx' must refer to a directory or the result is undefined */
    if (idx == 0 || idx < -NUM_VOLUMES)
        return NULL;

    if (idx > 0)
    {
        /* a normal entry */
        struct dircache_entry *ce = get_entry(idx);
        return ce ? &ce->down : NULL;
    }
    else
    {
        /* a volume root */
        return &get_idx_dcvolp(idx)->root_down;
    }
}

/**
 * return a pointer to the index referencing the cache entry that 'idx'
 * references
 */
static int * get_previdxp(int idx)
{
    struct dircache_entry *ce = get_entry(idx);

    int *prevp = get_downidxp(ce->up);
    if (!prevp)
        return NULL;

    while (1)
    {
        int next = *prevp;
        if (!next || next == idx)
            break;

        prevp = &get_entry(next)->next;
    }

    return prevp;
}

/**
 * if required, adjust the lists and directory read of any scan and build in
 * progress
 */
static void sab_sync_scan(struct sab *sabp, int *prevp, int *nextp)
{
    struct sab_component *abovep = NULL;
    FOR_EACH_SAB_COMP(sabp, p)
    {
        if (nextp != p->prevp)
        {
            abovep = p;
            continue;
        }

        /* removing an item being scanned; set the component position to the
           entry before this */
        p->prevp = prevp;

        if (p == sabp->top)
        {
            /* removed at item in the directory who's immediate contents are
               being scanned */
            if (prevp == p->downp)
            {
                /* was first item; rewind it */
                dircache_rewinddir_internal(&sabp->info);
            }
            else
            {
                struct dircache_entry *ceprev =
                    container_of(prevp, struct dircache_entry, next);
            #ifdef DIRCACHE_NATIVE
                sabp->info.fatfile.e.entry   = ceprev->direntry;
                sabp->info.fatfile.e.entries = ceprev->direntries;
            #endif
            }
        }
        else if (abovep)
        {
            /* the directory being scanned or a parent of it has been removed;
               abort its build or cache traversal */
            abovep->idx = 0;
        }

        break;
    }
}

/**
 * get a pointer to an allocated name given a cache index
 */
static inline unsigned char * get_name(int nameidx)
{
    return &dircache_runinfo.pname[nameidx];
}

/**
 * get the cache buffer index of the given name
 */
static inline int get_nameidx(const unsigned char *pname)
{
    return pname - dircache_runinfo.pname;
}

/**
 * copy the entry's name to a buffer (which assumed to be of sufficient size)
 */
static void entry_name_copy(char *dst, const struct dircache_entry *ce)
{
    if (LIKELY(!ce->tinyname))
    {
        strmemcpy(dst, get_name(ce->name), CE_NAMESIZE(ce->namelen));
        return;
    }

    const unsigned char *src = ce->namebuf;
    size_t len = 0;
    while (len++ < MAX_TINYNAME && *src)
        *dst++ = *src++;

    *dst = '\0';
}

/**
 * set the namesfree hint to a new position
 */
static void set_namesfree_hint(const unsigned char *hintp)
{
    int hintidx = get_nameidx(hintp);

    if (hintidx >= (int)(dircache.names + dircache.sizenames))
        hintidx = dircache.names;

    dircache.nextnamefree = hintidx;
}

/**
 * allocate a buffer to use for a new name
 */
static int alloc_name(size_t size)
{
    int nameidx = 0;

    if (dircache.namesfree >= size)
    {
        /* scan for a free gap starting at the hint point - first fit */
        unsigned char * const start = get_name(dircache.nextnamefree);
        unsigned char * const bufend = get_name(dircache.names + dircache.sizenames);
        unsigned char *p = start;
        unsigned char *end = bufend;

        while (1)
        {
            if ((size_t)(bufend - p) >= size && (p = memchr(p, 0xff, end - p)))
            {
                /* found a sentinel; see if there are enough in a row */
                unsigned char *q = p + size - 1;

                /* check end byte and every MAX_TINYNAME+1 bytes from the end;
                   the minimum-length indirectly allocated string that could be
                   in between must have at least one character at one of those
                   locations */
                while (q > p && *q == 0xff)
                    q -= MAX_TINYNAME+1;

                if (q <= p)
                {
                    nameidx = get_nameidx(p);
                    break;
                }

                p += size;
            }
            else
            {
                if (end == start)
                    break; /* exhausted */

                /* wrap */
                end = start;
                p = get_name(dircache.names);

                if (p == end)
                    break; /* initial hint was at names start */
            }
        }

        if (nameidx)
        {
            unsigned char *q = p + size;
            if (q[0] == 0xff && q[MAX_TINYNAME] != 0xff)
            {
                /* if only a tiny block remains after buffer, claim it and
                   hide it from scans since it's too small for indirect
                   allocation */
                do
                {
                    *q = 0xfe;
                    size++;
                }
                while (*++q == 0xff);
            }

            dircache.namesfree -= size;
            dircache.sizeused += size;
            set_namesfree_hint(p + size);
        }
    }

    if (!nameidx)
    {
        /* no sufficiently long free gaps; allocate anew */
        if (dircache_buf_remaining() <= size)
        {
            dircache.last_size = 0;
            return 0;
        }

        dircache.names     -= size;
        dircache.sizenames += size;
        nameidx             = dircache.names;
        dircache.size      += size;
        dircache.sizeused  += size;
        *get_name(dircache.names - 1) = 0;
    }

    return nameidx;
}

/**
 * mark a name as free and note that its bytes are available
 */
static void free_name(int nameidx, size_t size)
{
    unsigned char *beg = get_name(nameidx);
    unsigned char *end = beg + size;

    /* merge with any adjacent tiny blocks */
    while (beg[-1] == 0xfe)
        --beg;

    while (end[0] == 0xfe)
        ++end;

    size = end - beg;
    memset(beg, 0xff, size);
    dircache.namesfree += size;
    dircache.sizeused -= size;
    set_namesfree_hint(beg);
}

/**
 * allocate and assign a name to the entry
 */
static int entry_assign_name(struct dircache_entry *ce,
                             const unsigned char *name, size_t size)
{
    unsigned char *copyto;

    if (size <= MAX_TINYNAME)
    {
        copyto = ce->namebuf;

        if (size < MAX_TINYNAME)
            copyto[size] = '\0';

        ce->tinyname = 1;
    }
    else
    {
        if (size > DC_MAX_NAME)
            return -ENAMETOOLONG;

        int nameidx = alloc_name(size);
        if (!nameidx)
            return -ENOSPC;

        copyto = get_name(nameidx);

        ce->tinyname = 0;
        ce->name     = nameidx;
        ce->namelen  = NAMESIZE_CE(size);
    }

    memcpy(copyto, name, size);
    return 0;
}

/**
 * free the name for the entry
 */
static void entry_unassign_name(struct dircache_entry *ce)
{
    if (ce->tinyname)
        return;

    free_name(ce->name, CE_NAMESIZE(ce->namelen));
    ce->tinyname = 1;
}

/**
 * assign a new name to the entry
 */
static int entry_reassign_name(struct dircache_entry *ce,
                               const unsigned char *newname)
{
    size_t oldlen = ce->tinyname ? 0 : CE_NAMESIZE(ce->namelen);
    size_t newlen = strlen(newname);

    if (oldlen == newlen || (oldlen == 0 && newlen <= MAX_TINYNAME))
    {
        char *p = mempcpy(oldlen == 0 ? ce->namebuf : get_name(ce->name),
                          newname, newlen);
        if (newlen < MAX_TINYNAME)
            *p = '\0';
        return 0;
    }

    /* needs a new name allocation; if the new name fits in the freed block,
       it will use it immediately without a lengthy search */
    entry_unassign_name(ce);
    return entry_assign_name(ce, newname, newlen);
}

/**
 * allocate a dircache_entry from memory using freed ones if available
 */
static int alloc_entry(struct dircache_entry **res)
{
    struct dircache_entry *ce;
    int idx = dircache.free_list;

    if (idx)
    {
        /* reuse a freed entry */
        ce = get_entry(idx);
        dircache.free_list = ce->next;
    }
    else if (dircache_buf_remaining() > ENTRYSIZE)
    {
        /* allocate a new one */
        idx = ++dircache.numentries;
        dircache.size += ENTRYSIZE;
        ce = get_entry(idx);
    }
    else
    {
        dircache.last_size = 0;
        *res = NULL;
        return -ENOSPC;
    }

    dircache.sizeused += ENTRYSIZE;

    ce->next      = 0;
    ce->up        = 0;
    ce->down      = 0;
    ce->tinyname  = 1;
    ce->frontier  = FRONTIER_SETTLED;
    ce->serialnum = next_serialnum();

    *res = ce;
    return idx;
}

/**
 * free an entry's allocations in the cache; must not be linked to anything
 * by this time (orphan!)
 */
static void free_orphan_entry(struct dircache_runinfo_volume *dcrivolp,
                              struct dircache_entry *ce, int idx)
{
    if (dcrivolp)
    {
        /* was an established entry; find any associated resolved binding and
           dissolve it;  bindings are kept strictly synchronized with changes
           to the storage so a simple serial number comparison is sufficient */
        struct file_base_binding *prevp = NULL;
        FOR_EACH_BINDING(dcrivolp->resolved0, p)
        {
            if (p == dcrivolp->queued0)
                break;

            if (ce->serialnum == p->info.dcfile.serialnum)
            {
                binding_dissolve(prevp, p);
                break;
            }

            prevp = p;
        }
    }

    entry_unassign_name(ce);

    /* no serialnum says "it's free" (for cache-wide iterators) */
    ce->serialnum = 0;

    /* add to free list */
    ce->next = dircache.free_list;
    dircache.free_list = idx;
    dircache.sizeused -= ENTRYSIZE;
}

/**
 * allocates a new entry of with the name specified by 'basename'
 */
static int create_entry(const char *basename,
                        struct dircache_entry **res)
{
    int idx = alloc_entry(res);

    if (idx > 0)
    {
        int rc = entry_assign_name(*res, basename, strlen(basename));
        if (rc < 0)
        {
            free_orphan_entry(NULL, *res, idx);
            idx = rc;
        }
    }

    if (idx == -ENOSPC)
        logf("size limit reached");

    return idx;
}

/**
 * unlink the entry at *prevp and adjust the scanner if needed
 */
static void remove_entry(struct dircache_runinfo_volume *dcrivolp,
                         struct dircache_entry *ce, int *prevp)
{
    /* unlink it from its list */
    *prevp = ce->next;

    if (dcrivolp)
    {
        /* adjust scanner iterator if needed */
        struct sab *sabp = dcrivolp->sabp;
        if (sabp)
            sab_sync_scan(sabp, prevp, &ce->next);
    }
}

/**
 * free the entire subtree in the referenced parent down index
 */
static void free_subentries(struct dircache_runinfo_volume *dcrivolp, int *downp)
{
    while (1)
    {
        int idx = *downp;
        struct dircache_entry *ce = get_entry(idx);
        if (!ce)
            break;

        if ((ce->attr & ATTR_DIRECTORY) && ce->down)
            free_subentries(dcrivolp, &ce->down);

        remove_entry(dcrivolp, ce, downp);
        free_orphan_entry(dcrivolp, ce, idx);
    }
}

/**
 * free the specified file entry and its children
 */
static void free_file_entry(struct file_base_info *infop)
{
    int idx = infop->dcfile.idx;
    if (idx <= 0)
        return; /* can't remove a root/invalid */

    struct dircache_runinfo_volume *dcrivolp = DCRIVOL(infop);

    struct dircache_entry *ce = get_entry(idx);
    if ((ce->attr & ATTR_DIRECTORY) && ce->down)
    {
        /* gonna get all this contents (normally the "." and "..") */
        free_subentries(dcrivolp, &ce->down);
    }

    remove_entry(dcrivolp, ce, get_previdxp(idx));
    free_orphan_entry(dcrivolp, ce, idx);
}

/**
 * insert the new entry into the parent, sorted into position
 */
static void insert_file_entry(struct file_base_info *dirinfop,
                              struct dircache_entry *ce)
{
    /* DIRCACHE_NATIVE: the entires are sorted into the spot it would be on
     * the storage medium based upon the directory entry number, in-progress
     * scans will catch it or miss it in just the same way they would if
     * directly scanning the disk. If this is behind an init scan, it gets
     * added anyway; if in front of it, then scanning will compare what it
     * finds in order to avoid adding a duplicate.
     *
     * All others: the order of entries of the host filesystem is not known so
     * this must be placed at the end so that a build scan won't miss it and
     * add a duplicate since it will be comparing any entries it finds in front
     * of it.
     */
    int diridx = dirinfop->dcfile.idx;
    int *nextp = get_downidxp(diridx);

    while (8675309)
    {
        struct dircache_entry *nextce = get_entry(*nextp);
        if (!nextce)
            break;

#ifdef DIRCACHE_NATIVE
        if (nextce->direntry > ce->direntry)
            break;

        /* now, nothing should be equal to ours or that is a bug since it
           would already exist (and it shouldn't because it's just been
           created or moved) */
#endif /* DIRCACHE_NATIVE */

        nextp = &nextce->next;
    }

    ce->up   = diridx;
    ce->next = *nextp;
    *nextp   = get_index(ce);
}

/**
 * unlink the entry from its parent and return its pointer to the caller
 */
static struct dircache_entry * remove_file_entry(struct file_base_info *infop)
{
    struct dircache_runinfo_volume *dcrivolp = DCRIVOL(infop);
    struct dircache_entry *ce = get_entry(infop->dcfile.idx);
    remove_entry(dcrivolp, ce, get_previdxp(infop->dcfile.idx));
    return ce;
}

/**
 * set the frontier indicator for the given cache index
 */
static void establish_frontier(int idx, uint32_t code)
{
    if (idx < 0)
    {
        int volume = IF_MV_VOL(-idx - 1);
        uint32_t val = dircache.dcvol[volume].frontier;
        if (code & FRONTIER_RENEW)
            val &= ~FRONTIER_ZONED;
        dircache.dcvol[volume].frontier = code | (val & FRONTIER_ZONED);
    }
    else if (idx > 0)
    {
        struct dircache_entry *ce = get_entry(idx);
        uint32_t val = ce->frontier;
        if (code & FRONTIER_RENEW)
            val &= ~FRONTIER_ZONED;
        ce->frontier = code | (val & FRONTIER_ZONED);
    }
}

/**
 * remove all messages from the queue, responding to anyone waiting
 */
static void clear_dircache_queue(void)
{
    struct queue_event ev;

    while (1)
    {
        queue_wait_w_tmo(&dircache_queue, &ev, 0);
        if (ev.id == SYS_TIMEOUT)
            break;

        /* respond to any synchronous build queries; since we're already
           building and thusly allocated, any additional requests can be
           processed async */
        if (ev.id == DCM_BUILD)
        {
            int *rcp = (int *)ev.data;
            if (rcp)
                *rcp = 0;
        }
    }
}

/**
 * service dircache_queue during a scan and build
 */
static void process_events(void)
{
    yield();

    /* only count externally generated commands */
    if (!queue_peek_ex(&dircache_queue, NULL, 0, QPEEK_FILTER1(DCM_BUILD)))
        return;

    clear_dircache_queue();

    /* this reminds us to keep moving after we're done here; a volume we passed
       up earlier could have been mounted and need refreshing; it just condenses
       a slew of requests into one and isn't mistaken for an externally generated
       command */
    queue_post(&dircache_queue, DCM_PROCEED, 0);
}

#if defined (DIRCACHE_NATIVE)
/**
 * scan and build the contents of a subdirectory
 */
static void sab_process_sub(struct sab *sabp)
{
    struct fat_direntry *const fatentp = get_dir_fatent();
    struct filestr_base *const streamp = &sabp->stream;
    struct file_base_info *const infop = &sabp->info;

    int idx = infop->dcfile.idx;
    int *downp = get_downidxp(idx);
    if (!downp)
        return;

    while (1)
    {
        struct sab_component *compp = --sabp->top;
        compp->idx   = idx;
        compp->downp = downp;
        compp->prevp = downp;

        /* open directory stream */
        filestr_base_init(streamp);
        fileobj_fileop_open(streamp, infop, FO_DIRECTORY);
        fat_rewind(&streamp->fatstr);
        uncached_rewinddir_internal(infop);

        const long dircluster = streamp->infop->fatfile.firstcluster;

        /* first pass: read directory */
        while (1)
        {
            if (sabp->stack + 1 < sabp->stackend)
            {
                /* release control and process queued events */
                dircache_unlock();
                process_events();
                dircache_lock();

                if (sabp->quit || !compp->idx)
                    break;
            }
            /* else an immediate-contents directory scan */

            int rc = uncached_readdir_internal(streamp, infop, fatentp);
            if (rc <= 0)
            {
                if (rc < 0)
                    sabp->quit = true;
                else
                    compp->prevp = downp; /* rewind list */

                break;
            }

            struct dircache_entry *ce;
            int prev = *compp->prevp;

            if (prev)
            {
                /* there are entries ahead of us; they will be what was just
                   read or something to be subsequently read; if it belongs
                   ahead of this one, insert a new entry before it; if it's
                   the entry just scanned, do nothing further and continue
                   with the next */
                ce = get_entry(prev);
                if (ce->direntry == infop->fatfile.e.entry)
                {
                    compp->prevp = &ce->next;
                    continue; /* already there */
                }
            }

            int idx = create_entry(fatentp->name, &ce);
            if (idx <= 0)
            {
                if (idx == -ENAMETOOLONG)
                {
                    /* not fatal; just don't include it */
                    establish_frontier(compp->idx, FRONTIER_ZONED);
                    continue;
                }

                sabp->quit = true;
                break;
            }

            /* link it in */
            ce->up = compp->idx;
            ce->next = prev;
            *compp->prevp = idx;
            compp->prevp = &ce->next;

            if (!(fatentp->attr & ATTR_DIRECTORY))
                ce->filesize = fatentp->filesize;
            else if (!is_dotdir_name(fatentp->name))
                ce->frontier = FRONTIER_NEW; /* this needs scanning */

            /* copy remaining FS info */
            ce->direntry     = infop->fatfile.e.entry;
            ce->direntries   = infop->fatfile.e.entries;
            ce->attr         = fatentp->attr;
            ce->firstcluster = fatentp->firstcluster;
            ce->wrtdate      = fatentp->wrtdate;
            ce->wrttime      = fatentp->wrttime;

            /* resolve queued user bindings */
            infop->fatfile.firstcluster = fatentp->firstcluster;
            infop->fatfile.dircluster   = dircluster;
            infop->dcfile.idx           = idx;
            infop->dcfile.serialnum     = ce->serialnum;
            binding_resolve(infop);
        } /* end while */

        close_stream_internal(streamp);

        if (sabp->quit)
            return;

        establish_frontier(compp->idx, FRONTIER_SETTLED);

        /* second pass: "recurse!" */
        struct dircache_entry *ce = NULL;

        while (1)
        {
            idx = compp->idx && compp > sabp->stack ? *compp->prevp : 0;
            if (idx)
            {
                ce = get_entry(idx);
                compp->prevp = &ce->next;

                if (ce->frontier != FRONTIER_SETTLED)
                    break;
            }
            else
            {
                /* directory completed or removed/deepest level */
                compp = ++sabp->top;
                if (compp >= sabp->stackend)
                    return; /* scan completed/initial directory removed */
            }
        }

        /* even if it got zoned from outside it is about to be scanned in
           its entirety and may be considered new again */
        ce->frontier = FRONTIER_NEW;
        downp = &ce->down;

        /* set up info for next open
         * IF_MV: "volume" was set when scan began */
        infop->fatfile.firstcluster = ce->firstcluster;
        infop->fatfile.dircluster   = dircluster;
        infop->fatfile.e.entry      = ce->direntry;
        infop->fatfile.e.entries    = ce->direntries;
        infop->dcfile.idx           = idx;
        infop->dcfile.serialnum     = ce->serialnum;
    } /* end while */
}

/**
 * scan and build the contents of a directory or volume root
 */
static void sab_process_dir(struct file_base_info *infop, bool issab)
{
    /* infop should have been fully opened meaning that all its parent
       directory information is filled in and intact; the binding information
       should also filled in beforehand */

    /* allocate the stack right now to the max demand */
    struct dirsab
    {
        struct sab           sab;
        struct sab_component stack[issab ? DIRCACHE_MAX_DEPTH : 1];
    } dirsab;
    struct sab *sabp = &dirsab.sab;

    sabp->quit     = false;
    sabp->stackend = &sabp->stack[ARRAYLEN(dirsab.stack)];
    sabp->top      = sabp->stackend;
    sabp->info     = *infop;

    if (issab)
        DCRIVOL(infop)->sabp = sabp;

    establish_frontier(infop->dcfile.idx, FRONTIER_NEW | FRONTIER_RENEW);
    sab_process_sub(sabp);

    if (issab)
        DCRIVOL(infop)->sabp = NULL;
}

/**
 * scan and build the entire tree for a volume
 */
static void sab_process_volume(struct dircache_volume *dcvolp)
{
    int rc;

    int volume = IF_MV_VOL(dcvolp - dircache.dcvol);
    int idx = -volume - 1;

    logf("dircache - building volume %d", volume);

    /* gather everything sab_process_dir() needs in order to begin a scan */
    struct file_base_info info;
    rc = fat_open_rootdir(IF_MV(volume,) &info.fatfile);
    if (rc < 0)
    {
        /* probably not mounted */
        logf("SAB - no root %d: %d", volume, rc);
        establish_frontier(idx, FRONTIER_NEW);
        return;
    }

    info.dcfile.idx       = idx;
    info.dcfile.serialnum = dcvolp->serialnum;
    binding_resolve(&info);
    sab_process_dir(&info, true);
}

/**
 * this function is the back end to the public API's like readdir()
 */
int dircache_readdir_dirent(struct filestr_base *stream,
                            struct dirscan_info *scanp,
                            struct dirent *entry)
{
    struct file_base_info *dirinfop = stream->infop;

    if (!dirinfop->dcfile.serialnum)
        goto read_uncached; /* no parent cached => no entries cached */

    struct dircache_volume *dcvolp = DCVOL(dirinfop);

    int diridx = dirinfop->dcfile.idx;
    unsigned int frontier = diridx <= 0 ? dcvolp->frontier :
                                          get_entry(diridx)->frontier;

    /* if not settled, just readthrough; no binding information is needed for
       this; if it becomes settled, we'll get scan->dcfile caught up and do
       subsequent reads with the cache */
    if (frontier != FRONTIER_SETTLED)
        goto read_uncached;

    int idx = scanp->dcscan.idx;
    struct dircache_entry *ce;

    unsigned int direntry = scanp->fatscan.entry;
    while (1)
    {
        if (idx == 0 || direntry == FAT_DIRSCAN_RW_VAL) /* rewound? */
        {
            idx = diridx <= 0 ? dcvolp->root_down : get_entry(diridx)->down;
            break;
        }

        /* validate last entry scanned; it might have been replaced between
           calls or not there at all any more; if so, start the cache reader
           at the beginning and fast-forward to the correct point as indicated
           by the FS scanner */
        ce = get_entry(idx);
        if (ce && ce->serialnum == scanp->dcscan.serialnum)
        {
            idx = ce->next;
            break;
        }

        idx = 0;
    }

    while (1)
    {
        ce = get_entry(idx);
        if (!ce)
        {
            empty_dirent(entry);
            scanp->fatscan.entries = 0;
            return 0; /* end of dir */
        }

        if (ce->direntry > direntry || direntry == FAT_DIRSCAN_RW_VAL)
            break; /* cache reader is caught up to FS scan */

        idx = ce->next;
    }

    /* basic dirent information */
    entry_name_copy(entry->d_name, ce);
    entry->info.attr    = ce->attr;
    entry->info.size    = (ce->attr & ATTR_DIRECTORY) ? 0 : ce->filesize;
    entry->info.wrtdate = ce->wrtdate;
    entry->info.wrttime = ce->wrttime;

    /* FS scan information */
    scanp->fatscan.entry    = ce->direntry;
    scanp->fatscan.entries  = ce->direntries;

    /* dircache scan information */
    scanp->dcscan.idx       = idx;
    scanp->dcscan.serialnum = ce->serialnum;

    /* return whether this needs decoding */
    int rc = ce->direntries == 1 ? 2 : 1;

    yield();
    return rc;

read_uncached:
    dircache_dcfile_init(&scanp->dcscan);
    return uncached_readdir_dirent(stream, scanp, entry);
}

/**
 * rewind the directory scan cursor
 */
void dircache_rewinddir_dirent(struct dirscan_info *scanp)
{
    uncached_rewinddir_dirent(scanp);
    dircache_dcfile_init(&scanp->dcscan);
}

/**
 * this function is the back end to file API internal scanning, which requires
 * much more detail about the directory entries; this is allowed to make
 * assumptions about cache state because the cache will not be altered during
 * the scan process; an additional important property of internal scanning is
 * that any available binding information is not ignored even when a scan
 * directory is frontier zoned.
 */
int dircache_readdir_internal(struct filestr_base *stream,
                              struct file_base_info *infop,
                              struct fat_direntry *fatent)
{
    /* call with writer exclusion */
    struct file_base_info *dirinfop = stream->infop;
    struct dircache_volume *dcvolp = DCVOL(dirinfop);

    /* assume binding "not found" */
    infop->dcfile.serialnum = 0;

    /* is parent cached? if not, readthrough because nothing is here yet */
    if (!dirinfop->dcfile.serialnum)
    {
        if (stream->flags & FF_CACHEONLY)
            goto read_eod;

        return uncached_readdir_internal(stream, infop, fatent);
    }

    int diridx = dirinfop->dcfile.idx;
    unsigned int frontier = diridx < 0 ?
        dcvolp->frontier : get_entry(diridx)->frontier;

    int idx = infop->dcfile.idx;
    if (idx == 0) /* rewound? */
        idx = diridx <= 0 ? dcvolp->root_down : get_entry(diridx)->down;
    else
        idx = get_entry(idx)->next;

    struct dircache_entry *ce = get_entry(idx);

    if (frontier != FRONTIER_SETTLED && !(stream->flags & FF_CACHEONLY))
    {
        /* the directory being read is reported to be incompletely cached;
           readthrough and if the entry exists, return it with its binding
           information; otherwise return the uncached read result while
           maintaining the last index */
        int rc = uncached_readdir_internal(stream, infop, fatent);
        if (rc <= 0 || !ce || ce->direntry > infop->fatfile.e.entry)
            return rc;

        /* entry matches next one to read */
    }
    else if (!ce)
    {
        /* end of dir */
        goto read_eod;
    }

    /* FS entry information that we maintain */
    entry_name_copy(fatent->name, ce);
    fatent->shortname[0]     = '\0';
    fatent->attr             = ce->attr;
    /* file code file scanning does not need time information */
    fatent->filesize         = (ce->attr & ATTR_DIRECTORY) ? 0 : ce->filesize;
    fatent->firstcluster     = ce->firstcluster;

    /* FS entry directory information */
    infop->fatfile.e.entry   = ce->direntry;
    infop->fatfile.e.entries = ce->direntries;

    /* dircache file binding information */
    infop->dcfile.idx        = idx;
    infop->dcfile.serialnum  = ce->serialnum;

    /* return whether this needs decoding */
    int rc = ce->direntries == 1 ? 2 : 1;

    if (frontier == FRONTIER_SETTLED)
    {
        static long next_yield;
        if (TIME_AFTER(current_tick, next_yield))
        {
            yield();
            next_yield = current_tick + HZ/50;
        }
    }

    return rc;

read_eod:
    fat_empty_fat_direntry(fatent);
    infop->fatfile.e.entries = 0;
    return 0;    
}

/**
 * rewind the scan position for an internal scan
 */
void dircache_rewinddir_internal(struct file_base_info *infop)
{
    uncached_rewinddir_internal(infop);
    dircache_dcfile_init(&infop->dcfile);
}

#else /* !DIRCACHE_NATIVE (for all others) */

#####################
/* we require access to the host functions */
#undef opendir
#undef readdir
#undef closedir
#undef rewinddir

static char sab_path[MAX_PATH];

static int sab_process_dir(struct dircache_entry *ce)
{
    struct dirent_uncached *entry;
    struct dircache_entry *first_ce = ce;
    DIR *dir = opendir(sab_path);
    if(dir == NULL)
    {
        logf("Failed to opendir_uncached(%s)", sab_path);
        return -1;
    }

    while (1)
    {
        if (!(entry = readdir(dir)))
            break;

        if (IS_DOTDIR_NAME(entry->d_name))
        {
            /* add "." and ".." */
            ce->info.attribute = ATTR_DIRECTORY;
            ce->info.size = 0;
            ce->down = entry->d_name[1] == '\0' ? first_ce : first_ce->up;
            strcpy(ce->dot_d_name, entry->d_name);
            continue;
        }

        size_t size = strlen(entry->d_name) + 1;
        ce->d_name = (d_names_start -= size);
        ce->info = entry->info;

        strcpy(ce->d_name, entry->d_name);
        dircache_size += size;

        if(entry->info.attribute & ATTR_DIRECTORY)
        {
            dircache_gen_down(ce, ce);
            if(ce->down == NULL)
            {
                closedir_uncached(dir);
                return -1;
            }
            /* save current paths size */
            int pathpos = strlen(sab_path);
            /* append entry */
            strlcpy(&sab_path[pathpos], "/", sizeof(sab_path) - pathpos);
            strlcpy(&sab_path[pathpos+1], entry->d_name, sizeof(sab_path) - pathpos - 1);

            int rc = sab_process_dir(ce->down);
            /* restore path */
            sab_path[pathpos] = '\0';

            if(rc < 0)
            {
                closedir_uncached(dir);
                return rc;
            }
        }

        ce = dircache_gen_entry(ce);
        if (ce == NULL)
            return -5;

        yield();
    }

    closedir_uncached(dir);
    return 1;
}

static int sab_process_volume(IF_MV(int volume,) struct dircache_entry *ce)
{
    memset(ce, 0, sizeof(struct dircache_entry));
    strlcpy(sab_path, "/", sizeof sab_path);
    return sab_process_dir(ce);
}

int dircache_readdir_r(struct dircache_dirscan *dir, struct dirent *result)
{
    if (dircache_state != DIRCACHE_READY)
        return readdir_r(dir->###########3, result, &result);

    bool first = dir->dcinfo.scanidx == REWIND_INDEX;
    struct dircache_entry *ce = get_entry(first ? dir->dcinfo.index :
                                                  dir->dcinfo.scanidx);

    ce = first ? ce->down : ce->next;

    if (ce == NULL)
        return 0;

    dir->scanidx = ce - dircache_root;

    strlcpy(result->d_name, ce->d_name, sizeof (result->d_name));
    result->info = ce->dirinfo;

    return 1;
}

#endif /* DIRCACHE_* */

/**
 * reset the cache for the specified volume
 */
static void reset_volume(IF_MV_NONVOID(int volume))
{
    FOR_EACH_VOLUME(volume, i)
    {
        struct dircache_volume *dcvolp = DCVOL(i);

        if (dcvolp->status == DIRCACHE_IDLE)
            continue; /* idle => nothing happening there */

        struct dircache_runinfo_volume *dcrivolp = DCRIVOL(i);

        /* stop any scan and build on this one */
        if (dcrivolp->sabp)
            dcrivolp->sabp->quit = true;

    #ifdef HAVE_MULTIVOLUME
        /* if this call is for all volumes, subsequent code will just reset
           the cache memory usage and the freeing of individual entries may
           be skipped */
        if (volume >= 0)
            free_subentries(NULL, &dcvolp->root_down);
        else
    #endif
            binding_dissolve_volume(dcrivolp);

        /* set it back to unscanned */
        dcvolp->status      = DIRCACHE_IDLE;
        dcvolp->frontier    = FRONTIER_NEW;
        dcvolp->root_down   = 0;
        dcvolp->build_ticks = 0;
        dcvolp->serialnum   = 0;
    }
}

/**
 * reset the entire cache state for all volumes
 */
static void reset_cache(void)
{
    if (!dircache_runinfo.handle)
        return; /* no buffer => nothing cached */

    /* blast all the volumes */
    reset_volume(IF_MV(-1));

#ifdef DIRCACHE_DUMPSTER
    dumpster_clean_buffer(dircache_runinfo.p + ENTRYSIZE,
                          dircache_runinfo.bufsize);
#endif /* DIRCACHE_DUMPSTER */

    /* reset the memory */
    dircache.free_list    = 0;
    dircache.size         = 0;
    dircache.sizeused     = 0;
    dircache.numentries   = 0;
    dircache.names        = dircache_runinfo.bufsize + ENTRYSIZE;
    dircache.sizenames    = 0;
    dircache.namesfree    = 0;
    dircache.nextnamefree = 0;
    *get_name(dircache.names - 1) = 0;
    /* dircache.last_serialnum stays */
    /* dircache.reserve_used stays */
    /* dircache.last_size stays */
}

/**
 * checks each "idle" volume and builds it
 */
static void build_volumes(void)
{
    buffer_lock();

    for (int i = 0; i < NUM_VOLUMES; i++)
    {
        /* this does reader locking but we already own that */
        if (!volume_ismounted(IF_MV(i)))
            continue;

        struct dircache_volume *dcvolp = DCVOL(i);

        /* can't already be "scanning" because that's us; doesn't retry
           "ready" volumes */
        if (dcvolp->status == DIRCACHE_READY)
            continue;

        /* measure how long it takes to build the cache for each volume */
        if (!dcvolp->serialnum)
            dcvolp->serialnum = next_serialnum();

        dcvolp->status = DIRCACHE_SCANNING;
        dcvolp->start_tick = current_tick;

        sab_process_volume(dcvolp);

        if (dircache_runinfo.suspended)
            break;

        /* whatever happened, it's ready unless reset */
        dcvolp->build_ticks = current_tick - dcvolp->start_tick;
        dcvolp->status = DIRCACHE_READY;
    }

    size_t reserve_used = reserve_buf_used();
    if (reserve_used > dircache.reserve_used)
        dircache.reserve_used = reserve_used;

    if (DIRCACHE_STUFFED(reserve_used))
        dircache.last_size = 0;             /* reset */
    else if (dircache.size > dircache.last_size)
        dircache.last_size = dircache.size; /* grow */
    else if (!dircache_runinfo.suspended &&
             dircache.last_size - dircache.size > DIRCACHE_RESERVE)
        dircache.last_size = dircache.size; /* shrink if not suspended */

    logf("Done, %ld KiB used", dircache.size / 1024);

    buffer_unlock();
}

/**
 * allocate buffer and return whether or not a synchronous build should take
 * place; if 'realloced' is NULL, it's just a query about what will happen
 */
static int prepare_build(bool *realloced)
{
    /* called holding dircache lock */
    size_t size = dircache.last_size;

#ifdef HAVE_EEPROM_SETTINGS
    if (realloced)
    {
        dircache_unlock();
        remove_dircache_file();
        dircache_lock();

        if (dircache_runinfo.suspended)
            return -1;
    }
#endif /* HAVE_EEPROM_SETTINGS */

    bool stuffed = DIRCACHE_STUFFED(dircache.reserve_used);
    if (dircache_runinfo.bufsize > size && !stuffed)
    {
        if (realloced)
            *realloced = false;

        return 0; /* start a transparent rebuild */
    }

    int syncbuild = size > 0 && !stuffed ? 0 : 1;

    if (!realloced)
        return syncbuild;

    if (syncbuild)
    {
        /* start a non-transparent rebuild */
        /* we'll use the entire audiobuf to allocate the dircache */
        size = audio_buffer_available() + dircache_runinfo.bufsize;
        /* try to allocate at least the min and no more than the limit */
        size = MAX(DIRCACHE_MIN, MIN(size, DIRCACHE_LIMIT));
    }
    else
    {
        /* start a transparent rebuild */
        size = MAX(size, DIRCACHE_RESERVE) + DIRCACHE_RESERVE*2;
    }

    *realloced = true;
    reset_cache();

    buffer_lock();

    int handle = reset_buffer();
    dircache_unlock();

    if (handle > 0)
        core_free(handle);

    handle = alloc_cache(size);

    dircache_lock();

    if (dircache_runinfo.suspended && handle > 0)
    {
        /* if we got suspended, don't keep this huge buffer around */
        dircache_unlock();
        core_free(handle);
        handle = 0;
        dircache_lock();
    }

    if (handle <= 0)
    {
        buffer_unlock();
        return -1;
    }

    set_buffer(handle, size);
    buffer_unlock();

    return syncbuild;
}

/**
 * compact the dircache buffer after a successful full build
 */
static void compact_cache(void)
{
    /* called holding dircache lock */
    if (dircache_runinfo.suspended)
        return;

    void *p = dircache_runinfo.p + ENTRYSIZE;
    size_t leadsize = dircache.numentries * ENTRYSIZE + DIRCACHE_RESERVE;

    void *dst = p + leadsize;
    void *src = get_name(dircache.names - 1);
    if (dst >= src)
        return; /* cache got bigger than expected; never mind that */

    /* slide the names up in memory */
    memmove(dst, src, dircache.sizenames + 2);

    /* fix up name indexes */
    ptrdiff_t offset = dst - src;

    FOR_EACH_CACHE_ENTRY(ce)
    {
        if (!ce->tinyname)
             ce->name += offset;
    }

    dircache.names += offset;

    /* assumes beelzelib doesn't do things like calling callbacks or changing
       the pointer as a result of the shrink operation; it doesn't as of now
       but if it ever does that may very well cause deadlock problems since
       we're holding filesystem locks */
    size_t newsize = leadsize + dircache.sizenames + 1;
    core_shrink(dircache_runinfo.handle, p, newsize + 1);
    dircache_runinfo.bufsize = newsize;
    dircache.reserve_used = 0;
}

/**
 * internal thread that controls cache building; exits when no more requests
 * are pending or the cache is suspended
 */
static void dircache_thread(void)
{
    struct queue_event ev;

    /* calls made within the loop reopen the lock */
    dircache_lock();

    while (1)
    {
        queue_wait_w_tmo(&dircache_queue, &ev, 0);
        if (ev.id == SYS_TIMEOUT || dircache_runinfo.suspended)
        {
            /* nothing left to do/suspended */
            if (dircache_runinfo.suspended)
                clear_dircache_queue();
            dircache_runinfo.thread_done = true;
            break;
        }

        /* background-only builds are not allowed if a synchronous build is
           required first; test what needs to be done and if it checks out,
           do it for real below */
        int *rcp = (int *)ev.data;

        if (!rcp && prepare_build(NULL) > 0)
            continue;

        bool realloced;
        int rc = prepare_build(&realloced);
        if (rcp)
            *rcp = rc;

        if (rc < 0)
            continue;

        trigger_cpu_boost();
        build_volumes();

        /* if it was reallocated, compact it */
        if (realloced)
            compact_cache();
     }

     dircache_unlock();
}

/**
 * post a scan and build message to the thread, starting it if required
 */
static bool dircache_thread_post(int volatile *rcp)
{
    if (dircache_runinfo.thread_done)
    {
        /* mustn't recreate until it exits so that the stack isn't reused */
        thread_wait(dircache_runinfo.thread_id);
        dircache_runinfo.thread_done = false;
        dircache_runinfo.thread_id = create_thread(
            dircache_thread, dircache_stack, sizeof (dircache_stack), 0,
            dircache_thread_name IF_PRIO(, PRIORITY_BACKGROUND)
            IF_COP(, CPU));
    }

    bool started = dircache_runinfo.thread_id != 0;

    if (started)
        queue_post(&dircache_queue, DCM_BUILD, (intptr_t)rcp);

    return started;
}

/**
 * wait for the dircache thread to finish building; intended for waiting for a
 * non-transparent build to finish when dircache_resume() returns > 0
 */
void dircache_wait(void)
{
    thread_wait(dircache_runinfo.thread_id);
}

/**
 * call after mounting a volume or all volumes
 */
void dircache_mount(void)
{
    /* call with writer exclusion */
    if (dircache_runinfo.suspended)
        return;

    dircache_thread_post(NULL);
}

/**
 * call after unmounting a volume; specifying < 0 for all or >= 0 for the
 * specific one
 */
void dircache_unmount(IF_MV_NONVOID(int volume))
{
    /* call with writer exclusion */
    if (dircache_runinfo.suspended)
        return;

#ifdef HAVE_MULTIVOLUME
    if (volume >= 0)
        reset_volume(volume);
    else
#endif /* HAVE_MULTIVOLUME */
        reset_cache();
}

/* backend to dircache_suspend() and dircache_disable() */
static void dircache_suspend_internal(bool freeit)
{
    if (dircache_runinfo.suspended++ > 0 &&
        (!freeit || dircache_runinfo.handle <= 0))
        return;

    unsigned int thread_id = dircache_runinfo.thread_id;

    reset_cache();
    clear_dircache_queue();

    /* grab the buffer away into our control; the cache won't need it now */
    int handle = 0;
    if (freeit)
        handle = reset_buffer();

    dircache_unlock();

    if (handle > 0)
        core_free(handle);

    thread_wait(thread_id);

    dircache_lock();
}

/* backend to dircache_resume() and dircache_enable() */
static int dircache_resume_internal(bool build_now)
{
    int volatile rc = 0;

    if (dircache_runinfo.suspended == 0 || --dircache_runinfo.suspended == 0)
        rc = build_now && dircache_runinfo.enabled ? 1 : 0;

    if (rc)
        rc = dircache_thread_post(&rc) ? INT_MIN : -1;

    if (rc == INT_MIN)
    {
        dircache_unlock();

        while (rc == INT_MIN) /* poll for response */
            sleep(0);

        dircache_lock();
    }

    return rc < 0 ? rc * 10 - 2 : rc;
}

/**
 * service to dircache_enable() and dircache_load(); "build_now" starts a build
 * immediately if the cache was not enabled
 */
static int dircache_enable_internal(bool build_now)
{
    int rc = 0;

    if (!dircache_runinfo.enabled)
    {
        dircache_runinfo.enabled = true;
        rc = dircache_resume_internal(build_now);
    }

    return rc;
}

/**
 * service to dircache_disable()
 */
static void dircache_disable_internal(void)
{
    if (dircache_runinfo.enabled)
    {
        dircache_runinfo.enabled = false;
        dircache_suspend_internal(true);
    }
}

/**
 * disables dircache without freeing the buffer (so it can be re-enabled
 * afterwards with dircache_resume(); usually called when accepting an USB
 * connection
 */
void dircache_suspend(void)
{
    dircache_lock();
    dircache_suspend_internal(false);
    dircache_unlock();
}

/**
 * re-enables the dircache if previously suspended by dircache_suspend
 * or dircache_steal_buffer(), re-using the already allocated buffer if
 * available
 *
 * returns: 0 if the background build is started or dircache is still
 *          suspended
 *          > 0 if the build is non-background
 *          < 0 upon failure
 */
int dircache_resume(void)
{
    dircache_lock();
    int rc = dircache_resume_internal(true);
    dircache_unlock();
    return rc;
}

/**
 * as dircache_resume() but globally enables it; called by settings and init
 */
int dircache_enable(void)
{
    dircache_lock();
    int rc = dircache_enable_internal(true);
    dircache_unlock();
    return rc;
}

/**
 * as dircache_suspend() but also frees the buffer; usually called on shutdown
 * or when deactivated
 */
void dircache_disable(void)
{
    dircache_lock();
    dircache_disable_internal();
    dircache_unlock();
}

/**
 * have dircache give up its allocation; call dircache_resume() to restart it
 */
void dircache_free_buffer(void)
{
    dircache_lock();
    dircache_suspend_internal(true);
    dircache_unlock();
}


/** Dircache live updating **/

/**
 * obtain binding information for the file's root volume; this is the starting
 * point for internal path parsing and binding
 */
void dircache_get_rootinfo(struct file_base_info *infop)
{
    int volume = BASEINFO_VOL(infop);
    struct dircache_volume *dcvolp = DCVOL(volume);

    if (dcvolp->serialnum)
    {
        /* root has a binding */
        infop->dcfile.idx       = -volume - 1;
        infop->dcfile.serialnum = dcvolp->serialnum;
    }
    else
    {
        /* root is idle */
        dircache_dcfile_init(&infop->dcfile);
    }
}

/**
 * called by file code when the first reference to a file or directory is
 * opened
 */
void dircache_bind_file(struct file_base_binding *bindp)
{
    /* requires write exclusion */
    logf("dc open: %u", (unsigned int)bindp->info.dcfile.serialnum);
    binding_open(bindp);
}

/**
 * called by file code when the last reference to a file or directory is
 * closed
 */
void dircache_unbind_file(struct file_base_binding *bindp)
{
    /* requires write exclusion */
    logf("dc close: %u", (unsigned int)bindp->info.dcfile.serialnum);
    binding_close(bindp);
}

/**
 * called by file code when a file is newly created
 */
void dircache_fileop_create(struct file_base_info *dirinfop,
                            struct file_base_binding *bindp,
                            const char *basename,
                            const struct dirinfo_native *dinp)
{
    /* requires write exclusion */
    logf("dc create: %u \"%s\"",
         (unsigned int)bindp->info.dcfile.serialnum, basename);

    if (!dirinfop->dcfile.serialnum)
    {
        /* no parent binding => no child binding */
        return;
    }

    struct dircache_entry *ce;
    int idx = create_entry(basename, &ce);
    if (idx <= 0)
    {
        /* failed allocation; parent cache contents are not complete */
        establish_frontier(dirinfop->dcfile.idx, FRONTIER_ZONED);
        return;
    }

    struct file_base_info *infop = &bindp->info;

#ifdef DIRCACHE_NATIVE
    ce->firstcluster = infop->fatfile.firstcluster;
    ce->direntry     = infop->fatfile.e.entry;
    ce->direntries   = infop->fatfile.e.entries;
    ce->wrtdate      = dinp->wrtdate;
    ce->wrttime      = dinp->wrttime;
#else
    ce->mtime        = dinp->mtime;
#endif
    ce->attr         = dinp->attr;
    if (!(dinp->attr & ATTR_DIRECTORY))
        ce->filesize = dinp->size;

    insert_file_entry(dirinfop, ce);

    /* file binding will have been queued when it was opened; just resolve */
    infop->dcfile.idx       = idx;
    infop->dcfile.serialnum = ce->serialnum;
    binding_resolve(infop);

    if ((dinp->attr & ATTR_DIRECTORY) && !is_dotdir_name(basename))
    {
        /* scan-in the contents of the new directory at this level only */
        buffer_lock();
        sab_process_dir(infop, false);
        buffer_unlock();
    }
}

/**
 * called by file code when a file or directory is removed
 */
void dircache_fileop_remove(struct file_base_binding *bindp)
{
    /* requires write exclusion */
    logf("dc remove: %u\n", (unsigned int)bindp->info.dcfile.serialnum);

    if (!bindp->info.dcfile.serialnum)
        return; /* no binding yet */

    free_file_entry(&bindp->info);

    /* if binding was resolved; it should now be queued via above call */
}

/**
 * called by file code when a file is renamed
 */
void dircache_fileop_rename(struct file_base_info *dirinfop,
                            struct file_base_binding *bindp,
                            const char *basename)
{
    /* requires write exclusion */
    logf("dc rename: %u \"%s\"",
         (unsigned int)bindp->info.dcfile.serialnum, basename);

    if (!dirinfop->dcfile.serialnum)
    {
        /* new parent directory not cached; there is nowhere to put it so
           nuke it */
        if (bindp->info.dcfile.serialnum)
            free_file_entry(&bindp->info);
        /* else no entry anyway */

        return;
    }

    if (!bindp->info.dcfile.serialnum)
    {
        /* binding not resolved on the old file but it's going into a resolved
           parent which means the parent would be missing an entry in the cache;
           downgrade the parent */
        establish_frontier(dirinfop->dcfile.idx, FRONTIER_ZONED);
        return;
    }

    /* unlink the entry but keep it; it needs to be re-sorted since the
       underlying FS probably changed the order */
    struct dircache_entry *ce = remove_file_entry(&bindp->info);

#ifdef DIRCACHE_NATIVE
    /* update other name-related information before inserting */
    ce->direntry   = bindp->info.fatfile.e.entry;
    ce->direntries = bindp->info.fatfile.e.entries;
#endif

    /* place it into its new home */
    insert_file_entry(dirinfop, ce);

    /* lastly, update the entry name itself */
    if (entry_reassign_name(ce, basename) == 0)
    {
        /* it's not really the same one now so re-stamp it */
        dc_serial_t serialnum = next_serialnum();
        ce->serialnum = serialnum;
        bindp->info.dcfile.serialnum = serialnum;
    }
    else
    {
        /* it cannot be kept around without a valid name */
        free_file_entry(&bindp->info);
        establish_frontier(dirinfop->dcfile.idx, FRONTIER_ZONED);
    }
}

/**
 * called by file code to synchronize file entry information
 */
void dircache_fileop_sync(struct file_base_binding *bindp,
                          const struct dirinfo_native *dinp)
{
    /* requires write exclusion */
    struct file_base_info *infop = &bindp->info;
    logf("dc sync: %u\n", (unsigned int)infop->dcfile.serialnum);

    if (!infop->dcfile.serialnum)
        return; /* binding unresolved */

    struct dircache_entry *ce = get_entry(infop->dcfile.idx);
    if (!ce)
    {
        logf("  bad index %d", infop->dcfile.idx);
        return; /* a root (should never be called for this) */
    }

#ifdef DIRCACHE_NATIVE
    ce->firstcluster = infop->fatfile.firstcluster;
    ce->wrtdate      = dinp->wrtdate;
    ce->wrttime      = dinp->wrttime;
#else
    ce->mtime        = dinp->mtime;
#endif
    ce->attr         = dinp->attr;
    if (!(dinp->attr & ATTR_DIRECTORY))
        ce->filesize = dinp->size;
}


/** Dircache paths and files **/

/**
 * helper for returning a path and serial hash represented by an index
 */
struct get_path_sub_data
{
    char        *buf;
    size_t      size;
    dc_serial_t serialhash;
};

static ssize_t get_path_sub(int idx, struct get_path_sub_data *data)
{
    if (idx == 0)
        return -1; /* entry is an orphan split from any root */

    ssize_t len;
    char *cename;

    if (idx > 0)
    {
        struct dircache_entry *ce = get_entry(idx);

        data->serialhash = dc_hash_serialnum(ce->serialnum, data->serialhash);

        /* go all the way up then move back down from the root */
        len = get_path_sub(ce->up, data) - 1;
        if (len < 0)
            return -2;

        cename = alloca(DC_MAX_NAME + 1);
        entry_name_copy(cename, ce);
    }
    else /* idx < 0 */
    {
        len = 0;
        cename = "";

    #ifdef HAVE_MULTIVOLUME
        int volume = IF_MV_VOL(-idx - 1);
        if (volume > 0)
        {
            /* prepend the volume specifier for volumes > 0 */
            cename = alloca(VOL_MAX_LEN+1);
            get_volume_name(volume, cename);
        }
    #endif /* HAVE_MULTIVOLUME */

        data->serialhash = dc_hash_serialnum(get_idx_dcvolp(idx)->serialnum,
                                             data->serialhash);
    }

    return len + path_append(data->buf + len, PA_SEP_HARD, cename,
                             data->size > (size_t)len ? data->size - len : 0);
}

/**
 * validate the file's entry/binding serial number
 * the dircache file's serial number must match the indexed entry's or the
 * file reference is stale
 */
static int check_file_serialnum(const struct dircache_file *dcfilep)
{
    int idx = dcfilep->idx;

    if (idx == 0 || idx < -NUM_VOLUMES)
        return -EBADF;

    dc_serial_t serialnum = dcfilep->serialnum;

    if (serialnum == 0)
        return -EBADF;

    dc_serial_t s;

    if (idx > 0)
    {
        struct dircache_entry *ce = get_entry(idx);
        if (!ce || !(s = ce->serialnum))
            return -EBADF;
    }
    else /* idx < 0 */
    {
        struct dircache_volume *dcvolp = get_idx_dcvolp(idx);
        if (!(s = dcvolp->serialnum))
            return -EBADF;
    }

    if (serialnum != s)
        return -EBADF;

    return 0;
}

/**
 * Obtain the hash of the serial numbers of the canonical path, index to root
 */
static dc_serial_t get_file_serialhash(const struct dircache_file *dcfilep)
{
    int idx = dcfilep->idx;

    dc_serial_t h = DC_SERHASH_START;

    while (idx > 0)
    {
        struct dircache_entry *ce = get_entry(idx);
        h = dc_hash_serialnum(ce->serialnum, h);
        idx = ce->up;
    }

    if (idx < 0)
        h = dc_hash_serialnum(get_idx_dcvolp(idx)->serialnum, h);

    return h;
}

/**
 * Initialize the fileref
 */
void dircache_fileref_init(struct dircache_fileref *dcfrefp)
{
    dircache_dcfile_init(&dcfrefp->dcfile);
    dcfrefp->serialhash = DC_SERHASH_START;
}

/**
 * usermode function to construct a full absolute path from dircache into the
 * given buffer given the dircache file info
 *
 * returns:
 *   success - the length of the string, not including the trailing null or the
 *             buffer length required if the buffer is too small (return is >=
 *             size)
 *   failure - a negative value
 *
 * errors:
 *   EBADF  - Bad file number
 *   EFAULT - Bad address
 *   ENOENT - No such file or directory
 */
ssize_t dircache_get_fileref_path(const struct dircache_fileref *dcfrefp, char *buf,
                                  size_t size)
{
    ssize_t rc;

    if (!dcfrefp)
        FILE_ERROR_RETURN(EFAULT, -1);

    /* if missing buffer space, still return what's needed a la strlcpy */
    if (!buf)
        size = 0;
    else if (size)
        *buf = '\0';

    dircache_lock();

    /* first and foremost, there must be a cache and the serial number must
       check out */
    if (!dircache_runinfo.handle)
        FILE_ERROR(EBADF, -2);

    rc = check_file_serialnum(&dcfrefp->dcfile);
    if (rc < 0)
        FILE_ERROR(-rc, -3);

    struct get_path_sub_data data =
    {
        .buf        = buf,
        .size       = size,
        .serialhash = DC_SERHASH_START,
    };

    rc = get_path_sub(dcfrefp->dcfile.idx, &data);
    if (rc < 0)
        FILE_ERROR(ENOENT, rc * 10 - 4);

    if (data.serialhash != dcfrefp->serialhash)
        FILE_ERROR(ENOENT, -5);

file_error:
    dircache_unlock();
    return rc;
}

/**
 * Test a path to various levels of rigor and optionally return dircache file
 * info for the given path.
 *
 * If the file reference is used, it is checked first and the path is checked
 * only if all specified file reference checks fail.
 *
 * returns:
 *   success: 0 = not cached (very weak)
 *            1 = serial number checks out for the reference (weak)
 *            2 = serial number and hash check out for the reference (medium)
 *            3 = path is valid; reference updated if specified (strong)
 *   failure: a negative value
 *            if file definitely doesn't exist (errno = ENOENT)
 *            other error
 *
 * errors (including but not limited to):
 *   EFAULT       - Bad address
 *   EINVAL       - Invalid argument
 *   ENAMETOOLONG - File or path name too long
 *   ENOENT       - No such file or directory
 *   ENOTDIR      - Not a directory
 */
int dircache_search(unsigned int flags, struct dircache_fileref *dcfrefp,
                    const char *path)
{
    if (!(flags & (DCS_FILEREF | DCS_CACHED_PATH)))
        FILE_ERROR_RETURN(EINVAL, -1); /* search nothing? */

    if (!dcfrefp && (flags & (DCS_FILEREF | DCS_UPDATE_FILEREF)))
        FILE_ERROR_RETURN(EFAULT, -2); /* bad! */

    int rc = 0;

    dircache_lock();

    /* -- File reference search -- */
    if (!dircache_runinfo.handle)
        ;                       /* cache not enabled; not cached */
    else if (!(flags & DCS_FILEREF))
        ;                       /* don't use fileref */
    else if (check_file_serialnum(&dcfrefp->dcfile) < 0)
        ;                       /* serial number bad */
    else if (!(flags & _DCS_VERIFY_FLAG))
        rc = 1;                 /* only check idx and serialnum */
    else if (get_file_serialhash(&dcfrefp->dcfile) == dcfrefp->serialhash)
        rc = 2;                 /* reference is most likely still valid */

    /* -- Path cache and storage search -- */
    if (rc > 0)
        ; /* rc > 0 */          /* found by file reference */
    else if (!(flags & DCS_CACHED_PATH))
        ; /* rc = 0 */          /* reference bad/unused and no path */
    else
    {     /* rc = 0 */          /* check path with cache and/or storage */
        struct path_component_info compinfo;
        struct filestr_base stream;
        unsigned int ffcache = (flags & _DCS_STORAGE_FLAG) ? 0 : FF_CACHEONLY;
        int err = errno;
        int rc2 = open_stream_internal(path, ffcache | FF_ANYTYPE | FF_PROBE |
                                       FF_INFO | FF_PARENTINFO, &stream,
                                       &compinfo);
        if (rc2 <= 0)
        {
            if (ffcache == 0)
            {
                /* checked storage too: absent for sure */
                FILE_ERROR(rc2 ? ERRNO : ENOENT, rc2 * 10 - 5);
            }

            if (rc2 < 0)
            {
                /* no base info available */
                if (errno != ENOENT)
                    FILE_ERROR(ERRNO, rc2 * 10 - 6);

                /* only cache; something didn't exist: indecisive */
                errno = err;
                FILE_ERROR(ERRNO, RC); /* rc = 0 */
            }

            struct dircache_file *dcfp = &compinfo.parentinfo.dcfile;
            if (get_frontier(dcfp->idx) == FRONTIER_SETTLED)
                FILE_ERROR(ENOENT, -7); /* parent not a frontier; absent */
            /* else checked only cache; parent is incomplete: indecisive */
        }
        else
        {
            struct dircache_file *dcfp = &compinfo.info.dcfile;
            if (dcfp->serialnum != 0)
            {
                /* found by path in the cache afterall */
                if (flags & DCS_UPDATE_FILEREF)
                {
                    dcfrefp->dcfile     = *dcfp;
                    dcfrefp->serialhash = get_file_serialhash(dcfp);
                }

                rc = 3;
            }
        }
    }

file_error:
    if (rc <= 0 && (flags & DCS_UPDATE_FILEREF))
        dircache_fileref_init(dcfrefp);

    dircache_unlock();
    return rc;    
}

/**
 * Compare dircache file references (no validity check is made)
 *
 * returns: 0 - no match
 *          1 - indexes match
 *          2 - serial numbers match
 *          3 - serial and hashes match
 */
int dircache_fileref_cmp(const struct dircache_fileref *dcfrefp1,
                         const struct dircache_fileref *dcfrefp2)
{
    int cmp = 0;

    if (dcfrefp1->dcfile.idx == dcfrefp2->dcfile.idx)
    {
        cmp++;
        if (dcfrefp1->dcfile.serialnum == dcfrefp2->dcfile.serialnum)
        {
            cmp++;
            if (dcfrefp1->serialhash == dcfrefp2->serialhash)
                cmp++;
        }
    }

    return cmp;
}

/** Debug screen/info stuff **/

/**
 * return cache state parameters
 */
void dircache_get_info(struct dircache_info *info)
{
    static const char * const status_descriptions[] =
    {
        [DIRCACHE_IDLE]     = "Idle",
        [DIRCACHE_SCANNING] = "Scanning",
        [DIRCACHE_READY]    = "Ready",
    };

    if (!info)
        return;

    dircache_lock();

    enum dircache_status status = DIRCACHE_IDLE;
    info->build_ticks = 0;

    FOR_EACH_VOLUME(-1, volume)
    {
        struct dircache_volume *dcvolp = DCVOL(volume);
        enum dircache_status volstatus = dcvolp->status;

        switch (volstatus)
        {
        case DIRCACHE_SCANNING:
            /* if any one is scanning then overall status is "scanning" */
            status = volstatus;

            /* sum the time the scanning has taken so far */
            info->build_ticks += current_tick - dcvolp->start_tick;
            break;
        case DIRCACHE_READY:
            /* if all the rest are idle and at least one is ready, then
               status is "ready". */
            if (status == DIRCACHE_IDLE)
                status = DIRCACHE_READY;

            /* sum the build ticks of all "ready" volumes */
            info->build_ticks += dcvolp->build_ticks;
            break;
        case DIRCACHE_IDLE:
            /* if all are idle; then the whole cache is "idle" */
            break;
        }
    }

    info->status     = status;
    info->statusdesc = status_descriptions[status];
    info->last_size  = dircache.last_size;
    info->size_limit = DIRCACHE_LIMIT;
    info->reserve    = DIRCACHE_RESERVE;

    /* report usage only if there is something ready or being built */
    if (status != DIRCACHE_IDLE)
    {
        info->size         = dircache.size;
        info->sizeused     = dircache.sizeused;
        info->reserve_used = reserve_buf_used();
        info->entry_count  = dircache.numentries;
    }
    else
    {
        info->size         = 0;
        info->sizeused     = 0;
        info->reserve_used = 0;
        info->entry_count  = 0;
    }

    dircache_unlock();
}

#ifdef DIRCACHE_DUMPSTER
/**
 * dump RAW binary of buffer and CSV of all valid paths and volumes to disk
 */
void dircache_dump(void)
{
    /* open both now so they're in the cache */
    int fdbin = open(DIRCACHE_DUMPSTER_BIN, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    int fdcsv = open(DIRCACHE_DUMPSTER_CSV, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fdbin < 0 || fdcsv < 0)
    {
        close(fdbin);
        return;
    }

    trigger_cpu_boost();
    dircache_lock();

    if (dircache_runinfo.handle)
    {
        buffer_lock();

        /* bin */
        write(fdbin, dircache_runinfo.p + ENTRYSIZE,
              dircache_runinfo.bufsize + 1);

        /* CSV */
        fdprintf(fdcsv, "\"Index\",\"Serialnum\",\"Serialhash\","
                        "\"Path\",\"Frontier\","
                        "\"Attribute\",\"File Size\","
                        "\"Mod Date\",\"Mod Time\"\n");

        FOR_EACH_VOLUME(-1, volume)
        {
            struct dircache_volume *dcvolp = DCVOL(volume);
            if (dcvolp->status == DIRCACHE_IDLE)
                continue;

        #ifdef HAVE_MULTIVOLUME
            char name[VOL_MAX_LEN+1];
            get_volume_name(volume, name);
        #endif
            fdprintf(fdcsv,
                     "%d," DC_SERIAL_FMT "," DC_SERIAL_FMT ","
                     "\"%c" IF_MV("%s") "\",%u,"
                     "0x%08X,0,"
                     "\"\",\"\"\n",
                     -volume-1, dcvolp->serialnum,
                         dc_hash_serialnum(dcvolp->serialnum, DC_SERHASH_START), 
                     PATH_SEPCH, IF_MV(name,) dcvolp->frontier,
                     ATTR_DIRECTORY | ATTR_VOLUME);
        }

        FOR_EACH_CACHE_ENTRY(ce)
        {
        #ifdef DIRCACHE_NATIVE
            time_t mtime = fattime_mktime(ce->wrtdate, ce->wrttime);
        #else
            time_t mtime = ce->mtime;
        #endif
            struct tm tm;
            gmtime_r(&mtime, &tm);

            char buf[DC_MAX_NAME + 2];
            *buf = '\0';
            int idx = get_index(ce);

            struct get_path_sub_data data =
            {
                .buf        = buf,
                .size       = sizeof (buf),
                .serialhash = DC_SERHASH_START,
            };

            get_path_sub(idx, &data);

            fdprintf(fdcsv,
                     "%d," DC_SERIAL_FMT "," DC_SERIAL_FMT ","
                     "\"%s\",%u,"
                     "0x%08X,%lu,"
                     "%04d/%02d/%02d,"
                     "%02d:%02d:%02d\n",
                     idx, ce->serialnum, data.serialhash,
                     buf, ce->frontier,
                     ce->attr, (ce->attr & ATTR_DIRECTORY) ?
                                0ul : (unsigned long)ce->filesize,
                     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
        }

        buffer_unlock();
    }

    dircache_unlock();
    cancel_cpu_boost();

    close(fdbin);
    close(fdcsv);
}
#endif /* DIRCACHE_DUMPSTER */


/** Misc. stuff **/

/**
 * set the dircache file to initial values
 */
void dircache_dcfile_init(struct dircache_file *dcfilep)
{
    dcfilep->idx       = 0;
    dcfilep->serialnum = 0;
}

#ifdef HAVE_EEPROM_SETTINGS

#ifdef HAVE_HOTSWAP
/* NOTE: This is hazardous to the filesystem of any sort of removable
         storage unless it may be determined that the filesystem from save
         to load is identical. If it's not possible to do so in a timely
         manner, it's not worth persisting the cache. */
  #warning "Don't do this; you'll find the consequences unpleasant."
#endif

/* dircache persistence file header magic */
#define DIRCACHE_MAGIC  0x00d0c0a1

/* dircache persistence file header */
struct dircache_maindata
{
    uint32_t        magic;      /* DIRCACHE_MAGIC */
    struct dircache dircache;   /* metadata of the cache! */
    uint32_t        datacrc;    /* CRC32 of data */
    uint32_t        hdrcrc;     /* CRC32 of header through datacrc */
} __attribute__((packed, aligned (4)));

/**
 * verify that the clean status is A-ok
 */
static bool dircache_is_clean(bool saving)
{
    if (saving)
        return dircache.dcvol[0].status == DIRCACHE_READY;
    else
    {
        return dircache.dcvol[0].status == DIRCACHE_IDLE &&
               !dircache_runinfo.enabled;
    }
}

/**
 * function to load the internal cache structure from disk to initialize
 * the dircache really fast with little disk access.
 */
int dircache_load(void)
{
    logf("Loading directory cache");
    int fd = open_dircache_file(O_RDONLY);
    if (fd < 0)
        return -1;

    int rc = -1;

    ssize_t size;
    struct dircache_maindata maindata;
    uint32_t crc;
    int handle = 0;
    bool hasbuffer = false;

    size = sizeof (maindata);
    if (read(fd, &maindata, size) != size)
    {
        logf("dircache: header read failed");
        goto error_nolock;
    }

    /* sanity check the header */
    if (maindata.magic != DIRCACHE_MAGIC)
    {
        logf("dircache: invalid header magic");
        goto error_nolock;
    }

    crc = crc_32(&maindata, offsetof(struct dircache_maindata, hdrcrc),
                 0xffffffff);
    if (crc != maindata.hdrcrc)
    {
        logf("dircache: invalid header CRC32");
        goto error_nolock;
    }

    if (maindata.dircache.size !=
            maindata.dircache.sizeentries + maindata.dircache.sizenames ||
        ALIGN_DOWN(maindata.dircache.size, ENTRYSIZE) != maindata.dircache.size ||
        filesize(fd) - sizeof (maindata) != maindata.dircache.size)
    {
        logf("dircache: file header error");
        goto error_nolock;
    }

    /* allocate so that exactly the reserve size remains */
    size_t bufsize = maindata.dircache.size + DIRCACHE_RESERVE + 1;
    handle = alloc_cache(bufsize);
    if (handle <= 0)
    {
        logf("dircache: failed load allocation");
        goto error_nolock;
    }

    dircache_lock();
    buffer_lock();

    if (!dircache_is_clean(false))
        goto error;

    /* from this point on, we're actually dealing with the cache in RAM */
    dircache = maindata.dircache;

    set_buffer(handle, bufsize);
    hasbuffer = true;

    /* convert back to in-RAM representation */
    dircache.numentries = maindata.dircache.sizeentries / ENTRYSIZE;

    /* read the dircache file into memory; start with the entries */
    size = maindata.dircache.sizeentries;
    if (read(fd, dircache_runinfo.pentry + 1, size) != size)
    {
        logf("dircache read failed #1");
        goto error;
    }

    crc = crc_32(dircache_runinfo.pentry + 1, size, 0xffffffff);

    /* continue with the names; fix up indexes to them if needed */
    dircache.names -= maindata.dircache.sizenames;
    *get_name(dircache.names - 1) = 0;

    size = maindata.dircache.sizenames;
    if (read(fd, get_name(dircache.names), size) != size)
    {
        logf("dircache read failed #2");
        goto error;
    }

    crc = crc_32(get_name(dircache.names), size, crc);
    if (crc != maindata.datacrc)
    {
        logf("dircache: data failed CRC32");
        goto error;
    }

    /* only names will be changed in relative position so fix up those
       references */
    ssize_t offset = dircache.names - maindata.dircache.names;
    if (offset != 0)
    {
        /* nothing should be open besides the dircache file itself therefore
           no bindings need be resolved; the cache will have its own entry
           but that should get cleaned up when removing the file */
        FOR_EACH_CACHE_ENTRY(ce)
        {
            if (!ce->tinyname)
                ce->name += offset;
        }
    }

    dircache.reserve_used = 0;

    /* enable the cache but do not try to build it */
    dircache_enable_internal(false);

    /* cache successfully loaded */
    logf("Done, %ld KiB used", dircache.size / 1024);
    rc = 0;
error:
    if (rc < 0 && hasbuffer)
        reset_buffer();

    buffer_unlock();
    dircache_unlock();

error_nolock:
    if (rc < 0 && handle > 0)
        core_free(handle);

    if (fd >= 0)
        close(fd);

    remove_dircache_file();
    return rc;
}

/**
 * function to save the internal cache stucture to disk for fast loading
 * on boot
 */
int dircache_save(void)
{
    logf("Saving directory cache");

    int fd = open_dircache_file(O_WRONLY|O_CREAT|O_TRUNC|O_APPEND);
    if (fd < 0)
        return -1;

    dircache_lock();
    buffer_lock();

    int rc = -1;

    if (!dircache_is_clean(true))
        goto error;

    /* save the header structure along with the cache metadata */
    ssize_t size;
    uint32_t crc;
    struct dircache_maindata maindata =
    {
        .magic    = DIRCACHE_MAGIC,
        .dircache = dircache,
    };

    /* store the size since it better detects an invalid header */
    maindata.dircache.sizeentries = maindata.dircache.numentries * ENTRYSIZE;

    /* write the template header */
    size = sizeof (maindata);
    if (write(fd, &maindata, size) != size)
    {
        logf("dircache: write failed #1");
        goto error;
    }

    /* write the dircache entries */
    size = maindata.dircache.sizeentries;
    if (write(fd, dircache_runinfo.pentry + 1, size) != size)
    {
        logf("dircache: write failed #2");
        goto error;
    }

    crc = crc_32(dircache_runinfo.pentry + 1, size, 0xffffffff);

    /* continue with the names */
    size = maindata.dircache.sizenames;
    if (write(fd, get_name(dircache.names), size) != size)
    {
        logf("dircache: write failed #3");
        goto error;
    }

    crc = crc_32(get_name(dircache.names), size, crc);
    maindata.datacrc = crc;

    /* rewrite the header with CRC info */
    if (lseek(fd, 0, SEEK_SET) != 0)
    {
        logf("dircache: seek failed");
        goto error;
    }

    crc = crc_32(&maindata, offsetof(struct dircache_maindata, hdrcrc),
                 0xffffffff);
    maindata.hdrcrc = crc;

    if (write(fd, &maindata, sizeof (maindata)) != sizeof (maindata))
    {
        logf("dircache: write failed #4");
        goto error;
    }

    /* as of now, no changes to the volumes should be allowed at all since
       that makes what was saved completely invalid */
    rc = 0;
error:
    buffer_unlock();
    dircache_unlock();

    if (rc < 0)
        remove_dircache_file();

    close(fd);
    return rc;
}
#endif /* HAVE_EEPROM_SETTINGS */

/**
 * main one-time initialization function that must be called before any other
 * operations within the dircache
 */
void dircache_init(size_t last_size)
{
    queue_init(&dircache_queue, false);

    dircache.last_size = MIN(last_size, DIRCACHE_LIMIT);

    struct dircache_runinfo *dcrip = &dircache_runinfo;
    dcrip->suspended         = 1;
    dcrip->thread_done       = true;
    dcrip->ops.move_callback = move_callback;
}
