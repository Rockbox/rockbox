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

#include <stdio.h>
#include "config.h"
#include "thread.h"
#include "kernel.h"
#include "system.h"
#include "logf.h"
#include "string.h"
#include "usb.h"
#include "metadata.h"
#include "id3.h"
#include "tagcache.h"
#include "buffer.h"
#include "crc32.h"
#include "misc.h"
#include "settings.h"
#include "dircache.h"
#ifndef __PCTOOL__
#include "atoi.h"
#include "splash.h"
#include "lang.h"
#include "eeprom_settings.h"
#endif

#ifdef __PCTOOL__
#include <ctype.h>
#define yield() do { } while(0)
#define sim_sleep(timeout) do { } while(0)
#endif


#ifndef __PCTOOL__
/* Tag Cache thread. */
static struct event_queue tagcache_queue;
static long tagcache_stack[(DEFAULT_STACK_SIZE + 0x4000)/sizeof(long)];
static const char tagcache_thread_name[] = "tagcache";
#endif

/* Previous path when scanning directory tree recursively. */
static char curpath[TAG_MAXLEN+32];
static long curpath_size = sizeof(curpath);

/* Used when removing duplicates. */
static char *tempbuf;     /* Allocated when needed. */
static long tempbufidx;   /* Current location in buffer. */
static long tempbuf_size; /* Buffer size (TEMPBUF_SIZE). */
static long tempbuf_left; /* Buffer space left. */
static long tempbuf_pos;

/* Tags we want to get sorted (loaded to the tempbuf). */
static const int sorted_tags[] = { tag_artist, tag_album, tag_genre, tag_composer, tag_title };

/* Uniqued tags (we can use these tags with filters and conditional clauses). */
static const int unique_tags[] = { tag_artist, tag_album, tag_genre, tag_composer };

/* Numeric tags (we can use these tags with conditional clauses). */
static const int numeric_tags[] = { tag_year, tag_tracknumber, tag_length, tag_bitrate,
    tag_playcount, tag_playtime, tag_lastplayed, tag_virt_autoscore };

static const char *tags_str[] = { "artist", "album", "genre", "title", 
    "filename", "composer", "year", "tracknumber", "bitrate", "length",
    "playcount", "playtime", "lastplayed" };

/* Status information of the tagcache. */
static struct tagcache_stat tc_stat;

/* Queue commands. */
enum tagcache_queue {
    Q_STOP_SCAN = 0,
    Q_START_SCAN,
    Q_IMPORT_CHANGELOG,
    Q_UPDATE,
    Q_REBUILD,
};


/* Tag database structures. */

/* Variable-length tag entry in tag files. */
struct tagfile_entry {
    short tag_length;  /* Length of the data in bytes including '\0' */
    short idx_id;      /* Corresponding entry location in index file of not unique tags */
    char tag_data[0];  /* Begin of the tag data */
};

/* Fixed-size tag entry in master db index. */
struct index_entry {
    long tag_seek[TAG_COUNT]; /* Location of tag data or numeric tag data */
    long flag;                /* Status flags */
};

/* Header is the same in every file. */
struct tagcache_header {
    long magic;       /* Header version number */
    long datasize;    /* Data size in bytes */
    long entry_count; /* Number of entries in this file */
};

struct master_header {
    struct tagcache_header tch;
    long serial; /* Increasing counting number */
};

static long current_serial;

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
struct tempbuf_searchidx **lookup;

/* Used when building the temporary file. */
static int cachefd = -1, filenametag_fd;
static int total_entry_count = 0;
static int data_size = 0;
static int processed_dir_count;

/* Thread safe locking */
static volatile int write_lock;
static volatile int read_lock;

