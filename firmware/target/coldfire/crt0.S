/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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

    .section .startup,"ax",@progbits

    .global    start
start:

#if defined(BOOTLOADER) && defined(HAVE_DUALBOOT) \
    && (defined(IAUDIO_X5) || defined(IAUDIO_M5))

    /* 8 byte dualboot signature */
    bra.b   1f           /* 0x6006 */
    .short  0x4442      /* DB */
#if defined(IAUDIO_X5)
    .long   0x69617835  /* iax5 */
#elif defined(IAUDIO_M5)
    .long   0x69616d35  /* iam5 */
#else
#error Dualboot signature not defined
#endif
1:
    /* As the control registers are write-only, we're relying on MBAR2 being */
    /* set up correctly by the preloader for button check */
    /* Only use scratch regs until we're sure that we will boot rockbox */
    lea     MBAR2, %a1
    move.l  (%a1), %a0        /* store GPIO_READ result for button check in main() */

    tst.b 0x30000000 /* preloader sets this to 1
                        if woke up due to RTC alarm */
    bne .loadrockbox

    /* Wait ~3 seconds for ON-button release. We need roughly 300ns per
       iteration, so we check 10000000 times to reach the desired delay */
    move.l  #10000000, %d0
.on_button_test:
    move.l  (%a1), %d1        /* GPIO_READ */
    and.l   #0x06000000, %d1  /* Check main (bit 25=0) and remote (bit 26=0) */
    cmp.l   #0x06000000, %d1  /* ON buttons simultaneously */
    beq.b   .loadrockbox
    subq.l  #1, %d0
    bne.b   .on_button_test

.loadoriginal:
    jmp     0x10010

.loadrockbox:
    move.l  %a0, %d7          /* keep initial GPIO_READ value in %d7 for now */

#endif /* defined(BOOTLOADER && defined(HAVE_DUALBOOT)
          && (defined(IAUDIO_X5) || defined(IAUDIO_M5)) */

    move.w  #0x2700,%sr

    move.l  #vectors,%d0
    movec.l %d0,%vbr

    move.l  #MBAR+1,%d0
    movec.l %d0,%mbar
        
    move.l  #MBAR2+1,%d0
    movec.l %d0,%mbar2

    lea     MBAR,%a0
    lea     MBAR2,%a1

#if defined(BOOTLOADER) && !defined(HAVE_DUALBOOT) \
    && (defined(IAUDIO_X5) || defined(IAUDIO_M5))
    move.l  (%a1), %d7        /* store GPIO_READ result for button check in main() */
#endif

    clr.l  (0x180,%a1)  /* PLLCR = 0 */

    /* 64K DMA-capable SRAM at 0x10000000
       DMA is enabled and has priority in both banks
       All types of accesses are allowed
       (We might want to restrict that to save power) */
    move.l  #0x10000e01,%d0
    movec.l %d0,%rambar1

    /* 32K Non-DMA SRAM at 0x10010000
       All types of accesses are allowed
       (We might want to restrict that to save power) */
    move.l  #0x10010001,%d0
    movec.l %d0,%rambar0

    /* Chip select 0 - Flash ROM */
    moveq.l #0x00,%d0      /* CSAR0 - Base = 0x00000000 */
    move.l  %d0,(0x080,%a0)
    move.l  #FLASH_SIZE-0x10000+1,%d0 /* CSMR0 - All access */
    move.l  %d0,(0x084,%a0)
    move.l  #0x00000180,%d0 /* CSCR0 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x088,%a0)
        
#if (defined MPIO_HD200) || (defined MPIO_HD300)
    /* Chip select 3 - LCD controller */
    move.l  #0xf0000000,%d0 /* CSAR3 - Base = 0xf0000000 */
    move.l  %d0,(0x0a4,%a0)
    moveq.l #0x1,%d0        /* CSMR3 - 64K */
    move.l  %d0,(0x0a8,%a0)
    move.l  #0x00000180,%d0 /* CSCR3 - no wait states, 16 bits no bursts */
    move.l  %d0,(0x0ac,%a0)
#elif !(defined IAUDIO_M3)
    /* Chip select 1 - LCD controller */
    move.l  #0xf0000000,%d0 /* CSAR1 - Base = 0xf0000000 */
    move.l  %d0,(0x08c,%a0)
    moveq.l #0x1,%d0        /* CSMR1 - 64K */
    move.l  %d0,(0x090,%a0)
    move.l  #0x00000180,%d0 /* CSCR1 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x094,%a0)
