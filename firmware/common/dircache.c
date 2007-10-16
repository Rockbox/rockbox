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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "debug.h"
#include "atoi.h"
#include "system.h"
#include "logf.h"
#include "dircache.h"
#include "thread.h"
#include "kernel.h"
#include "usb.h"
#include "file.h"
#include "buffer.h"
#if CONFIG_RTC
#include "time.h"
#include "timefuncs.h"
#endif

/* Queue commands. */
#define DIRCACHE_BUILD 1
#define DIRCACHE_STOP  2

#define MAX_OPEN_DIRS 8
DIR_CACHED opendirs[MAX_OPEN_DIRS];

static struct dircache_entry *fd_bindings[MAX_OPEN_FILES];
static struct dircache_entry *dircache_root;

static bool dircache_initialized = false;
static bool dircache_initializing = false;
static bool thread_enabled = false;
static unsigned long allocated_size = DIRCACHE_LIMIT;
static unsigned long dircache_size = 0;
static unsigned long entry_count = 0;
static unsigned long reserve_used = 0;
static unsigned int  cache_build_ticks = 0;
static char dircache_cur_path[MAX_PATH*2];

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

/* This will eat ~30 KiB of memory!
 * We should probably use that as additional reserve buffer in future. */
#define MAX_SCAN_DEPTH 16
static struct travel_data dir_recursion[MAX_SCAN_DEPTH];

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
            /* Put the event back into the queue. */
            queue_post(&dircache_queue, ev.id, ev.data);
            return true;
    }
    
    return false;
}

/**
 * Internal function to iterate a path.
 */
static int dircache_scan(struct travel_data *td)
{
#ifdef SIMULATOR
    while ( ( td->entry = readdir_uncached(td->dir) ) )
#else
    while ( (fat_getnext(td->dir, &td->entry) >= 0) && (td->entry.name[0]))
#endif
    {
#ifdef SIMULATOR
        if (!strcmp(".", td->entry->d_name) ||
             !strcmp("..", td->entry->d_name))
        {
            continue;
        }
        
        td->ce->attribute = td->entry->attribute;
        td->ce->name_len = strlen(td->entry->d_name) + 1;
        td->ce->d_name = ((char *)dircache_root+dircache_size);
        td->ce->size = td->entry->size;
        td->ce->wrtdate = td->entry->wrtdate;
        td->ce->wrttime = td->entry->wrttime;
        memcpy(td->ce->d_name, td->entry->d_name, td->ce->name_len);
#else
        if (!strcmp(".", td->entry.name) ||
             !strcmp("..", td->entry.name))
        {
            continue;
        }
        
        td->ce->attribute = td->entry.attr;
        td->ce->name_len = strlen(td->entry.name) + 1;
        td->ce->d_name = ((char *)dircache_root+dircache_size);
        td->ce->startcluster = td->entry.firstcluster;
        td->ce->size = td->entry.filesize;
        td->ce->wrtdate = td->entry.wrtdate;
        td->ce->wrttime = td->entry.wrttime;
        memcpy(td->ce->d_name, td->entry.name, td->ce->name_len);
#endif
        dircache_size += td->ce->name_len;
        entry_count++;
        
#ifdef SIMULATOR
        if (td->entry->attribute & ATTR_DIRECTORY)
#else
        if (td->entry.attr & FAT_ATTR_DIRECTORY)
#endif
        {
            
            td->down_entry = dircache_gen_down(td->ce);
            if (td->down_entry == NULL)
                return -2;
            
            td->pathpos = strlen(dircache_cur_path);
            strncpy(&dircache_cur_path[td->pathpos], "/", 
                    sizeof(dircache_cur_path) - td->pathpos - 1);
#ifdef SIMULATOR
            strncpy(&dircache_cur_path[td->pathpos+1], td->entry->d_name, 
                    sizeof(dircache_cur_path) - td->pathpos - 2);
            
            td->newdir = opendir_uncached(dircache_cur_path);
            if (td->newdir == NULL)
            {
                logf("Failed to opendir_uncached(): %s", dircache_cur_path);
                return -3;
            }
#else
            strncpy(&dircache_cur_path[td->pathpos+1], td->entry.name,
                    sizeof(dircache_cur_path) - td->pathpos - 2);

            td->newdir = *td->dir;
            if (fat_opendir(IF_MV2(volume,) &td->newdir,
                td->entry.firstcluster, td->dir) < 0 )
            {
                return -3;
            }
#endif

            td->ce = dircache_gen_next(td->ce);
            if (td->ce == NULL)
                return -4;
            
            return 1;
        }
        
        td->ce->down = NULL;
        td->ce = dircache_gen_next(td->ce);
        if (td->ce == NULL)
            return -5;
        
        /* When simulator is used, it's only safe to yield here. */
        if (thread_enabled)
        {
            /* Stop if we got an external signal. */
            if (check_event_queue())
                return -6;
            yield();
        }
        
    }

    return 0;
}

