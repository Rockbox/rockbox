/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * AV stream manager implementation
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
#include "plugin.h"
#include "mpegplayer.h"
#include "grey.h"
#include "mpeg_settings.h"

static struct event_queue stream_mgr_queue NOCACHEBSS_ATTR;
static struct queue_sender_list stream_mgr_queue_send NOCACHEBSS_ATTR;
static uint32_t stream_mgr_thread_stack[DEFAULT_STACK_SIZE*2/sizeof(uint32_t)];

struct stream_mgr stream_mgr NOCACHEBSS_ATTR;

/* Forward decs */
static int stream_on_close(void);

struct str_broadcast_data
{
    long cmd;       /* Command to send to stream */
    intptr_t data;  /* Data to send with command */
};

static inline void stream_mgr_lock(void)
{
    rb->mutex_lock(&stream_mgr.str_mtx);
}

static inline void stream_mgr_unlock(void)
{
    rb->mutex_unlock(&stream_mgr.str_mtx);
}

static inline void actl_lock(void)
{
    rb->mutex_lock(&stream_mgr.actl_mtx);
}

static inline void actl_unlock(void)
{
    rb->mutex_unlock(&stream_mgr.actl_mtx);
}

static inline void stream_mgr_post_msg(long id, intptr_t data)
{
    rb->queue_post(stream_mgr.q, id, data);
}

static inline intptr_t stream_mgr_send_msg(long id, intptr_t data)
{
    return rb->queue_send(stream_mgr.q, id, data);
}

static inline void stream_mgr_reply_msg(intptr_t retval)
{
    rb->queue_reply(stream_mgr.q, retval);
}

int str_next_data_not_ready(struct stream *str)
{
    /* Save the current window since it actually might be ready by the time
     * the registration is received by buffering. */
    off_t win_right = str->hdr.win_right;

    if (str->hdr.win_right < disk_buf.filesize - MIN_BUFAHEAD &&
        disk_buf.filesize > MIN_BUFAHEAD)
    {
        /* Set right edge to where probing left off + the minimum margin */
        str->hdr.win_right += MIN_BUFAHEAD;
    }
    else
    {
        /* Request would be passed the end of the file */
        str->hdr.win_right = disk_buf.filesize;
    }

    switch (disk_buf_send_msg(DISK_BUF_DATA_NOTIFY, (intptr_t)str))
    {
    case DISK_BUF_NOTIFY_OK:
        /* Was ready - restore window and process */
        str->hdr.win_right = win_right;
        return STREAM_OK;

    case DISK_BUF_NOTIFY_ERROR:
        /* Error - quit parsing */
        str_end_of_stream(str);
        return STREAM_DATA_END;

    default:
        /* Not ready - go wait for notification from buffering. */
        str->pkt_flags = 0;
        return STREAM_DATA_NOT_READY;
    }
}

void str_data_notify_received(struct stream *str)
{
    /* Normalize win_right back to the packet length */
    if (str->state == SSTATE_END)
        return;

    if (str->curr_packet == NULL)
    {
        /* Nothing was yet parsed since init */
        str->hdr.win_right = str->hdr.win_left;
    }
    else
    {
        /* Restore window based upon current packet */
        str->hdr.win_right = str->hdr.win_left +
            (str->curr_packet_end - str->curr_packet);
    }
}

/* Set stream manager to a "no-file" state */
static void stream_mgr_init_state(void)
{
    stream_mgr.filename = NULL;
    stream_mgr.resume_time = 0;
    stream_mgr.seeked = false;
}

/* Add a stream to the playback pool */
void stream_add_stream(struct stream *str)
{
    actl_lock();

    list_remove_item(&str->l);
    list_add_item(&stream_mgr.strl, &str->l);

    actl_unlock();
}

/* Callback for various list-moving operations */
static bool strl_enum_callback(struct list_item *item, intptr_t data)
{
    actl_lock();

    list_remove_item(item);

    if (data == 1)
        list_add_item(&stream_mgr.actl, item);

    actl_unlock();

    return true;
}

