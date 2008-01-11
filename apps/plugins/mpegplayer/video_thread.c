/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mpegplayer video thread implementation
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
#include "mpeg2dec_config.h"
#include "gray.h"
#include "video_out.h"
#include "mpeg_settings.h"

/** Video stream and thread **/

/* Video thread data passed around to its various functions */
struct video_thread_data
{
    mpeg2dec_t *mpeg2dec;   /* Our video decoder */
    const  mpeg2_info_t *info; /* Info about video stream */
    int    state;           /* Thread state */
    int    status;          /* Media status */
    struct queue_event ev;  /* Our event queue to receive commands */
    int    num_drawn;       /* Number of frames drawn since reset */
    int    num_skipped;     /* Number of frames skipped since reset */
    uint32_t eta_stream;    /* Current time of stream */
    uint32_t eta_video;     /* Time that frame has been scheduled for */
    int32_t eta_early;      /* How early has the frame been decoded? */
    int32_t eta_late;       /* How late has the frame been decoded? */
    int frame_drop_level;   /* Drop severity */
    int skip_level;         /* Skip severity */
    long last_showfps;      /* Last time the FPS display was updated */
    long last_render;       /* Last time a frame was drawn */
    uint32_t curr_time;     /* Current due time of frame */
    uint32_t period;        /* Frame period in clock ticks */
    int      syncf_perfect; /* Last sync fit result */
};

/* TODO: Check if 4KB is appropriate - it works for my test streams,
   so maybe we can reduce it. */
#define VIDEO_STACKSIZE (4*1024)
static uint32_t video_stack[VIDEO_STACKSIZE / sizeof(uint32_t)] IBSS_ATTR;
static struct event_queue video_str_queue NOCACHEBSS_ATTR;
static struct queue_sender_list video_str_queue_send NOCACHEBSS_ATTR;
struct stream video_str IBSS_ATTR;

static void draw_fps(struct video_thread_data *td)
{
    uint32_t start;
    uint32_t clock_ticks = stream_get_ticks(&start);
    int fps = 0;
    char str[80];

    clock_ticks -= start;
    if (clock_ticks != 0)
        fps = muldiv_uint32(CLOCK_RATE*100, td->num_drawn, clock_ticks);

    rb->snprintf(str, sizeof(str), "%d.%02d %d %d    ",
                 fps / 100, fps % 100, td->num_skipped,
                 td->info->display_picture->temporal_reference);
    lcd_(putsxy)(0, 0, str);

    vo_lock();
    lcd_(update_rect)(0, 0, LCD_WIDTH, 8);
    vo_unlock();

    td->last_showfps = *rb->current_tick;
}

#if defined(DEBUG) || defined(SIMULATOR)
static unsigned char pic_coding_type_char(unsigned type)
{
    switch (type)
    {
    case PIC_FLAG_CODING_TYPE_I:
        return 'I'; /* Intra-coded */
    case PIC_FLAG_CODING_TYPE_P:
        return 'P'; /* Forward-predicted */
    case PIC_FLAG_CODING_TYPE_B:
        return 'B'; /* Bidirectionally-predicted */
    case PIC_FLAG_CODING_TYPE_D:
        return 'D'; /* DC-coded */
    default:
        return '?'; /* Say what? */
    }
}
#endif /*  defined(DEBUG) || defined(SIMULATOR) */

/* Multi-use:
 * 1) Find the sequence header and initialize video out
 * 2) Find the end of the final frame
 */
static int video_str_scan(struct video_thread_data *td,
                          struct str_sync_data *sd)
{
    int retval = STREAM_ERROR;
    uint32_t time = INVALID_TIMESTAMP;
    uint32_t period = 0;
    struct stream tmp_str;

    tmp_str.id = video_str.id;
    tmp_str.hdr.pos = sd->sk.pos;
    tmp_str.hdr.limit = sd->sk.pos + sd->sk.len;

    mpeg2_reset(td->mpeg2dec, false);
    mpeg2_skip(td->mpeg2dec, 1);

