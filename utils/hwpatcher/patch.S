.text
.global _start
_start:
    b       exec
branch_addr:
    .word   kill
hw_power_sts:
#if defined(CREATIVE_ZEN)
    .word   0x800440b0 /* STMP3700 */
#else
    .word   0x800440c0 /* IMX233 */
#endif
hw_pinctrl_din0:
    .word   0x80018600
hw_pinctrl_din1:
    .word   0x80018610
hw_pinctrl_din2:
    .word   0x80018620
kill:
    ldr     pc, branch_addr
exec:
#if defined(SANSA_FUZEPLUS)
    /* check PSWITCH=1 (power button pressed) */
    ldr     r0, hw_power_sts
    ldr     r0, [r0]
    mov     r0, r0, lsr #20
    and     r0, #3
    cmp     r0, #1
    bne     ret
    /* check B1P30=0 (volume down pressed) */
    ldr     r0, hw_pinctrl_din1
    ldr     r0, [r0]
    mov     r0, r0, lsr #30
    ands    r0, #1
    beq     kill
#elif defined(CREATIVE_ZENXFI3)
    /* check PSWITCH=1 (power button pressed) */
    ldr     r0, hw_power_sts
    ldr     r0, [r0]
    mov     r0, r0, lsr #20
    and     r0, #3
    cmp     r0, #1
    bne     ret
    /* check B2P07=0 (volume down pressed) */
    ldr     r0, hw_pinctrl_din2
    ldr     r0, [r0]
    mov     r0, r0, lsr #7
    ands    r0, #1
    beq     kill
#elif defined(CREATIVE_ZENXFI2)
    /* check B0P11=0 (power button pressed) and B0P14 (select button pressed) */
    ldr     r0, hw_pinctrl_din0
    ldr     r0, [r0]
    mov     r0, r0, lsr #11
    tst     r0, #1
    bne     ret
    mov     r0, r0, lsr #3
    tst     r0, #1
    beq     kill
#elif defined(CREATIVE_ZEN)
    /* check PSWITCH=1 (power button pressed) */
    ldr     r0, hw_power_sts
    ldr     r0, [r0]
    mov     r0, r0, lsr #18
    and     r0, #3
    cmp     r0, #0
    bne     kill
#elif defined(SONY_NWZ)
    /* check PSWITCH=3 (power button pressed) */
    ldr     r0, hw_power_sts
    ldr     r0, [r0]
    mov     r0, r0, lsr #20
    and     r0, #3
    cmp     r0, #3
    beq     kill
#elif defined(CREATIVE_ZENXFISTYLE)
    /* check PSWITCH=1 (power button pressed) */
    ldr     r0, hw_power_sts
    ldr     r0, [r0]
    mov     r0, r0, lsr #20
    and     r0, #3
    cmp     r0, #1
    beq     kill
#else
#error implement me
#endif
ret: