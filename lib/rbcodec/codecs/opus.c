/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Frederik M.J. Vestre
 * Based on speex.c codec interface:
 * Copyright (C) 2006 Frederik M.J. Vestre
 * Based on vorbis.c codec interface:
 * Copyright (C) 2002 Björn Stenberg
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
#include "inttypes.h"
#include "libopus/opus.h"
#include "libopus/opus_header.h"


#include "libopus/ogg/ogg.h"
#ifdef SIMULATOR
#include <tlsf.h>
#endif

CODEC_HEADER

#if defined(CPU_ARM) || defined(CPU_COLDFIRE) || defined(CPU_MIPS)
#include <setjmp.h>
jmp_buf rb_jump_buf;
#endif

/* Room for 120 ms of stereo audio at 48 kHz */
#define MAX_FRAME_SIZE  (2*120*48)
#define CHUNKSIZE       (16*1024)

//static uint16_t output[MAX_FRAME_SIZE] IBSS_ATTR;
/* Some standard functions and variables needed by Tremor */

static int get_more_data(ogg_sync_state *oy)
{
    int bytes;
    char *buffer;

    buffer = (char *)ogg_sync_buffer(oy, CHUNKSIZE);

    bytes = ci->read_filebuf(buffer, CHUNKSIZE);

    ogg_sync_wrote(oy,bytes);

    return bytes;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        if (codec_init())
            return CODEC_ERROR;
        ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
    }

    return CODEC_OK;
}



/* this is called for each file to process */
enum codec_status codec_run(void)
{
    int error = CODEC_ERROR;
    intptr_t param;

    int eof = 0;
    int eos = 0;
    ogg_sync_state oy;
    ogg_page og;
    ogg_packet op;
    ogg_stream_state os;
    int64_t page_granule = 0;
    int stream_init = 0;
    int sample_rate = 0;

    if (codec_init()) {
        goto done;
    }

    /* pre-init the ogg_sync_state buffer, so it won't need many reallocs */
    ogg_sync_init(&oy);
    oy.storage = 64*1024;
    oy.data = codec_malloc(oy.storage);

    //char state_mem[opus_decoder_get_size(2)];
    OpusHeader header;
    OpusDecoder *st = (OpusDecoder*) codec_malloc(opus_decoder_get_size(2)); //(OpusDecoder*)&state_mem;
    uint16_t *output = (uint16_t*) codec_malloc(MAX_FRAME_SIZE*sizeof(uint16_t));

#if defined(CPU_ARM) || defined(CPU_COLDFIRE) || defined(CPU_MIPS)
    if (setjmp(rb_jump_buf) != 0) {
        /* malloc failed; finish with this track */
        goto done;
    }
#endif

    ci->seek_buffer(0);

    if (ci->id3->offset) {
//        ci->seek_buffer(ci->id3->offset);
//        ov_raw_seek(&vf, ci->id3->offset);
//        ci->set_elapsed(ov_time_tell(&vf));
//        ci->set_offset(ov_raw_tell(&vf));
    }
    else {
        ci->set_elapsed(0);
    }

    eof = 0;
    while (!eof) {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            LOGF("Opus seek not implemented yet");
//            if (ov_time_seek(&vf, param)) {
                //ci->logf("ov_time_seek failed");
//            }

//            ci->set_elapsed(ov_time_tell(&vf));
            ci->seek_complete();
        }

        /*Get the ogg buffer for writing*/
        if (get_more_data(&oy) < 1) {
            goto done;
        }

        /* Loop for all complete pages we got (most likely only one) */
        while (ogg_sync_pageout(&oy, &og) == 1) {
            if (stream_init == 0) {
                ogg_stream_init(&os, ogg_page_serialno(&og));
                stream_init = 1;
            }

            /* Add page to the bitstream */
            ogg_stream_pagein(&os, &og);

            page_granule = ogg_page_granulepos(&og);
            /* page_nb_packets = ogg_page_packets(&og); */

            sample_rate = 48000;
            while (!eos && ogg_stream_packetout(&os, &op)==1){
                /* If first packet, process as Speex header */
                if (op.packetno == 0){
                    /* identification header */
                
                    if (opus_header_parse(op.packet, op.bytes, &header)==0){
                        LOGF("Could not parse header");
                        goto done;
                    }
                
                    opus_decoder_init(st, sample_rate, header.channels);
                    LOGF("Decoder inited");

                    opus_decoder_ctl(st, OPUS_SET_GAIN(header.gain));

                    ci->configure(DSP_SET_FREQUENCY, sample_rate);
                    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
                    ci->configure(DSP_SET_STEREO_MODE, (header.channels == 2) ?
                        STEREO_INTERLEAVED : STEREO_MONO);

                } else if (op.packetno == 1) {
                    /* Comment header */
                } else {
                    if (op.e_o_s) /* End of stream condition */
                        eos=1;

                    int ret;

                    /* Decode frame */
                    ret = opus_decode(st, (unsigned char*) op.packet, op.bytes, output, MAX_FRAME_SIZE, 0);

                    if (ret > 0) {
                        ci->pcmbuf_insert(output, NULL, ret);
                    } else {
                        if (ret < 0) {
                            LOGF("Returnfail");
                            goto done;
                        }
                        break;
                    }
                }
            }
            ci->set_elapsed(page_granule / 48);
        }
    }
    LOGF("Returned OK");
    error = CODEC_OK;
done:
    return error;
}

