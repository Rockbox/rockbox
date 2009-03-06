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

/*
 *                    TagCache API
 * 
 *       ----------x---------x------------------x-----
 *                 |         |                  |              External
 * +---------------x-------+ |       TagCache   |              Libraries
 * | Modification routines | |         Core     |           
 * +-x---------x-----------+ |                  |           
 *   | (R/W)   |             |                  |           |
 *   |  +------x-------------x-+  +-------------x-----+     |
 *   |  |                      x==x Filters & clauses |     |
 *   |  | Search routines      |  +-------------------+     |
 *   |  |                      x============================x DirCache
 *   |  +-x--------------------+                            | (optional)
 *   |    | (R)                                             |
 *   |    | +-------------------------------+  +---------+  |
 *   |    | | DB Commit (sort,unique,index) |  |         |  |
 *   |    | +-x--------------------------x--+  | Control |  |
 *   |    |   | (R/W)                    | (R) | Thread  |  |
 *   |    |   | +----------------------+ |     |         |  |
 *   |    |   | | TagCache DB Builder  | |     +---------+  |
 *   |    |   | +-x-------------x------+ |                  |
 *   |    |   |   | (R)         | (W)    |                  |
 *   |    |   |   |          +--x--------x---------+        |
 *   |    |   |   |          | Temporary Commit DB |        |
 *   |    |   |   |          +---------------------+        |
 * +-x----x-------x--+                                      |
 * | TagCache RAM DB x==\(W) +-----------------+            |
 * +-----------------+   \===x                 |            |
 *   |    |   |   |      (R) |  Ram DB Loader  x============x DirCache
 * +-x----x---x---x---+   /==x                 |            | (optional)
 * | Tagcache Disk DB x==/   +-----------------+            |  
 * +------------------+                                     |
 * 
 */

/* #define LOGF_ENABLE */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "ata_idle_notify.h"
#include "thread.h"
#include "kernel.h"
#include "system.h"
#include "logf.h"
#include "string.h"
#include "usb.h"
#include "metadata.h"
#include "metadata.h"
#include "tagcache.h"
#include "buffer.h"
#include "crc32.h"
#include "misc.h"
#include "settings.h"
#include "dir.h"
#include "structec.h"
#include "tagcache.h"

#ifndef __PCTOOL__
#include "splash.h"
#include "lang.h"
#include "eeprom_settings.h"
#endif

#ifdef __PCTOOL__
#define yield() do { } while(0)
#define sim_sleep(timeout) do { } while(0)
#define do_timed_yield() do { } while(0)
#endif

#ifndef __PCTOOL__
/* Tag Cache thread. */
static struct event_queue tagcache_queue;
static long tagcache_stack[(DEFAULT_STACK_SIZE + 0x4000)/sizeof(long)];
static const char tagcache_thread_name[] = "tagcache";
#endif

#define UNTAGGED "<Untagged>"

/* Previous path when scanning directory tree recursively. */
static char curpath[TAG_MAXLEN+32];

/* Used when removing duplicates. */
static char *tempbuf;     /* Allocated when needed. */
static long tempbufidx;   /* Current location in buffer. */
static long tempbuf_size; /* Buffer size (TEMPBUF_SIZE). */
static long tempbuf_left; /* Buffer space left. */
static long tempbuf_pos;

/* Tags we want to get sorted (loaded to the tempbuf). */
static const int sorted_tags[] = { tag_artist, tag_album, tag_genre, 
    tag_composer, tag_comment, tag_albumartist, tag_grouping, tag_title };

/* Uniqued tags (we can use these tags with filters and conditional clauses). */
static const int unique_tags[] = { tag_artist, tag_album, tag_genre, 
    tag_composer, tag_comment, tag_albumartist, tag_grouping };

/* Numeric tags (we can use these tags with conditional clauses). */
static const int numeric_tags[] = { tag_year, tag_discnumber, 
    tag_tracknumber, tag_length, tag_bitrate, tag_playcount, tag_rating,
    tag_playtime, tag_lastplayed, tag_commitid, tag_mtime,
    tag_virt_length_min, tag_virt_length_sec,
    tag_virt_playtime_min, tag_virt_playtime_sec,
    tag_virt_entryage, tag_virt_autoscore };

/* String presentation of the tags defined in tagcache.h. Must be in correct order! */
static const char *tags_str[] = { "artist", "album", "genre", "title", 
    "filename", "composer", "comment", "albumartist", "grouping", "year", 
    "discnumber", "tracknumber", "bitrate", "length", "playcount", "rating", 
    "playtime", "lastplayed", "commitid", "mtime" };

/* Status information of the tagcache. */
static struct tagcache_stat tc_stat;

/* Queue commands. */
enum tagcache_queue {
    Q_STOP_SCAN = 0,
    Q_START_SCAN,
    Q_IMPORT_CHANGELOG,
    Q_UPDATE,
    Q_REBUILD,
    
    /* Internal tagcache command queue. */
    CMD_UPDATE_MASTER_HEADER,
    CMD_UPDATE_NUMERIC,
};

struct tagcache_command_entry {
    int32_t command;
    int32_t idx_id;
    int32_t tag;
    int32_t data;
};

#ifndef __PCTOOL__
static struct tagcache_command_entry command_queue[TAGCACHE_COMMAND_QUEUE_LENGTH];
static volatile int command_queue_widx = 0;
static volatile int command_queue_ridx = 0;
static struct mutex command_queue_mutex;
#endif

/* Tag database structures. */

/* Variable-length tag entry in tag files. */
struct tagfile_entry {
    int32_t tag_length;  /* Length of the data in bytes including '\0' */
    int32_t idx_id;      /* Corresponding entry location in index file of not unique tags */
    char tag_data[0];  /* Begin of the tag data */
};

/* Fixed-size tag entry in master db index. */
struct index_entry {
    int32_t tag_seek[TAG_COUNT]; /* Location of tag data or numeric tag data */
    int32_t flag;                /* Status flags */
};

/* Header is the same in every file. */
struct tagcache_header {
    int32_t magic;       /* Header version number */
    int32_t datasize;    /* Data size in bytes */
    int32_t entry_count; /* Number of entries in this file */
};

struct master_header {
    struct tagcache_header tch;
    int32_t serial; /* Increasing counting number */
    int32_t commitid; /* Number of commits so far */
    int32_t dirty;
};

/* For the endianess correction */
static const char *tagfile_entry_ec   = "ll";
/**
 Note: This should be (1 + TAG_COUNT) amount of l's.
 */
static const char *index_entry_ec     = "lllllllllllllllllllll";

static const char *tagcache_header_ec = "lll";
static const char *master_header_ec   = "llllll";

static struct master_header current_tcmh;

#ifdef HAVE_TC_RAMCACHE
/* Header is created when loading database to ram. */
struct ramcache_header {
    struct master_header h;      /* Header from the master index */
    struct index_entry *indices; /* Master index file content */
    char *tags[TAG_COUNT];       /* Tag file content (not including filename tag) */
    int entry_count[TAG_COUNT];  /* Number of entries in the indices. */
};

# ifdef HAVE_EEPROM_SETTINGS
struct statefile_header {
    struct ramcache_header *hdr;
    struct tagcache_stat tc_stat;
};
# endif

/* Pointer to allocated ramcache_header */
static struct ramcache_header *hdr;
#endif

/** 
 * Full tag entries stored in a temporary file waiting 
 * for commit to the cache. */
struct temp_file_entry {
    long tag_offset[TAG_COUNT];
    short tag_length[TAG_COUNT];
    long flag;
    
    long data_length;
};

struct tempbuf_id_list {
    long id;
    struct tempbuf_id_list *next;
};

struct tempbuf_searchidx {
    long idx_id;
    char *str;
    int seek;
    struct tempbuf_id_list idlist;
};

/* Lookup buffer for fixing messed up index while after sorting. */
static long commit_entry_count;
static long lookup_buffer_depth;
static struct tempbuf_searchidx **lookup;

/* Used when building the temporary file. */
static int cachefd = -1, filenametag_fd;
static int total_entry_count = 0;
static int data_size = 0;
static int processed_dir_count;

/* Thread safe locking */
static volatile int write_lock;
static volatile int read_lock;

static bool delete_entry(long idx_id);

const char* tagcache_tag_to_str(int tag)
{
    return tags_str[tag];
}

bool tagcache_is_numeric_tag(int type)
{
    int i;

    for (i = 0; i < (int)(sizeof(numeric_tags)/sizeof(numeric_tags[0])); i++)
    {
        if (type == numeric_tags[i])
            return true;
    }

    return false;
}

bool tagcache_is_unique_tag(int type)
{
    int i;

    for (i = 0; i < (int)(sizeof(unique_tags)/sizeof(unique_tags[0])); i++)
    {
        if (type == unique_tags[i])
            return true;
    }

    return false;
}

bool tagcache_is_sorted_tag(int type)
{
    int i;

    for (i = 0; i < (int)(sizeof(sorted_tags)/sizeof(sorted_tags[0])); i++)
    {
        if (type == sorted_tags[i])
            return true;
    }

    return false;
}

#ifdef HAVE_DIRCACHE
/**
 * Returns true if specified flag is still present, i.e., dircache
 * has not been reloaded.
 */
static bool is_dircache_intact(void)
{
    return dircache_get_appflag(DIRCACHE_APPFLAG_TAGCACHE);
}
#endif

static int open_tag_fd(struct tagcache_header *hdr, int tag, bool write)
{
    int fd;
    char buf[MAX_PATH];
    int rc;
    
    if (tagcache_is_numeric_tag(tag) || tag < 0 || tag >= TAG_COUNT)
        return -1;
    
    snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, tag);
    
    fd = open(buf, write ? O_RDWR : O_RDONLY);
    if (fd < 0)
    {
        logf("tag file open failed: tag=%d write=%d file=%s", tag, write, buf);
        tc_stat.ready = false;
        return fd;
    }
    
    /* Check the header. */
    rc = ecread(fd, hdr, 1, tagcache_header_ec, tc_stat.econ);
    if (hdr->magic != TAGCACHE_MAGIC || rc != sizeof(struct tagcache_header))
    {
        logf("header error");
        tc_stat.ready = false;
        close(fd);
        return -2;
    }

    return fd;
}

static int open_master_fd(struct master_header *hdr, bool write)
{
    int fd;
    int rc;
    
    fd = open(TAGCACHE_FILE_MASTER, write ? O_RDWR : O_RDONLY);
    if (fd < 0)
    {
        logf("master file open failed for R/W");
        tc_stat.ready = false;
        return fd;
    }
    
    tc_stat.econ = false;
    
    /* Check the header. */
    rc = read(fd, hdr, sizeof(struct master_header));
    if (hdr->tch.magic == TAGCACHE_MAGIC && rc == sizeof(struct master_header))
    {
        /* Success. */
        return fd;
    }
    
    /* Trying to read again, this time with endianess correction enabled. */
    lseek(fd, 0, SEEK_SET);
    
    rc = ecread(fd, hdr, 1, master_header_ec, true);
    if (hdr->tch.magic != TAGCACHE_MAGIC || rc != sizeof(struct master_header))
    {
        logf("header error");
        tc_stat.ready = false;
        close(fd);
        return -2;
    }

    tc_stat.econ = true;
    
    return fd;
}

#ifndef __PCTOOL__
static bool do_timed_yield(void)
{
    /* Sorting can lock up for quite a while, so yield occasionally */
    static long wakeup_tick = 0;
    if (current_tick >= wakeup_tick)
    {
        wakeup_tick = current_tick + (HZ/4);
        yield();
        return true;
    }
    return false;
}
#endif

#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
static long find_entry_ram(const char *filename,
                           const struct dirent *dc)
{
    static long last_pos = 0;
    int i;
    
    /* Check if we tagcache is loaded into ram. */
    if (!tc_stat.ramcache)
        return -1;

    if (dc == NULL)
        dc = dircache_get_entry_ptr(filename);
    
    if (dc == NULL)
    {
        logf("tagcache: file not found.");
        return -1;
    }

    try_again:
            
    if (last_pos > 0)
        i = last_pos;
    else
        i = 0;
    
    for (; i < hdr->h.tch.entry_count; i++)
    {
        if (hdr->indices[i].tag_seek[tag_filename] == (long)dc)
        {
            last_pos = MAX(0, i - 3);
            return i;
        }
        
        do_timed_yield();
    }

    if (last_pos > 0)
    {
        last_pos = 0;
        goto try_again;
    }

    return -1;
}
#endif

static long find_entry_disk(const char *filename)
{
    struct tagcache_header tch;
    static long last_pos = -1;
    long pos_history[POS_HISTORY_COUNT];
    long pos_history_idx = 0;
    bool found = false;
    struct tagfile_entry tfe;
    int fd;
    char buf[TAG_MAXLEN+32];
    int i;
    int pos = -1;

    if (!tc_stat.ready)
        return -2;
    
    fd = filenametag_fd;
    if (fd < 0)
    {
        last_pos = -1;
        if ( (fd = open_tag_fd(&tch, tag_filename, false)) < 0)
            return -1;
    }
    
    check_again:
    
    if (last_pos > 0)
        lseek(fd, last_pos, SEEK_SET);
    else
        lseek(fd, sizeof(struct tagcache_header), SEEK_SET);

    while (true)
    {
        pos = lseek(fd, 0, SEEK_CUR);
        for (i = pos_history_idx-1; i >= 0; i--)
            pos_history[i+1] = pos_history[i];
        pos_history[0] = pos;

        if (ecread(fd, &tfe, 1, tagfile_entry_ec, tc_stat.econ) 
            != sizeof(struct tagfile_entry))
        {
            break ;
        }
        
        if (tfe.tag_length >= (long)sizeof(buf))
        {
            logf("too long tag #1");
            close(fd);
            last_pos = -1;
            return -2;
        }
        
        if (read(fd, buf, tfe.tag_length) != tfe.tag_length)
        {
            logf("read error #2");
            close(fd);
            last_pos = -1;
            return -3;
        }
        
        if (!strcasecmp(filename, buf))
        {
            last_pos = pos_history[pos_history_idx];
            found = true;
            break ;
        }
        
        if (pos_history_idx < POS_HISTORY_COUNT - 1)
            pos_history_idx++;
    }
    
    /* Not found? */
    if (!found)
    {
        if (last_pos > 0)
        {
            last_pos = -1;
            logf("seek again");
            goto check_again;
        }
        
        if (fd != filenametag_fd)
            close(fd);
        return -4;
    }
    
    if (fd != filenametag_fd)
        close(fd);
        
    return tfe.idx_id;
}

static int find_index(const char *filename)
{
    long idx_id = -1;
    
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    if (tc_stat.ramcache && is_dircache_intact())
        idx_id = find_entry_ram(filename, NULL);
#endif
    
    if (idx_id < 0)
        idx_id = find_entry_disk(filename);
    
    return idx_id;
}

bool tagcache_find_index(struct tagcache_search *tcs, const char *filename)
{
    int idx_id;
    
    if (!tc_stat.ready)
        return false;
    
    idx_id = find_index(filename);
    if (idx_id < 0)
        return false;
    
    if (!tagcache_search(tcs, tag_filename))
        return false;
    
    tcs->entry_count = 0;
    tcs->idx_id = idx_id;
    
    return true;
}

