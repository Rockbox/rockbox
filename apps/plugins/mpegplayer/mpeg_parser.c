/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Parser for MPEG streams
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

struct stream_parser str_parser SHAREDBSS_ATTR;

static void parser_init_state(void)
{
    str_parser.last_seek_time = 0;
    str_parser.format = STREAM_FMT_UNKNOWN;
    str_parser.start_pts = INVALID_TIMESTAMP;
    str_parser.end_pts = INVALID_TIMESTAMP;
    str_parser.flags = 0;
    str_parser.dims.w = 0;
    str_parser.dims.h = 0;
}

/* Place the stream in a state to begin parsing - sync will be performed
 * first */
void str_initialize(struct stream *str, off_t pos)
{
    /* Initial positions start here */
    str->hdr.win_left = str->hdr.win_right = pos;
    /* No packet */
    str->curr_packet = NULL;
    /* Pick up parsing from this point in the buffer */
    str->curr_packet_end = disk_buf_offset2ptr(pos);
    /* No flags */
    str->pkt_flags = 0;
    /* Sync first */
    str->state = SSTATE_SYNC;
}

/* Place the stream in an end of data state */
void str_end_of_stream(struct stream *str)
{
    /* Offsets that prevent this stream from being included in the
     * min left/max right window so that no buffering is triggered on
     * its behalf. Set right to the min first so a thread reading the
     * overall window gets doesn't see this as valid no matter what the
     * file length. */
    str->hdr.win_right = LONG_MIN;
    str->hdr.win_left = LONG_MAX;
    /* No packets */
    str->curr_packet = str->curr_packet_end = NULL;
    /* No flags */
    str->pkt_flags = 0;
    /* Fin */
    str->state = SSTATE_END;
}

/* Return a timestamp at address p+offset if the marker bits are in tact */
static inline uint32_t read_pts(uint8_t *p, off_t offset)
{
    return TS_CHECK_MARKERS(p, offset) ?
        TS_FROM_HEADER(p, offset) : INVALID_TIMESTAMP;
}

static inline bool validate_timestamp(uint32_t ts)
{
    return ts >= str_parser.start_pts && ts <= str_parser.end_pts;
}

/* Find a start code before or after a given position */
uint8_t * mpeg_parser_scan_start_code(struct stream_scan *sk, uint32_t code)
{
    stream_scan_normalize(sk);

    if (sk->dir < 0)
    {
        /* Reverse scan - start with at least the min needed */
        stream_scan_offset(sk, 4);
    }

    code &= 0xff; /* Only the low byte matters */

    while (sk->len >= 0 && sk->margin >= 4)
    {
        uint8_t *p;
        off_t pos = disk_buf_lseek(sk->pos, SEEK_SET);
        ssize_t len = disk_buf_getbuffer(4, &p, NULL, NULL);

        if (pos < 0 || len < 4)
            break;

        if (CMP_3_CONST(p, PACKET_START_CODE_PREFIX) && p[3] == code)
        {
            return p;
        }

        stream_scan_offset(sk, 1);
    }

    return NULL;
}

/* Find a PES packet header for any stream - return stream to which it
 * belongs */
unsigned mpeg_parser_scan_pes(struct stream_scan *sk)
{
    stream_scan_normalize(sk);

    if (sk->dir < 0)
    {
        /* Reverse scan - start with at least the min needed */
        stream_scan_offset(sk, 4);
    }

    while (sk->len >= 0 && sk->margin >= 4)
    {
        uint8_t *p;
        off_t pos = disk_buf_lseek(sk->pos, SEEK_SET);
        ssize_t len = disk_buf_getbuffer(4, &p, NULL, NULL);

        if (pos < 0 || len < 4)
            break;

        if (CMP_3_CONST(p, PACKET_START_CODE_PREFIX))
        {
            unsigned id = p[3];
            if (id >= 0xb9)
                return id;  /* PES header */
            /* else some video stream element */
        }

        stream_scan_offset(sk, 1);
    }

    return -1;
}

/* Return the first SCR found from the scan direction */
uint32_t mpeg_parser_scan_scr(struct stream_scan *sk)
{
    uint8_t *p = mpeg_parser_scan_start_code(sk, MPEG_STREAM_PACK_HEADER);

    if (p != NULL && sk->margin >= 9) /* 9 bytes total required */
    {
        sk->data = 9;

        if ((p[4] & 0xc0) == 0x40)      /* mpeg-2 */
        {
            /* Lookhead p+8 */
            if (MPEG2_CHECK_PACK_SCR_MARKERS(p, 4))
                return MPEG2_PACK_HEADER_SCR(p, 4);
        }
        else if ((p[4] & 0xf0) == 0x20) /* mpeg-1 */
        {
            /* Lookahead p+8 */
            if (TS_CHECK_MARKERS(p, 4))
                return TS_FROM_HEADER(p, 4);
        }
        /* Weird pack header */
        sk->data = 5;
    }

    return INVALID_TIMESTAMP;
}

