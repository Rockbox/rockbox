/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2010 by Rafaël Carré
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
#include "cpu.h"

#define CACHE_NONE  0
#define CACHE_ALL   0x0C
#define UNCACHED_ADDR(a) (a + 0x10000000)

#if defined(SANSA_CLIP) || defined(SANSA_M200V4) || defined(SANSA_C200V2)
/* 16 bits external bus, low power SDRAM, 16 Mbits = 2 Mbytes */
#define MEMORY_MODEL 0x21

#elif defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_CLIPV2) \
    || defined(SANSA_CLIPPLUS) || defined(SANSA_FUZEV2)
/* 16 bits external bus, high performance SDRAM, 64 Mbits = 8 Mbytes */
#define MEMORY_MODEL 0x5

#else
#error "The external memory in your player is unknown"
#endif

    .global    memory_init
    .text

memory_init:

#ifdef BOOTLOADER

    ldr     r2, =0xC80F0014 @ CGU_PERI
    ldr     r1, [r2]
    orr     r1, r1, #(CGU_EXTMEM_CLOCK_ENABLE|CGU_EXTMEMIF_CLOCK_ENABLE)
    str     r1, [r2]

    ldr     r3, =0xC6030000 @ MPMC_BASE

    mov     r2, #1          @ enable MPMC
    str     r2, [r3]        @ MPMC_CONTROL
    ldr     r2, =0x183      @ SDRAM NOP, all clocks high
    str     r2, [r3, #0x20] @ MPMC_DYNAMIC_CONTROL
    ldr     r2, =0x103      @ SDRAM PALL, all clocks high
    str     r2, [r3, #0x20] @ MPMC_DYNAMIC_CONTROL
    ldr     r1, =0x138      @ 0x138 * 16 HCLK ticks between SDRAM refresh cycles
    str     r1, [r3, #0x24] @ MPMC_DYNAMIC_REFRESH
    mov     r2, #0          @ little endian, HCLK:MPMCCLKOUT[3:0] ratio = 1:1
    str     r2, [r3, #8]    @ MPMC_CONFIG

    ldr     r2, [r3, #0xfe8]    @ MPMC_PERIPH_ID2
    tst     r2, #0xf0
    movne   r2, #1              @ command delayed, clock out not delayed
    strne   r2, [r3, #0x28]     @ MPMC_DYNAMIC_READ_CONFIG

    mov     r1, #2
    mov     r0, #5
    mov     ip, #4
    mov     r2, #0
    str     r1, [r3, #0x30]     @ tRP
    str     ip, [r3, #0x34]     @ tRAS
    str     r0, [r3, #0x38]     @ tSREX
    str     r2, [r3, #0x3c]     @ tAPR
    str     ip, [r3, #0x40]     @ tDAL
    str     r1, [r3, #0x44]     @ tWR
    str     r0, [r3, #0x48]     @ tRC
    str     r0, [r3, #0x4c]     @ tRFC
    str     r0, [r3, #0x50]     @ tXSR
    str     r1, [r3, #0x54]     @ tRRD
    str     r1, [r3, #0x58]     @ tMRD
    mov     ip, #(MEMORY_MODEL << 7)
    str     ip, [r3, #0x100]    @ MPMC_DYNAMIC_CONFIG_CONFIG_0
    orr     r1, r1, #(2<<8)     @ CAS & RAS latency = 2 clock cycle
    str     r1, [r3, #0x104]    @ MPMC_DYNAMIC_CONFIG_RASCAS_0

    str     r2, [r3, #0x120]    @ MPMC_DYNAMIC_CONFIG_CONFIG_1
    str     r2, [r3, #0x124]    @ MPMC_DYNAMIC_CONFIG_RASCAS_1
    str     r2, [r3, #0x140]    @ MPMC_DYNAMIC_CONFIG_CONFIG_2
    str     r2, [r3, #0x144]    @ MPMC_DYNAMIC_CONFIG_RASCAS_2
    str     r2, [r3, #0x160]    @ MPMC_DYNAMIC_CONFIG_CONFIG_3
    str     r2, [r3, #0x164]    @ MPMC_DYNAMIC_CONFIG_RASCAS_3

    mov     r1, #0x82           @ SDRAM MODE, MPMCCLKOUT runs continuously
    str     r1, [r3, #0x20]     @ MPMC_DYNAMIC_CONTROL

    ldr     r1, =DRAM_ORIG+(0x2300*MEMORYSIZE)
    ldr     r1, [r1]

    str     r2, [r3, #0x20]     @ MPMC_DYNAMIC_CONTROL= SDRAM NORMAL,
                                @ MPMCCLKOUT stopped when SDRAM is idle

    ldr     r2, [r3, #0x100]    @ MPMC_DYNAMIC_CONFIG_0
    orr     r2, r2, #(1<<19)    @ buffer enable
    str     r2, [r3, #0x100]

#endif /* BOOTLOADER */

    @ XXX: to avoid using the stack, we rely on the fact that:
    @   - ttb_init
    @   - map_section
    @   - enable_mmu
    @ do not modify ip (r12)
    mov     ip, lr

    /* Setup MMU */

    bl      ttb_init

    mov     r0, #0                          @ physical address
    mov     r1, #0                          @ virtual address
    mov     r2, #0x1000                     @ size (all memory)
    mov     r3, #CACHE_NONE
    bl      map_section

    mov     r0, #0                          @ physical address
    ldr     r1, =IRAM_ORIG                  @ virtual address
    mov     r2, #1                          @ size : 1MB
    mov     r3, #CACHE_ALL
    bl      map_section

    mov     r0, #0                          @ physical address
    ldr     r1, =UNCACHED_ADDR(IRAM_ORIG)   @ virtual address
    mov     r2, #1                          @ size : 1MB
    mov     r3, #CACHE_NONE
    bl      map_section

    mov     r0, #0x30000000                 @ physical address
    mov     r1, #DRAM_ORIG                  @ virtual address
    mov     r2, #MEMORYSIZE                 @ size
    mov     r3, #CACHE_ALL
    bl      map_section

    mov     r0, #0x30000000                 @ physical address
    mov     r1, #UNCACHED_ADDR(DRAM_ORIG)   @ virtual address
    mov     r2, #MEMORYSIZE                 @ size
    mov     r3, #CACHE_NONE
    bl      map_section

    /* map 1st mbyte of RAM at 0x0 to have exception vectors available */
#ifdef BOOTLOADER
    mov     r0, #0x81000000                 @ physical address
#else
    mov     r0, #0x30000000                 @ physical address
#endif
    mov     r1, #0                          @ virtual address
    mov     r2, #1                          @ size
    mov     r3, #CACHE_ALL
    bl      map_section

    bl      enable_mmu

    bx      ip