/** 
 * Recursively scan the hard disk and build the cache.
 */
#ifdef SIMULATOR
static int dircache_travel(DIR_UNCACHED *dir, struct dircache_entry *ce)
#else
static int dircache_travel(struct fat_dir *dir, struct dircache_entry *ce)
#endif
{
    int depth = 0;
    int result;

    memset(ce, 0, sizeof(struct dircache_entry));
    dir_recursion[0].dir = dir;
    dir_recursion[0].ce = ce;
    dir_recursion[0].first = ce;
    
    do {
        //logf("=> %s", dircache_cur_path);
        result = dircache_scan(&dir_recursion[depth]);
        switch (result) {
            case 0: /* Leaving the current directory. */
                /* Add the standard . and .. entries. */
                ce = dir_recursion[depth].ce;
                ce->d_name = ".";
                ce->name_len = 2;
#ifdef SIMULATOR
                closedir_uncached(dir_recursion[depth].dir);
                ce->attribute = ATTR_DIRECTORY;
#else
                ce->attribute = FAT_ATTR_DIRECTORY;
                ce->startcluster = dir_recursion[depth].dir->file.firstcluster;
#endif
                ce->size = 0;
                ce->down = dir_recursion[depth].first;
    
                depth--;
                if (depth < 0)
                    break ;

                dircache_cur_path[dir_recursion[depth].pathpos] = '\0';
            
                ce = dircache_gen_next(ce);
                if (ce == NULL)
                {
                    logf("memory allocation error");
                    return -3;
                }
#ifdef SIMULATOR
                ce->attribute = ATTR_DIRECTORY;
#else
                ce->attribute = FAT_ATTR_DIRECTORY;
                ce->startcluster = dir_recursion[depth].dir->file.firstcluster;
#endif
                ce->d_name = "..";
                ce->name_len = 3;
                ce->size = 0;
                ce->down = dir_recursion[depth].first;
    
                break ;

            case 1: /* Going down in the directory tree. */
                depth++;
                if (depth >= MAX_SCAN_DEPTH)
                {
                    logf("Too deep directory structure");
                    return -2;
                }
            
#ifdef SIMULATOR
                dir_recursion[depth].dir = dir_recursion[depth-1].newdir;
#else
                dir_recursion[depth].dir = &dir_recursion[depth-1].newdir;
#endif
                dir_recursion[depth].first = dir_recursion[depth-1].down_entry;
                dir_recursion[depth].ce = dir_recursion[depth-1].down_entry;
                break ;
            
            default:
                logf("Scan failed");
                logf("-> %s", dircache_cur_path);
                return -1;
        }
    } while (depth >= 0) ;
    
    return 0;
}

/**
 * Internal function to get a pointer to dircache_entry for a given filename.
 *   path: Absolute path to a file or directory.
 *   get_before: Returns the cache pointer before the last valid entry found.
 *   only_directories: Match only filenames which are a directory type.
 */