    while (1)
    {
        mpeg2_state_t mp2state = mpeg2_parse(td->mpeg2dec);
        rb->yield();

        switch (mp2state)
        {
        case STATE_BUFFER:
            switch (parser_get_next_data(&tmp_str, STREAM_PM_RANDOM_ACCESS))
            {
            case STREAM_DATA_END:
                DEBUGF("video_stream_scan:STREAM_DATA_END\n");
                goto scan_finished;

            case STREAM_OK:
                if (tmp_str.pkt_flags & PKT_HAS_TS)
                    mpeg2_tag_picture(td->mpeg2dec, tmp_str.pts, 0);

                mpeg2_buffer(td->mpeg2dec, tmp_str.curr_packet,
                             tmp_str.curr_packet_end);
                td->info = mpeg2_info(td->mpeg2dec);
                break;
            }
            break;

        case STATE_SEQUENCE:
            DEBUGF("video_stream_scan:STATE_SEQUENCE\n");
            vo_setup(td->info->sequence);

            if (td->ev.id == VIDEO_GET_SIZE)
            {
                retval = STREAM_OK;
                goto scan_finished;
            }
            break;

        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
        {
            if (td->info->display_picture == NULL)
                break;

            switch (td->ev.id)
            {
            case STREAM_SYNC:
                retval = STREAM_OK;
                goto scan_finished;

            case STREAM_FIND_END_TIME:
                if (td->info->display_picture->flags & PIC_FLAG_TAGS)
                    time = td->info->display_picture->tag;
                else if (time != INVALID_TIMESTAMP)
                    time += period;

                period = TC_TO_TS(td->info->sequence->frame_period);
                break;
            }

            break;
            }

        default:
            break;
        }
    }

scan_finished:

    if (td->ev.id == STREAM_FIND_END_TIME)
    {
        if (time != INVALID_TIMESTAMP)
        {
            sd->time = time + period;
            retval = STREAM_PERFECT_MATCH;
        }
        else
        {
            retval = STREAM_NOT_FOUND;
        }
    }

    mpeg2_skip(td->mpeg2dec, 0);
    return retval;
}

static bool init_sequence(struct video_thread_data *td)
{
    struct str_sync_data sd;

    sd.time = 0; /* Ignored */
    sd.sk.pos = 0;
    sd.sk.len = 1024*1024;
    sd.sk.dir = SSCAN_FORWARD;

    return video_str_scan(td, &sd) == STREAM_OK;
}

static bool check_needs_sync(struct video_thread_data *td, uint32_t time)
{
    uint32_t end_time;

    DEBUGF("check_needs_sync:\n");
    if (td->info == NULL || td->info->display_fbuf == NULL)
    {
        DEBUGF("  no fbuf\n");
        return true;
    }

    if (td->syncf_perfect == 0)
    {
        DEBUGF("  no frame\n");
        return true;
    }

    time = clip_time(&video_str, time);
    end_time = td->curr_time + td->period;

    DEBUGF("  sft:%u t:%u sfte:%u\n", (unsigned)td->curr_time,
           (unsigned)time, (unsigned)end_time);

    if (time < td->curr_time)
        return true;

    if (time >= end_time)
        return time < video_str.end_pts || end_time < video_str.end_pts;

    return false;
}

/* Do any needed decoding/slide up to the specified time */
static int sync_decoder(struct video_thread_data *td,
                        struct str_sync_data *sd)
{
    int retval = STREAM_ERROR;
    int ipic = 0, ppic = 0;
    uint32_t time = clip_time(&video_str, sd->time);

    td->syncf_perfect = 0;
    td->curr_time = 0;
    td->period = 0;

    /* Sometimes theres no sequence headers nearby and libmpeg2 may have reset
     * fully at some point */
    if ((td->info == NULL || td->info->sequence == NULL) && !init_sequence(td))
    {
        DEBUGF("sync_decoder=>init_sequence failed\n");
        goto sync_finished;
    }

    video_str.hdr.pos = sd->sk.pos;
    video_str.hdr.limit = sd->sk.pos + sd->sk.len;
    mpeg2_reset(td->mpeg2dec, false);
    mpeg2_skip(td->mpeg2dec, 1);

