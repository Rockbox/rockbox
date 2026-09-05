/***************************************************************************
 * S5L8702 hardware H.264 player.
 *
 * This is deliberately a narrow production contract: non-fragmented MP4/M4V,
 * the measured iTunes 9.2.1 Constrained Baseline syntax (Level 1.3 or 3.0,
 * one or two advertised reference pictures and ordered slices),
 * <= 640x480, <= 30 fps, with optional AAC-LC audio.
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

#include "backlight.h"
#include "button.h"
#include "crc32.h"
#include "dir.h"
#include "file.h"
#include "font.h"
#include "kernel.h"
#include "lang.h"
#include "lcd.h"
#include "menu.h"
#include "misc.h"
#include "mp4_demux.h"
#include "pcmbuf.h"
#include "rbpaths.h"
#include "settings.h"
#include "sound.h"
#include "splash.h"
#include "system.h"
#include "video_audio.h"
#include "video_pcm.h"
#include "video_playback.h"
#include "hw_h264.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define VIDEO_MAX_WIDTH          640
#define VIDEO_MAX_HEIGHT         480
#define VIDEO_MAX_LEVEL          30
#define VIDEO_MAX_FPS            30
#define VIDEO_MAX_SAMPLES        400000u
#define VIDEO_MAX_AUDIO_SAMPLES  600000u
#define VIDEO_READ_BUFFER        (512u * 1024u)
#define VIDEO_OUTPUT_BUFFER      (LCD_WIDTH * LCD_HEIGHT * 3u / 2u)
#define VIDEO_PCM_BUFFER         (256u * 1024u)
#define VIDEO_OSD_SHOW_TICKS     (HZ * 4)
#define VIDEO_RESUME_MAGIC       0x48323634u
#define VIDEO_RESUME_FILE        PLUGIN_APPS_DATA_DIR \
                                 "/h264-resume-%08lx.dat"

enum video_input_action
{
    VIDEO_INPUT_NONE = 0,
    VIDEO_INPUT_EXIT,
    VIDEO_INPUT_USB,
    VIDEO_INPUT_MENU,
    VIDEO_INPUT_SEEK_BACK,
    VIDEO_INPUT_SEEK_FORWARD,
};

struct video_resume_record
{
    uint32_t magic;
    uint32_t path_crc;
    int32_t frame;
    int32_t total_frames;
};

struct video_pool
{
    uint8_t *cursor;
    uint8_t *end;
};

struct video_timing
{
    uint32_t run;
    uint32_t in_run;
    uint64_t ticks;
};

static void *video_pool_take(struct video_pool *pool, size_t size,
                             size_t alignment)
{
    uintptr_t current;
    uintptr_t aligned;

    if (pool == NULL || alignment == 0)
        return NULL;
    current = (uintptr_t)pool->cursor;
    aligned = (current + alignment - 1) & ~(uintptr_t)(alignment - 1);
    if (aligned < current || aligned > (uintptr_t)pool->end ||
        size > (size_t)(pool->end - (uint8_t *)aligned))
        return NULL;
    pool->cursor = (uint8_t *)aligned + size;
    return (void *)aligned;
}

static bool video_tables_sane(const struct mp4v_demux_res *demux)
{
    uint64_t timed_samples = 0;
    uint32_t i;
    bool video_sane =
        demux->num_samples > 0 &&
        demux->num_samples <= VIDEO_MAX_SAMPLES &&
        demux->num_stco > 0 &&
        demux->num_stco <= demux->num_samples &&
        demux->num_stsc > 0 &&
        demux->num_stsc <= demux->num_stco &&
        demux->num_stts > 0 &&
        demux->num_stss <= demux->num_samples &&
        demux->timescale > 0;
    bool audio_sane =
        (demux->audio_num_samples == 0 &&
         demux->audio_num_stco == 0 && demux->audio_num_stsc == 0) ||
        (demux->audio_num_samples > 0 &&
         demux->audio_num_samples <= VIDEO_MAX_AUDIO_SAMPLES &&
         demux->audio_num_stco > 0 &&
         demux->audio_num_stco <= demux->audio_num_samples &&
         demux->audio_num_stsc > 0 &&
         demux->audio_num_stsc <= demux->audio_num_stco);

    if (!video_sane || !audio_sane)
        return false;
    for (i = 0; i < demux->num_stts; i++)
    {
        if (demux->stts[i].sample_count == 0 ||
            demux->stts[i].sample_delta == 0 ||
            (uint64_t)demux->stts[i].sample_delta * VIDEO_MAX_FPS <
                demux->timescale)
            return false;
        timed_samples += demux->stts[i].sample_count;
        if (timed_samples > demux->num_samples)
            return false;
    }
    return timed_samples == demux->num_samples;
}

/* Validate the actual video mapping after the second, full table pass.  The
 * old independent stco/stsc caps rejected long files authored by iTunes,
 * whose normal layout is one video sample per chunk and may therefore have
 * hundreds of thousands of chunks.  The MP4 relationships themselves give
 * tighter bounds: every chunk contains samples and every stsc run starts at
 * a real chunk. */
