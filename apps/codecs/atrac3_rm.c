/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

RMContext rmctx;
RMPacket pkt;
ATRAC3Context q IBSS_ATTR;

static void init_rm(RMContext *rmctx)
{
    memcpy(rmctx, (void*)(( (intptr_t)ci->id3->id3v2buf + 3 ) &~ 3), sizeof(RMContext));
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{ 
    static size_t buff_size;
    int datasize, res, consumed, i, time_offset;
    uint8_t *bit_buffer;
    uint16_t fs,sps,h;
    uint32_t packet_count;
    int scrambling_unit_size, num_units, elapsed = 0;
    int playback_on = -1;
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
    ci->memset(&pkt,0,sizeof(RMPacket));
    ci->memset(&q,0,sizeof(ATRAC3Context));

    init_rm(&rmctx);
 
    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    ci->configure(DSP_SET_SAMPLE_DEPTH, 17); /* Remark: atrac3 uses s15.0 by default, s15.2 was hacked. */
    ci->configure(DSP_SET_STEREO_MODE, rmctx.nb_channels == 1 ?
        STEREO_MONO : STEREO_NONINTERLEAVED);

    packet_count = rmctx.nb_packets;
    rmctx.audio_framesize = rmctx.block_align;
    rmctx.block_align = rmctx.sub_packet_size;
    fs = rmctx.audio_framesize;
    sps= rmctx.block_align;
    h = rmctx.sub_packet_h;
    scrambling_unit_size = h*fs;
    
    res =atrac3_decode_init(&q, &rmctx);
    if(res < 0) {
        DEBUGF("failed to initialize atrac decoder\n");
        return CODEC_ERROR;
    }

    /* check for a mid-track resume and force a seek time accordingly */
    if(resume_offset > rmctx.data_offset + DATA_HEADER_SIZE) {
        resume_offset -= rmctx.data_offset + DATA_HEADER_SIZE;
        num_units = (int)resume_offset / scrambling_unit_size;        
        /* put number of subpackets to skip in resume_offset */
        resume_offset /= (sps + PACKET_HEADER_SIZE);
        ci->seek_time =  (int)resume_offset * ((sps * 8 * 1000)/rmctx.bit_rate);                
    }
    
    ci->set_elapsed(0);
    ci->advance_buffer(rmctx.data_offset + DATA_HEADER_SIZE);

    /* The main decoder loop */  
seek_start :         
    while((unsigned)elapsed < rmctx.duration)
    {  
        bit_buffer = (uint8_t *) ci->request_buffer(&buff_size, scrambling_unit_size);
        consumed = rm_get_packet(&bit_buffer, &rmctx, &pkt);
        if(consumed < 0 && playback_on != 0) {
            if(playback_on == -1) {
            /* Error only if packet-parsing failed and playback hadn't started */
                DEBUGF("rm_get_packet failed\n");
                return CODEC_ERROR;
            }
            else
                goto done;
        }

        for(i = 0; i < rmctx.audio_pkt_cnt*(fs/sps) ; i++)
        { 
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

                ci->seek_buffer(rmctx.data_offset + DATA_HEADER_SIZE);
                packet_count = rmctx.nb_packets;
                rmctx.audio_pkt_cnt = 0;
                rmctx.frame_number = 0;

                /* Seek to the start of the track */
                if (ci->seek_time == 1) {
                    ci->set_elapsed(0);
                    ci->seek_complete();
                    goto seek_start;           
                }                                                                
                num_units = ((ci->seek_time)/(sps*1000*8/rmctx.bit_rate))/(h*(fs/sps));                    
                ci->seek_buffer(rmctx.data_offset + DATA_HEADER_SIZE + consumed * num_units);
                bit_buffer = (uint8_t *) ci->request_buffer(&buff_size, scrambling_unit_size);
                consumed = rm_get_packet(&bit_buffer, &rmctx, &pkt);
                if(consumed < 0 && playback_on != 0) {
                    if(playback_on == -1) {
                    /* Error only if packet-parsing failed and playback hadn't started */
                        DEBUGF("rm_get_packet failed\n");
                        return CODEC_ERROR;
                    }
                    else
                        goto done;
                }

                packet_count = rmctx.nb_packets - rmctx.audio_pkt_cnt * num_units;
                rmctx.frame_number = ((ci->seek_time)/(sps*1000*8/rmctx.bit_rate)); 
                while(rmctx.audiotimestamp > (unsigned) ci->seek_time) {
                    rmctx.audio_pkt_cnt = 0;
                    ci->seek_buffer(rmctx.data_offset + DATA_HEADER_SIZE + consumed * (num_units-1));
                    bit_buffer = (uint8_t *) ci->request_buffer(&buff_size, scrambling_unit_size); 
                    consumed = rm_get_packet(&bit_buffer, &rmctx, &pkt);                                                                             
                    packet_count += rmctx.audio_pkt_cnt;
                    num_units--;
                }
                time_offset = ci->seek_time - rmctx.audiotimestamp;
                i = (time_offset/((sps * 8 * 1000)/rmctx.bit_rate));
                elapsed = rmctx.audiotimestamp+(1000*8*sps/rmctx.bit_rate)*i;
                ci->set_elapsed(elapsed);
                ci->seek_complete(); 
            }
            if(pkt.length)    
                res = atrac3_decode_frame(&rmctx, &q, &datasize, pkt.frames[i], rmctx.block_align);
            else /* indicates that there are no remaining frames */
                goto done;

            if(res != rmctx.block_align) {
                DEBUGF("codec error\n");
                return CODEC_ERROR;
            }

            if(datasize)
                ci->pcmbuf_insert(q.outSamples, q.outSamples + 1024, q.samples_per_frame / rmctx.nb_channels);
            playback_on = 1;
            elapsed = rmctx.audiotimestamp+(1000*8*sps/rmctx.bit_rate)*i;
            ci->set_elapsed(elapsed);
            rmctx.frame_number++;
        }
        packet_count -= rmctx.audio_pkt_cnt;
        rmctx.audio_pkt_cnt = 0;
        ci->advance_buffer(consumed);
    }

    done :
    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;    
}
