/*
 * ttadec.c
 *
 * Description:  TTAv1 decoder library for HW players
 * Developed by: Alexander Djourik <ald@true-audio.com>
 *               Pavel Zhilin <pzh@true-audio.com>
 *
 * Copyright (c) 2004 True Audio Software. All rights reserved.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the True Audio Software nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "codeclib.h"

#include "ttalib.h"
#include "ttadec.h"
#include "filter.h"

/******************* static variables and structures *******************/

static unsigned char isobuffers[ISO_BUFFERS_SIZE + 4] IBSS_ATTR;
static unsigned char * const iso_buffers_end ICONST_ATTR = isobuffers + ISO_BUFFERS_SIZE;
static unsigned int pcm_buffer_size IBSS_ATTR;

static decoder tta[MAX_NCH]     IBSS_ATTR; /* decoder state */
/* Rockbox speciffic: cache is defined in get_samples() (non static value) */
/* static int     cache[MAX_NCH];   // decoder cache */

static tta_info *ttainfo        IBSS_ATTR; /* currently playing file info */

static unsigned int fframes     IBSS_ATTR; /* number of frames in file */
static unsigned int framelen    IBSS_ATTR; /* the frame length in samples */
static unsigned int lastlen     IBSS_ATTR; /* the length of the last frame in samples */
static unsigned int data_pos    IBSS_ATTR; /* currently playing frame index */
static unsigned int data_cur    IBSS_ATTR; /* the playing position in frame */

static int maxvalue             IBSS_ATTR; /* output data max value */

/* Rockbox speciffic: seek_table is static size */
static unsigned int seek_table[MAX_SEEK_TABLE_SIZE]; /* the playing position table */
static unsigned int st_state;    /* seek table status */

static unsigned int frame_crc32 IBSS_ATTR;
static unsigned int bit_count   IBSS_ATTR;
static unsigned int bit_cache   IBSS_ATTR;
static unsigned char *bitpos    IBSS_ATTR;

/* Rockbox speciffic: deletes read_id3_tags(). */
/* static int read_id3_tags (tta_info *info); */

/********************* rockbox helper functions *************************/

/* emulate stdio functions */
static size_t tta_fread(void *ptr, size_t size, size_t nobj)
{
    size_t read_size;
    unsigned char *buffer = ci->request_buffer(&read_size, size * nobj);

    if (read_size > 0)
    {
        ci->memcpy(ptr, buffer, read_size);
        ci->advance_buffer(read_size);
    }
    return read_size;
}

static int tta_fseek(long offset, int origin)
{
    switch (origin)
    {
        case SEEK_CUR:
            ci->advance_buffer(offset);
            break;
        case SEEK_SET:
            ci->seek_buffer(offset);
            break;
        case SEEK_END:
            ci->seek_buffer(offset + ci->id3->filesize);
            break;
        default:
            return -1;
    }
    return 0;
}

/************************* crc32 functions *****************************/

#define UPDATE_CRC32(x, crc) crc = \
    (((crc>>8) & 0x00FFFFFF) ^ crc32_table[(crc^x) & 0xFF])

static unsigned int 
crc32 (unsigned char *buffer, unsigned int len) {
    unsigned int i;
    unsigned int crc = 0xFFFFFFFF;

    for (i = 0; i < len; i++) UPDATE_CRC32(buffer[i], crc);

    return (crc ^ 0xFFFFFFFF);
}

/************************* bit operations ******************************/

#define GET_BINARY(value, bits) \
    while (bit_count < bits) { \
        if (bitpos == iso_buffers_end) { \
            if (!tta_fread(isobuffers, 1, ISO_BUFFERS_SIZE)) { \
                ttainfo->STATE = READ_ERROR; \
                return -1; \
            } \
            bitpos = isobuffers; \
        } \
        UPDATE_CRC32(*bitpos, frame_crc32); \
        bit_cache |= *bitpos << bit_count; \
        bit_count += 8; \
        bitpos++; \
    } \
    value = bit_cache & bit_mask[bits]; \
    bit_cache >>= bits; \
    bit_count -= bits; \
    bit_cache &= bit_mask[bit_count];

#define GET_UNARY(value) \
    value = 0; \
    while (!(bit_cache ^ bit_mask[bit_count])) { \
        if (bitpos == iso_buffers_end) { \
            if (!tta_fread(isobuffers, 1, ISO_BUFFERS_SIZE)) { \
                ttainfo->STATE = READ_ERROR; \
                return -1; \
            } \
            bitpos = isobuffers; \
        } \
        value += bit_count; \
        bit_cache = *bitpos++; \
        UPDATE_CRC32(bit_cache, frame_crc32); \
        bit_count = 8; \
    } \
    while (bit_cache & 1) { \
        value++; \
        bit_cache >>= 1; \
        bit_count--; \
    } \
    bit_cache >>= 1; \
    bit_count--;

