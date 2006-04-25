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
#include "settings.h"
#include "audio.h"
#include "dsp.h"

#define PCMBUF_WATERMARK     (NATIVE_FREQUENCY * 4 * 1)

/* Structure we can use to queue pcm chunks in memory to be played
 * by the driver code. */
struct pcmbufdesc
{
    void *addr;
    size_t size;
    struct pcmbufdesc* link;
    /* Call this when the buffer has been played */
    void (*callback)(void);
};

/* Size of the PCM buffer. */
static size_t pcmbuf_size IDATA_ATTR = 0;

static char *audiobuffer IDATA_ATTR;
/* Current audio buffer write index. */
static size_t audiobuffer_pos IDATA_ATTR;
/* Amount audiobuffer_pos will be increased.*/
static size_t audiobuffer_fillpos IDATA_ATTR;
static char *fadebuf IDATA_ATTR;
static char *voicebuf IDATA_ATTR;

static void (*pcmbuf_event_handler)(void) IDATA_ATTR;
static void (*position_callback)(size_t size) IDATA_ATTR;

/* Crossfade related state */
static bool crossfade_enabled;
static bool crossfade_mix;
static bool crossfade_active IDATA_ATTR;
static bool crossfade_init IDATA_ATTR;

/* Track the current location for processing crossfade */
static struct pcmbufdesc *crossfade_chunk IDATA_ATTR;
static size_t crossfade_sample IDATA_ATTR;

/* Counters for fading in new data */
static size_t crossfade_fade_in_total IDATA_ATTR;
static size_t crossfade_fade_in_rem IDATA_ATTR;

static size_t pcmbuf_descsize;
static struct pcmbufdesc *pcmbuf_read IDATA_ATTR;
static struct pcmbufdesc *pcmbuf_read_end IDATA_ATTR;
static struct pcmbufdesc *pcmbuf_write IDATA_ATTR;
static struct pcmbufdesc *pcmbuf_write_end IDATA_ATTR;
static size_t last_chunksize IDATA_ATTR;

static size_t pcmbuf_unplayed_bytes IDATA_ATTR;
static size_t pcmbuf_watermark IDATA_ATTR;

static struct pcmbufdesc *pcmbuf_mix_chunk IDATA_ATTR;
static size_t pcmbuf_mix_sample IDATA_ATTR;

static bool low_latency_mode = false;
static bool pcmbuf_flush;

/* Helpful macros for use in conditionals this assumes some of the above
 * static variable names */
#define NEED_FLUSH(position) \
    (audiobuffer_fillpos > PCMBUF_TARGET_CHUNK || position >= pcmbuf_size)
#define LOW_DATA(quarter_secs) \
    (pcmbuf_unplayed_bytes < NATIVE_FREQUENCY * quarter_secs)

static void pcmbuf_under_watermark(void);
static bool pcmbuf_flush_fillpos(void);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && !defined(SIMULATOR)
void pcmbuf_boost(bool state)
{
    static bool boost_state = false;

    if (crossfade_init || crossfade_active)
        return;

    if (state != boost_state) {
        cpu_boost(state);
        boost_state = state;
    }
}
#endif

#define CALL_IF_EXISTS(function, args...) if (function) function(args)
/* This function has 2 major logical parts (separated by brackets both for
 * readability and variable scoping).  The first part performs the
 * operastions related to finishing off the last buffer we fed to the DMA.
 * The second part performs the operations involved in sending a new buffer
 * to the DMA.  Finally the function checks the status of the buffer and
 * boosts if necessary */
