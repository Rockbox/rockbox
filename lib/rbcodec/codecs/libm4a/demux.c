/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * This is the quicktime container demuxer.
 *
 * http://crazney.net/programs/itunes/alac.html
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

//#define DEBUG

#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include "codeclib.h"

#include "m4a.h"

#undef DEBUGF
#if defined(DEBUG)
#define DEBUGF qtmovie->stream->ci->debugf
#else
#define DEBUGF(...)
#endif

/* The most tracks other than the one played whose place in the file is
 * kept, to see whether their data is among its chunks. */
#define OTHER_TRACKS_MAX 8

typedef struct
{
    stream_t *stream;
    demux_res_t *res;
    /* the first and last chunk offsets of the track played */
    uint32_t audio_first, audio_last;
    bool have_audio;
    /* and of the others, as many as there is room for */
    uint32_t other_first[OTHER_TRACKS_MAX], other_last[OTHER_TRACKS_MAX];
    int num_other;
} qtmovie_t;


/* chunk handlers */
static void read_chunk_ftyp(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;

    // filetype (supported ignore case values: m4a, m4b, mp42, 3gp6, qt, isom)
    char filetype[4];
    stream_read(qtmovie->stream, 4, filetype);
    size_remaining-=4;

    /* minor_ver = */ stream_read_uint32(qtmovie->stream);
    size_remaining-=4;

    /* compatible brands */
    while (size_remaining)
    {
        /* unused */
        /*fourcc_t cbrand =*/ stream_read_uint32(qtmovie->stream);
        size_remaining-=4;
    }
}

static uint32_t mp4ff_read_mp4_descr_length(stream_t* stream)
{
    uint8_t b;
    uint8_t numBytes = 0;
    uint32_t length = 0;

    do
    {
        b = stream_read_uint8(stream);
        numBytes++;
        length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && numBytes < 4);

    return length;
}

/* The following function is based on mp4ff */
static bool read_chunk_esds(qtmovie_t *qtmovie, size_t chunk_len)
{
    uint8_t tag;
    uint32_t temp;

    (void)chunk_len;
    /* version and flags */
    temp=stream_read_uint32(qtmovie->stream);

    /* get and verify ES_DescrTag */
    tag = stream_read_uint8(qtmovie->stream);
    if (tag == 0x03)
    {
        /* read length */
        if (mp4ff_read_mp4_descr_length(qtmovie->stream) < 5 + 15)
        {
            return false;
        }
        /* skip 3 bytes */
        stream_skip(qtmovie->stream,3);
    } else {
        /* skip 2 bytes */
        stream_skip(qtmovie->stream,2);
    }

    /* get and verify DecoderConfigDescrTab */
    if (stream_read_uint8(qtmovie->stream) != 0x04)
    {
        return false;
    }

    /* read length */
    temp = mp4ff_read_mp4_descr_length(qtmovie->stream);
    if (temp < 13) return false;

    /* audioType = */  stream_read_uint8(qtmovie->stream);
    /* temp = */       stream_read_int32(qtmovie->stream);//0x15000414 ????
    /* maxBitrate = */ stream_read_int32(qtmovie->stream);
    /* avgBitrate = */ stream_read_int32(qtmovie->stream);

    /* get and verify DecSpecificInfoTag */
    if (stream_read_uint8(qtmovie->stream) != 0x05)
    {
        return false;
    }

    /* read length */
    qtmovie->res->codecdata_len = mp4ff_read_mp4_descr_length(qtmovie->stream);
    if (qtmovie->res->codecdata_len > MAX_CODECDATA_SIZE)
    {
        DEBUGF("codecdata too large (%d) in esds\n",
               (int)qtmovie->res->codecdata_len);
        return false;
    }

    stream_read(qtmovie->stream, qtmovie->res->codecdata_len, qtmovie->res->codecdata);

    /* will skip the remainder of the atom */
    return true;
}

