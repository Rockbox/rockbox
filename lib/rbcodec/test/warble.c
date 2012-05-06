/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2011 Sean Bartell
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

#define _BSD_SOURCE /* htole64 from endian.h */
#include <sys/types.h>
#include <SDL.h>
#include <dlfcn.h>
#include <endian.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "buffering.h" /* TYPE_PACKET_AUDIO */
#include "codecs.h"
#include "dsp_core.h"
#include "metadata.h"
#include "settings.h"
#include "sound.h"
#include "tdspeed.h"
#include "kernel.h"
#include "platform.h"

/***************** EXPORTED *****************/

struct user_settings global_settings;

int set_irq_level(int level)
{
    return 0;
}

void mutex_init(struct mutex *m)
{
}

void mutex_lock(struct mutex *m)
{
}

void mutex_unlock(struct mutex *m)
{
}

void debugf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

int find_first_set_bit(uint32_t value)
{
    if (value == 0)
        return 32;
    return __builtin_ctz(value);
}

/***************** INTERNAL *****************/

static enum { MODE_PLAY, MODE_WRITE } mode;
static bool use_dsp = true;
static bool enable_loop = false;
static const char *config = "";

static int input_fd;
static enum codec_command_action codec_action;
static intptr_t codec_action_param = 0;
static unsigned long num_output_samples = 0;
static struct codec_api ci;

static struct {
    intptr_t freq;
    intptr_t stereo_mode;
    intptr_t depth;
    int channels;
} format;

/***** MODE_WRITE *****/

#define WAVE_HEADER_SIZE 0x2e
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_IEEE_FLOAT 3
static int output_fd;
static bool write_raw = false;
static bool write_header_written = false;

static void write_init(const char *output_fn)
{
    mode = MODE_WRITE;
    if (!strcmp(output_fn, "-")) {
        output_fd = STDOUT_FILENO;
    } else {
        output_fd = creat(output_fn, 0666);
        if (output_fd == -1) {
            perror(output_fn);
            exit(1);
        }
    }
}

static void set_le16(char *buf, uint16_t val)
{
    buf[0] = val;
    buf[1] = val >> 8;
}

static void set_le32(char *buf, uint32_t val)
{
    buf[0] = val;
    buf[1] = val >> 8;
    buf[2] = val >> 16;
    buf[3] = val >> 24;
}

static void write_wav_header(void)
{
    int channels, sample_size, freq, type;
    if (use_dsp) {
        channels = 2;
        sample_size = 16;
        freq = NATIVE_FREQUENCY;
        type = WAVE_FORMAT_PCM;
    } else {
        channels = format.channels;
        sample_size = 64;
        freq = format.freq;
        type = WAVE_FORMAT_IEEE_FLOAT;
    }

    /* The size fields are normally overwritten by write_quit(). If that fails,
     * this fake size ensures the file can still be played. */
    off_t total_size = 0x7fffff00 + WAVE_HEADER_SIZE;
    char header[WAVE_HEADER_SIZE] = {"RIFF____WAVEfmt \x12\0\0\0"
                                     "________________\0\0data____"};
    set_le32(header + 0x04, total_size - 8);
    set_le16(header + 0x14, type);
    set_le16(header + 0x16, channels);
    set_le32(header + 0x18, freq);
    set_le32(header + 0x1c, freq * channels * sample_size / 8);
    set_le16(header + 0x20, channels * sample_size / 8);
    set_le16(header + 0x22, sample_size);
    set_le32(header + 0x2a, total_size - WAVE_HEADER_SIZE);
    write(output_fd, header, sizeof(header));
    write_header_written = true;
}

