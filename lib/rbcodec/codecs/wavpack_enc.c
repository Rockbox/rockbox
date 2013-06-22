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

#include "codeclib.h"
#include "libwavpack/wavpack.h"

CODEC_ENC_HEADER

/** Types **/
typedef struct
{
    uint8_t type;       /* Type of metadata */
    uint8_t word_size;  /* Size of metadata in words */
} __attribute__((packed)) WavpackMetadataHeader;

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

#define RIFF_FMT_HEADER_SIZE   12 /* format -> format_size */
#define RIFF_FMT_DATA_SIZE     16 /* audio_format -> bits_per_sample */
#define RIFF_DATA_HEADER_SIZE   8 /* data_id -> data_size */

struct wvpk_chunk_data
{
    struct enc_chunk_data ckhdr;  /* The base data chunk header */
    WavpackHeader         wphdr;  /* The block wavpack info */
    uint8_t               data[]; /* Encoded audio data */
};

#define PCM_DEPTH_BITS         16
#define PCM_DEPTH_BYTES         2
#define PCM_SAMP_PER_CHUNK   5000

/** Data **/
static int32_t input_buffer[PCM_SAMP_PER_CHUNK*2] IBSS_ATTR;

static WavpackConfig config IBSS_ATTR;
static WavpackContext *wpc;
static uint32_t sample_rate;
static int num_channels;
static uint32_t total_samples;
static size_t out_reqsize;
static size_t frame_size;

static const WavpackMetadataHeader wvpk_mdh =
{
    ID_RIFF_HEADER,
    sizeof (struct riff_header) / sizeof (uint16_t),
};