static bool read_chunk_stsd(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    int j;
    uint32_t numentries;
//    size_t size_remaining = chunk_len - 8;
    bool got_codec_data = false;
    (void)chunk_len;

    /* version */
    stream_read_uint8(qtmovie->stream);
//    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
//    size_remaining -= 3;

    numentries = stream_read_uint32(qtmovie->stream);
//    size_remaining -= 4;

    /* if (numentries != 1)
    {
        DEBUGF("only expecting one entry in sample description atom!\n");
        return false;
    } */

    for (i = 0; i < numentries; i++)
    {
        uint32_t entry_size;

        uint32_t entry_remaining;

        entry_size = stream_read_uint32(qtmovie->stream);
        qtmovie->res->format = stream_read_uint32(qtmovie->stream);
        DEBUGF("format: %c%c%c%c\n",SPLITFOURCC(qtmovie->res->format));
        entry_remaining = entry_size;
        entry_remaining -= 8;

        /* sound info: */

        /* reserved + data reference index + sound version + reserved */
        stream_skip(qtmovie->stream, 6 + 2 + 2 + 6);
        entry_remaining -= 6 + 2 + 2 + 6;

        qtmovie->res->num_channels = stream_read_uint16(qtmovie->stream);
        qtmovie->res->sound_sample_size = stream_read_uint16(qtmovie->stream);
        entry_remaining -= 4;

        /* packet size */
        stream_skip(qtmovie->stream, 2);
        qtmovie->res->sound_sample_rate = stream_read_uint32(qtmovie->stream);
        /* reserved size */
        stream_skip(qtmovie->stream, 2);
        entry_remaining -= 8;

        /* remaining is codec data */

        if (qtmovie->res->format==MAKEFOURCC('a','l','a','c')) {
          if (qtmovie->stream->ci->id3->codectype!=AFMT_MP4_ALAC) {
               return false;
          }

          /* 12 = audio format atom, 8 = padding */
          qtmovie->res->codecdata_len = entry_remaining + 12 + 8;
          if (qtmovie->res->codecdata_len > MAX_CODECDATA_SIZE)
          {
              DEBUGF("codecdata too large (%d) in stsd\n",
                      (int)qtmovie->res->codecdata_len);
              return false;
          }

          memset(qtmovie->res->codecdata, 0, qtmovie->res->codecdata_len);
          /* audio format atom */
#if 0
          /* The ALAC decoder skips these bytes, so there is no need to store them,
             and this code isn't endian/alignment safe */
          ((unsigned int*)qtmovie->res->codecdata)[0] = 0x0c000000;
          ((unsigned int*)qtmovie->res->codecdata)[1] = MAKEFOURCC('a','m','r','f');
          ((unsigned int*)qtmovie->res->codecdata)[2] = MAKEFOURCC('c','a','l','a');
#endif

          stream_read(qtmovie->stream,
                  entry_remaining,
                  ((char*)qtmovie->res->codecdata) + 12);
          entry_remaining -= entry_remaining;
          got_codec_data = true;

          if (entry_remaining)
              stream_skip(qtmovie->stream, entry_remaining);

        } else if (qtmovie->res->format==MAKEFOURCC('m','p','4','a')) {
          if (qtmovie->stream->ci->id3->codectype!=AFMT_MP4_AAC &&
              qtmovie->stream->ci->id3->codectype!=AFMT_MP4_AAC_HE) {
               return false;
          }

          size_t sub_chunk_len;
          fourcc_t sub_chunk_id;

          sub_chunk_len = stream_read_uint32(qtmovie->stream);
          if (sub_chunk_len <= 1 || sub_chunk_len > entry_remaining)
          {
              DEBUGF("strange size (%lu) for chunk inside mp4a\n",
                     (unsigned long)sub_chunk_len);
              return false;
          }

          sub_chunk_id = stream_read_uint32(qtmovie->stream);

          if (sub_chunk_id != MAKEFOURCC('e','s','d','s'))
          {
              DEBUGF("Expected esds chunk inside mp4a, found %c%c%c%c\n",SPLITFOURCC(sub_chunk_id));
              return false;
          }

          j=qtmovie->stream->ci->curpos+sub_chunk_len-8;
          if (read_chunk_esds(qtmovie,sub_chunk_len)) {
             if (j!=qtmovie->stream->ci->curpos) {
               DEBUGF("curpos=%ld, j=%d - Skipping %ld bytes\n",qtmovie->stream->ci->curpos,j,j-qtmovie->stream->ci->curpos);
               stream_skip(qtmovie->stream,j-qtmovie->stream->ci->curpos);
             }
             got_codec_data = true;
             entry_remaining-=sub_chunk_len;
          } else {
              DEBUGF("Error reading esds\n");
              return false;
          }

          DEBUGF("entry_remaining=%ld\n",(long)entry_remaining);
          stream_skip(qtmovie->stream,entry_remaining);

        } else if (qtmovie->res->format==MAKEFOURCC('f','r','e','e')) {
            /* Skip "filler" atom */
            stream_skip(qtmovie->stream,entry_remaining);
        } else {
            DEBUGF("expecting 'alac', 'mp4a' or 'free' data format, got %c%c%c%c\n",
                 SPLITFOURCC(qtmovie->res->format));
            return false;
        }
    }
    return got_codec_data;
}

static bool read_chunk_stts(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8;

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_time_to_samples = numentries;
    qtmovie->res->time_to_sample = malloc(numentries * sizeof(*qtmovie->res->time_to_sample));
    if (qtmovie->res->time_to_sample && numentries > 1)
    {
        //Make sure we leave space for at least 100 records in lookup table
        void* enough_mem = malloc(sizeof(*qtmovie->res->lookup_table)*100);
        if (!enough_mem)
        {
            free(qtmovie->res->time_to_sample);
            qtmovie->res->time_to_sample = NULL;
        }
        else
        {
            free(enough_mem);
        }
    }
    if (!qtmovie->res->time_to_sample)
    {
        qtmovie->res->time_to_sample_offset = stream_tell(qtmovie->stream);
        stream_skip(qtmovie->stream, size_remaining);
        DEBUGF("stts too large %ld, save time_to_sample_offset\n", size_remaining);
        return true;
    }

    DEBUGF("stts numentires %d, size: %ld\n", numentries, numentries * sizeof(*qtmovie->res->time_to_sample));
    for (i = 0; i < numentries; i++)
    {
        qtmovie->res->time_to_sample[i].sample_count = stream_read_uint32(qtmovie->stream);
        qtmovie->res->time_to_sample[i].sample_duration = stream_read_uint32(qtmovie->stream);
        size_remaining -= 8;
    }

    if (size_remaining)
    {
        DEBUGF("ehm, size remianing?\n");
        stream_skip(qtmovie->stream, size_remaining);
    }

    return true;
}

static bool read_chunk_stsz(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;
    uint32_t numsizes;

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    /* default sample size */
    if (stream_read_uint32(qtmovie->stream) != 0)
    {
        DEBUGF("i was expecting variable samples sizes\n");
        stream_read_uint32(qtmovie->stream);
        size_remaining -= 4;
        return true;
    }
    size_remaining -= 4;

    numsizes = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    /* The sizes are not kept: build_seek_index() reads them from here. */
    qtmovie->res->num_sample_byte_sizes = numsizes;
    qtmovie->res->sample_byte_sizes_offset = stream_tell(qtmovie->stream);

    if (size_remaining)
    {
        stream_skip(qtmovie->stream, size_remaining);
    }

    return true;
}

static bool read_chunk_stsc(qtmovie_t *qtmovie, size_t chunk_len)
{
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8;

    /* version + flags */
    stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_sample_to_chunks = numentries;
    qtmovie->res->sample_to_chunk_offset = stream_tell(qtmovie->stream);
    if (size_remaining)
    {
        stream_skip(qtmovie->stream, size_remaining);
    }

    return true;
}

static void add_other_track(qtmovie_t *qtmovie, uint32_t first,
                            uint32_t last)
{
    if (qtmovie->num_other < OTHER_TRACKS_MAX)
    {
        qtmovie->other_first[qtmovie->num_other] = first;
        qtmovie->other_last[qtmovie->num_other] = last;
    }
    qtmovie->num_other++;
}

