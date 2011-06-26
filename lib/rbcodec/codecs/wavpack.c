/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 David Bryant
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
#include "libwavpack/wavpack.h"

CODEC_HEADER

#define BUFFER_SIZE 4096

static int32_t temp_buffer [BUFFER_SIZE] IBSS_ATTR;

static int32_t read_callback (void *buffer, int32_t bytes)
{
    int32_t retval = ci->read_filebuf (buffer, bytes);
    ci->set_offset(ci->curpos);
    return retval;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 28);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    WavpackContext *wpc;
    char error [80];
    /* rockbox: comment 'set but unused' variables
    int bps;
    */
    int nchans, sr_100;
    intptr_t param;

    if (codec_init())
        return CODEC_ERROR;

    ci->seek_buffer (ci->id3->offset);

    /* Create a decoder instance */
    wpc = WavpackOpenFileInput (read_callback, error);

    if (!wpc)
        return CODEC_ERROR;

    ci->configure(DSP_SWITCH_FREQUENCY, WavpackGetSampleRate (wpc));
    codec_set_replaygain(ci->id3);
    /* bps = WavpackGetBytesPerSample (wpc); */
    nchans = WavpackGetReducedChannels (wpc);
    ci->configure(DSP_SET_STEREO_MODE, nchans == 2 ? STEREO_INTERLEAVED : STEREO_MONO);
    sr_100 = ci->id3->frequency / 100;

    ci->set_elapsed (WavpackGetSampleIndex (wpc) / sr_100 * 10);

    /* The main decoder loop */

    while (1) {
        int32_t nsamples;
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            int curpos_ms = WavpackGetSampleIndex (wpc) / sr_100 * 10;
            int n, d, skip;

            if (param > curpos_ms) {
                n = param - curpos_ms;
                d = ci->id3->length - curpos_ms;
                skip = (int)((int64_t)(ci->filesize - ci->curpos) * n / d);
                ci->seek_buffer (ci->curpos + skip);
            }
            else if (curpos_ms != 0) {
                n = curpos_ms - param;
                d = curpos_ms;
                skip = (int)((int64_t) ci->curpos * n / d);
                ci->seek_buffer (ci->curpos - skip);
            }

            wpc = WavpackOpenFileInput (read_callback, error);
            if (!wpc)
            {
                ci->seek_complete();
                break;
            }

            ci->set_elapsed (WavpackGetSampleIndex (wpc) / sr_100 * 10);
            ci->seek_complete();
        }

        nsamples = WavpackUnpackSamples (wpc, temp_buffer, BUFFER_SIZE / nchans);  

        if (!nsamples)
            break;

        ci->pcmbuf_insert (temp_buffer, NULL, nsamples);
        ci->set_elapsed (WavpackGetSampleIndex (wpc) / sr_100 * 10);
    }

    return CODEC_OK;
}
