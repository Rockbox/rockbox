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
#include <codecs/demac/libdemac/demac.h>

CODEC_HEADER

#define BLOCKS_PER_LOOP     1024
#define MAX_CHANNELS        2
#define MAX_BYTESPERSAMPLE  3

/* Monkey's Audio files have one seekpoint per frame.  The framesize
   varies between 73728 and 1179648 samples.  

   At the smallest framesize, 30000 frames would be 50155 seconds of
   audio - almost 14 hours.  This should be enough for any file a user
   would want to play in Rockbox, given the 2GB FAT filesize (and 4GB
   seektable entry size) limit.

   This means the seektable is 120000 bytes, but we have a lot of
   spare room in the codec buffer - the APE codec itself is small.
*/

#define MAX_SEEKPOINTS      30000
static uint32_t seektablebuf[MAX_SEEKPOINTS];

#define INPUT_CHUNKSIZE     (32*1024)

/* 1024*4 = 4096 bytes per channel */
static int32_t decoded0[BLOCKS_PER_LOOP] IBSS_ATTR;
static int32_t decoded1[BLOCKS_PER_LOOP] IBSS_ATTR;

#define MAX_SUPPORTED_SEEKTABLE_SIZE 5000


/* Given an ape_ctx and a sample to seek to, return the file position
   to the frame containing that sample, and the number of samples to
   skip in that frame.
*/

static bool ape_calc_seekpos(struct ape_ctx_t* ape_ctx,
                             uint32_t new_sample,
                             uint32_t* newframe,
                             uint32_t* filepos,
                             uint32_t* samplestoskip)
{
    uint32_t n;

    n = new_sample / ape_ctx->blocksperframe;
    if (n >= ape_ctx->numseekpoints)
    {
        /* We don't have a seekpoint for that frame */
        return false;
    }

    *newframe = n;
    *filepos = ape_ctx->seektable[n];
    *samplestoskip = new_sample - (n * ape_ctx->blocksperframe);

    return true;
}

/* The resume offset is a value in bytes - we need to
   turn it into a frame number and samplestoskip value */

