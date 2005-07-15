/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "panic.h"
#include <kernel.h>
#include "pcmbuf.h"
#include "pcm_playback.h"
#include "logf.h"
#ifndef SIMULATOR
#include "cpu.h"
#endif
#include "system.h"
#include <string.h>
#include "buffer.h"

#define CHUNK_SIZE           32768
/* Must be a power of 2 */
#define NUM_PCM_BUFFERS      (PCMBUF_SIZE / CHUNK_SIZE)
#define NUM_PCM_BUFFERS_MASK (NUM_PCM_BUFFERS - 1)
#define PCMBUF_WATERMARK     (CHUNK_SIZE * 10)
#define PCMBUF_CF_WATERMARK  (PCMBUF_SIZE - CHUNK_SIZE*8)

/* Audio buffer related settings. */
static char *audiobuffer;
static long audiobuffer_pos;      /* Current audio buffer write index. */
long audiobuffer_free;            /* Amount of bytes left in the buffer. */
static long audiobuffer_fillpos;  /* Amount audiobuffer_pos will be increased. */
static char *guardbuf;

static void (*pcmbuf_event_handler)(void);

/* Crossfade related. */
static int crossfade_mode;
static bool crossfade_enabled;
static bool crossfade_active;
static bool crossfade_init;
static int crossfade_pos;
static int crossfade_amount;
static int crossfade_rem;


static bool boost_mode;

/* Crossfade modes. If CFM_CROSSFADE is selected, normal
 * crossfader will activate. Selecting CFM_FLUSH is a special
 * operation that only overwrites the pcm buffer without crossfading.
 */
enum {
    CFM_CROSSFADE,
    CFM_FLUSH
};

/* Structure we can use to queue pcm chunks in memory to be played
 * by the driver code. */
struct pcmbufdesc
{
    void *addr;
    int size;
    /* Call this when the buffer has been played */
    void (*callback)(void);
} pcmbuffers[NUM_PCM_BUFFERS];

volatile int pcmbuf_read_index;
volatile int pcmbuf_write_index;
int pcmbuf_unplayed_bytes;
int pcmbuf_watermark;
void (*pcmbuf_watermark_event)(int bytes_left);
static int last_chunksize;

static void pcmbuf_boost(bool state)
{
    static bool boost_state = false;
    
    if (crossfade_init || crossfade_active || boost_mode)
        return ;
        
    if (state != boost_state) {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        cpu_boost(state);
#endif    
        boost_state = state;
    }
}

int pcmbuf_num_used_buffers(void)
{
    return (pcmbuf_write_index - pcmbuf_read_index) & NUM_PCM_BUFFERS_MASK;
}

static void pcmbuf_callback(unsigned char** start, long* size)
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
    
    if(pcmbuf_num_used_buffers())
    {
        /* Play max 64K at a time */
        //sz = MIN(desc->size, 32768);
        sz = desc->size;
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
        if (pcmbuf_event_handler)
            pcmbuf_event_handler();
    }

    if(pcmbuf_unplayed_bytes <= pcmbuf_watermark)
    {
        if(pcmbuf_watermark_event)
        {
            pcmbuf_watermark_event(pcmbuf_unplayed_bytes);
        }
    }
}

void pcmbuf_set_watermark(int numbytes, void (*callback)(int bytes_left))
{
    pcmbuf_watermark = numbytes;
    pcmbuf_watermark_event = callback;
}

