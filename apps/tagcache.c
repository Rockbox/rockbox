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

#include <stdio.h>
#include "thread.h"
#include "kernel.h"
#include "system.h"
#include "logf.h"
#include "string.h"
#include "usb.h"
#include "dircache.h"
#include "metadata.h"
#include "id3.h"
#include "settings.h"
#include "splash.h"
#include "lang.h"
#include "tagcache.h"
#include "buffer.h"

/* Tag Cache thread. */
static struct event_queue tagcache_queue;
static long tagcache_stack[(DEFAULT_STACK_SIZE + 0x4000)/sizeof(long)];
static const char tagcache_thread_name[] = "tagcache";

/* Previous path when scanning directory tree recursively. */
static char curpath[MAX_PATH*2];
static long curpath_size = sizeof(curpath);

/* Used when removing duplicates. */
static char *tempbuf;     /* Allocated when needed. */
static long tempbufidx;   /* Current location in buffer. */
static long tempbuf_size; /* Buffer size (TEMPBUF_SIZE). */
static long tempbuf_left; /* Buffer space left. */
static long tempbuf_pos;

/* Tags we want to be unique (loaded to the tempbuf). */
static const int unique_tags[] = { tag_artist, tag_album, tag_genre, tag_title };

/* Queue commands. */
#define Q_STOP_SCAN     0
#define Q_START_SCAN    1
#define Q_FORCE_UPDATE  2

/* Tag database files. */
#define TAGCACHE_FILE_TEMP    ROCKBOX_DIR "/tagcache_tmp.tcd"
#define TAGCACHE_FILE_MASTER  ROCKBOX_DIR "/tagcache_idx.tcd"
#define TAGCACHE_FILE_INDEX   ROCKBOX_DIR "/tagcache_%d.tcd"

/* Tag database structures. */
#define TAGCACHE_MAGIC  0x01020316

/* Variable-length tag entry in tag files. */
struct tagfile_entry {
    short tag_length;
    char tag_data[0];
};

/* Fixed-size tag entry in master db index. */
struct index_entry {
    long tag_seek[TAG_COUNT];
};

/* Header is the same in every file. */
struct tagcache_header {
    long magic;
    long datasize;
    long entry_count;
};

#ifdef HAVE_TC_RAMCACHE
/* Header is created when loading database to ram. */
struct ramcache_header {
    struct tagcache_header h;
    struct index_entry *indices;
    char *tags[TAG_COUNT];
    int entry_count[TAG_COUNT];
};

static struct ramcache_header *hdr;
static bool ramcache = false;
static long tagcache_size = 0;
#endif

/** 
 * Full tag entries stored in a temporary file waiting 
 * for commit to the cache. */
struct temp_file_entry {
    long tag_offset[TAG_COUNT];
    short tag_length[TAG_COUNT];

    long data_length;
};

struct tempbuf_id {
    int id;
    struct tempbuf_id *next;
};

struct tempbuf_searchidx {
    struct tempbuf_id *id;
    char *str;
    int seek;
};


/* Used when building the temporary file. */
static int cachefd = -1, filenametag_fd;
static int total_entry_count = 0;
static int data_size = 0;
static int processed_dir_count;

