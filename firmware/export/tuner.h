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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __TUNER_H__
#define __TUNER_H__

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

    /* Put new general-purpose readback values above this line */
    __RADIO_GET_STANDARD_LAST
};

/** Tuner regions **/

/* Basic region information */
enum
{
    REGION_EUROPE = 0,
    REGION_US_CANADA,
    REGION_JAPAN,
    REGION_KOREA,

    /* Add new regions above this line */
    TUNER_NUM_REGIONS
};

struct fm_region_data
{
    int freq_min;
    int freq_max;
    int freq_step;
};

extern const struct fm_region_data fm_region_data[TUNER_NUM_REGIONS];

#if CONFIG_TUNER

#ifdef SIMULATOR
int tuner_set(int setting, int value);
int tuner_get(int setting);
#else

#ifdef CONFIG_TUNER_MULTI
extern int (*tuner_set)(int setting, int value);
extern int (*tuner_get)(int setting);
#endif /* CONFIG_TUNER_MULTI */

/** Sanyo LV24020LP **/
#if (CONFIG_TUNER & LV24020LP)
/* Sansa e200 Series */
#include "lv24020lp.h"
#endif

/** Samsung S1A0903X01 **/
#if (CONFIG_TUNER & S1A0903X01)
/* Ondio FM, FM Recorder */
#include "s1a0903x01.h"
#endif

/** Philips TEA5767 **/
#if (CONFIG_TUNER & TEA5767)
/* Ondio FM, FM Recorder, Recorder V2, iRiver h100/h300, iAudio x5 */
#include "tea5767.h"
#endif

#endif /* SIMULATOR */

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

void tuner_init(void);
bool tuner_power(bool power);

#endif /* #if CONFIG_TUNER */

#endif /* __TUNER_H__ */
