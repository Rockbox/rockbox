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

/* TODO:
   - Allow cache live updating while transparent rebuild is running.
*/

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "debug.h"
#include "system.h"
#include "logf.h"
#include "dircache.h"
#include "thread.h"
#include "kernel.h"
#include "usb.h"
#include "file.h"
#include "buffer.h"
#include "dir.h"
#if CONFIG_RTC
#include "time.h"
#include "timefuncs.h"
#endif

/* Queue commands. */
#define DIRCACHE_BUILD 1
#define DIRCACHE_STOP  2

#if ((defined(MEMORYSIZE) && (MEMORYSIZE > 8)) || MEM > 8)
#define MAX_OPEN_DIRS 12
#else
#define MAX_OPEN_DIRS 8
#endif
static DIR_CACHED opendirs[MAX_OPEN_DIRS];

static struct dircache_entry *fd_bindings[MAX_OPEN_FILES];
static struct dircache_entry *dircache_root;
#ifdef HAVE_MULTIVOLUME
static struct dircache_entry *append_position;
#endif

static bool dircache_initialized = false;
static bool dircache_initializing = false;
static bool thread_enabled = false;
static unsigned long allocated_size = DIRCACHE_LIMIT;
static unsigned long dircache_size = 0;
static unsigned long entry_count = 0;
static unsigned long reserve_used = 0;
static unsigned int  cache_build_ticks = 0;
static unsigned long appflags = 0;

static struct event_queue dircache_queue;
static long dircache_stack[(DEFAULT_STACK_SIZE + 0x900)/sizeof(long)];
static const char dircache_thread_name[] = "dircache";

static struct fdbind_queue fdbind_cache[MAX_PENDING_BINDINGS];
static int fdbind_idx = 0;

/* --- Internal cache structure control functions --- */

/** 
 * Internal function to allocate a new dircache_entry from memory.
 */
static struct dircache_entry* allocate_entry(void)
{
    struct dircache_entry *next_entry;
    
    if (dircache_size > allocated_size - MAX_PATH*2)
    {
        logf("size limit reached");
        return NULL;
    }
    
    next_entry = (struct dircache_entry *)((char *)dircache_root+dircache_size);
#ifdef ROCKBOX_STRICT_ALIGN
    /* Make sure the entry is long aligned. */
    if ((long)next_entry & 0x03)
    {
        dircache_size += 4 - ((long)next_entry & 0x03);
        next_entry = (struct dircache_entry *)(((long)next_entry & ~0x03) + 0x04);
    }
#endif
    next_entry->name_len = 0;
    next_entry->d_name = NULL;
    next_entry->up = NULL;
    next_entry->down = NULL;
    next_entry->next = NULL;
    
    dircache_size += sizeof(struct dircache_entry);

    return next_entry;
}

/**
 * Internal function to allocate a dircache_entry and set 
 * ->next entry pointers.
 */
static struct dircache_entry* dircache_gen_next(struct dircache_entry *ce)
{
    struct dircache_entry *next_entry;

    if ( (next_entry = allocate_entry()) == NULL)
        return NULL;
    next_entry->up = ce->up;
    ce->next = next_entry;
    
    return next_entry;
}

/*
 * Internal function to allocate a dircache_entry and set
 * ->down entry pointers.
 */
static struct dircache_entry* dircache_gen_down(struct dircache_entry *ce)
{
    struct dircache_entry *next_entry;

    if ( (next_entry = allocate_entry()) == NULL)
        return NULL;
    next_entry->up = ce;
    ce->down = next_entry;
    
    return next_entry;
}

/**
 * Returns true if there is an event waiting in the queue
 * that requires the current operation to be aborted.
 */
static bool check_event_queue(void)
{
    struct queue_event ev;
    
    queue_wait_w_tmo(&dircache_queue, &ev, 0);
    switch (ev.id)
    {
        case DIRCACHE_STOP:
        case SYS_USB_CONNECTED:
#ifdef HAVE_HOTSWAP
        case SYS_FS_CHANGED:
#endif
            /* Put the event back into the queue. */
            queue_post(&dircache_queue, ev.id, ev.data);
            return true;
    }
    
    return false;
}

#ifndef SIMULATOR
/* scan and build static data (avoid redundancy on stack) */
static struct
{
#ifdef HAVE_MULTIVOLUME
    int volume;
#endif
    struct fat_dir *dir;
    struct fat_direntry *direntry;
}sab;

