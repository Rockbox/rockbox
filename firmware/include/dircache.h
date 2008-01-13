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
#ifndef _DIRCACHE_H
#define _DIRCACHE_H

#include "dir_uncached.h"

#ifdef HAVE_DIRCACHE

#define DIRCACHE_RESERVE  (1024*64)
#define DIRCACHE_LIMIT    (1024*1024*6)
/* FIXME: We should use ROCKBOX_DIR here but it's defined in apps/ */
#define DIRCACHE_FILE     "/.rockbox/dircache.dat"

#define DIRCACHE_APPFLAG_TAGCACHE  0x0001

/* Internal structures. */
struct travel_data {
    struct dircache_entry *first;
    struct dircache_entry *ce;
    struct dircache_entry *down_entry;
#ifdef SIMULATOR
    DIR_UNCACHED *dir, *newdir;
    struct dirent_uncached *entry;
#else
    struct fat_dir *dir;
    struct fat_dir newdir;
    struct fat_direntry entry;
#endif
    int pathpos;
};

#define DIRCACHE_MAGIC  0x00d0c0a0
struct dircache_maindata {
    long magic;
    long size;
    long entry_count;
    long appflags;
    struct dircache_entry *root_entry;
};

#define MAX_PENDING_BINDINGS 2
struct fdbind_queue {
    char path[MAX_PATH];
    int fd;
};

/* Exported structures. */
struct dircache_entry {
    struct dircache_entry *next;
    struct dircache_entry *up;
    struct dircache_entry *down;
    int attribute;
    long size;
    long startcluster;
    unsigned short wrtdate;
    unsigned short wrttime;
    unsigned long name_len;
    char *d_name;
};

typedef struct {
    bool busy;
    struct dircache_entry *entry;
    struct dircache_entry *internal_entry;
    struct dircache_entry secondary_entry;
    DIR_UNCACHED *regulardir;
} DIR_CACHED;

void dircache_init(void);
int dircache_load(void);
int dircache_save(void);
int dircache_build(int last_size);
void* dircache_steal_buffer(long *size);
bool dircache_is_enabled(void);
bool dircache_is_initializing(void);
void dircache_set_appflag(long mask);
bool dircache_get_appflag(long mask);
int dircache_get_entry_count(void);
int dircache_get_cache_size(void);
int dircache_get_reserve_used(void);
int dircache_get_build_ticks(void);
void dircache_disable(void);
const struct dircache_entry *dircache_get_entry_ptr(const char *filename);
void dircache_copy_path(const struct dircache_entry *entry, char *buf, int size);

void dircache_bind(int fd, const char *path);
void dircache_update_filesize(int fd, long newsize, long startcluster);
void dircache_update_filetime(int fd);
void dircache_mkdir(const char *path);
void dircache_rmdir(const char *path);
void dircache_remove(const char *name);
void dircache_rename(const char *oldpath, const char *newpath);
void dircache_add_file(const char *path, long startcluster);

DIR_CACHED* opendir_cached(const char* name);
struct dircache_entry* readdir_cached(DIR_CACHED* dir);
int closedir_cached(DIR_CACHED *dir);
int mkdir_cached(const char *name);
int rmdir_cached(const char* name);
#endif /* !HAVE_DIRCACHE */

#endif
