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
    SMAF_AUDIO_TRACK_CHUNK = 0, /* PCM Audio Track */
    SMAF_SCORE_TRACK_CHUNK,     /* Score Track */
};

/* SMAF supported codec formats */
enum {
    SMAF_FORMAT_UNSUPPORT = 0,  /* unsupported format */
    SMAF_FORMAT_SIGNED_PCM,     /* 2's complement PCM */
    SMAF_FORMAT_UNSIGNED_PCM,   /* Offset Binary PCM */
    SMAF_FORMAT_ADPCM,          /* YAMAHA ADPCM */
};

static const int support_formats[2][3] = {
    {SMAF_FORMAT_SIGNED_PCM, SMAF_FORMAT_ADPCM,        SMAF_FORMAT_UNSUPPORT },
    {SMAF_FORMAT_SIGNED_PCM, SMAF_FORMAT_UNSIGNED_PCM, SMAF_FORMAT_ADPCM     },
};

static const struct pcm_entry pcm_codecs[] = {
    { SMAF_FORMAT_SIGNED_PCM,   get_linear_pcm_codec   },
    { SMAF_FORMAT_UNSIGNED_PCM, get_linear_pcm_codec   },
    { SMAF_FORMAT_ADPCM,        get_yamaha_adpcm_codec },
};

#define NUM_FORMATS 3

static const int basebits[4] = { 4, 8, 12, 16 };

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

