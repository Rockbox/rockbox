/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * AV disk buffer declarations
 *
 * Copyright (c) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef DISK_BUF_H
#define DISK_BUF_H

#define DISK_BUF_PAGE_SHIFT 15  /* 32KB cache lines */
#define DISK_BUF_PAGE_SIZE (1 << DISK_BUF_PAGE_SHIFT)
#define DISK_BUF_PAGE_MASK (DISK_BUF_PAGE_SIZE-1)

enum
{
    DISK_BUF_NOTIFY_ERROR = -1,
    DISK_BUF_NOTIFY_NULL  =  0,
    DISK_BUF_NOTIFY_OK,
    DISK_BUF_NOTIFY_TIMEDOUT,
    DISK_BUF_NOTIFY_PROCESS_EVENT,
    DISK_BUF_NOTIFY_REGISTERED,
};

/** Macros to map file offsets to cached data **/

/* Returns a cache tag given a file offset */
#define MAP_OFFSET_TO_TAG(o) \
    ((o) >> DISK_BUF_PAGE_SHIFT)

/* Returns the cache page number given a file offset */
#define MAP_OFFSET_TO_PAGE(o) \
    (MAP_OFFSET_TO_TAG(o) % disk_buf.pgcount)

/* Returns the buffer offset given a file offset */
#define MAP_OFFSET_TO_BUFFER(o) \
    (MAP_OFFSET_TO_PAGE(o) * DISK_BUF_PAGE_SIZE)

struct dbuf_range
{
    uint32_t tag_start;
    uint32_t tag_end;
    int pg_start;
};

/* This object is an extension of the stream manager and handles some
 * playback events as well as buffering */
struct disk_buf
{
    struct thread_entry *thread;
    struct event_queue *q;
    uint8_t *start;   /* Start pointer */
    uint8_t *end;     /* End of buffer pointer less MPEG_GUARDBUF_SIZE. The
                         guard space is used to wrap data at the buffer start to
                         pass continuous data packets */
    uint8_t *tail;    /* Location of last data + 1 filled into the buffer */
    ssize_t size;     /* The buffer length _not_ including the guard space (end-start) */
    int    pgcount;   /* Total number of available cached pages */
    uint32_t *cache;  /* Pointer to cache structure - allocated on buffer */
    int    in_file;   /* File being read */
    ssize_t filesize; /* Size of file in_file in bytes */
    int file_pages;   /* Number of pages in file (rounded up) */
    off_t offset;     /* Current position (random access) */
    off_t win_left;   /* Left edge of buffer window (streaming) */
    off_t win_right;  /* Right edge of buffer window (streaming) */
    uint32_t time_last; /* Last time watermark was checked */
    off_t pos_last;   /* Last position at watermark check time */
    ssize_t low_wm;   /* The low watermark for automatic rebuffering */
    int status;       /* Status as stream */
    int state;        /* Current thread state */
    bool need_seek;   /* Need to seek because a read was not contiguous */
};

extern struct disk_buf disk_buf SHAREDBSS_ATTR;

static inline bool disk_buf_is_data_ready(struct stream_hdr *sh,
                                          ssize_t margin)
{
    /* Data window available? */
    off_t right = sh->win_right;

    /* Margins past end-of-file can still return true */
    if (right > disk_buf.filesize - margin)
       right = disk_buf.filesize - margin;

    return sh->win_left >= disk_buf.win_left &&
           right + margin <= disk_buf.win_right;
}


bool disk_buf_init(void);
void disk_buf_exit(void);

static inline int disk_buf_status(void)
    { return disk_buf.status; }

int disk_buf_open(const char *filename);
void disk_buf_close(void);
ssize_t _disk_buf_getbuffer(size_t size, void **pp, void **pwrap,
                            size_t *sizewrap);
#define disk_buf_getbuffer(size, pp, pwrap, sizewrap) \
    _disk_buf_getbuffer((size), PUN_PTR(void **, (pp)), \
                        PUN_PTR(void **, (pwrap)), (sizewrap))
ssize_t disk_buf_read(void *buffer, size_t size);
ssize_t disk_buf_lseek(off_t offset, int whence);

static inline off_t disk_buf_ftell(void)
    { return disk_buf.offset; }

static inline ssize_t disk_buf_filesize(void)
    { return disk_buf.filesize; }

ssize_t disk_buf_prepare_streaming(off_t pos, size_t len);
ssize_t disk_buf_set_streaming_window(off_t left, off_t right);
void * disk_buf_offset2ptr(off_t offset);
int disk_buf_check_streaming_window(off_t left, off_t right);

intptr_t disk_buf_send_msg(long id, intptr_t data);
void disk_buf_post_msg(long id, intptr_t data);
void disk_buf_reply_msg(intptr_t retval);

#endif /* DISK_BUF_H */