/* Clear all streams from active and playback pools */
void stream_remove_streams(void)
{
    list_enum_items(&stream_mgr.strl, strl_enum_callback, 0);
}

/* Move the playback pool to the active list */
void move_strl_to_actl(void)
{
    list_enum_items(&stream_mgr.strl, strl_enum_callback, 1);
}

/* Remove a stream from the active list and return it to the pool */
static bool actl_stream_remove(struct stream *str)
{
    if (list_is_member(&stream_mgr.actl, &str->l))
    {
        actl_lock();

        list_remove_item(&str->l);
        list_add_item(&stream_mgr.strl, &str->l);

        actl_unlock();
        return true;
    }

    return false;
}

/* Broadcast a message to all active streams */
static bool actl_stream_broadcast_callback(struct list_item *item,
                                           struct str_broadcast_data *sbd)
{
    struct stream *str = TYPE_FROM_MEMBER(struct stream, item, l);

    switch (sbd->cmd)
    {
    case STREAM_PLAY:
    case STREAM_PAUSE:
        break;

    case STREAM_STOP:
        if (sbd->data != 0)
        {
            actl_lock();

            list_remove_item(item);
            list_add_item(&stream_mgr.strl, item);

            actl_unlock();
            sbd->data = 0;
        }
        break;

    default:
        return false;
    }

    str_send_msg(str, sbd->cmd, sbd->data);
    return true;
}

static void actl_stream_broadcast(int cmd, intptr_t data)
{
    struct str_broadcast_data sbd;
    sbd.cmd = cmd;
    sbd.data = data;
    list_enum_items(&stream_mgr.actl,
                    (list_enum_callback_t)actl_stream_broadcast_callback,
                    (intptr_t)&sbd);
}

/* Set the current base clock */
static void set_stream_clock(uint32_t time)
{
    /* Fudge: Start clock 100ms early to allow for some filling time */
    if (time > 100*TS_SECOND/1000)
        time -= 100*TS_SECOND/1000;
    else
        time = 0;

    pcm_output_set_clock(TS_TO_TICKS(time));
}

static void stream_start_playback(uint32_t time, bool fill_buffer)
{
    if (stream_mgr.seeked)
    {
        /* Clear any seeked status */
        stream_mgr.seeked = false;

        /* Flush old PCM data */
        pcm_output_flush();

        /* Set the master clock */
        set_stream_clock(time);

        /* Make sure streams are back in active pool */
        move_strl_to_actl();

        /* Prepare the parser and associated streams */
        parser_prepare_streaming();
    }

    /* Start buffer which optional force fill */
    disk_buf_send_msg(STREAM_PLAY, fill_buffer);

    /* Tell each stream to start - may generate end of stream signals
     * now - we'll handle this when finished */
    actl_stream_broadcast(STREAM_PLAY, 0);

    /* Actually start the clock */
    pcm_output_play_pause(true);
}

/* Return the play time relative to the specified play time */
static uint32_t time_from_whence(uint32_t time, int whence)
{
    int64_t currtime;
    uint32_t start;

    switch (whence)
    {
    case SEEK_SET:
        /* Set the current time (time = unsigned offset from 0) */
        if (time > str_parser.duration)
            time = str_parser.duration;
        break;
    case SEEK_CUR:
        /* Seek forward or backward from the current time
         * (time = signed offset from current) */
        currtime = stream_get_seek_time(&start);
        currtime -= start;
        currtime += (int32_t)time;

        if (currtime < 0)
            currtime = 0;
        else if ((uint64_t)currtime > str_parser.duration)
            currtime = str_parser.duration;

        time = (uint32_t)currtime;
        break;
    case SEEK_END:
        /* Seek from the end (time = unsigned offset from end) */
        if (time > str_parser.duration)
            time = str_parser.duration;
        time = str_parser.duration - time;
        break;
    }

    return time;
}

