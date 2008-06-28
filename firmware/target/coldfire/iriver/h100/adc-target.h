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
/* for the iriver h1x0 */

#ifndef _ADC_TARGET_H_
#define _ADC_TARGET_H_

#define NUM_ADC_CHANNELS 4

#define ADC_BUTTONS 0
#define ADC_REMOTE  1
#define ADC_BATTERY 2
#define ADC_REMOTEDETECT  3
#define ADC_UNREG_POWER ADC_BATTERY /* For compatibility */

/* ADC values for different remote control types */
#define ADCVAL_H300_LCD_REMOTE      0x5E
#define ADCVAL_H100_LCD_REMOTE      0x96
#define ADCVAL_H300_LCD_REMOTE_HOLD 0xCC
#define ADCVAL_H100_LCD_REMOTE_HOLD 0xEA

/* Force a scan now */
unsigned short adc_scan(int channel);

static inline unsigned short adc_read(int channel)
{ 
    return adc_scan(channel); 
}

#endif /* _ADC_TARGET_H_ */
