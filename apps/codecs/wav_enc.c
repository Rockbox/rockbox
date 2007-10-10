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

struct riff_header
{
    uint8_t  riff_id[4];      /* 00h - "RIFF"                            */
    uint32_t riff_size;       /* 04h - sz following headers + data_size  */
    /* format header */
    uint8_t  format[4];       /* 08h - "WAVE"                            */
    uint8_t  format_id[4];    /* 0Ch - "fmt "                            */
    uint32_t format_size;     /* 10h - 16 for PCM (sz format data)       */
    /* format data */
    uint16_t audio_format;    /* 14h - 1=PCM                             */
    uint16_t num_channels;    /* 16h - 1=M, 2=S, etc.                    */
    uint32_t sample_rate;     /* 18h - HZ                                */
    uint32_t byte_rate;       /* 1Ch - num_channels*sample_rate*bits_per_sample/8 */
    uint16_t block_align;     /* 20h - num_channels*bits_per_samples/8   */
    uint16_t bits_per_sample; /* 22h - 8=8 bits, 16=16 bits, etc.        */
    /* Not for audio_format=1 (PCM) */
/*  unsigned short extra_param_size;   24h - size of extra data          */ 
/*  unsigned char  *extra_params; */
    /* data header */
    uint8_t  data_id[4];      /* 24h - "data" */
    uint32_t data_size;       /* 28h - num_samples*num_channels*bits_per_sample/8 */
/*  unsigned char  *data;              2ch - actual sound data */
} __attribute__((packed));

#define RIFF_FMT_HEADER_SIZE       12 /* format -> format_size */
#define RIFF_FMT_DATA_SIZE         16 /* audio_format -> bits_per_sample */
#define RIFF_DATA_HEADER_SIZE       8 /* data_id -> data_size */

#define PCM_DEPTH_BYTES             2
#define PCM_DEPTH_BITS             16
#define PCM_SAMP_PER_CHUNK       2048
#define PCM_CHUNK_SIZE          (PCM_SAMP_PER_CHUNK*4)

static int      num_channels IBSS_ATTR;
static uint32_t sample_rate;
static uint32_t enc_size;
static int32_t  err          IBSS_ATTR;

static const struct riff_header riff_header =
{
    /* "RIFF" header */
    { 'R', 'I', 'F', 'F' },         /* riff_id          */
    0,                              /* riff_size   (*)  */
    /* format header */ 
    { 'W', 'A', 'V', 'E' },         /* format           */
    { 'f', 'm', 't', ' ' },         /* format_id        */
    H_TO_LE32(16),                  /* format_size      */
    /* format data */
    H_TO_LE16(1),                   /* audio_format     */
    0,                              /* num_channels (*) */
    0,                              /* sample_rate  (*) */
    0,                              /* byte_rate    (*) */
    0,                              /* block_align  (*) */
    H_TO_LE16(PCM_DEPTH_BITS),      /* bits_per_sample  */
    /* data header */
    { 'd', 'a', 't', 'a' },         /* data_id          */
    0                               /* data_size    (*) */
    /* (*) updated during ENC_END_FILE event */
};

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
        ci->logf("wav enc: NULL data");
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

    /* write template header */
    if (ci->write(data->rec_file, &riff_header, sizeof (riff_header))
            != sizeof (riff_header))
    {
        return false;
    }

    data->new_enc_size += sizeof (riff_header);
    return true;
} /* on_start_file */

static bool on_end_file(struct enc_file_event_data *data)
{
    /* update template header */
    struct riff_header hdr;
    uint32_t data_size;

    /* always _try_ to write the file header, even on error */

    if (ci->lseek(data->rec_file, 0, SEEK_SET) != 0 ||
        ci->read(data->rec_file, &hdr, sizeof (hdr)) != sizeof (hdr))
    {
        return false;
    }

    data_size = data->num_pcm_samples*num_channels*PCM_DEPTH_BYTES;

    /* "RIFF" header */
    hdr.riff_size    = htole32(RIFF_FMT_HEADER_SIZE + RIFF_FMT_DATA_SIZE
                               + RIFF_DATA_HEADER_SIZE + data_size);

    /* format data */
    hdr.num_channels = htole16(num_channels);
    hdr.sample_rate  = htole32(sample_rate);
    hdr.byte_rate    = htole32(sample_rate*num_channels* PCM_DEPTH_BYTES);
    hdr.block_align  = htole16(num_channels*PCM_DEPTH_BYTES);

    /* data header */
    hdr.data_size    = htole32(data_size);

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

/* convert native pcm samples to wav format samples */
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
    *(*dst)++ = swap_odd_even_be32((lr1 << 16) | (uint16_t)lr2);
} /* sample_to_mono */

