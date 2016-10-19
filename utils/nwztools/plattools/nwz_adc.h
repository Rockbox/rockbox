/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __NWZ_ADC_H__
#define __NWZ_ADC_H__

#define NWZ_ADC_DEV  "/dev/icx_adc"

#define NWZ_ADC_TYPE    'm'

#define NWZ_ADC_MIN_CHAN    0
#define NWZ_ADC_MAX_CHAN    7

#define NWZ_ADC_VCCBAT  0
#define NWZ_ADC_VCCVBUS 1
#define NWZ_ADC_ADIN3   2
#define NWZ_ADC_ADIN4   3
#define NWZ_ADC_ADIN5   4
#define NWZ_ADC_ADIN6   5
#define NWZ_ADC_ADIN7   6
#define NWZ_ADC_ADIN8   7

#define NWZ_ADC_GET_VAL(chan)   _IOR(NWZ_ADC_TYPE, chan, unsigned char)

#endif /* __NWZ_ADC_H__ */