    while (1)
    {
        mpeg2_state_t mp2state = mpeg2_parse(td->mpeg2dec);
        rb->yield();

        switch (mp2state)
        {
        case STATE_BUFFER:
            switch (parser_get_next_data(&video_str, STREAM_PM_RANDOM_ACCESS))
            {
            case STREAM_DATA_END:
                DEBUGF("sync_decoder:STR_DATA_END\n");
                if (td->info && td->info->display_picture &&
                    !(td->info->display_picture->flags & PIC_FLAG_SKIP))
                {
                    /* No frame matching the time was found up to the end of
                     * the stream - consider a perfect match since no better
                     * can be made */
                    retval = STREAM_PERFECT_MATCH;
                    td->syncf_perfect = 1;
                }
                goto sync_finished;

            case STREAM_OK:
                if (video_str.pkt_flags & PKT_HAS_TS)
                    mpeg2_tag_picture(td->mpeg2dec, video_str.pts, 0);

                mpeg2_buffer(td->mpeg2dec, video_str.curr_packet,
                              video_str.curr_packet_end);
                td->info = mpeg2_info(td->mpeg2dec);
                break;
            }
            break;

        case STATE_SEQUENCE:
            DEBUGF("  STATE_SEQUENCE\n");
            vo_setup(td->info->sequence);
            break;

        case STATE_GOP:
            DEBUGF("  STATE_GOP: (%s)\n",
                   (td->info->gop->flags & GOP_FLAG_CLOSED_GOP) ?
                    "closed" : "open");
            break;

        case STATE_PICTURE:
        {
            int type = td->info->current_picture->flags
                        & PIC_MASK_CODING_TYPE;

            switch (type)
            {
            case PIC_FLAG_CODING_TYPE_I:
                /* I-frame; start decoding */
                mpeg2_skip(td->mpeg2dec, 0);
                ipic = 1;
                break;
            case PIC_FLAG_CODING_TYPE_P:
                /* P-frames don't count without I-frames */
                ppic = ipic;
                break;
            }

            if (td->info->current_picture->flags & PIC_FLAG_TAGS)
            {
                DEBUGF("  STATE_PICTURE (%c): %u\n", pic_coding_type_char(type),
                        (unsigned)td->info->current_picture->tag);
            }
            else
            {
                DEBUGF("  STATE_PICTURE (%c): -\n", pic_coding_type_char(type));
            }
            
            break;
            }

        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
        {
            uint32_t end_time;

            if (td->info->display_picture == NULL)
            {
                DEBUGF("  td->info->display_picture == NULL\n");
                break; /* No picture */
            }

            int type = td->info->display_picture->flags
                        & PIC_MASK_CODING_TYPE;

            if (td->info->display_picture->flags & PIC_FLAG_TAGS)
            {
                td->curr_time = td->info->display_picture->tag;
                DEBUGF("  frame tagged:%u (%c%s)\n", (unsigned)td->curr_time,
                       pic_coding_type_char(type),
                       (td->info->display_picture->flags & PIC_FLAG_SKIP) ?
                            " skipped" : "");
            }
            else
            {
                td->curr_time += td->period;
                DEBUGF("  add period:%u (%c%s)\n", (unsigned)td->curr_time,
                       pic_coding_type_char(type),
                       (td->info->display_picture->flags & PIC_FLAG_SKIP) ?
                            " skipped" : "");
            }

            td->period = TC_TO_TS(td->info->sequence->frame_period);
            end_time = td->curr_time + td->period;

            DEBUGF("  ft:%u t:%u fe:%u (%c%s)",
                   (unsigned)td->curr_time,
                   (unsigned)time,
                   (unsigned)end_time,
                   pic_coding_type_char(type),
                   (td->info->display_picture->flags & PIC_FLAG_SKIP) ?
                        " skipped" : "");

            if (end_time <= time && end_time < video_str.end_pts)
            {
                /* Still too early and have not hit at EOS */
                DEBUGF(" too early\n");
                break;
            }
            else if (!(td->info->display_picture->flags & PIC_FLAG_SKIP))
            {
                /* One perfect point if dependent frames were decoded */
                td->syncf_perfect = ipic;

                if (type == PIC_FLAG_CODING_TYPE_B)
                   td->syncf_perfect &= ppic;

                if ((td->curr_time <= time && time < end_time) ||
                    end_time >= video_str.end_pts)
                {
                    /* One perfect point for matching time goal */
                    DEBUGF(" ft<=t<fe\n");
                    td->syncf_perfect++;
                }
                else
                {
                    DEBUGF(" ft>t\n");
                }

                /* Two or more perfect points = perfect match - yay! */
                retval = (td->syncf_perfect >= 2) ?
                    STREAM_PERFECT_MATCH : STREAM_MATCH;
            }
            else
            {
                /* Too late, no I-Frame yet */
                DEBUGF("\n");
            }

            goto sync_finished;
            }

        default:
            break;
        }

        rb->yield();
    } /* end while */

sync_finished:
    mpeg2_skip(td->mpeg2dec, 0);
    return retval;
}

