/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#if 1 // **** remove includes for CPU test
#include "rbconfig.h"
#include "rb_test.h"
#include "vmachine.h"
#include "memory.h"
#include "tia_video.h"
#endif // ****

#include "cpu.h"

/* =========================================================================
 * #defines to setup the emulator.
 */

/* -------------------------------------------------------------------------
 * Optimization setting
 */

#define OPT_CPU_PARANOID
#define OPT_CPU 1

/* -------------------------------------------------------------------------
 * Choose the emulated CPU derivative
 */

// #define CPU_65C02
// #define CPU_VARIANT_65C02_ROCKWELL
// #define CPU_VARIANT_65C02_WDC
/* none defined == NMOS 6502 */

/* -------------------------------------------------------------------------
 * Define detailed CPU behaviour/variant
 */

//#define NO_DECIMAL_MODE
//#define CLEAR_D_ON_INTERRUPT
//#define CORRECT_BCD_FLAGS
//#define UNDOCUMENTED_AS_NOP

/* =========================================================================
 * (optional) macros to connect the cpu core to the rest of the emulated
 * machine
 */

/* -------------------------------------------------------------------------
 * Clocking stuff
 */
#define CLOCK_CYCLES(a)     do {clk += (a); ebeamx += 3 * (a);} while (0)
#define CLOCK_CYCLES_STOPPED()      CLOCK_CYCLES(10)
#define CLOCK_CYCLE_INC()     CLOCK_CYCLES(1);

/* -------------------------------------------------------------------------
 * Memory access.
 * TODO: move the following to memory.h
 */

/* macros for all OPT_CPU settings */
#   define LOAD(a)              decRead(a)
#   define STORE(a,b)           decWrite((a),(b))

#if 1 // **** remove for CPU-test
#ifdef OPT_CPU_PARANOID
    /* handles wraparound in the middle of a 2-byte access */
// TODO: only used for JMP ind and for reading reset/brk vector; maybe unneccessary
#   define LOAD_ADDR16(a)       ((LOAD(((a)+1)&0xffff)<<8)+LOAD(a))
    /* mostly used in the form LOADEXEC(PC+1), so we want to ensure that
       the parameter never exceeds it's 16 bit range. */
#   define LOADEXEC(a)          LOAD((a) & 0xffff)
#   define LOADEXEC16(a)            (LOADEXEC ((a)&0xffff) | (LOADEXEC (((a)+1)&0xffff) << 8))
    /* handle wraparound inside zeropage */
#   define LOAD_ZPG_ADDR16(a)   (LOAD_ZPG(a) | (LOAD_ZPG(((a)+1) & 0xff) << 8))
    /* no special handling for executable memory/stack/zeropage. LOAD/STORE will be used. */

#else /* use OPT_CPU setting */

    // assuming no wraparound on indexed addressing modes
# if OPT_CPU == 0
    /* for indexed indirect addressing modes */
#   define LOAD_ZPG_ADDR16(a)   (LOAD_ZPG(a) | (LOAD_ZPG(((a)+1) & 0xff) << 8))
#   define LOADEXEC(a)              ( ((a) & 0x1000) \
                                      ? theRom[(a) & 0xfff] : theRam[(a) & 0x7f] )
/* on the atari, the stack page is a mirror of page 0 */
#   define LOAD_STACK(a)        LOAD_ZPG((a))
#   define STORE_STACK(a, b)    STORE_ZPG((a), (b))

# else /* OPT_CPU >= 1 */
#   define LOAD_ZPG(a)          (((a) & 0x80) ? theRam[(a) & 0x7f] : read_tia(a))
#   define STORE_ZPG(a,b)       (((a) & 0x80) ? (theRam[(a) & 0x7f]=(b)) : write_tia((a), (b)))
#   define LOAD_ZPG_ADDR16(a)   (theRam[(a) & 0x7f] + (theRam[((a)+1) & 0x7f] << 8))
// only JMP indirect or jump through reset/brk vector. assume always rom or ram.
#   define LOAD_ADDR16(a)       LOADEXEC16(a)
#  if OPT_CPU >= 2
    // assuming code is only run in rom, never in ram! (may break some games)
#   define LOADEXEC(a)              (theRom[(a) & 0xfff])
#  else
#   define LOADEXEC(a)            ( ((a) & 0x1000) ? theRom[(a) & 0xfff] : theRam[(a) & 0x7f] )
#  endif
#   define LOADEXEC16(a)            (LOADEXEC (a) | (LOADEXEC ((a)+1) << 8))
# endif /* OPT_CPU */
#  if OPT_CPU >= 2
    // assuming push/pull operates only on ram. Some games are incompatible, e.g. Atlantis
#   define LOAD_STACK(a)        (theRam[(a) & 0x7f])
#   define STORE_STACK(a, b)    (theRam[(a) & 0x7f] = (b))
#  else
#   define LOAD_STACK(a)        LOAD_ZPG((a))
#   define STORE_STACK(a, b)    STORE_ZPG((a), (b))
#  endif
#endif /* OPT_CPU / OPT_CPU_PARANOID */

#endif // **** remove for CPU test

/* -------------------------------------------------------------------------
 * debugging & profiling stuff
 */

#define UNDOCUMENTED_OPCODE()   \
    TST_PROFILE(PRF_CPU_UNDOC_OPCODE, "cpu undocumented opcode usage");

#define ILLEGAL_INSTRUCTION()   \
    DEBUGF("***WARNING: illegal opcode @ clock %d/PC %4x\n", clk, PC); \
    TST_PROFILE(PRF_CPU_UNDOC_OPCODE, "cpu undocumented opcode usage");



/* =========================================================================
 * ensure that defines are defined the right way
 */

#if defined(CPU_VARIANT_65C02_WDC)
#   define CPU_VARIANT_65C02_ROCKWELL
#endif
#if defined(CPU_VARIANT_65C02_ROCKWELL)
#   define CPU_65C02
#endif
#if defined(CPU_65C02)
#   define CLEAR_D_ON_INTERRUPT
#   define CORRECT_BCD_FLAGS
#endif

#ifdef OPT_CPU_PARANOID
#   undef  OPT_CPU
#endif
/* set macro if no optimization level is defined */
#ifndef OPT_CPU
#  define OPT_CPU   0
#endif


/* =========================================================================
 * MEMORY ACCESS
 *
 * Memory is accessed via macros that must be provided by your main program.
 * The following macros are mandatory:
 *   -  LOAD(addr)
 *   -  STORE(addr, val)    Load or store an 8 bit value from anywhere
 * in the 16 bit address space. All possible side effects (like memory
 * mapped I/O, bank switching ...) must be considered. NOTE: If you
 * define OPT_CPU_PARANOID, memory accesses is solely proceeded by
 * this macros as they are considered to be the most basic and secure.
 *
 * The following macros are optional. They each address a subsection of
 * memory and can hold some optimization potential. If you don't define one
 * of these macros it's replaced by the basic LOAD resp. STORE.
 *   -  LOADEXEC(a)         Load byte addressed by the program counter,
 * i.e. instruction opcodes and operands. Usually programs doesn't run
 * inside I/O space, so side effects can be ignored.
 *   -  LOAD_ZPG(a)
 *   -  STORE_ZPG(a, b)     Zero page access. Address can be 0x00..0xff.
 *   -  LOAD_STACK(a)
 *   -  STORE_STACK(a, b)   Load from/store to address pointed to by the
 * stack pointer (address 0x100..0x1ff). ATTENTION: the address parameter
 * passed to the macro is the value of the SP register, NOT the actually
 * addressed memory space! The macro must do the address translation,
 * e.g. LOAD((a)+0x100)
 *
 * 16 bit macros: define the following macros, if you can somehow optimize
 * 16 bit memory reads and writes, e.g. if you deliberately disregard
 * wraparound access, or if your host system allows unaligned memory
 * access (and has the same endianness as the emulated 6502!). Remind
 * that this will also break wraparound access. For this reason it will be
 * deactivated if OPT_CPU_PARANOID is defined.
 *   -  LOAD_ADDR16(a)      Load a 16 bit value from anywhere in the address
 * space. ATM only used for JMP indirect and for reading reset/break vector.
 *   -  LOADEXEC16(a)       Load a 16 bit operand. Used for absolute
 * addressing mode instructions.
 *   -  LOAD_ZPG_ADDR16(a)  Load 16 bit address from zero page. Used for
 * zero page indirect addressing modes.
 */

/* -------------------------------------------------------------------------
 * Provide optional memory access macros (if not defined before)
 */

#ifdef OPT_CPU_PARANOID
    /* make sure only LOAD and STORE is used, prevent special handling of memory areas */
# if defined(LOADEXEC)
#   undef LOADEXEC
# endif
# if defined(LOAD_ZPG)
#   undef LOAD_ZPG
# endif
# if defined(STORE_ZPG)
#   undef STORE_ZPG
# endif
# if defined(LOAD_STACK)
#   undef LOAD_STACK
# endif
# if defined(STORE_STACK)
#   undef STORE_STACK
# endif
    /* always handle wraparound in the middle of a 2-byte access */
#   define LOAD_ADDR16(a)       ((LOAD(((a)+1)&0xffff)<<8)+LOAD(a))
    /* mostly used in the form LOADEXEC(PC+1), so we want to ensure that
       the parameter never exceeds it's 16 bit range. */
#   define LOADEXEC(a)          LOAD((a) & 0xffff)
#   define LOADEXEC16(a)            (LOADEXEC ((a)&0xffff) | (LOADEXEC (((a)+1)&0xffff) << 8))
    /* handle wraparound inside zeropage */
#   define LOAD_ZPG_ADDR16(a)   (LOAD_ZPG(a) | (LOAD_ZPG(((a)+1) & 0xff) << 8))
    /* no special handling for executable memory/stack/zeropage. LOAD/STORE will be used. */
#endif /* OPT_CPU_PARANOID */

/* provide missing macros */
# if !defined(LOADEXEC)
#   define LOADEXEC(a)          LOAD(a)
# endif
# if !defined(LOAD_ZPG)
#   define LOAD_ZPG(a)          LOAD(a)
# endif
# if !defined(STORE_ZPG)
#   define STORE_ZPG(a, b)      STORE((a), (b))
# endif
# if !defined(LOAD_STACK)
#   define LOAD_STACK(a)        LOAD((a) + 0x100)
# endif
# if !defined(STORE_STACK)
#   define STORE_STACK(a, b)    STORE((a) + 0x100, (b))
# endif
/* 16 bit access */
# if !defined(LOAD_ADDR16)
#  if OPT_CPU > 0
#   define LOAD_ADDR16(a)       ((LOAD((a)+1) << 8) | LOAD(a))
#  else
#   define LOAD_ADDR16(a)       ((LOAD(((a)+1) & 0xffff) << 8) | LOAD(a))
#  endif
# endif
# if !defined(LOADEXEC16)
#  if OPT_CPU > 0
#   define LOADEXEC16(a)        (LOADEXEC(a) | (LOADEXEC((a)+1) << 8))
#  else
#   define LOADEXEC16(a)        (LOADEXEC(a) | (LOADEXEC(((a)+1) & 0xffff) << 8))
#  endif
# endif
# if !defined(LOAD_ZPG_ADDR16)
#  if OPT_CPU > 0
#   define LOAD_ZPG_ADDR16(a)   (LOAD_ZPG(a) | (LOAD_ZPG((a)+1) << 8))
#  else
#   define LOAD_ZPG_ADDR16(a)   (LOAD_ZPG(a) | (LOAD_ZPG(((a)+1) & 0xff) << 8))
#  endif
# endif

/* =========================================================================
 * The following macros must be provided by your emulator main part:
 *   -  CLOCK_CYCLES(n)     Advance the system clock by n machine cycles.
 * You must define this macro if your code has to be aware of elapsed
 * machine cycles.
 *   -  CLOCK_CYCLES_INC()  Increase the elapsed machine cycle count by one.
 * Only used for the flag-corrected versions of the ADC and SBC instruction
 * (like done on CMOS 65C02).
 *   -  CLOCK_CYCLES_STOPPED()  If the CPU loop is the only place where you
 * count up the system time you may like to use this macro, so your system
 * can reach your other time controlled tasks even if the CPU is stopped.
 * This macro is executed if (1) the CPU has reached an "illegal
 * instruction" (NMOS only) or (2) the CPU "executes" the STP or WAI
 * instruction (WDC variant only).
 */

#ifndef CLOCK_CYCLES
#   define CLOCK_CYCLES(n)
#endif
#ifndef CLOCK_CYCLE_INC
#   define CLOCK_CYCLE_INC()
#endif
#ifndef CLOCK_CYCLES_STOPPED
#   define CLOCK_CYCLES_STOPPED()
#endif

/* =========================================================================
 * More macros that you can use from your main routine.
 * Intended for debugging and profiling.
 *   -  UNDOCUMENTED_OPCODE()  This macro is called whenever an undocumented
 * instruction is executed. If CMOS variant (65C02) is emulated, this macro
 * is called upon unused instruction NOPs (i.e. all NOPs except $EA)
 *   -  UNSTABLE_INSTRUCTION() Some of the undocumented instruction are
 * unstable, i.e. give unpredictable results. This macro is called
 * if one of these gets executed.
 *   -  ILLEGAL_INSTRUCTION()  This macro is called if the cpu tries to
 * execute an illegal instruction (NMOS variant only). Illegal instructions
 * will halt the cpu.
 */

#ifndef UNDOCUMENTED_OPCODE
#   define UNDOCUMENTED_OPCODE()
#endif
#ifndef UNSTABLE_INSTRUCTION
#   define UNSTABLE_INSTRUCTION()
#endif
#ifndef ILLEGAL_INSTRUCTION
#   define ILLEGAL_INSTRUCTION()
#endif

/* =========================================================================
 * CPU INTERNALS
 */

/* -------------------------------------------------------------------------
 *  6502 cpu state
 *  space for saving registers, flags, etc.
 */

struct cpu cpu;

// cpu states. TODO!
#define CPU_STATE_NONE      0x00
#define CPU_STATE_ILLEGAL   0x01    // illegal instruction
#define CPU_STATE_INSANE    0x02    // register or stack storage overflow
#define CPU_STATE_INTERRUPT 0x04    // not used yet
#define CPU_STATE_STOPPED   0x08    /* CPU halted */

/* -------------------------------------------------------------------------
 *  Registers and Flags (shortcuts for convenience)
 */
#define PC              cpu.program_counter
#define AC              cpu.accumulator
#define XR              cpu.x_register
#define YR              cpu.y_register
#define SP              cpu.stack_pointer

#define CF              cpu.carry_flag
#define ZF              cpu.zero_flag
#define IF              cpu.interrupt_flag
#define DF              cpu.decimal_flag
#define VF              cpu.overflow_flag
#define NF              cpu.negative_flag