/* Handle seeking details if playing or paused */
static uint32_t stream_seek_intl(uint32_t time, int whence,
                                 int status, bool *was_buffering)
{
    if (status != STREAM_STOPPED)
    {
        bool wb;

        /* Place streams in a non-running state - keep them on actl */
        actl_stream_broadcast(STREAM_STOP, 0);

        /* Stop all buffering or else risk clobbering random-access data */
        wb = disk_buf_send_msg(STREAM_STOP, 0);

        if (was_buffering != NULL)
            *was_buffering = wb;
    }

    time = time_from_whence(time, whence);

    stream_mgr.seeked = true;

    return parser_seek_time(time);
}

/* Handle STREAM_OPEN */
void stream_on_open(const char *filename)
{
    int err = STREAM_ERROR;

    stream_mgr_lock();

    trigger_cpu_boost();

    /* Open the video file */
    if (disk_buf_open(filename) >= 0)
    {
        /* Initialize the parser */
        err = parser_init_stream();

        if (err >= STREAM_OK)
        {
            /* File ok - save the opened filename */
            stream_mgr.filename = filename;
        }
    }

    /* If error - cleanup */
    if (err < STREAM_OK)
        stream_on_close();

    cancel_cpu_boost();

    stream_mgr_unlock();

    stream_mgr_reply_msg(err);
}

/* Handler STREAM_PLAY */
static void stream_on_play(void)
{
    int status = stream_mgr.status;

    stream_mgr_lock();

    if (status == STREAM_STOPPED)
    {
        uint32_t start;

        /* We just say we're playing now */
        stream_mgr.status = STREAM_PLAYING;

        /* Reply with previous state */
        stream_mgr_reply_msg(status);

        trigger_cpu_boost();

        /* Seek to initial position and set clock to that time */

        /* Save the resume time */
        start = str_parser.last_seek_time - str_parser.start_pts;
        stream_mgr.resume_time = start;

        /* Prepare seek to start point */
        start = stream_seek_intl(start, SEEK_SET, STREAM_STOPPED, NULL);

        /* Sync and start - force buffer fill */
        stream_start_playback(start, true);
    }
    else
    {
        /* Reply with previous state */
        stream_mgr_reply_msg(status);
    }

    stream_mgr_unlock();
}

/* Handle STREAM_PAUSE */
static void stream_on_pause(void)
{
    int status = stream_mgr.status;

    stream_mgr_lock();

    /* Reply with previous state */
    stream_mgr_reply_msg(status);

    if (status == STREAM_PLAYING)
    {
        /* Pause the clock */
        pcm_output_play_pause(false);

        /* Pause each active stream */
        actl_stream_broadcast(STREAM_PAUSE, 0);

        /* Pause the disk buffer - buffer may continue filling */
        disk_buf_send_msg(STREAM_PAUSE, false);

        /* Unboost the CPU */
        cancel_cpu_boost();

        /* Offically paused */
        stream_mgr.status = STREAM_PAUSED;
    }

    stream_mgr_unlock();
}

/* Handle STREAM_RESUME */
static void stream_on_resume(void)
{
    int status = stream_mgr.status;

    stream_mgr_lock();

    /* Reply with previous state */
    stream_mgr_reply_msg(status);

    if (status == STREAM_PAUSED)
    {
        /* Boost the CPU */
        trigger_cpu_boost();

        /* Sync and start - no force buffering */
        stream_start_playback(str_parser.last_seek_time, false);

        /* Officially playing */
        stream_mgr.status = STREAM_PLAYING;
    }

    stream_mgr_unlock();
}

