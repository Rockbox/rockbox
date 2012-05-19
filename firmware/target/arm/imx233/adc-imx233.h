/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef _ADC_IMX233_H_
#define _ADC_IMX233_H_

#include "system.h"
#include "adc.h"
#include "adc-target.h"
#include "powermgmt-target.h"
#include "lradc-imx233.h"

/* Virtual channels */
#define IMX233_ADC_BATTERY      -1 /* Battery voltage (mV) */
#define IMX233_ADC_DIE_TEMP     -2 /* Die temperature (°C) */
#define IMX233_ADC_VDDIO        -3 /* VddIO voltage (mV) */
#define IMX233_ADC_VDD5V        -4 /* Vdd5V voltage (mV) */
#ifdef IMX233_BATT_TEMP_SENSOR
#define IMX233_ADC_BATT_TEMP    -5 /* Battery temperature (°C) */
#endif

/* Channel mapping */
extern int imx233_adc_mapping[];
/* Channel names */
extern const char *imx233_adc_channel_name[];

#endif