uint32_t mpeg_parser_scan_pts(struct stream_scan *sk, unsigned id)
{
    stream_scan_normalize(sk);

    if (sk->dir < 0)
    {
        /* Reverse scan - start with at least the min needed */
        stream_scan_offset(sk, 4);
    }

    while (sk->len >= 0 && sk->margin >= 4)
    {
        uint8_t *p;
        off_t pos = disk_buf_lseek(sk->pos, SEEK_SET);
        ssize_t len = disk_buf_getbuffer(35, &p, NULL, NULL);

        if (pos < 0 || len < 4)
            break;

        if (CMP_3_CONST(p, PACKET_START_CODE_PREFIX) && p[3] == id)
        {
            uint8_t *h = p;

            if (sk->margin < 6)
            {
                /* Insufficient data */
            }
            else if ((h[6] & 0xc0) == 0x80) /* mpeg2 */
            {
                if (sk->margin >= 14 && (h[7] & 0x80) != 0x00)
                {
                    sk->data = 14;
                    return read_pts(h, 9);
                }
            }
            else                            /* mpeg1 */
            {
                ssize_t l = 7;
                ssize_t margin = sk->margin;

                /* Skip stuffing_byte */
                while (h[l - 1] == 0xff && ++l <= 23)
                    --margin;

                if ((h[l - 1] & 0xc0) == 0x40)
                {
                    /* Skip STD_buffer_scale and STD_buffer_size */
                    margin -= 2;
                    l += 2;
                }

                if (margin >= 4)
                {
                    /* header points to the mpeg1 pes header */
                    h += l;

                    if ((h[-1] & 0xe0) == 0x20)
                    {
                        sk->data = (h + 4) - p;
                        return read_pts(h, -1);
                    }
                }
            }
            /* No PTS present - keep searching for a matching PES header with
             * one */
        }

        stream_scan_offset(sk, 1);
    }

    return INVALID_TIMESTAMP;
}

static bool init_video_info(void)
{
    DEBUGF("Getting movie size\n");

    /* The decoder handles this in order to initialize its knowledge of the
     * movie parameters making seeking easier */
    str_send_msg(&video_str, STREAM_RESET, 0);
    if (str_send_msg(&video_str, VIDEO_GET_SIZE,
                     (intptr_t)&str_parser.dims) != 0)
    {
        return true;
    }

    DEBUGF("  failed\n");
    return false;
}

static void init_times(struct stream *str)
{
    int i;
    struct stream tmp_str;
    const ssize_t filesize = disk_buf_filesize();
    const ssize_t max_probe = MIN(512*1024, filesize);

    /* Simply find the first earliest timestamp - this will be the one
     * used when streaming anyway */
    DEBUGF("Finding start_pts: 0x%02x\n", str->id);

    tmp_str.id = str->id;
    tmp_str.hdr.pos = 0;
    tmp_str.hdr.limit = max_probe;

    str->start_pts = INVALID_TIMESTAMP;

    /* Probe many for video because of B-frames */
    for (i = STREAM_IS_VIDEO(str->id) ? 5 : 1; i > 0;)
    {
        switch (parser_get_next_data(&tmp_str, STREAM_PM_RANDOM_ACCESS))
        {
        case STREAM_DATA_END:
            break;
        case STREAM_OK:
            if (tmp_str.pkt_flags & PKT_HAS_TS)
            {
                if (tmp_str.pts < str->start_pts)
                   str->start_pts = tmp_str.pts;
                i--; /* Decrement timestamp counter */
            }
            continue;
        }

        break;
    }

    DEBUGF("  start:%u\n", (unsigned)str->start_pts);

    /* Use the decoder thread to perform a synchronized search - no
     * decoding should take place but just a simple run through timestamps
     * and durations as the decoder would see them. This should give the
     * precise time at the end of the last frame for the stream. */
    DEBUGF("Finding end_pts: 0x%02x\n", str->id);

    str->end_pts = INVALID_TIMESTAMP;

    if (str->start_pts != INVALID_TIMESTAMP)
    {
        str_parser.parms.sd.time = MAX_TIMESTAMP;
        str_parser.parms.sd.sk.pos = filesize - max_probe;
        str_parser.parms.sd.sk.len = max_probe;
        str_parser.parms.sd.sk.dir = SSCAN_FORWARD;

        str_send_msg(str, STREAM_RESET, 0);

        if (str_send_msg(str, STREAM_FIND_END_TIME,
                (intptr_t)&str_parser.parms.sd) == STREAM_PERFECT_MATCH)
        {
            str->end_pts = str_parser.parms.sd.time;
            DEBUGF("  end:%u\n", (unsigned)str->end_pts);
        }
    }

    /* End must be greater than start. If the start PTS is found, the end PTS
     * must be valid too. If the start PTS was invalid, then the end will never
     * be scanned above. */
    if (str->start_pts >= str->end_pts || str->end_pts == INVALID_TIMESTAMP)
    {
        str->start_pts = INVALID_TIMESTAMP;
        str->end_pts = INVALID_TIMESTAMP;
    }
}

