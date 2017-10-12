/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Michael Sevakis
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

static struct emac_context
{
    unsigned long saved;
    unsigned long r[4];
} emac_context IBSS_ATTR;

/* Save emac context affected in ISR */
static FORCE_INLINE void save_emac_context(void)
{
    /* Save only if not already saved */
    if (emac_context.saved == 0) {
        emac_context.saved = 1;
        asm volatile (
            "move.l   %%macsr, %%d0             \n"
            "move.l   %%accext01, %%d1          \n"
            "movclr.l %%acc0, %%a0              \n"
            "movclr.l %%acc1, %%a1              \n"
            "movem.l  %%d0-%%d1/%%a0-%%a1, (%0) \n"
            "move.l   %1, %%macsr               \n"
            :
            : "a"(&emac_context.r), "i"(EMAC_ROUND | EMAC_SATURATE)
            : "d0", "d1", "a0", "a1");
    }
}

/* Restore emac context affected in ISR */
static FORCE_INLINE void restore_emac_context(void)
{
    /* Restore only if saved */
    if (UNLIKELY(emac_context.saved != 0)) {
        asm volatile (
           "movem.l (%0), %%d0-%%d1/%%a0-%%a1  \n"
           "move.l  %%a1, %%acc1               \n"
           "move.l  %%a0, %%acc0               \n"
           "move.l  %%d1, %%accext01           \n"
           "move.l  %%d0, %%macsr              \n"
           :
           : "a"(&emac_context.r)
           : "d0", "d1", "a0", "a1");
        emac_context.saved = 0;
    }
}

#define mixer_buffer_cleanup() restore_emac_context()

static void
write_samples_unity_1ch_s16i_s16i(struct pcm_stream_desc *desc,
                                  void *out,
                                  unsigned long count)
{
    /* SM0.SM1|SM2.SM3|SM4.SM5|SM6.SM7 */
    /* SM0....|SM1.SM2|SM3.SM4|SM5.SM6|SM7.SM8? */
    const void *src = desc->addr_tx;

    asm volatile (
        "cmp.l      #4, %2                    \n"
        "blo.b      2f                        \n"
    "1:  movem.l    (%0), %%d0/%%d2           \n"
        "lea.l      8(%0), %0                 \n"
        "move.l     %%d0, %%d4                \n" /* SM0.SM1 */
        "move.l     %%d0, %%d1                \n" /* SM0.SM1 */
        "swap.w     %%d1                      \n" /* SM1.SM0 */
        "move.w     %%d1, %%d0                \n" /* SM0.SM0 */
        "move.w     %%d4, %%d1                \n" /* SM1.SM1 */
        "move.l     %%d2, %%d4                \n" /* SM2.SM3 */
        "move.l     %%d2, %%d3                \n" /* SM2.SM3 */
        "swap.w     %%d3                      \n" /* SM3.SM2 */
        "move.w     %%d3, %%d2                \n" /* SM2.SM2 */
        "move.w     %%d4, %%d3                \n" /* SM3.SM3 */
        "movem.l    %%d0-%%d3, (%1)           \n"
        "lea.l      16(%1), %1                \n"
        "subq.l     #4, %2                    \n"
        "bhi.b      1b                        \n"
        "beq.b      3f                        \n"
    "2:  move.w     (%0)+, %%d0               \n"
        "move.l     %%d0, %%d4                \n"
        "swap.w     %%d0                      \n"
        "move.w     %%d4, %%d0                \n"
        "move.l     %%d0, (%1)+               \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      2b                        \n"
    "3:                                       \n"
        : "+a"(src), "+a"(out), "+d"(count)
        :
        : "d0", "d1", "d2", "d3", "d4");
}

static void
write_samples_unity_2ch_s16i_s16i(struct pcm_stream_desc *desc,
                                  void *out,
                                  unsigned long count)
{
    memcpy(out, desc->addr_tx, count*2*sizeof (int16_t));
}

static void
write_samples_gain_1ch_s16i_s16i(struct pcm_stream_desc *desc,
                                 void *out,
                                 unsigned long count)
{
    save_emac_context();

    const void *src = desc->addr_tx;
    unsigned long t0, t1, t2, t3;

    /* M0.M1|M2.M3|M4.M5
         4                */
    /* ...M0|M1.M2|M3.M4
         3     4          */
    asm volatile (
        "move.l     %1, %6                    \n"
        "btst.l     #1, %6                    \n"
        "bne.b      1f                        \n"
        "move.l     (%1)+, %4                 \n" /* longword aligned */
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc0 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      2f                        \n"
        "bra.b      4f                        \n"
    "1:  movea.w    (%1)+, %3                 \n" /* word aligned */
        "mac.l      %3, %7, (%1)+, %4, %%acc0 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      3f                        \n"
        "bra.b      4f                        \n"
    "2:  movclr.l   %%acc0, %5                \n"
        "mac.l      %3, %7, (%1)+, %4, %%acc0 \n"
        "move.l     %5, %6                    \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)+                 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      4f                        \n"
    "3:  movclr.l   %%acc0, %5                \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc0 \n"
        "move.l     %5, %6                    \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)+                 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      2b                        \n"
    "4:  movclr.l   %%acc0, %5                \n"
        "move.l     %5, %6                    \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)                  \n"
        : "+a"(out), "+a"(src), "+d"(count),
          "=&a"(t0), "=&d"(t1), "=&d"(t2), "=&d"(t3)
        : "r"(desc->amplitude_tx), "d"(16));
}

