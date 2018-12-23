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

#include "codeclib.h"
#include "librm/rm.h"
#include "libfaad/common.h"
#include "libfaad/structs.h"
#include "libfaad/decoder.h"
/* rockbox: not used
#include "libfaad/output.h"
*/

CODEC_HEADER

static void init_rm(RMContext *rmctx)
{
    memcpy(rmctx, (void*)(( (intptr_t)ci->id3->id3v2buf + 3 ) &~ 3), sizeof(RMContext));
}

static RMContext rmctx;
static RMPacket pkt;

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
    static NeAACDecFrameInfo frame_info;
    NeAACDecHandle decoder;
    size_t n;
    unsigned int i;
    unsigned char* buffer;
    int err, consumed, pkt_offset, skipped = 0;
    uint32_t s = 0; /* sample rate */
    unsigned char c = 0; /* channels */
    int playback_on = -1;
    size_t resume_offset;
    long action;
    intptr_t param;

    if (codec_init()) {
        DEBUGF("FAAD: Codec init error\n");
        return CODEC_ERROR;
    }

    action = CODEC_ACTION_NULL;
    param = ci->id3->elapsed;
    resume_offset = ci->id3->offset;

    ci->memset(&rmctx,0,sizeof(RMContext));
    ci->memset(&pkt,0,sizeof(RMPacket));

    ci->seek_buffer(0);

    init_rm(&rmctx);    
    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);
   
    /* initialise the sound converter */
    decoder = NeAACDecOpen();
    
    if (!decoder) {
        DEBUGF("FAAD: Decode open error\n");
        return CODEC_ERROR;
    }

    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(decoder);
    conf->outputFormat = FAAD_FMT_16BIT; /* irrelevant, we don't convert */
    NeAACDecSetConfiguration(decoder, conf);    
    
    decoder->config.defObjectType = rmctx.codec_extradata[0];
    decoder->config.defSampleRate = rmctx.sample_rate;      
    err = NeAACDecInit(decoder, NULL, 0, &s, &c);    
   
    if (err) {
        DEBUGF("FAAD: DecInit: %d, %d\n", err, decoder->object_type);
        return CODEC_ERROR;
    }
    
    /* check for a mid-track resume and force a seek time accordingly */
    if (resume_offset) {
        resume_offset -= MIN(resume_offset, rmctx.data_offset + DATA_HEADER_SIZE);
        /* put number of subpackets to skip in resume_offset */
        resume_offset /= (rmctx.block_align + PACKET_HEADER_SIZE +
                          ((rmctx.flags & RM_PKT_V1) ? 1 : 0));
        param = (int)resume_offset * ((rmctx.block_align * 8 * 1000)/rmctx.bit_rate);
    }

    if (param > 0) {
        action = CODEC_ACTION_SEEK_TIME;
    }
    else {
        /* Seek to the first packet */
        ci->set_elapsed(0);
        ci->advance_buffer(rmctx.data_offset + DATA_HEADER_SIZE);
    }

    /* The main decoding loop */
    while (1) {
        if (action == CODEC_ACTION_NULL)
            action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            /* Do not allow seeking beyond the file's length */
            if ((unsigned long)param > ci->id3->length) {
                ci->set_elapsed(ci->id3->length);
                ci->seek_complete();
                break;
            }       

            ci->seek_buffer(rmctx.data_offset + DATA_HEADER_SIZE);            

            /* Seek to the start of the track */
            if (param == 0) {
                ci->set_elapsed(0);
                ci->seek_complete();
                action = CODEC_ACTION_NULL;
                continue;          
            }
            
            skipped = 0;                                                                                       
            while(1) {                
                buffer = ci->request_buffer(&n,rmctx.audio_framesize + 1000);               
                pkt_offset = skipped - pkt.length;
                consumed = rm_get_packet(&buffer, &rmctx, &pkt);
                if(consumed < 0 && playback_on != 0) {
                    if(playback_on == -1) {
                    /* Error only if packet-parsing failed and playback hadn't started */
                        DEBUGF("rm_get_packet failed\n");
                        ci->seek_complete();
                        return CODEC_ERROR;
                    }
                    else {
                        ci->seek_complete();
                        return CODEC_OK;
                    }
                }
                skipped += pkt.length;

                if(pkt.timestamp > (unsigned)param)
                    break;
                
                ci->advance_buffer(pkt.length);
            }           

            ci->seek_buffer(pkt_offset + rmctx.data_offset + DATA_HEADER_SIZE);
            buffer = ci->request_buffer(&n,rmctx.audio_framesize + 1000);
            NeAACDecPostSeekReset(decoder, decoder->frame);
            ci->set_elapsed(pkt.timestamp);
            ci->seek_complete();            
        }

        action = CODEC_ACTION_NULL;

        /* Request the required number of bytes from the input buffer */ 
        buffer=ci->request_buffer(&n,rmctx.audio_framesize + 1000);        
        consumed = rm_get_packet(&buffer, &rmctx, &pkt);

        if(consumed < 0 && playback_on != 0) {
            if(playback_on == -1) {
            /* Error only if packet-parsing failed and playback hadn't started */
                DEBUGF("rm_get_packet failed\n");
                return CODEC_ERROR;
            }
            else
                break;
        }
        
        playback_on = 1;
        if (pkt.timestamp >= ci->id3->length)
            break;

        /* Decode one block - returned samples will be host-endian */                           
        for(i = 0; i < rmctx.sub_packet_cnt; i++) {
            NeAACDecDecode(decoder, &frame_info, buffer, rmctx.sub_packet_lengths[i]);
            buffer += rmctx.sub_packet_lengths[i];                      
            if (frame_info.error > 0) {
                DEBUGF("FAAD: decode error '%s'\n", NeAACDecGetErrorMessage(frame_info.error));
                return CODEC_ERROR;
            }
            ci->pcmbuf_insert(decoder->time_out[0],
                              decoder->time_out[1],
                              decoder->frameLength);
            ci->set_elapsed(pkt.timestamp);            
        }                                       
        
        ci->advance_buffer(pkt.length);              
    }

    return CODEC_OK;
}