static int sab_process_dir(unsigned long startcluster, struct dircache_entry *ce)
{
    /* normally, opendir expects a full fat_dir as parent but in our case,
     * it's completely useless because we don't modify anything
     * WARNING: this heavily relies on current FAT implementation ! */
    
    /* those field are necessary to update the FAT entry in case of modification
       here we don't touch anything so we put dummy values */
    sab.dir->entry = 0;
    sab.dir->entrycount = 0;
    sab.dir->file.firstcluster = 0;
    /* open directory */
    int rc = fat_opendir(IF_MV2(sab.volume,) sab.dir, startcluster, sab.dir);
    if(rc < 0)
    {
        logf("fat_opendir failed: %d", rc);
        return rc;
    }
    
    /* first pass : read dir */
    struct dircache_entry *first_ce = ce;
    
    /* read through directory */
    while((rc = fat_getnext(sab.dir, sab.direntry)) >= 0 && sab.direntry->name[0])
    {
        if(!strcmp(".", sab.direntry->name) ||
                !strcmp("..", sab.direntry->name))
            continue;
        
        ce->attribute = sab.direntry->attr;
        ce->name_len = strlen(sab.direntry->name) + 1;
        ce->d_name = ((char *)dircache_root + dircache_size);
        ce->startcluster = sab.direntry->firstcluster;
        ce->size = sab.direntry->filesize;
        ce->wrtdate = sab.direntry->wrtdate;
        ce->wrttime = sab.direntry->wrttime;
        memcpy(ce->d_name, sab.direntry->name, ce->name_len);
        
        dircache_size += ce->name_len;
        entry_count++;
        
        if(ce->attribute & FAT_ATTR_DIRECTORY)
            dircache_gen_down(ce);
                
        ce = dircache_gen_next(ce);
        if(ce == NULL)
            return -5;
        
        /* When simulator is used, it's only safe to yield here. */
        if(thread_enabled)
        {
            /* Stop if we got an external signal. */
            if(check_event_queue())
                return -6;
            yield();
        }
    }
    
    /* add "." and ".." */
    ce->d_name = ".";
    ce->name_len = 2;
    ce->attribute = FAT_ATTR_DIRECTORY;
    ce->startcluster = startcluster;
    ce->size = 0;
    ce->down = first_ce;
    
    ce = dircache_gen_next(ce);
    
    ce->d_name = "..";
    ce->name_len = 3;
    ce->attribute = FAT_ATTR_DIRECTORY;
    ce->startcluster = first_ce->up->startcluster;
    ce->size = 0;
    ce->down = first_ce->up;
    
    /* second pass: recurse ! */
    ce = first_ce;
    
    while(ce)
    {
        if(ce->name_len != 0 && ce->down != NULL && strcmp(ce->d_name, ".") && strcmp(ce->d_name, ".."))
            sab_process_dir(ce->startcluster, ce->down);
        
        ce = ce->next;
    }
    
    return rc;
}

static int dircache_scan_and_build(IF_MV2(int volume,) struct dircache_entry *ce)
{
    memset(ce, 0, sizeof(struct dircache_entry));

#ifdef HAVE_MULTIVOLUME
    if (volume > 0)
    {
        ce->d_name = ((char *)dircache_root+dircache_size);
        snprintf(ce->d_name, VOL_ENUM_POS + 3, VOL_NAMES, volume);
        ce->name_len = VOL_ENUM_POS + 3;
        dircache_size += ce->name_len;
        ce->attribute = FAT_ATTR_DIRECTORY | FAT_ATTR_VOLUME;
        ce->size = 0;
        append_position = dircache_gen_next(ce);
        ce = dircache_gen_down(ce);
    }
#endif

    struct fat_dir dir; /* allocate on stack once for all scan */
    struct fat_direntry direntry; /* ditto */
#ifdef HAVE_MULTIVOLUME
    sab.volume = volume;
#endif
    sab.dir = &dir;
    sab.direntry = &direntry;
    
    return sab_process_dir(0, ce);
}
#else /* !SIMULATOR */
static char sab_path[MAX_PATH];