int tagcache_str_to_tag(const char *str)
{
    int i;
    
    for (i = 0; i < (long)(sizeof(tags_str)/sizeof(tags_str[0])); i++)
    {
        if (!strcasecmp(tags_str[i], str))
            return i;
    }
    
    return -1;
}

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
    rc = read(fd, hdr, sizeof(struct tagcache_header));
    if (hdr->magic != TAGCACHE_MAGIC || rc != sizeof(struct tagcache_header))
    {
        logf("header error");
        tc_stat.ready = false;
        close(fd);
        return -2;
    }

    return fd;
}

#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
static long find_entry_ram(const char *filename,
                           const struct dircache_entry *dc)
{
    static long last_pos = 0;
    int counter = 0;
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
        
        if (++counter == 100)
        {
            yield();
            counter = 0;
        }
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

        if (read(fd, &tfe, sizeof(struct tagfile_entry)) !=
            sizeof(struct tagfile_entry))
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
    if (tc_stat.ramcache && dircache_is_enabled())
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
    
    lseek(masterfd, idxid * sizeof(struct index_entry) 
          + sizeof(struct master_header), SEEK_SET);
    if (read(masterfd, idx, sizeof(struct index_entry)) !=
        sizeof(struct index_entry))
    {
        logf("read error #3");
        return false;
    }
    
    if (idx->flag & FLAG_DELETED)
        return false;
    
    return true;
}

