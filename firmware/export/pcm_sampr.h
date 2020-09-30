/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
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

#ifndef PCM_SAMPR_H

/* File might be included for CPP config macros only. Allow it to be included
 * again for full C declarations. */
#ifndef PCM_SAMPR_CONFIG_ONLY
#define PCM_SAMPR_H
#endif

#ifndef HW_SAMPR_CAPS
#define HW_SAMPR_CAPS SAMPR_CAP_44 /* if not defined, default to 44100 */
#endif

/* These must be macros for comparison with SAMPR_CAP_* flags by the
   preprocessor. Add samplerate index in descending order renumbering
   the ones later in the list if any */
#define FREQ_192         0
#define FREQ_176         1
#define FREQ_96          2
#define FREQ_88          3
#define FREQ_64          4
#define FREQ_48          5
#define FREQ_44          6
#define FREQ_32          7
#define FREQ_24          8
#define FREQ_22          9
#define FREQ_16         10
#define FREQ_12         11
#define FREQ_11         12
#define FREQ_8          13
#define SAMPR_NUM_FREQ  14

/* sample rate values in HZ */
#define SAMPR_192       192000
#define SAMPR_176       176400
#define SAMPR_96        96000
#define SAMPR_88        88200
#define SAMPR_64        64000
#define SAMPR_48        48000
#define SAMPR_44        44100
#define SAMPR_32        32000
#define SAMPR_24        24000
#define SAMPR_22        22050
#define SAMPR_16        16000
#define SAMPR_12        12000
#define SAMPR_11        11025
#define SAMPR_8          8000

/* sample rate capability bits */
#define SAMPR_CAP_192   (1 << FREQ_192)
#define SAMPR_CAP_176   (1 << FREQ_176)
#define SAMPR_CAP_96    (1 << FREQ_96)
#define SAMPR_CAP_88    (1 << FREQ_88)
#define SAMPR_CAP_64    (1 << FREQ_64)
#define SAMPR_CAP_48    (1 << FREQ_48)
#define SAMPR_CAP_44    (1 << FREQ_44)
#define SAMPR_CAP_32    (1 << FREQ_32)
#define SAMPR_CAP_24    (1 << FREQ_24)
#define SAMPR_CAP_22    (1 << FREQ_22)
#define SAMPR_CAP_16    (1 << FREQ_16)
#define SAMPR_CAP_12    (1 << FREQ_12)
#define SAMPR_CAP_11    (1 << FREQ_11)
#define SAMPR_CAP_8     (1 << FREQ_8)

#define SAMPR_CAP_ALL_192   (SAMPR_CAP_192 | SAMPR_CAP_176 | \
                         SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_64 | \
                         SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32 | \
                         SAMPR_CAP_24 | SAMPR_CAP_22 | SAMPR_CAP_16 | \
                         SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

#define SAMPR_CAP_ALL_96   (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_64 | \
                         SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32 | \
                         SAMPR_CAP_24 | SAMPR_CAP_22 | SAMPR_CAP_16 | \
                         SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

#define SAMPR_CAP_ALL_48   (SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32 | \
                         SAMPR_CAP_24 | SAMPR_CAP_22 | SAMPR_CAP_16 | \
                         SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

/* List of sampling rates that are good enough for most purposes. */
#define SAMPR_CAP_ALL_GE_22   (SAMPR_CAP_192 | SAMPR_CAP_176 | \
                               SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_64 | \
                               SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32 | \
                               SAMPR_CAP_24 | SAMPR_CAP_22)

#ifndef PCM_SAMPR_CONFIG_ONLY
/* Master list of all "standard" rates supported. */
extern const unsigned long audio_master_sampr_list[SAMPR_NUM_FREQ];
#endif /* PCM_SAMPR_CONFIG_ONLY */

/** Hardware sample rates **/

#ifndef PCM_SAMPR_CONFIG_ONLY
/* Enumeration of supported frequencies where 0 is the highest rate
   supported and REC_NUM_FREQUENCIES is the number available */
