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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCM_H
#define PCM_H

#include <sys/types.h> /* off_t */
#include <errno.h>
#include "config.h"
#include "system.h"
#include "gcc_extensions.h"

/* do this to avoid and explicit typing of the handle */
typedef unsigned long pcm_handle_t;

#ifdef HAVE_SPDIF_REC
#define E_DMA_SPDIF     __ELASTERROR
#define PCM_STATUS_CONTINUABLE(status) ((status) == 0 || (status) == -E_DMA_SPDIF)
#else
#define PCM_STATUS_CONTINUABLE(status) ((status) == 0)
#endif

enum pcm_state
{
    PCM_STATE_STOPPED = 0x0,
    PCM_STATE_RUNNING = 0x1,
    PCM_STATE_PAUSED  = 0x2,
};

#define PCM_PLAY_STATE_MASK    0x3
#define PCM_REC_STATE_MASK     0x4

/* Full-duplex is possible on some players and the state values combine */ 
#define PCM_PLAY_STATE(status) ((status) & PCM_PLAY_STATE_MASK)
#define PCM_REC_STATE(status)  ((status) & PCM_REC_STATE_MASK)

enum pcm_handle_flags
{
    PCM_STREAM_PLAYBACK  = 0x0,
    PCM_STREAM_RECORDING = 0x1,
    PCM_STREAM_TYPE_MASK = 0x3,
};

#define PCM_FLAGS_STREAM_TYPE(flags) ((flags) & PCM_STREAM_TYPE_MASK)

pcm_handle_t pcm_open(unsigned int flags);
int pcm_close(pcm_handle_t pcm);
int pcm_lock_callback(pcm_handle_t pcm);
int pcm_unlock_callback(pcm_handle_t pcm);
unsigned long pcm_set_frequency(pcm_handle_t pcm, unsigned long samplerate);
unsigned long pcm_get_frequency(pcm_handle_t pcm);
int pcm_get_state(pcm_handle_t pcm);
int pcm_pause(pcm_handle_t pcm, bool pause);
int pcm_stop(pcm_handle_t pcm);
int pcm_set_amplitude(pcm_handle_t pcm, long amplitude);

#define PCM_FORMAT_CAP_S16_2 (1ul << 0)
#define PCM_FORMAT_CAP_S24_4 (1ul << 1)
#define PCM_FORMAT_CAP_S32_4 (1ul << 2)
#define PCM_FORMAT_CAP_S24_3 (1ul << 3)

#define PCM_FORMAT_CAP_2_BYTE_CAPS \
    (PCM_FORMAT_CAP_S16_2)

#define PCM_FORMAT_CAP_3_BYTE_CAPS \
    (PCM_FORMAT_CAP_S24_3)

#define PCM_FORMAT_CAP_4_BYTE_CAPS \
    (PCM_FORMAT_CAP_S24_4 | PCM_FORMAT_CAP_S32_4)

/* Override in config file otherwise use these defaults */
#ifndef CONFIG_PCM_FORMAT_CAPS
#define CONFIG_PCM_FORMAT_CAPS  PCM_FORMAT_CAP_S16_2
#endif

#if !(CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_S16_2)
#error PCM_FORMAT_S16_2 support is compulsory; please check CONFIG_PCM_FORMAT_CAPS
#endif

#define PCM_FORMAT_S16_2 0 /* compulsory */
#define PCM_FORMAT_S24_4 1
#define PCM_FORMAT_S32_4 2
#define PCM_FORMAT_S24_3 3
#define PCM_NUM_FORMATS  4

/* Override in config file otherwise use these defaults */
#ifndef PCM_DMA_T_FORMAT_CODE
#define PCM_DMA_T_FORMAT_CODE  PCM_FORMAT_S16_2
#endif

#ifndef PCM_DMA_T_CHANNELS
#define PCM_DMA_T_CHANNELS     2
#endif

struct pcm_format
{
    unsigned int code;   /* basic format of one sample */
    unsigned int chnum;  /* number of channels */
};