static bool video_stsc_sane(const struct mp4v_demux_res *demux)
{
    uint64_t mapped_samples = 0;
    uint32_t i;

    if (demux->stsc == NULL || demux->num_stsc == 0 ||
        demux->stsc_cap < demux->num_stsc ||
        demux->sample_sizes == NULL ||
        demux->sample_sizes_cap < demux->num_samples ||
        demux->chunk_offsets == NULL ||
        demux->chunk_offsets_cap < demux->num_stco ||
        (demux->num_stss > 0 &&
         (demux->sync_samples == NULL ||
          demux->sync_samples_cap < demux->num_stss)))
        return false;
    for (i = 0; i < demux->num_stsc; i++)
    {
        const struct mp4v_stsc_entry *entry = &demux->stsc[i];
        uint32_t next_chunk = i + 1 < demux->num_stsc ?
            demux->stsc[i + 1].first_chunk : demux->num_stco + 1;

        if ((i == 0 && entry->first_chunk != 1) ||
            entry->first_chunk == 0 || entry->first_chunk > demux->num_stco ||
            next_chunk <= entry->first_chunk ||
            next_chunk > demux->num_stco + 1 ||
            entry->samples_per_chunk == 0 || entry->sample_desc_index == 0)
            return false;
        mapped_samples +=
            (uint64_t)(next_chunk - entry->first_chunk) *
            entry->samples_per_chunk;
        if (mapped_samples > demux->num_samples)
            return false;
    }
    if (mapped_samples != demux->num_samples)
        return false;
    for (i = 0; i < demux->num_stss; i++)
    {
        if (demux->sync_samples[i] == 0 ||
            demux->sync_samples[i] > demux->num_samples ||
            (i > 0 && demux->sync_samples[i] <=
                      demux->sync_samples[i - 1]))
            return false;
    }
    return true;
}

static uint32_t video_pts_ms(const struct mp4v_demux_res *demux,
                             const struct video_timing *timing)
{
    if (demux->timescale == 0)
        return 0;
    return (uint32_t)(timing->ticks * 1000u / demux->timescale);
}

static void video_timing_advance(const struct mp4v_demux_res *demux,
                                 struct video_timing *timing)
{
    const struct mp4v_stts_entry *entry;

    if (timing->run >= demux->num_stts)
        return;
    entry = &demux->stts[timing->run];
    timing->ticks += entry->sample_delta;
    timing->in_run++;
    if (timing->in_run >= entry->sample_count)
    {
        timing->run++;
        timing->in_run = 0;
    }
}

static uint32_t video_duration_ms(const struct mp4v_demux_res *demux)
{
    uint64_t ticks = 0;
    uint32_t i;

    if (demux->timescale == 0)
        return 0;
    for (i = 0; i < demux->num_stts; i++)
        ticks += (uint64_t)demux->stts[i].sample_count *
                 demux->stts[i].sample_delta;
    return (uint32_t)(ticks * 1000u / demux->timescale);
}

static void video_timing_for_sample(const struct mp4v_demux_res *demux,
                                    uint32_t sample,
                                    struct video_timing *timing)
{
    uint32_t remaining = sample;

    rb->memset(timing, 0, sizeof(*timing));
    while (timing->run < demux->num_stts)
    {
        const struct mp4v_stts_entry *entry =
            &demux->stts[timing->run];

        if (remaining < entry->sample_count)
        {
            timing->in_run = remaining;
            timing->ticks += (uint64_t)remaining * entry->sample_delta;
            return;
        }
        timing->ticks += (uint64_t)entry->sample_count *
                         entry->sample_delta;
        remaining -= entry->sample_count;
        timing->run++;
    }
}

static uint32_t video_sample_for_ms(const struct mp4v_demux_res *demux,
                                    uint32_t target_ms,
                                    struct video_timing *timing)
{
    uint64_t target_ticks;
    uint64_t ticks = 0;
    uint32_t sample = 0;
    uint32_t run;

    if (demux->timescale == 0)
    {
        rb->memset(timing, 0, sizeof(*timing));
        return 0;
    }
    target_ticks = (uint64_t)target_ms * demux->timescale / 1000u;
    for (run = 0; run < demux->num_stts; run++)
    {
        const struct mp4v_stts_entry *entry = &demux->stts[run];
        uint64_t run_ticks = (uint64_t)entry->sample_count *
                             entry->sample_delta;

        if (target_ticks < ticks + run_ticks)
        {
            uint32_t in_run = entry->sample_delta == 0 ? 0 :
                (uint32_t)((target_ticks - ticks) / entry->sample_delta);

            if (in_run >= entry->sample_count)
                in_run = entry->sample_count - 1;
            sample += in_run;
            break;
        }
        ticks += run_ticks;
        sample += entry->sample_count;
    }
    if (sample >= demux->num_samples)
        sample = demux->num_samples - 1;
    while (sample > 0 && !mp4v_is_keyframe(demux, sample))
        sample--;
    video_timing_for_sample(demux, sample, timing);
    return sample;
}

