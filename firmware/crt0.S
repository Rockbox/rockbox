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

#if (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)
    .section .init.text
#else
    .section .init.text,"ax",@progbits
#endif
    .global    start
start:
#if (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)

/* Based on startup.s from the iPodLinux loader
 *
 * Copyright (c) 2003, Daniel Palffy (dpalffy (at) rainstorm.org)
 * Copyright (c) 2005, Bernard Leach <leachbj@bouncycastle.org>
 *
 */
	.equ	PP5002_PROC_ID,	0xc4000000
	.equ	PP5002COP_CTRL, 0xcf004058
	.equ	PP5020_PROC_ID,	0x60000000
	.equ	PP5020_COP_CTRL, 0x60007004

start:
	/* get the high part of our execute address */
	ldr	r0, =0xff000000
	and	r8, pc, r0     		@ r8 is used later

#if CONFIG_CPU==PP5002
	mov	r0, #PP5002_PROC_ID
#else
	mov	r0, #PP5020_PROC_ID
#endif
	ldr	r0, [r0]
	and	r0, r0, #0xff
	cmp	r0, #0x55
	beq	1f

	/* put us (co-processor) to sleep */
#if CONFIG_CPU==PP5002
	ldr	r4, =PP5002_COP_CTRL
	mov	r3, #0xca
#else
	ldr	r4, =PP5020_COP_CTRL
	mov	r3, #0x80000000
#endif
	str	r3, [r4]

	ldr	pc, =cop_wake_start

cop_wake_start:
	/* jump the COP to startup */
	ldr	r0, =startup_loc
	ldr	pc, [r0]

1:
	/* setup some stack */
	ldr	sp, = _stackbegin

	/* get the high part of our execute address */
	ldr	r2, =0xffffff00
	and	r4, pc, r2

	/* Copy bootloader to safe area - 0x40000000 */
	mov	r5, #0x40000000
	ldr	r6, = _dataend
	sub	r0, r6, r5       /* length of loader */
	add	r0, r4, r0	 /* r0 points to start of loader */
1:
	cmp	r5, r6
	ldrcc	r2, [r4], #4
	strcc	r2, [r5], #4
	bcc	1b

	ldr	pc, =start_loc	/* jump to the relocated start_loc:  */

start_loc:
        /* Initialise bss section to zero */
        ldr     r3, =_bssstart
	ldr	r1, =_bssend
	mov	r2, #0x0

1:
	cmp	r3, r1
	strcc	r2, [r3], #4
	bcc	1b

	/* execute the loader - this will load an image to 0x10000000 */
	bl	main

	/* save the startup address for the COP */
	ldr	r1, =startup_loc
	str	r0, [r1]

#if CONFIG_CPU==PP5002
	/* make sure COP is sleeping */
	ldr	r4, =0xcf004050
1:
	ldr	r3, [r4]
	ands	r3, r3, #0x4000
	beq	1b

	/* wake up COP */
	ldr	r4, =PP5002_COP_CTRL
	mov	r3, #0xce
	strh	r3, [r4]
#else
	/* make sure COP is sleeping */
	ldr	r4, =PP5020_COP_CTRL
1:
	ldr	r3, [r4]
	ands	r3, r3, #0x80000000
	beq	1b

	/* wake up COP */
	@ ldr	r4, =PP5020_COP_CTRL
	mov	r3, #0x0
	str	r3, [r4]
#endif

	/* jump to start location */
	mov	pc, r0

startup_loc:
	.word	0x0

.align 8	/* starts at 0x100 */
.global boot_table
boot_table:
	/* here comes the boot table, don't move its offset */
	.space 400

#elif CONFIG_CPU == TCC730
    /* Platform:     Gmini 120/SP */
    ;; disable all interrupts
     clrsr fe
    clrsr ie
    clrsr te
    ld a14, #0x3F0000

    ld r5, 0xA5
    ldb @[a14 + 6], r5    ; disable watchdog

    ld a11, #(_datacopy)    ; where the data section is in the flash
    ld a8, #(_datastart)    ; destination

    ;; copy data section from flash to ram.
    ld a9, #_datasize
    ld r6, e9
    cmp eq, r6, #0
    brf .data_copy_loop
    cmp eq, r9, #0
    brt .data_copy_end
.data_copy_loop:
    ldc r2, @a11
    ldw @[a8 + 0], r2
    add a11, #0x2
    add a8, #0x2
    sub r9, #0x2
    sbc r6, #0
    cmp ugt, r6, #0
    brt .data_copy_loop
    cmp ugt, r9, #0
    brt .data_copy_loop
.data_copy_end:

    ;; zero out bss
    ld r2, #0
    ld a8, #(_bssstart)    ; destination
    ld a9, #_bsssize
    ld r6, e9

    cmp eq, r6, #0
    brf .bss_init_loop
    cmp eq, r9, #0
    brt .bss_init_end
.bss_init_loop:
    ldw @[a8 + 0], r2
    add a8, #0x2
    sub r9, #0x2
    sbc r6, #0
    cmp ugt, r6, #0
    brt .bss_init_loop
    cmp ugt, r9, #0
    brt .bss_init_loop
