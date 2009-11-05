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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCMBUF_H
#define PCMBUF_H

/* playback */
size_t pcmbuf_init(unsigned char *bufend);
void pcmbuf_play_start(void);
void pcmbuf_play_stop(void);
void pcmbuf_pause(bool pause);
void pcmbuf_start_track_change(bool manual_skip);
void *pcmbuf_request_buffer(int *count);
void pcmbuf_write_complete(int count);

/* crossfade */
bool pcmbuf_is_crossfade_active(void);
void pcmbuf_request_crossfade_enable(bool on_off);
bool pcmbuf_is_same_size(void);

/* voice */
void *pcmbuf_request_voice_buffer(int *count);
void pcmbuf_write_voice_complete(int count);

/* debug menu, other metrics */
size_t pcmbuf_free(void);
size_t pcmbuf_get_bufsize(void);
int pcmbuf_descs(void);
int pcmbuf_used_descs(void);
#ifdef ROCKBOX_HAS_LOGF
unsigned char *pcmbuf_get_meminfo(size_t *length);
#endif

/* misc */
void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude);
bool pcmbuf_is_lowdata(void);
void pcmbuf_set_low_latency(bool state);
unsigned long pcmbuf_get_latency(void);

#endif