static void video_resume_filename(const char *path, char *filename,
                                  size_t size, uint32_t *crc_out)
{
    uint32_t crc = rb->crc_32(path, rb->strlen(path), 0xffffffff);

    rb->snprintf(filename, size, VIDEO_RESUME_FILE, (unsigned long)crc);
    if (crc_out != NULL)
        *crc_out = crc;
}

static uint32_t video_resume_load(const char *path, uint32_t total_samples)
{
    struct video_resume_record record;
    char filename[MAX_PATH];
    uint32_t crc;
    int fd;

    video_resume_filename(path, filename, sizeof(filename), &crc);
    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
        return 0;
    if (rb->read(fd, &record, sizeof(record)) != sizeof(record))
    {
        rb->close(fd);
        return 0;
    }
    rb->close(fd);
    if (record.magic != VIDEO_RESUME_MAGIC || record.path_crc != crc ||
        record.total_frames != (int32_t)total_samples || record.frame <= 0 ||
        record.frame >= (int32_t)(total_samples * 95u / 100u))
        return 0;
    return (uint32_t)record.frame;
}

static void video_resume_save(const char *path, uint32_t sample,
                              uint32_t total_samples)
{
    struct video_resume_record record;
    char filename[MAX_PATH];
    uint32_t crc;
    int fd;

    video_resume_filename(path, filename, sizeof(filename), &crc);
    if (sample == 0 || sample >= total_samples * 95u / 100u)
    {
        rb->remove(filename);
        return;
    }
    rb->mkdir(PLUGIN_APPS_DATA_DIR);
    record.magic = VIDEO_RESUME_MAGIC;
    record.path_crc = crc;
    record.frame = (int32_t)sample;
    record.total_frames = (int32_t)total_samples;
    fd = rb->open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return;
    (void)rb->write(fd, &record, sizeof(record));
    rb->close(fd);
}

static void video_resume_clear(const char *path)
{
    char filename[MAX_PATH];

    video_resume_filename(path, filename, sizeof(filename), NULL);
    rb->remove(filename);
}

static void video_volume_change(int delta)
{
    int minimum = rb->sound_min(SOUND_VOLUME);
    int maximum = rb->sound_max(SOUND_VOLUME);
    int volume = rb->global_status->volume + delta;

    if (volume < minimum)
        volume = minimum;
    if (volume > maximum)
        volume = maximum;
    if (volume != rb->global_status->volume)
        rb->sound_set(SOUND_VOLUME, volume);
}

static void video_set_paused(bool pause, bool *paused, long *start_tick,
                             long *pause_started, bool have_audio,
                             bool *cpu_boosted)
{
    if (pause == *paused)
        return;
    if (pause)
    {
        *paused = true;
        *pause_started = (*rb->current_tick);
        if (have_audio)
        {
            video_audio_pause();
            video_pcm_pause(true);
        }
        if (*cpu_boosted)
        {
            rb->cpu_boost(false);
            *cpu_boosted = false;
        }
    }
    else
    {
        if (!*cpu_boosted)
        {
            rb->cpu_boost(true);
            *cpu_boosted = true;
        }
        *paused = false;
        *start_tick += (*rb->current_tick) - *pause_started;
        if (have_audio)
        {
            video_audio_resume();
            video_pcm_pause(false);
        }
    }
}

static enum video_input_action video_input(bool *paused, long *start_tick,
                                           long *pause_started,
                                           bool have_audio,
                                           bool *cpu_boosted,
                                           long *overlay_until,
                                           bool *redraw)
{
    int action = rb->get_action(CONTEXT_WPS, TIMEOUT_NOBLOCK);

    *redraw = false;
    if (action == ACTION_NONE)
        return VIDEO_INPUT_NONE;
    if (action == ACTION_WPS_MENU)
        return VIDEO_INPUT_MENU;
    if (action == ACTION_WPS_STOP)
        return VIDEO_INPUT_EXIT;
    if (action == ACTION_WPS_VOLUP)
    {
        video_volume_change(1);
        *overlay_until = (*rb->current_tick) + VIDEO_OSD_SHOW_TICKS;
        *redraw = true;
        return VIDEO_INPUT_NONE;
    }
    if (action == ACTION_WPS_VOLDOWN)
    {
        video_volume_change(-1);
        *overlay_until = (*rb->current_tick) + VIDEO_OSD_SHOW_TICKS;
        *redraw = true;
        return VIDEO_INPUT_NONE;
    }
    if (action == ACTION_WPS_SKIPPREV || action == ACTION_WPS_SEEKBACK)
        return VIDEO_INPUT_SEEK_BACK;
    if (action == ACTION_WPS_SKIPNEXT || action == ACTION_WPS_SEEKFWD)
        return VIDEO_INPUT_SEEK_FORWARD;
    if (action == ACTION_WPS_PLAY)
    {
        video_set_paused(!*paused, paused, start_tick, pause_started,
                         have_audio, cpu_boosted);
        *overlay_until = (*rb->current_tick) + VIDEO_OSD_SHOW_TICKS;
        *redraw = true;
    }
    if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
        return VIDEO_INPUT_USB;
    return VIDEO_INPUT_NONE;
}