/* This only returns to play or quit */
static void video_thread_msg(struct video_thread_data *td)
{
    while (1)
    {
        intptr_t reply = 0;

        switch (td->ev.id)
        {
        case STREAM_PLAY:
            td->status = STREAM_PLAYING;

            switch (td->state)
            {
            case TSTATE_RENDER_WAIT:
                /* Settings may have changed to nonlimited - just draw
                 * what was previously being waited for */
                if (!settings.limitfps)
                    td->state = TSTATE_RENDER;
            case TSTATE_DECODE:
            case TSTATE_RENDER:
                break;

            case TSTATE_INIT:
                /* Begin decoding state */
                td->state = TSTATE_DECODE;
                break;

            case TSTATE_EOS:
                /* At end of stream - no playback possible so fire the
                 * completion event */
                stream_generate_event(&video_str, STREAM_EV_COMPLETE, 0);
                break;
            }

            reply = td->state != TSTATE_EOS;
            break;

        case STREAM_PAUSE:
            td->status = STREAM_PAUSED;
            reply = td->state != TSTATE_EOS;
            break;

        case STREAM_STOP:
            if (td->state == TSTATE_DATA)
                stream_clear_notify(&video_str, DISK_BUF_DATA_NOTIFY);

            td->status = STREAM_STOPPED;
            td->state = TSTATE_EOS;
            reply = true;
            break;

        case VIDEO_DISPLAY_IS_VISIBLE:
            reply = vo_is_visible();
            break;

        case VIDEO_DISPLAY_SHOW:
            /* Show video and draw the last frame we had if any or reveal the
             * underlying framebuffer if hiding */
            reply = vo_show(!!td->ev.data);

#ifdef HAVE_LCD_COLOR
            /* Match graylib behavior as much as possible */
            if (!td->ev.data == !reply)
                break;

            if (td->ev.data)
            {
                if (td->info != NULL && td->info->display_fbuf != NULL)
                    vo_draw_frame(td->info->display_fbuf->buf);
            }
            else
            {
                IF_COP(invalidate_icache());
                vo_lock();
                rb->lcd_update();
                vo_unlock();
            }
#endif
            break;

        case STREAM_RESET:
            if (td->state == TSTATE_DATA)
                stream_clear_notify(&video_str, DISK_BUF_DATA_NOTIFY);

            td->state = TSTATE_INIT;
            td->status = STREAM_STOPPED;

            /* Reset operational info but not sync info */
            td->eta_stream = UINT32_MAX;
            td->eta_video = 0;
            td->eta_early = 0;
            td->eta_late = 0;
            td->frame_drop_level = 0;
            td->skip_level = 0;
            td->num_drawn = 0;
            td->num_skipped = 0;
            td->last_showfps = *rb->current_tick - HZ;
            td->last_render = td->last_showfps;

            reply = true;
            break;

        case STREAM_NEEDS_SYNC:
            reply = check_needs_sync(td, td->ev.data);
            break;

        case STREAM_SYNC:
            if (td->state == TSTATE_INIT)
                reply = sync_decoder(td, (struct str_sync_data *)td->ev.data);
            break;

        case DISK_BUF_DATA_NOTIFY:
            /* Our bun is done */
            if (td->state != TSTATE_DATA)
                break;

            td->state = TSTATE_DECODE;
            str_data_notify_received(&video_str);
            break;

        case VIDEO_PRINT_THUMBNAIL:
            /* Print a thumbnail of whatever was last decoded - scale and
             * position to fill the specified rectangle */
            if (td->info != NULL && td->info->display_fbuf != NULL)
            {
                vo_draw_frame_thumb(td->info->display_fbuf->buf,
                                    (struct vo_rect *)td->ev.data);
                reply = true;
            }
            break;

        case VIDEO_SET_CLIP_RECT:
            vo_set_clip_rect((const struct vo_rect *)td->ev.data);
            break;

        case VIDEO_PRINT_FRAME:
            /* Print the last frame decoded */
            if (td->info != NULL && td->info->display_fbuf != NULL)
            {
                vo_draw_frame(td->info->display_fbuf->buf);
                reply = true;
            }
            break;

        case VIDEO_GET_SIZE:
        {
            if (td->state != TSTATE_INIT)
                break;

            if (init_sequence(td))
            {
                reply = true;
                vo_dimensions((struct vo_ext *)td->ev.data);
            }
            break;
            }

        case STREAM_FIND_END_TIME:
            if (td->state != TSTATE_INIT)
            {
                reply = STREAM_ERROR;
                break;
            }

            reply = video_str_scan(td, (struct str_sync_data *)td->ev.data);
            break;

        case STREAM_QUIT:
            /* Time to go - make thread exit */
            td->state = TSTATE_EOS;
            return;
        }

        str_reply_msg(&video_str, reply);

        if (td->status == STREAM_PLAYING)
        {
            switch (td->state)
            {
            case TSTATE_DECODE:
            case TSTATE_RENDER:
            case TSTATE_RENDER_WAIT:
                /* These return when in playing state */
                return;
            }
        }

        str_get_msg(&video_str, &td->ev);
    }
}

