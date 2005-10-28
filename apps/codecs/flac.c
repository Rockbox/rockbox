/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include <codecs/libffmpegFLAC/decoder.h>

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

struct codec_api* rb;
struct codec_api* ci;

/* The output buffers containing the decoded samples (channels 0 and 1) */
int32_t decoded0[MAX_BLOCKSIZE] IBSS_ATTR;
int32_t decoded1[MAX_BLOCKSIZE] IBSS_ATTR;

static bool flac_init(FLACContext* fc)
{
    unsigned char buf[255];
    bool found_streaminfo=false;
    int endofmetadata=0;
    int blocklength;

    ci->memset(fc,0,sizeof(FLACContext));

    if (ci->read_filebuf(buf, 4) < 4)
    {
        return false;
    }

    if (ci->memcmp(buf,"fLaC",4) != 0) 
    {
        return false;
    }
    fc->metadatalength = 4;

    while (!endofmetadata) {
        if (ci->read_filebuf(buf, 4) < 4)
        {
            return false;
        }

        endofmetadata=(buf[0]&0x80);
        blocklength = (buf[1] << 16) | (buf[2] << 8) | buf[3];
        fc->metadatalength+=blocklength+4;

        if ((buf[0] & 0x7f) == 0)       /* 0 is the STREAMINFO block */
        {
            /* FIXME: Don't trust the value of blocklength */
            if (ci->read_filebuf(buf, blocklength) < 0)
            {
                return false;
            }
          
            fc->filesize = ci->filesize;
            fc->min_blocksize = (buf[0] << 8) | buf[1];
            fc->max_blocksize = (buf[2] << 8) | buf[3];
            fc->min_framesize = (buf[4] << 16) | (buf[5] << 8) | buf[6];
            fc->max_framesize = (buf[7] << 16) | (buf[8] << 8) | buf[9];
            fc->samplerate = (buf[10] << 12) | (buf[11] << 4) 
                             | ((buf[12] & 0xf0) >> 4);
            fc->channels = ((buf[12]&0x0e)>>1) + 1;
            fc->bps = (((buf[12]&0x01) << 4) | ((buf[13]&0xf0)>>4) ) + 1;

            /* totalsamples is a 36-bit field, but we assume <= 32 bits are 
               used */
            fc->totalsamples = (buf[14] << 24) | (buf[15] << 16) 
                               | (buf[16] << 8) | buf[17];

            /* Calculate track length (in ms) and estimate the bitrate 
               (in kbit/s) */
            fc->length = (fc->totalsamples / fc->samplerate) * 1000;

            found_streaminfo=true;
        } else if ((buf[0] & 0x7f) == 3) { /* 3 is the SEEKTABLE block */
          ci->advance_buffer(blocklength);
        } else {
          /* Skip to next metadata block */
          ci->advance_buffer(blocklength);
        }
    }

   if (found_streaminfo) {
       fc->bitrate = ((fc->filesize-fc->metadatalength) * 8) / fc->length;
       return true;
   } else {
       return false;
   }
}

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    size_t n;
    static int8_t buf[MAX_FRAMESIZE];
    FLACContext fc;
    uint32_t samplesdone;
    uint32_t elapsedtime;
    int bytesleft;
    int consumed;
    int res;
    int frame;

    /* Generic codec initialisation */
    TEST_CODEC_API(api);

    rb = api;
    ci = (struct codec_api*)api;

#ifndef SIMULATOR
    ci->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

    ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*10));
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*128));

    ci->configure(CODEC_DSP_ENABLE, (bool *)true);
    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_STEREO_MODE, (long *)STEREO_NONINTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(28));
    
    next_track:

    if (!flac_init(&fc)) {
        LOGF("FLAC: Error initialising codec\n");
        return CODEC_ERROR;
    }

    while (!ci->taginfo_ready)
        ci->yield();
    
    ci->configure(DSP_SET_FREQUENCY, (long *)(ci->id3->frequency));

    /* The main decoding loop */
    samplesdone=0;
    frame=0;
    bytesleft=ci->read_filebuf(buf,sizeof(buf));
    while (bytesleft) {
        ci->yield();
        if (ci->stop_codec || ci->reload_codec) {
            break;
        }

        /* Deal with any pending seek requests */
        if (ci->seek_time) {
          /* We only support seeking to start of track at the moment */
          if (ci->seek_time==1) {
              if (ci->seek_buffer(fc.metadatalength)) {
                  /* Refill the input buffer */
                  bytesleft=ci->read_filebuf(buf,sizeof(buf));
                  ci->set_elapsed(0);
              }
          }
          ci->seek_time = 0;
        }

        if((res=flac_decode_frame(&fc,decoded0,decoded1,buf,
                             bytesleft,ci->yield)) < 0) {
             LOGF("FLAC: Frame %d, error %d\n",frame,res);
             return CODEC_ERROR;
        }
        consumed=fc.gb.index/8;
        frame++;

        ci->yield();
        while(!ci->pcmbuf_insert_split((char*)decoded0,(char*)decoded1,
              fc.blocksize*4)) {
            ci->yield();
        }

        /* Update the elapsed-time indicator */
        samplesdone=fc.samplenumber+fc.blocksize;
        elapsedtime=(samplesdone*10)/(ci->id3->frequency/100);
        ci->set_elapsed(elapsedtime);

        memmove(buf,&buf[consumed],bytesleft-consumed);
        bytesleft-=consumed;

        n=ci->read_filebuf(&buf[bytesleft],sizeof(buf)-bytesleft);
        if (n > 0) {
            bytesleft+=n;
        }
    }
    LOGF("FLAC: Decoded %d samples\n",samplesdone);

    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;
}
