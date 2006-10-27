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

#define NUM_ADC_CHANNELS 3

#define ADC_BUTTONS 0
#define ADC_REMOTE  1
#define ADC_BATTERY 2
#define ADC_UNREG_POWER ADC_BATTERY /* For compatibility */

/* Force a scan now */
unsigned short adc_scan(int channel);
void adc_enable_button_scan(bool enable);
bool adc_get_button_scan_enabled(void);

#endif /* _ADC_TARGET_H_ */
