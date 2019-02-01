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

#define SEEK_REWIND 3840    /* 80 ms @ 48 kHz */

/* the opus pseudo stack pointer */
extern char *global_stack;

/* Room for 120 ms of stereo audio at 48 kHz */
#define MAX_FRAME_SIZE  (2*120*48)
#define CHUNKSIZE       (16*1024)
#define SEEK_CHUNKSIZE 7*CHUNKSIZE

static int get_more_data(ogg_sync_state *oy)
{
    int bytes;
    char *buffer;

    buffer = (char *)ogg_sync_buffer(oy, CHUNKSIZE);
    bytes = ci->read_filebuf(buffer, CHUNKSIZE);
    ogg_sync_wrote(oy,bytes);

    return bytes;
}

/* seek to ogg page after given file position */
static int seek_ogg_page(uint64_t filepos)
{
    const char synccode[] = "OggS\0"; /* Note: there are two nulls here */
    char buf[sizeof(synccode)];
    ci->seek_buffer(filepos);
    while (ci->read_filebuf(buf, 1) == 1) {
        if (buf[0] == synccode[0]) {
            if (ci->read_filebuf(&buf[1], sizeof(buf) - 1) != sizeof(buf) - 1)
                break;
            if (memcmp(buf, synccode, sizeof(buf)) == 0) {
                ci->seek_buffer(ci->curpos - sizeof(buf));
                LOGF("next page %ld", ci->curpos);
                return 1;
            } else
                ci->seek_buffer(ci->curpos - (sizeof(buf) - 1));
        }
    }
    return 0;
}

/* seek to comment header */
static int seek_opus_tags(void)
{
    const char synccode[] = "OpusTags";
    char buf[sizeof(synccode) - 1]; /* Exclude null */
    ci->seek_buffer(0);
    while (ci->read_filebuf(buf, 1) == 1) {
        if (buf[0] == synccode[0]) {
            if (ci->read_filebuf(&buf[1], sizeof(buf) - 1) != sizeof(buf) - 1)
                break;
            if (memcmp(buf, synccode, sizeof(buf)) == 0) {
                ci->seek_buffer(ci->curpos - sizeof(buf));
                LOGF("OpusTags %ld", ci->curpos);
                return 1;
            } else
                ci->seek_buffer(ci->curpos - (sizeof(buf) - 1));
        }
    }
    /* comment header not found probably invalid file */
    return 0;
}

/* The read/seek functions track absolute position within the stream */
static int64_t get_next_page(ogg_sync_state *oy, ogg_page *og,
                                 int64_t boundary)
{
    int64_t localoffset = ci->curpos;
    long more;
    long ret;

    if (boundary > 0)
        boundary += ci->curpos;

    while (1) {
        more = ogg_sync_pageseek(oy,og);

        if (more < 0) {
            /* skipped n bytes */
            localoffset-=more;
        } else {
            if (more == 0) {
                /* send more data */
                if(!boundary)return(-1);
                {
                    ret = get_more_data(oy);
                    if (ret == 0)
                        return(-2);

                    if (ret < 0)
                        return(-3);
                }
            } else {
                /* got a page.  Return the offset at the page beginning,
                   advance the internal offset past the page end */

                int64_t ret=localoffset;

                return(ret);
            }
        }
    }
}

static int64_t seek_backwards(ogg_sync_state *oy, ogg_page *og,
                                  int64_t wantedpos)
{
    int64_t crofs;
    int64_t *curoffset=&crofs;
    *curoffset=ci->curpos;
    int64_t begin=*curoffset;
    int64_t end=begin;
    int64_t ret;
    int64_t offset=-1;
    int64_t avgpagelen=-1;
    int64_t lastgranule=-1;

    short time = -1;

    while (offset == -1) {

        begin -= SEEK_CHUNKSIZE;

        if (begin < 0) {
            if (time < 0) {
                begin = 0;
                time++;
            } else {
                LOGF("Can't seek that early:%lld\n",(long long int)begin);
                return -3;  /* too early */
            }
        }

        *curoffset = begin;

        ci->seek_buffer(*curoffset);

        ogg_sync_reset(oy);

        lastgranule = -1;

        while (*curoffset < end) {
            ret = get_next_page(oy,og,end-*curoffset);

            if (ret > 0) {
                if (lastgranule != -1) {
                    if (avgpagelen < 0)
                        avgpagelen = (ogg_page_granulepos(og)-lastgranule);
                    else
                       avgpagelen=((ogg_page_granulepos(og)-lastgranule)
                                   + avgpagelen) / 2;
                }

                lastgranule=ogg_page_granulepos(og);

                if ((lastgranule - (avgpagelen/4)) < wantedpos &&
                    (lastgranule + avgpagelen + (avgpagelen/4)) > wantedpos) {

                    /*wanted offset found Yeay!*/

                    /*LOGF("GnPagefound:%d,%d,%d,%d\n",ret,
                           lastgranule,wantedpos,avgpagelen);*/

                    return ret;

                } else if (lastgranule > wantedpos) {  /*too late, seek more*/
                    if (offset != -1) {
                        LOGF("Toolate, returnanyway:%lld,%lld,%lld,%lld\n",
                             (long long int)ret, (long long int)lastgranule,
                              (long long int)wantedpos, (long long int)avgpagelen);
                        return ret;
                    }
                    break;
                } else{ /*if (ogg_page_granulepos(&og)<wantedpos)*/
                    /*too early*/
                    offset = ret;
                    continue;
                }
            } else if (ret == -3)
                return(-3);
            else if (ret<=0)
                break;
            else if (*curoffset < end) {
                /*this should not be possible*/

                //LOGF("Seek:get_earlier_page:Offset:not_cached by granule:"\"%d,%d,%d,%d,%d\n",*curoffset,end,begin,wantedpos,curpos);

                offset=ret;
            }
        }
    }
    return -1;
}