enum video_menu_item
{
    VIDEO_MENU_RESUME = 0,
    VIDEO_MENU_QUIT,
};

static enum video_input_action video_show_menu(
    bool *paused, long *start_tick, long *pause_started, bool have_audio,
    bool *cpu_boosted, long *overlay_until, bool *redraw)
{
    bool resume_after = !*paused;
    int selection;

    MENUITEM_STRINGLIST(menu, "H.264 Player", NULL,
                        ID2P(LANG_RESUME_PLAYBACK),
                        ID2P(LANG_MENU_QUIT));

    video_set_paused(true, paused, start_tick, pause_started, have_audio,
                     cpu_boosted);
    rb->button_clear_queue();
    selection = rb->do_menu(&menu, NULL, NULL, false);
    rb->button_clear_queue();
    if (selection == MENU_ATTACHED_USB)
        return VIDEO_INPUT_USB;
    if (selection == VIDEO_MENU_QUIT)
        return VIDEO_INPUT_EXIT;
    if (resume_after)
        video_set_paused(false, paused, start_tick, pause_started,
                         have_audio, cpu_boosted);
    *overlay_until = (*rb->current_tick) + VIDEO_OSD_SHOW_TICKS;
    *redraw = true;
    return VIDEO_INPUT_NONE;
}

static void video_scale_plane(uint8_t *destination, int destination_stride,
                              int destination_width, int destination_height,
                              const uint8_t *source, int source_stride,
                              int source_width, int source_height)
{
    uint32_t source_y = 0;
    uint32_t step_y;
    uint32_t step_x;
    int row;

    if (destination_width <= 0 || destination_height <= 0 ||
        source_width <= 0 || source_height <= 0)
        return;
    step_x = ((uint32_t)source_width << 16) / destination_width;
    step_y = ((uint32_t)source_height << 16) / destination_height;
    for (row = 0; row < destination_height; row++)
    {
        const uint8_t *source_row = source +
            (source_y >> 16) * source_stride;
        uint8_t *destination_row = destination +
            row * destination_stride;
        uint32_t source_x = 0;
        int column;

        for (column = 0; column < destination_width; column++)
        {
            destination_row[column] = source_row[source_x >> 16];
            source_x += step_x;
        }
        source_y += step_y;
    }
}

static uint8_t video_clamp_yuv(int value)
{
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return (uint8_t)value;
}

static void video_yuv_color(int red, int green, int blue,
                            uint8_t *y, uint8_t *u, uint8_t *v)
{
    *y = video_clamp_yuv(
        ((66 * red + 129 * green + 25 * blue + 128) >> 8) + 16);
    *u = video_clamp_yuv(
        ((-38 * red - 74 * green + 112 * blue + 128) >> 8) + 128);
    *v = video_clamp_yuv(
        ((112 * red - 94 * green - 18 * blue + 128) >> 8) + 128);
}

static void video_yuv_pixel(uint8_t * const *planes, int x, int y,
                            int red, int green, int blue)
{
    uint8_t py;
    uint8_t pu;
    uint8_t pv;

    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT)
        return;
    video_yuv_color(red, green, blue, &py, &pu, &pv);
    planes[0][y * LCD_WIDTH + x] = py;
    planes[1][(y / 2) * (LCD_WIDTH / 2) + x / 2] = pu;
    planes[2][(y / 2) * (LCD_WIDTH / 2) + x / 2] = pv;
}

static void video_yuv_rect(uint8_t * const *planes, int x, int y,
                           int width, int height,
                           int red, int green, int blue)
{
    int row;
    int column;

    for (row = MAX(0, y); row < MIN(LCD_HEIGHT, y + height); row++)
        for (column = MAX(0, x);
             column < MIN(LCD_WIDTH, x + width); column++)
            video_yuv_pixel(planes, column, row, red, green, blue);
}

static int video_yuv_text_width(const char *text)
{
    struct font *font = rb->font_get(FONT_SYSFIXED);
    int width = 0;

    if (font == NULL)
        return 0;
    while (*text != '\0')
        width += rb->font_get_width(font, (unsigned char)*text++);
    return width;
}

static void video_yuv_text(uint8_t * const *planes, int x, int y,
                           const char *text,
                           int red, int green, int blue)
{
    struct font *font = rb->font_get(FONT_SYSFIXED);

    if (font == NULL || font->depth != 0)
        return;
    while (*text != '\0' && x < LCD_WIDTH)
    {
        unsigned char ch = *text++;
        int glyph_width = rb->font_get_width(font, ch);
        const unsigned char *bits = rb->font_get_bits(font, ch);
        int column;

        for (column = 0; column < glyph_width; column++)
        {
            const unsigned char *source = bits + column;
            int row;

            for (row = 0; row < (int)font->height; row++)
                if (source[(row >> 3) * glyph_width] &
                    (1u << (row & 7)))
                    video_yuv_pixel(planes, x + column, y + row,
                                    red, green, blue);
        }
        x += glyph_width;
    }
}

