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

#include <codecs.h>
#include <inttypes.h>
#include "m4a.h"

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
    stream->ci->read_filebuf(buf,size);
    if (stream->ci->curpos >= stream->ci->filesize) { stream->eof=1; }
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

int32_t stream_tell(stream_t *stream)
{
    return stream->ci->curpos;
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
  stream->ci->advance_buffer(skip);
}

int stream_eof(stream_t *stream)
{
    return stream->eof;
}

void stream_create(stream_t *stream,struct codec_api* ci)
{
    stream->ci=ci;
    stream->eof=0;
}

/* This function was part of the original alac decoder implementation */

int get_sample_info(demux_res_t *demux_res, uint32_t samplenum,
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

unsigned int alac_seek (demux_res_t* demux_res, 
                        stream_t* stream,
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
  newpos=demux_res->mdat_offset;
  for (i=0;i<newblock;i++) {
    newpos+=demux_res->sample_byte_size[i];
  }

  /* We know the new file position, so let's try to seek to it */
  if (stream->ci->seek_buffer(newpos)) {
    *samplesdone=newsample;
    *currentblock=newblock;
    return 1;
  } else {
    return 0;
  }
}