static int opus_seek_page_granule(int64_t pos, int64_t curpos,
                                   ogg_sync_state *oy, ogg_stream_state *os)
{
    /* TODO: Someone may want to try to implement seek to packet,
             instead of just to page (should be more accurate, not be any
             faster) */

    int64_t crofs;
    int64_t *curbyteoffset = &crofs;
    *curbyteoffset = ci->curpos;
    int64_t curoffset;
    curoffset = *curbyteoffset;
    int64_t offset = 0;
    ogg_page og = {0,0,0,0};
    int64_t avgpagelen = -1;
    int64_t lastgranule = -1;
#if 0
    if(abs(pos-curpos)>10000 && headerssize>0 && curoffset-headerssize>10000) {
        /* if seeking for more that 10sec,
           headersize is known & more than 10kb is played,
           try to guess a place to seek from the number of
           bytes playe for this position, this works best when
           the bitrate is relativly constant.
         */

        curoffset = (((*curbyteoffset-headerssize) * pos)/curpos)*98/100;
        if (curoffset < 0)
            curoffset=0;

        //int64_t toffset=curoffset;

        ci->seek_buffer(curoffset);

        ogg_sync_reset(oy);

        offset = get_next_page(oy,&og,-1);

        if (offset < 0) { /* could not find new page,use old offset */
            LOGF("Seek/guess/fault:%lld->-<-%d,%lld:%lld,%d,%ld,%d\n",
                 curpos,0,pos,offset,0,
                 ci->curpos,/*stream_length*/0);

            curoffset = *curbyteoffset;

            ci->seek_buffer(curoffset);

            ogg_sync_reset(oy);
        } else {
            if (ogg_page_granulepos(&og) == 0 && pos > 5000) {
                LOGF("SEEK/guess/fault:%lld->-<-%lld,%lld:%lld,%d,%ld,%d\n",
                     curpos,ogg_page_granulepos(&og),pos,
                     offset,0,ci->curpos,/*stream_length*/0);

                curoffset = *curbyteoffset;

                ci->seek_buffer(curoffset);

                ogg_sync_reset(oy);
            } else {
                curoffset = offset;
                curpos = ogg_page_granulepos(&og);
            }
        }
    }
#endif
    /* which way do we want to seek? */
    if (pos == 0) {  /* start */
        *curbyteoffset = 0;
        ci->seek_buffer(*curbyteoffset);
        ogg_sync_reset(oy);
        ogg_stream_reset(os);
        return 0;
    } else if (curpos > pos) {  /* backwards */
        offset = seek_backwards(oy,&og,pos);

        if (offset > 0) {
            *curbyteoffset = curoffset;
            return 1;
        }
    } else {  /* forwards */

        while ( (offset = get_next_page(oy,&og,-1)) > 0) {
            if (lastgranule != -1) {
               if (avgpagelen < 0)
                   avgpagelen = (ogg_page_granulepos(&og) - lastgranule);
               else
                   avgpagelen = ((ogg_page_granulepos(&og) - lastgranule)
                                 + avgpagelen) / 2;
            }

            lastgranule = ogg_page_granulepos(&og);

            if ( ((lastgranule - (avgpagelen/4)) < pos && ( lastgranule +
                  avgpagelen + (avgpagelen / 4)) > pos) ||
                 lastgranule > pos) {

                /*wanted offset found Yeay!*/

                *curbyteoffset = offset;

                return offset;
            }
        }
    }

    ci->seek_buffer(*curbyteoffset);

    ogg_sync_reset(oy);

    LOGF("Seek failed:%lld\n",  (long long int)offset);

    return -1;
}


/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    (void)reason; /* CODEC_LOAD, CODEC_UNLOAD */
    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    int error = CODEC_ERROR;
    long action;
    intptr_t param;
    ogg_sync_state *oy;
    ogg_stream_state *os;
    ogg_page og;
    ogg_packet op;
    int64_t page_granule = 0;
    int stream_init = -1;
    int sample_rate = 48000;
    OpusDecoder *st = NULL;
    OpusHeader *header;
    int ret;
    unsigned long strtoffset;
    int skip = 0;
    int64_t seek_target;
    uint64_t granule_pos;

    ogg_malloc_init();

    action = CODEC_ACTION_NULL;
    param = ci->id3->elapsed;
    strtoffset = ci->id3->offset;

