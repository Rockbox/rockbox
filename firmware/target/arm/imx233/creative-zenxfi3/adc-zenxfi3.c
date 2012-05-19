/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "adc-target.h"
#include "adc-imx233.h"

int imx233_adc_mapping[] =
{
    [ADC_BATTERY] = IMX233_ADC_BATTERY,
    [ADC_DIE_TEMP] = IMX233_ADC_DIE_TEMP,
    [ADC_VDDIO] = IMX233_ADC_VDDIO,
    [ADC_5V] = HW_LRADC_CHANNEL_5V,
    [ADC_BATT_TEMP] = IMX233_ADC_BATT_TEMP,
};

const char *imx233_adc_channel_name[] =
{
    "Battery(raw)",
    "Die temperature(°C)",
    "VddIO",
    "Vdd5V",
    "Battery temperature(raw)",
};
