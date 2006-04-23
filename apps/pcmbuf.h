/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCMBUF_H
#define PCMBUF_H

#define PCMBUF_TARGET_CHUNK 32768 /* This is the target fill size of chunks
                                     on the pcm buffer */
#define PCMBUF_MIN_CHUNK     4096 /* We try to never feed a chunk smaller than
                                     this to the DMA */
#define PCMBUF_MIX_CHUNK     8192 /* This is the maximum size of one packet
                                     for mixing (crossfade or voice) */

/* Returns true if the buffer needs to change size */
bool pcmbuf_is_same_size(size_t bufsize);
void pcmbuf_init(size_t bufsize);
/* Size in bytes used by the pcmbuffer */
size_t pcmbuf_get_bufsize(void);
size_t get_pcmbuf_descsize(void);

void pcmbuf_pause(bool pause);
void pcmbuf_play_stop(void);
bool pcmbuf_is_crossfade_active(void);

/* These functions are for playing chained buffers of PCM data */
#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && !defined(SIMULATOR)
void pcmbuf_boost(bool state);
void pcmbuf_set_boost_mode(bool state);
#else
#define pcmbuf_boost(state) do { } while(0)
#define pcmbuf_set_boost_mode(state) do { } while(0)
#endif
bool pcmbuf_is_lowdata(void);
void pcmbuf_play_start(void);
bool pcmbuf_crossfade_init(bool manual_skip);
void pcmbuf_set_event_handler(void (*callback)(void));
void pcmbuf_set_position_callback(void (*callback)(size_t size));
size_t pcmbuf_free(void);
unsigned int pcmbuf_get_latency(void);
void pcmbuf_set_low_latency(bool state);
void pcmbuf_write_complete(size_t length);
void* pcmbuf_request_buffer(size_t length, size_t *realsize);
void* pcmbuf_request_voice_buffer(size_t length, size_t *realsize, bool mix);
bool pcmbuf_is_crossfade_enabled(void);
void pcmbuf_crossfade_enable(bool on_off);

int pcmbuf_usage(void);
int pcmbuf_mix_free(void);
void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude);
void pcmbuf_mix_voice(size_t length);

int pcmbuf_used_descs(void);
int pcmbuf_descs(void);

#endif
