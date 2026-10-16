; a7 = result block. a7 is reserved for the harness; tests must not touch it.
        .org 0
        mov a7,lo(RES)
        movhw a7,hi(RES)
        st r0,a7,STORE_DUMMY    ; prime the store pipeline: pending
        st r0,a7,STORE_DUMMY    ; addresses = dummy slot