/* -------------------------------------------------------------------------
 * Flag Handling
 *
 * ALWAYS use this macros, NEVER access flags directly! (Some flags are
 * saved and used in a non-obvious way for optimization reasons)
 *
 * Not all of the following macros are available for all flags.
 * Replace 'FLAG' with the actual flag name.
 *   -  SET_FLAG()          Should be self-explaining
 *   -  CLEAR_FLAG()
 *   -  IS_FLAG()           returns true (anything but 0) if flag is set,
 * false (0) if flag is cleared.
 *   -  FLAG_TO_SR()        translate the flag in a way it can directly ORed
 * to the status register.
 *   -  Bn_TO_FLAG(x)       evaluate bit n of x and set flag accordingly.
 *   -  FLAG_TO_Bn()        result is 0 or 1<<n, depending on flag status.
 *   -  EVAL_FLAG(x)        evaluate expression (x) and set flag accordingly.
 * Only available for zero, negative and carry flag.
 */

/* ZERO FLAG. actually stored as not-zero */
#define SET_ZERO()              (ZF = 0)
#define CLEAR_ZERO()            (ZF = 1)
#define IS_ZERO()               (!ZF)
#define ZERO_TO_SR()            (ZF ? 0 : 0x02)
#define B1_TO_ZERO(x)           ((x) & 0x02 ? SET_ZERO() : CLEAR_ZERO())
#define EVAL_ZERO(x)            (ZF = (x))

#if 1
/* NEGATIVE FLAG. stored as whole byte, evaluated on read */
#define SET_NEGATIVE()          (NF = 0x80)
#define CLEAR_NEGATIVE()        (NF = 0)
#define IS_NEGATIVE()           (NF & 0x80)
#define NEGATIVE_TO_SR()        (NF & 0x80)
#define B7_TO_NEGATIVE(x)       (NF = (x) & 0x80)
#define EVAL_NEGATIVE(x)        (NF = (x))
#else
/* NEGATIVE FLAG. stored in bit 7 (like SR) */
#define SET_NEGATIVE()          (NF = 0x80)
#define CLEAR_NEGATIVE()        (NF = 0)
#define IS_NEGATIVE()           (NF)
#define NEGATIVE_TO_SR()        (NF)
#define B7_TO_NEGATIVE(x)       (NF = (x) & 0x80)
#define EVAL_NEGATIVE(x)        (NF = (x) & 0x80)
#endif

/* CARRY FLAG. stored in bit 0 (like SR) */
#define SET_CARRY()             (CF = 0x01)
#define CLEAR_CARRY()           (CF = 0)
#define IS_CARRY()              (CF)
#define CARRY_TO_SR()           (CF)
#define CARRY_TO_B0()           (CF)
#define CARRY_TO_B7()           (CF << 7)
#define CARRY_TO_B8()           (CF << 8)
#define B0_TO_CARRY(x)          (CF = (x) & 0x01)
#define B7_TO_CARRY(x)          (CF = ((x) >> 7) & 0x01)
#define EVAL_CARRY(x)           (CF = (x) >> 8) /* move the carry from bit 8 into the
                                     Carry Flag. Note that the value must be unsigned
                                      and is not allowed to be greater than 9 bit! */

/* OVERFLOW FLAG. stored in bit 6 (like SR) */
#define SET_OVERFLOW()          (VF = 0x40)
#define CLEAR_OVERFLOW()        (VF = 0)
#define IS_OVERFLOW()           (VF)
#define B6_TO_OVERFLOW(x)       (VF = (x) & 0x40)
#define B7_TO_OVERFLOW(x)       (VF = ((x) >> 1) & 0x40) /* for ADC, SBC */
#define OVERFLOW_TO_SR()        (VF)

/* INTERRUPT DISABLE FLAG. stored in bit 2 (like SR) */
#define SET_INTERRUPT()         (IF = 0x04)
#define CLEAR_INTERRUPT()       (IF = 0)
#define IS_INTERRUPT()          (IF)
#define INTERRUPT_TO_SR()       (IF)
#define B2_TO_INTERRUPT(x)      (IF = (x) & 0x04)

/* DECIMAL MODE FLAG. stored in bit 3 (like SR) */
#define SET_DECIMAL()           (DF = 0x08)
#define CLEAR_DECIMAL()         (DF = 0)
#define IS_DECIMAL()            (DF)
#define DECIMAL_TO_SR()         (DF)
#define B3_TO_DECIMAL(x)        (DF = (x) & 0x08)

/* The Status Register as a single entity is created "on the fly" from the
   individual flags. */

/* Compose Status Register with BREAK bit set (the normal case) */
#define SETUP_SR()          (  0x20 /* unused */ \
                             | 0x10 /* BRK bit */ \
                             | CARRY_TO_SR() \
                             | ZERO_TO_SR() \
                             | INTERRUPT_TO_SR() \
                             | DECIMAL_TO_SR() \
                             | OVERFLOW_TO_SR() \
                             | NEGATIVE_TO_SR() \
                            )

/* Compose Status Register with BREAK bit clear (an interrupt happened) */
#define SETUP_SR_WO_BRK()   (  0x20 \
                             | CARRY_TO_SR() \
                             | ZERO_TO_SR() \
                             | INTERRUPT_TO_SR() \
                             | DECIMAL_TO_SR() \
                             | OVERFLOW_TO_SR() \
                             | NEGATIVE_TO_SR() \
                            )

#define BREAKDOWN_SR(x)     do { \
                             B0_TO_CARRY(x); \
                             B1_TO_ZERO(x); \
                             B2_TO_INTERRUPT(x); \
                             B3_TO_DECIMAL(x); \
                             B6_TO_OVERFLOW(x); \
                             B7_TO_NEGATIVE(x); \
                            } while (0)

/* -------------------------------------------------------------------------
 *  Reset and Interrupt-Vectors
 */
#define NMI_VECTOR      0xfffa
#define RESET_VECTOR    0xfffc
#define BRK_VECTOR      0xfffe      /* This vector serves both IRQ & BRK, hence */
#define IRQ_VECTOR      0xfffe      /* two different names for the same thing */


/* =========================================================================
 * Macros for instruction operand fetching and instruction execution.
 * See comments on how optimization setting affects correctness of emulation.
 */

/* -------------------------------------------------------------------------
 * This should always give the most stable and secure emulation.
 */
#ifdef OPT_CPU_PARANOID /* should give the most stable and secure emulation */
#   define FETCH(addr)              LOADEXEC((addr) & 0xffff)
#   define FETCH_ADDR_ZPG()         FETCH(PC+1)
#   define FETCH_ADDR_ZPG_X()       ( (FETCH_ADDR_ZPG() + XR) & 0xff )
#   define FETCH_ADDR_ZPG_Y()       ( (FETCH_ADDR_ZPG() + YR) & 0xff )
#   define FETCH_ADDR_ABS()         (LOADEXEC16((PC+1) & 0xffff))
#   define FETCH_ADDR_ABS_X()       ( (FETCH_ADDR_ABS() + XR) & 0xffff)
#   define FETCH_ADDR_ABS_Y()       ( (FETCH_ADDR_ABS() + YR) & 0xffff)
#   define FETCH_ADDR_X_INDIRECT()  ({unsigned f_ix_=FETCH(PC+1);\
                                     LOAD_ZPG_ADDR16((f_ix_ + XR) & 0xff); })
#   define FETCH_ADDR_INDIRECT_Y()  ({unsigned f_iy_=FETCH(PC+1);\
                                     f_iy_=LOAD_ZPG_ADDR16(f_iy_) + YR; f_iy_ & 0xffff;})
    /* only available with 65C02 */
#   define FETCH_ADDR_ZPG_INDIRECT()  ({unsigned f_ix_=FETCH(PC+1); \
                                     LOAD_ZPG_ADDR16(f_ix_ & 0xff); })
    /* relative jumps: calculate jumps target address. NOTE: the passed "PC" value
       must be the address of the instruction following the branch, i.e. PC+2 */
#   define REL_ADDR(pc,rel)         (((pc)+((SIGNED_BYTE)(rel))) & 0xffff)
    /* handling the program counter itself */
#   define ADVANCE_PC(steps)        (PC = (PC + (steps)) & 0xffff)

/* Stack access: Handle both Stack Pointer and addressed memory. */
/* NOTE: uses gcc extensions! */
#   define PUSH(b)              {STORE_STACK(SP, (b));SP = (SP-1) & 0xff;}
#   define PULL()               ({SP = (SP+1) & 0xff;LOAD_STACK(SP);})
#   define PUSH16(b)            {PUSH((b) >> 8); PUSH((b) & 0xff);}
#   define PULL16()             ({unsigned result_=PULL(); result_ = result_ | (PULL() << 8)})

/* -------------------------------------------------------------------------
 * The most optimized setting. This may break emulation on some programs.
 * The following optimizations are made additional to the OPT_CPU==1 setting:
 */
#elif OPT_CPU >= 2

/* Stack access: Handle both Stack Pointer and addressed memory. */
/* assumes SP always targets ram. Some games will not run, e.g. Atlantis */
# if 1
#   define PUSH(b)              {STORE_STACK(SP, (b));SP--;}
# else
#   define PUSH(b)              {\
        if(SP<0x80){DEBUGF("PUSH mit SP=%x\n", SP);} \
        STORE_STACK(SP, (b));SP--;}
# endif
# if 1
#   define PULL()               ({++SP; LOAD_STACK(SP);})
# else
#   define PULL()  ({\
                if(SP<0x80){DEBUGF("PUSH mit SP=%x\n", SP);} \
                ++SP; LOAD_STACK(SP);})
# endif

/* -------------------------------------------------------------------------
 * Slightly optimized setting. This may break emulation on some programs.
 * Apart from the already 'optimized' assumptions from OPT_CPU==0 the
 * following things are changed:
 * - the absolute indexed addressing modes (abs,X and abs,Y) MUST NOT
 *   wraparound, i.e. it's assumed that [abs_addr+X] is NEVER > 0xffff
 * - same for the indirect indexed addressing mode ((ind),Y): it's assumed
 *   that the absolute address + Y NEVER exceeds 0xffff
 * - relative branches must not wraparound, i.e. branch from PC > 0xff80 to
 *   zero page, or from zero page back to the last page.
 */

#elif OPT_CPU >= 1

#   define FETCH_ADDR_ABS_X()       (FETCH_ADDR_ABS() + XR)
#   define FETCH_ADDR_ABS_Y()       (FETCH_ADDR_ABS() + YR)
#   define FETCH_ADDR_INDIRECT_Y()  ({unsigned f_iy_=LOADEXEC(PC+1);\
                                     LOAD_ZPG_ADDR16(f_iy_) + YR; })
#   define REL_ADDR(pc,rel)         ((pc)+((SIGNED_BYTE)(rel)))
    /* PC must not wraparound! */
#   define ADVANCE_PC(steps)        (PC += (steps))
/* Stack access: Handle both Stack Pointer and addressed memory. */
#   define PUSH(b)              {STORE_STACK(SP, (b));SP--;}
#   define PULL()               ({++SP; LOAD_STACK(SP);})

#   define PUSH16(w)            {theRam[SP & 0x7f] = (w) >> 8; \
                                 theRam[(SP & 0x7f)-1] = (w) & 0xff; SP -= 2;}
#   define PULL16()             ({unsigned result_=PULL(); result_ = result_ | (PULL() << 8)})

#endif /* OPT_CPU > 0 */

/* -------------------------------------------------------------------------
 * No optimization. This also defines the standard behaviour, i.e. all
 * macros that don't exist in an "optimized" form.
 * Some things are 'optimized' in relation to _PARANOID setting, that may
 * be problematic in some very special cases:
 * - PC must not wraparound inside an instruction, i.e. opcode at $FFFF and
 *   operand at $0000
 */

    /* NOTE: instructions must not wraparound address space! */
#if !defined(FETCH)
#   define FETCH(addr)              LOADEXEC(addr)
#endif
#if !defined(FETCH_ADDR_ZPG)
#   define FETCH_ADDR_ZPG()         FETCH(PC+1)
#endif
#if !defined(FETCH_ADDR_ZPG_X)
#   define FETCH_ADDR_ZPG_X()       ( (FETCH_ADDR_ZPG() + XR) & 0xff )
#   define FETCH_ADDR_ZPG_Y()       ( (FETCH_ADDR_ZPG() + YR) & 0xff )
#endif
#if !defined FETCH_ADDR_ZPG_INDIRECT
#   define FETCH_ADDR_ZPG_INDIRECT()  ({unsigned f_ix_=LOADEXEC((PC+1) & 0xffff);\
                                     LOAD_ZPG_ADDR16(f_ix_ & 0xff); })
#endif
#if !defined(FETCH_ADDR_ABS)
    /* must not wraparound! */
#   define FETCH_ADDR_ABS()         (LOADEXEC16(PC+1))
#endif
#ifndef FETCH_ADDR_ABS_X
#   define FETCH_ADDR_ABS_X()       ( (FETCH_ADDR_ABS() + XR) & 0xffff)
#endif
#ifndef FETCH_ADDR_ABS_Y
#   define FETCH_ADDR_ABS_Y()       ( (FETCH_ADDR_ABS() + YR) & 0xffff)
#endif

#ifndef FETCH_ADDR_X_INDIRECT
#   define FETCH_ADDR_X_INDIRECT()  ({unsigned f_ix_=LOADEXEC(PC+1);\
                                     LOAD_ZPG_ADDR16((f_ix_ + XR) & 0xff); })
#endif
#ifndef FETCH_ADDR_INDIRECT_Y
#   define FETCH_ADDR_INDIRECT_Y()  ({unsigned f_iy_=LOADEXEC(PC+1);\
                                     LOAD_ZPG_ADDR16(f_iy_) + YR; })
#endif
    /* relative jumps: calculate jumps target address. NOTE: the passed "PC" value
       must be the address of the instruction following the branch, i.e. PC+2 */
#ifndef REL_ADDR
#   define REL_ADDR(pc,rel)         ((pc)+((SIGNED_BYTE)(rel)))
#endif

#if !defined(ADVANCE_PC)
#   define ADVANCE_PC(steps)        (PC = (PC + (steps)) & 0xffff)
#endif

/* stack access: The following macros handle both the memory access
 * and the Stack Pointer value. */
/* Stack access: Handle both Stack Pointer and addressed memory. */
/* SP must not wraparound! */
#if !defined(PUSH)
#   define PUSH(b)              {STORE_STACK(SP, (b));SP--;}
#endif
#if !defined(PULL)
#   define PULL()               ({++SP; LOAD_STACK(SP);})
#endif
#if !defined(PUSH16)
#   define PUSH16(w)            {PUSH((w) >> 8); PUSH((w) & 0xff);}
#endif
#if !defined(PULL16)
#   define PULL16()             ({unsigned result_=PULL(); result_ = result_ | (PULL() << 8)})
#endif