static bool get_index(int masterfd, int idxid, 
                      struct index_entry *idx, bool use_ram)
{
    bool localfd = false;
    
    if (idxid < 0)
    {
        logf("Incorrect idxid: %d", idxid);
        return false;
    }
    
#ifdef HAVE_TC_RAMCACHE
    if (tc_stat.ramcache && use_ram)
    {
        if (hdr->indices[idxid].flag & FLAG_DELETED)
            return false;
        
        memcpy(idx, &hdr->indices[idxid], sizeof(struct index_entry));
        return true;
    }
#else
    (void)use_ram;
#endif
    
    if (masterfd < 0)
    {
        struct master_header tcmh;
        
        localfd = true;
        masterfd = open_master_fd(&tcmh, false);
        if (masterfd < 0)
            return false;
    }
    
    lseek(masterfd, idxid * sizeof(struct index_entry) 
          + sizeof(struct master_header), SEEK_SET);
    if (ecread(masterfd, idx, 1, index_entry_ec, tc_stat.econ) 
        != sizeof(struct index_entry))
    {
        logf("read error #3");
        if (localfd)
            close(masterfd);
        
        return false;
    }
    
    if (localfd)
        close(masterfd);
    
    if (idx->flag & FLAG_DELETED)
        return false;
    
    return true;
}

#ifndef __PCTOOL__

static bool write_index(int masterfd, int idxid, struct index_entry *idx)
{
    /* We need to exclude all memory only flags & tags when writing to disk. */
    if (idx->flag & FLAG_DIRCACHE)
    {
        logf("memory only flags!");
        return false;
    }
    
#ifdef HAVE_TC_RAMCACHE
    /* Only update numeric data. Writing the whole index to RAM by memcpy
     * destroys dircache pointers!
     */
    if (tc_stat.ramcache)
    {
        int tag;
        struct index_entry *idx_ram = &hdr->indices[idxid];
        
        for (tag = 0; tag < TAG_COUNT; tag++)
        {
            if (tagcache_is_numeric_tag(tag))
            {
                idx_ram->tag_seek[tag] = idx->tag_seek[tag];
            }
        }
        
        /* Don't touch the dircache flag or attributes. */
        idx_ram->flag = (idx->flag & 0x0000ffff) 
            | (idx_ram->flag & (0xffff0000 | FLAG_DIRCACHE));
    }
#endif
    
    lseek(masterfd, idxid * sizeof(struct index_entry) 
          + sizeof(struct master_header), SEEK_SET);
    if (ecwrite(masterfd, idx, 1, index_entry_ec, tc_stat.econ) 
        != sizeof(struct index_entry))
    {
        logf("write error #3");
        logf("idxid: %d", idxid);
        return false;
    }
    
    return true;
}

#endif /* !__PCTOOL__ */

static bool open_files(struct tagcache_search *tcs, int tag)
{
    if (tcs->idxfd[tag] < 0)
    {
        char fn[MAX_PATH];

        snprintf(fn, sizeof fn, TAGCACHE_FILE_INDEX, tag);
        tcs->idxfd[tag] = open(fn, O_RDONLY);
    }
    
    if (tcs->idxfd[tag] < 0)
    {
        logf("File not open!");
        return false;
    }
    
    return true;
}

static bool retrieve(struct tagcache_search *tcs, struct index_entry *idx, 
                     int tag, char *buf, long size)
{
    struct tagfile_entry tfe;
    long seek;
    
    *buf = '\0';
    
    if (tagcache_is_numeric_tag(tag))
        return false;
    
    seek = idx->tag_seek[tag];
    if (seek < 0)
    {
        logf("Retrieve failed");
        return false;
    }
    
#ifdef HAVE_TC_RAMCACHE
    if (tcs->ramsearch)
    {
        struct tagfile_entry *ep;
        
# ifdef HAVE_DIRCACHE
        if (tag == tag_filename && (idx->flag & FLAG_DIRCACHE)
            && is_dircache_intact())
        {
            dircache_copy_path((struct dirent *)seek,
                               buf, size);
            return true;
        }
        else
# endif
        if (tag != tag_filename)
        {
            ep = (struct tagfile_entry *)&hdr->tags[tag][seek];
            strncpy(buf, ep->tag_data, size-1);
            
            return true;
        }
    }
#endif
    
    if (!open_files(tcs, tag))
        return false;
    
    lseek(tcs->idxfd[tag], seek, SEEK_SET);
    if (ecread(tcs->idxfd[tag], &tfe, 1, tagfile_entry_ec, tc_stat.econ) 
        != sizeof(struct tagfile_entry))
    {
        logf("read error #5");
        return false;
    }
    
    if (tfe.tag_length >= size)
    {
        logf("too small buffer");
        return false;
    }
    
    if (read(tcs->idxfd[tag], buf, tfe.tag_length) !=
        tfe.tag_length)
    {
        logf("read error #6");
        return false;
    }
    
    buf[tfe.tag_length] = '\0';
    
    return true;
}

static long check_virtual_tags(int tag, const struct index_entry *idx)
{
    long data = 0;
    
    switch (tag) 
    {
        case tag_virt_length_sec:
            data = (idx->tag_seek[tag_length]/1000) % 60;
            break;
        
        case tag_virt_length_min:
            data = (idx->tag_seek[tag_length]/1000) / 60;
            break;
        
        case tag_virt_playtime_sec:
            data = (idx->tag_seek[tag_playtime]/1000) % 60;
            break;
        
        case tag_virt_playtime_min:
            data = (idx->tag_seek[tag_playtime]/1000) / 60;
            break;
        
        case tag_virt_autoscore:
            if (idx->tag_seek[tag_length] == 0 
                || idx->tag_seek[tag_playcount] == 0)
            {
                data = 0;
            }
            else
            {
                data = 100 * idx->tag_seek[tag_playtime]
                    / idx->tag_seek[tag_length]
                    / idx->tag_seek[tag_playcount];
            }
            break;
        
        /* How many commits before the file has been added to the DB. */
        case tag_virt_entryage:
            data = current_tcmh.commitid - idx->tag_seek[tag_commitid] - 1;
            break;
        
        default:
            data = idx->tag_seek[tag];
    }
    
    return data;
}

long tagcache_get_numeric(const struct tagcache_search *tcs, int tag)
{
    struct index_entry idx;
    
    if (!tc_stat.ready)
        return false;
    
    if (!tagcache_is_numeric_tag(tag))
        return -1;
    
    if (!get_index(tcs->masterfd, tcs->idx_id, &idx, true))
        return -2;
    
    return check_virtual_tags(tag, &idx);
}

inline static bool str_ends_with(const char *str1, const char *str2)
{
    int str_len = strlen(str1);
    int clause_len = strlen(str2);
    
    if (clause_len > str_len)
        return false;
    
    return !strcasecmp(&str1[str_len - clause_len], str2);
}

inline static bool str_oneof(const char *str, const char *list)
{
    const char *sep;
    int l, len = strlen(str);
    
    while (*list)
    {
        sep = strchr(list, '|');
        l = sep ? (long)sep - (long)list : (int)strlen(list);
        if ((l==len) && !strncasecmp(str, list, len))
            return true;
        list += sep ? l + 1 : l;
    }

    return false;
}

static bool check_against_clause(long numeric, const char *str,
                                 const struct tagcache_search_clause *clause)
{
    if (clause->numeric)
    {
        switch (clause->type)
        {
            case clause_is:
                return numeric == clause->numeric_data;
            case clause_is_not:
                return numeric != clause->numeric_data;
            case clause_gt:
                return numeric > clause->numeric_data;
            case clause_gteq:
                return numeric >= clause->numeric_data;
            case clause_lt:
                return numeric < clause->numeric_data;
            case clause_lteq:
                return numeric <= clause->numeric_data;        
            default:
                logf("Incorrect numeric tag: %d", clause->type);
        }
    }
    else
    {
        switch (clause->type)
        {
            case clause_is:
                return !strcasecmp(clause->str, str);
            case clause_is_not:
                return strcasecmp(clause->str, str);
            case clause_gt:
                return 0>strcasecmp(clause->str, str);
            case clause_gteq:
                return 0>=strcasecmp(clause->str, str);
            case clause_lt:
                return 0<strcasecmp(clause->str, str);
            case clause_lteq:
                return 0<=strcasecmp(clause->str, str);
            case clause_contains:
                return (strcasestr(str, clause->str) != NULL);
            case clause_not_contains:
                return (strcasestr(str, clause->str) == NULL);
            case clause_begins_with:
                return (strcasestr(str, clause->str) == str);
            case clause_not_begins_with:
                return (strcasestr(str, clause->str) != str);
            case clause_ends_with:
                return str_ends_with(str, clause->str);
            case clause_not_ends_with:
                return !str_ends_with(str, clause->str);
            case clause_oneof:
                return str_oneof(str, clause->str);
            
            default:
                logf("Incorrect tag: %d", clause->type);
        }
    }

    return false;
}

static bool check_clauses(struct tagcache_search *tcs,
                          struct index_entry *idx,
                          struct tagcache_search_clause **clause, int count)
{
    int i;
    
#ifdef HAVE_TC_RAMCACHE
    if (tcs->ramsearch)
    {
        /* Go through all conditional clauses. */
        for (i = 0; i < count; i++)
        {
            struct tagfile_entry *tfe;
            int seek;
            char buf[256];
            char *str = NULL;
            
            seek = check_virtual_tags(clause[i]->tag, idx);
            
            if (!tagcache_is_numeric_tag(clause[i]->tag))
            {
                if (clause[i]->tag == tag_filename)
                {
                    retrieve(tcs, idx, tag_filename, buf, sizeof buf);
                    str = buf;
                }
                else
                {
                    tfe = (struct tagfile_entry *)&hdr->tags[clause[i]->tag][seek];
                    str = tfe->tag_data;
                }
            }
        
            if (!check_against_clause(seek, str, clause[i]))
                return false;
        }
    }
    else
#endif
    {
        /* Check for conditions. */
        for (i = 0; i < count; i++)
        {
            struct tagfile_entry tfe;
            int seek;
            char str[256];
            
            seek = check_virtual_tags(clause[i]->tag, idx);
                
            memset(str, 0, sizeof str);
            if (!tagcache_is_numeric_tag(clause[i]->tag))
            {
                int fd = tcs->idxfd[clause[i]->tag];
                lseek(fd, seek, SEEK_SET);
                ecread(fd, &tfe, 1, tagfile_entry_ec, tc_stat.econ);
                if (tfe.tag_length >= (int)sizeof(str))
                {
                    logf("Too long tag read!");
                    break ;
                }

                read(fd, str, tfe.tag_length);
                
                /* Check if entry has been deleted. */
                if (str[0] == '\0')
                    break;
            }
            
            if (!check_against_clause(seek, str, clause[i]))
                return false;
        }
    }
    
    return true;
}

bool tagcache_check_clauses(struct tagcache_search *tcs,
                            struct tagcache_search_clause **clause, int count)
{
    struct index_entry idx;
    
    if (count == 0)
        return true;
    
    if (!get_index(tcs->masterfd, tcs->idx_id, &idx, true))
        return false;
    
    return check_clauses(tcs, &idx, clause, count);
}

static bool add_uniqbuf(struct tagcache_search *tcs, unsigned long id)
{
    int i;
    
    /* If uniq buffer is not defined we must return true for search to work. */
    if (tcs->unique_list == NULL 
        || (!tagcache_is_unique_tag(tcs->type)
            && !tagcache_is_numeric_tag(tcs->type)))
    {
        return true;
    }
    
    for (i = 0; i < tcs->unique_list_count; i++)
    {
        /* Return false if entry is found. */
        if (tcs->unique_list[i] == id)
            return false;
    }
    
    if (tcs->unique_list_count < tcs->unique_list_capacity)
    {
        tcs->unique_list[i] = id;
        tcs->unique_list_count++;
    }
    
    return true;
}

static bool build_lookup_list(struct tagcache_search *tcs)
{
    struct index_entry entry;
    int i;
    
    tcs->seek_list_count = 0;
    
#ifdef HAVE_TC_RAMCACHE
    if (tcs->ramsearch)
    {
        int j;

        for (i = tcs->seek_pos; i < hdr->h.tch.entry_count; i++)
        {
            struct index_entry *idx = &hdr->indices[i];
            if (tcs->seek_list_count == SEEK_LIST_SIZE)
                break ;
            
            /* Skip deleted files. */
            if (idx->flag & FLAG_DELETED)
                continue;
            
            /* Go through all filters.. */
            for (j = 0; j < tcs->filter_count; j++)
            {
                if (idx->tag_seek[tcs->filter_tag[j]] != tcs->filter_seek[j])
                {
                    break ;
                }
            }
            
            if (j < tcs->filter_count)
                continue ;

            /* Check for conditions. */
            if (!check_clauses(tcs, idx, tcs->clause, tcs->clause_count))
                continue;
            
            /* Add to the seek list if not already in uniq buffer. */
            if (!add_uniqbuf(tcs, idx->tag_seek[tcs->type]))
                continue;
            
            /* Lets add it. */
            tcs->seek_list[tcs->seek_list_count] = idx->tag_seek[tcs->type];
            tcs->seek_flags[tcs->seek_list_count] = idx->flag;
            tcs->seek_list_count++;
        }
        
        tcs->seek_pos = i;

        return tcs->seek_list_count > 0;
    }
#endif
    
    lseek(tcs->masterfd, tcs->seek_pos * sizeof(struct index_entry) +
            sizeof(struct master_header), SEEK_SET);
    
    while (ecread(tcs->masterfd, &entry, 1, index_entry_ec, tc_stat.econ) 
           == sizeof(struct index_entry))
    {
        if (tcs->seek_list_count == SEEK_LIST_SIZE)
            break ;

        tcs->seek_pos++;
        
        /* Check if entry has been deleted. */
        if (entry.flag & FLAG_DELETED)
            continue;
        
        /* Go through all filters.. */
        for (i = 0; i < tcs->filter_count; i++)
        {
            if (entry.tag_seek[tcs->filter_tag[i]] != tcs->filter_seek[i])
                break ;
        }
        
        if (i < tcs->filter_count)
            continue ;
        
        /* Check for conditions. */
        if (!check_clauses(tcs, &entry, tcs->clause, tcs->clause_count))
            continue;
            
        /* Add to the seek list if not already in uniq buffer. */
        if (!add_uniqbuf(tcs, entry.tag_seek[tcs->type]))
            continue;
            
        /* Lets add it. */
        tcs->seek_list[tcs->seek_list_count] = entry.tag_seek[tcs->type];
        tcs->seek_flags[tcs->seek_list_count] = entry.flag;
        tcs->seek_list_count++;
        
        yield();
    }

    return tcs->seek_list_count > 0;
}

        
static void remove_files(void)
{
    int i;
    char buf[MAX_PATH];
    
    tc_stat.ready = false;
    tc_stat.ramcache = false;
    tc_stat.econ = false;
    remove(TAGCACHE_FILE_MASTER);
    for (i = 0; i < TAG_COUNT; i++)
    {
        if (tagcache_is_numeric_tag(i))
            continue;
        
        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, i);
        remove(buf);
    }
}


