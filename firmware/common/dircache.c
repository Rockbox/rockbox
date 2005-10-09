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
   - Fix this to work with simulator (opendir & readdir) again.
*/

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "dir.h"
#include "debug.h"
#include "atoi.h"
#include "system.h"
#include "logf.h"
#include "dircache.h"
#include "thread.h"
#include "kernel.h"
#include "usb.h"
#include "file.h"

/* Queue commands. */
#define DIRCACHE_BUILD 1

extern char *audiobuf;

#define MAX_OPEN_DIRS 8
DIRCACHED opendirs[MAX_OPEN_DIRS];

static struct dircache_entry *fd_bindings[MAX_OPEN_FILES];
static struct dircache_entry *dircache_root;

static bool dircache_initialized = false;
static bool thread_enabled = false;
static unsigned long allocated_size = DIRCACHE_LIMIT;
static unsigned long dircache_size = 0;
static unsigned long reserve_used = 0;
static char dircache_cur_path[MAX_PATH];

static struct event_queue dircache_queue;
static long dircache_stack[(DEFAULT_STACK_SIZE + 0x800)/sizeof(long)];
static const char dircache_thread_name[] = "dircache";

/* --- Internal cache structure control functions --- */
static struct dircache_entry* allocate_entry(void)
{
    struct dircache_entry *next_entry;
    
    if (dircache_size > allocated_size - MAX_PATH*2)
    {
        logf("size limit reached");
        return NULL;
    }
    
    next_entry = (struct dircache_entry *)((char *)dircache_root+dircache_size);
    dircache_size += sizeof(struct dircache_entry);
    next_entry->name_len = 0;
    next_entry->d_name = NULL;
    next_entry->down = NULL;
    next_entry->next = NULL;

    return next_entry;
}

static struct dircache_entry* dircache_gen_next(struct dircache_entry *ce)
{
    struct dircache_entry *next_entry;

    next_entry = allocate_entry();
    ce->next = next_entry;
    
    return next_entry;
}

static struct dircache_entry* dircache_gen_down(struct dircache_entry *ce)
{
    struct dircache_entry *next_entry;

    next_entry = allocate_entry();
    ce->down = next_entry;
    
    return next_entry;
}

/* This will eat ~30 KiB of memory!
 * We should probably use that as additional reserve buffer in future. */
#define MAX_SCAN_DEPTH 16
static struct travel_data dir_recursion[MAX_SCAN_DEPTH];

static int dircache_scan(struct travel_data *td)
{
    while ( (fat_getnext(td->dir, &td->entry) >= 0) && (td->entry.name[0]))
    {
        if (thread_enabled)
        {
            /* Stop if we got an external signal. */
            if (!queue_empty(&dircache_queue))
                return -6;
            yield();
        }
            
        if (!strcmp(".", td->entry.name) ||
             !strcmp("..", td->entry.name))
        {
            continue;
        }
                
        td->ce->attribute = td->entry.attr;
        td->ce->name_len = MIN(254, strlen(td->entry.name)) + 1;
        td->ce->d_name = ((char *)dircache_root+dircache_size);
        td->ce->startcluster = td->entry.firstcluster;
        td->ce->size = td->entry.filesize;
        td->ce->wrtdate = td->entry.wrtdate;
        td->ce->wrttime = td->entry.wrttime;
        memcpy(td->ce->d_name, td->entry.name, td->ce->name_len);
        dircache_size += td->ce->name_len;
        
        if (td->entry.attr & FAT_ATTR_DIRECTORY)
        {
            
            td->down_entry = dircache_gen_down(td->ce);
            if (td->down_entry == NULL)
                return -2;
            
            td->pathpos = strlen(dircache_cur_path);
            strncpy(&dircache_cur_path[td->pathpos], "/", MAX_PATH - td->pathpos - 1);
            strncpy(&dircache_cur_path[td->pathpos+1], td->entry.name, MAX_PATH - td->pathpos - 2);

            td->newdir = *td->dir;
            if (fat_opendir(IF_MV2(volume,) &td->newdir,
                td->entry.firstcluster, td->dir) < 0 )
            {
                return -3;
            }

            td->ce = dircache_gen_next(td->ce);
            if (td->ce == NULL)
                return -4;
            
            return 1;
        }
        
        td->ce->down = NULL;
        td->ce = dircache_gen_next(td->ce);
        if (td->ce == NULL)
            return -5;
    }

    return 0;
}

