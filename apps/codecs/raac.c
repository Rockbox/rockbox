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
#include "libfaad/output.h"

CODEC_HEADER

/* Global buffers to be used in the mdct synthesis. This way the arrays can
 * be moved to IRAM for some targets */
#define GB_BUF_SIZE 1024
static ALIGN real_t gb_time_buffer[2][GB_BUF_SIZE] IBSS_ATTR_FAAD_LARGE_IRAM;
static ALIGN real_t gb_fb_intermed[2][GB_BUF_SIZE] IBSS_ATTR_FAAD_LARGE_IRAM;


static void init_rm(RMContext *rmctx)
{
    memcpy(rmctx, (void*)(( (intptr_t)ci->id3->id3v2buf + 3 ) &~ 3), sizeof(RMContext));
}

static RMContext rmctx;
static RMPacket pkt;
/* this is the codec entry point */
enum codec_status codec_main(void)
{
    static NeAACDecFrameInfo frame_info;
    NeAACDecHandle decoder;
    size_t n;
    void *ret;
    int needed_bufsize;
    unsigned int i;
    unsigned char* buffer;
    int err, consumed, pkt_offset, skipped = 0;
    uint32_t s = 0; /* sample rate */
    unsigned char c = 0; /* channels */
    int playback_on = -1;
    size_t resume_offset;

    /* Generic codec initialisation */
    ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, 29);

next_track:
    err = CODEC_OK;

    if (codec_init()) {
        DEBUGF("FAAD: Codec init error\n");
        return CODEC_ERROR;
    }

    if (codec_wait_taginfo() != 0)
        goto done;

    resume_offset = ci->id3->offset;

    ci->memset(&rmctx,0,sizeof(RMContext));
    ci->memset(&pkt,0,sizeof(RMPacket));
    init_rm(&rmctx);    
    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);
   
    /* initialise the sound converter */
    decoder = NeAACDecOpen();
    
    if (!decoder) {
        DEBUGF("FAAD: Decode open error\n");
        err = CODEC_ERROR;
        goto done;
    }   
    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(decoder);
    conf->outputFormat = FAAD_FMT_16BIT; /* irrelevant, we don't convert */
    NeAACDecSetConfiguration(decoder, conf);    
    
    decoder->config.defObjectType = rmctx.codec_extradata[0];
    decoder->config.defSampleRate = rmctx.sample_rate;      
    err = NeAACDecInit(decoder, NULL, 0, &s, &c);    
   
    if (err) {
        DEBUGF("FAAD: DecInit: %d, %d\n", err, decoder->object_type);
        err = CODEC_ERROR;
        goto done;
    }
    
    /* Set pointer to be able to use IRAM an to avoid alloc in decoder. Must
     * be called after NeAACDecOpen(). */
    /* A buffer of framelength or 2*frameLenght size must be allocated for
     * time_out. If frameLength is too big or SBR/forceUpSampling is active, 
     * we do not use the IRAM buffer and keep faad's internal allocation (see 
     * specrec.c). */
    needed_bufsize = decoder->frameLength;
#ifdef SBR_DEC
    if ((decoder->sbr_present_flag == 1) || (decoder->forceUpSampling == 1))
    {
        needed_bufsize *= 2;
    }
#endif
    if (needed_bufsize <= GB_BUF_SIZE)
    {
        decoder->time_out[0]    = &gb_time_buffer[0][0];
        decoder->time_out[1]    = &gb_time_buffer[1][0];
    }
    /* A buffer of with frameLength elements must be allocated for fb_intermed. 
     * If frameLength is too big, we do not use the IRAM buffer and keep faad's 
     * internal allocation (see specrec.c). */
    needed_bufsize = decoder->frameLength;
    if (needed_bufsize <= GB_BUF_SIZE)
    {
        decoder->fb_intermed[0] = &gb_fb_intermed[0][0];
        decoder->fb_intermed[1] = &gb_fb_intermed[1][0];
    }
    
    /* check for a mid-track resume and force a seek time accordingly */
    if(resume_offset > rmctx.data_offset + DATA_HEADER_SIZE) {
        resume_offset -= rmctx.data_offset + DATA_HEADER_SIZE;
        /* put number of subpackets to skip in resume_offset */
        resume_offset /= (rmctx.block_align + PACKET_HEADER_SIZE);
        ci->seek_time =  (int)resume_offset * ((rmctx.block_align * 8 * 1000)/rmctx.bit_rate);                
    }
    
    ci->id3->frequency = s;    
    ci->set_elapsed(0);
    ci->advance_buffer(rmctx.data_offset + DATA_HEADER_SIZE);

    /* The main decoding loop */
seek_start:
    while (1) {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }

        if (ci->seek_time) {            
            
            /* Do not allow seeking beyond the file's length */
            if ((unsigned) ci->seek_time > ci->id3->length) {
                ci->seek_complete();
                goto done;
            }       

            ci->seek_buffer(rmctx.data_offset + DATA_HEADER_SIZE);            

            /* Seek to the start of the track */
            if (ci->seek_time == 1) {
                ci->set_elapsed(0);
                ci->seek_complete();
                goto seek_start;           
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
                        return CODEC_ERROR;
                    }
                    else
                        goto done;
                }
                skipped += pkt.length;
                if(pkt.timestamp > (unsigned)ci->seek_time) break;                
                ci->advance_buffer(pkt.length);
            }           
            ci->seek_buffer(pkt_offset + rmctx.data_offset + DATA_HEADER_SIZE);
            buffer = ci->request_buffer(&n,rmctx.audio_framesize + 1000);
            NeAACDecPostSeekReset(decoder, decoder->frame);
            ci->seek_complete();            
        }

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
                goto done;
        }
        
        playback_on = 1;
        if (pkt.timestamp >= ci->id3->length)
            goto done;
        /* Decode one block - returned samples will be host-endian */                           
        for(i = 0; i < rmctx.sub_packet_cnt; i++) {
            ret = NeAACDecDecode(decoder, &frame_info, buffer, rmctx.sub_packet_lengths[i]);
            buffer += rmctx.sub_packet_lengths[i];                      
            if (frame_info.error > 0) {
                DEBUGF("FAAD: decode error '%s'\n", NeAACDecGetErrorMessage(frame_info.error));
                err = CODEC_ERROR;
                goto exit;
            }
            ci->pcmbuf_insert(decoder->time_out[0],
                              decoder->time_out[1],
                              decoder->frameLength);
            ci->set_elapsed(pkt.timestamp);            
        }                                       
        
        ci->advance_buffer(pkt.length);              
    }

done:
    if (ci->request_next_track())
        goto next_track;

exit:
    return err;
}
 
