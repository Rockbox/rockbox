/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#include "jz4740.h"
#include "powermgmt.h"
#include "kernel.h"
#include "logf.h"
#include "adc.h"

#define SADC_CFG_INIT   (                                 \
                        (2 << SADC_CFG_CLKOUT_NUM_BIT) |  \
                        (1 << SADC_CFG_CLKDIV_BIT)     |  \
                        SADC_CFG_PBAT_HIGH             |  \
                        SADC_CFG_CMD_INT_PEN              \
                        )

static volatile unsigned short bat_val;
static struct mutex battery_mtx;

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* TODO */
    1000
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    /* TODO */
    900
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* TODO */
    { 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* TODO */
    1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000
};

/* VBAT = (BDATA/4096) * 7.5V */
#define BATTERY_SCALE_FACTOR 1875

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    unsigned int dummy, timeout=HZ/4;
    
    mutex_lock(&battery_mtx);
    
    dummy = REG_SADC_BATDAT;
    dummy = REG_SADC_BATDAT;
    (void)dummy;

    REG_SADC_ENA |= SADC_ENA_PBATEN;
    bat_val = 0;
    
    /* primitive wakeup event */
    while(bat_val == 0 && timeout--)
        sleep(0);
    
    logf("%d %d", bat_val, (bat_val*BATTERY_SCALE_FACTOR)>>10);
    
    mutex_unlock(&battery_mtx);
    
    return (bat_val*BATTERY_SCALE_FACTOR)>>10;
}

void adc_init(void)
{
    __cpm_start_sadc();
    REG_SADC_ENA = 0;
    REG_SADC_STATE &= ~REG_SADC_STATE;
    REG_SADC_CTRL = 0x1F;
    
    REG_SADC_CFG = SADC_CFG_INIT;
    
    system_enable_irq(IRQ_SADC);
    
    REG_SADC_SAMETIME = 10;
    REG_SADC_WAITTIME = 100;
    REG_SADC_STATE &= ~REG_SADC_STATE;
    REG_SADC_CTRL = ~SADC_CTRL_PBATRDYM;
    REG_SADC_ENA = 0;
    
    mutex_init(&battery_mtx);
}

/* Interrupt handler */
void SADC(void)
{
    unsigned char state;
    unsigned char sadcstate;

    sadcstate = REG_SADC_STATE;
    state = REG_SADC_STATE & (~REG_SADC_CTRL);
    REG_SADC_STATE &= sadcstate;
    
    if(state & SADC_CTRL_PBATRDYM)
    {
        bat_val = REG_SADC_BATDAT;
        /* Battery AD IRQ */
    }
}
