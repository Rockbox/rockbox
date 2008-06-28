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
#define PCM_SAMPR_H

#ifndef HW_SAMPR_CAPS
#define HW_SAMPR_CAPS SAMPR_CAP_44 /* if not defined, default to 44100 */
#endif

/* These must be macros for comparison with SAMPR_CAP_* flags by the
   preprocessor. Add samplerate index in descending order renumbering
   the ones later in the list if any */
#define FREQ_96          0
#define FREQ_88          1
#define FREQ_64          2
#define FREQ_48          3
#define FREQ_44          4
#define FREQ_32          5
#define FREQ_24          6
#define FREQ_22          7
#define FREQ_16          8
#define FREQ_12          9
#define FREQ_11         10
#define FREQ_8          11
#define SAMPR_NUM_FREQ  12

/* sample rate values in HZ */
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
#define SAMPR_CAP_ALL   (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_64 | \
                         SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32 | \
                         SAMPR_CAP_24 | SAMPR_CAP_22 | SAMPR_CAP_16 | \
                         SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

/* Master list of all "standard" rates supported. */
extern const unsigned long audio_master_sampr_list[SAMPR_NUM_FREQ];

/** Hardware sample rates **/

/* Enumeration of supported frequencies where 0 is the highest rate
   supported and REC_NUM_FREQUENCIES is the number available */
enum hw_freq_indexes
{
    __HW_FREQ_START_INDEX = -1, /* Make sure first in list is 0 */

/* 96000 */
#if (HW_SAMPR_CAPS & SAMPR_CAP_96)     /* Macros and enums for each FREQ: */
    HW_FREQ_96,                        /* Index in enumeration */
#define HW_HAVE_96                     /* Defined if this FREQ is defined */
#define HW_HAVE_96_(...) __VA_ARGS__   /* Output its parameters for this FREQ */
#else
#define HW_HAVE_96_(...)               /* Discards its parameters for this FREQ */
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
    HW_FREQ_DEFAULT = HW_FREQ_44,
    HW_SAMPR_DEFAULT = SAMPR_44,
}; /* enum hw_freq_indexes */

/* list of hardware sample rates */
extern const unsigned long hw_freq_sampr[HW_NUM_FREQ];

#ifdef HAVE_RECORDING
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

/* Default to 44.1kHz if not otherwise specified */
#ifndef REC_FREQ_DEFAULT
#define REC_FREQ_DEFAULT REC_FREQ_44
#endif

#define REC_FREQ_CFG_VAL_LIST  &REC_HAVE_96_(",96") REC_HAVE_88_(",88") \
                                REC_HAVE_64_(",64") REC_HAVE_48_(",48") \
                                REC_HAVE_44_(",44") REC_HAVE_32_(",32") \
                                REC_HAVE_24_(",24") REC_HAVE_22_(",22") \
                                REC_HAVE_16_(",16") REC_HAVE_12_(",12") \
                                REC_HAVE_11_(",11") REC_HAVE_8_(",8")[1]

/* List of recording supported sample rates (set or subset of master list) */
extern const unsigned long rec_freq_sampr[REC_NUM_FREQ];
#endif /* HAVE_RECORDING */

#endif /* PCM_SAMPR_H */
