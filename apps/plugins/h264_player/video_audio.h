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
#ifndef VIDEO_AUDIO_H
#define VIDEO_AUDIO_H

/*
 * H.264 player audio decode service.
 *
 * Decodes AAC audio via Rockbox's aac.codec and feeds a PCM ring buffer.
 * Uses the audio clock from video_pcm for A/V synchronization.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mp4_demux.h"

/* Return enough codec heap to retain every AAC chunk offset. Apple M4V files
 * interleave audio and video densely, so a sparse chunk map cannot decode the
 * audio stream correctly. Returns 0 if the size cannot be represented. */
size_t video_audio_workspace_size(const struct mp4v_demux_res *demux);

/* Initialize audio decoder and thread.
 * filepath: MP4 file path (opens its own fd for independent seeks)
 * demux: parsed MP4 with audio track data (audio_format != 0)
 * Returns 0 on success, -1 on error. */
int video_audio_init(const char *filepath,
                     const struct mp4v_demux_res *demux,
                     void *codec_workspace, size_t workspace_size,
                     void *pcm_buffer, size_t pcm_buffer_size);

/* Start audio playback (begins decoding and filling PCM buffer). */
void video_audio_play(void);

/* Pause audio decode (PCM buffer drains, then silence). */
void video_audio_pause(void);

/* Resume audio decode after pause. */
void video_audio_resume(void);

/* Seek audio to target time in milliseconds. Flushes PCM buffer. */
void video_audio_seek(uint32_t target_ms);

/* Stop audio thread and release resources. Blocks until thread exits. */
void video_audio_stop(void);

/* Check if audio thread has pre-filled enough PCM data to start. */
bool video_audio_ready(void);

/* Check if audio thread is actively decoding (false after EOF or error). */
bool video_audio_is_active(void);

/* True if the dynamically loaded AAC codec failed to load or decode. */
bool video_audio_failed(void);

#endif /* VIDEO_AUDIO_H */
