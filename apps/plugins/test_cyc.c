/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2026 Michael Giacomelli
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

/* Measures what instructions cost on the running device, in CPU cycles.
 *
 * Each benchmark is sixteen copies of one instruction and the loop control.
 * Where plugins get IRAM it is timed with the code in IRAM and in DRAM, and
 * with the data it touches in IRAM and in DRAM (a 256-byte buffer, so the
 * DRAM copy stays cached).  The empty loop is timed the same way and
 * subtracted, so every figure is cycles per instruction.  Targets without
 * plugin IRAM print one column.
 *
 * Stores are also timed with ALU instructions between them, whose cost is
 * subtracted, to tell a cost of back-to-back stores from one of code and
 * data sharing a memory.
 *
 * Cache misses are priced two ways over the plugin buffer.  A pointer chase,
 * where every load's address is the value the previous load returned, gives
 * the full latency of a miss with nothing overlapping it.  Streams of
 * independent loads, a word, a 32-byte line and a four-word ldm at a time,
 * give what a miss costs when it is free to overlap the loads around it.
 *
 * Instructions whose condition fails are timed too (the loop leaves the
 * carry set, so "cc" never executes), and so are stores to the stack, on
 * this thread and again on the codec thread, whose stack a codec uses and
 * which may not be in the memory the main thread's is.
 *
 * Where plugins have the room, a store from IRAM code is also timed at
 * addresses 4 KB apart across IRAM, since that extra cycle depends on
 * which part of IRAM the data is in.
 *
 * Three more cache costs are timed: a store to a line that is not cached,
 * loads that miss far apart rather than next to each other, and an
 * instruction fetch miss, from a chain of branches through more DRAM code
 * than the cache holds.
 *
 * ARMv5 adds the DSP multiplies, clz, qadd, ldrd/strd and pld; ARMv6 the
 * top-word and dual 16-bit multiplies, umaal, ssat, rev, the extends, pkhbt,
 * the SIMD adds and multiply result latency.  Each set is behind #if
 * ARM_ARCH so the file builds for every ARM target.
 *
 * Runs are timed with the finest free-running timer the SoC has, or failing
 * that with the tick, stretching each run to a hundred ticks, which keeps
 * the figures to about a percent.
 *
 * Results go to the screen and to /test_cyc.txt.
 */

#include "plugin.h"

/* Rows that have been run and told nothing the rows left in do not, or
   that were a step to a finding since made another way.  They are kept,
   each group with a line on what it showed, and 1 here runs them again:

   - Instruction rows that came out as the sum of their parts on an
     ARM926: a return through lr and bx, a conditional return, conditional
     instructions that execute, flag-setting, a register shift in an add,
     ldm of 3, the third register of an ldm used next, an ldm feeding a
     multiply, writeback, stm then ldm, push and pop of ten, mul then use
     (as mla then use), a load feeding a register shift.
   - Multiplies with 24-bit and negative Rm, and mul, smull and smlal by
     operand size: the same whatever the operands, as the rows kept show.
   - A miss and then the rest of its line, back to back, which could not
     tell a word arriving from the whole line arriving; "miss, then" can.
   - Dirty evictions and a last-word miss with 16 instructions after:
     the longer gaps kept say the same.  Misses 64 and 256 instructions
     apart, and an ldm of 2 after a miss: as their neighbours.
   - The pair rewrite, a replay of FLAC's decorrelation in DRAM.  It never
     behaved like the codec, because on the device the codec is not in
     DRAM.
   - The filter by where its state and code sit, and frames of adds:
     they led to "adds in a loop", which measures the cause directly.
   - Lines kept after a stream of misses, and after new lines in a set:
     too coarse and too noisy to tell how the victim is chosen; "first
     line kept" does. */
#define TEST_CYC_ARCHIVE 0

#define ITERS   200000ul        /* first try at loop iterations per run */
#define UNROLL  16

#define X16(i) i i i i i i i i i i i i i i i i

/* The data a benchmark touches: small, so the DRAM copy is a cache hit. */
static unsigned long iram_buf[64] IBSS_ATTR __attribute__((aligned(32)));
static unsigned long dram_buf[64] __attribute__((aligned(32)));

/* On PP5022/PP5024 plugins have 80 KB of IRAM, enough to map what a store
   costs across most of it: the extra cycle a store pays when its code and
   data share a memory turns out to depend on where in IRAM the data is. */
#if (CONFIG_CPU == PP5022 || CONFIG_CPU == PP5024) && defined(USE_IRAM)
#define IRAM_MAP_BYTES (64 * 1024)
static unsigned long iram_map[IRAM_MAP_BYTES / 4] IBSS_ATTR;
#endif


typedef void (*benchfn)(unsigned long *p, unsigned long n);

/* One benchmark, twice: once in IRAM, once in DRAM.  r4-r9 are free for the
   body; p and n come in whatever registers the compiler picks. */
#define BENCH(name, setup, body)                                            \
static void ICODE_ATTR UNUSED_ATTR                                          \
name##_i(unsigned long *p, unsigned long n)                                 \
{                                                                           \
    asm volatile (setup "1:\n" X16(body)                                    \
                  "   subs    %[n], %[n], #1\n"                             \
                  "   bne     1b\n"                                         \
                  : [n] "+r" (n) : [p] "r" (p)                              \
                  : "r4", "r5", "r6", "r7", "r8", "r9", "cc", "memory");    \
}                                                                           \
BENCH_D(name, setup, body)

/* The DRAM copy alone, for bodies that call the helpers below, which a
   branch from IRAM would not reach.  lr is free for the body too. */
#define BENCH_D(name, setup, body)                                          \
static void name##_d(unsigned long *p, unsigned long n)                     \
{                                                                           \
    asm volatile (setup "1:\n" X16(body)                                    \
                  "   subs    %[n], %[n], #1\n"                             \
                  "   bne     1b\n"                                         \
                  : [n] "+r" (n) : [p] "r" (p)                              \
                  : "r4", "r5", "r6", "r7", "r8", "r9", "lr", "cc",         \
                    "memory");                                              \
}

#define NOSETUP ""
#define MULSETUP "   mov r6, #0x3f00\n   orr r6, r6, #0xff\n   mov r7, r6\n"
/* A full-width multiplier, so the ARM7TDMI makes all four passes. */
#define MUL32SETUP "   mvn r6, #0x80000000\n   mov r7, r6\n"
/* Offsets for the register-offset loads: 4 bytes, or 1 word scaled. */
#define OFFSETUP "   mov r6, #4\n   mov r7, #1\n"

BENCH(b_empty,    NOSETUP, "")
BENCH(b_alu,      NOSETUP, "   add     r4, r5, #1\n")
BENCH(b_regshift, NOSETUP, "   mov     r4, r5, lsl r6\n")
BENCH(b_branch,   NOSETUP, "   b       .+4\n")
BENCH(b_ldr,      NOSETUP, "   ldr     r4, [%[p]]\n"
                           "   ldr     r5, [%[p], #4]\n")
BENCH(b_ldruse,   NOSETUP, "   ldr     r4, [%[p]]\n   add     r5, r4, #1\n")
BENCH(b_ldrb,     NOSETUP, "   ldrb    r4, [%[p]]\n"
                           "   ldrb    r5, [%[p], #4]\n")
BENCH(b_ldm2,     NOSETUP, "   ldmia   %[p], { r4, r5 }\n")
BENCH(b_ldm4,     NOSETUP, "   ldmia   %[p], { r4-r7 }\n")
BENCH(b_ldm6,     NOSETUP, "   ldmia   %[p], { r4-r9 }\n")
BENCH(b_str,      NOSETUP, "   str     r4, [%[p]]\n")
BENCH(b_stm2,     NOSETUP, "   stmia   %[p], { r4, r5 }\n")
BENCH(b_stm4,     NOSETUP, "   stmia   %[p], { r4-r7 }\n")
BENCH(b_str1,     NOSETUP, "   str     r4, [%[p]]\n   add     r5, r5, #1\n")
BENCH(b_str2,     NOSETUP, "   str     r4, [%[p]]\n   add     r5, r5, #1\n"
                           "   add     r6, r6, #1\n")
BENCH(b_stm2gap,  NOSETUP, "   stmia   %[p], { r4, r5 }\n"
                           "   add     r6, r6, #1\n"
                           "   add     r7, r7, #1\n")
BENCH(b_strldr,   NOSETUP, "   str     r4, [%[p]]\n"
                           "   ldr     r5, [%[p], #4]\n")
BENCH(b_ldrbuse,  NOSETUP, "   ldrb    r4, [%[p]]\n   add     r5, r4, #1\n")
BENCH(b_ldm2use,  NOSETUP, "   ldmia   %[p], { r4, r5 }\n"
                           "   add     r6, r5, #1\n")
BENCH(b_ldrh,     NOSETUP, "   ldrh    r4, [%[p]]\n"
                           "   ldrh    r5, [%[p], #4]\n")
BENCH(b_ldrsh,    NOSETUP, "   ldrsh   r4, [%[p]]\n"
                           "   ldrsh   r5, [%[p], #4]\n")
BENCH(b_ldrhuse,  NOSETUP, "   ldrh    r4, [%[p]]\n   add     r5, r4, #1\n")
BENCH(b_ldrshuse, NOSETUP, "   ldrsh   r4, [%[p]]\n   add     r5, r4, #1\n")
BENCH(b_ldrreg,   OFFSETUP, "   ldr     r4, [%[p], r6]\n"
                            "   ldr     r5, [%[p], r6]\n")
BENCH(b_ldrlsl,   OFFSETUP, "   ldr     r4, [%[p], r7, lsl #2]\n"
                            "   ldr     r5, [%[p], r7, lsl #2]\n")
BENCH(b_ldrbreg,  OFFSETUP, "   ldrb    r4, [%[p], r6]\n"
                            "   ldrb    r5, [%[p], r6]\n")
BENCH(b_ldrhreg,  OFFSETUP, "   ldrh    r4, [%[p], r6]\n"
                            "   ldrh    r5, [%[p], r6]\n")
BENCH(b_ldrreguse, OFFSETUP, "   ldr     r4, [%[p], r6]\n"
                             "   add     r5, r4, #1\n")
/* The loop's subs leaves the carry set while n has not run out, and this
   sets it for the first pass, so a "cc" instruction never executes. */
#define CCSETUP "   cmp r4, r4\n"
BENCH(b_addcc,    CCSETUP, "   addcc   r4, r5, #1\n")
BENCH(b_ldrcc,    CCSETUP, "   ldrcc   r4, [%[p]]\n")
BENCH(b_strcc,    CCSETUP, "   strcc   r4, [%[p]]\n")
BENCH(b_ldmcc,    CCSETUP, "   ldmccia %[p], { r4-r7 }\n")
BENCH(b_mlacc,    MULSETUP CCSETUP, "   mlacc   r4, r5, r6, r4\n")
BENCH(b_bcc,      CCSETUP, "   bcc     .+4\n")
/* Stack traffic, just below sp so nothing live is touched. */
BENCH(b_spstr,    NOSETUP, "   str     r4, [sp, #-4]\n")
BENCH(b_spstm,    NOSETUP, "   stmdb   sp, { r4-r7 }\n")
BENCH(b_spldr,    NOSETUP, "   ldr     r4, [sp, #-4]\n")
BENCH(b_sppush,   NOSETUP, "   stmfd   sp!, { r4-r7 }\n"
                           "   ldmfd   sp!, { r4-r7 }\n")
BENCH(b_mla,      MULSETUP, "   mla     r4, r5, r6, r4\n")
BENCH(b_mla32,    MUL32SETUP, "   mla     r4, r5, r6, r4\n")
BENCH(b_smlal32,  MUL32SETUP, "   smlal   r4, r5, r8, r7\n")
#if ARM_ARCH >= 5
BENCH(b_smulbb,   MULSETUP, "   smulbb  r4, r5, r6\n")
BENCH(b_smlabb,   MULSETUP, "   smlabb  r4, r5, r6, r4\n")
BENCH(b_smlalbb,  MULSETUP, "   smlalbb r4, r5, r8, r6\n")
BENCH(b_smulwb,   MULSETUP, "   smulwb  r4, r5, r6\n")
BENCH(b_smlawb,   MULSETUP, "   smlawb  r4, r5, r6, r4\n")
BENCH(b_clz,      NOSETUP, "   clz     r4, r5\n")
BENCH(b_qadd,     NOSETUP, "   qadd    r4, r5, r6\n")
BENCH(b_ldrd,     NOSETUP, "   ldrd    r4, r5, [%[p]]\n")
BENCH(b_strd,     NOSETUP, "   strd    r4, r5, [%[p]]\n")
BENCH(b_pld,      NOSETUP, "   pld     [%[p]]\n")
#endif
#if ARM_ARCH >= 6
BENCH(b_smmul,    MULSETUP, "   smmul   r4, r5, r6\n")
BENCH(b_smmla,    MULSETUP, "   smmla   r4, r5, r6, r4\n")
BENCH(b_smmulr,   MULSETUP, "   smmulr  r4, r5, r6\n")
BENCH(b_smuad,    MULSETUP, "   smuad   r4, r5, r6\n")
BENCH(b_smlad,    MULSETUP, "   smlad   r4, r5, r6, r4\n")
BENCH(b_smlald,   MULSETUP, "   smlald  r4, r5, r8, r6\n")
BENCH(b_umaal,    MULSETUP, "   umaal   r4, r5, r8, r6\n")
BENCH(b_muluse,   MULSETUP, "   mul     r4, r5, r6\n   add     r7, r4, #1\n")
BENCH(b_smulluse, MULSETUP, "   smull   r4, r5, r8, r6\n"
                            "   add     r7, r5, #1\n")
