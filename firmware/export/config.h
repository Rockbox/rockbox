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

/* CONFIG_TUNER */
#define S1A0903X01 0 /* Samsung */
#define TEA5767    1 /* Philips */

/* CONFIG_HWCODEC */
#define MAS3587F 3587
#define MAS3507D 3507
#define MAS3539F 3539

/* CONFIG_CPU */
#define SH7034  7034
#define MCF5249 5249

/* CONFIG_KEYPAD */
#define PLAYER_PAD   0
#define RECORDER_PAD 1
#define ONDIO_PAD    2

/* CONFIG_BATTERY */
#define BATT_LIION2200      2200 /* FM/V2 recorder type */
#define BATT_4AA_NIMH       1500
#define BATT_3AAA_ALKALINE  1000

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
#else
/* no known platform */
#endif

#endif
