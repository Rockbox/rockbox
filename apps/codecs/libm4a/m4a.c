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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

unsigned int get_sample_offset(demux_res_t *demux_res, uint32_t sample)
{
    uint32_t chunk = 1;
    uint32_t range_samples = 0;
    uint32_t total_samples = 0;
    uint32_t chunk_sample;
    uint32_t prev_chunk;
    uint32_t prev_chunk_samples;
    uint32_t file_offset;
    uint32_t i;
    
    /* First check we have the appropriate metadata - we should always
     * have it.
     */
       
    if (sample >= demux_res->num_sample_byte_sizes ||
        !demux_res->num_sample_to_chunks ||
        !demux_res->num_chunk_offsets) 
    {
        return 0;
    }

    /* Locate the chunk containing the sample */
    
    prev_chunk = demux_res->sample_to_chunk[0].first_chunk;
    prev_chunk_samples = demux_res->sample_to_chunk[0].num_samples;

    for (i = 1; i < demux_res->num_sample_to_chunks; i++) 
    {
        chunk = demux_res->sample_to_chunk[i].first_chunk;
        range_samples = (chunk - prev_chunk) * prev_chunk_samples;

        if (sample < total_samples + range_samples)
        {
            break;
        }

        total_samples += range_samples;
        prev_chunk = demux_res->sample_to_chunk[i].first_chunk;
        prev_chunk_samples = demux_res->sample_to_chunk[i].num_samples;
    }

    if (sample >= demux_res->sample_to_chunk[0].num_samples)
    {
        chunk = prev_chunk + (sample - total_samples) / prev_chunk_samples;
    }
    else
    {
        chunk = 1;
    }
    
    /* Get sample of the first sample in the chunk */
    
    chunk_sample = total_samples + (chunk - prev_chunk) * prev_chunk_samples;
    
    /* Get offset in file */
    
    if (chunk > demux_res->num_chunk_offsets)
    {
        file_offset = demux_res->chunk_offset[demux_res->num_chunk_offsets - 1];
    }
    else
    {
        file_offset = demux_res->chunk_offset[chunk - 1];
    }
    
    if (chunk_sample > sample) 
    {
        return 0;
    }
 
    for (i = chunk_sample; i < sample; i++)
    {
        file_offset += demux_res->sample_byte_size[i];
    }
    
    if (file_offset > demux_res->mdat_offset + demux_res->mdat_len)
    {
        return 0;
    }
    
    return file_offset;
}

/* Seek to the sample containing sound_sample_loc. Return 1 on success 
 * (and modify sound_samples_done and current_sample), 0 if failed.
 *
 * Seeking uses the following arrays:
 *
 * 1) the time_to_sample array contains the duration (in sound samples) 
 *    of each sample of data.
 *
 * 2) the sample_byte_size array contains the length in bytes of each
 *    sample.
 *
 * 3) the sample_to_chunk array contains information about which chunk
 *    of samples each sample belongs to.
 *
 * 4) the chunk_offset array contains the file offset of each chunk.
 *
 * So find the sample number we are going to seek to (using time_to_sample)
 * and then find the offset in the file (using sample_to_chunk, 
 * chunk_offset sample_byte_size, in that order.).
 *
 */
unsigned int alac_seek(demux_res_t* demux_res, stream_t* stream, 
    uint32_t sound_sample_loc, uint32_t* sound_samples_done, 
    int* current_sample)
{
    uint32_t i;
    uint32_t j;
    uint32_t new_sample;
    uint32_t new_sound_sample;
    uint32_t new_pos;

    /* First check we have the appropriate metadata - we should always
     * have it.
     */
       
    if ((demux_res->num_time_to_samples==0) ||
       (demux_res->num_sample_byte_sizes==0))
    { 
        return 0; 
    }

    /* Find the destination block from time_to_sample array */
    
    i = 0;
    new_sample = 0;
    new_sound_sample = 0;

    while ((i < demux_res->num_time_to_samples) &&
        (new_sound_sample < sound_sample_loc)) 
    {
        j = (sound_sample_loc - new_sound_sample) /
            demux_res->time_to_sample[i].sample_duration;
    
        if (j <= demux_res->time_to_sample[i].sample_count)
        {
            new_sample += j;
            new_sound_sample += j * 
                demux_res->time_to_sample[i].sample_duration;
            break;
        } 
        else 
        {
            new_sound_sample += (demux_res->time_to_sample[i].sample_duration
                * demux_res->time_to_sample[i].sample_count);
            new_sample += demux_res->time_to_sample[i].sample_count;
            i++;
        }
    }

    /* We know the new block, now calculate the file position. */
  
    new_pos = get_sample_offset(demux_res, new_sample);

    /* We know the new file position, so let's try to seek to it */
  
    if (stream->ci->seek_buffer(new_pos)) 
    {
        *sound_samples_done = new_sound_sample;
        *current_sample = new_sample;
        return 1;
    }
    
    return 0;
}