static bool write_index(int masterfd, int idxid, struct index_entry *idx)
{
    /* We need to exclude all memory only flags & tags when writing to disk. */
    if (idx->flag & FLAG_DIRCACHE)
    {
        logf("memory only flags!");
        return false;
    }
    
#ifdef HAVE_TC_RAMCACHE
    if (tc_stat.ramcache)
    {
        memcpy(&hdr->indices[idxid], idx, sizeof(struct index_entry));
    }
#endif
    
    lseek(masterfd, idxid * sizeof(struct index_entry) 
          + sizeof(struct master_header), SEEK_SET);
    if (write(masterfd, idx, sizeof(struct index_entry)) !=
        sizeof(struct index_entry))
    {
        logf("write error #3");
        logf("idxid: %d", idxid);
        return false;
    }
    
    return true;
}

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
        if (tag == tag_filename && idx->flag & FLAG_DIRCACHE)
        {
            dircache_copy_path((struct dircache_entry *)seek,
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
    if (read(tcs->idxfd[tag], &tfe, sizeof(struct tagfile_entry)) !=
        sizeof(struct tagfile_entry))
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

bool check_clauses(struct tagcache_search *tcs,
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
                read(fd, &tfe, sizeof(struct tagfile_entry));
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
    
    while (read(tcs->masterfd, &entry, sizeof(struct index_entry)) ==
           sizeof(struct index_entry))
    {
        /* Check if entry has been deleted. */
        if (entry.flag & FLAG_DELETED)
            continue;
        
        if (tcs->seek_list_count == SEEK_LIST_SIZE)
            break ;

        /* Go through all filters.. */
        for (i = 0; i < tcs->filter_count; i++)
        {
            if (entry.tag_seek[tcs->filter_tag[i]] != tcs->filter_seek[i])
                break ;
        }
        
        tcs->seek_pos++;
        
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
    remove(TAGCACHE_FILE_MASTER);
    for (i = 0; i < TAG_COUNT; i++)
    {
        if (tagcache_is_numeric_tag(i))
            continue;
        
        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, i);
        remove(buf);
    }
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
    
    /* Check the header. */
    rc = read(fd, hdr, sizeof(struct master_header));
    if (hdr->tch.magic != TAGCACHE_MAGIC || rc != sizeof(struct master_header))
    {
        logf("header error");
        tc_stat.ready = false;
        close(fd);
        return -2;
    }
    
    return fd;
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

#define TAG_FILENAME_RAM(tcs) ((tcs->type == tag_filename) \
                                   ? (flag & FLAG_DIRCACHE) : 1)

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
            dircache_copy_path((struct dircache_entry *)tcs->position,
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
        if (read(tcs->idxfd[tcs->type], &entry, sizeof(struct tagfile_entry)) !=
            sizeof(struct tagfile_entry))
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
    return entry->tag_seek[tag];
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
    
    id3->title = get_tag(entry, tag_title)->tag_data;
    id3->artist = get_tag(entry, tag_artist)->tag_data;
    id3->album = get_tag(entry, tag_album)->tag_data;
    id3->genre_string = get_tag(entry, tag_genre)->tag_data;
    id3->composer = get_tag(entry, tag_composer)->tag_data;
    id3->year = get_tag_numeric(entry, tag_year);
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
    
    if (*tag == NULL || *tag[0] == '\0')
    {
        *tag = "<Untagged>";
        return 11; /* Tag length */
    }
    
    length = strlen(*tag);
    if (length > TAG_MAXLEN)
    {
        logf("over length tag: %s", *tag);
        length = TAG_MAXLEN;
        *tag[length] = '\0';
    }
    
    return length + 1;
}

#define ADD_TAG(entry,tag,data) \
    /* Adding tag */ \
    entry.tag_offset[tag] = offset; \
    entry.tag_length[tag] = check_if_empty(data); \
    offset += entry.tag_length[tag]
    
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
static void add_tagcache(char *path, const struct dircache_entry *dc)
#else
static void add_tagcache(char *path)
#endif
{
    struct track_info track;
    struct temp_file_entry entry;
    bool ret;
    int fd;
    char tracknumfix[3];
    char *genrestr;
    int offset = 0;
    int path_length = strlen(path);

    if (cachefd < 0)
        return ;

    /* Check for overlength file path. */
    if (path_length > TAG_MAXLEN)
    {
        /* Path can't be shortened. */
        logf("Too long path: %s");
        return ;
    }
    
    /* Check if the file is supported. */
    if (probe_file_format(path) == AFMT_UNKNOWN)
        return ;
    
    /* Check if the file is already cached. */
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    if (tc_stat.ramcache && dircache_is_enabled())
    {
        if (find_entry_ram(path, dc) >= 0)
            return ;
    }
    else
#endif
    {
        if (filenametag_fd >= 0)
        {
            if (find_entry_disk(path) >= 0)
                return ;
        }
    }
    
    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        logf("open fail: %s", path);
        return ;
    }

    memset(&track, 0, sizeof(struct track_info));
    memset(&entry, 0, sizeof(struct temp_file_entry));
    memset(&tracknumfix, 0, sizeof(tracknumfix));
    ret = get_metadata(&track, fd, path, false);
    close(fd);

    if (!ret)
        return ;

    logf("-> %s", path);
    
    /* Generate track number if missing. */
    if (track.id3.tracknum <= 0)
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
            track.id3.tracknum = atoi(tracknumfix);
            /* Set a flag to indicate track number has been generated. */
            entry.flag |= FLAG_TRKNUMGEN;
        }
        else
        {
            /* Unable to generate track number. */
            track.id3.tracknum = -1;
        }
    }
    
    genrestr = id3_get_genre(&track.id3);
    
    /* Numeric tags */
    entry.tag_offset[tag_year] = track.id3.year;
    entry.tag_offset[tag_tracknumber] = track.id3.tracknum;
    entry.tag_offset[tag_length] = track.id3.length;
    entry.tag_offset[tag_bitrate] = track.id3.bitrate;
    
    /* String tags. */
    ADD_TAG(entry, tag_filename, &path);
    ADD_TAG(entry, tag_title, &track.id3.title);
    ADD_TAG(entry, tag_artist, &track.id3.artist);
    ADD_TAG(entry, tag_album, &track.id3.album);
    ADD_TAG(entry, tag_genre, &genrestr);
    ADD_TAG(entry, tag_composer, &track.id3.composer);
    entry.data_length = offset;
    
    /* Write the header */
    write(cachefd, &entry, sizeof(struct temp_file_entry));
    
    /* And tags also... Correct order is critical */
    write_item(path);
    write_item(track.id3.title);
    write_item(track.id3.artist);
    write_item(track.id3.album);
    write_item(genrestr);
    write_item(track.id3.composer);
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
    struct tempbuf_searchidx *e1 = (struct tempbuf_searchidx *)p1;
    struct tempbuf_searchidx *e2 = (struct tempbuf_searchidx *)p2;
    
    /*
    if (!strncasecmp("the ", e1, 4))
        e1 = &e1[4];
    if (!strncasecmp("the ", e2, 4))
        e2 = &e2[4];
    */
    
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
        
        if (write(fd, &fe, sizeof(struct tagfile_entry)) !=
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
    struct master_header   tcmh;
    struct index_entry idx;
    int masterfd;
    int masterfd_pos;
    struct temp_file_entry *entrybuf = (struct temp_file_entry *)tempbuf;
    int max_entries;
    int entries_processed = 0;
    int i, j;
    
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
        int count = MIN(h->entry_count, max_entries);
         
        /* Read in as many entries as possible. */
        for (i = 0; i < count; i++)
        {
            /* Read in numeric data. */
            if (read(tmpfd, &entrybuf[i], sizeof(struct temp_file_entry)) !=
                sizeof(struct temp_file_entry))
            {
                logf("read fail #1");
                close(masterfd);
                return false;
            }

            /* Skip string data. */
            lseek(tmpfd, entrybuf[i].data_length, SEEK_CUR);
        }
        
        /* Commit the data to the index. */
        for (i = 0; i < count; i++)
        {
            int loc = lseek(masterfd, 0, SEEK_CUR);
            
            if (read(masterfd, &idx, sizeof(struct index_entry)) !=
                sizeof(struct index_entry))
            {
                logf("read fail #2");
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
            
            /* Write back the updated index. */
            lseek(masterfd, loc, SEEK_SET);
            if (write(masterfd, &idx, sizeof(struct index_entry)) !=
                sizeof(struct index_entry))
            {
                logf("write fail");
                close(masterfd);
                return false;
            }
        }
        
        entries_processed += count;
        logf("%d/%d entries processed", entries_processed, h->entry_count);
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

    /* Open the index file, which contains the tag names. */
    fd = open_tag_fd(&tch, index_type, true);
    if (fd >= 0)
    {
        logf("tch.datasize=%d", tch.datasize);
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
    
    logf("lookup_buffer_depth=%d", lookup_buffer_depth);
    logf("commit_entry_count=%d", commit_entry_count);
    
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
                
                if (read(fd, &entry, sizeof(struct tagfile_entry))
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
                yield();
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
        
        if (write(fd, &tch, sizeof(struct tagcache_header)) !=
            sizeof(struct tagcache_header))
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
        logf("Creating new index");
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
        write(masterfd, &tcmh, sizeof(struct master_header));
        init = true;
        masterfd_pos = lseek(masterfd, 0, SEEK_CUR);
        current_serial = 0;
    }
    else
    {
        /**
         * Master file already exists so we need to process the current
         * file first.
         */
        init = false;

        if (read(masterfd, &tcmh, sizeof(struct master_header)) !=
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
                logf("read fail #1");
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
                logf("read fail #3");
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
            yield();
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
            
            if (read(masterfd, idxbuf, sizeof(struct index_entry)*idxbuf_pos) !=
                (int)sizeof(struct index_entry)*idxbuf_pos)
            {
                logf("read fail #2");
                error = true;
                goto error_exit ;
            }
            lseek(masterfd, loc, SEEK_SET);
            
            for (j = 0; j < idxbuf_pos; j++)
            {
                if (idxbuf[j].flag & FLAG_DELETED)
                {
                    /* We can just ignore deleted entries. */
                    idxbuf[j].tag_seek[index_type] = 0;
                    continue;
                }
                
                idxbuf[j].tag_seek[index_type] = tempbuf_find_location(
                    idxbuf[j].tag_seek[index_type]/TAGFILE_ENTRY_CHUNK_LENGTH
                    + commit_entry_count);
                
                if (idxbuf[j].tag_seek[index_type] < 0)
                {
                    logf("update error: %d/%d", i+j, tcmh.tch.entry_count);
                    error = true;
                    goto error_exit;
                }
                
                yield();
            }
            
            /* Write back the updated index. */
            if (write(masterfd, idxbuf, sizeof(struct index_entry)*idxbuf_pos) !=
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
            
            if (read(masterfd, idxbuf, sizeof(struct index_entry)*idxbuf_pos) !=
                (int)sizeof(struct index_entry)*idxbuf_pos)
            {
                logf("read fail #2");
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
                    logf("read fail #1");
                    error = true;
                    break ;
                }
                
                /* Read data. */
                if (entry.tag_length[index_type] >= (int)sizeof(buf))
                {
                    logf("too long entry!");
                    logf("length=%d", entry.tag_length[index_type]);
                    logf("pos=0x%02x", lseek(tmpfd, 0, SEEK_CUR));
                    error = true;
                    break ;
                }
                
                lseek(tmpfd, entry.tag_offset[index_type], SEEK_CUR);
                if (read(tmpfd, buf, entry.tag_length[index_type]) !=
                    entry.tag_length[index_type])
                {
                    logf("read fail #3");
                    logf("offset=0x%02x", entry.tag_offset[index_type]);
                    logf("length=0x%02x", entry.tag_length[index_type]);
                    error = true;
                    break ;
                }

                /* Write to index file. */
                idxbuf[j].tag_seek[index_type] = lseek(fd, 0, SEEK_CUR);
                fe.tag_length = entry.tag_length[index_type];
                fe.idx_id = tcmh.tch.entry_count + i + j;
                write(fd, &fe, sizeof(struct tagfile_entry));
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
                    logf("entry not found (%d)");
                    error = true;
                    break ;
                }
            }
        }

        /* Write index. */
        if (write(masterfd, idxbuf, sizeof(struct index_entry)*idxbuf_pos) !=
            (int)sizeof(struct index_entry)*idxbuf_pos)
        {
            logf("tagcache: write fail #4");
            error = true;
            break ;
        }
        
        yield();
    }
    logf("done");
    
    /* Finally write the header. */
    tch.magic = TAGCACHE_MAGIC;
    tch.entry_count = tempbufidx;
    tch.datasize = lseek(fd, 0, SEEK_END) - sizeof(struct tagcache_header);
    lseek(fd, 0, SEEK_SET);
    write(fd, &tch, sizeof(struct tagcache_header));
    
    if (index_type != tag_filename)
        h->datasize += tch.datasize;
    logf("s:%d/%d/%d", index_type, tch.datasize, h->datasize);
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
    
    logf("commit %d entries...", tch.entry_count);
    
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
    
    build_numeric_indices(&tch, tmpfd);
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

    lseek(masterfd, 0, SEEK_SET);
    write(masterfd, &tcmh, sizeof(struct master_header));
    close(masterfd);
    
    logf("tagcache committed");
    remove(TAGCACHE_FILE_TEMP);
    tc_stat.ready = true;
    
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

static bool update_current_serial(long serial)
{
    struct master_header myhdr;
    int fd;
    
    if ( (fd = open_master_fd(&myhdr, true)) < 0)
        return false;
    
    myhdr.serial = serial;
    current_serial = serial;
    
#ifdef HAVE_TC_RAMCACHE
    if (hdr)
        hdr->h.serial = serial;
#endif
    
    /* Write it back */
    lseek(fd, 0, SEEK_SET);
    write(fd, &myhdr, sizeof(struct master_header));
    close(fd);
    
    return true;
}

long tagcache_increase_serial(void)
{
    if (!tc_stat.ready)
        return -2;
    
    while (read_lock)
        sleep(1);
    
    if (!update_current_serial(current_serial + 1))
        return -1;
    
    return current_serial;
}

long tagcache_get_serial(void)
{
    return current_serial;
}

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

static bool write_tag(int fd, const char *tagstr, const char *datastr)
{
    char buf[256];
    int i;
    
    snprintf(buf, sizeof buf, "%s=\"", tagstr);
    for (i = strlen(buf); i < (long)sizeof(buf)-2; i++)
    {
        if (*datastr == '\0')
            break;
        
        if (*datastr == '"')
        {
            buf[i] = '\\';
            datastr++;
            continue;
        }
        
        buf[i] = *(datastr++);
    }
    
    strcpy(&buf[i], "\" ");
    
    write(fd, buf, i + 2);
    
    return true;
}

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
            
            if (*src == '\\' && *(src+1) == '"')
            {
                dest[pos] = '"';
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
    const int import_tags[] = { tag_playcount, tag_playtime, tag_lastplayed };
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
        
        if (import_tags[i] == tag_lastplayed && data > current_serial)
            current_serial = data;
    }
    
    return write_index(masterfd, idx_id, &idx) ? 0 : -5;
}

#ifndef __PCTOOL__
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
    
    update_current_serial(current_serial);
    
    return true;
}
#endif

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
        read(tcs->masterfd, &myhdr, sizeof(struct master_header));
    }
    
    write(clfd, "## Changelog version 1\n", 23);
    
    for (i = 0; i < myhdr.tch.entry_count; i++)
    {
        if (read(tcs->masterfd, &idx, sizeof(struct index_entry)) 
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
        yield();
    }
    
    close(clfd);
    
    tagcache_search_finish(tcs);
    
    return true;
}

