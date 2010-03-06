/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Yoshihisa Uchida
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
#include "codecs/libpcm/support_formats.h"

CODEC_HEADER

/*
 * SMAF (Synthetic music Mobile Application Format)
 *
 * References
 * [1] YAMAHA Corporation, Synthetic music Mobile Application Format Ver.3.05, 2002
 */

enum {
    SMAF_TRACK_CHUNK_SCORE = 0, /* Score Track */
    SMAF_TRACK_CHUNK_AUDIO,     /* PCM Audio Track */
};

/* SMAF supported codec formats */
enum {
    SMAF_FORMAT_UNSUPPORT = 0,  /* unsupported format */
    SMAF_FORMAT_SIGNED_PCM,     /* 2's complement PCM */
    SMAF_FORMAT_UNSIGNED_PCM,   /* Offset Binary PCM */
    SMAF_FORMAT_ADPCM,          /* YAMAHA ADPCM */
};

static int support_formats[2][3] = {
    {SMAF_FORMAT_SIGNED_PCM, SMAF_FORMAT_UNSIGNED_PCM, SMAF_FORMAT_ADPCM     },
    {SMAF_FORMAT_SIGNED_PCM, SMAF_FORMAT_ADPCM,        SMAF_FORMAT_UNSUPPORT },
};

static const struct pcm_entry pcm_codecs[] = {
    { SMAF_FORMAT_SIGNED_PCM,   get_linear_pcm_codec   },
    { SMAF_FORMAT_UNSIGNED_PCM, get_linear_pcm_codec   },
    { SMAF_FORMAT_ADPCM,        get_yamaha_adpcm_codec },
};

#define NUM_FORMATS 3

static int basebits[4] = { 4, 8, 12, 16 };

#define PCM_SAMPLE_SIZE (2048*2)

static int32_t samples[PCM_SAMPLE_SIZE] IBSS_ATTR;

static const struct pcm_codec *get_codec(uint32_t formattag)
{
    int i;

    for (i = 0; i < NUM_FORMATS; i++)
    {
        if (pcm_codecs[i].format_tag == formattag)
        {
            if (pcm_codecs[i].get_codec)
                return pcm_codecs[i].get_codec();
            return 0;
        }
    }
    return 0;
}

