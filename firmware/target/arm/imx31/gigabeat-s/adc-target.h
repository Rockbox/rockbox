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

/* 16 channels in groups of 8 - each conversion converts 8 channels in
 * a group of 8 inputs */
#define NUM_ADC_CHANNELS 16

#define ADC_BATTERY            0 /* Battery Voltage (BATT) */
#define ADC_UNUSED1            1 /* Battery Current (BATT-BATTISNS) */
#define ADC_APPLICATION_SUPPLY 2 /* Application Supply (BP) */
#define ADC_CHARGER_VOLTAGE    3 /* Charger Voltage (CHRGRAW) */
#define ADC_CHARGER_CURRENT    4 /* Charger Current (CHRGISNSP-CHRGISNSN) */
#define ADC_BATTERY_TEMP       5 /* General Purpose ADIN5 / Battery Pack Thermistor */
#define ADC_UNUSED6            6 /* General Purpose ADIN6 / Backup Voltage (LICELL) */
#define ADC_UNUSED7            7 /* General Purpose ADIN7 / UID / Die Temperature */
#define ADC_HPREMOTE           8 /* General-purpose ADIN8 (Remote control) */
#define ADC_UNUSED9            9 /* General-purpose ADIN9 */
#define ADC_UNUSED10          10 /* General-purpose ADIN10 */
#define ADC_UNUSED11          11 /* General-purpose ADIN11 */
#define ADC_UNUSED12          12 /* General-purpose TSX1/Touch screen X-plate 1 */
#define ADC_UNUSED13          13 /* General-purpose TSX2/Touch screen X-plate 2 */
#define ADC_UNUSED14          14 /* General-purpose TSY1/Touch screen Y-plate 1 */
#define ADC_UNUSED15          15 /* General-purpose TSY2/Touch screen Y-plate 2 */


#define ADC_UNREG_POWER ADC_BATTERY /* For compatibility */
#define ADC_READ_ERROR 0xFFFF

void adc_done(void);
int battery_adc_charge_current(void);
unsigned int battery_adc_temp(void);

#endif