/* The first and last offset in an stco, or a co64 if wide, whose count
 * has been read.  An offset past 4 GB is past any the audio can have.
 * Leaves the stream after the chunk.  Returns false if it has none. */
static bool read_chunk_range(qtmovie_t *qtmovie, size_t size_remaining,
                             uint32_t count, bool wide,
                             uint32_t *first, uint32_t *last)
{
    const size_t entry = wide ? 8 : 4;
    uint32_t hi = 0;

    if (count == 0 || count > size_remaining / entry)
    {
        stream_skip(qtmovie->stream, size_remaining);
        return false;
    }

    if (wide)
        hi = stream_read_uint32(qtmovie->stream);
    *first = stream_read_uint32(qtmovie->stream);
    if (hi)
        *first = 0xffffffff;
    *last = *first;
    if (count > 1)
    {
        stream_skip(qtmovie->stream, (count - 2) * entry);
        if (wide)
            hi = stream_read_uint32(qtmovie->stream);
        *last = stream_read_uint32(qtmovie->stream);
        if (hi)
            *last = 0xffffffff;
    }
    stream_skip(qtmovie->stream, size_remaining - count * entry);
    return true;
}

static bool read_chunk_stco(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;
    uint32_t first = 0, last = 0;

    /* version + flags */
    stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_chunks = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    /* The offsets are not kept either: see build_seek_index(). */
    qtmovie->res->chunk_offset_offset = stream_tell(qtmovie->stream);

    /* The last audio track is the one played: an earlier one is now one
     * of the others. */
    if (qtmovie->have_audio)
        add_other_track(qtmovie, qtmovie->audio_first, qtmovie->audio_last);
    qtmovie->have_audio = read_chunk_range(qtmovie, size_remaining,
                                           qtmovie->res->num_chunks, false,
                                           &first, &last);
    qtmovie->audio_first = first;
    qtmovie->audio_last = last;

    return true;
}

/* Whether the data of another track is among the audio's chunks, which
 * playback has to step over. */
static bool other_tracks_interleaved(const qtmovie_t *qtmovie)
{
    int i;

    if (qtmovie->num_other > OTHER_TRACKS_MAX)
        return true;                    /* not all known: suppose so */
    for (i = 0; i < qtmovie->num_other; i++)
    {
        if (qtmovie->other_first[i] <= qtmovie->audio_last &&
            qtmovie->other_last[i] >= qtmovie->audio_first)
            return true;
    }
    return false;
}

/* The seek index.
 *
 * The file says where each chunk starts (stco), how many frames each chunk
 * has (stsc) and how long each frame is (stsz).  Seeking to a frame needs
 * its offset, which is its chunk's plus the sizes of the frames before it
 * in the chunk.  Keeping the sizes takes four bytes a frame, and what the
 * chunk table is worth depends on who wrote the file: chunks are a third of
 * a second in some files and minutes in others.
 *
 * So neither is kept.  They are read through once when the file is opened,
 * to make two things:
 *  - lookup_table[], the first frame of each chunk that does not start
 *    where the chunk before it ended, which playback needs to step over
 *    the gap (see m4a_check_sample_offset()).  Usually that is the first
 *    chunk alone, but a file with another track among the audio has one
 *    for nearly every chunk.
 *  - seek_offsets[], the file offset of every seek_step'th frame.  The step
 *    is the smallest that the memory there is allows: one, so that a seek
 *    is exact, for a file short enough, and more for a longer one.  With
 *    48 KB to spare that is every frame up to 4 1/2 minutes of AAC, a third
 *    of a second for an hour and a second for 3 1/3 hours.
 *
 * Both are in one piece of memory, the offsets from its start and the gaps
 * from its end, since how many gaps there are is not known until the end.
 * Playback cannot do without the gaps, so when the two meet it is the
 * offsets that give way: every other one is dropped and the step doubles.
 * That stops when the gaps have three quarters of the memory.  A file with
 * more than that, which is one with another track between the audio chunks
 * and too long for the heap, has only its first ones to begin with.  When
 * playback gets past them, or a seek does, the chunk tables are read again
 * for the chunks from there on (m4a_lookup_refill()): one more visit to the
 * tables for each stretch of the file that the memory covers.
 *
 * The three tables can be megabytes apart, and going from one to another
 * can cost a refill of the playback buffer.  So the two that have an entry
 * a chunk are read many entries at a time, into heap that is not yet in
 * use, and the sizes are read straight through between those.
 *
 * Reading every size takes too long for a very long file on a player that
 * reads slowly: 40 hours of AAC has 13 MB of them.  Past SEEK_SIZES_MAX the
 * sizes are left alone and the index is of chunks instead: lookup_table[]
 * gets the first frame and the offset of every lookup_step'th chunk, as
 * many as there is memory for, and there are no seek_offsets[].  A seek
 * then goes to a chunk and, if every chunk is there, reads the sizes of the
 * frames before the one it wants (see m4a_seek()).  A file whose chunks are
 * too long for that to be quick gets the index of frames all the same.  So
 * does one whose chunks do not all fit, if another track has data among
 * them (other_tracks_interleaved()): playback needs every gap to step over
 * that data. */

/* What to leave of the codec's heap for the decoder. */
#define SEEK_INDEX_RESERVE 8192
/* The first step is chosen to leave room for this many gaps. */
#define SEEK_GAPS_FIRST 64
/* The gaps can have all but one part in this many of the index's memory. */
#define SEEK_GAPS_SHARE 4
/* The most bytes of frame sizes to read when the file is opened. */
#ifndef SEEK_SIZES_MAX
#define SEEK_SIZES_MAX (1024 * 1024)
#endif
/* The most frames a chunk to have, on average, for an index of chunks. */
#define SEEK_CHUNK_FRAMES_MAX 16384

#define BE32(p) (((uint32_t)(p)[0] << 24) | ((uint32_t)(p)[1] << 16) | \
                 ((uint32_t)(p)[2] << 8) | (uint32_t)(p)[3])

