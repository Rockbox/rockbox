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

/* Guard buffer for crossfader when dsp is enabled. */
#define PCMBUF_GUARD  32768

void pcmbuf_init(long bufsize);
long pcmbuf_get_bufsize(void);

void pcmbuf_play_stop(void);
bool pcmbuf_is_crossfade_active(void);

/* These functions are for playing chained buffers of PCM data */
bool pcmbuf_add_chunk(void *addr, int size, void (*callback)(void));
int pcmbuf_num_used_buffers(void);
void pcmbuf_set_watermark(int numbytes, void (*callback)(int bytes_left));

void pcmbuf_boost(bool state);
void pcmbuf_set_boost_mode(bool state);
bool pcmbuf_is_lowdata(void);
void pcmbuf_flush_audio(void);
void pcmbuf_play_start(void);
bool pcmbuf_crossfade_init(int type);
void pcmbuf_add_event(void (*event_handler)(void));
unsigned int pcmbuf_get_latency(void);
bool pcmbuf_insert_buffer(char *buf, long length);
void pcmbuf_flush_buffer(long length);
void* pcmbuf_request_buffer(long length, long *realsize);
bool pcmbuf_is_crossfade_enabled(void);
void pcmbuf_crossfade_enable(bool on_off);

int pcmbuf_usage(void);
int pcmbuf_mix_usage(void);
void pcmbuf_beep(int frequency, int duration, int amplitude);
void pcmbuf_reset_mixpos(void);
void pcmbuf_mix(char *buf, long length);

#endif