static bool delete_entry(long idx_id)
{
    int fd;
    int tag, i;
    struct index_entry idx, myidx;
    struct master_header myhdr;
    char buf[TAG_MAXLEN+32];
    int in_use[TAG_COUNT];
    
    logf("delete_entry(): %d", idx_id);
    
#ifdef HAVE_TC_RAMCACHE
    /* At first mark the entry removed from ram cache. */
    if (hdr)
        hdr->indices[idx_id].flag |= FLAG_DELETED;
#endif
    
    if ( (fd = open_master_fd(&myhdr, true) ) < 0)
        return false;
    
    lseek(fd, idx_id * sizeof(struct index_entry), SEEK_CUR);
    if (read(fd, &myidx, sizeof(struct index_entry)) 
        != sizeof(struct index_entry))
    {
        logf("delete_entry(): read error");
        close(fd);
        return false;
    }
    
    myidx.flag |= FLAG_DELETED;
    lseek(fd, -sizeof(struct index_entry), SEEK_CUR);
    if (write(fd, &myidx, sizeof(struct index_entry))
        != sizeof(struct index_entry))
    {
        logf("delete_entry(): write_error");
        close(fd);
        return false;
    }
    
    /* Now check which tags are no longer in use (if any) */
    for (tag = 0; tag < TAG_COUNT; tag++)
        in_use[tag] = 0;
    
    lseek(fd, sizeof(struct master_header), SEEK_SET);
    for (i = 0; i < myhdr.tch.entry_count; i++)
    {
        if (read(fd, &idx, sizeof(struct index_entry)) 
            != sizeof(struct index_entry))
        {
            logf("delete_entry(): read error #2");
            close(fd);
            return false;
        }
        
        if (idx.flag & FLAG_DELETED)
            continue;
        
        for (tag = 0; tag < TAG_COUNT; tag++)
        {
            if (tagcache_is_numeric_tag(tag))
                continue;
            
            if (idx.tag_seek[tag] == myidx.tag_seek[tag])
                in_use[tag]++;
        }
    }
    
    close(fd);
    
    /* Now delete all tags no longer in use. */
    for (tag = 0; tag < TAG_COUNT; tag++)
    {
        if (tagcache_is_numeric_tag(tag))
            continue;
        
        if (in_use[tag])
        {
            logf("in use: %d/%d", tag, in_use[tag]);
            continue;
        }
        
        /* Open the index file, which contains the tag names. */
        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, tag);
        fd = open(buf, O_RDWR);
        
        if (fd < 0)
        {
            logf("open failed");
            return false;
        }
        
        /* Skip the header block */
        lseek(fd, myidx.tag_seek[tag] + sizeof(struct tagfile_entry), SEEK_SET);
       
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
    }
    
    return true;
}