static void ape_resume(struct ape_ctx_t* ape_ctx, size_t resume_offset, 
                       uint32_t* currentframe, uint32_t* samplesdone, 
                       uint32_t* samplestoskip, int* firstbyte)
{
    off_t newfilepos;
    int64_t framesize;
    int64_t offset;

    *currentframe = 0;
    *samplesdone = 0;
    *samplestoskip = 0;

    while ((*currentframe < ape_ctx->totalframes) &&
           (*currentframe < ape_ctx->numseekpoints) &&
           (resume_offset > ape_ctx->seektable[*currentframe]))
    {
        ++*currentframe;
        *samplesdone += ape_ctx->blocksperframe;
    }

    if ((*currentframe > 0) && 
        (ape_ctx->seektable[*currentframe] > resume_offset)) {
        --*currentframe;
        *samplesdone -= ape_ctx->blocksperframe;
    }

    newfilepos = ape_ctx->seektable[*currentframe];

    /* APE's bytestream is weird... */
    *firstbyte = 3 - (newfilepos & 3);
    newfilepos &= ~3;

    ci->seek_buffer(newfilepos);

    /* We estimate where we were in the current frame, based on the
       byte offset */
    if (*currentframe < (ape_ctx->totalframes - 1)) {
        framesize = ape_ctx->seektable[*currentframe+1] - ape_ctx->seektable[*currentframe];
        offset = resume_offset - ape_ctx->seektable[*currentframe];

        *samplestoskip = (offset * ape_ctx->blocksperframe) / framesize;
    }
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* Generic codec initialisation */
        ci->configure(DSP_SET_SAMPLE_DEPTH, APE_OUTPUT_DEPTH-1);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    struct ape_ctx_t ape_ctx;
    uint32_t samplesdone;
    uint32_t elapsedtime;
    size_t bytesleft;

    uint32_t currentframe;
    uint32_t newfilepos;
    uint32_t samplestoskip;
    int nblocks;
    int bytesconsumed;
    unsigned char* inbuffer;
    uint32_t blockstodecode;
    int res;
    int firstbyte;
    size_t resume_offset;
    long action;
    intptr_t param;

    if (codec_init()) {
        LOGF("APE: Error initialising codec\n");
        return CODEC_ERROR;
    }

    action = CODEC_ACTION_NULL;
    param = 0;

    /* Remember the resume position - when the codec is opened, the
       playback engine will reset it. */
    elapsedtime = ci->id3->elapsed;
    resume_offset = ci->id3->offset;

    ci->seek_buffer(0);
    inbuffer = ci->request_buffer(&bytesleft, INPUT_CHUNKSIZE);

    /* Read the file headers to populate the ape_ctx struct */
    if (ape_parseheaderbuf(inbuffer,&ape_ctx) < 0) {
        LOGF("APE: Error reading header\n");
        return CODEC_ERROR;
    }

    /* Initialise the seektable for this file */
    ape_ctx.seektable = seektablebuf;
    ape_ctx.numseekpoints = MIN(MAX_SEEKPOINTS,ape_ctx.numseekpoints);

    ci->advance_buffer(ape_ctx.seektablefilepos);

    /* The seektable may be bigger than the guard buffer (32KB), so we
       do a read() */
    ci->read_filebuf(ape_ctx.seektable, ape_ctx.numseekpoints * sizeof(uint32_t));

#ifdef ROCKBOX_BIG_ENDIAN
    /* Byte-swap the little-endian seekpoints */
    {
        uint32_t i;

        for(i = 0; i < ape_ctx.numseekpoints; i++)
            ape_ctx.seektable[i] = swap32(ape_ctx.seektable[i]);
    }
#endif

    /* Now advance the file position to the first frame */
    ci->advance_buffer(ape_ctx.firstframe - 
                       (ape_ctx.seektablefilepos +
                        ape_ctx.numseekpoints * sizeof(uint32_t)));

    ci->configure(DSP_SET_FREQUENCY, ape_ctx.samplerate);
    ci->configure(DSP_SET_STEREO_MODE, ape_ctx.channels == 1 ?
                  STEREO_MONO : STEREO_NONINTERLEAVED);
    codec_set_replaygain(ci->id3);

    /* The main decoding loop */

    if (resume_offset) {
        /* The resume offset is a value in bytes - we need to
           turn it into a frame number and samplestoskip value */

        ape_resume(&ape_ctx, resume_offset, 
                   &currentframe, &samplesdone, &samplestoskip, &firstbyte);
        elapsedtime = samplesdone*1000LL/ape_ctx.samplerate;
    }
    else {
        currentframe = 0;
        samplesdone = 0;
        samplestoskip = 0;
        firstbyte = 3;  /* Take account of the little-endian 32-bit byte ordering */

        if (elapsedtime) {
            /* Resume by simulated seeking */
            param = elapsedtime;
            action = CODEC_ACTION_SEEK_TIME;
        }
    }

    ci->set_elapsed(elapsedtime);

    /* Initialise the buffer */
    inbuffer = ci->request_buffer(&bytesleft, INPUT_CHUNKSIZE);

    /* The main decoding loop - we decode the frames a small chunk at a time */
    while (currentframe < ape_ctx.totalframes)
    {
frame_start:
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
            if (action == CODEC_ACTION_NULL)
                action = ci->get_command(&param);

            if (action != CODEC_ACTION_NULL) {
                if (action == CODEC_ACTION_HALT)
                    goto done;

                /* Deal with any pending seek requests */
                if (action == CODEC_ACTION_SEEK_TIME)
                {
                    if (ape_calc_seekpos(&ape_ctx,
                        (param/10) * (ci->id3->frequency/100),
                        &currentframe,
                        &newfilepos,
                        &samplestoskip))
                    {
                        samplesdone = currentframe * ape_ctx.blocksperframe;

                        /* APE's bytestream is weird... */
                        firstbyte = 3 - (newfilepos & 3);
                        newfilepos &= ~3;

                        ci->seek_buffer(newfilepos);
                        inbuffer = ci->request_buffer(&bytesleft,
                                                      INPUT_CHUNKSIZE);

                        elapsedtime = samplesdone*1000LL/ape_ctx.samplerate;
                        ci->set_elapsed(elapsedtime);
                        ci->seek_complete();
                        action = CODEC_ACTION_NULL;
                        goto frame_start;  /* Sorry... */
                    }

                    ci->seek_complete();
                }

                action = CODEC_ACTION_NULL;
            }

            blockstodecode = MIN(BLOCKS_PER_LOOP, nblocks);

            if ((res = decode_chunk(&ape_ctx, inbuffer, &firstbyte,
                                    &bytesconsumed,
                                    decoded0, decoded1,
                                    blockstodecode)) < 0)
            {
                /* Frame decoding error, abort */
                LOGF("APE: Frame %lu, error %d\n",(unsigned long)currentframe,res);
                return CODEC_ERROR;
            }

            ci->yield();

            if (samplestoskip > 0) {
                if (samplestoskip < blockstodecode) {
                    ci->pcmbuf_insert(decoded0 + samplestoskip, 
                                      decoded1 + samplestoskip, 
                                      blockstodecode - samplestoskip);
                    samplestoskip = 0;
                } else {
                    samplestoskip -= blockstodecode;
                }
            } else {
                ci->pcmbuf_insert(decoded0, decoded1, blockstodecode);
            }
        
            samplesdone += blockstodecode;

            if (!samplestoskip) {
                /* Update the elapsed-time indicator */
                elapsedtime = samplesdone*1000LL/ape_ctx.samplerate;
                ci->set_elapsed(elapsedtime);
            }

            ci->advance_buffer(bytesconsumed);
            inbuffer = ci->request_buffer(&bytesleft, INPUT_CHUNKSIZE);

            /* Decrement the block count */
            nblocks -= blockstodecode;
        }

        currentframe++;
    }

done:
    LOGF("APE: Decoded %lu samples\n",(unsigned long)samplesdone);
    return CODEC_OK;
}