static int sab_process_dir(struct dircache_entry *ce)
{
    struct dirent_uncached *entry;
    struct dircache_entry *first_ce = ce;
    DIR_UNCACHED *dir = opendir_uncached(sab_path);
    if(dir == NULL)
    {
        logf("Failed to opendir_uncached(%s)", sab_path);
        return -1;
    }
    
    while((entry = readdir_uncached(dir)))
    {
        if(!strcmp(".", entry->d_name) ||
                !strcmp("..", entry->d_name))
            continue;
        
        ce->attribute = entry->attribute;
        ce->name_len = strlen(entry->d_name) + 1;
        ce->d_name = ((char *)dircache_root + dircache_size);
        ce->size = entry->size;
        ce->wrtdate = entry->wrtdate;
        ce->wrttime = entry->wrttime;
        memcpy(ce->d_name, entry->d_name, ce->name_len);
        
        dircache_size += ce->name_len;
        entry_count++;
        
        if(entry->attribute & ATTR_DIRECTORY)
        {
            dircache_gen_down(ce);
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
        
        ce = dircache_gen_next(ce);
        if(ce == NULL)
            return -5;
        
        /* When simulator is used, it's only safe to yield here. */
        if(thread_enabled)
        {
            /* Stop if we got an external signal. */
            if(check_event_queue())
                return -1;
            yield();
        }
    }
    
    /* add "." and ".." */
    ce->d_name = ".";
    ce->name_len = 2;
    ce->attribute = ATTR_DIRECTORY;
    ce->size = 0;
    ce->down = first_ce;
    
    ce = dircache_gen_next(ce);
    
    ce->d_name = "..";
    ce->name_len = 3;
    ce->attribute = ATTR_DIRECTORY;
    ce->size = 0;
    ce->down = first_ce->up;
    
    closedir_uncached(dir);
    return 0;
}

static int dircache_scan_and_build(IF_MV2(int volume,) struct dircache_entry *ce)
{
    #ifdef HAVE_MULTIVOLUME
    (void) volume;
    #endif
    memset(ce, 0, sizeof(struct dircache_entry));
    
    strlcpy(sab_path, "/", sizeof sab_path);
    return sab_process_dir(ce);
}
#endif /* SIMULATOR */

/**
 * Internal function to get a pointer to dircache_entry for a given filename.
 *   path: Absolute path to a file or directory (see comment)
 *   go_down: Returns the first entry of the directory given by the path (see comment)
 *
 * As a a special case, accept path="" as an alias for "/".
 * Also if the path omits the first '/', it will be accepted.
 *
 * * If get_down=true:
 *   If path="/", the returned entry is the first of root directory (ie dircache_root)
 *   Otherwise, if 'entry' is the returned value when get_down=false, 
 *   the functions returns entry->down (which can be NULL)
 *
 * * If get_down=false:
 *   If path="/chunk_1/chunk_2/.../chunk_n" then this functions returns the entry
 *   root_entry()->chunk_1->chunk_2->...->chunk_(n-1)
 *   Which means that
 *   dircache_get_entry(path)->d_name == chunk_n
 *
 *   If path="/", the returned entry is NULL.
 *   If the entry doesn't exist, return NULL
 *
 *  NOTE: this functions silently handles double '/'
 */
static struct dircache_entry* dircache_get_entry(const char *path, bool go_down)
{
    char namecopy[MAX_PATH];
    char* part;
    char* end;
    
    bool at_root = true;
    struct dircache_entry *cache_entry = dircache_root;
    
    strlcpy(namecopy, path, sizeof(namecopy));
    
    for(part = strtok_r(namecopy, "/", &end); part; part = strtok_r(NULL, "/", &end))
    {
        /* If request another chunk, the current entry has to be directory
         * and so cache_entry->down has to be non-NULL/
         * Special case of root because it's already the first entry of the root directory
         *
         * NOTE: this is safe even if cache_entry->down is NULL */
        if(!at_root)
            cache_entry = cache_entry->down;
        else
            at_root = false;
        
        /* scan dir for name */
        while(cache_entry != NULL)
        {
            /* skip unused entries */
            if(cache_entry->name_len == 0)
            {
                cache_entry = cache_entry->next;
                continue;
            }
            /* compare names */
            if(!strcasecmp(part, cache_entry->d_name))
                break;
            /* go to next entry */
            cache_entry = cache_entry->next;
        }
        
        /* handle not found case */
        if(cache_entry == NULL)
            return NULL;
    }

    /* NOTE: here cache_entry!=NULL so taking ->down is safe */
    if(go_down)
        return at_root ? cache_entry : cache_entry->down;
    else
        return at_root ? NULL : cache_entry;
}

#ifdef HAVE_EEPROM_SETTINGS
/**
 * Function to load the internal cache structure from disk to initialize
 * the dircache really fast and little disk access.
 */
int dircache_load(void)
{
    struct dircache_maindata maindata;
    int bytes_read;
    int fd;
        
    if (dircache_initialized)
        return -1;
        
    logf("Loading directory cache");
    dircache_size = 0;
    
    fd = open(DIRCACHE_FILE, O_RDONLY);
    if (fd < 0)
        return -2;
        
    bytes_read = read(fd, &maindata, sizeof(struct dircache_maindata));
    if (bytes_read != sizeof(struct dircache_maindata)
        || maindata.size <= 0)
    {
        logf("Dircache file header error");
        close(fd);
        remove(DIRCACHE_FILE);
        return -3;
    }

    dircache_root = buffer_alloc(0);
    if ((long)maindata.root_entry != (long)dircache_root)
    {
        logf("Position missmatch");
        close(fd);
        remove(DIRCACHE_FILE);
        return -4;
    }
    
    dircache_root = buffer_alloc(maindata.size + DIRCACHE_RESERVE);
    entry_count = maindata.entry_count;
    appflags = maindata.appflags;
    bytes_read = read(fd, dircache_root, MIN(DIRCACHE_LIMIT, maindata.size));
    close(fd);
    remove(DIRCACHE_FILE);
    
    if (bytes_read != maindata.size)
    {
        logf("Dircache read failed");
        return -6;
    }

    /* Cache successfully loaded. */
    dircache_size = maindata.size;
    allocated_size = dircache_size + DIRCACHE_RESERVE;
    reserve_used = 0;
    logf("Done, %ld KiB used", dircache_size / 1024);
    dircache_initialized = true;
    memset(fd_bindings, 0, sizeof(fd_bindings));

    return 0;
}

/**
 * Function to save the internal cache stucture to disk for fast loading
 * on boot.
 */
int dircache_save(void)
{
    struct dircache_maindata maindata;
    int fd;
    unsigned long bytes_written;

    remove(DIRCACHE_FILE);
    
    if (!dircache_initialized)
        return -1;

    logf("Saving directory cache");
    fd = open(DIRCACHE_FILE, O_WRONLY | O_CREAT | O_TRUNC);

    maindata.magic = DIRCACHE_MAGIC;
    maindata.size = dircache_size;
    maindata.root_entry = dircache_root;
    maindata.entry_count = entry_count;
    maindata.appflags = appflags;

    /* Save the info structure */
    bytes_written = write(fd, &maindata, sizeof(struct dircache_maindata));
    if (bytes_written != sizeof(struct dircache_maindata))
    {
        close(fd);
        logf("dircache: write failed #1");
        return -2;
    }

    /* Dump whole directory cache to disk */
    bytes_written = write(fd, dircache_root, dircache_size);
    close(fd);
    if (bytes_written != dircache_size)
    {
        logf("dircache: write failed #2");
        return -3;
    }
    
    return 0;
}
#endif /* #if 0 */

/**
 * Internal function which scans the disk and creates the dircache structure.
 */
static int dircache_do_rebuild(void)
{
    unsigned int start_tick;
    int i;
    
    /* Measure how long it takes build the cache. */
    start_tick = current_tick;
    dircache_initializing = true;
    appflags = 0;
    entry_count = 0;
    
    dircache_size = sizeof(struct dircache_entry);

#ifdef HAVE_MULTIVOLUME
    append_position = dircache_root;

    for (i = NUM_VOLUMES; i >= 0; i--)
    {
        if (fat_ismounted(i))
        {
#endif
            cpu_boost(true);
#ifdef HAVE_MULTIVOLUME
            if (dircache_scan_and_build(IF_MV2(i,) append_position) < 0)
#else
            if (dircache_scan_and_build(IF_MV2(0,) dircache_root) < 0)
#endif /* HAVE_MULTIVOLUME */
            {
                logf("dircache_travel failed");
                cpu_boost(false);
                dircache_size = 0;
                dircache_initializing = false;
                return -2;
            }
            cpu_boost(false);
#ifdef HAVE_MULTIVOLUME
        }
    }
#endif

    logf("Done, %ld KiB used", dircache_size / 1024);
    
    dircache_initialized = true;
    dircache_initializing = false;
    cache_build_ticks = current_tick - start_tick;
    
    /* Initialized fd bindings. */
    memset(fd_bindings, 0, sizeof(fd_bindings));
    for (i = 0; i < fdbind_idx; i++)
        dircache_bind(fdbind_cache[i].fd, fdbind_cache[i].path);
    fdbind_idx = 0;
    
    if (thread_enabled)
    {
        if (allocated_size - dircache_size < DIRCACHE_RESERVE)
            reserve_used = DIRCACHE_RESERVE - (allocated_size - dircache_size);
    }
    else
    {
        /* We have to long align the audiobuf to keep the buffer access fast. */
        audiobuf += (long)((dircache_size & ~0x03) + 0x04);
        audiobuf += DIRCACHE_RESERVE;
        allocated_size = dircache_size + DIRCACHE_RESERVE;
        reserve_used = 0;
    }
    
    return 1;
}

/**
 * Internal thread that controls transparent cache building.
 */
static void dircache_thread(void)
{
    struct queue_event ev;

    while (1)
    {
        queue_wait(&dircache_queue, &ev);

        switch (ev.id)
        {
#ifdef HAVE_HOTSWAP
            case SYS_FS_CHANGED:
                if (!dircache_initialized)
                    break;
                dircache_initialized = false;
#endif
            case DIRCACHE_BUILD:
                thread_enabled = true;
                dircache_do_rebuild();
                thread_enabled = false;
                break ;
                
            case DIRCACHE_STOP:
                logf("Stopped the rebuilding.");
                dircache_initialized = false;
                break ;
            
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&dircache_queue);
                break ;
#endif
        }
    }
}

