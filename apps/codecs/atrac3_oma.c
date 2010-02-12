/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Mohamed Tarek
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

#include <string.h>

#include "logf.h"
#include "codeclib.h"
#include "inttypes.h"
#include "libatrac/atrac3.h"

CODEC_HEADER

#define FRAMESIZE ci->id3->bytesperframe
#define BITRATE   ci->id3->bitrate

/* The codec has nothing to do with RM, it just uses an RMContext struct to *
 * store the data needs to be passed to the decoder initializing function.  */
RMContext rmctx;
ATRAC3Context q IBSS_ATTR;

static void init_rm(RMContext *rmctx, struct mp3entry *id3)
{
    rmctx->sample_rate = id3->frequency;
    rmctx->nb_channels = 2;
    rmctx->bit_rate    = id3->bitrate;
    rmctx->block_align = id3->bytesperframe;

    /* 14-byte extra-data was faked in the metadata parser so that  *
     * the ATRAC3 decoder would parse it as WAV format extra-data.  */
    rmctx->extradata_size  = 14;
    memcpy(rmctx->codec_extradata, &id3->id3v1buf[0][0], 14);
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{ 
    static size_t buff_size;
    int datasize, res, frame_counter, total_frames, seek_frame_offset;
    uint8_t *bit_buffer;
    int elapsed = 0;
    size_t resume_offset = ci->id3->offset;

next_track:
    if (codec_init()) {
        DEBUGF("codec init failed\n");
        return CODEC_ERROR;
    }
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);
    ci->memset(&rmctx,0,sizeof(RMContext));
    ci->memset(&q,0,sizeof(ATRAC3Context));

    init_rm(&rmctx, ci->id3);
 
    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    ci->configure(DSP_SET_SAMPLE_DEPTH, 17); /* Remark: atrac3 uses s15.0 by default, s15.2 was hacked. */
    ci->configure(DSP_SET_STEREO_MODE, rmctx.nb_channels == 1 ?
        STEREO_MONO : STEREO_NONINTERLEAVED);
    
    res =atrac3_decode_init(&q, &rmctx);
    if(res < 0) {
        DEBUGF("failed to initialize atrac decoder\n");
        return CODEC_ERROR;
    }

    /* check for a mid-track resume and force a seek time accordingly */
    if(resume_offset > ci->id3->first_frame_offset) {
        resume_offset -= ci->id3->first_frame_offset;
        /* calculate resume_offset in frames */
        resume_offset = (int)resume_offset / FRAMESIZE;
        ci->seek_time =  (int)resume_offset * ((FRAMESIZE * 8)/BITRATE);                
    }
    total_frames = (ci->id3->filesize - ci->id3->first_frame_offset) / FRAMESIZE;
    frame_counter = 0;
    
    ci->set_elapsed(0);
    ci->advance_buffer(ci->id3->first_frame_offset);

    /* The main decoder loop */  
seek_start :         
    while(frame_counter < total_frames)
    {  
        bit_buffer = (uint8_t *) ci->request_buffer(&buff_size, FRAMESIZE);

        ci->yield();
        if (ci->stop_codec || ci->new_track)
            goto done;

            if (ci->seek_time) {
                ci->set_elapsed(ci->seek_time);

                /* Do not allow seeking beyond the file's length */
                if ((unsigned) ci->seek_time > ci->id3->length) {
                    ci->seek_complete();
                    goto done;
                }       

                /* Seek to the start of the track */
                if (ci->seek_time == 1) {
                    ci->set_elapsed(0);
                    ci->seek_complete();
                    ci->seek_buffer(ci->id3->first_frame_offset);
                    elapsed = 0;
                    goto seek_start;           
                }                                                                
                seek_frame_offset = (ci->seek_time * BITRATE) / (8 * FRAMESIZE);
                frame_counter = seek_frame_offset;
                ci->seek_buffer(ci->id3->first_frame_offset + seek_frame_offset* FRAMESIZE);
                bit_buffer = (uint8_t *) ci->request_buffer(&buff_size, FRAMESIZE);
                elapsed = ci->seek_time;

                ci->set_elapsed(elapsed);
                ci->seek_complete(); 
            }

            res = atrac3_decode_frame(&rmctx, &q, &datasize, bit_buffer, FRAMESIZE);

            if(res != (int)FRAMESIZE) {
                DEBUGF("codec error\n");
                return CODEC_ERROR;
            }

            if(datasize)
                ci->pcmbuf_insert(q.outSamples, q.outSamples + 1024, q.samples_per_frame / rmctx.nb_channels);

            elapsed += (FRAMESIZE * 8) / BITRATE;
            ci->set_elapsed(elapsed);
        
            ci->advance_buffer(FRAMESIZE);
            frame_counter++;
    }

    done:
    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;    
}
