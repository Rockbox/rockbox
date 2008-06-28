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

/* ADC channels */
#define NUM_ADC_CHANNELS 13

#define ADC_BVDD         0  /* Battery voltage of 4V LiIo accumulator */
#define ADC_RTCSUP       1  /* RTC backup battery voltage */
#define ADC_UVDD         2  /* USB host voltage */
#define ADC_CHG_IN       3  /* Charger input voltage */
#define ADC_CVDD         4  /* Charger pump output voltage */
#define ADC_BATTEMP      5  /* Battery charging temperature */
#define ADC_MICSUP1      6  /* Voltage on MicSup1 for remote control
                                or external voltage measurement */
#define ADC_MICSUP2      7  /* Voltage on MicSup1 for remote control
                                or external voltage measurement */
#define ADC_VBE1         8  /* Measuring junction temperature @ 2uA */
#define ADC_VBE2         9  /* Measuring junction temperature @ 1uA */
#define ADC_I_MICSUP1    10 /* Current of MicSup1 for remote control detection */
#define ADC_I_MICSUP2    11 /* Current of MicSup2 for remote control detection */
#define ADC_VBAT         12 /* Single cell battery voltage */

#define ADC_UNREG_POWER  ADC_BVDD   /* For compatibility */

#endif