/**
 * Start scanning the disk to build the dircache.
 * Either transparent or non-transparent build method is used.
 */
int dircache_build(int last_size)
{
    if (dircache_initialized || thread_enabled)
        return -3;

    logf("Building directory cache");
    remove(DIRCACHE_FILE);
    
    /* Background build, dircache has been previously allocated */
    if (dircache_size > 0)
    {
        thread_enabled = true;
        dircache_initializing = true;
        queue_post(&dircache_queue, DIRCACHE_BUILD, 0);
        return 2;
    }
    
    if (last_size > DIRCACHE_RESERVE && last_size < DIRCACHE_LIMIT )
    {
        allocated_size = last_size + DIRCACHE_RESERVE;
        dircache_root = buffer_alloc(allocated_size);
        thread_enabled = true;

        /* Start a transparent rebuild. */
        queue_post(&dircache_queue, DIRCACHE_BUILD, 0);
        return 3;
    }

    dircache_root = (struct dircache_entry *)(((long)audiobuf & ~0x03) + 0x04);

    /* Start a non-transparent rebuild. */
    return dircache_do_rebuild();
}

/**
 * Steal the allocated dircache buffer and disable dircache.
 */
void* dircache_steal_buffer(long *size)
{
    dircache_disable();
    if (dircache_size == 0)
    {
        *size = 0;
        return NULL;
    }
    
    *size = dircache_size + (DIRCACHE_RESERVE-reserve_used);
    
    return dircache_root;
}