static bool check_all_headers(void)
{
    struct master_header myhdr;
    struct tagcache_header tch;
    int tag;
    int fd;
    
    if ( (fd = open_master_fd(&myhdr, false)) < 0)
        return false;
    
    close(fd);
    if (myhdr.dirty)
    {
        logf("tagcache is dirty!");
        return false;
    }
    
    memcpy(&current_tcmh, &myhdr, sizeof(struct master_header));
    
    for (tag = 0; tag < TAG_COUNT; tag++)
    {
        if (tagcache_is_numeric_tag(tag))
            continue;
        
        if ( (fd = open_tag_fd(&tch, tag, false)) < 0)
            return false;
        
        close(fd);
    }
    
    return true;
}

bool tagcache_search(struct tagcache_search *tcs, int tag)
{
    struct tagcache_header tag_hdr;
    struct master_header   master_hdr;
    int i;

    if (tcs->initialized)
        tagcache_search_finish(tcs);
    
    while (read_lock)
        sleep(1);
    
    memset(tcs, 0, sizeof(struct tagcache_search));
    if (tc_stat.commit_step > 0 || !tc_stat.ready)
        return false;
    
    tcs->position = sizeof(struct tagcache_header);
    tcs->type = tag;
    tcs->seek_pos = 0;
    tcs->seek_list_count = 0;
    tcs->filter_count = 0;
    tcs->masterfd = -1;

    for (i = 0; i < TAG_COUNT; i++)
        tcs->idxfd[i] = -1;

#ifndef HAVE_TC_RAMCACHE
    tcs->ramsearch = false;
#else
    tcs->ramsearch = tc_stat.ramcache;
    if (tcs->ramsearch)
    {
        tcs->entry_count = hdr->entry_count[tcs->type];
    }
    else
#endif
    {
        if (!tagcache_is_numeric_tag(tcs->type))
        {
            tcs->idxfd[tcs->type] = open_tag_fd(&tag_hdr, tcs->type, false);
            if (tcs->idxfd[tcs->type] < 0)
                return false;
        }
        
        /* Always open as R/W so we can pass tcs to functions that modify data also
         * without failing. */
        tcs->masterfd = open_master_fd(&master_hdr, true);
        
        if (tcs->masterfd < 0)
            return false;
    }

    tcs->valid = true;
    tcs->initialized = true;
    write_lock++;
    
    return true;
}

void tagcache_search_set_uniqbuf(struct tagcache_search *tcs,
                                 void *buffer, long length)
{
    tcs->unique_list = (unsigned long *)buffer;
    tcs->unique_list_capacity = length / sizeof(*tcs->unique_list);
    tcs->unique_list_count = 0;
}

bool tagcache_search_add_filter(struct tagcache_search *tcs,
                                int tag, int seek)
{
    if (tcs->filter_count == TAGCACHE_MAX_FILTERS)
        return false;

    if (!tagcache_is_unique_tag(tag) || tagcache_is_numeric_tag(tag))
        return false;
    
    tcs->filter_tag[tcs->filter_count] = tag;
    tcs->filter_seek[tcs->filter_count] = seek;
    tcs->filter_count++;

    return true;
}

bool tagcache_search_add_clause(struct tagcache_search *tcs,
                                struct tagcache_search_clause *clause)
{
    int i;
    
    if (tcs->clause_count >= TAGCACHE_MAX_CLAUSES)
    {
        logf("Too many clauses");
        return false;
    }

    /* Check if there is already a similar filter in present (filters are
     * much faster than clauses). 
     */
    for (i = 0; i < tcs->filter_count; i++)
    {
        if (tcs->filter_tag[i] == clause->tag)
            return true;
    }
    
    if (!tagcache_is_numeric_tag(clause->tag) && tcs->idxfd[clause->tag] < 0)
    {
        char buf[MAX_PATH];

        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, clause->tag);        
        tcs->idxfd[clause->tag] = open(buf, O_RDONLY);
    }
    
    tcs->clause[tcs->clause_count] = clause;
    tcs->clause_count++;
    
    return true;
}

/* TODO: Remove this mess. */
#ifdef HAVE_DIRCACHE
#define TAG_FILENAME_RAM(tcs) ((tcs->type == tag_filename) \
    ? ((flag & FLAG_DIRCACHE) && is_dircache_intact()) : 1)
#else
#define TAG_FILENAME_RAM(tcs) (tcs->type != tag_filename)
#endif

static bool get_next(struct tagcache_search *tcs)
{
    static char buf[TAG_MAXLEN+32];
    struct tagfile_entry entry;
    long flag = 0;

    if (!tcs->valid || !tc_stat.ready)
        return false;
        
    if (tcs->idxfd[tcs->type] < 0 && !tagcache_is_numeric_tag(tcs->type)
#ifdef HAVE_TC_RAMCACHE
        && !tcs->ramsearch
#endif
        )
        return false;
    
    /* Relative fetch. */
    if (tcs->filter_count > 0 || tcs->clause_count > 0
        || tagcache_is_numeric_tag(tcs->type))
    {
        /* Check for end of list. */
        if (tcs->seek_list_count == 0)
        {
            /* Try to fetch more. */
            if (!build_lookup_list(tcs))
            {
                tcs->valid = false;
                return false;
            }
        }
        
        tcs->seek_list_count--;
        flag = tcs->seek_flags[tcs->seek_list_count];
        
        /* Seek stream to the correct position and continue to direct fetch. */
        if ((!tcs->ramsearch || !TAG_FILENAME_RAM(tcs))
            && !tagcache_is_numeric_tag(tcs->type))
        {
            if (!open_files(tcs, tcs->type))
                return false;
        
            lseek(tcs->idxfd[tcs->type], tcs->seek_list[tcs->seek_list_count], SEEK_SET);
        }
        else
            tcs->position = tcs->seek_list[tcs->seek_list_count];
    }
    
    if (tagcache_is_numeric_tag(tcs->type))
    {
        snprintf(buf, sizeof(buf), "%d", tcs->position);
        tcs->result_seek = tcs->position;
        tcs->result = buf;
        tcs->result_len = strlen(buf) + 1;
        return true;
    }
    
    /* Direct fetch. */
#ifdef HAVE_TC_RAMCACHE
    if (tcs->ramsearch && TAG_FILENAME_RAM(tcs))
    {
        struct tagfile_entry *ep;
        
        if (tcs->entry_count == 0)
        {
            tcs->valid = false;
            return false;
        }
        tcs->entry_count--;
        
        tcs->result_seek = tcs->position;

# ifdef HAVE_DIRCACHE
        if (tcs->type == tag_filename)
        {
            dircache_copy_path((struct dirent *)tcs->position,
                               buf, sizeof buf);
            tcs->result = buf;
            tcs->result_len = strlen(buf) + 1;
            tcs->idx_id = FLAG_GET_ATTR(flag);
            tcs->ramresult = false;
            
            return true;
        }
# endif
        
        ep = (struct tagfile_entry *)&hdr->tags[tcs->type][tcs->position];
        tcs->position += sizeof(struct tagfile_entry) + ep->tag_length;
        tcs->result = ep->tag_data;
        tcs->result_len = strlen(tcs->result) + 1;
        tcs->idx_id = ep->idx_id;
        tcs->ramresult = true;
        
        return true;
    }
    else
#endif
    {
        if (!open_files(tcs, tcs->type))
            return false;
        
        tcs->result_seek = lseek(tcs->idxfd[tcs->type], 0, SEEK_CUR);
        if (ecread(tcs->idxfd[tcs->type], &entry, 1,
                 tagfile_entry_ec, tc_stat.econ) != sizeof(struct tagfile_entry))
        {
            /* End of data. */
            tcs->valid = false;
            return false;
        }
    }
    
    if (entry.tag_length > (long)sizeof(buf))
    {
        tcs->valid = false;
        logf("too long tag #2");
        return false;
    }
    
    if (read(tcs->idxfd[tcs->type], buf, entry.tag_length) != entry.tag_length)
    {
        tcs->valid = false;
        logf("read error #4");
        return false;
    }
    
    tcs->result = buf;
    tcs->result_len = strlen(tcs->result) + 1;
    tcs->idx_id = entry.idx_id;
    tcs->ramresult = false;

    return true;
}

bool tagcache_get_next(struct tagcache_search *tcs)
{
    while (get_next(tcs))
    {
        if (tcs->result_len > 1)
            return true;
    }
    
    return false;
}

bool tagcache_retrieve(struct tagcache_search *tcs, int idxid, 
                       int tag, char *buf, long size)
{
    struct index_entry idx;
    
    *buf = '\0';
    if (!get_index(tcs->masterfd, idxid, &idx, true))
        return false;
    
    return retrieve(tcs, &idx, tag, buf, size);
}

static bool update_master_header(void)
{
    struct master_header myhdr;
    int fd;
    
    if (!tc_stat.ready)
        return false;
    
    if ( (fd = open_master_fd(&myhdr, true)) < 0)
        return false;
    
    myhdr.serial = current_tcmh.serial;
    myhdr.commitid = current_tcmh.commitid;
    
    /* Write it back */
    lseek(fd, 0, SEEK_SET);
    ecwrite(fd, &myhdr, 1, master_header_ec, tc_stat.econ);
    close(fd);
    
#ifdef HAVE_TC_RAMCACHE
    if (hdr)
    {
        hdr->h.serial = current_tcmh.serial;
        hdr->h.commitid = current_tcmh.commitid;
    }
#endif
    
    return true;
}

#if 0

void tagcache_modify(struct tagcache_search *tcs, int type, const char *text)
{
    struct tagentry *entry;
    
    if (tcs->type != tag_title)
        return ;

    /* We will need reserve buffer for this. */
    if (tcs->ramcache)
    {
        struct tagfile_entry *ep;

        ep = (struct tagfile_entry *)&hdr->tags[tcs->type][tcs->result_seek];
        tcs->seek_list[tcs->seek_list_count];
    }

    entry = find_entry_ram();
    
}
#endif

void tagcache_search_finish(struct tagcache_search *tcs)
{
    int i;
    
    if (!tcs->initialized)
        return;
    
    if (tcs->masterfd >= 0)
    {
        close(tcs->masterfd);
        tcs->masterfd = -1;
    }

    for (i = 0; i < TAG_COUNT; i++)
    {
        if (tcs->idxfd[i] >= 0)
        {
            close(tcs->idxfd[i]);
            tcs->idxfd[i] = -1;
        }
    }
    
    tcs->ramsearch = false;
    tcs->valid = false;
    tcs->initialized = 0;
    if (write_lock > 0)
        write_lock--;
}

#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
static struct tagfile_entry *get_tag(const struct index_entry *entry, int tag)
{
    return (struct tagfile_entry *)&hdr->tags[tag][entry->tag_seek[tag]];
}

static long get_tag_numeric(const struct index_entry *entry, int tag)
{
    return check_virtual_tags(tag, entry);
}

static char* get_tag_string(const struct index_entry *entry, int tag)
{
    char* s = get_tag(entry, tag)->tag_data;
    return strcmp(s, UNTAGGED) ? s : NULL;
}

bool tagcache_fill_tags(struct mp3entry *id3, const char *filename)
{
    struct index_entry *entry;
    int idx_id;
    
    if (!tc_stat.ready)
        return false;
    
    /* Find the corresponding entry in tagcache. */
    idx_id = find_entry_ram(filename, NULL);
    if (idx_id < 0 || !tc_stat.ramcache)
        return false;
    
    entry = &hdr->indices[idx_id];
    
    id3->title        = get_tag_string(entry, tag_title);
    id3->artist       = get_tag_string(entry, tag_artist);
    id3->album        = get_tag_string(entry, tag_album);
    id3->genre_string = get_tag_string(entry, tag_genre);
    id3->composer     = get_tag_string(entry, tag_composer);
    id3->comment      = get_tag_string(entry, tag_comment);
    id3->albumartist  = get_tag_string(entry, tag_albumartist);
    id3->grouping     = get_tag_string(entry, tag_grouping);

    id3->playcount  = get_tag_numeric(entry, tag_playcount);
    id3->rating     = get_tag_numeric(entry, tag_rating);
    id3->lastplayed = get_tag_numeric(entry, tag_lastplayed);
    id3->score      = get_tag_numeric(entry, tag_virt_autoscore) / 10;
    id3->year       = get_tag_numeric(entry, tag_year);

    id3->discnum = get_tag_numeric(entry, tag_discnumber);
    id3->tracknum = get_tag_numeric(entry, tag_tracknumber);
    id3->bitrate = get_tag_numeric(entry, tag_bitrate);
    if (id3->bitrate == 0)
        id3->bitrate = 1;

    return true;
}
#endif

static inline void write_item(const char *item)
{
    int len = strlen(item) + 1;

    data_size += len;
    write(cachefd, item, len);
}

static int check_if_empty(char **tag)
{
    int length;

    if (*tag == NULL || **tag == '\0')
    {
        *tag = UNTAGGED;
        return sizeof(UNTAGGED); /* Tag length */
    }

    length = strlen(*tag);
    if (length > TAG_MAXLEN)
    {
        logf("over length tag: %s", *tag);
        length = TAG_MAXLEN;
        (*tag)[length] = '\0';
    }

    return length + 1;
}

#define ADD_TAG(entry,tag,data) \
    /* Adding tag */ \
    entry.tag_offset[tag] = offset; \
    entry.tag_length[tag] = check_if_empty(data); \
    offset += entry.tag_length[tag]
    
