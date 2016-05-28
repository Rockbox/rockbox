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

#include "system.h"
#include "powermgmt.h"
#include "power-imx233.h"
#include "usb.h"
#include "string.h"
//#define LOGF_ENABLE
#include "logf.h"
#include "powermgmt-imx233.h"

#include "regs/power.h"

#if !defined(IMX233_CHARGE_CURRENT) || !defined(IMX233_STOP_CURRENT) \
    || !defined(IMX233_CHARGING_TIMEOUT) || !defined(IMX233_TOPOFF_TIMEOUT)
#error You must define IMX233_CHARGE_CURRENT, IMX233_STOP_CURRENT, \
    IMX233_CHARGING_TIMEOUT and IMX233_TOPOFF_TIMEOUT !
#endif

/* charger state is maintained in charge_state (see powermgmt.h) */
static int timeout_charging; /* timeout before charging will be declared broken */
static int timeout_topping_off; /* timeout before stopping charging after topping off */

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    /* battery value is in 8mV LSB */
    return BF_RD(POWER_BATTMONITOR, BATT_VAL) * 8;
}

void imx233_powermgmt_init(void)
{
    imx233_power_set_charge_current(IMX233_CHARGE_CURRENT);
    imx233_power_set_stop_current(IMX233_STOP_CURRENT);
#if IMX233_SUBTARGET >= 3700
    /* assume that adc_init was called and battery monitoring via LRADC setup */
    BF_WR(POWER_BATTMONITOR, EN_BATADJ(1));
    /* setup linear regulator offsets to 25 mV below to prevent contention between
     * linear regulators and DCDC */
    BF_WR(POWER_VDDDCTRL, LINREG_OFFSET(2));
    BF_WR(POWER_VDDACTRL, LINREG_OFFSET(2));
    BF_WR(POWER_VDDIOCTRL, LINREG_OFFSET(2));
    /* enable a few bits controlling the DC-DC as recommended by Freescale */
    BF_SET(POWER_LOOPCTRL, TOGGLE_DIF);
    BF_SET(POWER_LOOPCTRL, EN_CM_HYST);
    BF_CS(POWER_LOOPCTRL, EN_RCSCALE(1));
#else
    BF_SET(POWER_5VCTRL, LINREG_OFFSET);
#endif
}

#define MAX_4P2_ILIMIT  0x3f

#if IMX233_SUBTARGET >= 3780
/* The code below assumes HZ = 100 so that it runs every 10ms */
#if HZ != 100
#warning The ramp_up_4p2_rail() tick task assumes HZ = 100, this may break charging
#endif

static void ramp_up_4p2_rail(void)
{
    /* only ramp up in the TRICKLE state and if we haven't reached the maximum yet */
    if(charge_state == TRICKLE && BF_RD(POWER_5VCTRL, CHARGE_4P2_ILIMIT) < MAX_4P2_ILIMIT)
        HW_POWER_5VCTRL += BF_POWER_5VCTRL_CHARGE_4P2_ILIMIT(1);
}
#endif /* IMX233_SUBTARGET >= 3780 */

void powermgmt_init_target(void)
{
    charge_state = DISCHARGING;
    /* stmp < 3780 does not have a 4.2 rail */
#if IMX233_SUBTARGET >= 3780
    tick_add_task(&ramp_up_4p2_rail);
#endif
}

