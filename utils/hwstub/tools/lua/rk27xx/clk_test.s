@ RK27xx clock configuration probe, runs from IRAM@0x800 with the cache on
@ and IRQ/FIQ masked. Built into rk27xx/clk.lua as a blob (see there).
@
@ Executes a list of (op, arg) word pairs at IRAM 0xc00 (up to 64 ops),
@ results go to the word slots at IRAM 0xe00:
@   0 end                    6 STAMP slot: TMR1 -> slot
@   1 DIV v: SCU_DIVCON1[4:0] = v      7 LOOP: loop start
@   2 PLL v: SCU_PLLCON1 = v           8 ENDLOOP k: run the loop body k
@   3 LOCK slot: wait for the ARM PLL     times in all
@     lock, TMR1 ticks waited -> slot  9 TREF v: MCSDR_T_REF = v
@     (0xffffffff: no lock)           10 STATUS slot: SCU_STATUS -> slot
@   4 SPIN n: n passes of subs/bne (core clock, from the cache)
@   5 AHB n: n reads of INTC_IPR
@ Whatever the list did, the original SCU_DIVCON1, SCU_PLLCON1 and
@ MCSDR_T_REF are restored at the end (via slow mode and a PLL lock wait).
@ out: +0 original SCU_DIVCON1  +4 original SCU_PLLCON1  +8 MCSDR_T_REF
    .arm
    .global _start
_start:
    push    {r4-r11,lr}
    mov     r12, #0x700
    mrs     r11, cpsr
    orr     r0, r11, #0xc0
    msr     cpsr_c, r0

    ldr     r10, =0x18000000        @ timers
    ldr     r9, =0x1801c000         @ SCU
    ldr     r8, =0x180b0110         @ MCSDR_T_REF
    ldr     r0, [r9, #0x14]
    str     r0, [r12, #0]
    ldr     r0, [r9, #0x08]
    str     r0, [r12, #4]
    ldr     r0, [r8]
    str     r0, [r12, #8]

    @ cache: invalidate both ways and enable, I/O and DSP memory uncached
    ldr     r6, =0xefff0000
    ldr     r0, =0x180000fe
    str     r0, [r6, #0x10]
    str     r0, [r6, #0x14]
    ldr     r0, =0x300000fe
    str     r0, [r6, #0x18]
    ldr     r0, =0xee0000fe
    str     r0, [r6, #0x1c]
    mov     r0, #2
    str     r0, [r6, #4]
1:  ldr     r1, [r6, #4]
    tst     r1, #3
    bne     1b
    orr     r0, r0, #0x80000000
    str     r0, [r6, #4]
1:  ldr     r1, [r6, #4]
    tst     r1, #3
    bne     1b
    ldr     r0, [r6]
    orr     r0, r0, #0x80000000
    str     r0, [r6]

    mov     r7, #0xc00              @ ops
    mov     r6, #0xe00              @ result slots
    mov     r5, #0xc00              @ loop start
    mov     r4, #0                  @ loop count
next:
    ldr     r0, [r7], #4
    ldr     r1, [r7], #4
    cmp     r0, #10
    addls   pc, pc, r0, lsl #2
    b       done
    b       done                    @ 0
    b       op_div                  @ 1
    b       op_pll                  @ 2
    b       op_lock                 @ 3
    b       op_spin                 @ 4
    b       op_ahb                  @ 5
    b       op_stamp                @ 6
    b       op_loop                 @ 7
    b       op_endloop              @ 8
    b       op_tref                 @ 9
    b       op_status               @ 10

op_div:
    ldr     r2, [r9, #0x14]
    bic     r2, r2, #0x1f
    orr     r2, r2, r1
    str     r2, [r9, #0x14]
    b       next
op_pll:
    str     r1, [r9, #0x08]
    b       next
op_lock:
    ldr     r3, [r10, #0x14]
    mov     r2, #0x100000
1:  ldr     r0, [r9, #0x2c]
    tst     r0, #1
    bne     2f
    subs    r2, r2, #1
    bne     1b
    mvn     r0, #0
    str     r0, [r6, r1, lsl #2]
    b       next
2:  ldr     r0, [r10, #0x14]
    sub     r0, r3, r0
    str     r0, [r6, r1, lsl #2]
    b       next
op_spin:
    cmp     r1, #0
    beq     next
1:  subs    r1, r1, #1
    bne     1b
    b       next
op_ahb:
    cmp     r1, #0
    beq     next
    ldr     r2, =0x18080108         @ INTC_IPR
1:  ldr     r0, [r2]
    subs    r1, r1, #1
    bne     1b
    b       next
op_stamp:
    ldr     r0, [r10, #0x14]
    str     r0, [r6, r1, lsl #2]
    b       next
op_loop:
    mov     r5, r7
    mov     r4, #0
    b       next
op_endloop:
    add     r4, r4, #1
    cmp     r4, r1
    movlo   r7, r5
    b       next
op_tref:
    str     r1, [r8]
    b       next
op_status:
    ldr     r0, [r9, #0x2c]
    str     r0, [r6, r1, lsl #2]
    b       next

done:
    @ back to the original clocks: slow mode, PLL, lock, dividers, refresh
    ldr     r0, [r9, #0x14]
    orr     r0, r0, #1
    str     r0, [r9, #0x14]
    ldr     r0, [r12, #4]
    str     r0, [r9, #0x08]
    mov     r2, #0x100000
1:  ldr     r0, [r9, #0x2c]
    tst     r0, #1
    bne     2f
    subs    r2, r2, #1
    bne     1b
2:  ldr     r0, [r12, #0]
    str     r0, [r9, #0x14]
    ldr     r0, [r12, #8]
    str     r0, [r8]

    @ cache off (write-through: nothing to write back)
    ldr     r6, =0xefff0000
    ldr     r0, [r6]
    bic     r0, r0, #0x80000000
    str     r0, [r6]

    msr     cpsr_c, r11
    pop     {r4-r11,pc}
    .ltorg
