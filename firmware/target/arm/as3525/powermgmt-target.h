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

#if defined(SANSA_CLIP) \
    || defined(SANSA_CLIPV2)    /* FIXME */ \
    || defined(SANSA_CLIPPLUS)  /* FIXME */

/* Check if topped-off and monitor voltage while plugged. */
#define BATT_FULL_VOLTAGE   4160
#define BATT_VAUTO_RECHARGE 4100
#define BATT_CHG_V          CHG_V_4_20V
#define BATT_CHG_I          CHG_I_100MA
#define CHARGER_TOTAL_TIMER (6*3600*2)  /* about 1.5 * capacity / current */
#if defined(SANSA_CLIP)
#define ADC_BATTERY         ADC_BVDD
#else
/* ADC_RTCSUP seems to represent battery voltage better than ADC_BVDD during
 * charging (ADC_BVDD is way too high) and appears the same in normal use. */
#define ADC_BATTERY         ADC_RTCSUP
#endif

#elif defined(SANSA_E200V2)

/* Check if topped-off and monitor voltage while plugged. */
#define BATT_FULL_VOLTAGE   4160
#define BATT_VAUTO_RECHARGE 4100
#define BATT_CHG_V          CHG_V_4_20V
#define BATT_CHG_I          CHG_I_300MA
#define CHARGER_TOTAL_TIMER (4*3600*2)
#define ADC_BATTERY         ADC_BVDD

#elif defined(SANSA_FUZE) || defined(SANSA_FUZEV2) /* FIXME */

/* Check if topped-off and monitor voltage while plugged. */
#define BATT_FULL_VOLTAGE   4160
#define BATT_VAUTO_RECHARGE 4100
#define BATT_CHG_V          CHG_V_4_20V
#define BATT_CHG_I          CHG_I_200MA
#define CHARGER_TOTAL_TIMER (4*3600*2)
#define ADC_BATTERY         ADC_BVDD

#elif defined(SANSA_C200V2)

/* Check if topped-off and monitor voltage while plugged. */
#define BATT_FULL_VOLTAGE   4160
#define BATT_VAUTO_RECHARGE 4100
#define BATT_CHG_V          CHG_V_4_20V
#define BATT_CHG_I          CHG_I_200MA
#define CHARGER_TOTAL_TIMER (4*3600*2)
#define ADC_BATTERY         ADC_BVDD

#else
#error "Charger settings not defined!"
#endif

void powermgmt_init_target(void);
void charging_algorithm_step(void);
void charging_algorithm_close(void);

/* We want to be able to reset the averaging filter */
#define HAVE_RESET_BATTERY_FILTER

#define BATT_AVE_SAMPLES 32

#endif /*  POWERMGMT_TARGET_H */
