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

static int32_t *dec[2]; /* pointers to the output buffers in WMAProDecodeCtx in
                           wmaprodec.c */


/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_SAMPLE_DEPTH, WMAPRO_DSP_SAMPLE_DEPTH);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    uint32_t elapsedtime;
    asf_waveformatex_t wfx;     /* Holds the stream properties */
    int res;                    /* Return values from asf_read_packet() and decode_packet() */
    uint8_t* audiobuf;          /* Pointer to the payload of one wma pro packet */
    int audiobufsize;           /* Payload size */
    int packetlength = 0;       /* Logical packet size (minus the header size) */          
    int outlen = 0;             /* Number of bytes written to the output buffer */
    int pktcnt = 0;             /* Count of the packets played */
    uint8_t *data;              /* Pointer to decoder input buffer */
    int size;                   /* Size of the input frame to the decoder */
    intptr_t param;

restart_track:
    if (codec_init()) {
        LOGF("(WMA PRO) Error: Error initialising codec\n");
        return CODEC_ERROR;
    }

    /* Copy the format metadata we've stored in the id3 TOC field.  This
       saves us from parsing it again here. */
    memcpy(&wfx, ci->id3->toc, sizeof(wfx));
    
    ci->configure(DSP_SWITCH_FREQUENCY, wfx.rate);
    ci->configure(DSP_SET_STEREO_MODE, wfx.channels == 1 ?
                  STEREO_MONO : STEREO_NONINTERLEAVED);
    codec_set_replaygain(ci->id3);
    
    if (decode_init(&wfx) < 0) {
        LOGF("(WMA PRO) Error: Unsupported or corrupt file\n");
        return CODEC_ERROR;
    }

    /* Now advance the file position to the first frame */
    ci->seek_buffer(ci->id3->first_frame_offset);
    
    elapsedtime = 0;
    ci->set_elapsed(0);
    
    /* The main decoding loop */

    while (pktcnt < wfx.numpackets)
    { 
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        /* Deal with any pending seek requests */
        if (action == CODEC_ACTION_SEEK_TIME) {
            if (param == 0) {
                ci->set_elapsed(0);
                ci->seek_complete();
                goto restart_track; /* Pretend you never saw this... */
            }

            elapsedtime = asf_seek(param, &wfx);
            if (elapsedtime < 1){
                ci->set_elapsed(0);
                ci->seek_complete();
                break;
            }

            ci->set_elapsed(elapsedtime);
            ci->seek_complete();
        }

        res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &wfx);

        if (res < 0) {
            LOGF("(WMA PRO) Warning: asf_read_packet returned %d", res);
            return CODEC_ERROR;
        } else {
            data = audiobuf;
            size = audiobufsize;
            pktcnt++;
            
            /* We now loop on the packet, decoding and outputting the subframes
             * one-by-one. For more information about how wma pro structures its
             * audio frames, see libwmapro/wmaprodec.c */
            while(size > 0)
            {
                res = decode_packet(&wfx, dec, &outlen, data, size);
                if(res < 0) {
                    LOGF("(WMA PRO) Error: decode_packet returned %d", res);
                    return CODEC_ERROR;
                }
                data += res;
                size -= res;
                if(outlen) {
                    ci->yield ();
                    outlen /= (wfx.channels);
                    ci->pcmbuf_insert(dec[0], dec[1], outlen );
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