#endif
        
    /* Chip select 2 - ATA controller */
    move.l  #0x20000000,%d0 /* CSAR2 - Base = 0x20000000 */
    move.l  %d0,(0x098,%a0)
    moveq.l #0x1,%d0        /* CSMR2 - 64K */
    move.l  %d0,(0x09c,%a0)
    move.l  #0x00000080,%d0 /* CSCR2 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x0a0,%a0) /* wait states are handled by the coldfire
                             * IDE interface logic. */

#if defined(CONFIG_USBOTG) && CONFIG_USBOTG == USBOTG_ISP1362
    /* Chip select 3 - USBOTG controller */
    move.l  #0xc0000000,%d0 /* CSAR3 - Base = 0xc0000000 */
    move.l  %d0,(0x0a4,%a0)
    moveq.l #0x1,%d0        /* CSMR3 - 64K */
    move.l  %d0,(0x0a8,%a0)
    move.l  #0x00000180,%d0 /* CSCR3 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x0ac,%a0)
#endif

#ifdef BOOTLOADER
    /* Check if we have a Rockbox ROM image */
    lea     0x00100000,%a2
    move.l  (%a2),%d0
    move.l  #FLASH_MAGIC,%d1
    cmp.l   %d0,%d1
    beq.b   .imagefound
    
    /* Check for RAM image */
    lea     0x00001000,%a2
    move.l  (%a2),%d0
    move.l  #FLASH_MAGIC,%d1
    cmp.l   %d0,%d1
    beq.b   .imagefound
    
    /* Not either ROM or RAM image was found, so original firmware
       should be still present. */

    /* Check if the cookie is present. */
    lea     0x10017ffc,%a2
    move.l  (%a2),%d0
    move.l  #0xc0015a17,%d1
    cmp.l   %d0,%d1
    bne.b   .nocookie
    
    /* The cookie is not reset. This must mean that the boot loader
       has crashed. Let's start the original firmware immediately. */
    lea     0x10017ffc,%a2
    clr.l   (%a2)
    jmp     8
        
.nocookie:
    /* Set the cookie */
    move.l  %d1,(%a2)
.imagefound:

    /* Set up the DRAM controller. The refresh is based on the 11.2896MHz
       clock (5.6448MHz bus frequency). We haven't yet started the PLL */
#if MEMORYSIZE < 32
    move.w  #0x8004,%d0 /* DCR - Synchronous, 80 cycle refresh */
#else
    move.w  #0x8001,%d0 /* DCR - Synchronous, 32 cycle refresh */
#endif
    move.w  %d0,(0x100,%a0)

    /* Note on 32Mbyte models:
       We place the SDRAM on an 0x1000000 (16M) offset because
       the 5249 BGA chip has a fault which disables the use of A24. The
       suggested workaround by FreeScale is to offset the base address by
       half the DRAM size and increase the mask to the double.
       In our case this means that we set the base address 16M ahead and
       use a 64M mask.
    */
#if MEMORYSIZE < 32
    move.l  #0x31002324,%d0 /* DACR0 - Base 0x31000000, Banks on 21 and up,
                               CAS latency 2, Page mode, No refresh yet */
    move.l  %d0,(0x108,%a0)
    move.l  #0x00fc0001,%d0 /* Size: 16M */
    move.l  %d0,(0x10c,%a0) /* DMR0 - 32Mb */
#else
    move.l  #0x31002524,%d0 /* DACR0 - Base 0x31000000, Banks on 23 and up,
                               CAS latency 2, Page mode, No refresh yet */
    move.l  %d0,(0x108,%a0)
    move.l  #0x03fc0001,%d0 /* Size: 64M because of workaround above */
    move.l  %d0,(0x10c,%a0) /* DMR0 - 32Mb */
#endif

    /* Precharge */
    moveq.l #8,%d0
    or.l    %d0,(0x108,%a0) /* DACR0[IP] = 1, next access will issue a
                               Precharge command */
    move.l  #0xabcd1234,%d0
    move.l  %d0,0x31000000  /* Issue precharge command */

    move.l  #0x8000,%d0
    or.l    %d0,(0x108,%a0) /* Enable refresh */

    /* Let it refresh */
    move.l  #500,%d0
