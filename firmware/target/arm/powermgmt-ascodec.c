/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sevakis
 * Copyright (C) 2008 by Bertrik Sikken
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
#include "config.h"
#include "system.h"
#include "thread.h"
#include "as3514.h"
#include "ascodec.h"
#include "adc.h"
#include "powermgmt.h"
#include "power.h"
#include "usb-target.h"
#include "usb.h"

/*===========================================================================
 * These parameters may be defined per target:
 * 
 *   BATT_FULL_VOLTAGE - Upon connect a charge cycle begins if the reading is
 *                       lower than this value (millivolts).
 *
 * BATT_VAUTO_RECHARGE - While left plugged after cycle completion, the
 *                       charger restarts automatically if the reading drops
 *                       below this value (millivolts). Must be less than 
 *                       BATT_FULL_VOLTAGE.
 *
 *         ADC_BATTERY - ADC channel from which to read the battery voltage
 *
 *          BATT_CHG_V - Charger voltage regulation setting (as3514 regval)
 *
 *          BATT_CHG_I - Charger current regulation setting (as3514 regval)
 *
 * CHARGER_TOTAL_TIMER - Maximum allowed charging time (1/2-second steps)
 *===========================================================================
 */

/* This code assumes USB power input is not distinguishable from main
 * power and charger connect cannot wait for USB configuration before
 * considering USB charging available. Where they are distinguishable,
 * things get more complicated. */
static bool charger_close = false;  /* Shutting down? */
static int charger_total_timer = 0; /* Timeout in algorithm steps */

/* Current battery threshold for (re)charge:
 * First plugged = BATT_FULL_VOLTAGE
 * After charge cycle or non-start = BATT_VAUTO_RECHARGE
 */
static unsigned int batt_threshold = 0;

/* ADC should read 0x3ff=5.12V */
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_BATTERY) * 5125 + 512) >> 10;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return charge_state == CHARGING;
}

/* Reset the battery filter to a new voltage */
static void battery_voltage_sync(void)
{
    int i;
    unsigned int mv;

    for (i = 0, mv = 0; i < 5; i++)
        mv += battery_adc_voltage();

    reset_battery_filter(mv / 5);
}

/* Disable charger and minimize all settings. Reset timers, etc. */
static void disable_charger(void)
{
    ascodec_write(AS3514_IRQ_ENRD0, 0);
    ascodec_write(AS3514_CHARGER,
                  TMPSUP_OFF | CHG_I_50MA | CHG_V_3_90V | CHG_OFF);

    if (charge_state > DISCHARGING)
        charge_state = DISCHARGING; /* Not an error state already */

    charger_total_timer = 0;
    battery_voltage_sync();
}

/* Enable charger with specified settings. Start timers, etc. */
static void enable_charger(void)
{
    ascodec_write(AS3514_CHARGER, BATT_CHG_I | BATT_CHG_V);
    /* Watch for end of charge. Temperature supervision is handled in
     * hardware. Charger status can be read and has no interrupt enable. */
    ascodec_write(AS3514_IRQ_ENRD0, CHG_ENDOFCH);

    sleep(HZ/10); /* Allow charger turn-on time (it could be gradual). */

    ascodec_read(AS3514_IRQ_ENRD0); /* Clear out interrupts (important!) */

    charge_state = CHARGING;
    charger_total_timer = CHARGER_TOTAL_TIMER;
    battery_voltage_sync();
}

void powermgmt_init_target(void)
{
    /* Everything CHARGER, OFF! */
    ascodec_write(AS3514_IRQ_ENRD0, 0);
    ascodec_write(AS3514_CHARGER,
                  TMPSUP_OFF | CHG_I_50MA | CHG_V_3_90V | CHG_OFF);
}

static inline void charger_plugged(void)
{
    batt_threshold = BATT_FULL_VOLTAGE; /* Start with topped value. */
    battery_voltage_sync();
}

static inline void charger_control(void)
{
    switch (charge_state)
    {
    case DISCHARGING:
    {
        unsigned int millivolts;
        unsigned int thresh = batt_threshold;

        if (BATT_FULL_VOLTAGE == thresh)
        {
            /* Wait for CHG_status to be indicated. */
            if ((ascodec_read(AS3514_IRQ_ENRD0) & CHG_STATUS) == 0)
                break;

            batt_threshold = BATT_VAUTO_RECHARGE;
        }

        millivolts = battery_voltage();

        if (millivolts <= thresh)
            enable_charger();
        break;
        } /* DISCHARGING: */

    case CHARGING:
    {
        if ((ascodec_read(AS3514_IRQ_ENRD0) & CHG_ENDOFCH) == 0)
        {
            if (--charger_total_timer > 0)
                break;

            /* Timer ran out - require replug. */
            charge_state = CHARGE_STATE_ERROR;
        }
        /* else end of charge */

        disable_charger();
        break;
        } /* CHARGING: */

    default:
        /* DISABLED, ERROR */
        break;
    }
}

static inline void charger_unplugged(void)
{
    disable_charger();
    if (charge_state >= CHARGE_STATE_ERROR)
        charge_state = DISCHARGING; /* Reset error */
}

/* Main charging algorithm - called from powermgmt.c */
void charging_algorithm_step(void)
{
    switch (charger_input_state)
    {
    case NO_CHARGER:
        /* Nothing to do */
        break;

    case CHARGER_PLUGGED:
        charger_plugged();
        break;

    case CHARGER:
        charger_control();
        break;

    case CHARGER_UNPLUGGED:
        charger_unplugged();
        break;
    }

    if (charger_close)
    {
        /* Disable further charging and ack. */
        charge_state = CHARGE_STATE_DISABLED;
        disable_charger();
        charger_close = false;
    }
}

/* Disable the charger and prepare for poweroff - called off-thread so we
 * signal the charging thread to prepare to quit. */
void charging_algorithm_close(void)
{
    charger_close = true;

    /* Power management thread will set it false again. */
    while (charger_close)
        sleep(HZ/10);
}
