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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _ADC_TARGET_H_
#define _ADC_TARGET_H_

#define NUM_ADC_CHANNELS 8

#define ADC_BATTERY             0 /* Battery voltage always reads 0x3FF due to
                                     silly scaling */
#define ADC_CHARGE_REGULATOR    0 /* Uh, we read the battery voltage? */
#define ADC_USB_POWER           1 /* USB, reads 0x000 when USB is inserted */
#define ADC_BUTTON_OFF          2 /* the off button, high value if pressed */
#define ADC_BUTTON_ON           3 /* the on button, low value if pressed */
#define ADC_BUTTON_ROW1         4 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_BUTTON_ROW2         5 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_UNREG_POWER         6 /* Battery voltage with a better scaling */
#define ADC_EXT_POWER           7 /* The external power voltage, 0v or 2.7v */

#define EXT_SCALE_FACTOR 14800

#endif /* _ADC_TARGET_H_ */
