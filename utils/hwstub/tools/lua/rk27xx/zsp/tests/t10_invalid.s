; T10: one invalid opcode (-D OPC=0x....) between two markers in EXTRA.
        .equ TEST_ID, 0x0A00
        .include "common.inc"
        .include "prologue.inc"
        mov.l r0,0x1111
        pst st r0,a7,0x40
        .word OPC
        nop                     ; in case the core treats OPC as 32-bit
        mov.l r0,0x2222
        pst st r0,a7,0x41
.include "epilogue.inc"