static void pcmbuf_callback(unsigned char** start, size_t* size) ICODE_ATTR;
static void pcmbuf_callback(unsigned char** start, size_t* size)
{
    {
        struct pcmbufdesc *pcmbuf_current = pcmbuf_read;
        /* Take the finished buffer out of circulation */
        pcmbuf_read = pcmbuf_current->link;

        /* The buffer is finished, call the callback functions */
        CALL_IF_EXISTS(position_callback, last_chunksize);
        CALL_IF_EXISTS(pcmbuf_current->callback);

        /* Put the finished buffer back into circulation */
        pcmbuf_write_end->link = pcmbuf_current;
        pcmbuf_write_end = pcmbuf_current;

        /* If we've read up to the mix chunk while it's still mixing there */
        if (pcmbuf_current == pcmbuf_mix_chunk)
            pcmbuf_mix_chunk = NULL;
        /* If we've read up to the crossfade chunk while it's still fading */
        if (pcmbuf_current == crossfade_chunk)
            crossfade_chunk = NULL;
    }

process_new_buffer:
    {
        /* Send the new buffer to the pcm */
        struct pcmbufdesc *pcmbuf_new = pcmbuf_read;
        size_t *realsize = size;
        unsigned char** realstart = start;
        if(pcmbuf_new)
        {
            size_t current_size = pcmbuf_new->size;

            pcmbuf_unplayed_bytes -= current_size;
            last_chunksize = current_size;
            *realsize = current_size;
            *realstart = pcmbuf_new->addr;
        }
        else
        {
            /* There may be more data waiting to flush, try to use it */
            if (pcmbuf_flush_fillpos())
                goto process_new_buffer;
                
            /* No more buffers */
            last_chunksize = 0;
            *realsize = 0;
            *realstart = NULL;
            CALL_IF_EXISTS(pcmbuf_event_handler);
        }
    }
}

void pcmbuf_set_position_callback(void (*callback)(size_t size))
{
    position_callback = callback;
}

static void pcmbuf_set_watermark_bytes(size_t numbytes)
{
    pcmbuf_watermark = numbytes;
}

/* This is really just part of pcmbuf_flush_fillpos, but is easier to keep
 * in a separate function for the moment */
static inline void pcmbuf_add_chunk(void)
{
    register size_t size = audiobuffer_fillpos;
    /* Grab the next description to write, and change the write pointer */
    register struct pcmbufdesc *pcmbuf_current = pcmbuf_write;
    pcmbuf_write = pcmbuf_current->link;
    /* Fill in the values in the new buffer chunk */
    pcmbuf_current->addr = &audiobuffer[audiobuffer_pos];
    pcmbuf_current->size = size;
    pcmbuf_current->callback = pcmbuf_event_handler;
    pcmbuf_current->link = NULL;
    /* This is single use only */
    pcmbuf_event_handler = NULL;
    if (pcmbuf_read) {
        if (pcmbuf_flush)
        {
            pcmbuf_write_end->link = pcmbuf_read->link;
            pcmbuf_read->link = pcmbuf_current;
            while (pcmbuf_write_end->link)
            {
                pcmbuf_write_end = pcmbuf_write_end->link;
                pcmbuf_unplayed_bytes -= pcmbuf_write_end->size;
            }
            pcmbuf_flush = false;
        }
        /* If there is already a read buffer setup, add to it */
        else
            pcmbuf_read_end->link = pcmbuf_current;
    } else {
        /* Otherwise create the buffer */
        pcmbuf_read = pcmbuf_current;
    }
    /* This is now the last buffer to read */
    pcmbuf_read_end = pcmbuf_current;

    /* Update bytes counters */
    pcmbuf_unplayed_bytes += size;

    audiobuffer_pos += size;
    if (audiobuffer_pos >= pcmbuf_size)
        audiobuffer_pos -= pcmbuf_size;

    audiobuffer_fillpos = 0;
}

static void pcmbuf_under_watermark(void)
{
    /* Fill audio buffer by boosting cpu */
    pcmbuf_boost(true);
    /* Disable crossfade if < .5s of audio */
    if (LOW_DATA(2))
        crossfade_active = false;
}

void pcmbuf_set_event_handler(void (*event_handler)(void))
{
    pcmbuf_event_handler = event_handler;
}

unsigned int pcmbuf_get_latency(void)
{
    /* Be careful how this calculation is rearranted, it's easy to overflow */
    size_t bytes = pcmbuf_unplayed_bytes + pcm_get_bytes_waiting();
    return bytes / 4 / (NATIVE_FREQUENCY/1000);
}

void pcmbuf_set_low_latency(bool state)
{
    low_latency_mode = state;
}