.delayloop:
    subq.l  #1,%d0
    bne.b   .delayloop

    /* Mode Register init */
    moveq.l #0x40,%d0       /* DACR0[IMRS] = 1, next access will set the
                               Mode Register */
    or.l    %d0,(0x108,%a0)

    move.l  #0xabcd1234,%d0
    move.l  %d0,0x31000800  /* A11=1 means CASL=2 (connected to SDRAM A5). */

    /* DACR0[IMRS] gets deactivated by the SDRAM controller */
    
    /* Check if we have a Rockbox ROM image. For RAM image only cookie is
       not set at all. But we could support also RAM images loading. */
    lea     0x00100000,%a2
    move.l  (%a2),%d0
    move.l  #FLASH_MAGIC,%d1
    cmp.l   %d0,%d1
    bne.b   .noromimage
    
    /* Check again if the cookie is present. */
    lea     0x10017ffc,%a2
    move.l  (%a2),%d0
    move.l  #0xc0015a17,%d1
    cmp.l   %d0,%d1
    bne.b   .nocookie2
    
    /* We have found Rockbox in ROM!
       Clear the cookie and load the ROM image */
    lea     0x10017ffc,%a2
    clr.l   (%a2)
    lea     0x00100028+4,%a2
    move.l  (%a2),%sp
    lea     0x00100028+8,%a2
    move.l  (%a2),%d0
    move.l  %d0,%a2
    jmp     (%a2)
    
.nocookie2:
    /* Set the cookie */
    move.l  %d1,(%a2)
.noromimage:

#endif /* BOOTLOADER */

    /* Invalicate cache */
    move.l  #0x01000000,%d0
    movec.l %d0,%cacr
        
    /* Enable cache, default=non-cacheable, no buffered writes */
    move.l  #0x80000000,%d0
    movec.l %d0,%cacr

    /* Cache enabled in SDRAM only, buffered writes enabled */
    move.l  #0x3103c020,%d0
    movec.l %d0,%acr0
    /* Buffered writes enabled for the LCD */
    move.l  #0xf000c060,%d0
    movec.l %d0,%acr1

#ifndef BOOTLOADER
    /* .iram copy is done first since it is reclaimed for other
     * uninitialized sections */

    /* copy .init section */
    lea     _initcopy,%a2
    lea     _initstart,%a3
    lea     _initend,%a4
    bra.b   .initstart
.initloop:
    move.l  (%a2)+,(%a3)+
.initstart:
    cmp.l   %a3,%a4
    bhi.b   .initloop

    /* copy the .iram section */
    lea     _iramcopy,%a2
    lea     _iramstart,%a3
    lea     _iramend,%a4
    bra.b   .iramstart
.iramloop:
    move.l  (%a2)+,(%a3)+
.iramstart:
    cmp.l   %a3,%a4
    bhi.b   .iramloop

    /* zero out .ibss */
    lea     _iedata,%a2
    lea     _iend,%a4
    bra.b   .iedatastart
.iedataloop:
    clr.l   (%a2)+
.iedatastart:
    cmp.l   %a2,%a4
    bhi.b   .iedataloop
#endif /* !BOOTLOADER */

#ifdef IRIVER_H300_SERIES
    /* Set KEEP_ACT before doing the lengthy copy and zero-fill operations */
    move.l #0x00080000,%d0
    or.l %d0,(0xb4,%a1)
    or.l %d0,(0xb8,%a1)
    or.l %d0,(0xbc,%a1)
#endif

#if (defined MPIO_HD200) || (defined MPIO_HD300)
    /* Set KEEP_ACT
     * Set GPIO57 high to remove hissing nois on startup
     */
    move.l #0x02200000,%d0
    or.l %d0,(0xb4,%a1)
    or.l %d0,(0xb8,%a1)
    or.l %d0,(0xbc,%a1)
#endif

    /* zero out bss */
    lea     _edata,%a2
    lea     _end,%a4
    bra.b   .edatastart
.edataloop:
    clr.l   (%a2)+
.edatastart:
    cmp.l   %a2,%a4
    bhi.b   .edataloop

    /* copy the .data section */
    lea     _datacopy,%a2
    lea     _datastart,%a3
    cmp.l   %a2,%a3
    beq.b   .nodatacopy   /* Don't copy if src and dest are equal */
    lea     _dataend,%a4
    bra.b   .datastart
.dataloop:
    move.l  (%a2)+,(%a3)+
.datastart:
    cmp.l   %a3,%a4
    bhi.b   .dataloop
.nodatacopy:

    /* Munge the main stack */
    lea     stackbegin,%a2
    lea     stackend,%a4
    move.l  %a4,%sp
    move.l  #0xdeadbeef,%d0
.mungeloop:
    move.l  %d0,(%a2)+
    cmp.l   %a2,%a4
    bhi.b   .mungeloop
    
#if defined(BOOTLOADER) && (defined(IAUDIO_X5) || defined(IAUDIO_M5))
    move.l  %d7, initial_gpio_read
#endif

    jsr     main
.hoo:
    bra.b   .hoo

    .section .resetvectors
vectors:
    .long   stackend
    .long   start
