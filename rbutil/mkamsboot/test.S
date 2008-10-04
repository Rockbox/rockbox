/* int ucl_nrv2e_decompress_8(const unsigned char *src, unsigned char *dst, 
                           unsigned long *dst_len) */

.text
.global ucl_nrv2e_decompress_8


/* Vectors */
        ldr   pc, =start
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0
.word   0

/* These values are filled in by mkamsboot - don't move them from offset 0x20 */
ucl_unpack:      .word   0 /* Entry point (plus 1 - for thumb) of ucl_unpack after copy*/
ucl_start:       .word   0 /* Start of the ucl-compressed OF image after copy */
ucl_size:        .word   0 /* Length in bytes of the compressed OF image */
copy_start:      .word   0 /* Start of the copy of the ucl_unpack function */
copy_size:       .word   0 /* uclunpack_size + ucl_paddedsize */

start:
        /* A delay loop - just to prove we're running */
        mov   r1, #0x500000       /* Approximately 5 seconds */
loop:   subs  r1, r1, #1
        bne   loop

        /* First copy the compressed firmware to unused RAM */

        ldr   r0, copy_start   /* Source */
        ldr   r1, copy_size    /* Source length */

        mov   r2, #0x40000    /* Destination end */
        sub   r2, r2, r1

memcpy:
        ldrb  r3, [r0], #1
        strb  r3, [r2], #1
        cmp   r2, #0x40000    /* Stop when we reached dest_end */
        bne   memcpy

        /* Call the ucl decompress function, which will branch to 0x0 */
        /* on completion */

        ldr   r0, ucl_start   /* Address of compressed image */  
        ldr   r1, ucl_size    /* Compressed size */
        mov   r2, #0          /* Destination */

        ldr   r3, ucl_unpack
        bx    r3

        /* never reached */