#ifdef HAVE_TC_RAMCACHE
static struct index_entry *find_entry_ram(const char *filename,
                                          const struct dircache_entry *dc)
{
    static long last_pos = 0;
    int counter = 0;
    int i;
    
    /* Check if we tagcache is loaded into ram. */
    if (!ramcache)
        return NULL;

    if (dc == NULL)
        dc = dircache_get_entry_ptr(filename);
    
    if (dc == NULL)
    {
        logf("tagcache: file not found.");
        return NULL;
    }

    try_again:
            
    if (last_pos > 0)
        i = last_pos;
    else
        i = 0;
    
    for (; i < hdr->h.entry_count - last_pos; i++)
    {
        if (hdr->indices[i].tag_seek[tag_filename] == (long)dc)
        {
            last_pos = MAX(0, i - 3);
            return &hdr->indices[i];
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

    return NULL;
}
#endif

static struct index_entry *find_entry_disk(const char *filename, bool retrieve)
{
    static struct index_entry idx;
    static long last_pos = -1;
    long pos_history[POS_HISTORY_COUNT];
    long pos_history_idx = 0;
    struct tagcache_header tch;
    bool found = false;
    struct tagfile_entry tfe;
    int masterfd, fd = filenametag_fd;
    char buf[MAX_PATH];
    int i;
    int pos = -1;

    if (fd < 0)
    {
        last_pos = -1;
        return NULL;
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
            logf("too long tag");
            close(fd);
            last_pos = -1;
            return NULL;
        }
        
        if (read(fd, buf, tfe.tag_length) != tfe.tag_length)
        {
            logf("read error #2");
            close(fd);
            last_pos = -1;
            return NULL;
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
        //close(fd);
        return NULL;
    }
    
    if (!retrieve)
    {
        /* Just return a valid pointer without a valid entry. */
        return &idx;
    }
    
    /* Found. Now read the index_entry (if requested). */
    masterfd = open(TAGCACHE_FILE_MASTER, O_RDONLY);
    if (masterfd < 0)
    {
        logf("open fail");
        return NULL;
    }
    
    if (read(fd, &tch, sizeof(struct tagcache_header)) !=
        sizeof(struct tagcache_header) || tch.magic != TAGCACHE_MAGIC)
    {
        logf("header error");
        return NULL;
    }
    
    for (i = 0; i < tch.entry_count; i++)
    {
        if (read(masterfd, &idx, sizeof(struct index_entry)) !=
            sizeof(struct index_entry))
        {
            logf("read error #3");
            close(fd);
            return NULL;
        }
        
        if (idx.tag_seek[tag_filename] == pos)
            break ;
    }
    close(masterfd);
    
    /* Not found? */
    if (i == tch.entry_count)
    {
        logf("not found!");
        return NULL;
    }
    
    return &idx;
}

static bool build_lookup_list(struct tagcache_search *tcs)
{
    struct tagcache_header header;
    struct index_entry entry;
    int masterfd;
    int i;
    
    tcs->seek_list_count = 0;
    
#ifdef HAVE_TC_RAMCACHE
    if (tcs->ramsearch)
    {
        int j;

        for (i = tcs->seek_pos; i < hdr->h.entry_count - tcs->seek_pos; i++)
        {
            if (tcs->seek_list_count == SEEK_LIST_SIZE)
                break ;

            for (j = 0; j < tcs->filter_count; j++)
            {
                if (hdr->indices[i].tag_seek[tcs->filter_tag[j]] !=
                    tcs->filter_seek[j])
                    break ;
            }
            
            if (j < tcs->filter_count)
                continue ;
            
            /* Add to the seek list if not already there. */
            for (j = 0; j < tcs->seek_list_count; j++)
            {
                if (tcs->seek_list[j] == hdr->indices[i].tag_seek[tcs->type])
                    break ;
            }

            /* Lets add it. */
            if (j == tcs->seek_list_count)
            {
                tcs->seek_list[tcs->seek_list_count] =
                        hdr->indices[i].tag_seek[tcs->type];
                tcs->seek_list_count++;
            }
        }
        
        tcs->seek_pos = i;

        return tcs->seek_list_count > 0;
    }
#endif
    
    masterfd = open(TAGCACHE_FILE_MASTER, O_RDONLY);
    if (masterfd < 0)
    {
        logf("cannot open master index");
        return false;
    }
        
    /* Load the header. */
    if (read(masterfd, &header, sizeof(struct tagcache_header)) !=
        sizeof(struct tagcache_header) || header.magic != TAGCACHE_MAGIC)
    {
        logf("read error");
        close(masterfd);
        return false;
    }

    lseek(masterfd, tcs->seek_pos * sizeof(struct index_entry) +
            sizeof(struct tagcache_header), SEEK_SET);
    
    while (read(masterfd, &entry, sizeof(struct index_entry)) ==
           sizeof(struct index_entry))
    {
        if (tcs->seek_list_count == SEEK_LIST_SIZE)
            break ;

        for (i = 0; i < tcs->filter_count; i++)
        {
            if (entry.tag_seek[tcs->filter_tag[i]] != tcs->filter_seek[i])
                break ;
        }
        
        tcs->seek_pos++;
        
        if (i < tcs->filter_count)
            continue ;
        
        /* Add to the seek list if not already there. */
        for (i = 0; i < tcs->seek_list_count; i++)
        {
            if (tcs->seek_list[i] == entry.tag_seek[tcs->type])
                break ;
        }

        /* Lets add it. */
        if (i == tcs->seek_list_count)
        {
            tcs->seek_list[tcs->seek_list_count] =
                    entry.tag_seek[tcs->type];
            tcs->seek_list_count++;
        }
        
    }
    close(masterfd);

    return tcs->seek_list_count > 0;
}

bool tagcache_search(struct tagcache_search *tcs, int tag)
{
    struct tagcache_header h;
    char buf[MAX_PATH];

    if (tcs->valid)
        tagcache_search_finish(tcs);
    
    tcs->position = sizeof(struct tagcache_header);
    tcs->fd = -1;
    tcs->type = tag;
    tcs->seek_pos = 0;
    tcs->seek_list_count = 0;
    tcs->filter_count = 0;
    tcs->valid = true;

#ifndef HAVE_TC_RAMCACHE
    tcs->ramsearch = false;
#else
    tcs->ramsearch = ramcache;
    if (tcs->ramsearch)
    {
        tcs->entry_count = hdr->entry_count[tcs->type];
    }
    else
#endif
    {
        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, tcs->type);
        tcs->fd = open(buf, O_RDONLY);
        if (tcs->fd < 0)
        {
            logf("failed to open index");
            return false;
        }

        /* Check the header. */
        if (read(tcs->fd, &h, sizeof(struct tagcache_header)) !=
            sizeof(struct tagcache_header) || h.magic != TAGCACHE_MAGIC)
        {
            logf("incorrect header");
            return false;
        }
    }

    return true;
}

bool tagcache_search_add_filter(struct tagcache_search *tcs,
                                int tag, int seek)
{
    if (tcs->filter_count == TAGCACHE_MAX_FILTERS)
        return false;

    tcs->filter_tag[tcs->filter_count] = tag;
    tcs->filter_seek[tcs->filter_count] = seek;
    tcs->filter_count++;

    return true;
}

bool tagcache_get_next(struct tagcache_search *tcs)
{
    static char buf[MAX_PATH];
    struct tagfile_entry entry;

    if (!tcs->valid)
        return false;
        
    if (tcs->fd < 0 
#ifdef HAVE_TC_RAMCACHE
        && !tcs->ramsearch
#endif
        )
        return false;
    
    /* Relative fetch. */
    if (tcs->filter_count > 0)
    {
        /* Check for end of list. */
        if (tcs->seek_list_count == 0)
        {
            /* Try to fetch more. */
            if (!build_lookup_list(tcs))
                return false;
        }
        
        tcs->seek_list_count--;
        
        /* Seek stream to the correct position and continue to direct fetch. */
        if (!tcs->ramsearch)
            lseek(tcs->fd, tcs->seek_list[tcs->seek_list_count], SEEK_SET);
        else
            tcs->position = tcs->seek_list[tcs->seek_list_count];
    }
    
    /* Direct fetch. */
#ifdef HAVE_TC_RAMCACHE
    if (tcs->ramsearch)
    {
        struct tagfile_entry *ep;
        
        if (tcs->entry_count == 0)
        {
            tcs->valid = false;
            return false;
        }
        tcs->entry_count--;
        tcs->result_seek = tcs->position;
        
        if (tcs->type == tag_filename)
        {
            dircache_copy_path((struct dircache_entry *)tcs->position,
                                buf, sizeof buf);
            tcs->result = buf;
            tcs->result_len = strlen(buf) + 1;
            
            return true;
        }
        
        ep = (struct tagfile_entry *)&hdr->tags[tcs->type][tcs->position];
        tcs->position += sizeof(struct tagfile_entry) + ep->tag_length;
        tcs->result = ep->tag_data;
        tcs->result_len = ep->tag_length;

        return true;
    }
    else
#endif
    {
        tcs->result_seek = lseek(tcs->fd, 0, SEEK_CUR);
        if (read(tcs->fd, &entry, sizeof(struct tagfile_entry)) !=
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
        logf("too long tag");
        return false;
    }
    
    if (read(tcs->fd, buf, entry.tag_length) != entry.tag_length)
    {
        tcs->valid = false;
        logf("read error");
        return false;
    }
    
    tcs->result = buf;
    tcs->result_len = entry.tag_length;
    
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

    entry = find_entry_ram(
    
}
#endif

void tagcache_search_finish(struct tagcache_search *tcs)
{
    if (tcs->fd >= 0)
    {
        close(tcs->fd);
        tcs->fd = -1;
        tcs->ramsearch = false;
        tcs->valid = false;
    }
}

#ifdef HAVE_TC_RAMCACHE
struct tagfile_entry *get_tag(struct index_entry *entry, int tag)
{
    return (struct tagfile_entry *)&hdr->tags[tag][entry->tag_seek[tag]];
}

bool tagcache_fill_tags(struct mp3entry *id3, const char *filename)
{
    struct index_entry *entry;
    
    /* Find the corresponding entry in tagcache. */
    entry = find_entry_ram(filename, NULL);
    if (entry == NULL || !ramcache)
        return false;
    
    id3->title = get_tag(entry, tag_title)->tag_data;
    id3->artist = get_tag(entry, tag_artist)->tag_data;
    id3->album = get_tag(entry, tag_album)->tag_data;
    id3->genre_string = get_tag(entry, tag_genre)->tag_data;

    return true;
}
#endif

static inline void write_item(const char *item)
{
    int len = strlen(item) + 1;
    
    data_size += len;
    write(cachefd, item, len);
}

inline void check_if_empty(char **tag)
{
    if (tag == NULL || *tag == NULL || *tag[0] == '\0')
        *tag = "Unknown";
}

#define CRC_BUF_LEN 8

#ifdef HAVE_TC_RAMCACHE
static void add_tagcache(const char *path, const struct dircache_entry *dc)
#else
static void add_tagcache(const char *path)
#endif
{
    struct track_info track;
    struct temp_file_entry entry;
    bool ret;
    int fd;
    //uint32_t crcbuf[CRC_BUF_LEN];

    if (cachefd < 0)
        return ;

    /* Check if the file is supported. */
    if (probe_file_format(path) == AFMT_UNKNOWN)
        return ;
    
    /* Check if the file is already cached. */
#ifdef HAVE_TC_RAMCACHE
    if (ramcache)
    {
        if (find_entry_ram(path, dc))
            return ;
    }
    else
#endif
    {
        if (find_entry_disk(path, false))
            return ;
    }
    
    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        logf("open fail: %s", path);
        return ;
    }

    memset(&track, 0, sizeof(struct track_info));
    ret = get_metadata(&track, fd, path, false);
    close(fd);

    if (!ret)
    {
        track.id3.title = (char *)path;
        track.id3.artist = "Unknown";
        track.id3.album = "Unknown";
        track.id3.genre_string = "Unknown";
    }
    else
        check_if_empty(&track.id3.title);
    check_if_empty(&track.id3.artist);
    check_if_empty(&track.id3.album);
    check_if_empty(&track.id3.genre_string);
    
    entry.tag_length[tag_filename] = strlen(path) + 1;
    entry.tag_length[tag_title] = strlen(track.id3.title) + 1;
    entry.tag_length[tag_artist] = strlen(track.id3.artist) + 1;
    entry.tag_length[tag_album] = strlen(track.id3.album) + 1;
    entry.tag_length[tag_genre] = strlen(track.id3.genre_string) + 1;
    
    entry.tag_offset[tag_filename] = 0;
    entry.tag_offset[tag_title] = entry.tag_offset[tag_filename] + entry.tag_length[tag_filename];
    entry.tag_offset[tag_artist] = entry.tag_offset[tag_title] + entry.tag_length[tag_title];
    entry.tag_offset[tag_album] = entry.tag_offset[tag_artist] + entry.tag_length[tag_artist];
    entry.tag_offset[tag_genre] = entry.tag_offset[tag_album] + entry.tag_length[tag_album];
    entry.data_length = entry.tag_offset[tag_genre] + entry.tag_length[tag_genre];
    
    write(cachefd, &entry, sizeof(struct temp_file_entry));
    write_item(path);
    write_item(track.id3.title);
    write_item(track.id3.artist);
    write_item(track.id3.album);
    write_item(track.id3.genre_string);
    total_entry_count++;    
}

static void remove_files(void)
{
    int i;
    char buf[MAX_PATH];
    
    remove(TAGCACHE_FILE_MASTER);
    for (i = 0; i < TAG_COUNT; i++)
    {
        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, i);
        remove(buf);
    }
}

static bool tempbuf_insert(char *str, int id)
{
    struct tempbuf_searchidx *index = (struct tempbuf_searchidx *)tempbuf;
    int len = strlen(str)+1;
    
    /* Insert it to the buffer. */
    tempbuf_left -= len + sizeof(struct tempbuf_id);
    if (tempbuf_left - 4 < 0 || tempbufidx >= TAGFILE_MAX_ENTRIES-1)
        return false;
    
    index[tempbufidx].id = (struct tempbuf_id *)&tempbuf[tempbuf_pos];
#ifdef ROCKBOX_STRICT_ALIGN
    /* Make sure the entry is long aligned. */
    if ((long)index[tempbufidx].id & 0x03)
    {
        int fix = 4 - ((long)index[tempbufidx].id & 0x03);
        tempbuf_left -= fix;
        tempbuf_pos += fix;
        index[tempbufidx].id = (struct tempbuf_id *)((
                (long)index[tempbufidx].id & ~0x03) + 0x04);
    }
#endif
    index[tempbufidx].id->id = id;
    index[tempbufidx].id->next = NULL;
    tempbuf_pos += sizeof(struct tempbuf_id);
    
    index[tempbufidx].seek = -1;
    index[tempbufidx].str = &tempbuf[tempbuf_pos];
    memcpy(index[tempbufidx].str, str, len);
    tempbuf_pos += len;
    tempbufidx++;
    
    return true;
}

static bool tempbuf_unique_insert(char *str, int id)
{
    struct tempbuf_searchidx *index = (struct tempbuf_searchidx *)tempbuf;
    struct tempbuf_id *idp;
    int i;
    
    /* Check if string already exists. */
    for (i = 0; i < tempbufidx; i++)
    {
        if (!strcasecmp(str, index[i].str))
        {
            tempbuf_left -= sizeof(struct tempbuf_id);
            if (tempbuf_left - 4 < 0)
                return false;
            
            idp = index[i].id;
            while (idp->next != NULL)
                idp = idp->next;
            
            idp->next = (struct tempbuf_id *)&tempbuf[tempbuf_pos];
#ifdef ROCKBOX_STRICT_ALIGN
            /* Make sure the entry is long aligned. */
            if ((long)idp->next & 0x03)
            {
                int fix = 4 - ((long)idp->next & 0x03);
                tempbuf_left -= fix;
                tempbuf_pos += fix;
                idp->next = (struct tempbuf_id *)((
                        (long)idp->next & ~0x03) + 0x04);
            }
#endif
            idp = idp->next;
            idp->id = id;
            idp->next = NULL;
            tempbuf_pos += sizeof(struct tempbuf_id);
            
            return true;
        }
    }
    
    return tempbuf_insert(str, id);
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
    
    return strncasecmp(e1->str, e2->str, MAX_PATH);
}

static int tempbuf_sort(int fd)
{
    struct tempbuf_searchidx *index = (struct tempbuf_searchidx *)tempbuf;
    struct tagfile_entry fe;
    int i;
    
    qsort(index, tempbufidx, sizeof(struct tempbuf_searchidx), compare);

    for (i = 0; i < tempbufidx; i++)
    {
        index[i].seek = lseek(fd, 0, SEEK_CUR);
        fe.tag_length = strlen(index[i].str) + 1;
        if (write(fd, &fe, sizeof(struct tagfile_entry)) !=
            sizeof(struct tagfile_entry))
        {
            logf("tempbuf_sort: write error #1");
            return -1;
        }
        
        if (write(fd, index[i].str, fe.tag_length) !=
            fe.tag_length)
        {
            logf("tempbuf_sort: write error #2");
            return -2;
        }
    }

    return i;
}
    

static struct tempbuf_searchidx* tempbuf_locate(int id)
{
    struct tempbuf_searchidx *index = (struct tempbuf_searchidx *)tempbuf;
    struct tempbuf_id *idp;
    int i;
    
    /* Check if string already exists. */
    for (i = 0; i < tempbufidx; i++)
    {
        idp = index[i].id;
        while (idp != NULL)
        {
            if (idp->id == id)
                return &index[i];
            idp = idp->next;
        }
    }
    
    return NULL;
}


static int tempbuf_find_location(int id)
{
    struct tempbuf_searchidx *entry;

    entry = tempbuf_locate(id);
    if (entry == NULL)
        return -1;

    return entry->seek;
}

static bool is_unique_tag(int type)
{
    int i;

    for (i = 0; i < (int)(sizeof(unique_tags)/sizeof(unique_tags[0])); i++)
    {
        if (type == unique_tags[i])
            return true;
    }

    return false;
}

static bool build_index(int index_type, struct tagcache_header *h, int tmpfd)
{
    int i;
    struct tagcache_header tch;
    struct index_entry idx;
    char buf[MAX_PATH];
    int fd = -1, masterfd;
    bool error = false;
    int init;
    int masterfd_pos;
    
    logf("Building index: %d", index_type);
    
    tempbufidx = 0;
    tempbuf_pos = TAGFILE_MAX_ENTRIES * sizeof(struct tempbuf_searchidx);
    tempbuf_left = tempbuf_size - tempbuf_pos;
    if (tempbuf_left < 0)
    {
        logf("Buffer way too small!");
        return false;
    }

    /* Open the index file, which contains the tag names. */
    snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, index_type);
    fd = open(buf, O_RDWR);

    if (fd >= 0)
    {
        /* Read the header. */
        if (read(fd, &tch, sizeof(struct tagcache_header)) !=
            sizeof(struct tagcache_header) || tch.magic != TAGCACHE_MAGIC)
        {
            logf("header error");
            close(fd);
            return false;
        }

        /**
         * If tag file contains unique tags (sorted index), we will load
         * it entirely into memory so we can resort it later for use with
         * chunked browsing.
         */
        if (is_unique_tag(index_type))
        {
            for (i = 0; i < tch.entry_count; i++)
            {
                struct tagfile_entry entry;
                int loc = lseek(fd, 0, SEEK_CUR);
                
                if (read(fd, &entry, sizeof(struct tagfile_entry))
                    != sizeof(struct tagfile_entry))
                {
                    logf("read error");
                    close(fd);
                    return false;
                }
                
                if (entry.tag_length >= (int)sizeof(buf))
                {
                    logf("too long tag");
                    close(fd);
                    return false;
                }
                
                if (read(fd, buf, entry.tag_length) != entry.tag_length)
                {
                    logf("read error #2");
                    close(fd);
                    return false;
                }

                /**
                 * Save the tag and tag id in the memory buffer. Tag id
                 * is saved so we can later reindex the master lookup
                 * table when the index gets resorted.
                 */
                tempbuf_insert(buf, loc + TAGFILE_MAX_ENTRIES);
            }
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
        fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd < 0)
        {
            logf("%s open fail", buf);
            return false;
        }
        
        tch.magic = TAGCACHE_MAGIC;
        tch.entry_count = 0;
        tch.datasize = 0;
        
        if (write(fd, &tch, sizeof(struct tagcache_header)) !=
            sizeof(struct tagcache_header))
        {
            logf("header write failed");
            close(fd);
            return false;
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
            logf("Failure to create index file");
            close(fd);
            return false;
        }

        /* Write the header (write real values later). */
        tch = *h;
        tch.entry_count = 0;
        tch.datasize = 0;
        write(masterfd, &tch, sizeof(struct tagcache_header));
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

        if (read(masterfd, &tch, sizeof(struct tagcache_header)) !=
            sizeof(struct tagcache_header) || tch.magic != TAGCACHE_MAGIC)
        {
            logf("header error");
            close(fd);
            close(masterfd);
            return false;
        }

        /**
         * If we reach end of the master file, we need to expand it to
         * hold new tags. If the current index is not sorted, we can
         * simply append new data to end of the file.
         * However, if the index is sorted, we need to update all tag
         * pointers in the master file for the current index.
         */
        masterfd_pos = lseek(masterfd, tch.entry_count * sizeof(struct index_entry),
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
    if (is_unique_tag(index_type))
    {
        lseek(tmpfd, sizeof(struct tagcache_header), SEEK_SET);
        /* h is the header of the temporary file containing new tags. */
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

            if (!tempbuf_unique_insert(buf, i))
            {
                logf("insert error");
                error = true;
                goto error_exit;
            }
            
            /* Skip to next. */
            lseek(tmpfd, entry.data_length - entry.tag_offset[index_type] -
                    entry.tag_length[index_type], SEEK_CUR);
        }

        /* Sort the buffer data and write it to the index file. */
        lseek(fd, sizeof(struct tagcache_header), SEEK_SET);
        i = tempbuf_sort(fd);
        if (i < 0)
            goto error_exit;
        logf("sorted %d tags", i);
        
        /**
         * Now update all indexes in the master lookup file.
         */
        lseek(masterfd, sizeof(struct tagcache_header), SEEK_SET);
        for (i = 0; i < tch.entry_count; i++)
        {
            int loc = lseek(masterfd, 0, SEEK_CUR);

            if (read(masterfd, &idx, sizeof(struct index_entry)) !=
                sizeof(struct index_entry))
            {
                logf("read fail #2");
                error = true;
                goto error_exit ;
            }
            idx.tag_seek[index_type] = tempbuf_find_location(
                    idx.tag_seek[index_type]+TAGFILE_MAX_ENTRIES);
            if (idx.tag_seek[index_type] < 0)
            {
                logf("update error: %d/%d", i, tch.entry_count);
                error = true;
                goto error_exit;
            }

            /* Write back the updated index. */
            lseek(masterfd, loc, SEEK_SET);
            if (write(masterfd, &idx, sizeof(struct index_entry)) !=
                sizeof(struct index_entry))
            {
                logf("write fail");
                error = true;
                goto error_exit;
            }
        }
    }

    /**
     * Walk through the temporary file containing the new tags.
     */
    // build_normal_index(h, tmpfd, masterfd, idx);
    lseek(masterfd, masterfd_pos, SEEK_SET);
    lseek(tmpfd, sizeof(struct tagcache_header), SEEK_SET);
    lseek(fd, 0, SEEK_END);
    for (i = 0; i < h->entry_count; i++)
    {
        if (init)
        {
            memset(&idx, 0, sizeof(struct index_entry));
        }
        else
        {
            if (read(masterfd, &idx, sizeof(struct index_entry)) !=
                sizeof(struct index_entry))
            {
                logf("read fail #2");
                error = true;
                break ;
            }
            lseek(masterfd, -sizeof(struct index_entry), SEEK_CUR);
        }

        /* Read entry headers. */
        if (!is_unique_tag(index_type))
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
            idx.tag_seek[index_type] = lseek(fd, 0, SEEK_CUR);
            fe.tag_length = entry.tag_length[index_type];
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
            idx.tag_seek[index_type] = tempbuf_find_location(i);
            if (idx.tag_seek[index_type] < 0)
            {
                logf("entry not found (%d)");
                error = true;
                break ;
            }
        }


        /* Write index. */
        if (write(masterfd, &idx, sizeof(struct index_entry)) !=
            sizeof(struct index_entry))
        {
            logf("tagcache: write fail #4");
            error = true;
            break ;
        }

    }

    /* Finally write the uniqued tag index file. */
    if (is_unique_tag(index_type))
    {
        tch.magic = TAGCACHE_MAGIC;
        tch.entry_count = tempbufidx;
        tch.datasize = lseek(fd, 0, SEEK_END) - sizeof(struct tagcache_header);
        lseek(fd, 0, SEEK_SET);
        write(fd, &tch, sizeof(struct tagcache_header));
    }
    else
    {
        tch.magic = TAGCACHE_MAGIC;
        tch.entry_count = tempbufidx;
        tch.datasize = lseek(fd, 0, SEEK_CUR) - sizeof(struct tagcache_header);
        lseek(fd, 0, SEEK_SET);
        write(fd, &tch, sizeof(struct tagcache_header));
    }
    
    error_exit:
    
    close(fd);
    close(masterfd);

    return !error;
}

