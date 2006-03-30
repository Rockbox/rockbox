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

/* Size of the PCM buffer. */
static size_t pcmbuf_size IDATA_ATTR = 0;

static char *audiobuffer IDATA_ATTR;
/* Current audio buffer write index. */
static size_t audiobuffer_pos IDATA_ATTR;
/* Amount of bytes left in the buffer. */
size_t audiobuffer_free IDATA_ATTR;
/* Amount audiobuffer_pos will be increased.*/
static size_t audiobuffer_fillpos IDATA_ATTR;
static char *guardbuf IDATA_ATTR;

static void (*pcmbuf_event_handler)(void) IDATA_ATTR;
static void (*position_callback)(size_t size) IDATA_ATTR;

/* Crossfade related. */
static int crossfade_mode IDATA_ATTR;
static bool crossfade_enabled IDATA_ATTR;
static bool crossfade_active IDATA_ATTR;
static bool crossfade_init IDATA_ATTR;
static size_t crossfade_pos IDATA_ATTR;
static size_t crossfade_rem IDATA_ATTR;

static struct mutex pcmbuf_mutex IDATA_ATTR;

/* Crossfade modes. If CFM_CROSSFADE is selected, normal
 * crossfader will activate. Selecting CFM_FLUSH is a special
 * operation that only overwrites the pcm buffer without crossfading.
 */
enum {
    CFM_CROSSFADE,
    CFM_MIX,
    CFM_FLUSH
};

static size_t crossfade_fade_in_amount IDATA_ATTR;
static size_t crossfade_fade_in_rem IDATA_ATTR;


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

static size_t pcmbuf_descsize;
static struct pcmbufdesc *pcmbuf_read IDATA_ATTR;
static struct pcmbufdesc *pcmbuf_read_end IDATA_ATTR;
static struct pcmbufdesc *pcmbuf_write IDATA_ATTR;
static struct pcmbufdesc *pcmbuf_write_end IDATA_ATTR;
static size_t last_chunksize IDATA_ATTR;
static size_t pcmbuf_unplayed_bytes IDATA_ATTR;
static size_t pcmbuf_mix_used_bytes IDATA_ATTR;
static size_t pcmbuf_watermark IDATA_ATTR;
static short *mixpos IDATA_ATTR;
static bool low_latency_mode = false;

/* Helpful macros for use in conditionals this assumes some of the above
 * static variable names */
#define NEED_FLUSH(position) \
    (audiobuffer_fillpos > PCMBUF_TARGET_CHUNK || position >= pcmbuf_size)
#define LOW_DATA(quarter_secs) \
    (pcmbuf_unplayed_bytes < NATIVE_FREQUENCY * quarter_secs)

static void pcmbuf_flush_audio(void);
static void pcmbuf_under_watermark(void);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && !defined(SIMULATOR)
static bool boost_mode;

void pcmbuf_boost(bool state)
{
    static bool boost_state = false;

    if (crossfade_init || crossfade_active || boost_mode)
        return;

    if (state != boost_state) {
        cpu_boost(state);
        boost_state = state;
    }
}

void pcmbuf_set_boost_mode(bool state)
{
    if (state)
        pcmbuf_boost(true);
    boost_mode = state;
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

        {
            size_t finished_size = last_chunksize;
            audiobuffer_free += finished_size;

            /* The buffer is finished, call the callback functions */
            CALL_IF_EXISTS(position_callback, finished_size);
        }
        CALL_IF_EXISTS(pcmbuf_current->callback);

        /* Put the finished buffer back into circulation */
        pcmbuf_write_end->link = pcmbuf_current;
        pcmbuf_write_end = pcmbuf_current;
    }

    {
        /* Send the new buffer to the pcm */
        struct pcmbufdesc *pcmbuf_new = pcmbuf_read;
        size_t *realsize = size;
        unsigned char** realstart = start;
        if(pcmbuf_new)
        {
            size_t current_size = pcmbuf_new->size;
            pcmbuf_unplayed_bytes -= current_size;
            *realsize = current_size;
            last_chunksize = current_size;
            *realstart = pcmbuf_new->addr;
        }
        else
        {
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
        /* If there is already a read buffer setup, add to it */
        pcmbuf_read_end->link = pcmbuf_current;
    } else {
        /* Otherwise create the buffer */
        pcmbuf_read = pcmbuf_current;
    }
    /* This is now the last buffer to read */
    pcmbuf_read_end = pcmbuf_current;

    /* Update bytes counters */
    pcmbuf_unplayed_bytes += size;
    if (pcmbuf_mix_used_bytes > size)
        pcmbuf_mix_used_bytes -= size;
    else
        pcmbuf_mix_used_bytes = 0;

    audiobuffer_pos += size;
    if (audiobuffer_pos >= pcmbuf_size)
        audiobuffer_pos = 0;

    audiobuffer_fillpos = 0;
}

