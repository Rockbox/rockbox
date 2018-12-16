/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
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
#include <inttypes.h>  /* Needed by a52.h */
#include <codecs/liba52/config-a52.h>
#include <codecs/liba52/a52.h>

CODEC_HEADER

#define BUFFER_SIZE 4096

#define A52_SAMPLESPERFRAME (6*256)

static a52_state_t *state;
static unsigned long samplesdone;
static unsigned long frequency;

/* used outside liba52 */
static uint8_t buf[3840] IBSS_ATTR;
static uint8_t *bufptr = buf;
static uint8_t *bufpos = buf + 7;

static void a52_decoder_reset(void)
{
    bufptr = buf;
    bufpos = buf + 7;
}

static inline void output_audio(sample_t *samples)
{
    ci->yield();
    ci->pcmbuf_insert(&samples[0], &samples[256], 256);
}

static size_t a52_decode_data(uint8_t *start, uint8_t *end)
{
    /*
     * sample_rate and flags are static because this routine could
     * exit between the a52_syncinfo() and the ao_setup(), and we want
     * to have the same values when we get back !
     */
    static int sample_rate;
    static int flags;
    int bit_rate;
    int len;
    size_t consumed = 0;

    while (1) {
        len = end - start;
        if (!len)
            break;
        if (len > bufpos - bufptr)
            len = bufpos - bufptr;
        memcpy(bufptr, start, len);
        bufptr += len;
        start += len;
        consumed += len;
        if (bufptr == bufpos) {
            if (bufpos == buf + 7) {
                int length;

                length = a52_syncinfo(buf, &flags, &sample_rate, &bit_rate);
                if (!length) {
                    //DEBUGF("skip\n");
                    for (bufptr = buf; bufptr < buf + 6; bufptr++)
                        bufptr[0] = bufptr[1];
                    continue;
                }
                bufpos = buf + length;
            } else {
                /* Unity gain is 1 << 26, and we want to end up on 28 bits
                   of precision instead of the default 30.
                 */
                level_t level = 1 << 24;
                sample_t bias = 0;
                int i;

                /* This is the configuration for the downmixing: */
                flags = A52_STEREO | A52_ADJUST_LEVEL;

                if (a52_frame(state, buf, &flags, &level, bias))
                    goto error;
                a52_dynrng(state, NULL, NULL);
                frequency = sample_rate;

                /* An A52 frame consists of 6 blocks of 256 samples
                   So we decode and output them one block at a time */
                for (i = 0; i < 6; i++) {
                    if (a52_block(state))
                        goto error;
                    output_audio(a52_samples(state));
                    samplesdone += 256;
                }
                ci->set_elapsed(samplesdone/(frequency/1000));
                bufptr = buf;
                bufpos = buf + 7;
                break;
            error:
                //logf("Error decoding A52 stream\n");
                bufptr = buf;
                bufpos = buf + 7;
            }
        }   
    }
    return consumed;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
        ci->configure(DSP_SET_SAMPLE_DEPTH, 28);
    }
    else if (reason == CODEC_UNLOAD) {
        if (state)
            a52_free(state);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    size_t n, consumed = 0, rest = 0;
    unsigned char *filebuf;
    int sample_loc;
    intptr_t param;

    if (codec_init())
        return CODEC_ERROR;

    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);
    
    /* Initialise the A52 decoder and check for success */
    state = a52_init(0);

    samplesdone = 0;

    if (ci->id3->offset) {
        sample_loc = (ci->id3->offset / ci->id3->bytesperframe) *
                        A52_SAMPLESPERFRAME;
        param = ci->id3->offset;
    }
    else if (ci->id3->elapsed) {
        sample_loc = ci->id3->elapsed/1000 * ci->id3->frequency;
        param = sample_loc/A52_SAMPLESPERFRAME*ci->id3->bytesperframe;
    }
    else {
        sample_loc = 0;
        param = ci->id3->first_frame_offset;
    }

    if (ci->seek_buffer(param)) {
        samplesdone = sample_loc;
    }

    ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));

    /* The main decoding loop */

    while (1) {
        long action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            sample_loc = param/1000 * ci->id3->frequency;

            if (ci->seek_buffer((sample_loc/A52_SAMPLESPERFRAME)*
                                ci->id3->bytesperframe)) {
                samplesdone = sample_loc;
                ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));
            }
            ci->seek_complete();
            a52_decoder_reset();
        }

        filebuf = ci->request_buffer(&n, BUFFER_SIZE);

        if (n == 0) /* End of Stream */
        {
            filebuf += consumed;
            for (n = ci->curpos + consumed; rest; rest -= consumed, n += consumed)
            {
                ci->set_offset(n);
                consumed = a52_decode_data(filebuf, filebuf + rest);
            }
            break;
        }

        consumed = a52_decode_data(filebuf, filebuf + n);
        rest = n - consumed;
        ci->advance_buffer(consumed);
    }

    return CODEC_OK;
}
