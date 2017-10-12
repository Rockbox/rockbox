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
#define PCM_STATUS_CONTINUABLE(status) ((status) == 0 || \
                                        (status) == -E_DMA_SPDIF)
#else /* ndef HAVE_SPDIF_REC */
#define PCM_STATUS_CONTINUABLE(status) ((status) == 0)
#endif /* HAVE_SPDIF_REC */

enum pcm_state
{
    PCM_STATE_STOPPED = 0x0,
    PCM_STATE_RUNNING = 0x1,
    PCM_STATE_PAUSED  = 0x2,
};

enum pcm_handle_flags
{
    PCM_STREAM_PLAYBACK  = 0x0,
    PCM_STREAM_RECORDING = 0x1,
    PCM_STREAM_TYPE_MASK = 0x3,
};

#define PCM_FLAGS_STREAM_TYPE(flags) ((flags) & PCM_STREAM_TYPE_MASK)

#define PCM_FORMAT_CAP_1_BYTE_INT (1ul << 0)
#define PCM_FORMAT_CAP_2_BYTE_INT (1ul << 1)
#define PCM_FORMAT_CAP_3_BYTE_INT (1ul << 2)
#define PCM_FORMAT_CAP_4_BYTE_INT (1ul << 3)

/* Override in config file otherwise use these defaults */
#ifndef CONFIG_PCM_FORMAT_CAPS
#define CONFIG_PCM_FORMAT_CAPS  PCM_FORMAT_CAP_2_BYTE_INT
#define PCM_FORMAT_DMA_T        PCM_FORMAT_S16_2CH_2I
#endif /* CONFIG_PCM_FORMAT_CAPS */

typedef unsigned long pcm_format_t;

#define PCM_FORMAT_T_PARM(f)   (&(const pcm_format_t){ f })

#define PCM_FORMAT_SET_BITS(bits)   (((bits)  & 0xfful) <<  0)
#define PCM_FORMAT_SET_SIZE(size)   (((size)  &  0xful) <<  8)
#define PCM_FORMAT_SET_USGN(usgn)   (((usgn)  &  0x1ul) << 12)
#define PCM_FORMAT_SET_FLT(flt)     (((flt)   &  0x1ul) << 13)
#define PCM_FORMAT_SET_NI(ni)       (((ni)    &  0x1ul) << 14)
#define PCM_FORMAT_SET_END(end)     (((end)   &  0x3ul) << 15)
#define PCM_FORMAT_SET_CHNUM(chnum) (((chnum) &  0xful) << 17)

#define PCM_FORMAT_GET_BITS(f)      (((f) >>  0) & 0xfful)
#define PCM_FORMAT_GET_SIZE(f)      (((f) >>  8) &  0xful)
#define PCM_FORMAT_GET_USGN(f)      (((f) >> 12) &  0x1ul)
#define PCM_FORMAT_GET_FLT(f)       (((f) >> 13) &  0x1ul)
#define PCM_FORMAT_GET_NI(f)        (((f) >> 14) &  0x1ul)
#define PCM_FORMAT_GET_END(f)       (((f) >> 15) &  0x3ul)
#define PCM_FORMAT_GET_CHNUM(f)     (((f) >> 17) &  0xful)

#define PCM_FORMAT(bits, size, usgn, flt, ni, end, chnum) \
        (PCM_FORMAT_SET_BITS(bits) | \
         PCM_FORMAT_SET_SIZE(size) | \
         PCM_FORMAT_SET_USGN(usgn) | \
         PCM_FORMAT_SET_FLT(flt)   | \
         PCM_FORMAT_SET_NI(ni)     | \
         PCM_FORMAT_SET_END(end)   | \
         PCM_FORMAT_SET_CHNUM(chnum))

/* some common format predefines (and examples) */
#define PCM_FORMAT_S16_1CH_2I   PCM_FORMAT(16, 2, 0, 0, 0, 0, 1)
#define PCM_FORMAT_S16_2CH_2I   PCM_FORMAT(16, 2, 0, 0, 0, 0, 2)
#define PCM_FORMAT_S18_1CH_3I   PCM_FORMAT(18, 3, 0, 0, 0, 0, 1)
#define PCM_FORMAT_S18_2CH_3I   PCM_FORMAT(18, 3, 0, 0, 0, 0, 2)
#define PCM_FORMAT_S20_1CH_3I   PCM_FORMAT(20, 3, 0, 0, 0, 0, 1)
#define PCM_FORMAT_S20_2CH_3I   PCM_FORMAT(20, 3, 0, 0, 0, 0, 2)
#define PCM_FORMAT_S24_1CH_3I   PCM_FORMAT(24, 3, 0, 0, 0, 0, 1)
#define PCM_FORMAT_S24_2CH_3I   PCM_FORMAT(24, 3, 0, 0, 0, 0, 2)
#define PCM_FORMAT_S24_1CH_4I   PCM_FORMAT(24, 4, 0, 0, 0, 0, 1)
#define PCM_FORMAT_S24_2CH_4I   PCM_FORMAT(24, 4, 0, 0, 0, 0, 2)
#define PCM_FORMAT_S32_1CH_4I   PCM_FORMAT(32, 4, 0, 0, 0, 0, 1)
#define PCM_FORMAT_S32_2CH_4I   PCM_FORMAT(32, 4, 0, 0, 0, 0, 2)
#define PCM_FORMAT_F32_1CH_4I   PCM_FORMAT(32, 4, 0, 1, 0, 0, 1)
#define PCM_FORMAT_F32_2CH_4I   PCM_FORMAT(32, 4, 0, 1, 0, 0, 2)
#define PCM_FORMAT_F64_1CH_8I   PCM_FORMAT(64, 8, 0, 1, 0, 0, 1)
#define PCM_FORMAT_F64_2CH_8I   PCM_FORMAT(64, 8, 0, 1, 0, 0, 2)

