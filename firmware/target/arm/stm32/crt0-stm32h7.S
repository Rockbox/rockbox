/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "bootdata.h"
#include "stm32h7/pwr.h"

    .syntax unified
    .text

#if defined(HAVE_BOOTDATA) && !defined(BOOTLOADER)
    .section .bootdata

put_boot_data_here
#endif

    .section .init.text,"ax",%progbits

    .global reset_handler
    .type reset_handler, function
reset_handler:
    /*
     * After power-up the CPU will be in limited Run* mode and many things
     * won't work; we need to program the power supply configuration first
     * before doing anything else. Below the internal LDO is configured as
     * the power supply.
     */
    ldr     r0, =STA_PWR_CR3
    ldr     r1, [r0]                    @ Read PWR_CR3
    orr     r1, #BM_PWR_CR3_LDOEN       @ Set LDOEN=1
    bic     r1, #BM_PWR_CR3_BYPASS      @ Set BYPASS=0
    str     r1, [r0]                    @ Write updated value to PWR_CR3

    /* Wait for PSR_CSR1.ACTVOSRDY, which indicates we exited Run* mode */
    ldr     r0, =STA_PWR_CSR1
1:
    ldr     r1, [r0]                    @ Read PWR_CSR1
    ands    r1, #BM_PWR_CSR1_ACTVOSRDY
    beq     1b                          @ Loop back if ACTVOSRDY==0

    /* Now jump to the common startup code */
    b       crt0_start


    .global start
    .type start, function
crt0_start:
    /* Zero out BSS */
    ldr     a1, =_bssbegin
    ldr     a2, =_bssend
    mov     a3, #0
    bl      crt0_area_clear

    /* Copy data section */
    ldr     a1, =_databegin
    ldr     a2, =_dataend
    ldr     a3, =_datacopy
    bl      crt0_area_copy

    /* Clear the main thread stack */
    ldr     a1, =stackbegin
    ldr     a2, =stackend
    ldr     a3, =0xdeadbeef
    bl      crt0_area_clear

    /* Clear the IRQ stack */
    ldr     a1, =irqstackbegin
    ldr     a2, =irqstackend
    ldr     a3, =0xdeadbeef
    bl      crt0_area_clear

    /* Use MSP for IRQ handlers */
    ldr     r0, =irqstackend
    msr     msp, r0

    /* Use PSP for application code */
    ldr     r0, =stackend
    msr     psp, r0

    /* Enter thread mode; now sp points to PSP */
    mrs     r2, control
    orr     r2, #2
    msr     control, r2

    /* Jump to main */
    b       main


    .local crt0_area_copy
    .type crt0_area_copy, function
/* crt0_area_copy(uint32_t *dst, uint32_t *dst_end, const void *src) */
crt0_area_copy:
    cmp     a2, a1
    ldrhi   v1, [a3], #4
    strhi   v1, [a1], #4
    bhi     crt0_area_copy
    bx      lr


    .local crt0_area_clear
    .type crt0_area_clear, function
/* crt0_area_clear(const uint32_t *dst, const uint32_t *end, uint32_t value) */
crt0_area_clear:
    cmp     a2, a1
    strhi   a3, [a1], #4
    bhi     crt0_area_clear
    bx      lr
