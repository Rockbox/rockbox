/***************************************************************************
 * AAC audio service for the native S5L8702 H.264 player.
 *
 * The decoder itself remains Rockbox's normal dynamically-loaded aac.codec.
 * That is important on iPod 6G: FAAD's hot code and working state are an IRAM
 * overlay and cannot be linked into the permanent core without overflowing
 * IRAM. This file borrows the existing codec thread, supplies file-buffer
 * callbacks for the MP4, and sends decoded PCM to video_pcm.
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

#include <stddef.h>

#include "codec_thread.h"
#include "codecs.h"
#include "file.h"
#include "kernel.h"
#include "metadata.h"
#include "mp4_demux.h"
#include "video_audio.h"
#include "video_pcm.h"

#include <stdint.h>
#include <string.h>

#define AUDIO_FILE_BUFFER_SIZE 8192
#define AUDIO_PCM_FRAMES       1024
#define AUDIO_START_BUFFER_MS  1000
/* libm4a's sample_offset_t stores a uint32_t sample and file offset. Keep a
 * normal CODEC_SIZE of headroom after the complete Apple chunk map. */
#define AUDIO_CHUNK_LOOKUP_ENTRY_SIZE (2u * sizeof(uint32_t))

static struct codec_api video_ci;
static struct mp3entry video_id3;
static int audio_fd = -1;
static off_t audio_file_size;
static uint8_t audio_file_buffer[AUDIO_FILE_BUFFER_SIZE];
static off_t audio_file_buffer_offset;
static size_t audio_file_buffer_length;
static int16_t audio_pcm[AUDIO_PCM_FRAMES * 2];
static void *audio_codec_workspace;
static size_t audio_codec_workspace_size;
static uint32_t audio_sample_rate;
static int audio_sample_depth;
static int audio_stereo_mode;
static volatile long audio_action;
static volatile uint32_t audio_seek_target_ms;
static volatile bool audio_paused;
static volatile bool audio_is_ready;
static volatile bool audio_is_playing;
static volatile bool audio_has_error;
static bool audio_initialized;

size_t video_audio_workspace_size(const struct mp4v_demux_res *demux)
{
    size_t table_size;

    if (demux == NULL || demux->audio_num_samples == 0 ||
        demux->audio_num_stco == 0 ||
        demux->audio_num_stco >
            (SIZE_MAX - CODEC_SIZE) / AUDIO_CHUNK_LOOKUP_ENTRY_SIZE)
        return 0;
    table_size = (size_t)demux->audio_num_stco *
                 AUDIO_CHUNK_LOOKUP_ENTRY_SIZE;
    return CODEC_SIZE + table_size;
}

static int16_t audio_clip16(int32_t value)
{
    if (value > 32767)
        return 32767;
    if (value < -32768)
        return -32768;
    return (int16_t)value;
}

static void *audio_codec_get_buffer(size_t *size)
{
    *size = audio_codec_workspace_size;
    return audio_codec_workspace;
}

static bool audio_refill(off_t position)
{
    ssize_t count;

    if (position < 0 || position > audio_file_size ||
        rb->lseek(audio_fd, position, SEEK_SET) < 0)
        return false;
    count = rb->read(audio_fd, audio_file_buffer, sizeof(audio_file_buffer));
    if (count < 0)
        return false;
    audio_file_buffer_offset = position;
    audio_file_buffer_length = (size_t)count;
    return true;
}

static size_t audio_read_filebuf(void *destination, size_t size)
{
    size_t done = 0;

    while (done < size && video_ci.curpos < audio_file_size)
    {
        size_t in_buffer;
        size_t chunk;

        if (video_ci.curpos < audio_file_buffer_offset ||
            video_ci.curpos >= audio_file_buffer_offset +
                               (off_t)audio_file_buffer_length)
        {
            if (!audio_refill(video_ci.curpos))
                break;
        }
        in_buffer = (size_t)(video_ci.curpos - audio_file_buffer_offset);
        chunk = audio_file_buffer_length - in_buffer;
        if (chunk > size - done)
            chunk = size - done;
        if (chunk == 0)
            break;
        rb->memcpy((uint8_t *)destination + done,
               audio_file_buffer + in_buffer, chunk);
        video_ci.curpos += chunk;
        done += chunk;
    }
    return done;
}

