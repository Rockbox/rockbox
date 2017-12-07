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

struct aiff_header
{
    uint8_t   form_id[4];           /* 00h - 'FORM'                          */
    uint32_t  form_size;            /* 04h - size of file - 8                */
    uint8_t   aiff_id[4];           /* 08h - 'AIFF'                          */
    uint8_t   comm_id[4];           /* 0Ch - 'COMM'                          */
    int32_t   comm_size;            /* 10h - num_channels through sample_rate
                                             (18)                            */
    int16_t   num_channels;         /* 14h - 1=M, 2=S, etc.                  */
    uint32_t  num_sample_frames;    /* 16h - num samples for each channel    */
    int16_t   sample_size;          /* 1ah - 1-32 bits per sample            */
    uint8_t   sample_rate[10];      /* 1ch - IEEE 754 80-bit floating point  */
    uint8_t   ssnd_id[4];           /* 26h - "SSND"                          */
    int32_t   ssnd_size;            /* 2ah - size of chunk from offset to
                                             end of pcm data                 */
    uint32_t  offset;               /* 2eh - data offset from end of header  */
    uint32_t  block_size;           /* 32h - pcm data alignment              */
                                    /* 36h */
} __attribute__((packed));

#define PCM_DEPTH_BYTES             2
#define PCM_DEPTH_BITS             16
#define PCM_SAMP_PER_CHUNK       2048

static int num_channels;
static uint32_t sample_rate;
static size_t frame_size;
static size_t pcm_size;
static uint32_t num_sample_frames;

/* Template headers */
static const struct aiff_header aiff_template_header =
{
    { 'F', 'O', 'R', 'M' },             /* form_id               */
    0,                                  /* form_size         (*) */
    { 'A', 'I', 'F', 'F' },             /* aiff_id               */
    { 'C', 'O', 'M', 'M' },             /* comm_id               */
    htobe32(18),                        /* comm_size             */
    0,                                  /* num_channels      (*) */
    0,                                  /* num_sample_frames (*) */
    htobe16(PCM_DEPTH_BITS),            /* sample_size           */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },   /* sample_rate       (*) */
    { 'S', 'S', 'N', 'D' },             /* ssnd_id               */
    0,                                  /* ssnd_size         (*) */
    htobe32(0),                         /* offset                */
    htobe32(0),                         /* block_size            */
    /* (*) updated when finalizing stream */
};

static inline void frame_htobe(uint32_t *p, size_t size)
{
#ifdef ROCKBOX_LITTLE_ENDIAN
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
#endif /* ROCKBOX_LITTLE_ENDIAN */
    (void)p; (void)size;
}

/* convert unsigned 32 bit value to 80-bit floating point number */
static void uint32_h_to_ieee754_extended_be(uint8_t f[10], uint32_t l)
{
    ci->memset(f, 0, 10);

    if (l == 0)
        return;

    int shift = __builtin_clz(l);

    /* sign always zero - bit 79 */
    /* exponent is 0-31 (normalized: 30 - shift + 16383) - bits 64-78 */
    f[0] = 0x40;
    f[1] = (uint8_t)(30 - shift);
    /* mantissa is value left justified with most significant non-zero
       bit stored in bit 63 - bits 0-63 */
    l <<= shift;
    f[2] = (uint8_t)(l >> 24);
    f[3] = (uint8_t)(l >> 16);
    f[4] = (uint8_t)(l >>  8);
    f[5] = (uint8_t)(l >>  0);
}

static int on_stream_data(struct enc_chunk_data *data)
{
    size_t size = data->hdr.size;

    if (ci->enc_stream_write(data->data, size) != (ssize_t)size)
        return -1;

    pcm_size += size;
    num_sample_frames += data->pcm_count;

    return 0;
}

static int on_stream_start(void)
{
    /* reset sample count */
    pcm_size = 0;
    num_sample_frames = 0;

    /* write template header */
    if (ci->enc_stream_write(&aiff_template_header,
                             sizeof (struct aiff_header))
            != sizeof (struct aiff_header))
        return -1;

    return 0;
}

static int on_stream_end(union enc_chunk_hdr *hdr)
{
    /* update template header */
    struct aiff_header aiff;

    if (hdr->err)
    {
        /* Called for stream error; get correct data size */
        ssize_t size = ci->enc_stream_lseek(0, SEEK_END);

        if (size > (ssize_t)sizeof (aiff))
        {
            pcm_size = size - sizeof (aiff);
            num_sample_frames = pcm_size / (PCM_DEPTH_BYTES*num_channels);
        }
    }

    if (ci->enc_stream_lseek(0, SEEK_SET) != 0)
        return -1;

    if (ci->enc_stream_read(&aiff, sizeof (aiff)) != sizeof (aiff))
        return -2;

    /* 'FORM' chunk */
    aiff.form_size = htobe32(pcm_size + sizeof (aiff) - 8);

    /* 'COMM' chunk */
    aiff.num_channels = htobe16(num_channels);
    aiff.num_sample_frames = htobe32(num_sample_frames);
    uint32_h_to_ieee754_extended_be(aiff.sample_rate, sample_rate);

    /* 'SSND' chunk */
    aiff.ssnd_size = htobe32(pcm_size + 8);

    if (ci->enc_stream_lseek(0, SEEK_SET) != 0)
        return -3;

    if (ci->enc_stream_write(&aiff, sizeof (aiff)) != sizeof (aiff))
        return -4;

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

        frame_htobe((uint32_t *)data->data, frame_size);

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