static void pcmbuf_under_watermark(void)
{
    /* Fill audio buffer by boosting cpu */
    pcmbuf_boost(true);
    /* Disable crossfade if < .5s of audio */
    if (LOW_DATA(2) && crossfade_mode != CFM_FLUSH)
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

bool pcmbuf_crossfade_init(bool manual_skip)
{
    if (pcmbuf_unplayed_bytes < PCMBUF_TARGET_CHUNK * 8
        || !pcmbuf_is_crossfade_enabled()
        || crossfade_active || crossfade_init || low_latency_mode) {
        pcmbuf_flush_audio();
        return false;
    }
    logf("pcmbuf_crossfade_init");
    pcmbuf_boost(true);

    /* Don't enable mix mode when skipping tracks manually. */
    if (manual_skip)
        crossfade_mode = CFM_CROSSFADE;
    else
        crossfade_mode = global_settings.crossfade_fade_out_mixmode
                ? CFM_MIX : CFM_CROSSFADE;
    crossfade_init = true;

    return true;

}

void pcmbuf_play_stop(void)
{
    mutex_lock(&pcmbuf_mutex);
    /** Prevent a very tiny pop from happening by muting audio
     *  until dma has been initialized. */
    pcm_mute(true);
    pcm_play_stop();
    pcm_mute(false);

    pcmbuf_unplayed_bytes = 0;
    pcmbuf_mix_used_bytes = 0;
    if (pcmbuf_read) {
        pcmbuf_write_end->link = pcmbuf_read;
        pcmbuf_write_end = pcmbuf_read_end;
        pcmbuf_read = pcmbuf_read_end = NULL;
    }
    audiobuffer_pos = 0;
    audiobuffer_fillpos = 0;
    audiobuffer_free = pcmbuf_size;
    crossfade_init = false;
    crossfade_active = false;

    pcmbuf_set_boost_mode(false);
    pcmbuf_boost(false);

    mutex_unlock(&pcmbuf_mutex);
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
    return pcmbuf_size / PCMBUF_MINAVG_CHUNK;
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
    mutex_init(&pcmbuf_mutex);
    pcmbuf_size = bufsize;
    pcmbuf_descsize = pcmbuf_descs()*sizeof(struct pcmbufdesc);
    audiobuffer = (char *)&audiobuf[(audiobufend - audiobuf) -
        (pcmbuf_size + PCMBUF_FADE_CHUNK + pcmbuf_descsize)];
    guardbuf = &audiobuffer[pcmbuf_size];
    pcmbuf_write = (struct pcmbufdesc *)(&guardbuf[PCMBUF_FADE_CHUNK]);
    pcmbuf_init_pcmbuffers();
    position_callback = NULL;
    pcmbuf_event_handler = NULL;
    pcmbuf_play_stop();
}

size_t pcmbuf_get_bufsize(void)
{
    return pcmbuf_size;
}

/** Initialize a track switch so that audio playback will not stop but
 *  the switch to next track would happen as soon as possible.
 */
static void pcmbuf_flush_audio(void)
{
    if (crossfade_init || crossfade_active || !pcm_is_playing()) {
        pcmbuf_play_stop();
        return ;
    }

    pcmbuf_boost(true);
    crossfade_mode = CFM_FLUSH;
    crossfade_init = true;
}

void pcmbuf_pause(bool pause) {
    if (pause)
        pcm_mute(true);
    pcm_play_pause(!pause);
    if (!pause)
        pcm_mute(false);
    pcmbuf_boost(!pause);
}

/* Force playback. */
void pcmbuf_play_start(void)
{
    mutex_lock(&pcmbuf_mutex);

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

    mutex_unlock(&pcmbuf_mutex);
}

/**
 * Commit samples waiting to the pcm buffer.
 */
static void pcmbuf_flush_fillpos(void)
{
    mutex_lock(&pcmbuf_mutex);

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
    }

    mutex_unlock(&pcmbuf_mutex);
}