BENCH(b_ssat,     NOSETUP, "   ssat    r4, #16, r5\n")
BENCH(b_rev,      NOSETUP, "   rev     r4, r5\n")
BENCH(b_sxth,     NOSETUP, "   sxth    r4, r5\n")
BENCH(b_pkhbt,    NOSETUP, "   pkhbt   r4, r5, r6, lsl #16\n")
BENCH(b_sadd16,   NOSETUP, "   sadd16  r4, r5, r6\n")
BENCH(b_qadd16,   NOSETUP, "   qadd16  r4, r5, r6\n")
#endif
BENCH(b_smlal,    MULSETUP, "   smlal   r4, r5, r8, r7\n")

/* Calls and returns, results used by the next instruction, and conditional
   instructions that execute: what compiled code does all the time and the
   rows above do not.  The helpers are called from DRAM code only. */
void tc_leaf(void);
void tc_pop2(void);
void tc_poplr2(void);
void tc_pop10(void);
void tc_popne10(void);
__asm__(".text\n.align 2\n"
        ".type tc_leaf, %function\n"
        "tc_leaf:\n"
        "   bx      lr\n"
        ".type tc_pop2, %function\n"
        "tc_pop2:\n"
        "   stmfd   sp!, { r4, lr }\n"
        "   ldmfd   sp!, { r4, pc }\n"
        ".type tc_poplr2, %function\n"
        "tc_poplr2:\n"
        "   stmfd   sp!, { r4, lr }\n"
        "   ldmfd   sp!, { r4, lr }\n"
        "   bx      lr\n"
        ".type tc_pop10, %function\n"
        "tc_pop10:\n"
        "   stmfd   sp!, { r4-r11, ip, lr }\n"
        "   ldmfd   sp!, { r4-r11, ip, pc }\n"
        ".type tc_popne10, %function\n"
        "tc_popne10:\n"
        "   stmfd   sp!, { r4-r11, ip, lr }\n"
        "   cmp     sp, #0\n"
        "   ldmnefd sp!, { r4-r11, ip, pc }\n");
/* The loop's subs leaves Z clear while n has not run out, and this clears
   it for the first pass, so an "ne" instruction always executes. */