/* Up to want bytes of the file at pos, where they lie in the buffer.  They
 * are only good until the next call. */
static const unsigned char *index_peek(stream_t *stream, uint32_t pos,
                                       size_t want, size_t *got)
{
    if (!stream->ci->seek_buffer(pos))
    {
        *got = 0;
        return NULL;
    }
    return stream->ci->request_buffer(got, want);
}

/* Copy bytes of the file at pos to dst.  Returns how many there were. */
static size_t index_load(stream_t *stream, uint32_t pos, unsigned char *dst,
                         size_t bytes)
{
    size_t done = 0;

    while (done < bytes)
    {
        size_t got;
        size_t want = bytes - done > 16384 ? 16384 : bytes - done;
        const unsigned char *p = index_peek(stream, pos + done, want, &got);
        if (!p || !got)
            break;
        if (got > want)
            got = want;
        memcpy(dst + done, p, got);
        done += got;
    }
    return done;
}

/* Go through the sizes of n frames, big-endian at w: add them to *offset
 * and put the offset of every step'th frame in seek[], the next of them
 * being *to_next frames on.  The caller has seen that there is room.
 * Returns how many offsets that was.  This is where the time goes for a
 * long file, so it is kept small. */
static inline uint32_t index_run(const uint32_t *w, uint32_t n,
                                 uint32_t *offset, uint32_t *seek,
                                 uint32_t *to_next, uint32_t step)
{
    uint32_t *s = seek;
    uint32_t off = *offset;
    uint32_t t = *to_next;

    if (step == 1 && t == 0)
    {
        for (; n; n--)
        {
            *s++ = off;
            off += betoh32(*w++);
        }
    }
    else
    {
        while (n)
        {
            uint32_t m;
            if (t == 0)
            {
                *s++ = off;
                t = step;
            }
            m = t < n ? t : n;
            n -= m;
            t -= m;
            do
                off += betoh32(*w++);
            while (--m);
        }
    }
    *offset = off;
    *to_next = t;
    return s - seek;
}

/* How many stsc entries, of 12 bytes, to keep in scratch memory that also
 * has to hold stco entries, of 4. */
static uint32_t index_share(size_t scratch_size, uint32_t nstsc,
                            uint32_t nchunks)
{
    uint64_t sc_bytes = 12 * (uint64_t)nstsc;
    uint32_t n = scratch_size * sc_bytes / (sc_bytes + 4 * (uint64_t)nchunks)
                 / 12;
    if (n < 2)
        n = 2;                      /* an entry and the one after it */
    if (n > (scratch_size - 16) / 12)
        n = (scratch_size - 16) / 12;
    return n;
}

/* The two tables that have an entry a chunk, as much of each as is in the
 * scratch memory. */
struct chunk_tables
{
    stream_t *stream;
    demux_res_t *res;
    unsigned char *sc, *co;         /* the stsc and stco entries held */
    uint32_t sc_room, co_room;      /* how many there is room for */
    uint32_t sc_first, sc_n;
    uint32_t co_n, co_i;
    uint32_t stsc_i, next_first;
    uint32_t per_chunk;             /* frames in the chunk last asked for */
    bool loaded;                    /* the last call read from the file */
    bool counts_only;               /* leave the offsets alone */
};

/* The start of the tables. */
static const chunk_place_t first_chunk = { 1, 0, 0, 1, 0 };

/* place is where to go on from: first_chunk, or what chunk_tables_place()
 * gave.  With its chunk made 0, only the frame counts are read, and
 * chunk_tables_next() gives no offset. */
static void chunk_tables_init(struct chunk_tables *t, qtmovie_t *qtmovie,
                              unsigned char *scratch, size_t scratch_size,
                              const chunk_place_t *place)
{
    demux_res_t *res = qtmovie->res;

    memset(t, 0, sizeof(*t));
    t->stream = qtmovie->stream;
    t->res = res;
    /* the scratch is shared out by how big the two tables are */
    t->sc = scratch;
    t->sc_room = index_share(scratch_size, res->num_sample_to_chunks,
                             res->num_chunks);
    t->co = scratch + t->sc_room * 12;
    t->co_room = (scratch_size - t->sc_room * 12) / 4;
    if (place->chunk == 0)
    {
        /* all of the scratch for the frame counts */
        t->counts_only = true;
        t->sc_room = scratch_size / 12;
    }
    t->stsc_i = place->stsc_i;
    t->next_first = place->next_first;
    t->per_chunk = place->per_chunk;
}

/* Where to go on from to have chunk, the last one asked for, again: its
 * first frame is frame. */
static void chunk_tables_place(const struct chunk_tables *t, uint32_t chunk,
                               uint32_t frame, chunk_place_t *place)
{
    place->chunk = chunk;
    place->frame = frame;
    place->stsc_i = t->stsc_i;
    place->next_first = t->next_first;
    place->per_chunk = t->per_chunk;
}

/* Where chunk starts, in *offset, and how many frames it has, in
 * t->per_chunk.  Chunks are numbered from 1 and have to be asked for in
 * turn.  Returns false if the file could not be read. */
static inline bool chunk_tables_next(struct chunk_tables *t, uint32_t chunk,
                                     uint32_t *offset)
{
    demux_res_t *res = t->res;
    const uint32_t nstsc = res->num_sample_to_chunks;
    const unsigned char *p;

    t->loaded = false;

    /* how many frames this chunk has */
    while (chunk >= t->next_first && t->stsc_i < nstsc)
    {
        /* this entry and, for where it ends, the one after it */
        uint32_t last = t->stsc_i + 1 < nstsc ? t->stsc_i + 1 : t->stsc_i;
        if (t->stsc_i < t->sc_first || last >= t->sc_first + t->sc_n)
        {
            uint32_t n = nstsc - t->stsc_i;
            if (n > t->sc_room)
                n = t->sc_room;
            t->sc_n = index_load(t->stream,
                                 res->sample_to_chunk_offset + 12 * t->stsc_i,
                                 t->sc, 12 * n) / 12;
            t->sc_first = t->stsc_i;
            if (last >= t->sc_first + t->sc_n)
                return false;
            t->loaded = true;
        }
        p = t->sc + 12 * (t->stsc_i - t->sc_first);
        t->per_chunk = BE32(p + 4);
        t->next_first = ++t->stsc_i < nstsc ? BE32(p + 12) : 0xffffffff;
    }

    if (t->counts_only)
    {
        *offset = 0;
        return true;
    }

    /* where it starts */
    if (t->co_i == t->co_n)
    {
        uint32_t n = res->num_chunks - (chunk - 1);
        if (n > t->co_room)
            n = t->co_room;
        t->co_n = index_load(t->stream,
                             res->chunk_offset_offset + 4 * (chunk - 1),
                             t->co, 4 * n) / 4;
        if (!t->co_n)
            return false;
        t->co_i = 0;
        t->loaded = true;
    }
    *offset = BE32(t->co + 4 * t->co_i);
    t->co_i++;
    return true;
}

