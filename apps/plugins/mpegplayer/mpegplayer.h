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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef MPEGPLAYER_H
#define MPEGPLAYER_H

#ifdef HAVE_SCHEDULER_BOOSTCTRL
#define trigger_cpu_boost rb->trigger_cpu_boost
#define cancel_cpu_boost  rb->cancel_cpu_boost
#endif
/* #else function-like empty macros are defined in the headers */

/* Should be enough for now */
#define MPEGPLAYER_MAX_STREAMS 4

/* Memory allotments for various subsystems */
#define MIN_MEMMARGIN (4*1024)

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

/* NOTE: Sizes make no frame header allowance when considering duration */
#define PCMOUT_BUFSIZE       (CLOCK_RATE/2*4) /* 1/2s */
#define PCMOUT_GUARD_SIZE    (PCMOUT_BUFSIZE) /* guarantee contiguous sizes */
#define PCMOUT_ALLOC_SIZE    (PCMOUT_BUFSIZE + PCMOUT_GUARD_SIZE)
                             /* Start pcm playback @ 25% full */
#define PCMOUT_PLAY_WM       (PCMOUT_BUFSIZE/4)
#define PCMOUT_LOW_WM        (0)

/** disk buffer **/
#define DISK_BUF_LOW_WATERMARK (1024*1024)
/* 65535+6 is required since each PES has a 6 byte header with a 16 bit
 * packet length field */
#define DISK_GUARDBUF_SIZE     ALIGN_UP(65535+6, 4)

#ifdef HAVE_LCD_COLOR
#define mylcd_splash         rb->splash
#else
#include "lib/grey.h"
#define mylcd_splash         grey_splash
#endif

#include "lib/mylcd.h"

#include "libmpeg2/mpeg2.h"
#include "video_out.h"
#include "mpeg_stream.h"
#include "mpeg_misc.h"
#include "mpeg_alloc.h"
#include "stream_thread.h"
#include "parser.h"
#include "pcm_output.h"
#include "disk_buf.h"
#include "stream_mgr.h"

#define LCD_ENABLE_EVENT_0 MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0)
#define LCD_ENABLE_EVENT_1 MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 1)

#ifdef PLUGIN_USE_IRAM
/* IRAM preserving mechanism to enable talking menus */
extern void mpegplayer_iram_preserve(void);
extern void mpegplayer_iram_restore(void);
#endif

#endif /* MPEGPLAYER_H */