static void video_thread(void)
{
    struct video_thread_data td;

    td.status = STREAM_STOPPED;
    td.state = TSTATE_EOS;
    td.mpeg2dec = mpeg2_init();
    td.info = NULL;
    td.syncf_perfect = 0;
    td.curr_time = 0;
    td.period = 0;

    if (td.mpeg2dec == NULL)
    {
        td.status = STREAM_ERROR;
        /* Loop and wait for quit message */
        while (1)
        {
            str_get_msg(&video_str, &td.ev);
            if (td.ev.id == STREAM_QUIT)
                return;
            str_reply_msg(&video_str, STREAM_ERROR);
        }
    }

    vo_init();

    goto message_wait;

    while (1)
    {
        mpeg2_state_t mp2state;
        td.state = TSTATE_DECODE;

        /* Check for any pending messages and process them */
        if (str_have_msg(&video_str))
        {
        message_wait:
            /* Wait for a message to be queued */
            str_get_msg(&video_str, &td.ev);

        message_process:
            /* Process a message already dequeued */
            video_thread_msg(&td);

            switch (td.state)
            {
            /* These states are the only ones that should return */
            case TSTATE_DECODE:      goto picture_decode;
            case TSTATE_RENDER:      goto picture_draw;
            case TSTATE_RENDER_WAIT: goto picture_wait;
            /* Anything else is interpreted as an exit */
            default:
                vo_cleanup();
                mpeg2_close(td.mpeg2dec);
                return;
            }
        }

    picture_decode:
        mp2state = mpeg2_parse (td.mpeg2dec);
        rb->yield();

        switch (mp2state)
        {
        case STATE_BUFFER:
            /* Request next packet data */
            switch (parser_get_next_data(&video_str, STREAM_PM_STREAMING))
            {
            case STREAM_DATA_NOT_READY:
                /* Wait for data to be buffered */
                td.state = TSTATE_DATA;
                goto message_wait;

            case STREAM_DATA_END:
                /* No more data. */
                td.state = TSTATE_EOS;
                if (td.status == STREAM_PLAYING)
                    stream_generate_event(&video_str, STREAM_EV_COMPLETE, 0);
                goto message_wait;

            case STREAM_OK:
                if (video_str.pkt_flags & PKT_HAS_TS)
                    mpeg2_tag_picture(td.mpeg2dec, video_str.pts, 0);

                mpeg2_buffer(td.mpeg2dec, video_str.curr_packet,
                              video_str.curr_packet_end);
                td.info = mpeg2_info(td.mpeg2dec);
                break;
            }
            break;

        case STATE_SEQUENCE:
            /* New video sequence, inform output of any changes */
            vo_setup(td.info->sequence);
            break;

        case STATE_PICTURE:
        {
            int skip = 0; /* Assume no skip */

            if (td.frame_drop_level >= 1 || td.skip_level > 0)
            {
                /* A frame will be dropped in the decoder */

                /* Frame type: I/P/B/D */
                int type = td.info->current_picture->flags
                            & PIC_MASK_CODING_TYPE;

                switch (type)
                {
                case PIC_FLAG_CODING_TYPE_I:
                case PIC_FLAG_CODING_TYPE_D:
                    /* Level 5: Things are extremely late and all frames will
                       be dropped until the next key frame */
                    if (td.frame_drop_level >= 1)
                        td.frame_drop_level = 0; /* Key frame - reset drop level */
                    if (td.skip_level >= 5)
                    {
                        td.frame_drop_level = 1;
                        td.skip_level = 0; /* reset */
                    }
                    break;
                case PIC_FLAG_CODING_TYPE_P:
                    /* Level 4: Things are very late and all frames will be
                       dropped until the next key frame */
                    if (td.skip_level >= 4)
                    {
                        td.frame_drop_level = 1;
                        td.skip_level = 0; /* reset */
                    }
                    break;
                case PIC_FLAG_CODING_TYPE_B:
                    /* We want to drop something, so this B frame won't even
                       be decoded. Drawing can happen on the next frame if so
                       desired. Bring the level down as skips are done. */
                    skip = 1;
                    if (td.skip_level > 0)
                        td.skip_level--;
                }

                skip |= td.frame_drop_level;
            }

            mpeg2_skip(td.mpeg2dec, skip);
            break;  
            }

        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
        {
            int32_t offset;  /* Tick adjustment to keep sync */

            /* draw current picture */
            if (td.info->display_fbuf == NULL)
                break; /* No picture */

            /* Get presentation times in audio samples - quite accurate
               enough - add previous frame duration if not stamped */
            td.curr_time = (td.info->display_picture->flags & PIC_FLAG_TAGS) ?
                td.info->display_picture->tag : (td.curr_time + td.period);

            td.period = TC_TO_TS(td.info->sequence->frame_period);

            /* No limiting => no dropping - draw this frame */
            if (!settings.limitfps)
            {
                goto picture_draw;
            }

            td.eta_video = td.curr_time;
            td.eta_stream = TICKS_TO_TS(stream_get_time());

            /* How early/late are we? > 0 = late, < 0  early */
            offset = td.eta_stream - td.eta_video;

            if (!settings.skipframes)
            {
                /* Make no effort to determine whether this frame should be
                   drawn or not since no action can be taken to correct the
                   situation. We'll just wait if we're early and correct for
                   lateness as much as possible. */
                if (offset < 0)
                    offset = 0;

                td.eta_late = AVERAGE(td.eta_late, offset, 4);
                offset = td.eta_late;

                if ((uint32_t)offset > td.eta_video)
                    offset = td.eta_video;

                td.eta_video -= offset;
                goto picture_wait;
            }

            /** Possibly skip this frame **/

            /* Frameskipping has the following order of preference:
             *
             * Frame Type  Who      Notes/Rationale
             * B           decoder  arbitrarily drop - no decode or draw
             * Any         renderer arbitrarily drop - will be I/D/P
             * P           decoder  must wait for I/D-frame - choppy
             * I/D         decoder  must wait for I/D-frame - choppy
             *
             * If a frame can be drawn and it has been at least 1/2 second,
             * the image will be updated no matter how late it is just to
             * avoid looking stuck.
             */

            /* If we're late, set the eta to play the frame early so
               we may catch up. If early, especially because of a drop,
               mitigate a "snap" by moving back gradually. */
            if (offset >= 0) /* late or on time */
            {
                td.eta_early = 0; /* Not early now :( */

                td.eta_late = AVERAGE(td.eta_late, offset, 4);
                offset = td.eta_late;

                if ((uint32_t)offset > td.eta_video)
                    offset = td.eta_video;

                td.eta_video -= offset;
            }
            else
            {
                td.eta_late = 0; /* Not late now :) */

                if (offset > td.eta_early)
                {
                    /* Just dropped a frame and we're now early or we're
                       coming back from being early */
                    td.eta_early = offset;
                    if ((uint32_t)-offset > td.eta_video)
                        offset = -td.eta_video;

                    td.eta_video += offset;
                }
                else
                {
                    /* Just early with an offset, do exponential drift back */
                    if (td.eta_early != 0)
                    {
                        td.eta_early = AVERAGE(td.eta_early, 0, 8);
                        td.eta_video = ((uint32_t)-td.eta_early > td.eta_video) ?
                            0 : (td.eta_video + td.eta_early);
                    }

                    offset = td.eta_early;
                }
            }

            if (td.info->display_picture->flags & PIC_FLAG_SKIP)
            {
                /* This frame was set to skip so skip it after having updated
                   timing information */
                td.num_skipped++;
                td.eta_early = INT32_MIN;
                goto picture_skip;
            }

            if (td.skip_level == 3 &&
                TIME_BEFORE(*rb->current_tick, td.last_render + HZ/2))
            {
                /* Render drop was set previously but nothing was dropped in the
                   decoder or it's been to long since drawing the last frame. */
                td.skip_level = 0;
                td.num_skipped++;
                td.eta_early = INT32_MIN;
                goto picture_skip;
            }

            /* At this point a frame _will_ be drawn  - a skip may happen on
               the next however */
            td.skip_level = 0;

            if (offset > TS_SECOND*110/1000)
            {
                /* Decide which skip level is needed in order to catch up */

                /* TODO: Calculate this rather than if...else - this is rather
                   exponential though */
                if (offset > TS_SECOND*367/1000)
                    td.skip_level = 5; /* Decoder skip: I/D */
                if (offset > TS_SECOND*233/1000)
                    td.skip_level = 4; /* Decoder skip: P */
                else if (offset > TS_SECOND*167/1000)
                    td.skip_level = 3; /* Render skip */
                else if (offset > TS_SECOND*133/1000)
                    td.skip_level = 2; /* Decoder skip: B */
                else
                    td.skip_level = 1; /* Decoder skip: B */
            }

        picture_wait:
            td.state = TSTATE_RENDER_WAIT;

            /* Wait until time catches up */
            while (td.eta_video > td.eta_stream)
            {
                /* Watch for messages while waiting for the frame time */
                int32_t eta_remaining = td.eta_video - td.eta_stream;
                if (eta_remaining > TS_SECOND/HZ)
                {
                    /* Several ticks to wait - do some sleeping */
                    int timeout = (eta_remaining - HZ) / (TS_SECOND/HZ);
                    str_get_msg_w_tmo(&video_str, &td.ev, MAX(timeout, 1));
                    if (td.ev.id != SYS_TIMEOUT)
                        goto message_process;
                }
                else
                {
                    /* Just a little left - spin and be accurate */
                    rb->priority_yield();
                    if (str_have_msg(&video_str))
                        goto message_wait;
                }

                td.eta_stream = TICKS_TO_TS(stream_get_time());
            }
       
        picture_draw:
            /* Record last frame time */
            td.last_render = *rb->current_tick;
            vo_draw_frame(td.info->display_fbuf->buf);
            td.num_drawn++;

        picture_skip:
            if (!settings.showfps)
                break;

            if (TIME_BEFORE(*rb->current_tick, td.last_showfps + HZ))
                break;

            /* Calculate and display fps */
            draw_fps(&td);
            break;
            }

        default:
            break;
        }

        rb->yield();
    } /* end while */
}

/* Initializes the video thread */
bool video_thread_init(void)
{
    intptr_t rep;

    IF_COP(flush_icache());

    video_str.hdr.q = &video_str_queue;
    rb->queue_init(video_str.hdr.q, false);
    rb->queue_enable_queue_send(video_str.hdr.q, &video_str_queue_send);

    /* We put the video thread on another processor for multi-core targets. */
    video_str.thread = rb->create_thread(
        video_thread, video_stack, VIDEO_STACKSIZE, 0,
        "mpgvideo" IF_PRIO(,PRIORITY_PLAYBACK) IF_COP(, COP));

    if (video_str.thread == NULL)
        return false;

    /* Wait for thread to initialize */
    rep = str_send_msg(&video_str, STREAM_NULL, 0);
    IF_COP(invalidate_icache());

    return rep == 0; /* Normally STREAM_NULL should be ignored */
}

/* Terminates the video thread */
void video_thread_exit(void)
{
    if (video_str.thread != NULL)
    {
        str_post_msg(&video_str, STREAM_QUIT, 0);
        rb->thread_wait(video_str.thread);
        IF_COP(invalidate_icache());
        video_str.thread = NULL;
    }
}