/* Go through the tables once, filling size bytes at block with
 * seek_offsets[] from the start and lookup_table[] from the end.  scratch
 * is for parts of the chunk tables.  Returns false if the file could not
 * be read. */
static bool seek_index_pass(qtmovie_t *qtmovie, void *block, size_t size,
                            unsigned char *scratch, size_t scratch_size)
{
    demux_res_t *res = qtmovie->res;
    stream_t *stream = qtmovie->stream;
    const uint32_t nframes = res->num_sample_byte_sizes;
    const uint32_t nchunks = res->num_chunks;
    struct chunk_tables tables;
    const unsigned char *sizes = NULL;
    size_t sizes_left = 0;
    uint32_t chunk, frame = 0, to_next = 0, index = 0;
    uint32_t step = res->seek_step;
    uint32_t prev_end = 0xffffffff;
    uint32_t *seek = block;
    sample_offset_t *gaps_end = (sample_offset_t *)((char *)block + size);
    sample_offset_t *gap = gaps_end, *g;

    chunk_tables_init(&tables, qtmovie, scratch, scratch_size,
                      &first_chunk);
    for (chunk = 1; chunk <= nchunks && frame < nframes; chunk++)
    {
        uint32_t offset, k;
        uint32_t per_chunk;

        if (!chunk_tables_next(&tables, chunk, &offset))
            return false;
        if (tables.loaded)
            sizes_left = 0;
        per_chunk = tables.per_chunk;

        if (offset != prev_end && res->lookup_end == 0xffffffff)
        {
            /* a gap: make room for it if there is none, until the gaps
             * have their share of the memory */
            const size_t most = size / SEEK_GAPS_SHARE * (SEEK_GAPS_SHARE - 1);
            const size_t have = (char *)gaps_end - (char *)gap;

            while ((char *)(gap - 1) < (char *)&seek[index] && index > 1 &&
                   have < most)
            {
                for (k = 0; 2 * k < index; k++)
                    seek[k] = seek[2 * k];
                index = k;
                step *= 2;
                to_next = (step - frame % step) % step;
            }
            if ((char *)(gap - 1) >= (char *)&seek[index] && have < most)
            {
                gap--;
                gap->sample = frame;
                gap->offset = offset;
            }
            else
            {
                /* no room: the gaps from here on are read when playback
                 * gets to them */
                DEBUGF("no memory for the gap at frame %lu\n",
                       (unsigned long)frame);
                res->lookup_end = frame;
                chunk_tables_place(&tables, chunk, frame, &res->lookup_next);
            }
        }

        for (k = 0; k < per_chunk && frame < nframes; )
        {
            uint32_t aligned[64];
            const uint32_t *w;
            uint32_t n, room, wrote;

            room = ((char *)gap - (char *)&seek[index]) / sizeof(*seek);
            if (to_next == 0 && room == 0)
            {
                /* an offset is due and there is nowhere to put it: make
                 * the index half as fine, or go without */
                if (index > 1)
                {
                    uint32_t j;
                    for (j = 0; 2 * j < index; j++)
                        seek[j] = seek[2 * j];
                    index = j;
                    step *= 2;
                    to_next = (step - frame % step) % step;
                }
                else
                {
                    to_next = step;
                }
                continue;
            }

            if (sizes_left < 4)
            {
                n = nframes - frame;
                if (n > 1024)
                    n = 1024;
                sizes = index_peek(stream,
                                   res->sample_byte_sizes_offset + 4 * frame,
                                   4 * n, &sizes_left);
                if (!sizes || sizes_left < 4)
                    return false;
            }

            /* as many frames as are here and have room for their offsets */
            n = per_chunk - k;
            if (n > nframes - frame)
                n = nframes - frame;
            if (n > sizes_left / 4)
                n = sizes_left / 4;
            if (room == 0)
            {
                if (n > to_next)
                    n = to_next;
            }
            else if (n / step >= room)
            {
                n = room * step;
            }
            if ((uintptr_t)sizes & 3)
            {
                if (n > sizeof(aligned) / sizeof(aligned[0]))
                    n = sizeof(aligned) / sizeof(aligned[0]);
                memcpy(aligned, sizes, 4 * n);
                w = aligned;
            }
            else
            {
                w = (const uint32_t *)sizes;
            }
            wrote = index_run(w, n, &offset, &seek[index], &to_next, step);
            index += wrote;
            sizes += 4 * n;
            sizes_left -= 4 * n;
            frame += n;
            k += n;
        }
        prev_end = offset;
    }

    /* the gaps were put in last to first */
    res->num_lookup_table = gaps_end - gap;
    res->lookup_room = res->num_lookup_table;
    res->lookup_table = gap;
    for (g = gaps_end - 1; gap < g; gap++, g--)
    {
        sample_offset_t t = *gap;
        *gap = *g;
        *g = t;
    }
    res->seek_offsets = seek;
    res->num_seek_offsets = index;
    res->seek_step = step;
    return true;
}

/* Put the first frame and the offset of every step'th chunk in table[].
 * Returns how many that was, or 0 if the file could not be read. */