/**
 * Completely process the crossfade fade out effect with current pcm buffer.
 */
static void crossfade_process_buffer(size_t fade_in_delay,
        size_t fade_out_delay, size_t fade_out_rem)
{
    if (crossfade_mode == CFM_CROSSFADE)
    {
        /* Fade out the specified amount of the already processed audio */
        size_t total_fade_out = fade_out_rem;
        short *buf = (short *)&audiobuffer[crossfade_pos + fade_out_delay * 2];
        short *buf_end = (short *)guardbuf;

        /* Wrap the starting position if needed */
        if (buf >= buf_end) buf -= pcmbuf_size / 2;

        while (fade_out_rem > 0)
        {
            /* Each 1/10 second of audio will have the same fade applied */
            size_t block_rem = MIN(NATIVE_FREQUENCY * 2 / 10, fade_out_rem);
            unsigned int factor = (fade_out_rem << 8) / total_fade_out;
            short *block_end = buf + block_rem;

            fade_out_rem -= block_rem;

            /* Fade this block */
            while (buf < block_end)
            {
                /* Fade one sample */
                *buf = (*buf * factor) >> 8;
                buf++;

                if (buf >= buf_end)
                {
                    /* Wrap the pcmbuffer */
                    buf -= pcmbuf_size / 2;
                    /* Wrap the end pointer to ensure proper termination */
                    block_end -= pcmbuf_size / 2;
                }
            }
        }
    }

    /* And finally set the mixing position where we should start fading in. */
    crossfade_rem -= fade_in_delay;
    crossfade_pos += fade_in_delay*2;
    if (crossfade_pos >= pcmbuf_size)
        crossfade_pos -= pcmbuf_size;
    logf("process done!");
}

/**
 * Initializes crossfader, calculates all necessary parameters and
 * performs fade-out with the pcm buffer.
 */
static void crossfade_start(void)
{
    size_t fade_out_rem = 0;
    unsigned int fade_out_delay = 0;
    unsigned fade_in_delay = 0;

    crossfade_init = 0;
    /* Reject crossfade if less than .5s of data */
    if (LOW_DATA(2)) {
        logf("crossfade rejected");
        pcmbuf_play_stop();
        return ;
    }

    logf("crossfade_start");
    pcmbuf_boost(true);
    pcmbuf_flush_fillpos();
    crossfade_active = true;
    crossfade_pos = audiobuffer_pos;
    /* Initialize the crossfade buffer size to all of the buffered data that
     * has not yet been sent to the DMA */
    crossfade_rem = pcmbuf_unplayed_bytes / 2;

    switch (crossfade_mode) {
        case CFM_MIX:
        case CFM_CROSSFADE:
            /* Get fade out delay from settings. */
            fade_out_delay = NATIVE_FREQUENCY
                    * global_settings.crossfade_fade_out_delay * 2;

            /* Get fade out duration from settings. */
            fade_out_rem = NATIVE_FREQUENCY
                    * global_settings.crossfade_fade_out_duration * 2;

            /* We want only to modify the last part of the buffer. */
            if (crossfade_rem > fade_out_rem + fade_out_delay)
                crossfade_rem = fade_out_rem + fade_out_delay;

            /* Truncate fade out duration if necessary. */
            if (crossfade_rem < fade_out_rem + fade_out_delay)
                fade_out_rem -= (fade_out_rem + fade_out_delay) - crossfade_rem;

            /* Get also fade in duration and delays from settings. */
            crossfade_fade_in_rem = NATIVE_FREQUENCY
                    * global_settings.crossfade_fade_in_duration * 2;
            crossfade_fade_in_amount = crossfade_fade_in_rem;

            /* We should avoid to divide by zero. */
            if (crossfade_fade_in_amount == 0)
                crossfade_fade_in_amount = 1;

            fade_in_delay = NATIVE_FREQUENCY
                    * global_settings.crossfade_fade_in_delay * 2;

            /* Decrease the fade out delay if necessary. */
            if (crossfade_rem < fade_out_rem + fade_out_delay)
                fade_out_delay -=
                    (fade_out_rem + fade_out_delay) - crossfade_rem;
            break ;

        case CFM_FLUSH:
            crossfade_fade_in_rem = 0;
            crossfade_fade_in_amount = 0;
            break ;
    }

    if (crossfade_pos < crossfade_rem * 2)
        crossfade_pos += pcmbuf_size;
    crossfade_pos -= crossfade_rem*2;

    if (crossfade_mode != CFM_FLUSH) {
        /* Process the fade out part of the crossfade. */
        crossfade_process_buffer(fade_in_delay, fade_out_delay, fade_out_rem);
    }

}

