/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner abstraction layer
 *
 * Copyright (C) 2004 Jörg Hohensohn
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
#ifndef __TUNER_H__
#define __TUNER_H__

#include "config.h"
#include "hwcompat.h"

/** Settings to the tuner layer **/
enum
{
    RADIO_ALL = -1, /* debug */
    RADIO_SLEEP,
    RADIO_FREQUENCY,
    RADIO_MUTE,
    RADIO_FORCE_MONO,
    RADIO_SCAN_FREQUENCY,
    
    /* Put new general-purpose settings above this line */
    __RADIO_SET_STANDARD_LAST
};

/** Readback from the tuner layer **/
enum
{
    RADIO_PRESENT = 0,
    RADIO_TUNED,
    RADIO_STEREO,
    /* RADIO_EVENT is an event that requests a screen update */
    RADIO_EVENT,
    RADIO_RSSI,
    RADIO_RSSI_MIN,
    RADIO_RSSI_MAX,

    /* Put new general-purpose readback values above this line */
    __RADIO_GET_STANDARD_LAST
};

#ifdef HAVE_RDS_CAP
/** Readback from the tuner RDS layer **/
enum
{
    RADIO_RDS_NAME,
    RADIO_RDS_TEXT,

    /* Put new general-purpose readback values above this line */
    __RADIO_GET_RDS_INFO_STANDARD_LAST
};
#endif

/** Tuner regions **/

/* Basic region information */
enum
{
    REGION_EUROPE = 0,
    REGION_US_CANADA,
    REGION_JAPAN,
    REGION_KOREA,
    REGION_ITALY,
    REGION_OTHER,

    /* Add new regions above this line */
    TUNER_NUM_REGIONS
};

struct fm_region_data
{
    int freq_min;
    int freq_max;
    int freq_step;
    int deemphasis; /* in microseconds, usually 50 or 75 */
};

extern const struct fm_region_data fm_region_data[TUNER_NUM_REGIONS];

#if CONFIG_TUNER

#if !defined(SIMULATOR) && defined(CONFIG_TUNER_MULTI)
extern int tuner_detect_type(void);
extern int (*tuner_set)(int setting, int value);
extern int (*tuner_get)(int setting);
#endif /* CONFIG_TUNER_MULTI */

/** Sanyo LV24020LP **/
#if (CONFIG_TUNER & LV24020LP)
/* Sansa c200, e200 */
#include "lv24020lp.h"
#endif

/** Samsung S1A0903X01 **/
#if (CONFIG_TUNER & S1A0903X01)
/* Ondio FM, FM Recorder */
#include "s1a0903x01.h"
#endif

/** Philips TEA5760 **/
#if (CONFIG_TUNER & TEA5760)
#include "tea5760.h"
#endif

/** Philips TEA5767 **/
#if (CONFIG_TUNER & TEA5767)
/* Ondio FM, FM Recorder, Recorder V2, iRiver h100/h300, iAudio x5 */
#include "tea5767.h"
#endif

/* Silicon Labs 4700 */
#if (CONFIG_TUNER & SI4700)
#include "si4700.h"
#endif

/* RDA micro RDA5802 */
#if (CONFIG_TUNER & RDA5802)
#include "rda5802.h"
#endif

/* Apple remote tuner */
#if (CONFIG_TUNER & IPOD_REMOTE_TUNER)
#include "ipod_remote_tuner.h"
#endif

/* SigmaTel/Freescale STFM1000 */
#if (CONFIG_TUNER & STFM1000)
#include "stfm1000.h"
#endif

#if defined(SIMULATOR)
#undef tuner_set
int tuner_set(int setting, int value);
#undef tuner_get
int tuner_get(int setting);
#endif



/* Additional messages that get enumerated after tuner driver headers */

/* for tuner_set */
enum
{
    __RADIO_SET_ADDITIONAL_START = __RADIO_SET_STANDARD_LAST-1,
#ifdef HAVE_RADIO_REGION
    RADIO_REGION,
#endif

    RADIO_SET_CHIP_FIRST
};

/* for tuner_get */
enum
{
    __RADIO_GET_ADDITIONAL_START = __RADIO_GET_STANDARD_LAST-1,

    RADIO_GET_CHIP_FIRST
};

/** **/

void tuner_init(void) INIT_ATTR;

#endif /* #if CONFIG_TUNER */

#endif /* __TUNER_H__ */