/* =================================================================== */


/* Initialise the CPU */
void init_cpu (void) COLD_ATTR;
void init_cpu (void)
{
    BREAKDOWN_SR(0);    /* clear all flags ... */
    SET_INTERRUPT();    /* start with interrupts disabled */
    AC = 0;
    XR = 0;
    YR = 0;
    SP = 0xff;
    cpu.state = CPU_STATE_NONE;
    PC = LOAD_ADDR16(RESET_VECTOR);
}


/* =========================================================================
 * To reduce code duplication the instruction definitions make heavy use
 * of macros. There are 3 types of macros:
 *   -  Macros that replace an entire instruction
 *   -  Macros for addressing modes. These macros are divided in the PRE
 * part that must be executed before the operation, and the POST part.
 *   -  Macros that define the operation.
 * Note that this macros define local variables (addr and op) and should
 * therefore be used inside braces.
 * Note that for all instruction definitions the clock cycles are counted
 * at the beginning. This is because the last instruction cycle is most
 * often the decisive for memory access, and your system's memory mapped
 * I/O system may rely on it.
 * The PC gets advanced at the end of the instruction, so that outside code
 * can follow which instruction is executed just now.
 */

/* =========================================================================
 * Macros for specific instructions
 */

/* -------------------------------------------------------------------------
 * Conditional branch
 * opcodes 10,30,50,70,90,B0,D0,F0
 * opcodes 80 (65C02; unconditional branch)
 */

#define INSTRUCTION_BRANCH(condition) \
    {  \
        unsigned op = FETCH(PC + 1); \
        ADVANCE_PC(2); \
        if (condition) { \
            if ((REL_ADDR(PC, op) ^ PC) >> 8) /* page crossing? */ \
                CLOCK_CYCLES(4); \
            else \
                CLOCK_CYCLES(3); \
            PC = REL_ADDR(PC, op); \
        } \
        else { \
            CLOCK_CYCLES(2); \
        } \
    }


/* -------------------------------------------------------------------------
 * Stack operations
 */

/*
 * Push: PHP, PHA; PHY, PHX (65C02)
 * First push item onto stack and then decrement SP.
 * opcodes 08, 48; 5A, DA (65C02)
 */
#define INSTRUCTION__PUSH(value) \
    { \
        CLOCK_CYCLES(3); \
        PUSH(value); \
        ADVANCE_PC(1); \
    }


/*
 * Pull: PLA; PLY, PLX (65C02)
 * First increment stack pointer and then pull item from stack.
 * opcodes 68; 7A, FA (65C02)
 * NOTE: this macro can NOT be used with the PLP instruction, because the
 * flags are handled differently! (No flag evaluation)
 */
#define INSTRUCTION__PULL(reg) \
    { \
        CLOCK_CYCLES(4); \
        reg = PULL(); \
        EVAL_ZERO(reg); \
        EVAL_NEGATIVE(reg); \
        ADVANCE_PC(1); \
    }

/* -------------------------------------------------------------------------
 * Register and flag instructions
 */

/*
 * transfer register
 * TXA,TYA,TAY,TAX,TSX
 * opcodes 8A,98,A8,AA,BA
 * NOTE: don't use for TXS (TSX doesn't set flags)
 */
#define INSTRUCTION_TRF(source, dest) \
    { \
        CLOCK_CYCLES(2); \
        dest = (source); \
        EVAL_ZERO(dest); \
        EVAL_NEGATIVE(dest); \
        ADVANCE_PC(1); \
    }

/*
 * set/reset flag: CLC, SEC, CLI, SEI, CLV, CLD, SED
 * opcodes 18,38,58,78,B8,D8,F8
 */
#define INSTRUCTION_FLAG(operation) \
    { \
        CLOCK_CYCLES(2); \
        operation; \
        ADVANCE_PC(1); \
    }

/* -------------------------------------------------------------------------
 * Other instructions
 */

/*
 * undocumented/unused opcode NOP
 * Note that the original NMOS 6502 undocumented NOPs can have side effects
 * (i.e. memory read access); if you want to emulate this, don't use this macro!
 * Note that these side effects sometimes cause a variation in clock cycles
 * (e.g. page crossing reads).
 */
#define INSTRUCTION_UNOP(size, cycles) \
    { \
        CLOCK_CYCLES(cycles); \
        ADVANCE_PC(size); \
    }


/* --------------------------------------------------
 * instructions for Rockwell and WDC 65C02 variants
 */

/*
 * SMBn zpg - set memory bit
 */
#define INSTRUCTION__SMB(bit) \
    { \
        CLOCK_CYCLES(5); \
        unsigned addr = FETCH_ADDR_ZPG(); \
        unsigned op = LOAD_ZPG(addr); \
        op = op | (1 << bit); \
        STORE_ZPG(addr, op); \
        ADVANCE_PC(2); \
    }

/*
 * RMBn zpg - reset memory bit
 */
#define INSTRUCTION__RMB(bit) \
    { \
        CLOCK_CYCLES(5); \
        unsigned addr = FETCH_ADDR_ZPG(); \
        unsigned op = LOAD_ZPG(addr); \
        op = op & ~(1 << bit); \
        STORE_ZPG(addr, op); \
        ADVANCE_PC(2); \
    }

/*
 * BBSn zpg, rel
 * branch if bit set
 */
#define INSTRUCTION__BBS(bit) \
    { \
        CLOCK_CYCLES(5); \
        unsigned addr = FETCH(PC + 1); \
        unsigned op = LOAD_ZPG(addr); \
        op &= 1 << bit; \
        unsigned br = FETCH(PC + 2); \
        ADVANCE_PC(3); \
        if (op) \
            PC = REL_ADDR(PC, br); \
    }


/*
 * BBRn zpg, rel
 * branch if bit reset
 */
#define INSTRUCTION__BBR(bit) \
    { \
        CLOCK_CYCLES(5); \
        unsigned addr = FETCH(PC + 1); \
        unsigned op = LOAD_ZPG(addr); \
        op &= 1 << bit; \
        unsigned br = FETCH(PC + 2); \
        ADVANCE_PC(3); \
        if (!op) \
            PC = REL_ADDR(PC, br); \
    }


/* =========================================================================
 * Addressing Mode macros
 * These are split in two parts: first the PRE_ADDRMODE(), then follows the
 * operation (or the operation macro), then POST_ADDRMODE().
 * The PRE_.. macro declares a (local) variable 'addr', assigned with the
 * effective address of the operand. It also declares 'op' which is loaded
 * with the operand.
 * There are some variants to this macros:
 * The PRE_..._NOLOAD() macro doesn't declare and load 'op' (but has the
 * effective address)
 * The POST_STORE(val) macro stores val back to the effective address.
 * Macros named PRE_.._VC() emulate instructions with a variable
 * cycle count.
 */

/* -------------------------------------------------------------------------
 * register and immediate
 */

#define PRE_REGISTER(register) \
        CLOCK_CYCLES(2); \
        unsigned op = register;

#define POST_REGISTER_STORE(register) \
        register = op; \
        ADVANCE_PC(1);

#define PRE_IMMEDIATE() \
        CLOCK_CYCLES(2); \
        unsigned op = FETCH(PC + 1);

#define POST_IMMEDIATE() \
        ADVANCE_PC(2);

/* -------------------------------------------------------------------------
 * zero page addressing (zpg and zpg indexed)
 */

/* zpg */
#define PRE_ZPG(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ZPG(); \
        unsigned op = LOAD_ZPG(addr);

#define PRE_ZPG_NOLOAD(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ZPG();

#define POST_ZPG_STORE(val) \
        STORE_ZPG(addr, val); \
        ADVANCE_PC(2);

#define POST_ZPG() \
        ADVANCE_PC(2);

/* zpg,X / zpg,Y */
#define PRE_ZPGXY_NOLOAD(cycles, reg) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ZPG_##reg();

#define PRE_ZPGXY(cycles, reg) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ZPG_##reg(); \
        unsigned op = LOAD_ZPG(addr);

#define POST_ZPGXY_STORE(val) \
        STORE_ZPG(addr, val); \
        ADVANCE_PC(2);

#define POST_ZPGXY() \
        ADVANCE_PC(2);

/* -------------------------------------------------------------------------
 * absolute addressing (abs and abs,indexed)
 */

#define PRE_ABS(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ABS(); \
        unsigned op = LOAD(addr);

#define PRE_ABS_NOLOAD(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ABS();

#define POST_ABS_STORE(val) \
        STORE(addr, val); \
        ADVANCE_PC(3);

#define POST_ABS() \
        ADVANCE_PC(3);

