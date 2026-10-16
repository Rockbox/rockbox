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
Turn results/bench.txt (ZSP.bench in lua/zsp.lua) into cycles per group.

    cycles/group = (T(U2)/outer2 - T(U1)/outer1) * dsp_hz / tmr_hz / (INNER * (U2 - U1))

T/outer is the time of one outer pass (INNER inner iterations of the U-times
unrolled group plus fixed loop overhead); the difference between U2 and U1
cancels the overhead.
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    res = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'results', 'bench.txt')
    meta = json.load(open(os.path.join(HERE, '..', 'tests', 'bench', 'cases.json')))
    inner = meta['inner']
    runs = {}
    for ln in open(res):
        p = ln.split()
        if len(p) < 6:
            continue
        name, u, outer, ticks, thz, dhz = p[0], int(p[1]), int(p[2]), int(p[3]), float(p[4]), float(p[5])
        runs[(name, u)] = (ticks / outer, thz, dhz)
    cyc = {}
    print('%-12s %-6s %8s %8s  %-44s %s' % ('case', 'cat', 'cyc/grp', 'cyc/ins', 'group', 'note'))
    for c in meta['cases']:
        a, b = runs.get((c['name'], meta['u1'])), runs.get((c['name'], meta['u2']))
        if not a or not b:
            print('%-12s %-6s %8s' % (c['name'], c['cat'], 'n/a'))
            continue
        per = (b[0] - a[0]) * b[2] / b[1] / (inner * (meta['u2'] - meta['u1']))
        cyc[c['name']] = per
        print('%-12s %-6s %8.2f %8.2f  %-44s %s' % (c['name'], c['cat'], per, per / c['n'],
                                                     ' ; '.join(c['group']), c['note']))
    print('\nhazard penalties (cycles per occurrence):')
    for use, ref in [('load_use', 'load_nouse'), ('aload_use', 'aload_nouse'), ('st_ld_same', 'st_ld_diff'),
                     ('mac_use', 'mac_nouse'), ('mul_use', 'mul_nouse'), ('creg_use', 'creg_nouse'),
                     ('bz_taken', 'cmp_only'), ('bz_nottaken', 'cmp_only'), ('cexe_true', 'cexe_false')]:
        if use in cyc and ref in cyc:
            print('  %-12s - %-12s = %+.2f' % (use, ref, cyc[use] - cyc[ref]))
    if 'ld' in cyc:
        hz = list(runs.values())[0][2]
        print('\nDMEM bandwidth at %.0f MHz:' % (hz / 1e6))
        for n, bytes_ in [('ld', 2), ('ldd', 4), ('ldq', 8), ('st', 2), ('std', 4), ('stq', 8)]:
            if n in cyc and cyc[n] > 0:
                print('  %-4s %5.2f cycles/access  %7.1f MB/s' % (n, cyc[n], bytes_ * hz / cyc[n] / 1e6))


if __name__ == '__main__':
    main()
