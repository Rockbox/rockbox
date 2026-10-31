/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
    uint32_t i;

    /* lookup_table[] may not be of the whole file: see demux.c */
    if (frame >= demux_res->lookup_end || frame < demux_res->lookup_first)
    {
        m4a_lookup_refill(demux_res, demux_res->stream, frame);
        *start = 0;
    }

    i = *start;
    for (;i < demux_res->num_lookup_table; ++i)
    {
        if (demux_res->lookup_table[i].sample > frame)
            break;

        if (demux_res->lookup_table[i].sample == frame)
        {
            *start = i;
            return demux_res->lookup_table[i].offset;
        }
    }
    *start = i;
    return -1;
}

static inline void read_time_to_sample_entry(stream_t* stream, uint32_t* time_cnt, uint32_t* time_dur)
{
    *time_cnt = stream_read_uint32(stream);
    *time_dur = stream_read_uint32(stream);
}

/* A seek that would land within this many seconds of where it was asked
 * to go does not read frame sizes to do better: going to them and back can
 * cost two refills of the playback buffer. */
#define SEEK_NEAR_ENOUGH 3

/* Go through the sizes of up to n frames from frame first, which are in
 * the file, stopping at the frame that ends after byte limit of them.
 * *sum is the size of the frames passed.  Returns how many that was, or -1
 * if the sizes could not be read. */
static int32_t sum_frame_sizes(demux_res_t *demux_res, stream_t *stream,
                               uint32_t first, uint32_t n, uint32_t limit,
                               uint32_t *sum)
{
    uint32_t pos = demux_res->sample_byte_sizes_offset + 4 * first;
    uint32_t total = 0, left = n;

    while (left)
    {
        size_t got, want = left > 1024 ? 4096 : 4 * left;
        const unsigned char *p;

        if (!stream->ci->seek_buffer(pos))
            return -1;
        p = stream->ci->request_buffer(&got, want);
        if (!p || got < 4)
            return -1;
        if (got > want)
            got = want;
        got /= 4;
        pos += 4 * got;
        for (; got; got--, left--, p += 4)
        {
            uint32_t size = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                            ((uint32_t)p[2] << 8) | p[3];
            if (size > limit - total)
            {
                *sum = total;
                return n - left;
            }
            total += size;
        }
    }
    *sum = total;
    return n;
}

/* Seek to desired sound sample location. Return 1 on success (and modify
 * sound_samples_done and current_sample), 0 if failed. */
unsigned int m4a_seek(demux_res_t* demux_res, stream_t* stream,
    uint64_t sound_sample_loc, uint64_t* sound_samples_done,
    uint32_t* current_sample, uint32_t* lookup_table_idx)
{
    uint32_t i, sample_i;
    uint32_t time, time_cnt = 0, time_dur = 0;
    uint32_t chunk, chunk_first_sample;
    uint32_t offset;
    uint64_t sound_sample_i;
    time_to_sample_t *tts_tab = demux_res->time_to_sample;

    /* First check we have the required metadata - we should always have it. */
    if (!demux_res->num_time_to_samples || !demux_res->num_sample_byte_sizes ||
        (!demux_res->num_seek_offsets && !demux_res->lookup_step) ||
        !demux_res->num_lookup_table)
    {
        return 0;
    }

    /* The 'sound_sample_loc' we have is PCM-based and not directly usable.
     * We need to convert it to an MP4 sample number 'sample_i' first. */
    sample_i = sound_sample_i = 0;
    if (tts_tab == NULL)
    {
        stream_seek(stream, demux_res->time_to_sample_offset);
    }
    for (time = 0; time < demux_res->num_time_to_samples; ++time)
    {
        if (tts_tab)
        {
            time_dur = tts_tab[time].sample_duration;
            time_cnt = tts_tab[time].sample_count;
        }
        else
        {
            read_time_to_sample_entry(stream, &time_cnt, &time_dur);
        }
        uint64_t time_var = (uint64_t)time_cnt * time_dur;

        if (sound_sample_loc < sound_sample_i + time_var)
        {
            time_var = sound_sample_loc - sound_sample_i;
            sample_i += time_var / time_dur;
            break;
        }

        sample_i       += time_cnt;
        sound_sample_i += time_var;
    }

    /* The last frame the seek index has that is not after 'sample_i'. */
    if (sample_i >= demux_res->num_sample_byte_sizes)
        sample_i = demux_res->num_sample_byte_sizes - 1;
    if (demux_res->num_seek_offsets)
    {
        chunk = sample_i / demux_res->seek_step;
        if (chunk >= demux_res->num_seek_offsets)
            chunk = demux_res->num_seek_offsets - 1;
        chunk_first_sample = chunk * demux_res->seek_step;
        offset = demux_res->seek_offsets[chunk];
        /* m4a_check_sample_offset() finds its place again from the start */
        *lookup_table_idx = 0;
    }
    else
    {
        /* An index of chunks: the last of them that does not start after
         * 'sample_i'. */
        sample_offset_t *tab = demux_res->lookup_table;
        uint32_t above = demux_res->num_lookup_table;
        uint32_t ahead, sum;

        chunk = 0;
        while (above - chunk > 1)
        {
            uint32_t mid = chunk + (above - chunk) / 2;
            if (tab[mid].sample > sample_i)
                above = mid;
            else
                chunk = mid;
        }
        chunk_first_sample = tab[chunk].sample;
        offset = tab[chunk].offset;
        *lookup_table_idx = chunk;

        /* With every chunk in the index, the frames from there to
         * 'sample_i' are in this chunk and their sizes say where it is. */
        ahead = sample_i > chunk_first_sample ? sample_i - chunk_first_sample
                                              : 0;
        if (demux_res->lookup_step == 1 &&
            (uint64_t)ahead * time_dur >
            (uint64_t)SEEK_NEAR_ENOUGH * demux_res->sound_sample_rate &&
            sum_frame_sizes(demux_res, stream, chunk_first_sample, ahead,
                            0xffffffff, &sum) >= 0)
        {
            chunk_first_sample = sample_i;
            offset += sum;
        }
    }

    /* Compute the PCM sample number of the chunk's first sample
     * to get an accurate base for sound_sample_i. */
    i = sound_sample_i = 0;
    if (tts_tab == NULL)
    {
        stream_seek(stream, demux_res->time_to_sample_offset);
    }

    for (time = 0; time < demux_res->num_time_to_samples; ++time)
    {
        if (tts_tab)
        {
            time_dur = tts_tab[time].sample_duration;
            time_cnt = tts_tab[time].sample_count;
        }
        else
        {
            read_time_to_sample_entry(stream, &time_cnt, &time_dur);
        }

        if (chunk_first_sample < i + time_cnt)
        {
            sound_sample_i += (uint64_t)(chunk_first_sample - i) * time_dur;
            break;
        }

        i += time_cnt;
        sound_sample_i += (uint64_t)time_cnt * time_dur;
    }

    /* That frame is where to go: the one asked for if the index has every
     * frame or its size was read, and otherwise less than a step of the
     * index before it. */
    sample_i = chunk_first_sample;

    DEBUGF("seek chunk=%lu, chunk_first_sample=%lu, sample_i=%u, soundsample=%lu, offset=%lu\n",
           (unsigned long)chunk,
           (unsigned long)chunk_first_sample,
           sample_i,
           (unsigned long)sound_sample_i,
           (unsigned long)offset);

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
 * 1) the seek_offsets array, the seek index, contains the file offset of
 *    every seek_step'th frame.  A very long file has the lookup_table
 *    array of chunks instead.
 *
 * 2) the time_to_sample array contains the duration (in sound samples)
 *    of each sample of data.
 *
 * Locate the last entry of the index before location and go to its frame.
 * Then use time_to_sample to calculate the sound_samples_done value.
 */
