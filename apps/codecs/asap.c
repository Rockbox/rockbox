/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: asap.c 17847 2008-06-28 18:10:04Z domonoky $
 *
 * Copyright (C) 2008 Dominik Wenger
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
#include "asap/asap.h"

CODEC_HEADER

#define CHUNK_SIZE (1024*8)

static byte samples[CHUNK_SIZE];   /* The sample buffer */
static ASAP_State asap;         /* asap codec state */

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    int n_bytes;
    int song;
    int duration;
    char* module;

    /* Generic codec initialisation */
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);
    
next_track:
    if (codec_init()) {
        DEBUGF("codec init failed\n");
        return CODEC_ERROR;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);
        
    int bytes_done =0;   
    int filesize;
    ci->seek_buffer(0);
    module = ci->request_buffer(&filesize, ci->filesize);
    if (!module || (size_t)filesize < (size_t)ci->filesize) 
    {
        DEBUGF("loading error\n");
        return CODEC_ERROR;
    }

    /*Init ASAP */
    if (!ASAP_Load(&asap, ci->id3->path, module, filesize))
    {
        DEBUGF("%s: format not supported",ci->id3->path);
        return CODEC_ERROR;
    }  
    
      /* Make use of 44.1khz */
    ci->configure(DSP_SET_FREQUENCY, 44100);
    /* Sample depth is 16 bit little endian */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
    /* Stereo or Mono output ? */
    if(asap.module_info.channels ==1)
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    else
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
        
    /* reset eleapsed */
    ci->set_elapsed(0);

    song = asap.module_info.default_song;
    duration = asap.module_info.durations[song];
    if (duration < 0)
        duration = 180 * 1000;
    
    ASAP_PlaySong(&asap, song, duration);
    ASAP_MutePokeyChannels(&asap, 0);
    
    /* The main decoder loop */    
    while (1) {
        ci->yield();
        if (ci->stop_codec || ci->new_track)
            break;

        if (ci->seek_time) {
            /* New time is ready in ci->seek_time */
                      
            /* seek to pos */
            ASAP_Seek(&asap,ci->seek_time);
            /* update elapsed */
            ci->set_elapsed(ci->seek_time);
            /* update bytes_done */
            bytes_done = ci->seek_time*44.1*2;    
            /* seek ready */    
            ci->seek_complete();            
        }
        
        /* Generate a buffer full of Audio */
        #ifdef ROCKBOX_LITTLE_ENDIAN
        n_bytes = ASAP_Generate(&asap, samples, sizeof(samples), ASAP_FORMAT_S16_LE);
        #else
        n_bytes = ASAP_Generate(&asap, samples, sizeof(samples), ASAP_FORMAT_S16_BE);
        #endif
        
        ci->pcmbuf_insert(samples, NULL, n_bytes /2);
        
        bytes_done += n_bytes;
        ci->set_elapsed((bytes_done / 2) / 44.1);
        
        if(n_bytes != sizeof(samples))
            break;
    }

    if (ci->request_next_track())
        goto next_track;
 
    return CODEC_OK;    
}
