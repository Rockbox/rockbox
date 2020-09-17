/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Roman Stolyarov
 * Copyright (C) 2020 by William Wilgus
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
#include "cpu.h"
#include "button.h"
#include "button-target.h"
#include "powermgmt.h"
#include "kernel.h"
#include "backlight.h"
#include "logf.h"
#include "adc.h"

#define BATT_WAIT_TIME HZ * 30

#define PIN_BTN_POWER (32*0+30)
#define PIN_BTN_HOLD  (32*1+15)

#define PIN_KEY_INT   (32*4+13)
#define KEY_INT_IRQ   GPIO141

#define PIN_COM_BOP (32*3+17) /* common pin for Back Option Play */

#define PIN_CHARGE_CON (32*1+7)

#define PIN_PH_DECT   (32*1+11)
#define IRQ_PH_DECT   GPIO_IRQ(PIN_PH_DECT)
#define GPIO_PH_DECT  GPIO43

#define PIN_LO_DECT   (32*1+12)
#define IRQ_LO_DECT   GPIO_IRQ(PIN_LO_DECT)
#define GPIO_LO_DECT  GPIO44

#define KEY_IS_DOWN(pin) (__gpio_get_pin(pin) == 0)

#define ADC_MASK  0x0FFF

static volatile unsigned short bat_val, key_val;
static volatile int btn_last = BUTTON_NONE;
static volatile long btn_last_tick;

bool headphones_inserted(void)
{
    return (__gpio_get_pin(PIN_PH_DECT) != 0);
}

bool lineout_inserted(void)
{
    /* We want to prevent LO being "enabled" if HP is attached
       to avoid potential eardrum damage */
    return (__gpio_get_pin(PIN_LO_DECT) == 0) && !headphones_inserted();
}

void button_init_device(void)
{
    key_val = ADC_MASK;

    __gpio_as_input(PIN_BTN_POWER);
    __gpio_as_input(PIN_BTN_HOLD);

    __gpio_disable_pull(PIN_BTN_POWER);
    __gpio_disable_pull(PIN_BTN_HOLD);

    __gpio_as_irq_fall_edge(PIN_KEY_INT);
    __gpio_disable_pull(PIN_KEY_INT);
    system_enable_irq(GPIO_IRQ(PIN_KEY_INT));

    __gpio_set_pin(PIN_CHARGE_CON); /* 0.7 A */
    __gpio_as_output(PIN_CHARGE_CON);

    __gpio_as_input(PIN_PH_DECT);
    __gpio_enable_pull(PIN_PH_DECT);
    /*__gpio_disable_pull(PIN_PH_DECT); // Spurious Detections */

    __gpio_as_input(PIN_LO_DECT);
    __gpio_enable_pull(PIN_LO_DECT);
    /*__gpio_disable_pull(PIN_LO_DECT); // Spurious Detections */
}

bool button_hold(void)
{
    bool hold_button = __gpio_get_pin(PIN_BTN_HOLD) == 1;
#ifndef BOOTLOADER
    static bool hold_button_old = false;
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif
    return hold_button;
}

/* NOTE:  Due to how this is wired, button combinations are not allowed
 *  unless one of the two buttons is the POWER
 *
 * Note --Update 2020
 *  by toggling BOP common I was able to remove BACK, OPTION, PLAY from the
 * loop selectively and test which keys were pressed but this took two adc rounds
 * and proved to be minimally useful for the added overhead
 *
 * NOW multiple button presses are emulated but button priority needs to be taken
 * into consideration; higher priority keys 'overide' the lower priority keys
 * VOLUP[7] VOLDN[6] PREV[5] NEXT[4] PLAY[3] OPTION[2] HOME[1]
 */
int button_read_device(void)
{
    unsigned short key;
    int btn = BUTTON_NONE;
    int btn_pwr = BUTTON_NONE;

    if (button_hold())
        return BUTTON_NONE;

    if (KEY_IS_DOWN(PIN_BTN_POWER))
        btn_pwr = BUTTON_POWER;

    if (!KEY_IS_DOWN(PIN_KEY_INT))
    {
        __intc_mask_irq(IRQ_SADC);
        REG_SADC_ADENA &= ~ADENA_AUXEN;
        return btn_pwr;
    }

    key = (key_val & ADC_MASK);

    /* Don't initiate a new request if we have one pending */
    if(!(REG_SADC_ADENA & (ADENA_AUXEN)))
    {
        REG_SADC_ADENA |= ADENA_AUXEN;
    }

    if (key < 261)
        btn = BUTTON_VOL_UP;
    else
    if (key < 653)
        btn = BUTTON_VOL_DOWN;
    else
    if (key < 1101)
        btn = BUTTON_PREV;
    else
    if (key < 1498)
        btn = BUTTON_NEXT;
    else
    if (key < 1839)
        btn = BUTTON_PLAY;
    else
    if (key < 2213)
        btn = BUTTON_OPTION;
    else
    if (key < 2600)
        btn = BUTTON_HOME;

    if (btn_last == BUTTON_NONE && TIME_AFTER(current_tick, btn_last_tick + HZ/20))
        btn_last = btn;

    if (btn_pwr != BUTTON_NONE)
        btn  |= BUTTON_PWRALT;

    return btn | btn_last;
}

