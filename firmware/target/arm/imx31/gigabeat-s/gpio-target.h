/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
 *
 * Gigabeat S GPIO interrupt event descriptions header
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
#ifndef GPIO_TARGET_H
#define GPIO_TARGET_H

/* Gigabeat S definitions for static GPIO event registration */
#include "gpio-imx31.h"

#ifdef DEFINE_GPIO_VECTOR_TABLE

GPIO_EVENT_VECTOR_CB(GPIO1_31);
#if CONFIG_TUNER
GPIO_EVENT_VECTOR_CB(GPIO1_27);
#endif

GPIO_VECTOR_TBL_START()
    /* mc13783 keeps the PRIINT high (no low pulse) if other unmasked
     * interrupts become active when clearing them or if a source being
     * cleared becomes active at that time. Edge-detection will not get
     * a rising edge in that case so use high-level sense. */
    GPIO_EVENT_VECTOR(GPIO1_31, GPIO_SENSE_HIGH_LEVEL)
#if CONFIG_TUNER
    /* Generates a 5ms low pulse on the line - detect the falling edge */
    GPIO_EVENT_VECTOR(GPIO1_27, GPIO_SENSE_FALLING)
#endif /* CONFIG_TUNER */
GPIO_VECTOR_TBL_END()

#define GPIO1_INT_PRIO      INT_PRIO_DEFAULT

#endif /* DEFINE_GPIO_VECTOR_TABLE */

#define INT_MC13783         GPIO1_31_EVENT_CB
#define MC13783_EVENT_ID    GPIO1_31_ID

#if CONFIG_TUNER
#define INT_SI4700_RDS      GPIO1_27_EVENT_CB
#define SI4700_EVENT_ID     GPIO1_27_ID
#endif /* CONFIG_TUNER */

#endif /* GPIO_TARGET_H */
