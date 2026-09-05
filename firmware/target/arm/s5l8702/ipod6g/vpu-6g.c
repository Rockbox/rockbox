/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * S5L8702 VPU-B IRQ 35 completion handler.
 *
 * IRQ 35 is VIC1 bit 3 and is asserted when VPU-B completes a decode.
 * The handler captures the completion status, acknowledges the edge, and
 * releases a semaphore for the decoder driver.
 *
 * Copyright (C) 2025-2026 David Cormier
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
#include "s5l87xx.h"

#define VPU_IRQ_BIT (1u << 3)

static struct semaphore vpu_complete;
static volatile uint32_t vpu_last_status1;

void vpu_irq_init(void)
{
    semaphore_init(&vpu_complete, 1, 0);
    vpu_last_status1 = 0;
    VIC1INTENCLEAR = VPU_IRQ_BIT;
    VIC1EDGE0 |= VPU_IRQ_BIT;
    VIC1EDGE1 = VPU_IRQ_BIT;
}

void vpu_irq_arm(void)
{
    semaphore_wait(&vpu_complete, 0);
    VIC1EDGE1 = VPU_IRQ_BIT;
    VIC1INTENABLE = VPU_IRQ_BIT;
}

int vpu_irq_wait(int timeout_ticks)
{
    int ret = semaphore_wait(&vpu_complete, timeout_ticks);
    VIC1INTENCLEAR = VPU_IRQ_BIT;
    return ret;
}

uint32_t vpu_irq_status1(void)
{
    return vpu_last_status1;
}

void ICODE_ATTR INT_IRQ35(void)
{
    vpu_last_status1 = VPU_STATUS1;
    VIC1EDGE1 = VPU_IRQ_BIT;
    VIC1INTENCLEAR = VPU_IRQ_BIT;
    semaphore_release(&vpu_complete);
}