static void video_time_text(uint32_t milliseconds, char *text, size_t size)
{
    uint32_t seconds = milliseconds / 1000u;

    if (seconds >= 3600u)
        rb->snprintf(text, size, "%lu:%02lu:%02lu",
                 (unsigned long)(seconds / 3600u),
                 (unsigned long)((seconds / 60u) % 60u),
                 (unsigned long)(seconds % 60u));
    else
        rb->snprintf(text, size, "%lu:%02lu",
                 (unsigned long)(seconds / 60u),
                 (unsigned long)(seconds % 60u));
}

static void video_draw_status_icon(uint8_t * const *planes, int x, int y,
                                   bool paused)
{
    int row;

    if (paused)
    {
        video_yuv_rect(planes, x, y, 2, 9, 255, 255, 255);
        video_yuv_rect(planes, x + 5, y, 2, 9, 255, 255, 255);
        return;
    }
    for (row = 0; row < 9; row++)
    {
        int width = row <= 4 ? row + 1 : 9 - row;

        video_yuv_rect(planes, x, y + row, width, 1, 255, 255, 255);
    }
}

static void video_draw_overlay(uint8_t * const *planes, bool paused,
                               uint32_t position_ms, uint32_t duration_ms,
                               bool visible)
{
    char current[20];
    char duration[20];
    char volume[20];
    char minimum_volume[20];
    const char *unit = rb->sound_unit(SOUND_VOLUME);
    int top = LCD_HEIGHT - 34;
    int bar_x = 2;
    int bar_y = top + 21;
    int bar_height = 8;
    int volume_right = LCD_WIDTH - 2;
    int volume_field_width;
    int volume_width;
    int space_width;
    int bar_width;
    int fill_width;
    int duration_width;

    if (!visible)
        return;

    video_time_text(position_ms, current, sizeof(current));
    video_time_text(duration_ms, duration, sizeof(duration));
    rb->snprintf(volume, sizeof(volume), "%d%s",
             rb->sound_val2phys(SOUND_VOLUME, rb->global_status->volume), unit);
    rb->snprintf(minimum_volume, sizeof(minimum_volume), "%d%s",
             rb->sound_val2phys(SOUND_VOLUME,
                                rb->sound_min(SOUND_VOLUME)), unit);
    volume_field_width = video_yuv_text_width(minimum_volume);
    volume_width = video_yuv_text_width(volume);
    space_width = video_yuv_text_width(" ");
    bar_width = volume_right - volume_field_width - space_width - bar_x;
    fill_width = duration_ms == 0 ? 0 :
        (int)((uint64_t)MIN(position_ms, duration_ms) *
              (bar_width - 2) / duration_ms);
    duration_width = video_yuv_text_width(duration);

    /* Match MPEG player's raised purple OSD and element placement. */
    video_yuv_rect(planes, 0, top, LCD_WIDTH, 34, 115, 117, 189);
    video_yuv_rect(planes, 0, top, LCD_WIDTH, 1, 220, 221, 239);
    video_yuv_rect(planes, 0, top + 1, LCD_WIDTH, 1, 159, 160, 210);
    video_yuv_rect(planes, 0, LCD_HEIGHT - 2, LCD_WIDTH, 1, 93, 95, 153);
    video_yuv_rect(planes, 0, LCD_HEIGHT - 1, LCD_WIDTH, 1, 57, 58, 94);
    video_yuv_text(planes, bar_x, top + 6, current, 255, 255, 255);
    video_yuv_text(planes, bar_x + bar_width - duration_width, top + 6,
                   duration, 255, 255, 255);
    video_draw_status_icon(planes,
                           bar_x + (bar_width - 7) / 2,
                           top + 5, paused);
    video_yuv_rect(planes, bar_x, bar_y, bar_width, bar_height,
                   255, 255, 255);
    video_yuv_rect(planes, bar_x + 1, bar_y + 1,
                   bar_width - 2, bar_height - 2, 0, 0, 0);
    video_yuv_rect(planes, bar_x + 1, bar_y + 1,
                   fill_width, bar_height - 2, 255, 255, 255);
    video_yuv_text(planes, volume_right - volume_width, bar_y,
                   volume, 255, 255, 255);
}

