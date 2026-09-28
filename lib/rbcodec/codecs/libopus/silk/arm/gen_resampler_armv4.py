#!/usr/bin/env python3
"""Generate silk/arm/resampler_armv4_asm.S.

The FIR half of silk_resampler_private_IIR_FIR at 8, 12 and 16 kHz uses two
or three fixed phase filters in a fixed cycle (see the C), so every
coefficient is a compile-time constant.  On ARM7TDMI a constant multiply is
cheaper as shifts and adds than as mla: each add with a shifted operand is
one cycle, where mla is 3-4 cycles plus a load of the coefficient.

The kernel is sample-major: each input sample is loaded once per cycle and
added into the two or three accumulators it feeds, so a partial product
such as x*(1+2^a) can be shared between the coefficients one sample meets.
The search below finds, per sample, the cheapest set of up to two such
temporaries and the fewest shifted terms per coefficient.

    python3 gen_resampler_armv4.py > resampler_armv4_asm.S
"""
import itertools
import random
import sys
from functools import lru_cache

T = [[189, -600, 617, 30567], [117, -159, -1070, 29704],
     [52, 221, -2392, 28276], [-4, 529, -3350, 26341],
     [-48, 758, -3956, 23973], [-80, 905, -4235, 21254],
     [-99, 972, -4222, 18278], [-107, 967, -3957, 15143],
     [-103, 896, -3487, 11950], [-91, 773, -2865, 8798],
     [-71, 611, -2143, 5784], [-46, 425, -1375, 2996]]
# The eight taps of phase p, in sample order, as the C applies them.
P = {p: T[p] + T[11 - p][::-1] for p in range(12)}

LIM = 1 << 17


def csd(c):
    """Canonical signed digits of c as (sign, shift)."""
    d, e = [], 0
    while c:
        if c & 1:
            r = 2 - (c & 3)
            d.append((r, e))
            c -= r
        c >>= 1
        e += 1
    return d


@lru_cache(None)
def singles(V):
    s = {}
    for v in V:
        k = 0
        while (v << k) < LIM:
            for sg in (1, -1):
                s.setdefault(sg * (v << k), (sg, v, k))
            k += 1
    return s


@lru_cache(None)
def doubles(V):
    s1 = singles(V)
    d = {}
    for a, b in itertools.product(s1.items(), repeat=2):
        d.setdefault(a[0] + b[0], (a[1], b[1]))
    return d


@lru_cache(None)
def terms(c, V):
    """Fewest (sign, v, shift) terms, v in V, summing to c (None if > 4)."""
    s1, s2 = singles(V), doubles(V)
    if c in s1:
        return [s1[c]]
    if c in s2:
        return list(s2[c])
    for a, ta in s1.items():
        if c - a in s2:
            return [ta] + list(s2[c - a])
    for a, ta in s2.items():
        if c - a in s2:
            return list(ta) + list(s2[c - a])
    return None


def plain(c):
    return [(sg, 1, k) for sg, k in csd(c)]


TEMPS = sorted({(1 << a) + 1 for a in range(1, 15)} |
               {(1 << a) - 1 for a in range(2, 15)})


def program(cs):
    """(temps, [terms per coefficient]) with the fewest instructions."""
    best = (sum(len(csd(c)) for c in cs), (), [plain(c) for c in cs])
    for n in (1, 2):
        for ts in itertools.combinations(TEMPS, n):
            V = (1,) + ts
            tl = []
            cost = n
            for c in cs:
                t = terms(c, V)
                if t is None or len(t) > len(csd(c)):
                    t = plain(c)
                tl.append(t)
                cost += len(t)
                if cost >= best[0]:
                    break
            else:
                best = (cost, ts, tl)
    return best


# Rates: step, phases in cycle order, window offset of each output, samples
# advanced per cycle.
RATES = [
    ("16k", 43691, [0, 8, 4], [0, 0, 1], 2),
    ("8k", 21846, [0, 4, 8], [0, 0, 0], 1),
    ("12k", 32768, [0, 6], [0, 0], 1),
]

ACC = ["r3", "r4", "r5"]
XR = ["r6", "r7"]
TR = ["r8", "r9"]


