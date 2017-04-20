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
#include "pcm.h"

/*** PCM handle config settings ***/

#ifdef HAVE_RECORING
#define PCM_EXTRA_HANDLES 1
#else
#define PCM_EXTRA_HANDLES 0
#endif

/* Audio, voice, beep, plugin + aux */
#define PCM_MAX_HANDLES (4 + PCM_EXTRA_HANDLES)

/*** Internal PCM buffer config settings ***/

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

struct pcm_hw_settings
{
    unsigned long samplerate;
    int           fsel;
};

struct pcm_stream_desc;

struct pcm_dma_t_convert
{
    void (* write)(struct pcm_stream_desc *desc,
                   void *out,
                   unsigned long count);
    void (*   mix)(struct pcm_stream_desc *desc,
                   void *out,
                   unsigned long count);
};

/* Handle structure used for nearly all PCM driver interaction */
struct pcm_stream_desc
{
    unsigned int flags;             /* stream attributes */
    pcm_handle_t id;                /* current/next descriptor handle */
    int state;                      /* playback/recording state */
    union {
    struct { /* play */
    const void    *addr;            /* buffer pointer */
    unsigned long frames;           /* frames remaining */
    pcm_play_callback_type callback; /* registered callback */
    unsigned long prev_frames;      /* frames consumed data in prev. cycle */
    int32_t       amplitude;        /* stream's amplitude factor */
    };
#ifdef HAVE_RECORDING
    struct { /* rec */
    void          *addr_rec;        /* buffer pointer */
    unsigned long frames_rec;       /* frames remaining */
    pcm_record_callback_type callback_rec; /* registered callback */
    unsigned long seqnum_rec;       /* buffer sequence number */
    };
#endif /* HAVE_RECORDING */
    };
#ifdef CONFIG_PCM_MULTISIZE
    size_t        sample_size;      /* size of single sample */
    unsigned int  sample_bits;      /* # of value bits + sign bit */
#endif /* CONFIG_PCM_MULTISIZE */
    size_t        frame_size;       /* size of each frame in bytes */
    struct pcm_format format;       /* current format */
    struct pcm_dma_t_convert conv;  /* conversion mix/write functions */
};

#ifdef CONFIG_PCM_MULTISIZE
#define pcm_desc_sample_size(desc)  ((desc)->sample_size)
#define pcm_desc_sample_bits(desc)  ((desc)->sample_bits)
#else /* ndef CONFIG_PCM_MULTISIZE */
#define pcm_desc_sample_size(desc)  2
#define pcm_desc_sample_bits(desc)  16
#endif /* CONFIG_PCM_MULTISIZE */

#define pcm_desc_frame_size(desc)   ((desc)->frame_size)

/*** PCM mixer interface ***/
int pcm_mixer_format_config(struct pcm_stream_desc *desc);
unsigned long pcm_mixer_fill_buffer(int status, void *outbuf,
                                    unsigned long frames);
void pcm_mixer_activate_stream(struct pcm_stream_desc *desc);
void pcm_mixer_deactivate_stream(struct pcm_stream_desc *desc);
bool pcm_mixer_apply_settings(struct pcm_stream_desc *desc,
                              const struct pcm_hw_settings *settings);

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
static FORCE_INLINE void pcm_sw_volume_apply(void *buffer,
                                             unsigned long frames)
{
    extern void (* pcm_sw_volume_buffer_amp_fn)(void *, unsigned long);
    if (pcm_sw_volume_buffer_amp_fn) {
        pcm_sw_volume_buffer_amp_fn(buffer, frames);
    }
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

void pcm_stream_stopped(struct pcm_stream_desc *desc);
void pcm_play_lock_device_callback(void);
void pcm_play_unlock_device_callback(void);

/*** Target driver implemented interface ***/

/* Playback + Recording */
void pcm_dma_init(const struct pcm_hw_settings *settings);
void pcm_dma_postinit(const struct pcm_hw_settings *settings);
void pcm_dma_apply_settings(const struct pcm_hw_settings *settings);

/* Playback */
void pcm_play_dma_lock(void);
void pcm_play_dma_unlock(void);
void pcm_play_dma_prepare(void);
void pcm_play_dma_send_frames(const void *addr, unsigned long frames);
void pcm_play_dma_pause(bool pause);
void pcm_play_dma_stop(void);
void pcm_play_dma_complete_callback(int status);
unsigned long pcm_play_dma_get_frames_waiting(void);

const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem);

#ifdef HAVE_RECORDING
/* Recording */
void pcm_rec_dma_lock(void);
void pcm_rec_dma_unlock(void);
void pcm_rec_dma_init(void) INIT_ATTR;
void pcm_rec_dma_close(void);
void pcm_rec_dma_prepare(void);
void pcm_rec_dma_capture_frames(void *addr, unsigned long frames);
void pcm_rec_dma_stop(void);
void pcm_rec_dma_complete_callback(int status);

const void * pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail);

#endif /* HAVE_RECORDING */

/* Global internals */
void pcm_init(void) INIT_ATTR;

#endif /* PCM_INTERNAL_H */