/**
 * Main initialization function that must be called before any other
 * operations within the dircache.
 */
void dircache_init(void)
{
    int i;
    
    dircache_initialized = false;
    dircache_initializing = false;
    
    memset(opendirs, 0, sizeof(opendirs));
    for (i = 0; i < MAX_OPEN_DIRS; i++)
    {
        opendirs[i].secondary_entry.d_name = buffer_alloc(MAX_PATH);
    }
    
    queue_init(&dircache_queue, true);
    create_thread(dircache_thread, dircache_stack,
                sizeof(dircache_stack), 0, dircache_thread_name
                IF_PRIO(, PRIORITY_BACKGROUND)
                IF_COP(, CPU));
}

/**
 * Returns true if dircache has been initialized and is ready to be used.
 */
bool dircache_is_enabled(void)
{
    return dircache_initialized;
}

/**
 * Returns true if dircache is being initialized.
 */
bool dircache_is_initializing(void)
{
    return dircache_initializing || thread_enabled;
}

/**
 * Set application flags used to determine if dircache is still intact.
 */
void dircache_set_appflag(long mask)
{
    appflags |= mask;
}

/**
 * Get application flags used to determine if dircache is still intact.
 */
bool dircache_get_appflag(long mask)
{
    return dircache_is_enabled() && (appflags & mask);
}

/**
 * Returns the current number of entries (directories and files) in the cache.
 */