bool pcmbuf_is_lowdata(void)
{
    if (!pcm_is_playing() || pcm_is_paused() ||
            crossfade_init || crossfade_active)
        return false;

    /* 0.5 seconds of buffer is low data */
    return LOW_DATA(2);
}

/* Amount of bytes left in the buffer. */
inline size_t pcmbuf_free(void)
{
    if (pcmbuf_read)
    {
        size_t read = (size_t)pcmbuf_read->addr;
        size_t write =
            (size_t)&audiobuffer[audiobuffer_pos + audiobuffer_fillpos];
        if (read < write)
            read += pcmbuf_size;
        return read - write;
    }
    return pcmbuf_size;
}

bool pcmbuf_crossfade_init(bool manual_skip)
{
    /* Can't do two crossfades at once and, no fade if pcm is off now */
    if (crossfade_init || crossfade_active || !pcm_is_playing())
    {
        pcmbuf_play_stop();
        return false;
    }

    /* Not enough data, or crossfade disabled, flush the old data instead */
    if (LOW_DATA(6) || !pcmbuf_is_crossfade_enabled() || low_latency_mode)
    {
        pcmbuf_boost(true);
        pcmbuf_flush = true;
        return false;
    }

    logf("pcmbuf_crossfade_init");
    pcmbuf_boost(true);

    /* Don't enable mix mode when skipping tracks manually. */
    crossfade_mix = manual_skip && global_settings.crossfade_fade_out_mixmode;
    crossfade_init = true;

    return true;

}

void pcmbuf_play_stop(void)
{
    /** Prevent a very tiny pop from happening by muting audio
     *  until dma has been initialized. */
    pcm_mute(true);
    pcm_play_stop();
    pcm_mute(false);

    pcmbuf_unplayed_bytes = 0;
    pcmbuf_mix_chunk = NULL;
    if (pcmbuf_read) {
        pcmbuf_write_end->link = pcmbuf_read;
        pcmbuf_write_end = pcmbuf_read_end;
        pcmbuf_read = pcmbuf_read_end = NULL;
    }
    audiobuffer_pos = 0;
    audiobuffer_fillpos = 0;
    crossfade_init = false;
    crossfade_active = false;
    pcmbuf_flush = false;

    pcmbuf_boost(false);

}

int pcmbuf_used_descs(void) {
    struct pcmbufdesc *pcmbuf_temp = pcmbuf_read;
    unsigned int i = 0;
    while (pcmbuf_temp) {
        pcmbuf_temp = pcmbuf_temp->link;
        i++;
    }
    return i;
}

int pcmbuf_descs(void) {
    return pcmbuf_size / PCMBUF_TARGET_CHUNK;
}

size_t get_pcmbuf_descsize(void) {
    return pcmbuf_descsize;
}

static void pcmbuf_init_pcmbuffers(void) {
    struct pcmbufdesc *next = pcmbuf_write;
    next++;
    pcmbuf_write_end = pcmbuf_write;
    while ((void *)next < (void *)audiobufend) {
        pcmbuf_write_end->link=next;
        pcmbuf_write_end=next;
        next++;
    }
}

/* Initialize the pcmbuffer the structure looks like this:
 * ...CODECBUFFER|---------PCMBUF---------|GUARDBUF|DESCS| */
void pcmbuf_init(size_t bufsize)
{
    pcmbuf_size = bufsize;
    pcmbuf_descsize = pcmbuf_descs()*sizeof(struct pcmbufdesc);
    audiobuffer = (char *)&audiobuf[(audiobufend - audiobuf) -
        (pcmbuf_size + PCMBUF_MIX_CHUNK * 2 + pcmbuf_descsize)];
    fadebuf = &audiobuffer[pcmbuf_size];
    voicebuf = &fadebuf[PCMBUF_MIX_CHUNK];
    pcmbuf_write = (struct pcmbufdesc *)(&voicebuf[PCMBUF_MIX_CHUNK]);
    pcmbuf_init_pcmbuffers();
    position_callback = NULL;
    pcmbuf_event_handler = NULL;
    pcmbuf_play_stop();
}

size_t pcmbuf_get_bufsize(void)
{
    return pcmbuf_size;
}