static uint32_t chunk_index_pass(qtmovie_t *qtmovie, sample_offset_t *table,
                                 uint32_t step, unsigned char *scratch,
                                 size_t scratch_size)
{
    demux_res_t *res = qtmovie->res;
    const uint32_t nframes = res->num_sample_byte_sizes;
    const uint32_t nchunks = res->num_chunks;
    struct chunk_tables tables;
    uint32_t chunk, offset, frame = 0, to_next = 0, n = 0;

    chunk_tables_init(&tables, qtmovie, scratch, scratch_size,
                      &first_chunk);
    for (chunk = 1; chunk <= nchunks && frame < nframes; chunk++)
    {
        if (!chunk_tables_next(&tables, chunk, &offset))
            return 0;
        if (!tables.per_chunk)
            continue;                   /* nothing to play in it */
        if (to_next == 0)
        {
            table[n].sample = frame;
            table[n].offset = offset;
            n++;
            to_next = step;
        }
        to_next--;
        frame += tables.per_chunk < nframes - frame ? tables.per_chunk
                                                    : nframes - frame;
    }
    return n;
}

static bool build_seek_index(qtmovie_t *qtmovie)
{
    demux_res_t *res = qtmovie->res;
    int32_t resume_at = stream_tell(qtmovie->stream);
    const uint32_t nframes = res->num_sample_byte_sizes;
    const uint32_t nchunks = res->num_chunks;
    uint32_t entries, gaps;
    size_t size, scratch_size, tables;
    unsigned char *scratch;
    void *block;

    res->seek_offsets = NULL;
    res->num_seek_offsets = 0;
    res->seek_step = 1;
    res->lookup_table = NULL;
    res->num_lookup_table = 0;
    res->lookup_step = 0;
    res->lookup_first = 0;
    res->lookup_end = 0xffffffff;
    res->lookup_room = 0;

    if (!nframes || !nchunks || !res->num_sample_to_chunks)
        return true;                    /* nothing to seek in */

    /* What there is, or what every frame and every chunk would take. */
    codec_malloc_spare(&size);
    size = size > SEEK_INDEX_RESERVE + 1024 ? size - SEEK_INDEX_RESERVE
                                            : 1024;
    /* Chunk tables too big for what is left over as scratch memory get up
     * to an eighth of the index's, to be read in fewer pieces. */
    tables = nchunks < size / 4 && res->num_sample_to_chunks < size / 12
             ? 4 * nchunks + 12 * res->num_sample_to_chunks : size;
    if (tables > SEEK_INDEX_RESERVE)
    {
        tables -= SEEK_INDEX_RESERVE;
        size -= tables < size / 8 ? tables : size / 8;
    }

    if (nframes > SEEK_SIZES_MAX / 4 &&
        nframes / nchunks <= SEEK_CHUNK_FRAMES_MAX &&
        (!other_tracks_interleaved(qtmovie) ||
         nchunks <= size / sizeof(sample_offset_t)))
    {
        /* too long to read every size: an index of chunks */
        uint32_t step = 1;

        entries = size / sizeof(sample_offset_t);
        if (nchunks > entries)
            step = (nchunks + entries - 1) / entries;
        entries = (nchunks + step - 1) / step;
        block = malloc(entries * sizeof(sample_offset_t));
        scratch = codec_malloc_spare(&scratch_size);
        if (!block || scratch_size < 256)
        {
            DEBUGF("no memory for the seek index\n");
            return false;
        }
        res->num_lookup_table = chunk_index_pass(qtmovie, block, step,
                                                 scratch, scratch_size);
        if (!res->num_lookup_table)
        {
            DEBUGF("cannot read the tables for the seek index\n");
            return false;
        }
        res->lookup_table = block;
        res->lookup_step = step;
        DEBUGF("seek index: %lu chunks, one in %lu\n",
               (unsigned long)res->num_lookup_table, (unsigned long)step);

        stream_seek(qtmovie->stream, resume_at);
        return true;
    }

    if (nframes < size / sizeof(uint32_t) &&
        nchunks < size / sizeof(sample_offset_t) &&
        nframes * sizeof(uint32_t) + nchunks * sizeof(sample_offset_t) < size)
    {
        size = nframes * sizeof(uint32_t) + nchunks * sizeof(sample_offset_t);
    }
    size &= ~(sizeof(sample_offset_t) - 1);
    block = malloc(size);
    scratch = codec_malloc_spare(&scratch_size);
    if (!block || scratch_size < 256)
    {
        DEBUGF("no memory for the seek index\n");
        return false;
    }

    /* The smallest step whose offsets fit beside a few gaps. */
    gaps = nchunks < SEEK_GAPS_FIRST ? nchunks : SEEK_GAPS_FIRST;
    entries = (size - gaps * sizeof(sample_offset_t)) / sizeof(uint32_t);
    res->seek_step = (nframes + entries - 1) / entries;

    if (!seek_index_pass(qtmovie, block, size, scratch, scratch_size) ||
        !res->num_lookup_table ||
        !res->num_seek_offsets)
    {
        DEBUGF("cannot read the tables for the seek index\n");
        return false;
    }
    DEBUGF("seek index: %lu offsets, a frame in %lu, and %lu gaps\n",
           (unsigned long)res->num_seek_offsets,
           (unsigned long)res->seek_step,
           (unsigned long)res->num_lookup_table);

    stream_seek(qtmovie->stream, resume_at);
    return true;
}

/* Fill lookup_table[] again, with every chunk from the one that frame is in
 * for as many as there is room for, whether it follows on from the chunk
 * before it or not.  The stream is left where it was.
 *
 * This is during playback, with no heap to spare, and going from one table
 * to the other can cost a refill of the playback buffer each time.  So each
 * is read once: the frame counts up to the chunk wanted, then the offsets
 * of the chunks from there, into the far half of lookup_table[] itself,
 * then the frame counts again, to put each offset with its first frame.
 * An entry is twice the size of an offset, so the entries do not catch up
 * with the offsets that are still to be used. */