/* abs,X / abs,Y */
/* variable cycle count */
#define PRE_ABS_XY_VC(cycles, reg) \
        unsigned addr = FETCH_ADDR_ABS_##reg(); \
        if (((addr - reg##R) ^ addr) >> 8) \
            CLOCK_CYCLES((cycles)+1); \
        else \
            CLOCK_CYCLES(cycles); \
        unsigned op = LOAD(addr);

/* fixed cycle count */
#define PRE_ABS_XY(cycles, reg) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ABS_##reg(); \
        unsigned op = LOAD(addr);

#define PRE_ABS_XY_NOLOAD(cycles, reg) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_ABS_##reg();

#define POST_ABS_XY() \
        ADVANCE_PC(3);

#define POST_ABS_XY_STORE(val) \
        STORE(addr, val); \
        ADVANCE_PC(3);

/* -------------------------------------------------------------------------
 * indirect addressing
 */

/* zero page indirect (zp)
 * only available on 65C02 (opcodes 12,32,52,72,92,B2,D2,F2) */
#define PRE_ZPG_IND() \
        CLOCK_CYCLES(5); \
        unsigned addr = FETCH_ADDR_ZPG_INDIRECT(); \
        unsigned op = LOAD(addr);

#define PRE_ZPG_IND_NOLOAD() \
        CLOCK_CYCLES(5); \
        unsigned addr = FETCH_ADDR_ZPG_INDIRECT();

#define POST_ZPG_IND_STORE(val) \
        STORE(addr, val); \
        ADVANCE_PC(2);

#define POST_ZPG_IND() \
        ADVANCE_PC(2);

/* indexed indirect (zpg,X) */
#define PRE_X_IND(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_X_INDIRECT();  \
        unsigned op = LOAD(addr);

#define PRE_X_IND_NOLOAD(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_X_INDIRECT();

#define POST_X_IND_STORE(val) \
        STORE(addr, val); \
        ADVANCE_PC(2);

#define POST_X_IND() \
        ADVANCE_PC(2);

/* indirect indexed (zpg),Y */
#define PRE_IND_Y(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_INDIRECT_Y(); \
        unsigned op = LOAD(addr);

#define PRE_IND_Y_NOLOAD(cycles) \
        CLOCK_CYCLES(cycles); \
        unsigned addr = FETCH_ADDR_INDIRECT_Y();

#define PRE_IND_Y_VC(cycles) \
        unsigned addr = FETCH_ADDR_INDIRECT_Y(); \
        if (((addr - YR) ^ addr) >> 8) \
            CLOCK_CYCLES((cycles)+1);   /* Over page */ \
        else \
            CLOCK_CYCLES(cycles);   /* Same page */ \
        unsigned op = LOAD(addr);

#define POST_IND_Y_STORE(val) \
        STORE(addr, val); \
        ADVANCE_PC(2);

#define POST_IND_Y() \
        ADVANCE_PC(2);


/* =========================================================================
 * Operation macros
 * Note that although some operations always require a writeback to memory,
 * this is not included into the macro, because we use specialized write
 * macros (e.g. for zeropage) for optimization reasons.
 */

/* -------------------------------------------------------------------------
 * Load operation
 */

/* LD */
#define OP_LD(operand, destination) \
        destination = (operand); \
        EVAL_ZERO(operand); \
        EVAL_NEGATIVE(operand);


/* -------------------------------------------------------------------------
 * logical operations
 */

/* AND
 * A AND op -> A (Z,N) (opcodes 21,25,29,2D,31,35,39,3D) */
#define OP_AND(operand) \
        operand &= AC; \
        AC = (operand); \
        EVAL_ZERO(operand); \
        EVAL_NEGATIVE(operand);

/* ORA
 * A OR op -> A (Z,N) (opcodes 01,05,09,0D,11,15,19,1D) */
#define OP_ORA(operand) \
        operand |= AC; \
        AC = (operand); \
        EVAL_ZERO(operand); \
        EVAL_NEGATIVE(operand);

/* EOR
 * A XOR op -> A (Z,N) (opcodes 41,45,49,4D,51,55,59,5D) */
#define OP_EOR(operand) \
        operand ^= AC; \
        AC = (operand); \
        EVAL_ZERO(operand); \
        EVAL_NEGATIVE(operand);

/* -------------------------------------------------------------------------
 * bit shift operations (ASL, ASR, LSL, LSR, ROR, ROL)
 */

/* ASL
 * C <- [76543210] <- 0 (Z,N) (opcodes 06,0A,0E,16,1E) */
#define OP_ASL(op) \
        op <<= 1;   \
        EVAL_CARRY(op);  /* evaluate bit 8 */ \
        op &= 0xff;  \
        EVAL_ZERO(op); \
        EVAL_NEGATIVE(op);

/* LSR
 * 0 -> [76543210] -> C (Z,N) (opcodes 46,4A,4E,56,5E) */
/* Helper function, also used for undocumented LSE etc. */
#define LSR_(op)  \
        B0_TO_CARRY(op);    \
        op >>= 1;

/* regular LSR instruction */
#define OP_LSR(op)  \
        LSR_(op); \
        EVAL_ZERO(op); \
        EVAL_NEGATIVE(op);

/* ROR
 * C -> [76543210] -> C (Z,N) (opcodes 66,6A,6E,76,7E) */
// ROR helper function: this is also used for undocumented RRA
#define ROR_(op)  \
        op |= CARRY_TO_B8(); \
        B0_TO_CARRY(op);   \
        op >>= 1;

/* regular ROR instructions with flag evaluation */
#define OP_ROR(op)  \
        ROR_(op); \
        EVAL_ZERO(op); \
        EVAL_NEGATIVE(op);

/* ROL
 * C <- [76543210] <- C (Z,N) (opcodes 26,2A,2E,36,3E) */
// Helper function, also used for undocumented RLA
// rotate left operand, set carry accordingly.
// caller must save away the result and evaluate Z and S flag.
#define ROL_(op)  \
        op <<= 1;               \
        op |= CARRY_TO_B0();    \
        EVAL_CARRY(op);           \
        op &= 0xff;

/* regular function, w/flag evaluation */
#define OP_ROL(op)   \
        ROL_(op);  \
        EVAL_ZERO(op); \
        EVAL_NEGATIVE(op);


/* -------------------------------------------------------------------------
 * compare and bit test instructions: CMP/CPX/CPY, BIT, TSB, TRB
 */

/* CMP / CPX / CPY */
#define OP_CMP(register, op)  \
        op = register + 0x100 - op; \
        EVAL_CARRY(op); \
        op &= 0xff; \
        EVAL_NEGATIVE(op); \
        EVAL_ZERO(op);

/* BIT
 * NOTE: don't use this for 65C02's BIT #immediate, for this
 * behaves differently (in term of affected flags)! */
#define OP_BIT(op)  \
        EVAL_NEGATIVE(op); \
        EVAL_ZERO(op & AC); \
        B6_TO_OVERFLOW(op);      /* Copy bit 6 to OVERFLOW flag. */

/* TSB (test and set bits; only available on 65C02) */
#define OP_TSB(op)  \
        EVAL_ZERO(op & AC); \
        op |= AC;

/* TRB (test and reset bits; only available on 65C02) */
#define OP_TRB(op)  \
        EVAL_ZERO(op & AC); \
        op &= ~AC;

/* -------------------------------------------------------------------------
 * INC and DEC instructions
 */

#define OP_INC(op)  \
        op = (op + 1) & 0xff; \
        EVAL_ZERO(op); \
        EVAL_NEGATIVE(op);

#define OP_DEC(op)  \
        op = (op - 1) & 0xff; \
        EVAL_ZERO(op); \
        EVAL_NEGATIVE(op);

/* -------------------------------------------------------------------------
 * arithmetic instructions: ADC, SBC
 * These instructions need an additional temporary variable.
 * Note that there are several variants: the NMOS variant which doesn't set
 * Z and N flags correctly in decimal mode, the CMOS variant which handles
 * flags correctly but uses one additional CPU cycle, and a variant with
 * no decimal mode.
 */

/* without decimal mode */
#define OP_ADC_WO_D(src, result)  \
    result = AC + (src) + CARRY_TO_B0(); \
    EVAL_CARRY(result); \
    result &= 0xff; \
    EVAL_ZERO(result); \
    EVAL_NEGATIVE(result); \
    B7_TO_OVERFLOW((AC ^ result) & ((src) ^ result)); \
    AC = (result);

/* NMOS version with invalid Z,N flag in decimal mode */
#define OP_ADC_NMOS(src, result)  \
    result = AC + (src) + CARRY_TO_B0(); \
    if (LIKELY(!IS_DECIMAL())) { \
        EVAL_CARRY(result); \
        result &= 0xff; \
        EVAL_ZERO(result); \
        EVAL_NEGATIVE(result); \
        B7_TO_OVERFLOW((AC ^ result) & ((src) ^ result)); \
    } \
    else { \
        EVAL_ZERO ((result) & 0xff);     /* This is not valid in decimal mode */ \
        if (((AC & 0xf) + ((src) & 0xf) + CARRY_TO_B0()) > 0x09) \
            result += 0x06; \
        EVAL_NEGATIVE(result); \
        B7_TO_OVERFLOW((AC ^ result) & ((src) ^ result)); \
        if ((result) > 0x99) \
            result += 0x60; \
        /* TODO: if the initial numbers are not valid BCD, the carry flag may overflow! */ \
        EVAL_CARRY(result); \
        result &= 0xff; \
    } \
    AC = (result);

/* ADC with corrected Z and N flag (65C02 emulation) */
#define OP_ADC_CMOS(src, result)  \
    result = AC + (src) + CARRY_TO_B0(); \
    if (LIKELY(!IS_DECIMAL())) { \
        EVAL_CARRY(result); \
        result &= 0xff; \
        EVAL_ZERO(result); \
        EVAL_NEGATIVE(result); \
        B7_TO_OVERFLOW((AC ^ result) & ((src) ^ result)); \
    } \
    else { \
        CLOCK_CYCLES_INC();    /* 65C02: add 1 cycle */ \
        if (((AC & 0xf) + ((src) & 0xf) + CARRY_TO_B0()) > 0x09) \
            result += 0x06; \
        B7_TO_OVERFLOW((AC ^ result) & ((src) ^ result)); \
        if ((result) > 0x99) \
            result += 0x60; \
        /* TODO: if the initial numbers are not valid BCD, the carry flag may overflow! */ \
        EVAL_CARRY(result); \
        result &= 0xff; \
        EVAL_ZERO(result); \
        EVAL_NEGATIVE(result); \
    } \
    AC = (result);

#if defined(NO_DECIMAL_MODE)
#   define OP_ADC(src, result)  OP_ADC_WO_D(src, result)
#elif defined(CPU_65C02) || defined(CORRECT_BCD_FLAGS)
#   define OP_ADC(src, result)  OP_ADC_CMOS(src, result)
#else /* NMOS 6502 version */
#   define OP_ADC(src, result)  OP_ADC_NMOS(src, result)
#endif

/* SBC without decimal mode */
#define OP_SBC_WO_D(src, result)  \
    result = AC + 0xff - (src) + CARRY_TO_B0(); \
    EVAL_CARRY(result); \
    result &= 0xff; \
    EVAL_NEGATIVE(result); \
    EVAL_ZERO(result);  \
    B7_TO_OVERFLOW((AC ^ result) & (AC ^ (src))); \
    AC = (result);


/* NMOS version with invalid Z,N flag in decimal mode */
#define OP_SBC_NMOS(src, result)  \
    result = AC + 0xff - (src) + CARRY_TO_B0(); \
    EVAL_NEGATIVE(result); \
    B7_TO_OVERFLOW((AC ^ result) & (AC ^ (src))); \
    if (LIKELY(!IS_DECIMAL())) { \
        EVAL_CARRY(result); \
        result &= 0xff; \
        EVAL_ZERO(result); \
    } \
    else { \
        if ((unsigned)((AC & 0xf) - ((src) & 0x0f) - 1 + CARRY_TO_B0()) > 0x09) \
            result -= 6; \
        EVAL_ZERO(result & 0xff); \
        if ((result & 0xff) > 0x9f) \
            result -= 0x60; \
        EVAL_CARRY(result); \
        result &= 0xff; \
    } \
    AC = (result);

/* corrected flags version for 65C02 emulation */
#define OP_SBC_CMOS(src, result)  \
    result = AC + 0xff - (src) + CARRY_TO_B0(); \
    if (LIKELY(!IS_DECIMAL())) { \
        EVAL_CARRY(result); \
        result &= 0xff; \
        EVAL_NEGATIVE(result); \
        EVAL_ZERO(result); \
        B7_TO_OVERFLOW((AC ^ result) & (AC ^ (src))); \
    } \
    else { \
        CLOCK_CYCLE_INC();    /* 65C02: add 1 cycle */ \
        B7_TO_OVERFLOW((AC ^ result) & (AC ^ (src))); \
        if ((unsigned)((AC & 0x0f) - ((src) & 0x0f) - 1 + CARRY_TO_B0()) > 0x09) \
            result -= 6; \
        if ((result & 0xff) > 0x9f) \
            result -= 0x60; \
        EVAL_CARRY(result); \
        result &= 0xff; \
        EVAL_ZERO(result); \
        EVAL_NEGATIVE(result); \
    } \
    AC = (result);

#if defined(NO_DECIMAL_MODE)
#   define OP_SBC(src, result)  OP_SBC_WO_D(src, result)
#elif defined(CPU_65C02) || defined(CORRECT_BCD_FLAGS)
#   define OP_SBC(src, result)  OP_SBC_CMOS(src, result)
#else /* NMOS 6502 version */
#   define OP_SBC(src, result)  OP_SBC_NMOS(src, result)
#endif

/* -------------------------------------------------------------------------
 * undocumented instructions
 */

/* ASO (SLO)
 * combined ASL, ORA; ASL the contents of a memory location, and then OR
 * the result with the accumulator.
 * opcodes 03, 07, 0F, 13, 17, 1B, 1F */
#define OP_ASO(op) \
        op <<= 1;                \
        EVAL_CARRY(op);           \
        op &= 0xff;             \
        op |= AC;              \
        EVAL_ZERO(op); \
        EVAL_NEGATIVE(op);

/* RLA
 * combined ROL, AND; ROL memory and AND the result with accumulator
 * opcodes 23, 27, 2F, 33, 37, 3B, 3F */
#define OP_RLA(op)  \
        ROL_(op); \
        AC &= op; \
        EVAL_ZERO(AC); \
        EVAL_NEGATIVE(AC);

/* RRA
 * combined ROR, ADC; ROR memory location and then ADC result with accumulator */
#define OP_RRA(op, result)  \
        ROR_(op); \
        OP_ADC(op, temp);

/* DCP (DCM)
 * combined DEC, CMP; decrement memory location and compare result with accu */
#define OP_DCM(op)  \
        op = (op - 1) & 0xff;  \
        EVAL_CARRY(AC + 0x100 - op); \
        EVAL_NEGATIVE(AC + 0x100 - op); \
        EVAL_ZERO((AC + 0x100 - op) & 0xff);

/* ISC (ISB, INS)
 * combined INC, SBC; INC memory, then substract memory from accumulator with borrow */
#define OP_INS(op, result)  \
        op = ((op + 1) & 0xff); \
        OP_SBC(op, temp);
        /* remember to store op */

/* ANC (AAC)
 * Perform AND # instruction, and then move bit 7 of the accumulator
 * into the Carry flag.  */
#define OP_ANC(op)    \
        AC &= op; \
        EVAL_ZERO(AC); \
        EVAL_NEGATIVE(AC); \
        B7_TO_CARRY(AC);

/* =========================================================================
 * CPU main emulation function:
 * Execute one instruction
 */
void do_cpu(void)
{
    unsigned b;

    TST_PROFILE(PRF_CPU, "do_cpu()");

    b = LOADEXEC(PC);
    TST_PRF_CPU_INSTR(b);

    switch (b) {

    case 0x00:  /* BRK */
        {
            CLOCK_CYCLES(7);
            PUSH16(PC+2);   /* BRK instruction is 2 byte! (one padding byte) */
            PUSH(SETUP_SR());       /* Push status register into the stack. */
            SET_INTERRUPT();        /* Disable interrupts. */
#if defined(CPU_65C02) || defined(CLEAR_D_ON_INTERRUPT)
            CLEAR_DECIMAL();
#endif
            PC = LOAD_ADDR16(BRK_VECTOR);   /* Jump using BRK vector (0xfffe). */
        }
        break;

    case 0x01:  /* ORA X,ind */
        {
            PRE_X_IND(6);
            OP_ORA(op);
            POST_X_IND();
        }
        break;

#if defined(CPU_65C02)
    case 0x02:  /* unused opcode NOP (65C02) / illegal instruction */
        {
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 2);
# else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
# endif
        }
        break;
#endif

    case 0x03:  /* undocumented ASO (SLO) X,ind */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_X_IND(8);
            /* op = asl(op); AC = ora(op); */
            OP_ASO(op);
            STORE(addr, op);
            POST_X_IND();
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x04:  /* undocumented */
        {
#if defined(CPU_65C02)
            /* TSB zpg */
            PRE_ZPG(5);
            OP_TSB(op);
            POST_ZPG_STORE(op);
#else
            /* NOP zp */
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_NOP(2, 3);
# else
            PRE_ZPG(3);
            (void) op;
            POST_ZPG();
# endif
#endif
        }
        break;

    case 0x05:  /* ORA zpg */
        {
            PRE_ZPG(3);
            OP_ORA(op)
            POST_ZPG();
        }
        break;

    case 0x06:  /* ASL zpg */
        {
            PRE_ZPG(5);
            OP_ASL(op);
            POST_ZPG_STORE(op);
        }
        break;

    case 0x07:
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* RMB0 zpg */
            INSTRUCTION__RMB(0);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented ASO (SLO) zpg */
            /* ASL the contents of a memory location and then OR the result
               with the accumulator. */
            UNDOCUMENTED_OPCODE();
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(2, 5);
# else
            PRE_ZPG(5);
            OP_ASO(op);
            POST_ZPG_STORE(op);
# endif
#endif
        }
        break;

    case 0x08:  /* PHP */
        {
            INSTRUCTION__PUSH(SETUP_SR());
        }
        break;

    case 0x09:  /* ORA # */
        {
            PRE_IMMEDIATE();
            OP_ORA(op);
            POST_IMMEDIATE();
        }
        break;

    case 0x0a:  /* ASL A */
        {
            PRE_REGISTER(AC);
            OP_ASL(op);
            POST_REGISTER_STORE(AC);
        }
        break;

    case 0x0b:
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* undocumented ANC (AAC) # */
            /* Perform AND # instruction, and then move bit 7 of the accumulator
               into the Carry flag. */
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(2, 2);
# else
            PRE_IMMEDIATE();
            OP_ANC(op);
            POST_IMMEDIATE();
# endif
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x0c:  /* undocumented 3 byte NOP */
        {
#if defined(CPU_65C02)
            /* TSB abs */
            PRE_ABS(6);
            OP_TSB(op);
            POST_ABS_STORE(op);
#else
            UNDOCUMENTED_OPCODE();
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(3, 4);
# else
            PRE_ABS(4);
            (void) op;
            POST_ABS();
# endif
#endif
        }
        break;

    case 0x0d:  /* ORA abs */
        {
            PRE_ABS(4);
            OP_ORA(op);
            POST_ABS();
        }
        break;

    case 0x0e:  /* ASL abs */
        {
            PRE_ABS(6);
            OP_ASL(op);
            POST_ABS_STORE(op);
        }
        break;

    case 0x0f:  /* undocumented ASO (SLO) abs */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR0 zpg,rel */
            INSTRUCTION__BBR(0);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* ASL the contents of a memory location and then OR the result
               with the accumulator. */
            UNDOCUMENTED_OPCODE();
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(3, 6);
# else
            PRE_ABS(6);
            OP_ASO(op);
            POST_ABS_STORE(op);
# endif
#endif
        }
        break;

    case 0x10:  /* BPL rel */
        {
            /* Branch if sign flag is clear. */
            INSTRUCTION_BRANCH(!IS_NEGATIVE());
        }
        break;

    case 0x11:  /* ORA ind,Y */
        {
            PRE_IND_Y_VC(5);
            OP_ORA(op);
            POST_IND_Y();
        }
        break;

#ifdef CPU_65C02
    case 0x12:  /* ORA (zp) */
        {
            PRE_ZPG_IND();
            OP_ORA(op);
            POST_ZPG_IND();
        }
    break;
#endif

    case 0x13:
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* undocumented ASO (SLO) ind,Y */
            /* ASL the contents of a memory location and then OR the result
               with the accumulator. */
            PRE_IND_Y(8);
            OP_ASO(op);
            POST_IND_Y_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x14:  /* undocumented 2-byte NOP zpg,X */
        {
#if defined(CPU_65C02)
            /* TRB zpg */
            PRE_ZPG(5);
            OP_TRB(op);
            POST_ZPG_STORE(op);
#else
            /* NOP zp,X */
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_NOP(2, 4);
# else
            PRE_ZPGXY(4, X);
            (void) op;
            POST_ZPGXY();
# endif
#endif
        }
        break;

    case 0x15:  /* ORA zpg,X */
        {
            PRE_ZPGXY(4, X);
            OP_ORA(op);
            POST_ZPGXY();
        }
        break;

    case 0x16:  /* ASL zpg,X */
        {
            PRE_ZPGXY(6, X);
            OP_ASL(op);
            POST_ZPGXY_STORE(op);
        }
        break;

    case 0x17:  /* undocumented ASO (SLO) zpg,X */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* RMB1 zpg */
            INSTRUCTION__RMB(1);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ZPGXY(6, X);
            OP_ASO(op);
            POST_ZPGXY_STORE(op);
#endif
        }
        break;

    case 0x18:  /* CLC */
        {
            INSTRUCTION_FLAG(CLEAR_CARRY());
        }
        break;

    case 0x19:  /* ORA abs,Y */
        {
            PRE_ABS_XY_VC(4, Y);
            OP_ORA(op);
            POST_ABS_XY();
        }
        break;

    case 0x1a:  /* undocumented */
        {
#if !defined(CPU_65C02)
            /* NOP */
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 2);
#else
            /* INC A */
            PRE_REGISTER(AC);
            OP_INC(op);
            POST_REGISTER_STORE(AC);
#endif
        }
        break;

    case 0x1b:  /* undocumented ASO (SLO) abs,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_ABS(7);
            OP_ASO(op);
            POST_ABS_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
       }
        break;

    case 0x1c:  /* undocumented 3 byte NOP abs,X */
        {
#if defined(CPU_65C02)
            /* TRB abs */
            PRE_ABS(6);
            OP_TRB(op);
            POST_ABS_STORE(op);
#else
            UNDOCUMENTED_OPCODE();
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(3, 4);
# else
            PRE_ABS_XY_VC(4, X);
            (void) op;
            POST_ABS_XY();
# endif
#endif
        }
        break;

    case 0x1d:  /* ORA abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            OP_ORA(op);
            POST_ABS_XY();
        }
        break;

    case 0x1e:  /* ASL abs,X */
        {
#ifdef CPU_65C02
            PRE_ABS_XY_VC(6, X);
            OP_ASL(op);
            POST_ABS_XY_STORE(op);
#else
            PRE_ABS_XY(7, X);
            OP_ASL(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x1f:  /* undocumented ASO (SLO) abs,X */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR1 zpg,rel */
            INSTRUCTION__BBR(1);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY(7, X);
            OP_ASO(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x20:  /* JSR abs */
        {
            /* Jump to subroutine. */
            CLOCK_CYCLES(6);
            unsigned addr = FETCH_ADDR_ABS();
            PUSH16(PC+2);   /* push return address minus 1 */
            PC = addr;
        }
        break;

    case 0x21:  /* AND X,ind */
        {
            PRE_X_IND(6);
            OP_AND(op);
            POST_X_IND();
        }
        break;

#ifdef CPU_65C02
    case 0x22:  /* unused opcode NOP (65C02) / illegal instruction */
        {
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 2);
# else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
# endif
        }
        break;
#endif

    case 0x23:  /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* RLA (zp,X) */
            /* ROL memory and AND the result with accumulator */
            PRE_X_IND(8);
            OP_RLA(op);
            POST_X_IND_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x24:  /* BIT zpg */
        {
            PRE_ZPG(3);
            OP_BIT(op);
            POST_ZPG();
        }
        break;

    case 0x25:  /* AND zpg */
        {
            PRE_ZPG(3);
            OP_AND(op);
            POST_ZPG();
        }
        break;

    case 0x26:  /* ROL zpg */
        {
            PRE_ZPG(5);
            OP_ROL(op);
            POST_ZPG_STORE(op);
        }
        break;

    case 0x27:
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* RMB2 zpg */
            INSTRUCTION__RMB(2);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented RLA zpg */
            UNDOCUMENTED_OPCODE();
            PRE_ZPG(5);
            OP_RLA(op);
            POST_ZPG_STORE(op);
#endif
        }
        break;

    case 0x28:  /* PLP */
        {
            CLOCK_CYCLES(4);
            unsigned op;
            /* First increment stack pointer and then pull item from stack. */
            op = PULL();
            BREAKDOWN_SR(op);
            ADVANCE_PC(1);
        }
        break;

    case 0x29:  /* AND # */
        {
            PRE_IMMEDIATE();
            OP_AND(op);
            POST_IMMEDIATE();
        }
        break;

    case 0x2a:  /* ROL A */
        {
            PRE_REGISTER(AC);
            OP_ROL(op);
            POST_REGISTER_STORE(AC);
        }
        break;

    case 0x2b:  /* undocumented ANC # */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* AND # with Carry Flag set to Bit 7 of result */
            PRE_IMMEDIATE();
            OP_ANC(op);
            POST_IMMEDIATE();
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x2c:  /* BIT abs */
        {
            PRE_ABS(4);
            OP_BIT(op);
            POST_ABS();
        }
        break;

    case 0x2d:  /* AND abs */
        {
            PRE_ABS(4);
            OP_AND(op);
            POST_ABS();
        }
        break;

    case 0x2e:  /* ROL abs */
        {
            PRE_ABS(6);
            OP_ROL(op);
            POST_ABS_STORE(op);
        }
        break;

    case 0x2f:  /* undocumented RLA abs */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR2 zpg,rel */
            INSTRUCTION__BBR(2);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            PRE_ABS(6);
            OP_RLA(op);
            POST_ABS_STORE(op);
#endif
        }
        break;

    case 0x30:  /* BMI rel */
        {
            /* Branch if sign flag is set. */
            INSTRUCTION_BRANCH(IS_NEGATIVE());
        }
        break;

    case 0x31:  /* AND ind,Y */
        {
            PRE_IND_Y_VC(5);
            OP_AND(op);
            POST_IND_Y();
        }
        break;

