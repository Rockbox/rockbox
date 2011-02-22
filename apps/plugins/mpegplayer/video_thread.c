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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "mpegplayer.h"
#include "libmpeg2/mpeg2dec_config.h"
#include "lib/grey.h"
#include "video_out.h"
#include "mpeg_settings.h"

/** Video stream and thread **/

/* Video thread data passed around to its various functions */
struct video_thread_data
{
    /* Stream data */
    mpeg2dec_t *mpeg2dec;   /* Our video decoder */
    const mpeg2_info_t *info; /* Info about video stream */
    int      state;         /* Thread state */
    int      status;        /* Media status */
    struct   queue_event ev;/* Our event queue to receive commands */
    /* Operational info */
    uint32_t stream_time;   /* Current time from beginning of stream */
    uint32_t goal_time;     /* Scheduled time of current frame */
    int32_t  remain_time;   /* T-minus value to frame_time (-:early, +:late) */
    int      skip_ref_pics; /* Severe skipping - wait for I-frame */
    int      skip_level;    /* Number of frames still to skip */
    int      num_picture;   /* Number of picture headers read */
    int      num_intra;     /* Number of I-picture headers read */
    int      group_est;     /* Estmated number remaining as of last I */
    long     last_render;   /* Last time a frame was drawn */
    /* Sync info */
    uint32_t frame_time;    /* Current due time of frame (unadjusted) */
    uint32_t frame_period;  /* Frame period in clock ticks */
    int      num_ref_pics;  /* Number of I and P frames since sync/skip */
    int      syncf_perfect; /* Last sync fit result */
};

/* Number drawn since reset */
static int video_num_drawn SHAREDBSS_ATTR;
/* Number skipped since reset */
static int video_num_skipped SHAREDBSS_ATTR;

/* TODO: Check if 4KB is appropriate - it works for my test streams,
   so maybe we can reduce it. */
#define VIDEO_STACKSIZE (4*1024)
static uint32_t video_stack[VIDEO_STACKSIZE / sizeof(uint32_t)] IBSS_ATTR;
static struct event_queue video_str_queue SHAREDBSS_ATTR;
static struct queue_sender_list video_str_queue_send SHAREDBSS_ATTR;
struct stream video_str IBSS_ATTR;