/************************* rice operations ******************************/

static inline int update_rice(int value, adapt *rice, int depth,
                              const unsigned int *shift_table)
{
    if (depth > 0)
    {
        rice->sum1 += value - (rice->sum1 >> 4);
        if (rice->k1 > 0 && rice->sum1 < shift_table[rice->k1])
            rice->k1--;
        else if (rice->sum1 > shift_table[rice->k1 + 1])
            rice->k1++;
        value += *(shift_table + rice->k0 - 4);
    }
    rice->sum0 += value - (rice->sum0 >> 4);
    if (rice->k0 > 0 && rice->sum0 < shift_table[rice->k0])
        rice->k0--;
    else if (rice->sum0 > shift_table[rice->k0 + 1])
        rice->k0++;

    return DEC(value);
}

/************************* buffer functions ******************************/

static void init_buffer_read(void) {
    frame_crc32 = 0xFFFFFFFFUL;
    bit_count = bit_cache = 0;
    bitpos = iso_buffers_end;
}

static int done_buffer_read(void) {
    unsigned int crc32, rbytes;

    frame_crc32 ^= 0xFFFFFFFFUL;
    rbytes = iso_buffers_end - bitpos;

    if (rbytes < sizeof(int)) {
        ci->memcpy(isobuffers, bitpos, 4);
        if (!tta_fread(isobuffers + rbytes, 1, ISO_BUFFERS_SIZE - rbytes))
            return -1;
        bitpos = isobuffers;
    }

    ci->memcpy(&crc32, bitpos, 4);
    crc32 = ENDSWAP_INT32(crc32);
    bitpos += sizeof(int);
    
    if (crc32 != frame_crc32) return -1;

    bit_cache = bit_count = 0;
    frame_crc32 = 0xFFFFFFFFUL;

    return 0;
}

/************************* decoder functions ****************************/

/* rockbox: not used
const char *get_error_str (int error) {
    switch (error) {
    case NO_ERROR:      return "No errors found";
    case OPEN_ERROR:    return "Can't open file";
    case FORMAT_ERROR:  return "Not supported file format";
    case FILE_ERROR:    return "File is corrupted";
    case READ_ERROR:    return "Can't read from file";
    case MEMORY_ERROR:  return "Insufficient memory available";
    default:            return "Unknown error code";
    }
} */

int set_tta_info (tta_info *info)
{
    unsigned int checksum;
    unsigned int datasize;
    unsigned int origsize;
    tta_hdr ttahdr;

    /* clear the memory */
    ci->memset (info, 0, sizeof(tta_info));

    /* skip id3v2 tags */
    tta_fseek(ci->id3->id3v2len, SEEK_SET);

    /* read TTA header */
    if (tta_fread (&ttahdr, 1, sizeof (ttahdr)) == 0) {
        info->STATE = READ_ERROR;
        return -1;
    }

    /* check for TTA3 signature */
    if (ENDSWAP_INT32(ttahdr.TTAid) != TTA1_SIGN) {
        DEBUGF("ID error: %x\n", ENDSWAP_INT32(ttahdr.TTAid));
        info->STATE = FORMAT_ERROR;
        return -1;
    }

    ttahdr.CRC32 = ENDSWAP_INT32(ttahdr.CRC32);
    checksum = crc32((unsigned char *) &ttahdr,
    sizeof(tta_hdr) - sizeof(int));
    if (checksum != ttahdr.CRC32) {
        DEBUGF("CRC error: %x != %x\n", ttahdr.CRC32, checksum);
        info->STATE = FILE_ERROR;
        return -1;
    }

    ttahdr.AudioFormat = ENDSWAP_INT16(ttahdr.AudioFormat);
    ttahdr.NumChannels = ENDSWAP_INT16(ttahdr.NumChannels);
    ttahdr.BitsPerSample = ENDSWAP_INT16(ttahdr.BitsPerSample);
    ttahdr.SampleRate = ENDSWAP_INT32(ttahdr.SampleRate);
    ttahdr.DataLength = ENDSWAP_INT32(ttahdr.DataLength);

    /* check for player supported formats */
    if (ttahdr.AudioFormat != WAVE_FORMAT_PCM ||
        ttahdr.NumChannels > MAX_NCH ||
        ttahdr.BitsPerSample > MAX_BPS ||(
        ttahdr.SampleRate != 16000 &&
        ttahdr.SampleRate != 22050 &&
        ttahdr.SampleRate != 24000 &&
        ttahdr.SampleRate != 32000 &&
        ttahdr.SampleRate != 44100 &&
        ttahdr.SampleRate != 48000 &&
        ttahdr.SampleRate != 64000 &&
        ttahdr.SampleRate != 88200 &&
        ttahdr.SampleRate != 96000)) {
        info->STATE = FORMAT_ERROR;
        DEBUGF("illegal audio format: %d channels: %d samplerate: %d\n",
               ttahdr.AudioFormat, ttahdr.NumChannels, ttahdr.SampleRate);
        return -1;
    }

    /* fill the File Info */
    info->NCH = ttahdr.NumChannels;
    info->BPS = ttahdr.BitsPerSample;
    info->BSIZE = (ttahdr.BitsPerSample + 7)/8;
    info->FORMAT = ttahdr.AudioFormat;
    info->SAMPLERATE = ttahdr.SampleRate;
    info->DATALENGTH = ttahdr.DataLength;
    info->FRAMELEN = (int) MULTIPLY_FRAME_TIME(ttahdr.SampleRate);
    info->LENGTH = ttahdr.DataLength / ttahdr.SampleRate;
    info->DATAPOS = ci->id3->id3v2len;
    info->FILESIZE = ci->id3->filesize;

    datasize = info->FILESIZE - info->DATAPOS;
    origsize = info->DATALENGTH * info->BSIZE * info->NCH;

    /* info->COMPRESS = (double) datasize / origsize; */
    info->BITRATE = (int) ((uint64_t) datasize * info->SAMPLERATE * info->NCH * info->BPS
                                               / (origsize * 1000LL));

    return 0;
}

