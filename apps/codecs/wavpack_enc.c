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

#include "codeclib.h"
#include "libwavpack/wavpack.h"

CODEC_ENC_HEADER

/** Types **/
typedef struct
{
    uint8_t type;       /* Type of metadata */
    uint8_t word_size;  /* Size of metadata in words */
} __attribute__((packed)) WavpackMetadataHeader;

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
/*  unsigned short extra_param_size;   24h - size of extra data                */ 
/*  unsigned char  *extra_params; */
    /* data header */
    uint8_t  data_id[4];      /* 24h - "data" */
    uint32_t data_size;       /* 28h - num_samples*num_channels*bits_per_sample/8 */
/*  unsigned char  *data;              2ch - actual sound data */
} __attribute__((packed));

#define RIFF_FMT_HEADER_SIZE   12 /* format -> format_size */
#define RIFF_FMT_DATA_SIZE     16 /* audio_format -> bits_per_sample */
#define RIFF_DATA_HEADER_SIZE   8 /* data_id -> data_size */

#define PCM_DEPTH_BITS         16
#define PCM_DEPTH_BYTES         2
#define PCM_SAMP_PER_CHUNK   5000
#define PCM_CHUNK_SIZE      (4*PCM_SAMP_PER_CHUNK)

/** Data **/
static int8_t input_buffer[PCM_CHUNK_SIZE*2]     IBSS_ATTR;
static WavpackConfig config                      IBSS_ATTR;
static WavpackContext *wpc;
static int32_t data_size, input_size, input_step IBSS_ATTR;
static int32_t err                               IBSS_ATTR;

static const WavpackMetadataHeader wvpk_mdh =
{
    ID_RIFF_HEADER,
    sizeof (struct riff_header) / sizeof (uint16_t),
};

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

static inline void sample_to_int32_mono(int32_t **src, int32_t **dst)
{
    int32_t t = *(*src)++;
    /* endianness irrelevant */
    t = (int16_t)t + (t >> 16) + err;
    err = t & 1;
    *(*dst)++ = t >> 1;
} /* sample_to_int32_mono */

static inline void sample_to_int32_stereo(int32_t **src, int32_t **dst)
{
    int32_t t = *(*src)++;
#ifdef ROCKBOX_BIG_ENDIAN
    *(*dst)++ = t >> 16, *(*dst)++ = (int16_t)t;
#else
    *(*dst)++ = (int16_t)t, *(*dst)++ = t >> 16;
#endif
} /* sample_to_int32_stereo */

STATICIRAM void chunk_to_int32(int32_t *src) ICODE_ATTR;
STATICIRAM void chunk_to_int32(int32_t *src)
{
    int32_t *src_end, *dst;
#ifdef USE_IRAM
    /* copy to IRAM before converting data */
    dst     = (int32_t *)input_buffer + PCM_SAMP_PER_CHUNK;
    src_end = dst + PCM_SAMP_PER_CHUNK;

    memcpy(dst, src, PCM_CHUNK_SIZE);

    src = dst;
#else
    src_end = src + PCM_SAMP_PER_CHUNK;
#endif

    dst = (int32_t *)input_buffer;

    if (config.num_channels == 1)
    {
        /*
         *  |llllllllllllllll|rrrrrrrrrrrrrrrr| =>
         *  |mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm|
         */
        do
        {
            /* read 10 longs and write 10 longs */
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
            sample_to_int32_mono(&src, &dst);
        }
        while(src < src_end);

        return;
    }
    else
    {
        /*
         *  |llllllllllllllll|rrrrrrrrrrrrrrrr| =>
         *  |llllllllllllllllllllllllllllllll|rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr|
         */
        do
        {
            /* read 10 longs and write 20 longs */
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
            sample_to_int32_stereo(&src, &dst);
        }
        while (src < src_end);

        return;
    }
} /* chunk_to_int32 */

/* called very often - inline */
static inline bool is_file_data_ok(struct enc_file_event_data *data) ICODE_ATTR;
static inline bool is_file_data_ok(struct enc_file_event_data *data)
{
    return data->rec_file >= 0 && (long)data->chunk->flags >= 0;
} /* is_file_data_ok */

/* called very often - inline */
static inline bool on_write_chunk(struct enc_file_event_data *data) ICODE_ATTR;
static inline bool on_write_chunk(struct enc_file_event_data *data)
{
    if (!is_file_data_ok(data))
        return false;

    if (data->chunk->enc_data == NULL)
    {
#ifdef ROCKBOX_HAS_LOGF
        ci->logf("wvpk enc: NULL data");
#endif
        return true;
    }

    /* update timestamp (block_index) */
    ((WavpackHeader *)data->chunk->enc_data)->block_index =
            htole32(data->num_pcm_samples);

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
    if (ci->write(data->rec_file, &wvpk_mdh, sizeof (wvpk_mdh)) 
            != sizeof (wvpk_mdh) ||
        ci->write(data->rec_file, &riff_header, sizeof (riff_header))
            != sizeof (riff_header))
    {
        return false;
    }

    data->new_enc_size += sizeof(wvpk_mdh) + sizeof(riff_header);
    return true;
} /* on_start_file */

