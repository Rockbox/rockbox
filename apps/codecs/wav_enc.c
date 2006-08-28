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

CODEC_HEADER

static struct codec_api *ci;
static    int enc_channels;

#define CHUNK_SIZE 8192

static unsigned char wav_header[44] =
{'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,
 0,0,0,1,0,2,0,0x44,0xac,0,0,0x10,0xb1,2,0,4,0,16,0,'d','a','t','a',0,0,0,0};

static unsigned char wav_header_mono[44] =
{'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,
 0,0,0,1,0,1,0,0x44,0xac,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};

/* update file header info callback function (called by main application)  */
void enc_set_header(void *head_buffer,  /* ptr to the file header data     */
                   int head_size,       /* size of this header data        */
                   int num_pcm_samples, /* amount of processed pcm samples */
                   bool is_file_header)
{
    int num_file_bytes = num_pcm_samples * 2 * enc_channels;

    if(is_file_header)
    {
        /* update file header before file closing */
        if((int)sizeof(wav_header) < head_size)
        {
            /* update wave header size entries: special to WAV format */
            *(long*)(head_buffer+ 4) = htole32(num_file_bytes + 36);
            *(long*)(head_buffer+40) = htole32(num_file_bytes);
        }
    }
}

/* main codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    int            i;
    long           lr;
    unsigned long  t;
    unsigned long  *src;
    unsigned long  *dst;
    int            chunk_size, num_chunks, samp_per_chunk;
    int            enc_buffer_size;
    int            enc_quality;
    bool           cpu_boosted = true; /* start boosted */

    ci = api; // copy to global api pointer

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
    ci->enc_set_parameters(chunk_size, num_chunks, samp_per_chunk,
        (enc_channels == 2) ? wav_header : wav_header_mono,
        sizeof(wav_header), AFMT_PCM_WAV);

    /* main application waits for this flag during encoder loading */
    ci->enc_codec_loaded = true;

    /* main encoding loop */
    while(!ci->stop_codec)
    {
        while((src = (unsigned long*)ci->enc_get_wav_data(CHUNK_SIZE)) != NULL)
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

            dst = (unsigned long*)ci->enc_alloc_chunk();
            *dst++ = CHUNK_SIZE * enc_channels / 2; /* set size info */

            if(enc_channels == 2)
            {
                /* swap byte order & copy to destination */
                for (i=0; i<CHUNK_SIZE/4; i++)
                {
                    t = *src++;
                    *dst++ = ((t >> 8) & 0xff00ff) | ((t << 8) & 0xff00ff00);
                }
            }
            else
            {
                /* mix left/right, swap byte order & copy to destination */
                for (i=0; i<CHUNK_SIZE/8; i++)
                {
                    lr = (long)*src++;
                    lr = (((lr<<16)>>16) + (lr>>16)) >> 1; /* left+right */
                    t  = (lr << 16);
                    lr = (long)*src++;
                    lr = (((lr<<16)>>16) + (lr>>16)) >> 1; /* left+right */
                    t |= lr & 0xffff;
                    *dst++ = ((t >> 8) & 0xff00ff) | ((t << 8) & 0xff00ff00);
                }
            }

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
