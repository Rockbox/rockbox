/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Martin Cai
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
#include "codeclib.h"
#include "dst_decoder.h"

CODEC_HEADER

#define DSF_BLOCK 4096
#define PCM_FRAMES 512
#define ISO_SECTOR 2048

#if (ARCH == ARCH_M68K) || (CONFIG_CPU == PP5020) || (CONFIG_CPU == PP5002) || defined(CPU_TCC780X)
#define IBSS_ATTR_LARGE
#else
#define IBSS_ATTR_LARGE IBSS_ATTR
#endif

static int32_t pcm[PCM_FRAMES * 2] IBSS_ATTR;  /* 4KB */
static unsigned char left_block[DSF_BLOCK];
static unsigned char right_block[DSF_BLOCK];
static unsigned char iso_sector[ISO_SECTOR];
static unsigned char dst_frame[65536 + 8]; /* 64KB */
static unsigned char dst_output[DST_FRAME_BYTES * 2] IBSS_ATTR_LARGE; /* 9K */
static struct dst_decoder dst_state IBSS_ATTR; /* 2K */
static int16_t cic_table[2][256][3] IBSS_ATTR; /* 3K */

struct cic_state
{
    uint64_t integrator[3];
    uint64_t delay[3];
};

static struct cic_state left_cic;
static struct cic_state right_cic;

