#!/usr/bin/env python3
"""Generate silk/arm/resampler_armv5e_asm.S.

The FIR half of silk_resampler_private_IIR_FIR at 8, 12 and 16 kHz, for
ARMv5E and later.  Unlike ARMv4 (gen_resampler_armv4.py, beside this one),
the multiplier is the cheap path here: smla<x><y> is a 16x16
multiply-accumulate that issues in one cycle and picks either half of each
operand, so two samples arrive in one word, two coefficients in another, and
nothing is unpacked.

    python3 gen_resampler_armv5e.py > resampler_armv5e_asm.S
"""
import sys

T = [[189, -600, 617, 30567], [117, -159, -1070, 29704],
     [52, 221, -2392, 28276], [-4, 529, -3350, 26341],
     [-48, 758, -3956, 23973], [-80, 905, -4235, 21254],
     [-99, 972, -4222, 18278], [-107, 967, -3957, 15143],
     [-103, 896, -3487, 11950], [-91, 773, -2865, 8798],
     [-71, 611, -2143, 5784], [-46, 425, -1375, 2996]]
P = {p: T[p] + T[11 - p][::-1] for p in range(12)}

# Rates: name, outputs per loop iteration as (phase, first sample), and the
# words the window advances per iteration.  The buffer is word aligned, so
# sample s is half s&1 of word s>>1.  8 and 12 kHz advance one sample a
# cycle, so an iteration is two cycles and advances one word.
RATES = [
    ("16k", [(0, 0), (8, 0), (4, 1)], 1),
    ("8k", [(0, 0), (4, 0), (8, 0), (0, 1), (4, 1), (8, 1)], 1),
    ("12k", [(0, 0), (6, 0), (0, 1), (6, 1)], 1),
]

W = ["r5", "r6", "r7", "r8", "r9"]          # sample words W0..W4
CP = ["r10", "r12"]                          # coefficient words, first chain
CQ = ["r11", "lr"]                           # and second


def half(h):
    return "t" if h else "b"


def coef_words(p):
    """The four words of phase p's taps, low half the even tap."""
    c = P[p]
    return [((c[2 * i + 1] & 0xffff) << 16) | (c[2 * i] & 0xffff)
            for i in range(4)]


class Pool:
    def __init__(self, name):
        self.name, self.words = name, []

    def label(self, w):
        if w not in self.words:
            self.words.append(w)
        return ".L%s_k%d" % (self.name, self.words.index(w))


def mac(acc, tap, first, off, cw, bias):
    """smla/smul for tap `tap` of an output whose window starts at off."""
    s = off + tap
    x, y = half(s & 1), half(tap & 1)
    if first and not bias:
        return "smul%s%s  %s, %s, %s" % (x, y, acc, W[s >> 1], cw)
    return "smla%s%s  %s, %s, %s, %s" % (x, y, acc, W[s >> 1], cw, acc)


def chains(pool, a, b, first_loads_done):
    """Two interleaved multiply-accumulate chains.

    a and b are (acc, [(tap, off, word)], bias): each word covers two taps.
    Returns the instructions; the first word of each chain is expected to
    be loaded already into CP[0] and CQ[0].
    """
    out = []
    (acc_a, wa, bias_a), (acc_b, wb, bias_b) = a, b
    n = max(len(wa), len(wb))
    for w in range(n):
        ra, rb = CP[w % 2], CQ[w % 2]
        na, nb = CP[(w + 1) % 2], CQ[(w + 1) % 2]
        if w < len(wa):
            tap, off, _ = wa[w]
            out.append(mac(acc_a, tap, w == 0, off, ra, bias_a))
        if w < len(wb):
            tap, off, _ = wb[w]
            out.append(mac(acc_b, tap, w == 0, off, rb, bias_b))
        if w + 1 < len(wa):
            out.append("ldr     %s, %s" % (na, pool.label(wa[w + 1][2])))
        if w < len(wa):
            tap, off, _ = wa[w]
            out.append(mac(acc_a, tap + 1, False, off, ra, bias_a))
        if w + 1 < len(wb):
            out.append("ldr     %s, %s" % (nb, pool.label(wb[w + 1][2])))
        if w < len(wb):
            tap, off, _ = wb[w]
            out.append(mac(acc_b, tap + 1, False, off, rb, bias_b))
    return out


