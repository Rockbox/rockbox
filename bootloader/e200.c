/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "panic.h"
#include "power.h"
#include "file.h"

static inline void blink(void)
{
    volatile unsigned int* ptr;
    int i;
    ptr = (volatile unsigned int*)0x70000020;

    *ptr &= ~(1 << 13);
    for(i = 0; i < 0xfffff; i++)
    {
    }
    *ptr |= (1 << 13);
    for(i = 0; i < 0xfffff; i++)
    {
    }
}

static inline void slow_blink(void)
{
    volatile unsigned int* ptr;
    int i;
    ptr = (volatile unsigned int*)0x70000020;

    *ptr &= ~(1 << 13);
    for(i = 0; i < (0xfffff); i++)
    {
    }
    for(i = 0; i < (0xfffff); i++)
    {
    }
    for(i = 0; i < (0xfffff); i++)
    {
    }

    *ptr |= (1 << 13);
    for(i = 0; i < (0xfffff); i++)
    {
    }
    for(i = 0; i < (0xfffff); i++)
    {
    }
    for(i = 0; i < (0xfffff); i++)
    {
    }
}

static inline unsigned long get_pc(void)
{
    unsigned long pc;
    asm volatile (
        "mov %0, pc\n"
        : "=r"(pc)
    );
    return pc;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */

void reset_poweroff_timer(void)
{
}

int dbg_ports(void)
{
    unsigned int gpio_a, gpio_b, gpio_c, gpio_d;
    unsigned int gpio_e, gpio_f, gpio_g, gpio_h;
    unsigned int gpio_i, gpio_j, gpio_k, gpio_l;

    char buf[128];
    int line;

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        gpio_a = GPIOA_INPUT_VAL;
        gpio_b = GPIOB_INPUT_VAL;
        gpio_c = GPIOC_INPUT_VAL;

        gpio_g = GPIOG_INPUT_VAL;
        gpio_h = GPIOH_INPUT_VAL;
        gpio_i = GPIOI_INPUT_VAL;

        line = 0;
        snprintf(buf, sizeof(buf), "GPIO_A: %02x GPIO_G: %02x", gpio_a, gpio_g);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_B: %02x GPIO_H: %02x", gpio_b, gpio_h);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_C: %02x GPIO_I: %02x", gpio_c, gpio_i);
        lcd_puts(0, line++, buf);
        line++;

        gpio_d = GPIOD_INPUT_VAL;
        gpio_e = GPIOE_INPUT_VAL;
        gpio_f = GPIOF_INPUT_VAL;

        gpio_j = GPIOJ_INPUT_VAL;
        gpio_k = GPIOK_INPUT_VAL;
        gpio_l = GPIOL_INPUT_VAL;

        snprintf(buf, sizeof(buf), "GPIO_D: %02x GPIO_J: %02x", gpio_d, gpio_j);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_E: %02x GPIO_K: %02x", gpio_e, gpio_k);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIO_F: %02x GPIO_L: %02x", gpio_f, gpio_l);
        lcd_puts(0, line++, buf);
        line++;
        snprintf(buf, sizeof(buf), "ADC_1: %02x", adc_read(ADC_0));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_2: %02x", adc_read(ADC_1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_3: %02x", adc_read(ADC_2));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "ADC_4: %02x", adc_read(ADC_3));
        lcd_puts(0, line++, buf);
        lcd_update();
    }
    return 0;
}

void mpeg_stop(void)
{
}

void usb_acknowledge(void)
{
}

void usb_wait_for_disconnect(void)
{
}

void sys_poweroff(void)
{
}

void system_reboot(void)
{

}

void main(void)
{
    kernel_init();
    adc_init();
    lcd_init_device();

    dbg_ports();
}