/* Return the best-fit file offset of a timestamp in the PES where
 * timstamp <= time < next timestamp. Will try to return something reasonably
 * valid if best-fit could not be made. */
static off_t mpeg_parser_seek_PTS(uint32_t time, unsigned id)
{
    ssize_t pos_left = 0;
    ssize_t pos_right = disk_buf.filesize;
    ssize_t pos, pos_new;
    uint32_t time_left = str_parser.start_pts;
    uint32_t time_right = str_parser.end_pts;
    uint32_t pts = 0;
    uint32_t prevpts = 0;
    enum state_enum state = state0;
    struct stream_scan sk;

    /* Initial estimate taken from average bitrate - later interpolations are
     * taken similarly based on the remaining file interval */
    pos_new = muldiv_uint32(time - time_left, pos_right - pos_left,
                            time_right - time_left) + pos_left;

    /* return this estimated position if nothing better comes up */
    pos = pos_new;

    DEBUGF("Seeking stream 0x%02x\n", id);
    DEBUGF("$$ tl:%u t:%u ct:?? tr:%u\n   pl:%ld pn:%ld pr:%ld\n",
           (unsigned)time_left, (unsigned)time, (unsigned)time_right,
           pos_left, pos_new, pos_right);

    sk.dir = SSCAN_REVERSE;

    while (state < state9)
    {
        uint32_t currpts;
        sk.pos = pos_new;
        sk.len = (sk.dir < 0) ? pos_new - pos_left : pos_right - pos_new;

        currpts = mpeg_parser_scan_pts(&sk, id);

        if (currpts != INVALID_TIMESTAMP)
        {
            ssize_t pos_adj; /* Adjustment to over or under-estimate */

            /* Found a valid timestamp - see were it lies in relation to
             * target */
            if (currpts < time)
            {
                /* Time at current position is before seek time - move
                 * forward */
                if (currpts > pts)
                {
                    /* This is less than the desired time but greater than
                     * the currently seeked one; move the position up */
                    pts = currpts;
                    pos = sk.pos;
                }

                /* No next timestamp can be sooner */
                pos_left = sk.pos + sk.data;
                time_left = currpts;

                if (pos_right <= pos_left)
                    break; /* If the window disappeared - we're done */

                pos_new = muldiv_uint32(time - time_left,
                                        pos_right - pos_left,
                                        time_right - time_left);
                /* Point is ahead of us - fudge estimate a bit high */
                pos_adj = pos_new / 10;

                if (pos_adj > 512*1024)
                    pos_adj = 512*1024;

                pos_new += pos_left + pos_adj;

                if (pos_new >= pos_right)
                {
                    /* Estimate could push too far */
                    pos_new = pos_right;
                }

                state = state2; /* Last scan was early */
                sk.dir = SSCAN_REVERSE;
   
                DEBUGF(">> tl:%u t:%u ct:%u tr:%u\n   pl:%ld pn:%ld pr:%ld\n",
                       (unsigned)time_left, (unsigned)time, (unsigned)currpts,
                       (unsigned)time_right, pos_left, pos_new, pos_right);
            }
            else if (currpts > time)
            {
                /* Time at current position is past seek time - move
                   backward */
                pos_right = sk.pos;
                time_right = currpts;

                if (pos_right <= pos_left)
                    break; /* If the window disappeared - we're done */

                pos_new = muldiv_uint32(time - time_left,
                                        pos_right - pos_left,
                                        time_right - time_left);
                /* Overshot the seek point - fudge estimate a bit low */
                pos_adj = pos_new / 10;

                if (pos_adj > 512*1024)
                    pos_adj = 512*1024;

                pos_new += pos_left - pos_adj;

                state = state3; /* Last scan was late */
                sk.dir = SSCAN_REVERSE;

                DEBUGF("<< tl:%u t:%u ct:%u tr:%u\n   pl:%ld pn:%ld pr:%ld\n",
                       (unsigned)time_left, (unsigned)time, (unsigned)currpts,
                       (unsigned)time_right, pos_left, pos_new, pos_right);
            }
            else
            {
                /* Exact match - it happens */
                DEBUGF("|| tl:%u t:%u ct:%u tr:%u\n   pl:%ld pn:%ld pr:%ld\n",
                       (unsigned)time_left, (unsigned)time, (unsigned)currpts,
                       (unsigned)time_right, pos_left, pos_new, pos_right);
                pts = currpts;
                pos = sk.pos;
                state = state9;
            }
        }
        else
        {
            /* Nothing found */

            switch (state)
            {
            case state1:
                /* We already tried the bruteforce scan and failed again - no
                 * more stamps could possibly exist in the interval */
                DEBUGF("!! no timestamp 2x\n");
                break;
            case state0:
                /* Hardly likely except at very beginning - just do L->R scan
                 * to find something */
                DEBUGF("!! no timestamp on first probe: %ld\n", sk.pos);
            case state2:
            case state3:
                /* Could just be missing timestamps because the interval is
                 * narrowing down. A large block of data from another stream
                 * may also be in the midst of our chosen points which could
                 * cluster at either extreme end. If anything is there, this
                 * will find it. */
                pos_new = pos_left;
                sk.dir = SSCAN_FORWARD;
                DEBUGF("?? tl:%u t:%u ct:%u tr:%u\n   pl:%ld pn:%ld pr:%ld\n",
                       (unsigned)time_left, (unsigned)time, (unsigned)currpts,
                       (unsigned)time_right, pos_left, pos_new, pos_right);
                state = state1;
                break;
            default:
                DEBUGF("?? Invalid state: %d\n", state);
            }
        }

        /* Same timestamp twice = quit */
        if (currpts == prevpts)
        {
            DEBUGF("!! currpts == prevpts (stop)\n");
            state = state9;
        }

        prevpts = currpts;
    }

#if defined(DEBUG) || defined(SIMULATOR)
    /* The next pts after the seeked-to position should be greater -
     * most of the time - frames out of presentation order may muck it
     * up a slight bit */
    sk.pos = pos + 1;
    sk.len = disk_buf.filesize;
    sk.dir = SSCAN_FORWARD;

    uint32_t nextpts = mpeg_parser_scan_pts(&sk, id);
    DEBUGF("Seek pos:%ld pts:%u t:%u next pts:%u \n",
        pos, (unsigned)pts, (unsigned)time, (unsigned)nextpts);

    if (pts <= time && time < nextpts)
    {
        /* Smile - it worked */
        DEBUGF("  :) pts<=time<next pts\n");
    }
    else
    {
        /* See where things ended up */
        if (pts > time)
        {
            /* Hmm */
            DEBUGF("  :\\ pts>time\n");
        }
        if (pts >= nextpts)
        {
            /* Weird - probably because of encoded order & tends to be right
             * anyway if other criteria are met */
            DEBUGF("  :p pts>=next pts\n");
        }
        if (time >= nextpts)
        {
            /* Ugh */
            DEBUGF("  :( time>=nextpts\n");
        }
    }
#endif

    return pos;
}