/* Recursively scan the hard disk and build the cache. */
static int dircache_travel(struct fat_dir *dir, struct dircache_entry *ce)
{
    int depth = 0;
    int result;

    dir_recursion[0].dir = dir;
    dir_recursion[0].ce = ce;
    
    do {
        //logf("=> %s", dircache_cur_path);
        result = dircache_scan(&dir_recursion[depth]);
        switch (result) {
            case 0: /* Leaving the current directory. */
                depth--;
                if (depth >= 0)
                    dircache_cur_path[dir_recursion[depth].pathpos] = '\0';
                break ;

            case 1: /* Going down in the directory tree. */
                depth++;
                if (depth >= MAX_SCAN_DEPTH)
                {
                    logf("Too deep directory structure");
                    return -2;
                }
                    
                dir_recursion[depth].dir = &dir_recursion[depth-1].newdir;
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

static struct dircache_entry* dircache_get_entry(const char *path,
        bool get_before, bool only_directories)
{
    struct dircache_entry *cache_entry, *before;
    char namecopy[MAX_PATH];
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

#if 0
int dircache_load(const char *path)
{
    struct dircache_maindata maindata;
    int bytes_read;
    int fd;
        
    if (dircache_initialized)
        return -1;
        
    logf("Loading directory cache");
    dircache_size = 0;
    
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -2;
        
    bytes_read = read(fd, &maindata, sizeof(struct dircache_maindata));
    if (bytes_read != sizeof(struct dircache_maindata)
        || (long)maindata.root_entry != (long)audiobuf
        || maindata.size <= 0)
    {
        close(fd);
        return -3;
    }

    dircache_root = (struct dircache_entry *)audiobuf;
    bytes_read = read(fd, dircache_root, MIN(DIRCACHE_LIMIT, maindata.size));
    close(fd);
    
    if (bytes_read != maindata.size)
        return -6;

    /* Cache successfully loaded. */
    dircache_size = maindata.size;
    logf("Done, %d KiB used", dircache_size / 1024);
    dircache_initialized = true;
    memset(fd_bindings, 0, sizeof(fd_bindings));
    
    /* We have to long align the audiobuf to keep the buffer access fast. */
    audiobuf += (long)((dircache_size & ~0x03) + 0x04);
    audiobuf += DIRCACHE_RESERVE;

    return 0;
}

int dircache_save(const char *path)
{
    struct dircache_maindata maindata;
    int fd;
    unsigned long bytes_written;

    remove(path);
    
    while (thread_enabled)
        sleep(1);
        
    if (!dircache_initialized)
        return -1;

    logf("Saving directory cache");
    fd = open(path, O_WRONLY | O_CREAT);

    maindata.magic = DIRCACHE_MAGIC;
    maindata.size = dircache_size;
    maindata.root_entry = dircache_root;

    /* Save the info structure */
    bytes_written = write(fd, &maindata, sizeof(struct dircache_maindata));
    if (bytes_written != sizeof(struct dircache_maindata))
    {
        close(fd);
        return -2;
    }

    /* Dump whole directory cache to disk */
    bytes_written = write(fd, dircache_root, dircache_size);
    close(fd);
    if (bytes_written != dircache_size)
        return -3;

    return 0;
}
#endif /* #if 0 */

static int dircache_do_rebuild(void)
{
    struct fat_dir dir;
    
    if ( fat_opendir(IF_MV2(volume,) &dir, 0, NULL) < 0 ) {
        logf("Failed opening root dir");
        return -3;
    }

    //return -5;
/*    dir = opendir("/");
    if (dir == NULL)
    {
        logf("failed to open rootdir");
        return -1;
}*/

    memset(dircache_cur_path, 0, MAX_PATH);
    dircache_size = sizeof(struct dircache_entry);

    cpu_boost(true);
    if (dircache_travel(&dir, dircache_root) < 0)
    {
        logf("dircache_travel failed");
        cpu_boost(false);
        dircache_size = 0;
        return -2;
    }
    cpu_boost(false);

    logf("Done, %d KiB used", dircache_size / 1024);
    dircache_initialized = true;
    
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
    }
    
    return 1;
}

static void dircache_thread(void)
{
    struct event ev;

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
                
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&dircache_queue);
                break ;
#endif
        }
    }
}