#ifndef __PCTOOL__
/**
 * Returns true if there is an event waiting in the queue
 * that requires the current operation to be aborted.
 */
static bool check_event_queue(void)
{
    struct event ev;
    
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
    int yield_count = 0;

# ifdef HAVE_DIRCACHE
    while (dircache_is_initializing())
        sleep(1);
# endif
    
    logf("loading tagcache to ram...");
    
    fd = open(TAGCACHE_FILE_MASTER, O_RDONLY);
    if (fd < 0)
    {
        logf("tagcache open failed");
        return false;
    }
    
    if (read(fd, &hdr->h, sizeof(struct master_header))
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
        rc = read(fd, idx, sizeof(struct index_entry));
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
            
            if (yield_count++ == 100)
            {
                yield();
                /* Abort if we got a critical event in queue */
                if (check_event_queue())
                    return false;
                yield_count = 0;
            }
            
            fe = (struct tagfile_entry *)p;
            pos = lseek(fd, 0, SEEK_CUR);
            rc = read(fd, fe, sizeof(struct tagfile_entry));
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
                const struct dircache_entry *dc;
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
                    
# if 0 /* Maybe we could enable this for flash players. Too slow otherwise. */
                    /* Check if entry has been removed. */
                    if (global_settings.tagcache_autoupdate)
                    {
                        int testfd;
                        
                        testfd = open(buf, O_RDONLY);
                        if (testfd < 0)
                        {
                            logf("Entry no longer valid.");
                            logf("-> %s", buf);
                            delete_entry(fe->idx_id);
                            continue;
                        }
                        close(testfd);
                    }
# endif
                }
                
                continue ;
            }
            
            bytesleft -= sizeof(struct tagfile_entry) + fe->tag_length;
            if (bytesleft < 0)
            {
                logf("too big tagcache #2");
                logf("tl: %d", fe->tag_length);
                logf("bl: %d", bytesleft);
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
                logf("pos=0x%04x", lseek(fd, 0, SEEK_CUR)); // 0x433
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
    int fd, testfd;
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
    while (read(fd, &tfe, sizeof(struct tagfile_entry))
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
        
        /* Now check if the file exists. */
        testfd = open(buf, O_RDONLY);
        if (testfd < 0)
        {
            logf("Entry no longer valid.");
            logf("-> %s", buf);
            delete_entry(tfe.idx_id);
        }
        close(testfd);
    }
    
    close(fd);
    
    logf("done");
    
    return true;
}

