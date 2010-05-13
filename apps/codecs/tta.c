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
#include "codecs/libtta/ttalib.h"

CODEC_HEADER

/*
 * TTA (True Audio) codec:
 *
 * References
 * [1] TRUE AUDIO CODEC SOFTWARE http://true-audio.com/
 */

static int32_t samples[PCM_BUFFER_LENGTH * 2] IBSS_ATTR;

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    tta_info info;
    int status = CODEC_OK;
    unsigned int decodedsamples = 0;
    int endofstream;
    int new_pos = 0;
    int sample_count;

    /* Generic codec initialisation */
    ci->configure(DSP_SET_SAMPLE_DEPTH, TTA_OUTPUT_DEPTH - 1);
  
    if (codec_init())
    {
        DEBUGF("codec_init() error\n");
        status = CODEC_ERROR;
        goto exit;
    }

next_track:
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    if (set_tta_info(&info) < 0)
    {
        status = CODEC_ERROR;
        goto exit;
    }
    if (player_init(&info) < 0)
    {
        status = CODEC_ERROR;
        goto exit;
    }

    codec_set_replaygain(ci->id3);

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    if (info.NCH == 2) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    } else if (info.NCH == 1) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    } else {
        DEBUGF("CODEC_ERROR: more than 2 channels\n");
        status = CODEC_ERROR;
        goto done;
    }

    /* The main decoder loop */
    endofstream = 0;

    if (ci->id3->offset > 0)
    {
        /* Need to save offset for later use (cleared indirectly by advance_buffer) */
        new_pos = set_position(ci->id3->offset, TTA_SEEK_POS);
        if (new_pos >= 0)
            decodedsamples = new_pos;
        ci->seek_complete();
    }

    while (!endofstream)
    {
        ci->yield();
        if (ci->stop_codec || ci->new_track)
            break;

        if (ci->seek_time)
        {
            new_pos = set_position(ci->seek_time / SEEK_STEP, TTA_SEEK_TIME);
            if (new_pos >= 0)
            {
                decodedsamples = new_pos;
                ci->seek_complete();
            }
        }

        sample_count = get_samples(samples);
        if (sample_count < 0)
        {
            status = CODEC_ERROR;
            break;
        }
        ci->pcmbuf_insert(samples, NULL, sample_count);
        decodedsamples += sample_count;
        if (decodedsamples >= info.DATALENGTH)
            endofstream = 1;
        ci->set_elapsed((uint64_t)info.LENGTH * 1000 * decodedsamples / info.DATALENGTH);
    }
    status = CODEC_OK;
done:
    player_stop();
    if (ci->request_next_track())
        goto next_track;

exit:
    return status;
}
