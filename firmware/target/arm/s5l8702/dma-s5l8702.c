/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Cástor Muñoz
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

#include <config.h>

#include "s5l8702.h"
#include "pl080.h"
#include "dma-s5l8702.h"

/* s5l8702 PL080 controllers configuration */
struct dmac s5l8702_dmac0 = {
    .baddr = S5L8702_DMAC0_BASE,
    .m1 = DMACCONFIG_M_LITTLE_ENDIAN,
    .m2 = DMACCONFIG_M_LITTLE_ENDIAN,
};

struct dmac s5l8702_dmac1 = {
    .baddr = S5L8702_DMAC1_BASE,
    .m1 = DMACCONFIG_M_LITTLE_ENDIAN,
    .m2 = DMACCONFIG_M_LITTLE_ENDIAN,
};

void ICODE_ATTR INT_DMAC0(void)
{
    dmac_callback(&s5l8702_dmac0);
}

void ICODE_ATTR INT_DMAC1(void)
{
    dmac_callback(&s5l8702_dmac1);
}

void dma_init_ctrl(struct dmac* dmac, int irq, int clockgate, int onoff)
{
    /* init DMAC */
    VIC0INTENCLEAR = (1 << irq);        /* disable interrupts */
    PWRCON(0) &= ~(1 << clockgate);     /* unmask clock gate */
    dmac_open(dmac);                    /* init/reset controller */

    if (onoff)
        VIC0INTENABLE = (1 << irq);     /* enable interrupts */
    else
        PWRCON(0) |= (1 << clockgate);  /* mask clockgate */
}

void dma_init(void)
{
    dma_init_ctrl(&s5l8702_dmac0, IRQ_DMAC0, CLOCKGATE_DMAC0, 1);
    dma_init_ctrl(&s5l8702_dmac1, IRQ_DMAC1, CLOCKGATE_DMAC1, 0);
}