int dircache_get_entry_count(void)
{
    return entry_count;
}

/**
 * Returns the allocated space for dircache (without reserve space).
 */
int dircache_get_cache_size(void)
{
    return dircache_is_enabled() ? dircache_size : 0;
}

/**
 * Returns how many bytes of the reserve allocation for live cache
 * updates have been used.
 */
int dircache_get_reserve_used(void)
{
    return dircache_is_enabled() ? reserve_used : 0;
}

/**
 * Returns the time in kernel ticks that took to build the cache.
 */
int dircache_get_build_ticks(void)
{
    return dircache_is_enabled() ? cache_build_ticks : 0;
}

/**
 * Disables the dircache. Usually called on shutdown or when
 * accepting a usb connection.
 */
void dircache_disable(void)
{
    int i;
    bool cache_in_use;
    
    if (thread_enabled)
        queue_post(&dircache_queue, DIRCACHE_STOP, 0);
    
    while (thread_enabled)
        sleep(1);
    dircache_initialized = false;

    logf("Waiting for cached dirs to release");
    do {
        cache_in_use = false;
        for (i = 0; i < MAX_OPEN_DIRS; i++) {
            if (!opendirs[i].regulardir && opendirs[i].busy)
            {
                cache_in_use = true;
                sleep(1);
                break ;
            }
        }
    } while (cache_in_use) ;
    
    logf("Cache released");
    entry_count = 0;
}

/**
 * Usermode function to return dircache_entry pointer to the given path.
 */
const struct dircache_entry *dircache_get_entry_ptr(const char *filename)
{
    if (!dircache_initialized || filename == NULL)
        return NULL;
    
    return dircache_get_entry(filename, false);
}

/**
 * Function to copy the full absolute path from dircache to the given buffer
 * using the given dircache_entry pointer.
 */
void dircache_copy_path(const struct dircache_entry *entry, char *buf, int size)
{
    int path_size = 0;
    int idx;
    const struct dircache_entry *temp = entry;
    
    if (size <= 0)
        return ;
        
    /* first compute the necessary size */
    while(temp != NULL)
    {
        path_size += temp->name_len; /* '/' + d_name */
        temp = temp->up;
    }
    
    /* now copy the path */
    /* doesn't matter with trailing 0 because it's added later */
    idx = path_size;
    while(entry != NULL)
    {
        idx -= entry->name_len;
        /* available size */
        int rem = size - idx;
        
        if(rem >= 1)
        {
            buf[idx] = '/';
            memcpy(buf + idx + 1, entry->d_name, MIN((size_t)(rem - 1), (size_t)(entry->name_len - 1)));
        }
        entry = entry->up;
    }
    
    /* add trailing 0 */
    buf[MIN(path_size, size-1)] = 0;
}

/* --- Directory cache live updating functions --- */
static int block_until_ready(void)
{
    /* Block until dircache has been built. */
    while (!dircache_initialized && dircache_is_initializing())
        sleep(1);
    
    if (!dircache_initialized)
        return -1;
    
    return 0;
}

static struct dircache_entry* dircache_new_entry(const char *path, int attribute)
{
    struct dircache_entry *entry;
    char basedir[MAX_PATH*2];
    char *new;
    long last_cache_size = dircache_size;

    strlcpy(basedir, path, sizeof(basedir));
    new = strrchr(basedir, '/');
    if (new == NULL)
    {
        logf("error occurred");
        dircache_initialized = false;
        return NULL;
    }

    *new = '\0';
    new++;

    entry = dircache_get_entry(basedir, true);
    if (entry == NULL)
    {
        logf("basedir not found!");
        logf("%s", basedir);
        dircache_initialized = false;
        return NULL;
    }

    if (reserve_used + 2*sizeof(struct dircache_entry) + strlen(new)+1
        >= DIRCACHE_RESERVE)
    {
        logf("not enough space");
        dircache_initialized = false;
        return NULL;
    }
    
    while (entry->next != NULL)
        entry = entry->next;

    if (entry->name_len > 0)
        entry = dircache_gen_next(entry);

    if (entry == NULL)
    {
        dircache_initialized = false;
        return NULL;
    }
        
    entry->attribute = attribute;
    entry->name_len = MIN(254, strlen(new)) + 1;
    entry->d_name = ((char *)dircache_root+dircache_size);
    entry->startcluster = 0;
    entry->wrtdate = 0;
    entry->wrttime = 0;
    entry->size = 0;
    memcpy(entry->d_name, new, entry->name_len);
    dircache_size += entry->name_len;

