/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2005 Mark Arigo
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
#include <codecs/libffmpegFLAC/shndec.h>

CODEC_HEADER

#ifndef IBSS_ATTR_SHORTEN_DECODED0
#define IBSS_ATTR_SHORTEN_DECODED0 IBSS_ATTR
#endif

static int32_t decoded0[MAX_DECODE_SIZE] IBSS_ATTR_SHORTEN_DECODED0;
static int32_t decoded1[MAX_DECODE_SIZE] IBSS_ATTR;

static int32_t offset0[MAX_OFFSET_SIZE] IBSS_ATTR;
static int32_t offset1[MAX_OFFSET_SIZE] IBSS_ATTR;

static int8_t ibuf[MAX_BUFFER_SIZE] IBSS_ATTR;

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
        ci->configure(DSP_SET_SAMPLE_DEPTH, SHN_OUTPUT_DEPTH-1);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    ShortenContext sc;
    uint32_t samplesdone;
    uint32_t elapsedtime;
    int8_t *buf;
    int consumed, res, nsamples;
    size_t bytesleft;
    intptr_t param;

    /* Codec initialization */
    if (codec_init()) {
        LOGF("Shorten: codec_init error\n");
        return CODEC_ERROR;
    }

    codec_set_replaygain(ci->id3);

    /* Shorten decoder initialization */
    ci->memset(&sc, 0, sizeof(ShortenContext));

    /* Skip id3v2 tags */
    ci->seek_buffer(ci->id3->first_frame_offset);

    /* Read the shorten & wave headers */
    buf = ci->request_buffer(&bytesleft, MAX_HEADER_SIZE);
    res = shorten_init(&sc, (unsigned char *)buf, bytesleft);
    if (res < 0) {
        LOGF("Shorten: shorten_init error: %d\n", res);
        return CODEC_ERROR;
    }

    ci->id3->frequency = sc.sample_rate;
    ci->configure(DSP_SET_FREQUENCY, sc.sample_rate);

    if (sc.sample_rate) {
        ci->id3->length = (sc.totalsamples / sc.sample_rate) * 1000;
    } else {
        ci->id3->length = 0;
    }

    if (ci->id3->length) {
        ci->id3->bitrate = (ci->id3->filesize * 8) / ci->id3->length;
    }

    consumed = sc.gb.index/8;
    ci->advance_buffer(consumed);
    sc.bitindex = sc.gb.index - 8*consumed;

seek_start:
    ci->set_elapsed(0);

    /* The main decoding loop */
    ci->memset(&decoded0, 0, sizeof(int32_t)*MAX_DECODE_SIZE);
    ci->memset(&decoded1, 0, sizeof(int32_t)*MAX_DECODE_SIZE);
    ci->memset(&offset0, 0, sizeof(int32_t)*MAX_OFFSET_SIZE);
    ci->memset(&offset1, 0, sizeof(int32_t)*MAX_OFFSET_SIZE);

    samplesdone = 0;
    buf = ci->request_buffer(&bytesleft, MAX_BUFFER_SIZE);
    while (bytesleft) {
        long action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        /* Seek to start of track */
        if (action == CODEC_ACTION_SEEK_TIME) {
            if (param == 0 &&
                ci->seek_buffer(sc.header_bits/8 + ci->id3->first_frame_offset)) {
                sc.bitindex = sc.header_bits - 8*(sc.header_bits/8);
                ci->seek_complete();
                goto seek_start;
            }
            ci->seek_complete();
        }

        /* Decode a frame */
        ci->memcpy(ibuf, buf, bytesleft); /* copy buf to iram */
        res = shorten_decode_frames(&sc, &nsamples, decoded0, decoded1,
                                    offset0, offset1, (unsigned char *)ibuf,
                                    bytesleft, ci->yield);
 
        if (res == FN_ERROR) {
            LOGF("Shorten: shorten_decode_frames error (%lu)\n",
                (unsigned long)samplesdone);
            return CODEC_ERROR;
        } else {
            /* Insert decoded samples in pcmbuf */
            if (nsamples) {
                ci->yield();
                ci->pcmbuf_insert(decoded0 + sc.nwrap, decoded1 + sc.nwrap,
                                  nsamples);

                /* Update the elapsed-time indicator */
                samplesdone += nsamples;
                elapsedtime = samplesdone*1000LL/sc.sample_rate;
                ci->set_elapsed(elapsedtime);
            }

            /* End of shorten stream...go to next track */
            if (res == FN_QUIT)
                break;
        }

        consumed = sc.gb.index/8;
        ci->advance_buffer(consumed);
        buf = ci->request_buffer(&bytesleft, MAX_BUFFER_SIZE);
        sc.bitindex = sc.gb.index - 8*consumed;
    }

    return CODEC_OK;
}
