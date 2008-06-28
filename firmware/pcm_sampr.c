/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Michael Sevakis
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
#include "config.h"
#include "pcm_sampr.h"

/* Master list of all "standard" rates supported. */
const unsigned long audio_master_sampr_list[SAMPR_NUM_FREQ] =
{
    [0 ... SAMPR_NUM_FREQ-1] = -1, /* any gaps set to -1 */
    [FREQ_96] = SAMPR_96,
    [FREQ_88] = SAMPR_88,
    [FREQ_64] = SAMPR_64,
    [FREQ_48] = SAMPR_48,
    [FREQ_44] = SAMPR_44,
    [FREQ_32] = SAMPR_32,
    [FREQ_24] = SAMPR_24,
    [FREQ_22] = SAMPR_22,
    [FREQ_16] = SAMPR_16,
    [FREQ_12] = SAMPR_12,
    [FREQ_11] = SAMPR_11,
    [FREQ_8 ] = SAMPR_8, 
};

/* List of all hardware rates supported (set or subset of master list) */
const unsigned long hw_freq_sampr[HW_NUM_FREQ] =
{
    [0 ... HW_NUM_FREQ-1] = -1,
    HW_HAVE_96_([HW_FREQ_96] = SAMPR_96,)
    HW_HAVE_88_([HW_FREQ_88] = SAMPR_88,)
    HW_HAVE_64_([HW_FREQ_64] = SAMPR_64,)
    HW_HAVE_48_([HW_FREQ_48] = SAMPR_48,)
    HW_HAVE_44_([HW_FREQ_44] = SAMPR_44,)
    HW_HAVE_32_([HW_FREQ_32] = SAMPR_32,)
    HW_HAVE_24_([HW_FREQ_24] = SAMPR_24,)
    HW_HAVE_22_([HW_FREQ_22] = SAMPR_22,)
    HW_HAVE_16_([HW_FREQ_16] = SAMPR_16,)
    HW_HAVE_12_([HW_FREQ_12] = SAMPR_12,)
    HW_HAVE_11_([HW_FREQ_11] = SAMPR_11,)
    HW_HAVE_8_( [HW_FREQ_8 ] = SAMPR_8 ,)
};

#ifdef HAVE_RECORDING
/* List of recording supported sample rates (set or subset of master list) */
const unsigned long rec_freq_sampr[REC_NUM_FREQ] =
{
    [0 ... REC_NUM_FREQ-1] = -1,
    REC_HAVE_96_([REC_FREQ_96] = SAMPR_96,)
    REC_HAVE_88_([REC_FREQ_88] = SAMPR_88,)
    REC_HAVE_64_([REC_FREQ_64] = SAMPR_64,)
    REC_HAVE_48_([REC_FREQ_48] = SAMPR_48,)
    REC_HAVE_44_([REC_FREQ_44] = SAMPR_44,)
    REC_HAVE_32_([REC_FREQ_32] = SAMPR_32,)
    REC_HAVE_24_([REC_FREQ_24] = SAMPR_24,)
    REC_HAVE_22_([REC_FREQ_22] = SAMPR_22,)
    REC_HAVE_16_([REC_FREQ_16] = SAMPR_16,)
    REC_HAVE_12_([REC_FREQ_12] = SAMPR_12,)
    REC_HAVE_11_([REC_FREQ_11] = SAMPR_11,)
    REC_HAVE_8_( [REC_FREQ_8 ] = SAMPR_8 ,)
};
#endif /* HAVE_RECORDING */