void pcmbuf_pause(bool pause) {
    if (pause)
        pcm_mute(true);
    pcm_play_pause(!pause);
    if (!pause)
        pcm_mute(false);
    pcmbuf_boost(!pause && pcm_is_playing());
}

/* Force playback. */
void pcmbuf_play_start(void)
{
    if (!pcm_is_playing() && pcmbuf_unplayed_bytes)
    {
        /** Prevent a very tiny pop from happening by muting audio
         *  until dma has been initialized. */
        pcm_mute(true);

        last_chunksize = pcmbuf_read->size;
        pcmbuf_unplayed_bytes -= last_chunksize;
        pcm_play_data(pcmbuf_callback,
                (unsigned char *)pcmbuf_read->addr, last_chunksize);

        /* Now unmute the audio. */
        pcm_mute(false);
    }
}

/**
 * Commit samples waiting to the pcm buffer.
 */
static bool pcmbuf_flush_fillpos(void)
{
    if (audiobuffer_fillpos) {
        /* Never use the last buffer descriptor */
        while (pcmbuf_write == pcmbuf_write_end) {
            logf("pcmbuf_flush_fillpos no descriptors");
            /* Deboost to let the playback catchup */
            pcmbuf_boost(false);
            /* If this happens, something is being stupid */
            if (!pcm_is_playing()) {
                logf("pcmbuf_flush_fillpos error");
                pcmbuf_play_start();
            }
            /* Let approximately one chunk of data playback */
            sleep(PCMBUF_TARGET_CHUNK/(NATIVE_FREQUENCY * 4) / 5);
        }
        pcmbuf_add_chunk();
        return true;
    }
    return false;
}

/**
 * Completely process the crossfade fade out effect with current pcm buffer.
 */
static void crossfade_process_buffer(size_t fade_in_delay,
        size_t fade_out_delay, size_t fade_out_rem)
{
    if (!crossfade_mix)
    {
        /* Fade out the specified amount of the already processed audio */
        size_t total_fade_out = fade_out_rem;
        size_t fade_out_sample;
        struct pcmbufdesc *fade_out_chunk = crossfade_chunk;

        /* Find the right chunk to start fading out */
        fade_out_delay += crossfade_sample * 2;
        while (fade_out_delay >= fade_out_chunk->size)
        {
            fade_out_delay -= fade_out_chunk->size;
            fade_out_chunk = fade_out_chunk->link;
        }
        /* The start sample within the chunk */
        fade_out_sample = fade_out_delay / 2;
        
        while (fade_out_rem > 0)
        {
            /* Each 1/10 second of audio will have the same fade applied */
            size_t block_rem = MIN(NATIVE_FREQUENCY * 4 / 10, fade_out_rem);
            int factor = (fade_out_rem << 8) / total_fade_out;

            fade_out_rem -= block_rem;

            /* Fade this block */
            while (block_rem > 0)
            {
                /* Fade one sample */
                short *buf = (short *)(fade_out_chunk->addr);
                int sample = buf[fade_out_sample];
                buf[fade_out_sample++] = (sample * factor) >> 8;

                block_rem -= 2;
                /* Move to the next chunk as needed */
                if (fade_out_sample * 2 >= fade_out_chunk->size)
                {
                    fade_out_chunk = fade_out_chunk->link;
                    fade_out_sample = 0;
                }
            }
        }
    }

    /* Find the right chunk and sample to start fading in */
    fade_in_delay += crossfade_sample * 2;
    while (fade_in_delay >= crossfade_chunk->size)
    {
        fade_in_delay -= crossfade_chunk->size;
        crossfade_chunk = crossfade_chunk->link;
    }
    crossfade_sample = fade_in_delay / 2;
    logf("process done!");
}

/* Initializes crossfader, calculates all necessary parameters and
 * performs fade-out with the pcm buffer.  */
