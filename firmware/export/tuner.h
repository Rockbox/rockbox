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

/* settings to the tuner layer */
#define RADIO_SLEEP 0
#define RADIO_FREQUENCY 1
#define RADIO_MUTE 2
#define RADIO_IF_MEASUREMENT 3
#define RADIO_SENSITIVITY 4
#define RADIO_FORCE_MONO 5
/* readback from the tuner layer */
#define RADIO_PRESENT 0
#define RADIO_TUNED 1
#define RADIO_STEREO 2
#define RADIO_ALL 3 /* debug */

#ifdef CONFIG_TUNER

#if (CONFIG_TUNER & S1A0903X01)
void samsung_set(int setting, int value);
int samsung_get(int setting);
#endif

#if (CONFIG_TUNER & TEA5767)
void philips_set(int setting, int value);
int philips_get(int setting);
#endif

#endif /* #ifdef CONFIG_TUNER */

#endif
