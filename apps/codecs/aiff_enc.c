/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Antonius Hellmann
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef SIMULATOR

#include <inttypes.h>
#include "codeclib.h"

CODEC_ENC_HEADER

struct aiff_header
{
    uint8_t   form_id[4];           /* 00h - 'FORM'                          */
    uint32_t  form_size;            /* 04h - size of file - 8                */
    uint8_t   aiff_id[4];           /* 08h - 'AIFF'                          */
    uint8_t   comm_id[4];           /* 0Ch - 'COMM'                          */
    int32_t   comm_size;            /* 10h - num_channels through sample_rate
                                             (18)                            */
    int16_t   num_channels;         /* 14h - 1=M, 2=S, etc.                  */
    uint32_t  num_sample_frames;    /* 16h - num samples for each channel    */
    int16_t   sample_size;          /* 1ah - 1-32 bits per sample            */
    uint8_t   sample_rate[10];      /* 1ch - IEEE 754 80-bit floating point  */
    uint8_t   ssnd_id[4];           /* 26h - "SSND"                          */
    int32_t   ssnd_size;            /* 2ah - size of chunk from offset to
                                             end of pcm data                 */
    uint32_t  offset;               /* 2eh - data offset from end of header  */
    uint32_t  block_size;           /* 32h - pcm data alignment              */
                                    /* 36h */
} __attribute__((packed));

#define PCM_DEPTH_BYTES             2
#define PCM_DEPTH_BITS             16
#define PCM_SAMP_PER_CHUNK       2048
#define PCM_CHUNK_SIZE          (PCM_SAMP_PER_CHUNK*4)

/* Template headers */
struct aiff_header aiff_header =
{
    { 'F', 'O', 'R', 'M' },             /* form_id               */
    0,                                  /* form_size         (*) */
    { 'A', 'I', 'F', 'F' },             /* aiff_id               */
    { 'C', 'O', 'M', 'M' },             /* comm_id               */
    H_TO_BE32(18),                      /* comm_size             */
    0,                                  /* num_channels      (*) */
    0,                                  /* num_sample_frames (*) */
    H_TO_BE16(PCM_DEPTH_BITS),          /* sample_size           */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },   /* sample_rate       (*) */
    { 'S', 'S', 'N', 'D' },             /* ssnd_id               */
    0,                                  /* ssnd_size         (*) */
    H_TO_BE32(0),                       /* offset                */
    H_TO_BE32(0),                       /* block_size            */
};

/* (*) updated when finalizing file */

static int      num_channels IBSS_ATTR;
static uint32_t sample_rate;
static uint32_t enc_size;
static int32_t  err          IBSS_ATTR;

/* convert unsigned 32 bit value to 80-bit floating point number */
STATICIRAM void uint32_h_to_ieee754_extended_be(uint8_t f[10], uint32_t l)
                                                ICODE_ATTR;
STATICIRAM void uint32_h_to_ieee754_extended_be(uint8_t f[10], uint32_t l)
{
    int32_t exp;

    ci->memset(f, 0, 10);

    if (l == 0)
        return;

    for (exp = 30; (l & (1ul << 31)) == 0; exp--)
        l <<= 1;

    /* sign always zero - bit 79 */
    /* exponent is 0-31 (normalized: 31 - shift + 16383) - bits 64-78 */
    f[0] = 0x40;
    f[1] = (uint8_t)exp;
    /* mantissa is value left justified with most significant non-zero
       bit stored in bit 63 - bits 0-63 */
    *(uint32_t *)&f[2] = htobe32(l);
} /* uint32_h_to_ieee754_extended_be */

/* called version often - inline */
static inline bool is_file_data_ok(struct enc_file_event_data *data) ICODE_ATTR;
static inline bool is_file_data_ok(struct enc_file_event_data *data)
{
    return data->rec_file >= 0 && (long)data->chunk->flags >= 0;
} /* is_file_data_ok */