#define PCM_FORMAT(fmtcode, fmtchnum) \
    (&(struct pcm_format){ .code = (fmtcode), .chnum = (fmtchnum) })

#define PCM_FORMAT_DMA_T() \
    PCM_FORMAT(PCM_DMA_T_FORMAT_CODE, PCM_DMA_T_CHANNELS)

/* pcm_dma_t is the native sample type of the native format */
#if PCM_DMA_T_FORMAT_CODE == PCM_FORMAT_S16_2
/* 2 bytes */
typedef int16_t pcm_dma_t;
#define PCM_DMA_T_MIN    INT16_MIN
#define PCM_DMA_T_MAX    INT16_MAX
#define PCM_DMA_T_SIZE   ((size_t)2)
#define PCM_DMA_T_DEPTH  16
#elif PCM_DMA_T_FORMAT_CODE == PCM_FORMAT_S24_4
/* 4 bytes */
typedef int32_t pcm_dma_t;
#define PCM_DMA_T_MIN    (INT32_MIN/0x100)
#define PCM_DMA_T_MAX    (INT32_MAX/0x100)
#define PCM_DMA_T_SIZE   ((size_t)4)
#define PCM_DMA_T_DEPTH  24
#elif PCM_DMA_T_FORMAT_CODE == PCM_FORMAT_S32_4
/* 4 bytes */
typedef int32_t pcm_dma_t;
#define PCM_DMA_T_MIN    INT32_MIN
#define PCM_DMA_T_MAX    INT32_MAX
#define PCM_DMA_T_SIZE   ((size_t)4)
#define PCM_DMA_T_DEPTH  32
#else
/* unknown */
#error Invalid native PCM format code.
#endif

#define PCM_DMA_T_FRAME_SIZE ((size_t)(PCM_DMA_T_CHANNELS*PCM_DMA_T_SIZE))

/* Direct volume amplitude factors */
#define PCM_AMP_MAX      (0x10000L)
#define PCM_AMP_MIN      (0x00001L)
#define PCM_AMP_UNITY    (0x10000L)
#define PCM_AMP_MUTE     (0x00000L)
#define PCM_AMP_FRACBITS (16)

#if 0
/* Centibel volume levels */
#define PCM_VOL_UNITY    ( 000L)
#define PCM_VOL_MAX      ( 000L)
#define PCM_VOL_MIN      (-790L)
#define PCM_VOL_MUTE     (-800L)
#endif /* later! */

/** RAW PCM routines used with playback and recording **/

/* Typedef for registered data callback */
typedef int (*pcm_play_callback_type)(int status, const void **start,
                                      unsigned long *frames);

/* This is for playing "raw" PCM data */
int pcm_play_data(pcm_handle_t pcm,
                  pcm_play_callback_type callback,
                  const void *addr,
                  unsigned long frames,
                  const struct pcm_format *format);

/* Kept internally for global PCM and used by mixer's verion of peak
   calculation */
struct pcm_peaks
{
    /* data maintained for internal use */
    long period;            /* For tracking calling period */
    long tick;              /* Last tick called */
    const void *addr;       /* last read address */
    unsigned long frames;   /* frames remaining or now available */
    const void *end_addr;   /* end of last readout */
#ifdef HAVE_RECORDING
    unsigned long seq;      /* record buffer sequence stamp */
#endif
    uint32_t peak[];        /* peaks values (in containing allocation) */
};

/* initialize when allocating non-statically */
static inline void pcm_peaks_init(struct pcm_peaks *peaks)
{
    *peaks = (struct pcm_peaks){ .frames = 0 };
}

int pcm_get_peaks(pcm_handle_t pcm, struct pcm_peaks *peaks);
const void * pcm_get_unplayed_data(pcm_handle_t pcm,
                                   unsigned long *frames_rem);
unsigned long pcm_get_frames_waiting(pcm_handle_t pcm);
int pcm_adjust_address(pcm_handle_t pcm, off_t offset);