STATICIRAM void chunk_to_wav_format(uint32_t *src, uint32_t *dst) ICODE_ATTR;
STATICIRAM void chunk_to_wav_format(uint32_t *src, uint32_t *dst)
{
    if (num_channels == 1)
    {
        /* On big endian:
         *  |LLLLLLLLllllllll|RRRRRRRRrrrrrrrr|
         *  |LLLLLLLLllllllll|RRRRRRRRrrrrrrrr| =>
         *  |mmmmmmmmMMMMMMMM|mmmmmmmmMMMMMMMM|
         *
         * On little endian:
         *  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR|
         *  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR| =>
         *  |mmmmmmmmMMMMMMMM|mmmmmmmmMMMMMMMM|
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
         *  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR|
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
#else
        /*  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR| =>
         *  |llllllllLLLLLLLL|rrrrrrrrRRRRRRRR|
         */
        ci->memcpy(dst, src, PCM_CHUNK_SIZE);
#endif
    }
} /* chunk_to_wav_format */

static bool init_encoder(void)
{
    struct enc_inputs     inputs;
    struct enc_parameters params;

    if (ci->enc_get_inputs         == NULL ||
        ci->enc_set_parameters     == NULL ||
        ci->enc_get_chunk          == NULL ||
        ci->enc_finish_chunk       == NULL ||
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        ci->enc_pcm_buf_near_empty == NULL ||
#endif
        ci->enc_get_pcm_data       == NULL )
        return false;

    ci->enc_get_inputs(&inputs);

    if (inputs.config->afmt != AFMT_PCM_WAV)
        return false;

    sample_rate  = inputs.sample_rate;
    num_channels = inputs.num_channels;
    err          = 0;

    /* configure the buffer system */
    params.afmt            = AFMT_PCM_WAV;
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
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    bool cpu_boosted;
#endif

    if (!init_encoder())
    {
        ci->enc_codec_loaded = -1;
        return CODEC_ERROR;
    }

    /* main application waits for this flag during encoder loading */
    ci->enc_codec_loaded = 1;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    ci->cpu_boost(true);
    cpu_boosted = true;
#endif

    /* main encoding loop */
    while(!ci->stop_encoder)
    {
        uint32_t *src;

        while ((src = (uint32_t *)ci->enc_get_pcm_data(PCM_CHUNK_SIZE)) != NULL)
        {
            struct enc_chunk_hdr *chunk;

            if (ci->stop_encoder)
                break;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            if (!cpu_boosted && ci->enc_pcm_buf_near_empty() == 0)
            {
                ci->cpu_boost(true);
                cpu_boosted = true;
            }
#endif
            chunk           = ci->enc_get_chunk();
            chunk->enc_size = enc_size;
            chunk->num_pcm  = PCM_SAMP_PER_CHUNK;
            chunk->enc_data = ENC_CHUNK_SKIP_HDR(chunk->enc_data, chunk);

            chunk_to_wav_format(src, (uint32_t *)chunk->enc_data);

            ci->enc_finish_chunk();
            ci->yield();
        }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        if (cpu_boosted && ci->enc_pcm_buf_near_empty() != 0)
        {
            ci->cpu_boost(false);
            cpu_boosted = false;
        }
#endif
        ci->yield();
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if (cpu_boosted) /* set initial boost state */
        ci->cpu_boost(false);
#endif

    /* reset parameters to initial state */
    ci->enc_set_parameters(NULL);

    /* main application waits for this flag during encoder removing */
    ci->enc_codec_loaded = 0;

    return CODEC_OK;
} /* codec_start */

#endif /* ndef SIMULATOR */
