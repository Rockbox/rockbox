/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Antonius Hellmann
 * Copyright (C) 2006-2013 Michael Sevakis
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

#include <inttypes.h>
#include <stdio.h>

#include "codeclib.h"

CODEC_ENC_HEADER

struct riff_header
{
    uint8_t  riff_id[4];      /* 00h - "RIFF"                            */
    uint32_t riff_size;       /* 04h - sz following headers + data_size  */
    /* format header */
    uint8_t  format[4];       /* 08h - "WAVE"                            */
    uint8_t  format_id[4];    /* 0Ch - "fmt "                            */
    uint32_t format_size;     /* 10h - 16 for PCM (sz format data)       */
    /* format data */
    uint16_t audio_format;    /* 14h - 1=PCM                             */
    uint16_t num_channels;    /* 16h - 1=M, 2=S, etc.                    */
    uint32_t sample_rate;     /* 18h - HZ                                */
    uint32_t byte_rate;       /* 1Ch - num_channels*sample_rate*bits_per_sample/8 */
    uint16_t block_align;     /* 20h - num_channels*bits_per_samples/8   */
    uint16_t bits_per_sample; /* 22h - 8=8 bits, 16=16 bits, etc.        */
    /* Not for audio_format=1 (PCM) */
/*  uint16_t extra_param_size;   24h - size of extra data                */
/*  uint8_t  extra_params[extra_param_size];                             */
    /* data header */
    uint8_t  data_id[4];      /* 24h - "data" */
    uint32_t data_size;       /* 28h - num_samples*num_channels*bits_per_sample/8 */
/*  uint8_t  data[data_size];    2Ch - actual sound data */
} __attribute__((packed));

#define RIFF_FMT_HEADER_SIZE       12 /* format -> format_size */
#define RIFF_FMT_DATA_SIZE         16 /* audio_format -> bits_per_sample */
#define RIFF_DATA_HEADER_SIZE       8 /* data_id -> data_size */

#define PCM_DEPTH_BYTES             2
#define PCM_DEPTH_BITS             16
#define PCM_SAMP_PER_CHUNK       2048

static int num_channels;
static uint32_t sample_rate;
static size_t frame_size;
static size_t data_size;

static const struct riff_header riff_template_header =
{
    /* "RIFF" header */
    { 'R', 'I', 'F', 'F' },         /* riff_id          */
    0,                              /* riff_size    (*) */
    /* format header */
    { 'W', 'A', 'V', 'E' },         /* format           */
    { 'f', 'm', 't', ' ' },         /* format_id        */
    htole32(16),                    /* format_size      */
    /* format data */
    htole16(1),                     /* audio_format     */
    0,                              /* num_channels (*) */
    0,                              /* sample_rate  (*) */
    0,                              /* byte_rate    (*) */
    0,                              /* block_align  (*) */
    htole16(PCM_DEPTH_BITS),        /* bits_per_sample  */
    /* data header */
    { 'd', 'a', 't', 'a' },         /* data_id          */
    0                               /* data_size    (*) */
    /* (*) updated when finalizing stream */
};

static inline void frame_htole(uint32_t *p, size_t size)
{
#ifdef ROCKBOX_BIG_ENDIAN
    /* Byte-swap samples, stereo or mono */
    do
    {
        uint32_t t;
        t = swap_odd_even32(*p); *p++ = t;
        t = swap_odd_even32(*p); *p++ = t;
        t = swap_odd_even32(*p); *p++ = t;
        t = swap_odd_even32(*p); *p++ = t;
        t = swap_odd_even32(*p); *p++ = t;
        t = swap_odd_even32(*p); *p++ = t;
        t = swap_odd_even32(*p); *p++ = t;
        t = swap_odd_even32(*p); *p++ = t;
    }
    while (size -= 8 * 2 * PCM_DEPTH_BYTES);
#endif /* ROCKBOX_BIG_ENDIAN */
    (void)p; (void)size;
}

