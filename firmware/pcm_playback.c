/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "config.h"
#include "debug.h"
#include "panic.h"
#include <kernel.h>
#ifndef SIMULATOR
#include "cpu.h"
#include "i2c.h"
#include "uda1380.h"
#include "system.h"
#endif
#include "logf.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "pcm_playback.h"
#include "lcd.h"
#include "button.h"
#include "file.h"
#include "buffer.h"
#include "sprintf.h"
#include "button.h"
#include <string.h>

#ifdef HAVE_UDA1380

#define CHUNK_SIZE           32768
/* Must be a power of 2 */
#define NUM_PCM_BUFFERS      (PCMBUF_SIZE / CHUNK_SIZE)
#define NUM_PCM_BUFFERS_MASK (NUM_PCM_BUFFERS - 1)
#define PCM_WATERMARK        (CHUNK_SIZE * 6)
#define PCM_CF_WATERMARK     (PCMBUF_SIZE - CHUNK_SIZE*8)

static bool pcm_playing;
static bool pcm_paused;
static int pcm_freq = 0x6; /* 44.1 is default */

static char *audiobuffer;
static long audiobuffer_pos;
long audiobuffer_free;
static long audiobuffer_fillpos;
static bool boost_mode;

/* Crossfade modes. If CFM_CROSSFADE is selected, normal
 * crossfader will activate. Selecting CFM_FLUSH is a special
 * operation that only overwrites the pcm buffer without crossfading.
 */
enum {
    CFM_CROSSFADE,
    CFM_FLUSH
};

static int crossfade_mode;
static bool crossfade_enabled;
static bool crossfade_active;
static bool crossfade_init;
static int crossfade_pos;
static int crossfade_amount;
static int crossfade_rem;

static char *guardbuf;
static void (*pcm_event_handler)(void);

static unsigned char *next_start;
static long next_size;

static int last_chunksize = 0;

struct pcmbufdesc
{
    void *addr;
    int size;
    void (*callback)(void); /* Call this when the buffer has been played */
} pcmbuffers[NUM_PCM_BUFFERS];

volatile int pcmbuf_read_index;
volatile int pcmbuf_write_index;
int pcmbuf_unplayed_bytes;
int pcmbuf_watermark;
void (*pcmbuf_watermark_callback)(int bytes_left);

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
static void dma_start(const void *addr, long size)
{
    pcm_playing = true;

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    /* Reset the audio FIFO */
    IIS2CONFIG = 0x800;
    EBU1CONFIG = 0x800;

    /* Set up DMA transfer  */
    SAR0 = ((unsigned long)addr); /* Source address */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = size;                  /* Bytes to transfer */

    /* Enable the FIFO and force one write to it */
    IIS2CONFIG = (pcm_freq << 12) | 0x300 | 4 << 2;
    /* Also send the audio to S/PDIF */
    EBU1CONFIG = 7 << 12 |  3 << 8 | 5 << 2;
    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_SINC | DMA_START;
}

void pcm_boost(bool state)
{
    static bool boost_state = false;
    
    if (crossfade_active || boost_mode)
        return ;
        
    if (state != boost_state) {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        cpu_boost(state);
#endif    
        boost_state = state;
    }
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = false;

    DCR0 = 0;
    /* Reset the FIFO */
    IIS2CONFIG = 0x800;
    EBU1CONFIG = 0x800;
    
    pcmbuf_unplayed_bytes = 0;
    last_chunksize = 0;
    audiobuffer_pos = 0;
    audiobuffer_fillpos = 0;
    audiobuffer_free = PCMBUF_SIZE;
    pcmbuf_read_index = 0;
    pcmbuf_write_index = 0;
    next_start = NULL;
    next_size = 0;
    crossfade_init = 0;
    pcm_paused = false;
}

/* sets frequency of input to DAC */
void pcm_set_frequency(unsigned int frequency)
{
    switch(frequency)
    {
    case 11025:
        pcm_freq = 0x4;
        uda1380_set_nsorder(3);
        break;
    case 22050:
        pcm_freq = 0x6;
        uda1380_set_nsorder(3);
        break;
    case 44100:
    default:
        pcm_freq = 0xC;
        uda1380_set_nsorder(5);
        break;
    }
}

