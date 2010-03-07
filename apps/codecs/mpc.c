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

MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH] IBSS_ATTR;

/* Our implementations of the mpc_reader callback functions. */
static mpc_int32_t read_impl(mpc_reader *reader, void *ptr, mpc_int32_t size)
{
    struct codec_api *ci = (struct codec_api *)(reader->data);
    return ((mpc_int32_t)(ci->read_filebuf(ptr, size)));
}

static mpc_bool_t seek_impl(mpc_reader *reader, mpc_int32_t offset)
{  
    struct codec_api *ci = (struct codec_api *)(reader->data);

    /* WARNING: assumes we don't need to skip too far into the past,
       this might not be supported by the buffering layer yet */
    return ci->seek_buffer(offset);
}

static mpc_int32_t tell_impl(mpc_reader *reader)
{
    struct codec_api *ci = (struct codec_api *)(reader->data);

    return ci->curpos;
}

static mpc_int32_t get_size_impl(mpc_reader *reader)
{
    struct codec_api *ci = (struct codec_api *)(reader->data);
    
    return ci->filesize;
}

static mpc_bool_t canseek_impl(mpc_reader *reader)
{
    (void)reader;
    
    /* doesn't much matter, libmusepack ignores this anyway */
    return true;
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    mpc_int64_t samplesdone;
    uint32_t frequency; /* 0.1 kHz accuracy */
    uint32_t elapsed_time; /* milliseconds */
    mpc_status status;
    mpc_reader reader;
    mpc_streaminfo info;
    mpc_frame_info frame;
    mpc_demux *demux = NULL;
    int retval = CODEC_OK;
    
    frame.buffer = sample_buffer;
    
    /* musepack's sample representation is 18.14
     * DSP_SET_SAMPLE_DEPTH = 14 (FRACT) + 16 (NATIVE) - 1 (SIGN) = 29 */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 29);
    
    /* Create a decoder instance */
    reader.read     = read_impl;
    reader.seek     = seek_impl;
    reader.tell     = tell_impl;
    reader.get_size = get_size_impl;
    reader.canseek  = canseek_impl;
    reader.data     = ci;

next_track:    
    if (codec_init()) 
    {
        retval = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    samplesdone = ci->id3->offset;

    /* initialize demux/decoder */
    demux = mpc_demux_init(&reader);
    if (NULL == demux)
    {
        retval = CODEC_ERROR;
        goto done;
    }
    /* read file's streaminfo data */
    mpc_demux_get_info(demux, &info);
    

    frequency = info.sample_freq / 100; /* 0.1 kHz accuracy */
    ci->configure(DSP_SWITCH_FREQUENCY, info.sample_freq);
        
    /* set playback engine up for correct number of channels */
    /* NOTE: current musepack format only allows for stereo files
       but code is here to handle other configurations anyway */
    if      (info.channels == 2)
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
    else if (info.channels == 1)
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    else 
    {
       retval = CODEC_ERROR;
       goto done;
    }
    
    codec_set_replaygain(ci->id3);

    /* Resume to saved sample offset. */
    if (samplesdone > 0) {
        /* hack to improve seek time if filebuf goes empty */
        if (mpc_demux_seek_sample(demux, samplesdone) == MPC_STATUS_OK) 
        {
            elapsed_time = (samplesdone*10)/frequency;
            ci->set_elapsed(elapsed_time);
        } 
        else 
        {
            samplesdone = 0;
        }
        /* reset chunksize */
    }

    /* This is the decoding loop. */
    do {
       /* Complete seek handler. */
        if (ci->seek_time) 
        {
            /* hack to improve seek time if filebuf goes empty */
            mpc_int64_t new_offset = ((ci->seek_time - 1)/10)*frequency;
            if (mpc_demux_seek_sample(demux, new_offset) == MPC_STATUS_OK) 
            {
                samplesdone = new_offset;
                ci->set_elapsed(ci->seek_time);
            }
            ci->seek_complete();
            /* reset chunksize */
        }
        if (ci->stop_codec || ci->new_track)
            break;

        status = mpc_demux_decode(demux, &frame);
        ci->yield();
        if (frame.bits == -1) /* decoding stopped */
        {
            retval = (status == MPC_STATUS_OK) ? CODEC_OK : CODEC_ERROR;
            goto done;
        } 
        else 
        {
            ci->pcmbuf_insert(frame.buffer,
                              frame.buffer + MPC_FRAME_LENGTH,
                              frame.samples);
            samplesdone += frame.samples;
            elapsed_time = (samplesdone*10)/frequency;
            ci->set_elapsed(elapsed_time);
            ci->set_offset(samplesdone);
        }
    } while (true);

done:
    if (ci->request_next_track())
        goto next_track;

exit:
    return retval;
}

