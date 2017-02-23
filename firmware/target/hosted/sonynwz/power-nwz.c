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
#include "button-target.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/reboot.h>

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

static int write_string_to_file(const char *file, const char *msg)
{
    int fd = open(file, O_WRONLY);
    if(fd < 0)
        return -1;
    const int len = strlen(msg);
    int res = write(fd, msg, len);
    close(fd);
    return res == len ? 0 : -1;
}

int do_nwz_power_suspend(void)
{
    /* older devices use /proc/pm, while the new one use the standard sys file */
    if(write_string_to_file("/proc/pm", "S3") == 0)
        return 0;
    return write_string_to_file("/sys/power/state", "mem");
}

int nwz_power_suspend(void)
{
    int ret = do_nwz_power_suspend();
    /* the button driver tracks button status using events, but the kernel does
     * not generate an event if a key is changed during suspend, so make sure we
     * reload as much information as possible */
    nwz_button_reload_after_suspend();
    return ret;
}

int nwz_power_shutdown(void)
{
    sync(); /* man page advises to sync to avoid data loss */
    return reboot(RB_POWER_OFF);
}

int nwz_power_restart(void)
{
    sync(); /* man page advises to sync to avoid data loss */
    return reboot(RB_AUTOBOOT);
}