static unsigned int get_be32(const uint8_t *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static int convert_smaf_channels(unsigned int ch)
{
    return (ch >> 7) + 1;
}

static int convert_smaf_audio_format(unsigned int chunk, unsigned int audio_format)
{
    int idx = (audio_format & 0x70) >> 4;

    if (idx < 3)
        return support_formats[chunk][idx];

    DEBUGF("CODEC_ERROR: unsupport audio format: %d\n", audio_format);
    return SMAF_FORMAT_UNSUPPORT;
}

static int convert_smaf_audio_basebit(unsigned int basebit)
{
    if (basebit < 4)
        return basebits[basebit];

    DEBUGF("CODEC_ERROR: illegal basebit: %d\n", basebit);
    return 0;
}

static unsigned int search_chunk(const unsigned char *name, int nlen, off_t *pos)
{
    const unsigned char *buf;
    unsigned int chunksize;
    size_t size;

    while (true)
    {
        buf = ci->request_buffer(&size, 8);
        if (size < 8)
            break;

        chunksize = get_be32(buf + 4);
        ci->advance_buffer(8);
        *pos += 8;
        if (memcmp(buf, name, nlen) == 0)
            return chunksize;

        ci->advance_buffer(chunksize);
        *pos += chunksize;
    }
    DEBUGF("CODEC_ERROR: missing '%s' chunk\n", name);
    return 0;
}

static bool parse_audio_track(struct pcm_format *fmt, unsigned int chunksize, off_t *pos)
{
    const unsigned char *buf;
    size_t size;

    /* search PCM Audio Track Chunk */
    ci->advance_buffer(chunksize);
    *pos += chunksize;
    if (search_chunk("ATR", 3, pos) == 0)
    {
        DEBUGF("CODEC_ERROR: missing PCM Audio Track Chunk\n");
        return false;
    }

    /*
     * get format
     *  buf
     *    +0: Format Type
     *    +1: Sequence Type
     *    +2: bit 7 0:mono/1:stereo, bit 4-6 format, bit 0-3: frequency
     *    +3: bit 4-7: base bit
     *    +4: TimeBase_D
     *    +5: TimeBase_G
     *
     * Note: If PCM Audio Track does not include Sequence Data Chunk,
     *       tmp+6 is the start position of Wave Data Chunk.
     */
    buf = ci->request_buffer(&size, 6);
    if (size < 6)
    {
        DEBUGF("CODEC_ERROR: smaf is too small\n");
        return false;
    }

    fmt->formattag     = convert_smaf_audio_format(SMAF_AUDIO_TRACK_CHUNK, buf[2]);
    fmt->channels      = convert_smaf_channels(buf[2]);
    fmt->bitspersample = convert_smaf_audio_basebit(buf[3] >> 4);

    /* search Wave Data Chunk */
    ci->advance_buffer(6);
    *pos += 6;
    fmt->numbytes = search_chunk("Awa", 3, pos);
    if (fmt->numbytes == 0)
    {
        DEBUGF("CODEC_ERROR: missing Wave Data Chunk\n");
        return false;
    }

    return true;
}

static bool parse_score_track(struct pcm_format *fmt, off_t *pos)
{
    const unsigned char *buf;
    unsigned int chunksize;
    size_t size;

    /* parse Optional Data Chunk */
    buf = ci->request_buffer(&size, 13);
    if (size < 13)
    {
        DEBUGF("CODEC_ERROR: smaf is too small\n");
        return false;
    }

    if (memcmp(buf + 5, "OPDA", 4) != 0)
    {
        DEBUGF("CODEC_ERROR: missing Optional Data Chunk\n");
        return false;
    }

    /* Optional Data Chunk size */
    chunksize = get_be32(buf + 9);

    /* search Score Track Chunk */
    ci->advance_buffer(13 + chunksize);
    *pos += (13 + chunksize);
    if (search_chunk("MTR", 3, pos) == 0)
    {
        DEBUGF("CODEC_ERROR: missing Score Track Chunk\n");
        return false;
    }

    /*
     * search next chunk
     * usually, next chunk ('M***') found within 40 bytes.
     */
    buf = ci->request_buffer(&size, 40);
    if (size < 40)
    {
        DEBUGF("CODEC_ERROR: smaf is too small\n");
        return false;
    }

    size = 0;
    while (size < 40 && buf[size] != 'M')
        size++;

    if (size >= 40)
    {
        DEBUGF("CODEC_ERROR: missing Score Track Stream PCM Data Chunk");
        return false;
    }

    /* search Score Track Stream PCM Data Chunk */
    ci->advance_buffer(size);
    *pos += size;
    if (search_chunk("Mtsp", 4, pos) == 0)
    {
        DEBUGF("CODEC_ERROR: missing Score Track Stream PCM Data Chunk\n");
        return false;
    }

    /*
     * parse Score Track Stream Wave Data Chunk
     *  buf
     *    +4-7: chunk size (WaveType(3bytes) + wave data count)
     *    +8:   bit 7 0:mono/1:stereo, bit 4-6 format, bit 0-3: base bit
     *    +9:   frequency (MSB)
     *    +10:  frequency (LSB)
     */
    buf = ci->request_buffer(&size, 9);
    if (size < 9)
    {
        DEBUGF("CODEC_ERROR: smaf is too small\n");
        return false;
    }

    if (memcmp(buf, "Mwa", 3) != 0)
    {
        DEBUGF("CODEC_ERROR: missing Score Track Stream Wave Data Chunk\n");
        return false;
    }

    fmt->formattag     = convert_smaf_audio_format(SMAF_SCORE_TRACK_CHUNK, buf[8]);
    fmt->channels      = convert_smaf_channels(buf[8]);
    fmt->bitspersample = convert_smaf_audio_basebit(buf[8] & 0xf);
    fmt->numbytes      = get_be32(buf + 4) - 3;

    *pos += 11;
    return true;
}

static bool parse_header(struct pcm_format *fmt, off_t *pos)
{
    const unsigned char *buf;
    unsigned int chunksize;
    size_t size;

    ci->memset(fmt, 0, sizeof(struct pcm_format));

    /* check File Chunk and Contents Info Chunk */
    buf = ci->request_buffer(&size, 16);
    if (size < 16)
    {
        DEBUGF("CODEC_ERROR: smaf is too small\n");
        return false;
    }

    if ((memcmp(buf, "MMMD", 4) != 0) || (memcmp(buf + 8, "CNTI", 4) != 0))
    {
        DEBUGF("CODEC_ERROR: does not smaf format\n");
        return false;
    }

    chunksize = get_be32(buf + 12);
    ci->advance_buffer(16);
    *pos = 16;
    if (chunksize > 5)
    {
        if (!parse_audio_track(fmt, chunksize, pos))
            return false;
    }
    else if (!parse_score_track(fmt, pos))
        return false;

    /* data signess (default signed) */
    fmt->is_signed = (fmt->formattag != SMAF_FORMAT_UNSIGNED_PCM);

    /* data is always big endian */
    fmt->is_little_endian = false;

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

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_SAMPLE_DEPTH, PCM_OUTPUT_DEPTH-1);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    uint32_t decodedsamples;
    size_t n;
    int bufcount;
    int endofstream;
    uint8_t *smafbuf;
    off_t firstblockposn;     /* position of the first block in file */
    const struct pcm_codec *codec;
    intptr_t param;
  
    if (codec_init())
        return CODEC_ERROR;

    codec_set_replaygain(ci->id3);

    /* Need to save offset for later use (cleared indirectly by advance_buffer) */
    bytesdone = ci->id3->offset;

    decodedsamples = 0;
    codec = 0;

    ci->seek_buffer(0);
    if (!parse_header(&format, &firstblockposn))
    {
        return CODEC_ERROR;
    }

    codec = get_codec(format.formattag);
    if (codec == 0)
    {
        DEBUGF("CODEC_ERROR: unsupport audio format: 0x%x\n", (int)format.formattag);
        return CODEC_ERROR;
    }

    if (!codec->set_format(&format))
    {
        return CODEC_ERROR;
    }

    /* check chunksize */
    if ((format.chunksize / format.blockalign) * format.samplesperblock * format.channels
           > PCM_SAMPLE_SIZE)
        format.chunksize = (PCM_SAMPLE_SIZE / format.blockalign) * format.blockalign;
    if (format.chunksize == 0)
    {
        DEBUGF("CODEC_ERROR: chunksize is 0\n");
        return CODEC_ERROR;
    }

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);

    if (format.channels == 2) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    } else if (format.channels == 1) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    } else {
        DEBUGF("CODEC_ERROR: more than 2 channels unsupported\n");
        return CODEC_ERROR;
    }

    ci->seek_buffer(firstblockposn);

    /* make sure we're at the correct offset */
    if (bytesdone > (uint32_t) firstblockposn)
    {
        /* Round down to previous block */
        struct pcm_pos *newpos = codec->get_seek_pos(bytesdone - firstblockposn,
                                                     PCM_SEEK_POS, &read_buffer);

        if (newpos->pos > format.numbytes)
            return CODEC_OK;

        if (ci->seek_buffer(firstblockposn + newpos->pos))
        {
            bytesdone      = newpos->pos;
            decodedsamples = newpos->samples;
        }
    }
    else
    {
        /* already where we need to be */
        bytesdone = 0;
    }

    ci->set_elapsed(decodedsamples*1000LL/ci->id3->frequency);

    /* The main decoder loop */
    endofstream = 0;

    while (!endofstream) {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            struct pcm_pos *newpos = codec->get_seek_pos(param, PCM_SEEK_TIME,
                                                         &read_buffer);

            if (newpos->pos > format.numbytes)
            {
                ci->set_elapsed(ci->id3->length);
                ci->seek_complete();
                break;
            }

            if (ci->seek_buffer(firstblockposn + newpos->pos))
            {
                bytesdone      = newpos->pos;
                decodedsamples = newpos->samples;
            }

            ci->set_elapsed(decodedsamples*1000LL/ci->id3->frequency);
            ci->seek_complete();
        }

        smafbuf = (uint8_t *)ci->request_buffer(&n, format.chunksize);

        if (n == 0)
            break; /* End of stream */

        if (bytesdone + n > format.numbytes) {
            n = format.numbytes - bytesdone;
            endofstream = 1;
        }

        if (codec->decode(smafbuf, n, samples, &bufcount) == CODEC_ERROR)
        {
            DEBUGF("codec error\n");
            return CODEC_ERROR;
        }

        ci->pcmbuf_insert(samples, NULL, bufcount);

        ci->advance_buffer(n);
        bytesdone += n;
        decodedsamples += bufcount;
        if (bytesdone >= format.numbytes)
            endofstream = 1;

        ci->set_elapsed(decodedsamples*1000LL/ci->id3->frequency);
    }

    return CODEC_OK;
}
