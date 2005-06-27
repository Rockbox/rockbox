/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCM_PLAYBACK_H
#define PCM_PLAYBACK_H

/* Guard buffer for crossfader when dsp is enabled. */
#define PCMBUF_GUARD  32768

/* PCM audio buffer. */
#define PCMBUF_SIZE   (1*1024*1024)

void pcm_init(void);
void pcm_set_frequency(unsigned int frequency);

/* This is for playing "raw" PCM data */
void pcm_play_data(const unsigned char* start, int size,
                   void (*get_more)(unsigned char** start, long* size));

void pcm_play_stop(void);
void pcm_play_pause(bool play);
bool pcm_is_playing(void);

/* These functions are for playing chained buffers of PCM data */
void pcm_play_init(void);
void pcm_play_start(void);
bool pcm_play_add_chunk(void *addr, int size, void (*callback)(void));
int pcm_play_num_used_buffers(void);
void pcm_play_set_watermark(int numbytes, void (*callback)(int bytes_left));

void pcm_set_boost_mode(bool state);
bool pcm_is_lowdata(void);
bool pcm_crossfade_init(void);
void audiobuffer_add_event(void (*event_handler)(void));
unsigned int audiobuffer_get_latency(void);
bool pcm_insert_buffer(char *buf, long length);
void pcm_flush_buffer(long length);
void* pcm_request_buffer(long length, long *realsize);
bool pcm_is_crossfade_enabled(void);
void pcm_crossfade_enable(bool on_off);

#endif
