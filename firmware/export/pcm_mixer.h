/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Michael Sevakis
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

#ifndef PCM_MIXER_H
#define PCM_MIXER_H

#include <sys/types.h>

/** Simple config **/

/* Length of PCM frames (always) */
#if CONFIG_CPU == PP5002
/* There's far less time to do mixing because HW FIFOs are short */
#define MIX_FRAME_SAMPLES 64
#elif (CONFIG_PLATFORM & PLATFORM_MAEMO5)
/* Maemo 5 needs 2048 samples for decent performance.
   Otherwise the locking overhead inside gstreamer costs too much */
#define MIX_FRAME_SAMPLES 2048
/* Assume HW DMA engine is available or sufficient latency exists in the
   PCM pathway */
#else
#define MIX_FRAME_SAMPLES 256
#endif

#if defined(CPU_COLDFIRE) ||  defined(CPU_PP)
/* For Coldfire, it's just faster
   For PortalPlayer, this also avoids more expensive cache coherency */
#define DOWNMIX_BUF_IBSS        IBSS_ATTR
#else
/* Otherwise can't DMA from IRAM, IRAM is pointless or worse */
#define DOWNMIX_BUF_IBSS
#endif

#if defined(CPU_COLDFIRE) || defined(CPU_PP)
#define MIXER_CALLBACK_ICODE    ICODE_ATTR
#else
#define MIXER_CALLBACK_ICODE
#endif


/** Definitions **/

/* Channels are preassigned for simplicity */
enum pcm_mixer_channel
{
    PCM_MIXER_CHAN_PLAYBACK = 0,
    PCM_MIXER_CHAN_VOICE,
#ifndef HAVE_HARDWARE_BEEP
    PCM_MIXER_CHAN_BEEP,
#endif
    /* Add new channel indexes above this line */
    PCM_MIXER_NUM_CHANNELS,
};

/* Channel playback states */
enum channel_status
{
    CHANNEL_STOPPED = 0,
    CHANNEL_PLAYING,
    CHANNEL_PAUSED,
};

#define MIX_AMP_UNITY    0x00010000
#define MIX_AMP_MUTE     0x00000000


/** Public interfaces **/

/* Start playback on a channel */
void mixer_channel_play_data(enum pcm_mixer_channel channel,
                             pcm_play_callback_type get_more,
                             const void *start, size_t size);

/* Pause or resume a channel (when started) */
void mixer_channel_play_pause(enum pcm_mixer_channel channel, bool play);

/* Stop playback on a channel */
void mixer_channel_stop(enum pcm_mixer_channel channel);

/* Set channel's amplitude factor */
void mixer_channel_set_amplitude(enum pcm_mixer_channel channel,
                                 unsigned int amplitude);

/* Return channel's playback status */
enum channel_status mixer_channel_status(enum pcm_mixer_channel channel);

/* Returns amount data remaining in channel before next callback */
size_t mixer_channel_get_bytes_waiting(enum pcm_mixer_channel channel);

/* Return pointer to channel's playing audio data and the size remaining */
const void * mixer_channel_get_buffer(enum pcm_mixer_channel channel, int *count);

/* Calculate peak values for channel */
void mixer_channel_calculate_peaks(enum pcm_mixer_channel channel,
                                   struct pcm_peaks *peaks);

/* Adjust channel pointer by a given offset to support movable buffers */
void mixer_adjust_channel_address(enum pcm_mixer_channel channel,
                                  off_t offset);

/* Stop ALL channels and PCM and reset state */
void mixer_reset(void);

#endif /* PCM_MIXER_H */