    if (attribute & ATTR_DIRECTORY)
    {
        logf("gen_down");
        dircache_gen_down(entry);
    }
        
    reserve_used += dircache_size - last_cache_size;

    return entry;
}

void dircache_bind(int fd, const char *path)
{
    struct dircache_entry *entry;
    
    /* Queue requests until dircache has been built. */
    if (!dircache_initialized && dircache_is_initializing())
    {
        if (fdbind_idx >= MAX_PENDING_BINDINGS)
            return ;
        strlcpy(fdbind_cache[fdbind_idx].path, path, 
                sizeof(fdbind_cache[fdbind_idx].path));
        fdbind_cache[fdbind_idx].fd = fd;
        fdbind_idx++;
        return ;
    }
    
    if (!dircache_initialized)
        return ;

    logf("bind: %d/%s", fd, path);
    entry = dircache_get_entry(path, false);
    if (entry == NULL)
    {
        logf("not found!");
        dircache_initialized = false;
        return ;
    }

    fd_bindings[fd] = entry;
}

void dircache_update_filesize(int fd, long newsize, long startcluster)
{
    if (!dircache_initialized || fd < 0)
        return ;

    if (fd_bindings[fd] == NULL)
    {
        logf("dircache fd(%d) access error", fd);
        dircache_initialized = false;
        return ;
    }
    
    fd_bindings[fd]->size = newsize;
    fd_bindings[fd]->startcluster = startcluster;
}
void dircache_update_filetime(int fd)
{
#if CONFIG_RTC == 0
    (void)fd;
#else
    short year;
    struct tm *now = get_time();
    if (!dircache_initialized || fd < 0)
        return ;

    if (fd_bindings[fd] == NULL)
    {
        logf("dircache fd access error");
        dircache_initialized = false;
        return ;
    }
    year = now->tm_year+1900-1980;
    fd_bindings[fd]->wrtdate = (((year)&0x7f)<<9)           |
                               (((now->tm_mon+1)&0xf)<<5)   |
                               (((now->tm_mday)&0x1f));
    fd_bindings[fd]->wrttime = (((now->tm_hour)&0x1f)<<11)  |
                               (((now->tm_min)&0x3f)<<5)    |
                               (((now->tm_sec/2)&0x1f));
#endif
}

void dircache_mkdir(const char *path)
{ /* Test ok. */
    if (block_until_ready())
        return ;
        
    logf("mkdir: %s", path);
    dircache_new_entry(path, ATTR_DIRECTORY);
}

void dircache_rmdir(const char *path)
{ /* Test ok. */
    struct dircache_entry *entry;
    
    if (block_until_ready())
        return ;
        
    logf("rmdir: %s", path);
    entry = dircache_get_entry(path, false);
    if (entry == NULL || entry->down == NULL)
    {
        logf("not found or not a directory!");
        dircache_initialized = false;
        return ;
    }

    entry->down = NULL;
    entry->name_len = 0;
}

/* Remove a file from cache */
void dircache_remove(const char *name)
{ /* Test ok. */
    struct dircache_entry *entry;
    
    if (block_until_ready())
        return ;
        
    logf("remove: %s", name);
    
    entry = dircache_get_entry(name, false);

    if (entry == NULL)
    {
        logf("not found!");
        dircache_initialized = false;
        return ;
    }
    
    entry->name_len = 0;
}

void dircache_rename(const char *oldpath, const char *newpath)
{ /* Test ok. */
    struct dircache_entry *entry, *newentry;
    struct dircache_entry oldentry;
    char absolute_path[MAX_PATH*2];
    char *p;
    
    if (block_until_ready())
        return ;
        
    logf("rename: %s->%s", oldpath, newpath);
    
    entry = dircache_get_entry(oldpath, false);
    if (entry == NULL)
    {
        logf("not found!");
        dircache_initialized = false;
        return ;
    }

    /* Delete the old entry. */
    entry->name_len = 0;

    /** If we rename the same filename twice in a row, we need to
     * save the data, because the entry will be re-used. */
    oldentry = *entry;

    /* Generate the absolute path for destination if necessary. */
    if (newpath[0] != '/')
    {
        strlcpy(absolute_path, oldpath, sizeof(absolute_path));
        p = strrchr(absolute_path, '/');
        if (!p)
        {
            logf("Invalid path");
            dircache_initialized = false;
            return ;
        }
        
        *p = '\0';
        strlcpy(p, absolute_path, sizeof(absolute_path)-strlen(p));
        newpath = absolute_path;
    }
    
    newentry = dircache_new_entry(newpath, entry->attribute);
    if (newentry == NULL)
    {
        dircache_initialized = false;
        return ;
    }

    newentry->down = oldentry.down;
    newentry->size = oldentry.size;
    newentry->startcluster = oldentry.startcluster;
    newentry->wrttime = oldentry.wrttime;
    newentry->wrtdate = oldentry.wrtdate;
}

