
/* This value is filled in by mkamsboot */
originalentry:   .word   0

        /* A delay loop - just to prove we're running */
        mov   r1, #0x500000       /* Approximately 5 seconds */
loop:   subs  r1, r1, #1
        bne   loop

        /* Now branch back to the original firmware's entry point */
        ldr   pc, originalentry