static bool on_end_file(struct enc_file_event_data *data)
{
    struct
    {
        WavpackMetadataHeader wpmdh;
        struct riff_header    rhdr;
        WavpackHeader         wph;
    } __attribute__ ((packed)) h;

    uint32_t data_size;

    if (data->rec_file < 0)
        return false; /* file already closed, nothing more we can do */

    /* always _try_ to write the file header, even on error */

    /* read template headers at start */
    if (ci->lseek(data->rec_file, 0, SEEK_SET) != 0 ||
        ci->read(data->rec_file, &h, sizeof (h)) != sizeof (h))
        return false;

    data_size = data->num_pcm_samples*config.num_channels*PCM_DEPTH_BYTES;

    /** "RIFF" header **/
    h.rhdr.riff_size    = htole32(RIFF_FMT_HEADER_SIZE +
            RIFF_FMT_DATA_SIZE + RIFF_DATA_HEADER_SIZE + data_size);

    /* format data */
    h.rhdr.num_channels = htole16(config.num_channels);
    h.rhdr.sample_rate  = htole32(config.sample_rate);
    h.rhdr.byte_rate    = htole32(config.sample_rate*config.num_channels*
                                      PCM_DEPTH_BYTES);
    h.rhdr.block_align  = htole16(config.num_channels*PCM_DEPTH_BYTES);

    /* data header */
    h.rhdr.data_size    = htole32(data_size);

    /** Wavpack header **/
    h.wph.ckSize        = htole32(letoh32(h.wph.ckSize) + sizeof (h.wpmdh)
                                + sizeof (h.rhdr));
    h.wph.total_samples = htole32(data->num_pcm_samples);

    /* MDH|RIFF|WVPK => WVPK|MDH|RIFF */
    if (ci->lseek(data->rec_file, 0, SEEK_SET)
            != 0 ||
        ci->write(data->rec_file, &h.wph, sizeof (h.wph))
            != sizeof (h.wph) ||
        ci->write(data->rec_file, &h.wpmdh, sizeof (h.wpmdh))
            != sizeof (h.wpmdh) ||
        ci->write(data->rec_file, &h.rhdr, sizeof (h.rhdr))
            != sizeof (h.rhdr) ||
        ci->close(data->rec_file) != 0 )
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
        /* write metadata header and RIFF header */
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

static bool init_encoder(void)
{
    struct enc_inputs     inputs;
    struct enc_parameters params;
    
    codec_init();

    if (ci->enc_get_inputs         == NULL ||
        ci->enc_set_parameters     == NULL ||
        ci->enc_get_chunk          == NULL ||
        ci->enc_finish_chunk       == NULL ||
        ci->enc_get_pcm_data       == NULL ||
        ci->enc_unget_pcm_data     == NULL )
        return false;

    ci->enc_get_inputs(&inputs);

    if (inputs.config->afmt != AFMT_WAVPACK)
        return false;

    memset(&config, 0, sizeof (config));
    config.bits_per_sample  = PCM_DEPTH_BITS;
    config.bytes_per_sample = PCM_DEPTH_BYTES;
    config.sample_rate      = inputs.sample_rate;
    config.num_channels     = inputs.num_channels;

    wpc = WavpackOpenFileOutput ();

    if (!WavpackSetConfiguration(wpc, &config, -1))
        return false;

    err = 0;

    /* configure the buffer system */
    params.afmt            = AFMT_WAVPACK;
    input_size             = PCM_CHUNK_SIZE*inputs.num_channels / 2;
    data_size              = 105*input_size / 100;
    input_size            *= 2;
    input_step             = input_size / 4;
    params.chunk_size      = data_size;
    params.enc_sample_rate = inputs.sample_rate;
    params.reserve_bytes   = 0;
    params.events_callback = enc_events_callback;

    ci->enc_set_parameters(&params);

    return true;
} /* init_encoder */

enum codec_status codec_main(void)
{
    /* initialize params and config */
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
        uint8_t *src;

        while ((src = ci->enc_get_pcm_data(PCM_CHUNK_SIZE)) != NULL)
        {
            struct enc_chunk_hdr *chunk;
            bool     abort_chunk;
            uint8_t *dst;
            uint8_t *src_end;

            if(ci->stop_encoder)
                break;

            abort_chunk = true;

            chunk = ci->enc_get_chunk();

            /* reset counts and pointer */
            chunk->enc_size = 0;
            chunk->num_pcm  = 0;
            chunk->enc_data = NULL;

            dst = ENC_CHUNK_SKIP_HDR(dst, chunk);

            WavpackStartBlock(wpc, dst, dst + data_size);

            chunk_to_int32((uint32_t*)src);
            src      = input_buffer;
            src_end  = src + input_size;

            /* encode chunk in four steps yielding between each */
            do
            {
                if (WavpackPackSamples(wpc, (int32_t *)src,
                                       PCM_SAMP_PER_CHUNK/4))
                {
                    chunk->num_pcm += PCM_SAMP_PER_CHUNK/4;
                    ci->yield();
                    /* could've been stopped in some way */
                    abort_chunk = ci->stop_encoder ||
                                  (chunk->flags & CHUNKF_ABORT);
                }

                src += input_step;
            }
            while (!abort_chunk && src < src_end);

            if (!abort_chunk)
            {
                chunk->enc_data = dst;
                if (chunk->num_pcm < PCM_SAMP_PER_CHUNK)
                    ci->enc_unget_pcm_data(PCM_CHUNK_SIZE - chunk->num_pcm*4);
            /* finish the chunk and store chunk size info */
                chunk->enc_size = WavpackFinishBlock(wpc);
                ci->enc_finish_chunk();
            }
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
