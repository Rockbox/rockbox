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

/* settings to the tuner layer */
#define RADIO_ALL               -1 /* debug */
#define RADIO_SLEEP             0
#define RADIO_FREQUENCY         1
#define RADIO_MUTE              2
#define RADIO_IF_MEASUREMENT    3
#define RADIO_SENSITIVITY       4
#define RADIO_FORCE_MONO        5
#define RADIO_SCAN_FREQUENCY    6
#if (CONFIG_TUNER & TEA5767)
#define RADIO_SET_DEEMPHASIS    7
#define RADIO_SET_BAND          8
#endif
#if (CONFIG_TUNER & LV24020LP)
#define RADIO_REGION            9 /* to be used for all tuners */
#define RADIO_REG_STAT          100
#define RADIO_MSS_FM            101
#define RADIO_MSS_IF            102
#define RADIO_MSS_SD            103
#define RADIO_IF_SET            104
#define RADIO_SD_SET            105
#endif
/* readback from the tuner layer */
#define RADIO_PRESENT           0
#define RADIO_TUNED             1
#define RADIO_STEREO            2

#define REGION_EUROPE           0
#define REGION_US_CANADA        1
#define REGION_JAPAN            2
#define REGION_KOREA            3

#if CONFIG_TUNER

#ifdef SIMULATOR
int radio_set(int setting, int value);
int radio_get(int setting);
#else
#if CONFIG_TUNER == S1A0903X01 /* FM recorder */
#define radio_set samsung_set
#define radio_get samsung_get
#elif CONFIG_TUNER == LV24020LP  /* Sansa */
#define radio_set sanyo_set
#define radio_get sanyo_get
#elif CONFIG_TUNER == TEA5767  /* iRiver, iAudio */
#define radio_set philips_set
#define radio_get philips_get
#elif CONFIG_TUNER == (S1A0903X01 | TEA5767) /* OndioFM */
#define radio_set _radio_set
#define radio_get _radio_get
int (*_radio_set)(int setting, int value);
int (*_radio_get)(int setting);
#endif /* CONFIG_TUNER == */
#endif /* SIMULATOR */

#if (CONFIG_TUNER & S1A0903X01)
int samsung_set(int setting, int value);
int samsung_get(int setting);
#endif /* CONFIG_TUNER & S1A0903X01 */

#if (CONFIG_TUNER & LV24020LP)
int sanyo_set(int setting, int value);
int sanyo_get(int setting);
#endif /* CONFIG_TUNER & LV24020LP */

#if (CONFIG_TUNER & TEA5767)
struct philips_dbg_info
{
    unsigned char read_regs[5];
    unsigned char write_regs[5];
};
int philips_set(int setting, int value);
int philips_get(int setting);
void philips_dbg_info(struct philips_dbg_info *info);
#endif /* CONFIG_TUNER & TEA5767 */

/* Just inline here since only radio screen needs this atm and
   there's no tuner.c. */
static inline void tuner_init(void)
{
#ifndef SIMULATOR
#if CONFIG_TUNER == (S1A0903X01 | TEA5767)
    if (HW_MASK & TUNER_MODEL)
    {
        _radio_set = philips_set;
        _radio_get = philips_get;
    }
    else
    {
        _radio_set = samsung_set;
        _radio_get = samsung_get;
    }
#endif
#endif
}

#endif /* #if CONFIG_TUNER */

#endif /* __TUNER_H__ */