static void write_quit(void)
{
    if (!write_raw) {
        /* Write the correct size fields in the header. If lseek fails (e.g.
         * for a pipe) nothing is written. */
        off_t total_size = lseek(output_fd, 0, SEEK_CUR);
        if (total_size != (off_t)-1) {
            char buf[4];
            set_le32(buf, total_size - 8);
            lseek(output_fd, 4, SEEK_SET);
            write(output_fd, buf, 4);
            set_le32(buf, total_size - WAVE_HEADER_SIZE);
            lseek(output_fd, 0x2a, SEEK_SET);
            write(output_fd, buf, 4);
        }
    }
    if (output_fd != STDOUT_FILENO)
        close(output_fd);
}

static uint64_t make_float64(int32_t sample, int shift)
{
    /* TODO: be more portable */
    double val = ldexp(sample, -shift);
    return *(uint64_t*)&val;
}

static void write_pcm(int16_t *pcm, int count)
{
    if (!write_header_written)
        write_wav_header();
    int i;
    for (i = 0; i < 2 * count; i++)
        pcm[i] = htole16(pcm[i]);
    write(output_fd, pcm, 4 * count);
}

static void write_pcm_raw(int32_t *pcm, int count)
{
    if (write_raw) {
        write(output_fd, pcm, count * sizeof(*pcm));
    } else {
        if (!write_header_written)
            write_wav_header();
        int i;
        uint64_t buf[count];

        for (i = 0; i < count; i++)
            buf[i] = htole64(make_float64(pcm[i], format.depth));
        write(output_fd, buf, count * sizeof(*buf));
    }
}

/***** MODE_PLAY *****/

/* MODE_PLAY uses a double buffer: one half is read by the playback thread and
 * the other half is written to by the main thread. When a thread is done with
 * its current half, it waits for the other thread and then switches. The main
 * advantage of this method is its simplicity; the main disadvantage is that it
 * has long latency. ALSA buffer underruns still occur sometimes, but this is
 * SDL's fault. */

#define PLAYBACK_BUFFER_SIZE 0x10000
static bool playback_running = false;
static char playback_buffer[2][PLAYBACK_BUFFER_SIZE];
static int playback_play_ind, playback_decode_ind;
static int playback_play_pos, playback_decode_pos;
static SDL_sem *playback_play_sema, *playback_decode_sema;

