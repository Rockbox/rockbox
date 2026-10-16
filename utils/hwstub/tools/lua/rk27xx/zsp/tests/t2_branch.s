; T2: branch offsets and conditions. Each case writes 1 (expected path)
; or 0xBAD (wrong path) into its register.
        .equ TEST_ID, 0x0002
        .include "common.inc"
        .include "prologue.inc"
        mov.l r5,0
        mov.l r6,0
        mov.l r7,0
        mov.l r8,0
        mov.l r9,0
        mov.l r10,0
        mov.l r11,0
; A: 16-bit bz forward, taken
        cmp r5,0
        bz a_t
        mov.l r6,0xBAD
        br a_end
a_t:    mov.l r6,1
a_end:
; B: 16-bit bnz forward, not taken
        cmp r5,0
        bnz b_bad
        mov.l r7,1
        br b_end
b_bad:  mov.l r7,0xBAD
b_end:
; C: 16-bit bz backward, taken
        br c_fwd
c_back: mov.l r8,1
        br c_end
c_fwd:  cmp r5,0
        bz c_back
        mov.l r8,0xBAD
c_end:
; D: agn0 loop with %loop0 = 3 -> r9 = number of passes
        mov.l r0,3
        mov %loop0,r0
d_loop: add r9,1
        agn0 d_loop
; E: 32-bit conditional forward (bz, condition 6: not in the forced-backward set)
        cmp r5,0
        bz.l e_t
        mov.l r10,0xBAD
        br e_end
e_t:    mov.l r10,1
e_end:
; F: 32-bit br forward over > 127 halfwords
        br f_t
        .space 200
f_t:    mov.l r11,1
        br dump
.include "epilogue.inc"
