/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Mohamed Tarek
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
#include "libasf/asf.h"
#include "libwmavoice/wmavoice.h"

CODEC_HEADER

static AVCodecContext avctx;
static AVPacket avpkt;

#define MAX_FRAMES           3   /*maximum number of frames per superframe*/
#define MAX_FRAMESIZE        160 /* maximum number of samples per frame */
#define BUFSIZE              MAX_FRAMES*MAX_FRAMESIZE
static int32_t decoded[BUFSIZE] IBSS_ATTR;


/* This function initialises AVCodecContext with the data needed for the wmapro
 * decoder to work. The required data is taken from asf_waveformatex_t because that's
 * what the rockbox asf metadata parser fill/work with. In the future, when the 
 * codec is being optimised for on-target playback this function should not be needed. */
static void init_codec_ctx(AVCodecContext *avctx, asf_waveformatex_t *wfx)
{
    /* Copy the extra-data */
    avctx->extradata_size = wfx->datalen;
    avctx->extradata = (uint8_t *)malloc(wfx->datalen*sizeof(uint8_t));
    memcpy(avctx->extradata, wfx->data, wfx->datalen*sizeof(uint8_t));
    
    avctx->block_align = wfx->blockalign;
    avctx->sample_rate = wfx->rate;
    avctx->channels    = wfx->channels;
    
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 31);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    uint32_t elapsedtime;
    asf_waveformatex_t wfx;     /* Holds the stream properties */
    size_t resume_offset;
    int res;                    /* Return values from asf_read_packet() and decode_packet() */
    uint8_t* audiobuf;          /* Pointer to the payload of one wma pro packet */
    int audiobufsize;           /* Payload size */
    int packetlength = 0;       /* Logical packet size (minus the header size) */          
    int outlen = 0;             /* Number of bytes written to the output buffer */
    int pktcnt = 0;             /* Count of the packets played */
    intptr_t param;

    /* Remember the resume position */
    resume_offset = ci->id3->offset;
restart_track:
    if (codec_init()) {
        LOGF("(WMA Voice) Error: Error initialising codec\n");
        return CODEC_ERROR;
    }

    /* Copy the format metadata we've stored in the id3 TOC field.  This
       saves us from parsing it again here. */
    memcpy(&wfx, ci->id3->toc, sizeof(wfx));
    memset(&avctx, 0, sizeof(AVCodecContext));
    memset(&avpkt, 0, sizeof(AVPacket));
    
    ci->configure(DSP_SWITCH_FREQUENCY, wfx.rate);
    ci->configure(DSP_SET_STEREO_MODE, wfx.channels == 1 ?
                  STEREO_MONO : STEREO_INTERLEAVED);
    codec_set_replaygain(ci->id3);

    ci->seek_buffer(0);
    
    /* Initialise the AVCodecContext */
    init_codec_ctx(&avctx, &wfx);

    if (wmavoice_decode_init(&avctx) < 0) {
        LOGF("(WMA Voice) Error: Unsupported or corrupt file\n");
        return CODEC_ERROR;
    }

    /* Now advance the file position to the first frame */
    ci->seek_buffer(ci->id3->first_frame_offset);
    
    elapsedtime = 0;
    ci->set_elapsed(0);

    resume_offset = 0;
    
    /* The main decoding loop */

    while (pktcnt < wfx.numpackets)
    {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        /* Deal with any pending seek requests */
        if (action == CODEC_ACTION_SEEK_TIME) {
            ci->set_elapsed(param);

            if (param == 0) {
                ci->set_elapsed(0);
                ci->seek_complete();
                goto restart_track; /* Pretend you never saw this... */
            }

            elapsedtime = asf_seek(param, &wfx);
            if (elapsedtime < 1){
                ci->set_elapsed(0);
                ci->seek_complete();
                goto next_track;
            }

            ci->set_elapsed(elapsedtime);
            ci->seek_complete();
        }
        
new_packet:
        res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &wfx);

        if (res < 0) {
            LOGF("(WMA Voice) read_packet error %d\n",res);
            return CODEC_ERROR;
        } else {
            avpkt.data = audiobuf;
            avpkt.size = audiobufsize;
            pktcnt++;
            
            while(avpkt.size > 0)
            {
                /* wmavoice_decode_packet checks for the output buffer size to 
                   avoid overflows */
                outlen = BUFSIZE*sizeof(int32_t);

                res = wmavoice_decode_packet(&avctx, decoded, &outlen, &avpkt);
                if(res < 0) {
                    LOGF("(WMA Voice) Error: decode_packet returned %d", res);
                    if(res == ERROR_WMAPRO_IN_WMAVOICE){
                    /* Just skip this packet */
                        ci->advance_buffer(packetlength);
                        goto new_packet;    
                    }
                    else {
                        return CODEC_ERROR;
                    }
                }
                avpkt.data += res;
                avpkt.size -= res;
                if(outlen) {
                    ci->yield ();
                    outlen /= sizeof(int32_t);
                    ci->pcmbuf_insert(decoded, NULL, outlen);
                    elapsedtime += outlen*10/(wfx.rate/100);
                    ci->set_elapsed(elapsedtime);
                    ci->yield ();
                }
            }

        }

        /* Advance to the next logical packet */
        ci->advance_buffer(packetlength);
    }

    return CODEC_OK;
}

