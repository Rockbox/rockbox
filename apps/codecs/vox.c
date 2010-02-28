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
enum codec_status codec_main(void)
{
    int status = CODEC_OK;
    uint32_t decodedsamples;
    size_t n;
    int bufcount;
    int endofstream;
    uint8_t *voxbuf;
    off_t firstblockposn;     /* position of the first block in file */
    const struct pcm_codec *codec;
    int offset = 0;

    /* Generic codec initialisation */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 28);
  
next_track:
    if (codec_init()) {
        DEBUGF("codec_init() error\n");
        status = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);
    
    ci->memset(&format, 0, sizeof(struct pcm_format));

    /* set format */
    format.channels      = 1;
    format.bitspersample = 4;
    format.numbytes      = ci->id3->filesize;
    format.blockalign    = 1;

    /* advance to first WAVE chunk */
    ci->advance_buffer(offset);

    firstblockposn = offset;

    decodedsamples = 0;
    bytesdone = 0;

    /*
     * get codec
     * supports dialogic oki adpcm only
     */
    codec = get_dialogic_oki_adpcm_codec();
    if (!codec)
    {
        DEBUGF("CODEC_ERROR: dialogic oki adpcm codec does not load.\n");
        status = CODEC_ERROR;
        goto done;
    }

    if (!codec->set_format(&format))
    {
        status = CODEC_ERROR;
        goto done;
    }

    if (format.numbytes == 0) {
        DEBUGF("CODEC_ERROR: data size is 0\n");
        status = CODEC_ERROR;
        goto done;
    }

    /* check chunksize */
    if (format.chunksize * 2 > PCM_SAMPLE_SIZE)
        format.chunksize = PCM_SAMPLE_SIZE / 2;
    if (format.chunksize == 0)
    {
        DEBUGF("CODEC_ERROR: chunksize is 0\n");
        status = CODEC_ERROR;
        goto done;
    }

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);

    /* The main decoder loop */
    endofstream = 0;

    while (!endofstream) {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }

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

        voxbuf = (uint8_t *)ci->request_buffer(&n, format.chunksize);
        if (n == 0)
            break; /* End of stream */
        if (bytesdone + n > format.numbytes) {
            n = format.numbytes - bytesdone;
            endofstream = 1;
        }

        status = codec->decode(voxbuf, n, samples, &bufcount);
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
    status = CODEC_OK;

done:
    if (ci->request_next_track())
        goto next_track;

exit:
    return status;
}
