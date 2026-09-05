/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * S5L8702 VPU-B IRQ completion for H.264 hardware decoder.
 *
 * VPU-B asserts IRQ 35 (VIC1 bit 3, edge-triggered) on decode completion.
 * These functions provide semaphore-based synchronous wait, replacing
 * the sleep(200ms) polling approach.
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

#ifndef VPU_6G_H
#define VPU_6G_H

#include <stdint.h>

/* Initialize VPU-B IRQ 35: setup semaphore, configure edge trigger.
 * Call once before first decode. */
void vpu_irq_init(void);

/* Arm for next decode: drain stale semaphore signal, clear pending
 * edge, enable IRQ 35. Call BEFORE triggering VPU decode. */
void vpu_irq_arm(void);

/* Wait for VPU decode completion. Blocks until ISR fires or timeout.
 * Disables IRQ 35 after return. Returns OBJ_WAIT_TIMEDOUT on timeout,
 * OBJ_WAIT_SUCCEEDED on success. */
int vpu_irq_wait(int timeout_ticks);

/* Get STATUS1 value captured by ISR during last completion. */
uint32_t vpu_irq_status1(void);

#endif /* VPU_6G_H */