static void video_draw_frame(const uint8_t *y, const uint8_t *cb,
                             const uint8_t *cr, int width, int height,
                             int stride, uint8_t *scaled, bool paused,
                             uint32_t position_ms, uint32_t duration_ms,
                             bool overlay_visible)
{
    unsigned char *planes[3];
    int draw_width;
    int draw_height;
    int x;
    int y_pos;

    if (y == NULL || cb == NULL || cr == NULL || scaled == NULL ||
        width <= 0 || height <= 0 || stride < width)
        return;

    draw_width = LCD_WIDTH;
    draw_height = (int)((int64_t)height * LCD_WIDTH / width);
    if (draw_height > LCD_HEIGHT)
    {
        draw_height = LCD_HEIGHT;
        draw_width = (int)((int64_t)width * LCD_HEIGHT / height);
    }
    draw_width &= ~1;
    draw_height &= ~1;
    x = ((LCD_WIDTH - draw_width) / 2) & ~1;
    y_pos = ((LCD_HEIGHT - draw_height) / 2) & ~1;

    planes[0] = scaled;
    planes[1] = scaled + LCD_WIDTH * LCD_HEIGHT;
    planes[2] = planes[1] + LCD_WIDTH * LCD_HEIGHT / 4;
    rb->memset(planes[0], 16, LCD_WIDTH * LCD_HEIGHT);
    rb->memset(planes[1], 128, LCD_WIDTH * LCD_HEIGHT / 4);
    rb->memset(planes[2], 128, LCD_WIDTH * LCD_HEIGHT / 4);
    video_scale_plane(planes[0] + y_pos * LCD_WIDTH + x, LCD_WIDTH,
                      draw_width, draw_height, y, stride, width, height);
    video_scale_plane(planes[1] + (y_pos / 2) * (LCD_WIDTH / 2) + x / 2,
                      LCD_WIDTH / 2, draw_width / 2, draw_height / 2,
                      cb, stride / 2, width / 2, height / 2);
    video_scale_plane(planes[2] + (y_pos / 2) * (LCD_WIDTH / 2) + x / 2,
                      LCD_WIDTH / 2, draw_width / 2, draw_height / 2,
                      cr, stride / 2, width / 2, height / 2);
    video_draw_overlay(planes, paused, position_ms, duration_ms,
                       overlay_visible);
    rb->lcd_blit_yuv(planes, 0, 0, LCD_WIDTH, 0, 0,
                 LCD_WIDTH, LCD_HEIGHT);
}

static void video_redraw_frame(struct vpu_h264 *decoder, uint8_t *scaled,
                               bool paused, uint32_t position_ms,
                               uint32_t duration_ms, bool overlay_visible)
{
    const uint8_t *frame_y;
    const uint8_t *frame_cb;
    const uint8_t *frame_cr;
    int width;
    int height;
    int stride;

    rb->hw_h264->get_frame(decoder, &frame_y, &frame_cb, &frame_cr,
                       &width, &height, &stride);
    video_draw_frame(frame_y, frame_cb, frame_cr, width, height, stride,
                     scaled, paused, position_ms, duration_ms,
                     overlay_visible);
}

