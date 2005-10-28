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
#include <codecs/libalac/demux.h>
#include <codecs/libalac/decomp.h>
#include <codecs/libalac/stream.h>

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

#define destBufferSize (1024*16)

char inputBuffer[1024*32];    /* Input buffer */
int32_t outputbuffer[ALAC_MAX_CHANNELS][ALAC_BLOCKSIZE] IBSS_ATTR;
size_t mdat_offset;
struct codec_api* rb;
struct codec_api* ci;

/* Implementation of the stream.h functions used by libalac */

#define _Swap32(v) do { \
                   v = (((v) & 0x000000FF) << 0x18) | \
                       (((v) & 0x0000FF00) << 0x08) | \
                       (((v) & 0x00FF0000) >> 0x08) | \
                       (((v) & 0xFF000000) >> 0x18); } while(0)

#define _Swap16(v) do { \
                   v = (((v) & 0x00FF) << 0x08) | \
                       (((v) & 0xFF00) >> 0x08); } while (0)

/* A normal read without any byte-swapping */
void stream_read(stream_t *stream, size_t size, void *buf)
{
    ci->read_filebuf(buf,size);
    if (ci->curpos >= ci->filesize) { stream->eof=1; }
}

int32_t stream_read_int32(stream_t *stream)
{
    int32_t v;
    stream_read(stream, 4, &v);
#ifdef ROCKBOX_LITTLE_ENDIAN
    _Swap32(v);
#endif
    return v;
}

uint32_t stream_read_uint32(stream_t *stream)
{
    uint32_t v;
    stream_read(stream, 4, &v);
#ifdef ROCKBOX_LITTLE_ENDIAN
    _Swap32(v);
#endif
    return v;
}

int16_t stream_read_int16(stream_t *stream)
{
    int16_t v;
    stream_read(stream, 2, &v);
#ifdef ROCKBOX_LITTLE_ENDIAN
    _Swap16(v);
#endif
    return v;
}

uint16_t stream_read_uint16(stream_t *stream)
{
    uint16_t v;
    stream_read(stream, 2, &v);
#ifdef ROCKBOX_LITTLE_ENDIAN
    _Swap16(v);
#endif
    return v;
}

int8_t stream_read_int8(stream_t *stream)
{
    int8_t v;
    stream_read(stream, 1, &v);
    return v;
}

uint8_t stream_read_uint8(stream_t *stream)
{
    uint8_t v;
    stream_read(stream, 1, &v);
    return v;
}

void stream_skip(stream_t *stream, size_t skip)
{
  (void)stream;
  ci->advance_buffer(skip);
}

int stream_eof(stream_t *stream)
{
    return stream->eof;
}

void stream_create(stream_t *stream)
{
    stream->eof=0;
}

/* This function was part of the original alac decoder implementation */

static int get_sample_info(demux_res_t *demux_res, uint32_t samplenum,
                           uint32_t *sample_duration,
                           uint32_t *sample_byte_size)
{
    unsigned int duration_index_accum = 0;
    unsigned int duration_cur_index = 0;

    if (samplenum >= demux_res->num_sample_byte_sizes) { 
        return 0;
    }

    if (!demux_res->num_time_to_samples) {
        return 0;
    }

    while ((demux_res->time_to_sample[duration_cur_index].sample_count
            + duration_index_accum) <= samplenum) {
        duration_index_accum +=
        demux_res->time_to_sample[duration_cur_index].sample_count;

        duration_cur_index++;
        if (duration_cur_index >= demux_res->num_time_to_samples) {
            return 0;
        }
    }

    *sample_duration =
     demux_res->time_to_sample[duration_cur_index].sample_duration;
    *sample_byte_size = demux_res->sample_byte_size[samplenum];

    return 1;
}

/* Seek to sample_loc (or close to it).  Return 1 on success (and
   modify samplesdone and currentblock), 0 if failed 

   Seeking uses the following two arrays:

     1) the sample_byte_size array contains the length in bytes of
        each block ("sample" in Applespeak).

     2) the time_to_sample array contains the duration (in samples) of
        each block of data.

     So we just find the block number we are going to seek to (using
     time_to_sample) and then find the offset in the file (using
     sample_byte_size).

     Each ALAC block seems to be independent of all the others.
 */

