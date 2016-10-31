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
#include "nwz_lib.h"
#include "nwz_plattools.h"

static const char *charge_status_name(int chgstat)
{
    switch(chgstat)
    {
        case NWZ_POWER_STATUS_CHARGE_STATUS_CHARGING: return "charging";
        case NWZ_POWER_STATUS_CHARGE_STATUS_SUSPEND: return "suspend";
        case NWZ_POWER_STATUS_CHARGE_STATUS_TIMEOUT: return "timeout";
        case NWZ_POWER_STATUS_CHARGE_STATUS_NORMAL: return "normal";
        default: return "unknown";
    }
}

static const char *get_batt_gauge_name(int gauge)
{
    switch(gauge)
    {
        case NWZ_POWER_BAT_NOBAT: return "no batt";
        case NWZ_POWER_BAT_VERYLOW: return "very low";
        case NWZ_POWER_BAT_LOW: return "low";
        case NWZ_POWER_BAT_GAUGE0: return "____";
        case NWZ_POWER_BAT_GAUGE1: return "O___";
        case NWZ_POWER_BAT_GAUGE2: return "OO__";
        case NWZ_POWER_BAT_GAUGE3: return "OOO_";
        case NWZ_POWER_BAT_GAUGE4: return "OOOO";
        default: return "unknown";
    }
}

static const char *acc_charge_mode_name(int mode)
{
    switch(mode)
    {
        case NWZ_POWER_ACC_CHARGE_NONE: return "none";
        case NWZ_POWER_ACC_CHARGE_VBAT: return "vbat";
        case NWZ_POWER_ACC_CHARGE_VSYS: return "vsys";
        default: return "unknown";
    }
}

int NWZ_TOOL_MAIN(test_power)(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "test_power");
    nwz_lcdmsg(false, 0, 2, "BACK: quit");
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 3, 4, "Cannot open input device");
        sleep(2);
        return 1;
    }
    /* open adc device */
    int power_fd = nwz_power_open();
    if(power_fd < 0)
    {
        nwz_key_close(input_fd);
        nwz_lcdmsg(false, 3, 4, "Cannot open power device");
        sleep(2);
        return 1;
    }
    /* open pminfo device */
    int pminfo_fd = nwz_pminfo_open();
    if(pminfo_fd < 0)
    {
        nwz_key_close(power_fd);
        nwz_key_close(input_fd);
        nwz_lcdmsg(false, 3, 4, "Cannot open pminfo device");
        sleep(2);
        return 1;
    }
    /* display input state in a loop */
    while(1)
    {
        /* print status */
        int line = 4;
        int status = nwz_power_get_status(power_fd);
        int chgstat = status & NWZ_POWER_STATUS_CHARGE_STATUS;
        int acc_chg_mode = nwz_power_get_acc_charge_mode(power_fd);
        nwz_lcdmsgf(false, 0, line++, "ac detected: %s   ",
            (status & NWZ_POWER_STATUS_AC_DET) ? "yes" : "no");
        nwz_lcdmsgf(false, 0, line++, "vbus detected: %s   ",
            (status & NWZ_POWER_STATUS_VBUS_DET) ? "yes" : "no");
        nwz_lcdmsgf(false, 0, line++, "vbus voltage: %d mV (AD=%d)      ",
            nwz_power_get_vbus_voltage(power_fd), nwz_power_get_vbus_adval(power_fd));
        nwz_lcdmsgf(false, 0, line++, "vbus limit: %d mA      ",
            nwz_power_get_vbus_limit(power_fd));
        nwz_lcdmsgf(false, 0, line++, "vsys voltage: %d mV (AD=%d)      ",
            nwz_power_get_vsys_voltage(power_fd), nwz_power_get_vsys_adval(power_fd));
        nwz_lcdmsgf(false, 0, line++, "charge switch: %s       ",
            nwz_power_get_charge_switch(power_fd) ? "on" : "off");
        nwz_lcdmsgf(false, 0, line++, "full voltage: %s V      ",
            (status & NWZ_POWER_STATUS_CHARGE_LOW) ? "4.1" : "4.2");
        nwz_lcdmsgf(false, 0, line++, "current limit: %d mA     ",
            nwz_power_get_charge_current(power_fd));
        nwz_lcdmsgf(false, 0, line++, "charge status: %s (%x)     ",
            charge_status_name(chgstat), chgstat);
        nwz_lcdmsgf(false, 0, line++, "battery full: %s       ",
            nwz_power_is_fully_charged(power_fd) ? "yes" : "no");
        nwz_lcdmsgf(false, 0, line++, "bat gauge: %s (%d)       ",
            get_batt_gauge_name(nwz_power_get_battery_gauge(power_fd)),
            nwz_power_get_battery_gauge(power_fd));
        nwz_lcdmsgf(false, 0, line++, "avg voltage: %d mV (AD=%d)      ",
            nwz_power_get_battery_voltage(power_fd), nwz_power_get_battery_adval(power_fd));
        nwz_lcdmsgf(false, 0, line++, "sample count: %d       ",
            nwz_power_get_sample_count(power_fd));
        nwz_lcdmsgf(false, 0, line++, "raw voltage: %d mV (AD=%d)      ",
            nwz_power_get_vbat_voltage(power_fd), nwz_power_get_vbat_adval(power_fd));
        nwz_lcdmsgf(false, 0, line++, "acc charge mode: %s (%d)     ",
            acc_charge_mode_name(acc_chg_mode), acc_chg_mode);
        /* pminfo */
        line++;
        nwz_lcdmsgf(false, 0, line++, "pminfo: %#x     ", nwz_pminfo_get_factor(pminfo_fd));
        /* wait for event (1s) */
        int ret = nwz_key_wait_event(input_fd, 1000000);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK && !nwz_key_event_is_press(&evt))
            break;
    }
    /* finish nicely */
    nwz_key_close(power_fd);
    nwz_key_close(input_fd);
    nwz_pminfo_close(pminfo_fd);
    return 0;
}
