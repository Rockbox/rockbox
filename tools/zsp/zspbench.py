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
Compares zspsim.py's timing model with cycle benchmarks measured on an RK27xx.

    zspbench.py [--suite DIR] [CASE...]

DIR is the ZSP test suite, by default utils/hwstub/tools/lua/rk27xx/zsp in
this tree: tests/bench/<case>_u<U>.bin and cases.json (make bench there)
and results/bench.txt (ZSP.bench() in rk27xx/zsp.lua, on the device).
Each benchmark repeats an instruction group U times in a hardware loop of
INNER iterations; as in the suite's tools/bench_report.py the cost of one
group is the difference between the U2 and U1 builds, which cancels the
loop overhead. The iteration column (U1 build, loop included) checks the
loop branch too.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import zspsim     # noqa: E402


def measured(path):
    """{(case, u): cycles of one outer pass} from results/bench.txt"""
    out = {}
    if not os.path.exists(path):
        return out
    for ln in open(path):
        p = ln.split()
        if len(p) < 6 or p[3] == 'TIMEOUT':
            continue
        name, u, outer, ticks, thz, dhz = p[0], int(p[1]), int(p[2]), int(p[3]), float(p[4]), float(p[5])
        out[(name, u)] = ticks / outer * dhz / thz
    return out


def simulate(path, mhz):
    """cycles of one outer pass (PARAM = 0) and the cycles before it"""
    s = zspsim.Sim(dsp_hz=mhz * 1e6)
    s.load_raw(open(path, 'rb').read())
    why = s.run(20_000_000)
    if why != 'halt':
        raise zspsim.SimError('stopped: ' + why)
    return s.cycles


SUITE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..',
                     'utils', 'hwstub', 'tools', 'lua', 'rk27xx', 'zsp')


def main():
    args = sys.argv[1:]
    rig = SUITE
    if args[:1] == ['--suite']:
        rig, args = args[1], args[2:]
    elif args[:1] in (['-h'], ['--help']):
        sys.exit(__doc__)
    if not os.path.exists(os.path.join(rig, 'tests', 'bench', 'cases.json')):
        sys.exit('no benchmarks in %s: run make bench there' % rig)
    want = set(args)
    meta = json.load(open(os.path.join(rig, 'tests', 'bench', 'cases.json')))
    inner, u1, u2 = meta['inner'], meta['u1'], meta['u2']
    hw = measured(os.path.join(rig, 'results', 'bench.txt'))
    print('%-12s %-6s %8s %8s %6s  %8s %8s  %s' % ('case', 'cat', 'hw/grp', 'sim/grp', 'diff',
                                                  'hw/iter', 'sim/iter', 'group'))
    worst = []
    for c in meta['cases']:
        name = c['name']
        if want and name not in want:
            continue
        try:
            t1, t2 = (simulate(os.path.join(rig, 'tests', 'bench', '%s_u%d.bin' % (name, u)), 160)
                      for u in (u1, u2))
        except zspsim.SimError as e:
            print('%-12s %-6s %8s  (%s)' % (name, c['cat'], 'n/s', e))
            continue
        sim = (t2 - t1) / (inner * (u2 - u1))
        sim_it = t1 / inner
        a, b = hw.get((name, u1)), hw.get((name, u2))
        if a and b:
            meas = (b - a) / (inner * (u2 - u1))
            meas_it = a / inner
            d = '%+6.2f' % (sim - meas)
            worst.append((abs(sim - meas), name))
            print('%-12s %-6s %8.2f %8.2f %6s  %8.2f %8.2f  %s' % (name, c['cat'], meas, sim, d,
                                                               meas_it, sim_it, ' ; '.join(c['group'])))
        else:
            print('%-12s %-6s %8s %8.2f %6s  %8s %8.2f  %s' % (name, c['cat'], '-', sim, '', '-',
                                                           sim_it, ' ; '.join(c['group'])))
    if worst:
        worst.sort(reverse=True)
        bad = [n for d, n in worst if d > 0.1]
        print('\n%d cases with hardware data, %d off by more than 0.1 cycle/group%s'
              % (len(worst), len(bad), (': ' + ' '.join(bad)) if bad else ''))


if __name__ == '__main__':
    main()