static void *audio_request_buffer(size_t *real_size, size_t requested)
{
    size_t available;
    size_t in_buffer;
    size_t needed;
    off_t remaining;

    if (video_ci.curpos >= audio_file_size)
    {
        *real_size = 0;
        return audio_file_buffer;
    }

    remaining = audio_file_size - video_ci.curpos;
    needed = requested;
    if ((off_t)needed > remaining)
        needed = (size_t)remaining;

    if (video_ci.curpos < audio_file_buffer_offset ||
        video_ci.curpos >= audio_file_buffer_offset +
                           (off_t)audio_file_buffer_length)
    {
        if (!audio_refill(video_ci.curpos))
        {
            *real_size = 0;
            return audio_file_buffer;
        }
    }
    in_buffer = (size_t)(video_ci.curpos - audio_file_buffer_offset);
    available = audio_file_buffer_length - in_buffer;

    /* codec_api.request_buffer promises the requested span whenever that
     * many file bytes remain. Do not return the short tail of our 8 KiB
     * cache: FAAD treats a truncated AAC frame as a fatal decode error. */
    if (available < needed)
    {
        if (!audio_refill(video_ci.curpos))
        {
            *real_size = 0;
            return audio_file_buffer;
        }
        in_buffer = 0;
        available = audio_file_buffer_length;
    }

    if (available > needed)
        available = needed;
    *real_size = available;
    return audio_file_buffer + in_buffer;
}

static void audio_advance_buffer(size_t amount)
{
    off_t remaining = audio_file_size - video_ci.curpos;

    if ((off_t)amount > remaining)
        amount = remaining > 0 ? (size_t)remaining : 0;
    video_ci.curpos += amount;
    video_id3.offset = video_ci.curpos;
}

static bool audio_seek_buffer(size_t position)
{
    if ((off_t)position > audio_file_size)
        return false;
    video_ci.curpos = position;
    video_id3.offset = position;
    return true;
}

static void audio_seek_complete(void)
{
}

static void audio_set_elapsed(unsigned long elapsed)
{
    video_id3.elapsed = elapsed;
}

static void audio_set_offset(size_t offset)
{
    video_id3.offset = offset;
}

static void audio_configure(int setting, intptr_t value)
{
    switch (setting)
    {
        case DSP_SET_FREQUENCY:
            if (value > 0)
                audio_sample_rate = (uint32_t)value;
            break;
        case DSP_SET_SAMPLE_DEPTH:
            audio_sample_depth = (int)value;
            break;
        case DSP_SET_STEREO_MODE:
            audio_stereo_mode = (int)value;
            break;
        default:
            break;
    }
}

static long audio_get_command(intptr_t *parameter)
{
    long action;

    while (audio_paused && audio_action == CODEC_ACTION_NULL)
        rb->sleep(1);
    rb->yield();
    action = audio_action;
    if (action == CODEC_ACTION_SEEK_TIME)
    {
        if (parameter != NULL)
            *parameter = (intptr_t)audio_seek_target_ms;
        /* A seek is a one-shot command.  HALT deliberately remains latched
         * until codec teardown so the producer cannot resume behind us. */
        audio_action = CODEC_ACTION_NULL;
    }
    return action;
}

static bool audio_loop_track(void)
{
    return false;
}

static void audio_strip_filesize(off_t size)
{
    video_ci.filesize = size;
}

static int32_t audio_sample_value(const void *channel, int index)
{
    if (audio_sample_depth <= 16)
        return ((const int16_t *)channel)[index];

    {
        int shift = audio_sample_depth + 1 - 16;
        int32_t value = ((const int32_t *)channel)[index];
        int32_t bias = shift > 0 ? (int32_t)1 << (shift - 1) : 0;

        return shift > 0 ? (value + bias) >> shift : value;
    }
}