static bool check_dir(const char *dirname)
{
    DIRCACHED *dir;
    int len;
    int success = false;

    dir = opendir_cached(dirname);
    if (!dir)
    {
        logf("tagcache: opendir_cached() failed");
        return false;
    }
    
    /* Recursively scan the dir. */
#ifdef __PCTOOL__
    while (1)
#else
    while (!check_event_queue())
#endif
    {
        struct dircache_entry *entry;

        entry = readdir_cached(dir);
    
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
        snprintf(&curpath[len], curpath_size - len, "/%s",
                 entry->d_name);
        
        processed_dir_count++;
        if (entry->attribute & ATTR_DIRECTORY)
            check_dir(curpath);
        else
#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
            add_tagcache(curpath, dir->internal_entry);
#else
            add_tagcache(curpath);
#endif

        curpath[len] = '\0';
    }
    
    closedir_cached(dir);

    return success;
}

void build_tagcache(const char *path)
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
    
    cpu_boost_id(true, CPUBOOSTID_TAGCACHE);

    logf("Scanning files...");
    /* Scan for new files. */
    memset(&header, 0, sizeof(struct tagcache_header));
    write(cachefd, &header, sizeof(struct tagcache_header));

    if (strcmp("/", path) != 0)
        strcpy(curpath, path);
    ret = check_dir(path);
    
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
        cpu_boost_id(false, CPUBOOSTID_TAGCACHE);
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
    
    cpu_boost_id(false, CPUBOOSTID_TAGCACHE);
}

