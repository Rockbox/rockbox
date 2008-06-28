/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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

/* only two channels used by the Gigabeat */
#define NUM_ADC_CHANNELS 2

#define ADC_BATTERY     0
#define ADC_HPREMOTE    1
#define ADC_UNKNOWN_3   2
#define ADC_UNKNOWN_4   3
#define ADC_UNKNOWN_5   4
#define ADC_UNKNOWN_6   5
#define ADC_UNKNOWN_7   6
#define ADC_UNKNOWN_8   7

#define ADC_UNREG_POWER ADC_BATTERY /* For compatibility */
#define ADC_READ_ERROR 0xFFFF

#endif