static void add_tagcache(char *path, unsigned long mtime
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
                         ,const struct dirent *dc
#endif
                         )
{
    struct mp3entry id3;
    struct temp_file_entry entry;
    bool ret;
    int fd;
    int idx_id = -1;
    char tracknumfix[3];
    int offset = 0;
    int path_length = strlen(path);
    bool has_albumartist;
    bool has_grouping;

    if (cachefd < 0)
        return ;

    /* Check for overlength file path. */
    if (path_length > TAG_MAXLEN)
    {
        /* Path can't be shortened. */
        logf("Too long path: %s", path);
        return ;
    }
    
    /* Check if the file is supported. */
    if (probe_file_format(path) == AFMT_UNKNOWN)
        return ;
    
    /* Check if the file is already cached. */
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    if (tc_stat.ramcache && is_dircache_intact())
    {
        idx_id = find_entry_ram(path, dc);
    }
    else
#endif
    {
        if (filenametag_fd >= 0)
        {
            idx_id = find_entry_disk(path);
        }
    }
    
    /* Check if file has been modified. */
    if (idx_id >= 0)
    {
        struct index_entry idx;
        
        /* TODO: Mark that the index exists (for fast reverse scan) */
        //found_idx[idx_id/8] |= idx_id%8;
        
        if (!get_index(-1, idx_id, &idx, true))
        {
            logf("failed to retrieve index entry");
            return ;
        }
        
        if ((unsigned long)idx.tag_seek[tag_mtime] == mtime)
        {
            /* No changes to file. */
            return ;
        }
        
        /* Metadata might have been changed. Delete the entry. */
        logf("Re-adding: %s", path);
        if (!delete_entry(idx_id))
        {
            logf("delete_entry failed: %d", idx_id);
            return ;
        }
    }
    
    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        logf("open fail: %s", path);
        return ;
    }

    memset(&id3, 0, sizeof(struct mp3entry));
    memset(&entry, 0, sizeof(struct temp_file_entry));
    memset(&tracknumfix, 0, sizeof(tracknumfix));
    ret = get_metadata(&id3, fd, path);
    close(fd);

    if (!ret)
        return ;

    logf("-> %s", path);
    
    /* Generate track number if missing. */
    if (id3.tracknum <= 0)
    {
        const char *p = strrchr(path, '.');
        
        if (p == NULL)
            p = &path[strlen(path)-1];
        
        while (*p != '/')
        {
            if (isdigit(*p) && isdigit(*(p-1)))
            {
                tracknumfix[1] = *p--;
                tracknumfix[0] = *p;
                break;
            }
            p--;
        }
        
        if (tracknumfix[0] != '\0')
        {
            id3.tracknum = atoi(tracknumfix);
            /* Set a flag to indicate track number has been generated. */
            entry.flag |= FLAG_TRKNUMGEN;
        }
        else
        {
            /* Unable to generate track number. */
            id3.tracknum = -1;
        }
    }
    
    /* Numeric tags */
    entry.tag_offset[tag_year] = id3.year;
    entry.tag_offset[tag_discnumber] = id3.discnum;
    entry.tag_offset[tag_tracknumber] = id3.tracknum;
    entry.tag_offset[tag_length] = id3.length;
    entry.tag_offset[tag_bitrate] = id3.bitrate;
    entry.tag_offset[tag_mtime] = mtime;
    
    /* String tags. */
    has_albumartist = id3.albumartist != NULL
        && strlen(id3.albumartist) > 0;
    has_grouping = id3.grouping != NULL
        && strlen(id3.grouping) > 0;

    ADD_TAG(entry, tag_filename, &path);
    ADD_TAG(entry, tag_title, &id3.title);
    ADD_TAG(entry, tag_artist, &id3.artist);
    ADD_TAG(entry, tag_album, &id3.album);
    ADD_TAG(entry, tag_genre, &id3.genre_string);
    ADD_TAG(entry, tag_composer, &id3.composer);
    ADD_TAG(entry, tag_comment, &id3.comment);
    if (has_albumartist)
    {
        ADD_TAG(entry, tag_albumartist, &id3.albumartist);
    }
    else
    {
        ADD_TAG(entry, tag_albumartist, &id3.artist);
    }
    if (has_grouping)
    {
        ADD_TAG(entry, tag_grouping, &id3.grouping);
    }
    else
    {
        ADD_TAG(entry, tag_grouping, &id3.title);
    }
    entry.data_length = offset;
    
    /* Write the header */
    write(cachefd, &entry, sizeof(struct temp_file_entry));
    
    /* And tags also... Correct order is critical */
    write_item(path);
    write_item(id3.title);
    write_item(id3.artist);
    write_item(id3.album);
    write_item(id3.genre_string);
    write_item(id3.composer);
    write_item(id3.comment);
    if (has_albumartist)
    {
        write_item(id3.albumartist);
    }
    else
    {
        write_item(id3.artist);
    }
    if (has_grouping)
    {
        write_item(id3.grouping);
    }
    else
    {
        write_item(id3.title);
    }
    total_entry_count++;    
}

static bool tempbuf_insert(char *str, int id, int idx_id, bool unique)
{
    struct tempbuf_searchidx *index = (struct tempbuf_searchidx *)tempbuf;
    int len = strlen(str)+1;
    int i;
    unsigned crc32;
    unsigned *crcbuf = (unsigned *)&tempbuf[tempbuf_size-4];
    char buf[TAG_MAXLEN+32];
    
    for (i = 0; str[i] != '\0' && i < (int)sizeof(buf)-1; i++)
        buf[i] = tolower(str[i]);
    buf[i] = '\0';
    
    crc32 = crc_32(buf, i, 0xffffffff);
    
    if (unique)
    {
        /* Check if the crc does not exist -> entry does not exist for sure. */
        for (i = 0; i < tempbufidx; i++)
        {
            if (crcbuf[-i] != crc32)
                continue;
            
            if (!strcasecmp(str, index[i].str))
            {
                if (id < 0 || id >= lookup_buffer_depth)
                {
                    logf("lookup buf overf.: %d", id);
                    return false;
                }
                
                lookup[id] = &index[i];
                return true;
            }
        }
    }
    
    /* Insert to CRC buffer. */
    crcbuf[-tempbufidx] = crc32;
    tempbuf_left -= 4;
    
    /* Insert it to the buffer. */
    tempbuf_left -= len;
    if (tempbuf_left - 4 < 0 || tempbufidx >= commit_entry_count-1)
        return false;
    
    if (id >= lookup_buffer_depth)
    {
        logf("lookup buf overf. #2: %d", id);
        return false;
    }

    if (id >= 0)
    {
        lookup[id] = &index[tempbufidx];
        index[tempbufidx].idlist.id = id;
    }
    else
        index[tempbufidx].idlist.id = -1;
    
    index[tempbufidx].idlist.next = NULL;
    index[tempbufidx].idx_id = idx_id;
    index[tempbufidx].seek = -1;
    index[tempbufidx].str = &tempbuf[tempbuf_pos];
    memcpy(index[tempbufidx].str, str, len);
    tempbuf_pos += len;
    tempbufidx++;
    
    return true;
}

static int compare(const void *p1, const void *p2)
{
    do_timed_yield();

    struct tempbuf_searchidx *e1 = (struct tempbuf_searchidx *)p1;
    struct tempbuf_searchidx *e2 = (struct tempbuf_searchidx *)p2;
    
    if (strcmp(e1->str, UNTAGGED) == 0)
    {
        if (strcmp(e2->str, UNTAGGED) == 0)
            return 0;
        return -1;
    }
    else if (strcmp(e2->str, UNTAGGED) == 0)
        return 1;
    
    return strncasecmp(e1->str, e2->str, TAG_MAXLEN);
}

static int tempbuf_sort(int fd)
{
    struct tempbuf_searchidx *index = (struct tempbuf_searchidx *)tempbuf;
    struct tagfile_entry fe;
    int i;
    int length;
    
    /* Generate reverse lookup entries. */
    for (i = 0; i < lookup_buffer_depth; i++)
    {
        struct tempbuf_id_list *idlist;
        
        if (!lookup[i])
            continue;
        
        if (lookup[i]->idlist.id == i)
            continue;
        
        idlist = &lookup[i]->idlist;
        while (idlist->next != NULL)
            idlist = idlist->next;
        
        tempbuf_left -= sizeof(struct tempbuf_id_list);
        if (tempbuf_left - 4 < 0)
            return -1;
        
        idlist->next = (struct tempbuf_id_list *)&tempbuf[tempbuf_pos];
        if (tempbuf_pos & 0x03)
        {
            tempbuf_pos = (tempbuf_pos & ~0x03) + 0x04;
            tempbuf_left -= 3;
            idlist->next = (struct tempbuf_id_list *)&tempbuf[tempbuf_pos];
        }
        tempbuf_pos += sizeof(struct tempbuf_id_list);
        
        idlist = idlist->next;
        idlist->id = i;
        idlist->next = NULL;

        do_timed_yield();
    }
    
    qsort(index, tempbufidx, sizeof(struct tempbuf_searchidx), compare);
    memset(lookup, 0, lookup_buffer_depth * sizeof(struct tempbuf_searchidx **));
    
    for (i = 0; i < tempbufidx; i++)
    {
        struct tempbuf_id_list *idlist = &index[i].idlist;
        
        /* Fix the lookup list. */
        while (idlist != NULL)
        {
            if (idlist->id >= 0)
                lookup[idlist->id] = &index[i];
            idlist = idlist->next;
        }
        
        index[i].seek = lseek(fd, 0, SEEK_CUR);
        length = strlen(index[i].str) + 1;
        fe.tag_length = length;
        fe.idx_id = index[i].idx_id;
        
        /* Check the chunk alignment. */
        if ((fe.tag_length + sizeof(struct tagfile_entry)) 
            % TAGFILE_ENTRY_CHUNK_LENGTH)
        {
            fe.tag_length += TAGFILE_ENTRY_CHUNK_LENGTH - 
                ((fe.tag_length + sizeof(struct tagfile_entry)) 
                 % TAGFILE_ENTRY_CHUNK_LENGTH);
        }
        
#ifdef TAGCACHE_STRICT_ALIGN
        /* Make sure the entry is long aligned. */
        if (index[i].seek & 0x03)
        {
            logf("tempbuf_sort: alignment error!");
            return -3;
        }
#endif
        
        if (ecwrite(fd, &fe, 1, tagfile_entry_ec, tc_stat.econ) !=
            sizeof(struct tagfile_entry))
        {
            logf("tempbuf_sort: write error #1");
            return -1;
        }
        
        if (write(fd, index[i].str, length) != length)
        {
            logf("tempbuf_sort: write error #2");
            return -2;
        }
        
        /* Write some padding. */
        if (fe.tag_length - length > 0)
            write(fd, "XXXXXXXX", fe.tag_length - length);
    }

    return i;
}
    
inline static struct tempbuf_searchidx* tempbuf_locate(int id)
{
    if (id < 0 || id >= lookup_buffer_depth)
        return NULL;
    
    return lookup[id];
}


inline static int tempbuf_find_location(int id)
{
    struct tempbuf_searchidx *entry;

    entry = tempbuf_locate(id);
    if (entry == NULL)
        return -1;

    return entry->seek;
}

static bool build_numeric_indices(struct tagcache_header *h, int tmpfd)
{
    struct master_header tcmh;
    struct index_entry idx;
    int masterfd;
    int masterfd_pos;
    struct temp_file_entry *entrybuf = (struct temp_file_entry *)tempbuf;
    int max_entries;
    int entries_processed = 0;
    int i, j;
    char buf[TAG_MAXLEN];
    
    max_entries = tempbuf_size / sizeof(struct temp_file_entry) - 1;
    
    logf("Building numeric indices...");
    lseek(tmpfd, sizeof(struct tagcache_header), SEEK_SET);
    
    if ( (masterfd = open_master_fd(&tcmh, true)) < 0)
        return false;
    
    masterfd_pos = lseek(masterfd, tcmh.tch.entry_count * sizeof(struct index_entry),
                         SEEK_CUR);
    if (masterfd_pos == filesize(masterfd))
    {
        logf("we can't append!");
        close(masterfd);
        return false;
    }

    while (entries_processed < h->entry_count)
    {
        int count = MIN(h->entry_count - entries_processed, max_entries);
         
        /* Read in as many entries as possible. */
        for (i = 0; i < count; i++)
        {
            struct temp_file_entry *tfe = &entrybuf[i];
            int datastart;
            
            /* Read in numeric data. */
            if (read(tmpfd, tfe, sizeof(struct temp_file_entry)) !=
                sizeof(struct temp_file_entry))
            {
                logf("read fail #1");
                close(masterfd);
                return false;
            }

            datastart = lseek(tmpfd, 0, SEEK_CUR);
            
            /**
             * Read string data from the following tags:
             * - tag_filename
             * - tag_artist
             * - tag_album
             * - tag_title
             * 
             * A crc32 hash is calculated from the read data
             * and stored back to the data offset field kept in memory.
             */
#define tmpdb_read_string_tag(tag) \
    lseek(tmpfd, tfe->tag_offset[tag], SEEK_CUR); \
    if ((unsigned long)tfe->tag_length[tag] > sizeof buf) \
    { \
        logf("read fail: buffer overflow"); \
        close(masterfd); \
        return false; \
    } \
    \
    if (read(tmpfd, buf, tfe->tag_length[tag]) != \
        tfe->tag_length[tag]) \
    { \
        logf("read fail #2"); \
        close(masterfd); \
        return false; \
    } \
    \
    tfe->tag_offset[tag] = crc_32(buf, strlen(buf), 0xffffffff); \
    lseek(tmpfd, datastart, SEEK_SET)
            
            tmpdb_read_string_tag(tag_filename);
            tmpdb_read_string_tag(tag_artist);
            tmpdb_read_string_tag(tag_album);
            tmpdb_read_string_tag(tag_title);
            
            /* Seek to the end of the string data. */
            lseek(tmpfd, tfe->data_length, SEEK_CUR);
        }
        
        /* Backup the master index position. */
        masterfd_pos = lseek(masterfd, 0, SEEK_CUR);
        lseek(masterfd, sizeof(struct master_header), SEEK_SET);
        
        /* Check if we can resurrect some deleted runtime statistics data. */
        for (i = 0; i < tcmh.tch.entry_count; i++)
        {
            /* Read the index entry. */
            if (ecread(masterfd, &idx, 1, index_entry_ec, tc_stat.econ) 
                != sizeof(struct index_entry))
            {
                logf("read fail #3");
                close(masterfd);
                return false;
            }
            
            /**
             * Skip unless the entry is marked as being deleted
             * or the data has already been resurrected.
             */
            if (!(idx.flag & FLAG_DELETED) || idx.flag & FLAG_RESURRECTED)
                continue;
            
            /* Now try to match the entry. */
            /**
             * To succesfully match a song, the following conditions
             * must apply:
             * 
             * For numeric fields: tag_length
             * - Full identical match is required
             * 
             * If tag_filename matches, no further checking necessary.
             * 
             * For string hashes: tag_artist, tag_album, tag_title
             * - Two of these must match
             */
            for (j = 0; j < count; j++)
            {
                struct temp_file_entry *tfe = &entrybuf[j];
                
                /* Try to match numeric fields first. */
                if (tfe->tag_offset[tag_length] != idx.tag_seek[tag_length])
                    continue;
                
                /* Now it's time to do the hash matching. */
                if (tfe->tag_offset[tag_filename] != idx.tag_seek[tag_filename])
                {
                    int match_count = 0;
                    
                    /* No filename match, check if we can match two other tags. */
#define tmpdb_match(tag) \
    if (tfe->tag_offset[tag] == idx.tag_seek[tag]) \
        match_count++
                    
                    tmpdb_match(tag_artist);
                    tmpdb_match(tag_album);
                    tmpdb_match(tag_title);
                    
                    if (match_count < 2)
                    {
                        /* Still no match found, give up. */
                        continue;
                    }
                }
                
                /* A match found, now copy & resurrect the statistical data. */
#define tmpdb_copy_tag(tag) \
    tfe->tag_offset[tag] = idx.tag_seek[tag]
                
                tmpdb_copy_tag(tag_playcount);
                tmpdb_copy_tag(tag_rating);
                tmpdb_copy_tag(tag_playtime);
                tmpdb_copy_tag(tag_lastplayed);
                tmpdb_copy_tag(tag_commitid);
                
                /* Avoid processing this entry again. */
                idx.flag |= FLAG_RESURRECTED;
                
                lseek(masterfd, -sizeof(struct index_entry), SEEK_CUR);
                if (ecwrite(masterfd, &idx, 1, index_entry_ec, tc_stat.econ) 
                    != sizeof(struct index_entry))
                {
                    logf("masterfd writeback fail #1");
                    close(masterfd);
                    return false;
                }
                
                logf("Entry resurrected");
            }
        }
        
        
        /* Restore the master index position. */
        lseek(masterfd, masterfd_pos, SEEK_SET);
        
        /* Commit the data to the index. */
        for (i = 0; i < count; i++)
        {
            int loc = lseek(masterfd, 0, SEEK_CUR);
            
            if (ecread(masterfd, &idx, 1, index_entry_ec, tc_stat.econ) 
                != sizeof(struct index_entry))
            {
                logf("read fail #3");
                close(masterfd);
                return false;
            }
            
            for (j = 0; j < TAG_COUNT; j++)
            {
                if (!tagcache_is_numeric_tag(j))
                    continue;
                
                idx.tag_seek[j] = entrybuf[i].tag_offset[j];
            }
            idx.flag = entrybuf[i].flag;
            
            if (idx.tag_seek[tag_commitid])
            {
                /* Data has been resurrected. */
                idx.flag |= FLAG_DIRTYNUM;
            }
            else if (tc_stat.ready && current_tcmh.commitid > 0)
            {
                idx.tag_seek[tag_commitid] = current_tcmh.commitid;
                idx.flag |= FLAG_DIRTYNUM;
            }
            
            /* Write back the updated index. */
            lseek(masterfd, loc, SEEK_SET);
            if (ecwrite(masterfd, &idx, 1, index_entry_ec, tc_stat.econ) 
                != sizeof(struct index_entry))
            {
                logf("write fail");
                close(masterfd);
                return false;
            }
        }
        
        entries_processed += count;
        logf("%d/%ld entries processed", entries_processed, h->entry_count);
    }
    
    close(masterfd);
    
    return true;
}

