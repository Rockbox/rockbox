/* ANSI concatenation macros.  */

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

/* Use the right prefix for global labels.  */

#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)

#define d0 %d0
#define d1 %d1
#define d2 %d2
#define d3 %d3
#define d4 %d4
#define d5 %d5
#define d6 %d6
#define d7 %d7
#define a0 %a0
#define a1 %a1
#define a2 %a2
#define a3 %a3
#define a4 %a4
#define a5 %a5
#define a6 %a6
#define fp %fp
#define sp %sp


.global SYM (setjmp)
.global SYM (longjmp)

SYM (setjmp):
    moveal sp@(4),a0
    movel sp@(0),a0@(12)
    movel sp,a0@(8)
    moveml d2-d7/a2-a6,a0@(20)
    clrl d0
    rts

SYM (longjmp):
    moveal sp@(4),a0
    movel sp@(8),d0
    bne 1f
    movel &1,d0
1:
    moveml a0@(20),d2-d7/a2-a6
    moveal a0@(8),sp
    movel a0@(12),sp@
    rts

#ifdef M68881
.global SYM (setjmp_68881)
.global SYM (longjmp_68881)

SYM (setjmp_68881):
    moveal sp@(4),a0
    movel sp@(0),a0@(12)
    movel sp,a0@(8)
    moveml d2-d7/a2-a6,a0@(20)
    fmovemx fp2-fp7,a0@(64)
    clrl d0
    rts

SYM (longjmp_68881):
    moveal sp@(4),a0
    fmovemx a0@(64),fp2-fp7
    movel sp@(8),d0
    bne 1f
    movel &1,d0
1:
    moveml a0@(20),d2-d7/a2-a6
    moveal a0@(8),sp
    movel a0@(12),sp@
    rts
#endif