static void
write_samples_gain_2ch_s16i_s16i(struct pcm_stream_desc *desc,
                                 void *out,
                                 unsigned long count)
{
    save_emac_context();

    const void *src = desc->addr_tx;
    int32_t t0, t1, t2, t3;

    /* L0.R0|L1.R1|L2.R2
         4                */
    /* ...L0|R0.L1|R1.L2
         3     4         */
    asm volatile (
        "move.l     %1, %6                    \n"
        "btst.l     #1, %6                    \n"
        "bne.b      2f                        \n"
        "move.l     (%1)+, %4                 \n" /* longword aligned */
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc0 \n"
        "mac.l      %3, %7, (%1)+, %4, %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      4f                        \n"
    "1:  movclr.l   %%acc0, %5                \n"
        "movclr.l   %%acc1, %6                \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc0 \n"
        "mac.l      %3, %7, (%1)+, %4, %%acc1 \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)+                 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      4f                        \n"
        "movclr.l   %%acc0, %5                \n"
        "movclr.l   %%acc1, %6                \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc0 \n"
        "mac.l      %3, %7, (%1)+, %4, %%acc1 \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)+                 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      1b                        \n"
        "bra.b      4f                        \n"
    "2:  movea.w    (%1)+, %3                 \n" /* word aligned */
        "mac.l      %3, %7, (%1)+, %4, %%acc0 \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      4f                        \n"
    "3:  movclr.l   %%acc0, %5                \n"
        "movclr.l   %%acc1, %6                \n"
        "mac.l      %3, %7, (%1)+, %4, %%acc0 \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc1 \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)+                 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      4f                        \n"
        "movclr.l   %%acc0, %5                \n"
        "movclr.l   %%acc1, %6                \n"
        "mac.l      %3, %7, (%1)+, %4, %%acc0 \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %8, %4                    \n"
        "mac.l      %4, %7           , %%acc1 \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)+                 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      3b                        \n"
    "4:  movclr.l   %%acc0, %5                \n"
        "movclr.l   %%acc1, %6                \n"
        "swap.w     %6                        \n"
        "move.w     %6, %5                    \n"
        "move.l     %5, (%0)                  \n"
        : "+a"(out), "+a"(src), "+d"(count),
          "=&a"(t0), "=&d"(t1), "=&d"(t2), "=&d"(t3)
        : "r"(desc->amplitude_tx), "d"(16));
}

static void
mix_samples_1ch_s16i_s16i(struct pcm_stream_desc *desc,
                          void *out,
                          unsigned long count)
{
    save_emac_context();

    const void *src = desc->addr_tx;
    int32_t t0, t1, t2, t3, t4, t5;

    /* DL0.DR0|DL1.DR1|DL2.DR2
          6
       SM0.SM1|SM2.SM4|SM4.SM5..
          4
       ....SM0|SM1.SM2|SM3.SM4..
          3       4
     */
    asm volatile (
        "move.l     %1, %7                    \n"
        "move.l     (%0), %6                  \n"
        "btst.l     #1, %7                    \n"
        "bne.b      1f                        \n"
        "move.l     (%1)+, %4                 \n" /* longword-aligned */
        "bra.b      3f                        \n"
    "1:  movea.w    (%1)+, %3                 \n" /* word-aligned */
        "movea.w    %6, %5                    \n"
        "asr.l      %10, %6                   \n"
        "bra.b      4f                        \n"
    "2:  movclr.l   %%acc0, %7                \n"
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)+                 \n"
    "3:  movea.w    %4, %3                    \n"
        "asr.l      %10, %4                   \n"
        "movea.w    %6, %5                    \n"
        "asr.l      %10, %6                   \n"
        "move.l     %6, %%acc0                \n"
        "move.l     %5, %%acc1                \n"
        "mac.l      %4, %9           , %%acc0 \n"
        "mac.l      %4, %9, 4(%0), %6, %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      5f                        \n"
        "movea.w    %6, %5                    \n"
        "asr.l      %10, %6                   \n"
        "movclr.l   %%acc0, %7                \n"
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)+                 \n"
    "4:  move.l     %6, %%acc0                \n"
        "move.l     %5, %%acc1                \n"
        "mac.l      %3, %9, (%1)+, %4, %%acc0 \n"
        "mac.l      %3, %9, 4(%0), %6, %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      2b                        \n"
    "5:  movclr.l   %%acc0, %7                \n"
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)                  \n"
        : "+a"(out), "+a"(src), "+d"(count),
          "=&a"(t0), "=&d"(t1), "=&a"(t2), "=&d"(t3), "=&d"(t4), "=&d"(t5)
        : "r"(desc->amplitude_tx), "d"(16));
}

