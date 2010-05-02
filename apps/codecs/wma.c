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

/* The output buffer containing the decoded samples (channels 0 and 1)
   BLOCK_MAX_SIZE is 2048 (samples) and MAX_CHANNELS is 2.
 */

static uint32_t decoded[BLOCK_MAX_SIZE * MAX_CHANNELS] IBSS_ATTR;

/* NOTE: WMADecodeContext is 120152 bytes (on x86) */
static WMADecodeContext wmadec;

/*entry point for seeks*/
static int seek(int ms, asf_waveformatex_t* wfx)
{
    int time, duration, delta, temp, count=0;

    /*estimate packet number from bitrate*/
    int initial_packet = ci->curpos/wfx->packet_size;
    int packet_num = (((int64_t)ms)*(wfx->bitrate>>3))/wfx->packet_size/1000;
    int last_packet = ci->id3->filesize / wfx->packet_size;

    if (packet_num > last_packet) {
        packet_num = last_packet;
    }

    /*calculate byte address of the start of that packet*/
    int packet_offset = packet_num*wfx->packet_size;

    /*seek to estimated packet*/
    ci->seek_buffer(ci->id3->first_frame_offset+packet_offset);
    temp = ms;
    while (1)
    {
        /*for very large files it can be difficult and unimportant to find the exact packet*/
        count++;

        /*check the time stamp of our packet*/
        time = asf_get_timestamp(&duration);
        DEBUGF("seeked to %d ms with duration %d\n", time, duration);

        if (time < 0) {
            /*unknown error, try to recover*/
            DEBUGF("UKNOWN SEEK ERROR\n");
            ci->seek_buffer(ci->id3->first_frame_offset+initial_packet*wfx->packet_size);
            /*seek failed so return time stamp of the initial packet*/
            return asf_get_timestamp(&duration);
        }

        if ((time+duration>=ms && time<=ms) || count > 10) {
            DEBUGF("Found our packet! Now at %d packet\n", packet_num);
            return time;
        } else {
            /*seek again*/
            delta = ms-time;
            /*estimate new packet number from bitrate and our current position*/
            temp += delta;
            packet_num = ((temp/1000)*(wfx->bitrate>>3) - (wfx->packet_size>>1))/wfx->packet_size;  //round down!
            packet_offset = packet_num*wfx->packet_size;
            ci->seek_buffer(ci->id3->first_frame_offset+packet_offset);
        }
    }
}



/* this is the codec entry point */
enum codec_status codec_main(void)
{
    uint32_t elapsedtime;
    int retval;
    asf_waveformatex_t wfx;
    size_t resume_offset;
    int i;
    int wmares, res;
    uint8_t* audiobuf;
    int audiobufsize;
    int packetlength = 0;
    int errcount = 0;

    /* Generic codec initialisation */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 29);

next_track:

    /* Wait for the metadata to be read */
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    retval = CODEC_OK;

    /* Remember the resume position - when the codec is opened, the
       playback engine will reset it. */
    resume_offset = ci->id3->offset;
restart_track:
    if (codec_init()) {
        LOGF("WMA: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    /* Copy the format metadata we've stored in the id3 TOC field.  This
       saves us from parsing it again here. */
    memcpy(&wfx, ci->id3->toc, sizeof(wfx));

    if (wma_decode_init(&wmadec,&wfx) < 0) {
        LOGF("WMA: Unsupported or corrupt file\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    DEBUGF("**************** IN WMA.C ******************\n");

    if (resume_offset > ci->id3->first_frame_offset)
    {
        /* Get start of current packet */
        int packet_offset = (resume_offset - ci->id3->first_frame_offset)
            % wfx.packet_size;
        ci->seek_buffer(resume_offset - packet_offset);
        elapsedtime = asf_get_timestamp(&i);
        ci->set_elapsed(elapsedtime);
    }
    else
    {
        /* Now advance the file position to the first frame */
        ci->seek_buffer(ci->id3->first_frame_offset);
        elapsedtime = 0;
    }

    resume_offset = 0;
    ci->configure(DSP_SWITCH_FREQUENCY, wfx.rate);
    ci->configure(DSP_SET_STEREO_MODE, wfx.channels == 1 ?
                  STEREO_MONO : STEREO_INTERLEAVED);
    codec_set_replaygain(ci->id3);

    /* The main decoding loop */

    res = 1;
    while (res >= 0)
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

            elapsedtime = seek(ci->seek_time, &wfx);
            if (elapsedtime < 1){
                ci->seek_complete();
                goto next_track;
            }
            /*DEBUGF("Seek returned %d\n", (int)elapsedtime);*/
            ci->set_elapsed(elapsedtime);

            /*flush the wma decoder state*/
            wmadec.last_superframe_len = 0;
            wmadec.last_bitoffset = 0;
            ci->seek_complete();
        }
        errcount = 0;
new_packet:
        res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &wfx);

        if (res < 0) {
            /* We'll try to recover from a parse error a certain number of
             * times. If we succeed, the error counter will be reset.
             */

            errcount++;
            DEBUGF("read_packet error %d, errcount %d\n",wmares, errcount);
            if (errcount > 5) {
                goto done;
            } else {
                ci->advance_buffer(packetlength);
                goto new_packet;
            }
        } else if (res > 0) {
            wma_decode_superframe_init(&wmadec, audiobuf, audiobufsize);

            for (i=0; i < wmadec.nb_frames; i++)
            {
                wmares = wma_decode_superframe_frame(&wmadec,
                                                     decoded,
                                                     audiobuf, audiobufsize);

                ci->yield ();

                if (wmares < 0) {
                    /* Do the above, but for errors in decode. */
                    errcount++;
                    DEBUGF("WMA decode error %d, errcount %d\n",wmares, errcount);
                    if (errcount > 5) {
                        goto done;
                    } else {
                        ci->advance_buffer(packetlength);
                        goto new_packet;
                    }
                } else if (wmares > 0) {
                    ci->pcmbuf_insert(decoded, NULL, wmares);
                    elapsedtime += (wmares*10)/(wfx.rate/100);
                    ci->set_elapsed(elapsedtime);
                }
                ci->yield();
            }
        }

        ci->advance_buffer(packetlength);
    }
    retval = CODEC_OK;

done:
    /*LOGF("WMA: Decoded %ld samples\n",elapsedtime*wfx.rate/1000);*/

    if (ci->request_next_track())
        goto next_track;
exit:
    return retval;
}
