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

#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../codec.h"

#include "m4a.h"

#if defined(DEBUG) || defined(SIMULATOR)
#define DEBUGF qtmovie->stream->ci->debugf
#else
#define DEBUGF(...)
#endif

typedef struct
{
    stream_t *stream;
    demux_res_t *res;
} qtmovie_t;


/* chunk handlers */
static void read_chunk_ftyp(qtmovie_t *qtmovie, size_t chunk_len)
{
    fourcc_t type;
    uint32_t minor_ver;
    size_t size_remaining = chunk_len - 8;

    type = stream_read_uint32(qtmovie->stream);
    size_remaining-=4;
    if ((type != MAKEFOURCC('M','4','A',' ')) &&
        (type != MAKEFOURCC('M','4','B',' ')) &&
        (type != MAKEFOURCC('m','p','4','2')) &&
        (type != MAKEFOURCC('3','g','p','6')) &&
        (type != MAKEFOURCC('q','t',' ',' ')))
    {
        DEBUGF("not M4A file\n");
        return;
    }
    minor_ver = stream_read_uint32(qtmovie->stream);
    size_remaining-=4;

    /* compatible brands */
    while (size_remaining)
    {
        /* unused */
        /*fourcc_t cbrand =*/ stream_read_uint32(qtmovie->stream);
        size_remaining-=4;
    }
}

uint32_t mp4ff_read_mp4_descr_length(stream_t* stream)
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
    int audioType;
    int32_t maxBitrate;
    int32_t avgBitrate;

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

    audioType = stream_read_uint8(qtmovie->stream);
    temp=stream_read_int32(qtmovie->stream);//0x15000414 ????
    maxBitrate = stream_read_int32(qtmovie->stream);
    avgBitrate = stream_read_int32(qtmovie->stream);
    DEBUGF("audioType=%d, maxBitrate=%ld, avgBitrate=%ld\n",audioType,
           (long)maxBitrate,(long)avgBitrate);

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

    if (numentries != 1)
    {
        DEBUGF("only expecting one entry in sample description atom!\n");
        return false;
    }

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

        if ((qtmovie->res->format==MAKEFOURCC('a','l','a','c'))) {
          if (qtmovie->stream->ci->id3->codectype!=AFMT_ALAC) {
               return false;
          }

          /* 12 = audio format atom, 8 = padding */
          qtmovie->res->codecdata_len = entry_remaining + 12 + 8;
          if (qtmovie->res->codecdata_len > MAX_CODECDATA_SIZE)
          {
             DEBUGF("codecdata too large (%d) in stsd\n", 
                    (int)qtmovie->res->codecdata_len);
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

          if (entry_remaining)
              stream_skip(qtmovie->stream, entry_remaining);

        } else if (qtmovie->res->format==MAKEFOURCC('m','p','4','a')) {
          if (qtmovie->stream->ci->id3->codectype!=AFMT_AAC) {
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
             entry_remaining-=sub_chunk_len;
          } else {
              DEBUGF("Error reading esds\n");
              return false;
          }

          DEBUGF("entry_remaining=%ld\n",(long)entry_remaining);
          stream_skip(qtmovie->stream,entry_remaining);

        } else {
            DEBUGF("expecting 'alac' or 'mp4a' data format, got %c%c%c%c\n",
                 SPLITFOURCC(qtmovie->res->format));
            return false;
        }
    }
    return true;
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

    if (!qtmovie->res->time_to_sample)
    {
        DEBUGF("stts too large\n");
        return false;
    }

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

    /* default sample size */
    if (stream_read_uint32(qtmovie->stream) != 0)
    {
        DEBUGF("i was expecting variable samples sizes\n");
        stream_read_uint32(qtmovie->stream);
        size_remaining -= 4;
        return true;
    }
    size_remaining -= 4;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_sample_byte_sizes = numentries;
    qtmovie->res->sample_byte_size = malloc(numentries * sizeof(*qtmovie->res->sample_byte_size));

    if (!qtmovie->res->sample_byte_size)
    {
        DEBUGF("stsz too large\n");
        return false;
    }

    for (i = 0; i < numentries; i++)
    {
        uint32_t v = stream_read_uint32(qtmovie->stream);
        
        if (v > 0x0000ffff)
        {
            DEBUGF("stsz[%d] > 65 kB (%ld)\n", i, (long)v);
            return false;
        }
        
        qtmovie->res->sample_byte_size[i] = v;
        size_remaining -= 4;
    }

    if (size_remaining)
    {
        DEBUGF("ehm, size remianing?\n");
        stream_skip(qtmovie->stream, size_remaining);
    }
    
    return true;
}

