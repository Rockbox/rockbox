; Register dump, then MAGIC/TEST_ID, then spin.
; Layout (DSP data units from RES):
;   0-15 r0-r15 | 16-29 a0-a6 (2 units each) | 30-37 n0-n7
;   38-52 control regs (see list) | 53-56 g pairs | 58-59 store dummy
;   62 TEST_ID | 63 MAGIC
dump:   pst st r0,a7,0
        pst st r1,a7,1
        pst st r2,a7,2
        pst st r3,a7,3
        pst st r4,a7,4
        pst st r5,a7,5
        pst st r6,a7,6
        pst st r7,a7,7
        pst st r8,a7,8
        pst st r9,a7,9
        pst st r10,a7,10
        pst st r11,a7,11
        pst st r12,a7,12
        pst st r13,a7,13
        pst st r14,a7,14
        pst st r15,a7,15
        pst std a0,a7,16
        pst std a1,a7,18
        pst std a2,a7,20
        pst std a3,a7,22
        pst std a4,a7,24
        pst std a5,a7,26
        pst std a6,a7,28
        pst st n0,a7,30
        pst st n1,a7,31
        pst st n2,a7,32
        pst st n3,a7,33
        pst st n4,a7,34
        pst st n5,a7,35
        pst st n6,a7,36
        pst st n7,a7,37
        mov r0,%fmode
        pst st r0,a7,38
        mov r0,%tc
        pst st r0,a7,39
        mov r0,%imask
        pst st r0,a7,40
        mov r0,%loop0
        pst st r0,a7,41
        mov r0,%loop1
        pst st r0,a7,42
        mov r0,%loop2
        pst st r0,a7,43
        mov r0,%loop3
        pst st r0,a7,44
        mov r0,%amode
        pst st r0,a7,45
        mov r0,%smode
        pst st r0,a7,46
        mov r0,%rpc
        pst st r0,a7,47
        mov r0,%tpc
        pst st r0,a7,48
        mov r0,%hwflag
        pst st r0,a7,49
        mov r0,%shwflag
        pst st r0,a7,50
        mov r0,%psmode
        pst st r0,a7,51
        mov r0,%vitr
        pst st r0,a7,52
        mov r0,g0
        pst st r0,a7,53
        mov r0,g2
        pst st r0,a7,54
        mov r0,g4
        pst st r0,a7,55
        mov r0,g6
        pst st r0,a7,56
        mov.l r0,TEST_ID
        pst st r0,a7,62
        mov.l r0,MAGIC
        pst st r0,a7,63
spin:   br spin