static uint32_t le32(const unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t le64(const unsigned char *p)
{
    return le32(p) | ((uint64_t)le32(p + 4) << 32);
}

static uint64_t be64(const unsigned char *p)
{
    uint64_t hi = ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    uint64_t lo = ((uint32_t)p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
    return (hi << 32) | lo;
}

static uint32_t be32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static void reset_filter(void)
{
    ci->memset(&left_cic, 0, sizeof(left_cic));
    ci->memset(&right_cic, 0, sizeof(right_cic));
}

static void init_cic_table(void)
{
    for (int order = 0; order < 2; ++order)
    {
        for (int value = 0; value < 256; ++value)
        {
            int sum = 0;
            int first = 0;
            int second = 0;
            for (int bit = 0; bit < 8; ++bit)
            {
                int shift = order ? bit : 7 - bit;
                int sample = (value & (1 << shift)) ? 1 : -1;
                int remaining = 8 - bit;
                sum += sample;
                first += remaining * sample;
                second += remaining * (remaining + 1) / 2 * sample;
            }
            cic_table[order][value][0] = sum;
            cic_table[order][value][1] = first;
            cic_table[order][value][2] = second;
        }
    }
}

static int32_t __attribute__((optimize("O3")))
decimate64(struct cic_state *state, const unsigned char *p, bool lsb_first)
{
    for (int byte = 0; byte < 8; ++byte)
    {
        const int16_t *entry = cic_table[lsb_first][p[byte]];
        uint64_t first = state->integrator[0];
        uint64_t second = state->integrator[1];
        state->integrator[2] += second * 8 + first * 36 + entry[2];
        state->integrator[1] += first * 8 + entry[1];
        state->integrator[0] += entry[0];
    }

    uint64_t value = state->integrator[2];
    for (int stage = 0; stage < 3; ++stage)
    {
        uint64_t next = value - state->delay[stage];
        state->delay[stage] = value;
        value = next;
    }
    return (int32_t)(uint32_t)value * 1024;
}

static bool read_exact(void *dst, size_t size)
{
    return ci->read_filebuf(dst, size) == size;
}

static bool valid_dsd_rate(uint32_t rate)
{
    //No validation for now...
    //dsd64 (2822400), dsd128(5644800), dsd256(11289600)...
    (void)rate;
    return true;
}

static bool parse_dsf(uint64_t *data_size, uint64_t *sample_count,
                      bool *lsb_first)
{
    unsigned char h[92];
    if (!ci->seek_buffer(0) || !read_exact(h, sizeof(h)) ||
        ci->memcmp(h, "DSD ", 4) || ci->memcmp(h + 28, "fmt ", 4) ||
        le32(h + 52) != 2 || !valid_dsd_rate(le32(h + 56)) ||
        (le32(h + 60) != 1 && le32(h + 60) != 8) ||
        le32(h + 72) != DSF_BLOCK ||
        ci->memcmp(h + 80, "data", 4))
        return false;
    uint64_t chunk_size = le64(h + 84);
    if (chunk_size < 12)
        return false;
    *data_size = chunk_size - 12;
    *sample_count = le64(h + 64);
    *lsb_first = le32(h + 60) == 1;
    return true;
}

static enum codec_status decode_dsf(void)
{
    uint64_t remaining;
    uint64_t sample_count;
    bool lsb_first;
    if (!parse_dsf(&remaining, &sample_count, &lsb_first))
        return CODEC_ERROR;

    uint64_t total = remaining;
    uint64_t total_frames = sample_count / 64;
    uint64_t frames_done = 0;

    intptr_t param = ci->id3->elapsed;
    long action = CODEC_ACTION_NULL;

    if (param) {
        action = CODEC_ACTION_SEEK_TIME;
        goto seek;
    } else if (ci->id3->offset) {
        /* TODO: Work out target param */
    }

    reset_filter();

    while (remaining >= DSF_BLOCK * 2 && frames_done < total_frames)
    {
        action = ci->get_command(&param);
        if (action == CODEC_ACTION_HALT)
            break;
    seek:
        if (action == CODEC_ACTION_SEEK_TIME)
        {
            uint64_t target_frame = MIN((uint64_t)param * ci->id3->frequency / 1000,
                                        total_frames);
            uint64_t block = target_frame / PCM_FRAMES;
            if (!ci->seek_buffer(92 + block * DSF_BLOCK * 2))
                return CODEC_ERROR;
            frames_done = block * PCM_FRAMES;
            remaining = total - MIN(total, block * DSF_BLOCK * 2);
            reset_filter();
            ci->seek_complete();
        }
        if (!read_exact(left_block, DSF_BLOCK) || !read_exact(right_block, DSF_BLOCK))
            break;
        int frames = MIN((uint64_t)PCM_FRAMES, total_frames - frames_done);
        for (int i = 0; i < frames; ++i)
        {
            pcm[i * 2] = decimate64(&left_cic, left_block + i * 8,
                                    lsb_first);
            pcm[i * 2 + 1] = decimate64(&right_cic, right_block + i * 8,
                                        lsb_first);
        }
        ci->pcmbuf_insert(pcm, NULL, frames);
        frames_done += frames;
        remaining -= DSF_BLOCK * 2;
        ci->set_elapsed(frames_done * 1000 / ci->id3->frequency);
        ci->set_offset((size_t) ci->curpos);
    }
    return CODEC_OK;
}

static enum codec_status decode_dff(void)
{
    unsigned char head[16];
    unsigned char chunk[12];
    uint64_t data_size = 0;
    uint64_t total_size = 0;
    size_t data_start = 0;

    intptr_t param = ci->id3->elapsed;
    long action = CODEC_ACTION_NULL;

    if (!ci->seek_buffer(0) || !read_exact(head, sizeof(head)) ||
        ci->memcmp(head, "FRM8", 4) || ci->memcmp(head + 12, "DSD ", 4))
        return CODEC_ERROR;

    const uint32_t pcm_rate = ci->id3->frequency;
    while (read_exact(chunk, sizeof(chunk)))
    {
        uint64_t size = be64(chunk + 4);
        size_t data_start_pos = ci->curpos;
        if (size > (uint64_t)ci->filesize - data_start_pos)
            return CODEC_ERROR;
        if (!ci->memcmp(chunk, "PROP", 4) && size >= 4)
        {
            unsigned char type[4];
            if (!read_exact(type, 4) || ci->memcmp(type, "SND ", 4))
                return CODEC_ERROR;
            continue;
        }
        else if (!ci->memcmp(chunk, "DSD ", 4))
        {
            data_size = size;
            total_size = size;
            data_start = ci->curpos;
            break;
        }
        //can be used to validate dsd rate
/*        else if (!ci->memcmp(chunk, "FS  ", 4) && size >= 4)
        {
            unsigned char value[4];
            if (!read_exact(value, sizeof(value)))
                return CODEC_ERROR;
            rate = be32(value);
            continue;
        }
*/
        if (!ci->memcmp(chunk, "DST ", 4) ||
            !ci->seek_buffer(data_start_pos + size + (size & 1)))
            return CODEC_ERROR;
    }
    if (!data_size)
        return CODEC_ERROR;

    reset_filter();
    uint64_t frames_done = 0;
    unsigned char packed[16];

    if (param) {
        action = CODEC_ACTION_SEEK_TIME;
        goto seek;
    } else if (ci->id3->offset) {
        /* TODO: Work out target param */
    }

    while (data_size >= sizeof(packed))
    {
        action  = ci->get_command(&param);
        if (action == CODEC_ACTION_HALT)
            break;
    seek:
        if (action == CODEC_ACTION_SEEK_TIME)
        {
            uint64_t frame = (uint64_t)param * pcm_rate / 1000;
            uint64_t offset = MIN(total_size, frame * 16);

            offset -= offset % 16;
            if (!ci->seek_buffer(data_start + offset))
                return CODEC_ERROR;
            data_size = total_size - offset;
            frames_done = offset / 16;
            reset_filter();
            ci->seek_complete();
        }
        int frames = 0;
        while (frames < PCM_FRAMES && data_size >= sizeof(packed) &&
               read_exact(packed, sizeof(packed)))
        {
            for (int i = 0; i < 8; ++i)
            {
                left_block[i] = packed[i * 2];
                right_block[i] = packed[i * 2 + 1];
            }
            pcm[frames * 2] = decimate64(&left_cic, left_block, false);
            pcm[frames * 2 + 1] = decimate64(&right_cic, right_block, false);
            ++frames;
            data_size -= sizeof(packed);
        }
        ci->pcmbuf_insert(pcm, NULL, frames);
        frames_done += frames;
        ci->set_elapsed(frames_done * 1000 / pcm_rate);
        ci->set_offset((size_t) ci->curpos);
    }
    return CODEC_OK;
}

static bool read_iso_sector(uint32_t lsn, unsigned char *sector)
{
    return ci->seek_buffer((size_t)lsn * ISO_SECTOR) &&
           read_exact(sector, ISO_SECTOR);
}

static bool find_iso_area(uint32_t *start, uint32_t *end,
                          unsigned long *duration_ms, bool *dst_encoded)
{
    if (!read_iso_sector(510, iso_sector) ||
        ci->memcmp(iso_sector, "SACDMTOC", 8))
        return false;

    uint32_t areas[2] = { be32(iso_sector + 64), be32(iso_sector + 72) };
    for (int i = 0; i < 2; ++i)
    {
        if (!areas[i] || !read_iso_sector(areas[i], iso_sector) ||
            ci->memcmp(iso_sector, "TWOCHTOC", 8) || iso_sector[32] != 2)
            continue;
        *dst_encoded = (iso_sector[21] & 0x0f) == 0;
        *start = be32(iso_sector + 72);
        *end = be32(iso_sector + 76);
        *duration_ms = (iso_sector[64] * 60ul + iso_sector[65]) * 1000ul +
                       iso_sector[66] * 1000ul / 75;
        return *start && *end >= *start && *duration_ms;
    }
    return false;
}

static int output_dsd(const unsigned char *data, size_t size, int *frames)
{
    size &= ~(size_t)15;
    for (size_t offset = 0; offset < size; offset += 16)
    {
        for (int i = 0; i < 8; ++i)
        {
            left_block[i] = data[offset + i * 2];
            right_block[i] = data[offset + i * 2 + 1];
        }
        pcm[*frames * 2] = decimate64(&left_cic, left_block, false);
        pcm[*frames * 2 + 1] = decimate64(&right_cic, right_block, false);
        if (++*frames == PCM_FRAMES)
        {
            ci->pcmbuf_insert(pcm, NULL, *frames);
            *frames = 0;
        }
    }
    return 0;
}

static int output_dst_frame(size_t frame_size, int *frames)
{
    ci->memset(dst_frame + frame_size, 0, 8);
    int decoded = dst_decode(&dst_state, dst_frame, frame_size, dst_output);
    if (decoded != DST_FRAME_BYTES * 2)
        return -1;
    return output_dsd(dst_output, decoded, frames);
}

static enum codec_status decode_iso(void)
{
    uint32_t start = 0, end = 0;
    unsigned long duration_ms = 0;
    bool dst_encoded;
    if (!find_iso_area(&start, &end, &duration_ms, &dst_encoded))
        return CODEC_ERROR;

    uint32_t lsn = start;
    unsigned char channel_bytes[2][8];
    int channel_fill = 0;
    int pending_left = -1;
    int frames = 0;
    size_t dst_size = 0;
    bool dst_started = false;
    int dst_sectors = 0;
    reset_filter();
    if (!ci->seek_buffer((size_t)start * ISO_SECTOR))
        return CODEC_ERROR;

    intptr_t param = ci->id3->elapsed;
    long action = CODEC_ACTION_NULL;

    if (param) {
        action = CODEC_ACTION_SEEK_TIME;
        goto seek;
    } else if (ci->id3->offset) {
        /* TODO: Work out target param */
    }

    while (lsn <= end)
    {
        action = ci->get_command(&param);
        if (action == CODEC_ACTION_HALT)
            break;
    seek:
        if (action == CODEC_ACTION_SEEK_TIME)
        {
            uint64_t span = (uint64_t)end - start + 1;
            lsn = start + MIN(span - 1, span * (uint64_t)param / duration_ms);
            channel_fill = 0;
            pending_left = -1;
            frames = 0;
            dst_size = 0;
            dst_started = false;
            dst_sectors = 0;
            reset_filter();
            if (!ci->seek_buffer((size_t)lsn * ISO_SECTOR))
                return CODEC_ERROR;
            ci->seek_complete();
        }
        if (!read_exact(iso_sector, ISO_SECTOR))
            return CODEC_ERROR;
        ++lsn;

        unsigned header = iso_sector[0];
        unsigned packet_count = (header >> 5) & 7;
        unsigned frame_count = (header >> 2) & 7;
        if (!!(header & 1) != dst_encoded)
            return CODEC_ERROR;
        size_t frame_info_size = dst_encoded ? 4 : 3;
        size_t offset = 1 + packet_count * 2 + frame_count * frame_info_size;
        if (offset > ISO_SECTOR)
            return CODEC_ERROR;
        unsigned frame_info = 0;

        for (unsigned packet = 0; packet < packet_count; ++packet)
        {
            unsigned char *info = iso_sector + 1 + packet * 2;
            bool frame_start = info[0] & 0x80;
            unsigned type = (info[0] >> 3) & 7;
            unsigned length = ((info[0] & 7) << 8) | info[1];
            if (offset + length > ISO_SECTOR)
                return CODEC_ERROR;
            if (type == 2)
            {
                if (dst_encoded)
                {
                    if (frame_start)
                    {
                        if (dst_started && dst_size &&
                            output_dst_frame(dst_size, &frames) < 0)
                            return CODEC_ERROR;
                        if (frame_info >= frame_count)
                            return CODEC_ERROR;
                        unsigned char count = iso_sector[1 + packet_count * 2 +
                                                         frame_info * 4 + 3];
                        dst_sectors = (count >> 2) & 0x1f;
                        dst_size = 0;
                        dst_started = true;
                        ++frame_info;
                    }
                    if (dst_started)
                    {
                        if (dst_size + length > 65536)
                            return CODEC_ERROR;
                        ci->memcpy(dst_frame + dst_size, iso_sector + offset, length);
                        dst_size += length;
                        if (--dst_sectors == 0)
                        {
                            if (output_dst_frame(dst_size, &frames) < 0)
                                return CODEC_ERROR;
                            dst_size = 0;
                            dst_started = false;
                        }
                    }
                    offset += length;
                    continue;
                }
                for (unsigned i = 0; i < length; ++i)
                {
                    if (pending_left < 0)
                    {
                        pending_left = iso_sector[offset + i];
                        continue;
                    }
                    channel_bytes[0][channel_fill] = pending_left;
                    channel_bytes[1][channel_fill++] = iso_sector[offset + i];
                    pending_left = -1;
                    if (channel_fill == 8)
                    {
                        pcm[frames * 2] = decimate64(&left_cic,
                                                     channel_bytes[0], false);
                        pcm[frames * 2 + 1] = decimate64(&right_cic,
                                                         channel_bytes[1], false);
                        channel_fill = 0;
                        if (++frames == PCM_FRAMES)
                        {
                            ci->pcmbuf_insert(pcm, NULL, frames);
                            frames = 0;
                        }
                    }
                }
            }
            offset += length;
        }
        ci->set_elapsed((uint64_t)(lsn - start) * duration_ms /
                        ((uint64_t)end - start + 1));
        ci->set_offset((uint64_t) ci->curpos);
    }
    if (dst_encoded && dst_started && dst_size &&
        output_dst_frame(dst_size, &frames) < 0)
        return CODEC_ERROR;
    if (frames)
        ci->pcmbuf_insert(pcm, NULL, frames);
    return CODEC_OK;
}

enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD)
        ci->configure(DSP_SET_SAMPLE_DEPTH, 28);
    return CODEC_OK;
}

enum codec_status codec_run(void)
{
    unsigned char magic[16];
    if (codec_init())
        return CODEC_ERROR;
    codec_set_replaygain(ci->id3);
    init_cic_table();
    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    if (!ci->seek_buffer(0) || !read_exact(magic, sizeof(magic)))
        return CODEC_ERROR;
    if (!ci->memcmp(magic, "DSD ", 4))
        return decode_dsf();
    if (!ci->memcmp(magic, "FRM8", 4))
        return decode_dff();
    return decode_iso();
}
