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

/* The output buffers containing the decoded samples (channels 0 and 1) */
int32_t decoded0[MAX_BLOCKSIZE] IBSS_ATTR_FLAC_DECODED0;
int32_t decoded1[MAX_BLOCKSIZE] IBSS_ATTR;

#define MAX_SUPPORTED_SEEKTABLE_SIZE 5000

/* Notes about seeking:

   The full seek table consists of:
      uint64_t sample (only 36 bits are used)
      uint64_t offset
      uint32_t blocksize

   We also limit the sample and offset values to 32-bits - Rockbox doesn't
   support files bigger than 2GB on FAT32 filesystems.

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
    uint16_t blocksize;
};

struct FLACseekpoints seekpoints[MAX_SUPPORTED_SEEKTABLE_SIZE];
int nseekpoints;

static int8_t *bit_buffer;
static size_t buff_size;

static bool flac_init(FLACContext* fc, int first_frame_offset)
{
    unsigned char buf[255];
    bool found_streaminfo=false;
    uint32_t seekpoint_hi,seekpoint_lo;
    uint32_t offset_hi,offset_lo;
    uint16_t blocksize;
    int endofmetadata=0;
    uint32_t blocklength;

    ci->memset(fc,0,sizeof(FLACContext));
    nseekpoints=0;

    fc->sample_skip = 0;

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
            if (ci->read_filebuf(buf, blocklength) < blocklength) return false;
          
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
                if (ci->read_filebuf(buf,18) < 18) return false;
                blocklength-=18;

                seekpoint_hi=(buf[0] << 24) | (buf[1] << 16) | 
                             (buf[2] << 8) | buf[3];
                seekpoint_lo=(buf[4] << 24) | (buf[5] << 16) | 
                             (buf[6] << 8) | buf[7];
                offset_hi=(buf[8] << 24) | (buf[9] << 16) | 
                             (buf[10] << 8) | buf[11];
                offset_lo=(buf[12] << 24) | (buf[13] << 16) | 
                             (buf[14] << 8) | buf[15];

                blocksize=(buf[16] << 8) | buf[17];

                /* Only store seekpoints where the high 32 bits are zero */
                if ((seekpoint_hi == 0) && (seekpoint_lo != 0xffffffff) &&
                    (offset_hi == 0)) {
                        seekpoints[nseekpoints].sample=seekpoint_lo;
                        seekpoints[nseekpoints].offset=offset_lo;
                        seekpoints[nseekpoints].blocksize=blocksize;
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

/* Synchronize to next frame in stream - adapted from libFLAC 1.1.3b2 */
bool frame_sync(FLACContext* fc) {
    unsigned int x = 0;
    bool cached = false;

    /* Make sure we're byte aligned. */
    align_get_bits(&fc->gb);

    while(1) {
        if(fc->gb.size_in_bits - get_bits_count(&fc->gb) < 8) {
            /* Error, end of bitstream, a valid stream should never reach here
             * since the buffer should contain at least one frame header.
             */
            return false;
        }

        if(cached)
            cached = false;
        else
            x = get_bits(&fc->gb, 8);

        if(x == 0xff) { /* MAGIC NUMBER for first 8 frame sync bits. */
            x = get_bits(&fc->gb, 8);
            /* We have to check if we just read two 0xff's in a row; the second
             * may actually be the beginning of the sync code.
             */
            if(x == 0xff) { /* MAGIC NUMBER for first 8 frame sync bits. */
                cached = true;
            }
            else if(x >> 2 == 0x3e) { /* MAGIC NUMBER for last 6 sync bits. */
                /* Succesfully synced. */
                break;
            }
        }
    }

    /* Advance and init bit buffer to the new frame. */
    ci->advance_buffer((get_bits_count(&fc->gb)-16)>>3); /* consumed bytes */
    bit_buffer = ci->request_buffer(&buff_size, MAX_FRAMESIZE+16);
    init_get_bits(&fc->gb, bit_buffer, buff_size*8);

    /* Decode the frame to verify the frame crc and
     * fill fc with its metadata.
     */
    if(flac_decode_frame(fc, decoded0, decoded1,
       bit_buffer, buff_size, ci->yield) < 0) {
        return false;
    }

    return true;
}

/* Seek to sample - adapted from libFLAC 1.1.3b2+ */
bool flac_seek(FLACContext* fc, uint32_t target_sample) {
    off_t orig_pos = ci->curpos;
    off_t pos = -1;
    unsigned long lower_bound, upper_bound;
    unsigned long lower_bound_sample, upper_bound_sample;
    int i;
    unsigned approx_bytes_per_frame;
    uint32_t this_frame_sample = fc->samplenumber;
    unsigned this_block_size = fc->blocksize;
    bool needs_seek = true, first_seek = true;

    /* We are just guessing here. */
    if(fc->max_framesize > 0)
        approx_bytes_per_frame = (fc->max_framesize + fc->min_framesize)/2 + 1;
    /* Check if it's a known fixed-blocksize stream. */
    else if(fc->min_blocksize == fc->max_blocksize && fc->min_blocksize > 0)
        approx_bytes_per_frame = fc->min_blocksize*fc->channels*fc->bps/8 + 64;
    else
        approx_bytes_per_frame = 4608 * fc->channels * fc->bps/8 + 64;

    /* Set an upper and lower bound on where in the stream we will search. */
    lower_bound = fc->metadatalength;
    lower_bound_sample = 0;
    upper_bound = fc->filesize;
    upper_bound_sample = fc->totalsamples>0 ? fc->totalsamples : target_sample;

    /* Refine the bounds if we have a seektable with suitable points. */
    if(nseekpoints > 0) {
        /* Find the closest seek point <= target_sample, if it exists. */
        for(i = nseekpoints-1; i >= 0; i--) {
            if(seekpoints[i].sample <= target_sample)
                break;
        }
        if(i >= 0) { /* i.e. we found a suitable seek point... */
            lower_bound = fc->metadatalength + seekpoints[i].offset;
            lower_bound_sample = seekpoints[i].sample;
        }

        /* Find the closest seek point > target_sample, if it exists. */
        for(i = 0; i < nseekpoints; i++) {
            if(seekpoints[i].sample > target_sample)
                break;
        }
        if(i < nseekpoints) { /* i.e. we found a suitable seek point... */
            upper_bound = fc->metadatalength + seekpoints[i].offset;
            upper_bound_sample = seekpoints[i].sample;
        }
    }

    while(1) {
        /* Check if bounds are still ok. */
        if(lower_bound_sample >= upper_bound_sample ||
           lower_bound > upper_bound) {
            return false;
        }

        /* Calculate new seek position */
        if(needs_seek) {
            pos = (off_t)(lower_bound +
              (((target_sample - lower_bound_sample) *
              (int64_t)(upper_bound - lower_bound)) /
              (upper_bound_sample - lower_bound_sample)) -
              approx_bytes_per_frame);
            
            if(pos >= (off_t)upper_bound)
                pos = (off_t)upper_bound-1;
            if(pos < (off_t)lower_bound)
                pos = (off_t)lower_bound;
        }

        if(!ci->seek_buffer(pos))
            return false;

        bit_buffer = ci->request_buffer(&buff_size, MAX_FRAMESIZE+16);
        init_get_bits(&fc->gb, bit_buffer, buff_size*8);

        /* Now we need to get a frame.  It is possible for our seek
         * to land in the middle of audio data that looks exactly like
         * a frame header from a future version of an encoder.  When
         * that happens, frame_sync() will return false.
         * But there is a remote possibility that it is properly
         * synced at such a "future-codec frame", so to make sure,
         * we wait to see several "unparseable" errors in a row before
         * bailing out.
         */
        {
            unsigned unparseable_count;
            bool got_a_frame = false;
            for(unparseable_count = 0; !got_a_frame
                && unparseable_count < 10; unparseable_count++) {
                if(frame_sync(fc))
                    got_a_frame = true;
            }
            if(!got_a_frame) {
                ci->seek_buffer(orig_pos);
                return false;
            }
        }

        this_frame_sample = fc->samplenumber;
        this_block_size = fc->blocksize;

        if(target_sample >= this_frame_sample
           && target_sample < this_frame_sample+this_block_size) {
            /* Found the frame containing the target sample. */
            fc->sample_skip = target_sample - this_frame_sample;
            break;
        }

        if(this_frame_sample + this_block_size >= upper_bound_sample &&
           !first_seek) {
            if(pos == (off_t)lower_bound || !needs_seek) {
                ci->seek_buffer(orig_pos);
                return false;
            }
            /* Our last move backwards wasn't big enough, try again. */
            approx_bytes_per_frame *= 2;
            continue;
        }
        /* Allow one seek over upper bound,
         * required for streams with unknown total samples.
         */
        first_seek = false;

        /* Make sure we are not seeking in a corrupted stream */
        if(this_frame_sample < lower_bound_sample) {
            ci->seek_buffer(orig_pos);
            return false;
        }

        approx_bytes_per_frame = this_block_size*fc->channels*fc->bps/8 + 64;

        /* We need to narrow the search. */
        if(target_sample < this_frame_sample) {
            upper_bound_sample = this_frame_sample;
            upper_bound = ci->curpos;
        }
        else { /* Target is beyond this frame. */
            /* We are close, continue in decoding next frames. */
            if(target_sample < this_frame_sample + 4*this_block_size) {
                pos = ci->curpos + fc->framesize;
                needs_seek = false;
            }

            lower_bound_sample = this_frame_sample + this_block_size;
            lower_bound = ci->curpos + fc->framesize;
        }
    }

    return true;
}

/* Seek to file offset */
bool flac_seek_offset(FLACContext* fc, uint32_t offset) {
    unsigned unparseable_count;
    bool got_a_frame = false;

    if(!ci->seek_buffer(offset))
        return false;

    bit_buffer = ci->request_buffer(&buff_size, MAX_FRAMESIZE);
    init_get_bits(&fc->gb, bit_buffer, buff_size*8);

    for(unparseable_count = 0; !got_a_frame
        && unparseable_count < 10; unparseable_count++) {
        if(frame_sync(fc))
            got_a_frame = true;
    }
    
    if(!got_a_frame) {
        ci->seek_buffer(fc->metadatalength);
        return false;
    }

    return true;
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    int8_t *buf;
    FLACContext fc;
    uint32_t samplesdone = 0;
    uint32_t elapsedtime;
    size_t bytesleft;
    int consumed;
    int res;
    int frame;
    int retval;

    /* Generic codec initialisation */
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);

    ci->configure(DSP_SET_SAMPLE_DEPTH, FLAC_OUTPUT_DEPTH-1);

    next_track:
        
    /* Need to save offset for later use (cleared indirectly by flac_init) */
    samplesdone=ci->id3->offset;

    if (codec_init()) {
        LOGF("FLAC: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto exit;
    }

    if (!flac_init(&fc,ci->id3->first_frame_offset)) {
        LOGF("FLAC: Error initialising codec\n");
        retval = CODEC_ERROR;
        goto done;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);
    
    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    ci->configure(DSP_SET_STEREO_MODE, fc.channels == 1 ?
                  STEREO_MONO : STEREO_NONINTERLEAVED);
    codec_set_replaygain(ci->id3);

    if (samplesdone) {
        flac_seek_offset(&fc, samplesdone);
        samplesdone=0;
    }

    /* The main decoding loop */
    frame=0;
    buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
    while (bytesleft) {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }

        /* Deal with any pending seek requests */
        if (ci->seek_time) {
            if (flac_seek(&fc,(uint32_t)(((uint64_t)(ci->seek_time-1)
                *ci->id3->frequency)/1000))) {
                /* Refill the input buffer */
                buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
            }
            ci->seek_complete();
        }

        if((res=flac_decode_frame(&fc,decoded0,decoded1,buf,
                             bytesleft,ci->yield)) < 0) {
             LOGF("FLAC: Frame %d, error %d\n",frame,res);
             retval = CODEC_ERROR;
             goto done;
        }
        consumed=fc.gb.index/8;
        frame++;

        ci->yield();
        ci->pcmbuf_insert(&decoded0[fc.sample_skip], &decoded1[fc.sample_skip],
                          fc.blocksize - fc.sample_skip);
        
        fc.sample_skip = 0;

        /* Update the elapsed-time indicator */
        samplesdone=fc.samplenumber+fc.blocksize;
        elapsedtime=(samplesdone*10)/(ci->id3->frequency/100);
        ci->set_elapsed(elapsedtime);

        ci->advance_buffer(consumed);

        buf = ci->request_buffer(&bytesleft, MAX_FRAMESIZE);
    }
    retval = CODEC_OK;

done:
    LOGF("FLAC: Decoded %ld samples\n",samplesdone);

    if (ci->request_next_track())
        goto next_track;

exit:
    return retval;
}