#define DEFAULT_GOP_SIZE    INT_MAX /* no I/P skips until it learns */
#define DROP_THRESHOLD      (100*TS_SECOND/1000)
#define MAX_EARLINESS       (120*TS_SECOND/1000)

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

    /* Fully reset if obtaining size for a new stream */
    mpeg2_reset(td->mpeg2dec, td->ev.id == VIDEO_GET_SIZE);
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
    end_time = td->frame_time + td->frame_period;

    DEBUGF("  sft:%u t:%u sfte:%u\n", (unsigned)td->frame_time,
           (unsigned)time, (unsigned)end_time);

    if (time < td->frame_time)
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
    uint32_t time = clip_time(&video_str, sd->time);

    td->syncf_perfect = 0;
    td->frame_time = 0;
    td->frame_period = 0;
    td->num_ref_pics = 0;

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
                td->num_ref_pics++;
                break;

            case PIC_FLAG_CODING_TYPE_P:
                /* P-frames don't count without I-frames */
                if (td->num_ref_pics > 0)
                    td->num_ref_pics++;
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
                td->frame_time = td->info->display_picture->tag;
                DEBUGF("  frame tagged:%u (%c%s)\n", (unsigned)td->frame_time,
                       pic_coding_type_char(type),
                       (td->info->display_picture->flags & PIC_FLAG_SKIP) ?
                            " skipped" : "");
            }
            else
            {
                td->frame_time += td->frame_period;
                DEBUGF("  add frame_period:%u (%c%s)\n", (unsigned)td->frame_time,
                       pic_coding_type_char(type),
                       (td->info->display_picture->flags & PIC_FLAG_SKIP) ?
                            " skipped" : "");
            }

            td->frame_period = TC_TO_TS(td->info->sequence->frame_period);
            end_time = td->frame_time + td->frame_period;

            DEBUGF("  ft:%u t:%u fe:%u (%c%s)",
                   (unsigned)td->frame_time,
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
                switch (type)
                {
                case PIC_FLAG_CODING_TYPE_B:
                    if (td->num_ref_pics > 1)
                    {
                case PIC_FLAG_CODING_TYPE_P:
                        if (td->num_ref_pics > 0)
                        {
                case PIC_FLAG_CODING_TYPE_I:
                            td->syncf_perfect = 1;
                            break;
                        }
                    }
                }

                if ((td->frame_time <= time && time < end_time) ||
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

static bool frame_print_handler(struct video_thread_data *td)
{
    bool retval;
    uint8_t * const * buf = NULL;

    if (td->info != NULL && td->info->display_fbuf != NULL &&
        td->syncf_perfect > 0)
        buf = td->info->display_fbuf->buf;

    if (td->ev.id == VIDEO_PRINT_THUMBNAIL)
    {
        /* Print a thumbnail of whatever was last decoded - scale and
         * position to fill the specified rectangle */
        retval = vo_draw_frame_thumb(buf, (struct vo_rect *)td->ev.data);
    }
    else
    {
        /* Print the last frame decoded */
        vo_draw_frame(buf);
        retval = buf != NULL;
    }

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
            case TSTATE_INIT:
                /* Begin decoding state */
                td->state = TSTATE_DECODE;
                /* */
            case TSTATE_DECODE:
                if (td->syncf_perfect <= 0)
                    break;
                /* There should be a frame already, just draw it */
                td->goal_time = td->frame_time;
                td->state = TSTATE_RENDER_WAIT;
                /* */
            case TSTATE_RENDER_WAIT:
                /* Settings may have changed to nonlimited - just draw
                 * what was previously being waited for */
                td->stream_time = TICKS_TO_TS(stream_get_time());
                if (!settings.limitfps)
                    td->state = TSTATE_RENDER;
                /* */
            case TSTATE_RENDER:
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
                frame_print_handler(td);
            }
            else
            {
                IF_COP(rb->cpucache_invalidate());
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
            td->stream_time = UINT32_MAX;
            td->goal_time = 0;
            td->remain_time = 0;
            td->skip_ref_pics = 0;
            td->skip_level = 0;
            td->num_picture = 0;
            td->num_intra = 0;
            td->group_est = DEFAULT_GOP_SIZE;
            td->last_render = *rb->current_tick - HZ;
            video_num_drawn = 0;
            video_num_skipped = 0;

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

        case VIDEO_PRINT_FRAME:
        case VIDEO_PRINT_THUMBNAIL:
            reply = frame_print_handler(td);
            break;

        case VIDEO_SET_CLIP_RECT:
            vo_set_clip_rect((const struct vo_rect *)td->ev.data);
            break;

        case VIDEO_GET_CLIP_RECT:
            reply = vo_get_clip_rect((struct vo_rect *)td->ev.data);
            break;

        case VIDEO_GET_SIZE:
        {
            if (td->state != TSTATE_INIT)
                break; /* Can only use after a reset was issued */

            /* This will reset the decoder in full for this particular event */
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

        case VIDEO_SET_POST_FRAME_CALLBACK:
            vo_set_post_draw_callback((void (*)(void))td->ev.data);
            reply = true;
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

    memset(&td, 0, sizeof (td));
    td.mpeg2dec = mpeg2_init();
    td.status = STREAM_STOPPED;
    td.state = TSTATE_EOS;

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
            default:                 goto video_exit;
            }
        }

    picture_decode:
        mp2state = mpeg2_parse (td.mpeg2dec);

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
            /* This is not in presentation order - do our best anyway */
            int skip = td.skip_ref_pics;

            /* Frame type: I/P/B/D */
            switch (td.info->current_picture->flags & PIC_MASK_CODING_TYPE)
            {
            case PIC_FLAG_CODING_TYPE_I:
                if (++td.num_intra >= 2)
                    td.group_est = td.num_picture / (td.num_intra - 1);

                /* Things are extremely late and all frames will be
                   dropped until the next key frame */
                if (td.skip_level > 0 && td.skip_level >= td.group_est)
                {
                    td.skip_level--;           /* skip frame  */
                    skip = td.skip_ref_pics = 1; /* wait for I-frame */
                    td.num_ref_pics = 0;
                }
                else if (skip != 0)
                {
                    skip = td.skip_ref_pics = 0; /* now, decode */
                    td.num_ref_pics = 1;
                }
                break;

            case PIC_FLAG_CODING_TYPE_P:
                if (skip == 0)
                {
                    td.num_ref_pics++;

                    /* If skip_level at least the estimated number of frames
                       left in I-I span, skip until next I-frame */
                    if (td.group_est > 0 && td.skip_level >= td.group_est)
                    {
                        skip = td.skip_ref_pics = 1; /* wait for I-frame */
                        td.num_ref_pics = 0;
                    }
                }

                if (skip != 0)
                    td.skip_level--; 
                break;

            case PIC_FLAG_CODING_TYPE_B:
                /* We want to drop something, so this B-frame won't even be
                   decoded. Drawing can happen on the next frame if so desired
                   so long as the B-frames were not dependent upon those from
                   a previous open GOP where the needed reference frames were
                   skipped */
                if (td.skip_level > 0 || td.num_ref_pics < 2)
                {
                    skip = 1;
                    td.skip_level--;
                }
                break;

            default:
                skip = 1;
                break;
            }

            if (td.num_intra > 0)
                td.num_picture++;

            td.group_est--;

            mpeg2_skip(td.mpeg2dec, skip);
            break;  
            }

        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
        {
            int32_t offset;  /* Tick adjustment to keep sync */

            if (td.info->display_fbuf == NULL)
                break; /* No picture */

            /* Get presentation times in audio samples - quite accurate
               enough - add previous frame duration if not stamped */
            if (td.info->display_picture->flags & PIC_FLAG_TAGS)
                td.frame_time = td.info->display_picture->tag;
            else
                td.frame_time += td.frame_period;

            td.frame_period = TC_TO_TS(td.info->sequence->frame_period);

            if (!settings.limitfps)
            {
                /* No limiting => no dropping or waiting - draw this frame */
                td.remain_time = 0;
                td.skip_level = 0;
                td.syncf_perfect = 1; /* have frame */
                goto picture_draw;
            }

            td.goal_time = td.frame_time;
            td.stream_time = TICKS_TO_TS(stream_get_time());

            /* How early/late are we? > 0 = late, < 0  early */
            offset = td.stream_time - td.goal_time;

            if (offset >= 0)
            {
                /* Late or on-time */
                if (td.remain_time < 0)
                    td.remain_time = 0;              /* now, late */

                offset = AVERAGE(td.remain_time, offset, 4);
                td.remain_time = offset;
            }
            else
            {
                /* Early */
                if (td.remain_time >= 0)
                    td.remain_time = 0;              /* now, early */
                else if (offset > td.remain_time)
                    td.remain_time = MAX(offset, -MAX_EARLINESS); /* less early */
                else if (td.remain_time != 0)
                    td.remain_time = AVERAGE(td.remain_time, 0, 8); /* earlier/same */
                /* else there's been no frame drop */

                offset = -td.remain_time;
            }

            /* Skip anything not decoded */
            if (td.info->display_picture->flags & PIC_FLAG_SKIP)
                goto picture_skip;

            td.syncf_perfect = 1; /* have frame (assume so from now on) */

            /* Keep goal_time >= 0 */
            if ((uint32_t)offset > td.goal_time)
                offset = td.goal_time;

            td.goal_time -= offset;

            if (!settings.skipframes)
            {
                /* No skipping - just wait if we're early and correct for
                   lateness as much as possible. */
                td.skip_level = 0;
                goto picture_wait;
            }

            /** Possibly skip this frame **/

            /* Frameskipping has the following order of preference:
             *
             * Frame Type  Who      Notes/Rationale
             * B           decoder  arbitrarily drop - no decode or draw
             * Any         renderer arbitrarily drop - I/P unless B decoded
             * P           decoder  must wait for I-frame
             * I           decoder  must wait for I-frame
             *
             * If a frame can be drawn and it has been at least 1/2 second,
             * the image will be updated no matter how late it is just to
             * avoid looking stuck.
             */
            if (td.skip_level > 0 &&
                TIME_BEFORE(*rb->current_tick, td.last_render + HZ/2))
            {
                /* Frame skip was set previously but either there wasn't anything
                   dropped yet or not dropped enough. So we quit at least rendering 
                   the actual frame to avoid further increase of a/v-drift. */
                td.skip_level--;
                goto picture_skip;
            }

            /* At this point a frame _will_ be drawn  - a skip may happen on
               the next however */

            /* Calculate number of frames to drop/skip - allow brief periods
               of lateness before producing skips */
            td.skip_level = 0;
            if (td.remain_time > 0 && (uint32_t)offset > DROP_THRESHOLD)
            {
                td.skip_level = (offset - DROP_THRESHOLD + td.frame_period)
                                    / td.frame_period;
            }

        picture_wait:
            td.state = TSTATE_RENDER_WAIT;

            /* Wait until time catches up */
            while (1)
            {
                int32_t twait = td.goal_time - td.stream_time;
                /* Watch for messages while waiting for the frame time */

                if (twait <= 0)
                    break;

                if (twait > TS_SECOND/HZ)
                {
                    /* Several ticks to wait - do some sleeping */
                    int timeout = (twait - HZ) / (TS_SECOND/HZ);
                    str_get_msg_w_tmo(&video_str, &td.ev, MAX(timeout, 1));
                    if (td.ev.id != SYS_TIMEOUT)
                        goto message_process;
                }
                else
                {
                    /* Just a little left - spin and be accurate */
                    rb->yield();
                    if (str_have_msg(&video_str))
                        goto message_wait;
                }

                td.stream_time = TICKS_TO_TS(stream_get_time());
            }
       
        picture_draw:
            /* Record last frame time */
            td.last_render = *rb->current_tick;

            vo_draw_frame(td.info->display_fbuf->buf);
            video_num_drawn++;
            break;

        picture_skip:
            if (td.remain_time <= DROP_THRESHOLD)
            {
                td.skip_level = 0;
                if (td.remain_time <= 0)
                    td.remain_time = INT32_MIN;
            }

            video_num_skipped++;
            break;
            }

        default:
            break;
        }

        rb->yield();
    } /* end while */