def gen_rate(name, step, phases, offs, adv, out):
    nsamp = max(offs) + 8
    # coefficients each sample meets, per accumulator
    need = []
    for j in range(nsamp):
        cs = []
        for o, (p, off) in enumerate(zip(phases, offs)):
            i = j - off
            if 0 <= i < 8:
                cs.append((o, P[p][i]))
        need.append(cs)
    # Process x1 .. x(n-1), then x0, whose load post-increments the pointer.
    order = list(range(1, nsamp)) + [0]
    progs = {}
    total = 0
    for j in order:
        cost, ts, tl = program(tuple(c for _, c in need[j]))
        progs[j] = (ts, tl)
        total += cost
    # Check every program arithmetically before emitting it.
    for j in order:
        ts, tl = progs[j]
        for x in (1, -1, 32767, -32768, random.randint(-32768, 32767)):
            val = {1: x}
            for t in ts:
                val[t] = x * t
            for (o, c), tt in zip(need[j], tl):
                assert sum(sg * (val[v] << k) for sg, v, k in tt) == c * x, \
                    (name, j, c, tt)

    w = out.write
    fn = "silk_IIR_FIR_%s_armv4" % name
    w("\n/* %s: phases %s, %d samples a cycle, advancing %d.  %d shifted"
      " adds a\n   cycle, %.1f an output. */\n"
      % (name, "/".join(map(str, phases)), nsamp, adv, total,
         total / len(phases)))
    w("    .global %s\n    .type   %s, %%function\n%s:\n" % (fn, fn, fn))
    w("    push    {r4-r11, lr}\n")
    w("    mov     r10, #0x7f00\n    orr     r10, r10, #0xff        @ 0x7fff, for the clamp\n")
    w("    mov     r11, #0x4000                  @ the rounding bias\n")
    w("    ldrsh   %s, [r1, #2]                  @ x1\n" % XR[0])
    w(".L%s_loop:\n" % name)
    started = set()
    for n, j in enumerate(order):
        xr = XR[n % 2]
        ts, tl = progs[j]
        body = []
        treg = {1: xr}
        for t, r in zip(ts, TR):
            treg[t] = r
            if (t - 1) & (t - 2) == 0:                     # 2^a + 1
                a = (t - 1).bit_length() - 1
                body.append("add     %s, %s, %s, lsl #%d" % (r, xr, xr, a))
            else:                                          # 2^a - 1
                a = (t + 1).bit_length() - 1
                assert (1 << a) - 1 == t
                body.append("rsb     %s, %s, %s, lsl #%d" % (r, xr, xr, a))
        for (o, c), tt in zip(need[j], tl):
            acc = ACC[o]
            first = len(body)
            for sg, v, k in tt:
                sh = (", lsl #%d" % k) if k else ""
                base = acc if o in started else "r11"
                started.add(o)
                body.append("%s     %s, %s, %s%s" % ("add" if sg > 0 else "sub",
                                                    acc, base, treg[v], sh))
            body[first] += " " * max(1, 34 - len(body[first])) + \
                "@ x%d * %d" % (j, c)
        # The next sample's load goes after this program's first instruction,
        # into the register the sample before this one has finished with.
        if n + 1 < len(order):
            nj = order[n + 1]
            if nj == 0:
                ld = "ldrsh   %s, [r1], #%d" % (XR[(n + 1) % 2], 2 * adv)
                ld += " " * max(1, 34 - len(ld)) + "@ x0, and advance"
            else:
                ld = "ldrsh   %s, [r1, #%d]" % (XR[(n + 1) % 2], 2 * nj)
                ld += " " * max(1, 34 - len(ld)) + "@ x%d" % nj
            body.insert(1, ld)
        for b in body:
            w("    " + b + "\n")
    # Clamp and store; the next cycle's x1 loads under the first clamp.
    for o in range(len(phases)):
        acc, r = ACC[o], TR[o % 2]
        w("    teq     %s, %s, lsl #1              @ out of 16 bits?\n" % (acc, acc))
        if o == 0:
            w("    ldrsh   %s, [r1, #2]                  @ next x1\n" % XR[0])
        w("    mov     %s, %s, asr #15\n" % (r, acc))
        w("    eormi   %s, r10, %s, asr #31\n" % (r, acc))
        w("    strh    %s, [r0], #2\n" % r)
    w("    subs    r2, r2, #1\n")
    w("    bne     .L%s_loop\n" % name)
    w("    pop     {r4-r11, pc}\n")
    w("    .size   %s, .-%s\n" % (fn, fn))
    return total


HEAD = """/* Copyright (c) 2026 Michael Giacomelli
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions stated in
 * silk/resampler_private_IIR_FIR.c are met.
 */

/* GENERATED by silk/arm/gen_resampler_armv4.py -- edit that, not this.
 *
 * ARMv4 FIR interpolation for silk_resampler_private_IIR_FIR at the three
 * SILK rates.  Each step visits two or three fixed phase filters in a fixed
 * cycle, so every coefficient is a constant, and on ARM7TDMI a constant
 * multiply is cheaper as adds of shifted operands (one cycle each) than as
 * mla (three or four, plus loading the coefficient).
 *
 * Sample-major: each input sample is loaded once a cycle and added into the
 * two or three accumulators it feeds, sharing a partial product x*(2^a+-1)
 * between them where that saves an add.  The rounding bias rides in on
 * each accumulator's first add.  Every sum is exact in 32 bits: the largest
 * phase has sum |c| = 50,045, so |sum| < 2^31 for 16-bit input.
 *
 * opus_int16 *silk_IIR_FIR_<rate>_armv4(opus_int16 *out,
 *                                       const opus_int16 *buf, int cycles)
 *
 * Runs `cycles` whole cycles (cycles > 0) and returns the advanced out.
 * r0 out, r1 buf, r2 cycles, r3-r5 accumulators, r6/r7 samples, r8/r9
 * partial products, r10 0x7fff, r11 the rounding bias.
 */

#if defined(__thumb__) || defined(__thumb2__)
#error "resampler_armv4_asm.S must be assembled in ARM mode"
#endif

    .text
    .align  2
"""


def main():
    random.seed(1)
    out = sys.stdout
    out.write(HEAD)
    tot = {}
    for r in RATES:
        tot[r[0]] = gen_rate(*r, out=out)
    sys.stderr.write("adds per cycle: %s\n" % tot)


if __name__ == "__main__":
    main()