static bool commit(void)
{
    struct tagcache_header header, header_old;
    int i, len, rc;
    int tmpfd;
    int masterfd;
    
    logf("committing tagcache");
    
    tmpfd = open(TAGCACHE_FILE_TEMP, O_RDONLY);
    if (tmpfd < 0)
    {
        logf("nothing to commit");
        return true;
    }
    
    /* Load the header. */
    len = sizeof(struct tagcache_header);
    rc = read(tmpfd, &header, len);
    
    if (header.magic != TAGCACHE_MAGIC || rc != len)
    {
        logf("incorrect header");
        close(tmpfd);
        remove_files();
        remove(TAGCACHE_FILE_TEMP);
        return false;
    }

    if (header.entry_count == 0)
    {
        logf("nothing to commit");
        close(tmpfd);
        remove(TAGCACHE_FILE_TEMP);
        return true;
    }

    if (tempbuf_size == 0)
    {
        logf("delaying commit until next boot");
        close(tmpfd);
        return false;
    }
    
    logf("commit %d entries...", header.entry_count);
    
    /* Now create the index files. */
    for (i = 0; i < TAG_COUNT; i++)
    {
        if (!build_index(i, &header, tmpfd))
        {
            logf("tagcache failed init");
            remove_files();
            return false;
        }
    }
    
    close(tmpfd);
    
    /* Update the master index headers. */
    masterfd = open(TAGCACHE_FILE_MASTER, O_RDWR);
    if (masterfd < 0)
    {
        logf("failed to open master index");
        return false;
    }

    if (read(masterfd, &header_old, sizeof(struct tagcache_header))
        != sizeof(struct tagcache_header) ||
        header_old.magic != TAGCACHE_MAGIC)
    {
        logf("incorrect header");
        close(masterfd);
        remove_files();
        return false;
    }

    header.entry_count += header_old.entry_count;
    header.datasize += header_old.datasize;

    lseek(masterfd, 0, SEEK_SET);
    write(masterfd, &header, sizeof(struct tagcache_header));
    close(masterfd);
    
    logf("tagcache committed");
    remove(TAGCACHE_FILE_TEMP);
    
    return true;
}