#if defined(CPU_COLDFIRE)
    /* EMAC rounding is disabled because of MULT16_32_Q15, which will be
       inaccurate with rounding in its current incarnation */
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
#endif

    /* Allocate some stuff off the heap */
    header = _ogg_malloc(sizeof(*header));
    os = _ogg_malloc(sizeof(*os));
    oy = _ogg_malloc(sizeof(*oy));

    /* pre-init the ogg_sync_state buffer, so it won't need many reallocs */
    ogg_sync_init(oy);
    oy->storage = 64*1024;
    oy->data = _ogg_malloc(oy->storage);

    /* allocate output buffer */
    uint16_t *output = (uint16_t*) _ogg_malloc(MAX_FRAME_SIZE*sizeof(uint16_t));

    ci->seek_buffer(0);
    ci->set_elapsed(0);

    goto next_page; /* need to decode header before we do anything else */

    while (1) {
        action = ci->get_command(&param);

    process_action:
        if (action == CODEC_ACTION_SEEK_TIME) {
            if (st != NULL) {
                /* calculate granule to seek to (including seek rewind) */
                seek_target = (48LL * param) + header->preskip;
                skip = MIN(seek_target, SEEK_REWIND);
                seek_target -= skip;

                LOGF("Opus seek page:%lld,%lld,%ld\n", (long long int)seek_target,
                     (long long int)page_granule, (long)param);
                opus_seek_page_granule(seek_target, page_granule, oy, os);
                /* reset the state to help ensure that subsequent packets won't
                   use state set by unrelated packets processed before seek */
                opus_decoder_ctl(st, OPUS_RESET_STATE);
            }

            ci->set_elapsed(param);
            ci->seek_complete();
            param = 0;
        } else  if (action == CODEC_ACTION_HALT)
            break;

    next_page:
        /*Get the ogg buffer for writing*/
        if (get_more_data(oy) < 1) {
            goto done;
        }

        /* Loop for all complete pages we got (most likely only one) */
        while (ogg_sync_pageout(oy, &og) == 1) {
            if (stream_init != 0) {
                stream_init = ogg_stream_init(os, ogg_page_serialno(&og));
                if (stream_init != 0) {
                    LOGF("Stream init failed");
                    goto done;
                }
            }

            /* Add page to the bitstream */
            ogg_stream_pagein(os, &og);

            page_granule = ogg_page_granulepos(&og);
            granule_pos = page_granule;

            while ((ogg_stream_packetout(os, &op) == 1) && !op.e_o_s) {
                if (op.packetno == 0){
                    /* identification header */

                    if (opus_header_parse(op.packet, op.bytes, header) == 0) {
                        LOGF("Could not parse header");
                        goto done;
                    }
                    skip = header->preskip;

                    st = opus_decoder_create(sample_rate, header->channels, &ret);
                    if (ret != OPUS_OK) {
                        LOGF("opus_decoder_create failed %d", ret);
                        goto done;
                    }
                    LOGF("Decoder inited");

                    codec_set_replaygain(ci->id3);

                    opus_decoder_ctl(st, OPUS_SET_GAIN(header->gain));

                    ci->configure(DSP_SET_FREQUENCY, sample_rate);
                    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
                    ci->configure(DSP_SET_STEREO_MODE, (header->channels == 2) ?
                        STEREO_INTERLEAVED : STEREO_MONO);

                    if (strtoffset)
                        seek_ogg_page(strtoffset);
                    else if (param) {
                        action = CODEC_ACTION_SEEK_TIME;
                        goto process_action;
                    } else {
                    /* seek buffer directly to the first audio packet to avoid
                       allocating space for huge comment packets
                       (embedded Album Art) */
                        if (seek_opus_tags())
                            seek_ogg_page(ci->curpos);
                        else {
                            LOGF("Could not find comment header");
                            goto done;
                        }
                    }
                    LOGF("sync reset");
                    ogg_sync_reset(oy); /* next page */
                    break;
                } else {
                    /* report progress */
                    ci->set_offset((size_t) ci->curpos);
                    ci->set_elapsed((granule_pos - header->preskip) / 48);

                    /* Decode audio packets */
                    ret = opus_decode(st, op.packet, op.bytes, output, MAX_FRAME_SIZE, 0);

                    if (ret > skip) {
                        /* part of or entire output buffer is played */
                        ret -= skip;
                        ci->pcmbuf_insert(&output[skip * header->channels], NULL, ret);
                        skip = 0;
                    } else {
                        if (ret < 0) {
                            LOGF("opus_decode failed %d", ret);
                            goto done;
                        } else if (ret == 0)
                            break;
                        else {
                            /* entire output buffer is skipped */
                            skip -= ret;
                            ret = 0;
                        }
                    }
                }
            }
        }
    }
    LOGF("Returned OK");
    error = CODEC_OK;
done:
    ogg_malloc_destroy();
    return error;
}