/* Handle STREAM_STOP */
static void stream_on_stop(bool reply)
{
    int status = stream_mgr.status;

    stream_mgr_lock();

    if (reply)
        stream_mgr_reply_msg(status);

    if (status != STREAM_STOPPED)
    {
        /* Pause the clock */
        pcm_output_play_pause(false);

        /* Assume invalidity */
        stream_mgr.resume_time = 0;

        if (stream_can_seek())
        {
            /* Read the current stream time or the last seeked position */
            uint32_t start;
            uint32_t time = stream_get_seek_time(&start);

            if (time >= str_parser.start_pts && time < str_parser.end_pts)
            {
                /* Save the current stream time */
                stream_mgr.resume_time = time - start;
            }
        }

        /* Not stopped = paused or playing */
        stream_mgr.seeked = false;

        /* Stop buffering */
        disk_buf_send_msg(STREAM_STOP, 0);

        /* Clear any still-active streams and remove from actl */
        actl_stream_broadcast(STREAM_STOP, 1);

        /* Stop PCM output (and clock) */
        pcm_output_stop();

        /* Cancel our processor boost */
        cancel_cpu_boost();

        stream_mgr.status = STREAM_STOPPED;
    }

    stream_mgr_unlock();
}

/* Handle STREAM_SEEK */
static void stream_on_seek(struct stream_seek_data *skd)
{
    uint32_t time = skd->time;
    int whence = skd->whence;

    switch (whence)
    {
    case SEEK_SET:
    case SEEK_CUR:
    case SEEK_END:
        if (stream_mgr.filename == NULL)
            break;

        /* Keep things spinning if already doing so */
        stream_keep_disk_active();

        /* Have data - reply in order to acquire lock */
        stream_mgr_reply_msg(STREAM_OK);

        stream_mgr_lock();

        if (stream_can_seek())
        {
            bool buffer;

            if (stream_mgr.status == STREAM_PLAYING)
            {
                /* Keep clock from advancing while seeking */
                pcm_output_play_pause(false);
            }

            time = stream_seek_intl(time, whence, stream_mgr.status, &buffer);

            if (stream_mgr.status == STREAM_PLAYING)
            {
                /* Sync and restart - no force buffering */
                stream_start_playback(time, buffer);
            }
        }

        stream_mgr_unlock();
        return;
    }

    /* Invalid parameter or no file */
    stream_mgr_reply_msg(STREAM_ERROR);
}

/* Handle STREAM_CLOSE */
static int stream_on_close(void)
{
    int status = STREAM_STOPPED;

    stream_mgr_lock();

    /* Any open file? */
    if (stream_mgr.filename != NULL)
    {
        /* Yes - hide video */
        stream_show_vo(false);
        /* Stop any playback */
        status = stream_mgr.status;
        stream_on_stop(false);
        /* Tell parser file is finished */
        parser_close_stream();
        /* Close file */
        disk_buf_close();
        /* Reinitialize manager */
        stream_mgr_init_state();
    }

    stream_mgr_unlock();

    return status;
}

/* Handle STREAM_EV_COMPLETE */
static void stream_on_ev_complete(struct stream *str)
{
    stream_mgr_lock();

    /* Stream is active? */
    if (actl_stream_remove(str))
    {
        /* No - remove this stream from the active list */
        DEBUGF("  finished: 0x%02x\n", str->id);
        if (list_is_empty(&stream_mgr.actl))
        {
            /* All streams have acked - stop playback */
            stream_on_stop(false);
            stream_mgr.resume_time = 0; /* Played to end - no resume */
        }
        else
        {
            /* Stream is done - stop it and place back in pool */
            str_send_msg(str, STREAM_STOP, 1);
        }
    }

    stream_mgr_unlock();
}

/* Callback for stream to notify about events internal to them */
void stream_generate_event(struct stream *str, long id, intptr_t data)
{
    if (str == NULL)
        return;

    switch (id)
    {
    case STREAM_EV_COMPLETE:
        /* The last stream has ended */
        stream_mgr_post_msg(STREAM_EV_COMPLETE, (intptr_t)str);
        break;
    }

    (void)data;
}

