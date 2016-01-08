 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "xrick/config.h"

#ifdef ENABLE_SOUND

#include "xrick/system/system.h"

#include "xrick/game.h"
#include "xrick/debug.h"
#include "xrick/system/syssnd_rockbox.h"

#include "plugin.h"

/*
 * Global variables
 */
const U8 syssnd_period = 20;

/*
 * Local variables
 */
enum
{
    SYSSND_MIX_CHANNELS = 5,
    SYSSND_MIX_SAMPLES = 1024, /* try changing this value if sound mixing is too slow or choppy */
    SYSSND_SOURCE_SAMPLES = SYSSND_MIX_SAMPLES / 2
};

/* channels to be mixed */
static channel_t channels[SYSSND_MIX_CHANNELS];
/* buffer used to mix sounds sent to pcm playback, stores 16b stereo 44Khz audio samples */
enum { AUDIO_BUFFER_COUNT = 4 };
typedef struct
{
    U32 data[SYSSND_MIX_SAMPLES];
    size_t length; /* in 8 bit mono samples */
} mix_buffer_t;
static mix_buffer_t mixBuffers[AUDIO_BUFFER_COUNT];
static size_t writeIndex;
static size_t readIndex;
static size_t fillCount;
static bool isAudioPlaying;
static bool isAudioInitialised = false;

/*
 * Prototypes
 */
static void endChannel(size_t c);
static void get_more(const void **start, size_t *size);

/*
 * Deactivate channel
 */
static void endChannel(size_t c)
{
    channels[c].loop = 0;
    channels[c].sound = NULL;
}

/*
 * Audio callback
 */
static void get_more(const void **start, size_t *size)
{
    if (fillCount > 0)
    {
        /* Store output data address and size. */
        *start = mixBuffers[readIndex].data;
        *size = mixBuffers[readIndex].length * 8;

        /* Free this part of output buffer. */
        mixBuffers[readIndex].length = 0;

        /* Advance to the next part of output buffer. */
        readIndex = (readIndex + 1) & (AUDIO_BUFFER_COUNT - 1);
        fillCount--;
    }
    else
    {
        /* Nothing to play. */
        isAudioPlaying = false;
    }
}

/*
 * Mix audio samples and fill playback buffer
 */
void syssnd_update(void)
{
    if (!isAudioInitialised)
    {
        return;
    }

    for (;;)
    {
        size_t c;
        size_t sampleOffset;
        size_t maxSampleCount;
        bool isFirstSound;
        U8 *sourceBuf, *sourceBufEnd;
        U32 *destBuf;

        /* Cancel if whole buffer filled. */
        if (fillCount >= (AUDIO_BUFFER_COUNT - 1))
        {
            return;
        }

        maxSampleCount = 0;

        sampleOffset = mixBuffers[writeIndex].length;
        destBuf = mixBuffers[writeIndex].data + sampleOffset * 2;

        isFirstSound = true;
        for (c = 0; c < SYSSND_MIX_CHANNELS ; ++c)
        {
            U32 * mixBuffer;
            size_t sampleCount;
            channel_t * channel = &channels[c];

            if (!channel->sound /* no sound to play on this channel */
                || (channel->loop == 0)) /* channel is inactive */
            {
                continue;
            }

            if (isFirstSound)
            {
                /* clear mixing buffer */
                rb->memset(destBuf, 0, (SYSSND_MIX_SAMPLES - (sampleOffset * 2)) * sizeof(U32));
                isFirstSound = false;
            }

            sampleCount = MIN(SYSSND_SOURCE_SAMPLES - sampleOffset, channel->len);
            if (maxSampleCount < sampleCount)
            {
                maxSampleCount = sampleCount;
            }

            /* mix sound samples */
            mixBuffer = destBuf;
            sourceBuf = channel->buf;
            sourceBufEnd = channel->buf + sampleCount;
            while (sourceBuf < sourceBufEnd)
            {
                /* Convert from unsigned 8 bit mono 22khz to signed 16 bit stereo 44khz */
                const int sourceSample = *sourceBuf++;
                int monoSample = (sourceSample - 0x80) << 8;
                U32 stereoSample = *mixBuffer;
                monoSample += (S32)(stereoSample) >> 16;
                if (monoSample >= 0x8000)
                {
                    monoSample = 0x7FFF;
                }
                else if (monoSample < -0x8000)
                {
                    monoSample = -0x8000;
                }
                stereoSample = (U16)monoSample | ((U16)monoSample << 16);
                *mixBuffer++ = stereoSample;
                *mixBuffer++ = stereoSample;
            }
            channel->buf = sourceBufEnd;

            channel->len -= sampleCount;
            if (channel->len == 0) /* ending ? */
            {
                if (channel->loop > 0)
                {
                    channel->loop--;
                }
                if (channel->loop)
                {
                    /* just loop */
                    IFDEBUG_AUDIO2(sys_printf("xrick/audio: channel %d - loop\n", c););
                    channel->buf = channel->sound->buf;
                    channel->len = channel->sound->len;
                }
                else
                {
                    /* end for real */
                    IFDEBUG_AUDIO2(sys_printf("xrick/audio: channel %d - end\n", c););
                    endChannel(c);
                }
            }
        }

        if (maxSampleCount == 0)
        {
            return;
        }

        mixBuffers[writeIndex].length += maxSampleCount;

        /* Advance one part of audio buffer. */
        writeIndex = (writeIndex + 1) & (AUDIO_BUFFER_COUNT - 1);
        fillCount++;

        if (!isAudioPlaying && fillCount > 0)
        {
            rb->pcm_play_data(&get_more, NULL, NULL, 0);
            isAudioPlaying = true;
        }
    }
}