static void allocate_tempbuf(void)
{
    /* Yeah, malloc would be really nice now :) */
    tempbuf = (char *)(((long)audiobuf & ~0x03) + 0x04);
    tempbuf_size = (long)audiobufend - (long)audiobuf - 4;
    audiobuf += tempbuf_size;
}

static void free_tempbuf(void)
{
    if (tempbuf_size == 0)
        return ;
    
    audiobuf -= tempbuf_size;
    tempbuf = NULL;
    tempbuf_size = 0;
}

#ifdef HAVE_TC_RAMCACHE
static bool allocate_tagcache(void)
{
    int rc, len;
    int fd;

    hdr = NULL;
    
    fd = open(TAGCACHE_FILE_MASTER, O_RDONLY);
    if (fd < 0)
    {
        logf("no tagcache file found.");
        return false;
    }

    /* Load the header. */
    hdr = (struct ramcache_header *)(((long)audiobuf & ~0x03) + 0x04);
    memset(hdr, 0, sizeof(struct ramcache_header));
    len = sizeof(struct tagcache_header);
    rc = read(fd, &hdr->h, len);
    close(fd);
    
    if (hdr->h.magic != TAGCACHE_MAGIC || rc != len)
    {
        logf("incorrect header");
        remove_files();
        hdr = NULL;
        return false;
    }

    hdr->indices = (struct index_entry *)(hdr + 1);
    
    /** 
     * Now calculate the required cache size plus 
     * some extra space for alignment fixes. 
     */
    tagcache_size = hdr->h.datasize + 128 +
        sizeof(struct index_entry) * hdr->h.entry_count +
        sizeof(struct ramcache_header) + TAG_COUNT*sizeof(void *);
    logf("tagcache: %d bytes allocated.", tagcache_size);
    logf("at: 0x%04x", audiobuf);
    audiobuf += (long)((tagcache_size & ~0x03) + 0x04);

    return true;
}

