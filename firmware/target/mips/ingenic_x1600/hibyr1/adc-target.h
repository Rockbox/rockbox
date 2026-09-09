/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __ADC_TARGET_H__
#define __ADC_TARGET_H__

/* SADC channel assignments from the vendor configuration. */
#define ADC_BUTTONS         0   /* key resistor ladder */
#define ADC_HP_DETECT       1   /* headphone jack presence */
#define ADC_REMOTE          2   /* in-line remote / earpod buttons */
#define ADC_PANEL_ID        3   /* unverified, see below */

#define NUM_ADC_CHANNELS    4

/* The vendor reports adc_vref=3300, and its idle reading is also 3300 */
#define ADC_VREF_MV         3300

/* 12-bit converter: V = ADATA * VREF / 4096 */
#define ADC_RESOLUTION      4096

/* adc_read() returns the raw code; the thresholds below are millivolts */
#define ADC_TO_MV(raw)      (((raw) * ADC_VREF_MV) / ADC_RESOLUTION)

/* Defined in power-hibyr1.c */
extern int adc_read_mv(int channel);

/* Key ladder on AUX0, from the vendor's key1..key3; key4..key8 are
 * unpopulated, and the window is its own +/- deviation */
#define ADC_KEY_WINDOW_MV   100
#define ADC_KEY_IDLE_MV     3300
#define ADC_KEY_PLAY_MV     0
#define ADC_KEY_VOLDOWN_MV  1000
#define ADC_KEY_VOLUP_MV    1650

/* Headphone detect on AUX1: in range means inserted, with a 200 ms debounce */
#define ADC_HP_MIN_MV       2800
#define ADC_HP_MAX_MV       3300

/* In-line remote on AUX2, fed from aldo4. Not wired up yet; recorded so the
 * numbers are not lost. */
#define ADC_REMOTE_PLAY_MIN_MV      0
#define ADC_REMOTE_PLAY_MAX_MV      10
#define ADC_REMOTE_VOLUP_MIN_MV     50
#define ADC_REMOTE_VOLUP_MAX_MV     250
#define ADC_REMOTE_VOLDOWN_MIN_MV   350
#define ADC_REMOTE_VOLDOWN_MAX_MV   550

/* AUX3 is unused; its board connection is unknown. */

#endif /* __ADC_TARGET_H__ */