static bool prepare_image(uint32_t time)
{
    struct stream_scan sk;
    int tries;
    int result;

    if (!str_send_msg(&video_str, STREAM_NEEDS_SYNC, time))
    {
        DEBUGF("Image was ready\n");
        return true; /* Should already have the image */
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true); /* No interference with trigger_cpu_boost */
#endif

    str_send_msg(&video_str, STREAM_RESET, 0);

    sk.pos = parser_can_seek() ?
                mpeg_parser_seek_PTS(time, video_str.id) : 0;
    sk.len = sk.pos;
    sk.dir = SSCAN_REVERSE;

    tries = 1;
try_again:

    if (mpeg_parser_scan_start_code(&sk, MPEG_START_GOP))
    {
        DEBUGF("GOP found at: %ld\n", sk.pos);

        unsigned id = mpeg_parser_scan_pes(&sk);

        if (id != video_str.id && sk.pos > 0)
        {
            /* Not part of our stream */
            DEBUGF("  wrong stream: 0x%02x\n", id);
            goto try_again;
        }

        /* This will hit the PES header since it's known to be there */
        uint32_t pts = mpeg_parser_scan_pts(&sk, id);

        if (pts == INVALID_TIMESTAMP || pts > time)
        {
            DEBUGF("  wrong timestamp: %u\n", (unsigned)pts);
            goto try_again;
        }
    }

    str_parser.parms.sd.time = time;
    str_parser.parms.sd.sk.pos = MAX(sk.pos, 0);
    str_parser.parms.sd.sk.len = 1024*1024;
    str_parser.parms.sd.sk.dir = SSCAN_FORWARD;

    DEBUGF("thumb pos:%ld len:%ld\n", str_parser.parms.sd.sk.pos,
           str_parser.parms.sd.sk.len);

    result = str_send_msg(&video_str, STREAM_SYNC,
                          (intptr_t)&str_parser.parms.sd);

    if (result != STREAM_PERFECT_MATCH)
    {
        /* Two tries should be all that is nescessary to find the exact frame
         * if the first GOP actually started later than the timestamp - the
         * GOP just prior must then start on or earlier. */
        if (++tries <= 2)
            goto try_again;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    return result > STREAM_OK;
}

static void prepare_audio(uint32_t time)
{
    off_t pos;

    if (!str_send_msg(&audio_str, STREAM_NEEDS_SYNC, time))
    {
        DEBUGF("Audio was ready\n");
        return;
    }

    pos = mpeg_parser_seek_PTS(time, audio_str.id);
    str_send_msg(&audio_str, STREAM_RESET, 0);

    str_parser.parms.sd.time = time;
    str_parser.parms.sd.sk.pos = pos;
    str_parser.parms.sd.sk.len = 1024*1024;
    str_parser.parms.sd.sk.dir = SSCAN_FORWARD;

    str_send_msg(&audio_str, STREAM_SYNC, (intptr_t)&str_parser.parms.sd);
}

/* This function demuxes the streams and gives the next stream data
 * pointer.
 *
 * STREAM_PM_STREAMING is for operation during playback. If the nescessary
 * data and worst-case lookahead margin is not available, the stream is
 * registered for notification when the data becomes available. If parsing
 * extends beyond the end of the file or the end of stream marker is reached,
 * STREAM_DATA_END is returned and the stream state changed to SSTATE_EOS.
 *
 * STREAM_PM_RANDOM_ACCESS is for operation when not playing such as seeking.
 * If the file cache misses for the current position + lookahead, it will be
 * loaded from disk. When the specified limit is reached, STREAM_DATA_END is
 * returned.
 *
 * The results from one mode may be used as input to the other. Random access
 * requires cooperation amongst threads to avoid evicting another stream's
 * data.
 */
static int parse_demux(struct stream *str, enum stream_parse_mode type)
{
    #define INC_BUF(offset) \
        ({ off_t _o = (offset);           \
           str->hdr.win_right += _o;      \
           if ((p += _o) >= disk_buf.end) \
                p -= disk_buf.size; })

    static const int mpeg1_skip_table[16] =
        { 0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    uint8_t *p = str->curr_packet_end;

    str->pkt_flags = 0;

    while (1)
    {
        uint8_t *header;
        unsigned id;
        ssize_t length, bytes;

        switch (type)
        {
        case STREAM_PM_STREAMING:
            /* Has the end been reached already? */
            switch (str->state)
            {
            case SSTATE_PARSE: /* Expected case first if no jumptable */
                /* Are we at the end of file? */
                if (str->hdr.win_left < disk_buf.filesize)
                    break;
                str_end_of_stream(str);
                return STREAM_DATA_END;

            case SSTATE_SYNC:
                /* Is sync at the end of file? */
                if (str->hdr.win_right < disk_buf.filesize)
                    break;
                str_end_of_stream(str);
                /* Fall-through */
            case SSTATE_END:
                return STREAM_DATA_END;
            }

            if (!disk_buf_is_data_ready(&str->hdr, MIN_BUFAHEAD))
            {
                /* This data range is not buffered yet - register stream to
                 * be notified when it becomes available. Stream is obliged
                 * to enter a TSTATE_DATA state if it must wait. */
                int res = str_next_data_not_ready(str);

                if (res != STREAM_OK)
                    return res;
            }
            break;
            /* STREAM_PM_STREAMING: */

        case STREAM_PM_RANDOM_ACCESS:
            str->hdr.pos = disk_buf_lseek(str->hdr.pos, SEEK_SET);

            if (str->hdr.pos < 0 || str->hdr.pos >= str->hdr.limit ||
                disk_buf_getbuffer(MIN_BUFAHEAD, &p, NULL, NULL) <= 0)
            {
                str_end_of_stream(str);
                return STREAM_DATA_END;
            }

            str->state = SSTATE_SYNC;
            str->hdr.win_left = str->hdr.pos;
            str->curr_packet = NULL;
            str->curr_packet_end = p;
            break;
            /* STREAM_PM_RANDOM_ACCESS: */
        }

        if (str->state == SSTATE_SYNC)
        {
            /* Scanning for start code */
            if (!CMP_3_CONST(p, PACKET_START_CODE_PREFIX))
            {
                INC_BUF(1);
                continue;
            }
        }

        /* Found a start code - enter parse state */
        str->state = SSTATE_PARSE;

        /* Pack header, skip it */
        if (CMP_4_CONST(p, PACK_START_CODE))
        {
            /* Max lookahead: 14 */
            if ((p[4] & 0xc0) == 0x40)      /* mpeg-2 */
            {
                /* Max delta: 14 + 7 = 21 */
                /* Skip pack header and any stuffing bytes*/
                bytes = 14 + (p[13] & 7);
            }
            else if ((p[4] & 0xf0) == 0x20) /* mpeg-1 */
            {
                bytes = 12;
            }
            else                            /* unknown - skip it */
            {
                DEBUGF("weird pack header!\n");
                bytes = 5;
            }

            INC_BUF(bytes);
        }

        /* System header, parse and skip it - 6 bytes + size */
        if (CMP_4_CONST(p, SYSTEM_HEADER_START_CODE))
        {
            /* Skip start code */
            /* Max Delta = 65535 + 6 = 65541 */
            bytes = 6 + ((p[4] << 8) | p[5]);
            INC_BUF(bytes);
        }

        /* Packet header, parse it */
        if (!CMP_3_CONST(p, PACKET_START_CODE_PREFIX))
        {
            /* Problem? Meh...probably not but just a corrupted section.
             * Try to resync the parser which will probably succeed. */
            DEBUGF("packet start code prefix not found: 0x%02x\n"
                   "  wl:%lu wr:%lu\n"
                   "  p:%p cp:%p cpe:%p\n"
                   "  dbs:%p dbe:%p dbt:%p\n",
                   str->id, str->hdr.win_left, str->hdr.win_right,
                   p, str->curr_packet, str->curr_packet_end,
                   disk_buf.start, disk_buf.end, disk_buf.tail);
            str->state = SSTATE_SYNC;
            INC_BUF(1); /* Next byte - this one's no good */
            continue;
        }

        /* We retrieve basic infos */
        /* Maximum packet length: 6 + 65535 = 65541 */
        id = p[3];
        length = ((p[4] << 8) | p[5]) + 6;

        if (id != str->id)
        {
            switch (id)
            {
            case MPEG_STREAM_PROGRAM_END:
                /* end of stream */
                str_end_of_stream(str);
                DEBUGF("MPEG program end: 0x%02x\n", str->id);
                return STREAM_DATA_END;
            case MPEG_STREAM_PACK_HEADER:
            case MPEG_STREAM_SYSTEM_HEADER:
                /* These shouldn't be here - no increment or resync
                 * since we'll pick it up above. */
                continue;
            default:
                /* It's not the packet we're looking for, skip it */
                INC_BUF(length);
                continue;
            }
        }

        /* Ok, it's our packet */
        header = p;

        if ((header[6] & 0xc0) == 0x80) /* mpeg2 */
        {
            /* Max Lookahead: 18         */
            /* Min length: 9             */
            /* Max length: 9 + 255 = 264 */
            length = 9 + header[8];

            /* header points to the mpeg2 pes header */
            if ((header[7] & 0x80) != 0)
            {
                /* header has a pts */
                uint32_t pts = read_pts(header, 9);

                if (pts != INVALID_TIMESTAMP)
                {
                    str->pts = pts;
#if 0
                   /* DTS isn't used for anything since things just get
                      decoded ASAP but keep the code around */
                    if (STREAM_IS_VIDEO(id))
                    {
                        /* Video stream - header may have a dts as well */
                        str->dts = pts;

                        if (header[7] & 0x40) != 0x00)
                        {
                            pts = read_pts(header, 14);
                            if (pts != INVALID_TIMESTAMP)
                                str->dts = pts;
                        }
                    }
#endif
                    str->pkt_flags |= PKT_HAS_TS;
                }
            }
        }
        else                            /* mpeg1 */
        {
            /* Max lookahead: 24 + 2 + 9 = 35 */
            /* Max len_skip: 24 + 2 = 26      */
            /* Min length: 7                  */
            /* Max length: 24 + 2 + 9 = 35    */
            off_t len_skip;
            uint8_t * ptsbuf;

            length = 7;

            while (header[length - 1] == 0xff)
            {
                if (++length > 23)
                {
                    DEBUGF("Too much stuffing" );
                    break;
                }
            }

            if ((header[length - 1] & 0xc0) == 0x40)
                length += 2;

            len_skip = length;
            length += mpeg1_skip_table[header[length - 1] >> 4];

            /* Header points to the mpeg1 pes header */
            ptsbuf = header + len_skip;

            if ((ptsbuf[-1] & 0xe0) == 0x20 && TS_CHECK_MARKERS(ptsbuf, -1))
            {
                /* header has a pts */
                uint32_t pts = read_pts(ptsbuf, -1);

                if (pts != INVALID_TIMESTAMP)
                {
                    str->pts = pts;
#if 0
                    /* DTS isn't used for anything since things just get
                       decoded ASAP but keep the code around */
                    if (STREAM_IS_VIDEO(id))
                    {
                        /* Video stream - header may have a dts as well */
                        str->dts = pts;

                        if (ptsbuf[-1] & 0xf0) == 0x30)
                        {
                            pts = read_pts(ptsbuf, 4);

                            if (pts != INVALID_TIMESTAMP)
                                str->dts = pts;
                        }
                    }
#endif
                    str->pkt_flags |= PKT_HAS_TS;
                }
            }
        }

        p += length;
        /* Max bytes: 6 + 65535 - 7 = 65534 */
        bytes = 6 + (header[4] << 8) + header[5] - length;

        str->curr_packet = p;
        str->curr_packet_end = p + bytes;
        str->hdr.win_left = str->hdr.win_right + length;
        str->hdr.win_right = str->hdr.win_left + bytes;

        if (str->hdr.win_right > disk_buf.filesize)
        {
            /* No packet that exceeds end of file can be valid */
            str_end_of_stream(str);
            return STREAM_DATA_END;
        }

        return STREAM_OK;
    } /* end while */

    #undef INC_BUF
}

