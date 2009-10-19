/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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

/*  Channel 0 is connected to an on board pot for testing
    Channels 0-3 are available via expansion connector CON4
    Channels 4-7 are routed to LCD connector for touchscreen operation if
        supported by display panel. 
*/
#define NUM_ADC_CHANNELS 8

#define ADC_ONBOARD     0
#define ADC_SPARE_1     1
#define ADC_SPARE_2     2
#define ADC_SPARE_3     3
#define ADC_TSYM        4
#define ADC_TSYP        5
#define ADC_TSXM        6
#define ADC_TSXP        7

#define ADC_READ_ERROR 0xFFFF

#endif