bool pcmbuf_add_chunk(void *addr, int size, void (*callback)(void))
{
    /* We don't use the last buffer, since we can't see the difference
       between the full and empty condition */
    if(pcmbuf_num_used_buffers() < (NUM_PCM_BUFFERS - 2))
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

void pcmbuf_watermark_callback(int bytes_left)
{
    /* Fill audio buffer by boosting cpu */
    pcmbuf_boost(true);
    if (bytes_left <= CHUNK_SIZE * 2)
        crossfade_active = false;
}

void pcmbuf_set_boost_mode(bool state)
{
    if (state)
        pcmbuf_boost(true);
    boost_mode = state;
}

void pcmbuf_add_event(void (*event_handler)(void))
{
    pcmbuf_event_handler = event_handler;
}

unsigned int pcmbuf_get_latency(void)
{
    int latency;
    
    /* This has to be done better. */
    latency = (PCMBUF_SIZE - audiobuffer_free - CHUNK_SIZE)/4 / (44100/1000);
    if (latency < 0)
        latency = 0;
    
    return latency;
}

bool pcmbuf_is_lowdata(void)
{
    if (!pcm_is_playing() || pcm_is_paused() || crossfade_init || crossfade_active)
        return false;
    
    if (pcmbuf_unplayed_bytes < PCMBUF_WATERMARK)
        return true;
        
    return false;
}

bool pcmbuf_crossfade_init(void)
{
    if (PCMBUF_SIZE - audiobuffer_free < CHUNK_SIZE * 8 || !crossfade_enabled
        || crossfade_active || crossfade_init) {
        return false;
    }
    logf("pcmbuf_crossfade_init");
    pcmbuf_boost(true);
    crossfade_mode = CFM_CROSSFADE;
    crossfade_init = true;
    
    return true;
    
}

void pcmbuf_play_stop(void)
{
    pcm_play_stop();
    last_chunksize = 0;
    pcmbuf_unplayed_bytes = 0;
    pcmbuf_read_index = 0;
    pcmbuf_write_index = 0;
    audiobuffer_pos = 0;
    audiobuffer_fillpos = 0;
    audiobuffer_free = PCMBUF_SIZE;
    crossfade_init = false;
    crossfade_active = false;
    
    pcmbuf_set_boost_mode(false);
    pcmbuf_boost(false);
    
}

void pcmbuf_init(void)
{
    audiobuffer = &audiobuf[(audiobufend - audiobuf) - 
                            PCMBUF_SIZE - PCMBUF_GUARD];
    guardbuf = &audiobuffer[PCMBUF_SIZE];
    pcmbuf_event_handler = NULL;
    pcm_init();
    pcmbuf_play_stop();
}

/** Initialize a track switch so that audio playback will not stop but
 *  the switch to next track would happen as soon as possible.
 */
void pcmbuf_flush_audio(void)
{
    if (crossfade_init || crossfade_active || !pcm_is_playing()) {
        pcmbuf_play_stop();
        return ;
    }
    
    crossfade_mode = CFM_FLUSH;
    crossfade_init = true;
}

void pcmbuf_flush_fillpos(void)
{
    int copy_n;

    copy_n = MIN(audiobuffer_fillpos, CHUNK_SIZE);
    
    if (copy_n) {
        while (!pcmbuf_add_chunk(&audiobuffer[audiobuffer_pos],
                                   copy_n, pcmbuf_event_handler)) {
            pcmbuf_boost(false);
            sleep(1);
            /* This is a fatal error situation that should never happen. */
            if (!pcm_is_playing()) {
                logf("pcm_flush_fillpos error");
                break ;
            }
        }
        pcmbuf_event_handler = NULL;
        audiobuffer_pos += copy_n;
        if (audiobuffer_pos >= PCMBUF_SIZE)
            audiobuffer_pos -= PCMBUF_SIZE;
        audiobuffer_free -= copy_n;
        audiobuffer_fillpos -= copy_n;
    }
}

static void crossfade_start(void)
{
    int bytesleft = pcmbuf_unplayed_bytes;
    
    crossfade_init = 0;
    if (bytesleft < CHUNK_SIZE * 4) {
        logf("crossfade rejected");
        pcmbuf_play_stop();
        return ;
    }

    logf("crossfade_start");
    pcmbuf_flush_fillpos();
    pcmbuf_boost(true);
    crossfade_active = true;
    crossfade_pos = audiobuffer_pos;

    switch (crossfade_mode) {
        case CFM_CROSSFADE:
            crossfade_amount = (bytesleft - (CHUNK_SIZE * 2))/2;
            crossfade_rem = crossfade_amount;
            break ;

        case CFM_FLUSH:
            crossfade_amount = (bytesleft - (CHUNK_SIZE * 2))/2;
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

static bool prepare_insert(long length)
{
    if (crossfade_init)
        crossfade_start();
    
    if (audiobuffer_free < length + audiobuffer_fillpos
           + CHUNK_SIZE && !crossfade_active) {
        pcmbuf_boost(false);
        return false;
    }
    
    if (!pcm_is_playing()) {
        pcmbuf_boost(true);
        crossfade_active = false;
        if (audiobuffer_free < PCMBUF_SIZE - CHUNK_SIZE*4) {
            logf("pcm starting");
            pcm_play_data(pcmbuf_callback);
        }
    }
    
    return true;
}

void* pcmbuf_request_buffer(long length, long *realsize)
{
    void *ptr = NULL;

    if (crossfade_init)
        crossfade_start();
        
    while (audiobuffer_free < length + audiobuffer_fillpos
           + CHUNK_SIZE && !crossfade_active) {
        pcmbuf_boost(false);
        sleep(1);
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

bool pcmbuf_is_crossfade_active(void)
{
    return crossfade_active || crossfade_init;
}

void pcmbuf_flush_buffer(long length)
{
    int copy_n;
    char *buf;

    prepare_insert(length);
    
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
                pcmbuf_flush_fillpos();
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
        pcmbuf_flush_fillpos();
        copy_n = MIN(copy_n, PCMBUF_GUARD);
        memcpy(&audiobuffer[0], &guardbuf[0], copy_n);
        audiobuffer_fillpos = copy_n;
        goto try_flush;
    }
    pcmbuf_flush_fillpos();
}

bool pcmbuf_insert_buffer(char *buf, long length)
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
                pcmbuf_flush_fillpos();
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

        pcmbuf_flush_fillpos();
    }

    return true;
}

void pcmbuf_crossfade_enable(bool on_off)
{
    crossfade_enabled = on_off;
    
    if (crossfade_enabled) {
        pcmbuf_set_watermark(PCMBUF_CF_WATERMARK, pcmbuf_watermark_callback);
    } else {
        pcmbuf_set_watermark(PCMBUF_WATERMARK, pcmbuf_watermark_callback);
    }    
}

bool pcmbuf_is_crossfade_enabled(void)
{
    return crossfade_enabled;
}