int video_h264_play(const char *filepath, void *buffer, size_t buffer_size)
{
    static struct mp4v_demux_res demux;
    static uint32_t probe_video_sample[1];
    static uint32_t probe_video_chunk[1];
    static uint32_t probe_video_sync[1];
    struct video_pool pool;
    struct video_timing timing;
    struct vpu_h264 *decoder = NULL;
    struct mp4v_stsc_entry *video_stsc;
    uint32_t *video_samples;
    uint32_t *video_chunks;
    uint32_t *video_sync;
    uint8_t *decoder_buffer;
    uint8_t *read_buffer;
    uint8_t *scale_buffer;
    void *audio_codec_workspace;
    void *pcm_buffer;
    size_t decoder_size;
    size_t audio_codec_workspace_size;
    int video_fd = -1;
    uint32_t duration_ms;
    uint32_t displayed_ms = 0;
    uint32_t sample = 0;
    uint32_t last_sample = 0;
    long start_tick;
    long pause_started = 0;
    long overlay_until;
    bool paused = false;
    bool cpu_boosted = false;
    bool have_audio = false;
    bool have_frame = false;
    bool overlay_drawn = false;
    bool audio_master = false;
    enum video_input_action action = VIDEO_INPUT_NONE;
    int result = -1;

    if (filepath == NULL || filepath[0] == '\0' || buffer == NULL ||
        buffer_size == 0)
        return -1;

    rb->splash(0, "Loading video...");
    rb->cpu_boost(true);
    cpu_boosted = true;
    rb->memset(&demux, 0, sizeof(demux));
    if (mp4v_demux_open(filepath, &demux,
                        probe_video_sample, 1,
                        probe_video_chunk, 1, NULL, 0,
                        probe_video_sync, 1) < 0)
    {
        rb->splash(HZ * 2, "Invalid MP4/M4V file");
        goto cleanup;
    }
    if (demux.format != MAKEFOURCC('a', 'v', 'c', '1') ||
        demux.avc_profile != 66 || demux.avc_level > VIDEO_MAX_LEVEL ||
        demux.width == 0 || demux.height == 0 ||
        demux.width > VIDEO_MAX_WIDTH || demux.height > VIDEO_MAX_HEIGHT ||
        demux.nalu_len_size < 1 || demux.nalu_len_size > 4)
    {
        rb->splash(HZ * 3, "Need H.264 Baseline <= L3.0\nMax 640x480");
        goto cleanup;
    }
    if (!video_tables_sane(&demux))
    {
        rb->splash(HZ * 3, "Movie tables exceed safe limit");
        goto cleanup;
    }

    pool.cursor = buffer;
    pool.end = (uint8_t *)buffer + buffer_size;
    video_samples = video_pool_take(
        &pool, demux.num_samples * sizeof(*video_samples), 32);
    video_chunks = video_pool_take(
        &pool, demux.num_stco * sizeof(*video_chunks), 32);
    video_stsc = video_pool_take(
        &pool, demux.num_stsc * sizeof(*video_stsc), 32);
    video_sync = demux.num_stss > 0 ? video_pool_take(
        &pool, demux.num_stss * sizeof(*video_sync), 32) : NULL;
    decoder_size = rb->hw_h264->buf_size(
        (demux.width + 15) & ~15, (demux.height + 15) & ~15);
    decoder_buffer = video_pool_take(&pool, decoder_size, 4096);
    read_buffer = video_pool_take(&pool, VIDEO_READ_BUFFER, 32);
    scale_buffer = video_pool_take(&pool, VIDEO_OUTPUT_BUFFER, 32);
    audio_codec_workspace_size = demux.audio_num_samples > 0 ?
        video_audio_workspace_size(&demux) : 0;
    audio_codec_workspace = audio_codec_workspace_size > 0 ?
        video_pool_take(&pool, audio_codec_workspace_size,
                        CACHEALIGN_SIZE) : NULL;
    pcm_buffer = audio_codec_workspace_size > 0 ?
        video_pool_take(&pool, VIDEO_PCM_BUFFER, CACHEALIGN_SIZE) : NULL;
    if (video_samples == NULL || video_chunks == NULL ||
        video_stsc == NULL ||
        (demux.num_stss > 0 && video_sync == NULL) ||
        decoder_buffer == NULL || read_buffer == NULL ||
        scale_buffer == NULL ||
        (demux.audio_num_samples > 0 &&
         (audio_codec_workspace == NULL || pcm_buffer == NULL)))
    {
        rb->splash(HZ * 2, "Not enough video memory");
        goto cleanup;
    }
    rb->splash(0, "Loading movie index...");
    if (mp4v_demux_open(filepath, &demux,
                        video_samples, demux.num_samples,
                        video_chunks, demux.num_stco,
                        video_stsc, demux.num_stsc,
                        video_sync, demux.num_stss) < 0 ||
        !video_tables_sane(&demux) || !video_stsc_sane(&demux))
    {
        rb->splash(HZ * 2, "MP4 table parse failed");
        goto cleanup;
    }
    duration_ms = video_duration_ms(&demux);
    sample = video_resume_load(filepath, demux.num_samples);
    while (sample > 0 && !mp4v_is_keyframe(&demux, sample))
        sample--;
    video_timing_for_sample(&demux, sample, &timing);

    /* Preparation, decode, and LCD presentation need the 108 MHz bus clock.
     * Keep one boost reference from table parsing through active playback. */
    rb->splash(0, "Starting video...");
    decoder = rb->hw_h264->open(
        decoder_buffer, decoder_size,
        (demux.width + 15) & ~15, (demux.height + 15) & ~15);
    if (decoder == NULL ||
        rb->hw_h264->configure(decoder, demux.codecdata,
                           demux.codecdata_len) < 0)
    {
        rb->splash(HZ * 2, "H.264 decoder init failed");
        goto cleanup;
    }
    video_fd = rb->open(filepath, O_RDONLY);
    if (video_fd < 0)
        goto cleanup;

    rb->pcmbuf_fade(false, true);
    rb->sound_set(SOUND_VOLUME, rb->global_status->volume);
    start_tick = (*rb->current_tick) -
        (long)((uint64_t)video_pts_ms(&demux, &timing) * HZ / 1000u);
    overlay_until = (*rb->current_tick) + VIDEO_OSD_SHOW_TICKS;
    if (demux.audio_format == MAKEFOURCC('m', 'p', '4', 'a') &&
        demux.audio_codecdata_len > 0 &&
        video_audio_init(filepath, &demux, audio_codec_workspace,
                         audio_codec_workspace_size, pcm_buffer,
                         VIDEO_PCM_BUFFER) == 0)
    {
        int wait = 0;
        uint32_t initial_ms = video_pts_ms(&demux, &timing);

        have_audio = true;
        audio_master = true;
        video_audio_play();
        if (initial_ms > 0)
            video_audio_seek(initial_ms);
        while (!video_audio_ready() && wait < HZ && action == VIDEO_INPUT_NONE)
        {
            bool redraw_requested;

            action = video_input(&paused, &start_tick,
                                 &pause_started, true, &cpu_boosted,
                                 &overlay_until, &redraw_requested);
            if (action == VIDEO_INPUT_MENU)
                action = video_show_menu(
                    &paused, &start_tick, &pause_started, true,
                    &cpu_boosted, &overlay_until, &redraw_requested);
            rb->sleep(1);
            wait++;
        }
        if (video_audio_failed())
        {
            video_audio_stop();
            have_audio = false;
            audio_master = false;
        }
    }

    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_update();
    rb->backlight_on();
    rb->backlight_set_timeout(0);
    while (sample < demux.num_samples && action == VIDEO_INPUT_NONE)
    {
        uint32_t offset;
        uint32_t size;
        uint32_t pts_ms = video_pts_ms(&demux, &timing);
        bool seek_requested = false;
        int decoded;

        while (action == VIDEO_INPUT_NONE)
        {
            uint32_t clock_ms;
            bool redraw_requested;

            action = video_input(&paused, &start_tick,
                                 &pause_started, have_audio, &cpu_boosted,
                                 &overlay_until, &redraw_requested);
            if (action == VIDEO_INPUT_MENU)
                action = video_show_menu(
                    &paused, &start_tick, &pause_started, have_audio,
                    &cpu_boosted, &overlay_until, &redraw_requested);
            if (redraw_requested && have_frame)
            {
                video_redraw_frame(decoder, scale_buffer, paused,
                                   displayed_ms, duration_ms, true);
                overlay_drawn = true;
            }
            if (action == VIDEO_INPUT_SEEK_BACK ||
                action == VIDEO_INPUT_SEEK_FORWARD)
            {
                int64_t target =
                    (int64_t)(audio_master ? video_pcm_get_clock_ms() :
                              pts_ms) +
                    (action == VIDEO_INPUT_SEEK_BACK ? -10000 : 10000);

                if (target < 0)
                    target = 0;
                if ((uint64_t)target >= duration_ms && duration_ms > 0)
                    target = duration_ms - 1;
                sample = video_sample_for_ms(&demux, (uint32_t)target,
                                             &timing);
                pts_ms = video_pts_ms(&demux, &timing);
                rb->hw_h264->close(decoder);
                decoder = rb->hw_h264->open(
                    decoder_buffer, decoder_size,
                    (demux.width + 15) & ~15,
                    (demux.height + 15) & ~15);
                if (decoder == NULL ||
                    rb->hw_h264->configure(decoder, demux.codecdata,
                                       demux.codecdata_len) < 0)
                {
                    rb->splash(HZ * 2, "H.264 seek failed");
                    decoder = NULL;
                    goto cleanup;
                }
                if (have_audio)
                    video_audio_seek(pts_ms);
                start_tick = (*rb->current_tick) -
                    (long)((uint64_t)pts_ms * HZ / 1000u);
                overlay_until = (*rb->current_tick) + VIDEO_OSD_SHOW_TICKS;
                action = VIDEO_INPUT_NONE;
                seek_requested = true;
                break;
            }
            if (action != VIDEO_INPUT_NONE)
                break;
            if (paused)
            {
                if (overlay_drawn &&
                    !TIME_BEFORE((*rb->current_tick), overlay_until))
                {
                    video_redraw_frame(decoder, scale_buffer, true,
                                       displayed_ms, duration_ms, false);
                    overlay_drawn = false;
                }
                rb->sleep(1);
                continue;
            }
            clock_ms = audio_master ? video_pcm_get_clock_ms() :
                (uint32_t)(((*rb->current_tick) - start_tick) * 1000 / HZ);
            if (audio_master &&
                (video_audio_failed() ||
                 (!video_audio_is_active() && video_pcm_empty())))
            {
                video_audio_stop();
                have_audio = false;
                audio_master = false;
                start_tick = (*rb->current_tick) -
                    (long)((uint64_t)clock_ms * HZ / 1000u);
            }
            if (clock_ms + 2 >= pts_ms)
                break;
            rb->sleep(1);
        }
        if (action != VIDEO_INPUT_NONE)
            break;
        if (seek_requested)
            continue;
        if (mp4v_get_sample_offset(&demux, sample, &offset, &size) < 0 ||
            size == 0 || size > VIDEO_READ_BUFFER ||
            rb->lseek(video_fd, offset, SEEK_SET) < 0 ||
            rb->read(video_fd, read_buffer, size) != (ssize_t)size)
        {
            rb->splash(HZ * 2, "Video sample read failed");
            goto cleanup;
        }
        decoded = rb->hw_h264->decode_sample(
            decoder, read_buffer, size, demux.nalu_len_size);
        if (decoded < 0)
        {
            rb->splash(HZ * 2, "VPU sample decode failed");
            goto cleanup;
        }
        if (decoded > 0)
        {
            const uint8_t *frame_y;
            const uint8_t *frame_cb;
            const uint8_t *frame_cr;
            int width;
            int height;
            int stride;
            bool overlay_visible =
                TIME_BEFORE((*rb->current_tick), overlay_until);

            rb->hw_h264->get_frame(decoder, &frame_y, &frame_cb, &frame_cr,
                               &width, &height, &stride);
            video_draw_frame(frame_y, frame_cb, frame_cr, width, height,
                             stride, scale_buffer, paused, pts_ms,
                             duration_ms, overlay_visible);
            displayed_ms = pts_ms;
            have_frame = true;
            overlay_drawn = overlay_visible;
        }
        last_sample = sample;
        video_timing_advance(&demux, &timing);
        sample++;
    }
    result = action == VIDEO_INPUT_USB ? 2 :
        (action == VIDEO_INPUT_EXIT ? 1 : 0);
    if (result == 0)
        video_resume_clear(filepath);
    else
        video_resume_save(filepath, last_sample, demux.num_samples);

cleanup:
    if (have_audio)
        video_audio_stop();
    if (video_fd >= 0)
        rb->close(video_fd);
    if (decoder != NULL)
        rb->hw_h264->close(decoder);
    rb->pcmbuf_fade(false, false);
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_update();
    if (cpu_boosted)
        rb->cpu_boost(false);
    return result;
}

#endif /* HAVE_HW_H264 */
