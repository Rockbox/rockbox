/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Codec for aac files without container
 *
 * Written by Igor B. Poretsky
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
#include "libfaad/common.h"
#include "libfaad/structs.h"
#include "libfaad/decoder.h"

CODEC_HEADER

/* The maximum buffer size handled by faad. 12 bytes are required by libfaad
 * as headroom (see libfaad/bits.c). FAAD_BYTE_BUFFER_SIZE bytes are buffered
 * for each frame. */
#define FAAD_BYTE_BUFFER_SIZE (2048-12)

static void update_playing_time(void)
{
    ci->set_elapsed((unsigned long)((ci->id3->offset - ci->id3->first_frame_offset) * 8LL / ci->id3->bitrate));
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
        ci->configure(DSP_SET_SAMPLE_DEPTH, 29);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    size_t n;
    int32_t bread;
    unsigned int frame_samples;
    uint32_t s = 0;
    unsigned char c = 0;
    long action = CODEC_ACTION_NULL;
    intptr_t param;
    unsigned char* buffer;
    NeAACDecFrameInfo frame_info;
    NeAACDecHandle decoder;
    NeAACDecConfigurationPtr conf;

    /* Clean and initialize decoder structures */
    if (codec_init()) {
        LOGF("FAAD: Codec init error\n");
        return CODEC_ERROR;
    }

    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);

    ci->seek_buffer(ci->id3->first_frame_offset);

    /* initialise the sound converter */
    decoder = NeAACDecOpen();

    if (!decoder) {
        LOGF("FAAD: Decode open error\n");
        return CODEC_ERROR;
    }

    conf = NeAACDecGetCurrentConfiguration(decoder);
    conf->outputFormat = FAAD_FMT_24BIT; /* irrelevant, we don't convert */
    NeAACDecSetConfiguration(decoder, conf);

    buffer=ci->request_buffer(&n, FAAD_BYTE_BUFFER_SIZE);
    bread = NeAACDecInit(decoder, buffer, n, &s, &c);
    if (bread < 0) {
        LOGF("FAAD: DecInit: %ld, %d\n", bread, decoder->object_type);
        return CODEC_ERROR;
    }
    ci->advance_buffer(bread);

    if (ci->id3->offset > ci->id3->first_frame_offset) {
        /* Resume the desired (byte) position. */
        ci->seek_buffer(ci->id3->offset);
        NeAACDecPostSeekReset(decoder, 0);
        update_playing_time();
    } else if (ci->id3->elapsed) {
        action = CODEC_ACTION_SEEK_TIME;
        param = ci->id3->elapsed;
    } else {
        ci->set_elapsed(0);
        ci->set_offset(ci->id3->first_frame_offset);
    }

    /* The main decoding loop */
    while (1) {
        if (action == CODEC_ACTION_NULL)
            action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        /* Deal with any pending seek requests */
        if (action == CODEC_ACTION_SEEK_TIME) {
            /* Seek to the desired time position. */
            ci->seek_buffer(ci->id3->first_frame_offset + (uint32_t)((uint64_t)param * ci->id3->bitrate / 8));
            ci->set_elapsed((unsigned long)param);
            NeAACDecPostSeekReset(decoder, 0);
            ci->seek_complete();
        }

        action = CODEC_ACTION_NULL;

        /* Request the required number of bytes from the input buffer */
        buffer=ci->request_buffer(&n, FAAD_BYTE_BUFFER_SIZE);

        if (n == 0) /* End of Stream */
            break;

        /* Decode one block - returned samples will be host-endian */
        if (NeAACDecDecode(decoder, &frame_info, buffer, n) == NULL || frame_info.error > 0) {
            LOGF("FAAD: decode error '%s'\n", NeAACDecGetErrorMessage(frame_info.error));
            return CODEC_ERROR;
        }

        /* Advance codec buffer (no need to call set_offset because of this) */
        ci->advance_buffer(frame_info.bytesconsumed);

        /* Output the audio */
        ci->yield();
        frame_samples = frame_info.samples >> 1;
        ci->pcmbuf_insert(&decoder->time_out[0][0], &decoder->time_out[1][0], frame_samples);

        /* Update the elapsed-time indicator */
        update_playing_time();
    }

    LOGF("AAC: Decoding complete\n");
    return CODEC_OK;
}