static void rice_init(adapt *rice, unsigned int k0, unsigned int k1) {
    rice->k0 = k0;
    rice->k1 = k1;
    rice->sum0 = shift_16[k0];
    rice->sum1 = shift_16[k1];
}

static void decoder_init(decoder *tta, int nch, int byte_size) {
    int shift = flt_set[byte_size - 1];
    int i;

    for (i = 0; i < nch; i++) {
        filter_init(&tta[i].fst, shift);
        rice_init(&tta[i].rice, 10, 10);
        tta[i].last = 0;
    }
}

static void seek_table_init (unsigned int *seek_table,
    unsigned int len, unsigned int data_offset) {
    unsigned int *st, frame_len;

    for (st = seek_table; st < (seek_table + len); st++) {
        frame_len = ENDSWAP_INT32(*st);
        *st = data_offset;
        data_offset += frame_len;
    }
}

int set_position (unsigned int pos, enum tta_seek_type type)
{
    unsigned int i;
    unsigned int seek_pos;

    if (type == TTA_SEEK_TIME)
    {
        if (pos >= fframes)
            pos = fframes -1;
    }
    else
    {
        pos -= ttainfo->DATAPOS;
        for (i = 1; i < fframes; i++)
        {
            if (seek_table[i] > pos)
                break;
        }
        pos = i - 1;
    }
    if (!st_state) {
        ttainfo->STATE = FILE_ERROR;
        return -1;
    }
    seek_pos = ttainfo->DATAPOS + seek_table[data_pos = pos];
    if (tta_fseek(seek_pos, SEEK_SET) < 0) {
        ttainfo->STATE = READ_ERROR;
        return -1;
    }

    data_cur = 0;
    framelen = 0;

    /* init bit reader */
    init_buffer_read();
    return data_pos * ttainfo->FRAMELEN;
}