/* This simply reads data from the file one page at a time and returns a
 * pointer to it in the buffer. */
static int parse_elementary(struct stream *str, enum stream_parse_mode type)
{
    uint8_t *p;
    ssize_t len = 0;

    str->pkt_flags = 0;

    switch (type)
    {
    case STREAM_PM_STREAMING:
        /* Has the end been reached already? */
        if (str->state == SSTATE_END)
            return STREAM_DATA_END;

        /* Are we at the end of file? */
        if (str->hdr.win_left >= disk_buf.filesize)
        {
            str_end_of_stream(str);
            return STREAM_DATA_END;
        }

        if (!disk_buf_is_data_ready(&str->hdr, MIN_BUFAHEAD))
        {
            /* This data range is not buffered yet - register stream to
             * be notified when it becomes available. Stream is obliged
             * to enter a TSTATE_DATA state if it must wait. */
            int res = str_next_data_not_ready(str);

            if (res != STREAM_OK)
                return res;
        }

        len = DISK_BUF_PAGE_SIZE;

        if ((size_t)(str->hdr.win_right + len) > (size_t)disk_buf.filesize)
            len = disk_buf.filesize - str->hdr.win_right;

        if (len <= 0)
        {
            str_end_of_stream(str);
            return STREAM_DATA_END;
        }

        p = str->curr_packet_end;
        if (p >= disk_buf.end)
            p -= disk_buf.size;
        break;
        /* STREAM_PM_STREAMING: */

    case STREAM_PM_RANDOM_ACCESS:
        str->hdr.pos = disk_buf_lseek(str->hdr.pos, SEEK_SET);
        len = disk_buf_getbuffer(DISK_BUF_PAGE_SIZE, &p, NULL, NULL);

        if (len <= 0 || str->hdr.pos < 0 || str->hdr.pos >= str->hdr.limit)
        {
            str_end_of_stream(str);
            return STREAM_DATA_END;
        }
        break;
        /* STREAM_PM_RANDOM_ACCESS: */
    }

    str->state = SSTATE_PARSE;
    str->curr_packet = p;
    str->curr_packet_end = p + len;
    str->hdr.win_left = str->hdr.win_right;
    str->hdr.win_right = str->hdr.win_left + len;

    return STREAM_OK;
}