/* the registered callback function to ask for more mp3 data */
static void (*callback_for_more)(unsigned char**, long*) = NULL;

int pcm_play_num_used_buffers(void)
{
    return (pcmbuf_write_index - pcmbuf_read_index) & NUM_PCM_BUFFERS_MASK;
}

static void pcm_play_callback(unsigned char** start, long* size)
{
    struct pcmbufdesc *desc = &pcmbuffers[pcmbuf_read_index];
    int sz;

    pcmbuf_unplayed_bytes -= last_chunksize;
    audiobuffer_free += last_chunksize;
    
    
    if(desc->size == 0)
    {
        /* The buffer is finished, call the callback function */
        if(desc->callback)
            desc->callback();

        /* Advance to the next buffer */
        pcmbuf_read_index = (pcmbuf_read_index + 1) & NUM_PCM_BUFFERS_MASK;
        desc = &pcmbuffers[pcmbuf_read_index];
    }
    
    if(pcm_play_num_used_buffers())
    {
        /* Play max 64K at a time */
        sz = MIN(desc->size, 32768);
        *start = desc->addr;
        *size = sz;

        /* Update the buffer descriptor */
        desc->size -= sz;
        desc->addr += sz;

        last_chunksize = sz;
    }
    else
    {
        /* No more buffers */
        *size = 0;
        if (pcm_event_handler)
            pcm_event_handler();
    }
#if 1
    if(pcmbuf_unplayed_bytes <= pcmbuf_watermark)
    {
        if(pcmbuf_watermark_callback)
        {
            pcmbuf_watermark_callback(pcmbuf_unplayed_bytes);
        }
    }
#endif
}

void pcm_play_data(const unsigned char* start, int size,
                   void (*get_more)(unsigned char** start, long* size))
{
    callback_for_more = get_more;
    get_more(&next_start, &next_size);
    dma_start(start, size);
    
    /* Sleep a while, then unmute audio output */
    sleep(HZ/8);
    uda1380_mute(false);
}

void pcm_play_stop(void)
{
    crossfade_active = false;
    pcm_set_boost_mode(false);
    pcm_boost(false);
    if (pcm_playing) {
        uda1380_mute(true);
        sleep(1);
        dma_stop();
    }
}

