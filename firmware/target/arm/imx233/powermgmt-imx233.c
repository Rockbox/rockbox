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

#if IMX233_SUBTARGET < 3700
/* The STMP3600 is completely different from the STMP3700+, we don't support it
 * at the moment */
static void charging_init(void)
{
}

static void charging_close(void)
{
}

static void charging_step(void)
{
    /* Unimplemented on STMP3600 */
}
#elif IMX233_SUBTARGET < 3780
/* The STMP3700 features a DCDC that takes battery input and produces
 * VDDD, VDDA and VDDIO. It also has a chain of three linear regulators:
 * VDD5V -> VDDIO -> VDDA -> VDDD that can be independently controlled.
 * Currently, we setup things as follows:
 * - on battery: use DCDC for maximal efficiency
 * - on 5V: use linear regulators to only draw from 5V and enable charger
 * Important note: we don't control the initial state, we can be ROLOed in any
 * state and the OF bootloader/stub can use a very different setup */
static void charging_init(void)
{
    /* the bootloader may be using some unusual setup so make sure the
     * linear regulation current limit is not set */
    BF_CLR(POWER_5VCTRL, ILIMIT_EQ_ZERO);
    BF_CLR(POWER_5VCTRL, ENABLE_ILIMIT);
    /* since we will power from regulators, disable stepping as per manual;
     * this appears to be very important, otherwise the chip will randomly
     * crash when switching from DCDC to linear regulators */
    BF_WR(POWER_VDDIOCTRL, DISABLE_STEPPING(1));
    BF_WR(POWER_VDDDCTRL, DISABLE_STEPPING(1));
    BF_WR(POWER_VDDACTRL, DISABLE_STEPPING(1));
}

static void charging_close(void)
{
}

static void charging_step(void)
{
    bool is_5v_present = usb_detect() == USB_INSERTED;

    /* initial state & 5v -> battery transition */
    if(!is_5v_present && charge_state != DISCHARGING)
    {
        logf("pwrmgmt: * -> discharging");
        charge_state = DISCHARGING;
        /* 5V has been lost: disable charger and make sure linear regulators are
         * disabled so we use DCDC */
        BF_WR(POWER_VDDDCTRL, ENABLE_LINREG(0));
        BF_WR(POWER_VDDACTRL, ENABLE_LINREG(0));
        BF_SET(POWER_CHARGE, PWD_BATTCHRG);
    }
    /* battery -> 5v transition */
    else if(is_5v_present && charge_state == DISCHARGING)
    {
        logf("pwrmgmt: discharging -> charging");
        /* disable DCDC and setup DCDC transfer for when we loose 5V */
        BF_CLR(POWER_5VCTRL, ENABLE_DCDC);
        BF_SET(POWER_5VCTRL, DCDC_XFER);
        /* enable battery charging */
        BF_CLR(POWER_CHARGE, PWD_BATTCHRG);
        charge_state = CHARGING;
        timeout_charging = current_tick + IMX233_CHARGING_TIMEOUT;
    }
    /* charging -> error transition */
    else if(charge_state == CHARGING && TIME_AFTER(current_tick, timeout_charging))
    {
        /* we have charged for a too long time, declare charger broken */
        logf("pwrmgmt: charging timeout exceeded!");
        logf("pwrmgmt: charging -> error");
        /* stop charging, note that we disabled the DCDC so we are now running
         * from linear regulators and thus we leave the battery untouched */
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
        /* stop charging, note that we disabled the DCDC so we are now running
         * from linear regulators and thus we leave the battery untouched */
        BF_SET(POWER_CHARGE, PWD_BATTCHRG);
        charge_state = CHARGE_STATE_DISABLED;
    }
}
#else /* IMX233_SUBTARGET >= 3780 */
/* The i.MX233 has a different architecture from STMP3700: it
 * features a DCDC that takes input either from battery from from a 4.2V rail
 * powered by a VDD5V -> 4P2 linear regulator. It also has a chain of three linear
 * regulators: VDD5V -> VDDIO -> VDDA -> VDDD that can be independently controlled,
 * and an extra linear regulator VDDIO -> VDDMEM that is always on.
 * Currently, we setup things as follows:
 * - on battery: we use the DCDC that we run from battery
 * - on 5V: we use the DCDC that we run from 4P2 and enable charger
 * Important note: we don't control the initial state, we can be ROLOed in any
 * state and the OF bootloader/stub can use a very different setup. In particular,
 * it is possible that the system is running from linear regulators */

