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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"

    .section .init.text,"ax",@progbits

    .global    start
start:

    move.w  #0x2700,%sr

    move.l  #vectors,%d0
    movec.l %d0,%vbr
        
    move.l  #MBAR+1,%d0
    movec.l %d0,%mbar
        
    move.l  #MBAR2+1,%d0
    movec.l %d0,%mbar2

    lea     MBAR,%a0
    lea     MBAR2,%a1
        
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
        
    /* Chip select 1 - LCD controller */
    move.l  #0xf0000000,%d0 /* CSAR1 - Base = 0xf0000000 */
    move.l  %d0,(0x08c,%a0)
    moveq.l #0x1,%d0        /* CSMR1 - 64K */
    move.l  %d0,(0x090,%a0)
    move.l  #0x00000180,%d0 /* CSCR1 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x094,%a0)
        
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
#if MEM < 32
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
#if MEM < 32
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
    /* zero out .ibss */
    lea     _iedata,%a2
    lea     _iend,%a4
    bra.b   .iedatastart
.iedataloop:
    clr.l   (%a2)+
.iedatastart:
    cmp.l   %a2,%a4
    bhi.b   .iedataloop

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
#endif /* !BOOTLOADER */

#ifdef IRIVER_H300_SERIES
    /* Set KEEP_ACT before doing the lengthy copy and zero-fill operations */
    move.l #0x00080000,%d0
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

    jsr     main
.hoo:
    bra.b   .hoo

    .section .resetvectors
vectors:
    .long   stackend
    .long   start
