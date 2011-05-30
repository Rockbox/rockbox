.section .text,"ax",%progbits
.global start

start:
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */

pll_setup:
    mov     r0, #0x18000000
    add     r0, r0, #0x1c000

    /* setup ARM core freq = 200MHz */
    /* AHB bus freq (HCLK) = 100MHz */
    /* APB bus freq (PCLK) = 50MHz  */
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    orr     r1, #9           /* ARM slow mode, HCLK:PCLK = 2:1 */
    str     r1, [r0,#0x14]

    ldr     r1,=0x01970c70   /* (1<<24) | (1<<23) | (23<<16) | (199<<4) */
    str     r1, [r0,#0x08]

    ldr     r2,=0x40000
1:
    ldr     r1, [r0,#0x2c]   /* SCU_STATUS */
    tst     r1, #1           /* ARM pll lock */
    bne     1f
    subs    r2, #1
    bne     1b
1:
    ldr     r1, [r0,#0x14]   /* SCU_DIVCON1 */
    bic     r1, #5           /* leave ARM slow mode, ARMclk:HCLK = 2:1 */
    str     r1, [r0,#0x14]

sdram_config:
    add     r0, r0, #0x94000 /* SDRAM base */

    mov     r1, #1
    str     r1, [r0,#0x10c]  /* MCSDR_BASIC Round-robin, SDRAM width 16bits */

    add     r1, #0x10
    str     r1, [r0,#0x108] /* MCSDR_ADDCFG 12 bits row/9 bits col addr */

    mov     pc, lr          /* we are done, return to bootrom code */ 
