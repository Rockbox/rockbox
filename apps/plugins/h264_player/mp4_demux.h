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
 * H.264 player MP4 demuxer - extracts video track data from MP4/M4V files.
 * Forked from libm4a/demux.c, adapted for fd-based I/O and video tracks.
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
#ifndef MP4_DEMUX_H
#define MP4_DEMUX_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Max SPS/PPS codec data from avcC box */
#define MP4V_MAX_CODECDATA  512

#define MP4V_MAX_STTS       4096

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ( \
    ( (int32_t)(char)(ch0) << 24 ) | \
    ( (int32_t)(char)(ch1) << 16 ) | \
    ( (int32_t)(char)(ch2) << 8 ) | \
    ( (int32_t)(char)(ch3) ) )
#endif

struct mp4v_stts_entry {
    uint32_t sample_count;
    uint32_t sample_delta;
};

struct mp4v_stsc_entry {
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t sample_desc_index;
};

struct mp4v_demux_res {
    /* Video format */
    uint32_t format;         /* fourcc: 'avc1' etc. */
    uint16_t width;          /* coded width from avc1 sample entry */
    uint16_t height;         /* coded height from avc1 sample entry */
    uint16_t display_width;  /* display width from tkhd (PAR-adjusted) */
    uint16_t display_height; /* display height from tkhd */
    uint32_t timescale;      /* from mdhd */

    /* avcC decoder config (SPS + PPS) */
    uint8_t  avc_profile;
    uint8_t  avc_level;
    uint8_t  nalu_len_size;  /* length field size (1-4 bytes, typically 4) */
    uint8_t  num_sps;
    uint8_t  num_pps;
    uint32_t codecdata_len;
    uint8_t  codecdata[MP4V_MAX_CODECDATA]; /* raw avcC data */

    /* Sample sizes (stsz) */
    uint32_t num_samples;
    uint32_t *sample_sizes;  /* caller-provided buffer */
    uint32_t sample_sizes_cap;

    /* Time-to-sample (stts) */
    uint32_t num_stts;
    struct mp4v_stts_entry stts[MP4V_MAX_STTS];

    /* Sample-to-chunk (stsc) */
    uint32_t num_stsc;
    struct mp4v_stsc_entry *stsc;    /* caller-provided buffer */
    uint32_t stsc_cap;

    /* Chunk offsets (stco) */
    uint32_t num_stco;
    uint32_t *chunk_offsets;  /* caller-provided buffer */
    uint32_t chunk_offsets_cap;

    /* Sync samples / keyframes (stss) */
    uint32_t num_stss;
    uint32_t *sync_samples;  /* caller-provided buffer */
    uint32_t sync_samples_cap;

    /* File and mdat location */
    uint32_t file_size;
    uint32_t mdat_offset;
    uint32_t mdat_len;

    /* Audio track (populated when mp4a trak found) */
    uint32_t audio_format;          /* fourcc: 'mp4a' or 0 if no audio */
    uint32_t audio_sample_rate;     /* from mp4a sample entry */
    uint8_t  audio_codecdata[64];   /* AudioSpecificConfig from esds */
    uint32_t audio_codecdata_len;
    uint32_t audio_lead_trim;       /* encoder priming from edts/elst */

    /* Audio table counts used to size aac.codec's workspace. The AAC codec
     * parses its own table data from the file. */
    uint32_t audio_num_samples;
    uint32_t audio_num_stsc;
    uint32_t audio_num_stco;
};

/* Open and parse an MP4 file, extracting video and audio tracks.
 *
 * filepath:         path to the .mp4/.m4v file
 * res:              output struct (caller allocates)
 * sample_buf:       buffer for video sample_sizes array
 * sample_cap:       number of uint32_t entries in sample_buf
 * chunk_buf:        buffer for video chunk_offsets array
 * chunk_cap:        number of uint32_t entries in chunk_buf
 * stsc_buf:         buffer for video sample-to-chunk entries
 * stsc_cap:         number of entries in stsc_buf
 * sync_buf:         buffer for video sync-sample entries
 * sync_cap:         number of entries in sync_buf
 *
 * Returns 0 on success (video found), -1 on error.
 * Audio track is optional; check res->audio_format != 0. */
int mp4v_demux_open(const char *filepath,
                    struct mp4v_demux_res *res,
                    uint32_t *sample_buf, uint32_t sample_cap,
                    uint32_t *chunk_buf, uint32_t chunk_cap,
                    struct mp4v_stsc_entry *stsc_buf, uint32_t stsc_cap,
                    uint32_t *sync_buf, uint32_t sync_cap);

/* Get the file offset and size of video sample N (0-based).
 * Returns 0 on success, -1 on error. */
int mp4v_get_sample_offset(const struct mp4v_demux_res *res,
                           uint32_t sample_index,
                           uint32_t *offset_out,
                           uint32_t *size_out);

/* Check if video sample N is a keyframe (sync sample).
 * Returns true if it's a keyframe or if no stss table exists
 * (meaning all samples are sync samples). */
bool mp4v_is_keyframe(const struct mp4v_demux_res *res,
                      uint32_t sample_index);

#endif /* MP4_DEMUX_H */