/* pcm_dma_t is the native sample type of the native format */
#define PCM_DMA_T_SIZE          PCM_FORMAT_GET_SIZE(PCM_FORMAT_DMA_T)
#define PCM_DMA_T_BITS          PCM_FORMAT_GET_BITS(PCM_FORMAT_DMA_T)
#define PCM_DMA_T_CHANNELS      PCM_FORMAT_GET_CHNUM(PCM_FORMAT_DMA_T)
#define PCM_DMA_T_FRAME_SIZE    ((size_t)(PCM_DMA_T_CHANNELS*PCM_DMA_T_SIZE))

#if   PCM_DMA_T_SIZE == 2
typedef int16_t pcm_dma_t;
#elif PCM_DMA_T_SIZE == 3
typedef int8_t  pcm_dma_t;
#elif PCM_DMA_T_SIZE == 4
typedef int32_t pcm_dma_t;
#else
#error Invalid PCM_DMA_T_SIZE.
#endif

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

/** Routines used with playback and recording **/
pcm_handle_t pcm_open(unsigned int flags);
int pcm_close(pcm_handle_t pcm);
int pcm_lock_callback(pcm_handle_t pcm);
int pcm_unlock_callback(pcm_handle_t pcm);
unsigned long pcm_set_frequency(pcm_handle_t pcm, unsigned long samplerate);
unsigned long pcm_get_frequency(pcm_handle_t pcm);
int pcm_get_state(pcm_handle_t pcm);
int pcm_pause(pcm_handle_t pcm, bool pause);
int pcm_stop(pcm_handle_t pcm);
int pcm_set_amplitude(pcm_handle_t pcm, int32_t amplitude);

/* Typedef for registered data callback */
typedef int (*pcm_play_callback_type)(int status, const void **start,
                                      unsigned long *frames);

/* This is for playing "raw" PCM data */
int pcm_play_data(pcm_handle_t pcm,
                  pcm_play_callback_type callback,
                  const void *addr,
                  unsigned long frames,
                  const pcm_format_t *format);

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
                    const pcm_format_t *format);

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
pcm_dma_t_read_preidx(const void **pp,
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

static FORCE_INLINE void
pcm_dma_t_write_preidx(void **pp,
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

static FORCE_INLINE void
pcm_dma_t_advance(const void **pp,
                  int idx,
                  unsigned int numchannels)
{
    *pp = PTR_ADD(*pp, PCM_DMA_T_SIZE*idx*(int)numchannels);
}

static FORCE_INLINE int32_t
pcm_dma_t_read_postidx(const void **pp,
                       int postidx,
                       unsigned int numchannels)
{
    int32_t value = pcm_dma_t_read_preidx(pp, 0, false);
    pcm_dma_t_advance(pp, postidx, numchannels);
    return value;
}

static FORCE_INLINE void
pcm_dma_t_write_postidx(void **pp,
                        int32_t value,
                        int postidx,
                        unsigned int numchannels)
{
    pcm_dma_t_write_preidx(pp, value, 0, false);
    pcm_dma_t_advance((const void **)pp, postidx, numchannels);
}

#define pcm_dma_t_frames2size(frames) \
    ((size_t)(PCM_DMA_T_FRAME_SIZE*(frames)))

#define pcm_dma_t_size2frames(nbyte) \
    ((unsigned long)((nbyte) / PCM_DMA_T_FRAME_SIZE))

static FORCE_INLINE int32_t
    pcm_dma_t_scale_to(int32_t value, unsigned int bits)
{
    int shift = bits - PCM_DMA_T_BITS;

    if (shift < 0) {
        value >>= -shift;
    }
    else if (shift > 0) {
        value <<= shift;
    }

    return value;
}

#endif /* PCM_H */