static void crossfade_start(void)
{
    size_t crossfade_rem;
    size_t crossfade_need;
    size_t fade_out_rem;
    size_t fade_out_delay;
    size_t fade_in_delay;

    crossfade_init = false;
    /* Reject crossfade if less than .5s of data */
    if (LOW_DATA(2)) {
        logf("crossfade rejected");
        pcmbuf_play_stop();
        return ;
    }

    logf("crossfade_start");
    pcmbuf_flush_fillpos();
    crossfade_active = true;

    /* Initialize the crossfade buffer size to all of the buffered data that
     * has not yet been sent to the DMA */
    crossfade_rem = pcmbuf_unplayed_bytes;
    crossfade_chunk = pcmbuf_read->link;
    crossfade_sample = 0;

    /* Get fade out delay from settings. */
    fade_out_delay =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_out_delay * 4;

    /* Get fade out duration from settings. */
    fade_out_rem =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_out_duration * 4;

    crossfade_need = fade_out_delay + fade_out_rem;
    /* We want only to modify the last part of the buffer. */
    if (crossfade_rem > crossfade_need)
    {
        size_t crossfade_extra = crossfade_rem - crossfade_need;
        while (crossfade_extra > crossfade_chunk->size)
        {
            crossfade_extra -= crossfade_chunk->size;
            crossfade_chunk = crossfade_chunk->link;
        }
        crossfade_sample = crossfade_extra / 2;
    }
    /* Truncate fade out duration if necessary. */
    else if (crossfade_rem < crossfade_need)
    {
        size_t crossfade_short = crossfade_need - crossfade_rem;
        if (fade_out_rem >= crossfade_short)
            fade_out_rem -= crossfade_short;
        else
        {
            fade_out_delay -= crossfade_short - fade_out_rem;
            fade_out_rem = 0;
        }
    }

    /* Get also fade in duration and delays from settings. */
    crossfade_fade_in_total =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_in_duration * 4;
    crossfade_fade_in_rem = crossfade_fade_in_total;

    fade_in_delay =
        NATIVE_FREQUENCY * global_settings.crossfade_fade_in_delay * 4;

    crossfade_process_buffer(fade_in_delay, fade_out_delay, fade_out_rem);
}

static size_t fade_mix(int factor, const char *buf, size_t fade_rem)
{
    const short *input_buf = (const short *)buf;
    size_t samples = 0;
    short *output_buf = (short *)(crossfade_chunk->addr);
    short *chunk_end = (short *)((size_t)output_buf + crossfade_chunk->size);
    output_buf = &output_buf[crossfade_sample];

    fade_rem /= 2;
    while (samples < fade_rem)
    {
        int sample = *input_buf++;
        sample = ((sample * factor) >> 8) + *output_buf;
        *output_buf++ = MIN(32767, MAX(-32768, sample));
        samples++;

        if (output_buf >= chunk_end)
        {
            crossfade_chunk = crossfade_chunk->link;
            if (!crossfade_chunk)
                return samples * 2;
            output_buf = (short *)(crossfade_chunk->addr);
            chunk_end = (short *)((size_t)output_buf + crossfade_chunk->size);
        }
    }
    crossfade_sample = (size_t)(output_buf - (short *)(crossfade_chunk->addr));
    return samples * 2;
}

static void pcmbuf_flush_buffer(const char *buf, size_t length)
{
    size_t copy_n;
    while (length > 0) {
        size_t audiobuffer_index = audiobuffer_pos + audiobuffer_fillpos;
        if (NEED_FLUSH(audiobuffer_index))
        {
            pcmbuf_flush_fillpos();
            audiobuffer_index = audiobuffer_pos + audiobuffer_fillpos;
        }
        copy_n = MIN(length, pcmbuf_size - audiobuffer_index);
        memcpy(&audiobuffer[audiobuffer_index], buf, copy_n);
        buf += copy_n;
        audiobuffer_fillpos += copy_n;
        length -= copy_n;
    }
}