video_exit:
    vo_cleanup();
    mpeg2_close(td.mpeg2dec);
}

/* Initializes the video thread */
bool video_thread_init(void)
{
    intptr_t rep;

    IF_COP(rb->cpucache_flush());

    video_str.hdr.q = &video_str_queue;
    rb->queue_init(video_str.hdr.q, false);

    /* We put the video thread on another processor for multi-core targets. */
    video_str.thread = rb->create_thread(
        video_thread, video_stack, VIDEO_STACKSIZE, 0,
        "mpgvideo" IF_PRIO(,PRIORITY_PLAYBACK) IF_COP(, COP));

    rb->queue_enable_queue_send(video_str.hdr.q, &video_str_queue_send,
                                video_str.thread);

    if (video_str.thread == 0)
        return false;

    /* Wait for thread to initialize */
    rep = str_send_msg(&video_str, STREAM_NULL, 0);
    IF_COP(rb->cpucache_invalidate());

    return rep == 0; /* Normally STREAM_NULL should be ignored */
}

/* Terminates the video thread */
void video_thread_exit(void)
{
    if (video_str.thread != 0)
    {
        str_post_msg(&video_str, STREAM_QUIT, 0);
        rb->thread_wait(video_str.thread);
        IF_COP(rb->cpucache_invalidate());
        video_str.thread = 0;
    }
}


/** Misc **/
void video_thread_get_stats(struct video_output_stats *s)
{
    uint32_t start;
    uint32_t now = stream_get_ticks(&start);
    s->num_drawn = video_num_drawn;
    s->num_skipped = video_num_skipped;

    s->fps = 0;

    if (now > start)
        s->fps = muldiv_uint32(CLOCK_RATE*100, s->num_drawn, now - start);
}

