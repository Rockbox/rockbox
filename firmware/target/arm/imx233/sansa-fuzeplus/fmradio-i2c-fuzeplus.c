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

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "pinctrl-imx233.h"
#include "si4700.h"

/**
 * Sansa Fuze+ fmradio uses the following pins:
 * - B0P29 as CE (active high)
 * - B1P24 as SDA
 * - B1P22 as SCL
 * - B2P27 as STC/RDS
 */

#ifdef HAVE_RDS_CAP
/* Low-level RDS Support */
static struct semaphore rds_sema;
static uint32_t rds_stack[DEFAULT_STACK_SIZE / sizeof(uint32_t)];

/* RDS GPIO interrupt handler */
static void stc_rds_callback(int bank, int pin, intptr_t user)
{
    (void) bank;
    (void) pin;
    (void) user;

    semaphore_release(&rds_sema);
}

/* Captures RDS data and processes it */
static void NORETURN_ATTR rds_thread(void)
{
    while(true)
    {
        semaphore_wait(&rds_sema, TIMEOUT_BLOCK);
        si4700_rds_process();

        /* renable callback */
        imx233_pinctrl_setup_irq(2, 27, true, true, false, &stc_rds_callback, 0);
    }
}

/* true after full radio power up, and false before powering down */
void si4700_rds_powerup(bool on)
{
    if(on)
    {
        imx233_pinctrl_acquire(2, 27, "tuner stc/rds");
        imx233_pinctrl_set_function(2, 27, PINCTRL_FUNCTION_GPIO);
        imx233_pinctrl_enable_gpio(2, 27, false);
        /* pin is set to 0 when an RDS packet has arrived */
        imx233_pinctrl_setup_irq(2, 27, true, true, false, &stc_rds_callback, 0);
    }
    else
    {
        imx233_pinctrl_setup_irq(2, 27, false, false, false, NULL, 0);
        imx233_pinctrl_release(2, 27, "tuner stc/rds");
    }
}

/* One-time RDS init at startup */
void si4700_rds_init(void)
{
    semaphore_init(&rds_sema, 1, 0);
    create_thread(rds_thread, rds_stack, sizeof(rds_stack), 0, "rds"
        IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}
#endif /* HAVE_RDS_CAP */
