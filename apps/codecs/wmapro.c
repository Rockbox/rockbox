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
#include "libwmapro/wmaprodec.h"

CODEC_HEADER

#define MAXSAMPLES  (1L << 12)  /* Max number of samples in a wma pro subframe */
#define MAXCHANNELS 8
#define BUFSIZE     MAXCHANNELS * MAXSAMPLES
static int32_t decoded[BUFSIZE] IBSS_ATTR;

AVCodecContext avctx;
AVPacket avpkt;

/* This function initialises AVCodecContext with the data needed for the wmapro
 * decoder to work. The required data is taken from asf_waveformatex_t because that's
 * what the rockbox asf metadata parser fill/work with. In the future, when the 
 * codec is being optimised for on-target playback this function should not be needed, 
 * as we will be working directly with WMAProDecodeCtx (declared in wmaprodec.c) */
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
enum codec_status codec_main(void)
{
    uint32_t elapsedtime;
    int retval;
    asf_waveformatex_t wfx;     /* Holds the stream properties */
    size_t resume_offset;
    int res;                    /* Return values from asf_read_packet() and decode_packet() */
    uint8_t* audiobuf;          /* Pointer to the payload of one wma pro packet */
    int audiobufsize;           /* Payload size */
    int packetlength = 0;       /* Logical packet size (minus the header size) */          
    int outlen = 0;             /* Number of bytes written to the output buffer */
    int pktcnt = 0;             /* Count of the packets played */

    /* Generic codec initialisation */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 32);
    

next_track:

    /* Wait for the metadata to be read */
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    retval = CODEC_OK;

    /* Remember the resume position */
    resume_offset = ci->id3->offset;
 restart_track:   
    if (codec_init()) {
        LOGF("(WMA PRO) Error: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto done;
    }

    /* Copy the format metadata we've stored in the id3 TOC field.  This
       saves us from parsing it again here. */
    memcpy(&wfx, ci->id3->toc, sizeof(wfx));
    
    ci->configure(DSP_SWITCH_FREQUENCY, wfx.rate);
    ci->configure(DSP_SET_STEREO_MODE, wfx.channels == 1 ?
                  STEREO_MONO : STEREO_INTERLEAVED);
    codec_set_replaygain(ci->id3);

    /* Initialise the AVCodecContext */
    init_codec_ctx(&avctx, &wfx);
    
    if (decode_init(&avctx) < 0) {
        LOGF("(WMA PRO) Error: Unsupported or corrupt file\n");
        retval = CODEC_ERROR;
        goto done;
    }

    /* Now advance the file position to the first frame */
    ci->seek_buffer(ci->id3->first_frame_offset);
    
    elapsedtime = 0;
    resume_offset = 0;
    
    /* The main decoding loop */

    while (pktcnt < wfx.numpackets)
    {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            goto done;
        }
        
        /* Deal with any pending seek requests */
        if (ci->seek_time){

            if (ci->seek_time == 1) {
                ci->seek_complete();
                goto restart_track; /* Pretend you never saw this... */
            }

            elapsedtime = asf_seek(ci->seek_time, &wfx);
            if (elapsedtime < 1){
                ci->seek_complete();
                goto next_track;
            }

            ci->set_elapsed(elapsedtime);
            ci->seek_complete();
        }

        res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &wfx);

        if (res < 0) {
           LOGF("(WMA PRO) Warning: asf_read_packet returned %d", res);
           goto done;
        } else {
            avpkt.data = audiobuf;
            avpkt.size = audiobufsize;
            pktcnt++;
            
            /* We now loop on the packet, decoding and outputting the subframes
             * one-by-one. For more information about how wma pro structures its
             * audio frames, see libwmapro/wmaprodec.c */
            while(avpkt.size > 0)
            {
                outlen = BUFSIZE;   /* decode_packet needs to know the size of the output buffer */
                res = decode_packet(&avctx, decoded, &outlen, &avpkt);
                avpkt.data += res;
                avpkt.size -= res;
                avctx.frame_number++;
                if(outlen) {
                    ci->yield ();
                    /* outlen now holds the size of the data in bytes - we want the
                     * number of samples. */
                    outlen /= (sizeof(int32_t) * wfx.channels);
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
    retval = CODEC_OK;

done:
    if (ci->request_next_track())
        goto next_track;
    
    return retval;
}

