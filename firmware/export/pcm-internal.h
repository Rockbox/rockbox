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
#ifndef PCM_INTERNAL_H
#define PCM_INTERNAL_H

#include "config.h"
#include <errno.h>

/*** PCM handle config settings ***/

/* Audio, voice, beep, plugin + aux */
#define PCM_MAX_HANDLES (4 + PCM_EXTRA_HANDLES)

#ifdef HAVE_RECORING
#define PCM_EXTRA_HANDLES 1
#endif

/*** PCM mixer config settings ***/

/* Length of PCM frames (always) */
#if CONFIG_CPU == PP5002
/* There's far less time to do mixing because HW FIFOs are short */
#define PCM_DBL_BUF_FRAMES 64
#elif (CONFIG_PLATFORM & PLATFORM_MAEMO5) || defined(DX50) || defined(DX90)
/* Maemo 5 needs 2048 samples for decent performance.
   Otherwise the locking overhead inside gstreamer costs too much */
/* iBasso Devices: Match Rockbox PCM buffer size to ALSA PCM buffer size
   to minimize memory transfers. */
#define PCM_DBL_BUF_FRAMES 2048
#else
/* Assume HW DMA engine is available or sufficient latency exists in the
   PCM pathway */
#define PCM_DBL_BUF_FRAMES 256
#endif

#if defined(CPU_COLDFIRE) ||  defined(CPU_PP)
/* For Coldfire, it's just faster
   For PortalPlayer, this also avoids more expensive cache coherency */
#define PCM_DBL_BUF_IBSS        IBSS_ATTR
#else
/* Otherwise can't DMA from IRAM, IRAM is pointless or worse */
#define PCM_DBL_BUF_IBSS
#endif

#if defined(CPU_COLDFIRE) || defined(CPU_PP)
#define MIXER_CALLBACK_ICODE    ICODE_ATTR
#else
#define MIXER_CALLBACK_ICODE
#endif

/*** PCM mixer interface ***/
int pcm_mixer_format_config(struct pcm_handle *pcm);
unsigned long pcm_mixer_fill_buffer(int status, void *outbuf,
                                    unsigned long frames);
void pcm_mixer_activate_stream(struct pcm_handle *pcm);
void pcm_mixer_deactivate_stream(struct pcm_handle *pcm);
void pcm_mixer_reset(void);

typedef void (* pcm_mixer_samples_fn)(void *out, const struct pcm_handle *pcm,
                                      unsigned long count);

struct mix_write_samples_desc
{
    pcm_mixer_samples_fn write;
    pcm_mixer_samples_fn mix;
};

#ifdef HAVE_SW_VOLUME_CONTROL
/*** Software volume control config settings ***/

#ifndef PCM_SW_VOLUME_FRACBITS
/* Allows -73 to +6dB gain, sans large integer math */
#define PCM_SW_VOLUME_FRACBITS  (15)
#endif

/* Constants selected based on integer math overflow avoidance */
#if PCM_SW_VOLUME_FRACBITS <= 16
#define PCM_SW_VOLUME_FACTOR_MAX    0x00010000L
#elif PCM_SW_VOLUME_FRACBITS <= 30
#define PCM_SW_VOLUME_FACTOR_MAX    0x40000000L
#else
#error PCM_SW_VOLUME_FRACBITS value is invalid.
#endif /* PCM_SW_VOLUME_FRACBITS */

#define PCM_SW_VOLUME_FACTOR_UNITY (1L << PCM_SW_VOLUME_FRACBITS)

/*** Software volume control PCM interface ***/
static FORCE_INLINE void pcm_sw_volume_apply(void *buffer, unsigned long frames)
{
    extern void (* pcm_sw_volume_scaling_fn)(void *, const void *, unsigned long);
    pcm_sw_volume_scaling_fn(buffer, buffer, frames);
}

void pcm_sw_volume_sync_factors(void);

#endif /* HAVE_SW_VOLUME_CONTROL */

#define ALIGN_AUDIOBUF(buf, frames, alignsize) \
    ({  uintptr_t __p = (uintptr_t)(buf); \
        size_t __az = (alignsize);        \
        size_t __amt = __p % __az;        \
        size_t __fr = (frames);           \
        if (__amt && __fr) {              \
            __p += __az - __amt;          \
            __fr--;                       \
        }                                 \
        (buf) = (typeof (buf))__p;        \
        (frames) = __fr;                  })

/* Handle structure used for nearly all PCM driver interaction */
struct pcm_stream_desc
{
    unsigned int flags;              /* stream attributes */
    pcm_handle_t id;                 /* current/next descriptor handle */
    union {
    const void *addr;                /* buffer pointer */
#ifdef HAVE_RECORDING
    void       *addr_rec;
#endif
    };
    unsigned long frames;            /* frames remaining */
    union {
    unsigned long prev_frames;       /* frames consumed data in prev. cycle */
#ifdef HAVE_RECORDING
    unsigned long seqnum_rec;        /* buffer sequence number */
#endif
    };
    size_t        sample_size;       /* size of single sample */
    size_t        frame_size;        /* size of each frame in bytes */
    int32_t       amplitude;         /* amp. factor: 0x0000 = mute, 0x10000 = unity */
    union {
    pcm_play_callback_type callback; /* registered callback */
#ifdef HAVE_RECORDING
    pcm_rec_callback_type callback_rec;
#endif
    };
    int state;                       /* playback/recording state */
    struct pcm_format format;        /* current format */
};

void pcm_stream_stopped(struct pcm_handle *pcm);
void pcm_play_lock_device_callback(void);
void pcm_play_unlock_device_callback(void);

#define pcm_curr_sampr \
    ({ extern unsigned long __pcm_curr_sampr; \
       *(const unsigned long *)&__pcm_curr_sampr; })

#define pcm_sampr \
    ({ extern unsigned long __pcm_sampr; \
       *(const unsigned long *)&__pcm_sampr; })

#define pcm_fsel \
    ({ extern int __pcm_fsel; \
       *(const int *)&__pcm_fsel })

/*** Target driver implemented interface ***/

/* Playback + Recording */
void pcm_dma_apply_settings(void);

/* Playback */
void pcm_play_dma_lock(void);
void pcm_play_dma_unlock(void);
void pcm_play_dma_init(void) INIT_ATTR;
void pcm_play_dma_postinit(void);
void pcm_play_dma_prepare(void);
void pcm_play_dma_send_frames(const void *addr, unsigned long frames);
void pcm_play_dma_pause(bool pause);
void pcm_play_dma_stop(void);
void pcm_play_dma_complete_callback(int status);

const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem);

#ifdef HAVE_RECORDING
/* Recording */
void pcm_rec_dma_lock(void);
void pcm_rec_dma_unlock(void);
void pcm_rec_dma_init(void);
void pcm_rec_dma_close(void);
void pcm_rec_dma_prepare(void);
void pcm_rec_dma_capture_frames(void *addr, unsigned long frames);
void pcm_rec_dma_stop(void);
void pcm_rec_dma_complete_callback(int status);

const void * pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail);

#endif /* HAVE_RECORDING */

/* Global internals */
void pcm_init(void) INIT_ATTR;
void pcm_postinit(void);
bool pcm_is_initialized(void);
void pcm_reset(void);

#endif /* PCM_INTERNAL_H */