/* Clear any particular notification for which a stream registered */
void stream_clear_notify(struct stream *str, int for_msg)
{
    switch (for_msg)
    {
    case DISK_BUF_DATA_NOTIFY:
        disk_buf_send_msg(DISK_BUF_CLEAR_DATA_NOTIFY, (intptr_t)str);
        break;
    }
}

/* Show/hide the video output */
bool stream_show_vo(bool show)
{
    bool vis;
    stream_mgr_lock();

    vis = parser_send_video_msg(VIDEO_DISPLAY_SHOW, show);
#ifndef HAVE_LCD_COLOR
    GRAY_VIDEO_INVALIDATE_ICACHE();
    GRAY_INVALIDATE_ICACHE();

    grey_show(show);

    GRAY_FLUSH_ICACHE();
#endif
    stream_mgr_unlock();

    return vis;
}

/* Query the visibility of video output */
bool stream_vo_is_visible(void)
{
    bool vis;
    stream_mgr_lock();
    vis = parser_send_video_msg(VIDEO_DISPLAY_IS_VISIBLE, 0);
    stream_mgr_unlock();
    return vis;
}

/* Return the video dimensions */
bool stream_vo_get_size(struct vo_ext *sz)
{
    bool retval = false;

    stream_mgr_lock();

    if (str_parser.dims.w > 0 && str_parser.dims.h > 0)
    {
        *sz = str_parser.dims;
        retval = true;
    }

    stream_mgr_unlock();

    return retval;
}

void stream_vo_set_clip(const struct vo_rect *rc)
{
    stream_mgr_lock();

    if (rc)
    {
        stream_mgr.parms.rc = *rc;
        rc = &stream_mgr.parms.rc;
    }

    parser_send_video_msg(VIDEO_SET_CLIP_RECT, (intptr_t)rc);

    stream_mgr_unlock();
}

#ifndef HAVE_LCD_COLOR
/* Show/hide the gray video overlay (independently of vo visibility). */
void stream_gray_show(bool show)
{
    stream_mgr_lock();

    GRAY_VIDEO_INVALIDATE_ICACHE();
    GRAY_INVALIDATE_ICACHE();

    grey_show(show);

    GRAY_FLUSH_ICACHE();

    stream_mgr_unlock();
}

#ifdef GRAY_CACHE_MAINT
void stream_gray_pause(bool pause)
{
    static bool gray_paused = false;

    if (pause && !gray_paused)
    {
        if (_grey_info.flags & _GREY_RUNNING)
        {
            rb->timer_unregister();
#if defined(CPU_PP) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
            rb->cpu_boost(false);
#endif
            _grey_info.flags &= ~_GREY_RUNNING;
            rb->screen_dump_set_hook(NULL);
            gray_paused = true;
        }
    }
    else if (!pause && gray_paused)
    {
        gray_paused = false;
        grey_show(true);
    }
}
#endif

#endif /* !HAVE_LCD_COLOR */

/* Display a thumbnail at the last seek point */
bool stream_display_thumb(const struct vo_rect *rc)
{
    bool retval;

    if (rc == NULL)
        return false;

    stream_mgr_lock();

    stream_mgr.parms.rc = *rc;
    retval = parser_send_video_msg(VIDEO_PRINT_THUMBNAIL,
                (intptr_t)&stream_mgr.parms.rc);

    stream_mgr_unlock();

    return retval;
}

bool stream_draw_frame(bool no_prepare)
{
    bool retval;
    stream_mgr_lock();

    retval = parser_send_video_msg(VIDEO_PRINT_FRAME, no_prepare);

    stream_mgr_unlock();

    return retval;
}

/* Return the time playback should resume if interrupted */
uint32_t stream_get_resume_time(void)
{
    uint32_t resume_time;

    /* A stop request is async and replies before setting this - must lock */
    stream_mgr_lock();

    resume_time = stream_mgr.resume_time;

    stream_mgr_unlock();

    return resume_time;
}