static const struct riff_header riff_template_header =
{
    /* "RIFF" header */
    { 'R', 'I', 'F', 'F' },         /* riff_id          */
    0,                              /* riff_size   (*)  */
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

static inline void sample_to_int32(int32_t **dst, int32_t **src)
{
    uint32_t t = *(*src)++;
#ifdef ROCKBOX_BIG_ENDIAN
    *(*dst)++ = (int32_t)t >> 16;
    *(*dst)++ = (int16_t)t;
#else
    *(*dst)++ = (int16_t)t;
    *(*dst)++ = (int32_t)t >> 16;
#endif
}

static void ICODE_ATTR input_buffer_to_int32(size_t size)
{
    int32_t *dst = input_buffer;
    int32_t *src = input_buffer + PCM_SAMP_PER_CHUNK;

    do
    {
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
        sample_to_int32(&dst, &src);
    }
    while (size -= 10 * 2 * PCM_DEPTH_BYTES);
}

static int on_stream_data(struct wvpk_chunk_data *wpdata)
{
    /* update timestamp (block_index) */
    wpdata->wphdr.block_index = htole32(total_samples);

    size_t size = wpdata->ckhdr.hdr.size;
    if (ci->enc_stream_write(wpdata->ckhdr.data, size) != (ssize_t)size)
        return -1;

    total_samples += wpdata->ckhdr.pcm_count;

    return 0;
}

static int on_stream_start(void)
{
    /* reset sample count */
    total_samples = 0;

    /* write template headers */
    if (ci->enc_stream_write(&wvpk_mdh, sizeof (wvpk_mdh))
            != sizeof (wvpk_mdh))
        return -1;

    if (ci->enc_stream_write(&riff_template_header,
                             sizeof (riff_template_header))
            != sizeof (riff_template_header))
        return -2;

    return 0;
}

static int on_stream_end(void)
{
    struct
    {
        WavpackMetadataHeader wpmdh;
        struct riff_header    rhdr;
        WavpackHeader         wph;
    } __attribute__ ((packed)) h;

    /* Correcting sizes on error is a bit of a pain */

    /* read template headers at start */
    if (ci->enc_stream_lseek(0, SEEK_SET) != 0)
        return -1;

    if (ci->enc_stream_read(&h, sizeof (h)) != sizeof (h))
        return -2;

    size_t data_size = total_samples*config.num_channels*PCM_DEPTH_BYTES;

    /** "RIFF" header **/
    h.rhdr.riff_size    = htole32(RIFF_FMT_HEADER_SIZE +
            RIFF_FMT_DATA_SIZE + RIFF_DATA_HEADER_SIZE + data_size);

    /* format data */
    h.rhdr.num_channels = htole16(config.num_channels);
    h.rhdr.sample_rate  = htole32(config.sample_rate);
    h.rhdr.byte_rate    = htole32(config.sample_rate*config.num_channels*
                                      PCM_DEPTH_BYTES);
    h.rhdr.block_align  = htole16(config.num_channels*PCM_DEPTH_BYTES);

    /* data header */
    h.rhdr.data_size    = htole32(data_size);

    /** Wavpack header **/
    h.wph.ckSize        = htole32(letoh32(h.wph.ckSize) + sizeof (h.wpmdh)
                                + sizeof (h.rhdr));
    h.wph.total_samples = htole32(total_samples);

    /* MDH|RIFF|WVPK => WVPK|MDH|RIFF */
    if (ci->enc_stream_lseek(0, SEEK_SET) != 0)
        return -3;

    if (ci->enc_stream_write(&h.wph, sizeof (h.wph)) != sizeof (h.wph))
        return -4;

    if (ci->enc_stream_write(&h.wpmdh, sizeof (h.wpmdh)) != sizeof (h.wpmdh))
        return -5;

    if (ci->enc_stream_write(&h.rhdr, sizeof (h.rhdr)) != sizeof (h.rhdr))
        return -6;

    return 0;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD)
        codec_init();

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    enum { GETBUF_ENC, GETBUF_PCM } getbuf = GETBUF_ENC;
    struct enc_chunk_data *data = NULL;

    /* main encoding loop */
    while (1)
    {
        enum codec_command_action action = ci->get_command(NULL);

        if (action != CODEC_ACTION_NULL)
            break;

        /* First obtain output buffer; when available, get PCM data */
        switch (getbuf)
        {
        case GETBUF_ENC:
            if (!(data = ci->enc_encbuf_get_buffer(out_reqsize)))
                continue;
            getbuf = GETBUF_PCM;
        case GETBUF_PCM:
            if (!ci->enc_pcmbuf_read(input_buffer + PCM_SAMP_PER_CHUNK,
                                     PCM_SAMP_PER_CHUNK))
                continue;
            getbuf = GETBUF_ENC;
        }

        input_buffer_to_int32(frame_size);

        if (WavpackStartBlock(wpc, data->data, data->data + out_reqsize) &&
            WavpackPackSamples(wpc, input_buffer, PCM_SAMP_PER_CHUNK))
        {
            /* finish the chunk and store chunk size info */
            data->hdr.size = WavpackFinishBlock(wpc);
            data->pcm_count = PCM_SAMP_PER_CHUNK;
        }
        else
        {
            data->hdr.err = 1;
        }

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
            return on_stream_end();
        }
    }
    else if (reason == ENC_CB_INPUTS)
    {
        /* Save parameters */
        struct enc_inputs *inputs = params;
        sample_rate = inputs->sample_rate;
        num_channels = inputs->num_channels;
        frame_size = PCM_SAMP_PER_CHUNK*PCM_DEPTH_BYTES*num_channels;
        out_reqsize = frame_size*110 / 100; /* Add 10% */

        /* Setup Wavpack encoder */
        memset(&config, 0, sizeof (config));
        config.bits_per_sample = PCM_DEPTH_BITS;
        config.bytes_per_sample = PCM_DEPTH_BYTES;
        config.sample_rate = sample_rate;
        config.num_channels = num_channels;

        wpc = WavpackOpenFileOutput();

        if (!WavpackSetConfiguration(wpc, &config, -1))
            return -1;
    }

    return 0;
}
