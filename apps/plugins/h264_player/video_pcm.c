/***************************************************************************
 * PCM ring buffer for video player audio output.
 *
 * Simplified version of mpegplayer's pcm_output.c.
 * Single-producer (audio decode thread), single-consumer (DMA ISR).
 * Audio-master clock via monotonic sample counter.
 *
 * get_more() runs in DMA ISR context: no yield, sleep, mutex, or file I/O.
 *
 * Copyright (C) 2025-2026 David Cormier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 ****************************************************************************/
#include "plugin.h"

#ifdef HAVE_HW_H264

#include "system.h"
#include "kernel.h"
#include "pcm.h"
#include "pcm_mixer.h"
#include "pcm_sampr.h"
#include "video_pcm.h"

#include <string.h>

/* Silence clip for underrun (256 stereo samples = ~5.8ms at 44.1kHz) */
#define SILENCE_SAMPLES   256

static int16_t *pcm_buffer;
static uint32_t pcm_buffer_frames;
static int16_t silence[SILENCE_SAMPLES * 2];

/* Read/write positions in stereo samples (wrap via modulo) */
static volatile uint32_t pcm_read_pos;
static volatile uint32_t pcm_write_pos;

/* Master clock: stereo samples consumed by DMA (uint32_t sufficient
 * for >27 hours at 44.1kHz; avoids torn reads on 32-bit ARM) */
static volatile uint32_t clock_samples;

/* Clock base: absolute time (ms) at last flush/init */
static uint32_t clock_base_ms;
static uint32_t clock_sample_rate;

/* Flush guard: prevents write during flush (set by main, checked by audio) */
static volatile bool flush_pending;
static struct mutex write_mutex;

/* Saved mixer frequency for restore on stop */
static unsigned int saved_sampr;

static uint32_t pcm_used(void)
{
    return pcm_write_pos - pcm_read_pos;
}

static uint32_t pcm_free(void)
{
    uint32_t used = pcm_used();

    return used < pcm_buffer_frames ? pcm_buffer_frames - used : 0;
}

/* DMA callback runs in ISR context.
 * Pulls from ring buffer, advances clock. On underrun, plays silence. */
static void video_pcm_get_more(const void **start, size_t *size)
{
    uint32_t avail = pcm_used();

    if (avail > 0)
    {
        uint32_t rd = pcm_read_pos % pcm_buffer_frames;
        uint32_t chunk = avail;

        /* Don't wrap around the buffer end */
        if (rd + chunk > pcm_buffer_frames)
            chunk = pcm_buffer_frames - rd;

        /* Limit chunk to one mixer frame so the entire returned buffer
         * is consumed before pcm_read_pos advances. Without this, the
         * mixer stores our pointer and gradually reads over 8 callbacks
         * while the write side sees the positions as "free" and
         * overwrites them, causing audible fragment repetition. */
        if (chunk > MIX_FRAME_SAMPLES)
            chunk = MIX_FRAME_SAMPLES;

        *start = &pcm_buffer[rd * 2];
        *size = chunk * 4; /* stereo 16-bit = 4 bytes per sample */
        pcm_read_pos += chunk;
        clock_samples += chunk;
    }
    else
    {
        /* Underrun: play silence. Do NOT advance the clock;
         * advancing would create a permanent A/V offset because the
         * audio content hasn't actually been played yet. */
        *start = silence;
        *size = sizeof(silence);
    }
}

static const struct mixer_play_cbs video_pcm_cbs = {
    .get_more = video_pcm_get_more,
};

bool video_pcm_init(uint32_t sample_rate, void *buffer, size_t buffer_size)
{
    if (sample_rate == 0 || buffer == NULL ||
        buffer_size / (2 * sizeof(int16_t)) < MIX_FRAME_SAMPLES)
        return false;

    /* Save current frequency for restore */
    saved_sampr = rb->mixer_get_frequency();
    pcm_buffer = buffer;
    pcm_buffer_frames = buffer_size / (2 * sizeof(int16_t));

    /* Reset buffer state */
    pcm_read_pos = 0;
    pcm_write_pos = 0;
    clock_samples = 0;
    clock_base_ms = 0;
    clock_sample_rate = sample_rate;
    flush_pending = false;
    rb->mutex_init(&write_mutex);
    rb->memset(silence, 0, sizeof(silence));

    /* Set sample rate BEFORE starting playback
     * (mixer_set_frequency calls mixer_reset which stops all channels) */
#if INPUT_SRC_CAPS != 0
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    rb->mixer_set_frequency(sample_rate);

    /* Start playback with DMA callback */
    rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_PLAYBACK, MIX_AMP_UNITY);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK,
                            &video_pcm_cbs, NULL, 0);
    return true;
}

int video_pcm_write(const int16_t *pcm, int stereo_samples)
{
    int written = 0;

    rb->mutex_lock(&write_mutex);
    if (flush_pending)
    {
        rb->mutex_unlock(&write_mutex);
        return 0;
    }

    while (written < stereo_samples)
    {
        uint32_t space = pcm_free();
        if (space == 0 || flush_pending)
            break;

        uint32_t wr = pcm_write_pos % pcm_buffer_frames;
        uint32_t chunk = stereo_samples - written;

        if (chunk > space)
            chunk = space;

        /* Don't wrap around the buffer end */
        if (wr + chunk > pcm_buffer_frames)
            chunk = pcm_buffer_frames - wr;

        rb->memcpy(&pcm_buffer[wr * 2], &pcm[written * 2], chunk * 4);
        pcm_write_pos += chunk;
        written += chunk;
    }

    rb->mutex_unlock(&write_mutex);
    return written;
}

uint32_t video_pcm_get_clock_ms(void)
{
    uint32_t samples = clock_samples; /* atomic 32-bit read on ARM */
    if (clock_sample_rate == 0)
        return clock_base_ms;
    return clock_base_ms + (uint32_t)((uint64_t)samples * 1000
                                     / clock_sample_rate);
}

void video_pcm_flush(uint32_t base_ms, uint32_t sample_rate)
{
    /* Serialize against the codec producer before resetting its indexes. */
    rb->mutex_lock(&write_mutex);
    flush_pending = true;

    /* Stop mixer channel to invalidate any stale buffer pointer,
     * then reset state, then restart (mpegplayer pattern). */
    rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);

    pcm_read_pos = 0;
    pcm_write_pos = 0;
    clock_samples = 0;
    clock_base_ms = base_ms;
    clock_sample_rate = sample_rate;

    /* Restart mixer with fresh callback */
    rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_PLAYBACK, MIX_AMP_UNITY);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK,
                            &video_pcm_cbs, NULL, 0);

    flush_pending = false;
    rb->mutex_unlock(&write_mutex);
}

void video_pcm_pause(bool pause)
{
    rb->mixer_channel_play_pause(PCM_MIXER_CHAN_PLAYBACK, !pause);
}

void video_pcm_stop(void)
{
    flush_pending = false;

    /* Serialize callback removal before plugin-owned memory is released. */
    rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);

    /* Restore original sample rate */
    if (saved_sampr != 0)
        rb->mixer_set_frequency(saved_sampr);
    pcm_buffer = NULL;
    pcm_buffer_frames = 0;
}

bool video_pcm_has_space(int stereo_samples)
{
    return pcm_free() >= (uint32_t)stereo_samples;
}

bool video_pcm_empty(void)
{
    return pcm_used() == 0;
}

uint32_t video_pcm_buffered_samples(void)
{
    return pcm_used();
}

#endif /* HAVE_HW_H264 */