uint32_t stream_get_seek_time(uint32_t *start)
{
    uint32_t time;

    stream_mgr_lock();

    if (stream_mgr.seeked)
    {
        time = str_parser.last_seek_time;
    }
    else
    {
        time = TICKS_TO_TS(pcm_output_get_clock());

        /* Clock can be start early so keep in range */
        if (time < str_parser.start_pts)
            time = str_parser.start_pts;
    }

    if (start != NULL)
        *start = str_parser.start_pts;

    stream_mgr_unlock();

    return time;
}

/* Returns the smallest file window that includes all active streams'
 * windows */
static bool stream_get_window_callback(struct list_item *item,
                                       struct stream_window *sw)
{
    struct stream *str = TYPE_FROM_MEMBER(struct stream, item, l);    
    off_t swl = str->hdr.win_left;
    off_t swr = str->hdr.win_right;

    if (swl < sw->left)
        sw->left = swl;

    if (swr > sw->right)
        sw->right = swr;

    return true;
}

bool stream_get_window(struct stream_window *sw)
{
    if (sw == NULL)
        return false;

    sw->left = LONG_MAX;
    sw->right = LONG_MIN;

    actl_lock();
    list_enum_items(&stream_mgr.actl,
                    (list_enum_callback_t)stream_get_window_callback,
                    (intptr_t)sw);
    actl_unlock();

    return sw->left <= sw->right;
}

/* Playback control thread */
static void stream_mgr_thread(void)
{
    struct queue_event ev;

    while (1)
    {
        rb->queue_wait(stream_mgr.q, &ev);

        switch (ev.id)
        {
        case STREAM_OPEN:
            stream_on_open((const char *)ev.data);
            break;

        case STREAM_CLOSE:
            stream_on_close();
            break;

        case STREAM_PLAY:
            stream_on_play();
            break;

        case STREAM_PAUSE:
            if (ev.data)
                stream_on_resume();
            else
                stream_on_pause();
            break;

        case STREAM_STOP:
            stream_on_stop(true);
            break;

        case STREAM_SEEK:
            stream_on_seek((struct stream_seek_data *)ev.data);
            break;

        case STREAM_EV_COMPLETE:
            stream_on_ev_complete((struct stream *)ev.data);
            break;

        case STREAM_QUIT:
            if (stream_mgr.status != STREAM_STOPPED)
                stream_on_stop(false);
            return;
        }
    }
}

/* Stream command interface APIs */

/* Opens a new file */
int stream_open(const char *filename)
{
    if (stream_mgr.thread != NULL)
        return stream_mgr_send_msg(STREAM_OPEN, (intptr_t)filename);
    return STREAM_ERROR;
}

/* Plays the current file starting at time 'start' */
int stream_play(void)
{
    if (stream_mgr.thread != NULL)
        return stream_mgr_send_msg(STREAM_PLAY, 0);
    return STREAM_ERROR;
}

/* Pauses playback if playing */
int stream_pause(void)
{
    if (stream_mgr.thread != NULL)
        return stream_mgr_send_msg(STREAM_PAUSE, false);
    return STREAM_ERROR;
}

/* Resumes playback if paused */
int stream_resume(void)
{
    if (stream_mgr.thread != NULL)
        return stream_mgr_send_msg(STREAM_PAUSE, true);
    return STREAM_ERROR;
}

/* Stops playback if not stopped */
int stream_stop(void)
{
    if (stream_mgr.thread != NULL)
        return stream_mgr_send_msg(STREAM_STOP, 0);
    return STREAM_ERROR;
}

/* Seeks playback time to/by the specified time */
int stream_seek(uint32_t time, int whence)
{
    int ret;

    if (stream_mgr.thread == NULL)
        return STREAM_ERROR;

    stream_mgr_lock();

    stream_mgr.parms.skd.time = time;
    stream_mgr.parms.skd.whence = whence;

    ret = stream_mgr_send_msg(STREAM_SEEK, (intptr_t)&stream_mgr.parms.skd);

    stream_mgr_unlock();

    return ret;
}