/**
 * Fades in samples passed to the function and inserts them
 * to the pcm buffer.
 */
static void fade_insert(const short *inbuf, size_t length)
{
    size_t copy_n;
    int factor;
    unsigned int i, samples;
    short *buf;

    factor = ((crossfade_fade_in_amount-crossfade_fade_in_rem)<<8)
        /crossfade_fade_in_amount;

    while (audiobuffer_free < length)
    {
        pcmbuf_boost(false);
        sleep(1);
    }
    audiobuffer_free -= length;

    while (length > 0) {
        unsigned int audiobuffer_index = audiobuffer_pos + audiobuffer_fillpos;
        /* Flush as needed */
        if (NEED_FLUSH(audiobuffer_index))
        {
            pcmbuf_flush_fillpos();
            audiobuffer_index = audiobuffer_pos + audiobuffer_fillpos;
        }

        copy_n = MIN(length, pcmbuf_size - audiobuffer_index);

        buf = (short *)&audiobuffer[audiobuffer_index];
        samples = copy_n / 2;
        for (i = 0; i < samples; i++)
            buf[i] = (inbuf[i] * factor) >> 8;

        inbuf += samples;
        audiobuffer_fillpos += copy_n;
        length -= copy_n;
    }
}

/**
 * Fades in buf2 and mixes it with buf.
 */
static int crossfade(short *buf, const short *buf2, unsigned int length)
{
    size_t size;
    unsigned int i;
    size_t size_insert = 0;
    int factor;

    size = MIN(length, crossfade_rem);
    switch (crossfade_mode) {
        /* Fade in the current stream and mix it. */
        case CFM_MIX:
        case CFM_CROSSFADE:
            factor = ((crossfade_fade_in_amount-crossfade_fade_in_rem)<<8) /
                crossfade_fade_in_amount;

            for (i = 0; i < size; i++) {
                buf[i] = MIN(32767, MAX(-32768,
                            buf[i] + ((buf2[i] * factor) >> 8)));
            }
            break ;

        /* Join two streams. */
        case CFM_FLUSH:
            for (i = 0; i < size; i++) {
                buf[i] = buf2[i];
            }
            //memcpy((char *)buf, (char *)buf2, size*2);
            break ;
    }

    if (crossfade_fade_in_rem > size)
        crossfade_fade_in_rem = crossfade_fade_in_rem - size;
    else
        crossfade_fade_in_rem = 0;

    crossfade_rem -= size;
    if (crossfade_rem == 0)
    {
        if (crossfade_fade_in_rem > 0 && crossfade_fade_in_amount > 0)
        {
            size_insert = MIN(crossfade_fade_in_rem, length - size);
            fade_insert(&buf2[size], size_insert*2);
            crossfade_fade_in_rem -= size_insert;
        }

        if (crossfade_fade_in_rem == 0)
            crossfade_active = false;
    }

    return size + size_insert;
}

