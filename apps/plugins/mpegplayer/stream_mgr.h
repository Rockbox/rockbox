/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * AV stream manager decalarations
 *
 * Copyright (c) 2007 Michael Sevakis
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
#ifndef STREAM_MGR_H
#define STREAM_MGR_H

/* Basic media control interface - this handles state changes and stream
 * coordination with assistance from the parser */
struct stream_mgr
{
    struct thread_entry *thread; /* Playback control thread */
    struct event_queue *q;       /* event queue for control thread */
    const char *filename;        /* Current filename */
    uint32_t resume_time;        /* The stream tick where playback was
                                    stopped (or started) */
    bool seeked;                 /* A seek happened and things must be
                                    resynced */
    int    status;          /* Current playback status */
    struct list_item strl;  /* List of available streams */
    struct list_item actl;  /* List of active streams */
    struct mutex str_mtx;   /* Main stream manager mutex */
    struct mutex actl_mtx;  /* Lock for current-streams list */
    union /* A place for reusable non-cacheable parameters */
    {
        struct vo_rect rc;
        struct stream_seek_data skd;
    } parms;
};

extern struct stream_mgr stream_mgr SHAREDBSS_ATTR;

struct stream_window
{
    off_t left, right;
};

/** Interface for use by streams and other internal objects **/
bool stream_get_window(struct stream_window *sw);
void stream_clear_notify(struct stream *str, int for_msg);
int str_next_data_not_ready(struct stream *str);
/* Called by a stream to say it got its buffering notification */
void str_data_notify_received(struct stream *str);
void stream_add_stream(struct stream *str);
void stream_remove_streams(void);

enum stream_events
{
    __STREAM_EV_FIRST = STREAM_MESSAGE_LAST-1,
    STREAM_EV_COMPLETE,
};

void stream_generate_event(struct stream *str, long id, intptr_t data);

/** Main control functions **/

/* Initialize the playback engine */
int stream_init(void);

/* Close the playback engine */
void stream_exit(void);

/* Open a new file */
int stream_open(const char *filename);

/* Close the current file */
int stream_close(void);

/* Plays from the current seekpoint if stopped */
int stream_play(void);

/* Pauses playback if playing */
int stream_pause(void);

/* Resumes playback if paused */
int stream_resume(void);

/* Stops all streaming activity if playing or paused */
int stream_stop(void);

/* Point stream at a particular time.
 * whence = one of SEEK_SET, SEEK_CUR, SEEK_END */
int stream_seek(uint32_t time, int whence);

/* Show/Hide the video image at the current seekpoint */
bool stream_show_vo(bool show);

/* Set the visible section of video */
void stream_vo_set_clip(const struct vo_rect *rc);

#ifndef HAVE_LCD_COLOR
void stream_gray_show(bool show);
#endif

/* Display thumbnail of the current seekpoint */
bool stream_display_thumb(const struct vo_rect *rc);

/* Draw the frame at the current position */
bool stream_draw_frame(bool no_prepare);

/* Return video dimensions */
bool stream_vo_get_size(struct vo_ext *sz);

/* Returns the resume time in timestamp ticks */
uint32_t stream_get_resume_time(void);

/* Returns stream_get_time if no seek is pending or else the
   last time give to seek */
uint32_t stream_get_seek_time(uint32_t *start);

/* Return the absolute stream time in clock ticks - adjusted by
 * master clock stream via audio timestamps */
static inline uint32_t stream_get_time(void)
    { return pcm_output_get_clock(); }

/* Return the absolute clock time in clock ticks - unadjusted */
static inline uint32_t stream_get_ticks(uint32_t *start)
    { return pcm_output_get_ticks(start); }

/* Returns the current playback status */
static inline int stream_status(void)
    { return stream_mgr.status; }

/* Wait for a state transistion to complete */
void stream_wait_status(void);

/* Returns the playback length of the stream */
static inline uint32_t stream_get_duration(void)
    { return str_parser.duration; }

static inline bool stream_can_seek(void)
    { return parser_can_seek(); }

/* Keep the disk spinning (for seeking and browsing) */
static inline void stream_keep_disk_active(void)
{
#ifndef HAVE_FLASH_STORAGE
    rb->ata_spin();
#endif
    }

#endif /* STREAM_MGR_H */
