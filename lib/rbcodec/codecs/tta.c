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
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_SAMPLE_DEPTH, TTA_OUTPUT_DEPTH - 1);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    tta_info info;
    unsigned int decodedsamples;
    int endofstream;
    int new_pos = 0;
    int sample_count;
    intptr_t param;
  
    if (codec_init())
    {
        DEBUGF("codec_init() error\n");
        return CODEC_ERROR;
    }

    ci->seek_buffer(0);

    if (set_tta_info(&info) < 0 || player_init(&info) < 0)
        return CODEC_ERROR;

    codec_set_replaygain(ci->id3);

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    if (info.NCH == 2) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    } else if (info.NCH == 1) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    } else {
        DEBUGF("CODEC_ERROR: more than 2 channels\n");
        player_stop();
        return CODEC_ERROR;
    }

    /* The main decoder loop */
    decodedsamples = 0;
    endofstream = 0;

    if (ci->id3->offset > 0)
    {
        /* Need to save offset for later use (cleared indirectly by advance_buffer) */
        new_pos = set_position(ci->id3->offset, TTA_SEEK_POS);
        if (new_pos >= 0)
            decodedsamples = new_pos;
    }

    ci->set_elapsed((uint64_t)info.LENGTH * 1000 * decodedsamples / info.DATALENGTH);

    while (!endofstream)
    {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME)
        {
            new_pos = set_position(param / SEEK_STEP, TTA_SEEK_TIME);
            if (new_pos >= 0)
            {
                decodedsamples = new_pos;
            }

            ci->set_elapsed((uint64_t)info.LENGTH * 1000 * decodedsamples / info.DATALENGTH);
            ci->seek_complete();
        }

        sample_count = get_samples(samples);
        if (sample_count < 0)
            break;
 
        ci->pcmbuf_insert(samples, NULL, sample_count);
        decodedsamples += sample_count;
        if (decodedsamples >= info.DATALENGTH)
            endofstream = 1;
        ci->set_elapsed((uint64_t)info.LENGTH * 1000 * decodedsamples / info.DATALENGTH);
    }

    player_stop();
    return CODEC_OK;
}
