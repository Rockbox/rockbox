#!/usr/bin/env python3
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (C) 2026 Marcin Bukat
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
"""
Bus and DMA timing against the DSP clock, from ZSP.sweep() (lua/zsp.lua).

    bus_sweep.py [SWEEPDIR]

SWEEPDIR (default results/sweep) holds one directory per DSP clock with
t30_bustime.txt, t35_dma.txt and t36_dma2.txt. The tests count in DSP
cycles, so a cost that is fixed in time grows with the clock and one fixed
in cycles does not. Each case is fitted as

    cycles = fixed + ns * f_DSP / 1000

(least squares; f in MHz): 'fixed' are DSP cycles, 'ns' the part that takes
fixed time (bus, SDRAM, DMA, set by HCLK). The clocks of each run are in its
result header; a sweep is only meaningful at one HCLK.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tests'))
import expect                                    # noqa: E402
from gen_t21 import T30, T35, T36                # noqa: E402


def header_clocks(path):
    ln = open(path).readline()
    out = {}
    for f in ln.split():
        k, _, v = f.partition('=')
        if k in ('DSP_HZ', 'ARM_HZ', 'HCLK'):
            out[k] = float(v)
    return out


def t30_cases(R):
    w = expect.out_words(R, 3 * len(T30) + 1)
    base = w[0]
    for i, (name, body, note) in enumerate(T30):
        cyc, r2, tag = w[3 * i:3 * i + 3]
        if i == 0 or tag != 0xC000 + i:
            continue
        n = sum(1 for b in body if b.split()[0] in ('ld', 'ldd', 'st', 'std')) or 1
        yield 't30 ' + name, ((cyc - base) & 0xffff) / n, 'per access'


def t35_cases(R):
    w = expect.out_words(R, 14 * len(T35) + 1)
    for i, (words, ctl, bts, note) in enumerate(T35):
        v = w[14 * i:14 * i + 14]
        if v[13] != 0xC000 + i or (v[1] == 0xFFFF and v[2] == 0xFFFF):
            continue
        cyc = (v[0] - 1) & 0xffff
        if cyc < 0xFFF0:
            yield 't35 %4d wd %s' % (words, note[:28]), cyc, 'total'


def t36_cases(R):
    w = expect.out_words(R, 8 * len(T36) + 1)
    for i, (kind, words, ctl, note) in enumerate(T36):
        tot, bus, l1, l0, raw, chen, bad, tag = w[8 * i:8 * i + 8]
        if tag != 0xC000 + i:
            continue
        if kind in ('ld', 'st'):
            yield 't36 %s' % note[:36], bus / words, 'per access'
        elif not (l0 == 0xFFFF and l1 == 0xFFFF):
            yield 't36 %4d wd %s' % (words, note[:28]), (tot - 1) & 0xffff, 'total'


TESTS = (('t30_bustime', t30_cases), ('t35_dma', t35_cases), ('t36_dma2', t36_cases))


def fit(pts):
    """cycles = a + b * f (f in MHz); returns a, b * 1000 (ns), max residual"""
    n = len(pts)
    sx = sum(f for f, _ in pts)
    sy = sum(c for _, c in pts)
    sxx = sum(f * f for f, _ in pts)
    sxy = sum(f * c for f, c in pts)
    d = n * sxx - sx * sx
    if n < 2 or d == 0:
        return None
    b = (n * sxy - sx * sy) / d
    a = (sy - b * sx) / n
    res = max(abs(c - (a + b * f)) for f, c in pts)
    return a, b * 1000, res


def main():
    sweep = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'results', 'sweep')
    runs = []                                    # (MHz, dir)
    for d in os.listdir(sweep):
        p = os.path.join(sweep, d)
        if os.path.isdir(p):
            hdr = os.path.join(p, 't30_bustime.txt')
            clk = header_clocks(hdr) if os.path.exists(hdr) else {}
            runs.append((clk.get('DSP_HZ', float(d) * 1e6) / 1e6, p, clk))
    runs.sort()
    if not runs:
        sys.exit('no runs in ' + sweep)
    hclks = {round(c.get('HCLK', 0) / 1e6, 1) for _, _, c in runs}
    print('DSP clocks (MHz): %s; HCLK (MHz): %s%s' % (
        ' '.join('%g' % round(m, 2) for m, _, _ in runs), ' '.join('%g' % h for h in sorted(hclks)),
        '  (differs between runs: fits mix two bus speeds)' if len(hclks) > 1 else ''))
    table = {}                                   # case -> {MHz: cycles}
    order = []
    for mhz, p, _ in runs:
        for fname, fn in TESTS:
            path = os.path.join(p, fname + '.txt')
            if not os.path.exists(path):
                continue
            R = expect.Result(path)
            for name, cyc, unit in fn(R):
                if name not in table:
                    table[name] = {}
                    order.append((name, unit))
                table[name][mhz] = cyc
    mhzs = [m for m, _, _ in runs]
    print('%-42s %-10s ' % ('case', 'unit') + ' '.join('%8s' % ('%gM' % round(m, 1)) for m in mhzs)
          + '  %8s %8s %6s' % ('fixed', 'ns', 'resid'))
    for name, unit in order:
        pts = sorted(table[name].items())
        cells = ' '.join('%8.1f' % table[name][m] if m in table[name] else '%8s' % '-' for m in mhzs)
        r = fit(pts)
        tail = '  %8.1f %8.1f %6.1f' % r if r else ''
        print('%-42s %-10s %s%s' % (name, unit, cells, tail))


if __name__ == '__main__':
    main()