.bss_init_end:

    ;; set stack pointer
    ld a15, _stackend

    ;; go!
    jsr _main

    ;; soft reset
    ld a10, #0
    ldc r10, @a10
    jmp a10


    .section .vectors, "ax"
irq_handler:

    push r0, r1
    push r2, r3
    push r4, r5
    push r6, r7
    push a8, a9
    push a10, a11
    push a12, a13
    push a14
    ld a13, #0x3f0000
    ldb r0, @[a13 + 0x26]
    add r0, r0
    ld a10, #_interrupt_vector
    ldw a13, @[a10 + r0]
    jsr a13
    pop a14
    pop a13, a12
    pop a11, a10
    pop a9, a8
    pop r7, r6
    pop r5, r4
    pop r3, r2
    pop r1, r0
    ret_irq

#elif defined(IRIVER_H100_SERIES)
    /* Platform:     iRiver H120/H140 */
    move.w  #0x2700,%sr

    move.l  #vectors,%d0
    movec.l %d0,%vbr
        
    move.l  #MBAR+1,%d0
    movec.l %d0,%mbar
        
    move.l  #MBAR2+1,%d0
    movec.l %d0,%mbar2

    lea     MBAR,%a0
    lea     MBAR2,%a1
        
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
    move.l  #0x001f0001,%d0 /* CSMR0 - 2M, All access */
    move.l  %d0,(0x084,%a0)
    move.l  #0x00000180,%d0 /* CSCR0 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x088,%a0)
        
    /* Chip select 1 - LCD controller */
    move.l  #0xf0000000,%d0 /* CSAR1 - Base = 0xf0000000 */
    move.l  %d0,(0x08c,%a0)
    moveq.l #0x75,%d0      /* CSMR1 - 64K, Only data access */
    move.l  %d0,(0x090,%a0)
    move.l  #0x00000180,%d0 /* CSCR1 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x094,%a0)
        
    /* Chip select 2 - ATA controller */
    move.l  #0x20000000,%d0 /* CSAR2 - Base = 0x20000000 */
    move.l  %d0,(0x098,%a0)
    move.l  #0x000f0001,%d0 /* CSMR2 - 64K, Only data access */
    move.l  %d0,(0x09c,%a0)
    move.l  #0x00000080,%d0 /* CSCR2 - no wait states, 16 bits, no bursts */
    move.l  %d0,(0x0a0,%a0) /* NOTE:  I'm not sure about the wait states.
                               We have to be careful with the access times,
                               since IORDY isn't connected to the HDD. */
        

#ifdef BOOTLOADER
    /* The cookie is not reset. This must mean that the boot loader
       has crashed. Let's start the original firmware immediately. */
    lea     0x10017ffc,%a2
    move.l  (%a2),%d0
    move.l  #0xc0015a17,%d1
    cmp.l   %d0,%d1
    bne     .nocookie
    /* Clear the cookie again */
    clr.l   (%a2)
    jmp     8
        
.nocookie:
    /* Set the cookie */
    move.l  %d1,(%a2)
        
    /* Set up the DRAM controller. The refresh is based on the 11.2896MHz
       clock (5.6448MHz bus frequency). We haven't yet started the PLL */
#if MEM < 32
    move.w  #0x8204,%d0 /* DCR - Synchronous, 80 cycle refresh */
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
    move.l  #0x31002320,%d0 /* DACR0 - Base 0x31000000, Banks on 21 and up,
                               CAS latency 1, No refresh yet */
    move.l  %d0,(0x108,%a0)
    move.l  #0x00fc0001,%d0 /* Size: 16M */
    move.l  %d0,(0x10c,%a0) /* DMR0 - 32Mb */
#else
    move.l  #0x31002520,%d0 /* DACR0 - Base 0x31000000, Banks on 23 and up,
                               CAS latency 1, No refresh yet */
    move.l  %d0,(0x108,%a0)
    move.l  #0x03fc0001,%d0 /* Size: 64M because of workaround above */
    move.l  %d0,(0x10c,%a0) /* DMR0 - 32Mb */
#endif

    /* Precharge */
    move.l  #8,%d0
    or.l  %d0,(0x108,%a0)   /* DACR0[IP] = 1, next access will issue a
                               Precharge command */
    move.l  #0xabcd1234,%d0
    move.l  %d0,0x31000000  /* Issue precharge command */

    /* Let it refresh */
    move.l  #1000,%d0
.delayloop:
    subq.l  #1,%d0
    bne     .delayloop
        
    /* Refresh */
    move.l #0x8000,%d0
    or.l   %d0,(0x108,%a0) /* Enable refresh */

    /* Mode Register init */
    move.l #0x40,%d0       /* DACR0[IMRS] = 1, next access will set the
                              Mode Register */
    or.l  %d0,(0x108,%a0)

    move.l  #0xabcd1234,%d0
    move.l  %d0,0x31000800  /* A12=1 means CASL=1 (a0 is not connected) */

    move.l  #0xffffffbf,%d0
    and.l  %d0,(0x108,%a0)  /* Back to normal, the DRAM is now ready */
