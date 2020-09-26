/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * VTX Codec for rockbox based on the Ayumi engine
 *
 * Ayumi engine Written by Peter Sovietov in 2015
 * Ported to rockbox '2019 by Roman Stolyarov
 *
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

#include <codecs/lib/codeclib.h>
#include "libayumi/ayumi_render.h" 

CODEC_HEADER

#define VTX_SAMPLE_RATE 44100

/* Maximum number of bytes to process in one iteration */
#define CHUNK_SIZE (1024*2)

static int16_t samples[CHUNK_SIZE] IBSS_ATTR;
extern ayumi_render_t ay;

/****************** rockbox interface ******************/

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* we only render 16 bits */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 16);

        /* 44 Khz, Interleaved stereo */
        ci->configure(DSP_SET_FREQUENCY, VTX_SAMPLE_RATE);
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    uint8_t *buf;
    size_t n;
    intptr_t param;
    uint32_t elapsed_time;
    long smp;
    int res;

    /* reset values */
    elapsed_time = 0;
    param = ci->id3->elapsed;

    DEBUGF("VTX: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }  

    codec_set_replaygain(ci->id3);

    /* Read the entire file */
    DEBUGF("VTX: request file\n");
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, ci->filesize);
    if (!buf || n < (size_t)ci->filesize) {
        DEBUGF("VTX: file load failed\n");
        return CODEC_ERROR;
    }

    res = AyumiRender_LoadFile((void *)buf, ci->filesize);
    if (!res) {
        DEBUGF("VTX: AyumiRender_LoadFile failed\n");
        return CODEC_ERROR;
    }

    res = AyumiRender_AyInit(ay.info.chiptype, VTX_SAMPLE_RATE, ay.info.chipfreq, ay.info.playerfreq, 1);
    if (!res) {
        DEBUGF("VTX: AyumiRender_AyInit failed\n");
        return CODEC_ERROR;
    }
    
    res = AyumiRender_SetLayout(ay.info.layout, 0);
    if (!res) {
        DEBUGF("VTX: AyumiRender_SetLayout failed\n");
        return CODEC_ERROR;
    }

    if (param) {
        goto resume_start;
    }

    /* The main decoder loop */
    while (1) {
        long action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
        resume_start:
            ci->set_elapsed(param);
            elapsed_time = param;
            ulong sample = ((ulong)elapsed_time * 441) / 10;
            AyumiRender_Seek(sample);
            ci->seek_complete();
        }
    
        /* Generate audio buffer */
        smp = AyumiRender_AySynth((void *)&samples[0], CHUNK_SIZE >> 1);

        ci->pcmbuf_insert(samples, NULL, smp);

        /* Set elapsed time for one track files */
        elapsed_time += smp * 10 / 441;
        ci->set_elapsed(elapsed_time);

        if (AyumiRender_GetPos() >= AyumiRender_GetMaxPos())
            break;
    }

    return CODEC_OK;
}
