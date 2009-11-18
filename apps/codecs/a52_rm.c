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
#include <codecs/librm/rm.h>
#include <inttypes.h>  /* Needed by a52.h */
#include <codecs/liba52/config-a52.h>
#include <codecs/liba52/a52.h>

CODEC_HEADER

#define BUFFER_SIZE 4096

#define A52_SAMPLESPERFRAME (6*256)

static a52_state_t *state;
unsigned long samplesdone;
unsigned long frequency;
RMContext rmctx;
RMPacket pkt;

static void init_rm(RMContext *rmctx)
{
    memcpy(rmctx, (void*)(( (intptr_t)ci->id3->id3v2buf + 3 ) &~ 3), sizeof(RMContext));
}

/* used outside liba52 */
static uint8_t buf[3840] IBSS_ATTR;

/* The following two functions, a52_decode_data and output_audio are taken from apps/codecs/a52.c */
static inline void output_audio(sample_t *samples)
{
    ci->yield();
    ci->pcmbuf_insert(&samples[0], &samples[256], 256);
}

static void a52_decode_data(uint8_t *start, uint8_t *end)
{
    static uint8_t *bufptr = buf;
    static uint8_t *bufpos = buf + 7;
    /*
     * sample_rate and flags are static because this routine could
     * exit between the a52_syncinfo() and the ao_setup(), and we want
     * to have the same values when we get back !
     */
    static int sample_rate;
    static int flags;
    int bit_rate;
    int len;

    while (1) {
        len = end - start;
        if (!len)
            break;
        if (len > bufpos - bufptr)
            len = bufpos - bufptr;
        memcpy(bufptr, start, len);
        bufptr += len;
        start += len;
        if (bufptr == bufpos) {
            if (bufpos == buf + 7) {
                int length;

                length = a52_syncinfo(buf, &flags, &sample_rate, &bit_rate);
                if (!length) {
                    //DEBUGF("skip\n");
                    for (bufptr = buf; bufptr < buf + 6; bufptr++)
                        bufptr[0] = bufptr[1];
                    continue;
                }
                bufpos = buf + length;
            } else {
                /* Unity gain is 1 << 26, and we want to end up on 28 bits
                   of precision instead of the default 30.
                 */
                level_t level = 1 << 24;
                sample_t bias = 0;
                int i;

                /* This is the configuration for the downmixing: */
                flags = A52_STEREO | A52_ADJUST_LEVEL;

                if (a52_frame(state, buf, &flags, &level, bias))
                    goto error;
                a52_dynrng(state, NULL, NULL);
                frequency = sample_rate;

                /* An A52 frame consists of 6 blocks of 256 samples
                   So we decode and output them one block at a time */
                for (i = 0; i < 6; i++) {
                    if (a52_block(state))
                        goto error;
                    output_audio(a52_samples(state));
                    samplesdone += 256;
                }
                ci->set_elapsed(samplesdone/(frequency/1000));
                bufptr = buf;
                bufpos = buf + 7;
                continue;
            error:
                //logf("Error decoding A52 stream\n");
                bufptr = buf;
                bufpos = buf + 7;
            }
        }   
    }
}


/* this is the codec entry point */
enum codec_status codec_main(void)
{
    size_t n;
    uint8_t *filebuf;
    int retval, consumed, packet_offset;
    int playback_on = -1;
    size_t resume_offset = ci->id3->offset;

    /* Generic codec initialisation */
    ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, 28);

next_track:
    if (codec_init()) {
        retval = CODEC_ERROR;
        goto exit;
    }

    while (!ci->taginfo_ready)
        ci->yield();

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);

    /* Intializations */
    state = a52_init(0);
    ci->memset(&rmctx,0,sizeof(RMContext)); 
    ci->memset(&pkt,0,sizeof(RMPacket));
    init_rm(&rmctx);

    /* check for a mid-track resume and force a seek time accordingly */
    if(resume_offset > rmctx.data_offset + DATA_HEADER_SIZE) {
        resume_offset -= rmctx.data_offset + DATA_HEADER_SIZE;
        /* put number of subpackets to skip in resume_offset */
        resume_offset /= (rmctx.block_align + PACKET_HEADER_SIZE);
        ci->seek_time =  (int)resume_offset * ((rmctx.block_align * 8 * 1000)/rmctx.bit_rate);                
    }
    
    /* Seek to the first packet */
    ci->advance_buffer(rmctx.data_offset + DATA_HEADER_SIZE );

    /* The main decoding loop */
    while((unsigned)rmctx.audio_pkt_cnt < rmctx.nb_packets) {
        ci->yield();
        if (ci->stop_codec || ci->new_track)
            break;

        if (ci->seek_time) {
            packet_offset = ci->seek_time / ((rmctx.block_align*8*1000)/rmctx.bit_rate);
            ci->seek_buffer(rmctx.data_offset + DATA_HEADER_SIZE + packet_offset*(rmctx.block_align + PACKET_HEADER_SIZE));
            rmctx.audio_pkt_cnt = packet_offset;
            samplesdone = (rmctx.sample_rate/1000 * ci->seek_time);
            ci->seek_complete();
        }

        filebuf = ci->request_buffer(&n, rmctx.block_align + PACKET_HEADER_SIZE);
        consumed = rm_get_packet(&filebuf, &rmctx, &pkt);

        if(consumed < 0 && playback_on != 0) {
            if(playback_on == -1) {
            /* Error only if packet-parsing failed and playback hadn't started */
                DEBUGF("rm_get_packet failed\n");
                return CODEC_ERROR;
            }
            else {
                retval = CODEC_OK;
                goto exit;
            }
        }

        playback_on = 1;
        a52_decode_data(filebuf, filebuf + rmctx.block_align);
        ci->advance_buffer(pkt.length);
    }

    retval = CODEC_OK;

    if (ci->request_next_track())
        goto next_track;

exit:
    a52_free(state);
    return retval;
}