intptr_t parser_send_video_msg(long id, intptr_t data)
{
    intptr_t retval = 0;

    if (video_str.thread != 0 && disk_buf.in_file >= 0)
    {
        /* Hook certain messages since they involve multiple operations
         * behind the scenes */
        switch (id)
        {
        case VIDEO_DISPLAY_SHOW:
            if (data != 0 && disk_buf_status() == STREAM_STOPPED)
            {   /* Only prepare image if showing and not playing */
                prepare_image(str_parser.last_seek_time);
            }
            break;

        case VIDEO_PRINT_FRAME:
            if (data)
                break;
        case VIDEO_PRINT_THUMBNAIL:
            if (disk_buf_status() != STREAM_STOPPED)
                break; /* Prepare image if not playing */

            if (!prepare_image(str_parser.last_seek_time))
                return false; /* Preparation failed */

            /* Image ready - pass message to video thread */
            break;
        }

        retval = str_send_msg(&video_str, id, data);
    }

    return retval;
}

/* Seek parser to the specified time and return absolute time.
 * No actual hard stuff is performed here. That's done when streaming is
 * about to begin or something from the current position is requested */
uint32_t parser_seek_time(uint32_t time)
{
    if (!parser_can_seek())
        time = 0;
    else if (time > str_parser.duration)
        time = str_parser.duration;

    str_parser.last_seek_time = time + str_parser.start_pts;
    return str_parser.last_seek_time;
}

