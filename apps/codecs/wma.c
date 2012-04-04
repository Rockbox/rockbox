/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "libwma/wmadec.h"

CODEC_HEADER

/* NOTE: WMADecodeContext is 120152 bytes (on x86) */
static WMADecodeContext wmadec;

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 29);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    uint32_t elapsedtime;
    asf_waveformatex_t wfx;
    size_t resume_offset;
    int i;
    int wmares = 0;
    int res = 0;
    uint8_t* audiobuf;
    int audiobufsize;
    int packetlength = 0;
    int errcount = 0;
    intptr_t param;

    /* Remember the resume position - when the codec is opened, the
       playback engine will reset it. */
    resume_offset = ci->id3->offset;

restart_track:

    /* Proper reset of the decoder context. */
    memset(&wmadec, 0, sizeof(wmadec));

    if (codec_init()) {
        LOGF("WMA: Error initialising codec\n");
        return CODEC_ERROR;
    }

    /* Copy the format metadata we've stored in the id3 TOC field.  This
       saves us from parsing it again here. */
    memcpy(&wfx, ci->id3->toc, sizeof(wfx));

    ci->seek_buffer(ci->id3->first_frame_offset);
    if (wma_decode_init(&wmadec,&wfx) < 0) {
        LOGF("WMA: Unsupported or corrupt file\n");
        return CODEC_ERROR;
    }

    if (resume_offset > ci->id3->first_frame_offset)
    {
        /* Get start of current packet */
        int packet_offset = (resume_offset - ci->id3->first_frame_offset)
            % wfx.packet_size;
        ci->seek_buffer(resume_offset - packet_offset);
        elapsedtime = asf_get_timestamp(&i);
    }
    else
    {
        /* Now advance the file position to the first frame */
        ci->seek_buffer(ci->id3->first_frame_offset);
        elapsedtime = 0;
    }

    ci->set_elapsed(elapsedtime);

    resume_offset = 0;
    ci->configure(DSP_SWITCH_FREQUENCY, wfx.rate);
    ci->configure(DSP_SET_STEREO_MODE, wfx.channels == 1 ?
                  STEREO_MONO : STEREO_NONINTERLEAVED);
    codec_set_replaygain(ci->id3);

    /* The main decoding loop */
    while (res >= 0)
    {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        /* Deal with any pending seek requests */
        if (action == CODEC_ACTION_SEEK_TIME) {

            /*flush the wma decoder state*/
            wmadec.last_superframe_len = 0;
            wmadec.last_bitoffset = 0;

            /*zero the frame out buffer so we don't overlap with a 
                stale samples*/
            memset((*(wmadec.frame_out)), 0,
                sizeof(fixed32) * MAX_CHANNELS * BLOCK_MAX_SIZE * 2);

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
            /*DEBUGF("Seek returned %d\n", (int)elapsedtime);*/

            ci->set_elapsed(elapsedtime);
            ci->seek_complete();
        }
        errcount = 0;
new_packet:
        res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &wfx);

        if (res < 0) {
            /* We'll try to recover from a parse error a certain number of
             * times. If we succeed, the error counter will be reset.
             */

            if (res == ASF_ERROR_EOF) {
                /* File ended - not an error */
                break;
            }

            errcount++;
            DEBUGF("read_packet error %d, errcount %d\n",wmares, errcount);
            if (errcount > 5) {
                return CODEC_ERROR;
            } else {
                ci->advance_buffer(packetlength);
                goto new_packet;
            }
        } else if (res > 0) {
            wma_decode_superframe_init(&wmadec, audiobuf, audiobufsize);

            for (i=0; i < wmadec.nb_frames; i++)
            {
                wmares = wma_decode_superframe_frame(&wmadec,
                                                     audiobuf, audiobufsize);

                ci->yield ();

                if (wmares < 0) {
                    /* Do the above, but for errors in decode. */
                    errcount++;
                    DEBUGF("WMA decode error %d, errcount %d\n",wmares, errcount);
                    if (errcount > 5) {
                        return CODEC_ERROR;
                    } else {
                        ci->advance_buffer(packetlength);
                        goto new_packet;
                    }
                } else if (wmares > 0) {
                    ci->pcmbuf_insert((*wmadec.frame_out)[0], (*wmadec.frame_out)[1], wmares);
                    elapsedtime += (wmares*10)/(wfx.rate/100);
                    ci->set_elapsed(elapsedtime);
                }
            }
        }

        ci->advance_buffer(packetlength);
    }

    /*LOGF("WMA: Decoded %ld samples\n",elapsedtime*wfx.rate/1000);*/
    return CODEC_OK;
}