static void audio_pcm_insert(const void *channel1, const void *channel2,
                             int count)
{
    int consumed = 0;

    while (consumed < count && audio_action != CODEC_ACTION_HALT)
    {
        int frames;
        int written = 0;
        int i;

        while (audio_paused && audio_action != CODEC_ACTION_HALT)
            rb->sleep(1);
        if (audio_action == CODEC_ACTION_HALT)
            break;

        frames = count - consumed;
        if (frames > AUDIO_PCM_FRAMES)
            frames = AUDIO_PCM_FRAMES;
        for (i = 0; i < frames; i++)
        {
            int source = consumed + i;
            int32_t left;
            int32_t right;

            if (audio_stereo_mode == STEREO_INTERLEAVED)
            {
                left = audio_sample_value(channel1, source * 2);
                right = audio_sample_value(channel1, source * 2 + 1);
            }
            else
            {
                left = audio_sample_value(channel1, source);
                right = audio_stereo_mode == STEREO_MONO ? left :
                        audio_sample_value(channel2, source);
            }
            audio_pcm[i * 2] = audio_clip16(left);
            audio_pcm[i * 2 + 1] = audio_clip16(right);
        }
        while (written < frames && audio_action != CODEC_ACTION_HALT)
        {
            int amount;

            while (audio_paused && audio_action != CODEC_ACTION_HALT)
                rb->sleep(1);
            amount = video_pcm_write(audio_pcm + written * 2,
                                     frames - written);
            if (amount == 0)
            {
                rb->sleep(1);
                continue;
            }
            written += amount;
        }
        consumed += frames;
        if (!audio_is_ready && audio_sample_rate > 0 &&
            video_pcm_buffered_samples() >=
                audio_sample_rate * AUDIO_START_BUFFER_MS / 1000)
            audio_is_ready = true;
    }
}

static void audio_codec_api_init(void)
{
    rb->memset(&video_ci, 0, sizeof(video_ci));

    video_ci.dsp = rb->dsp_get_config(CODEC_IDX_AUDIO);
    video_ci.codec_get_buffer = audio_codec_get_buffer;
    video_ci.pcmbuf_insert = audio_pcm_insert;
    video_ci.set_elapsed = audio_set_elapsed;
    video_ci.read_filebuf = audio_read_filebuf;
    video_ci.request_buffer = audio_request_buffer;
    video_ci.advance_buffer = audio_advance_buffer;
    video_ci.seek_buffer = audio_seek_buffer;
    video_ci.seek_complete = audio_seek_complete;
    video_ci.set_offset = audio_set_offset;
    video_ci.configure = audio_configure;
    video_ci.get_command = audio_get_command;
    video_ci.loop_track = audio_loop_track;
    video_ci.strip_filesize = audio_strip_filesize;

#if defined(ARM_NEED_DIV0)
    video_ci.__div0 = rb->__div0;
#endif
    video_ci.sleep = rb->sleep;
    video_ci.yield = rb->yield;

#if NUM_CORES > 1
    video_ci.create_thread = rb->create_thread;
    video_ci.thread_thaw = rb->thread_thaw;
    video_ci.thread_wait = rb->thread_wait;
    video_ci.semaphore_init = rb->semaphore_init;
    video_ci.semaphore_wait = rb->semaphore_wait;
    video_ci.semaphore_release = rb->semaphore_release;
#endif

    video_ci.commit_dcache = rb->commit_dcache;
    video_ci.commit_discard_dcache = rb->commit_discard_dcache;
    video_ci.commit_discard_idcache = rb->commit_discard_idcache;
    video_ci.strcpy = rb->strcpy;
    video_ci.strlen = rb->strlen;
    video_ci.strcmp = rb->strcmp;
    video_ci.strcat = rb->strcat;
    video_ci.memset = rb->memset;
    video_ci.memcpy = rb->memcpy;
    video_ci.memmove = rb->memmove;
    video_ci.memcmp = rb->memcmp;
    video_ci.memchr = rb->memchr;
#if defined(DEBUG) || defined(SIMULATOR)
    video_ci.debugf = rb->debugf;
#endif
#ifdef ROCKBOX_HAS_LOGF
    video_ci.logf = rb->logf;
#endif
    video_ci.qsort = rb->qsort;

#ifdef RB_PROFILE
    video_ci.profile_thread = rb->profile_thread;
    video_ci.profstop = rb->profstop;
    video_ci.profile_func_enter = rb->profile_func_enter;
    video_ci.profile_func_exit = rb->profile_func_exit;
#endif

#ifdef HAVE_RECORDING
    video_ci.round_value_to_list32 = rb->round_value_to_list32;
#endif
    video_ci.panicf = rb->panicf;
}

