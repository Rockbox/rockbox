/* This is a simple version of setjmp and longjmp.

   Nick Clifton, Cygnus Solutions, 13 June 1997.  */

/* ANSI concatenation macros.  */
#define CONCAT(a, b)  CONCAT2(a, b)
#define CONCAT2(a, b) a##b

#ifndef __USER_LABEL_PREFIX__
#error  __USER_LABEL_PREFIX__ not defined
#endif

#define SYM(x) CONCAT (__USER_LABEL_PREFIX__, x)

#ifdef __ELF__
#define TYPE(x) .type SYM(x),function
#define SIZE(x) .size SYM(x), . - SYM(x)
#else
#define TYPE(x)
#define SIZE(x)
#endif

/* Arm/Thumb interworking support:

   The interworking scheme expects functions to use a BX instruction
   to return control to their parent.  Since we need this code to work
   in both interworked and non-interworked environments as well as with
   older processors which do not have the BX instruction we do the 
   following:
    Test the return address.
    If the bottom bit is clear perform an "old style" function exit.
    (We know that we are in ARM mode and returning to an ARM mode caller).
    Otherwise use the BX instruction to perform the function exit.

   We know that we will never attempt to perform the BX instruction on 
   an older processor, because that kind of processor will never be 
   interworked, and a return address with the bottom bit set will never 
   be generated.

   In addition, we do not actually assemble the BX instruction as this would
   require us to tell the assembler that the processor is an ARM7TDMI and
   it would store this information in the binary.  We want this binary to be
   able to be linked with binaries compiled for older processors however, so
   we do not want such information stored there.  

   If we are running using the APCS-26 convention however, then we never
   test the bottom bit, because this is part of the processor status.  
   Instead we just do a normal return, since we know that we cannot be 
   returning to a Thumb caller - the Thumb does not support APCS-26.
    
   Function entry is much simpler.  If we are compiling for the Thumb we 
   just switch into ARM mode and then drop through into the rest of the
   function.  The function exit code will take care of the restore to
   Thumb mode.
   
   For Thumb-2 do everything in Thumb mode.  */

#ifdef __APCS_26__
#define RET movs        pc, lr
#elif defined(__thumb2__)
#define RET bx lr
#elif !defined(ROCKBOX_SUPPORTS_AN_ARMV4_NON_T_CPU)
#define RET bx lr   /* BX exists on v4T, v5 and more recent architectures */
#else
#define RET tst     lr, #1; \
            moveq       pc, lr ; \
.word           0xe12fff1e  /* bx lr */
#endif

#ifdef __thumb2__
.macro COND where when 
    i\where \when
.endm
#else
.macro COND where when 
.endm
#endif

#if defined(__thumb2__)
.syntax unified
.macro MODE
    .thumb
    .thumb_func
.endm
.macro PROLOGUE name
.endm

#elif defined(__thumb__)
#define MODE        .thumb_func
.macro PROLOGUE name
    .code 16
    bx  pc
    nop 
    .code 32
SYM (.arm_start_of.\name):
.endm
#else /* Arm */
#define MODE        .code 32
.macro PROLOGUE name
.endm
#endif
    
.macro FUNC_START name
    .text
    .align 2
    MODE
    .globl SYM (\name)
    TYPE (\name)
SYM (\name):
    PROLOGUE \name
.endm

.macro FUNC_END name
    RET
    SIZE (\name)
.endm
    
/* --------------------------------------------------------------------
                 int setjmp (jmp_buf); 
   -------------------------------------------------------------------- */
    
    FUNC_START setjmp

    /* Save all the callee-preserved registers into the jump buffer.  */
#ifdef __thumb2__
    stmea       a1!, { v1-v7, fp, ip, lr }
    str     sp, [a1],#+4
#else
    stmea       a1!, { v1-v7, fp, ip, sp, lr }
#endif
    
#if 0   /* Simulator does not cope with FP instructions yet.  */
#ifndef __SOFTFP__
    /* Save the floating point registers.  */
    sfmea       f4, 4, [a1]
#endif
#endif      
    /* When setting up the jump buffer return 0.  */
    mov     a1, #0

    FUNC_END setjmp
    
/* --------------------------------------------------------------------
        volatile void longjmp (jmp_buf, int);
   -------------------------------------------------------------------- */
    
    FUNC_START longjmp

    /* If we have stack extension code it ought to be handled here.  */
    
    /* Restore the registers, retrieving the state when setjmp() was called.  */
#ifdef __thumb2__
    ldmfd       a1!, { v1-v7, fp, ip, lr }
#if __ARM_ARCH_PROFILE == 'M'
    /*
     * Errata 752419: Interrupted loads to SP can cause erroneous behaviour
     * Apply the suggested workaround of loading to an intermediate register
     * and then moving into sp. This only affects Cortex-M3 but a warning is
     * issued by the assembler for all armv7-m targets with binutils 2.38.
     */
    ldr         a3, [a1], #4
    mov         sp, a3
#else
    ldr         sp, [a1], #4
#endif
#else
    ldmfd       a1!, { v1-v7, fp, ip, sp, lr }
#endif
    
#if 0   /* Simulator does not cope with FP instructions yet.  */
#ifndef __SOFTFP__
    /* Restore floating point registers as well.  */
    lfmfd       f4, 4, [a1]
#endif
#endif  
    /* Put the return value into the integer result register.
       But if it is zero then return 1 instead.  */ 
    movs        a1, a2
#ifdef __thumb2__
    it      eq
#endif
    moveq       a1, #1

    FUNC_END longjmp