#define MAX_4P2_ILIMIT  0x3f

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

static void charging_init(void)
{
    /* we need tick task to ramp up the 4P2 rail */
    tick_add_task(&ramp_up_4p2_rail);
    /* since we will run the DCDC all the time, make sure that it draws from 4.2
     * when available, that way when 5V is available, we never draw from battery */
    BF_WR(POWER_DCDC4P2, CMPTRIP(0)); /* 85% */
    BF_WR(POWER_DCDC4P2, DROPOUT_CTRL(0xe)); /* select greater, 200 mV drop */
    /* keep DCDC running when on 5V insertion */
    BF_CLR(POWER_5VCTRL, DCDC_XFER);
    BF_SET(POWER_5VCTRL, ENABLE_DCDC);
}

static void charging_close(void)
{
    tick_remove_task(&ramp_up_4p2_rail);
}

static void charging_step(void)
{
    bool is_5v_present = usb_detect() == USB_INSERTED;

    /* initial state & 5v -> battery transition */
    if(!is_5v_present && charge_state != DISCHARGING)
    {
        logf("pwrmgmt: * -> discharging");
        logf("pwrmgmt: disable charger and 4p2"); 
        charge_state = DISCHARGING;
        /* 5V has been lost: disable 4p2 power rail */
        BF_SET(POWER_CHARGE, PWD_BATTCHRG);
        BF_WR(POWER_DCDC4P2, ENABLE_DCDC(0));
        BF_WR(POWER_DCDC4P2, ENABLE_4P2(0));
        BF_WR(POWER_5VCTRL, CHARGE_4P2_ILIMIT(0));
        BF_SET(POWER_5VCTRL, PWD_CHARGE_4P2);
        /* prepare for next 5V transition: keep DCDC running on 5V insertion */
        BF_WR(POWER_5VCTRL, ENABLE_DCDC(1));
        /* at this point we can safely disable linear regulators in case those
         * were enabled */
        BF_WR(POWER_VDDDCTRL, ENABLE_LINREG(0));
        BF_WR(POWER_VDDACTRL, ENABLE_LINREG(0));
    }
    /* battery -> 5v transition */
    else if(is_5v_present && charge_state == DISCHARGING)
    {
        logf("pwrmgmt: discharging -> trickle");
        logf("pwrmgmt: begin charging 4p2");
        /* 5V has been detected: prepare 4.2V power rail for activation
         * WARNING we can reach this situation when starting after Freescale bootloader
         * or after RoLo in a state where the DCDC is already running. In this case,
         * we must *NOT* disable it or this will shutdown the device. This procedure
         * is safe: it will never disable the DCDC and will not reduce the charge
         * limit on the 4P2 rail. */
        BF_WR(POWER_DCDC4P2, ENABLE_4P2(1));
        BF_SET(POWER_CHARGE, ENABLE_LOAD);
        BF_CLR(POWER_5VCTRL, PWD_CHARGE_4P2);
        BF_WR(POWER_DCDC4P2, ENABLE_DCDC(1));
        /* the tick task will take care of slowly ramping up the current in the rail
         * every 10ms (since it runs at HZ and HZ=100) */
        charge_state = TRICKLE;
    }
    /* trickle -> charging transition */
    else if(charge_state == TRICKLE)
    {
        /* If 4.2V current limit has not reached 780mA, don't do anything, the
         * tick task is still running */
        /* If we've reached the maximum, take action */
        if(BF_RD(POWER_5VCTRL, CHARGE_4P2_ILIMIT) == MAX_4P2_ILIMIT)
        {
            logf("pwrmgmt: enable dcdc and charger");
            logf("pwrmgmt: trickle -> charging");
            /* adjust arbitration between 4.2 and battery */
            BF_WR(POWER_DCDC4P2, CMPTRIP(0)); /* 85% */
            BF_WR(POWER_DCDC4P2, DROPOUT_CTRL(0xe)); /* select greater, 200 mV drop */
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
}
#endif /* IMX233_SUBTARGET */

void powermgmt_init_target(void)
{
    charge_state = DISCHARGING;
    charging_init();
}

void charging_algorithm_step(void)
{
    charging_step();
}

void charging_algorithm_close(void)
{
    charging_close();
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
