/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Declarations for stream-specific threading
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
#ifndef STREAM_THREAD_H
#define STREAM_THREAD_H

#define PKT_HAS_TS  0x1

/* Stream header which is the minimum to receive asynchronous buffering
 * notifications.
 * Layed-out to allow streaming access after random-access parsing */
struct stream_hdr
{
    struct event_queue *q; /* Communication queue - separate to allow it
                              to be placed in another section */
    off_t win_left;        /* Left position within data stream */
    union
    {
        off_t win_right;   /* Right position within data stream */
        off_t pos;         /* Start/current position for random-access read */
    };
    off_t limit;           /* Limit for random-access read */
    struct list_item nf;   /* List for data notification */
};

struct stream
{
    struct stream_hdr hdr;       /* Base stream data */
    struct thread_entry *thread; /* Stream's thread */
    uint8_t* curr_packet;        /* Current stream packet beginning */
    uint8_t* curr_packet_end;    /* Current stream packet end */
    struct list_item l;          /* List of streams - either reserve pool
                                    or active pool */
    int      state;              /* State machine parsing mode */
    uint32_t start_pts;          /* First timestamp for stream */
    uint32_t end_pts;            /* Last timestamp for stream */
    uint32_t pts;                /* Last presentation timestamp */
    uint32_t pkt_flags;          /* PKT_* flags */
    unsigned id;                 /* Stream identifier */
};

/* Make sure there there is always enough data buffered ahead for
 * the worst possible case - regardless of whether a valid stream
 * would actually produce that */
#define MIN_BUFAHEAD (21+65535+6+65535+6) /* 131103 */

/* States that a stream's thread assumes internally */
enum thread_states
{
                            /* Stream thread... */
    TSTATE_INIT = 0,        /* is initialized and primed */
    TSTATE_DATA,            /* is awaiting data to be available */
    TSTATE_BUFFERING,       /* is buffering data */
    TSTATE_EOS,             /* has hit the end of data */
    TSTATE_DECODE,          /* is in a decoding state */
    TSTATE_RENDER,          /* is in a rendering state */
    TSTATE_RENDER_WAIT,     /* is waiting to render */
    TSTATE_RENDER_WAIT_END, /* is waiting on remaining data */
};

/* Commands that streams respond to */
enum stream_message
{
    STREAM_NULL = 0,   /* A NULL message for whatever reason -
                          usually ignored */
    STREAM_PLAY,       /* Start playback at current position */
    STREAM_PAUSE,      /* Stop playing and await further commands */
    STREAM_RESET,      /* Reset the stream for a discontinuity */
    STREAM_STOP,       /* Stop stream - requires a reset later */
    STREAM_SEEK,       /* Seek the current stream to a new location */
    STREAM_OPEN,       /* Open a new file */
    STREAM_CLOSE,      /* Close the current file */
    STREAM_QUIT,       /* Exit the stream and thread */
    STREAM_NEEDS_SYNC, /* Need to sync before stream decoding? */
    STREAM_SYNC,       /* Sync to the specified time from some key point */
    STREAM_FIND_END_TIME, /* Get the exact end time of an elementary
                           * stream - ie. time just after last frame is finished */
    /* Disk buffer */
    STREAM_DISK_BUF_FIRST,
    DISK_BUF_DATA_NOTIFY = STREAM_DISK_BUF_FIRST,
    DISK_BUF_CLEAR_DATA_NOTIFY, /* Cancel pending data notification */
    DISK_BUF_CACHE_RANGE,       /* Cache a range of the file in the buffer */
    /* Audio stream */
    STREAM_AUDIO_FIRST,
    /* Video stream */
    STREAM_VIDEO_FIRST,
    VIDEO_DISPLAY_SHOW = STREAM_VIDEO_FIRST, /* Show/hide video output */
    VIDEO_DISPLAY_IS_VISIBLE, /* Is the video output visible? */
    VIDEO_GET_SIZE,           /* Get the video dimensions */
    VIDEO_PRINT_FRAME,        /* Print the frame at the current position */
    VIDEO_PRINT_THUMBNAIL,    /* Print a thumbnail of the current position */
    VIDEO_SET_CLIP_RECT,      /* Set the visible video area */
#ifdef GRAY_CACHE_MAINT
    VIDEO_GRAY_CACHEOP,
#endif
    STREAM_MESSAGE_LAST,
};

/* Data parameter for STREAM_SEEK */
struct stream_seek_data
{
    uint32_t time; /* Time to seek to/by */
    int whence;    /* Specification of relationship to current position/file */
};

/* Data parameter for STREAM_SYNC */
struct str_sync_data
{
    uint32_t time; /* Time to sync to */
    struct stream_scan sk; /* Specification of start/limits/direction */
};

/* Stream status codes - not eqivalent to thread states */
enum stream_status
{
                             /* Stream status is... */
    STREAM_DATA_END       = -4, /* Stream has ended */
    STREAM_DATA_NOT_READY = -3, /* Data was not available yet */
    STREAM_UNSUPPORTED    = -2, /* Format is unsupported */
    STREAM_ERROR          = -1, /* some kind of error - quit it or reset it */
    STREAM_OK             =  0, /* General inequality for success >= is OK, < error */
    STREAM_STOPPED        =  0, /* stopped and awaiting commands - send STREAM_INIT */
    STREAM_PLAYING,             /* playing and rendering its data */
    STREAM_PAUSED,              /* paused and awaiting commands */
    /* Other status codes (> STREAM_OK) */
    STREAM_MATCH,               /* A good match was found */
    STREAM_PERFECT_MATCH,       /* Exactly what was wanted was found or
                                   no better match is possible */
    STREAM_NOT_FOUND,           /* Match not found */
};

#define STR_FROM_HEADER(sh) ((struct stream *)(sh))

/* Clip time to range for a particular stream */
static inline uint32_t clip_time(struct stream *str, uint32_t time)
{
    if (time < str->start_pts)
        time = str->start_pts;
    else if (time >= str->end_pts)
        time = str->end_pts;

    return time;
}

extern struct stream video_str IBSS_ATTR;
extern struct stream audio_str IBSS_ATTR;

bool video_thread_init(void);
void video_thread_exit(void);
bool audio_thread_init(void);
void audio_thread_exit(void);

/* Some queue function wrappers to keep things clean-ish */

/* For stream use only */
static inline bool str_have_msg(struct stream *str)
    { return !rb->queue_empty(str->hdr.q); }

static inline void str_get_msg(struct stream *str, struct queue_event *ev)
    { rb->queue_wait(str->hdr.q, ev); }

static inline void str_get_msg_w_tmo(struct stream *str, struct queue_event *ev,
                                     int timeout)
    { rb->queue_wait_w_tmo(str->hdr.q, ev, timeout); }

static inline void str_reply_msg(struct stream *str, intptr_t reply)
    { rb->queue_reply(str->hdr.q, reply); }

/* Public use */
static inline intptr_t str_send_msg(struct stream *str, long id, intptr_t data)
    { return rb->queue_send(str->hdr.q, id, data); }

static inline void str_post_msg(struct stream *str, long id, intptr_t data)
    { rb->queue_post(str->hdr.q, id, data); }

#endif /* STREAM_THREAD_H */