#define NESETUP "   cmp sp, #0\n"
BENCH_D(b_call,     NOSETUP, "   bl      tc_leaf\n")
BENCH_D(b_callpop,  NOSETUP, "   bl      tc_pop2\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_callpoplr, NOSETUP, "   bl      tc_poplr2\n")
#endif
BENCH_D(b_callpop10, NOSETUP, "   bl      tc_pop10\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_callpopne, NOSETUP, "   bl      tc_popne10\n")
#endif
BENCH_D(b_bx,       NOSETUP, "   adr     r4, 2f\n   bx      r4\n2:\n")
BENCH_D(b_ldrpc,    NOSETUP, "   adr     r4, 2f\n   str     r4, [%[p]]\n"
                             "   ldr     pc, [%[p]]\n2:\n")
BENCH_D(b_bne,      NESETUP, "   bne     .+4\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_addne,    NESETUP, "   addne   r4, r5, #1\n")
BENCH_D(b_ldmne,    NESETUP, "   ldmneia %[p], { r4-r7 }\n")
BENCH_D(b_stmne,    NESETUP, "   stmneia %[p], { r4-r7 }\n")
BENCH_D(b_ands,     NOSETUP, "   ands    r4, r5, #63\n")
#endif
BENCH_D(b_orrasr,   NOSETUP, "   orr     r4, r5, r6, asr #30\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_addasr,   NOSETUP, "   add     r4, r5, r6, asr r7\n")
BENCH_D(b_ldm3,     NOSETUP, "   ldmia   %[p], { r4, r5, lr }\n")
#endif
BENCH_D(b_ldm4first, NOSETUP, "   ldmia   %[p], { r4-r7 }\n"
                              "   add     r8, r4, #1\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_ldm4third, NOSETUP, "   ldmia   %[p], { r4-r7 }\n"
                              "   add     r8, r6, #1\n")
#endif
BENCH_D(b_ldm4last, NOSETUP, "   ldmia   %[p], { r4-r7 }\n"
                             "   add     r8, r7, #1\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_ldm4mla,  NOSETUP, "   ldmia   %[p], { r4-r7 }\n"
                             "   mla     r8, r4, r5, r8\n")
BENCH_D(b_ldmwb,    "   mov r8, %[p]\n",
                    "   ldmia   r8!, { r4-r7 }\n   sub     r8, r8, #16\n")
BENCH_D(b_stmwb,    "   mov r8, %[p]\n",
                    "   stmia   r8!, { r4-r7 }\n   sub     r8, r8, #16\n")
BENCH_D(b_stmldm,   "   add r8, %[p], #32\n",
                    "   stmia   %[p], { r4-r7 }\n   ldmia   r8, { r4-r7 }\n")
#endif
BENCH_D(b_strldrsame, NOSETUP, "   str     r4, [%[p]]\n"
                               "   ldr     r5, [%[p]]\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_push10,   NOSETUP, "   stmfd   sp!, { r4-r11, ip, lr }\n"
                             "   ldmfd   sp!, { r4-r11, ip, lr }\n")
#endif
BENCH_D(b_mlause,   MULSETUP, "   mla     r4, r5, r6, r7\n"
                              "   add     r8, r4, #1\n")
BENCH_D(b_mlamul,   MULSETUP, "   mla     r4, r5, r6, r7\n"
                              "   mla     r8, r4, r6, r7\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_muladd,   MULSETUP, "   mul     r4, r5, r6\n"
                              "   add     r8, r4, #1\n")
#endif
BENCH_D(b_smullacc, MULSETUP, "   smull   r4, r5, r8, r6\n"
                              "   add     r7, r5, #1\n")
BENCH_D(b_ldrmla,   MULSETUP, "   ldr     r4, [%[p]]\n"
                              "   mla     r8, r4, r6, r8\n")
#if TEST_CYC_ARCHIVE
BENCH_D(b_ldrshift, NOSETUP, "   ldr     r6, [%[p]]\n"
                             "   add     r4, r5, r4, asr r6\n")
#endif

/* A pointer chase: each word holds the address of the next one, stride
   bytes on, wrapping at the end.  Sixteen dependent loads per iteration, so
   the loop's two instructions are a small share and every load waits for the
   last. */
static void chase(unsigned long *start, unsigned long n)
{
    asm volatile (
        "   mov     r4, %[s]\n"
        "1:\n"
        X16("   ldr     r4, [r4]\n")
        "   subs    %[n], %[n], #1\n"
        "   bne     1b\n"
        : [n] "+r" (n) : [s] "r" (start) : "r4", "cc", "memory");
}

/* Sequential loads through sbuf, which is sbytes long (a multiple of 512),
   rewinding at the end.  r4-r7 take the data and nothing reads it.  p and
   n are early-clobber: p starts equal to sbuf, and gcc would otherwise give
   p and base one register, making the rewind a no-op. */
static unsigned long *sbuf;
static unsigned long sbytes;

#define STREAM(name, body)                                                  \
static void name(unsigned long *unused, unsigned long n)                    \
{                                                                           \
    unsigned long *p = sbuf, *end = sbuf + sbytes / 4;                      \
    (void)unused;                                                           \
    asm volatile ("1:\n" X16(body)                                          \
                  "   cmp     %[p], %[end]\n"                               \
                  "   movhs   %[p], %[base]\n"                              \
                  "   subs    %[n], %[n], #1\n"                             \
                  "   bne     1b\n"                                         \
                  : [n] "+&r" (n), [p] "+&r" (p)                            \
                  : [end] "r" (end), [base] "r" (sbuf)                      \
                  : "r4", "r5", "r6", "r7", "cc", "memory");                \
}
STREAM(stream_word, "   ldr     r4, [%[p]], #4\n")
STREAM(stream_line, "   ldr     r4, [%[p]], #32\n")
STREAM(stream_ldm4, "   ldmia   %[p]!, { r4-r7 }\n")
/* The same with stores, to price a store whose line is not cached. */
STREAM(wstream_word, "   str     r4, [%[p]], #4\n")
STREAM(wstream_line, "   str     r4, [%[p]], #32\n")
STREAM(wstream_stm4, "   stmia   %[p]!, { r4-r7 }\n")

/* A load per line with work between that touches no memory, to see whether
   a miss costs less when the next access is not waiting on it. */
#define X8(i) i i i i i i i i
#define WORK8 X8("   add     r5, r5, #1\n")
STREAM(stream_work8, "   ldr     r4, [%[p]], #32\n" WORK8)
STREAM(stream_work16, "   ldr     r4, [%[p]], #32\n" WORK8 WORK8)
STREAM(stream_work32, "   ldr     r4, [%[p]], #32\n" WORK8 WORK8 WORK8 WORK8)

/* A miss on the last word of each line, and on its fourth: if the fill
   brings the word asked for first these cost what a miss on the first word
   does, and if it runs in order from the start of the line they cost
   more. */
STREAM(stream_last, "   ldr     r4, [%[p], #28]\n   add     %[p], %[p], #32\n")
STREAM(stream_mid, "   ldr     r4, [%[p], #12]\n   add     %[p], %[p], #32\n")
STREAM(stream_first, "   ldr     r4, [%[p]]\n   add     %[p], %[p], #32\n")
#if TEST_CYC_ARCHIVE
/* The same misses with the rest of the line read straight after, a word
   at a time, in order from the start. */
#define REST_OF_LINE \
    "   ldr     r5, [%[p]]\n   ldr     r5, [%[p], #4]\n"                    \
    "   ldr     r5, [%[p], #8]\n   ldr     r5, [%[p], #16]\n"               \
    "   ldr     r5, [%[p], #20]\n   ldr     r5, [%[p], #24]\n"              \
    "   add     %[p], %[p], #32\n"
STREAM(stream_last_rest, "   ldr     r4, [%[p], #28]\n" REST_OF_LINE)
STREAM(stream_mid_rest, "   ldr     r4, [%[p], #12]\n" REST_OF_LINE)
#endif

/* A miss whose line is then written, so that every line a later miss
   evicts is dirty and has to go back to memory first: one store dirties
   half the line on the ARM926, two the whole of it. */
STREAM(stream_dirty4, "   ldr     r4, [%[p]]\n   str     r4, [%[p]]\n"
                      "   add     %[p], %[p], #32\n")
STREAM(stream_dirty8, "   ldr     r4, [%[p]]\n   str     r4, [%[p]]\n"
                      "   str     r4, [%[p], #16]\n"
                      "   add     %[p], %[p], #32\n")

/* The dirty eviction and the miss on a last word again, with work after
   that touches no memory: whether the write-back, and the fill from the
   last word, hold the core or only the next miss. */
#define DIRTY8 "   ldr     r4, [%[p]]\n   str     r4, [%[p]]\n"              \
               "   str     r4, [%[p], #16]\n   add     %[p], %[p], #32\n"
#define WORK16 WORK8 WORK8
#define WORK32 WORK16 WORK16
#if TEST_CYC_ARCHIVE
STREAM(stream_dirty8_w16, DIRTY8 WORK16)
#endif
STREAM(stream_dirty8_w32, DIRTY8 WORK32)
STREAM(stream_dirty8_w64, DIRTY8 WORK32 WORK32)
STREAM(stream_dirty8_w96, DIRTY8 WORK32 WORK32 WORK32)
#define LAST "   ldr     r4, [%[p], #28]\n   add     %[p], %[p], #32\n"
#if TEST_CYC_ARCHIVE
STREAM(stream_last_w16, LAST WORK16)
#endif
STREAM(stream_last_w32, LAST WORK32)

/* A miss with a long way to the next one, and what else is read from its
   line straight after: whether the stall is all there is when nothing
   follows, and what a second word of the line, or the rest of an ldm,
   waits for. */
#define WORK64 WORK32 WORK32
#define LINE0 "   ldr     r4, [%[p]]\n"
#define NEXTLINE "   add     %[p], %[p], #32\n"
STREAM(gap_32, LINE0 NEXTLINE WORK32)
#if TEST_CYC_ARCHIVE
STREAM(gap_64, LINE0 NEXTLINE WORK64)
#endif
STREAM(gap_128, LINE0 NEXTLINE WORK64 WORK64)
#if TEST_CYC_ARCHIVE
STREAM(gap_256, LINE0 NEXTLINE WORK64 WORK64 WORK64 WORK64)
#endif
STREAM(gap_w1, LINE0 "   ldr     r5, [%[p], #4]\n" NEXTLINE WORK32)
STREAM(gap_w7, LINE0 "   ldr     r5, [%[p], #28]\n" NEXTLINE WORK32)
#if TEST_CYC_ARCHIVE
STREAM(gap_ldm2, "   ldmia   %[p], { r4, r5 }\n" NEXTLINE WORK32)
#endif
STREAM(gap_ldm4, "   ldmia   %[p], { r4-r7 }\n" NEXTLINE WORK32)
STREAM(gap_ldm4mid, "   add     r5, %[p], #8\n"
                    "   ldmia   r5, { r4-r7 }\n" NEXTLINE WORK32)
STREAM(gap_str, LINE0 "   str     r4, [%[p]]\n" NEXTLINE WORK32)

/* A copy, four words at a time, to 128 KB further on: line fills and
   buffered writes both wanting the bus, as in a memcpy between buffers
   that are not in the cache. */
STREAM(copy_4, "   ldmia   %[p], { r4-r7 }\n"
               "   add     r4, %[p], #0x20000\n"
               "   stmia   r4, { r4-r7 }\n"
               "   add     %[p], %[p], #16\n")

/* The chase with work after each load, for a miss far from the last. */
static void chase_work32(unsigned long *start, unsigned long n)
{
    asm volatile (
        "   mov     r4, %[s]\n"
        "1:\n"
        X16("   ldr     r4, [r4]\n" WORK32)
        "   subs    %[n], %[n], #1\n"
        "   bne     1b\n"
        : [n] "+r" (n) : [s] "r" (start) : "r4", "r5", "cc", "memory");
}

#if TEST_CYC_ARCHIVE
/* What a codec does with a block of two channels: each buffer is filtered
   in place, one after the other (pair_filter), then both are rewritten
   together, four words of each per pass (pair_pass, the loop of the FLAC
   decorrelation).  pair_frame is all three and pair_filters the first two,
   so their difference is the passes alone, as they find the cache. */
void pair_filter(unsigned long *p, unsigned long passes);
void pair_pass(unsigned long *a, unsigned long *b, unsigned long passes);
__asm__(".text\n.align 2\n.type pair_filter, %function\n"
        "pair_filter:\n"
        "   stmfd   sp!, { r4-r7 }\n"
        "1:\n"
        "   ldmia   r0, { r4-r7 }\n"
        "   stmia   r0!, { r4-r7 }\n"
        "   subs    r1, r1, #1\n"
        "   bne     1b\n"
        "   ldmfd   sp!, { r4-r7 }\n"
        "   bx      lr\n"
        ".type pair_pass, %function\n"
        "pair_pass:\n"
        "   stmfd   sp!, { r4-r11 }\n"
        "   mov     r3, #0\n"
        "1:\n"
        "   ldmia   r0, { r4-r7 }\n"
        "   ldmia   r1, { r8-r11 }\n"
        "   mov     r4, r4, lsl r3\n"
        "   mov     r8, r8, lsl r3\n"
        "   mov     r5, r5, lsl r3\n"
        "   mov     r9, r9, lsl r3\n"
        "   mov     r6, r6, lsl r3\n"
        "   mov     r10, r10, lsl r3\n"
        "   mov     r7, r7, lsl r3\n"
        "   mov     r11, r11, lsl r3\n"
        "   stmia   r0!, { r4-r7 }\n"
        "   stmia   r1!, { r8-r11 }\n"
        "   subs    r2, r2, #1\n"
        "   bne     1b\n"
        "   ldmfd   sp!, { r4-r11 }\n"
        "   bx      lr\n");

static unsigned long *pair_a, *pair_b;
static unsigned long pair_passes;       /* 16 bytes of each buffer a pass */

static void pair_filters(unsigned long *unused, unsigned long n)
{
    (void)unused;
    while (n--)
    {
        pair_filter(pair_a, pair_passes);
        pair_filter(pair_b, pair_passes);
    }
}

static void pair_frame(unsigned long *unused, unsigned long n)
{
    (void)unused;
    while (n--)
    {
        pair_filter(pair_a, pair_passes);
        pair_filter(pair_b, pair_passes);
        pair_pass(pair_a, pair_b, pair_passes);
    }
}
#endif

/* Taken branches, one per cache line of DRAM code, each to the next, so
   over more code than the cache holds every one is a fetch miss.  Lines
   are 16 bytes on the PP502x and 32 on the ARM9 targets. */
#if ARM_ARCH >= 5
#define ICHAIN_PAD "28"
#else
#define ICHAIN_PAD "12"
#endif
#define ICHAIN(name, lines)                                                 \
static void name(unsigned long *unused, unsigned long n)                    \
{                                                                           \
    (void)unused;                                                           \
    asm volatile ("1:\n"                                                    \
                  "   .rept " #lines "\n"                                   \
                  "   b       2f\n"                                         \
                  "   .space  " ICHAIN_PAD "\n"                             \
                  "2:\n"                                                    \
                  "   .endr\n"                                              \
                  "   subs    %[n], %[n], #1\n"                             \
                  "   bne     1b\n"                                         \
                  : [n] "+r" (n) : : "cc");                                 \
}
#define ICHAIN_SMALL 32                 /* 512 bytes, or 1 KB */
#define ICHAIN_BIG   2048               /* 32 KB, or 64 KB */
ICHAIN(ichain_small, 32)
ICHAIN(ichain_big, 2048)
ICHAIN(ichain_1, 1)
ICHAIN(ichain_2, 2)
ICHAIN(ichain_3, 3)
ICHAIN(ichain_4, 4)
ICHAIN(ichain_5, 5)
ICHAIN(ichain_6, 6)
ICHAIN(ichain_7, 7)
ICHAIN(ichain_8, 8)
ICHAIN(ichain_9, 9)
ICHAIN(ichain_10, 10)
ICHAIN(ichain_12, 12)
ICHAIN(ichain_16, 16)
ICHAIN(ichain_64, 64)
ICHAIN(ichain_128, 128)
ICHAIN(ichain_256, 256)

/* Adds in a row and nothing else, the loop starting on a cache line: how
   much code a loop can run through before an instruction costs more than
   its cycle. */
#define SEQ(name, count)                                                    \
static void name(unsigned long *unused, unsigned long n)                    \
{                                                                           \
    (void)unused;                                                           \
    asm volatile ("   b       1f\n"                                         \
                  "   .balign 32\n"                                         \
                  "1:\n"                                                    \
                  "   .rept " #count "\n"                                   \
                  "   add     r4, r4, #1\n"                                 \
                  "   .endr\n"                                              \
                  "   subs    %[n], %[n], #1\n"                             \
                  "   bne     1b\n"                                         \
                  : [n] "+r" (n) : : "r4", "cc");                           \
}
SEQ(seq_6, 6)
SEQ(seq_14, 14)
SEQ(seq_22, 22)
SEQ(seq_30, 30)
SEQ(seq_38, 38)
SEQ(seq_46, 46)
SEQ(seq_54, 54)
SEQ(seq_62, 62)
SEQ(seq_70, 70)
SEQ(seq_78, 78)
SEQ(seq_94, 94)
SEQ(seq_126, 126)
SEQ(seq_254, 254)
SEQ(seq_510, 510)
SEQ(seq_1022, 1022)
SEQ(seq_2046, 2046)

/* The same chain in scattered order: a full-period linear congruence over
   the nodes (bytes / stride, a power of two), so consecutive loads are far
   apart instead of next to each other. */
static void make_chain_scattered(unsigned char *buf, unsigned long bytes,
                                 unsigned long stride)
{
    unsigned long i, nodes = bytes / stride;
    for (i = 0; i < nodes; i++)
        *(unsigned long *)(buf + i * stride) = (unsigned long)
            (buf + ((i * 1664525ul + 1013904223ul) & (nodes - 1)) * stride);
}

static void make_chain(unsigned char *buf, unsigned long bytes,
                       unsigned long stride)
{
    unsigned long off;
    for (off = 0; off < bytes; off += stride)
        *(unsigned long *)(buf + off) =
            (unsigned long)(buf + (off + stride) % bytes);
}

/* The finest free-running timer each SoC has.  Plugins do not see the SoC
   headers, so the registers are named here, as USEC_TIMER (or the
   S5L8700's 5 us counter) is defined for each in firmware/.  Anywhere
   else the tick has to do. */
#if CONFIG_CPU == PP5020 || CONFIG_CPU == PP5022 || CONFIG_CPU == PP5024
#define NOW()       (*(volatile unsigned long *)0x60005010)
#define PER_SEC     1000000ll
#define TIMER_NAME  "PP502x usec timer"
#elif CONFIG_CPU == S5L8700 || CONFIG_CPU == S5L8701
#define NOW()       (*(volatile unsigned long *)0x3C700084)   /* TICNTL */
#define PER_SEC     200000ll
#define TIMER_NAME  "S5L8700 5 usec timer"
#elif CONFIG_CPU == PP5002
#define NOW()       (*(volatile unsigned long *)0xcf001110)
#define PER_SEC     1000000ll
#define TIMER_NAME  "PP5002 usec timer"
#elif CONFIG_CPU == S5L8702 || CONFIG_CPU == S5L8720
#define NOW()       (*(volatile unsigned long *)0x3C7000B4)   /* TECNT */
#define PER_SEC     1000000ll
#define TIMER_NAME  "S5L8702/8720 usec timer"
#elif CONFIG_CPU == TCC7801
#define NOW()       (*(volatile unsigned long *)0xF3003094)   /* TC32MCNT */
#define PER_SEC     1000000ll
#define TIMER_NAME  "TCC7801 usec timer"
#elif CONFIG_CPU == IMX233
#define NOW()       (*(volatile unsigned long *)0x8001c0c0)   /* DIGCTL usec */
#define PER_SEC     1000000ll
#define TIMER_NAME  "i.MX233 usec timer"
#elif CONFIG_CPU == AS3525v2
/* No free-running counter, but the kernel tick's timer counts down from
   15000 at 1.5 MHz, so the tick and that together are one. */
static unsigned long as3525_now(void)
{
    long t;
    unsigned long v;
    do {
        t = *rb->current_tick;
        v = *(volatile unsigned long *)0xC8040024;      /* TIMER2_VALUE */
    } while (t != *rb->current_tick);
    return (unsigned long)t * 15000 + (15000 - v);
}
#define NOW()       as3525_now()
#define PER_SEC     1500000ll
#define TIMER_NAME  "AS3525v2 tick timer, 2/3 usec"
#else
#define NOW()       (*rb->current_tick)
#define PER_SEC     ((long long)HZ)
#define TICK_TIMED
#define TIMER_NAME  "the tick: runs of a second or more"
#endif

#ifdef TICK_TIMED
#define MIN_ELAPSED 100ul               /* a second of ticks */
#else
#define MIN_ELAPSED ((unsigned long)(PER_SEC / 50))  /* 20 ms */
#endif

/* Hundredths of a cycle per loop iteration.  The run doubles in length until
   the clock has moved far enough for the figure to be good to a percent. */
static long long cpi_from(benchfn fn, unsigned long *p, unsigned long n)
{
    unsigned long t0, t1;
    fn(p, n < 64 ? n : 64);             /* warm the caches */
    for (;;)
    {
        t0 = NOW();
        fn(p, n);
        t1 = NOW();
        if (t1 - t0 >= MIN_ELAPSED || n >= (1ul << 28))
            break;
        n *= 2;
    }
    return (long long)(t1 - t0) * *rb->cpu_frequency * 100 / PER_SEC / n;
}

static long long cpi(benchfn fn, unsigned long *p)
{
    return cpi_from(fn, p, ITERS);
}

static int fd = -1;

/* The screen keeps as many of the latest lines as fit, older ones scrolling
   off the top; the file gets every line. */
#define MAXROWS 64
static char shown[MAXROWS][48];
static int nshown;

static void say(const char *fmt, ...)
{
    char buf[96];
    int i, rows;
    va_list ap;
    va_start(ap, fmt);
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (fd >= 0)
    {
        rb->write(fd, buf, rb->strlen(buf));
        rb->write(fd, "\n", 1);
    }
    rows = LCD_HEIGHT / rb->font_get(FONT_SYSFIXED)->height;
    if (rows > MAXROWS)
        rows = MAXROWS;
    if (rows < 1)
        rows = 1;
    if (nshown >= rows)
    {
        rb->memmove(shown[0], shown[nshown - rows + 1],
                    (rows - 1) * sizeof(shown[0]));
        nshown = rows - 1;
    }
    rb->strlcpy(shown[nshown++], buf, sizeof(shown[0]));
    rb->lcd_clear_display();
    for (i = 0; i < nshown; i++)
        rb->lcd_puts(0, i, shown[i]);
    rb->lcd_update();
}

/* Hundredths of a cycle per instruction, over the loop's own cost. */
static int per_insn(long long cyc, long long empty)
{
    return (int)((cyc - empty) / UNROLL);
}

/* Multiplies by the size of each operand: Rm in r5 and Rs in r6, each
   14 bits, 24 bits, 31 bits or a small negative number. */
#define RM14  "   mov r5, #0x3f00\n   orr r5, r5, #0xff\n"
#define RM24  "   mov r5, #0x7f0000\n   orr r5, r5, #0xff00\n"
#define RM31  "   mvn r5, #0x80000000\n"
#define RMNEG "   mvn r5, #0x3f00\n"
#define RS14  "   mov r6, #0x3f00\n   orr r6, r6, #0xff\n"
#define RS24  "   mov r6, #0x7f0000\n   orr r6, r6, #0xff00\n"
#define RS31  "   mvn r6, #0x80000000\n"
#define RSNEG "   mvn r6, #0x3f00\n"
#define MLA   "   mla     r4, r5, r6, r4\n"
#define MUL   "   mul     r4, r5, r6\n"
#define SMULL "   smull   r8, r9, r5, r6\n"
#define SMLAL "   smlal   r8, r9, r5, r6\n"
BENCH_D(b_mla_14_14, RM14 RS14, MLA)
#if TEST_CYC_ARCHIVE
BENCH_D(b_mla_24_14, RM24 RS14, MLA)
#endif
BENCH_D(b_mla_31_14, RM31 RS14, MLA)
#if TEST_CYC_ARCHIVE
BENCH_D(b_mla_n_14,  RMNEG RS14, MLA)
BENCH_D(b_mla_14_24, RM14 RS24, MLA)
#endif
BENCH_D(b_mla_14_31, RM14 RS31, MLA)
BENCH_D(b_mla_14_n,  RM14 RSNEG, MLA)
#if TEST_CYC_ARCHIVE
BENCH_D(b_mla_24_24, RM24 RS24, MLA)
#endif
BENCH_D(b_mla_31_31, RM31 RS31, MLA)
#if TEST_CYC_ARCHIVE
BENCH_D(b_mul_14_14, RM14 RS14, MUL)
BENCH_D(b_mul_31_14, RM31 RS14, MUL)
BENCH_D(b_mul_14_31, RM14 RS31, MUL)
BENCH_D(b_mul_31_31, RM31 RS31, MUL)
#endif
BENCH_D(b_smull_14_14, RM14 RS14, SMULL)
#if TEST_CYC_ARCHIVE
BENCH_D(b_smull_31_14, RM31 RS14, SMULL)
BENCH_D(b_smull_14_31, RM14 RS31, SMULL)
#endif
BENCH_D(b_smull_31_31, RM31 RS31, SMULL)
#if TEST_CYC_ARCHIVE
BENCH_D(b_smlal_14_14, RM14 RS14, SMLAL)
BENCH_D(b_smlal_31_31, RM31 RS31, SMLAL)
#endif

struct row {
    const char *name;
    benchfn i, d;
    int per;                            /* instructions per X16 copy */
    int alu;                            /* ALU cycles per copy, subtracted */
};

/* DRAM code only; a call row is the whole call, there and back, and the
   pairs are both instructions together. */
#define D(fn) fn##_d, fn##_d
static const struct row B2[] = {
    { "bl, bx lr ", D(b_call),       1, 0 },
    { "bl,pop2 pc", D(b_callpop),    1, 0 },
#if TEST_CYC_ARCHIVE
    { "bl,pop2 lr", D(b_callpoplr),  1, 0 },
#endif
    { "bl,pop10pc", D(b_callpop10),  1, 0 },
#if TEST_CYC_ARCHIVE
    { "bl,popne10", D(b_callpopne),  1, 1 },
#endif
    { "bx reg    ", D(b_bx),         1, 1 },
    { "ldr pc    ", D(b_ldrpc),      1, 2 },
    { "bne taken ", D(b_bne),        1, 0 },
#if TEST_CYC_ARCHIVE
    { "addne     ", D(b_addne),      1, 0 },
    { "ldmne 4   ", D(b_ldmne),      1, 0 },
    { "stmne 4   ", D(b_stmne),      1, 0 },
    { "ands      ", D(b_ands),       1, 0 },
#endif
    { "orr asr # ", D(b_orrasr),     1, 0 },
#if TEST_CYC_ARCHIVE
    { "add asr Rs", D(b_addasr),     1, 0 },
    { "ldm 3     ", D(b_ldm3),       1, 0 },
#endif
    { "ldm4+use 1", D(b_ldm4first),  1, 0 },
#if TEST_CYC_ARCHIVE
    { "ldm4+use 3", D(b_ldm4third),  1, 0 },
#endif
    { "ldm4+use 4", D(b_ldm4last),   1, 0 },
#if TEST_CYC_ARCHIVE
    { "ldm4+mla 1", D(b_ldm4mla),    1, 0 },
    { "ldm 4 wb  ", D(b_ldmwb),      1, 1 },
    { "stm 4 wb  ", D(b_stmwb),      1, 1 },
    { "stm4+ldm4 ", D(b_stmldm),     1, 0 },
#endif
    { "str+ldr = ", D(b_strldrsame), 1, 0 },
#if TEST_CYC_ARCHIVE
    { "push+pop10", D(b_push10),     1, 0 },
#endif
    { "mla+use   ", D(b_mlause),     1, 0 },
    { "mla+mla Rm", D(b_mlamul),     1, 0 },
#if TEST_CYC_ARCHIVE
    { "mul+use   ", D(b_muladd),     1, 0 },
#endif
    { "smull+use ", D(b_smullacc),   1, 0 },
    { "ldr+mla   ", D(b_ldrmla),     1, 0 },
#if TEST_CYC_ARCHIVE
    { "ldr+asr Rs", D(b_ldrshift),   1, 0 },
#endif
};
#define NB2 ((int)(sizeof(B2) / sizeof(B2[0])))

/* Multiplies, named by the bits of Rm and of Rs; n is a small negative. */
static const struct row B3[] = {
    { "mla 14x14 ", D(b_mla_14_14),   1, 0 },
#if TEST_CYC_ARCHIVE
    { "mla 24x14 ", D(b_mla_24_14),   1, 0 },
#endif
    { "mla 31x14 ", D(b_mla_31_14),   1, 0 },
#if TEST_CYC_ARCHIVE
    { "mla  nx14 ", D(b_mla_n_14),    1, 0 },
    { "mla 14x24 ", D(b_mla_14_24),   1, 0 },
#endif
    { "mla 14x31 ", D(b_mla_14_31),   1, 0 },
    { "mla 14xn  ", D(b_mla_14_n),    1, 0 },
#if TEST_CYC_ARCHIVE
    { "mla 24x24 ", D(b_mla_24_24),   1, 0 },
#endif
    { "mla 31x31 ", D(b_mla_31_31),   1, 0 },
#if TEST_CYC_ARCHIVE
    { "mul 14x14 ", D(b_mul_14_14),   1, 0 },
    { "mul 31x14 ", D(b_mul_31_14),   1, 0 },
    { "mul 14x31 ", D(b_mul_14_31),   1, 0 },
    { "mul 31x31 ", D(b_mul_31_31),   1, 0 },
#endif
    { "smull14x14", D(b_smull_14_14), 1, 0 },
#if TEST_CYC_ARCHIVE
    { "smull31x14", D(b_smull_31_14), 1, 0 },
    { "smull14x31", D(b_smull_14_31), 1, 0 },
#endif
    { "smull31x31", D(b_smull_31_31), 1, 0 },
#if TEST_CYC_ARCHIVE
    { "smlal14x14", D(b_smlal_14_14), 1, 0 },
    { "smlal31x31", D(b_smlal_31_31), 1, 0 },
#endif
};
#define NB3 ((int)(sizeof(B3) / sizeof(B3[0])))

static const struct row B[] = {
    { "add       ", b_alu_i,      b_alu_d,      1, 0 },
    { "mov lsl Rs", b_regshift_i, b_regshift_d, 1, 0 },
    { "b taken   ", b_branch_i,   b_branch_d,   1, 0 },
    { "ldr       ", b_ldr_i,      b_ldr_d,      2, 0 },
    { "ldr+use   ", b_ldruse_i,   b_ldruse_d,   1, 0 },
    { "ldrb      ", b_ldrb_i,     b_ldrb_d,     2, 0 },
    { "ldm 2     ", b_ldm2_i,     b_ldm2_d,     1, 0 },
    { "ldm 4     ", b_ldm4_i,     b_ldm4_d,     1, 0 },
    { "ldm 6     ", b_ldm6_i,     b_ldm6_d,     1, 0 },
    { "str       ", b_str_i,      b_str_d,      1, 0 },
    { "stm 2     ", b_stm2_i,     b_stm2_d,     1, 0 },
    { "stm 4     ", b_stm4_i,     b_stm4_d,     1, 0 },
    { "str,1 alu ", b_str1_i,     b_str1_d,     1, 1 },
    { "str,2 alu ", b_str2_i,     b_str2_d,     1, 2 },
    { "stm2,2 alu", b_stm2gap_i,  b_stm2gap_d,  1, 2 },
    { "str+ldr   ", b_strldr_i,   b_strldr_d,   1, 0 },
    { "ldrb+use  ", b_ldrbuse_i,  b_ldrbuse_d,  1, 1 },
    { "ldm2+use  ", b_ldm2use_i,  b_ldm2use_d,  1, 1 },
    { "ldrh      ", b_ldrh_i,     b_ldrh_d,     2, 0 },
    { "ldrsh     ", b_ldrsh_i,    b_ldrsh_d,    2, 0 },
    { "ldrh+use  ", b_ldrhuse_i,  b_ldrhuse_d,  1, 1 },
    { "ldrsh+use ", b_ldrshuse_i, b_ldrshuse_d, 1, 1 },
    { "ldr [Rn,Rm", b_ldrreg_i,   b_ldrreg_d,   2, 0 },
    { "ldr Rm lsl", b_ldrlsl_i,   b_ldrlsl_d,   2, 0 },
    { "ldrb Rm   ", b_ldrbreg_i,  b_ldrbreg_d,  2, 0 },
    { "ldrh Rm   ", b_ldrhreg_i,  b_ldrhreg_d,  2, 0 },
    { "ldr Rm+use", b_ldrreguse_i, b_ldrreguse_d, 1, 1 },
    { "addcc fail", b_addcc_i,    b_addcc_d,    1, 0 },
    { "ldrcc fail", b_ldrcc_i,    b_ldrcc_d,    1, 0 },
    { "strcc fail", b_strcc_i,    b_strcc_d,    1, 0 },
    { "ldmcc fail", b_ldmcc_i,    b_ldmcc_d,    1, 0 },
    { "mlacc fail", b_mlacc_i,    b_mlacc_d,    1, 0 },
    { "bcc fail  ", b_bcc_i,      b_bcc_d,      1, 0 },
    { "mla 14b Rs", b_mla_i,      b_mla_d,      1, 0 },
    { "smlal 14b ", b_smlal_i,    b_smlal_d,    1, 0 },
    { "mla 32b Rs", b_mla32_i,    b_mla32_d,    1, 0 },
    { "smlal 32b ", b_smlal32_i,  b_smlal32_d,  1, 0 },
#if ARM_ARCH >= 5
    { "smulbb    ", b_smulbb_i,   b_smulbb_d,   1, 0 },
    { "smlabb acc", b_smlabb_i,   b_smlabb_d,   1, 0 },
    { "smlalbb   ", b_smlalbb_i,  b_smlalbb_d,  1, 0 },
    { "smulwb    ", b_smulwb_i,   b_smulwb_d,   1, 0 },
    { "smlawb acc", b_smlawb_i,   b_smlawb_d,   1, 0 },
    { "clz       ", b_clz_i,      b_clz_d,      1, 0 },
    { "qadd      ", b_qadd_i,     b_qadd_d,     1, 0 },
    { "ldrd      ", b_ldrd_i,     b_ldrd_d,     1, 0 },
    { "strd      ", b_strd_i,     b_strd_d,     1, 0 },
    { "pld       ", b_pld_i,      b_pld_d,      1, 0 },
#endif
#if ARM_ARCH >= 6
    { "smmul     ", b_smmul_i,    b_smmul_d,    1, 0 },
    { "smmla acc ", b_smmla_i,    b_smmla_d,    1, 0 },
    { "smmulr    ", b_smmulr_i,   b_smmulr_d,   1, 0 },
    { "smuad     ", b_smuad_i,    b_smuad_d,    1, 0 },
    { "smlad acc ", b_smlad_i,    b_smlad_d,    1, 0 },
    { "smlald    ", b_smlald_i,   b_smlald_d,   1, 0 },
    { "umaal     ", b_umaal_i,    b_umaal_d,    1, 0 },
    { "mul+use   ", b_muluse_i,   b_muluse_d,   1, 1 },
    { "smull+use ", b_smulluse_i, b_smulluse_d, 1, 1 },
    { "ssat      ", b_ssat_i,     b_ssat_d,     1, 0 },
    { "rev       ", b_rev_i,      b_rev_d,      1, 0 },
    { "sxth      ", b_sxth_i,     b_sxth_d,     1, 0 },
    { "pkhbt     ", b_pkhbt_i,    b_pkhbt_d,    1, 0 },
    { "sadd16    ", b_sadd16_i,   b_sadd16_d,   1, 0 },
    { "qadd16    ", b_qadd16_i,   b_qadd16_d,   1, 0 },
#endif
};
#define NB ((int)(sizeof(B) / sizeof(B[0])))

static void fmt(char *out, int v)
{
    rb->snprintf(out, 8, "%3d.%02d", v / 100, v < 0 ? -v % 100 : v % 100);
}

/* Stack rows: where the stack is depends on the thread, so these are run
   on the caller's thread, with code in IRAM and in DRAM. */
static const struct {
    const char *name;
    benchfn i, d;
} SP[] = {
    { "str [sp]  ", b_spstr_i,  b_spstr_d },
    { "stm 4 [sp]", b_spstm_i,  b_spstm_d },
    { "ldr [sp]  ", b_spldr_i,  b_spldr_d },
    { "push+pop 4", b_sppush_i, b_sppush_d },
};
#define NSP ((int)(sizeof(SP) / sizeof(SP[0])))

#ifdef USE_IRAM
static long long sp_empty_i;
#endif
static long long sp_empty_d;
static int sp_res[NSP][2];
static unsigned long sp_addr;
static volatile bool sp_done;

static void stack_rows(void)
{
    int k;
    asm volatile ("mov %0, sp" : "=r" (sp_addr));
    for (k = 0; k < NSP; k++)
    {
#ifdef USE_IRAM
        sp_res[k][0] = per_insn(cpi(SP[k].i, dram_buf), sp_empty_i);
#endif
        sp_res[k][1] = per_insn(cpi(SP[k].d, dram_buf), sp_empty_d);
    }
    sp_done = true;
}

static void say_stack_rows(const char *thread)
{
    int k;
    say("stack rows, %s thread, sp %08lx", thread, sp_addr);
#ifdef USE_IRAM
    say("cyc/insn  code:IRAM   DRAM");
#endif
    for (k = 0; k < NSP; k++)
    {
        char a[8], b[8];
        fmt(b, sp_res[k][1]);
#ifdef USE_IRAM
        fmt(a, sp_res[k][0]);
        say("%s %s %s", SP[k].name, a, b);
#else
        (void)a;
        say("%s %s", SP[k].name, b);
#endif
    }
}

/* A real kernel, a stage at a time: libtta's hybrid_filter (filter_arm.S),
   its error > 0 path, cut off after each stage in turn, to find where a
   whole function costs more than its instructions do one by one.  Each
   stage is the ones before it and:
     0  the frame: push and pop of ten registers, through the pc
     1  the three-word load of the state, five adds, the branches
     2  ldm 4, ldm 4, four adds, stm 4: the first weights
     3  ldm 4 and four mla
     4  stage 2 again for the next four weights
     5  stage 3 again
     6  ldr, ldr, add with a register shift, str, ldr, add, ands, stm 2
     7  mov, four orr with a shift, three shifts, three subs
     8  the two conditional stm 4 and the conditional return: the whole
        function, shifting its delay lines every sixteenth call */
#define TCF_ADD4 "   add     r5, r5, r9\n   add     r6, r6, r10\n"            \
                 "   add     r7, r7, r11\n   add     r8, r8, r12\n"
#define TCF_MLA4 "   mla     lr, r5, r9, lr\n   mla     lr, r6, r10, lr\n"    \
                 "   mla     lr, r7, r11, lr\n   mla     lr, r8, r12, lr\n"
__asm__(".text\n.align 2\n"
        ".macro TCF name, stage, sub=0, step=4\n"
        ".type \\name, %function\n"
        "\\name:\n"
        "   stmdb   sp!, { r4-r12, lr }\n"
        ".if \\stage >= 1\n"
        "   ldmia   r0, { r5, r6, lr }\n"
        "   add     r2, r0, #148\n"
        "   add     r3, r0, #52\n"
        "   add     r4, r0, #20\n"
        "   add     r2, r2, r5\n"
        "   add     r3, r3, r5\n"
        "   cmp     r6, #0\n"
        "   bmi     9f\n"
        "   bne     1f\n"
        "   b       9f\n"
        "1:\n"
        ".endif\n"
        ".if \\stage >= 2\n"
        "   ldmia   r4, { r5, r6, r7, r8 }\n"
        "   ldmia   r3!, { r9, r10, r11, r12 }\n"
        TCF_ADD4
        "   stmia   r4!, { r5, r6, r7, r8 }\n"
        ".endif\n"
        ".if \\stage >= 3\n"
        "   ldmia   r2!, { r9, r10, r11, r12 }\n"
        TCF_MLA4
        ".endif\n"
        ".if \\stage >= 4\n"
        "   ldmia   r4, { r5, r6, r7, r8 }\n"
        "   ldmia   r3!, { r9, r10, r11, r12 }\n"
        TCF_ADD4
        "   stmia   r4!, { r5, r6, r7, r8 }\n"
        ".endif\n"
        ".if \\stage >= 5\n"
        "   ldmia   r2!, { r9, r10, r11, r12 }\n"
        TCF_MLA4
        ".endif\n"
        ".irp k, 1, 2, 3, 4, 5, 6, 7, 8\n"
        ".if (\\stage >= 6) | (\\sub >= \\k)\n"
        ".if \\k == 1\n   ldr     r5, [r1]\n.endif\n"
        ".if \\k == 2\n   ldr     r6, [r0, #12]\n.endif\n"
        ".if \\k == 3\n   add     lr, r5, lr, asr r6\n.endif\n"
        ".if \\k == 4\n   str     lr, [r1]\n.endif\n"
        ".if \\k == 5\n   ldr     r1, [r0]\n.endif\n"
        ".if \\k == 6\n   add     r1, r1, #\\step\n.endif\n"
        ".if \\k == 7\n   ands    r1, r1, #63\n.endif\n"
        ".if \\k == 8\n   stmia   r0, { r1, r5 }\n.endif\n"
        ".endif\n"
        ".endr\n"
        ".if \\stage >= 7\n"
        "   mov     r4, #1\n"
        "   orr     r5, r4, r9, asr #30\n"
        "   orr     r6, r4, r10, asr #30\n"
        "   orr     r7, r4, r11, asr #30\n"
        "   orr     r8, r4, r12, asr #30\n"
        "   mov     r6, r6, lsl #1\n"
        "   mov     r7, r7, lsl #1\n"
        "   mov     r8, r8, lsl #2\n"
        "   sub     r12, lr, r12\n"
        "   sub     r11, r12, r11\n"
        "   sub     r10, r11, r10\n"
        ".endif\n"
        ".if \\stage >= 8\n"
        "   stmneda r2, { r10, r11, r12, lr }\n"
        "   stmneda r3, { r5, r6, r7, r8 }\n"
        "   ldmnefd sp!, { r4-r12, pc }\n"
        "   add     r2, r0, #212\n"
        "   ldmia   r2, { r1, r3, r4 }\n"
        "   sub     r2, r2, #64\n"
        "   stmia   r2, { r1, r3, r4, r9-r12, lr }\n"
        "   add     r9, r0, #116\n"
        "   ldmia   r9, { r1, r2, r3, r4 }\n"
        "   sub     r9, r9, #64\n"
        "   stmia   r9, { r1-r8 }\n"
        ".endif\n"
        "9:\n"
        "   ldmfd   sp!, { r4-r12, pc }\n"
        ".endm\n"
        "TCF tcf_0, 0\nTCF tcf_1, 1\nTCF tcf_2, 2\nTCF tcf_3, 3\n"
        "TCF tcf_4, 4\nTCF tcf_5, 5\nTCF tcf_6, 6\nTCF tcf_7, 7\n"
        "TCF tcf_8, 8\n"
        "TCF tcf_61, 5, 1\nTCF tcf_62, 5, 2\nTCF tcf_63, 5, 3\n"
        "TCF tcf_64, 5, 4\nTCF tcf_65, 5, 5\nTCF tcf_66, 5, 6\n"
        "TCF tcf_67, 5, 7\nTCF tcf_6s, 6, 0, 0\nTCF tcf_7s, 7, 0, 0\n"
        /* Stage 5 again at each word of a cache line, and frames holding
           only adds, as long as the filter's stages are. */
        ".irp k, 0, 1, 2, 3, 4, 5, 6, 7\n"
        ".balign 32\n"
        ".if \\k\n"
        ".space 4 * \\k\n"
        ".endif\n"
        "TCF tcf_5c\\k, 5\n"
        ".endr\n"
        ".macro TCP name, n\n"
        ".type \\name, %function\n"
        "\\name:\n"
        "   stmdb   sp!, { r4-r12, lr }\n"
        ".rept \\n\n"
        "   add     r4, r4, #1\n"
        ".endr\n"
        "   ldmfd   sp!, { r4-r12, pc }\n"
        ".endm\n"
        "TCP tcp_40, 40\nTCP tcp_60, 60\nTCP tcp_70, 70\n"
        "TCP tcp_80, 80\nTCP tcp_100, 100\nTCP tcp_120, 120\n"
        ".type tcf_ret, %function\n"
        "tcf_ret:\n"
        "   bx      lr\n");
int tcf_5c0(int *, int *); int tcf_5c1(int *, int *);
int tcf_5c2(int *, int *); int tcf_5c3(int *, int *);
int tcf_5c4(int *, int *); int tcf_5c5(int *, int *);
int tcf_5c6(int *, int *); int tcf_5c7(int *, int *);
int tcp_40(int *, int *); int tcp_60(int *, int *); int tcp_70(int *, int *);
int tcp_80(int *, int *); int tcp_100(int *, int *);
int tcp_120(int *, int *);
typedef int (*tcf_fn)(int *fs, int *in);
int tcf_0(int *, int *); int tcf_1(int *, int *); int tcf_2(int *, int *);
int tcf_3(int *, int *); int tcf_4(int *, int *); int tcf_5(int *, int *);
int tcf_6(int *, int *); int tcf_7(int *, int *); int tcf_8(int *, int *);
int tcf_61(int *, int *); int tcf_62(int *, int *); int tcf_63(int *, int *);
int tcf_64(int *, int *); int tcf_65(int *, int *); int tcf_66(int *, int *);
int tcf_67(int *, int *); int tcf_6s(int *, int *); int tcf_7s(int *, int *);
int tcf_ret(int *, int *);

/* The filter's state, as fltst lays it out: index, error, round, shift,
   a spare word, then qm[8], dx[24] and dl[24]. */
static int tcf_buf[61 + 8] __attribute__((aligned(32)));
static int *tcf_fs = tcf_buf;           /* the state, somewhere in tcf_buf */
static int tcf_in[64];
static tcf_fn tcf_now;

static void tcf_loop(unsigned long *unused, unsigned long n)
{
    tcf_fn fn = tcf_now;
    int *fs = tcf_fs;
    unsigned long i;
    int v;
    (void)unused;
    for (i = 0; i < n; i++)
    {
        v = tcf_in[i & 63];
        fn(fs, &v);
    }
}

/* Hundredths of a cycle a call, the function's own return included. */
static long long tcf_cycles(tcf_fn fn)
{
    int i;
    rb->memset(tcf_buf, 0, sizeof(tcf_buf));
    tcf_fs[1] = 1;                      /* error > 0 */
    tcf_fs[2] = 512;                    /* round */
    tcf_fs[3] = 10;                     /* shift */
    for (i = 0; i < 64; i++)
        tcf_in[i] = 500 + i * 37 % 1000;
    tcf_now = fn;
    return cpi(tcf_loop, NULL);
}

static void say_filter_rows(void)
{
    /* By the rows above: the frame 24, then 13, 16, 12, 16, 12, 12 and 11,
       and 9.75 for the last, the delay lines' shift included.  On an
       ARM926 the later stages come out a tenth over, which is the cycle a
       line that "adds in a loop" measures: with its caller the function
       is more than eight lines of code.  The archive has stage 6 an
       instruction at a time (6.1 to 6.7, 6 being all eight), and stages
       6 and 7 with the index left where it is (6s, 7s). */
    static const struct { const char *name; tcf_fn fn; int want; } T[] = {
        { "0  ", tcf_0, 2400 },   { "1  ", tcf_1, 3700 },
        { "2  ", tcf_2, 5300 },   { "3  ", tcf_3, 6500 },
        { "4  ", tcf_4, 8100 },   { "5  ", tcf_5, 9300 },
#if TEST_CYC_ARCHIVE
        { "6.1", tcf_61, 9400 },  { "6.2", tcf_62, 9500 },
        { "6.3", tcf_63, 9800 },  { "6.4", tcf_64, 9900 },
        { "6.5", tcf_65, 10000 }, { "6.6", tcf_66, 10200 },
        { "6.7", tcf_67, 10300 },
#endif
        { "6  ", tcf_6, 10500 },
        { "7  ", tcf_7, 11600 },  { "8  ", tcf_8, 12575 },
#if TEST_CYC_ARCHIVE
        { "6s ", tcf_6s, 10500 }, { "7s ", tcf_7s, 11600 },
#endif
    };
    long long base = tcf_cycles(tcf_ret);
    unsigned k;
    int last = 0;
    say("tta filter, state at %08lx", (unsigned long)tcf_fs);
    say("  stage    cyc   step  by rows");
    for (k = 0; k < sizeof(T) / sizeof(T[0]); k++)
    {
        char a[8], b[8], c[8];
        int v = (int)(tcf_cycles(T[k].fn) - base) + 300;
        fmt(a, v);
        fmt(b, v - last);
        fmt(c, T[k].want);
        say("  %s  %s %s %s", T[k].name, a, b, c);
        last = v;
    }
}

#if TEST_CYC_ARCHIVE
/* Where the filter's cost depends on something other than its
   instructions: stage 5 with its state at each word of a cache line, the
   same code at each word of a line, and frames of nothing but adds. */
static void say_filter_layout(void)
{
    static const tcf_fn code[] = { tcf_5c0, tcf_5c1, tcf_5c2, tcf_5c3,
                                   tcf_5c4, tcf_5c5, tcf_5c6, tcf_5c7 };
    static const struct { tcf_fn fn; int n; } pad[] = {
        { tcp_40, 40 }, { tcp_60, 60 }, { tcp_70, 70 }, { tcp_80, 80 },
        { tcp_100, 100 }, { tcp_120, 120 } };
    long long base = tcf_cycles(tcf_ret);
    unsigned k;
    char a[8], b[8];
    say("stage 5 (93) and 4 (81), state at word");
    for (k = 0; k < 8; k++)
    {
        tcf_fs = tcf_buf + k;
        fmt(a, (int)(tcf_cycles(tcf_5) - base) + 300);
        fmt(b, (int)(tcf_cycles(tcf_4) - base) + 300);
        say("  %u  %s %s", k, a, b);
    }
    tcf_fs = tcf_buf;
    say("stage 5 (93), code at word");
    for (k = 0; k < 8; k++)
    {
        fmt(a, (int)(tcf_cycles(code[k]) - base) + 300);
        say("  %u  %s  at %08lx", k, a, (unsigned long)code[k]);
    }
    say("frame of adds   cyc  by rows");
    for (k = 0; k < sizeof(pad) / sizeof(pad[0]); k++)
    {
        fmt(a, (int)(tcf_cycles(pad[k].fn) - base) + 300);
        fmt(b, (24 + pad[k].n) * 100);
        say("  %3d adds  %s %s", pad[k].n, a, b);
    }
}
#endif

/* The empty loop, by where its code and its data are. */
static long long e_d_d;
#ifdef USE_IRAM
static long long e_i_i, e_i_d, e_d_i;
#endif

#ifndef TICK_TIMED
#if TEST_CYC_ARCHIVE
/* Which lines a miss evicts.  The 16 KB at the start of the buffer is
   read until the data cache holds it, then a stream of misses is run
   through memory elsewhere, a load a line, and the first 16 KB is read
   once more and timed: what share of its lines are still there.  The
   stream's loop is padded with 0 to 5 adds, since a victim chosen by a
   free-running counter would depend on the loop's length where a random
   one would not: at random, a quarter of what is left goes with every 128
   lines, leaving 75%, 32%, and after 2048 lines nothing. */
#define EVICT(name, pad)                                                    \
static void name(unsigned long *p, unsigned long n)                         \
{                                                                           \
    asm volatile ("1:\n"                                                    \
                  "   ldr     r4, [%[p]], #32\n"                            \
                  pad                                                       \
                  "   subs    %[n], %[n], #1\n"                             \
                  "   bne     1b\n"                                         \
                  : [n] "+&r" (n), [p] "+&r" (p) : : "r4", "r5", "cc");     \
}
#define PAD1 "   add     r5, r5, #1\n"
EVICT(evict_0, "")
EVICT(evict_1, PAD1)
EVICT(evict_2, PAD1 PAD1)
EVICT(evict_3, PAD1 PAD1 PAD1)
EVICT(evict_5, PAD1 PAD1 PAD1 PAD1 PAD1)

/* Timer counts for one read of the first 16 KB, after `lines` misses made
   by fn elsewhere; fn NULL leaves it as it is. */
static unsigned long evict_pass(unsigned char *big, benchfn fn,
                                unsigned long lines)
{
    unsigned long t;
    int i;
    for (i = 0; i < 12; i++)
        evict_0((unsigned long *)big, 512);
    if (fn)
        fn((unsigned long *)(big + 65536), lines);
    t = NOW();
    evict_0((unsigned long *)big, 512);
    return NOW() - t;
}

/* Which way a miss takes, miss after miss.  Four lines are loaded into
   one set until they fill it; then some new lines are loaded into that
   set, with misses in other sets in between; then the four are read again
   and timed, each followed by enough adds for a miss to cost its stall
   alone.  A victim picked at random each time keeps 75%, 56%, 42% and 32%
   of the four after 1 to 4 new lines whatever comes between; one that
   takes the ways in turn keeps 75%, 50%, 25% and none. */
void evict_reload(unsigned char *line0);
__asm__(".text\n.align 2\n.type evict_reload, %function\n"
        "evict_reload:\n"
        "   mov     r2, #4\n"
        "1:\n"
        "   ldr     r1, [r0]\n"
        "   add     r0, r0, #4096\n"
        "   .rept 32\n"
        "   add     r3, r3, #1\n"
        "   .endr\n"
        "   subs    r2, r2, #1\n"
        "   bne     1b\n"
        "   bx      lr\n");

static unsigned long evict_set_trial(unsigned char *big, int news, int gap)
{
    unsigned long total = 0, t, fill = 0;
    int set, k, round, i;
    for (set = 0; set < 128; set++)
    {
        unsigned char *line = big + set * 32;
        for (round = 0; round < 8; round++)
            for (k = 0; k < 4; k++)
                (void)*(volatile unsigned long *)(line + k * 4096);
        for (k = 0; k < news; k++)
        {
            (void)*(volatile unsigned long *)(line + (4 + k) * 4096);
            for (i = 0; i < gap; i++, fill++)
                (void)*(volatile unsigned long *)(big + 131072
                    + ((set + 1 + fill % 126) & 127) * 32
                    + (fill / 126 % 16) * 4096);
        }
        t = NOW();
        evict_reload(line);
        total += NOW() - t;
    }
    return total;
}

static void say_evict_set_rows(void)
{
    static const unsigned char gaps[] = { 0, 1, 2, 3, 7, 127 };
    size_t bufsize;
    unsigned char *big = rb->plugin_get_buffer(&bufsize);
    unsigned long hit = 0, miss = 0;
    unsigned g, n, r;
    big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
    if (bufsize < 262144)
        return;
    for (r = 0; r < 8; r++)
    {
        hit += evict_set_trial(big, 0, 0);
        miss += evict_set_trial(big, 20, 0);
    }
    say("4 lines of a set: kept %lu, gone %lu counts", hit / 8, miss / 8);
    say("kept, %%, after new lines in the set");
    say("  between    1    2    3    4    8");
    for (g = 0; g < sizeof(gaps); g++)
    {
        static const unsigned char news[] = { 1, 2, 3, 4, 8 };
        int kept[5];
        for (n = 0; n < 5; n++)
        {
            unsigned long t = 0;
            for (r = 0; r < 8; r++)
                t += evict_set_trial(big, news[n], gaps[g]);
            kept[n] = miss > hit
                ? (int)(100 - 100 * ((long long)t - hit) / (miss - hit)) : 0;
        }
        say("  %3d     %4d %4d %4d %4d %4d", gaps[g], kept[0], kept[1],
            kept[2], kept[3], kept[4]);
    }
}
#endif

/* The same question asked more sharply: a line is loaded, then one to
   five more into its set, each a miss with the bus idle before it (32
   adds, plus 0 to 7 more to move the misses in time), and the first is
   read again.  Chosen at random it survives 75%, 56%, 42%, 32% and 24% of
   the time, whatever the spacing.  With the ways taken in turn it always
   survives three more and never four.  With the victim read off a
   free-running counter it depends on the spacing, a row at a time. */
#define EVICTN(name, pad)                                                   \
void name(unsigned char *line, unsigned long n);
#define EVICTN_ASM(name, pad)                                               \
        ".type " #name ", %function\n"                                      \
        #name ":\n"                                                         \
        "1:\n"                                                              \
        "   ldr     r2, [r0]\n"                                             \
        "   add     r0, r0, #4096\n"                                        \
        "   .rept 32 + " #pad "\n"                                          \
        "   add     r3, r3, #1\n"                                           \
        "   .endr\n"                                                        \
        "   subs    r1, r1, #1\n"                                           \
        "   bne     1b\n"                                                   \
        "   bx      lr\n"
EVICTN(evictn_0, 0) EVICTN(evictn_1, 1) EVICTN(evictn_2, 2)
EVICTN(evictn_3, 3) EVICTN(evictn_4, 4) EVICTN(evictn_5, 5)
EVICTN(evictn_6, 6) EVICTN(evictn_7, 7)
__asm__(".text\n.align 2\n"
        EVICTN_ASM(evictn_0, 0) EVICTN_ASM(evictn_1, 1)
        EVICTN_ASM(evictn_2, 2) EVICTN_ASM(evictn_3, 3)
        EVICTN_ASM(evictn_4, 4) EVICTN_ASM(evictn_5, 5)
        EVICTN_ASM(evictn_6, 6) EVICTN_ASM(evictn_7, 7));

/* Timer counts to read back the first of 1 + more lines loaded into each
   set in turn.  The lines come from 56 that share the set, taken round,
   so that nearly every load is a miss. */
static unsigned long evict_lag_trial(unsigned char *big,
                                     void (*fn)(unsigned char *,
                                                unsigned long),
                                     int more)
{
    static unsigned tag;
    unsigned long total = 0, t;
    int set;
    for (set = 0; set < 128; set++)
    {
        unsigned char *line;
        if (tag + 1 + more > 56)
            tag = 0;
        line = big + set * 32 + tag * 4096;
        fn(line, 1 + more);
        t = NOW();
        evictn_0(line, 1);
        total += NOW() - t;
        tag += 1 + more;
    }
    return total;
}

static void say_evict_lag_rows(void)
{
    static void (*const fn[])(unsigned char *, unsigned long) = {
        evictn_0, evictn_1, evictn_2, evictn_3,
        evictn_4, evictn_5, evictn_6, evictn_7 };
    size_t bufsize;
    unsigned char *big = rb->plugin_get_buffer(&bufsize);
    unsigned long hit = 0, miss = 0;
    unsigned d, k, r;
    big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
    if (bufsize < 56 * 4096 + 8192)
        return;
    for (r = 0; r < 16; r++)
    {
        hit += evict_lag_trial(big, evictn_0, 0);
        miss += evict_lag_trial(big, evictn_0, 16);
    }
    say("first line back: kept %lu, gone %lu counts", hit / 16, miss / 16);
    say("first line kept, %%, after more misses in its set");
    say("  pad     1    2    3    4    5");
    for (d = 0; d < 8; d++)
    {
        int kept[5];
        for (k = 0; k < 5; k++)
        {
            unsigned long t = 0;
            for (r = 0; r < 16; r++)
                t += evict_lag_trial(big, fn[d], k + 1);
            kept[k] = miss > hit
                ? (int)(100 - 100 * ((long long)t - hit) / (miss - hit)) : 0;
        }
        say("  %u    %4d %4d %4d %4d %4d", d, kept[0], kept[1], kept[2],
            kept[3], kept[4]);
    }
}

#if TEST_CYC_ARCHIVE
static void say_evict_rows(void)
{
    static const struct { benchfn fn; int pad; } E[] = {
        { evict_0, 0 }, { evict_1, 1 }, { evict_2, 2 }, { evict_3, 3 },
        { evict_5, 5 } };
    static const unsigned short lines[] = { 128, 512, 2048 };
    size_t bufsize;
    unsigned char *big = rb->plugin_get_buffer(&bufsize);
    unsigned long hit = 0, miss = 0, t;
    unsigned k, j, r;
    big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
    if (bufsize < 65536 + 2048 * 32 + 64)
        return;
    for (r = 0; r < 16; r++)
    {
        hit += evict_pass(big, NULL, 0);
        miss += evict_pass(big, evict_0, 4096);
    }
    say("16 KB read: cached %lu, evicted %lu counts", hit / 16, miss / 16);
    say("lines kept, %%, after misses elsewhere");
    say("  pad    128   512  2048 lines");
    for (k = 0; k < sizeof(E) / sizeof(E[0]); k++)
    {
        int kept[3];
        for (j = 0; j < 3; j++)
        {
            t = 0;
            for (r = 0; r < 16; r++)
                t += evict_pass(big, E[k].fn, lines[j]);
            kept[j] = miss > hit
                ? (int)(100 - 100 * ((long long)t - hit) / (miss - hit)) : 0;
        }
        say("  %d    %4d  %4d  %4d", E[k].pad, kept[0], kept[1], kept[2]);
    }
}
#endif
#endif

/* Misses with room after them, and back to back, in the memory at big:
   the plugin buffer, which is DRAM, or on an AS3525v2 the megabyte of RAM
   inside the SoC, where codecs are loaded with all their data. */
static void say_miss_gap_rows(const char *where, unsigned char *big,
                              size_t bufsize)
{
    static const struct { const char *name; benchfn fn; } G[] = {
        { "+32 alu     ", gap_32 },
#if TEST_CYC_ARCHIVE
        { "+64 alu     ", gap_64 },
#endif
        { "+128 alu    ", gap_128 },
#if TEST_CYC_ARCHIVE
        { "+256 alu    ", gap_256 },
#endif
        { "ldr w1, +32 ", gap_w1 },
        { "ldr w7, +32 ", gap_w7 },
#if TEST_CYC_ARCHIVE
        { "ldm 2, +32  ", gap_ldm2 },
#endif
        { "ldm 4, +32  ", gap_ldm4 },
        { "ldm w2-5,+32", gap_ldm4mid },
        { "str w0, +32 ", gap_str },
    };
    static const struct { const char *name; benchfn fn; } P[] = {
        { "line, no gap", stream_line },
        { "ldm 4 stream", stream_ldm4 },
        { "dirty 4     ", stream_dirty4 },
        { "dirty 8     ", stream_dirty8 },
        { "str stream  ", wstream_word },
        { "str a line  ", wstream_line },
        { "stm 4 stream", wstream_stm4 },
        { "copy 4 words", copy_4 },
    };
    unsigned long large;
    unsigned k;
    char a[8], b[8], c[8];
    int hit, miss;
    large = (bufsize > 262144 ? 262144 : bufsize) & ~511ul;
    say("%s at %08lx", where, (unsigned long)big);
    say("miss, then      cached  %luKB  extra", large / 1024);
    for (k = 0; k < sizeof(G) / sizeof(G[0]); k++)
    {
        sbuf = (unsigned long *)big;
        sbytes = 1024;
        hit = per_insn(cpi_from(G[k].fn, NULL, 2048), e_d_d);
        sbytes = large;
        miss = per_insn(cpi_from(G[k].fn, NULL, 2048), e_d_d);
        fmt(a, hit);
        fmt(b, miss);
        fmt(c, miss - hit);
        say("  %s %s %s %s", G[k].name, a, b, c);
    }
    if (bufsize >= 262144)
    {
        make_chain(big, 1024, 32);
        hit = per_insn(cpi_from(chase_work32, (unsigned long *)big, 2048),
                       e_d_d);
        make_chain(big, 262144, 32);
        miss = per_insn(cpi_from(chase_work32, (unsigned long *)big, 2048),
                        e_d_d);
        fmt(a, hit);
        fmt(b, miss);
        fmt(c, miss - hit);
        say("  chase, +32   %s %s %s", a, b, c);
        make_chain_scattered(big, 262144, 32);
        miss = per_insn(cpi_from(chase_work32, (unsigned long *)big, 2048),
                        e_d_d);
        fmt(b, miss);
        fmt(c, miss - hit);
        say("  scattered    %s %s %s", a, b, c);
    }
    /* Back to back.  The first kilobyte is read before each row, so the
       store rows' first column is of stores to lines that are cached. */
    for (k = 0; k < sizeof(P) / sizeof(P[0]); k++)
    {
        sbuf = (unsigned long *)big;
        sbytes = 1024;
        stream_word(NULL, 64);
        hit = per_insn(cpi_from(P[k].fn, NULL, 2048), e_d_d);
        /* The copy reads the first half of the buffer and writes the
           second. */
        sbytes = P[k].fn == copy_4 ? 131072 : large;
        miss = per_insn(cpi_from(P[k].fn, NULL, 2048), e_d_d);
        fmt(a, hit);
        fmt(b, miss);
        fmt(c, miss - hit);
        say("  %s %s %s %s", P[k].name, a, b, c);
    }
}

static void say_miss_rows_everywhere(void)
{
    size_t bufsize;
    unsigned char *big = rb->plugin_get_buffer(&bufsize);
    big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
    say_miss_gap_rows("DRAM", big, bufsize - 32);
#if CONFIG_CPU == AS3525v2
    /* The SoC's own RAM is mapped after the DRAM; its first 32 KB is the
       firmware's and the rest is where a codec goes, free while nothing
       plays. */
    rb->audio_stop();
    say_miss_gap_rows("SoC RAM", (unsigned char *)0x30000000
                      + MEMORYSIZE * 0x100000 + 0x40000, 0x80000);
#endif
}

#if ARM_ARCH >= 5
/* Fetch misses in any memory: the code is written where it is to run.  A
   loop of one taken branch a line, or of lines of eight adds, over 4 KB,
   which the cache holds, and over 64 KB, where every line is a miss. */
static void make_code(unsigned long *code, int lines, bool adds)
{
    unsigned long *w = code;
    int k, j;
    for (k = 0; k < lines; k++, w += 8)
    {
        for (j = 0; j < 8; j++)
            w[j] = 0xe2800001;          /* add r0, r0, #1 */
        if (!adds)
            w[0] = 0xea000006;          /* b the next line */
    }
    w[0] = 0xe2511001;                  /* subs r1, r1, #1 */
    w[1] = 0x1a000000                   /* bne the first line */
           | ((unsigned long)(code - (w + 1) - 2) & 0xfffffful);
    w[2] = 0xe12fff1e;                  /* bx lr */
    rb->commit_dcache();
    rb->commit_discard_idcache();
}

static void say_code_miss_rows(const char *where, unsigned long *code)
{
    static const char *const name[] = { "a branch a line", "8 adds a line " };
    int v;
    char a[8], b[8], c[8];
    say("%s at %08lx, code", where, (unsigned long)code);
    say("cycles a line    4KB   64KB  extra");
    for (v = 0; v < 2; v++)
    {
        int hit, miss;
        make_code(code, 128, v);
        hit = (int)(cpi_from((benchfn)code, NULL, 512) / 128);
        make_code(code, 2048, v);
        miss = (int)(cpi_from((benchfn)code, NULL, 64) / 2048);
        fmt(a, hit);
        fmt(b, miss);
        fmt(c, miss - hit);
        say("  %s %s %s %s", name[v], a, b, c);
    }
}

#if CONFIG_CPU == AS3525v2
/* The copy of copy_4, from one memory to the other: whether a fill in one
   waits for the writes still to go out to the other. */
static unsigned long sdelta;
static void copy_across(unsigned long *unused, unsigned long n)
{
    unsigned long *p = sbuf, *end = sbuf + sbytes / 4;
    (void)unused;
    asm volatile ("1:\n"
                  X16("   ldmia   %[p], { r4-r7 }\n"
                      "   add     r8, %[p], %[d]\n"
                      "   stmia   r8, { r4-r7 }\n"
                      "   add     %[p], %[p], #16\n")
                  "   cmp     %[p], %[end]\n"
                  "   movhs   %[p], %[base]\n"
                  "   subs    %[n], %[n], #1\n"
                  "   bne     1b\n"
                  : [n] "+&r" (n), [p] "+&r" (p)
                  : [end] "r" (end), [base] "r" (sbuf), [d] "r" (sdelta)
                  : "r4", "r5", "r6", "r7", "r8", "cc", "memory");
}

static void say_copy_across(const char *name, unsigned long *from,
                            unsigned long *to)
{
    char a[8], b[8], c[8];
    int hit, miss;
    sbuf = from;
    sdelta = (unsigned long)to - (unsigned long)from;
    sbytes = 1024;
    stream_word(NULL, 64);
    hit = per_insn(cpi_from(copy_across, NULL, 2048), e_d_d);
    sbytes = 131072;
    miss = per_insn(cpi_from(copy_across, NULL, 2048), e_d_d);
    fmt(a, hit);
    fmt(b, miss);
    fmt(c, miss - hit);
    say("  %s %s %s %s", name, a, b, c);
}
#endif

/* Tremor's window loop, which reads two buffers, one upwards and one
   downwards, and stores a word to each of two more, none of them cached:
   args is the buffer, a quarter of its size and a small table.  And the
   two streams of stores alone.  Each pass is 16 times the body. */
#define VBODY_STORES                                                        \
    "   str     r5, [r9, #-4]!\n"                                           \
    "   str     r5, [r6], #4\n"
#define VBODY_WINDOW                                                        \
    "   ldr     r0, [r7], #4\n"                                             \
    "   ldr     r1, [ip, #-4]!\n"                                           \
    "   ldr     r2, [r8]\n"                                                 \
    "   ldr     r3, [r8, #4]\n"                                             \
    "   smull   r4, r5, r0, r2\n"                                           \
    "   smlal   r4, r5, r1, r3\n"                                           \
    "   rsb     r2, r2, #0\n"                                               \
    "   mov     r5, r5, lsl #1\n"                                           \
    "   str     r5, [r9, #-4]!\n"                                           \
    "   smull   r4, r5, r0, r3\n"                                           \
    "   smlal   r4, r5, r1, r2\n"                                           \
    "   mov     r5, r5, lsl #1\n"                                           \
    "   str     r5, [r6], #4\n"
#define VLOOP(name, body, step)                                             \
void name(unsigned long *args, unsigned long n);                            \
asm ("   .pushsection .text." #name ", \"ax\", %progbits\n"                 \
     "   .align  2\n"                                                       \
     "   .type   " #name ", %function\n"                                    \
     #name ":\n"                                                            \
     "   stmfd   sp!, { r4-r11, lr }\n"                                     \
     "   mov     r11, r0\n"                                                 \
     "   mov     r10, r1\n"                                                 \
     "   ldr     r8, [r11, #8]\n"                                           \
     "0:\n"                                                                 \
     "   ldr     r7, [r11]\n"                                               \
     "   ldr     r3, [r11, #4]\n"                                           \
     "   add     ip, r7, r3, lsl #1\n"                                      \
     "   add     r6, ip, r3\n"                                              \
     "   mov     r9, r6\n"                                                  \
     "   add     lr, r7, r3\n"                                              \
     "1:\n"                                                                 \
     X16(body)                                                              \
     step                                                                   \
     "   subs    r10, r10, #1\n"                                            \
     "   beq     2f\n"                                                      \
     "   cmp     r7, lr\n"                                                  \
     "   blo     1b\n"                                                      \
     "   b       0b\n"                                                      \
     "2:\n"                                                                 \
     "   ldmfd   sp!, { r4-r11, pc }\n"                                     \
     "   .popsection\n");
VLOOP(vloop_stores, VBODY_STORES, "   add     r7, r7, #64\n")
VLOOP(vloop_window, VBODY_WINDOW, "")

static void say_window_rows(const char *where, unsigned long *big,
                            size_t bufsize)
{
    static const struct { const char *name; benchfn fn; } V[] = {
        { "2 str streams", vloop_stores },
        { "window loop  ", vloop_window },
    };
    static unsigned long table[2] = { 0x3fffffff, 0x12345678 };
    unsigned long args[3];
    unsigned long large = (bufsize > 262144 ? 262144 : bufsize) & ~1023ul;
    unsigned k;
    char a[8], b[8], c[8];
    say("%s at %08lx", where, (unsigned long)big);
    say("two streams out cached  %luKB  extra", large / 1024);
    args[0] = (unsigned long)big;
    args[2] = (unsigned long)table;
    for (k = 0; k < sizeof(V) / sizeof(V[0]); k++)
    {
        int hit, miss;
        sbuf = big;
        sbytes = 1024;
        stream_word(NULL, 64);
        args[1] = 256;
        hit = per_insn(cpi_from(V[k].fn, args, 2048), e_d_d);
        args[1] = large / 4;
        miss = per_insn(cpi_from(V[k].fn, args, 2048), e_d_d);
        fmt(a, hit);
        fmt(b, miss);
        fmt(c, miss - hit);
        say("  %s %s %s %s", V[k].name, a, b, c);
    }
}

static void say_window_rows_everywhere(void)
{
    size_t bufsize;
    unsigned char *big = rb->plugin_get_buffer(&bufsize);
    big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
    say_window_rows("DRAM", (unsigned long *)big, bufsize - 32);
#if CONFIG_CPU == AS3525v2
    rb->audio_stop();
    say_window_rows("SoC RAM", (unsigned long *)(0x30000000
                    + MEMORYSIZE * 0x100000 + 0x40000), 0x80000);
#endif
}

static void say_code_miss_rows_everywhere(void)
{
    size_t bufsize;
    unsigned char *big = rb->plugin_get_buffer(&bufsize);
    big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
    if (bufsize >= 0x10100)
        say_code_miss_rows("DRAM", (unsigned long *)big);
#if CONFIG_CPU == AS3525v2
    rb->audio_stop();
    say_code_miss_rows("SoC RAM", (unsigned long *)(0x30000000
                       + MEMORYSIZE * 0x100000 + 0x40000));
    if (bufsize >= 0x20100)
    {
        unsigned long *soc = (unsigned long *)(0x30000000
                             + MEMORYSIZE * 0x100000 + 0x40000);
        say("copy 4 words    cached  128KB  extra");
        say_copy_across("DRAM to SoC ", (unsigned long *)big, soc);
        say_copy_across("SoC to DRAM ", soc, (unsigned long *)big);
    }
#endif
}
#endif

/* How much code a loop holds against what its instructions cost: loops of
   adds filling one cache line to 256 of them, and of one taken branch a
   line. */
static void say_code_size_rows(void)
{
    static const struct { benchfn fn; int n; } S[] = {
        { seq_6, 6 }, { seq_14, 14 }, { seq_22, 22 }, { seq_30, 30 },
        { seq_38, 38 }, { seq_46, 46 }, { seq_54, 54 }, { seq_62, 62 },
        { seq_70, 70 }, { seq_78, 78 }, { seq_94, 94 },
        { seq_126, 126 }, { seq_254, 254 }, { seq_510, 510 },
        { seq_1022, 1022 }, { seq_2046, 2046 } };
    static const struct { benchfn fn; int n; } C[] = {
        { ichain_1, 1 }, { ichain_2, 2 }, { ichain_3, 3 }, { ichain_4, 4 },
        { ichain_5, 5 }, { ichain_6, 6 }, { ichain_7, 7 }, { ichain_8, 8 },
        { ichain_9, 9 }, { ichain_10, 10 }, { ichain_12, 12 },
        { ichain_16, 16 }, { ichain_small, 32 },
        { ichain_64, 64 }, { ichain_128, 128 }, { ichain_256, 256 } };
    unsigned k;
    char a[8], b[8];
    say("adds in a loop  lines  cyc/add  over");
    for (k = 0; k < sizeof(S) / sizeof(S[0]); k++)
    {
        long long c = cpi_from(S[k].fn, NULL, 3200000ul / S[k].n) - e_d_d;
        fmt(a, (int)(c / S[k].n));
        fmt(b, (int)(c - S[k].n * 100));
        say("  %4d adds  %3d  %s %s", S[k].n, (S[k].n + 2) / 8, a, b);
    }
    say("a branch a line  cyc/branch");
    for (k = 0; k < sizeof(C) / sizeof(C[0]); k++)
    {
        long long c = cpi_from(C[k].fn, NULL, 1600000ul / C[k].n) - e_d_d;
        fmt(a, (int)(c / C[k].n));
        say("  %3d lines  %s", C[k].n, a);
    }
    /* The data side: the pointer chase round a few lines, all cached. */
    {
        static const unsigned char lines[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                               10, 11, 12, 14, 16, 24, 32 };
        size_t bufsize;
        unsigned char *big = rb->plugin_get_buffer(&bufsize);
        big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
        say("chase, cached   cyc/load");
        for (k = 0; k < sizeof(lines); k++)
        {
            make_chain(big, lines[k] * 32, 32);
            fmt(a, per_insn(cpi(chase, (unsigned long *)big), e_d_d));
            say("  %3d lines  %s", lines[k], a);
        }
    }
}

static void say_rows(const struct row *r, int n)
{
    int k;
    for (k = 0; k < n; k++)
    {
        char d[8];
        int div = r[k].per, sub = r[k].alu * 100;
        fmt(d, (per_insn(cpi(r[k].d, dram_buf), e_d_d) - sub) / div);
#ifdef USE_IRAM
        {
            char a[8], b[8], c[8];
            fmt(a, (per_insn(cpi(r[k].i, iram_buf), e_i_i) - sub) / div);
            fmt(b, (per_insn(cpi(r[k].i, dram_buf), e_i_d) - sub) / div);
            fmt(c, (per_insn(cpi(r[k].d, iram_buf), e_d_i) - sub) / div);
            say("%s %s %s %s %s", r[k].name, a, b, c, d);
        }
#else
        say("%s %s", r[k].name, d);
#endif
    }
}

/* Define to run only the newest rows, for a quick turn on a device
   whose other rows are already known. */
/* #define TEST_CYC_QUICK */

enum plugin_status plugin_start(const void* parameter)
{
    long long loop_cyc;
    (void)parameter;

    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_clear_display();
    fd = rb->open("/test_cyc.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0)
        say("cannot create /test_cyc.txt (%d): screen only", fd);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    /* subs and a taken branch: four cycles an iteration by the datasheet.
       A different figure here scales every other number the same way. */
    e_d_d = cpi(b_empty_d, dram_buf);
#ifdef USE_IRAM
    e_i_i = cpi(b_empty_i, iram_buf);
    e_i_d = cpi(b_empty_i, dram_buf);
    e_d_i = cpi(b_empty_d, iram_buf);
    loop_cyc = e_i_i;
#else
    loop_cyc = e_d_d;
#endif
    say("ARMv%d, clock %ld MHz, loop %d.%02d cyc (want 4)", ARM_ARCH,
        *rb->cpu_frequency / 1000000, (int)(loop_cyc / 100),
        (int)(loop_cyc % 100));
    say("timer: %s", TIMER_NAME);
#ifdef USE_IRAM
    say("cyc/insn  code:IRAM IRAM  DRAM  DRAM");
    say("          data:IRAM DRAM  IRAM  DRAM");
#else
    say("no plugin IRAM: code and data in DRAM");
    say("cyc/insn");
#endif

#if ARM_ARCH >= 5 && !defined(CPU_ARM_MICRO)
    {
        /* The control register, whose bit 14 picks round-robin cache
           replacement over random, and the cache type register. */
        unsigned long ctrl, ctype;
        asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (ctrl));
        asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (ctype));
        say("cp15 control %08lx (RR bit %lu), cache type %08lx", ctrl,
            (ctrl >> 14) & 1, ctype);
    }
#endif

#ifdef TEST_CYC_QUICK
    say("quick run: the newest rows only");
#if ARM_ARCH >= 5
    say_window_rows_everywhere();
#else
    say_miss_rows_everywhere();
#endif
#else
    say_rows(B, NB);
    say_rows(B2, NB2);
    say_rows(B3, NB3);
    say_filter_rows();
#if TEST_CYC_ARCHIVE
    say_filter_layout();
#endif
    say_code_size_rows();
#ifndef TICK_TIMED
#if TEST_CYC_ARCHIVE
    say_evict_rows();
    say_evict_set_rows();
#endif
    say_evict_lag_rows();
#endif
    say_miss_rows_everywhere();
#if ARM_ARCH >= 5
    say_code_miss_rows_everywhere();
    say_window_rows_everywhere();
#endif

    /* Stack traffic, here and on the codec thread. */
#ifdef USE_IRAM
    sp_empty_i = e_i_d;
#endif
    sp_empty_d = e_d_d;
    stack_rows();
    say_stack_rows("main");
    sp_done = false;
    rb->codec_thread_do_callback(stack_rows, NULL);
    while (!sp_done)
        rb->sleep(HZ / 10);
    rb->codec_thread_do_callback(NULL, NULL);
    say_stack_rows("codec");

#ifdef IRAM_MAP_BYTES
    /* What a store costs from IRAM code, by where in IRAM it lands. */
    {
        unsigned long off;
        say("iram store map, code at %08lx", (unsigned long)b_str_i);
        for (off = 0; off < IRAM_MAP_BYTES; off += 4096)
        {
            char a[8];
            unsigned long *q = &iram_map[off / 4];
            fmt(a, per_insn(cpi(b_str_i, q), e_i_i));
            say("  %08lx  %s", (unsigned long)q, a);
        }
    }
#endif

    /* Misses: a pointer chase over working sets either side of the cache,
       in DRAM code, from the plugin buffer.  The loop's own cost is the
       empty loop's, which the DRAM-code, DRAM-data run measured above. */
    {
        static const unsigned long sizes[] = { 1024, 4096, 8192, 16384,
                                               32768, 65536, 262144 };
        size_t bufsize;
        unsigned char *big = rb->plugin_get_buffer(&bufsize);
        unsigned long s32, k2;
        big = (unsigned char *)(((unsigned long)big + 31) & ~31ul);
        bufsize -= 32;
        say("addr: iram %08lx dram %08lx chase %08lx",
            (unsigned long)iram_buf, (unsigned long)dram_buf,
            (unsigned long)big);
        say("chase cyc/load    stride 16  stride 32");
        for (k2 = 0; k2 < sizeof(sizes) / sizeof(sizes[0]); k2++)
        {
            char a[8], b[8];
            int v[2];
            unsigned long bytes = sizes[k2];
            if (bytes > bufsize)
                break;
            for (s32 = 0; s32 < 2; s32++)
            {
                make_chain(big, bytes, s32 ? 32 : 16);
                v[s32] = per_insn(cpi(chase, (unsigned long *)big), e_d_d);
            }
            fmt(a, v[0]);
            fmt(b, v[1]);
            say("  %4lu KB          %s     %s", bytes / 1024, a, b);
        }

        /* Streaming: the same plugin buffer, as large as it allows up to
           256 KB, against 1 KB of it. */
        {
            static const struct { const char *name; benchfn fn; } S[] = {
                { "word   ", stream_word },
                { "line 32", stream_line },
                { "ldm 4  ", stream_ldm4 },
            };
            unsigned long large = bufsize > 262144 ? 262144 : bufsize;
            large &= ~511ul;
            say("stream cyc/load   cached  %luKB  extra", large / 1024);
            for (k2 = 0; k2 < sizeof(S) / sizeof(S[0]); k2++)
            {
                char a[8], b[8], c[8];
                int hit, miss;
                sbuf = (unsigned long *)big;
                sbytes = 1024;
                hit = per_insn(cpi(S[k2].fn, NULL), e_d_d);
                sbytes = large;
                miss = per_insn(cpi(S[k2].fn, NULL), e_d_d);
                fmt(a, hit);
                fmt(b, miss);
                fmt(c, miss - hit);
                say("  %s       %s  %s  %s", S[k2].name, a, b, c);
            }
        }

        /* Stores: to 1 KB that was read first, so its lines are cached;
           to 1 KB after 32 KB elsewhere was read, so they are not, unless
           a store brings its line in; and over the large buffer. */
        {
            static const struct { const char *name; benchfn fn; } W[] = {
                { "word   ", wstream_word },
                { "line 32", wstream_line },
                { "stm 4  ", wstream_stm4 },
            };
            unsigned long large = bufsize > 262144 ? 262144 : bufsize;
            large &= ~511ul;
            say("store stream     1K read 1K cold  %luKB", large / 1024);
            for (k2 = 0; k2 < sizeof(W) / sizeof(W[0]); k2++)
            {
                char a[8], b[8], c[8];
                sbuf = (unsigned long *)big;
                sbytes = 1024;
                stream_word(NULL, 64);
                fmt(a, per_insn(cpi(W[k2].fn, NULL), e_d_d));
                if (bufsize >= 131072)
                {
                    sbuf = (unsigned long *)(big + 65536);
                    sbytes = 32768;
                    stream_line(NULL, 256);
                }
                sbuf = (unsigned long *)big;
                sbytes = 1024;
                fmt(b, per_insn(cpi_from(W[k2].fn, NULL, ITERS), e_d_d));
                sbytes = large;
                fmt(c, per_insn(cpi(W[k2].fn, NULL), e_d_d));
                say("  %s       %s  %s  %s", W[k2].name, a, b, c);
            }
        }

        /* A miss with work after it. */
        {
            static const struct { const char *name; benchfn fn; } K[] = {
                { "+0 alu ", stream_line },
                { "+8 alu ", stream_work8 },
                { "+16 alu", stream_work16 },
                { "+32 alu", stream_work32 },
            };
            unsigned long large = bufsize > 262144 ? 262144 : bufsize;
            large &= ~511ul;
            say("load a line      cached  %luKB  extra", large / 1024);
            for (k2 = 0; k2 < sizeof(K) / sizeof(K[0]); k2++)
            {
                char a[8], b[8], c[8];
                int hit, miss;
                sbuf = (unsigned long *)big;
                sbytes = 1024;
                hit = per_insn(cpi(K[k2].fn, NULL), e_d_d);
                sbytes = large;
                miss = per_insn(cpi(K[k2].fn, NULL), e_d_d);
                fmt(a, hit);
                fmt(b, miss);
                fmt(c, miss - hit);
                say("  %s       %s  %s  %s", K[k2].name, a, b, c);
            }
        }

        /* Which word of the line the miss asks for, and what it costs
           when the lines it evicts are dirty. */
        {
            static const struct { const char *name; benchfn fn; } O[] = {
                { "word 0     ", stream_first },
                { "word 3     ", stream_mid },
                { "word 7     ", stream_last },
#if TEST_CYC_ARCHIVE
                { "word 3+rest", stream_mid_rest },
                { "word 7+rest", stream_last_rest },
                { "w0, dirty 4", stream_dirty4 },
                { "w0, dirty 8", stream_dirty8 },
                { "dirty 8 +16", stream_dirty8_w16 },
#endif
                { "dirty 8 +32", stream_dirty8_w32 },
                { "dirty 8 +64", stream_dirty8_w64 },
                { "dirty 8 +96", stream_dirty8_w96 },
#if TEST_CYC_ARCHIVE
                { "word 7 +16 ", stream_last_w16 },
#endif
                { "word 7 +32 ", stream_last_w32 },
            };
            unsigned long large = bufsize > 262144 ? 262144 : bufsize;
            large &= ~511ul;
            say("miss on         cached  %luKB  extra", large / 1024);
            for (k2 = 0; k2 < sizeof(O) / sizeof(O[0]); k2++)
            {
                char a[8], b[8], c[8];
                int hit, miss;
                sbuf = (unsigned long *)big;
                sbytes = 1024;
                hit = per_insn(cpi(O[k2].fn, NULL), e_d_d);
                sbytes = large;
                miss = per_insn(cpi(O[k2].fn, NULL), e_d_d);
                fmt(a, hit);
                fmt(b, miss);
                fmt(c, miss - hit);
                say("  %s  %s  %s  %s", O[k2].name, a, b, c);
            }
        }

#if TEST_CYC_ARCHIVE
        /* Two buffers filtered in turn and then rewritten together: cycles
           a pass of the rewrite, by buffer size and how far apart they
           are.  512 bytes each stay cached; 16 KB each do not both fit. */
        if (bufsize >= 131072)
        {
            static const struct { unsigned bytes, apart; } P[] = {
                { 512, 32768 }, { 4096, 32768 }, { 8192, 32768 },
                { 16384, 32768 }, { 16384, 34816 }, { 16384, 16384 },
                { 32768, 32768 },
            };
            say("pair rewrite     cyc/pass, 2 x 16 bytes");
            for (k2 = 0; k2 < sizeof(P) / sizeof(P[0]); k2++)
            {
                char a[8];
                long long both, filt;
                pair_a = (unsigned long *)big;
                pair_b = (unsigned long *)(big + P[k2].apart);
                pair_passes = P[k2].bytes / 16;
                filt = cpi_from(pair_filters, NULL, 4);
                both = cpi_from(pair_frame, NULL, 4);
                fmt(a, (int)((both - filt) / pair_passes));
                say("  %2uK, %2uK apart  %s", P[k2].bytes / 1024,
                    P[k2].apart / 1024, a);
            }
        }
#endif

        /* Loads far apart: the chase again, in scattered order. */
        {
            static const unsigned long sizes[] = { 65536, 262144, 1048576 };
            say("chase scattered   stride 32");
            for (k2 = 0; k2 < sizeof(sizes) / sizeof(sizes[0]); k2++)
            {
                char a[8];
                if (sizes[k2] > bufsize)
                    break;
                make_chain_scattered(big, sizes[k2], 32);
                fmt(a, per_insn(cpi(chase, (unsigned long *)big), e_d_d));
                say("  %4lu KB          %s", sizes[k2] / 1024, a);
            }
        }
    }

    /* Instruction fetch misses: a taken branch per line of DRAM code, over
       32 lines of it and over 2048. */
    {
        char a[8], b[8], c[8];
        int small = (int)((cpi_from(ichain_small, NULL, 4096) - e_d_d)
                          / ICHAIN_SMALL);
        int big2 = (int)((cpi_from(ichain_big, NULL, 64) - e_d_d)
                         / ICHAIN_BIG);
        fmt(a, small);
        fmt(b, big2);
        fmt(c, big2 - small);
        say("code chain cyc/branch  small %s  big %s  extra %s", a, b, c);
    }
#endif /* TEST_CYC_QUICK */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    if (fd >= 0)
    {
        rb->close(fd);
        fd = -1;
        say("wrote /test_cyc.txt, at the root of the main drive");
    }

    say("any key to exit");
    rb->button_clear_queue();
    rb->button_get(true);
    rb->lcd_setfont(FONT_UI);
    return PLUGIN_OK;
}