#ifdef CPU_65C02
    case 0x32:  /* AND (zp) */
        {
            PRE_ZPG_IND();
            OP_AND(op);
            POST_ZPG_IND();
        }
    break;
#endif

    case 0x33:  /* undocumented RLA ind,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_IND_Y(8);
            OP_RLA(op);
            POST_IND_Y_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x34:  /* undocumented 2-byte NOP zpg,X */
        {
#if defined(CPU_65C02)
            /* BIT zpg,X (65C02) */
            PRE_ZPGXY(4, X);
            OP_BIT(op);
            POST_ZPGXY();
#else
            /* NOP zp,X */
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_NOP(2, 4);
# else
            PRE_ZPGXY(4, X);
            (void) op;
            POST_ZPGXY();
# endif
#endif
        }
        break;


    case 0x35:  /* AND zpg,X */
        {
            PRE_ZPGXY(4, X);
            OP_AND(op);
            POST_ZPGXY();
        }
        break;

    case 0x36:  /* ROL zpg,X */
        {
            PRE_ZPGXY(6, X);
            OP_ROL(op);
            POST_ZPGXY_STORE(op);
        }
        break;

    case 0x37:  /* undocumented RLA zpg,X */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* RMB3 zpg */
            INSTRUCTION__RMB(3);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ZPGXY(6, X);
            OP_RLA(op);
            POST_ZPGXY_STORE(op);
#endif
        }
        break;

    case 0x38: /* SEC */
        {
            INSTRUCTION_FLAG(SET_CARRY());
        }
        break;

    case 0x39:  /* AND abs,Y */
        {
            PRE_ABS_XY_VC(4, Y);
            OP_AND(op);
            POST_ABS_XY();
        }
        break;

    case 0x3a:  /* undocumented */
        {
#if !defined(CPU_65C02)
            /* NOP */
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 2);
#else
            /* DEC A */
            PRE_REGISTER(AC);
            OP_DEC(op);
            POST_REGISTER_STORE(AC);

#endif
        }
        break;

    case 0x3b:  /* undocumented RLA abs,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_ABS_XY(7, Y);
            OP_RLA(op);
            POST_ABS_XY_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x3c:  /* BIT abs,X (65C02) / undocumented 3 byte NOP abs,X */
        {
#if defined(CPU_65C02)
            /* BIT abs,X (65C02) */
            PRE_ABS_XY_VC(4, X);
            OP_BIT(op);
            POST_ABS_XY();
#else
            UNDOCUMENTED_OPCODE();
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(3, 4);
# else
            PRE_ABS_XY_VC(4, X);
            (void) op;
            POST_ABS_XY();
# endif
#endif
        }
        break;

    case 0x3d:  /* AND abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            OP_AND(op);
            POST_ABS_XY();
        }
        break;

    case 0x3e:  /* ROL abs,X */
        {
#ifdef CPU_65C02
            PRE_ABS_XY_VC(6, X);
            OP_ROL(op);
            POST_ABS_XY_STORE(op);
#else
            PRE_ABS_XY(7, X);
            OP_ROL(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x3f:
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR3 zpg,rel */
            INSTRUCTION__BBR(3);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented RLA abs,X */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY(7, X);
            OP_RLA(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x40:  /* RTI */
        {
            /* Return from interrupt. */
            CLOCK_CYCLES(6);
            unsigned op;
            /* Load program status and program counter from stack. */
            op = PULL();
            BREAKDOWN_SR (op);
            op = PULL();
            op |= (PULL() << 8);  /* Load return address from stack.
                 Note that unlike RTS the pulled address must not be incremented! */
            PC = (op);
        }
        break;

    case 0x41:  /* EOR x,ind */
        {
            PRE_X_IND(6);
            OP_EOR(op);
            POST_X_IND();
        }
        break;

#ifdef CPU_65C02
    case 0x42:  /* unused opcode NOP (65C02) / illegal instruction */
        {
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 2);
# else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
# endif
        }
        break;
#endif

    case 0x43:  /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* SRE (LSE) (zp,X) */
            /* LSR memory, than EOR accumulator with memory */
            PRE_X_IND(8);
            /* op = lsr(op); AC = eor(op); */
            LSR_(op);
            op ^= AC;
            EVAL_ZERO (op);
            EVAL_NEGATIVE (op);
            POST_X_IND_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x44:  /* undocumented 2-byte NOP zpg */
        {
            UNDOCUMENTED_OPCODE();
#ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 3);
#else
            PRE_ZPG(3);
            (void) op;
            POST_ZPG();
#endif
        }
        break;

    case 0x45:  /* EOR zpg */
        {
            PRE_ZPG(3);
            OP_EOR(op);
            POST_ZPG();
        }
        break;

    case 0x46:  /* LSR zpg */
        {
            PRE_ZPG(5);
            OP_LSR(op);
            POST_ZPG_STORE(op);
        }
        break;

    case 0x47:
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* RMB4 zpg */
            INSTRUCTION__RMB(4);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented LSE (SRE) zpg */
            UNDOCUMENTED_OPCODE();
            PRE_ZPG(5);
            LSR_(op);
            op ^= AC;
            EVAL_ZERO(op);
            EVAL_NEGATIVE(op);
            POST_ZPG_STORE(op);
#endif
        }
        break;

    case 0x48:  /* PHA */
        {
            INSTRUCTION__PUSH(AC);
        }
        break;

    case 0x49:  /* EOR # */
        {
            PRE_IMMEDIATE();
            OP_EOR(op);
            POST_IMMEDIATE();
        }
        break;

    case 0x4a:  /* LSR A */
        {
            PRE_REGISTER(AC);
            OP_LSR(op);
            POST_REGISTER_STORE(AC);
        }
        break;

    case 0x4b:  /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* ALR (ASR) # */
            /* AND # and then LSR the result */
            PRE_IMMEDIATE();
            op &= AC;
            LSR_(op);
            AC = op;
            EVAL_ZERO (op);
            EVAL_NEGATIVE (op);
            POST_IMMEDIATE();
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x4c:  /* JMP abs */
        {
            CLOCK_CYCLES(3);
            unsigned addr = FETCH_ADDR_ABS();
            PC = (addr);
        }
        break;

    case 0x4d:  /* EOR abs */
        {
            PRE_ABS(4);
            OP_EOR(op);
            POST_ABS();
        }
        break;

    case 0x4e:  /* LSR abs */
        {
            PRE_ABS(6);
            OP_LSR(op);
            POST_ABS_STORE(op);
        }
        break;

    case 0x4f:  /* undocumented LSE (SRE) abs */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR4 zpg,rel */
            INSTRUCTION__BBR(4);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ABS(6);
            LSR_(op);
            op ^= AC;
            EVAL_ZERO (op);
            EVAL_NEGATIVE (op);
            POST_ABS_STORE(op);
#endif
        }
        break;

    case 0x50:  /* BVC rel */
        {
            /* Branch if overflow flag is clear. */
            INSTRUCTION_BRANCH(!IS_OVERFLOW());
        }
        break;

    case 0x51:  /* EOR ind,Y */
        {
            PRE_IND_Y_VC(5);
            OP_EOR(op);
            POST_IND_Y();
        }
        break;

#ifdef CPU_65C02
    case 0x52:  /* EOR (zp) */
        {
            PRE_ZPG_IND();
            OP_EOR(op);
            POST_ZPG_IND();
        }
    break;
#endif

    case 0x53:  /* undocumented LSE (SRE) ind,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_IND_Y(8);
            /* op = lsr(op); AC = eor(op); */
            LSR_(op);
            op ^= AC;
            EVAL_ZERO(op);
            EVAL_NEGATIVE(op);
            POST_IND_Y_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x54:  /* undocumented 2-byte NOP zpg,X */
        {
            UNDOCUMENTED_OPCODE();
#ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 4);
#else
            PRE_ZPGXY(4, X);
            (void) op;
            POST_ZPGXY();
#endif
        }
        break;

    case 0x55:  /* EOR zpg,X */
        {
            PRE_ZPGXY(4, X);
            OP_EOR(op);
            POST_ZPGXY();
        }
        break;

    case 0x56:  /* LSR zpg,X */
        {
            PRE_ZPGXY(6, X);
            OP_LSR(op);
            POST_ZPGXY_STORE(op);
        }
        break;

    case 0x57:  /* undocumented LSE (SRE) zpg,X */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* RMB5 zpg */
            INSTRUCTION__RMB(5);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ZPGXY(6, X);
            /* op = lsr(op); AC = eor(op); */
            LSR_(op);
            op ^= AC;
            EVAL_ZERO (op);
            EVAL_NEGATIVE (op);
            POST_ZPGXY_STORE(op);
