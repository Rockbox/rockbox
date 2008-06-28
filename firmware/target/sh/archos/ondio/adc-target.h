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

#define ADC_MMC_SWITCH          0 /* low values if MMC inserted */
#define ADC_USB_POWER           1 /* USB, reads 0x000 when USB is inserted */
#define ADC_BUTTON_OPTION       2 /* the option button, low value if pressed */
#define ADC_BUTTON_ONOFF        3 /* the on/off button, high value if pressed */
#define ADC_BUTTON_ROW1         4 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_USB_ACTIVE          5 /* USB bridge activity */
#define ADC_UNREG_POWER         7 /* Battery voltage */

#define EXT_SCALE_FACTOR 14800

#endif /* _ADC_TARGET_H_ */