static unsigned int alac_seek (demux_res_t* demux_res, 
                               unsigned int sample_loc, 
                               uint32_t* samplesdone, int* currentblock)
{
  int flag;
  unsigned int i,j;
  unsigned int newblock;
  unsigned int newsample;
  unsigned int newpos;

  /* First check we have the appropriate metadata - we should always
     have it. */
  if ((demux_res->num_time_to_samples==0) ||
    (demux_res->num_sample_byte_sizes==0)) { return 0; }

  /* Find the destination block from time_to_sample array */
  i=0;
  newblock=0;
  newsample=0;
  flag=0;

  while ((i<demux_res->num_time_to_samples) && (flag==0) && 
         (newsample < sample_loc)) {
    j=(sample_loc-newsample) /
      demux_res->time_to_sample[i].sample_duration;
    
    if (j <= demux_res->time_to_sample[i].sample_count) {
      newblock+=j;
      newsample+=j*demux_res->time_to_sample[i].sample_duration;
      flag=1;
    } else {
      newsample+=(demux_res->time_to_sample[i].sample_duration
                 * demux_res->time_to_sample[i].sample_count);
      newblock+=demux_res->time_to_sample[i].sample_count;
      i++;
    }
  }

  /* We know the new block, now calculate the file position */
  newpos=mdat_offset;
  for (i=0;i<newblock;i++) {
    newpos+=demux_res->sample_byte_size[i];
  }

  /* We know the new file position, so let's try to seek to it */
  if (ci->seek_buffer(newpos)) {
    *samplesdone=newsample;
    *currentblock=newblock;
    return 1;
  } else {
    return 0;
  }
}

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
  size_t n;
  demux_res_t demux_res;
  static stream_t input_stream;
  uint32_t samplesdone;
  uint32_t elapsedtime;
  uint32_t sample_duration;
  uint32_t sample_byte_size;
  int samplesdecoded;
  unsigned int i;
  unsigned char* buffer;
  alac_file alac;

  /* Generic codec initialisation */
  TEST_CODEC_API(api);

  rb = api;
  ci = (struct codec_api*)api;

#ifndef SIMULATOR
  rb->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

  ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*10));
  ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));
  ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*128));

  ci->configure(CODEC_DSP_ENABLE, (bool *)true);
  ci->configure(DSP_DITHER, (bool *)false);
  ci->configure(DSP_SET_STEREO_MODE, (int *)STEREO_NONINTERLEAVED);
  ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(28));
  
  next_track:

  if (codec_init(api)) {
    LOGF("ALAC: Error initialising codec\n");
    return CODEC_ERROR;
  }

  while (!rb->taginfo_ready)
    rb->yield();
  
  ci->configure(DSP_SET_FREQUENCY, (long *)(rb->id3->frequency));

  stream_create(&input_stream);

  /* if qtmovie_read returns successfully, the stream is up to
   * the movie data, which can be used directly by the decoder */
  if (!qtmovie_read(&input_stream, &demux_res)) {
    LOGF("ALAC: Error initialising file\n");
    return CODEC_ERROR;
  }

  /* Keep track of start of stream in file - used for seeking */
  mdat_offset=ci->curpos;

  /* initialise the sound converter */
  create_alac(demux_res.sample_size, demux_res.num_channels,&alac);
  alac_set_info(&alac, demux_res.codecdata);

  i=0;
  samplesdone=0;
  /* The main decoding loop */
  while (i < demux_res.num_sample_byte_sizes) {
    rb->yield();
    if (ci->stop_codec || ci->reload_codec) {
      break;
    }

    /* Deal with any pending seek requests */
    if (ci->seek_time) {
      if (alac_seek(&demux_res,
                    (ci->seek_time/10) * (ci->id3->frequency/100),
                    &samplesdone, &i)) {
        elapsedtime=(samplesdone*10)/(ci->id3->frequency/100);
        ci->set_elapsed(elapsedtime);
      }
      ci->seek_time = 0;
    }

    /* Lookup the length (in samples and bytes) of block i */
    if (!get_sample_info(&demux_res, i, &sample_duration, 
                         &sample_byte_size)) {
      LOGF("ALAC: Error in get_sample_info\n");
      return CODEC_ERROR;
    }

    /* Request the required number of bytes from the input buffer */

    buffer=ci->request_buffer((long*)&n,sample_byte_size);
    if (n!=sample_byte_size) {
      /* The decode_frame function requires the whole frame, so if we
         can't get it contiguously from the buffer, then we need to
         copy it via a read - i.e. we are at the buffer wraparound
         point */

      /* Check we estimated the maximum buffer size correctly */
      if (sample_byte_size > sizeof(inputBuffer)) {
        LOGF("ALAC: Input buffer < %d bytes\n",sample_byte_size);
        return CODEC_ERROR;
      }

      n=ci->read_filebuf(inputBuffer,sample_byte_size);
      if (n!=sample_byte_size) {
        LOGF("ALAC: Error reading data\n");
        return CODEC_ERROR;
      }
      buffer=inputBuffer;
    }

    /* Decode one block - returned samples will be host-endian */
    rb->yield();
    samplesdecoded=alac_decode_frame(&alac, buffer, outputbuffer, rb->yield);

    /* Advance codec buffer - unless we did a read */
    if ((char*)buffer!=(char*)inputBuffer) {
      ci->advance_buffer(n);
    }

    /* Output the audio */
    rb->yield();
    while(!ci->pcmbuf_insert_split(outputbuffer[0],
                                   outputbuffer[1],
                                   samplesdecoded*sizeof(int32_t)))
      rb->yield();

    /* Update the elapsed-time indicator */
    samplesdone+=sample_duration;
    elapsedtime=(samplesdone*10)/(ci->id3->frequency/100);
    ci->set_elapsed(elapsedtime);

    /* Keep track of current position - for resuming */
    ci->set_offset(elapsedtime);

    i++;
  }

  LOGF("ALAC: Decoded %d samples\n",samplesdone);

  if (ci->request_next_track())
   goto next_track;

  return CODEC_OK;
}