static void pcmbuf_flush_buffer(const char *buf, size_t length)
{
    size_t copy_n;
    audiobuffer_free -= length;
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

static void flush_crossfade(const char *buf, size_t length) {
    size_t copy_n;

    while (length > 0 && crossfade_active) {
        copy_n = MIN(length, pcmbuf_size - crossfade_pos);
        copy_n = 2 * crossfade((short *)&audiobuffer[crossfade_pos],
                (const short *)buf, copy_n/2);
        buf += copy_n;
        length -= copy_n;
        crossfade_pos += copy_n;
        if (crossfade_pos >= pcmbuf_size)
            crossfade_pos = 0;
    }

    pcmbuf_flush_buffer(buf, length);
}

static bool prepare_insert(size_t length)
{
    if (crossfade_init)
        crossfade_start();

    if (low_latency_mode)
    {
        /* 1/4s latency. */
        if (pcmbuf_unplayed_bytes > NATIVE_FREQUENCY * 4 / 4
            && pcm_is_playing())
            return false;
    }

    /* Need to save PCMBUF_MIN_CHUNK to prevent wrapping overwriting */
    if (audiobuffer_free < length + PCMBUF_MIN_CHUNK && !crossfade_active)
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
    if (crossfade_active) {
        *realsize = MIN(length, PCMBUF_FADE_CHUNK);
        return &guardbuf[0];
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
        *realsize = MIN(length, PCMBUF_FADE_CHUNK);
        return &guardbuf[0];
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
    if (crossfade_active) {
        length = MIN(length, PCMBUF_FADE_CHUNK);
        flush_crossfade(&guardbuf[0],length);
    }
    else
    {
        audiobuffer_free -= length;
        audiobuffer_fillpos += length;

        if (NEED_FLUSH(audiobuffer_pos + audiobuffer_fillpos))
            pcmbuf_flush_fillpos();
    }
}

bool pcmbuf_insert_buffer(const char *buf, size_t length)
{
    if (!prepare_insert(length))
        return false;

    if (crossfade_active) {
        flush_crossfade(buf,length);
    }
    else
    {
        pcmbuf_flush_buffer(buf, length);
    }
    return true;
}

/* Get a pointer to where to mix immediate audio */
static inline short* get_mix_insert_pos(void) {
    /* Give at least 1/8s clearance here */
    size_t pcmbuf_mix_back_pos =
        pcmbuf_unplayed_bytes - NATIVE_FREQUENCY * 4 / 8;

    if (audiobuffer_pos < pcmbuf_mix_back_pos)
        return (short *)&audiobuffer[pcmbuf_size +
            audiobuffer_pos - pcmbuf_mix_back_pos];
    else
        return (short *)&audiobuffer[audiobuffer_pos - pcmbuf_mix_back_pos];
}

/* Generates a constant square wave sound with a given frequency
   in Hertz for a duration in milliseconds. */
void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude)
{
    unsigned int count = 0, i = 0;
    bool state = false;
    unsigned int interval = NATIVE_FREQUENCY / frequency;
    short *buf;
    short *pcmbuf_end = (short *)guardbuf;
    bool playing = pcm_is_playing();
    size_t samples = NATIVE_FREQUENCY / 1000 * duration;

    if (playing) {
        buf = get_mix_insert_pos();
    } else {
        buf = (short *)audiobuffer;
    }
    while (i++ < samples)
    {
        long sample = *buf;
        if (state) {
            *buf++ = MIN(MAX(sample + amplitude, -32768), 32767);
            if (buf > pcmbuf_end)
                buf = (short *)audiobuffer;
            sample = *buf;
            *buf++ = MIN(MAX(sample + amplitude, -32768), 32767);
        } else {
            *buf++ = MIN(MAX(sample - amplitude, -32768), 32767);
            if (buf > pcmbuf_end)
                buf = (short *)audiobuffer;
            sample = *buf;
            *buf++ = MIN(MAX(sample - amplitude, -32768), 32767);
        }

        if (++count >= interval)
        {
            count = 0;
            state = !state;
        }
        if (buf > pcmbuf_end)
            buf = (short *)audiobuffer;
    }
    if (!playing) {
        pcm_play_data(NULL, (unsigned char *)audiobuffer, samples * 4);
    }
}

/* Returns pcm buffer usage in percents (0 to 100). */
int pcmbuf_usage(void)
{
    return pcmbuf_unplayed_bytes * 100 / pcmbuf_size;
}

int pcmbuf_mix_usage(void)
{
    return pcmbuf_mix_used_bytes * 100 / pcmbuf_unplayed_bytes;
}

void pcmbuf_reset_mixpos(void)
{
    mixpos = get_mix_insert_pos();
    pcmbuf_mix_used_bytes = 0;
}

void pcmbuf_mix(char *buf, size_t length)
{
    short *ibuf = (short *)buf;
    short *pcmbuf_end = (short *)guardbuf;

    if (pcmbuf_mix_used_bytes == 0)
        pcmbuf_reset_mixpos();

    pcmbuf_mix_used_bytes += length;
    length /= 2;

    while (length-- > 0) {
        long sample = *ibuf++;
        sample += *mixpos >> 2;
        *mixpos++ = MIN(MAX(sample, -32768), 32767);

        if (mixpos >= pcmbuf_end)
            mixpos = (short *)audiobuffer;
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