static bool load_tagcache(void)
{
    struct tagcache_header *tch;
    long bytesleft = tagcache_size;
    struct index_entry *idx;
    int rc, fd;
    char *p;
    int i;

    /* We really need the dircache for this. */
    while (!dircache_is_enabled())
        sleep(HZ);

    logf("loading tagcache to ram...");
    
    fd = open(TAGCACHE_FILE_MASTER, O_RDONLY);
    if (fd < 0)
    {
        logf("tagcache open failed");
        return false;
    }

    lseek(fd, sizeof(struct tagcache_header), SEEK_SET);
    
    idx = hdr->indices;

    /* Load the master index table. */
    for (i = 0; i < hdr->h.entry_count; i++)
    {
        rc = read(fd, idx, sizeof(struct index_entry));
        if (rc != sizeof(struct index_entry))
        {
            logf("read error #1");
            close(fd);
            return false;
        }
    
        bytesleft -= sizeof(struct index_entry);
        if (bytesleft < 0 || ((long)idx - (long)hdr->indices) >= tagcache_size)
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
    for (i = 0; i < TAG_COUNT; i++)
    {
        struct tagfile_entry *fe;
        char buf[MAX_PATH];

        //p = ((void *)p+1);
        p = (char *)((long)p & ~0x03) + 0x04;
        hdr->tags[i] = p;

        snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, i);
        fd = open(buf, O_RDONLY);
        
        if (fd < 0)
        {
            logf("%s open fail", buf);
            return false;
        }

        /* Check the header. */
        tch = (struct tagcache_header *)p;
        rc = read(fd, tch, sizeof(struct tagcache_header));
        p += rc;
        if (rc != sizeof(struct tagcache_header) ||
            tch->magic != TAGCACHE_MAGIC)
        {
            logf("incorrect header");
            close(fd);
            return false;
        }
        
        for (hdr->entry_count[i] = 0;
             hdr->entry_count[i] < tch->entry_count;
             hdr->entry_count[i]++)
        {
            yield();
            fe = (struct tagfile_entry *)p;
#ifdef ROCKBOX_STRICT_ALIGN
            /* Make sure the entry is long aligned. */
            if ((long)fe & 0x03)
            {
                bytesleft -= 4 - ((long)fe & 0x03);
                fe = (struct tagfile_entry *)(((long)fe & ~0x03) + 0x04);
            }
#endif
            rc = read(fd, fe, sizeof(struct tagfile_entry));
            if (rc != sizeof(struct tagfile_entry))
            {
                /* End of lookup table. */
                logf("read error");
                close(fd);
                return false;
            }

            /* We have a special handling for the filename tags. */
            if (i == tag_filename)
            {
                const struct dircache_entry *dc;
                
                if (fe->tag_length >= (long)sizeof(buf)-1)
                {
                    logf("too long filename");
                    close(fd);
                    return false;
                }
                
                rc = read(fd, buf, fe->tag_length);
                if (rc != fe->tag_length)
                {
                    logf("read error #3");
                    close(fd);
                    return false;
                }
                
                dc = dircache_get_entry_ptr(buf);
                if (dc == NULL)
                {
                    logf("Entry no longer valid.");
                    logf("-> %s", buf);
                    continue ;
                }

                hdr->indices[hdr->entry_count[i]].tag_seek[tag_filename]
                        = (long)dc;
                
                continue ;
            }

            bytesleft -= sizeof(struct tagfile_entry) + fe->tag_length;
            if (bytesleft < 0)
            {
                logf("too big tagcache.");
                close(fd);
                return false;
            }

            p = fe->tag_data;
            rc = read(fd, fe->tag_data, fe->tag_length);
            p += rc;

            if (rc != fe->tag_length)
            {
                logf("read error #4");
                logf("rc=0x%04x", rc); // 0x431
                logf("len=0x%04x", fe->tag_length); // 0x4000
                logf("pos=0x%04x", lseek(fd, 0, SEEK_CUR)); // 0x433
                logf("i=0x%02x", i); // 0x00
                close(fd);
                return false;
            }
        }
        close(fd);
    }
    
    logf("tagcache loaded into ram!");

    return true;
}
#endif

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
    while (queue_empty(&tagcache_queue))
    {
        struct dircache_entry *entry;

        entry = readdir_cached(dir);
    
        if (entry == NULL)
        {
            success = true;
            break ;
        }
        
        if (!strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, ".."))
            continue;

        yield();
        
        len = strlen(curpath);
        snprintf(&curpath[len], curpath_size - len, "/%s",
                 entry->d_name);
        
        processed_dir_count++;
        if (entry->attribute & ATTR_DIRECTORY)
            check_dir(curpath);
        else