#endif
        }
        break;

    case 0x58:  /* CLI */
        {
            INSTRUCTION_FLAG(CLEAR_INTERRUPT());    /* enable interrupts */
        }
        break;

    case 0x59:  /* EOR abs,Y */
        {
            PRE_ABS_XY_VC(4, Y);
            OP_EOR(op);
            POST_ABS_XY();
        }
        break;

    case 0x5a:  /* PHY (65C02) / undocumented NOP */
        {
#if !defined(CPU_65C02)
            /* NOP */
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 2);
#else
            /* PHY */
            INSTRUCTION__PUSH(YR);
#endif
        }
        break;

    case 0x5b:  /* undocumented LSE (SRE) abs,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_ABS_XY(7, Y);
            /* op = lsr(op); AC = eor(op); */
            LSR_(op);
            op ^= AC;
            EVAL_ZERO (op);
            EVAL_NEGATIVE (op);
            POST_ABS_XY_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x5c:  /* undocumented 3 byte NOP abs,X */
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            /* NOP abs */
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(3, 8);
# else
            PRE_ABS(8);
            (void) op;
            POST_ABS();
# endif
#else /* NMOS */
            /* NOP abs,X */
# ifdef UNKNOWN_OPCODE_ALL_NOP
            INSTRUCTION_UNOP(3, 4);
# else
            PRE_ABS_XY_VC(4, X);
            (void) op;
            POST_ABS_XY();
# endif
#endif
        }
        break;

    case 0x5d:  /* EOR abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            OP_EOR(op);
            POST_ABS_XY();
        }
        break;

    case 0x5e:  /* LSR abs,X */
        {
#ifdef CPU_65C02
            PRE_ABS_XY_VC(6, X);
            OP_LSR(op);
            POST_ABS_XY_STORE(op);
#else
            PRE_ABS_XY(7, X);
            /* op = lsr(op); AC = eor(op); */
            OP_LSR(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x5f:
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR5 zpg,rel */
            INSTRUCTION__BBR(5);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented LSE (SRE) abs,X */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY(7, X);
            /* op = lsr(op); AC = eor(op); */
            LSR_(op);
            op ^= AC;
            EVAL_ZERO (op);
            EVAL_NEGATIVE (op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x60:  /* RTS */
        {
            /* Return from subroutine. */
            CLOCK_CYCLES(6);
            unsigned op;
            op = PULL();
            op += ((PULL()) << 8) + 1;    /* Load return address from stack and add 1. */
            PC = (op);
        }
        break;

    case 0x61:  /* ADC X,ind */
        {
            PRE_X_IND(6);
            unsigned temp;
            OP_ADC(op, temp);
            POST_X_IND();
        }
        break;

#ifdef CPU_65C02
    case 0x62:  /* unused opcode NOP (65C02) / illegal instruction */
        {
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 2);
# else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
# endif
        }
        break;
#endif

    case 0x63:  /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* RRA (zp,X) */
            /* ROR memory location and then ADC result with accumulator */
            PRE_X_IND(8);
            unsigned temp;
            OP_RRA(op, temp);
            POST_X_IND_STORE(temp);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x64:
        {
#if defined(CPU_65C02)
            /* STZ zp */
            PRE_ZPG_NOLOAD(3);
            POST_ZPG_STORE(0x00);
#else
            /* NOP zp */
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_NOP(2, 3);
# else
            PRE_ZPG(3);
            (void) op;
            POST_ZPG();
# endif
#endif
        }
        break;

    case 0x65:  /* ADC zpg */
        {
            PRE_ZPG(3);
            unsigned temp;
            OP_ADC(op, temp);
            POST_ZPG();
        }
        break;

    case 0x66:  /* ROR zpg */
        {
            PRE_ZPG(5);
            OP_ROR(op);
            POST_ZPG_STORE(op);
        }
        break;

    case 0x67:  /* RRA zpg */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* RMB6 zpg */
            INSTRUCTION__RMB(6);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ZPG(5);
            unsigned temp;
            OP_RRA(op, temp);
            POST_ZPG_STORE(temp);
#endif
        }
        break;

    case 0x68:  /* PLA */
        {
            INSTRUCTION__PULL(AC);
        }
        break;

    case 0x69:  /* ADC # */
        {
            PRE_IMMEDIATE();
            unsigned temp;
            OP_ADC(op, temp);
            POST_IMMEDIATE();
        }
        break;

    case 0x6a:  /* ROR A */
        {
            PRE_REGISTER(AC);
            OP_ROR(op);
            POST_REGISTER_STORE(AC);
        }
        break;

    case 0x6b:  /* undocumented ARR # */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* ARR */
            /* combined AND, ROR; AND accumulator with immediate value and
               then ROR the result. Flags are partly changed like ADC. */
            PRE_IMMEDIATE();
            op = AC & op;
            op |= CARRY_TO_B8();
            B7_TO_CARRY(AC);
            B6_TO_OVERFLOW(op ^ (op >> 1));
            op >>= 1;
            AC = op;
            EVAL_ZERO(op);
            EVAL_NEGATIVE(op);
            POST_IMMEDIATE();
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x6c:  /* JMP ind */
        {
            /* NOTE: we don't emulate the infamous JMP ind page boundary bug */
            CLOCK_CYCLES(5);
            unsigned addr = LOADEXEC16(PC+1);
            unsigned op = LOAD_ADDR16(addr);
            PC = op;
        }
        break;

    case 0x6d:  /* ADC abs */
        {
            PRE_ABS(4);
            unsigned temp;
            OP_ADC(op, temp);
            POST_ABS();
        }
        break;

    case 0x6e:  /* ROR abs */
        {
            PRE_ABS(6);
            OP_ROR(op);
            POST_ABS_STORE(op);
        }
        break;

    case 0x6f:  /* undocumented RRA abs */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR6 zpg,rel */
            INSTRUCTION__BBR(6);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ABS(6);
            unsigned temp;
            OP_RRA(op, temp);
            POST_ABS_STORE(temp);
#endif
        }
        break;

    case 0x70:  /* BVS rel */
        {
            /* Branch if overflow flag is set. */
            INSTRUCTION_BRANCH(IS_OVERFLOW());
        }
        break;

    case 0x71:  /* ADC ind,Y */
        {
            PRE_IND_Y_VC(5);
            unsigned temp;
            OP_ADC(op, temp);
            POST_IND_Y();
        }
        break;

#ifdef CPU_65C02
    case 0x72:  /* ADC (zp) */
        {
            PRE_ZPG_IND();
            unsigned temp;
            OP_ADC(op, temp);
            POST_ZPG_IND();
        }
    break;
#endif

    case 0x73:  /* undocumented RRA ind,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_IND_Y(8);
            unsigned temp;
            OP_RRA(op, temp);
            POST_IND_Y_STORE(temp);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x74:  /* undocumented 2-byte NOP zpg,X */
        {
#if defined(CPU_65C02)
            /* STZ zp,X */
            PRE_ZPGXY_NOLOAD(4, X);
            POST_ZPGXY_STORE(0x00);
#else
            /* NOP zp,X */
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_NOP(2, 4);
# else
            PRE_ZPGXY(4, X);
            (void) op;
            POST_ZPGXY();
# endif
#endif
        }
        break;

    case 0x75:  /* ADC zpg,X */
        {
            unsigned temp;
            PRE_ZPGXY(4, X);
            OP_ADC(op, temp);
            POST_ZPGXY();
        }
        break;

    case 0x76:  /* ROR zpg,X */
        {
            PRE_ZPGXY(6, X);
            OP_ROR(op);
            POST_ZPGXY_STORE(op);
        }
        break;

    case 0x77:  /* undocumented RRA zpg,X */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* RMB7 zpg */
            INSTRUCTION__RMB(7);
#else
            UNDOCUMENTED_OPCODE();
            PRE_ZPGXY(6, X);
            unsigned int temp;
            OP_RRA(op, temp);
            POST_ZPGXY_STORE(temp);
#endif
        }
        break;

    case 0x78:  /* SEI */
        {
            INSTRUCTION_FLAG(SET_INTERRUPT());      /* disable interrupts */
        }
        break;

    case 0x79:  /* ADC abs,Y */
        {
            PRE_ABS_XY_VC(4, Y);
            unsigned temp;
            OP_ADC(op, temp);
            POST_ABS_XY();
        }
        break;

    case 0x7a:  /* PLY (65C02) / undocumented NOP */
        {
#if !defined(CPU_65C02)
            /* NOP */
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 2);
#else
            /* PLY */
            INSTRUCTION__PULL(YR);
#endif
        }
        break;

    case 0x7b:  /* undocumented RRA abs,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_ABS_XY(7, Y);
            unsigned temp;
            OP_RRA(op, temp);
            POST_ABS_XY_STORE(temp);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x7c:  /* JMP abs,X (65C02) / undocumented 3 byte NOP abs,X */
        {
#if defined(CPU_65C02)
            /* JMP abs,X */
            CLOCK_CYCLES(6);
            unsigned addr = FETCH_ADDR_ABS_X();
            unsigned op = LOAD_ADDR16(addr);
            PC = op;
#else
            UNDOCUMENTED_OPCODE();
# if defined(UNDOCUMENTED_AS_NOP)
            INSTRUCTION_UNOP(3, 4);
# else
            PRE_ABS_XY_VC(4, X);
            (void) op;
            POST_ABS_XY();
# endif
#endif
        }
        break;

    case 0x7d:  /* ADC abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            unsigned temp;
            OP_ADC(op, temp);
            POST_ABS_XY();
        }
        break;

    case 0x7e:  /* ROR abs,X */
        {
#if defined(CPU_65C02)
            PRE_ABS_XY_VC(6, X);
            OP_ROR(op);
            POST_ABS_XY_STORE(op);
#else
            PRE_ABS_XY(7, X);
            OP_ROR(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x7f:  /* undocumented RRA abs,X */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBR7 zpg,rel */
            INSTRUCTION__BBR(7);
#elif defined (CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY(7, X);
            unsigned temp;
            OP_RRA(op, temp);
            POST_ABS_XY_STORE(temp);
#endif
        }
        break;

    case 0x80:  /* undocumented 2-byte NOP # */
        {
#if defined(CPU_65C02)
            /* BRA */
            INSTRUCTION_BRANCH(1);  /* branch always */
#else
            /* NOP # */
            UNDOCUMENTED_OPCODE();
# ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_NOP(2, 2);
# else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
# endif
#endif
        }
        break;

    case 0x81:  /* STA x,ind */
        {
            PRE_X_IND_NOLOAD(6);
            POST_X_IND_STORE(AC);
        }
        break;

    case 0x82:  /* unused opcode NOP (65C02) / undocumented 2-byte NOP # */
        {
            UNDOCUMENTED_OPCODE();
#ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 2);
#else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
#endif
        }
        break;

    case 0x83:
        {
#if defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented AND(A,X)  SAX X,ind */
            /* ANDs the contents of the A and X registers (without changing the
               contents of either register) and stores the result in memory.
               Does not affect any flags in the processor status register. */
            UNDOCUMENTED_OPCODE();
            PRE_X_IND_NOLOAD(6);
            unsigned op = (AC & XR);
            POST_X_IND_STORE(op);
#endif
        }
        break;

    case 0x84:  /* STY zpg */
        {
            PRE_ZPG_NOLOAD(3);
            POST_ZPG_STORE(YR);
        }
        break;

    case 0x85:  /* STA zpg */
        {
            PRE_ZPG_NOLOAD(3);
            POST_ZPG_STORE(AC);
        }
        break;

    case 0x86:  /* STX zpg */
        {
            PRE_ZPG_NOLOAD(3);
            POST_ZPG_STORE(XR);
        }
        break;

    case 0x87:
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* SMB0 zpg */
            INSTRUCTION__SMB(0);
#elif defined (CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented AND(A,X) zpg */
            /* ANDs the contents of the A and X registers (without changing the
               contents of either register) and stores the result in memory.
               Does not affect any flags in the processor status register. */
            UNDOCUMENTED_OPCODE();
            PRE_ZPG_NOLOAD(3);
            unsigned op = (AC & XR);
            POST_ZPG_STORE(op);
#endif
        }
        break;

    case 0x88:  /* DEY */
        {
            PRE_REGISTER(YR);
            OP_DEC(op);
            POST_REGISTER_STORE(YR);
        }
        break;

    case 0x89:  /* undocumented 2 byte NOP # */
        {
#if defined(CPU_65C02)
            /* BIT # */
            /* NOTE: this one affects flags differently than other addressing modes! */
            PRE_IMMEDIATE();
            EVAL_ZERO (op & AC);
            POST_IMMEDIATE();
#elif defined(UNDOCUMENTED_AS_NOP)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_NOP(2, 2);
#else
            /* NOP # */
            UNDOCUMENTED_OPCODE();
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
#endif
        }
        break;

    case 0x8a:  /* TXA */
        {
            INSTRUCTION_TRF(XR, AC);
        }
        break;

    case 0x8b:
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* undocumented/unstable ANE (XAA) # - TXA,AND with bits 0 and 4
               probably not  transfered but ANDed to A */
            UNSTABLE_INSTRUCTION();
            PRE_IMMEDIATE();
            /* The 'magic constant' is highly instable and may vary from one
               chip to another, or even vary with temperature. */
            AC = (AC | 0xee) & XR & op;
            EVAL_ZERO(AC);
            EVAL_NEGATIVE(AC);
            POST_IMMEDIATE();
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0x8c:  /* STY abs */
        {
            PRE_ABS_NOLOAD(4);
            POST_ABS_STORE(YR);
        }
        break;

    case 0x8d:  /* STA abs */
        {
            PRE_ABS_NOLOAD(4);
            POST_ABS_STORE(AC);
        }
        break;

    case 0x8e:  /* STX abs */
        {
            PRE_ABS_NOLOAD(4);
            POST_ABS_STORE(XR);
        }
        break;

    case 0x8f:
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBS0 zpg,rel */
            INSTRUCTION__BBS(0);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented AND(A,X) abs */
            /* ANDs the contents of the A and X registers (without changing the
               contents of either register) and stores the result in memory.
               Does not affect any flags in the processor status register. */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_NOLOAD(4);
            unsigned op = (AC & XR);
            POST_ABS_STORE(op);
