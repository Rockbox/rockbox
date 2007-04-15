/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _ADC_TARGET_H_
#define _ADC_TARGET_H_

#define NUM_ADC_CHANNELS 8

#define ADC_BUTTON_LEFT         0
#define ADC_BUTTON_MENU         1
#define ADC_BUTTON_RIGHT        2 
#define ADC_BUTTON_PLAY         3
#define ADC_UNREG_POWER         6 /* Battery voltage with a better scaling */
#define ADC_EXT_POWER           7 /* The external power voltage, 0v or 2.7v */

#define EXT_SCALE_FACTOR 14800

#endif /* _ADC_TARGET_H_ */