#ifdef HAVE_TC_RAMCACHE
            add_tagcache(curpath, dir->internal_entry);
#else
            add_tagcache(curpath);
#endif

        curpath[len] = '\0';
    }
    
    closedir_cached(dir);

    return success;
}

static void build_tagcache(void)
{
    struct tagcache_header header;
    bool ret;
    char buf[MAX_PATH];

    curpath[0] = '\0';
    data_size = 0;
    total_entry_count = 0;
    processed_dir_count = 0;
    
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
        logf("master file open failed");
        return ;
    }

    snprintf(buf, sizeof buf, TAGCACHE_FILE_INDEX, tag_filename);
    filenametag_fd = open(buf, O_RDONLY);

    if (filenametag_fd >= 0)
    {
        if (read(filenametag_fd, &header, sizeof(struct tagcache_header)) !=
            sizeof(struct tagcache_header) || header.magic != TAGCACHE_MAGIC)
        {
            logf("header error");
            close(filenametag_fd);
            filenametag_fd = -1;
        }
    }

            
    cpu_boost(true);

    /* Scan for new files. */
    memset(&header, 0, sizeof(struct tagcache_header));
    write(cachefd, &header, sizeof(struct tagcache_header));

    //strcpy(curpath, "/Best");
    ret = check_dir("/");
    
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
    if (commit())
    {
        remove(TAGCACHE_FILE_TEMP);
        logf("tagcache built!");
    }
    
    cpu_boost(false);
}