/* called version often - inline */
static inline bool on_write_chunk(struct enc_file_event_data *data) ICODE_ATTR;
static inline bool on_write_chunk(struct enc_file_event_data *data)
{
    if (!is_file_data_ok(data))
        return false;

    if (data->chunk->enc_data == NULL)
    {
#ifdef ROCKBOX_HAS_LOGF
        ci->logf("aiff enc: NULL data");
#endif
        return true;
    }

    if (ci->write(data->rec_file, data->chunk->enc_data,
                  data->chunk->enc_size) != (ssize_t)data->chunk->enc_size)
        return false;

    data->num_pcm_samples += data->chunk->num_pcm;
    return true;
} /* on_write_chunk */

static bool on_start_file(struct enc_file_event_data *data)
{
    if ((data->chunk->flags & CHUNKF_ERROR) || *data->filename == '\0')
        return false;

    data->rec_file = ci->open(data->filename, O_RDWR|O_CREAT|O_TRUNC);

    if (data->rec_file < 0)
        return false;

    /* reset sample count */
    data->num_pcm_samples = 0;

    /* write template headers */
    if (ci->write(data->rec_file, &aiff_header, sizeof (aiff_header))
            != sizeof (aiff_header))
    {
        return false;
    }

    data->new_enc_size += sizeof(aiff_header);
    return true;
} /* on_start_file */

static bool on_end_file(struct enc_file_event_data *data)
{
    /* update template headers */
    struct aiff_header hdr;
    uint32_t data_size;

    if (!is_file_data_ok(data))
        return false;

    if (ci->lseek(data->rec_file, 0, SEEK_SET) != 0 ||
        ci->read(data->rec_file, &hdr, sizeof (hdr)) != sizeof (hdr))
    {
        return false;
    }

    data_size = data->num_pcm_samples*num_channels*PCM_DEPTH_BYTES;

    /* 'FORM' chunk */
    hdr.form_size         = htobe32(data_size + sizeof (hdr) - 8);

    /* 'COMM' chunk */
    hdr.num_channels      = htobe16(num_channels);
    hdr.num_sample_frames = htobe32(data->num_pcm_samples);
    uint32_h_to_ieee754_extended_be(hdr.sample_rate, sample_rate);

    /* 'SSND' chunk */
    hdr.ssnd_size         = htobe32(data_size + 8);

    if (ci->lseek(data->rec_file, 0, SEEK_SET) != 0 ||
        ci->write(data->rec_file, &hdr, sizeof (hdr)) != sizeof (hdr) ||
        ci->close(data->rec_file) != 0)
    {
        return false;
    }

    data->rec_file = -1;

    return true;
} /* on_end_file */

STATICIRAM void enc_events_callback(enum enc_events event, void *data)
                                    ICODE_ATTR;
STATICIRAM void enc_events_callback(enum enc_events event, void *data)
{
    if (event == ENC_WRITE_CHUNK)
    {
        if (on_write_chunk((struct enc_file_event_data *)data))
            return;
    }
    else if (event == ENC_START_FILE)
    {
        if (on_start_file((struct enc_file_event_data *)data))
            return;
    }
    else if (event == ENC_END_FILE)
    {
        if (on_end_file((struct enc_file_event_data *)data))
            return;
    }
    else
    {
        return;
    }

    ((struct enc_file_event_data *)data)->chunk->flags |= CHUNKF_ERROR;
} /* enc_events_callback */

/* convert native pcm samples to aiff format samples */
static inline void sample_to_mono(uint32_t **src, uint32_t **dst)
{
    int32_t lr1, lr2;

    lr1 = *(*src)++;
    lr1 = (int16_t)lr1 + (lr1 >> 16) + err;
    err = lr1 & 1;
    lr1 >>= 1;

    lr2 = *(*src)++;
    lr2 = (int16_t)lr2 + (lr2 >> 16) + err;
    err = lr2 & 1;
    lr2 >>= 1;
    *(*dst)++ = swap_odd_even_le32((lr1 << 16) | (uint16_t)lr2);
} /* sample_to_mono */

