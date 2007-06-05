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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#define ROCKBOX
#include <codecs/demac/libdemac/demac.h>

CODEC_HEADER

#define BLOCKS_PER_LOOP     4608
#define MAX_CHANNELS        2
#define MAX_BYTESPERSAMPLE  3

#define INPUT_CHUNKSIZE     (32*1024)

/* 4608*4 = 18432 bytes per channel */
static int32_t decoded0[BLOCKS_PER_LOOP] IBSS_ATTR;
static int32_t decoded1[BLOCKS_PER_LOOP] IBSS_ATTR;

#define MAX_SUPPORTED_SEEKTABLE_SIZE 5000

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    struct ape_ctx_t ape_ctx;
    uint32_t samplesdone;
    uint32_t elapsedtime;
    size_t bytesleft;
    int retval;

    uint32_t currentframe;
    int nblocks;
    int bytesconsumed;
    unsigned char* inbuffer;
    int blockstodecode;
    int res;
    int firstbyte;

    /* Generic codec initialisation */
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, 1024*128);

    ci->configure(DSP_SET_SAMPLE_DEPTH, APE_OUTPUT_DEPTH-1);

    next_track:
        
    if (codec_init()) {
        LOGF("APE: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    inbuffer = ci->request_buffer(&bytesleft, INPUT_CHUNKSIZE);

    /* Read the file headers to populate the ape_ctx struct */
    if (ape_parseheaderbuf(inbuffer,&ape_ctx) < 0) {
        LOGF("APE: Error reading header\n");
        retval = CODEC_ERROR;
        goto exit;
    }
    ci->advance_buffer(ape_ctx.firstframe);

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);
    
    ci->configure(DSP_SWITCH_FREQUENCY, ape_ctx.samplerate);
    ci->configure(DSP_SET_STEREO_MODE, ape_ctx.channels == 1 ?
                  STEREO_MONO : STEREO_NONINTERLEAVED);
    codec_set_replaygain(ci->id3);

    /* The main decoding loop */

    currentframe = 0;
    samplesdone = 0;

    /* Initialise the buffer */
    inbuffer = ci->request_buffer(&bytesleft, INPUT_CHUNKSIZE);
    firstbyte = 3;  /* Take account of the little-endian 32-bit byte ordering */

    /* The main decoding loop - we decode the frames a small chunk at a time */
    while (currentframe < ape_ctx.totalframes)
    {
        /* Calculate how many blocks there are in this frame */
        if (currentframe == (ape_ctx.totalframes - 1))
            nblocks = ape_ctx.finalframeblocks;
        else
            nblocks = ape_ctx.blocksperframe;

        ape_ctx.currentframeblocks = nblocks;

        /* Initialise the frame decoder */
        init_frame_decoder(&ape_ctx, inbuffer, &firstbyte, &bytesconsumed);

        ci->advance_buffer(bytesconsumed);
        inbuffer = ci->request_buffer(&bytesleft, INPUT_CHUNKSIZE);

        /* Decode the frame a chunk at a time */
        while (nblocks > 0)
        {
            ci->yield();
            if (ci->stop_codec || ci->new_track) {
                break;
            }

            blockstodecode = MIN(BLOCKS_PER_LOOP, nblocks);

            if ((res = decode_chunk(&ape_ctx, inbuffer, &firstbyte,
                                    &bytesconsumed,
                                    decoded0, decoded1,
                                    blockstodecode)) < 0)
            {
                /* Frame decoding error, abort */
                LOGF("APE: Frame %d, error %d\n",currentframe,res);
                retval = CODEC_ERROR;
                goto done;
            }

            ci->yield();
            ci->pcmbuf_insert(decoded0, decoded1, blockstodecode);
        
            /* Update the elapsed-time indicator */
            samplesdone += blockstodecode;
            elapsedtime = (samplesdone*10)/(ape_ctx.samplerate/100);
            ci->set_elapsed(elapsedtime);

            ci->advance_buffer(bytesconsumed);
            inbuffer = ci->request_buffer(&bytesleft, INPUT_CHUNKSIZE);

            /* Decrement the block count */
            nblocks -= blockstodecode;
        }

        currentframe++;
    }

    retval = CODEC_OK;

done:
    LOGF("APE: Decoded %ld samples\n",samplesdone);

    if (ci->request_next_track())
        goto next_track;

exit:
    return retval;
}