#ifdef HAVE_TC_RAMCACHE
static void load_ramcache(void)
{
    if (!hdr)
        return ;
        
    cpu_boost(true);
    
    /* At first we should load the cache (if exists). */
    ramcache = load_tagcache();

    if (!ramcache)
    {
        hdr = NULL;
        remove_files();
    }
    
    cpu_boost(false);
}
#endif

static void tagcache_thread(void)
{
    struct event ev;
    bool check_done = false;

    while (1)
    {
        queue_wait_w_tmo(&tagcache_queue, &ev, HZ);

        switch (ev.id)
        {
            case Q_START_SCAN:
                check_done = false;
                break ;

            case Q_FORCE_UPDATE:
                //remove_files();
                build_tagcache();
                break ;
                
#ifdef HAVE_TC_RAMCACHE
            case SYS_TIMEOUT:
                if (check_done || !dircache_is_enabled())
                    break ;
                
                if (!ramcache && global_settings.tagcache_ram)
                    load_ramcache();
    
                if (global_settings.tagcache_ram)
                    build_tagcache();
                
                check_done = true;
                break ;
#endif

            case Q_STOP_SCAN:
                break ;
                
            case SYS_POWEROFF:
                break ;
                
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&tagcache_queue);
                break ;
#endif
        }
    }
}

int tagcache_get_progress(void)
{
    int total_count = -1;

#ifdef HAVE_DIRCACHE
    if (dircache_is_enabled())
    {
        total_count = dircache_get_entry_count();
    }
    else
    {
        if (hdr)
            total_count = hdr->h.entry_count;
    }
#endif

    if (total_count < 0)
        return -1;
    
    return processed_dir_count * 100 / total_count;
}

void tagcache_start_scan(void)
{
    queue_post(&tagcache_queue, Q_START_SCAN, 0);
}

bool tagcache_force_update(void)
{
    queue_post(&tagcache_queue, Q_FORCE_UPDATE, 0);
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
    return ramcache;
}
#endif

void tagcache_init(void)
{
    /* If the previous cache build/update was interrupted, commit
     * the changes first. */
    cpu_boost(true);
    allocate_tempbuf();
    commit();
    free_tempbuf();
    cpu_boost(false);
    
#ifdef HAVE_TC_RAMCACHE
    /* Allocate space for the tagcache if found on disk. */
    allocate_tagcache();
#endif
    
    queue_init(&tagcache_queue);
    create_thread(tagcache_thread, tagcache_stack,
                  sizeof(tagcache_stack), tagcache_thread_name);
}