/* Seek to the sample containing file_loc. Return 1 on success (and modify
 * sound_samples_done and current_sample), 0 if failed.
 *
 * Seeking uses the following arrays:
 *
 * 1) the chunk_offset array contains the file offset of each chunk.
 *
 * 2) the sample_to_chunk array contains information about which chunk
 *    of samples each sample belongs to.
 *
 * 3) the sample_byte_size array contains the length in bytes of each
 *    sample.
 *
 * 4) the time_to_sample array contains the duration (in sound samples) 
 *    of each sample of data.
 *
 * Locate the chunk containing location (using chunk_offset), find the 
 * sample of that chunk (using sample_to_chunk) and finally the location
 * of that sample (using sample_byte_size). Then use time_to_sample to
 * calculate the sound_samples_done value.
 */
unsigned int alac_seek_raw(demux_res_t* demux_res, stream_t* stream,
    uint32_t file_loc, uint32_t* sound_samples_done, 
    int* current_sample)
{
    uint32_t chunk_sample = 0;
    uint32_t total_samples = 0;
    uint32_t new_sound_sample = 0;
    uint32_t new_pos;
    uint32_t chunk;
    uint32_t i;

    if (!demux_res->num_chunk_offsets ||
        !demux_res->num_sample_to_chunks) 
    {
        return 0;
    }

    /* Locate the chunk containing file_loc. */

    for (i = 0; i < demux_res->num_chunk_offsets && 
        file_loc < demux_res->chunk_offset[i]; i++)
    {
    }
    
    chunk = i + 1;
    new_pos = demux_res->chunk_offset[chunk - 1];

    /* Get the first sample of the chunk. */
    
    for (i = 1; i < demux_res->num_sample_to_chunks &&
        chunk < demux_res->sample_to_chunk[i - 1].first_chunk; i++) 
    {
        chunk_sample += demux_res->sample_to_chunk[i - 1].num_samples *
            (demux_res->sample_to_chunk[i].first_chunk - 
                demux_res->sample_to_chunk[i - 1].first_chunk);
    }
    
    chunk_sample += (chunk - demux_res->sample_to_chunk[i - 1].first_chunk) *
        demux_res->sample_to_chunk[i - 1].num_samples;

    /* Get the position within the chunk. */
    
    for (; chunk_sample < demux_res->num_sample_byte_sizes; chunk_sample++)
    {
        if (file_loc < new_pos + demux_res->sample_byte_size[chunk_sample])
        {
            break;
        }
        
        new_pos += demux_res->sample_byte_size[chunk_sample];
    }
    
    /* Get sound sample offset. */

    for (i = 0; i < demux_res->num_time_to_samples; i++)
    {
        if (chunk_sample < 
            total_samples + demux_res->time_to_sample[i].sample_count)
        {
            break;
        }
        
        total_samples += demux_res->time_to_sample[i].sample_count;
        new_sound_sample += demux_res->time_to_sample[i].sample_count 
            * demux_res->time_to_sample[i].sample_duration;
    }
    
    new_sound_sample += (chunk_sample - total_samples) 
        * demux_res->time_to_sample[i].sample_duration;

    /* Go to the new file position. */
    
    if (stream->ci->seek_buffer(new_pos)) 
    {
        *sound_samples_done = new_sound_sample;
        *current_sample = chunk_sample;
        return 1;
    } 

    return 0;
}