static struct dircache_entry* dircache_get_entry(const char *path,
        bool get_before, bool only_directories)
{
    struct dircache_entry *cache_entry, *before;
    char namecopy[MAX_PATH*2];
    char* part;
    char* end;

    strncpy(namecopy, path, sizeof(namecopy) - 1);
    cache_entry = dircache_root;
    before = NULL;
    
    for ( part = strtok_r(namecopy, "/", &end); part;
          part = strtok_r(NULL, "/", &end)) {

        /* scan dir for name */
        while (1)
        {
            if (cache_entry == NULL)
            {
                return NULL;
            }
            else if (cache_entry->name_len == 0)
            {
                cache_entry = cache_entry->next;
                continue ;
            }

            if (!strcasecmp(part, cache_entry->d_name))
            {
                before = cache_entry;
                if (cache_entry->down || only_directories)
                    cache_entry = cache_entry->down;
                break ;
            }

            cache_entry = cache_entry->next;
        }
    }

    if (get_before)
        cache_entry = before;
        
    return cache_entry;
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
#ifdef SIMULATOR
    DIR_UNCACHED *pdir;
#else
    struct fat_dir dir, *pdir;
#endif
    unsigned int start_tick;
    int i;
    
    /* Measure how long it takes build the cache. */
    start_tick = current_tick;
    dircache_initializing = true;
    
#ifdef SIMULATOR
    pdir = opendir_uncached("/");
    if (pdir == NULL)
    {
        logf("Failed to open rootdir");
        dircache_initializing = false;
        return -3;
    }
#else
    if ( fat_opendir(IF_MV2(volume,) &dir, 0, NULL) < 0 ) {
        logf("Failed opening root dir");
        dircache_initializing = false;
        return -3;
    }
    pdir = &dir;
#endif

    memset(dircache_cur_path, 0, sizeof(dircache_cur_path));
    dircache_size = sizeof(struct dircache_entry);

    cpu_boost(true);
    if (dircache_travel(pdir, dircache_root) < 0)
    {
        logf("dircache_travel failed");
        cpu_boost(false);
        dircache_size = 0;
        dircache_initializing = false;
        return -2;
    }
    cpu_boost(false);

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
    
    return dircache_get_entry(filename, false, false);
}

/**
 * Function to copy the full absolute path from dircache to the given buffer
 * using the given dircache_entry pointer.
 */
void dircache_copy_path(const struct dircache_entry *entry, char *buf, int size)
{
    const struct dircache_entry *down[MAX_SCAN_DEPTH];
    int depth = 0;
    
    if (size <= 0)
        return ;
    
    buf[0] = '\0';
    
    if (entry == NULL)
        return ;
    
    do {
        down[depth] = entry;
        entry = entry->up;
        depth++;
    } while (entry != NULL && depth < MAX_SCAN_DEPTH);
    
    while (--depth >= 0)
    {
        snprintf(buf, size, "/%s", down[depth]->d_name);
        buf += down[depth]->name_len; /* '/' + d_name */
        size -= down[depth]->name_len;
        if (size <= 0)
            break ;
    }
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

    strncpy(basedir, path, sizeof(basedir)-1);
    new = strrchr(basedir, '/');
    if (new == NULL)
    {
        logf("error occurred");
        dircache_initialized = false;
        return NULL;
    }

    *new = '\0';
    new++;

    entry = dircache_get_entry(basedir, false, true);
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
        strncpy(fdbind_cache[fdbind_idx].path, path, 
                sizeof(fdbind_cache[fdbind_idx].path)-1);
        fdbind_cache[fdbind_idx].fd = fd;
        fdbind_idx++;
        return ;
    }
    
    if (!dircache_initialized)
        return ;

    logf("bind: %d/%s", fd, path);
    entry = dircache_get_entry(path, false, false);
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
        logf("dircache fd access error");
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
    entry = dircache_get_entry(path, true, true);
    if (entry == NULL)
    {
        logf("not found!");
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
    
    entry = dircache_get_entry(name, false, false);

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
    
    entry = dircache_get_entry(oldpath, true, false);
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
        strncpy(absolute_path, oldpath, sizeof(absolute_path)-1);
        p = strrchr(absolute_path, '/');
        if (!p)
        {
            logf("Invalid path");
            dircache_initialized = false;
            return ;
        }
        
        *p = '\0';
        strncpy(p, absolute_path, sizeof(absolute_path)-1-strlen(p));
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
    cache_entry = dircache_get_entry(name, false, true);
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

        strncpy(dir->secondary_entry.d_name, regentry->d_name, MAX_PATH-1);
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

    strncpy(dir->secondary_entry.d_name, ce->d_name, MAX_PATH-1);
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