static void flush_crossfade(char *buf, size_t length)
{
    if (length && crossfade_fade_in_rem)
    {
        int factor = ((crossfade_fade_in_total - crossfade_fade_in_rem) << 8) /
            crossfade_fade_in_total;
        /* Bytes to fade */
        size_t fade_rem = MIN(length, crossfade_fade_in_rem);
        crossfade_fade_in_rem -= fade_rem;

        if (crossfade_chunk)
        {
            /* Mix the data */
            size_t complete = fade_mix(factor, buf, fade_rem);
            length -= complete;
            buf += complete;
            fade_rem -= complete;
            /* If there is still fading to be done */
            if (fade_rem)
                goto mix_done;
        }
        else
        {
            size_t samples;
            short *input_buf;
mix_done:
            /* Fade samples in place */
            samples = fade_rem / 2;
            input_buf = (short *)buf;
            while (samples)
            {
                int sample = *input_buf;
                *input_buf++ = (sample * factor) >> 8;
                samples--;
            }
        }
    }

    if (length)
    {
        /* Flush samples to the buffer */
        while (pcmbuf_free() < length)
        {
            pcmbuf_boost(false);
            sleep(1);
        }
        pcmbuf_flush_buffer(buf, length);
    }

    if (!crossfade_fade_in_rem)
        crossfade_active = false;
}

static bool prepare_insert(size_t length)
{
    if (low_latency_mode)
    {
        /* 1/4s latency. */
        if (pcmbuf_unplayed_bytes > NATIVE_FREQUENCY * 4 / 4
            && pcm_is_playing())
            return false;
    }

    /* Need to save PCMBUF_MIN_CHUNK to prevent wrapping overwriting */
    if (pcmbuf_free() < length + PCMBUF_MIN_CHUNK && !crossfade_active)
    {
        pcmbuf_boost(false);
        return false;
    }

    if (!pcm_is_playing())
    {
        pcmbuf_boost(true);
        crossfade_active = false;
        /* Pre-buffer 1s. */
        if (!LOW_DATA(4))
        {
            logf("pcm starting");
            pcmbuf_play_start();
        }
    } else if (pcmbuf_unplayed_bytes <= pcmbuf_watermark)
        pcmbuf_under_watermark();

    return true;
}

void* pcmbuf_request_buffer(size_t length, size_t *realsize)
{
    if (crossfade_init)
        crossfade_start();

    if (crossfade_active) {
        *realsize = MIN(length, PCMBUF_MIX_CHUNK);
        return fadebuf;
    }
    else
    {
        if(prepare_insert(length))
        {
            size_t audiobuffer_index = audiobuffer_pos + audiobuffer_fillpos;
            *realsize = length;
            if (pcmbuf_size - audiobuffer_index >= PCMBUF_MIN_CHUNK)
            {
                /* Usual case, there's space here */
                return &audiobuffer[audiobuffer_index];
            }
            else
            {
                /* Flush and wrap the buffer */
                pcmbuf_flush_fillpos();
                audiobuffer_pos = 0;
                return &audiobuffer[0];
            }
        }
        else
        {
            *realsize = 0;
            return NULL;
        }
    }
}

void* pcmbuf_request_voice_buffer(size_t length, size_t *realsize, bool mix)
{
    if (mix)
    {
        if (pcmbuf_mix_chunk || pcmbuf_read->link)
        {
            *realsize = MIN(length, PCMBUF_MIX_CHUNK);
            return voicebuf;
        }
        else
        {
            *realsize = 0;
            return NULL;
        }
    }
    else
        return pcmbuf_request_buffer(length, realsize);
}

bool pcmbuf_is_crossfade_active(void)
{
    return crossfade_active || crossfade_init;
}

void pcmbuf_write_complete(size_t length)
{
    if (crossfade_active)
        flush_crossfade(fadebuf, length);
    else
    {
        audiobuffer_fillpos += length;

        if (NEED_FLUSH(audiobuffer_pos + audiobuffer_fillpos))
            pcmbuf_flush_fillpos();
    }
}

bool pcmbuf_insert_buffer(char *buf, size_t length)
{
    if (!prepare_insert(length))
        return false;

    if (crossfade_active) {
        flush_crossfade(buf, length);
    }
    else
    {
        pcmbuf_flush_buffer(buf, length);
    }
    return true;
}

/* Generates a constant square wave sound with a given frequency
   in Hertz for a duration in milliseconds. */