#ifdef HAVE_TC_RAMCACHE
static void load_ramcache(void)
{
    if (!hdr)
        return ;
        
    cpu_boost_id(true, CPUBOOSTID_TAGCACHE);
    
    /* At first we should load the cache (if exists). */
    tc_stat.ramcache = load_tagcache();

    if (!tc_stat.ramcache)
    {
        /* If loading failed, it must indicate some problem with the db
         * so disable it entirely to prevent further issues. */
        tc_stat.ready = false;
        hdr = NULL;
    }
    
    cpu_boost_id(false, CPUBOOSTID_TAGCACHE);
}

void tagcache_unload_ramcache(void)
{
    tc_stat.ramcache = false;
    /* Just to make sure there is no statefile present. */
    // remove(TAGCACHE_STATEFILE);
}
#endif

static bool check_all_headers(void)
{
    struct master_header myhdr;
    struct tagcache_header tch;
    int tag;
    int fd;
    
    if ( (fd = open_master_fd(&myhdr, false)) < 0)
        return false;
    
    close(fd);
    current_serial = myhdr.serial;
    
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

#ifndef __PCTOOL__
static void tagcache_thread(void)
{
    struct event ev;
    bool check_done = false;

    /* If the previous cache build/update was interrupted, commit
     * the changes first in foreground. */
    cpu_boost_id(true, CPUBOOSTID_TAGCACHE);
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
    
    cpu_boost_id(false, CPUBOOSTID_TAGCACHE);
    tc_stat.initialized = true;
    
    /* Don't delay bootup with the header check but do it on background. */
    sleep(HZ);
    tc_stat.ready = check_all_headers();
    
    while (1)
    {
        queue_wait_w_tmo(&tagcache_queue, &ev, HZ);

        switch (ev.id)
        {
            case Q_IMPORT_CHANGELOG:
                tagcache_import_changelog();
                break;
            
            case Q_REBUILD:
                remove_files();
                build_tagcache("/");
                break;
            
            case Q_UPDATE:
                build_tagcache("/");
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
                        build_tagcache("/");
                }
                else
#endif
                if (global_settings.tagcache_autoupdate)
                {
                    build_tagcache("/");
                    /* Don't do auto removal without dircache (very slow). */
#ifdef HAVE_DIRCACHE
                    if (dircache_is_enabled())
                        check_deleted_files();
#endif
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
    gui_syncsplash(HZ*2, true, str(LANG_TAGCACHE_FORCE_UPDATE_SPLASH));
    
    return false;
}

bool tagcache_rebuild(void)
{
    queue_post(&tagcache_queue, Q_REBUILD, 0);
    gui_syncsplash(HZ*2, true, str(LANG_TAGCACHE_FORCE_UPDATE_SPLASH));
    
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
    filenametag_fd = -1;
    current_serial = 0;
    write_lock = read_lock = 0;
    
#ifndef __PCTOOL__
    queue_init(&tagcache_queue, true);
    create_thread(tagcache_thread, tagcache_stack,
                  sizeof(tagcache_stack), tagcache_thread_name 
                  IF_PRIO(, PRIORITY_BACKGROUND));
#else
    tc_stat.initialized = true;
    allocate_tempbuf();
    commit();
    free_tempbuf();
    tc_stat.ready = check_all_headers();
#endif
}

bool tagcache_is_initialized(void)
{
    return tc_stat.initialized;
}
    
int tagcache_get_commit_step(void)
{
    return tc_stat.commit_step;
}