static unsigned int get_be32(uint8_t *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static int convert_smaf_audio_format(int track_chunk, unsigned int audio_format)
{
    if (audio_format > 3)
        return SMAF_FORMAT_UNSUPPORT;

    return support_formats[track_chunk][audio_format];
}

static int convert_smaf_audio_basebit(unsigned int basebit)
{
    if (basebit > 4)
        return 0;
    return basebits[basebit];
}

static bool parse_audio_track(struct pcm_format *fmt,
                              unsigned char **stbuf, unsigned char *endbuf)
{
    unsigned char *buf = *stbuf;
    int chunksize;

    buf += 8;
    fmt->channels = ((buf[2] & 0x80) >> 7) + 1;
    fmt->formattag = convert_smaf_audio_format(SMAF_TRACK_CHUNK_AUDIO,
                                               (buf[2] >> 4) & 0x07);
    if (fmt->formattag == SMAF_FORMAT_UNSUPPORT)
    {
        DEBUGF("CODEC_ERROR: unsupport pcm data format : %d\n", (buf[2] >> 4) & 0x07);
        return false;
    }
    fmt->bitspersample = convert_smaf_audio_basebit(buf[3] >> 4);
    if (fmt->bitspersample == 0)
    {
        DEBUGF("CODEC_ERROR: unsupport pcm data basebit : %d\n", buf[3] >> 4);
        return false;
    }
    buf += 6;
    while (buf < endbuf)
    {
        chunksize = get_be32(buf + 4) + 8;
        if (memcmp(buf, "Awa", 3) == 0)
        {
            fmt->numbytes = get_be32(buf + 4);
            buf += 8;
            return true;
        }
        buf += chunksize;
    }
    DEBUGF("CODEC_ERROR: smaf does not include stream pcm data\n");
    return false;
}

static bool parse_score_track(struct pcm_format *fmt,
                              unsigned char **stbuf, unsigned char *endbuf)
{
    unsigned char *buf = *stbuf;
    int chunksize;

    if (buf[9] != 0x00)
    {
        DEBUGF("CODEC_ERROR: score track chunk unsupport sequence type %d\n", buf[9]);
        return false;
    }

    /*
     * skip to the next chunk.
     *    MA-2/MA-3/MA-5: padding 16 bytes
     *    MA-7:           padding 32 bytes
     */
    if (buf[3] < 7)
        buf += 28;
    else
        buf += 44;

    while (buf < endbuf)
    {
        chunksize = get_be32(buf + 4) + 8;
        if (memcmp(buf, "Mtsp", 4) == 0)
        {
            buf += 8;
            if (memcmp(buf, "Mwa", 3) != 0)
            {
                DEBUGF("CODEC_ERROR: smaf does not include stream pcm data\n");
                return false;
            }
            fmt->numbytes = get_be32(buf + 4) - 3;
            fmt->channels = ((buf[8] & 0x80) >> 7) + 1;
            fmt->formattag = convert_smaf_audio_format(SMAF_TRACK_CHUNK_SCORE,
                                                       (buf[8] >> 4) & 0x07);
            if (fmt->formattag == SMAF_FORMAT_UNSUPPORT)
            {
                DEBUGF("CODEC_ERROR: unsupport pcm data format : %d\n",
                                                       (buf[8] >> 4) & 0x07);
                return false;
            }
            fmt->bitspersample = convert_smaf_audio_basebit(buf[8] & 0x0f);
            if (fmt->bitspersample == 0)
            {
                DEBUGF("CODEC_ERROR: unsupport pcm data basebit : %d\n",
                                                           buf[8] & 0x0f);
                return false;
            }
            buf += 11;
            return true;
        }
        buf += chunksize;
    }

    DEBUGF("CODEC_ERROR: smaf does not include stream pcm data\n");
    return false;
}

static bool parse_header(struct pcm_format *fmt, size_t *pos)
{
    unsigned char *buf, *stbuf, *endbuf;
    size_t chunksize;

    ci->memset(fmt, 0, sizeof(struct pcm_format));

    /* assume the SMAF pcm data position is less than 1024 bytes */
    stbuf = ci->request_buffer(&chunksize, 1024);
    if (chunksize < 1024)
        return false;

    buf = stbuf;
    endbuf = stbuf + chunksize;
 
    if (memcmp(buf, "MMMD", 4) != 0)
    {
        DEBUGF("CODEC_ERROR: does not smaf format %c%c%c%c\n",
                                  buf[0], buf[1], buf[2], buf[3]);
        return false;
    }
    buf += 8;

    while (buf < endbuf)
    {
        chunksize = get_be32(buf + 4) + 8;
        if (memcmp(buf, "ATR", 3) == 0)
        {
            if (!parse_audio_track(fmt, &buf, endbuf))
                return false;
            break;
        }
        if (memcmp(buf, "MTR", 3) == 0)
        {
            if (!parse_score_track(fmt, &buf, endbuf))
                return false;
            break;
        }
        buf += chunksize;
    }

    if (buf >= endbuf)
    {
        DEBUGF("CODEC_ERROR: unsupported smaf format\n");
        return false;
    }

    /* blockalign */
    if (fmt->formattag == SMAF_FORMAT_SIGNED_PCM ||
        fmt->formattag == SMAF_FORMAT_UNSIGNED_PCM)
        fmt->blockalign = fmt->channels * fmt->bitspersample >> 3;

    /* data signess (default signed) */
    fmt->is_signed = (fmt->formattag != SMAF_FORMAT_UNSIGNED_PCM);

    fmt->is_little_endian = false;

    /* sets pcm data position */
    *pos = buf - stbuf;

    return true;
}

static struct pcm_format format;
static uint32_t bytesdone;

static uint8_t *read_buffer(size_t *realsize)
{
    uint8_t *buffer = (uint8_t *)ci->request_buffer(realsize, format.chunksize);
    if (bytesdone + (*realsize) > format.numbytes)
        *realsize = format.numbytes - bytesdone;
    bytesdone += *realsize;
    ci->advance_buffer(*realsize);
    return buffer;
}

enum codec_status codec_main(void)
{
    int status = CODEC_OK;
    uint32_t decodedsamples;
    uint32_t i = CODEC_OK;
    size_t n;
    int bufcount;
    int endofstream;
    uint8_t *smafbuf;
    off_t firstblockposn;     /* position of the first block in file */
    const struct pcm_codec *codec;

    /* Generic codec initialisation */
    ci->configure(DSP_SET_SAMPLE_DEPTH, PCM_OUTPUT_DEPTH);
  
next_track:
    if (codec_init()) {
        i = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);

    ci->memset(&format, 0, sizeof(struct pcm_format));
    format.is_signed = true;
    format.is_little_endian = false;

    decodedsamples = 0;
    codec = 0;

    if (!parse_header(&format, &n))
    {
        i = CODEC_ERROR;
        goto done;
    }

    codec = get_codec(format.formattag);
    if (codec == 0)
    {
        DEBUGF("CODEC_ERROR: unsupport audio format: 0x%x\n", (int)format.formattag);
        i = CODEC_ERROR;
        goto done;
    }

    if (!codec->set_format(&format))
    {
        i = CODEC_ERROR;
        goto done;
    }

    /* common format check */
    if (format.channels == 0) {
        DEBUGF("CODEC_ERROR: 'fmt ' chunk not found or 0-channels file\n");
        status = CODEC_ERROR;
        goto done;
    }
    if (format.samplesperblock == 0) {
        DEBUGF("CODEC_ERROR: 'fmt ' chunk not found or 0-wSamplesPerBlock file\n");
        status = CODEC_ERROR;
        goto done;
    }
    if (format.blockalign == 0)
    {
        DEBUGF("CODEC_ERROR: 'fmt ' chunk not found or 0-blockalign file\n");
        i = CODEC_ERROR;
        goto done;
    }
    if (format.numbytes == 0) {
        DEBUGF("CODEC_ERROR: 'data' chunk not found or has zero-length\n");
        status = CODEC_ERROR;
        goto done;
    }

    /* check chunksize */
    if ((format.chunksize / format.blockalign) * format.samplesperblock * format.channels
           > PCM_SAMPLE_SIZE)
        format.chunksize = (PCM_SAMPLE_SIZE / format.blockalign) * format.blockalign;
    if (format.chunksize == 0)
    {
        DEBUGF("CODEC_ERROR: chunksize is 0\n");
        i = CODEC_ERROR;
        goto done;
    }

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);

    if (format.channels == 2) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    } else if (format.channels == 1) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    } else {
        DEBUGF("CODEC_ERROR: more than 2 channels unsupported\n");
        i = CODEC_ERROR;
        goto done;
    }

    firstblockposn = 1024 - n;
    ci->advance_buffer(firstblockposn);

    /* The main decoder loop */
    bytesdone = 0;
    ci->set_elapsed(0);
    endofstream = 0;

    while (!endofstream) {
        ci->yield();
        if (ci->stop_codec || ci->new_track)
            break;

        if (ci->seek_time) {
            struct pcm_pos *newpos = codec->get_seek_pos(ci->seek_time, &read_buffer);

            decodedsamples = newpos->samples;
            if (newpos->pos > format.numbytes)
                break;
            if (ci->seek_buffer(firstblockposn + newpos->pos))
            {
                bytesdone = newpos->pos;
            }
            ci->seek_complete();
        }
        smafbuf = (uint8_t *)ci->request_buffer(&n, format.chunksize);

        if (n == 0)
            break; /* End of stream */

        if (bytesdone + n > format.numbytes) {
            n = format.numbytes - bytesdone;
            endofstream = 1;
        }

        status = codec->decode(smafbuf, n, samples, &bufcount);
        if (status == CODEC_ERROR)
        {
            DEBUGF("codec error\n");
            goto done;
        }

        ci->pcmbuf_insert(samples, NULL, bufcount);

        ci->advance_buffer(n);
        bytesdone += n;
        decodedsamples += bufcount;
        if (bytesdone >= format.numbytes)
            endofstream = 1;

        ci->set_elapsed(decodedsamples*1000LL/ci->id3->frequency);
    }
    i = CODEC_OK;

done:
    if (ci->request_next_track())
        goto next_track;

exit:
    return i;
}

