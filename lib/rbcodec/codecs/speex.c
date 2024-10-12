/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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
 ***************************************************************************/

#include "libspeex/speex/ogg.h"
#include "libspeex/speex/speex.h"
#include "libspeex/speex/speex_callbacks.h"
#include "libspeex/speex/speex_header.h"
#include "libspeex/speex/speex_stereo.h"
#include "libspeex/speex/speex_config_types.h"
#include "codeclib.h"

/* Room for one stereo frame of max size, 2*640 */
#define MAX_FRAME_SIZE 1280
#define CHUNKSIZE 10000  /*2kb*/
#define SEEK_CHUNKSIZE 7*CHUNKSIZE

CODEC_HEADER

static spx_int16_t output[MAX_FRAME_SIZE] IBSS_ATTR;

static int get_more_data(spx_ogg_sync_state *oy)
{
    int bytes;
    char *buffer;

    buffer = (char *)spx_ogg_sync_buffer(oy,CHUNKSIZE);

    bytes = ci->read_filebuf(buffer, sizeof(char)*CHUNKSIZE);

    spx_ogg_sync_wrote(oy,bytes);

    return bytes;
}

/* The read/seek functions track absolute position within the stream */

static spx_int64_t get_next_page(spx_ogg_sync_state *oy,spx_ogg_page *og,
                                 spx_int64_t boundary)
{
    spx_int64_t localoffset = ci->curpos;
    long more;
    long ret;

    if (boundary > 0)
        boundary += ci->curpos;

    while (1) {
        more = spx_ogg_sync_pageseek(oy,og);

        if (more < 0) {
            /* skipped n bytes */
            localoffset-=more;
        } else {
            if (more == 0) {
                /* send more paramedics */
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

                spx_int64_t ret=localoffset;

                return(ret);
            }
        }
    }
}