static void playback_init(void)
{
    mode = MODE_PLAY;
    if (SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr, "error: Can't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    playback_play_ind = 1;
    playback_play_pos = PLAYBACK_BUFFER_SIZE;
    playback_decode_ind = 0;
    playback_decode_pos = 0;
    playback_play_sema = SDL_CreateSemaphore(0);
    playback_decode_sema = SDL_CreateSemaphore(0);
}

static void playback_callback(void *userdata, Uint8 *stream, int len)
{
    while (len > 0) {
        if (!playback_running && playback_play_ind == playback_decode_ind
                && playback_play_pos >= playback_decode_pos) {
            /* end of data */
            memset(stream, 0, len);
            SDL_SemPost(playback_play_sema);
            return;
        }
        if (playback_play_pos >= PLAYBACK_BUFFER_SIZE) {
            SDL_SemPost(playback_play_sema);
            SDL_SemWait(playback_decode_sema);
            playback_play_ind = !playback_play_ind;
            playback_play_pos = 0;
        }
        char *play_buffer = playback_buffer[playback_play_ind];
        int copy_len = MIN(len, PLAYBACK_BUFFER_SIZE - playback_play_pos);
        memcpy(stream, play_buffer + playback_play_pos, copy_len);
        len -= copy_len;
        stream += copy_len;
        playback_play_pos += copy_len;
    }
}

static void playback_start(void)
{
    playback_running = true;
    SDL_AudioSpec spec = {0};
    spec.freq = NATIVE_FREQUENCY;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = 0x400;
    spec.callback = playback_callback;
    spec.userdata = NULL;
    if (SDL_OpenAudio(&spec, NULL)) {
        fprintf(stderr, "error: Can't open SDL audio: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_PauseAudio(0);
}

static void playback_quit(void)
{
    if (!playback_running)
        playback_start();
    memset(playback_buffer[playback_decode_ind] + playback_decode_pos, 0,
           PLAYBACK_BUFFER_SIZE - playback_decode_pos);
    playback_running = false;
    SDL_SemPost(playback_decode_sema);
    SDL_SemWait(playback_play_sema);
    SDL_SemWait(playback_play_sema);
    SDL_Quit();
}

static void playback_pcm(int16_t *pcm, int count)
{
    const char *stream = (const char *)pcm;
    count *= 4;

    while (count > 0) {
        if (playback_decode_pos >= PLAYBACK_BUFFER_SIZE) {
            if (!playback_running)
                playback_start();
            SDL_SemPost(playback_decode_sema);
            SDL_SemWait(playback_play_sema);
            playback_decode_ind = !playback_decode_ind;
            playback_decode_pos = 0;
        }
        char *decode_buffer = playback_buffer[playback_decode_ind];
        int copy_len = MIN(count, PLAYBACK_BUFFER_SIZE - playback_decode_pos);
        memcpy(decode_buffer + playback_decode_pos, stream, copy_len);
        stream += copy_len;
        count -= copy_len;
        playback_decode_pos += copy_len;
    }
}

/***** ALL MODES *****/

static void perform_config(void)
{
    /* TODO: equalizer, etc. */
    while (config) {
        const char *name = config;
        const char *eq = strchr(config, '=');
        if (!eq)
            break;
        const char *val = eq + 1;
        const char *end = val + strcspn(val, ": \t\n");

        if (!strncmp(name, "wait=", 5)) {
            if (atoi(val) > num_output_samples)
                return;
        } else if (!strncmp(name, "dither=", 7)) {
            dsp_dither_enable(atoi(val) ? true : false);
        } else if (!strncmp(name, "halt=", 5)) {
            if (atoi(val))
                codec_action = CODEC_ACTION_HALT;
        } else if (!strncmp(name, "loop=", 5)) {
            enable_loop = atoi(val) != 0;
        } else if (!strncmp(name, "offset=", 7)) {
            ci.id3->offset = atoi(val);
        } else if (!strncmp(name, "rate=", 5)) {
            sound_set_pitch(atof(val) * PITCH_SPEED_100);
        } else if (!strncmp(name, "seek=", 5)) {
            codec_action = CODEC_ACTION_SEEK_TIME;
            codec_action_param = atoi(val);
        } else if (!strncmp(name, "tempo=", 6)) {
            dsp_set_timestretch(atof(val) * PITCH_SPEED_100);
#ifdef HAVE_SW_VOLUME_CONTROL
        } else if (!strncmp(name, "vol=", 4)) {
            global_settings.volume = atoi(val);
            dsp_callback(DSP_CALLBACK_SET_SW_VOLUME, 0);
#endif
        } else {
            fprintf(stderr, "error: unrecognized config \"%.*s\"\n",
                    (int)(eq - name), name);
            exit(1);
        }

        if (*end)
            config = end + 1;
        else
            config = NULL;
    }
}

static void *ci_codec_get_buffer(size_t *size)
{
    static char buffer[64 * 1024 * 1024];
    char *ptr = buffer;
    *size = sizeof(buffer);
    if ((intptr_t)ptr & (CACHEALIGN_SIZE - 1))
        ptr += CACHEALIGN_SIZE - ((intptr_t)ptr & (CACHEALIGN_SIZE - 1));
    return ptr;
}

static void ci_pcmbuf_insert(const void *ch1, const void *ch2, int count)
{
    num_output_samples += count;

    if (use_dsp) {
        struct dsp_buffer src;
        src.remcount = count;
        src.pin[0] = ch1;
        src.pin[1] = ch2;
        src.proc_mask = 0;
        while (1) {
            int out_count = MAX(count, 512);
            int16_t buf[2 * out_count];
            struct dsp_buffer dst;

            dst.remcount = 0;
            dst.p16out = buf;
            dst.bufcount = out_count;

            dsp_process(ci.dsp, &src, &dst);

            if (dst.remcount > 0) {
                if (mode == MODE_WRITE)
                    write_pcm(buf, dst.remcount);
                else if (mode == MODE_PLAY)
                    playback_pcm(buf, dst.remcount);
            } else if (src.remcount <= 0) {
                break;
            }
        }
    } else {
        /* Convert to 32-bit interleaved. */
        count *= format.channels;
        int i;
        int32_t buf[count];
        if (format.depth > 16) {
            if (format.stereo_mode == STEREO_NONINTERLEAVED) {
                for (i = 0; i < count; i += 2) {
                    buf[i+0] = ((int32_t*)ch1)[i/2];
                    buf[i+1] = ((int32_t*)ch2)[i/2];
                }
            } else {
                memcpy(buf, ch1, sizeof(buf));
            }
        } else {
            if (format.stereo_mode == STEREO_NONINTERLEAVED) {
                for (i = 0; i < count; i += 2) {
                    buf[i+0] = ((int16_t*)ch1)[i/2];
                    buf[i+1] = ((int16_t*)ch2)[i/2];
                }
            } else {
                for (i = 0; i < count; i++) {
                    buf[i] = ((int16_t*)ch1)[i];
                }
            }
        }

        if (mode == MODE_WRITE)
            write_pcm_raw(buf, count);
    }

    perform_config();
}

static void ci_set_elapsed(unsigned long value)
{
    //debugf("Time elapsed: %lu\n", value);
}

static char *input_buffer = 0;

/*
 * Read part of the input file into a provided buffer.
 *
 * The entire size requested will be provided except at the end of the file.
 * The current file position will be moved, just like with advance_buffer, but
 * the offset is not updated. This invalidates buffers returned by
 * request_buffer.
 */
static size_t ci_read_filebuf(void *ptr, size_t size)
{
    free(input_buffer);
    input_buffer = NULL;

    ssize_t actual = read(input_fd, ptr, size);
    if (actual < 0)
        actual = 0;
    ci.curpos += actual;
    return actual;
}

/*
 * Request a buffer containing part of the input file.
 *
 * The size provided will be the requested size, or the remaining size of the
 * file, whichever is smaller. Packet audio has an additional maximum of 32
 * KiB. The returned buffer remains valid until the next time read_filebuf,
 * request_buffer, advance_buffer, or seek_buffer is called.
 */
static void *ci_request_buffer(size_t *realsize, size_t reqsize)
{
    free(input_buffer);
    if (!rbcodec_format_is_atomic(ci.id3->codectype))
        reqsize = MIN(reqsize, 32 * 1024);
    input_buffer = malloc(reqsize);
    *realsize = read(input_fd, input_buffer, reqsize);
    if (*realsize < 0)
        *realsize = 0;
    lseek(input_fd, -*realsize, SEEK_CUR);
    return input_buffer;
}

/*
 * Advance the current position in the input file.
 *
 * This automatically updates the current offset. This invalidates buffers
 * returned by request_buffer.
 */
static void ci_advance_buffer(size_t amount)
{
    free(input_buffer);
    input_buffer = NULL;

    lseek(input_fd, amount, SEEK_CUR);
    ci.curpos += amount;
    ci.id3->offset = ci.curpos;
}

/*
 * Seek to a position in the input file.
 *
 * This invalidates buffers returned by request_buffer.
 */
static bool ci_seek_buffer(size_t newpos)
{
    free(input_buffer);
    input_buffer = NULL;

    off_t actual = lseek(input_fd, newpos, SEEK_SET);
    if (actual >= 0)
        ci.curpos = actual;
    return actual != -1;
}

static void ci_seek_complete(void)
{
}

static void ci_set_offset(size_t value)
{
    ci.id3->offset = value;
}

static void ci_configure(int setting, intptr_t value)
{
    if (use_dsp) {
        dsp_configure(ci.dsp, setting, value);
    } else {
        if (setting == DSP_SET_FREQUENCY
                || setting == DSP_SWITCH_FREQUENCY)
            format.freq = value;
        else if (setting == DSP_SET_SAMPLE_DEPTH)
            format.depth = value;
        else if (setting == DSP_SET_STEREO_MODE) {
            format.stereo_mode = value;
            format.channels = (value == STEREO_MONO) ? 1 : 2;
        }
    }
}

static enum codec_command_action ci_get_command(intptr_t *param)
{
    enum codec_command_action ret = codec_action;
    *param = codec_action_param;
    codec_action = CODEC_ACTION_NULL;
    return ret;
}

static bool ci_should_loop(void)
{
    return enable_loop;
}

static unsigned ci_sleep(unsigned ticks)
{
    return 0;
}

static void ci_debugf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#ifdef ROCKBOX_HAS_LOGF
static void ci_logf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    putc('\n', stderr);
    va_end(ap);
}
#endif

static void ci_yield(void)
{
}

static void commit_dcache(void) {}
static void commit_discard_dcache(void) {}
static void commit_discard_idcache(void) {}

static struct codec_api ci = {

    0,                   /* filesize */
    0,                   /* curpos */
    NULL,                /* id3 */
    -1,                  /* audio_hid */
    NULL,                /* struct dsp_config *dsp */
    ci_codec_get_buffer,
    ci_pcmbuf_insert,
    ci_set_elapsed,
    ci_read_filebuf,
    ci_request_buffer,
    ci_advance_buffer,
    ci_seek_buffer,
    ci_seek_complete,
    ci_set_offset,
    ci_configure,
    ci_get_command,
    ci_should_loop,

    ci_sleep,
    ci_yield,

#if NUM_CORES > 1
    ci_create_thread,
    ci_thread_thaw,
    ci_thread_wait,
    ci_semaphore_init,
    ci_semaphore_wait,
    ci_semaphore_release,
#endif

    commit_dcache,
    commit_discard_dcache,
    commit_discard_idcache,

    /* strings and memory */
    strcpy,
    strlen,
    strcmp,
    strcat,
    memset,
    memcpy,
    memmove,
    memcmp,
    memchr,
#if defined(DEBUG) || defined(SIMULATOR)
    ci_debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    ci_logf,
#endif

    qsort,

#ifdef HAVE_RECORDING
    ci_enc_get_inputs,
    ci_enc_set_parameters,
    ci_enc_get_chunk,
    ci_enc_finish_chunk,
    ci_enc_get_pcm_data,
    ci_enc_unget_pcm_data,

    /* file */
    open,
    close,
    read,
    lseek,
    write,
    ci_round_value_to_list32,

#endif /* HAVE_RECORDING */
};

static void print_mp3entry(const struct mp3entry *id3, FILE *f)
{
    fprintf(f, "Path: %s\n", id3->path);
    if (id3->title) fprintf(f, "Title: %s\n", id3->title);
    if (id3->artist) fprintf(f, "Artist: %s\n", id3->artist);
    if (id3->album) fprintf(f, "Album: %s\n", id3->album);
    if (id3->genre_string) fprintf(f, "Genre: %s\n", id3->genre_string);
    if (id3->disc_string || id3->discnum) fprintf(f, "Disc: %s (%d)\n", id3->disc_string, id3->discnum);
    if (id3->track_string || id3->tracknum) fprintf(f, "Track: %s (%d)\n", id3->track_string, id3->tracknum);
    if (id3->year_string || id3->year) fprintf(f, "Year: %s (%d)\n", id3->year_string, id3->year);
    if (id3->composer) fprintf(f, "Composer: %s\n", id3->composer);
    if (id3->comment) fprintf(f, "Comment: %s\n", id3->comment);
    if (id3->albumartist) fprintf(f, "Album artist: %s\n", id3->albumartist);
    if (id3->grouping) fprintf(f, "Grouping: %s\n", id3->grouping);
    if (id3->layer) fprintf(f, "Layer: %d\n", id3->layer);
    if (id3->id3version) fprintf(f, "ID3 version: %u\n", (int)id3->id3version);
    fprintf(f, "Codec: %s\n", audio_formats[id3->codectype].label);
    fprintf(f, "Bitrate: %d kb/s\n", id3->bitrate);
    fprintf(f, "Frequency: %lu Hz\n", id3->frequency);
    if (id3->id3v2len) fprintf(f, "ID3v2 length: %lu\n", id3->id3v2len);
    if (id3->id3v1len) fprintf(f, "ID3v1 length: %lu\n", id3->id3v1len);
    if (id3->first_frame_offset) fprintf(f, "First frame offset: %lu\n", id3->first_frame_offset);
    fprintf(f, "File size without headers: %lu\n", id3->filesize);
    fprintf(f, "Song length: %lu ms\n", id3->length);
    if (id3->lead_trim > 0 || id3->tail_trim > 0) fprintf(f, "Trim: %d/%d\n", id3->lead_trim, id3->tail_trim);
    if (id3->samples) fprintf(f, "Number of samples: %lu\n", id3->samples);
    if (id3->frame_count) fprintf(f, "Number of frames: %lu\n", id3->frame_count);
    if (id3->bytesperframe) fprintf(f, "Bytes per frame: %lu\n", id3->bytesperframe);
    if (id3->vbr) fprintf(f, "VBR: true\n");
    if (id3->has_toc) fprintf(f, "Has TOC: true\n");
    if (id3->channels) fprintf(f, "Number of channels: %u\n", id3->channels);
    if (id3->extradata_size) fprintf(f, "Size of extra data: %u\n", id3->extradata_size);
    if (id3->needs_upsampling_correction) fprintf(f, "Needs upsampling correction: true\n");
    /* TODO: replaygain; albumart; cuesheet */
    if (id3->mb_track_id) fprintf(f, "Musicbrainz track ID: %s\n", id3->mb_track_id);
}

static void decode_file(const char *input_fn)
{
    /* Initialize DSP before any sort of interaction */
    dsp_init();

    /* Set up global settings */
    memset(&global_settings, 0, sizeof(global_settings));
    global_settings.timestretch_enabled = true;
    dsp_timestretch_enable(true);

    /* Open file */
    if (!strcmp(input_fn, "-")) {
        input_fd = STDIN_FILENO;
    } else {
        input_fd = open(input_fn, O_RDONLY);
        if (input_fd == -1) {
            perror(input_fn);
            exit(1);
        }
    }

    /* Set up ci */
    struct mp3entry id3;
    if (!get_metadata(&id3, input_fd, input_fn)) {
        fprintf(stderr, "error: metadata parsing failed\n");
        exit(1);
    }
    print_mp3entry(&id3, stderr);
    ci.filesize = filesize(input_fd);
    ci.id3 = &id3;
    if (use_dsp) {
        ci.dsp = dsp_get_config(CODEC_IDX_AUDIO);
        dsp_configure(ci.dsp, DSP_RESET, 0);
        dsp_dither_enable(false);
    }
    perform_config();

    /* Load codec */
    char str[MAX_PATH];
    snprintf(str, sizeof(str), CODECDIR"/%s.codec", audio_formats[id3.codectype].codec_root_fn);
    debugf("Loading %s\n", str);
    void *dlcodec = dlopen(str, RTLD_NOW);
    if (!dlcodec) {
        fprintf(stderr, "error: dlopen failed: %s\n", dlerror());
        exit(1);
    }
    struct codec_header *c_hdr = NULL;
    c_hdr = dlsym(dlcodec, "__header");
    if (c_hdr->lc_hdr.magic != CODEC_MAGIC) {
        fprintf(stderr, "error: %s invalid: incorrect magic\n", str);
        exit(1);
    }
    if (c_hdr->lc_hdr.target_id != TARGET_ID) {
        fprintf(stderr, "error: %s invalid: incorrect target id\n", str);
        exit(1);
    }
    if (c_hdr->lc_hdr.api_version != CODEC_API_VERSION) {
        fprintf(stderr, "error: %s invalid: incorrect API version\n", str);
        exit(1);
    }

    /* Run the codec */
    *c_hdr->api = &ci;
    if (c_hdr->entry_point(CODEC_LOAD) != CODEC_OK) {
        fprintf(stderr, "error: codec returned error from codec_main\n");
        exit(1);
    }
    if (c_hdr->run_proc() != CODEC_OK) {
        fprintf(stderr, "error: codec error\n");
    }
    c_hdr->entry_point(CODEC_UNLOAD);

    /* Close */
    dlclose(dlcodec);
    if (input_fd != STDIN_FILENO)
        close(input_fd);
}

static void print_help(const char *progname)
{
    fprintf(stderr, "Usage:\n"
                    "        Play: %s [options] INPUTFILE\n"
                    "Write to WAV: %s [options] INPUTFILE OUTPUTFILE\n"
                    "\n"
                    "general options:\n"
                    "  -c a=1:b=2    Configuration (see below)\n"
                    "  -h            Show this help\n"
                    "\n"
                    "write to WAV options:\n"
                    "  -f            Write raw codec output converted to 64-bit float\n"
                    "  -r            Write raw 32-bit codec output without WAV header\n"
                    "\n"
                    "configuration:\n"
                    "  dither=<0|1>  Enable/disable dithering [0]\n"
                    "  halt=<0|1>    Stop decoding if 1 [0]\n"
                    "  loop=<0|1>    Enable/disable looping [0]\n"
                    "  offset=<n>    Start at byte offset within the file [0]\n"
                    "  rate=<n>      Multiply rate by <n> [1.0]\n"
                    "  seek=<n>      Seek <n> ms into the file\n"
                    "  tempo=<n>     Timestretch by <n> [1.0]\n"
#ifdef HAVE_SW_VOLUME_CONTROL
                    "  vol=<n>       Set volume to <n> dB [0]\n"
#endif
                    "  wait=<n>      Don't apply remaining configuration until\n"
                    "                <n> total samples have output\n"
                    "\n"
                    "examples:\n"
                    "  # Play while looping; stop after 44100 output samples\n"
                    "  %s in.adx -c loop=1:wait=44100:halt=1\n"
                    "  # Lower pitch 1 octave and write to out.wav\n"
                    "  %s in.ogg -c rate=0.5:tempo=2 out.wav\n"
                    , progname, progname, progname, progname);
}

int main(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "c:fhr")) != -1) {
        switch (opt) {
        case 'c':
            config = optarg;
            break;
        case 'f':
            use_dsp = false;
            break;
        case 'r':
            use_dsp = false;
            write_raw = true;
            break;
        case 'h': /* fallthrough */
        default:
            print_help(argv[0]);
            exit(1);
        }
    }

    if (argc == optind + 2) {
        write_init(argv[optind + 1]);
    } else if (argc == optind + 1) {
        if (!use_dsp) {
            fprintf(stderr, "error: -r can't be used for playback\n");
            print_help(argv[0]);
            exit(1);
        }
        playback_init();
    } else {
        if (argc > 1)
            fprintf(stderr, "error: wrong number of arguments\n");
        print_help(argv[0]);
        exit(1);
    }

    decode_file(argv[optind]);

    if (mode == MODE_WRITE)
        write_quit();
    else if (mode == MODE_PLAY)
        playback_quit();

    return 0;
}
