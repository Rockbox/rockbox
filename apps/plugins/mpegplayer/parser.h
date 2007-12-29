/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * AV parser inteface declarations
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
#ifndef PARSER_H
#define PARSER_H

enum stream_formats
{
    STREAM_FMT_UNKNOWN = -1,
    STREAM_FMT_MPEG_TS, /* MPEG transport stream */
    STREAM_FMT_MPEG_PS, /* MPEG program stream */
    STREAM_FMT_MPV,     /* MPEG Video only (1 or 2) */
    STREAM_FMT_MPA,     /* MPEG Audio only */
};

/* Structure used by a thread that handles a single demuxed data stream and
 * receives commands from the stream manager */
enum stream_parse_states
{
                  /* Stream is... */
    SSTATE_SYNC,  /* synchronizing by trying to find a start code */
    SSTATE_PARSE, /* parsing the stream looking for packets */
    SSTATE_END,   /* at the end of data */
};

enum stream_parse_mode
{
    STREAM_PM_STREAMING = 0, /* Next packet when streaming */
    STREAM_PM_RANDOM_ACCESS, /* Random-access parsing */
};

enum stream_parser_flags
{
    STREAMF_CAN_SEEK = 0x1, /* Seeking possible for this stream */
};

struct stream_parser
{
    /* Common generic parser data */
    enum stream_formats format; /* Stream format */
    uint32_t start_pts;    /* The movie start time as represented by
                              the first audio PTS tag in the
                              stream converted to half minutes */
    uint32_t end_pts;      /* The movie end time as represented by
                              the maximum audio PTS tag in the
                              stream converted to half minutes */
    uint32_t duration;     /* Duration in PTS units */
    unsigned flags;        /* Various attributes set at init */
    struct vo_ext dims;    /* Movie dimensions in pixels */
    uint32_t last_seek_time;
    int (*next_data)(struct stream *str, enum stream_parse_mode type);
    union /* A place for reusable no-cache parameters */
    {
        struct str_sync_data sd;
    } parms;
};

extern struct stream_parser str_parser;

/* MPEG parsing */
uint8_t * mpeg_parser_scan_start_code(struct stream_scan *sk, uint32_t code);
unsigned mpeg_parser_scan_pes(struct stream_scan *sk);
uint32_t mpeg_parser_scan_scr(struct stream_scan *sk);
uint32_t mpeg_parser_scan_pts(struct stream_scan *sk, unsigned id);
off_t mpeg_stream_stream_seek_PTS(uint32_t time, int id);

/* General parsing */
bool parser_init(void);
void str_initialize(struct stream *str, off_t pos);
intptr_t parser_send_video_msg(long id, intptr_t data);
bool parser_get_video_size(struct vo_ext *sz);
int parser_init_stream(void);
void parser_close_stream(void);
static inline bool parser_can_seek(void)
    { return str_parser.flags & STREAMF_CAN_SEEK; }
uint32_t parser_seek_time(uint32_t time);
void parser_prepare_streaming(void);
void str_end_of_stream(struct stream *str);

static inline int parser_get_next_data(struct stream *str,
                                       enum stream_parse_mode type)
    { return str_parser.next_data(str, type); }

#endif /* PARSER_H */
