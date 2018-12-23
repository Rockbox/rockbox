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
static unsigned long samplesdone;
static unsigned long frequency;
static RMContext rmctx;
static RMPacket pkt;

static void init_rm(RMContext *rmctx)
{
    memcpy(rmctx, (void*)(( (intptr_t)ci->id3->id3v2buf + 3 ) &~ 3), sizeof(RMContext));
}

/* used outside liba52 */
static uint8_t buf[3840] IBSS_ATTR;
static uint8_t *bufptr = buf;
static uint8_t *bufpos = buf + 7;

static void a52_decoder_reset(void)
{
    bufptr = buf;
    bufpos = buf + 7;
}

/* The following two functions, a52_decode_data and output_audio are taken from a52.c */
static inline void output_audio(sample_t *samples)
{
    ci->yield();
    ci->pcmbuf_insert(&samples[0], &samples[256], 256);
}

static size_t a52_decode_data(uint8_t *start, uint8_t *end)
{
    /*
     * sample_rate and flags are static because this routine could
     * exit between the a52_syncinfo() and the ao_setup(), and we want
     * to have the same values when we get back !
     */
    static int sample_rate;
    static int flags;
    int bit_rate;
    int len;
    size_t consumed = 0;

    while (1) {
        len = end - start;
        if (!len)
            break;
        if (len > bufpos - bufptr)
            len = bufpos - bufptr;
        memcpy(bufptr, start, len);
        bufptr += len;
        start += len;
        consumed += len;
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
                break;
            error:
                //logf("Error decoding A52 stream\n");
                bufptr = buf;
                bufpos = buf + 7;
            }
        }   
    }
    return consumed;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
        ci->configure(DSP_SET_SAMPLE_DEPTH, 28);
    }
    else if (reason == CODEC_UNLOAD) {
        if (state)
            a52_free(state);        
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    size_t consumed = 0, n = 0;
    uint8_t *filebuf;
    int packet_offset;
    int playback_on = -1;
    size_t resume_offset;
    size_t data_offset;
    size_t packet_size;
    long action;
    intptr_t param;

    if (codec_init()) {
        return CODEC_ERROR;
    }

    action = CODEC_ACTION_NULL;
    param = ci->id3->elapsed;
    resume_offset = ci->id3->offset;

    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
    codec_set_replaygain(ci->id3);

    ci->seek_buffer(0);

    /* Initializations */
    state = a52_init(0);
    ci->memset(&rmctx,0,sizeof(RMContext)); 
    ci->memset(&pkt,0,sizeof(RMPacket));
    init_rm(&rmctx);
    data_offset = rmctx.data_offset +
      ((rmctx.flags & RM_RAW_DATASTREAM) ? 0 : DATA_HEADER_SIZE);
    packet_size = rmctx.block_align +
      ((rmctx.flags & RM_RAW_DATASTREAM) ?
       0 :
       (PACKET_HEADER_SIZE +
        ((rmctx.flags & RM_PKT_V1) ? 1 : 0)));

    samplesdone = 0;

    /* check for a mid-track resume and force a seek time accordingly */
    if (resume_offset) {
        resume_offset -= MIN(resume_offset, data_offset);
        /* put number of subpackets to skip in resume_offset */
        resume_offset /= packet_size;
        param = (int)resume_offset * ((rmctx.block_align * 8 * 1000)/rmctx.bit_rate);
    }

    if (param > 0) {
        action = CODEC_ACTION_SEEK_TIME;
    }
    else {
        /* Seek to the first packet */
        ci->set_elapsed(0);
        ci->advance_buffer(data_offset);
    }

    /* The main decoding loop */
    while ((rmctx.flags & RM_RAW_DATASTREAM) || (unsigned)rmctx.audio_pkt_cnt < rmctx.nb_packets) {
        if (action == CODEC_ACTION_NULL)
            action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            /* Do not allow seeking beyond the file's length */
            if ((unsigned) param > ci->id3->length) {
                ci->set_elapsed(ci->id3->length);
                ci->seek_complete();
                break;
            }

            if (n)
                rm_ac3_swap_bytes(filebuf, (rmctx.flags & RM_RAW_DATASTREAM) ? n : rmctx.block_align);
            packet_offset = param / ((rmctx.block_align*8*1000)/rmctx.bit_rate);
            ci->seek_buffer(data_offset + packet_offset * packet_size);
            rmctx.audio_pkt_cnt = packet_offset;
            samplesdone = packet_offset * A52_SAMPLESPERFRAME;
            ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));
            ci->seek_complete();
            a52_decoder_reset();
            consumed = 0;
            n = 0;
        }

        action = CODEC_ACTION_NULL;

        if (rmctx.flags & RM_RAW_DATASTREAM) {
            if (n > consumed) {
                consumed += a52_decode_data(filebuf + consumed, filebuf + n);
                ci->set_offset(ci->curpos + consumed);
            }
            else {
                if (n) {
                    rm_ac3_swap_bytes(filebuf, n);
                    ci->advance_buffer(n);
                }
                filebuf = ci->request_buffer(&n, BUFFER_SIZE);
                if (n == 0)
                    break;
                rm_ac3_swap_bytes(filebuf, n);
                consumed = 0;
            }
        }
        else {
            filebuf = ci->request_buffer(&n, packet_size);

            if (n == 0)
                break;

            if (rm_get_packet(&filebuf, &rmctx, &pkt) < 0 && playback_on != 0) {
                if(playback_on == -1) {
                    /* Error only if packet-parsing failed and playback hadn't started */
                    DEBUGF("rm_get_packet failed\n");
                    return CODEC_ERROR;
                }
                else {
                    break;
                }
            }

            playback_on = 1;
            consumed = 0;
            while (consumed < rmctx.block_align)
                consumed += a52_decode_data(filebuf + consumed, filebuf + rmctx.block_align);
            rm_ac3_swap_bytes(filebuf, rmctx.block_align);
            ci->advance_buffer(pkt.length);
        }
    }

    return CODEC_OK;
}
