/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman, 2011 Andree Buschmann
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

#undef DEBUGF
#if defined(DEBUG)
#define DEBUGF stream->ci->debugf
#else
#define DEBUGF(...)
#endif

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

uint16_t stream_read_uint16(stream_t *stream)
{
    uint16_t v;
    stream_read(stream, 2, &v);
#ifdef ROCKBOX_LITTLE_ENDIAN
    _Swap16(v);
#endif
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

void stream_seek(stream_t *stream, size_t offset)
{
    stream->ci->seek_buffer(offset);
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

/* Check if there is a dedicated byte position contained for the given frame.
 * Return this byte position in case of success or return -1. This allows to
 * skip empty samples. 
 * During standard playback the search result (index i) will always increase. 
 * Therefor we save this index and let the caller set this value again as start
 * index when calling m4a_check_sample_offset() for the next frame. This 
 * reduces the overall loop count significantly. */
int m4a_check_sample_offset(demux_res_t *demux_res, uint32_t frame, uint32_t *start)
{
    uint32_t i = *start;
    for (i=0; i<demux_res->num_lookup_table; ++i)
    {
        if (demux_res->lookup_table[i].sample > frame ||
            demux_res->lookup_table[i].offset == 0)
            return -1;
        if (demux_res->lookup_table[i].sample == frame)
            break;
    }
    *start = i;
    return demux_res->lookup_table[i].offset;
}

/* Seek to desired sound sample location. Return 1 on success (and modify
 * sound_samples_done and current_sample), 0 if failed. */
unsigned int m4a_seek(demux_res_t* demux_res, stream_t* stream, 
    uint32_t sound_sample_loc, uint32_t* sound_samples_done, 
    int* current_sample)
{
    uint32_t i, sample_i, sound_sample_i;
    uint32_t time, time_cnt, time_dur;
    uint32_t chunk, chunk_first_sample;
    uint32_t offset;
    time_to_sample_t *tts_tab = demux_res->time_to_sample;
    sample_offset_t *tco_tab = demux_res->lookup_table;
    uint32_t *tsz_tab = demux_res->sample_byte_sizes;

    /* First check we have the required metadata - we should always have it. */
    if (!demux_res->num_time_to_samples || !demux_res->num_sample_byte_sizes)
    {
        return 0;
    }

    /* The 'sound_sample_loc' we have is PCM-based and not directly usable.
     * We need to convert it to an MP4 sample number 'sample_i' first. */
    sample_i = sound_sample_i = 0;
    for (time = 0; time < demux_res->num_time_to_samples; ++time)
    {
        time_cnt = tts_tab[time].sample_count;
        time_dur = tts_tab[time].sample_duration;
        uint32_t time_var = time_cnt * time_dur;

        if (sound_sample_loc < sound_sample_i + time_var)
        {
            time_var = sound_sample_loc - sound_sample_i;
            sample_i += time_var / time_dur;
            break;
        }

        sample_i       += time_cnt;
        sound_sample_i += time_var;
    }

    /* Find the chunk after 'sample_i'. */
    for (chunk = 1; chunk < demux_res->num_lookup_table; ++chunk)
    {
        if (tco_tab[chunk].offset == 0)
            break;
        if (tco_tab[chunk].sample > sample_i)
            break;
    }

    /* The preceding chunk is the one that contains 'sample_i'. */
    chunk--;
    chunk_first_sample = tco_tab[chunk].sample;
    offset = tco_tab[chunk].offset;

    /* Compute the PCM sample number of the chunk's first sample
     * to get an accurate base for sound_sample_i. */
    i = sound_sample_i = 0;
    for (time = 0; time < demux_res->num_time_to_samples; ++time)
    {
        time_cnt = tts_tab[time].sample_count;
        time_dur = tts_tab[time].sample_duration;

        if (chunk_first_sample < i + time_cnt)
        {
            sound_sample_i += (chunk_first_sample - i) * time_dur;
            break;
        }

        i += time_cnt;
        sound_sample_i += time_cnt * time_dur;
    }

    DEBUGF("seek chunk=%lu, sample=%lu, soundsample=%lu, offset=%lu\n",
           (unsigned long)chunk, (unsigned long)chunk_first_sample,
           (unsigned long)sound_sample_i, (unsigned long)offset);

    if (tsz_tab) {
        /* We have a sample-to-bytes table available so we can do accurate
         * seeking. Move one sample at a time and update the file offset and
         * PCM sample offset as we go. */
        for (i = chunk_first_sample;
             i < sample_i && i < demux_res->num_sample_byte_sizes; ++i)
        {
            /* this could be unnecessary */
            if (time_cnt == 0 && ++time < demux_res->num_time_to_samples)
            {
                time_cnt = tts_tab[time].sample_count;
                time_dur = tts_tab[time].sample_duration;
            }

            offset += tsz_tab[i];
            sound_sample_i += time_dur;
            time_cnt--;
        }
    } else {
        /* No sample-to-bytes table available so we can only seek to the
         * start of a chunk, which is often much lower resolution. */
        sample_i = chunk_first_sample;
    }

    if (stream->ci->seek_buffer(offset))
    {
        *sound_samples_done = sound_sample_i;
        *current_sample = sample_i;
        return 1;
    }

    return 0;
}

/* Seek to the sample containing file_loc. Return 1 on success (and modify
 * sound_samples_done and current_sample), 0 if failed.
 *
 * Seeking uses the following arrays:
 *
 * 1) the lookup_table array contains the file offset for the first sample
 *    of each chunk.
 *
 * 2) the time_to_sample array contains the duration (in sound samples) 
 *    of each sample of data.
 *
 * Locate the chunk containing location (using lookup_table), find the first
 * sample of that chunk (using lookup_table). Then use time_to_sample to
 * calculate the sound_samples_done value.
 */
unsigned int m4a_seek_raw(demux_res_t* demux_res, stream_t* stream,
    uint32_t file_loc, uint32_t* sound_samples_done, 
    int* current_sample)
{
    uint32_t i;
    uint32_t chunk_sample     = 0;
    uint32_t total_samples    = 0;
    uint32_t new_sound_sample = 0;
    uint32_t tmp_dur;
    uint32_t tmp_cnt;
    uint32_t new_pos;

    /* We know the desired byte offset, search for the chunk right before. 
     * Return the associated sample to this chunk as chunk_sample. */
    for (i=0; i < demux_res->num_lookup_table; ++i)
    {
        if (demux_res->lookup_table[i].offset > file_loc)
            break;
    }
    i = (i>0) ? i-1 : 0; /* We want the last chunk _before_ file_loc. */
    chunk_sample = demux_res->lookup_table[i].sample;
    new_pos      = demux_res->lookup_table[i].offset;
    
    /* Get sound sample offset. */
    i = 0;
    time_to_sample_t *tab2 = demux_res->time_to_sample;
    while (i < demux_res->num_time_to_samples)
    {
        tmp_dur = tab2[i].sample_duration;
        tmp_cnt = tab2[i].sample_count;
        total_samples    += tmp_cnt;
        new_sound_sample += tmp_cnt * tmp_dur;
        if (chunk_sample <= total_samples)
        {
            new_sound_sample += (chunk_sample - total_samples) * tmp_dur;
            break;
        }
        ++i;
    }

    /* Go to the new file position. */
    if (stream->ci->seek_buffer(new_pos)) 
    {
        *sound_samples_done = new_sound_sample;
        *current_sample = chunk_sample;
        return 1;
    } 

    return 0;
}
