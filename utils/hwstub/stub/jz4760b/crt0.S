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
     * Indeed, if the invalidate the entire icache in the cache-as-ram case, we
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

    /* jump to C code */
    la      t0, main
    jr      t0
    move    a0, k0