STATICIRAM void chunk_to_aiff_format(uint32_t *src, uint32_t *dst) ICODE_ATTR;
STATICIRAM void chunk_to_aiff_format(uint32_t *src, uint32_t *dst)
{
    if (num_channels == 1)
    {
        /* On big endian:
         *  |LLLLLLLLllllllll|RRRRRRRRrrrrrrrr|
         *  |LLLLLLLLllllllll|RRRRRRRRrrrrrrrr| =>
         *  |MMMMMMMMmmmmmmmm|MMMMMMMMmmmmmmmm|
         *
         * On little endian:
         *  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR|
         *  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR| =>
         *  |MMMMMMMMmmmmmmmm|MMMMMMMMmmmmmmmm|
         */
        uint32_t *src_end = src + PCM_SAMP_PER_CHUNK;

        do
        {
            sample_to_mono(&src, &dst);
            sample_to_mono(&src, &dst);
            sample_to_mono(&src, &dst);
            sample_to_mono(&src, &dst);
            sample_to_mono(&src, &dst);
            sample_to_mono(&src, &dst);
            sample_to_mono(&src, &dst);
            sample_to_mono(&src, &dst);
        }
        while (src < src_end);
    }
    else
    {
#ifdef ROCKBOX_BIG_ENDIAN
        /*  |LLLLLLLLllllllll|RRRRRRRRrrrrrrrr| =>
         *  |LLLLLLLLllllllll|RRRRRRRRrrrrrrrr|
         */
        ci->memcpy(dst, src, PCM_CHUNK_SIZE);
#else
        /*  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR| =>
         *  |LLLLLLLLllllllll|RRRRRRRRrrrrrrrr|
         */
        uint32_t *src_end = src + PCM_SAMP_PER_CHUNK;

        do
        {
            *dst++ = swap_odd_even32(*src++);
            *dst++ = swap_odd_even32(*src++);
            *dst++ = swap_odd_even32(*src++);
            *dst++ = swap_odd_even32(*src++);
            *dst++ = swap_odd_even32(*src++);
            *dst++ = swap_odd_even32(*src++);
            *dst++ = swap_odd_even32(*src++);
            *dst++ = swap_odd_even32(*src++);
        }
        while (src < src_end);
#endif
    }
} /* chunk_to_aiff_format */

static bool init_encoder(void)
{
    struct enc_inputs     inputs;
    struct enc_parameters params;

    if (ci->enc_get_inputs         == NULL ||
        ci->enc_set_parameters     == NULL ||
        ci->enc_get_chunk          == NULL ||
        ci->enc_finish_chunk       == NULL ||
        ci->enc_get_pcm_data       == NULL )
        return false;

    ci->enc_get_inputs(&inputs);

    if (inputs.config->afmt != AFMT_AIFF)
        return false;

    sample_rate  = inputs.sample_rate;
    num_channels = inputs.num_channels;
    err          = 0;

    /* configure the buffer system */
    params.afmt            = AFMT_AIFF;
    enc_size               = PCM_CHUNK_SIZE*inputs.num_channels / 2;
    params.chunk_size      = enc_size;
    params.enc_sample_rate = sample_rate;
    params.reserve_bytes   = 0;
    params.events_callback = enc_events_callback;
    ci->enc_set_parameters(&params);

    return true;
} /* init_encoder */

/* main codec entry point */
enum codec_status codec_main(void)
{
    if (!init_encoder())
    {
        ci->enc_codec_loaded = -1;
        return CODEC_ERROR;
    }

    /* main application waits for this flag during encoder loading */
    ci->enc_codec_loaded = 1;

    /* main encoding loop */
    while(!ci->stop_encoder)
    {
        uint32_t *src;

        while ((src = (uint32_t *)ci->enc_get_pcm_data(PCM_CHUNK_SIZE)) != NULL)
        {
            struct enc_chunk_hdr *chunk;

            if (ci->stop_encoder)
                break;

            chunk           = ci->enc_get_chunk();
            chunk->enc_size = enc_size;
            chunk->num_pcm  = PCM_SAMP_PER_CHUNK;
            chunk->enc_data = ENC_CHUNK_SKIP_HDR(chunk->enc_data, chunk);

            chunk_to_aiff_format(src, (uint32_t *)chunk->enc_data);

            ci->enc_finish_chunk();
            ci->yield();
        }

        ci->yield();
    }

    /* reset parameters to initial state */
    ci->enc_set_parameters(NULL);
 
    /* main application waits for this flag during encoder removing */
    ci->enc_codec_loaded = 0;

    return CODEC_OK;
} /* codec_start */

#endif /* ndef SIMULATOR */