int dircache_build(int last_size)
{
    if (dircache_initialized)
        return -3;

    while (thread_enabled)
        sleep(1);
        
    logf("Building directory cache");
    if (dircache_size > 0)
    {
        allocated_size = dircache_size + (DIRCACHE_RESERVE-reserve_used);
        thread_enabled = true;
        queue_post(&dircache_queue, DIRCACHE_BUILD, 0);
        return 2;
    }
    else
    {
        dircache_root = (struct dircache_entry *)audiobuf;
        dircache_size = 0;
    }

    if (last_size > DIRCACHE_RESERVE && last_size < DIRCACHE_LIMIT)
    {
        allocated_size = last_size + DIRCACHE_RESERVE;
        
        /* We have to long align the audiobuf to keep the buffer access fast. */
        audiobuf += (long)((allocated_size & ~0x03) + 0x04);
        thread_enabled = true;

        /* Start a transparent rebuild. */
        queue_post(&dircache_queue, DIRCACHE_BUILD, 0);
        return 3;
    }

    /* Start a non-transparent rebuild. */
    return dircache_do_rebuild();
}

void dircache_init(void)
{
    int i;
    
    memset(opendirs, 0, sizeof(opendirs));
    for (i = 0; i < MAX_OPEN_DIRS; i++)
    {
        opendirs[i].secondary_entry.d_name = audiobuf;
        audiobuf += MAX_PATH;
    }
    
    queue_init(&dircache_queue);
    create_thread(dircache_thread, dircache_stack,
                sizeof(dircache_stack), dircache_thread_name);
}

bool dircache_is_enabled(void)
{
    return dircache_initialized;
}

int dircache_get_cache_size(void)
{
    if (!dircache_is_enabled())
        return 0;
    
    return dircache_size;
}

void dircache_disable(void)
{
    int i;
    
    while (thread_enabled)
        sleep(1);
    dircache_initialized = false;

    for (i = 0; i < MAX_OPEN_DIRS; i++)
        opendirs[i].busy = false;
}

/* --- Directory cache live updating functions --- */
static struct dircache_entry* dircache_new_entry(const char *path, int attribute)
{
    struct dircache_entry *entry;
    char basedir[MAX_PATH];
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

void dircache_update_filesize(int fd, long newsize)
{
    if (!dircache_initialized || fd < 0)
        return ;

    fd_bindings[fd]->size = newsize;
}

void dircache_mkdir(const char *path)
{ /* Test ok. */
    if (!dircache_initialized)
        return ;
        
    logf("mkdir: %s", path);
    dircache_new_entry(path, ATTR_DIRECTORY);
}

void dircache_rmdir(const char *path)
{ /* Test ok. */
    struct dircache_entry *entry;
    
    if (!dircache_initialized)
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
    
    if (!dircache_initialized)
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
    
    if (!dircache_initialized)
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

    newentry = dircache_new_entry(newpath, entry->attribute);
    if (newentry == NULL)
    {
        dircache_initialized = false;
        return ;
    }

    //newentry->down = entry->down;
    //entry->down = 0;
    
    newentry->size = entry->size;
    newentry->startcluster = entry->startcluster;
    newentry->wrttime = entry->wrttime;
    newentry->wrtdate = entry->wrtdate;
}

void dircache_add_file(const char *path)
{
    if (!dircache_initialized)
        return ;
        
    logf("add file: %s", path);
    dircache_new_entry(path, 0);
}

DIRCACHED* opendir_cached(const char* name)
{
    struct dircache_entry *cache_entry;
    int dd;
    DIRCACHED* pdir = opendirs;

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
        pdir->regulardir = opendir(name);
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

struct dircache_entry* readdir_cached(DIRCACHED* dir)
{
    struct dirent *regentry;
    struct dircache_entry *ce;
    
    if (!dir->busy)
        return NULL;

    if (dir->regulardir != NULL)
    {
        regentry = readdir(dir->regulardir);
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

    //logf("-> %s", ce->name);
    return &dir->secondary_entry;
}

int closedir_cached(DIRCACHED* dir)
{
    if (!dir->busy)
        return -1;
        
    dir->busy=false;
    if (dir->regulardir != NULL)
        return closedir(dir->regulardir);
    
    return 0;
}

