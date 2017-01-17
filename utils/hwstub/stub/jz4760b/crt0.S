#include "mips.h"

.extern main
.global start

.set mips32
.set noreorder
.set noat

.section .init.text,"ax",%progbits
/* WARNING
 * We have no idea where the stubs starts running, there basically are three cases:
 * - tcsm0: the stub is already at the right place, nothing do to
 * - ram: sdram/ddram is active and we just need to move the stub
 * - cache: the bootrom put us in cache-as-ram, we need to be careful
 * Note that that those are initially quite different because:
 * - tcsm0 is uncached
 * - ram is almost always cached when we are running from it
 * - cache-as-ram is cached but the cache is the only copy of our code and the
 *   icache was prefilled from dcache by the bootrom using some mips magic
 *
 * This means we have to be very careful with the cache because if we flush the
 * icache in the cache-as-cache case, we cannot refill it, and worse, we cannot
 * commit the dcache either because the ram might not even be initialised. Thus
 * the only safe option in all cases is to copy the stub to an *uncached* location
 * so that we don't have to commit the dcache and the icache can safely read from
 * it.
 */
start:
    bltzal zero, load_addr      /* ra = PC + 8, branch not taken */
    nop
load_addr:
    addiu   v0, ra, -8          /* calc real load address
                                   account for branch delay slot */
    move    k0, v0              /* store starting location to give it to main */

    la      t0, relocstart      /* relocate code if needed */
    la      t1, relocend
    beq     t0, v0, clear_bss   /* no relocation needed */
    nop
reloc_loop:
    lw      s0, 0(v0)           /* v0 = src */
    lw      s1, 4(v0)
    lw      s2, 8(v0)
    lw      s3, 12(v0)

    sw      s0, 0(t0)           /* t0 = dst */
    sw      s1, 4(t0)
    sw      s2, 8(t0)
    sw      s3, 12(t0)

    /* Tricky part: as explained earlier, tcsm0 is uncached so no need to commit
     * the dcache but we want to invalidate the icache ONLY AT THIS LOCATION.
     * Indeed, if we invalidate the entire icache in the cache-as-ram case, we
     * will miserably crash */
    cache   ICHitInv, 0(t0)   /* invalidate virtual address in icache */

    addiu   t0, t0, 16          /* inc dst addr */
    slt     t2, t0, t1
    bnez    t2, reloc_loop
    addiu   v0, v0, 16          /* inc src addr */

    /* jump to tcsm0 */
    la      t0, tcsm0_entry
    jr      t0
    sync
tcsm0_entry:
    /* now that we are running from tcsm0, which is uncached, we can finally
     * properly invalidate all caches just to be sure */
    mtc0    zero, C0_TagLo
    mtc0    zero, C0_DataLo
    la      t0, 0x80000000          /* an idx op should use an unmappable address */
    ori     t1, t0, 0x4000          /* 16kB cache */

cache_inv_loop:
    cache   ICIndexStTag, 0(t0)     /* index store icache tag */
    cache   DCIndexStTag, 0(t0)     /* index store dcache tag */
    bne     t0, t1, cache_inv_loop
    addiu   t0, 0x20                /* 32 bytes per line */

clear_bss:
    la      t0, bssbegin
    la      t1, bssend
    beq     t0, t1, stack_setup
    nop

clear_bss_loop:
    sw      zero, 0(t0)
    bne     t0, t1, clear_bss_loop
    addiu   t0, 4

stack_setup:
    la      sp, oc_stackend

    /* the tcsm0 is usually accessed by its weird 0xf4000000 address but this
     * address is not in the range available for EBASE (because EBASE[31:30]
     * is hardwired to 0b10). Fortunately, the TCSM0 can be accessed by its
     * physical address (0x132b0000) if we ungate the AHB1 */
    la      t0, 0xb0000028 /* CPM_CLKGATE1 */
    lw      t1, 0(t0)
    li      t2, 0xffffff7e /* AHB1 */
    and     t1, t2 /* clear AHB1 */
    sw      t1, 0(t0)
    /* keep interrupts disabled, use normal exception vectors (to use EBASE) */
    li      t0, 0
    mtc0    t0, C0_STATUS
    /* set EBASE */
    la      t0, irqbase
    mtc0    t0, C0_EBASE
    /* jump to C code */
    la      t0, main
    jr      t0
    move    a0, k0

die_blink:
    /* die blinking */
    la      a0, 0xb0010400
    li      a1, 2
    sw      a1, 0x48(a0) /* clear function (gpio or interrupt) */
    sw      a1, 0x58(a0) /* clear select (gpio) */
    sw      a1, 0x64(a0) /* set direction (out) */
    sw      a1, 0x34(a0) /* set pull (disable) */
    /* turn backlight on and off */
    la      a0, 0xb0010414
    li      a1, 2
.blink_loop:
    sw      a1, (a0)
    la      v0, 10000000
.wait:
    bnez    v0, .wait
    subu    v0, 1
    sw      a1, 4(a0)
    la      v0, 10000000
.wait2:
    bnez    v0, .wait2
    subu    v0, 1
    j       .blink_loop
    nop

/* restore_data_abort_jmp restores the context and returns from exception */
    .extern restore_data_abort_jmp

    .global tlb_refill_handler
    .section .exception.tlb_refill,"ax",%progbits
tlb_refill_handler:
    la      k0, die_blink
    jr      k0
    nop

    .global cache_error_handler
    .section .exception.cache_error,"ax",%progbits
cache_error_handler:
    la      k0, die_blink
    jr      k0
    nop

    .global general_exception_handler
    .section .exception.general_exception,"ax",%progbits
general_exception_handler:
    la      k0, die_blink
    jr      k0
    nop

/* no IRQ handler because we don't use interrupts and mask them all */
