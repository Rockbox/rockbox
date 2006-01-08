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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include <codecs/libffmpegFLAC/shndec.h>

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

struct codec_api* rb;
struct codec_api* ci;

#define MAX_DECODED (DEFAULT_BLOCK_SIZE + MAX_NWRAP)
int32_t decoded0[MAX_DECODED] IBSS_ATTR;
int32_t decoded1[MAX_DECODED] IBSS_ATTR;

#define MAX_OFFSETS MAX_NMEAN
int32_t offset0[MAX_OFFSETS] IBSS_ATTR;
int32_t offset1[MAX_OFFSETS] IBSS_ATTR;

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    ShortenContext sc;
    uint32_t samplesdone;
    uint32_t elapsedtime;
    int8_t *buf;
    int cur_chan, consumed, res;
    long bytesleft;

    /* Generic codec initialisation */
    TEST_CODEC_API(api);
    rb = api;
    ci = (struct codec_api*)api;

#ifdef USE_IRAM
    ci->memcpy(iramstart, iramcopy, iramend-iramstart);
    ci->memset(iedata, 0, iend - iedata);
#endif

    ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*128));

    ci->configure(CODEC_DSP_ENABLE, (bool *)true);
    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_STEREO_MODE, (long *)STEREO_NONINTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(SHN_OUTPUT_DEPTH-1));
    
next_track:
    /* Codec initialization */
    if (codec_init(api)) {
        LOGF("Shorten: Error initialising codec\n");
        return CODEC_ERROR;
    }

    while (!*ci->taginfo_ready)
        ci->yield();

    /* Shorten decoder initialization */
    ci->memset(&sc, 0, sizeof(ShortenContext));

    /* Skip id3v2 tags */
    if (ci->id3->first_frame_offset) {
        buf = ci->request_buffer(&bytesleft, ci->id3->first_frame_offset);
        ci->advance_buffer(ci->id3->first_frame_offset);
    }

    /* Read the shorten & wave headers */
    buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
    res = shorten_init(&sc, (unsigned char *)buf, bytesleft);
    if (res < 0) {
        LOGF("shorten_init error: %d\n", res);
        return CODEC_ERROR;
    }

    ci->id3->frequency = sc.sample_rate;
    ci->configure(DSP_SET_FREQUENCY, (long *)(sc.sample_rate));

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
    /* The main decoding loop */
    ci->memset(&decoded0, 0, sizeof(int32_t)*MAX_DECODED);
    ci->memset(&decoded1, 0, sizeof(int32_t)*MAX_DECODED);
    ci->memset(&offset0, 0, sizeof(int32_t)*MAX_OFFSETS);
    ci->memset(&offset1, 0, sizeof(int32_t)*MAX_OFFSETS);

    cur_chan = 0;
    samplesdone = 0;
    buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
    while (bytesleft) {
        ci->yield();
        if (ci->stop_codec || ci->reload_codec) {
            break;
        }

        /* Seek to start of track */
        if (ci->seek_time == 1) {
            if (ci->seek_buffer(sc.header_bits/8 + ci->id3->first_frame_offset)) {
                sc.bitindex = sc.header_bits - 8*(sc.header_bits/8);
                ci->set_elapsed(0);
                ci->seek_complete();
                goto seek_start;
            }
            ci->seek_complete();
        }

        /* Decode a frame */
        ci->yield();
        if (cur_chan == 0) {
            res = shorten_decode_frame(&sc, decoded0 + sc.nwrap, offset0,
                                       (unsigned char *)buf, bytesleft);
        } else {
            res = shorten_decode_frame(&sc, decoded1 + sc.nwrap, offset1,
                                       (unsigned char *)buf, bytesleft);
        }
        cur_chan++;

        if (res == 0 && cur_chan == sc.channels) {
            cur_chan = 0;

            /* Insert decoded samples in pcmbuf */
            ci->yield();
            while (!ci->pcmbuf_insert_split((char*)(decoded0 + sc.nwrap),
                    (char*)(decoded1 + sc.nwrap), sc.blocksize*4)) {
                ci->yield();
            }

            /* Update the elapsed-time indicator */
            samplesdone += sc.blocksize;
            elapsedtime = (samplesdone*10) / (sc.sample_rate/100);
            ci->set_elapsed(elapsedtime);
        } else if (res == 1) {
            /* End of shorten stream...go to next track */
            break;
        } else if (res < 0) {
            LOGF("shorten_decode_frame error: \n", res);
            return CODEC_ERROR;
        }

        consumed = sc.gb.index/8;
        ci->advance_buffer(consumed);
        sc.bitindex = sc.gb.index - 8*consumed;
        buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
    }

    LOGF("Shorten: Decoded %d samples\n", samplesdone);

    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;
}
