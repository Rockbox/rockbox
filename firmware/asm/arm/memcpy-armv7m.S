#include "config.h"

#if (CONFIG_CPU == STM32H743)
# define CPU_SUPPORTS_UNALIGNED_ACCESS
#endif

    .syntax unified
    .section .icode,"ax",%progbits

    .global     memcpy
    .type       memcpy,%function
    .global     mempcpy
    .type       mempcpy,%function

    /* Prototype: void *memcpy(void *dest, const void *src, size_t n); */
    /* Prototype: void *mempcpy(void *dest, const void *src, size_t n); */

    /*
     * r0 = dest
     * r1 = src
     * r2 = n
     * r3 = scratch
     * ip = saved r0
     */

mempcpy:
    add     ip, r0, r2
    b       0f

memcpy:
    movs    ip, r0

0:
#if !defined(CPU_SUPPORTS_UNALIGNED_ACCESS)
    /* fall back to byte copy loop if operands not aligned */
    ands    r3, r0, #3
    bne     5f
    ands    r3, r1, #3
    bne     5f
#endif

    /* check remaining bytes >= 16 */
    subs    r2, r2, #16
    blt     2f

1:  /* copy 16-byte blocks */
    ldr     r3, [r1, #0]
    str     r3, [r0, #0]
    ldr     r3, [r1, #4]
    str     r3, [r0, #4]
    ldr     r3, [r1, #8]
    str     r3, [r0, #8]
    ldr     r3, [r1, #12]
    str     r3, [r0, #12]
    adds    r0, #16
    adds    r1, #16
    subs    r2, r2, #16
    bge     1b

2:  /* get count of remaining bytes (0-15) */
    and     r2, r2, #15

3:  /* copy 4 bytes at a time */
    subs    r2, r2, #4
    ldrge   r3, [r1], #4
    strge   r3, [r0], #4
    bgt     3b

4:  /* get count of remaining bytes (0-3) */
    and     r2, #3

5:  /* copy 1 byte at a time */
    subs    r2, r2, #1
    ldrbge  r3, [r1], #1
    strbge  r3, [r0], #1
    bgt     5b

6:  /* return */
    movs    r0, ip
    bx      lr
