/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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

/* Sun Audio file (Au file format) codec
 *
 * References
 * [1] Sun Microsystems, Inc., Header file for Audio, .au, 1992
 *     URL http://www.opengroup.org/public/pubs/external/auformat.html
 * [2] Wikipedia, Au file format, URL: http://en.wikipedia.org/wiki/Sun_Audio
 */

#define PCM_SAMPLE_SIZE (1024*2)

static int32_t samples[PCM_SAMPLE_SIZE] IBSS_ATTR;

enum
{
    AU_FORMAT_UNSUPPORT = 0, /* unsupported format */
    AU_FORMAT_MULAW,         /* G.711 MULAW */
    AU_FORMAT_PCM,           /* Linear PCM */
    AU_FORMAT_IEEE_FLOAT,    /* IEEE float */
    AU_FORMAT_ALAW,          /* G.711 ALAW */
};

static const char support_formats[9][2] = {
  { AU_FORMAT_UNSUPPORT,  0  }, /* encoding */
  { AU_FORMAT_MULAW,      8  }, /* 1:  G.711 MULAW */
  { AU_FORMAT_PCM,        8  }, /* 2:  Linear PCM 8bit (signed) */
  { AU_FORMAT_PCM,        16 }, /* 3:  Linear PCM 16bit (signed, big endian) */
  { AU_FORMAT_PCM,        24 }, /* 4:  Linear PCM 24bit (signed, big endian) */
  { AU_FORMAT_PCM,        32 }, /* 5:  Linear PCM 32bit (signed, big endian) */
  { AU_FORMAT_IEEE_FLOAT, 32 }, /* 6:  Linear PCM float 32bit (signed, big endian) */
  { AU_FORMAT_IEEE_FLOAT, 64 }, /* 7:  Linear PCM float 64bit (signed, big endian) */
                                /* encoding 8 - 26 unsupported. */
  { AU_FORMAT_ALAW,       8  }, /* 27: G.711 ALAW */
};

static const struct pcm_entry au_codecs[] = {
    { AU_FORMAT_MULAW,      get_itut_g711_mulaw_codec },
    { AU_FORMAT_PCM,        get_linear_pcm_codec      },
    { AU_FORMAT_IEEE_FLOAT, get_ieee_float_codec      },
    { AU_FORMAT_ALAW,       get_itut_g711_alaw_codec  },
};

#define NUM_FORMATS 4

static const struct pcm_codec *get_au_codec(uint32_t formattag)
{
    int i;

    for (i = 0; i < NUM_FORMATS; i++)
    {
        if (au_codecs[i].format_tag == formattag)
        {
            if (au_codecs[i].get_codec)
                return au_codecs[i].get_codec();
            return 0;
        }
    }
    return 0;
}

static unsigned int get_be32(uint8_t *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static int convert_au_format(unsigned int encoding, struct pcm_format *fmt)
{
    fmt->formattag = AU_FORMAT_UNSUPPORT;
    if (encoding < 8)
    {
        fmt->formattag = support_formats[encoding][0];
        fmt->bitspersample = support_formats[encoding][1];
    }
    else if (encoding == 27)
    {
        fmt->formattag = support_formats[8][0];
        fmt->bitspersample = support_formats[8][1];
    }

    return fmt->formattag;
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
    struct pcm_format format;
    uint32_t bytesdone, decodedsamples;
    size_t n;
    int bufcount;
    int endofstream;
    unsigned char *buf;
    uint8_t *aubuf;
    off_t firstblockposn;     /* position of the first block in file */
    const struct pcm_codec *codec;
    int offset = 0;
    intptr_t param;
 
    if (codec_init()) {
        DEBUGF("codec_init() error\n");
        return CODEC_ERROR;
    }

    codec_set_replaygain(ci->id3);
    
    /* Need to save offset for later use (cleared indirectly by advance_buffer) */
    bytesdone = ci->id3->offset;

    ci->memset(&format, 0, sizeof(struct pcm_format));
    format.is_signed = true;
    format.is_little_endian = false;

    /* set format */
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, 24);
    if (n < 24 || (memcmp(buf, ".snd", 4) != 0))
    {
        /*
         * headerless sun audio file
         * It is decoded under conditions.
         *     format:    G.711 mu-law
         *     channel:   mono
         *     frequency: 8000 kHz
         */
        offset = 0;
        format.formattag     = AU_FORMAT_MULAW;
        format.channels      = 1;
        format.bitspersample = 8;
        format.numbytes      = ci->id3->filesize;
    }
    else
    {
        /* parse header */

        /* data offset */
        offset = get_be32(buf + 4);
        if (offset < 24)
        {
            DEBUGF("CODEC_ERROR: sun audio offset size is small: %d\n", offset);
            return CODEC_ERROR;
        }
        /* data size */
        format.numbytes = get_be32(buf + 8);
        if (format.numbytes == (uint32_t)0xffffffff)
            format.numbytes = ci->id3->filesize - offset;
        /* encoding */
        format.formattag = convert_au_format(get_be32(buf + 12), &format);
        if (format.formattag == AU_FORMAT_UNSUPPORT)
        {
            DEBUGF("CODEC_ERROR: sun audio unsupport format: %d\n", get_be32(buf + 12));
            return CODEC_ERROR;
        }
        /* skip sample rate */
        format.channels = get_be32(buf + 20);
    }

    /* advance to first WAVE chunk */
    ci->advance_buffer(offset);

    firstblockposn = offset;

    decodedsamples = 0;
    codec = 0;

    /* get codec */
    codec = get_au_codec(format.formattag);
    if (!codec)
    {
        DEBUGF("CODEC_ERROR: unsupport sun audio format: %x\n", (int)format.formattag);
        return CODEC_ERROR;
    }

    if (!codec->set_format(&format))
    {
        return CODEC_ERROR;
    }

    if (format.numbytes == 0) {
        DEBUGF("CODEC_ERROR: data size is 0\n");
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
        DEBUGF("CODEC_ERROR: more than 2 channels\n");
        return CODEC_ERROR;
    }

    /* make sure we're at the correct offset */
    if (bytesdone > (uint32_t) firstblockposn) {
        /* Round down to previous block */
        struct pcm_pos *newpos = codec->get_seek_pos(bytesdone - firstblockposn,
                                                     PCM_SEEK_POS, NULL);

        if (newpos->pos > format.numbytes)
            goto done;
        if (ci->seek_buffer(firstblockposn + newpos->pos))
        {
            bytesdone      = newpos->pos;
            decodedsamples = newpos->samples;
        }
    } else {
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
            /* 3rd args(read_buffer) is unnecessary in the format which Sun Audio supports.  */
            struct pcm_pos *newpos = codec->get_seek_pos(param, PCM_SEEK_TIME, NULL);

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

        aubuf = (uint8_t *)ci->request_buffer(&n, format.chunksize);
        if (n == 0)
            break; /* End of stream */
        if (bytesdone + n > format.numbytes) {
            n = format.numbytes - bytesdone;
            endofstream = 1;
        }

        if (codec->decode(aubuf, n, samples, &bufcount) == CODEC_ERROR)
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

done:
    return CODEC_OK;
}
