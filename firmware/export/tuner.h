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
#ifndef __TUNER_SAMSUNG_H__
#define __TUNER_SAMSUNG_H__

#include "hwcompat.h"

/* settings to the tuner layer */
#define RADIO_ALL -1 /* debug */
#define RADIO_SLEEP 0
#define RADIO_FREQUENCY 1
#define RADIO_MUTE 2
#define RADIO_IF_MEASUREMENT 3
#define RADIO_SENSITIVITY 4
#define RADIO_FORCE_MONO 5
#define RADIO_SCAN_FREQUENCY 6
#if (CONFIG_TUNER & TEA5767)
#define RADIO_SET_DEEMPHASIS 7
#define RADIO_SET_BAND 8
#endif
/* readback from the tuner layer */
#define RADIO_PRESENT 0
#define RADIO_TUNED 1
#define RADIO_STEREO 2

#if CONFIG_TUNER

#ifdef SIMULATOR
int radio_set(int setting, int value);
int radio_get(int setting);
#else
#if CONFIG_TUNER == S1A0903X01 /* FM recorder */
#define radio_set samsung_set
#define radio_get samsung_get
#elif CONFIG_TUNER == TEA5767  /* iRiver, iAudio */
#define radio_set philips_set
#define radio_get philips_get
#elif CONFIG_TUNER == (S1A0903X01 | TEA5767) /* OndioFM */
#define radio_set _radio_set
#define radio_get _radio_get
int (*_radio_set)(int setting, int value);
int (*_radio_get)(int setting);
#endif
#endif

#if (CONFIG_TUNER & S1A0903X01)
int samsung_set(int setting, int value);
int samsung_get(int setting);
#endif /* CONFIG_TUNER & S1A0903X01 */

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
    if (read_hw_mask() & TUNER_MODEL)
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

#endif
