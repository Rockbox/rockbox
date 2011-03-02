/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for AS3514 audio codec
 *
 * Copyright (c) 2007 Daniel Ankers
 * Copyright (c) 2007 Christian Gmeiner
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

#ifndef _ASCODEC_TARGET_H
#define _ASCODEC_TARGET_H

#ifndef SIMULATOR

#include "as3514.h"
#include "kernel.h"       /* for struct semaphore */
#include "clock-target.h" /* for AS3525_I2C_PRESCALER */
#include "system-arm.h"

/*  Charge Pump and Power management Settings  */
#define AS314_CP_DCDC3_SETTING    \
               ((0<<7) |  /* CP_SW  Auto-Switch Margin 0=200/300 1=150/255 */  \
                (0<<6) |  /* CP_on  0=Normal op 1=Chg Pump Always On  */       \
                (0<<5) |  /* LREG_CPnot  Always write 0 */                     \
                (0<<3) |  /* DCDC3p  BVDD setting 3.6/3.2/3.1/3.0  */          \
                (1<<2) |  /* LREG_off 1=Auto mode switching 0=Length Reg only*/\
                (0<<0) )  /* CVDDp  Core Voltage Setting 1.2/1.15/1.10/1.05*/

#define CVDD_1_20          0
#define CVDD_1_15          1
#define CVDD_1_10          2
#define CVDD_1_05          3

void ascodec_init(void) INIT_ATTR;

int ascodec_write(unsigned int index, unsigned int value);

int ascodec_read(unsigned int index);

int ascodec_readbytes(unsigned int index, unsigned int len, unsigned char *data);

void ascodec_lock(void);

void ascodec_unlock(void);

void ascodec_wait_adc_finished(void);

static inline void ascodec_monitor_endofch(void) {} /* already enabled */

bool ascodec_endofch(void);

bool ascodec_chg_status(void);

#if CONFIG_CPU == AS3525v2
static inline void ascodec_write_pmu(unsigned int index, unsigned int subreg,
                                     unsigned int value)
{
    /* we disable interrupts to make sure no operation happen on the i2c bus
     * between selecting the sub register and writing to it */
    int oldstatus = disable_irq_save();
    ascodec_write(AS3543_PMU_ENABLE, 8|subreg);
    ascodec_write(index, value);
    restore_irq(oldstatus);
}

static inline int ascodec_read_pmu(unsigned int index, unsigned int subreg)
{
    /* we disable interrupts to make sure no operation happen on the i2c bus
     * between selecting the sub register and reading it */
    int oldstatus = disable_irq_save();
    ascodec_write(AS3543_PMU_ENABLE, subreg);
    int ret = ascodec_read(index);
    restore_irq(oldstatus);
    return ret;
}
#endif /* CONFIG_CPU == AS3525v2 */

static inline void ascodec_write_charger(int value)
{
#if CONFIG_CPU == AS3525
    ascodec_write(AS3514_CHARGER, value);
#else
    ascodec_write_pmu(AS3543_CHARGER, 1, value);
#endif
}

static inline int ascodec_read_charger(void)
{
#if CONFIG_CPU == AS3525
    return ascodec_read(AS3514_CHARGER);
#else
    return ascodec_read_pmu(AS3543_CHARGER, 1);
#endif
}

#endif /* !SIMULATOR */

#endif /* !_ASCODEC_TARGET_H */
