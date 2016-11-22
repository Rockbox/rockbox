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

#include "system.h"
#include "power-nwz.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static int power_fd = -1; /* file descriptor */

void power_init(void)
{
    power_fd = open(NWZ_POWER_DEV, O_RDWR);
}

void power_close(void)
{
    close(power_fd);
}

int nwz_power_get_status(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_STATUS, &status) < 0)
        return -1;
    return status;
}

static int nwz_power_adval_to_mv(int adval, int ad_base)
{
    if(adval == -1)
        return -1;
    /* the AD base corresponds to the millivolt value if adval was 255 */
    return (adval * ad_base) / 255;
}

int nwz_power_get_vbus_adval(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_VBUS_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vbus_voltage(void)
{
    return nwz_power_adval_to_mv(nwz_power_get_vbus_adval(), NWZ_POWER_AD_BASE_VBUS);
}

int nwz_power_get_vbus_limit(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_VBUS_LIMIT, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_charge_switch(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_CHARGE_SWITCH, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_charge_current(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_CHARGE_CURRENT, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_battery_gauge(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_BAT_GAUGE, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_battery_adval(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_BAT_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_battery_voltage(void)
{
    return nwz_power_adval_to_mv(nwz_power_get_battery_adval(), NWZ_POWER_AD_BASE_VBAT);
}

int nwz_power_get_vbat_adval(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_VBAT_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vbat_voltage(void)
{
    return nwz_power_adval_to_mv(nwz_power_get_vbat_adval(), NWZ_POWER_AD_BASE_VBAT);
}

int nwz_power_get_sample_count(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_SAMPLE_COUNT, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vsys_adval(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_VSYS_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vsys_voltage(void)
{
    return nwz_power_adval_to_mv(nwz_power_get_vsys_adval(), NWZ_POWER_AD_BASE_VSYS);
}

int nwz_power_get_acc_charge_mode(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_GET_ACCESSARY_CHARGE_MODE, &status) < 0)
        return -1;
    return status;
}

int nwz_power_is_fully_charged(void)
{
    int status;
    if(ioctl(power_fd, NWZ_POWER_IS_FULLY_CHARGED, &status) < 0)
        return -1;
    return status;
}