void parser_prepare_streaming(void)
{
    struct stream_window sw;

    DEBUGF("parser_prepare_streaming\n");

    /* Prepare initial video frame */
    prepare_image(str_parser.last_seek_time);

    /* Sync audio stream */
    if (audio_str.start_pts != INVALID_TIMESTAMP)
        prepare_audio(str_parser.last_seek_time);

    /* Prequeue some data and set buffer window */
    if (!stream_get_window(&sw))
        sw.left = sw.right = disk_buf.filesize;

    DEBUGF("  swl:%ld swr:%ld\n", sw.left, sw.right);

    if (sw.right > disk_buf.filesize - 4*MIN_BUFAHEAD)
        sw.right = disk_buf.filesize - 4*MIN_BUFAHEAD;

    disk_buf_prepare_streaming(sw.left,
        sw.right - sw.left + 4*MIN_BUFAHEAD);
}

int parser_init_stream(void)
{
    if (disk_buf.in_file < 0)
        return STREAM_ERROR;

    /* TODO: Actually find which streams are available */
    audio_str.id = MPEG_STREAM_AUDIO_FIRST;
    video_str.id = MPEG_STREAM_VIDEO_FIRST;

    /* Try to pull a video PES - if not found, try video init anyway which
     * should succeed if it really is a video-only stream */
    video_str.hdr.pos = 0;
    video_str.hdr.limit = 256*1024;
    
    if (parse_demux(&video_str, STREAM_PM_RANDOM_ACCESS) == STREAM_OK)
    {
        /* Found a video packet - assume program stream */
        str_parser.format = STREAM_FMT_MPEG_PS;
        str_parser.next_data = parse_demux;
    }
    else
    {
        /* No PES element found - assume video elementary stream */
        str_parser.format = STREAM_FMT_MPV;
        str_parser.next_data = parse_elementary;
    }

    if (!init_video_info())
    {
        /* Cannot determine video size, etc. */
        return STREAM_UNSUPPORTED;
    }

    if (str_parser.format == STREAM_FMT_MPEG_PS)
    {
        /* Initalize start_pts and end_pts with the length (in 45kHz units) of
         * the movie. INVALID_TIMESTAMP if the time could not be determined */
        init_times(&audio_str);
        init_times(&video_str);

        if (video_str.start_pts == INVALID_TIMESTAMP)
        {
            /* Must have video at least */
            return STREAM_UNSUPPORTED;
        }

        str_parser.flags |= STREAMF_CAN_SEEK;

        if (audio_str.start_pts != INVALID_TIMESTAMP)
        {
            /* Overall duration is maximum span */
            str_parser.start_pts = MIN(audio_str.start_pts, video_str.start_pts);
            str_parser.end_pts = MAX(audio_str.end_pts, video_str.end_pts);

            /* Audio will be part of playback pool */
            stream_add_stream(&audio_str);
        }
        else
        {
            /* No audio stream - use video only */
            str_parser.start_pts = video_str.start_pts;
            str_parser.end_pts = video_str.end_pts;
        }

        str_parser.last_seek_time = str_parser.start_pts;
    }
    else
    {
        /* There's no way to handle times on this without a full file
         * scan */
        audio_str.start_pts = INVALID_TIMESTAMP;
        audio_str.end_pts = INVALID_TIMESTAMP;
        video_str.start_pts = 0;
        video_str.end_pts = INVALID_TIMESTAMP;
        str_parser.start_pts = 0;
        str_parser.end_pts = INVALID_TIMESTAMP;
    }

    /* Add video to playback pool */
    stream_add_stream(&video_str);

    /* Cache duration - it's used very often */
    str_parser.duration = str_parser.end_pts - str_parser.start_pts;

    DEBUGF("Movie info:\n"
           "  size:%dx%d\n"
           "  start:%u\n"
           "  end:%u\n"
           "  duration:%u\n",
           str_parser.dims.w, str_parser.dims.h,
           (unsigned)str_parser.start_pts,
           (unsigned)str_parser.end_pts,
           (unsigned)str_parser.duration);

    return STREAM_OK;
}

void parser_close_stream(void)
{
    stream_remove_streams();
    parser_init_state();
}

bool parser_init(void)
{
    parser_init_state();
    return true;
}
