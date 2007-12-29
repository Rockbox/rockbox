/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Main mpegplayer config header.
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
#ifndef MPEGPLAYER_H
#define MPEGPLAYER_H

/* Global API pointer */
extern struct plugin_api* rb;

#ifdef HAVE_SCHEDULER_BOOSTCTRL
#define trigger_cpu_boost rb->trigger_cpu_boost
#define cancel_cpu_boost  rb->cancel_cpu_boost
#endif
/* #else function-like empty macros are defined in the headers */

/* Memory allotments for various subsystems */
#define MIN_MEMMARGIN (4*1024)

enum mpeg_malloc_reason_t
{
    __MPEG_ALLOC_FIRST = -256,
    MPEG_ALLOC_CODEC_MALLOC,
    MPEG_ALLOC_CODEC_CALLOC,
    MPEG_ALLOC_MPEG2_BUFFER,
    MPEG_ALLOC_AUDIOBUF,
    MPEG_ALLOC_PCMOUT,
    MPEG_ALLOC_DISKBUF,
};

/** Video thread **/
#define LIBMPEG2_ALLOC_SIZE (2*1024*1024)

/** MPEG audio buffer **/
#define AUDIOBUF_GUARD_SIZE (MPA_MAX_FRAME_SIZE + 2*MAD_BUFFER_GUARD)
#define AUDIOBUF_SIZE       (64*1024)
#define AUDIOBUF_ALLOC_SIZE (AUDIOBUF_SIZE+AUDIOBUF_GUARD_SIZE)

/** PCM buffer **/
#define CLOCK_RATE 44100 /* Our clock rate in ticks/second (samplerate) */

/* Define this as "1" to have a test tone instead of silence clip */
#define SILENCE_TEST_TONE 0

#define PCMOUT_BUFSIZE    (CLOCK_RATE) /* 1s */
#define PCMOUT_GUARD_SIZE (1152*4 + sizeof (struct pcm_frame_header))
#define PCMOUT_ALLOC_SIZE (PCMOUT_BUFSIZE + PCMOUT_GUARD_SIZE)
                          /* Start pcm playback @ 25% full */
#define PCMOUT_PLAY_WM    (PCMOUT_BUFSIZE/4)
                          /* No valid audio frame is smaller */
#define PCMOUT_LOW_WM     (sizeof (struct pcm_frame_header))

/** disk buffer **/
#define DISK_BUF_LOW_WATERMARK (1024*1024)
/* 65535+6 is required since each PES has a 6 byte header with a 16 bit
 * packet length field */
#define DISK_GUARDBUF_SIZE     ALIGN_UP(65535+6, 4)

#ifdef HAVE_LCD_COLOR
#define DRAW_BLACK            LCD_BLACK
#define DRAW_DARKGRAY         LCD_DARKGRAY
#define DRAW_LIGHTGRAY        LCD_LIGHTGRAY
#define DRAW_WHITE            LCD_WHITE
#define lcd_(fn)              rb->lcd_##fn
#define lcd_splash            splash

#define GRAY_FLUSH_ICACHE()
#define GRAY_INVALIDATE_ICACHE()
#define GRAY_VIDEO_FLUSH_ICACHE()
#define GRAY_VIDEO_INVALIDATE_ICACHE()
#else
#include "gray.h"
#define DRAW_BLACK            GRAY_BLACK
#define DRAW_DARKGRAY         GRAY_DARKGRAY
#define DRAW_LIGHTGRAY        GRAY_LIGHTGRAY
#define DRAW_WHITE            GRAY_WHITE
#define lcd_(fn)              gray_##fn

#define GRAY_FLUSH_ICACHE() \
    IF_COP(flush_icache())
#define GRAY_INVALIDATE_ICACHE() \
    IF_COP(invalidate_icache())
#define GRAY_VIDEO_FLUSH_ICACHE() \
    IF_COP(parser_send_video_msg(VIDEO_GRAY_CACHEOP, 0))
#define GRAY_VIDEO_INVALIDATE_ICACHE() \
    IF_COP(parser_send_video_msg(VIDEO_GRAY_CACHEOP, 1))
#if NUM_CORES > 1
#define GRAY_CACHE_MAINT
#endif
#endif

#include "mpeg2.h"
#include "video_out.h"
#include "mpeg_stream.h"
#include "mpeg_linkedlist.h"
#include "mpeg_misc.h"
#include "mpeg_alloc.h"
#include "stream_thread.h"
#include "parser.h"
#include "pcm_output.h"
#include "disk_buf.h"
#include "stream_mgr.h"

#endif /* MPEGPLAYER_H */
