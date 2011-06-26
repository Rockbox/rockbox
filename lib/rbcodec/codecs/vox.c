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

/* vox codec (Dialogic telephony file formats) */

#define PCM_SAMPLE_SIZE (2048)

static int32_t samples[PCM_SAMPLE_SIZE] IBSS_ATTR;

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
    uint8_t *voxbuf;
    off_t firstblockposn = 0;     /* position of the first block in file */
    const struct pcm_codec *codec;
    intptr_t param;

    if (codec_init()) {
        DEBUGF("codec_init() error\n");
        return CODEC_ERROR;
    }

    codec_set_replaygain(ci->id3);

    /* Need to save offset for later use (cleared indirectly by advance_buffer) */
    bytesdone = ci->id3->offset;
    ci->seek_buffer(0);

    ci->memset(&format, 0, sizeof(struct pcm_format));

    /* set format */
    format.channels      = 1;
    format.bitspersample = 4;
    format.numbytes      = ci->id3->filesize;
    format.blockalign    = 1;

    /* advance to first WAVE chunk */
    firstblockposn = 0;
    decodedsamples = 0;
    ci->advance_buffer(firstblockposn);

    /*
     * get codec
     * supports dialogic oki adpcm only
     */
    codec = get_dialogic_oki_adpcm_codec();
    if (!codec)
    {
        DEBUGF("CODEC_ERROR: dialogic oki adpcm codec does not load.\n");
        return CODEC_ERROR;
    }

    if (!codec->set_format(&format)) {
        return CODEC_ERROR;
    }

    if (format.numbytes == 0) {
        DEBUGF("CODEC_ERROR: data size is 0\n");
        return CODEC_ERROR;
    }

    /* check chunksize */
    if (format.chunksize * 2 > PCM_SAMPLE_SIZE)
        format.chunksize = PCM_SAMPLE_SIZE / 2;
    if (format.chunksize == 0)
    {
        DEBUGF("CODEC_ERROR: chunksize is 0\n");
        return CODEC_ERROR;
    }

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);

    /* make sure we're at the correct offset */
    if (bytesdone > (uint32_t) firstblockposn) {
        /* Round down to previous block */
        struct pcm_pos *newpos = codec->get_seek_pos(bytesdone - firstblockposn,
                                                     PCM_SEEK_POS, &read_buffer);

        if (newpos->pos > format.numbytes) {
            return CODEC_OK;
        }
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

        voxbuf = (uint8_t *)ci->request_buffer(&n, format.chunksize);
        if (n == 0)
            break; /* End of stream */
        if (bytesdone + n > format.numbytes) {
            n = format.numbytes - bytesdone;
            endofstream = 1;
        }

        if (codec->decode(voxbuf, n, samples, &bufcount) == CODEC_ERROR)
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