/*
 * Initialise audio
 */
bool syssnd_init(void)
{
    if (isAudioInitialised)
    {
        return true;
    }

    IFDEBUG_AUDIO(sys_printf("xrick/audio: start\n"););

    rb->talk_disable(true);

    /* Stop playback to reconfigure audio settings and acquire audio buffer */
    rb->pcm_play_stop();

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    rb->pcm_set_frequency(HW_FREQ_44);
    rb->pcm_apply_settings();

    rb->memset(channels, 0, sizeof(channels));
    rb->memset(mixBuffers, 0, sizeof(mixBuffers));

    writeIndex = 0;
    readIndex = 0;
    fillCount = 0;
    isAudioPlaying = false;

    isAudioInitialised = true;
    IFDEBUG_AUDIO(sys_printf("xrick/audio: ready\n"););
    return true;
}

/*
 * Shutdown
 */
void syssnd_shutdown(void)
{
    if (!isAudioInitialised)
    {
        return;
    }

    /* Stop playback. */
    rb->pcm_play_stop();

    /* Reset playing status. */
    isAudioPlaying = false;

    /* Restore default sampling rate. */
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
    rb->pcm_apply_settings();

    rb->talk_disable(false);

    isAudioInitialised = false;
    IFDEBUG_AUDIO(sys_printf("xrick/audio: stop\n"););
}

/*
 * Play a sound
 *
 * loop: number of times the sound should be played, -1 to loop forever
 *
 * NOTE if sound is already playing, simply reset it (i.e. can not have
 * twice the same sound playing -- tends to become noisy when too many
 * bad guys die at the same time).
 */
void syssnd_play(sound_t *sound, S8 loop)
{
    size_t c;

    if (!isAudioInitialised || !sound)
    {
        return;
    }

    c = 0;
    while (channels[c].sound != sound &&
           channels[c].loop != 0 &&
           c < SYSSND_MIX_CHANNELS)
    {
        c++;
    }
    if (c >= SYSSND_MIX_CHANNELS)
    {
        return;
    }

    if (!sound->buf)
    {
        syssnd_load(sound);
        if (!sound->buf)
        {
            sys_error("(audio) can not load %s", sound->name);
            return;
        }
    }

    IFDEBUG_AUDIO(
        if (channels[c].sound == sound)
        {
            sys_printf("xrick/audio: already playing %s on channel %d - resetting\n",
                sound->name, c);
        }
        else
        {
            sys_printf("xrick/audio: playing %s on channel %d\n", sound->name, c);
        }
    );

    channels[c].loop = loop;
    channels[c].sound = sound;
    channels[c].buf = sound->buf;
    channels[c].len = sound->len;
}

/*
 * Pause all sounds
 */
void syssnd_pauseAll(bool pause)
{
    if (!isAudioInitialised)
    {
        return;
    }

    rb->pcm_play_lock();
    rb->pcm_play_pause(!pause);
    rb->pcm_play_unlock();
}

/*
 * Stop a sound
 */
void syssnd_stop(sound_t *sound)
{
    size_t c;

    if (!isAudioInitialised || !sound)
    {
        return;
    }

    for (c = 0; c < SYSSND_MIX_CHANNELS; c++)
    {
        if (channels[c].sound == sound)
        {
            endChannel(c);
        }
    }
}

/*
 * Stops all channels.
 */
void syssnd_stopAll(void)
{
    size_t c;

    if (!isAudioInitialised)
    {
        return;
    }

    for (c = 0; c < SYSSND_MIX_CHANNELS; c++)
    {
        if (channels[c].sound)
        {
            endChannel(c);
        }
    }
}

/*
 * Load a sound.
 */
void syssnd_load(sound_t *sound)
{
    int bytesRead;
    file_t fp;
    bool success;

    if (!isAudioInitialised || !sound)
    {
        return;
    }

    success = false;
    do
    {
        sound->buf = sysmem_push(sound->len);
        if (!sound->buf)
        {
            sys_error("(audio) not enough memory for \"%s\", %d bytes needed", sound->name, sound->len);
            break;
        }

        fp = sysfile_open(sound->name);
        if (!fp)
        {
            sys_error("(audio) unable to open \"%s\"", sound->name);
            break;
        }

        sysfile_seek(fp, sizeof(wave_header_t), SEEK_SET); /* skip WAVE header */

        bytesRead = sysfile_read(fp, sound->buf, sound->len, 1);
        sysfile_close(fp);
        if (bytesRead != 1)
        {
            sys_error("(audio) unable to read from \"%s\"", sound->name);
            break;
        }

        success = true;
    } while (false);

    if (!success)
    {
        sysmem_pop(sound->buf);
        sound->buf = NULL;
        sound->len = 0;
        return;
    }

    IFDEBUG_AUDIO(sys_printf("xrick/audio: successfully loaded \"%s\"\n", sound->name););
}

/*
 * Unload a sound.
 */
void syssnd_unload(sound_t *sound)
{
    if (!isAudioInitialised || !sound || !sound->buf)
    {
        return;
    }

    sysmem_pop(sound->buf);
    sound->buf = NULL;
    sound->len = 0;
}

#endif /* ENABLE_SOUND */

/* eof */
