/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025-2026 David Cormier
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
#ifndef VIDEO_PCM_H
#define VIDEO_PCM_H

/*
 * PCM ring buffer for H.264 player audio output.
 *
 * Simplified version of mpegplayer's pcm_output.c.
 * Audio-master clock: DMA callback advances clock_samples monotonically.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Initialize PCM output at the given sample rate.
 * Saves and changes mixer frequency. */
bool video_pcm_init(uint32_t sample_rate, void *buffer, size_t buffer_size);

/* Write interleaved stereo int16 PCM to the ring buffer.
 * Returns number of stereo samples actually written (may be less if full). */
int video_pcm_write(const int16_t *pcm, int stereo_samples);

/* Get audio master clock in milliseconds.
 * Returns base_ms + (samples_played * 1000 / sample_rate). */
uint32_t video_pcm_get_clock_ms(void);

/* Flush ring buffer and set clock base (for seek).
 * After flush, get_clock returns base_ms + samples_played. */
void video_pcm_flush(uint32_t base_ms, uint32_t sample_rate);

/* Pause/resume PCM output. */
void video_pcm_pause(bool pause);

/* Stop PCM output and restore original mixer frequency. */
void video_pcm_stop(void);

/* Check if ring buffer has space for at least n stereo samples. */
bool video_pcm_has_space(int stereo_samples);

/* Check if ring buffer is empty (all data played). */
bool video_pcm_empty(void);

/* Return number of stereo samples currently buffered. */
uint32_t video_pcm_buffered_samples(void);

#endif /* VIDEO_PCM_H */
