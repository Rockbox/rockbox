/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Andrew Ryabinin
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
#include "kernel.h"
#include "i2c-rk27xx.h"
#include "pca9555.h"

static struct semaphore pca9555_sem;
static char pca9555_thread_stack[DEFAULT_STACK_SIZE];
volatile unsigned short pca9555_in_ports;

void INT_GPIO1(void)
{
    if (GPIO1_ISR & (1<<10)) { /* GPIO F2 pin */
        GPIO_IEF &= ~(1<<2);
        GPIO_ICF |= (1<<2);
        semaphore_release(&pca9555_sem);
    }
}

static void pca9555_read_thread(void)
{
    while(1) {
        if ((GPIO_PFDR&(1<<2)) == 0) {
            pca9555_in_ports = pca9555_read_input();
        } else {
            pca9555_in_ports = (1<<15)|(1<<11); /* restore defaults */
        }
        sleep(HZ/20);
        GPIO_IEF |= (1<<2);
        semaphore_wait(&pca9555_sem, TIMEOUT_BLOCK);
    }
}

static void pca9555_ports_init(void)
{
    unsigned short data = 0;

    data = 0xf800; /* port0 - pins 0-7 output, port1 - pins 0-2 output, pins 3-7 input */
    pca9555_write_config(data, 0xffff);

    /*
     *   IO0-7    IO0-6   IO0-5    IO0_4    IO0_3    IO0_2     IO0_1      IO0_0
     * USB_SEL   KP_LED     POP   DAC_EN   AMP_EN   SPI_CS   SPI_CLK   SPI_DATA
     *   1:MSD    1:OFF       1        0        0        1         1          1
     */
    data = ((1<<7)|(1<<6)|(1<<5)|(0<<4)|(0<<3)|(1<<2)|(1<<1)|(1<<0));

    /*
     *  IO1-2    IO1_1       IO1_0
     * CHG_EN   DAC_EN   DF1704_CS
     *      1        0           1
     */
    data |= ((1<<10)|(0<<9)|(1<<8));
    pca9555_write_output(data, 0xffff);
    pca9555_in_ports = pca9555_read_input();
}

void pca9555_target_init(void)
{
    GPIO_PFCON &= ~(1<<2); /* PF2 for PCA9555 input INT */

    pca9555_ports_init();
    semaphore_init(&pca9555_sem, 1, 0);

    INTC_IMR |= IRQ_ARM_GPIO1;
    INTC_IECR |= IRQ_ARM_GPIO1;
    GPIO_ISF |= (1<<2);

    create_thread(pca9555_read_thread,
                  pca9555_thread_stack,
                  sizeof(pca9555_thread_stack),
                  0,
                  "pca9555_read" IF_PRIO(, PRIORITY_SYSTEM)
                  IF_COP(, CPU));
}
