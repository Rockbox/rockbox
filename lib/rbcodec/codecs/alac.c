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
#include "libm4a/m4a.h"
#include "libalac/decomp.h"

CODEC_HEADER

/* The maximum buffer size handled. This amount of bytes is buffered for each 
 * frame. */
#define ALAC_BYTE_BUFFER_SIZE 32768

static int32_t outputbuffer[ALAC_MAX_CHANNELS][ALAC_BLOCKSIZE] IBSS_ATTR;

static void set_elapsed_samples(uint32_t samplesdone)
{
    uint32_t elapsedtime = (uint64_t)samplesdone * 1000ULL / ci->id3->frequency;
    ci->set_elapsed(elapsedtime);
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
        ci->configure(DSP_SET_SAMPLE_DEPTH, ALAC_OUTPUT_DEPTH-1);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    size_t n;
    demux_res_t demux_res;
    stream_t input_stream;
    uint32_t samplesdone;
    int samplesdecoded;
    unsigned int i;
    unsigned char* buffer;
    alac_file alac;
    intptr_t param;
    unsigned long resume_time;
    uint32_t resume_offset;
    unsigned int did_resume;

    /* Clean and initialize decoder structures */
    memset(&demux_res , 0, sizeof(demux_res));
    if (codec_init()) {
        LOGF("ALAC: Error initialising codec\n");
        return CODEC_ERROR;
    }

    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);

    ci->seek_buffer(0);

    stream_create(&input_stream,ci);

    /* Save resume info because qtmovie_read() can modify it. */
    resume_time = ci->id3->elapsed;
    resume_offset = ci->id3->offset;

    /* if qtmovie_read returns successfully, the stream is up to
     * the movie data, which can be used directly by the decoder */
    if (!qtmovie_read(&input_stream, &demux_res)) {
        LOGF("ALAC: Error initialising file\n");
        return CODEC_ERROR;
    }

    /* initialise the sound converter */
    alac_set_info(&alac, demux_res.codecdata);

    if (resume_time)
        did_resume = m4a_seek(&demux_res, &input_stream,
                              (uint64_t)resume_time * ci->id3->frequency / 1000ULL,
                              &samplesdone, (int *) &i);
    else if (resume_offset)
        did_resume = m4a_seek_raw(&demux_res, &input_stream, resume_offset,
                                  &samplesdone, (int *) &i);
    else
        did_resume = 0;

    /* Start from the beginning if we did not resume. */
    if (!did_resume) {
        i = 0;
        samplesdone = 0;
    }

    set_elapsed_samples(samplesdone);

    /* The main decoding loop */
    while (i < demux_res.num_sample_byte_sizes) {
        long action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        /* Deal with any pending seek requests */
        if (action == CODEC_ACTION_SEEK_TIME) {
            if (m4a_seek(&demux_res, &input_stream,
                         (uint64_t)param * ci->id3->frequency / 1000ULL,
                         &samplesdone, (int *) &i))
                set_elapsed_samples(samplesdone);

            ci->seek_complete();
        }

        /* Request the required number of bytes from the input buffer */
        buffer=ci->request_buffer(&n, ALAC_BYTE_BUFFER_SIZE);

        /* Decode one block - returned samples will be host-endian */
        samplesdecoded=alac_decode_frame(&alac, buffer, outputbuffer, ci->yield);
        ci->yield();

        /* Advance codec buffer by amount of consumed bytes */
        ci->advance_buffer(alac.bytes_consumed);

        /* Output the audio */
        ci->pcmbuf_insert(outputbuffer[0], outputbuffer[1], samplesdecoded);

        /* Update the elapsed-time indicator */
        samplesdone+=samplesdecoded;
        set_elapsed_samples(samplesdone);

        i++;
    }

    LOGF("ALAC: Decoded %lu samples\n",(unsigned long)samplesdone);
    return CODEC_OK;
}