static spx_int64_t seek_backwards(spx_ogg_sync_state *oy, spx_ogg_page *og,
                                  spx_int64_t wantedpos)
{
    spx_int64_t crofs;
    spx_int64_t *curoffset=&crofs;
    *curoffset=ci->curpos;
    spx_int64_t begin=*curoffset;
    spx_int64_t end=begin;
    spx_int64_t ret;
    spx_int64_t offset=-1;
    spx_int64_t avgpagelen=-1;
    spx_int64_t lastgranule=-1;

    short time = -1;

    while (offset == -1) {

        begin -= SEEK_CHUNKSIZE;

        if (begin < 0) {
            if (time < 0) {
                begin = 0;
                time++;
            } else {
                LOGF("Can't seek that early:%lld\n", (long long int)begin);
                return -3;  /* too early */
            }
        }

        *curoffset = begin;

        ci->seek_buffer(*curoffset);

        spx_ogg_sync_reset(oy);

        lastgranule = -1;

        while (*curoffset < end) {
            ret = get_next_page(oy,og,end-*curoffset);

            if (ret > 0) {
                if (lastgranule != -1) {
                    if (avgpagelen < 0)
                        avgpagelen = (spx_ogg_page_granulepos(og)-lastgranule);
                    else
                       avgpagelen=((spx_ogg_page_granulepos(og)-lastgranule)
                                   + avgpagelen) / 2;
                }

                lastgranule=spx_ogg_page_granulepos(og);

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
                } else{ /*if (spx_ogg_page_granulepos(&og)<wantedpos)*/
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

static int speex_seek_page_granule(spx_int64_t pos, spx_int64_t curpos,
                                   spx_ogg_sync_state *oy,
                                   spx_int64_t headerssize)
{
    /* TODO: Someone may want to try to implement seek to packet, 
             instead of just to page (should be more accurate, not be any 
             faster) */

    spx_int64_t crofs;
    spx_int64_t *curbyteoffset = &crofs;
    *curbyteoffset = ci->curpos;
    spx_int64_t curoffset;
    curoffset = *curbyteoffset;
    spx_int64_t offset = 0;
    spx_ogg_page og = {0,0,0,0};
    spx_int64_t avgpagelen = -1;
    spx_int64_t lastgranule = -1;

    if(llabs(pos-curpos)>10000 && headerssize>0 && curoffset-headerssize>10000) {
        /* if seeking for more that 10sec,
           headersize is known & more than 10kb is played,
           try to guess a place to seek from the number of
           bytes playe for this position, this works best when
           the bitrate is relativly constant.
         */

        curoffset = (((*curbyteoffset-headerssize) * pos)/curpos)*98/100;
        if (curoffset < 0)
            curoffset=0;

        //spx_int64_t toffset=curoffset;

        ci->seek_buffer(curoffset);

        spx_ogg_sync_reset(oy);

        offset = get_next_page(oy,&og,-1);

        if (offset < 0) { /* could not find new page,use old offset */
            LOGF("Seek/guess/fault:%lld->-<-%d,%lld:%lld,%d,%ld,%d\n",
                  (long long int)curpos,0, (long long int)pos,
                  (long long int)offset,0,ci->curpos,/*stream_length*/0);

            curoffset = *curbyteoffset;

            ci->seek_buffer(curoffset);

            spx_ogg_sync_reset(oy);
        } else {
            if (spx_ogg_page_granulepos(&og) == 0 && pos > 5000) {
                LOGF("SEEK/guess/fault:%lld->-<-%lld,%lld:%lld,%d,%ld,%d\n",
                     (long long int)curpos,(long long int)spx_ogg_page_granulepos(&og),
                     (long long int)pos, (long long int)offset,0,ci->curpos,/*stream_length*/0);

                curoffset = *curbyteoffset;

                ci->seek_buffer(curoffset);

                spx_ogg_sync_reset(oy);
            } else {
                curoffset = offset;
                curpos = spx_ogg_page_granulepos(&og);
            }
        }
    }

    /* which way do we want to seek? */

    if (curpos > pos) {  /* backwards */
        offset = seek_backwards(oy,&og,pos);

        if (offset > 0) {
            *curbyteoffset = curoffset;
            return 1;
        }
    } else {  /* forwards */

        while ( (offset = get_next_page(oy,&og,-1)) > 0) {
            if (lastgranule != -1) {
               if (avgpagelen < 0)
                   avgpagelen = (spx_ogg_page_granulepos(&og) - lastgranule);
               else
                   avgpagelen = ((spx_ogg_page_granulepos(&og) - lastgranule)
                                 + avgpagelen) / 2;
            }

            lastgranule = spx_ogg_page_granulepos(&og);

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

    spx_ogg_sync_reset(oy);

    LOGF("Seek failed:%lld\n", (long long int)offset);

    return -1;
}

static void *process_header(spx_ogg_packet *op,
                            int enh_enabled,
                            int *frame_size,
                            int *rate,
                            int *nframes,
                            int *channels,
                            SpeexStereoState *stereo,
                            int *extra_headers
                           )
{
    void *st;
    const SpeexMode *mode;
    SpeexHeader *header;
    int modeID;
    SpeexCallback callback;

    header = speex_packet_to_header((char*)op->packet, op->bytes);

    if (!header){
        DEBUGF ("Cannot read header\n");
        return NULL;
    }

    if (header->mode >= SPEEX_NB_MODES){
        DEBUGF ("Mode does not exist\n");
        return NULL;
    }

    modeID = header->mode;

    mode = speex_lib_get_mode(modeID);

    if (header->speex_version_id > 1) {
        DEBUGF("Undecodeable bitstream");
        return NULL;
    }

    if (mode->bitstream_version < header->mode_bitstream_version){
        DEBUGF("Undecodeable bitstream, newer bitstream");
        return NULL;
    }

    if (mode->bitstream_version > header->mode_bitstream_version){
        DEBUGF("Too old bitstream");
        return NULL;
    }
   
    st = speex_decoder_init(mode);
    if (!st){
        DEBUGF("Decoder init failed");
        return NULL;
    }
    speex_decoder_ctl(st, SPEEX_SET_ENH, &enh_enabled);
    speex_decoder_ctl(st, SPEEX_GET_FRAME_SIZE, frame_size);

    if (header->nb_channels!=1){
        callback.callback_id = SPEEX_INBAND_STEREO;
        callback.func = speex_std_stereo_request_handler;
        callback.data = stereo;
        speex_decoder_ctl(st, SPEEX_SET_HANDLER, &callback);
    }
    *channels = header->nb_channels;

    if (!*rate)
        *rate = header->rate;

    speex_decoder_ctl(st, SPEEX_SET_SAMPLING_RATE, rate);

    *nframes = header->frames_per_packet;

    *extra_headers = header->extra_headers;

    return st;
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    /* Nothing to do */
    return CODEC_OK;
    (void)reason;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    int error = CODEC_ERROR;

    SpeexBits bits;
    spx_ogg_sync_state oy;
    spx_ogg_page og;
    spx_ogg_packet op;
    spx_ogg_stream_state os;
    spx_int64_t page_granule = 0;
    spx_int64_t cur_granule = 0;
    int enh_enabled = 1;
    int nframes = 2;
    int eos = 0;
    SpeexStereoState *stereo;
    int channels = -1;
    int samplerate = ci->id3->frequency;
    int extra_headers = 0;
    int stream_init = 0;
    /* rockbox: comment 'set but unused' variables
    int page_nb_packets;
    */
    int frame_size;
    int packet_count = 0;
    int lookahead;
    int headerssize = 0;
    unsigned long strtoffset;
    void *st = NULL;
    int j = 0;
    long action;
    intptr_t param;

    memset(&bits, 0, sizeof(bits));
    memset(&oy, 0, sizeof(oy));

    /* Ogg handling still uses mallocs, so reset the malloc buffer per track */
    if (codec_init()) {
        goto exit;
    }

    action = CODEC_ACTION_NULL;
    param = ci->id3->elapsed;
    strtoffset = ci->id3->offset;

    ci->seek_buffer(0);
    ci->set_elapsed(0);

    stereo = speex_stereo_state_init();
    spx_ogg_sync_init(&oy);
    spx_ogg_alloc_buffer(&oy,2*CHUNKSIZE);

    codec_set_replaygain(ci->id3);

    if (!strtoffset && param) {
        action = CODEC_ACTION_SEEK_TIME;
    }

    goto next_page;

    while (1) {
        if (action == CODEC_ACTION_NULL)
            action = ci->get_command(&param);

        if (action != CODEC_ACTION_NULL) {
            if (action == CODEC_ACTION_HALT)
                break;

            /*seek (seeks to the page before the position) */
            if (action == CODEC_ACTION_SEEK_TIME) {
                if(samplerate!=0&&packet_count>1){
                    LOGF("Speex seek page:%lld,%lld,%ld,%lld,%d\n",
                         (long long int)((spx_int64_t)param/1000) *
                         (spx_int64_t)samplerate,
                         (long long int)page_granule, (long)param,
                         (long long int)((page_granule/samplerate)*1000), samplerate);

                    speex_seek_page_granule(((spx_int64_t)param/1000) *
                                            (spx_int64_t)samplerate,
                                            page_granule, &oy, headerssize);
                }

                ci->set_elapsed(param);
                ci->seek_complete();
            }

            action = CODEC_ACTION_NULL;
        }

next_page:
        /*Get the ogg buffer for writing*/
        if(get_more_data(&oy)<1){/*read error*/
            goto done;
        }

        /* Loop for all complete pages we got (most likely only one) */
        while (spx_ogg_sync_pageout(&oy, &og) == 1) {
            int packet_no;
            if (stream_init == 0) {
                spx_ogg_stream_init(&os, spx_ogg_page_serialno(&og));
                stream_init = 1;
            }

            /* Add page to the bitstream */
            spx_ogg_stream_pagein(&os, &og);

            page_granule = spx_ogg_page_granulepos(&og);
            /* page_nb_packets = spx_ogg_page_packets(&og); */

            cur_granule = page_granule;

            /* Extract all available packets */
            packet_no=0;

            while (!eos && spx_ogg_stream_packetout(&os, &op)==1){
                /* If first packet, process as Speex header */
                if (packet_count==0){
                    st = process_header(&op, enh_enabled, &frame_size,
                                         &samplerate, &nframes, &channels,
                                         stereo, &extra_headers);

                    speex_decoder_ctl(st, SPEEX_GET_LOOKAHEAD, &lookahead);
                    if (!nframes)
                        nframes=1;

                    if (!st){
                        goto done;
                    }

                    ci->configure(DSP_SET_FREQUENCY, ci->id3->frequency);
                    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
                    if (channels == 2) {
                        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
                    } else if (channels == 1) {
                        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
                    }

                    /* Speex header in its own page, add the whole page
                       headersize */
                    headerssize += og.header_len+og.body_len;

                } else if (packet_count<=1+extra_headers){
                    /* add packet to headersize */
                    headerssize += op.bytes;

                    /* Ignore extra headers */
                } else {
                    if (packet_count <= 2+extra_headers) {
                        if (strtoffset) {
                            ci->seek_buffer(strtoffset);
                            spx_ogg_sync_reset(&oy);
                            packet_count++;
                            goto next_page;
                        }
                    }
                    packet_no++;

                    if (op.e_o_s) /* End of stream condition */
                        eos=1;

                    /* Set Speex bitstream to point to Ogg packet */
                    speex_bits_set_bit_buffer(&bits, (char *)op.packet,
                                                     op.bytes);
                    for (j = 0; j != nframes; j++){
                        int ret;

                        /* Decode frame */
                        ret = speex_decode_int(st, &bits, output);

                        if (ret == -1)
                            break;

                        if (ret == -2)
                            break;

                        if (speex_bits_remaining(&bits) < 0)
                            break;

                        if (channels == 2)
                            speex_decode_stereo_int(output, frame_size, stereo);

                        if (frame_size > 0) {
                            spx_int16_t *frame_start = output + lookahead;

                            if (channels == 2)
                                frame_start += lookahead;
                            ci->pcmbuf_insert(frame_start, NULL,
                                              frame_size - lookahead);
                            lookahead = 0;
                            /* 2 bytes/sample */
                            cur_granule += frame_size / 2;

                            ci->set_offset((long) ci->curpos);

                            ci->set_elapsed((samplerate == 0) ? 0 :
                                             cur_granule * 1000LL / samplerate);
                         }
                    }
                }
                packet_count++;
            }
        }
    }

    error = CODEC_OK;
done:
    /* Clean things up for the next track */
    speex_bits_destroy(&bits);

    if (st)
        speex_decoder_destroy(st);

    if (stream_init)
       spx_ogg_stream_destroy(&os);

    spx_ogg_sync_destroy(&oy);

exit:
    return error;
}
