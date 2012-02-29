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

#include "powermgmt.h"
#include "power-imx233.h"
#include "usb.h"
#include "string.h"
//#define LOGF_ENABLE
#include "logf.h"

#if !defined(IMX233_CHARGE_CURRENT) || !defined(IMX233_STOP_CURRENT) \
    || !defined(IMX233_CHARGING_TIMEOUT) || !defined(IMX233_TOPOFF_TIMEOUT)
#error You must define IMX233_CHARGE_CURRENT, IMX233_STOP_CURRENT, \
    IMX233_CHARGING_TIMEOUT and IMX233_TOPOFF_TIMEOUT !
#endif

/* charger state is maintained in charge_state (see powermgmt.h) */
static int timeout_charging; /* timeout before charging will be declared broken */
static int timeout_topping_off; /* timeout before stopping charging after topping off */
static int timeout_4p2_ilimit_increase; /* timeout before increasing 4p2 ilimit */

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    /* battery value is in 8mV LSB */
    return __XTRACT(HW_POWER_BATTMONITOR, BATT_VAL) * 8;
}

void powermgmt_init_target(void)
{
    imx233_power_set_charge_current(IMX233_CHARGE_CURRENT);
    imx233_power_set_stop_current(IMX233_STOP_CURRENT);
    /* assume that adc_init was called and battery monitoring via LRADC setup */
    __REG_SET(HW_POWER_BATTMONITOR) = HW_POWER_BATTMONITOR__ENBATADJ;
    /* make sure we are in a known state: disable charger and 4p2 */
    __REG_SET(HW_POWER_CHARGE) = HW_POWER_CHARGE__PWD_BATTCHRG;
    __REG_CLR(HW_POWER_DCDC4P2) = HW_POWER_DCDC4P2__ENABLE_DCDC |
        HW_POWER_DCDC4P2__ENABLE_4P2;
    __REG_SET(HW_POWER_5VCTRL) = HW_POWER_5VCTRL__PWD_CHARGE_4P2;
    charge_state = DISCHARGING;
}

void charging_algorithm_step(void)
{
    bool is_5v_present = usb_detect() == USB_INSERTED;

    /* initial state & 5v -> battery transition */
    if(!is_5v_present && charge_state != DISCHARGING)
    {
        logf("pwrmgmt: * -> discharging");
        logf("pwrmgmt: disable charger and 4p2"); 
        /* 5V has been lost: disable 4p2 power rail */
        __REG_SET(HW_POWER_CHARGE) = HW_POWER_CHARGE__PWD_BATTCHRG;
        __REG_CLR(HW_POWER_DCDC4P2) = HW_POWER_DCDC4P2__ENABLE_DCDC |
            HW_POWER_DCDC4P2__ENABLE_4P2;
        __REG_SET(HW_POWER_5VCTRL) = HW_POWER_5VCTRL__PWD_CHARGE_4P2;
        charge_state = DISCHARGING;
    }
    /* battery -> 5v transition */
    else if(is_5v_present && charge_state == DISCHARGING)
    {
        logf("pwrmgmt: discharging -> trickle");
        logf("pwrmgmt: begin charging 4p2");
        /* 5V has been detected: prepare 4.2V power rail for activation */
        __REG_SET(HW_POWER_DCDC4P2) = HW_POWER_DCDC4P2__ENABLE_4P2;
        __REG_SET(HW_POWER_CHARGE) = HW_POWER_CHARGE__ENABLE_LOAD;
        __FIELD_SET(HW_POWER_5VCTRL, CHARGE_4P2_ILIMIT, 1);
        __REG_CLR(HW_POWER_5VCTRL) = HW_POWER_5VCTRL__PWD_CHARGE_4P2;// FIXME: manual error ?
        __REG_SET(HW_POWER_DCDC4P2) = HW_POWER_DCDC4P2__ENABLE_DCDC;
        timeout_4p2_ilimit_increase = current_tick + HZ / 100;
        charge_state = TRICKLE;
    }
    else if(charge_state == TRICKLE && TIME_AFTER(current_tick, timeout_4p2_ilimit_increase))
    {
        /* if 4.2V current limit has not reached 780mA, increase it slowly to
         * charge the 4.2V capacitance */
        if(__XTRACT(HW_POWER_5VCTRL, CHARGE_4P2_ILIMIT) != 0x3f)
        {
            //logf("pwrmgmt: incr 4.2 ilimit");
            HW_POWER_5VCTRL += 1 << HW_POWER_5VCTRL__CHARGE_4P2_ILIMIT_BP;
            timeout_4p2_ilimit_increase = current_tick + HZ / 100;
        }
        /* we've reached the maximum, take action */
        else
        {
            logf("pwrmgmt: enable dcdc and charger");
            logf("pwrmgmt: trickle -> charging");
            /* adjust arbitration between 4.2 and battery */
            __FIELD_SET(HW_POWER_DCDC4P2, CMPTRIP, 0); /* 85% */
            __FIELD_SET(HW_POWER_DCDC4P2, DROPOUT_CTRL, 0xe); /* select greater, 200 mV drop */
            __REG_CLR(HW_POWER_5VCTRL) = HW_POWER_5VCTRL__DCDC_XFER;
            __REG_SET(HW_POWER_5VCTRL) = HW_POWER_5VCTRL__ENABLE_DCDC;
            /* enable battery charging */
            __REG_CLR(HW_POWER_CHARGE) = HW_POWER_CHARGE__PWD_BATTCHRG;
            charge_state = CHARGING;
            timeout_charging = current_tick + IMX233_CHARGING_TIMEOUT;
        }
    }
    else if(charge_state == CHARGING && TIME_AFTER(current_tick, timeout_charging))
    {
        /* we have charged for a too long time, declare charger broken */
        logf("pwrmgmt: charging timeout exceeded!");
        logf("pwrmgmt: charging -> error");
        /* stop charging */
        __REG_SET(HW_POWER_5VCTRL) = HW_POWER_5VCTRL__PWD_CHARGE_4P2;
        /* goto error state */
        charge_state = CHARGE_STATE_ERROR;
    }
    else if(charge_state == CHARGING && !(HW_POWER_STS & HW_POWER_STS__CHRGSTS))
    {
        logf("pwrmgmt: topping off");
        logf("pwrmgmt: charging -> topoff");
        charge_state = TOPOFF;
        timeout_topping_off = current_tick + IMX233_TOPOFF_TIMEOUT;
    }
    else if(charge_state == TOPOFF && TIME_AFTER(current_tick, timeout_topping_off))
    {
        logf("pwrmgmt: charging finished");
        logf("pwrmgmt: topoff -> disabled");
        /* stop charging */
        __REG_SET(HW_POWER_CHARGE) = HW_POWER_CHARGE__PWD_BATTCHRG;
        charge_state = CHARGE_STATE_DISABLED;
    }
}

void charging_algorithm_close(void)
{
}

struct imx233_powermgmt_info_t imx233_powermgmt_get_info(void)
{
    struct imx233_powermgmt_info_t info;
    memset(&info, 0, sizeof(info));
    info.state = charge_state;
    info.charging_timeout =
        charge_state == CHARGING ? timeout_charging - current_tick : 0;
    info.topoff_timeout =
        charge_state == TOPOFF ? timeout_topping_off - current_tick : 0;
    info.incr_4p2_ilimit_timeout =
        charge_state == TRICKLE ? timeout_4p2_ilimit_increase - current_tick : 0;
    return info;
}