void pcm_play_pause(bool play)
{
    if(pcm_paused && play && pcmbuf_unplayed_bytes)
    {
        logf("unpause");
        /* Reset chunk size so dma has enough data to fill the fifo. */
        /* This shouldn't be needed anymore. */
        //SAR0 = (unsigned long)next_start;
        //BCR0 = next_size;
        /* Enable the FIFO and force one write to it */
        IIS2CONFIG = (pcm_freq << 12) | 0x300 | 4 << 2;
        EBU1CONFIG = 7 << 12 |  3 << 8 | 5 << 2;
        DCR0 |= DMA_EEXT | DMA_START;
        sleep(HZ/8);
        uda1380_mute(false);
    }
    else if(!pcm_paused && !play)
    {
        logf("pause");
        uda1380_mute(true);
        
        /* Disable DMA peripheral request. */
        DCR0 &= ~DMA_EEXT;
        IIS2CONFIG = 0x800;
        EBU1CONFIG = 0x800;
    }
    pcm_paused = !play;
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

/* DMA0 Interrupt is called when the DMA has finished transfering a chunk */
void DMA0(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA0(void)
{
    int res = DSR0;

    DSR0 = 1;    /* Clear interrupt */
    DCR0 &= ~DMA_EEXT;
    
    /* Stop on error */
    if(res & 0x70)
    {
       dma_stop();
       logf("DMA Error");
    }
    else
    {
        if(next_size)
        {
            SAR0 = (unsigned long)next_start;  /* Source address */
            BCR0 = next_size;                  /* Bytes to transfer */
            DCR0 |= DMA_EEXT;
            if (callback_for_more)
                callback_for_more(&next_start, &next_size);
        }
        else
        {
            /* Finished playing */
            dma_stop();
            logf("DMA No Data");
        }
    }
    
    IPR |= (1<<14); /* Clear pending interrupt request */
}

void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;
    
    uda1380_init();

    BUSMASTER_CTRL = 0x81; /* PARK[1,0]=10 + BCR24BIT */
    DIVR0 = 54;            /* DMA0 is mapped into vector 54 in system.c */
    DMAROUTE = (DMAROUTE & 0xffffff00) | DMA0_REQ_AUDIO_1;
    DMACONFIG = 1;   /* DMA0Req = PDOR3 */

    /* Reset the audio FIFO */
    IIS2CONFIG = 0x800;
    
    /* Enable interrupt at level 7, priority 0 */
    ICR4 = (ICR4 & 0xffff00ff) | 0x00001c00;
    IMR &= ~(1<<14);      /* bit 14 is DMA0 */
    
    pcm_set_frequency(44100);
    
    /* Turn on headphone power with audio output muted. */
    uda1380_mute(true);
    sleep(HZ/4);
    uda1380_enable_output(true);
    
    pcm_play_init();
}

void pcm_play_set_watermark(int numbytes, void (*callback)(int bytes_left))
{
    pcmbuf_watermark = numbytes;
    pcmbuf_watermark_callback = callback;
}

bool pcm_play_add_chunk(void *addr, int size, void (*callback)(void))
{
    /* We don't use the last buffer, since we can't see the difference
       between the full and empty condition */
    if(pcm_play_num_used_buffers() < (NUM_PCM_BUFFERS - 2))
    {
        pcmbuffers[pcmbuf_write_index].addr = addr;
        pcmbuffers[pcmbuf_write_index].size = size;
        pcmbuffers[pcmbuf_write_index].callback = callback;
        pcmbuf_write_index = (pcmbuf_write_index+1) & NUM_PCM_BUFFERS_MASK;
        pcmbuf_unplayed_bytes += size;
        return true;
    }
    else
        return false;
}

void pcm_watermark_callback(int bytes_left)
{
    /* Fill audio buffer by boosting cpu */
    pcm_boost(true);
    if (bytes_left <= CHUNK_SIZE * 2)
        crossfade_active = false;
}

void pcm_set_boost_mode(bool state)
{
    if (state)
        pcm_boost(true);
    boost_mode = state;
}

void audiobuffer_add_event(void (*event_handler)(void))
{
    pcm_event_handler = event_handler;
}

unsigned int audiobuffer_get_latency(void)
{
    int latency;
    
    /* This has to be done better. */
    latency = (PCMBUF_SIZE - audiobuffer_free - CHUNK_SIZE)/4 / (44100/1000);
    if (latency < 0)
        latency = 0;
    
    return latency;
}

bool pcm_is_lowdata(void)
{
    if (!pcm_is_playing() || pcm_paused)
        return false;
    
    if (pcmbuf_unplayed_bytes < PCM_WATERMARK || crossfade_active)
        return true;
        
    return false;
}

bool pcm_crossfade_init(void)
{
    if (PCMBUF_SIZE - audiobuffer_free < CHUNK_SIZE * 8 || !crossfade_enabled) {
        return false;
    }
    logf("crossfading!");
    crossfade_mode = CFM_CROSSFADE;
    crossfade_init = true;
    
    return true;
    
}

/** Initialize a track switch so that audio playback will not stop but
 *  the switch to next track would happen as soon as possible.
 */
void pcm_flush_audio(void)
{
    if (crossfade_init || crossfade_active || !pcm_playing)
        return ;

    crossfade_mode = CFM_FLUSH;
    crossfade_init = true;
}

void pcm_flush_fillpos(void)
{
    if (audiobuffer_fillpos) {
        while (!pcm_play_add_chunk(&audiobuffer[audiobuffer_pos], 
                                   audiobuffer_fillpos, pcm_event_handler)) {
            pcm_boost(false);
            yield();
            /* This is a fatal error situation that should never happen. */
            if (!pcm_playing)
                break ;
        }
        pcm_event_handler = NULL;
        audiobuffer_pos += audiobuffer_fillpos;
        if (audiobuffer_pos >= PCMBUF_SIZE)
            audiobuffer_pos -= PCMBUF_SIZE;
        audiobuffer_free -= audiobuffer_fillpos;
        audiobuffer_fillpos = 0;
    }
}

static void crossfade_start(void)
{
    crossfade_init = 0;
    if (PCMBUF_SIZE - audiobuffer_free < CHUNK_SIZE * 4) {
        if (crossfade_mode == CFM_FLUSH)
            pcm_play_stop();
        return ;
    }
        
    pcm_flush_fillpos();
    pcm_boost(true);
    crossfade_active = true;
    crossfade_pos = audiobuffer_pos;

    switch (crossfade_mode) {
        case CFM_CROSSFADE:
            crossfade_amount = (PCMBUF_SIZE - audiobuffer_free - (CHUNK_SIZE * 2))/2;
            crossfade_rem = crossfade_amount;
            break ;

        case CFM_FLUSH:
            crossfade_amount = (PCMBUF_SIZE - audiobuffer_free - (CHUNK_SIZE * 2))/2;
            crossfade_rem = crossfade_amount;
            break ;
    }
    
    crossfade_pos -= crossfade_amount*2;
    if (crossfade_pos < 0)
        crossfade_pos += PCMBUF_SIZE;
}

static __inline
int crossfade(short *buf, const short *buf2, int length)
{
    int size, i;
    int val1, val2;
    
    size = MIN(length, crossfade_rem);
    switch (crossfade_mode) {
        case CFM_CROSSFADE:
            val1 = (crossfade_rem<<10)/crossfade_amount;
            val2 = ((crossfade_amount-crossfade_rem)<<10)/crossfade_amount;
            
            for (i = 0; i < size; i++) {
                buf[i] = ((buf[i] * val1) + (buf2[i] * val2)) >> 10;
            }
            break ;

        case CFM_FLUSH:
            for (i = 0; i < size; i++) {
                buf[i] = buf2[i];
            }
            //memcpy((char *)buf, (char *)buf2, size*2);
            break ;
    }
    
    crossfade_rem -= size;
    if (crossfade_rem <= 0)
        crossfade_active = false;
    
    return size;
}

inline static bool prepare_insert(long length)
{
    if (crossfade_init)
        crossfade_start();
    
    if (audiobuffer_free < length + audiobuffer_fillpos
           + CHUNK_SIZE && !crossfade_active) {
        pcm_boost(false);
        return false;
    }
    
    if (!pcm_is_playing()) {
        pcm_boost(true);
        crossfade_active = false;
        if (audiobuffer_free < PCMBUF_SIZE - CHUNK_SIZE*4)
            pcm_play_start();
    }
    
    return true;
}

void* pcm_request_buffer(long length, long *realsize)
{
    void *ptr = NULL;
    
    if (!prepare_insert(length)) {
        *realsize = 0;
        return NULL;
    }
    
    if (crossfade_active) {
        *realsize = MIN(length, PCMBUF_GUARD);
        ptr = &guardbuf[0];
    } else {
        *realsize = MIN(length, PCMBUF_SIZE - audiobuffer_pos
                            - audiobuffer_fillpos);
        if (*realsize < length) {
            *realsize += MIN((long)(length - *realsize), PCMBUF_GUARD);
        }
        ptr = &audiobuffer[audiobuffer_pos + audiobuffer_fillpos];
    }
    
    return ptr;
}

bool pcm_is_crossfade_active(void)
{
    return crossfade_active;
}

void pcm_flush_buffer(long length)
{
    int copy_n;
    char *buf;
    
    if (crossfade_active) {
        buf = &guardbuf[0];
        length = MIN(length, PCMBUF_GUARD);
        while (length > 0 && crossfade_active) {
            copy_n = MIN(length, PCMBUF_SIZE - crossfade_pos);
            copy_n = 2 * crossfade((short *)&audiobuffer[crossfade_pos], 
                    (const short *)buf, copy_n/2);
            buf += copy_n;
            length -= copy_n;
            crossfade_pos += copy_n;
            if (crossfade_pos >= PCMBUF_SIZE)
                crossfade_pos -= PCMBUF_SIZE;
        }
        
        while (length > 0) {
            copy_n = MIN(length, PCMBUF_SIZE - audiobuffer_pos);
            memcpy(&audiobuffer[audiobuffer_pos], buf, copy_n);
            audiobuffer_fillpos = copy_n;
            buf += copy_n;
            length -= copy_n;
            if (length > 0)
                pcm_flush_fillpos();
        }
    }
    
    audiobuffer_fillpos += length;
    
    try_flush:
    if (audiobuffer_fillpos < CHUNK_SIZE && PCMBUF_SIZE
        - audiobuffer_pos - audiobuffer_fillpos > 0)
        return ;
    
    copy_n = audiobuffer_fillpos - (PCMBUF_SIZE - audiobuffer_pos);
    if (copy_n > 0) {
        audiobuffer_fillpos -= copy_n;
        pcm_flush_fillpos();
        copy_n = MIN(copy_n, PCMBUF_GUARD);
        memcpy(&audiobuffer[0], &guardbuf[0], copy_n);
        audiobuffer_fillpos = copy_n;
        goto try_flush;
    }
    pcm_flush_fillpos();
}

bool pcm_insert_buffer(char *buf, long length)
{
    long copy_n = 0;
    
    if (!prepare_insert(length))
        return false;

    
    if (crossfade_active) {
        while (length > 0 && crossfade_active) {
            copy_n = MIN(length, PCMBUF_SIZE - crossfade_pos);
                
            copy_n = 2 * crossfade((short *)&audiobuffer[crossfade_pos],
                                    (const short *)buf, copy_n/2);
            buf += copy_n;
            length -= copy_n;
            crossfade_pos += copy_n;
            if (crossfade_pos >= PCMBUF_SIZE)
                crossfade_pos -= PCMBUF_SIZE;
        }
        
        while (length > 0) {
            copy_n = MIN(length, PCMBUF_SIZE - audiobuffer_pos);
            memcpy(&audiobuffer[audiobuffer_pos], buf, copy_n);
            audiobuffer_fillpos = copy_n;
            buf += copy_n;
            length -= copy_n;
            if (length > 0)
                pcm_flush_fillpos();
        }
    }
        
    while (length > 0) {
        copy_n = MIN(length, PCMBUF_SIZE - audiobuffer_pos -
                    audiobuffer_fillpos);
        copy_n = MIN(CHUNK_SIZE - audiobuffer_fillpos, copy_n);

        memcpy(&audiobuffer[audiobuffer_pos+audiobuffer_fillpos],
                buf, copy_n);
        buf += copy_n;
        audiobuffer_fillpos += copy_n;
        length -= copy_n;
        
        /* Pre-buffer to meet CHUNK_SIZE requirement */
        if (audiobuffer_fillpos < CHUNK_SIZE && length == 0) {
            return true;
        }

        pcm_flush_fillpos();
    }

    return true;
}

void pcm_play_init(void)
{
    audiobuffer = &audiobuf[(audiobufend - audiobuf) - 
                            PCMBUF_SIZE - PCMBUF_GUARD];
    guardbuf = &audiobuffer[PCMBUF_SIZE];
    audiobuffer_free = PCMBUF_SIZE;
    audiobuffer_pos = 0;
    audiobuffer_fillpos = 0;
    boost_mode = 0;
    pcmbuf_read_index = 0;
    pcmbuf_write_index = 0;
    pcmbuf_unplayed_bytes = 0;
    crossfade_active = false;
    crossfade_init = false;
    pcm_event_handler = NULL;
}

void pcm_crossfade_enable(bool on_off)
{
    crossfade_enabled = on_off;
}

bool pcm_is_crossfade_enabled(void)
{
    return crossfade_enabled;
}

void pcm_play_start(void)
{
    struct pcmbufdesc *desc = &pcmbuffers[pcmbuf_read_index];
    int size;
    char *start;
    
    if (crossfade_enabled) {
        pcm_play_set_watermark(PCM_CF_WATERMARK, pcm_watermark_callback);
    } else {
        pcm_play_set_watermark(PCM_WATERMARK, pcm_watermark_callback);
    }
    crossfade_active = false;
    
    if(!pcm_is_playing())
    {
        size = MIN(desc->size, 32768);
        start = desc->addr;
        last_chunksize = size;
        desc->size -= size;
        desc->addr += size;
        pcm_play_data(start, size, pcm_play_callback);
    }
}

#endif /* HAVE_UDA1380 */