void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude)
{
    unsigned int count = 0, i = 0;
    unsigned int interval = NATIVE_FREQUENCY / frequency;
    long sample;
    short *buf;
    short *pcmbuf_end = (short *)fadebuf;
    size_t samples = NATIVE_FREQUENCY / 1000 * duration;

    if (pcm_is_playing())
    {
        if (pcmbuf_read->link)
        {
            /* Get the next chunk */
            char *pcmbuf_mix_buf = pcmbuf_read->link->addr;
            /* Give at least 1/8s clearance. */
            buf = (short *)&pcmbuf_mix_buf[NATIVE_FREQUENCY * 4 / 8];
        }
        else
        {
            logf("No place to beep");
            return;
        }

        while (i++ < samples)
        {
            sample = *buf;
            *buf++ = MIN(MAX(sample + amplitude, -32768), 32767);
            if (buf > pcmbuf_end)
                buf = (short *)audiobuffer;
            sample = *buf;
            *buf++ = MIN(MAX(sample + amplitude, -32768), 32767);

            /* Toggle square wav side */
            if (++count >= interval)
            {
                count = 0;
                amplitude = -amplitude;
            }
            if (buf > pcmbuf_end)
                buf = (short *)audiobuffer;
        }
    }
    else
    {
        buf = (short *)audiobuffer;
        while (i++ < samples)
        {
            *buf++ = amplitude;
            if (buf > pcmbuf_end)
                buf = (short *)audiobuffer;
            *buf++ = amplitude;

            /* Toggle square wav side */
            if (++count >= interval)
            {
                count = 0;
                amplitude = -amplitude;
            }
            if (buf > pcmbuf_end)
                buf = (short *)audiobuffer;
        }
        pcm_play_data(NULL, (unsigned char *)audiobuffer, samples * 4);
    }
}

/* Returns pcm buffer usage in percents (0 to 100). */
int pcmbuf_usage(void)
{
    return pcmbuf_unplayed_bytes * 100 / pcmbuf_size;
}

int pcmbuf_mix_free(void)
{
    if (pcmbuf_mix_chunk)
    {
        size_t my_mix_end =
            (size_t)&((short *)pcmbuf_mix_chunk->addr)[pcmbuf_mix_sample];
        size_t my_write_pos = (size_t)&audiobuffer[audiobuffer_pos];
        if (my_write_pos < my_mix_end)
            my_write_pos += pcmbuf_size;
        return (my_write_pos - my_mix_end) * 100 / pcmbuf_unplayed_bytes;
    }
    return 100;
}

void pcmbuf_mix_voice(size_t length)
{
    short *ibuf = (short *)voicebuf;
    short *obuf;
    size_t chunk_samples;

    if (!pcmbuf_mix_chunk && pcmbuf_read)
    {
        pcmbuf_mix_chunk = pcmbuf_read->link;
        /* Start 1/8s into the next chunk */
        pcmbuf_mix_sample = NATIVE_FREQUENCY * 4 / 16;
    }
    if (!pcmbuf_mix_chunk)
        return;

    obuf = (short *)pcmbuf_mix_chunk->addr;
    chunk_samples = pcmbuf_mix_chunk->size / 2;

    length /= 2;

    while (length-- > 0) {
        int sample = *ibuf++;
        if (pcmbuf_mix_sample >= chunk_samples)
        {
            pcmbuf_mix_chunk = pcmbuf_mix_chunk->link;
            if (!pcmbuf_mix_chunk)
                return;
            pcmbuf_mix_sample = 0;
            obuf = pcmbuf_mix_chunk->addr;
            chunk_samples = pcmbuf_mix_chunk->size / 2;
        }
        sample += obuf[pcmbuf_mix_sample] >> 2;
        obuf[pcmbuf_mix_sample++] = MIN(MAX(sample, -32768), 32767);
    }
}

void pcmbuf_crossfade_enable(bool on_off)
{
    crossfade_enabled = on_off;

    if (crossfade_enabled) {
        /* If crossfading, try to keep the buffer full other than 2 second */
        pcmbuf_set_watermark_bytes(pcmbuf_size - PCMBUF_WATERMARK * 2);
    } else {
        /* Otherwise, just keep it above 1 second */
        pcmbuf_set_watermark_bytes(PCMBUF_WATERMARK);
    }
}

bool pcmbuf_is_crossfade_enabled(void)
{
    if (global_settings.crossfade == CROSSFADE_ENABLE_SHUFFLE)
        return global_settings.playlist_shuffle;

    return crossfade_enabled;
}

