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

/* the opus pseudo stack pointer */
extern char *global_stack;

/* Room for 120 ms of stereo audio at 48 kHz */
#define MAX_FRAME_SIZE  (2*120*48)
#define CHUNKSIZE       (16*1024)

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
    (void)reason;

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    int error = CODEC_ERROR;
    intptr_t param;
    ogg_sync_state oy;
    ogg_page og;
    ogg_packet op;
    ogg_stream_state os;
    int64_t page_granule = 0;
    int stream_init = 0;
    int sample_rate = 48000;
    OpusDecoder *st = NULL;
    OpusHeader header;
    int ret;

    /* reset our simple malloc */
    if (codec_init()) {
        goto done;
    }
    global_stack = 0;

    /* pre-init the ogg_sync_state buffer, so it won't need many reallocs */
    ogg_sync_init(&oy);
    oy.storage = 64*1024;
    oy.data = codec_malloc(oy.storage);

    /* allocate output buffer */
    uint16_t *output = (uint16_t*) codec_malloc(MAX_FRAME_SIZE*sizeof(uint16_t));

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

    while (1) {
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

            while ((ogg_stream_packetout(&os, &op) == 1) && !op.e_o_s) {
                if (op.packetno == 0){
                    /* identification header */
                
                    if (opus_header_parse(op.packet, op.bytes, &header) == 0) {
                        LOGF("Could not parse header");
                        goto done;
                    }
                
                    st = opus_decoder_create(sample_rate, header.channels, &ret);
                    if (ret != OPUS_OK) {
                        LOGF("opus_decoder_create failed %d", ret);
                        goto done;
                    }
                    LOGF("Decoder inited");

                    codec_set_replaygain(ci->id3);

                    opus_decoder_ctl(st, OPUS_SET_GAIN(header.gain));

                    ci->configure(DSP_SET_FREQUENCY, sample_rate);
                    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
                    ci->configure(DSP_SET_STEREO_MODE, (header.channels == 2) ?
                        STEREO_INTERLEAVED : STEREO_MONO);

                } else if (op.packetno == 1) {
                    /* Comment header */
                } else {
                    /* Decode audio packets */
                    ret = opus_decode(st, (unsigned char*) op.packet, op.bytes, output, MAX_FRAME_SIZE, 0);

                    if (ret > 0) {
                        ci->pcmbuf_insert(output, NULL, ret);
                    } else {
                        if (ret < 0) {
                            LOGF("opus_decode failed %d", ret);
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

