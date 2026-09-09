/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#include "button.h"
#include "touchscreen.h"
#include "cst8xx.h"
#include "kernel.h"
#include "system.h"
#include "adc.h"
#include "gpio-x1600.h"
#include "irq-x1600.h"
#include "i2c-ingenic.h"
#include <stdbool.h>

/* POWER is active-low PC31; NEXT is active-high PC28. PLAY and volume keys
 * use SADC AUX0. NEXT also selects BootROM USB mode when USB is attached.
 * There is no physical PREV key. */

/* SADC key ladder */

/* Require two consecutive matching ladder samples. The vendor uses a
 * longer, 100 ms debounce. */
#define ADC_KEY_AGREE_COUNT 2

static int adc_key_last = 0;
static int adc_key_agree = 0;
static int adc_key_stable = 0;

static inline bool adc_near(int mv, int target)
{
    return mv >= (target - ADC_KEY_WINDOW_MV) &&
           mv <= (target + ADC_KEY_WINDOW_MV);
}

static int adc_key_decode(int mv)
{
    /* The idle ladder voltage is nominally 3300 mV. */
    if(adc_near(mv, ADC_KEY_IDLE_MV))
        return 0;

    if(adc_near(mv, ADC_KEY_PLAY_MV))
        return BUTTON_PLAY;
    if(adc_near(mv, ADC_KEY_VOLDOWN_MV))
        return BUTTON_VOL_DOWN;
    if(adc_near(mv, ADC_KEY_VOLUP_MV))
        return BUTTON_VOL_UP;

    /* Between windows: mid-press or noise, hold the previous state. */
    return -1;
}

static int adc_key_read(void)
{
    int key = adc_key_decode(adc_read_mv(ADC_BUTTONS));

    if(key < 0)
        return adc_key_stable;

    if(key == adc_key_last) {
        if(adc_key_agree < ADC_KEY_AGREE_COUNT)
            ++adc_key_agree;
    } else {
        adc_key_last = key;
        adc_key_agree = 1;
    }

    if(adc_key_agree >= ADC_KEY_AGREE_COUNT)
        adc_key_stable = key;

    return adc_key_stable;
}

/* Rockbox interface */

void button_init_device(void)
{
    i2c_ingenic_set_freq(CST8XX_BUS, I2C_FREQ_400K);
    cst8xx_init();

    /* ALDO2 is enabled by power_init(). The 5 ms reset pulse and 50 ms
     * startup delay have not been verified for this controller. */
    gpio_set_level(GPIO_CST8XX_RESET, 0);
    mdelay(5);
    gpio_set_level(GPIO_CST8XX_RESET, 1);
    mdelay(50);

    /* Touch data is IRQ-driven, which is the primary defence against
     * reading a half-updated register block. */
    system_set_irq_handler(GPIO_TO_IRQ(GPIO_CST8XX_INTERRUPT),
                           cst8xx_irq_handler);
    gpio_set_function(GPIO_CST8XX_INTERRUPT, GPIOF_IRQ_EDGE(0));
    gpio_enable_irq(GPIO_CST8XX_INTERRUPT);
}

int button_read_device(int* data)
{
    int r = 0;

    uint32_t c = REG_GPIO_PIN(GPIO_C);

    /* POWER is active LOW (PC31 is the SoC's WKUP_ pad). */
    if((c & (1 << 31)) == 0)
        r |= BUTTON_POWER;

    /* NEXT is active HIGH (PC28, shared with the BOOT_SEL1 strap). */
    if(c & (1 << 28))
        r |= BUTTON_NEXT;

    /* PLAY / VOL+ / VOL- come off the SADC ladder. */
    r |= adc_key_read();

    /* Convert the single touch point to the configured touchscreen mode. */
    const struct cst8xx_point* point = &cst8xx_state.points[0];
    int* out = (touchscreen_get_mode() == TOUCHSCREEN_POINT) ? data : NULL;
    /* Supply the last coordinates on release, as on the Shanling Q1. */
    int touch = touchscreen_to_pixels(point->pos_x, point->pos_y, out);
    if(cst8xx_state.nr_points > 0 &&
       (point->event == CST8XX_EVT_DOWN ||
        point->event == CST8XX_EVT_CONTACT))
        r |= touch;

    return r;
}

void touchscreen_enable_device(bool en)
{
    /* Hold reset to disable touch interrupts. A controller sleep command
     * has not been established. */
    if(en) {
        gpio_set_level(GPIO_CST8XX_RESET, 1);
        mdelay(50);
        cst8xx_enable(true);
        gpio_enable_irq(GPIO_CST8XX_INTERRUPT);
    } else {
        gpio_disable_irq(GPIO_CST8XX_INTERRUPT);
        cst8xx_enable(false);
        gpio_set_level(GPIO_CST8XX_RESET, 0);
    }
}

bool headphones_inserted(void)
{
    /* Vendor headset-detect range on AUX1: 2800-3300 mV. The generic
     * button driver debounces insertion and removal. */
    int mv = adc_read_mv(ADC_HP_DETECT);
    return mv >= ADC_HP_MIN_MV && mv <= ADC_HP_MAX_MV;
}