/* Closes the current file */
int stream_close(void)
{
    if (stream_mgr.thread != NULL)
        return stream_mgr_send_msg(STREAM_CLOSE, 0);
    return STREAM_ERROR;
}

/* Initializes the playback engine */
int stream_init(void)
{
    void *mem;
    size_t memsize;

    stream_mgr.status = STREAM_STOPPED;
    stream_mgr_init_state();
    list_initialize(&stream_mgr.actl);

    /* Initialize our window to the outside world first */
    rb->mutex_init(&stream_mgr.str_mtx);
    rb->mutex_init(&stream_mgr.actl_mtx);

    stream_mgr.q = &stream_mgr_queue;
    rb->queue_init(stream_mgr.q, false);
    rb->queue_enable_queue_send(stream_mgr.q, &stream_mgr_queue_send);

    /* sets audiosize and returns buffer pointer */
    mem = rb->plugin_get_audio_buffer(&memsize);

    /* Initialize non-allocator blocks first */
#ifndef HAVE_LCD_COLOR
    bool success;
    long graysize;

    /* This can run on another processor - align data */
    memsize = CACHEALIGN_BUFFER(&mem, memsize);

    success = grey_init(rb, mem, memsize, true, LCD_WIDTH,
                        LCD_HEIGHT, &graysize);

    /* This can run on another processor - align size */
    graysize = CACHEALIGN_UP(graysize);

    mem += graysize;
    memsize -= graysize;

    if (!success || (ssize_t)memsize <= 0)
    {
        rb->splash(HZ, "greylib init failed!");
        return STREAM_ERROR;
    }

    grey_clear_display();
#endif /* !HAVE_LCD_COLOR */

    stream_mgr.thread = rb->create_thread(stream_mgr_thread,
        stream_mgr_thread_stack, sizeof(stream_mgr_thread_stack),
        0, "mpgstream_mgr" IF_PRIO(, PRIORITY_SYSTEM) IF_COP(, CPU));

    if (stream_mgr.thread == NULL)
    {
        rb->splash(HZ, "Could not create stream manager thread!");
        return STREAM_ERROR;
    }

    /* Wait for thread to initialize */
    stream_mgr_send_msg(STREAM_NULL, 0);

    /* Initialise our malloc buffer */
    if (!mpeg_alloc_init(mem, memsize))
    {
        rb->splash(HZ, "Out of memory in stream_init");
    }
    /* These inits use the allocator */
    else if (!pcm_output_init())
    {
        rb->splash(HZ, "Could not initialize PCM!");
    }
    else if (!audio_thread_init())
    {
        rb->splash(HZ, "Cannot create audio thread!");
    }
    else if (!video_thread_init())
    {
        rb->splash(HZ, "Cannot create video thread!");
    }
    /* Disk buffer takes max allotment of what's left so it must be last */
    else if (!disk_buf_init())
    {
        rb->splash(HZ, "Cannot create buffering thread!");
    }
    else if (!parser_init())
    {
        rb->splash(HZ, "Parser init failed!");
    }
    else
    {    
        return STREAM_OK;
    }

    return STREAM_ERROR;
}

/* Cleans everything up */
void stream_exit(void)
{
    stream_close();

    /* Stop the threads and wait for them to terminate */
    video_thread_exit();
    audio_thread_exit();
    disk_buf_exit();
    pcm_output_exit();

    if (stream_mgr.thread != NULL)
    {
        stream_mgr_post_msg(STREAM_QUIT, 0);
        rb->thread_wait(stream_mgr.thread);
        stream_mgr.thread = NULL;
    }

#ifndef HAVE_LCD_COLOR
    grey_release();
#endif
}