static void audio_codec_worker(void)
{
    int status;
#ifdef HAVE_PRIORITY_SCHEDULING
    /* Match mpegplayer: core DSP/AAC yields often enough that the normal
     * codec priority can be starved by the video render loop. */
    int old_priority = rb->thread_set_priority(rb->thread_self(),
                                            PRIORITY_PLAYBACK - 4);
#endif

    status = rb->codec_load_file("aac", &video_ci);
    if (status >= 0)
        status = rb->codec_run_proc();
    rb->codec_close();
#ifdef HAVE_PRIORITY_SCHEDULING
    rb->thread_set_priority(rb->thread_self(), old_priority);
#endif
    if (status < 0 && audio_action != CODEC_ACTION_HALT)
        audio_has_error = true;
    audio_is_ready = true;
    audio_is_playing = false;
}

int video_audio_init(const char *filepath,
                     const struct mp4v_demux_res *demux,
                     void *codec_workspace, size_t workspace_size,
                     void *pcm_buffer, size_t pcm_buffer_size)
{
    size_t required_workspace = video_audio_workspace_size(demux);

    if (audio_initialized || filepath == NULL || demux == NULL ||
        codec_workspace == NULL || pcm_buffer == NULL ||
        required_workspace == 0 ||
        workspace_size < required_workspace ||
        demux->audio_format != MAKEFOURCC('m', 'p', '4', 'a') ||
        demux->audio_sample_rate == 0 || demux->audio_num_samples == 0)
        return -1;

    audio_fd = rb->open(filepath, O_RDONLY);
    if (audio_fd < 0)
        return -1;
    audio_file_size = rb->ffilesize(audio_fd);
    if (audio_file_size <= 0)
    {
        rb->close(audio_fd);
        audio_fd = -1;
        return -1;
    }

    audio_codec_workspace = codec_workspace;
    audio_codec_workspace_size = workspace_size;
    audio_file_buffer_offset = 0;
    audio_file_buffer_length = 0;
    audio_sample_rate = demux->audio_sample_rate;
    audio_sample_depth = 29;
    audio_stereo_mode = STEREO_NONINTERLEAVED;
    audio_action = CODEC_ACTION_NULL;
    audio_seek_target_ms = 0;
    audio_paused = false;
    audio_is_ready = false;
    audio_is_playing = false;
    audio_has_error = false;

    rb->memset(&video_id3, 0, sizeof(video_id3));
    video_id3.codectype = AFMT_MP4_AAC;
    video_id3.frequency = demux->audio_sample_rate;
    video_id3.lead_trim = demux->audio_lead_trim;

    audio_codec_api_init();
    video_ci.filesize = audio_file_size;
    video_ci.curpos = 0;
    video_ci.id3 = &video_id3;
    video_ci.audio_hid = -1;
    if (!video_pcm_init(audio_sample_rate, pcm_buffer, pcm_buffer_size))
    {
        rb->close(audio_fd);
        audio_fd = -1;
        audio_codec_workspace = NULL;
        audio_codec_workspace_size = 0;
        return -1;
    }
    audio_initialized = true;
    return 0;
}

void video_audio_play(void)
{
    if (!audio_initialized || audio_is_playing)
        return;
    audio_action = CODEC_ACTION_NULL;
    audio_paused = false;
    audio_is_playing = true;
    rb->codec_thread_do_callback(audio_codec_worker, NULL);
}

void video_audio_pause(void)
{
    audio_paused = true;
}

void video_audio_resume(void)
{
    audio_paused = false;
}

void video_audio_seek(uint32_t target_ms)
{
    if (!audio_initialized || !audio_is_playing)
        return;

    audio_is_ready = false;
    audio_seek_target_ms = target_ms;
    video_pcm_flush(target_ms, audio_sample_rate);
    audio_action = CODEC_ACTION_SEEK_TIME;
}

void video_audio_stop(void)
{
    if (!audio_initialized)
        return;

    audio_action = CODEC_ACTION_HALT;
    audio_paused = false;
    rb->codec_thread_do_callback(NULL, NULL);
    video_pcm_stop();
    if (audio_fd >= 0)
    {
        rb->close(audio_fd);
        audio_fd = -1;
    }
    audio_codec_workspace = NULL;
    audio_codec_workspace_size = 0;
    audio_initialized = false;
    audio_is_playing = false;
}

bool video_audio_ready(void)
{
    return audio_is_ready || audio_has_error;
}

bool video_audio_is_active(void)
{
    return audio_is_playing;
}

bool video_audio_failed(void)
{
    return audio_has_error;
}

#endif /* HAVE_HW_H264 */