void m4a_lookup_refill(demux_res_t *res, stream_t *stream, uint32_t frame)
{
    unsigned char scratch[1020];
    qtmovie_t qtmovie = { .stream = stream, .res = res };
    const uint32_t nframes = res->num_sample_byte_sizes;
    const uint32_t nchunks = res->num_chunks;
    const uint32_t resume_at = stream->ci->curpos;
    sample_offset_t *table = res->lookup_table;
    struct chunk_tables tables;
    chunk_place_t place = res->lookup_next;
    unsigned char *raw;
    uint32_t chunk, first, offset, at, per_chunk, n = 0, count, i;

    /* all of the file from here, unless there is more than fits */
    res->lookup_first = frame;
    res->lookup_end = 0xffffffff;
    res->num_lookup_table = 0;
    if (!res->lookup_room)
        return;

    /* The chunk the frame is in: on from where the last ones stopped, if
     * that is not past it. */
    if (place.chunk == 0 || place.frame > frame)
        place = first_chunk;
    first = place.chunk;
    at = place.frame;
    place.chunk = 0;                    /* the frame counts alone */
    chunk_tables_init(&tables, &qtmovie, scratch, sizeof(scratch), &place);
    for (;; first++)
    {
        if (first > nchunks || at >= nframes ||
            !chunk_tables_next(&tables, first, &offset))
        {
            stream->ci->seek_buffer(resume_at);
            return;                     /* past the end: nothing to skip */
        }
        per_chunk = tables.per_chunk < nframes - at ? tables.per_chunk
                                                    : nframes - at;
        if (at + per_chunk > frame && per_chunk)
            break;
        at += per_chunk;
    }
    chunk_tables_place(&tables, 0, at, &place);

    /* the offsets of the chunks from there */
    count = nchunks - first + 1;
    if (count > res->lookup_room)
        count = res->lookup_room;
    raw = (unsigned char *)table + 4 * count;
    count = index_load(stream, res->chunk_offset_offset + 4 * (first - 1),
                       raw, 4 * count) / 4;
    if (!count)
    {
        stream->ci->seek_buffer(resume_at);
        return;                         /* cannot be read: go without */
    }

    /* and the first frame of each */
    chunk_tables_init(&tables, &qtmovie, scratch, sizeof(scratch), &place);
    for (i = 0, chunk = first; i < count && at < nframes; i++, chunk++)
    {
        if (!chunk_tables_next(&tables, chunk, &offset))
            break;                      /* go without the rest */
        offset = BE32(raw + 4 * i);
        per_chunk = tables.per_chunk < nframes - at ? tables.per_chunk
                                                    : nframes - at;
        if (!per_chunk)
            continue;                   /* nothing to play in it */
        table[n].sample = at;
        table[n].offset = offset;
        n++;
        at += per_chunk;
    }
    if (i == count && chunk <= nchunks && at < nframes)
    {
        /* there are more: these end here */
        res->lookup_end = at;
        chunk_tables_place(&tables, chunk, at, &res->lookup_next);
    }
    res->num_lookup_table = n;
    if (n)
        res->lookup_first = table[0].sample;
    DEBUGF("gaps for frames %lu to %lu: %lu chunks\n",
           (unsigned long)res->lookup_first, (unsigned long)res->lookup_end,
           (unsigned long)n);

    stream->ci->seek_buffer(resume_at);
}

static bool read_chunk_stbl(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            DEBUGF("strange size (%lu) for chunk inside stbl\n",
                   (unsigned long)sub_chunk_len);
            return false;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('s','t','s','d'):
            if (!read_chunk_stsd(qtmovie, sub_chunk_len)) {
               return false;
            }
            break;
        case MAKEFOURCC('s','t','t','s'):
            if (!read_chunk_stts(qtmovie, sub_chunk_len))
            {
               return false;
            }
            break;
        case MAKEFOURCC('s','t','s','z'):
            if (!read_chunk_stsz(qtmovie, sub_chunk_len))
            {
               return false;
            }
            break;
        case MAKEFOURCC('s','t','s','c'):
            if (!read_chunk_stsc(qtmovie, sub_chunk_len))
            {
               return false;
            }
            break;
        case MAKEFOURCC('s','t','c','o'):
            if (!read_chunk_stco(qtmovie, sub_chunk_len))
            {
               return false;
            }
            break;
        default:
            /*DEBUGF("(stbl) unknown chunk id: %c%c%c%c\n",
                   SPLITFOURCC(sub_chunk_id));*/
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
        }

        size_remaining -= sub_chunk_len;
    }

    return true;
}

/* The rest of a minf, or of its stbl, that is not the audio: note where the
 * track's chunks lie, from its stco or co64.  Leaves the stream after it. */
static void read_other_minf(qtmovie_t *qtmovie, size_t size_remaining)
{
    while (size_remaining >= 8)
    {
        uint32_t sub_chunk_len = stream_read_uint32(qtmovie->stream);
        fourcc_t sub_chunk_id = stream_read_uint32(qtmovie->stream);
        uint32_t first, last, count;

        if (sub_chunk_len < 8 || sub_chunk_len > size_remaining)
        {
            /* cannot tell where it is: suppose it among the audio */
            add_other_track(qtmovie, 0, 0xffffffff);
            stream_skip(qtmovie->stream, size_remaining - 8);
            return;
        }

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('s','t','b','l'):
            read_other_minf(qtmovie, sub_chunk_len - 8);
            break;
        case MAKEFOURCC('s','t','c','o'):
        case MAKEFOURCC('c','o','6','4'):
            if (sub_chunk_len < 16)
            {
                stream_skip(qtmovie->stream, sub_chunk_len - 8);
                break;
            }
            stream_read_uint32(qtmovie->stream);    /* version + flags */
            count = stream_read_uint32(qtmovie->stream);
            if (read_chunk_range(qtmovie, sub_chunk_len - 16, count,
                                 sub_chunk_id == MAKEFOURCC('c','o','6','4'),
                                 &first, &last))
                add_other_track(qtmovie, first, last);
            break;
        default:
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
            break;
        }

        size_remaining -= sub_chunk_len;
    }
    if (size_remaining)
        stream_skip(qtmovie->stream, size_remaining);
}

