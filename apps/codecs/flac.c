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

CODEC_HEADER

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

struct codec_api* rb;
struct codec_api* ci;

/* The output buffers containing the decoded samples (channels 0 and 1) */
int32_t decoded0[MAX_BLOCKSIZE] IBSS_ATTR;
int32_t decoded1[MAX_BLOCKSIZE] IBSS_ATTR;

#define MAX_SUPPORTED_SEEKTABLE_SIZE 5000

/* Notes about seeking:

   The full seek table consists of:
      uint64_t sample (only 36 bits are used)
      uint64_t offset
      uint32_t blocksize

   We don't store the blocksize (of the target frame) and we also
   limit the sample and offset values to 32-bits - Rockbox doesn't support
   files bigger than 2GB on FAT32 filesystems.

   The reference FLAC encoder produces a seek table with points every
   10 seconds, but this can be overridden by the user when encoding a file.

   With the default settings, a typical 4 minute track will contain
   24 seek points.

   Taking the extreme case of a Rockbox supported file to be a 2GB (compressed)
   16-bit/44.1KHz mono stream with a likely uncompressed size of 4GB:
      Total duration is: 48694 seconds (about 810 minutes - 13.5 hours)
      Total number of seek points: 4869

   Therefore we limit the number of seek points to 5000.  This is a
   very extreme case, and requires 5000*8=40000 bytes of storage.

   If we come across a FLAC file with more than this number of seekpoints, we
   just use the first 5000.

*/

struct FLACseekpoints {
    uint32_t sample;
    uint32_t offset;
};

struct FLACseekpoints seekpoints[MAX_SUPPORTED_SEEKTABLE_SIZE];
int nseekpoints;

static bool flac_init(FLACContext* fc, int first_frame_offset)
{
    unsigned char buf[255];
    bool found_streaminfo=false;
    uint32_t seekpoint_hi,seekpoint_lo;
    uint32_t offset_hi,offset_lo;
    int endofmetadata=0;
    int blocklength;
    int n;

    ci->memset(fc,0,sizeof(FLACContext));
    nseekpoints=0;

    /* Skip any foreign tags at start of file */
    ci->seek_buffer(first_frame_offset);

    fc->metadatalength = first_frame_offset;

    if (ci->read_filebuf(buf, 4) < 4)
    {
        return false;
    }

    if (ci->memcmp(buf,"fLaC",4) != 0) 
    {
        return false;
    }
    fc->metadatalength += 4;

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
            /* FIXME: Don't trust the value of blocklength, use actual return
             * value in bytes instead */
            ci->read_filebuf(buf, blocklength);
          
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
            while ((nseekpoints < MAX_SUPPORTED_SEEKTABLE_SIZE) && 
                   (blocklength >= 18)) {
                n=ci->read_filebuf(buf,18);
                if (n < 18) return false;
                blocklength-=n;

                seekpoint_hi=(buf[0] << 24) | (buf[1] << 16) | 
                             (buf[2] << 8) | buf[3];
                seekpoint_lo=(buf[4] << 24) | (buf[5] << 16) | 
                             (buf[6] << 8) | buf[7];
                offset_hi=(buf[8] << 24) | (buf[9] << 16) | 
                             (buf[10] << 8) | buf[11];
                offset_lo=(buf[12] << 24) | (buf[13] << 16) | 
                             (buf[14] << 8) | buf[15];

                /* The final two bytes contain the number of samples in the 
                   target frame - but we don't care about that. */

                /* Only store seekpoints where the high 32 bits are zero */
                if ((seekpoint_hi == 0) && (seekpoint_lo != 0xffffffff) &&
                    (offset_hi == 0)) {
                        seekpoints[nseekpoints].sample=seekpoint_lo;
                        seekpoints[nseekpoints].offset=offset_lo;
                        nseekpoints++;
                }
            }
            /* Skip any unread seekpoints */
            if (blocklength > 0)
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

/* A very simple seek implementation - seek to the seekpoint before
   the target sample.

   This needs to be improved to seek with greater accuracy
*/
bool flac_seek(FLACContext* fc, uint32_t newsample) {
  uint32_t offset;
  int i;

  if (nseekpoints==0) {
      offset=0;
  } else {
      i=nseekpoints-1;
      while ((i > 0) && (seekpoints[i].sample > newsample)) {
          i--;
      }

      if ((i==0) && (seekpoints[i].sample > newsample)) {
          offset=0;
      } else {
          offset=seekpoints[i].offset;
      }
  }

  offset+=fc->metadatalength;

  if (ci->seek_buffer(offset)) {
      return true;
  } else {
      return false;
  }
}

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    int8_t *buf;
    FLACContext fc;
    uint32_t samplesdone;
    uint32_t elapsedtime;
    long bytesleft;
    int consumed;
    int res;
    int frame;
    int retval;

    /* Generic codec initialisation */
    rb = api;
    ci = api;

#ifdef USE_IRAM
    ci->memcpy(iramstart, iramcopy, iramend-iramstart);
    ci->memset(iedata, 0, iend - iedata);
#endif

    ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*128));

    ci->configure(CODEC_DSP_ENABLE, (bool *)true);
    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_STEREO_MODE, (long *)STEREO_NONINTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(FLAC_OUTPUT_DEPTH-1));

    next_track:

    if (codec_init(api)) {
        LOGF("FLAC: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    if (!flac_init(&fc,ci->id3->first_frame_offset)) {
        LOGF("FLAC: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready)
        ci->yield();
    
    ci->configure(DSP_SET_FREQUENCY, (long *)(ci->id3->frequency));
    codec_set_replaygain(ci->id3);

    /* The main decoding loop */
    samplesdone=0;
    frame=0;
    buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
    while (bytesleft) {
        ci->yield();
        if (ci->stop_codec || ci->reload_codec) {
            break;
        }

        /* Deal with any pending seek requests */
        if (ci->seek_time) {
            if (flac_seek(&fc,((ci->seek_time-1)/20)*(ci->id3->frequency/50))) {
                /* Refill the input buffer */
                buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
            }
            ci->seek_complete();
        }

        if((res=flac_decode_frame(&fc,decoded0,decoded1,buf,
                             bytesleft,ci->yield)) < 0) {
             LOGF("FLAC: Frame %d, error %d\n",frame,res);
             retval = CODEC_ERROR;
             goto exit;
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

        ci->advance_buffer(consumed);

        buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
    }
    LOGF("FLAC: Decoded %d samples\n",samplesdone);

    if (ci->request_next_track())
        goto next_track;

    retval = CODEC_OK;
exit:
    return retval;
}