/**
 * Return values:
 *     > 0   success
 *    == 0   temporary failure
 *     < 0   fatal error
 */
static int build_index(int index_type, struct tagcache_header *h, int tmpfd)
{
    int i;
    struct tagcache_header tch;
    struct master_header   tcmh;
    struct index_entry idxbuf[IDX_BUF_DEPTH];
    int idxbuf_pos;
    char buf[TAG_MAXLEN+32];
    int fd = -1, masterfd;
    bool error = false;
    int init;
    int masterfd_pos;
    
    logf("Building index: %d", index_type);
    
    /* Check the number of entries we need to allocate ram for. */
    commit_entry_count = h->entry_count + 1;
    
    masterfd = open_master_fd(&tcmh, false);
    if (masterfd >= 0)
    {
        commit_entry_count += tcmh.tch.entry_count;
        close(masterfd);
    }
    else
        remove_files(); /* Just to be sure we are clean. */

    /* Open the index file, which contains the tag names. */
    fd = open_tag_fd(&tch, index_type, true);
    if (fd >= 0)
    {
        logf("tch.datasize=%ld", tch.datasize);
        lookup_buffer_depth = 1 +
        /* First part */ commit_entry_count +
        /* Second part */ (tch.datasize / TAGFILE_ENTRY_CHUNK_LENGTH);
    }
    else
    {
        lookup_buffer_depth = 1 +
        /* First part */ commit_entry_count +
        /* Second part */ 0;
    }
    
    logf("lookup_buffer_depth=%ld", lookup_buffer_depth);
    logf("commit_entry_count=%ld", commit_entry_count);
    
    /* Allocate buffer for all index entries from both old and new
     * tag files. */
    tempbufidx = 0;
    tempbuf_pos = commit_entry_count * sizeof(struct tempbuf_searchidx);

    /* Allocate lookup buffer. The first portion of commit_entry_count
     * contains the new tags in the temporary file and the second
     * part for locating entries already in the db.
     * 
     *  New tags  Old tags
     * +---------+---------------------------+
     * |  index  | position/ENTRY_CHUNK_SIZE |  lookup buffer
     * +---------+---------------------------+
     *              
     * Old tags are inserted to a temporary buffer with position:
     *     tempbuf_insert(position/ENTRY_CHUNK_SIZE, ...);
     * And new tags with index:
     *     tempbuf_insert(idx, ...);
     * 
     * The buffer is sorted and written into tag file:
     *     tempbuf_sort(...);
     * leaving master index locations messed up.
     * 
     * That is fixed using the lookup buffer for old tags:
     *     new_seek = tempbuf_find_location(old_seek, ...);
     * and for new tags:
     *     new_seek = tempbuf_find_location(idx);
     */
    lookup = (struct tempbuf_searchidx **)&tempbuf[tempbuf_pos];
    tempbuf_pos += lookup_buffer_depth * sizeof(void **);
    memset(lookup, 0, lookup_buffer_depth * sizeof(void **));
    
    /* And calculate the remaining data space used mainly for storing
     * tag data (strings). */
    tempbuf_left = tempbuf_size - tempbuf_pos - 8;
    if (tempbuf_left - TAGFILE_ENTRY_AVG_LENGTH * commit_entry_count < 0)
    {
        logf("Buffer way too small!");
        return 0;
    }

    if (fd >= 0)
    {
        /**
         * If tag file contains unique tags (sorted index), we will load
         * it entirely into memory so we can resort it later for use with
         * chunked browsing.
         */
        if (tagcache_is_sorted_tag(index_type))
        {
            logf("loading tags...");
            for (i = 0; i < tch.entry_count; i++)
            {
                struct tagfile_entry entry;
                int loc = lseek(fd, 0, SEEK_CUR);
                bool ret;
                
                if (ecread(fd, &entry, 1, tagfile_entry_ec, tc_stat.econ)
                    != sizeof(struct tagfile_entry))
                {
                    logf("read error #7");
                    close(fd);
                    return -2;
                }
                
                if (entry.tag_length >= (int)sizeof(buf))
                {
                    logf("too long tag #3");
                    close(fd);
                    return -2;
                }
                
                if (read(fd, buf, entry.tag_length) != entry.tag_length)
                {
                    logf("read error #8");
                    close(fd);
                    return -2;
                }

                /* Skip deleted entries. */
                if (buf[0] == '\0')
                    continue;
                
                /**
                 * Save the tag and tag id in the memory buffer. Tag id
                 * is saved so we can later reindex the master lookup
                 * table when the index gets resorted.
                 */
                ret = tempbuf_insert(buf, loc/TAGFILE_ENTRY_CHUNK_LENGTH 
                                     + commit_entry_count, entry.idx_id,
                                     tagcache_is_unique_tag(index_type));
                if (!ret)
                {
                    close(fd);
                    return -3;
                }
                do_timed_yield();
            }
            logf("done");
        }
        else
            tempbufidx = tch.entry_count;
    }
    else
    {
        /**
         * Creating new index file to store the tags. No need to preload
         * anything whether the index type is sorted or not.
         */
        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, index_type);
        fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd < 0)
        {
            logf("%s open fail", buf);
            return -2;
        }
        
        tch.magic = TAGCACHE_MAGIC;
        tch.entry_count = 0;
        tch.datasize = 0;
        
        if (ecwrite(fd, &tch, 1, tagcache_header_ec, tc_stat.econ) 
            != sizeof(struct tagcache_header))
        {
            logf("header write failed");
            close(fd);
            return -2;
        }
    }

    /* Loading the tag lookup file as "master file". */
    logf("Loading index file");
    masterfd = open(TAGCACHE_FILE_MASTER, O_RDWR);

    if (masterfd < 0)
    {
        logf("Creating new DB");
        masterfd = open(TAGCACHE_FILE_MASTER, O_WRONLY | O_CREAT | O_TRUNC);

        if (masterfd < 0)
        {
            logf("Failure to create index file (%s)", TAGCACHE_FILE_MASTER);
            close(fd);
            return -2;
        }

        /* Write the header (write real values later). */
        memset(&tcmh, 0, sizeof(struct master_header));
        tcmh.tch = *h;
        tcmh.tch.entry_count = 0;
        tcmh.tch.datasize = 0;
        tcmh.dirty = true;
        ecwrite(masterfd, &tcmh, 1, master_header_ec, tc_stat.econ);
        init = true;
        masterfd_pos = lseek(masterfd, 0, SEEK_CUR);
    }
    else
    {
        /**
         * Master file already exists so we need to process the current
         * file first.
         */
        init = false;

        if (ecread(masterfd, &tcmh, 1, master_header_ec, tc_stat.econ) !=
            sizeof(struct master_header) || tcmh.tch.magic != TAGCACHE_MAGIC)
        {
            logf("header error");
            close(fd);
            close(masterfd);
            return -2;
        }

        /**
         * If we reach end of the master file, we need to expand it to
         * hold new tags. If the current index is not sorted, we can
         * simply append new data to end of the file.
         * However, if the index is sorted, we need to update all tag
         * pointers in the master file for the current index.
         */
        masterfd_pos = lseek(masterfd, tcmh.tch.entry_count * sizeof(struct index_entry),
            SEEK_CUR);
        if (masterfd_pos == filesize(masterfd))
        {
            logf("appending...");
            init = true;
        }
    }

    /**
     * Load new unique tags in memory to be sorted later and added
     * to the master lookup file.
     */
    if (tagcache_is_sorted_tag(index_type))
    {
        lseek(tmpfd, sizeof(struct tagcache_header), SEEK_SET);
        /* h is the header of the temporary file containing new tags. */
        logf("inserting new tags...");
        for (i = 0; i < h->entry_count; i++)
        {
            struct temp_file_entry entry;
            
            if (read(tmpfd, &entry, sizeof(struct temp_file_entry)) !=
                sizeof(struct temp_file_entry))
            {
                logf("read fail #3");
                error = true;
                goto error_exit;
            }

            /* Read data. */
            if (entry.tag_length[index_type] >= (long)sizeof(buf))
            {
                logf("too long entry!");
                error = true;
                goto error_exit;
            }

            lseek(tmpfd, entry.tag_offset[index_type], SEEK_CUR);
            if (read(tmpfd, buf, entry.tag_length[index_type]) !=
                entry.tag_length[index_type])
            {
                logf("read fail #4");
                error = true;
                goto error_exit;
            }
            
            if (tagcache_is_unique_tag(index_type))
                error = !tempbuf_insert(buf, i, -1, true);
            else
                error = !tempbuf_insert(buf, i, tcmh.tch.entry_count + i, false);
            
            if (error)
            {
                logf("insert error");
                goto error_exit;
            }
            
            /* Skip to next. */
            lseek(tmpfd, entry.data_length - entry.tag_offset[index_type] -
                    entry.tag_length[index_type], SEEK_CUR);
            do_timed_yield();
        }
        logf("done");

        /* Sort the buffer data and write it to the index file. */
        lseek(fd, sizeof(struct tagcache_header), SEEK_SET);
        i = tempbuf_sort(fd);
        if (i < 0)
            goto error_exit;
        logf("sorted %d tags", i);
        
        /**
         * Now update all indexes in the master lookup file.
         */
        logf("updating indices...");
        lseek(masterfd, sizeof(struct master_header), SEEK_SET);
        for (i = 0; i < tcmh.tch.entry_count; i += idxbuf_pos)
        {
            int j;
            int loc = lseek(masterfd, 0, SEEK_CUR);
            
            idxbuf_pos = MIN(tcmh.tch.entry_count - i, IDX_BUF_DEPTH);
            
            if (ecread(masterfd, idxbuf, idxbuf_pos, index_entry_ec, tc_stat.econ) 
                != (int)sizeof(struct index_entry)*idxbuf_pos)
            {
                logf("read fail #5");
                error = true;
                goto error_exit ;
            }
            lseek(masterfd, loc, SEEK_SET);
            
            for (j = 0; j < idxbuf_pos; j++)
            {
                if (idxbuf[j].flag & FLAG_DELETED)
                {
                    /* We can just ignore deleted entries. */
                    // idxbuf[j].tag_seek[index_type] = 0;
                    continue;
                }
                
                idxbuf[j].tag_seek[index_type] = tempbuf_find_location(
                    idxbuf[j].tag_seek[index_type]/TAGFILE_ENTRY_CHUNK_LENGTH
                    + commit_entry_count);
                
                if (idxbuf[j].tag_seek[index_type] < 0)
                {
                    logf("update error: %d/%d/%d", 
                         idxbuf[j].flag, i+j, tcmh.tch.entry_count);
                    error = true;
                    goto error_exit;
                }
                
                do_timed_yield();
            }
            
            /* Write back the updated index. */
            if (ecwrite(masterfd, idxbuf, idxbuf_pos,
                        index_entry_ec, tc_stat.econ) !=
                (int)sizeof(struct index_entry)*idxbuf_pos)
            {
                logf("write fail");
                error = true;
                goto error_exit;
            }
        }
        logf("done");
    }

    /**
     * Walk through the temporary file containing the new tags.
     */
    // build_normal_index(h, tmpfd, masterfd, idx);
    logf("updating new indices...");
    lseek(masterfd, masterfd_pos, SEEK_SET);
    lseek(tmpfd, sizeof(struct tagcache_header), SEEK_SET);
    lseek(fd, 0, SEEK_END);
    for (i = 0; i < h->entry_count; i += idxbuf_pos)
    {
        int j;
        
        idxbuf_pos = MIN(h->entry_count - i, IDX_BUF_DEPTH);
        if (init)
        {
            memset(idxbuf, 0, sizeof(struct index_entry)*IDX_BUF_DEPTH);
        }
        else
        {
            int loc = lseek(masterfd, 0, SEEK_CUR);
            
            if (ecread(masterfd, idxbuf, idxbuf_pos, index_entry_ec, tc_stat.econ) 
                != (int)sizeof(struct index_entry)*idxbuf_pos)
            {
                logf("read fail #6");
                error = true;
                break ;
            }
            lseek(masterfd, loc, SEEK_SET);
        }

        /* Read entry headers. */
        for (j = 0; j < idxbuf_pos; j++)
        {
            if (!tagcache_is_sorted_tag(index_type))
            {
                struct temp_file_entry entry;
                struct tagfile_entry fe;
                
                if (read(tmpfd, &entry, sizeof(struct temp_file_entry)) !=
                    sizeof(struct temp_file_entry))
                {
                    logf("read fail #7");
                    error = true;
                    break ;
                }
                
                /* Read data. */
                if (entry.tag_length[index_type] >= (int)sizeof(buf))
                {
                    logf("too long entry!");
                    logf("length=%d", entry.tag_length[index_type]);
                    logf("pos=0x%02lx", lseek(tmpfd, 0, SEEK_CUR));
                    error = true;
                    break ;
                }
                
                lseek(tmpfd, entry.tag_offset[index_type], SEEK_CUR);
                if (read(tmpfd, buf, entry.tag_length[index_type]) !=
                    entry.tag_length[index_type])
                {
                    logf("read fail #8");
                    logf("offset=0x%02lx", entry.tag_offset[index_type]);
                    logf("length=0x%02x", entry.tag_length[index_type]);
                    error = true;
                    break ;
                }

                /* Write to index file. */
                idxbuf[j].tag_seek[index_type] = lseek(fd, 0, SEEK_CUR);
                fe.tag_length = entry.tag_length[index_type];
                fe.idx_id = tcmh.tch.entry_count + i + j;
                ecwrite(fd, &fe, 1, tagfile_entry_ec, tc_stat.econ);
                write(fd, buf, fe.tag_length);
                tempbufidx++;

                /* Skip to next. */
                lseek(tmpfd, entry.data_length - entry.tag_offset[index_type] -
                      entry.tag_length[index_type], SEEK_CUR);
            }
            else
            {
                /* Locate the correct entry from the sorted array. */
                idxbuf[j].tag_seek[index_type] = tempbuf_find_location(i + j);
                if (idxbuf[j].tag_seek[index_type] < 0)
                {
                    logf("entry not found (%d)", j);
                    error = true;
                    break ;
                }
            }
        }

        /* Write index. */
        if (ecwrite(masterfd, idxbuf, idxbuf_pos,
                  index_entry_ec, tc_stat.econ) !=
            (int)sizeof(struct index_entry)*idxbuf_pos)
        {
            logf("tagcache: write fail #4");
            error = true;
            break ;
        }
        
        do_timed_yield();
    }
    logf("done");
    
    /* Finally write the header. */
    tch.magic = TAGCACHE_MAGIC;
    tch.entry_count = tempbufidx;
    tch.datasize = lseek(fd, 0, SEEK_END) - sizeof(struct tagcache_header);
    lseek(fd, 0, SEEK_SET);
    ecwrite(fd, &tch, 1, tagcache_header_ec, tc_stat.econ);
    
    if (index_type != tag_filename)
        h->datasize += tch.datasize;
    logf("s:%d/%ld/%ld", index_type, tch.datasize, h->datasize);
    error_exit:
    
    close(fd);
    close(masterfd);

    if (error)
        return -2;
    
    return 1;
}