static void
mix_samples_2ch_s16i_s16i(struct pcm_stream_desc *desc,
                          void *out,
                          unsigned long count)
{
    save_emac_context();

    const void *src = desc->addr_tx;
    int32_t t0, t1, t2, t3, t4, t5;

    /* DL0.DR0|DL1.DR1|DL2.DR2
          6
       SL0.SR0|SL1.SR1|SL2.SR2..
          4
       ....SL0|SR0.SL1|SR1.SL2..
          3       4
     */
    asm volatile (
        "move.l     %1, %7                    \n"
        "move.l     (%0), %6                  \n"
        "btst.l     #1, %7                    \n"
        "bne.b      3f                        \n"
        "move.l     (%1)+, %4                 \n"
        "bra.b      2f                        \n"
    "1:  movclr.l   %%acc0, %7                \n" /* longword-aligned */
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)+                 \n"
    "2:  movea.w    %4, %3                    \n"
        "asr.l      %10, %4                   \n"
        "movea.w    %6, %5                    \n"
        "asr.l      %10, %6                   \n"
        "move.l     %6, %%acc0                \n"
        "move.l     %5, %%acc1                \n"
        "mac.l      %4, %9, (%1)+, %4, %%acc0 \n"
        "mac.l      %3, %9, 4(%0), %6, %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      6f                        \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %10, %4                   \n"
        "movea.w    %6, %5                    \n"
        "asr.l      %10, %6                   \n"
        "movclr.l   %%acc0, %7                \n"
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)+                 \n"
        "move.l     %6, %%acc0                \n"
        "move.l     %5, %%acc1                \n"
        "mac.l      %4, %9, (%1)+, %4, %%acc0 \n"
        "mac.l      %3, %9, 4(%0), %6, %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      1b                        \n"
        "bra.b      6f                        \n"
    "3:  movea.w    (%1)+, %3                 \n" /* word-aligned */
        "bra.b      5f                        \n"
    "4:  movclr.l   %%acc0, %7                \n"
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)+                 \n"
    "5:  movea.w    %6, %5                    \n"
        "asr.l      %10, %6                   \n"
        "move.l     %6, %%acc0                \n"
        "move.l     %5, %%acc1                \n"
        "mac.l      %3, %9, (%1)+, %4, %%acc0 \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %10, %4                   \n"
        "mac.l      %4, %9, 4(%0), %6, %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bls.b      6f                        \n"
        "movea.w    %6, %5                    \n"
        "asr.l      %10, %6                   \n"
        "movclr.l   %%acc0, %7                \n"
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)+                 \n"
        "move.l     %6, %%acc0                \n"
        "move.l     %5, %%acc1                \n"
        "mac.l      %3, %9, (%1)+, %4, %%acc0 \n"
        "movea.w    %4, %3                    \n"
        "asr.l      %10, %4                   \n"
        "mac.l      %4, %9, 4(%0), %6, %%acc1 \n"
        "subq.l     #1, %2                    \n"
        "bhi.b      4b                        \n"
    "6:  movclr.l   %%acc0, %7                \n"
        "movclr.l   %%acc1, %8                \n"
        "swap.w     %8                        \n"
        "move.w     %8, %7                    \n"
        "move.l     %7, (%0)+                 \n"

        : "+a"(out), "+a"(src), "+d"(count),
          "=&a"(t0), "=&d"(t1), "=&a"(t2), "=&d"(t3), "=&d"(t4), "=&d"(t5)
        : "r"(desc->amplitude_tx), "d"(16));
}

/* these are the same for unity and gain applied */
#define mix_samples_unity_1ch_s16i_s16i     mix_samples_1ch_s16i_s16i
#define mix_samples_gain_1ch_s16i_s16i      mix_samples_1ch_s16i_s16i
#define mix_samples_unity_2ch_s16i_s16i     mix_samples_2ch_s16i_s16i
#define mix_samples_gain_2ch_s16i_s16i      mix_samples_2ch_s16i_s16i
