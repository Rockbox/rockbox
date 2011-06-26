/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "libasap/asap.h"

CODEC_HEADER

#define CHUNK_SIZE (1024*2)

static byte samples[CHUNK_SIZE] IBSS_ATTR;   /* The sample buffer */
static ASAP_State asap IBSS_ATTR;         /* asap codec state */

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    /* Nothing to do */
    return CODEC_OK;
    (void)reason;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    int n_bytes;
    int song;
    int duration;
    char* module;
    int bytesPerSample =2;
    intptr_t param;
    
    if (codec_init()) {
        DEBUGF("codec init failed\n");
        return CODEC_ERROR;
    }

    codec_set_replaygain(ci->id3);
        
    int bytes_done =0;   
    size_t filesize;
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
    if(asap.module_info->channels ==1)
    {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
        bytesPerSample = 2;
    }
    else
    {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
        bytesPerSample = 4; 
    }    
    /* reset eleapsed */
    ci->set_elapsed(0);

    song = asap.module_info->default_song;
    duration = asap.module_info->durations[song];
    if (duration < 0)
        duration = 180 * 1000;
    
    /* set id3 length, because metadata parse might not have done it */
    ci->id3->length = duration;
    
    ASAP_PlaySong(&asap, song, duration);
    ASAP_MutePokeyChannels(&asap, 0);
    
    /* The main decoder loop */    
    while (1) {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            /* New time is ready in param */

            /* seek to pos */
            ASAP_Seek(&asap,param);
            /* update bytes_done */
            bytes_done = param*44.1*2;    
            /* update elapsed */
            ci->set_elapsed((bytes_done / 2) / 44.1);
            /* seek ready */    
            ci->seek_complete();            
        }
        
        /* Generate a buffer full of Audio */
        #ifdef ROCKBOX_LITTLE_ENDIAN
        n_bytes = ASAP_Generate(&asap, samples, sizeof(samples), ASAP_FORMAT_S16_LE);
        #else
        n_bytes = ASAP_Generate(&asap, samples, sizeof(samples), ASAP_FORMAT_S16_BE);
        #endif
        
        ci->pcmbuf_insert(samples, NULL, n_bytes /bytesPerSample);
        
        bytes_done += n_bytes;
        ci->set_elapsed((bytes_done / 2) / 44.1);
        
        if(n_bytes != sizeof(samples))
            break;
    }
 
    return CODEC_OK;    
}