def gen_rate(name, outs, adv, f):
    pool = Pool(name)
    fn = "silk_IIR_FIR_%s_armv5e" % name
    w = f.write
    w("\n/* %s: %d outputs an iteration, phases %s. */\n"
      % (name, len(outs), "/".join(str(p) for p, _ in outs)))
    w("    .global %s\n    .type   %s, %%function\n%s:\n" % (fn, fn, fn))
    w("    push    {r4-r11, lr}\n")
    w(".L%s_loop:\n" % name)

    # Group outputs in pairs, two chains each; an odd one out is split into
    # its first and last four taps and the halves summed.
    groups = []
    i = 0
    while i < len(outs):
        if i + 1 < len(outs):
            groups.append(("pair", outs[i], outs[i + 1]))
            i += 2
        else:
            groups.append(("split", outs[i]))
            i += 1

    def spec(p, off):
        return [(2 * k, off, cw) for k, cw in enumerate(coef_words(p))]

    body = []
    body.append("ldr     %s, [r1], #%d" % (W[0], 4 * adv))
    body.append("ldmia   r1, {%s-%s}" % (W[1], W[4]))
    loaded = False
    for g in groups:
        if g[0] == "pair":
            (pa, oa), (pb, ob) = g[1], g[2]
            a = ("r3", spec(pa, oa), True)
            b = ("r4", spec(pb, ob), True)
            desc = ["phase %d at x%d" % (pa, oa), "phase %d at x%d" % (pb, ob)]
        else:
            p, o = g[1]
            s = spec(p, o)
            a = ("r3", s[:2], True)
            b = ("r4", s[2:], False)
            desc = ["phase %d at x%d, taps 0-3" % (p, o), "taps 4-7"]
        pre = ["ldr     %s, %s" % (CP[0], pool.label(a[1][0][2])),
               "ldr     %s, %s" % (CQ[0], pool.label(b[1][0][2])),
               "mov     r3, #0x4000                 @ rounding bias"]
        if b[2]:
            pre.append("mov     r4, #0x4000")
        pre[0] += " " * max(1, 36 - len(pre[0])) + "@ " + desc[0]
        pre[1] += " " * max(1, 36 - len(pre[1])) + "@ " + desc[1]
        body += pre
        body += chains(pool, a, b, loaded)
        if g[0] == "pair":
            body += ["qadd    r3, r3, r3                  @ SAT16(x >> 15)",
                     "qadd    r4, r4, r4                  @ in the top half",
                     "mov     r3, r3, asr #16",
                     "mov     r4, r4, asr #16",
                     "strh    r3, [r0], #2",
                     "strh    r4, [r0], #2"]
        else:
            body += ["add     r3, r3, r4",
                     "qadd    r3, r3, r3                  @ SAT16(x >> 15)",
                     "mov     r3, r3, asr #16",
                     "strh    r3, [r0], #2"]
    body += ["subs    r2, r2, #1", "bne     .L%s_loop" % name,
             "pop     {r4-r11, pc}"]
    for b in body:
        w("    " + b + "\n")
    w("    .align  2\n")
    for k, v in enumerate(pool.words):
        lo, hi = v & 0xffff, v >> 16
        sg = lambda h: h - 0x10000 if h & 0x8000 else h
        w(".L%s_k%d: .word   0x%08x              @ %d, %d\n"
          % (name, k, v, sg(lo), sg(hi)))
    w("    .size   %s, .-%s\n" % (fn, fn))


HEAD = """/* Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * silk/resampler_private_IIR_FIR.c are met.
 */

/* GENERATED by silk/arm/gen_resampler_armv5e.py -- edit that, not this.
 *
 * ARMv5E FIR interpolation for silk_resampler_private_IIR_FIR at the three
 * SILK rates.  Each step visits two or three fixed phase filters in a fixed
 * cycle (see the C), so the coefficients are constants, packed two to a word
 * in each function's literal pool.  The buffer is word aligned, so one load
 * brings two samples and smla<x><y> takes whichever halves a tap needs:
 * a window starting on an odd sample costs nothing extra.
 *
 * Outputs are computed in pairs, as two interleaved multiply-accumulate
 * chains, so no smla waits on the one before it.  An odd output out splits
 * its eight taps across the two chains and sums them.  The rounding bias
 * starts each accumulator, and qadd doubles with saturation, so the top half
 * is exactly SAT16((sum + 2^14) >> 15).
 *
 * opus_int16 *silk_IIR_FIR_<rate>_armv5e(opus_int16 *out,
 *                                        const opus_int16 *buf, int iters)
 *
 * buf must be word aligned.  Runs `iters` whole iterations (iters > 0) --
 * one cycle at 16 kHz, two at 8 and 12 kHz -- and returns the advanced out.
 * Each iteration reads the ten samples from buf and advances it one word.
 * r0 out, r1 buf, r2 iters, r3/r4 accumulators, r5-r9 sample words,
 * r10/r12 and r11/lr coefficient words for the two chains.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "resampler_armv5e_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2
"""


def main():
    f = sys.stdout
    f.write(HEAD)
    for r in RATES:
        gen_rate(*r, f=f)


if __name__ == "__main__":
    main()