#endif
        }
        break;

    case 0x90:  /* BCC rel */
        {
            /* Branch if carry flag is clear. */
            INSTRUCTION_BRANCH(!IS_CARRY());
        }
        break;

    case 0x91:  /* STA ind,Y */
        {
            PRE_IND_Y_NOLOAD(6);
            POST_IND_Y_STORE(AC);
        }
        break;

#ifdef CPU_65C02
    case 0x92:  /* STA (zp) */
        {
            PRE_ZPG_IND_NOLOAD();
            POST_ZPG_IND_STORE(AC);
        }
    break;
#endif

    case 0x93:
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            /* undocumented/unstable SHA (AXA, AHX) (zp),Y */
            UNSTABLE_INSTRUCTION();
            /* This opcode stores the result of A AND X AND the high byte
               of the target address of the operand +1 in memory. */
            PRE_IND_Y(6);
            op = (AC & XR);
            op &= (addr >> 8) + 1;  /* sometimes this part droppes off */
            /* if page boundary is crossed, the target address get's messed up.
               We don't emulate this behaviour however. */
            POST_IND_Y_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
      break;

    case 0x94:  /* STY zpg,X */
        {
            PRE_ZPGXY_NOLOAD(4, X);
            POST_ZPGXY_STORE(YR);
        }
        break;

    case 0x95:  /* STA zpg,X */
        {
            PRE_ZPGXY_NOLOAD(4, X);
            POST_ZPGXY_STORE(AC);
        }
        break;

    case 0x96:  /* STX zpg,Y */
        {
            PRE_ZPGXY_NOLOAD(4, Y);
            POST_ZPGXY_STORE(XR);
        }
        break;

    case 0x97:
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* SMB1 zpg */
            INSTRUCTION__SMB(1);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented AND(A,X) (SAX) zpg,Y */
            /* ANDs the contents of the A and X registers (without changing the
               contents of either register) and stores the result in memory.
               Does not affect any flags in the processor status register. */
            UNDOCUMENTED_OPCODE();
            PRE_ZPGXY_NOLOAD(4, Y);
            unsigned op = (AC & XR);
            POST_ZPGXY_STORE(op);
#endif
        }
        break;

    case 0x98:  /* TYA */
        {
            INSTRUCTION_TRF(YR, AC);
        }
        break;

    case 0x99:  /* STA abs,Y */
        {
            PRE_ABS_XY_NOLOAD(5, Y);
            POST_ABS_XY_STORE(AC);
        }
        break;

    case 0x9a:  /* TXS */
        {
            /* Don't use INSTRUCTION_TRF(XR, SP) (wrong flag setting)! */
            CLOCK_CYCLES(2);
            unsigned op = XR;
            SP = (op);
            ADVANCE_PC(1);
        }
        break;

    case 0x9b:
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else
            /* undocumented/unstable TAS abs,Y */
            UNSTABLE_INSTRUCTION();
            /* This opcode ANDs the contents of the A and X registers (without
               changing the contents of either register) and transfers the
               result to the stack pointer.  It then ANDs that result with the
               contents of the high byte of the target address of the
               operand +1 and stores that final result in memory.  */
            /// NOLOAD??
            PRE_ABS_XY_NOLOAD(5, Y);
            unsigned op = (AC & XR);
            SP = op; /* SHS */
            op &= (addr >> 8) + 1;  /* sometimes this part drops off */
            /* if page boundary is crossed, the target address may get messed
               up. We don't emulate this. */
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0x9c:  /* undocumented/unstable SHY abs,X */
        {
#if !defined(CPU_65C02)
            /* undocumented/unstable SHY (SYA, SAY) abs,X */
            /* This opcode ANDs the contents of the Y register with <ab+1>
               and stores the result in memory. */
            UNDOCUMENTED_OPCODE();
            UNSTABLE_INSTRUCTION();
            /* I actually don't now if a read operation takes place or not */
            // NOLOAD??
            PRE_ABS_XY_NOLOAD(5, X);
            unsigned op = YR;
            op &= (addr >> 8) + 1;  /* sometimes this part drops off */
            /* if page boundary is crossed, the target address is messed up.
               We don't emulate this however */
            POST_ABS_XY_STORE(op);
#else
            /* STZ abs */
            PRE_ABS_NOLOAD(4);
            POST_ABS_STORE(0x00);
#endif
        }
      break;

    case 0x9d:  /* STA abs,X */
        {
            PRE_ABS_XY_NOLOAD(5, X);
            POST_ABS_XY_STORE(AC);
        }
        break;

    case 0x9e:  /* undocumented/unstable SHX abs,Y */
        {
#if !defined(CPU_65C02)
            /* undocumented/unstable SHX abs,Y */
            /* This opcode ANDs the contents of the X register with high byte
               of target address + 1 and stores the result in memory. */
            UNDOCUMENTED_OPCODE();
            UNSTABLE_INSTRUCTION();
            // NOLOAD?
            PRE_ABS_XY_NOLOAD(5, Y);
            unsigned op = XR;
            op &= (addr >> 8) + 1;      /* sometimes this part drops off */
            /* if page boundary is crossed, the target address is messed up.
               We don't emulate this however */
            POST_ABS_XY_STORE(op);
#else /* CPU_65C02 */
            /* STZ abs */
            PRE_ABS_XY_NOLOAD(5, X);
            POST_ABS_XY_STORE(0x00);
#endif
        }
        break;

    case 0x9f:  /* undocumented/unstable SHA abs,Y */
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* BBS1 zpg,rel */
            INSTRUCTION__BBS(1);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented/unstable SHA (AXA, AHX) abs,Y */
            UNDOCUMENTED_OPCODE();
            UNSTABLE_INSTRUCTION();
            /* This opcode stores the result of A AND X AND the high byte
               of the target address of the operand +1 in memory. */
            // NOLOAD or PRE_ABS_XY(5, Y) ?
            PRE_ABS_XY_NOLOAD(5, Y);
            unsigned op = (AC & XR);
            op &= (addr >> 8) + 1;  /* sometimes this part droppes off */
            /* if page boundary is crossed, the target address get's messed up.
               We don't emulate this behaviour however. */
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0xa0:  /* LDY # */
        {
            PRE_IMMEDIATE();
            OP_LD(op, YR);
            POST_IMMEDIATE();
        }
        break;

    case 0xa1:  /* LDA X,ind */
        {
            PRE_X_IND(6);
            OP_LD(op, AC);
            POST_X_IND();
        }
        break;

    case 0xa2:  /* LDX # */
        {
            PRE_IMMEDIATE();
            OP_LD(op, XR);
            POST_IMMEDIATE();
        }
        break;

    case 0xa3:
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else
            /* undocumented combined LDA, LDX X,ind */
            PRE_X_IND(6);
            OP_LD(op, AC = XR);
            POST_X_IND();
#endif
        }
        break;

    case 0xa4:  /* LDY zpg */
        {
            PRE_ZPG(3);
            OP_LD(op, YR)
            POST_ZPG();
        }
        break;

    case 0xa5:  /* LDA zpg */
        {
            PRE_ZPG(3);
            OP_LD(op, AC)
            POST_ZPG();
        }
        break;

    case 0xa6:  /* LDX zpg */
        {
            PRE_ZPG(3);
            OP_LD(op, XR)
            POST_ZPG();
        }
        break;

    case 0xa7:  /* undocumented combined LDA, LDX (LAX) zpg */
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* SMB2 zpg */
            INSTRUCTION__SMB(2);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ZPG(3);
            OP_LD(op, AC = XR);
            POST_ZPG();
#endif
        }
        break;

    case 0xa8:  /* TAY */
        {
            INSTRUCTION_TRF(AC, YR);
        }
        break;

    case 0xa9:  /* LDA # */
        {
            PRE_IMMEDIATE();
            OP_LD(op, AC);
            POST_IMMEDIATE();
        }
        break;

    case 0xaa:  /* TAX */
        {
            INSTRUCTION_TRF(AC, XR);
        }
        break;

    case 0xab:
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else
            UNSTABLE_INSTRUCTION();
            /* undocumented/unstable LAX # */
            /* This opcode ORs the A register with a magic constant, ANDs the
               result with the immediate value, and then stores the result in
               both A and X. The 'magic constant' is chip and/or temperature
               dependent. Common values are 0x00, 0xff, 0xee... */
            PRE_IMMEDIATE();
            op = (AC | 0x00) & op;
            AC = XR = op;
            POST_IMMEDIATE();
#endif
        }
        break;

    case 0xac:  /* LDY abs */
        {
            PRE_ABS(4);
            OP_LD(op, YR);
            POST_ABS();
        }
        break;

    case 0xad:  /* LDA abs */
        {
            PRE_ABS(4);
            OP_LD(op, AC);
            POST_ABS();
        }
        break;

    case 0xae:  /* LDX abs */
        {
            PRE_ABS(4);
            OP_LD(op, XR);
            POST_ABS();
        }
        break;

    case 0xaf:  /* undocumented combined LDA, LDX (LAX) abs*/
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* BBS2 zpg,rel */
            INSTRUCTION__BBS(2);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ABS(4);
            OP_LD(op, AC = XR);
            POST_ABS();
#endif
        }
        break;


    case 0xb0:  /* BCS rel */
        {
            /* Branch if carry flag is set. */
            INSTRUCTION_BRANCH(IS_CARRY());
        }
        break;

    case 0xb1:  /* LDA ind,Y */
        {
            PRE_IND_Y_VC(5);
            OP_LD(op, AC);
            POST_IND_Y();
        }
        break;

#ifdef CPU_65C02
    case 0xb2:  /* LDA (zp) */
        {
            PRE_ZPG_IND();
            OP_LD(op, AC);
            POST_ZPG_IND();
        }
        break;
#endif


    case 0xb3:  /* undocumented combined LDA, LDX (LAX) ind,Y */
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else
            PRE_IND_Y_VC(5);
            OP_LD(op, AC = XR);
            POST_IND_Y();
#endif
        }
        break;

    case 0xb4:  /* LDY zpg,X */
        {
            PRE_ZPGXY(4, X);
            OP_LD(op, YR);
            POST_ZPGXY();
        }
        break;

    case 0xb5:  /* LDA zpg,X */
        {
            PRE_ZPGXY(4, X);
            OP_LD(op, AC);
            POST_ZPGXY();
        }
        break;

    case 0xb6:  /* LDX zpg,Y */
        {
            PRE_ZPGXY(4, Y);
            OP_LD(op, XR);
            POST_ZPGXY();
        }
        break;

    case 0xb7:  /* undocumented combined LDA,LDX zpg,Y */
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* SMB3 zpg */
            INSTRUCTION__SMB(3);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ZPGXY(4, Y);
            OP_LD(op, AC = XR);
            POST_ZPGXY();
#endif
        }
        break;

    case 0xb8:  /* CLV */
        {
            INSTRUCTION_FLAG(CLEAR_OVERFLOW());
        }
        break;

    case 0xb9:  /* LDA abs,Y */
        {
            PRE_ABS_XY_VC(4, Y);
            OP_LD(op, AC);
            POST_ABS_XY();
        }
        break;

    case 0xba:  /* TSX */
        {
            INSTRUCTION_TRF(SP, XR);
        }
        break;

    case 0xbb:  /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* LAS (LAR) abs,Y */
            /* AND memory with stack pointer and store the result in the
               accumulator, X register, and stack pointer.  Affected flags: N Z. */
            PRE_ABS_XY_VC(4, Y);
            op &= SP;
            AC = XR = SP = op;
            EVAL_ZERO(op);
            EVAL_NEGATIVE(op);
            POST_ABS_XY();
#endif
        }
        break;

    case 0xbc:  /* LDY abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            OP_LD(op, YR);
            POST_ABS_XY();
        }
        break;

    case 0xbd:  /* LDA abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            OP_LD(op, AC);
            POST_ABS_XY();
        }
        break;

    case 0xbe: /* LDX abs,Y */
        {
            PRE_ABS_XY_VC(4, Y);
            OP_LD(op, XR);
            POST_ABS_XY();
        }
        break;

    case 0xbf: /* undocumented combined LDA, LDX abs,Y */
        {
#ifdef CPU_VARIANT_65C02_ROCKWELL
            /* BBS3 zpg,rel */
            INSTRUCTION__BBS(3);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY_VC(4, Y);
            OP_LD(op, AC = XR);
            POST_ABS_XY();
#endif
        }
        break;

    case 0xc0:  /* CPY # */
        {
            PRE_IMMEDIATE();
            OP_CMP(YR, op);
            POST_IMMEDIATE();
        }
        break;

    case 0xc1:  /* CMP X,ind */
        {
            PRE_X_IND(6);
            OP_CMP(AC, op);
            POST_X_IND();
        }
        break;

    case 0xc2:  /* unused opcode NOP (65C02) / undocumented 2-byte NOP # */
        {
            UNDOCUMENTED_OPCODE();
#ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 2);
#else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
#endif
        }
        break;

    case 0xc3: /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* DCP (DCM) (zp,X) */
            /* DEC memory location and CMP with accumulator */
            PRE_X_IND(8);
            OP_DCM(op);
            POST_X_IND_STORE(op);
#endif
        }
        break;

    case 0xc4:  /* CPY zpg */
        {
            PRE_ZPG(3);
            OP_CMP(YR, op);
            POST_ZPG();
        }
        break;

    case 0xc5:  /* CMP zpg */
        {
            PRE_ZPG(3);
            OP_CMP(AC, op);
            POST_ZPG();
        }
        break;

    case 0xc6:  /* DEC zpg */
        {
            PRE_ZPG(5);
            OP_DEC(op);
            POST_ZPG_STORE(op);
        }
        break;

    case 0xc7:
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* SMB4 zpg */
            INSTRUCTION__SMB(4);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented DCM (DCP) zpg */
            UNDOCUMENTED_OPCODE();
            PRE_ZPG(5);
            OP_DCM(op);
            POST_ZPG_STORE(op);