#endif /* BOOTLOADER */

    /* Invalicate cache */
    move.l  #0x01000000,%d0
    movec.l %d0,%cacr
        
    /* Enable cache, default=non-cacheable,no buffered writes */
    move.l  #0x80000000,%d0
    movec.l %d0,%cacr

    /* Cache enabled in SDRAM only, buffered writes enabled */
    move.l  #0x3103c020,%d0
    movec.l %d0,%acr0
    moveq.l #0,%d0
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
#elif defined(IRIVER_H300)
    /* Platform:     iRiver H320/H340 */

    /* Fill in code here */
#elif CONFIG_CPU == PP5020
    /* Platform:     iPod */

#warning TODO: Implement crt0.S

    /* Fill in code here */

#else
    /* Platform:     Archos Jukebox */

    mov.l   .vbr_k,r1
#ifdef DEBUG
    /* If we have built our code to be loaded via the standalone GDB
     * stub, we will have out VBR at some other location than 0x9000000.
     * We must copy the trap vectors for the GDB stub to our vector table. */
    mov.l   .orig_vbr_k,r2

    /* Move the invalid instruction vector (4) */
    mov     #4,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the invalid slot vector (6) */
    mov     #6,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the bus error vector (9) */
    mov     #9,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the DMA bus error vector (10) */
    mov     #10,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the NMI vector as well (11) */
    mov     #11,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the UserBreak vector as well (12) */
    mov     #12,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the breakpoint trap vector (32) */
    mov     #32,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the IO trap vector (33) */
    mov     #33,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the serial Rx interrupt vector (105) */
    mov     #105,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)

    /* Move the single step trap vector (127) */
    mov     #127,r0
    shll2   r0
    mov.l   @(r0,r2),r3
    mov.l   r3,@(r0,r1)
#endif /* DEBUG */
    ldc     r1,vbr

    mov     #0,r0
    ldc     r0,gbr

    /* zero out .ibss */
    mov.l   .iedata_k,r0
    mov.l   .iend_k,r1
    bra     .iedatastart
    mov     #0,r2
.iedataloop: /* backwards is faster and shorter */
    mov.l   r2,@-r1
.iedatastart:
    cmp/hi  r0,r1
    bt      .iedataloop

    /* copy the .iram section */
    mov.l   .iramcopy_k,r0
    mov.l   .iram_k,r1
    mov.l   .iramend_k,r2
    /* Note: We cannot put a PC relative load into the delay slot of a 'bra'
       instruction (the offset would be wrong), but there is nothing else to
       do before the loop, so the delay slot would be 'nop'. The cmp / bf
       sequence is the same length, but more efficient. */
    cmp/hi  r1,r2
    bf      .noiramcopy
.iramloop:
    mov.l   @r0+,r3
    mov.l   r3,@r1
    add     #4,r1
    cmp/hi  r1,r2
    bt      .iramloop
.noiramcopy:

    /* zero out bss */
    mov.l   .edata_k,r0
    mov.l   .end_k,r1
    bra     .edatastart
    mov     #0,r2
.edataloop: /* backwards is faster and shorter */
    mov.l   r2,@-r1
.edatastart:
    cmp/hi  r0,r1
    bt      .edataloop

    /* copy the .data section, for rombased execution */
    mov.l   .datacopy_k,r0
    mov.l   .data_k,r1
    cmp/eq  r0,r1
    bt      .nodatacopy   /* Don't copy if src and dest are equal */
    mov.l   .dataend_k,r2
    cmp/hi  r1,r2
    bf      .nodatacopy
.dataloop:
    mov.l   @r0+,r3
    mov.l   r3,@r1
    add     #4,r1
    cmp/hi  r1,r2
    bt      .dataloop
.nodatacopy:

    /* Munge the main thread stack */
    mov.l   .stackbegin_k,r0
    mov.l   .stackend_k,r1
    mov     r1,r15
    mov.l   .deadbeef_k,r2
.mungeloop: /* backwards is faster and shorter */
    mov.l   r2,@-r1
    cmp/hi  r0,r1
    bt      .mungeloop

    /* call the mainline */
    mov.l   .main_k,r0
    jsr     @r0
    nop
.hoo:
    bra     .hoo
    nop

    .align  2
.vbr_k:
    .long   vectors
#ifdef DEBUG
.orig_vbr_k:
    .long   0x09000000
#endif
.iedata_k:
    .long   _iedata
.iend_k:
    .long   _iend
.iramcopy_k:
    .long   _iramcopy
.iram_k:
    .long   _iramstart
.iramend_k:
    .long   _iramend
.edata_k:
    .long   _edata
.end_k:
    .long   _end
.datacopy_k:
    .long   _datacopy
.data_k:
    .long   _datastart
.dataend_k:
    .long   _dataend
.stackbegin_k:
    .long   _stackbegin
.stackend_k:
    .long   _stackend
.deadbeef_k:
    .long   0xdeadbeef
.main_k:
    .long   _main

    .section .resetvectors
vectors:
    .long   start
    .long   _stackend
    .long   start
    .long   _stackend
#endif