#ifdef HAVE_RECORDING
/** PCM recording routines **/

/* Typedef for registered data callback */
typedef int (*pcm_record_callback_type)(int status, void **start,
                                        unsigned long *frames);

/* To use these functions, the stream must have been opened with
 * PCM_STREAM_RECORDING specified */

/* Start capturing PCM data */
int pcm_record_data(pcm_handle_t pcm,
                    pcm_record_callback_type callback,
                    void *addr,
                    unsigned long frames,
                    const struct pcm_format *format);

#endif /* HAVE_RECORDING */

/** pcm_dma_t assistance **/

/* Packed 24-bit audio will make these more hideous; these assume 16 or 32
 * bits for the moment */
static FORCE_INLINE void *
    pcm_dma_t_ptr(const void *p, int preidx)
{
    return (pcm_dma_t *)p + preidx;
}

static FORCE_INLINE int32_t
    __pcm_dma_t_read_preidx(const void **pp,
                            int preidx,
                            bool writeback)
{
    const void *p = (const pcm_dma_t *)*pp + preidx;
    int32_t value = *(pcm_dma_t *)p;

    if (writeback) {
        *pp = p;
    }

    return value;
}

#define pcm_dma_t_read_preidx(pp, preidx, writeback) \
    __pcm_dma_t_read_preidx((const void **)(pp), (preidx), (writeback))

static FORCE_INLINE void
    __pcm_dma_t_write_preidx(void **pp,
                             int32_t value,
                             int preidx,
                             bool writeback)
{
    void *p = (pcm_dma_t *)*pp + preidx;
    *(pcm_dma_t *)p = value;

    if (writeback) {
        *pp = p;
    }
}

#define pcm_dma_t_write_preidx(pp, value, preidx, writeback) \
    __pcm_dma_t_write_preidx((void **)(pp), (value), (preidx), (writeback))

static FORCE_INLINE void
    __pcm_dma_t_advance(const void **pp,
                        int idx,
                        unsigned int numchannels)
{
    *pp = PTR_ADD(*pp, PCM_DMA_T_SIZE*idx*(int)numchannels);
}

#define pcm_dma_t_advance(pp, idx, numchannels) \
    __pcm_dma_t_advance((const void **)(pp), (idx), (numchannels))

static FORCE_INLINE int32_t
    __pcm_dma_t_read_postidx(const void **pp,
                             int postidx,
                             unsigned int numchannels)
{
    int32_t value = pcm_dma_t_read_preidx(pp, 0, false);
    pcm_dma_t_advance(pp, postidx, numchannels);
    return value;
}

#define pcm_dma_t_read_postidx(pp, postidx, numchannels) \
    __pcm_dma_t_read_postidx((const void **)(pp), (postidx), (numchannels))

static FORCE_INLINE void
    __pcm_dma_t_write_postidx(void **pp,
                              int32_t value,
                              int postidx,
                              unsigned int numchannels)
{
    pcm_dma_t_write_preidx(pp, value, 0, false);
    pcm_dma_t_advance(pp, postidx, numchannels);
}

#define pcm_dma_t_write_postidx(pp, value, postidx, numchannels) \
    __pcm_dma_t_write_postidx((void **)(pp), (value), (postidx), (numchannels))

#define pcm_dma_t_frames2size(frames) \
    ((size_t)(PCM_DMA_T_FRAME_SIZE*(frames)))

#define pcm_dma_t_size2frames(nbyte) \
    ((unsigned long)((nbyte) / PCM_DMA_T_FRAME_SIZE))

static FORCE_INLINE int32_t
    pcm_dma_t_scale_to(int32_t value, unsigned int bits)
{
    int shift = bits - PCM_DMA_T_DEPTH;

    if (shift < 0) {
        value >>= -shift;
    }
    else if (shift > 0) {
        value <<= shift;
    }

    return value;
}

#endif /* PCM_H */