#endif
        }
        break;

    case 0xc8:  /* INY */
        {
            PRE_REGISTER(YR);
            OP_INC(op);
            POST_REGISTER_STORE(YR);
        }
        break;

    case 0xc9:  /* CMP # */
        {
            PRE_IMMEDIATE();
            OP_CMP(AC, op);
            POST_IMMEDIATE();
        }
        break;

    case 0xca:  /* DEX */
        {
            PRE_REGISTER(XR);
            OP_DEC(op);
            POST_REGISTER_STORE(XR);
        }
        break;

    case 0xcb:  /* undocumented */
        {
#if defined (CPU_VARIANT_65C02_WDC)
            /* WAI (WDC variant only) */
            /* Wait for interrupt; Stop the processor until a hardware
               interrupt (IRQ, NMI, RESET) occurs */
            cpu.state |= CPU_STATE_STOPPED;
            cpu.state |= CPU_STATE_WAIT;
            /* Don't advance PC so we will stick in an endless loop. */
            /* Don't advance clock cycles although this instruction takes
               3 cycles, because if we loop here we end up adding more and more. */
            CLOCK_CYCLES_STOPPED();
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* SBX (AXS, SAX) # */
            /* combined CMP, DEX; AND accumulator and X (leaving A's original
               value), substract immediate value and store result in X.
               Substraction is performed in the way of CMP, not SBC (flags!) */
            UNDOCUMENTED_OPCODE();
            PRE_IMMEDIATE();
            op = (AC & XR) + 0x100 - op;  /* Carry is ignored (CMP) */
            EVAL_CARRY(op);
            op &= 0xff;
            XR = (op);
            EVAL_NEGATIVE(op);
            EVAL_ZERO(op);
            POST_IMMEDIATE();
#endif
        }
        break;

    case 0xcc:  /* CPY abs */
        {
            PRE_ABS(4);
            OP_CMP(YR, op);
            POST_ABS();
        }
        break;

    case 0xcd:  /* CMP abs */
        {
            PRE_ABS(4);
            OP_CMP(AC, op);
            POST_ABS();
        }
        break;

    case 0xce: /* DEC abs */
        {
            PRE_ABS(6);
            OP_DEC(op);
            POST_ABS_STORE(op);
        }
        break;

    case 0xcf:
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* BBS4 zpg,rel */
            INSTRUCTION__BBS(4);
#elif defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented DCM (DCP) abs */
            UNDOCUMENTED_OPCODE();
            PRE_ABS(6);
            OP_DCM(op);
            POST_ABS_STORE(op);
#endif
        }
        break;

    case 0xd0:  /* BNE rel */
        {
            /* Branch if  zero flag is clear. */
            INSTRUCTION_BRANCH(!IS_ZERO());
        }
        break;

    case 0xd1:  /* CMP ind,Y */
        {
            PRE_IND_Y_VC(5);
            OP_CMP(AC, op);
            POST_IND_Y();
        }
        break;

#ifdef CPU_65C02
    case 0xD2:  /* CMP (zp) */
        {
            PRE_ZPG_IND();
            OP_CMP(AC, op);
            POST_ZPG_IND();
        }
    break;
#endif

    case 0xd3:  /* undocumented DCM (DCP) ind,Y */
        {
            UNDOCUMENTED_OPCODE();
#if !defined(CPU_65C02)
            PRE_IND_Y(8);
            OP_DCM(op);
            POST_IND_Y_STORE(op);
#else
            INSTRUCTION_UNOP(1, 1);
#endif
        }
        break;

    case 0xd4:  /* undocumented 2-byte NOP zpg,X */
        {
            UNDOCUMENTED_OPCODE();
#ifdef UNDOCUMENTED_AS_NOP
            INSTRUCTION_UNOP(2, 4);
#else
            PRE_ZPGXY(4, X);
            (void) op;
            POST_ZPGXY();
#endif
        }
        break;

    case 0xd5:  /* CMP zpg,X */
        {
            PRE_ZPGXY(4, X);
            OP_CMP(AC, op);
            POST_ZPGXY();
        }
        break;

    case 0xd6:  /* DEC zpg,X */
        {
            PRE_ZPGXY(6, X);
            OP_DEC(op);
            POST_ZPGXY_STORE(op);
        }
        break;

    case 0xd7:
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* SMB5 zpg */
            INSTRUCTION__SMB(5);
#elif defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented DCM (DCP) zpg,X */
            UNDOCUMENTED_OPCODE();
            PRE_ZPGXY(6, X);
            OP_DCM(op);
            POST_ZPGXY_STORE(op);
#endif
        }
        break;

    case 0xd8:  /* CLD */
        {
            INSTRUCTION_FLAG(CLEAR_DECIMAL());
        }
        break;

    case 0xd9:  /* CMP abs,Y */
        {
            PRE_ABS_XY_VC(4, Y);
            OP_CMP(AC, op);
            POST_ABS_XY();
        }
        break;

    case 0xda:  /* PHX (65C02) / undocumented NOP */
        {
#if !defined(CPU_65C02)
            /* NOP */
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 2);
#else
            /* PHX */
            INSTRUCTION__PUSH(XR);
#endif
        }
        break;

    case 0xdb:
        {
#ifdef CPU_VARIANT_65C02_WDC
            /* STP (WDC variant only)
               Stop the processor until a hardware reset occurs */
            cpu.state |= CPU_STATE_STOPPED;
            /* Don't advance PC so we will stick in an endless loop. */
            /* Don't advance clock cycles although this instruction takes
               3 cycles, because if we loop here we end up adding more and more. */
            CLOCK_CYCLES_STOPPED();
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented DCM (DCP) abs,Y */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY(7, Y);
            OP_DCM(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0xdc:  /* undocumented 3 byte NOP abs,X */
        {
            UNDOCUMENTED_OPCODE();
#if defined(UNDOCUMENTED_AS_NOP)
            INSTRUCTION_UNOP(3, 4);
#elif !defined(CPU_65C02)
            /* NMOS 6502 */
            /* undocumented 3 byte NOP abs,X */
            PRE_ABS_XY_VC(4, X);
            (void) op;
            POST_ABS_XY();
#else
            /* CMOS 65C02 */
            /* NOP abs */
            PRE_ABS(4);
            (void) op;
            POST_ABS();
#endif
        }
        break;

    case 0xdd:  /* CMP abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            OP_CMP(AC, op);
            POST_ABS_XY();
        }
        break;

    case 0xde:  /* DEC abs,X */
        {
            PRE_ABS_XY(7, X);
            OP_DEC(op);
            POST_ABS_XY_STORE(op);
        }
        break;

    case 0xdf:
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* BBS5 zpg,rel */
            INSTRUCTION__BBS(5);
#elif defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented DCM (DCP) abs,X */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY(7, X);
            OP_DCM(op);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0xe0:  /* CPX # */
        {
            PRE_IMMEDIATE();
            OP_CMP(XR, op);
            POST_IMMEDIATE();
        }
        break;

    case 0xe1:  /* SBC X,ind */
        {
            PRE_X_IND(6);
            unsigned temp;
            OP_SBC(op, temp);
            POST_X_IND();
        }
        break;

    case 0xe2:  /* unused opcode NOP (65C02) / undocumented 2-byte NOP # */
        {
            UNDOCUMENTED_OPCODE();
#if defined(UNDOCUMENTED_AS_NOP)
            INSTRUCTION_UNOP(2, 2);
#else
            PRE_IMMEDIATE();
            (void) op;
            POST_IMMEDIATE();
#endif
        }
        break;

    case 0xe3:  /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* ISC (ISB, INS) (zp,X) */
            /* INC memory, then substract memory from accumulator with borrow */
            PRE_X_IND(8);
            unsigned temp;
            OP_INS(op, temp);
            POST_X_IND_STORE(op);
#endif
        }
        break;

    case 0xe4:  /* CPX zpg */
        {
            PRE_ZPG(3);
            OP_CMP(XR, op);
            POST_ZPG();
        }
        break;

    case 0xe5:  /* SBC zpg */
        {
            unsigned temp;
            PRE_ZPG(3);
            OP_SBC(op, temp);
            POST_ZPG();
        }
        break;

    case 0xe6:  /* INC zpg */
        {
            PRE_ZPG(5);
            OP_INC(op);
            POST_ZPG_STORE(op);
        }
        break;

    case 0xe7:
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* SMB6 zpg */
            INSTRUCTION__SMB(6);
#elif defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented INS (ISC) zpg */
            UNDOCUMENTED_OPCODE();
            PRE_ZPG(5);
            /* op = ++op & 0xff; AC = sbc(op); */
            unsigned temp;
            OP_INS(op, temp);
            POST_ZPG_STORE(op);
#endif
        }
        break;

    case 0xe8:  /* INX */
        {
            PRE_REGISTER(XR);
            OP_INC(op);
            POST_REGISTER_STORE(XR);
        }
        break;

    case 0xe9:  /* SBC # */
        {
            unsigned temp;
            PRE_IMMEDIATE();
            OP_SBC(op, temp);
            POST_IMMEDIATE();
        }
        break;

    case 0xea:  /* NOP (the "official" one) */
        {
            CLOCK_CYCLES(2);
            ADVANCE_PC(1);
        }
        break;

    case 0xeb:  /* undocumented */
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* SBC # */
            /* same as regular $E9 (SBC #) */
            unsigned temp;
            PRE_IMMEDIATE();
            OP_SBC(op, temp);
            POST_IMMEDIATE();
#endif
        }
        break;

    case 0xec:  /* CPX abs */
        {
            PRE_ABS(4);
            OP_CMP(XR, op);
            POST_ABS();
        }
        break;

    case 0xed:  /* SBC abs */
        {
            PRE_ABS(4);
            unsigned temp;
            OP_SBC(op, temp);
            POST_ABS();
        }
      break;

    case 0xee:  /* INC abs */
        {
            PRE_ABS(6);
            OP_INC(op);
            POST_ABS_STORE(op);
        }
        break;

    case 0xef:  /* undocumented INS (ISC) abs */
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* BBS6 zpg,rel */
            INSTRUCTION__BBS(6);
#elif defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
            PRE_ABS(6);
            /* op = ++op & 0xff; AC = sbc(op); */
            unsigned temp;
            OP_INS(op, temp);
            POST_ABS_STORE(op);
#endif
        }
        break;

    case 0xf0:  /* BEQ rel */
        {
            /* Branch if  zero flag is set. */
            INSTRUCTION_BRANCH(IS_ZERO());
        }
        break;

    case 0xf1:  /* SBC ind,Y */
        {
            PRE_IND_Y_VC(5);
            unsigned temp;
            OP_SBC(op, temp);
            POST_IND_Y();
        }
        break;

#ifdef CPU_65C02
    case 0xf2:  /* SBC (zp) */
        {
            PRE_ZPG_IND();
            unsigned temp;
            OP_SBC(op, temp);
            POST_ZPG_IND();
        }
    break;
#endif

    case 0xf3:
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented INS (ISC) ind,Y */
            PRE_IND_Y(8);
            /* op = ++op & 0xff; AC = sbc(op); */
            unsigned temp;
            OP_INS(op, temp);
            POST_IND_Y_STORE(op);
#endif
        }
        break;

    case 0xf4:  /* undocumented 2-byte NOP zpg,X */
        {
            UNDOCUMENTED_OPCODE();
#if defined(UNDOCUMENTED_AS_NOP)
            INSTRUCTION_UNOP(2, 4);
#else
            PRE_ZPGXY(4, X);
            (void) op;
            POST_ZPGXY();
#endif
        }
        break;

    case 0xf5:  /* SBC zpg,X */
        {
            unsigned temp;
            PRE_ZPGXY(4, X);
            OP_SBC(op, temp);
            POST_ZPGXY();
        }
        break;

    case 0xf6:  /* INC zpg,X */
        {
            PRE_ZPGXY(6, X);
            OP_INC(op);
            POST_ZPGXY_STORE(op);
        }
        break;

    case 0xf7:  /* undocumented INS (ISC) zpg,X */
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* SMB7 zpg */
            INSTRUCTION__SMB(7);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            UNDOCUMENTED_OPCODE();
# if defined(UNDOCUMENTED_AS_NOP)
            INSTRUCTION_UNOP(2, 6);
# else
            PRE_ZPGXY(6, X);
            /* op = ++op & 0xff; AC = sbc(op); */
            unsigned temp;
            OP_INS(op, temp);
            POST_ZPGXY_STORE(op);
# endif
#endif
        }
        break;

    case 0xf8:  /* SED */
        {
            INSTRUCTION_FLAG(SET_DECIMAL());
        }
        break;

    case 0xf9:  /* SBC abs,Y */
        {
            unsigned temp;
            PRE_ABS_XY_VC(4, Y);
            OP_SBC(op, temp);
            POST_ABS_XY();
        }
        break;

    case 0xfa:  /* PLX (65C02) / undocumented NOP */
        {
#if defined(CPU_65C02)
            /* PLX */
            INSTRUCTION__PULL(XR);
#else /* NMOS */
            /* NOP */
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 2);
#endif
        }
        break;

    case 0xfb:
        {
            UNDOCUMENTED_OPCODE();
#if defined(CPU_65C02)
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS */
            /* undocumented INS (ISC) abs,Y */
            PRE_ABS_XY(7, Y);
            unsigned temp;
            OP_INS(op, temp);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    case 0xfc:
        {
            UNDOCUMENTED_OPCODE();
#if defined(UNDOCUMENTED_AS_NOP)
            INSTRUCTION_UNOP(3, 4);
#elif !defined(CPU_65C02)
            /* NMOS 6502 */
            /* undocumented 3 byte NOP abs,X */
            PRE_ABS_XY_VC(4, X);
            (void) op;
            POST_ABS_XY();
#else
            /* CMOS 65C02 */
            /* NOP abs */
            PRE_ABS(4);
            (void) op;
            POST_ABS();
#endif
        }
        break;

    case 0xfd:  /* SBC abs,X */
        {
            PRE_ABS_XY_VC(4, X);
            unsigned temp;
            OP_SBC(op, temp);
            POST_ABS_XY();
        }
        break;

    case 0xfe:  /* INC abs,X */
        {
            PRE_ABS_XY(7, X);
            OP_INC(op);
            POST_ABS_XY_STORE(op);
        }
        break;

    case 0xff:
        {
#if defined(CPU_VARIANT_65C02_ROCKWELL)
            /* BBS7 zpg,rel */
            INSTRUCTION__BBS(7);
#elif defined(CPU_65C02)
            UNDOCUMENTED_OPCODE();
            INSTRUCTION_UNOP(1, 1);
#else /* NMOS 6502 */
            /* undocumented INS (ISC) abs,X */
            UNDOCUMENTED_OPCODE();
            PRE_ABS_XY(7, X);
            unsigned temp;
            OP_INS(op, temp);
            POST_ABS_XY_STORE(op);
#endif
        }
        break;

    default:
        {
            /* Illegal instruction.
             * Opcodes 02, 12, 22, 32, 42, 52, 62, 72, 92, B2, D2, F2 completely
             * halt the cpu, even hardware interrupts won't execute.
             * Only a reset will revive it. */
            cpu.state |= CPU_STATE_STOPPED;
            ILLEGAL_INSTRUCTION();
            /* the emulator gets stuck in a deadlock, because we don't advance
               the PC. Depending on your system implementation you'd like to
               advance the system clock though. That's what's the macro below
               is for. */
            CLOCK_CYCLES_STOPPED();
        }
        break;

    } /* switch */
}
