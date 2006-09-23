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

CODEC_HEADER

typedef unsigned  long uint32;
typedef unsigned short uint16;
typedef unsigned  char  uint8;

static unsigned char wav_header_ster [46] =
{33,22,'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,
 0,0,0,1,0,2,0,0x44,0xac,0,0,0x10,0xb1,2,0,4,0,16,0,'d','a','t','a',0,0,0,0};

static unsigned char wav_header_mono   [46] =
{33,22,'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,
 0,0,0,1,0,1,0,0x44,0xac,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};

static struct codec_api *ci;
static int              enc_channels;

#define CHUNK_SIZE 20000

static long input_buffer[CHUNK_SIZE/2] IBSS_ATTR;

/* update file header info callback function */
void enc_set_header(void *head_buffer,    /* ptr to the file header data     */
                    int  head_size,       /* size of this header data        */
                    int  num_pcm_sampl,   /* amount of processed pcm samples */
                    bool is_file_header)  /* update file/chunk header        */
{
    if(is_file_header)
    {
        /* update file header before file closing */
        if(sizeof(WavpackHeader) + sizeof(wav_header_mono) < (unsigned)head_size)
        {
            char* riff_header    = (char*)head_buffer + sizeof(WavpackHeader);
            char* wv_header      = (char*)head_buffer + sizeof(wav_header_mono);
            int   num_file_bytes = num_pcm_sampl * 2 * enc_channels;
            unsigned long ckSize;

            /* RIFF header and WVPK header have to be swapped */
            /* copy wavpack header to file start position */
            ci->memcpy(head_buffer, wv_header, sizeof(WavpackHeader));
            wv_header = head_buffer; /* recalc wavpack header position */

            if(enc_channels == 2)
                ci->memcpy(riff_header, wav_header_ster, sizeof(wav_header_ster));
            else
                ci->memcpy(riff_header, wav_header_mono, sizeof(wav_header_mono));

            /* update the Wavpack header first chunk size & total frame count */
            ckSize = htole32(((WavpackHeader*)wv_header)->ckSize)
                   + sizeof(wav_header_mono);
            ((WavpackHeader*)wv_header)->total_samples = htole32(num_pcm_sampl);
            ((WavpackHeader*)wv_header)->ckSize        = htole32(ckSize);

            /* update the RIFF WAV header size entries */
            *(long*)(riff_header+ 6) = htole32(num_file_bytes + 36);
            *(long*)(riff_header+42) = htole32(num_file_bytes);
        }
    }
    else
    {
        /* update timestamp (block_index) */
        ((WavpackHeader*)head_buffer)->block_index = htole32(num_pcm_sampl);
    }
}


enum codec_status codec_start(struct codec_api* api)
{
    int            i;
    long           t;
    uint32         *src;
    uint32         *dst;
    int            chunk_size, num_chunks, samp_per_chunk;
    int            enc_buffer_size;
    int            enc_quality;
    WavpackConfig  config;
    WavpackContext *wpc;
    bool           cpu_boosted = true; /* start boosted */

    ci = api; // copy to global api pointer
    
    codec_init(ci);

    if(ci->enc_get_inputs          == NULL ||
       ci->enc_set_parameters      == NULL ||
       ci->enc_alloc_chunk         == NULL ||
       ci->enc_free_chunk          == NULL ||
       ci->enc_wavbuf_near_empty   == NULL ||
       ci->enc_get_wav_data        == NULL ||
       ci->enc_set_header_callback == NULL )
        return CODEC_ERROR;

    ci->cpu_boost(true);

    *ci->enc_set_header_callback = enc_set_header;
    ci->enc_get_inputs(&enc_buffer_size, &enc_channels, &enc_quality);

    /* configure the buffer system */
    chunk_size     = sizeof(long) + CHUNK_SIZE * enc_channels / 2;
    num_chunks     = enc_buffer_size / chunk_size;
    samp_per_chunk = CHUNK_SIZE / 4;

    /* inform the main program about buffer dimensions and other params */
    /* add wav_header_mono as place holder to file start position */
    /* wav header and wvpk header have to be reordered later */
    ci->enc_set_parameters(chunk_size, num_chunks, samp_per_chunk,
                           wav_header_mono, sizeof(wav_header_mono),
                           AFMT_WAVPACK);

    wpc = WavpackOpenFileOutput ();

    memset (&config, 0, sizeof (config));
    config.bits_per_sample  = 16;
    config.bytes_per_sample = 2;
    config.sample_rate      = 44100;
    config.num_channels     = enc_channels;

    if (!WavpackSetConfiguration (wpc, &config, 1))
        return CODEC_ERROR;

    /* main application waits for this flag during encoder loading */
    ci->enc_codec_loaded = true;

    /* main encoding loop */
    while(!ci->stop_codec)
    {
        while((src = (uint32*)ci->enc_get_wav_data(CHUNK_SIZE)) != NULL)
        {
            if(ci->stop_codec)
                break;

            if(ci->enc_wavbuf_near_empty() == 0)
            {
                if(!cpu_boosted)
                {
                    ci->cpu_boost(true);
                    cpu_boosted = true;
                }
            }

            dst = (uint32*)ci->enc_alloc_chunk() + 1;

            WavpackStartBlock (wpc, (uint8*)dst, (uint8*)dst + CHUNK_SIZE);

            if(enc_channels == 2)
            {
                for (i=0; i<CHUNK_SIZE/4; i++)
                {
                    t = (long)*src++;

                    input_buffer[2*i + 0] = t >> 16;
                    input_buffer[2*i + 1] = (short)t;
                }
            }
            else
            {
                for (i=0; i<CHUNK_SIZE/4; i++)
                {
                    t = (long)*src++;
                    t = (((t<<16)>>16) + (t>>16)) >> 1; /* left+right */

                    input_buffer[i] = t;
                }
            }

            if (!WavpackPackSamples (wpc, input_buffer, CHUNK_SIZE/4))
                return CODEC_ERROR;

            /* finish the chunk and store chunk size info */
            dst[-1] = WavpackFinishBlock (wpc);

            ci->enc_free_chunk();
            ci->yield();
        }

        if(ci->enc_wavbuf_near_empty())
        {
            if(cpu_boosted)
            {
                ci->cpu_boost(false);
                cpu_boosted = false;
            }
        }
        ci->yield();
    }

    if(cpu_boosted) /* set initial boost state */
        ci->cpu_boost(false);

    /* reset parameters to initial state */
    ci->enc_set_parameters(0, 0, 0, 0, 0, 0);
 
    /* main application waits for this flag during encoder removing */
    ci->enc_codec_loaded = false;

    return CODEC_OK;
}
#endif