static bool commit(void)
{
    struct tagcache_header tch;
    struct master_header   tcmh;
    int i, len, rc;
    int tmpfd;
    int masterfd;
#ifdef HAVE_DIRCACHE
    bool dircache_buffer_stolen = false;
#endif
    bool local_allocation = false;
    
    logf("committing tagcache");
    
    while (write_lock)
        sleep(1);

    tmpfd = open(TAGCACHE_FILE_TEMP, O_RDONLY);
    if (tmpfd < 0)
    {
        logf("nothing to commit");
        return true;
    }
    
    
    /* Load the header. */
    len = sizeof(struct tagcache_header);
    rc = read(tmpfd, &tch, len);
    
    if (tch.magic != TAGCACHE_MAGIC || rc != len)
    {
        logf("incorrect header");
        close(tmpfd);
        remove(TAGCACHE_FILE_TEMP);
        return false;
    }

    if (tch.entry_count == 0)
    {
        logf("nothing to commit");
        close(tmpfd);
        remove(TAGCACHE_FILE_TEMP);
        return true;
    }

#ifdef HAVE_EEPROM_SETTINGS
    remove(TAGCACHE_STATEFILE);
#endif
    
    /* At first be sure to unload the ramcache! */
#ifdef HAVE_TC_RAMCACHE
    tc_stat.ramcache = false;
#endif
    
    read_lock++;
    
    /* Try to steal every buffer we can :) */
    if (tempbuf_size == 0)
        local_allocation = true;
    
#ifdef HAVE_DIRCACHE
    if (tempbuf_size == 0)
    {
        /* Try to steal the dircache buffer. */
        tempbuf = dircache_steal_buffer(&tempbuf_size);
        tempbuf_size &= ~0x03;
        
        if (tempbuf_size > 0)
        {
            dircache_buffer_stolen = true;
        }
    }
#endif    
    
#ifdef HAVE_TC_RAMCACHE
    if (tempbuf_size == 0 && tc_stat.ramcache_allocated > 0)
    {
        tempbuf = (char *)(hdr + 1);
        tempbuf_size = tc_stat.ramcache_allocated - sizeof(struct ramcache_header) - 128;
        tempbuf_size &= ~0x03;
    }
#endif
    
    /* And finally fail if there are no buffers available. */
    if (tempbuf_size == 0)
    {
        logf("delaying commit until next boot");
        tc_stat.commit_delayed = true;
        close(tmpfd);
        read_lock--;
        return false;
    }
    
    logf("commit %ld entries...", tch.entry_count);
    
    /* Mark DB dirty so it will stay disabled if commit fails. */
    current_tcmh.dirty = true;
    update_master_header();
    
    /* Now create the index files. */
    tc_stat.commit_step = 0;
    tch.datasize = 0;
    tc_stat.commit_delayed = false;
    
    for (i = 0; i < TAG_COUNT; i++)
    {
        int ret;
        
        if (tagcache_is_numeric_tag(i))
            continue;
        
        tc_stat.commit_step++;
        ret = build_index(i, &tch, tmpfd);
        if (ret <= 0)
        {
            close(tmpfd);
            logf("tagcache failed init");
            if (ret < 0)
                remove_files();
            else
                tc_stat.commit_delayed = true;
            tc_stat.commit_step = 0;
            read_lock--;
            return false;
        }
    }
    
    if (!build_numeric_indices(&tch, tmpfd))
    {
        logf("Failure to commit numeric indices");
        close(tmpfd);
        remove_files();
        tc_stat.commit_step = 0;
        read_lock--;
        return false;
    }
    
    close(tmpfd);
    tc_stat.commit_step = 0;
    
    /* Update the master index headers. */
    if ( (masterfd = open_master_fd(&tcmh, true)) < 0)
    {
        read_lock--;
        return false;
    }
    
    tcmh.tch.entry_count += tch.entry_count;
    tcmh.tch.datasize = sizeof(struct master_header) 
        + sizeof(struct index_entry) * tcmh.tch.entry_count
        + tch.datasize;
    tcmh.dirty = false;
    tcmh.commitid++;

    lseek(masterfd, 0, SEEK_SET);
    ecwrite(masterfd, &tcmh, 1, master_header_ec, tc_stat.econ);
    close(masterfd);
    
    logf("tagcache committed");
    remove(TAGCACHE_FILE_TEMP);
    tc_stat.ready = check_all_headers();
    tc_stat.readyvalid = true;
    
    if (local_allocation)
    {
        tempbuf = NULL;
        tempbuf_size = 0;
    }
    
#ifdef HAVE_DIRCACHE
    /* Rebuild the dircache, if we stole the buffer. */
    if (dircache_buffer_stolen)
        dircache_build(0);
#endif

#ifdef HAVE_TC_RAMCACHE
    /* Reload tagcache. */
    if (tc_stat.ramcache_allocated > 0)
        tagcache_start_scan();
#endif
    
    read_lock--;
    
    return true;
}

static void allocate_tempbuf(void)
{
    /* Yeah, malloc would be really nice now :) */
#ifdef __PCTOOL__
    tempbuf_size = 32*1024*1024;
    tempbuf = malloc(tempbuf_size);
#else
    tempbuf = (char *)(((long)audiobuf & ~0x03) + 0x04);
    tempbuf_size = (long)audiobufend - (long)audiobuf - 4;
    audiobuf += tempbuf_size;
#endif
}

static void free_tempbuf(void)
{
    if (tempbuf_size == 0)
        return ;
    
#ifdef __PCTOOL__
    free(tempbuf);
#else
    audiobuf -= tempbuf_size;
#endif
    tempbuf = NULL;
    tempbuf_size = 0;
}

#ifndef __PCTOOL__

static bool modify_numeric_entry(int masterfd, int idx_id, int tag, long data)
{
    struct index_entry idx;
    
    if (!tc_stat.ready)
        return false;
    
    if (!tagcache_is_numeric_tag(tag))
        return false;
    
    if (!get_index(masterfd, idx_id, &idx, false))
        return false;
    
    idx.tag_seek[tag] = data;
    idx.flag |= FLAG_DIRTYNUM;
    
    return write_index(masterfd, idx_id, &idx);
}

#if 0
bool tagcache_modify_numeric_entry(struct tagcache_search *tcs, 
                                   int tag, long data)
{
    struct master_header myhdr;
    
    if (tcs->masterfd < 0)
    {
        if ( (tcs->masterfd = open_master_fd(&myhdr, true)) < 0)
            return false;
    }
    
    return modify_numeric_entry(tcs->masterfd, tcs->idx_id, tag, data);
}
#endif

#define COMMAND_QUEUE_IS_EMPTY (command_queue_ridx == command_queue_widx)

static bool command_queue_is_full(void)
{
    int next;
    
    next = command_queue_widx + 1;
    if (next >= TAGCACHE_COMMAND_QUEUE_LENGTH)
        next = 0;
    
    return (next == command_queue_ridx);
}

static bool command_queue_sync_callback(void)
{
    
    struct master_header myhdr;
    int masterfd;
        
    mutex_lock(&command_queue_mutex);
	
    if ( (masterfd = open_master_fd(&myhdr, true)) < 0)
        return false;
    
    while (command_queue_ridx != command_queue_widx)
    {
        struct tagcache_command_entry *ce = &command_queue[command_queue_ridx];
        
        switch (ce->command)
        {
            case CMD_UPDATE_MASTER_HEADER:
            {
                close(masterfd);
                update_master_header();
                
                /* Re-open the masterfd. */
                if ( (masterfd = open_master_fd(&myhdr, true)) < 0)
                    return true;
                
                break;
            }
            case CMD_UPDATE_NUMERIC:
            {
                modify_numeric_entry(masterfd, ce->idx_id, ce->tag, ce->data);
                break;
            }
        }
        
        if (++command_queue_ridx >= TAGCACHE_COMMAND_QUEUE_LENGTH)
            command_queue_ridx = 0;
    }
    
    close(masterfd);
    
    tc_stat.queue_length = 0;
    mutex_unlock(&command_queue_mutex);
    return true;
}

static void run_command_queue(bool force)
{
    if (COMMAND_QUEUE_IS_EMPTY)
        return;
    
    if (force || command_queue_is_full())
        command_queue_sync_callback();
    else
        register_storage_idle_func(command_queue_sync_callback);
}

static void queue_command(int cmd, long idx_id, int tag, long data)
{
    while (1)
    {
        int next;
        
        mutex_lock(&command_queue_mutex);
        next = command_queue_widx + 1;
        if (next >= TAGCACHE_COMMAND_QUEUE_LENGTH)
            next = 0;
        
        /* Make sure queue is not full. */
        if (next != command_queue_ridx)
        {
            struct tagcache_command_entry *ce = &command_queue[command_queue_widx];
            
            ce->command = cmd;
            ce->idx_id = idx_id;
            ce->tag = tag;
            ce->data = data;
            
            command_queue_widx = next;
            
            tc_stat.queue_length++;
            
            mutex_unlock(&command_queue_mutex);
            break;
        }
        
        /* Queue is full, try again later... */
        mutex_unlock(&command_queue_mutex);
        sleep(1);
    }
}

long tagcache_increase_serial(void)
{
    long old;
    
    if (!tc_stat.ready)
        return -2;
    
    while (read_lock)
        sleep(1);
    
    old = current_tcmh.serial++;
    queue_command(CMD_UPDATE_MASTER_HEADER, 0, 0, 0);
    
    return old;
}

void tagcache_update_numeric(int idx_id, int tag, long data)
{
    queue_command(CMD_UPDATE_NUMERIC, idx_id, tag, data);
}
#endif /* !__PCTOOL__ */

long tagcache_get_serial(void)
{
    return current_tcmh.serial;
}

long tagcache_get_commitid(void)
{
    return current_tcmh.commitid;
}

static bool write_tag(int fd, const char *tagstr, const char *datastr)
{
    char buf[512];
    int i;
    
    snprintf(buf, sizeof buf, "%s=\"", tagstr);
    for (i = strlen(buf); i < (long)sizeof(buf)-4; i++)
    {
        if (*datastr == '\0')
            break;
        
        if (*datastr == '"' || *datastr == '\\')
            buf[i++] = '\\';
        
        buf[i] = *(datastr++);
    }
    
    strcpy(&buf[i], "\" ");
    
    write(fd, buf, i + 2);
    
    return true;
}

#ifndef __PCTOOL__

static bool read_tag(char *dest, long size, 
                     const char *src, const char *tagstr)
{
    int pos;
    char current_tag[32];
    
    while (*src != '\0')
    {
        /* Skip all whitespace */
        while (*src == ' ')
            src++;
        
        if (*src == '\0')
            break;
        
        pos = 0;
        /* Read in tag name */
        while (*src != '=' && *src != ' ')
        {
            current_tag[pos] = *src;
            src++;
            pos++;
        
            if (*src == '\0' || pos >= (long)sizeof(current_tag))
                return false;
        }
        current_tag[pos] = '\0';
        
        /* Read in tag data */
        
        /* Find the start. */
        while (*src != '"' && *src != '\0')
            src++;

        if (*src == '\0' || *(++src) == '\0')
            return false;
    
        /* Read the data. */
        for (pos = 0; pos < size; pos++)
        {
            if (*src == '\0')
                break;
            
            if (*src == '\\')
            {
                dest[pos] = *(src+1);
                src += 2;
                continue;
            }
            
            dest[pos] = *src;
            
            if (*src == '"')
            {
                src++;
                break;
            }
            
            if (*src == '\0')
                break;
            
            src++;
        }
        dest[pos] = '\0';

        if (!strcasecmp(tagstr, current_tag))
            return true;
    }
    
    return false;
}

static int parse_changelog_line(int line_n, const char *buf, void *parameters)
{
    struct index_entry idx;
    char tag_data[TAG_MAXLEN+32];
    int idx_id;
    long masterfd = (long)parameters;
    const int import_tags[] = { tag_playcount, tag_rating, tag_playtime, tag_lastplayed,
        tag_commitid };
    int i;
    (void)line_n;
    
    if (*buf == '#')
        return 0;
    
    logf("%d/%s", line_n, buf);
    if (!read_tag(tag_data, sizeof tag_data, buf, "filename"))
    {
        logf("filename missing");
        logf("-> %s", buf);
        return 0;
    }
    
    idx_id = find_index(tag_data);
    if (idx_id < 0)
    {
        logf("entry not found");
        return 0;
    }
    
    if (!get_index(masterfd, idx_id, &idx, false))
    {
        logf("failed to retrieve index entry");
        return 0;
    }
    
    /* Stop if tag has already been modified. */
    if (idx.flag & FLAG_DIRTYNUM)
        return 0;
    
    logf("import: %s", tag_data);
    
    idx.flag |= FLAG_DIRTYNUM;
    for (i = 0; i < (long)(sizeof(import_tags)/sizeof(import_tags[0])); i++)
    {
        int data;
        
        if (!read_tag(tag_data, sizeof tag_data, buf,
                      tagcache_tag_to_str(import_tags[i])))
        {
            continue;
        }
        
        data = atoi(tag_data);
        if (data < 0)
            continue;
        
        idx.tag_seek[import_tags[i]] = data;
        
        if (import_tags[i] == tag_lastplayed && data >= current_tcmh.serial)
            current_tcmh.serial = data + 1;
        else if (import_tags[i] == tag_commitid && data >= current_tcmh.commitid)
            current_tcmh.commitid = data + 1;
    }
    
    return write_index(masterfd, idx_id, &idx) ? 0 : -5;
}

bool tagcache_import_changelog(void)
{
    struct master_header myhdr;
    struct tagcache_header tch;
    int clfd;
    long masterfd;
    char buf[2048];
    
    if (!tc_stat.ready)
        return false;
    
    while (read_lock)
        sleep(1);
    
    clfd = open(TAGCACHE_FILE_CHANGELOG, O_RDONLY);
    if (clfd < 0)
    {
        logf("failure to open changelog");
        return false;
    }
    
    if ( (masterfd = open_master_fd(&myhdr, true)) < 0)
    {
        close(clfd);
        return false;
    }
    
    write_lock++;
    
    filenametag_fd = open_tag_fd(&tch, tag_filename, false);
    
    fast_readline(clfd, buf, sizeof buf, (long *)masterfd,
                  parse_changelog_line);
    
    close(clfd);
    close(masterfd);
    
    if (filenametag_fd >= 0)
        close(filenametag_fd);
    
    write_lock--;
    
    update_master_header();
    
    return true;
}