unsigned int m4a_seek_raw(demux_res_t* demux_res, stream_t* stream,
    uint32_t file_loc, uint64_t* sound_samples_done,
    uint32_t* current_sample, uint32_t* lookup_table_idx)
{
    uint32_t i;
    uint32_t chunk_sample     = 0;
    uint32_t total_samples    = 0;
    uint64_t new_sound_sample = 0;
    uint32_t time_dur;
    uint32_t time_cnt;
    uint32_t new_pos;

    /* We know the desired byte offset, search for the frame right before
     * it that the seek index has.  Return it as chunk_sample. */
    if (demux_res->num_seek_offsets)
    {
        for (i = 1; i < demux_res->num_seek_offsets; ++i)
        {
            if (demux_res->seek_offsets[i] > file_loc)
                break;
        }
        --i; /* We want the last entry _before_ file_loc. */
        *lookup_table_idx = 0;
        chunk_sample = i * demux_res->seek_step;
        new_pos      = demux_res->seek_offsets[i];
    }
    else if (demux_res->lookup_step && demux_res->num_lookup_table)
    {
        /* An index of chunks: the same, with the chunks it has. */
        for (i = 1; i < demux_res->num_lookup_table; ++i)
        {
            if (demux_res->lookup_table[i].offset > file_loc)
                break;
        }
        --i;
        *lookup_table_idx = i;
        chunk_sample = demux_res->lookup_table[i].sample;
        new_pos      = demux_res->lookup_table[i].offset;

        /* With every chunk in the index, the sizes of this one's frames
         * say which of them file_loc is in. */
        if (demux_res->lookup_step == 1 && file_loc > new_pos)
        {
            uint32_t sum;
            uint32_t end = i + 1 < demux_res->num_lookup_table
                           ? demux_res->lookup_table[i + 1].sample
                           : demux_res->num_sample_byte_sizes;
            /* the last frame of the chunk at most: what is between it
             * and the next chunk is not this track's */
            int32_t n = sum_frame_sizes(demux_res, stream, chunk_sample,
                                        end - chunk_sample - 1,
                                        file_loc - new_pos, &sum);
            if (n > 0)
            {
                chunk_sample += n;
                new_pos += sum;
            }
        }
    }
    else
    {
        return 0;
    }

    /* Get sound sample offset. */
    i = 0;
    time_to_sample_t *tts_tab = demux_res->time_to_sample;
    if (tts_tab == NULL)
    {
        stream_seek(stream, demux_res->time_to_sample_offset);
    }
    while (i < demux_res->num_time_to_samples)
    {
        if (tts_tab)
        {
            time_dur = tts_tab[i].sample_duration;
            time_cnt = tts_tab[i].sample_count;
        }
        else
        {
            read_time_to_sample_entry(stream, &time_cnt, &time_dur);
        }
        total_samples    += time_cnt;
        new_sound_sample += (uint64_t)time_cnt * time_dur;
        if (chunk_sample <= total_samples)
        {
            new_sound_sample -= (uint64_t)(total_samples - chunk_sample) * time_dur;
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
