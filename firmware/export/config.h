/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* symbolic names for multiple choice configurations: */

/* CONFIG_TUNER (note these are combineable bit-flags) */
#define S1A0903X01 0x01 /* Samsung */
#define TEA5767    0x02 /* Philips */

/* CONFIG_HWCODEC */
#define MAS3587F 3587
#define MAS3507D 3507
#define MAS3539F 3539

/* CONFIG_CPU */
#define SH7034  7034
#define MCF5249 5249
#define TCC730   730   /* lacking a proper abbrivation */

/* CONFIG_KEYPAD */
#define PLAYER_PAD      0
#define RECORDER_PAD    1
#define ONDIO_PAD       2
#define IRIVER_H100_PAD 3
#define GMINI100_PAD    4

/* CONFIG_BATTERY */
#define BATT_LIION2200      2200 /* FM/V2 recorder type */
#define BATT_4AA_NIMH       1500
#define BATT_3AAA_ALKALINE  1000

/* CONFIG_LCD */
#define LCD_GMINI100 0
#define LCD_SSD1815  1 /* as used by Archos Recorders and Ondios */
#define LCD_SSD1801  2 /* as used by Archos Player/Studio */
#define LCD_S1D15E06 3 /* as used by iRiver H100 series */

/* CONFIG_BACKLIGHT */
#define BL_PA14_LO  0 /* Player, PA14 low active */
#define BL_RTC      1 /* Recorder, RTC square wave output */
#define BL_PA14_HI  2 /* Ondio, PA14 high active */
#define BL_IRIVER   3 /* IRiver GPIO */

/* now go and pick yours */
#if defined(ARCHOS_PLAYER)
#include "config-player.h"
#elif defined(ARCHOS_RECORDER)
#include "config-recorder.h"
#elif defined(ARCHOS_FMRECORDER)
#include "config-fmrecorder.h"
#elif defined(ARCHOS_RECORDERV2)
#include "config-recorderv2.h"
#elif defined(ARCHOS_ONDIOSP)
#include "config-ondiosp.h"
#elif defined(ARCHOS_ONDIOFM)
#include "config-ondiofm.h"
#elif defined(IRIVER_H100)
#include "config-h100.h"
#elif defined(ARCHOS_GMINI120)
#include "config-gmini120.h"
#else
/* no known platform */
#endif

#endif