static int on_stream_data(struct enc_chunk_data *data)
{
    size_t size = data->hdr.size;

    if (ci->enc_stream_write(data->data, size) != (ssize_t)size)
        return -1;

    data_size += size;

    return 0;
}

static int on_stream_start(void)
{
    /* reset sample count */
    data_size = 0;

    /* write template header */
    if (ci->enc_stream_write(&riff_template_header, sizeof (struct riff_header))
            != sizeof (struct riff_header))
        return -1;

    return 0;
}

static int on_stream_end(union enc_chunk_hdr *hdr)
{
    /* update template header */
    struct riff_header riff;

    if (hdr->err)
    {
        /* Called for stream error; get correct data size */
        ssize_t size = ci->enc_stream_lseek(0, SEEK_END);

        if (size > (ssize_t)sizeof (riff))
            data_size = size - sizeof (riff);
    }

    if (ci->enc_stream_lseek(0, SEEK_SET) ||
        ci->enc_stream_read(&riff, sizeof (riff)) != sizeof (riff))
        return -1;

    /* "RIFF" header */
    riff.riff_size = htole32(RIFF_FMT_HEADER_SIZE + RIFF_FMT_DATA_SIZE
                             + RIFF_DATA_HEADER_SIZE + data_size);

    /* format data */
    riff.num_channels = htole16(num_channels);
    riff.sample_rate = htole32(sample_rate);
    riff.byte_rate = htole32(sample_rate*num_channels*PCM_DEPTH_BYTES);
    riff.block_align = htole16(num_channels*PCM_DEPTH_BYTES);

    /* data header */
    riff.data_size = htole32(data_size);

    if (ci->enc_stream_lseek(0, SEEK_SET) != 0)
        return -2;

    if (ci->enc_stream_write(&riff, sizeof (riff)) != sizeof (riff))
        return -3;

    return 0;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    return CODEC_OK;
    (void)reason;
}

/* this is called for each file to process */
enum codec_status ICODE_ATTR codec_run(void)
{
    enum { GETBUF_ENC, GETBUF_PCM } getbuf = GETBUF_ENC;
    struct enc_chunk_data *data = NULL;

    /* main encoding loop */
    while (1)
    {
        long action = ci->get_command(NULL);

        if (action != CODEC_ACTION_NULL)
            break;

        /* First obtain output buffer; when available, get PCM data */
        switch (getbuf)
        {
        case GETBUF_ENC:
            if (!(data = ci->enc_encbuf_get_buffer(frame_size)))
                continue;
            getbuf = GETBUF_PCM;
        case GETBUF_PCM:
            if (!ci->enc_pcmbuf_read(data->data, PCM_SAMP_PER_CHUNK))
                continue;
            getbuf = GETBUF_ENC;
        }

        data->hdr.size = frame_size;
        data->pcm_count = PCM_SAMP_PER_CHUNK;

        frame_htole((uint32_t *)data->data, frame_size);

        ci->enc_pcmbuf_advance(PCM_SAMP_PER_CHUNK);
        ci->enc_encbuf_finish_buffer();
    }

    return CODEC_OK;
}

/* this is called by recording system */
int ICODE_ATTR enc_callback(enum enc_callback_reason reason,
                            void *params)
{
    if (LIKELY(reason == ENC_CB_STREAM))
    {
        switch (((union enc_chunk_hdr *)params)->type)
        {
        case CHUNK_T_DATA:
            return on_stream_data(params);
        case CHUNK_T_STREAM_START:
            return on_stream_start();
        case CHUNK_T_STREAM_END:
            return on_stream_end(params);
        }
    }
    else if (reason == ENC_CB_INPUTS)
    {
        struct enc_inputs *inputs = params;
        sample_rate = inputs->sample_rate;
        num_channels = inputs->num_channels;
        frame_size = PCM_SAMP_PER_CHUNK*PCM_DEPTH_BYTES*num_channels;
    }

    return 0;
}