int player_init (tta_info *info) {
    unsigned int checksum;
    unsigned int data_offset;
    unsigned int st_size;

#ifdef CPU_COLDFIRE
    coldfire_set_macsr(0); /* signed integer mode */
#endif

    ttainfo = info;

    framelen = 0;
    data_pos = 0;
    data_cur = 0;

    lastlen = ttainfo->DATALENGTH % ttainfo->FRAMELEN;
    fframes = ttainfo->DATALENGTH / ttainfo->FRAMELEN + (lastlen ? 1 : 0);
    st_size = (fframes + 1) * sizeof(int);

    /*
     * Rockbox speciffic
     *    playable tta file is to MAX_SEEK_TABLE_SIZE frames
     *    about 1:08:15 (frequency 44.1 kHz)
     */
    if (fframes > MAX_SEEK_TABLE_SIZE)
    {
        LOGF("frame is too many: %d > %d", fframes, MAX_SEEK_TABLE_SIZE);
        return -1;
    }

    /* read seek table */
    if (!tta_fread(seek_table, st_size, 1)) {
        ttainfo->STATE = READ_ERROR;
        return -1;
    }

    checksum = crc32((unsigned char *) seek_table, st_size - sizeof(int));
    st_state = (checksum == ENDSWAP_INT32(seek_table[fframes]));
    data_offset = sizeof(tta_hdr) + st_size;

    /* init seek table */
    seek_table_init(seek_table, fframes, data_offset);

    /* init bit reader */
    init_buffer_read();

    /*
     * Rockbox speciffic
     *    because pcm data is int32_t, does not multiply ttainfo->BSIZE.
     */
    pcm_buffer_size = PCM_BUFFER_LENGTH * ttainfo->NCH;
    maxvalue = (1UL << ttainfo->BPS) - 1;

    return 0;
}

/*
 * Rockbox specffic
 *    because the seek table is static size buffer, player_stop() is nooperation function.
 */
void player_stop (void) {
/*
    if (seek_table) {
        free(seek_table);
        seek_table = NULL;
    }
*/
}

/*
 * Rockbox speciffic
 *   with the optimization, the decoding logic is modify a little.
 */
int get_samples (int32_t *buffer) {
    unsigned int k, depth, unary, binary;
    int32_t *p = buffer;
    decoder *dec = tta;
    int value, res;
    int cur_pos = pcm_buffer_size;
    int pcm_shift_bits = TTA_OUTPUT_DEPTH - ttainfo->BPS;
    int pr_bits = (ttainfo->BSIZE == 1)? 4 : 5;
    int cache = 0;  /* decoder cache */

    fltst *fst;
    adapt *rice;

    for (res = 0; --cur_pos >= 0;) {
        fst  = &dec->fst;
        rice = &dec->rice;

        if (data_cur == framelen) {
            if (data_pos == fframes) break;
            if (framelen && done_buffer_read()) {
                if (set_position(data_pos, TTA_SEEK_TIME) < 0) return -1;
                if (res) break;
            }

            if (data_pos == fframes - 1 && lastlen)
                framelen = lastlen;
            else framelen = ttainfo->FRAMELEN;

            decoder_init(tta, ttainfo->NCH, ttainfo->BSIZE);
            data_pos++; data_cur = 0;
        }

        /* decode Rice unsigned */
        GET_UNARY(unary);

        switch (unary) {
        case 0: depth = 0; k = rice->k0; break;
        default:
            depth = 1; k = rice->k1;
            unary--;
        }

        if (k) {
            GET_BINARY(binary, k);
            value = (unary << k) + binary;
        } else value = unary;

        value = update_rice(value, rice, depth, shift_16);

        /* Rockbox specific: the following logic move to update_rice() */
#if 0
        if (depth > 0)
        {
            rice->sum1 += value - (rice->sum1 >> 4);
            if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
                rice->k1--;
            else if (rice->sum1 > shift_16[rice->k1 + 1])
                rice->k1++;
            value += bit_shift[rice->k0];
        }
        rice->sum0 += value - (rice->sum0 >> 4);
        if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
            rice->k0--;
        else if (rice->sum0 > shift_16[rice->k0 + 1])
            rice->k0++;

        value = DEC(value);
#endif

        /* decompress stage 1: adaptive hybrid filter */
        hybrid_filter(fst, &value);

        /* decompress stage 2: fixed order 1 prediction */
        value += PREDICTOR1(dec->last, pr_bits);
        dec->last = value;

        /* check for errors */
        if (abs(value) > maxvalue) {
            unsigned int tail =
                pcm_buffer_size / (ttainfo->BSIZE * ttainfo->NCH) - res;
            ci->memset(buffer, 0, pcm_buffer_size * sizeof(int32_t));
            data_cur += tail; res += tail;
            break;
        }

        /* Rockbox speciffic: Rockbox supports max 2channels */
        if (ttainfo->NCH == 1)
        {
            *p++ = value << pcm_shift_bits;
            data_cur++;
            res++;
        }
        else
        {
            if (dec == tta)
            {
                cache = value;
                dec++;
            }
            else
            {
                value += cache / 2;
                cache = value - cache;
                dec = tta;
                *p++ = cache << pcm_shift_bits;
                *p++ = value << pcm_shift_bits;
                data_cur++;
                res++;
            }
        }
    }

    return res;
}

/* Rockbox speciffic: id3 tags functions delete. */

/* eof */
