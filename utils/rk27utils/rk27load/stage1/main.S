/* rk27xx DRAM init routine
 * Based on disassembly of the first binary image uploaded in rom DFU mode
 * Copyright (C) 2013 Marcin Bukat
 */

.section .text,"ax",%progbits
.global start

start:
    push {r4,r5,r6,r7,lr}

/* setup 200 MHz clock */
pll_setup:
    ldr     r0,=0x180e8000
    mov     r1, #0x81
    str     r1, [r0, #4]     /* FMWAIT */

    ldr     r0,=0x1801c000
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    bic     r1, r1, #0x1f
    orr     r1, r1, #9       /* ((1<<3)|(1<<0)) ARM slow mode,
                              * HCLK:PCLK = 2:1
                              */
    str     r1, [r0,#0x14]

    ldr     r1,=0x1850310    /* ((1<<24)|(1<<23)|(5<<16)|(49<<4)) */
    str     r1, [r0,#0x08]   /* SCU_PLLCON1 */

    ldr     r2,=0x40000

pll_lock_wait:
    ldr     r1, [r0,#0x2c]   /* SCU_STATUS */
    tst     r1, #1           /* ARM pll lock */
    bne     pll_locked
    subs    r2, r2, #1
    bne     pll_lock_wait

pll_locked:
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    bic     r1, #1           /* leave ARM slow mode */
    str     r1, [r0,#0x14]

/* detect SDRAM organization */
    ldr     r0,=0x180b0000   /* SDRAM controller base addr */
    mov     r2, #0x60000000  /* start of DRAM */
    ldr     r1,=0x5aa5f00f   /* test pattern */
    mov     r3, #1           /* used for bitshifts */
    mov     r4, #4           /* reg cfg 12bits col address */

col_loop:
    str     r4, [r0, #0x108] /* MCSDR_ADDCFG */
    add     r5, r4, #8       /* col_num_bits */
    mov     r6, r3, lsl r5   /* offset to the col1 (1<<col_num_bits) */
    mov     r7, #0
    str     r7, [r2]         /* *(0x60000000) = 0 */
    str     r1, [r2, r6]     /* store test pattern in col1 addr */
    ldr     r7, [r2]
    cmp     r7, #0           /* check if beginning of dram is not touched */
    ldreq   r7, [r2, r6]     /* readback col1 addr */
    cmpeq   r7, r1           /* check if test pattern is valid */
    beq     row_loop_setup   /* quit column loop */
    subs    r4, #1
    bpl     col_loop

row_loop_setup:
    mov     r5, #2           /* reg cfg 13bits row address */

row_loop:
    orr     r7, r4, r5, lsl#4
    str     r7, [r0, #0x108] /* MCSDR_ADDCFG */

    add     r7, r5, #11      /* row_num_bits */
    mov     lr, r3, lsl r7   /* 1<<row_num_bits */
    mul     lr, lr, r6       /* (1<<row_num_bits)*(1<<col_num_bits) */

    mov     r7, #0
    str     r7, [r2]         /* *(0x60000000) = 0 */   
    str     r1, [r2, lr]     /* store test pattern */
    ldr     r7, [r2]
    cmp     r7, #0           /* check if beginning of dram is not touched */
    ldreq   r7, [r2, lr]     /* readback row1 addr */
    cmpeq   r7, r1           /* check if test pattern is valid */
    beq     end
    subs    r5, #1
    bpl     row_loop

end:
    orr     r0, r4, r5, lsl#4
    pop     {r4,r5,r6,r7,pc}