#endif /* !__PCTOOL__ */

bool tagcache_create_changelog(struct tagcache_search *tcs)
{
    struct master_header myhdr;
    struct index_entry idx;
    char buf[TAG_MAXLEN+32];
    char temp[32];
    int clfd;
    int i, j;
    
    if (!tc_stat.ready)
        return false;
    
    if (!tagcache_search(tcs, tag_filename))
        return false;
    
    /* Initialize the changelog */
    clfd = open(TAGCACHE_FILE_CHANGELOG, O_WRONLY | O_CREAT | O_TRUNC);
    if (clfd < 0)
    {
        logf("failure to open changelog");
        return false;
    }
    
    if (tcs->masterfd < 0)
    {
        if ( (tcs->masterfd = open_master_fd(&myhdr, false)) < 0)
            return false;
    }
    else
    {
        lseek(tcs->masterfd, 0, SEEK_SET);
        ecread(tcs->masterfd, &myhdr, 1, master_header_ec, tc_stat.econ);
    }
    
    write(clfd, "## Changelog version 1\n", 23);
    
    for (i = 0; i < myhdr.tch.entry_count; i++)
    {
        if (ecread(tcs->masterfd, &idx, 1, index_entry_ec, tc_stat.econ) 
            != sizeof(struct index_entry))
        {
            logf("read error #9");
            tagcache_search_finish(tcs);
            close(clfd);
            return false;
        }
        
        /* Skip until the entry found has been modified. */
        if (! (idx.flag & FLAG_DIRTYNUM) )
            continue;
        
        /* Skip deleted entries too. */
        if (idx.flag & FLAG_DELETED)
            continue;
        
        /* Now retrieve all tags. */
        for (j = 0; j < TAG_COUNT; j++)
        {
            if (tagcache_is_numeric_tag(j))
            {
                snprintf(temp, sizeof temp, "%d", idx.tag_seek[j]);
                write_tag(clfd, tagcache_tag_to_str(j), temp);
                continue;
            }
            
            tcs->type = j;
            tagcache_retrieve(tcs, i, tcs->type, buf, sizeof buf);
            write_tag(clfd, tagcache_tag_to_str(j), buf);
        }
        
        write(clfd, "\n", 1);
        do_timed_yield();
    }
    
    close(clfd);
    
    tagcache_search_finish(tcs);
    
    return true;
}

static bool delete_entry(long idx_id)
{
    int fd = -1;
    int masterfd = -1;
    int tag, i;
    struct index_entry idx, myidx;
    struct master_header myhdr;
    char buf[TAG_MAXLEN+32];
    int in_use[TAG_COUNT];
    
    logf("delete_entry(): %ld", idx_id);
    
#ifdef HAVE_TC_RAMCACHE
    /* At first mark the entry removed from ram cache. */
    if (tc_stat.ramcache)
        hdr->indices[idx_id].flag |= FLAG_DELETED;
#endif
    
    if ( (masterfd = open_master_fd(&myhdr, true) ) < 0)
        return false;
    
    lseek(masterfd, idx_id * sizeof(struct index_entry), SEEK_CUR);
    if (ecread(masterfd, &myidx, 1, index_entry_ec, tc_stat.econ)
        != sizeof(struct index_entry))
    {
        logf("delete_entry(): read error");
        goto cleanup;
    }
    
    if (myidx.flag & FLAG_DELETED)
    {
        logf("delete_entry(): already deleted!");
        goto cleanup;
    }
    
    myidx.flag |= FLAG_DELETED;
    lseek(masterfd, -sizeof(struct index_entry), SEEK_CUR);
    if (ecwrite(masterfd, &myidx, 1, index_entry_ec, tc_stat.econ)
        != sizeof(struct index_entry))
    {
        logf("delete_entry(): write_error #1");
        goto cleanup;
    }

    /* Now check which tags are no longer in use (if any) */
    for (tag = 0; tag < TAG_COUNT; tag++)
        in_use[tag] = 0;
    
    lseek(masterfd, sizeof(struct master_header), SEEK_SET);
    for (i = 0; i < myhdr.tch.entry_count; i++)
    {
        struct index_entry *idxp;
        
#ifdef HAVE_TC_RAMCACHE
        /* Use RAM DB if available for greater speed */
        if (tc_stat.ramcache)
            idxp = &hdr->indices[i];
        else
#endif
        {
            if (ecread(masterfd, &idx, 1, index_entry_ec, tc_stat.econ)
                != sizeof(struct index_entry))
            {
                logf("delete_entry(): read error #2");
                goto cleanup;
            }
            idxp = &idx;
        }
        
        if (idxp->flag & FLAG_DELETED)
            continue;
        
        for (tag = 0; tag < TAG_COUNT; tag++)
        {
            if (tagcache_is_numeric_tag(tag))
                continue;
            
            if (idxp->tag_seek[tag] == myidx.tag_seek[tag])
                in_use[tag]++;
        }
    }
    
    /* Now delete all tags no longer in use. */
    for (tag = 0; tag < TAG_COUNT; tag++)
    {
        struct tagcache_header tch;
        int oldseek = myidx.tag_seek[tag];
        
        if (tagcache_is_numeric_tag(tag))
            continue;
        
        /** 
         * Replace tag seek with a hash value of the field string data.
         * That way runtime statistics of moved or altered files can be
         * resurrected.
         */
#ifdef HAVE_TC_RAMCACHE
        if (tc_stat.ramcache && tag != tag_filename)
        {
            struct tagfile_entry *tfe;
            int32_t *seek = &hdr->indices[idx_id].tag_seek[tag];
            
            tfe = (struct tagfile_entry *)&hdr->tags[tag][*seek];
            *seek = crc_32(tfe->tag_data, strlen(tfe->tag_data), 0xffffffff);
            myidx.tag_seek[tag] = *seek;
        }
        else
#endif
        {
            struct tagfile_entry tfe;
            
            /* Open the index file, which contains the tag names. */
            if ((fd = open_tag_fd(&tch, tag, true)) < 0)
                goto cleanup;
            
            /* Skip the header block */
            lseek(fd, myidx.tag_seek[tag], SEEK_SET);
            if (ecread(fd, &tfe, 1, tagfile_entry_ec, tc_stat.econ) 
                != sizeof(struct tagfile_entry))
            {
                logf("delete_entry(): read error #3");
                goto cleanup;
            }
            
            if (read(fd, buf, tfe.tag_length) != tfe.tag_length)
            {
                logf("delete_entry(): read error #4");
                goto cleanup;
            }
            
            myidx.tag_seek[tag] = crc_32(buf, strlen(buf), 0xffffffff);
        }
        
        if (in_use[tag])
        {
            logf("in use: %d/%d", tag, in_use[tag]);
            if (fd >= 0)
            {
                close(fd);
                fd = -1;
            }
            continue;
        }
        
#ifdef HAVE_TC_RAMCACHE
        /* Delete from ram. */
        if (tc_stat.ramcache && tag != tag_filename)
        {
            struct tagfile_entry *tagentry = (struct tagfile_entry *)&hdr->tags[tag][oldseek];
            tagentry->tag_data[0] = '\0';
        }
#endif
        
        /* Open the index file, which contains the tag names. */
        if (fd < 0)
        {
            if ((fd = open_tag_fd(&tch, tag, true)) < 0)
                goto cleanup;
        }
        
        /* Skip the header block */
        lseek(fd, oldseek + sizeof(struct tagfile_entry), SEEK_SET);
       
        /* Debug, print 10 first characters of the tag
        read(fd, buf, 10);
        buf[10]='\0';
        logf("TAG:%s", buf);
        lseek(fd, -10, SEEK_CUR);
        */
        
        /* Write first data byte in tag as \0 */
        write(fd, "", 1);
    
        /* Now tag data has been removed */
        close(fd);
        fd = -1;
    }
    
    /* Write index entry back into master index. */
    lseek(masterfd, sizeof(struct master_header) +
          (idx_id * sizeof(struct index_entry)), SEEK_SET);
    if (ecwrite(masterfd, &myidx, 1, index_entry_ec, tc_stat.econ)
        != sizeof(struct index_entry))
    {
        logf("delete_entry(): write_error #2");
        goto cleanup;
    }
    
    close(masterfd);
    
    return true;
    
    cleanup:
    if (fd >= 0)
        close(fd);
    if (masterfd >= 0)
        close(masterfd);
  
    return false;
}

#ifndef __PCTOOL__
/**
 * Returns true if there is an event waiting in the queue
 * that requires the current operation to be aborted.
 */
static bool check_event_queue(void)
{
    struct queue_event ev;
    
    queue_wait_w_tmo(&tagcache_queue, &ev, 0);
    switch (ev.id)
    {
        case Q_STOP_SCAN:
        case SYS_POWEROFF:
        case SYS_USB_CONNECTED:
            /* Put the event back into the queue. */
            queue_post(&tagcache_queue, ev.id, ev.data);
            return true;
    }
    
    return false;
}
#endif

#ifdef HAVE_TC_RAMCACHE
static bool allocate_tagcache(void)
{
    struct master_header tcmh;
    int fd;

    /* Load the header. */
    if ( (fd = open_master_fd(&tcmh, false)) < 0)
    {
        hdr = NULL;
        return false;
    }
    
    close(fd);
    
    /** 
     * Now calculate the required cache size plus 
     * some extra space for alignment fixes. 
     */
    tc_stat.ramcache_allocated = tcmh.tch.datasize + 128 + TAGCACHE_RESERVE +
        sizeof(struct ramcache_header) + TAG_COUNT*sizeof(void *);
    hdr = buffer_alloc(tc_stat.ramcache_allocated + 128);
    memset(hdr, 0, sizeof(struct ramcache_header));
    memcpy(&hdr->h, &tcmh, sizeof(struct master_header));
    hdr->indices = (struct index_entry *)(hdr + 1);
    logf("tagcache: %d bytes allocated.", tc_stat.ramcache_allocated);

    return true;
}

# ifdef HAVE_EEPROM_SETTINGS
static bool tagcache_dumpload(void)
{
    struct statefile_header shdr;
    int fd, rc;
    long offpos;
    int i;
    
    fd = open(TAGCACHE_STATEFILE, O_RDONLY);
    if (fd < 0)
    {
        logf("no tagcache statedump");
        return false;
    }
    
    /* Check the statefile memory placement */
    hdr = buffer_alloc(0);
    rc = read(fd, &shdr, sizeof(struct statefile_header));
    if (rc != sizeof(struct statefile_header)
        /* || (long)hdr != (long)shdr.hdr */)
    {
        logf("incorrect statefile");
        hdr = NULL;
        close(fd);
        return false;
    }
    
    offpos = (long)hdr - (long)shdr.hdr;
    
    /* Lets allocate real memory and load it */
    hdr = buffer_alloc(shdr.tc_stat.ramcache_allocated);
    rc = read(fd, hdr, shdr.tc_stat.ramcache_allocated);
    close(fd);
    
    if (rc != shdr.tc_stat.ramcache_allocated)
    {
        logf("read failure!");
        hdr = NULL;
        return false;
    }
    
    memcpy(&tc_stat, &shdr.tc_stat, sizeof(struct tagcache_stat));
    
    /* Now fix the pointers */
    hdr->indices = (struct index_entry *)((long)hdr->indices + offpos);
    for (i = 0; i < TAG_COUNT; i++)
        hdr->tags[i] += offpos;
    
    return true;
}

static bool tagcache_dumpsave(void)
{
    struct statefile_header shdr;
    int fd;
    
    if (!tc_stat.ramcache)
        return false;
    
    fd = open(TAGCACHE_STATEFILE, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        logf("failed to create a statedump");
        return false;
    }
    
    /* Create the header */
    shdr.hdr = hdr;
    memcpy(&shdr.tc_stat, &tc_stat, sizeof(struct tagcache_stat));
    write(fd, &shdr, sizeof(struct statefile_header));
    
    /* And dump the data too */
    write(fd, hdr, tc_stat.ramcache_allocated);
    close(fd);
    
    return true;
}
# endif

static bool load_tagcache(void)
{
    struct tagcache_header *tch;
    long bytesleft = tc_stat.ramcache_allocated;
    struct index_entry *idx;
    int rc, fd;
    char *p;
    int i, tag;

# ifdef HAVE_DIRCACHE
    while (dircache_is_initializing())
        sleep(1);

    dircache_set_appflag(DIRCACHE_APPFLAG_TAGCACHE);    
# endif
    
    logf("loading tagcache to ram...");
    
    fd = open(TAGCACHE_FILE_MASTER, O_RDONLY);
    if (fd < 0)
    {
        logf("tagcache open failed");
        return false;
    }
    
    if (ecread(fd, &hdr->h, 1, master_header_ec, tc_stat.econ)
        != sizeof(struct master_header)
        || hdr->h.tch.magic != TAGCACHE_MAGIC)
    {
        logf("incorrect header");
        return false;
    }

    idx = hdr->indices;

    /* Load the master index table. */
    for (i = 0; i < hdr->h.tch.entry_count; i++)
    {
        rc = ecread(fd, idx, 1, index_entry_ec, tc_stat.econ);
        if (rc != sizeof(struct index_entry))
        {
            logf("read error #10");
            close(fd);
            return false;
        }
    
        bytesleft -= sizeof(struct index_entry);
        if (bytesleft < 0 || ((long)idx - (long)hdr->indices) >= tc_stat.ramcache_allocated)
        {
            logf("too big tagcache.");
            close(fd);
            return false;
        }

        idx++;
    }

    close(fd);

    /* Load the tags. */
    p = (char *)idx;
    for (tag = 0; tag < TAG_COUNT; tag++)
    {
        struct tagfile_entry *fe;
        char buf[TAG_MAXLEN+32];

        if (tagcache_is_numeric_tag(tag))
            continue ;
        
        //p = ((void *)p+1);
        p = (char *)((long)p & ~0x03) + 0x04;
        hdr->tags[tag] = p;

        /* Check the header. */
        tch = (struct tagcache_header *)p;
        p += sizeof(struct tagcache_header);
        
        if ( (fd = open_tag_fd(tch, tag, false)) < 0)
            return false;
        
        for (hdr->entry_count[tag] = 0;
             hdr->entry_count[tag] < tch->entry_count;
             hdr->entry_count[tag]++)
        {
            long pos;
            
            if (do_timed_yield())
            {
                /* Abort if we got a critical event in queue */
                if (check_event_queue())
                    return false;
            }
            
            fe = (struct tagfile_entry *)p;
            pos = lseek(fd, 0, SEEK_CUR);
            rc = ecread(fd, fe, 1, tagfile_entry_ec, tc_stat.econ);
            if (rc != sizeof(struct tagfile_entry))
            {
                /* End of lookup table. */
                logf("read error #11");
                close(fd);
                return false;
            }

            /* We have a special handling for the filename tags. */
            if (tag == tag_filename)
            {
# ifdef HAVE_DIRCACHE
                const struct dirent *dc;
# endif
                
                // FIXME: This is wrong!
                // idx = &hdr->indices[hdr->entry_count[i]];
                idx = &hdr->indices[fe->idx_id];
                
                if (fe->tag_length >= (long)sizeof(buf)-1)
                {
                    read(fd, buf, 10);
                    buf[10] = '\0';
                    logf("TAG:%s", buf);
                    logf("too long filename");
                    close(fd);
                    return false;
                }
                
                rc = read(fd, buf, fe->tag_length);
                if (rc != fe->tag_length)
                {
                    logf("read error #12");
                    close(fd);
                    return false;
                }
                
                /* Check if the entry has already been removed */
                if (idx->flag & FLAG_DELETED)
                    continue;
                    
                /* This flag must not be used yet. */
                if (idx->flag & FLAG_DIRCACHE)
                {
                    logf("internal error!");
                    close(fd);
                    return false;
                }
                
                if (idx->tag_seek[tag] != pos)
                {
                    logf("corrupt data structures!");
                    close(fd);
                    return false;
                }

# ifdef HAVE_DIRCACHE
                if (dircache_is_enabled())
                {
                    dc = dircache_get_entry_ptr(buf);
                    if (dc == NULL)
                    {
                        logf("Entry no longer valid.");
                        logf("-> %s", buf);
                        delete_entry(fe->idx_id);
                        continue ;
                    }

                    idx->flag |= FLAG_DIRCACHE;
                    FLAG_SET_ATTR(idx->flag, fe->idx_id);
                    idx->tag_seek[tag_filename] = (long)dc;
                }
                else
# endif
                {
                    /* This will be very slow unless dircache is enabled
                       or target is flash based, but do it anyway for
                       consistency. */
                    /* Check if entry has been removed. */
                    if (global_settings.tagcache_autoupdate)
                    {
                        if (!file_exists(buf))
                        {
                            logf("Entry no longer valid.");
                            logf("-> %s", buf);
                            delete_entry(fe->idx_id);
                            continue;
                        }
                    }
                }
                
                continue ;
            }
            
            bytesleft -= sizeof(struct tagfile_entry) + fe->tag_length;
            if (bytesleft < 0)
            {
                logf("too big tagcache #2");
                logf("tl: %d", fe->tag_length);
                logf("bl: %ld", bytesleft);
                close(fd);
                return false;
            }

            p = fe->tag_data;
            rc = read(fd, fe->tag_data, fe->tag_length);
            p += rc;

            if (rc != fe->tag_length)
            {
                logf("read error #13");
                logf("rc=0x%04x", rc); // 0x431
                logf("len=0x%04x", fe->tag_length); // 0x4000
                logf("pos=0x%04lx", lseek(fd, 0, SEEK_CUR)); // 0x433
                logf("tag=0x%02x", tag); // 0x00
                close(fd);
                return false;
            }
        }
        close(fd);
    }
    
    tc_stat.ramcache_used = tc_stat.ramcache_allocated - bytesleft;
    logf("tagcache loaded into ram!");

    return true;
}
#endif /* HAVE_TC_RAMCACHE */

static bool check_deleted_files(void)
{
    int fd;
    char buf[TAG_MAXLEN+32];
    struct tagfile_entry tfe;
    
    logf("reverse scan...");
    snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, tag_filename);
    fd = open(buf, O_RDONLY);
    
    if (fd < 0)
    {
        logf("%s open fail", buf);
        return false;
    }

    lseek(fd, sizeof(struct tagcache_header), SEEK_SET);
    while (ecread(fd, &tfe, 1, tagfile_entry_ec, tc_stat.econ)
           == sizeof(struct tagfile_entry) 
#ifndef __PCTOOL__
           && !check_event_queue()
#endif
           )
    {
        if (tfe.tag_length >= (long)sizeof(buf)-1)
        {
            logf("too long tag");
            close(fd);
            return false;
        }
        
        if (read(fd, buf, tfe.tag_length) != tfe.tag_length)
        {
            logf("read error #14");
            close(fd);
            return false;
        }
        
        /* Check if the file has already deleted from the db. */
        if (*buf == '\0')
            continue;
        
        /* Now check if the file exists. */
        if (!file_exists(buf))
        {
            logf("Entry no longer valid.");
            logf("-> %s / %d", buf, tfe.tag_length);
            delete_entry(tfe.idx_id);
        }
    }
    
    close(fd);
    
    logf("done");
    
    return true;
}

static bool check_dir(const char *dirname, int add_files)
{
    DIR *dir;
    int len;
    int success = false;
    int ignore, unignore;
    char newpath[MAX_PATH];

    dir = opendir(dirname);
    if (!dir)
    {
        logf("tagcache: opendir() failed");
        return false;
    }
    
    /* check for a database.ignore file */
    snprintf(newpath, MAX_PATH, "%s/database.ignore", dirname);
    ignore = file_exists(newpath);
    /* check for a database.unignore file */
    snprintf(newpath, MAX_PATH, "%s/database.unignore", dirname);
    unignore = file_exists(newpath);

    /* don't do anything if both ignore and unignore are there */
    if (ignore != unignore)
        add_files = unignore;

    /* Recursively scan the dir. */
#ifdef __PCTOOL__
    while (1)
#else
    while (!check_event_queue())
#endif
    {
        struct dirent *entry;

        entry = readdir(dir);
    
        if (entry == NULL)
        {
            success = true;
            break ;
        }
        
        if (!strcmp((char *)entry->d_name, ".") ||
            !strcmp((char *)entry->d_name, ".."))
            continue;

        yield();
        
        len = strlen(curpath);
        snprintf(&curpath[len], sizeof(curpath) - len, "/%s",
                 entry->d_name);
        
        processed_dir_count++;
        if (entry->attribute & ATTR_DIRECTORY)
            check_dir(curpath, add_files);
        else if (add_files)
        {
            tc_stat.curentry = curpath;
            
            /* Add a new entry to the temporary db file. */
            add_tagcache(curpath, (entry->wrtdate << 16) | entry->wrttime
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
                         , dir->internal_entry
#endif
                         );
            
            /* Wait until current path for debug screen is read and unset. */
            while (tc_stat.syncscreen && tc_stat.curentry != NULL)
                yield();
            
            tc_stat.curentry = NULL;
        }

        curpath[len] = '\0';
    }
    
    closedir(dir);

    return success;
}

void tagcache_screensync_event(void)
{
    tc_stat.curentry = NULL;
}

void tagcache_screensync_enable(bool state)
{
    tc_stat.syncscreen = state;
}

void tagcache_build(const char *path)
{
    struct tagcache_header header;
    bool ret;

    curpath[0] = '\0';
    data_size = 0;
    total_entry_count = 0;
    processed_dir_count = 0;
    
#ifdef HAVE_DIRCACHE
    while (dircache_is_initializing())
        sleep(1);
#endif
    
    logf("updating tagcache");
    
    cachefd = open(TAGCACHE_FILE_TEMP, O_RDONLY);
    if (cachefd >= 0)
    {
        logf("skipping, cache already waiting for commit");
        close(cachefd);
        return ;
    }
    
    cachefd = open(TAGCACHE_FILE_TEMP, O_RDWR | O_CREAT | O_TRUNC);
    if (cachefd < 0)
    {
        logf("master file open failed: %s", TAGCACHE_FILE_TEMP);
        return ;
    }

    filenametag_fd = open_tag_fd(&header, tag_filename, false);
    
    cpu_boost(true);

    logf("Scanning files...");
    /* Scan for new files. */
    memset(&header, 0, sizeof(struct tagcache_header));
    write(cachefd, &header, sizeof(struct tagcache_header));

    if (strcmp("/", path) != 0)
        strcpy(curpath, path);
    ret = check_dir(path, true);
    
    /* Write the header. */
    header.magic = TAGCACHE_MAGIC;
    header.datasize = data_size;
    header.entry_count = total_entry_count;
    lseek(cachefd, 0, SEEK_SET);
    write(cachefd, &header, sizeof(struct tagcache_header));
    close(cachefd);

    if (filenametag_fd >= 0)
    {
        close(filenametag_fd);
        filenametag_fd = -1;
    }

    if (!ret)
    {
        logf("Aborted.");
        cpu_boost(false);
        return ;
    }

    /* Commit changes to the database. */
#ifdef __PCTOOL__
    allocate_tempbuf();
#endif
    if (commit())
    {
        remove(TAGCACHE_FILE_TEMP);
        logf("tagcache built!");
    }
#ifdef __PCTOOL__
    free_tempbuf();
#endif
    
#ifdef HAVE_TC_RAMCACHE
    if (hdr)
    {
        /* Import runtime statistics if we just initialized the db. */
        if (hdr->h.serial == 0)
            queue_post(&tagcache_queue, Q_IMPORT_CHANGELOG, 0);
    }
#endif
    
    cpu_boost(false);
}

#ifdef HAVE_TC_RAMCACHE
static void load_ramcache(void)
{
    if (!hdr)
        return ;
        
    cpu_boost(true);
    
    /* At first we should load the cache (if exists). */
    tc_stat.ramcache = load_tagcache();

    if (!tc_stat.ramcache)
    {
        /* If loading failed, it must indicate some problem with the db
         * so disable it entirely to prevent further issues. */
        tc_stat.ready = false;
        hdr = NULL;
    }
    
    cpu_boost(false);
}

void tagcache_unload_ramcache(void)
{
    tc_stat.ramcache = false;
    /* Just to make sure there is no statefile present. */
    // remove(TAGCACHE_STATEFILE);
}
#endif

#ifndef __PCTOOL__
static void tagcache_thread(void)
{
    struct queue_event ev;
    bool check_done = false;

    /* If the previous cache build/update was interrupted, commit
     * the changes first in foreground. */
    cpu_boost(true);
    allocate_tempbuf();
    commit();
    free_tempbuf();
    
#ifdef HAVE_TC_RAMCACHE
# ifdef HAVE_EEPROM_SETTINGS
    if (firmware_settings.initialized && firmware_settings.disk_clean)
        check_done = tagcache_dumpload();

    remove(TAGCACHE_STATEFILE);
# endif
    
    /* Allocate space for the tagcache if found on disk. */
    if (global_settings.tagcache_ram && !tc_stat.ramcache)
        allocate_tagcache();
#endif
    
    cpu_boost(false);
    tc_stat.initialized = true;
    
    /* Don't delay bootup with the header check but do it on background. */
    sleep(HZ);
    tc_stat.ready = check_all_headers();
    tc_stat.readyvalid = true;
    
    while (1)
    {
        run_command_queue(false);
        
        queue_wait_w_tmo(&tagcache_queue, &ev, HZ);

        switch (ev.id)
        {
            case Q_IMPORT_CHANGELOG:
                tagcache_import_changelog();
                break;
            
            case Q_REBUILD:
                remove_files();
                remove(TAGCACHE_FILE_TEMP);
                tagcache_build("/");
                break;
            
            case Q_UPDATE:
                tagcache_build("/");
#ifdef HAVE_TC_RAMCACHE
                load_ramcache();
#endif
                check_deleted_files();
                break ;
                
            case Q_START_SCAN:
                check_done = false;
            case SYS_TIMEOUT:
                if (check_done || !tc_stat.ready)
                    break ;
                
#ifdef HAVE_TC_RAMCACHE
                if (!tc_stat.ramcache && global_settings.tagcache_ram)
                {
                    load_ramcache();
                    if (tc_stat.ramcache && global_settings.tagcache_autoupdate)
                        tagcache_build("/");
                }
                else
#endif
                if (global_settings.tagcache_autoupdate)
                {
                    tagcache_build("/");
                    
                    /* This will be very slow unless dircache is enabled
                       or target is flash based, but do it anyway for
                       consistency. */
                    check_deleted_files();
                }
            
                logf("tagcache check done");
    
                check_done = true;
                break ;

            case Q_STOP_SCAN:
                break ;
                
            case SYS_POWEROFF:
                break ;
                
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                logf("USB: TagCache");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&tagcache_queue);
                break ;
#endif
        }
    }
}

bool tagcache_prepare_shutdown(void)
{
    if (tagcache_get_commit_step() > 0)
        return false;
    
    tagcache_stop_scan();
    while (read_lock || write_lock)
        sleep(1);
    
    return true;
}

void tagcache_shutdown(void)
{
    /* Flush the command queue. */
    run_command_queue(true);
    
#ifdef HAVE_EEPROM_SETTINGS
    if (tc_stat.ramcache)
        tagcache_dumpsave();
#endif
}

static int get_progress(void)
{
    int total_count = -1;

#ifdef HAVE_DIRCACHE
    if (dircache_is_enabled())
    {
        total_count = dircache_get_entry_count();
    }
    else
#endif
#ifdef HAVE_TC_RAMCACHE
    {
        if (hdr && tc_stat.ramcache)
            total_count = hdr->h.tch.entry_count;
    }
#endif

    if (total_count < 0)
        return -1;
    
    return processed_dir_count * 100 / total_count;
}

struct tagcache_stat* tagcache_get_stat(void)
{
    tc_stat.progress = get_progress();
    tc_stat.processed_entries = processed_dir_count;
    
    return &tc_stat;
}

void tagcache_start_scan(void)
{
    queue_post(&tagcache_queue, Q_START_SCAN, 0);
}

bool tagcache_update(void)
{
    if (!tc_stat.ready)
        return false;
    
    queue_post(&tagcache_queue, Q_UPDATE, 0);
    return false;
}

bool tagcache_rebuild()
{
    queue_post(&tagcache_queue, Q_REBUILD, 0);
    return false;
}

void tagcache_stop_scan(void)
{
    queue_post(&tagcache_queue, Q_STOP_SCAN, 0);
}

#ifdef HAVE_TC_RAMCACHE
bool tagcache_is_ramcache(void)
{
    return tc_stat.ramcache;
}
#endif

#endif /* !__PCTOOL__ */


void tagcache_init(void)
{
    memset(&tc_stat, 0, sizeof(struct tagcache_stat));
    memset(&current_tcmh, 0, sizeof(struct master_header));
    filenametag_fd = -1;
    write_lock = read_lock = 0;
    
#ifndef __PCTOOL__
    mutex_init(&command_queue_mutex);
    queue_init(&tagcache_queue, true);
    create_thread(tagcache_thread, tagcache_stack,
                  sizeof(tagcache_stack), 0, tagcache_thread_name 
                  IF_PRIO(, PRIORITY_BACKGROUND)
		          IF_COP(, CPU));
#else
    tc_stat.initialized = true;
    allocate_tempbuf();
    commit();
    free_tempbuf();
    tc_stat.ready = check_all_headers();
#endif
}

#ifdef __PCTOOL__
void tagcache_reverse_scan(void)
{
    logf("Checking for deleted files");
    check_deleted_files();
}
#endif

bool tagcache_is_initialized(void)
{
    return tc_stat.initialized;
}
bool tagcache_is_usable(void)
{
    return tc_stat.initialized && tc_stat.ready;
}
int tagcache_get_commit_step(void)
{
    return tc_stat.commit_step;
}
int tagcache_get_max_commit_step(void)
{
    return (int)(sizeof(sorted_tags)/sizeof(sorted_tags[0]))+1;
}

