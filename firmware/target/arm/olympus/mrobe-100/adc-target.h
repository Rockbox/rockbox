/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

#define ADC_ADDR            (*(volatile unsigned long*)(0x7000ad00))
#define ADC_STATUS          (*(volatile unsigned long*)(0x7000ad04))
#define ADC_DATA_1          (*(volatile unsigned long*)(0x7000ad20))
#define ADC_DATA_2          (*(volatile unsigned long*)(0x7000ad24))
#define ADC_INIT            (*(volatile unsigned long*)(0x7000ad2c))

#define NUM_ADC_CHANNELS 4

#define ADC_BATTERY     0
#define ADC_UNKNOWN_1   1
#define ADC_REMOTE      2
#define ADC_SCROLLPAD   3
#define ADC_UNREG_POWER ADC_BATTERY /* For compatibility */

/* Force a scan now */
unsigned short adc_scan(int channel);

#endif