static bool read_chunk_stsc(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8;

    /* version + flags */
    stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_sample_to_chunks = numentries;
    qtmovie->res->sample_to_chunk = malloc(numentries *
        sizeof(*qtmovie->res->sample_to_chunk));

    if (!qtmovie->res->sample_to_chunk)
    {
        DEBUGF("stsc too large\n");
        return false;
    }

    for (i = 0; i < numentries; i++)
    {
        qtmovie->res->sample_to_chunk[i].first_chunk = 
            stream_read_uint32(qtmovie->stream);
        qtmovie->res->sample_to_chunk[i].num_samples = 
            stream_read_uint32(qtmovie->stream);
        stream_read_uint32(qtmovie->stream);
        size_remaining -= 12;
    }

    if (size_remaining)
    {
        DEBUGF("ehm, size remianing?\n");
        stream_skip(qtmovie->stream, size_remaining);
    }
    
    return true;
}

static bool read_chunk_stco(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8;

    /* version + flags */
    stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_chunk_offsets = numentries;
    qtmovie->res->chunk_offset = malloc(numentries * 
        sizeof(*qtmovie->res->chunk_offset));

    if (!qtmovie->res->chunk_offset)
    {
        DEBUGF("stco too large\n");
        return false;
    }

    for (i = 0; i < numentries; i++)
    {
        qtmovie->res->chunk_offset[i] = stream_read_uint32(qtmovie->stream);
        size_remaining -= 4;
    }

    if (size_remaining)
    {
        DEBUGF("ehm, size remianing?\n");
        stream_skip(qtmovie->stream, size_remaining);
    }
    
    return true;
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

static bool read_chunk_minf(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8;
    uint32_t i;

    /* Check for smhd, only kind of minf we care about */

    if ((i = stream_read_uint32(qtmovie->stream)) != 16)
    {
        DEBUGF("unexpected size in media info: %ld\n", (long)i);
        stream_skip(qtmovie->stream, size_remaining-4);
        return true;
    }

    if (stream_read_uint32(qtmovie->stream) != MAKEFOURCC('s','m','h','d'))
    {
        DEBUGF("not a sound header! can't handle this.\n");
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
    return true;
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
    qtmovie.stream = file;
    qtmovie.res = demux_res;

    /* read the chunks */
    while (1)
    {
        size_t chunk_len;
        fourcc_t chunk_id;

        chunk_len = stream_read_uint32(qtmovie.stream);
        if (stream_eof(qtmovie.stream))
        {
            return 0;
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
            /* once we hit mdat we stop reading and return.
             * this is on the assumption that there is no furhter interesting
             * stuff in the stream. if there is stuff will fail (:()).
             * But we need the read pointer to be at the mdat stuff
             * for the decoder. And we don't want to rely on fseek/ftell,
             * as they may not always be avilable */
        case MAKEFOURCC('m','d','a','t'):
            read_chunk_mdat(&qtmovie, chunk_len);
            /* Keep track of start of stream in file - used for seeking */
            qtmovie.res->mdat_offset=stream_tell(qtmovie.stream);
            /* There can be empty mdats before the real one. If so, skip them */
            if (qtmovie.res->mdat_len > 0) {
                return 1;
            }
            break;

            /*  these following atoms can be skipped !!!! */
        case MAKEFOURCC('f','r','e','e'):
            stream_skip(qtmovie.stream, chunk_len - 8);
            break;
        default:
            //DEBUGF("(top) unknown chunk id: %c%c%c%c\n",SPLITFOURCC(chunk_id));
            return 0;
        }

    }
    return 0;
}


