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

#define ITERS   200000ul        /* first try at loop iterations per run */
#define UNROLL  16

#define X16(i) i i i i i i i i i i i i i i i i

/* The data a benchmark touches: small, so the DRAM copy is a cache hit. */
static unsigned long iram_buf[64] IBSS_ATTR __attribute__((aligned(32)));
static unsigned long dram_buf[64] __attribute__((aligned(32)));


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
static void name##_d(unsigned long *p, unsigned long n)                     \
{                                                                           \
    asm volatile (setup "1:\n" X16(body)                                    \
                  "   subs    %[n], %[n], #1\n"                             \
                  "   bne     1b\n"                                         \
                  : [n] "+r" (n) : [p] "r" (p)                              \
                  : "r4", "r5", "r6", "r7", "r8", "r9", "cc", "memory");    \
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
static long long cpi(benchfn fn, unsigned long *p)
{
    unsigned long n = ITERS, t0, t1;
    fn(p, 64);                          /* warm the caches */
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

static const struct {
    const char *name;
    benchfn i, d;
    int per;                            /* instructions per X16 copy */
    int alu;                            /* ALU cycles per copy, subtracted */
} B[] = {
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

enum plugin_status plugin_start(const void* parameter)
{
    long long e_d_d, loop_cyc;
#ifdef USE_IRAM
    long long e_i_i, e_i_d, e_d_i;
#endif
    int k;
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

    for (k = 0; k < NB; k++)
    {
        char d[8];
        int div = B[k].per, sub = B[k].alu * 100;
        fmt(d, (per_insn(cpi(B[k].d, dram_buf), e_d_d) - sub) / div);
#ifdef USE_IRAM
        {
            char a[8], b[8], c[8];
            fmt(a, (per_insn(cpi(B[k].i, iram_buf), e_i_i) - sub) / div);
            fmt(b, (per_insn(cpi(B[k].i, dram_buf), e_i_d) - sub) / div);
            fmt(c, (per_insn(cpi(B[k].d, iram_buf), e_d_i) - sub) / div);
            say("%s %s %s %s %s", B[k].name, a, b, c, d);
        }
#else
        say("%s %s", B[k].name, d);
#endif
    }

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
    }

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