static bool read_chunk_minf(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;
    uint32_t i;
    fourcc_t header;

    /* Check for smhd, the only kind of minf we care about. MP4 video files
     * normally put a video trak before the AAC trak. Older Rockbox code
     * treated that vmhd as fatal, which made aac.codec unable to decode the
     * audio from an otherwise ordinary .mp4/.m4v. Skip a non-audio minf and
     * continue looking for the sound track. */
    i = stream_read_uint32(qtmovie->stream);
    if (i < 8 || i > size_remaining)
    {
        DEBUGF("unexpected size in media info: %ld\n", (long)i);
        return false;
    }

    header = stream_read_uint32(qtmovie->stream);
    if (header != MAKEFOURCC('s','m','h','d'))
    {
        stream_skip(qtmovie->stream, i - 8);
        read_other_minf(qtmovie, size_remaining - i);
        return true;
    }
    if (i != 16)
    {
        DEBUGF("unexpected sound header size: %ld\n", (long)i);
        return false;
    }

    /* now skip the rest of the atom */
    stream_skip(qtmovie->stream, 16 - 8);
    size_remaining -= 16;

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);

        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            DEBUGF("strange size (%lu) for chunk inside minf\n",
                   (unsigned long)sub_chunk_len);
            return false;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('s','t','b','l'):
            if (!read_chunk_stbl(qtmovie, sub_chunk_len)) {
                return false;
            }
            break;
        default:
            /*DEBUGF("(minf) unknown chunk id: %c%c%c%c\n",
                   SPLITFOURCC(sub_chunk_id));*/
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
            break;
        }

        size_remaining -= sub_chunk_len;
    }
    return true;
}

static bool read_chunk_mdia(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            DEBUGF("strange size (%lu) for chunk inside mdia\n",
                   (unsigned long)sub_chunk_len);
            return false;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('m','i','n','f'):
            if (!read_chunk_minf(qtmovie, sub_chunk_len)) {
               return false;
            }
            break;
        default:
            /*DEBUGF("(mdia) unknown chunk id: %c%c%c%c\n",
                   SPLITFOURCC(sub_chunk_id));*/
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
            break;
        }

        size_remaining -= sub_chunk_len;
    }
    return true;
}

/* 'trak' - a movie track - contains other atoms */
static bool read_chunk_trak(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            DEBUGF("strange size (%lu) for chunk inside trak\n",
                   (unsigned long)sub_chunk_len);
            return false;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('m','d','i','a'):
            if (!read_chunk_mdia(qtmovie, sub_chunk_len)) {
               return false;
            }
            break;
        default:
            /*DEBUGF("(trak) unknown chunk id: %c%c%c%c\n",
                    SPLITFOURCC(sub_chunk_id));*/
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
            break;
        }

        size_remaining -= sub_chunk_len;
    }
    return true;
}

/* 'moov' movie atom - contains other atoms */
static bool read_chunk_moov(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            DEBUGF("strange size (%lu) for chunk inside moov\n",
                   (unsigned long)sub_chunk_len);
            return false;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('t','r','a','k'):
            if (!read_chunk_trak(qtmovie, sub_chunk_len)) {
               return false;
            }
            break;
        default:
            /*DEBUGF("(moov) unknown chunk id: %c%c%c%c\n",
                    SPLITFOURCC(sub_chunk_id));*/
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
            break;
        }

        size_remaining -= sub_chunk_len;
    }

    /* now that it is known what other tracks there are */
    return build_seek_index(qtmovie);
}

static void read_chunk_mdat(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;

    qtmovie->res->mdat_len = size_remaining;
}

int qtmovie_read(stream_t *file, demux_res_t *demux_res)
{
    qtmovie_t qtmovie;

    /* construct the stream */
    memset(&qtmovie, 0, sizeof(qtmovie));
    qtmovie.stream = file;
    qtmovie.res = demux_res;
    demux_res->stream = file;
    demux_res->lookup_end = 0xffffffff;

    /* read the chunks */
    while (1)
    {
        size_t chunk_len;
        fourcc_t chunk_id;

        chunk_len = stream_read_uint32(qtmovie.stream);
        if (stream_eof(qtmovie.stream))
        {
            if(qtmovie.res->mdat_offset == 0 || qtmovie.res->format == 0)
                return 0;
            stream_seek(qtmovie.stream, qtmovie.res->mdat_offset);
            return 1;
        }

        if (chunk_len == 1)
        {
            //DEBUGF("need 64bit support\n");
            return 0;
        }
        chunk_id = stream_read_uint32(qtmovie.stream);

        //DEBUGF("Found a chunk %c%c%c%c, length=%d\n",SPLITFOURCC(chunk_id),chunk_len);
        switch (chunk_id)
        {
        case MAKEFOURCC('f','t','y','p'):
            read_chunk_ftyp(&qtmovie, chunk_len);
            break;
        case MAKEFOURCC('m','o','o','v'):
            if (!read_chunk_moov(&qtmovie, chunk_len)) {
               return 0;
            }
            break;
        case MAKEFOURCC('m','d','a','t'):
            /* There can be empty mdats before the real one. If so, skip them */
            if (chunk_len == 8)
                break;
            read_chunk_mdat(&qtmovie, chunk_len);
            qtmovie.res->mdat_offset=stream_tell(qtmovie.stream);
            /* If we've already seen the format, assume there's nothing
               interesting after the mdat chunk (the file is "streamable").
               This avoids having to seek, which might cause rebuffering. */
            if(qtmovie.res->format > 0)
                return 1;
            stream_skip(qtmovie.stream, chunk_len - 8);
            break;

            /*  these following atoms can be skipped !!!! */
        case MAKEFOURCC('f','r','e','e'):
            stream_skip(qtmovie.stream, chunk_len - 8);
            break;
        default:
            //DEBUGF("(top) unknown chunk id: %c%c%c%c\n",SPLITFOURCC(chunk_id));
            stream_skip(qtmovie.stream, chunk_len - 8);
            break;
        }

    }
    return 0;
}
