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
#include <codecs/libmusepack/musepack.h>

CODEC_HEADER

mpc_decoder decoder IBSS_ATTR;

/* Our implementations of the mpc_reader callback functions. */
mpc_int32_t read_impl(void *data, void *ptr, mpc_int32_t size)
{
    struct codec_api *ci = (struct codec_api *)data;

    return ((mpc_int32_t)(ci->read_filebuf(ptr, size)));
}

mpc_bool_t seek_impl(void *data, mpc_int32_t offset)
{  
    struct codec_api *ci = (struct codec_api *)data;

    /* WARNING: assumes we don't need to skip too far into the past,
       this might not be supported by the buffering layer yet */
    return ci->seek_buffer(offset);
}

mpc_int32_t tell_impl(void *data)
{
    struct codec_api *ci = (struct codec_api *)data;

    return ci->curpos;
}

mpc_int32_t get_size_impl(void *data)
{
    struct codec_api *ci = (struct codec_api *)data;
    
    return ci->filesize;
}

mpc_bool_t canseek_impl(void *data)
{
    (void)data;
    
    /* doesn't much matter, libmusepack ignores this anyway */
    return true;
}

MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH]
                  IBSS_ATTR_MPC_SAMPLE_BUF;

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    mpc_int64_t samplesdone;
    unsigned long frequency;
    unsigned status;
    mpc_reader reader;
    mpc_streaminfo info;
    int retval = CODEC_OK;
    
    /* musepack's sample representation is 18.14
     * DSP_SET_SAMPLE_DEPTH = 14 (FRACT) + 16 (NATIVE) - 1 (SIGN) = 29 */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 29);
    
    /* Create a decoder instance */
    reader.read = read_impl;
    reader.seek = seek_impl;
    reader.tell = tell_impl;
    reader.get_size = get_size_impl;
    reader.canseek = canseek_impl;
    reader.data = ci;

next_track:    
    if (codec_init()) {
        retval = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    samplesdone = ci->id3->offset;

    /* read file's streaminfo data */
    mpc_streaminfo_init(&info);
    if (mpc_streaminfo_read(&info, &reader) != ERROR_CODE_OK) {
        retval = CODEC_ERROR;
        goto done;
    }
    frequency = info.sample_freq / 1000;
    ci->configure(DSP_SWITCH_FREQUENCY, info.sample_freq);
        
    /* set playback engine up for correct number of channels */
    /* NOTE: current musepack format only allows for stereo files
       but code is here to handle other configurations anyway */
    if (info.channels == 2)
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
    else if (info.channels == 1)
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    else {
       retval = CODEC_ERROR;
       goto done;
    }
    
    codec_set_replaygain(ci->id3);
    /* instantiate a decoder with our file reader */
    mpc_decoder_setup(&decoder, &reader);
    if (!mpc_decoder_initialize(&decoder, &info)) {
        retval = CODEC_ERROR;
        goto done;
    }
    
    /* Resume to saved sample offset. */
    if(samplesdone > 0) {
        /* hack to improve seek time if filebuf goes empty */
        if (mpc_decoder_seek_sample(&decoder, samplesdone)) {
            ci->set_elapsed(samplesdone/frequency);
        } else {
            samplesdone = 0;
        }
        /* reset chunksize */
    }

    /* This is the decoding loop. */
    do {
       #if 1
       /* Complete seek handler. */
        if (ci->seek_time) {
            /* hack to improve seek time if filebuf goes empty */
            mpc_int64_t new_offset = (ci->seek_time - 1)*frequency;
            if (mpc_decoder_seek_sample(&decoder, new_offset)) {
                samplesdone = new_offset;
                ci->set_elapsed(ci->seek_time);
            }
            ci->seek_complete();
            /* reset chunksize */

        }
        #else
        /* Seek to start of track handler. */
        if (ci->seek_time) {
            if (ci->seek_time == 1 && mpc_decoder_seek_sample(&decoder, 0)) {
                samplesdone = 0;
                ci->set_elapsed(0);
            }
            ci->seek_complete();
        }
        #endif
        if (ci->stop_codec || ci->new_track)
            break;

        status = mpc_decoder_decode(&decoder, sample_buffer, NULL, NULL);
        ci->yield();
        if (status == 0) /* end of file reached */
            goto done;
        if (status == (unsigned)(-1)) { /* decode error */
            retval = CODEC_ERROR;
            goto done;
        } else {
            ci->pcmbuf_insert(sample_buffer,
                              sample_buffer + MPC_FRAME_LENGTH,
                              status);
            samplesdone += status;
            ci->set_elapsed(samplesdone/frequency);
            ci->set_offset(samplesdone);
        }
    } while (status != 0);
    retval = CODEC_OK;

done:
    if (ci->request_next_track())
        goto next_track;

exit:
    return retval;
}