void charging_algorithm_step(void)
{
#if IMX233_SUBTARGET >= 3700
    bool is_5v_present = usb_detect() == USB_INSERTED;

    /* initial state & 5v -> battery transition */
    if(!is_5v_present && charge_state != DISCHARGING)
    {
        logf("pwrmgmt: * -> discharging");
        logf("pwrmgmt: disable charger and 4p2"); 
        charge_state = DISCHARGING;
        /* 5V has been lost: disable 4p2 power rail */
        BF_SET(POWER_CHARGE, PWD_BATTCHRG);
#if IMX233_SUBTARGET >= 3780
        BF_WR(POWER_DCDC4P2, ENABLE_DCDC(0));
        BF_WR(POWER_DCDC4P2, ENABLE_4P2(0));
        BF_WR(POWER_5VCTRL, CHARGE_4P2_ILIMIT(0));
        BF_SET(POWER_5VCTRL, PWD_CHARGE_4P2);
#endif
    }
    /* battery -> 5v transition */
    else if(is_5v_present && charge_state == DISCHARGING)
    {
        logf("pwrmgmt: discharging -> trickle");
        logf("pwrmgmt: begin charging 4p2");
#if IMX233_SUBTARGET >= 3780
        /* 5V has been detected: prepare 4.2V power rail for activation
         * WARNING we can reach this situation when starting after Freescale bootloader
         * or after RoLo in a state where the DCDC is running. In this case,
         * we must *NOT* disable it or this will shutdown the device. This procedure
         * is safe: it will never disable the DCDC and will not reduce the charge
         * limit on the 4P2 rail. */
        BF_WR(POWER_DCDC4P2, ENABLE_4P2(1));
        BF_SET(POWER_CHARGE, ENABLE_LOAD);
        BF_WR(POWER_5VCTRL, CHARGE_4P2_ILIMIT(0)); /* start by drawing 0mA */
        BF_CLR(POWER_5VCTRL, PWD_CHARGE_4P2);// FIXME: manual error ?
        BF_WR(POWER_DCDC4P2, ENABLE_DCDC(1));
        /* the tick task will take care of slowly ramping up the current in the rail
         * every 10ms (since it runs at HZ and HZ=100) */
#endif
        charge_state = TRICKLE;
    }
    /* trickle -> charging transition */
    else if(charge_state == TRICKLE)
    {
#if IMX233_SUBTARGET >= 3780
        /* If 4.2V current limit has not reached 780mA, don't do anything, the
         * DPC is still running */
        /* If we've reached the maximum, take action */
        if(BF_RD(POWER_5VCTRL, CHARGE_4P2_ILIMIT) == MAX_4P2_ILIMIT)
#endif
        {
            logf("pwrmgmt: enable dcdc and charger");
            logf("pwrmgmt: trickle -> charging");
#if IMX233_SUBTARGET >= 3780
            /* adjust arbitration between 4.2 and battery */
            BF_WR(POWER_DCDC4P2, CMPTRIP(0)); /* 85% */
            BF_WR(POWER_DCDC4P2, DROPOUT_CTRL(0xe)); /* select greater, 200 mV drop */
#endif
            /* switch to DCDC */
            BF_CLR(POWER_5VCTRL, DCDC_XFER);
            BF_SET(POWER_5VCTRL, ENABLE_DCDC);
            /* enable battery charging */
            BF_CLR(POWER_CHARGE, PWD_BATTCHRG);
            charge_state = CHARGING;
            timeout_charging = current_tick + IMX233_CHARGING_TIMEOUT;
        }
    }
    /* charging -> error transition */
    else if(charge_state == CHARGING && TIME_AFTER(current_tick, timeout_charging))
    {
        /* we have charged for a too long time, declare charger broken */
        logf("pwrmgmt: charging timeout exceeded!");
        logf("pwrmgmt: charging -> error");
        /* stop charging, note that we leave the 4.2 rail active so that the DCDC
         * keep drawing current from the 4.2 only and leave the battery untouched */
        BF_SET(POWER_CHARGE, PWD_BATTCHRG);
        /* goto error state */
        charge_state = CHARGE_STATE_ERROR;
    }
    /* charging -> topoff transition */
    else if(charge_state == CHARGING && !BF_RD(POWER_STS, CHRGSTS))
    {
        logf("pwrmgmt: topping off");
        logf("pwrmgmt: charging -> topoff");
        charge_state = TOPOFF;
        timeout_topping_off = current_tick + IMX233_TOPOFF_TIMEOUT;
    }
    /* topoff -> disabled transition */
    else if(charge_state == TOPOFF && TIME_AFTER(current_tick, timeout_topping_off))
    {
        logf("pwrmgmt: charging finished");
        logf("pwrmgmt: topoff -> disabled");
        /* stop charging, note that we leave the 4.2 rail active so that the DCDC
         * keep drawing current from the 4.2 only and leave the battery untouched */
        BF_SET(POWER_CHARGE, PWD_BATTCHRG);
        charge_state = CHARGE_STATE_DISABLED;
    }
#endif
}

void charging_algorithm_close(void)
{
#if IMX233_SUBTARGET >= 3780
    tick_remove_task(&ramp_up_4p2_rail);
#endif
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
    return info;
}
