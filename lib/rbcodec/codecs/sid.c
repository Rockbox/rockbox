/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * SID Codec for Rockbox using Hermit's cRSID library
 *
 * Written by Hermit (Mihaly Horvath) and Ninja (Wolfram Sang) in 2022/23.
 * Some generic codec handling taken from the former SID codec done by
 * Tammo Hinrichs, Rainer Sinsch, Dave Chapman, Stefan Waigand, Igor Poretsky,
 * Solomon Peachy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "cRSID/libcRSID.h"
#include "codeclib.h"
#include <inttypes.h>

CODEC_HEADER

/* bigger CHUNK_SIZE makes Clip sluggish when playing 2SIDs */
#define CHUNK_SIZE 512
#define SID_BUFFER_SIZE 0x10000
#define SAMPLE_RATE 44100

static int16_t samples[CHUNK_SIZE] IBSS_ATTR;

static void sid_render(int16_t *buffer, unsigned long len) ICODE_ATTR;

static void sid_render(int16_t *buffer, unsigned long len) {
    unsigned long bp;

    for (bp = 0; bp < len; bp++) {
        buffer[bp] = cRSID_generateSample(&cRSID_C64);
    }
}

enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        ci->configure(DSP_SET_FREQUENCY, SAMPLE_RATE);
        ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
        cRSID_init(SAMPLE_RATE, CHUNK_SIZE);
    }

    return CODEC_OK;
}

enum codec_status codec_run(void)
{

    cRSID_SIDheader *SIDheader;
    size_t filesize;
    unsigned char subSong;
    unsigned char *sidfile = NULL;
    intptr_t param;
    bool resume;
    long action;

    if (codec_init())
        return CODEC_ERROR;

    codec_set_replaygain(ci->id3);

    ci->seek_buffer(0);
    sidfile = ci->request_buffer(&filesize, SID_BUFFER_SIZE);
    if (filesize == 0)
        return CODEC_ERROR;

    param = ci->id3->elapsed;
    resume = param != 0;

    /* Start first song */
    action = CODEC_ACTION_SEEK_TIME;

    while (action != CODEC_ACTION_HALT) {
        if (action == CODEC_ACTION_SEEK_TIME) {
            /* Start playing from scratch */
            SIDheader = cRSID_processSIDfile(&cRSID_C64, sidfile, filesize);
            subSong = SIDheader->DefaultSubtune - 1;

            /* Now use the current seek time in seconds as subsong */
            if (!resume || (resume && param))
                subSong = param / 1000;

            /* Set the elapsed time to the current subsong (in seconds) */
            ci->set_elapsed(subSong * 1000);
            ci->seek_complete();
            resume = false;

            cRSID_initSIDtune(&cRSID_C64, SIDheader, subSong + 1);
        }

        sid_render(samples, CHUNK_SIZE);
        ci->pcmbuf_insert(samples, NULL, CHUNK_SIZE);

        /* New time is in param */
        action = ci->get_command(&param);
    }

    return CODEC_OK;
}
