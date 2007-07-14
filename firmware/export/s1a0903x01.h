/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner header for the Samsung S1A0903X01
 *
 * Copyright (C) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _S1A0903X01_H_
#define _S1A0903X01_H_

/* Define additional tuner messages here */
#define HAVE_RADIO_MUTE_TIMEOUT

#if 0
#define S1A0903X01_IF_MEASUREMENT (RADIO_SET_CHIP_FIRST+0)
#define S1A0903X01_SENSITIVITY    (RADIO_SET_CHIP_FIRST+1)
#endif

int s1a0903x01_set(int setting, int value);
int s1a0903x01_get(int setting);

#ifndef CONFIG_TUNER_MULTI
#define tuner_get s1a0903x01_get
#define tuner_set s1a0903x01_set
#endif

#endif /* _S1A0903X01_H_ */