void dircache_add_file(const char *path, long startcluster)
{
    struct dircache_entry *entry;
    
    if (block_until_ready())
        return ;
    
    logf("add file: %s", path);
    entry = dircache_new_entry(path, 0);
    if (entry == NULL)
        return ;
    
    entry->startcluster = startcluster;
}

DIR_CACHED* opendir_cached(const char* name)
{
    struct dircache_entry *cache_entry;
    int dd;
    DIR_CACHED* pdir = opendirs;

    if ( name[0] != '/' )
    {
        DEBUGF("Only absolute paths supported right now\n");
        return NULL;
    }

    /* find a free dir descriptor */
    for ( dd=0; dd<MAX_OPEN_DIRS; dd++, pdir++)
        if ( !pdir->busy )
            break;

    if ( dd == MAX_OPEN_DIRS )
    {
        DEBUGF("Too many dirs open\n");
        errno = EMFILE;
        return NULL;
    }

    if (!dircache_initialized)
    {
        pdir->regulardir = opendir_uncached(name);
        if (!pdir->regulardir)
            return NULL;
        
        pdir->busy = true;
        return pdir;
    }
    
    pdir->busy = true;
    pdir->regulardir = NULL;
    cache_entry = dircache_get_entry(name, true);
    pdir->entry = cache_entry;

    if (cache_entry == NULL)
    {
        pdir->busy = false;
        return NULL;
    }

    return pdir;
}

struct dircache_entry* readdir_cached(DIR_CACHED* dir)
{
    struct dirent_uncached *regentry;
    struct dircache_entry *ce;
    
    if (!dir->busy)
        return NULL;

    if (dir->regulardir != NULL)
    {
        regentry = readdir_uncached(dir->regulardir);
        if (regentry == NULL)
            return NULL;

        strlcpy(dir->secondary_entry.d_name, regentry->d_name, MAX_PATH);
        dir->secondary_entry.size = regentry->size;
        dir->secondary_entry.startcluster = regentry->startcluster;
        dir->secondary_entry.attribute = regentry->attribute;
        dir->secondary_entry.wrttime = regentry->wrttime;
        dir->secondary_entry.wrtdate = regentry->wrtdate;
        dir->secondary_entry.next = NULL;
        
        return &dir->secondary_entry;
    }
        
    do {
        if (dir->entry == NULL)
            return NULL;

        ce = dir->entry;
        if (ce->name_len == 0)
            dir->entry = ce->next;
    } while (ce->name_len == 0) ;

    dir->entry = ce->next;

    strlcpy(dir->secondary_entry.d_name, ce->d_name, MAX_PATH);
    /* Can't do `dir->secondary_entry = *ce`
       because that modifies the d_name pointer. */
    dir->secondary_entry.size = ce->size;
    dir->secondary_entry.startcluster = ce->startcluster;
    dir->secondary_entry.attribute = ce->attribute;
    dir->secondary_entry.wrttime = ce->wrttime;
    dir->secondary_entry.wrtdate = ce->wrtdate;
    dir->secondary_entry.next = NULL;
    dir->internal_entry = ce;

    //logf("-> %s", ce->name);
    return &dir->secondary_entry;
}

int closedir_cached(DIR_CACHED* dir)
{
    if (!dir->busy)
        return -1;
        
    dir->busy=false;
    if (dir->regulardir != NULL)
        return closedir_uncached(dir->regulardir);
    
    return 0;
}

int mkdir_cached(const char *name)
{
    int rc=mkdir_uncached(name);
    if (rc >= 0)
        dircache_mkdir(name);
    return(rc);
}

int rmdir_cached(const char* name)
{
    int rc=rmdir_uncached(name);
    if(rc>=0)
        dircache_rmdir(name);
    return(rc);
}
