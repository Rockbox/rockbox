/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 by Amaury Pouly
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

#include "adc.h"
#include "adc-target.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static int adc_fd = -1; /* file descriptor */

static const char *nwz_adc_channel_name[NWZ_ADC_MAX_CHAN + 1] =
{
    [NWZ_ADC_VCCBAT] = "VCCBAT",
    [NWZ_ADC_VCCVBUS] = "VCCBUS",
    [NWZ_ADC_ADIN3] = "ADIN3",
    [NWZ_ADC_ADIN4] = "ADIN4",
    [NWZ_ADC_ADIN5] = "ADIN5",
    [NWZ_ADC_ADIN6] = "ADIN6",
    [NWZ_ADC_ADIN7] = "ADIN7",
    [NWZ_ADC_ADIN8] = "ADIN8"
};

void adc_init(void)
{
    adc_fd = open(NWZ_ADC_DEV, O_RDONLY);
}

unsigned short adc_read(int channel)
{
    unsigned char val;
    if(ioctl(adc_fd, NWZ_ADC_GET_VAL(channel), &val) < 0)
        return 0;
    else
        return val;
}

const char *adc_name(int channel)
{
    if(channel < NWZ_ADC_MIN_CHAN || channel > NWZ_ADC_MAX_CHAN)
        return "";
    return nwz_adc_channel_name[channel];
}
 
