/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sevakis
 * Copyright (C) 2008 by Bertrik Sikken
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
#ifndef POWERMGMT_TARGET_H
#define POWERMGMT_TARGET_H

#if defined(SANSA_C200)
/* This configuration triggers a single charge cycle upon plugging and stops.
 * The true voltage cannot be read accurately and so monitoring isn't really
 * possible. The battery filter is still synced to have a proper reading
 * when disconnecting. */
#define BATT_FULL_VOLTAGE   5121 /* Won't read this high - force start */
#define BATT_VAUTO_RECHARGE 0    /* Won't read this low - force one cycle */
#define BATT_CHG_V          CHG_V_4_20V
#define BATT_CHG_I          CHG_I_300MA
#define CHARGER_TOTAL_TIMER (4*3600*2) /* 4 hours enough? */
#define ADC_BATTERY         ADC_BVDD
#elif defined(SANSA_E200)
/* PREFERRED - Check if topped-off and monitor voltage while plugged. */
#define BATT_FULL_VOLTAGE   4160
#define BATT_VAUTO_RECHARGE 4100
#define BATT_CHG_V          CHG_V_4_20V
#define BATT_CHG_I          CHG_I_300MA
#define CHARGER_TOTAL_TIMER (4*3600*2) /* 4 hours enough? */
/* On e200 ADC_RTCSUP seems to represent battery voltage better than
 * ADC_BVDD during charging (ADC_BVDD is way too high) and appears the
 * same in normal use.
 */
#define ADC_BATTERY         ADC_RTCSUP
#endif

void powermgmt_init_target(void);
void charging_algorithm_step(void);
void charging_algorithm_close(void);

/* We want to be able to reset the averaging filter */
#define HAVE_RESET_BATTERY_FILTER

#define BATT_AVE_SAMPLES 32

#endif /*  POWERMGMT_TARGET_H */
