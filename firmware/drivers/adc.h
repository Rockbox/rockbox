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
#ifndef _ADC_H_
#define _ADC_H_

#define NUM_ADC_CHANNELS 8

#define ADC_BATTERY             0
#define ADC_CHARGE_REGULATOR    1
#define ADC_USB_POWER           2

#define ADC_BUTTON_ROW1         4
#define ADC_BUTTON_ROW2         5
#define ADC_UNREG_POWER         6
#define ADC_EXT_POWER           7

unsigned short adc_read(int channel);
void adc_init(void);

#endif