/* called on button press interrupt */
void KEY_INT_IRQ(void)
{
    btn_last = BUTTON_NONE;
    key_val = ADC_MASK;
    __intc_unmask_irq(IRQ_SADC);
     REG_SADC_ADENA |= ADENA_AUXEN;
    btn_last_tick = current_tick;
}

/* Notes on batteries

   xDuoo shipped two types of batteries:

   First is the 2000mAh battery shipped in newer units
   Second is the 1500mAh battery shipped in older units

*/

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* 5% */
    3414, 3634
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    /* 0% */
    3307, 3307
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3307, 3459, 3530, 3575, 3608, 3648, 3723, 3819, 3918, 4022, 4162 },
    { 3300, 3652, 3704, 3730, 3753, 3786, 3836, 3906, 3973, 4061, 4160 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
//    { 3300, 3652, 3704, 3730, 3753, 3786, 3836, 3906, 3973, 4061, 4160 };
    { 3444, 3827, 3893, 3909, 3931, 4001, 4067, 4150, 4206, 4207, 4208 };
#endif /* CONFIG_CHARGING */

/* VBAT = (BDATA/1024) * 2.5V */
#define BATTERY_SCALE_FACTOR 2460

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    static long last_tick = 0;

    if (TIME_AFTER(current_tick, last_tick) && REG_SADC_ADENA == 0)
    {
        last_tick = current_tick;
        __intc_mask_irq(IRQ_SADC);
        REG_SADC_ADENA |= ADENA_VBATEN;
        /* poll for value from our context instead of ISR */
        while (!(REG_SADC_ADSTATE & ADCTRL_VRDYM) &&
               TIME_BEFORE(current_tick, last_tick + HZ / 20))
            ;;
        if ((REG_SADC_ADSTATE & ADCTRL_VRDYM))
        {
            last_tick += BATT_WAIT_TIME;
            bat_val = REG_SADC_ADVDAT;
            REG_SADC_ADSTATE &= ADCTRL_VRDYM; /* clear ready bit by writing 1*/
        }
        __intc_unmask_irq(IRQ_SADC);
    }

    return (bat_val*BATTERY_SCALE_FACTOR)>>10;
}

/* 12MHz XTAL
    /61    = 196 MHz base clock  (max is 200, err on the side of safety)
     /(1+1) = 98.4KHz "us_clk (ie ~10us/tick)
     /(199+1) = 983.6KHz "ms_clk" (ie ~1ms/tick)
*/
void adc_init(void)
{
    bat_val = ADC_MASK;
    /* don't re-init*/
    if (!(REG_CPM_CLKGR0 & CLKGR0_SADC) && !(REG_SADC_ADENA & ADENA_POWER))
    {
        system_enable_irq(IRQ_SADC);
        return;
    }

    __cpm_start_sadc();
    mdelay(20);
    REG_SADC_ADENA = 0; /* Power Up */
    mdelay(70);
    REG_SADC_ADSTATE = 0;
    REG_SADC_ADCTRL = ADCTRL_MASK_ALL - ADCTRL_ARDYM - ADCTRL_VRDYM;
    REG_SADC_ADCFG = ADCFG_VBAT_SEL | ADCFG_CMD_AUX(1);  /* VBAT_SEL is undocumented but required! */
    REG_SADC_ADCLK = (199 << 16) | (1 << 8) | 61;
    system_enable_irq(IRQ_SADC);
    REG_SADC_ADENA |= ADENA_AUXEN | ADENA_VBATEN;
}

void adc_close(void)
{
    REG_SADC_ADENA = ADENA_POWER; /* Power Down */
    __intc_mask_irq(IRQ_SADC);
    mdelay(20);
    __cpm_stop_sadc();
}

/* Interrupt handler */
void SADC(void)
{
    unsigned char state;
    unsigned char sadcstate;
    sadcstate = REG_SADC_ADSTATE;
    state = REG_SADC_ADSTATE & (~REG_SADC_ADCTRL);
    REG_SADC_ADSTATE &= sadcstate;

    if(state & ADCTRL_ARDYM)
    {
        key_val = REG_SADC_ADADAT;
    }
    else if(UNLIKELY(state & ADCTRL_VRDYM))
    {
        bat_val = REG_SADC_ADVDAT;
    }
}