enum hw_freq_indexes
{
    __HW_FREQ_START_INDEX = -1, /* Make sure first in list is 0 */

/* 192000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_192)    /* Macros and enums for each FREQ: */
    HW_FREQ_192,                       /* Index in enumeration */
#define HW_HAVE_192                    /* Defined if this FREQ is defined */
#define HW_HAVE_192_(...) __VA_ARGS__  /* Output its parameters for this FREQ */
#else
#define HW_HAVE_192_(...)              /* Discards its parameters for this FREQ */
#endif
/* 176400 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_176)
    HW_FREQ_176,
#define HW_HAVE_176
#define HW_HAVE_176_(...) __VA_ARGS__
#else
#define HW_HAVE_176_(...)
#endif
/* 96000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_96)
    HW_FREQ_96,
#define HW_HAVE_96
#define HW_HAVE_96_(...) __VA_ARGS__
#else
#define HW_HAVE_96_(...)
#endif
/* 88200 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_88)
    HW_FREQ_88,
#define HW_HAVE_88
#define HW_HAVE_88_(...) __VA_ARGS__
#else
#define HW_HAVE_88_(...)
#endif
/* 64000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_64)
    HW_FREQ_64,
#define HW_HAVE_64
#define HW_HAVE_64_(...) __VA_ARGS__
#else
#define HW_HAVE_64_(...)
#endif
/* 48000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_48)
    HW_FREQ_48,
#define HW_HAVE_48
#define HW_HAVE_48_(...) __VA_ARGS__
#else
#define HW_HAVE_48_(...)
#endif
/* 44100 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_44)
    HW_FREQ_44,
#define HW_HAVE_44
#define HW_HAVE_44_(...) __VA_ARGS__
#else
#define HW_HAVE_44_(...)
#endif
/* 32000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_32)
    HW_FREQ_32,
#define HW_HAVE_32
#define HW_HAVE_32_(...) __VA_ARGS__
#else
#define HW_HAVE_32_(...)
#endif
/* 24000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_24)
    HW_FREQ_24,
#define HW_HAVE_24
#define HW_HAVE_24_(...) __VA_ARGS__
#else
#define HW_HAVE_24_(...)
#endif
/* 22050 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_22)
    HW_FREQ_22,
#define HW_HAVE_22
#define HW_HAVE_22_(...) __VA_ARGS__
#else
#define HW_HAVE_22_(...)
#endif
/* 16000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_16)
    HW_FREQ_16,
#define HW_HAVE_16
#define HW_HAVE_16_(...) __VA_ARGS__
#else
#define HW_HAVE_16_(...)
#endif
/* 12000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_12)
    HW_FREQ_12,
#define HW_HAVE_12
#define HW_HAVE_12_(...) __VA_ARGS__
#else
#define HW_HAVE_12_(...)
#endif
/* 11025 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_11)
    HW_FREQ_11,
#define HW_HAVE_11
#define HW_HAVE_11_(...) __VA_ARGS__
#else
#define HW_HAVE_11_(...)
#endif
/* 8000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_8 )
    HW_FREQ_8,
#define HW_HAVE_8
#define HW_HAVE_8_(...) __VA_ARGS__
#else
#define HW_HAVE_8_(...)
#endif
    HW_NUM_FREQ,
}; /* enum hw_freq_indexes */

/* list of hardware sample rates */
extern const unsigned long hw_freq_sampr[HW_NUM_FREQ];
#endif /* PCM_SAMPR_CONFIG_ONLY */

#if HW_SAMPR_CAPS & SAMPR_CAP_44
#define HW_FREQ_DEFAULT     HW_FREQ_44
#define HW_SAMPR_DEFAULT    SAMPR_44
#elif HW_SAMPR_CAPS & SAMPR_CAP_48
#define HW_FREQ_DEFAULT     HW_FREQ_48
#define HW_SAMPR_DEFAULT    SAMPR_48
#else
#error "Neither 48 or 44KHz supported?"
#endif


#if HW_SAMPR_CAPS & SAMPR_CAP_192
# define HW_SAMPR_MAX   SAMPR_192
#elif HW_SAMPR_CAPS & SAMPR_CAP_176
# define HW_SAMPR_MAX   SAMPR_176
#elif HW_SAMPR_CAPS & SAMPR_CAP_96
# define HW_SAMPR_MAX   SAMPR_96
#elif HW_SAMPR_CAPS & SAMPR_CAP_88
# define HW_SAMPR_MAX   SAMPR_88
#elif HW_SAMPR_CAPS & SAMPR_CAP_64
# define HW_SAMPR_MAX   SAMPR_64
#elif HW_SAMPR_CAPS & SAMPR_CAP_48
# define HW_SAMPR_MAX   SAMPR_48
#else
# define HW_SAMPR_MAX   SAMPR_44
#endif

#if HW_SAMPR_CAPS & SAMPR_CAP_8
# define HW_SAMPR_MIN   SAMPR_8
#elif HW_SAMPR_CAPS & SAMPR_CAP_11
# define HW_SAMPR_MIN   SAMPR_11
#elif HW_SAMPR_CAPS & SAMPR_CAP_12
# define HW_SAMPR_MIN   SAMPR_12
#elif HW_SAMPR_CAPS & SAMPR_CAP_16
# define HW_SAMPR_MIN   SAMPR_16
#elif HW_SAMPR_CAPS & SAMPR_CAP_22
# define HW_SAMPR_MIN   SAMPR_22
#elif HW_SAMPR_CAPS & SAMPR_CAP_24
# define HW_SAMPR_MIN   SAMPR_24
#elif HW_SAMPR_CAPS & SAMPR_CAP_32
# define HW_SAMPR_MIN   SAMPR_32
#else
# define HW_SAMPR_MIN   SAMPR_44
#endif

#define HW_SAMPR_CAPS_QUAL (HW_SAMPR_CAPS & SAMPR_CAP_ALL_GE_22)
#if HW_SAMPR_CAPS_QUAL & SAMPR_CAP_22
# define HW_SAMPR_MIN_GE_22  SAMPR_22
#elif HW_SAMPR_CAPS_QUAL & SAMPR_CAP_24
# define HW_SAMPR_MIN_GE_22  SAMPR_24
#elif HW_SAMPR_CAPS_QUAL & SAMPR_CAP_32
# define HW_SAMPR_MIN_GE_22  SAMPR_32
#else
# define HW_SAMPR_MIN_GE_22  SAMPR_44
#endif

#ifdef HAVE_RECORDING

#ifndef PCM_SAMPR_CONFIG_ONLY
/* Enumeration of supported frequencies where 0 is the highest rate
   supported and REC_NUM_FREQUENCIES is the number available */
enum rec_freq_indexes
{
    __REC_FREQ_START_INDEX = -1, /* Make sure first in list is 0 */

/* 96000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_96)     /* Macros and enums for each FREQ: */
    REC_FREQ_96,                        /* Index in enumeration */
#define REC_HAVE_96                     /* Defined if this FREQ is defined */
#define REC_HAVE_96_(...) __VA_ARGS__   /* Output its parameters for this FREQ */
#else
#define REC_HAVE_96_(...)               /* Discards its parameters for this FREQ */
#endif
/* 88200 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_88)
    REC_FREQ_88,
#define REC_HAVE_88
#define REC_HAVE_88_(...) __VA_ARGS__
#else
#define REC_HAVE_88_(...)
#endif
/* 64000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_64)
    REC_FREQ_64,
#define REC_HAVE_64
#define REC_HAVE_64_(...) __VA_ARGS__
#else
#define REC_HAVE_64_(...)
#endif
/* 48000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_48)
    REC_FREQ_48,
#define REC_HAVE_48
#define REC_HAVE_48_(...) __VA_ARGS__
#else
#define REC_HAVE_48_(...)
#endif
/* 44100 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_44)
    REC_FREQ_44,
#define REC_HAVE_44
#define REC_HAVE_44_(...) __VA_ARGS__
#else
#define REC_HAVE_44_(...)
#endif
/* 32000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_32)
    REC_FREQ_32,
#define REC_HAVE_32
#define REC_HAVE_32_(...) __VA_ARGS__
#else
#define REC_HAVE_32_(...)
#endif
/* 24000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_24)
    REC_FREQ_24,
#define REC_HAVE_24
#define REC_HAVE_24_(...) __VA_ARGS__
#else
#define REC_HAVE_24_(...)
#endif
/* 22050 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_22)
    REC_FREQ_22,
#define REC_HAVE_22
#define REC_HAVE_22_(...) __VA_ARGS__
#else
#define REC_HAVE_22_(...)
#endif
/* 16000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_16)
    REC_FREQ_16,
#define REC_HAVE_16
#define REC_HAVE_16_(...) __VA_ARGS__
#else
#define REC_HAVE_16_(...)
#endif
/* 12000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_12)
    REC_FREQ_12,
#define REC_HAVE_12
#define REC_HAVE_12_(...) __VA_ARGS__
#else
#define REC_HAVE_12_(...)
#endif
/* 11025 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_11)
    REC_FREQ_11,
#define REC_HAVE_11
#define REC_HAVE_11_(...) __VA_ARGS__
#else
#define REC_HAVE_11_(...)
#endif
/* 8000 */
#if (REC_SAMPR_CAPS & SAMPR_CAP_8 )
    REC_FREQ_8,
#define REC_HAVE_8
#define REC_HAVE_8_(...) __VA_ARGS__
#else
#define REC_HAVE_8_(...)
#endif
    REC_NUM_FREQ,
}; /* enum rec_freq_indexes */

/* List of recording supported sample rates (set or subset of master list) */
extern const unsigned long rec_freq_sampr[REC_NUM_FREQ];
#endif /* PCM_SAMPR_CONFIG_ONLY */

/* Default to 44.1kHz if not otherwise specified */
#ifndef REC_FREQ_DEFAULT
#define REC_FREQ_DEFAULT REC_FREQ_44
#endif

#ifndef REC_SAMPR_DEFAULT
#define REC_SAMPR_DEFAULT SAMPR_44
#endif

#define HW_SAMPR_RESET  0

#define REC_FREQ_CFG_VAL_LIST  &REC_HAVE_96_(",96") REC_HAVE_88_(",88") \
                                REC_HAVE_64_(",64") REC_HAVE_48_(",48") \
                                REC_HAVE_44_(",44") REC_HAVE_32_(",32") \
                                REC_HAVE_24_(",24") REC_HAVE_22_(",22") \
                                REC_HAVE_16_(",16") REC_HAVE_12_(",12") \
                                REC_HAVE_11_(",11") REC_HAVE_8_(",8")[1]


#endif /* HAVE_RECORDING */

#ifdef CONFIG_SAMPR_TYPES

#define SAMPR_TYPE_MASK (0xff << 24)
#define SAMPR_TYPE_PLAY (0x00 << 24)
#ifdef HAVE_RECORDING
#define SAMPR_TYPE_REC  (0x01 << 24)
#endif

#ifndef PCM_SAMPR_CONFIG_ONLY
unsigned int pcm_sampr_to_hw_sampr(unsigned int samplerate,
                                   unsigned int type);
#endif

#else /* ndef CONFIG_SAMPR_TYPES */

/* Types are ignored and == 0 */
#define SAMPR_TYPE_PLAY 0
#ifdef HAVE_RECORDING
#define SAMPR_TYPE_REC  0
#endif

#endif /* CONFIG_SAMPR_TYPES */

#endif /* PCM_SAMPR_H */
