/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Thom Johansen
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
#include <codecs/libmusepack/mpcdec.h>
#include <codecs/libmusepack/internal.h>

CODEC_HEADER

static MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH] IBSS_ATTR;

/* Our implementations of the mpc_reader callback functions. */
static mpc_int32_t read_impl(mpc_reader *reader, void *ptr, mpc_int32_t size)
{
    (void)reader;
    return ((mpc_int32_t)(ci->read_filebuf(ptr, size)));
}

static mpc_bool_t seek_impl(mpc_reader *reader, mpc_int32_t offset)
{
    (void)reader;
    return ci->seek_buffer(offset);
}

static mpc_int32_t tell_impl(mpc_reader *reader)
{
    (void)reader;
    return ci->curpos;
}

static mpc_int32_t get_size_impl(mpc_reader *reader)
{
    (void)reader;
    return ci->filesize;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* musepack's sample representation is 18.14
         * DSP_SET_SAMPLE_DEPTH = 14 (FRACT) + 16 (NATIVE) - 1 (SIGN) = 29 */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 29);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    mpc_int64_t samplesdone;
    uint32_t frequency;     /* 0.1 kHz accuracy */
    uint32_t elapsed_time;  /* milliseconds */
    uint32_t byterate;      /* bytes per second */
    mpc_status status;
    mpc_reader reader;
    mpc_streaminfo info;
    mpc_frame_info frame;
    mpc_demux *demux = NULL;
    intptr_t param;
    
    frame.buffer = sample_buffer;

    /* Create a decoder instance */
    reader.read     = read_impl;
    reader.seek     = seek_impl;
    reader.tell     = tell_impl;
    reader.get_size = get_size_impl;

    if (codec_init()) 
        return CODEC_ERROR;

    /* Prep position */
    ci->seek_buffer(0);

    /* Initialize demux/decoder. */
    demux = mpc_demux_init(&reader);
    if (NULL == demux)
        return CODEC_ERROR;

    /* Read file's streaminfo data. */
    mpc_demux_get_info(demux, &info);
    
    byterate  = (mpc_uint32_t)(info.average_bitrate) / 8;
    frequency = info.sample_freq / 100; /* 0.1 kHz accuracy */
    ci->configure(DSP_SET_FREQUENCY, info.sample_freq);

    /* Remark: rockbox offset is the file offset in bytes. So, estimate the 
     * sample seek position from the file offset, the sampling frequency and
     * the bitrate. As the saved position is exactly calculated the reverse way 
     * there is no loss of information except rounding. */
    elapsed_time = ci->id3->elapsed;
    samplesdone = 100 * (((mpc_uint64_t)ci->id3->offset * frequency) / byterate);
        
    /* Set up digital signal processing for correct number of channels */
    /* NOTE: current musepack format only allows for stereo files
       but code is here to handle other configurations anyway */
    if      (info.channels == 2)
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
    else if (info.channels == 1)
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    else
        return CODEC_ERROR;
    
    codec_set_replaygain(ci->id3);

    if (samplesdone > 0 || elapsed_time)
    {
        mpc_int64_t new_offset = samplesdone;

        if (new_offset <= 0)
            new_offset = (elapsed_time/10)*frequency; /* by time */

        /* Resume to sample offset. */
        if (mpc_demux_seek_sample(demux, new_offset) == MPC_STATUS_OK)
        {
            samplesdone = new_offset;
        } 
        else 
        {
            samplesdone = 0;
        }

        elapsed_time = (samplesdone*10LL)/frequency;
    }

    ci->set_elapsed(elapsed_time);

    /* This is the decoding loop. */
    do 
    {
        long action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            return CODEC_OK;

        /* Complete seek handler. */
        if (action == CODEC_ACTION_SEEK_TIME) 
        {
            mpc_int64_t new_offset = (param/10)*frequency;
            if (mpc_demux_seek_sample(demux, new_offset) == MPC_STATUS_OK) 
            {
                samplesdone = new_offset;
            }

            elapsed_time = (samplesdone*10LL)/frequency;
            ci->set_elapsed(elapsed_time);
            ci->seek_complete();
        }

        /* Decode one frame. */
        status = mpc_demux_decode(demux, &frame);
        ci->yield();
        if (frame.bits == -1)
        {
            /* Decoding error, exit decoding loop. */
            return (status == MPC_STATUS_OK) ? CODEC_OK : CODEC_ERROR;
        } 
        else 
        {
            /* Decoding passed, insert samples to PCM buffer. */
            ci->pcmbuf_insert(frame.buffer,
                              frame.buffer + MPC_FRAME_LENGTH,
                              frame.samples);
            samplesdone += frame.samples;
            elapsed_time = (samplesdone*10LL)/frequency;
            ci->set_elapsed(elapsed_time);
            /* Remark: rockbox offset is the file offset in bytes. So estimate 
             * this offset from the samples, sampling frequency and bitrate */
            ci->set_offset( (samplesdone * byterate)/(frequency*100) );
        }
    } while (true);
}
