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
Check on-device results (results/<test>.txt written by lua/zsp.lua) against
the encoding model. Prints PASS / FAIL / INFO per claim.

    python3 tests/expect.py [results_dir] [test ...]
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CREGS = ['fmode', 'tc', 'imask', 'loop0', 'loop1', 'loop2', 'loop3', 'amode', 'smode',
         'rpc', 'tpc', 'hwflag', 'shwflag', 'psmode', 'vitr']
RES_OFF, SCRATCH_OFF = 0x100, 0x300
MAGIC = 0xD0DE


class Result:
    def __init__(self, path):
        self.name = os.path.splitext(os.path.basename(path))[0]
        self.w = {}
        self.raw = {}
        self.dsp_hz = 160e6         # results from before the clock was recorded
        for ln in open(path):
            m = re.match(r'# (\S+) (\S+) DMEM_DSP=(\S+) DMEM_UNIT=(\d+)', ln)
            if m:
                self.done = m.group(2) == 'done'
                self.dmem_dsp = int(m.group(3), 16)
                self.unit = int(m.group(4))
                m = re.search(r' DSP_HZ=(\d+)', ln)
                if m:
                    self.dsp_hz = float(m.group(1))
                continue
            m = re.match(r'raw ([0-9a-f]+): (.*)', ln)
            if m:
                base = int(m.group(1), 16)
                for i, h in enumerate(m.group(2).split()):
                    self.raw[base + 2 * i] = int(h, 16)
                continue
            m = re.match(r'([0-9a-f]+): (.*)', ln)
            if m:
                base = int(m.group(1), 16)
                for i, h in enumerate(m.group(2).split()):
                    self.w[base + i] = int(h, 16)
        self.r = [self.w[i] for i in range(16)]
        self.n = [self.w[30 + i] for i in range(8)]
        self.creg = {n: self.w[38 + i] for i, n in enumerate(CREGS)}
        self.g = [self.w[53 + i] for i in range(4)]
        self.scratch = self.dmem_dsp + SCRATCH_OFF
        self.extra = [self.w[0x40 + i] for i in range(0xc0)]

    def a(self, k):
        """a-register as (low-word-first, high-word-first) interpretations."""
        w0, w1 = self.w[16 + 2 * k], self.w[17 + 2 * k]
        return (w1 << 16) | w0, (w0 << 16) | w1

    def a_lo(self, k):
        """Possible low halves of a_k (word order of std a is itself unknown)."""
        return {self.w[16 + 2 * k], self.w[17 + 2 * k]}


def symbols(name):
    syms = {}
    lst = os.path.join(HERE, name + '.lst')
    if os.path.exists(lst):
        for ln in open(lst):
            m = re.match(r'; (\w+)\s+0x([0-9A-F]+)', ln)
            if m:
                syms[m.group(1)] = int(m.group(2), 16)
    return syms


class Report:
    def __init__(self):
        self.counts = {'PASS': 0, 'FAIL': 0, 'INFO': 0}

    def check(self, what, got, exp):
        """got may be a set/tuple of candidate readings (unknown word order)."""
        ok = exp in got if isinstance(got, (set, tuple)) else got == exp
        tag = 'PASS' if ok else 'FAIL'
        self.counts[tag] += 1
        print('  %-4s %-46s got %-19s expect %s' % (tag, what, fmt(got), fmt(exp)))
        return ok

    def info(self, what, val):
        self.counts['INFO'] += 1
        print('  INFO %-46s %s' % (what, val))


def h(v):
    return '%04X' % v


def fmt(v):
    if isinstance(v, (set, tuple)):
        return '{' + ','.join(fmt(x) for x in sorted(v)) + '}'
    return '%08X' % v if v > 0xffff else '%04X' % v


# ---------------------------------------------------------------- per test

def t0c_units(R, rep):
    for i, v in enumerate([0x1122, 0x3344, 0x5566, 0x7788, 0x99AA]):
        rep.check('read back SCRATCH+%d (r%d)' % (i, i + 1), R.r[i + 1], v)
    rep.check('stu post-modify +1 (a2 low)', R.a_lo(2), (R.scratch + 4) & 0xffff)
    pos = {}
    for off, v in R.raw.items():
        if v in (0x1122, 0x3344, 0x5566, 0x7788, 0x99AA):
            pos.setdefault(v, []).append(off)
    rep.info('host byte offsets of markers', ' '.join('%04X@%s' % (v, ','.join('%#x' % o for o in pos[v]))
                                                      for v in sorted(pos)))
    # the result block also holds these values (dumped r1-r5); use the scratch copies
    sbase = (R.scratch - R.dmem_dsp) * R.unit
    near = {v: [o for o in pos.get(v, []) if o >= sbase] for v in (0x1122, 0x3344)}
    if near[0x1122] and near[0x3344]:
        d = near[0x3344][0] - near[0x1122][0]
        rep.check('host bytes per DSP data address (DMEM_UNIT)', d, R.unit)
    rep.check('std r0,a0,6: SCRATCH+6 = r0 (low address = even reg)', R.raw.get(sbase + 6 * R.unit), 0xBBCC)
    rep.info('dump slot 0 (r0, last set to BBCC)', h(R.r[0]))


def t1_call(R, rep):
    s = symbols('t1_call')
    rep.check('%rpc after 32-bit call (r1) == ret1', R.r[1], s['ret1'])
    rep.check('%rpc after 16-bit call r3 (r2) == ret2', R.r[2], s['ret2'])
    rep.check('mov.l r6,ret1 (label value sanity)', R.r[6], s['ret1'])
    rep.info('%pc read at "here" (r4) - here', '%+d' % (R.r[4] - s['here']))


def t2_branch(R, rep):
    for reg, what in [(6, 'A 16-bit bz forward taken'), (7, 'B 16-bit bnz not taken'),
                      (8, 'C 16-bit bz backward taken'), (10, 'E 32-bit bz.l forward'),
                      (11, 'F 32-bit br forward > 127')]:
        rep.check(what + ' (r%d)' % reg, R.r[reg], 1)
    rep.info('D agn0 passes with %loop0=3 (r9)', R.r[9])


def t2b_bgt32(R, rep):
    if not R.done:
        rep.info('bgt.l forward', 'TIMEOUT: hardware takes it backward, like the DLL')
        return
    rep.check('bgt.l +4 skipped next insn (r10)', R.r[10], 0)
    rep.check('landed on target (r11)', R.r[11], 1)


def t3_regs(R, rep):
    for i in (0, 1, 2, 3, 6, 7, 10, 11):
        exp = 0xA003 if i == 6 else 0x1000 + i
        rep.check('r%d' % i, R.r[i], exp)
    pair = {(0x1003, 0x1002): 'a1 = {r3:r2} (odd register is high)',
            (0x1002, 0x1003): 'a1 = {r2:r3} (even register is high)'}
    rep.info('mov.e a1,r2 then movhw r4 / movlw r5', '%s %s -> %s' % (h(R.r[4]), h(R.r[5]),
             pair.get((R.r[4], R.r[5]), '?')))
    rep.check('mov a2,imm + movhw a2,imm', R.a(2), 0x33332222)
    rep.check('mov a3,a2', R.a(3)[0], R.a(2)[0])
    w = R.a(2)
    rep.info('std word order', 'low word first' if w[0] == 0x33332222 else 'high word first' if w[1] == 0x33332222 else '?')
    rep.check('mov n1,imm', R.n[1], 0x0111)
    rep.check('mov n2,r1', R.n[2], 0x1001)
    rep.check('mov n3,n1', R.n[3], 0x0111)
    rep.info('ldd r8,a0 -> r8 r9', '%s %s' % (h(R.r[8]), h(R.r[9])))
    rep.info('ldq r12,a0 -> r12..r15', ' '.join(h(x) for x in R.r[12:16]))
    rep.info('ldd a4,a0 -> a4', '%08X / %08X' % R.a(4))
    rep.info('mov r0,g0/g2/g4/g6 after g_i = 0x10+0x11*i', ' '.join(h(x) for x in R.g))


def t4_mem(R, rep):
    B = (R.scratch + 8) & 0xffff
    for reg, exp, what in [(1, 0x4008, 'ldu [B]'), (2, 0x400A, 'ldu [B+2]'), (3, 0x400B, 'ldu [B+3]'),
                           (4, 0x4009, 'ldu [B+1]'), (5, 0x400B, 'ld a5,3'), (6, 0x4007, 'ld a5,-1'),
                           (8, 0x400A, 'ld r8,r7,2 (r base)'), (9, 0x400B, 'ldx a5,n1 (n1=3)'),
                           (12, 0x5A5A, 'pop after push')]:
        rep.check('%s (r%d)' % (what, reg), R.r[reg], exp)
    for k, d, m in [(1, 2, 2), (2, 3, 1), (3, 1, -2), (4, 0, -1)]:
        rep.check('a%d after ldu ...,%d' % (k, m), R.a_lo(k), (B + d) & 0xffff)
    sp = (R.scratch + 0x40) & 0xffff
    rep.info('a6 after push (r11) - start', '%+d' % (R.r[11] - sp))
    rep.info('a6 after pop (r13) - start', '%+d' % (R.r[13] - sp))
    where = [n for n, v in (('SCRATCH+0x3F', R.r[14]), ('SCRATCH+0x40', R.r[15]), ('SCRATCH+0x41', R.r[7]))
             if v == 0x5A5A]
    rep.info('push wrote to', ', '.join(where) or 'none of +0x3F..+0x41')


def t5_imm(R, rep):
    rep.check('add a0,-3 from 0x100', R.a_lo(0), 0x00FD)
    rep.check('shll a1,3 (shift_imm[0])', R.a_lo(1), 0x0008)
    rep.check('shll a2,4 (shift_imm[3])', R.a_lo(2), 0x0010)
    rep.check('shrl a3,1 (shift_imm[2])', R.a_lo(3), 0x0080)
    rep.check('add n1,7', R.n[1], 0x0107)
    for reg, exp, what in [(1, 0xFFF8, 'mov r1,0xFFF8 (ADxx imm4)'), (2, 0xFFFE, 'add r2,-2 (AExx)'),
                           (3, 0x0000, 'and r3,0x00FF'), (4, 0xFFF0, 'add r4,-32 (F85x)'),
                           (5, 0x0F0F, 'or r5,0x000F'), (6, 0x2340, 'shll r6,4 (A8xx)'),
                           (7, 0xF000, 'shra r7,3 (A8xx)'), (8, 0x000F, 'cmp_imm 0,1,-2,-1 all set Z')]:
        rep.check('%s (r%d)' % (what, reg), R.r[reg], exp)


def t6_creg(R, rep):
    for reg, exp, what in [(2, 0x0011, '62xx write %loop0'), (3, 0x0011, 'second read'),
                           (4, 0x0033, '7F0x write %loop0 from n1'), (5, 0x0044, 'F96x mov %loop0,imm'),
                           (6, 0x0055, 'F94x movl %loop0'), (7, 0x6655, 'F95x movh %loop0'),
                           (8, 0x0123, '%loop1 62xx/60xx'), (9, 0x0C00, '%cb0_beg 62xx -> 7Bxx'),
                           (10, 0x0C10, '%cb0_end 62xx -> 7Bxx')]:
        rep.check('%s (r%d)' % (what, reg), R.r[reg], exp)
    s = symbols('t6_creg')
    rep.check('mov r14,%rpc == c_ret', R.r[14], s.get('c_ret', -1))
    rep.info('mov.e r12,%rpc -> r12 r13', '%s %s' % (h(R.r[12]), h(R.r[13])))


def t7_xbits(R, rep, rdir):
    ref = os.path.join(rdir, 't7_xbits_ref.txt')
    if not os.path.exists(ref):
        rep.info('x bits', 'run t7_xbits_ref too')
        return
    Q = Result(ref)
    diff = [i for i in range(62) if R.w[i] != Q.w[i]]
    # Finding: bits marked x are NOT don't-care on hardware; encodings must
    # have them 0 (the manual says so). A difference here is the expected result.
    rep.info('words differing with x bits set (expected: x bits must be 0)', len(diff))
    for i in diff:
        rep.info('word %d differs' % i, '%s (x=1) vs %s (x=0)' % (h(R.w[i]), h(Q.w[i])))


def t8_alu(R, rep):
    meta = json.load(open(os.path.join(HERE, 't8_alu.json')))
    for m in meta:
        got = R.extra[m['i']]
        what = '%-14s r1=%04X r2=%04X' % (m['insn'], m['r1'], m['r2'])
        if m['expect'] is None:
            rep.info(what, h(got))
        else:
            rep.check(what, got, m['expect'])


def t9_cexe(R, rep):
    rep.check('cexe true !2 executes both (r6)', R.r[6], 2)
    rep.check('insn after shadow (r7)', R.r[7], 1)
    rep.check('cexe false !2 skips both (r8)', R.r[8], 0)
    rep.check('cexe false !1 skips one (r9)', R.r[9], 1)


def t10(R, rep):
    rep.check('marker before invalid opcode', R.extra[0], 0x1111)
    rep.info('execution after invalid opcode',
             'continued' if R.extra[1] == 0x2222 else 'stopped / trapped (%s)' % h(R.extra[1]))


def t12_hazards(R, rep):
    meta = json.load(open(os.path.join(HERE, 't12_hazards.json')))
    base = None
    for m in meta:
        e = R.extra[m['slot']:m['slot'] + 4]
        vi, ci, vd, cd = e
        if m['name'] == 'empty':
            base = ci
            rep.info('timer overhead (two back-to-back reads)', '%d cycles' % ci)
            continue
        cyc = 'imm %d, delayed %d cycles (timer overhead removed)' % (ci - (base or 0), cd - (base or 0))
        what = '%-9s %-40s' % (m['name'], ' ; '.join(m['seq'])[:40])
        if m['fresh'] is not None:
            ok = rep.check(what, vi, m['fresh'])
            if not ok and vi == m['stale']:
                rep.info('  -> %s' % m['name'], 'STALE value: hazard, no interlock')
        else:
            rep.check(what + ' imm == dly', vi, vd)
        rep.info('  cycles %s' % m['name'], cyc + ', delayed value %s' % h(vd))


def t13_inferred(R, rep):
    cases = json.load(open(os.path.join(HERE, 't13_inferred.json')))
    base = (R.dmem_dsp + SCRATCH_OFF + 0x100 - R.dmem_dsp) * R.unit
    bad = 0
    print('  %-14s %-22s %-4s %-4s %-4s %-4s %3s' % ('case', 'instruction', 'r0', 'r1', 'r2', 'r3', 'cyc'))
    for i, c in enumerate(cases):
        w = [R.raw.get(base + (6 * i + k) * R.unit) for k in range(6)]
        if w[5] != 0xC000 + i:
            bad += 1
        ins = c['insn'][0] + (' x%d' % len(c['insn']) if len(c['insn']) > 1 else '')
        print('  %-14s %-22s %s %3d  %s' % (c['name'], ins, ' '.join('%04X' % (x or 0) for x in w[:4]),
                                            ((w[4] or 0) - 1) & 0xffff, c['note']))
    rep.check('t13 result markers present', bad, 0)


def t14_settle(R, rep):
    cases = json.load(open(os.path.join(HERE, 't14_settle.json')))
    base = (SCRATCH_OFF + 0x100) * R.unit
    bad = 0
    print('  %-16s %-18s %s %3s' % ('case', 'instruction', '  '.join('r%d  ' % k for k in range(8)), 'cyc'))
    for i, c in enumerate(cases):
        w = [R.raw.get(base + (10 * i + k) * R.unit) for k in range(10)]
        if w[9] != 0xC000 + i:
            bad += 1
        print('  %-16s %-18s %s %3d  %s' % (c['name'], c['insn'][0], ' '.join('%04X' % (x or 0) for x in w[:8]),
                                            ((w[8] or 0) - 1) & 0xffff, c['note']))
    rep.check('t14 result markers present', bad, 0)


def t15_rest(R, rep):
    cases = json.load(open(os.path.join(HERE, 't15_rest.json')))
    base = (SCRATCH_OFF + 0x100) * R.unit
    bad = 0
    for i, c in enumerate(cases):
        n = len(c['rec'])
        w = [R.raw.get(base + (c['off'] + k) * R.unit) for k in range(n + 2)]
        if w[-1] != 0xC000 + i:
            bad += 1
        vals = '  '.join('%s=%04X' % (it, v if v is not None else 0) for it, v in zip(c['rec'], w[1:-1]))
        print('  %-22s cyc=%-3d %s' % (c['name'], ((w[0] or 0) - 1) & 0xffff, vals))
    rep.check('t15 result markers present', bad, 0)


def t16_flags(R, rep):
    m = json.load(open(os.path.join(HERE, 't16_flags.json')))
    base = (SCRATCH_OFF + 0x100) * R.unit
    per = 2 + len(m['br']) + len(m['cx'])
    print('  %-22s %-4s %-4s  %s | %s' % ('scenario', 'hw', 'shw', ' '.join('%-4s' % b for b in m['br']),
                                          ' '.join('%-4s' % c for c in m['cx'])))
    for k, (op, x, y) in enumerate(m['scen']):
        w = [R.raw.get(base + (k * per + j) * R.unit) or 0 for j in range(per)]
        print('  %-22s %04X %04X  %s | %s' % ('%s %04X,%04X' % (op, x, y), w[0], w[1],
                                              ' '.join('%-4d' % v for v in w[2:2 + len(m['br'])]),
                                              ' '.join('%-4d' % v for v in w[2 + len(m['br']):])))


def t17_circ(R, rep, jname='t17_circ.json'):
    vals = json.load(open(os.path.join(HERE, jname)))
    s = R.scratch & 0xffff
    print('  amode   readback  a0-beg a1-beg a2-beg a3-beg   (linear = 6)')
    for i, v in enumerate(vals):
        w = R.extra[5 * i:5 * i + 5]
        print('  %04X    %04X      %s' % (v, w[4], '  '.join('%5d' % ((w[k] - (s + 0x10 + 0x10 * k)) & 0xffff)
                                                          for k in range(4))))


def t17b_circ(R, rep):
    t17_circ(R, rep, 't17b_circ.json')


def t20_final(R, rep):
    cases = json.load(open(os.path.join(HERE, 't20_final.json')))
    base = (SCRATCH_OFF + 0x100) * R.unit
    bad = 0
    for i, c in enumerate(cases):
        n = 3 + len(c['rec'])
        w = [R.raw.get(base + (c['off'] + k) * R.unit) or 0 for k in range(n)]
        if w[-1] != 0xC000 + i:
            bad += 1
        extra = '  '.join('%s=%04X' % (it, v) for it, v in zip(c['rec'], w[2:-1]))
        print('  %-44s hw=%04X shw=%04X  %s' % (c['name'][:44], w[0], w[1], extra))
    rep.check('t20 result markers present', bad, 0)


# ------------------------------------------------- interrupts (gen_t21.py)
OUT_OFF = 0x400
T21_REGS = ['%ip0', '%ip1', '%psmode', '%ded', '%dei', '%ireq (read 3x, not written)']


def out_words(R, n, start=0):
    base = (OUT_OFF + start) * R.unit
    return [R.raw.get(base + k * R.unit) for k in range(n)]


def t21_ireq(R, rep):
    w = out_words(R, 547)
    for i, reg in enumerate(T21_REGS):
        o, f, z = w[3 * i:3 * i + 3]
        rep.info('%-7s orig / ffff / 0' % reg, '%s %s %s' % (h(o), h(f), h(z)))
    rep.info('timer0 after write 0200, ireq, timer0 after start', ' '.join(h(x) for x in w[18:21]))
    pairs = [(w[21 + 2 * k], w[22 + 2 * k]) for k in range(256)]
    # compress the timeline: print a sample whenever ireq changes or timer0 goes up
    print('  timer0 timeline (sample: timer0 ireq), ~35 cycles apart; shown where ireq changes or timer0 jumps up')
    prev = None
    for k, (t, q) in enumerate(pairs):
        if prev is None or q != prev[1] or t > prev[0] or k == 255:
            print('    #%3d  %s %s' % (k, h(t), h(q)))
        prev = (t, q)
    rep.info('poll: loop1 left, loop2 left, ireq, timer0', ' '.join(h(x) for x in w[533:537]))
    rep.info('clear: ireq, after w0, after w(orig), after w0', ' '.join(h(x) for x in w[537:541]))
    rep.info('timer1: ireq, timer1, loop1, loop2, ireq after clear', ' '.join(h(x) for x in w[541:546]))
    rep.check('t21 end marker', w[546], 0xE21D)


T22_CATCH = ['marker', 'r14 (low sled)', 'r13 (tail sled)', '%ireq', '%tpc', '%tpc.e lo', '%tpc.e hi',
             '%imask', '%smode', '%psmode', '%rpc', '%timer0', '%hwflag']


def t22_vec(R, rep):
    rb = out_words(R, 4)
    rep.info('read-backs after writes (imask/smode/ireq order of the variant)', ' '.join(h(x) for x in rb if x is not None))
    e = R.extra
    after = out_words(R, 5, 8)
    if after[0] in (0, 0xCA7C):     # reti variant: main ran on after the spin
        rep.info('after spin: catch ran, entries, %imask, %hwflag, %ireq',
                 '%s %d %s %s %s' % ('yes' if after[0] == 0xCA7C else 'no', after[1], h(after[2]), h(after[3]), h(after[4])))
        if after[0] == 0:
            return
        rep.info('last entry record (catch cleared %ireq, then reti)', '')
    if e[0] == 0x0BAD:
        rep.info('result', 'no interrupt taken (spin finished)')
        return
    if e[0] != 0xCA7C:
        rep.info('result', 'no catch record (%s)' % h(e[0]))
        return
    for k, name in enumerate(T22_CATCH):
        rep.info(name, h(e[k]))
    if e[1]:
        rep.info('ENTRY ADDRESS (low sled)', '%04X' % (0x7700 - e[1]))
    elif e[2]:
        rep.info('ENTRY ADDRESS (tail sled)', '%04X' % (0x7FFE - e[2]))
    else:
        rep.info('ENTRY ADDRESS', 'catch or main directly (r13 = r14 = 0)')


def t25_smode(R, rep):
    w = out_words(R, 4)
    if w[3] != 0xE25D:
        rep.info('%s' % R.name, 'stopped after writing the bit (OUT %s)' % ' '.join(h(x) for x in w[:3]))
        return
    rep.info('%s' % R.name, 'read-back %s, after 0 %s, %%psmode %s' % (h(w[0]), h(w[1]), h(w[2])))


def t26_ext(R, rep):
    w = out_words(R, 767)
    blocks = [('0x8000..0x80FF', [0x8000 + k for k in range(256)], w[0:256]),
              ('k*0x10000', [k * 0x10000 for k in range(1, 256)], w[256:511]),
              ('k*0x10000+0x8000', [k * 0x10000 + 0x8000 for k in range(1, 256)], w[511:766])]
    if w[766] != 0xE26D:
        rep.info('stopped', 'block %s, k %s' % (h(R.extra[0]), h(R.extra[1])))
    for name, addrs, vals in blocks:
        got = [(a, v) for a, v in zip(addrs, vals) if v not in (0, None, 0xA5A5)]
        unread = sum(1 for v in vals if v in (None, 0xA5A5))
        rep.info(name, '%d non-zero, %d not read' % (len(got), unread))
        for a, v in got[:24]:
            print('    %06X: %s' % (a, h(v)))
        if len(got) > 24:
            print('    ...')


T27_MB = [0x00, 0x02, 0x10, 0x12, 0x40, 0x20, 0x22, 0x24, 0x26, 0x10]
T27_MBNAME = {0x00: 'MAILBOX_ID lo', 0x02: 'MAILBOX_ID hi', 0x10: 'H2C_STA lo', 0x12: 'H2C_STA hi',
              0x40: 'C2H_STA lo', 0x20: 'H2C0 lo', 0x22: 'H2C0 hi', 0x24: 'H2C0b lo (read clears)',
              0x26: 'H2C0b hi'}
T27_DM = [0x700, 0x702, 0x704, 0x706, 0x701]


def t27_ext(R, rep):
    w = out_words(R, 4 + 2 * len(T27_MB) + 2 + 2 * len(T27_DM) + 3)
    rep.info('progress (step)', h(R.extra[0]))
    rep.info('ldx r1,a1 with n0=1, n1=2', '%s (%s)' % (h(w[0]), {0xB001: 'n0', 0xB002: 'n1'}.get(w[0], '?')))
    rep.info('stx r2,a0: SCR+0x11 / SCR+0x12', '%s %s (%s)' % (h(w[1]), h(w[2]),
             'n1' if w[2] == 0x1234 else 'n0' if w[1] == 0x1234 else '?'))
    rep.info('EPR[4] read back (wrote 0180)', h(w[3]))
    k = 4
    for off in T27_MB:
        rep.info('0xD88000+%02X %s' % (off, T27_MBNAME[off]), '%s %s' % (h(w[k]), h(w[k + 1])))
        k += 2
    rep.info('C2H_STA after DSP writes', '%s %s' % (h(w[k]), h(w[k + 1])))
    k += 2
    for off in T27_DM:
        rep.info('0xD00000+%03X (DMEM via bus; SCR = B000..)' % off, '%s %s' % (h(w[k]), h(w[k + 1])))
        k += 2
    rep.info('ldd at 0xD00700', '%s %s' % (h(w[k]), h(w[k + 1])))
    rep.check('t27 end marker', w[k + 2], 0xE27D)


T28_NAMES = {('mb', 0x00): 'MAILBOX_ID lo', ('mb', 0x01): 'MAILBOX_ID hi', ('mb', 0x08): 'H2C_STA',
             ('mb', 0x10): 'H2C0 lo', ('mb', 0x11): 'H2C0 hi', ('mb', 0x13): 'H2C0b hi',
             ('mb', 0x12): 'H2C0b lo (read clears?)', ('mb', 0x20): 'C2H_STA',
             ('dm', 0x380): 'DMEM 0x380 (B000)', ('dm', 0x381): 'DMEM 0x381 (B001)', ('dm', 0x38F): 'DMEM 0x38F (B00F)'}
T28_READS = [('mb', 0x00), ('mb', 0x01), ('mb', 0x08), ('mb', 0x10), ('mb', 0x11),
             ('mb', 0x13), ('mb', 0x12), ('mb', 0x08), ('mb', 0x20),
             ('dm', 0x380), ('dm', 0x381), ('dm', 0x38F)]


def t28_mbox(R, rep):
    w = out_words(R, 2 + 2 * len(T28_READS) + 3)
    rep.info('progress (step)', h(R.extra[0]))
    rep.info('EPR[2], EPR[0] read back (wrote 00C0, 0180)', '%s %s' % (h(w[0]), h(w[1])))
    k = 2
    for key in T28_READS:
        base = 0xD44000 if key[0] == 'mb' else 0xD00000
        rep.info('%06X %s' % (base + key[1], T28_NAMES[key]), '%s %s' % (h(w[k]), h(w[k + 1])))
        k += 2
    rep.info('C2H_STA after DSP writes', '%s %s' % (h(w[k]), h(w[k + 1])))
    rep.check('t28 end marker', w[k + 2], 0xE28D)


T29_OPS = [('ld', 'mb', 0x00, 'MAILBOX_ID (16-bit)'), ('ldd', 'mb', 0x00, 'MAILBOX_ID'),
           ('ldd', 'mb', 0x08, 'H2C_STA'), ('ldd', 'mb', 0x10, 'H2C0'),
           ('ldd', 'mb', 0x12, 'H2C0b (read clears)'), ('ldd', 'mb', 0x08, 'H2C_STA after'),
           ('ldd', 'mb', 0x20, 'C2H_STA'),
           ('ld', 'dm', 0x380, 'DMEM 0x380 (B000, 16-bit)'), ('ldd', 'dm', 0x380, 'DMEM 0x380 (B000 B001)'),
           ('ldd', 'dm', 0x38E, 'DMEM 0x38E (B00E B00F)')]


def t29_mbox(R, rep):
    w = out_words(R, 2 + 2 * len(T29_OPS) + 3)
    rep.info('progress (step)', h(R.extra[0]))
    rep.info('EPR[2], EPR[0] read back (wrote 00C0, 0180)', '%s %s' % (h(w[0]), h(w[1])))
    k = 2
    for op, kind, off, name in T29_OPS:
        base = 0xD44000 if kind == 'mb' else 0xD00000
        rep.info('%-3s %06X %s' % (op, base + off, name), '%s %s' % (h(w[k]), h(w[k + 1])))
        k += 2
    rep.info('C2H_STA (ldd) after C2H1/C2H1b writes', '%s %s' % (h(w[k]), h(w[k + 1])))
    rep.check('t29 end marker', w[k + 2], 0xE29D)


def t30_bustime(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T30
    rep.info('progress (case)', h(R.extra[0]))
    w = out_words(R, 3 * len(T30) + 1)
    base = w[0]
    print('  %-16s %6s %8s %7s  %-5s %s' % ('case', 'cycles', 'per acc', 'ns/acc', 'r2', 'note'))
    for i, (name, body, note) in enumerate(T30):
        cyc, r2, tag = w[3 * i:3 * i + 3]
        if tag != 0xC000 + i:
            print('  %-16s not reached' % name)
            continue
        net = (cyc - base) & 0xffff if i else cyc
        n = sum(1 for b in body if b.split()[0] in ('ld', 'ldd', 'st', 'std')) or 1
        print('  %-16s %6d %8.2f %7.1f  %s  %s' % (name, net, net / n, net / n * 1e9 / R.dsp_hz, h(r2), note))
    rep.check('t30 end marker', w[3 * len(T30)], 0xE30D)


def t31_bususe(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T31
    exp = [e for _, _, _, e in T31] + [0xB00A, 0xB00C]
    names = [n for n, _, _, _ in T31] + ['2 loads: 1st (38A)', '2 loads: 2nd (38C)']
    rep.info('progress (case)', h(R.extra[0]))
    w = out_words(R, 4 * len(exp) + 1)
    print('  %-20s %-6s %-6s %-6s %-6s' % ('case', 'expect', 'mov', '8 nops', 'stored'))
    for i, (n, e) in enumerate(zip(names, exp)):
        a, b, c, tag = w[4 * i:4 * i + 4]
        if tag != 0xC000 + i:
            print('  %-20s not reached' % n)
            continue
        print('  %-20s %s   %s   %s   %s' % (n, h(e), h(a), h(b), h(c)))
    rep.check('t31 end marker', w[4 * len(exp)], 0xE31D)


def t32_repro(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T30, T32
    v = R.name[-1]
    title, idx, timer, trace = T32[v]
    rep.info('scenario', title)
    rep.info('progress (case)', h(R.extra[0]))
    w = out_words(R, 3 * len(idx))
    want = {6: 0xB001, 7: 0xB003, 10: 0x1357, 11: 0xB000}
    for k, i in enumerate(idx):
        cyc, r2, tag = w[3 * k:3 * k + 3]
        if tag != 0xC000 + i:
            print('    case %d %-16s not reached' % (i, T30[i][0]))
            continue
        flag = ''
        if i in want:
            flag = 'OK' if r2 == want[i] else 'WRONG (expect %s)' % h(want[i])
        print('    case %d %-16s cycles %5d  r2 %s  %s' % (i, T30[i][0], cyc, h(r2), flag))
    if trace:
        def dm(off):
            return 0xB000 + off - 0x380 if 0x380 <= off < 0x390 else 0xA5A5
        exp = []
        for i in idx:
            for b in T30[i][1]:
                op = b.split()[0]
                if op in ('ld', 'ldd') and 'a2,' in b:
                    off = int(b.split(',')[-1], 16)
                    exp += [(i, b, dm(off))] + ([(i, b + ' (hi)', dm(off + 1))] if op == 'ldd' else [])
        tr = out_words(R, len(exp), 0x100)
        bad = [(e, g) for e, g in zip(exp, tr) if g != e[2]]
        rep.info('trace', '%d loads, %d wrong' % (len(exp), len(bad)))
        for (i, b, e), g in bad[:30]:
            print('    case %d  %-20s got %s expect %s' % (i, b, h(g) if g is not None else 'none', h(e)))


T33_EXPECT = [  # (byte offset, halfwords, value pattern)
    (0x000, 16, [0x1111]), (0x040, 1, [0x1111]), (0x042, 31, [0xEEEE]),
    (0x080, 32, [0x3333, 0x2222]), (0x100, 2, [0x3333, 0x2222]), (0x104, 30, [0xEEEE]),
    (0x140, 64, [0x4444]), (0x200, 128, [0x3333, 0x2222]),
    (0x300, 1, [0x1357]), (0x310, 2, [0x3333, 0x2222]),
    (0x320, 16, [0x1111]), (0x360, 16, [0x1111]), (0x380, 64, [0xEEEE]),
]


def t33_sdstore(R, rep, rdir):
    sys.path.insert(0, HERE)
    from gen_t21 import T33
    rep.info('progress (case)', h(R.extra[0]))
    w = out_words(R, 1 + 3 * len(T33) + 1)
    rep.info('EPR read back', h(w[0]))
    want = {7: 0x1357, 8: 0x2222, 9: 0x1111, 10: 0x1111}
    print('  %-14s %6s %8s %7s  %-5s %s' % ('case', 'cycles', 'per st', 'ns/st', 'r2', 'note'))
    base = w[1]
    for i, (name, body, note) in enumerate(T33):
        cyc, r2, tag = w[1 + 3 * i:4 + 3 * i]
        if tag != 0xC000 + i:
            print('  %-14s not reached' % name)
            continue
        net = (cyc - base) & 0xffff if i else cyc
        n = sum(1 for b in body if b.split()[0] in ('st', 'std')) or 1
        flag = ('OK' if r2 == want[i] else 'WRONG (expect %s)' % h(want[i])) if i in want else ''
        print('  %-14s %6d %8.2f %7.1f  %s  %s %s' % (name, net, net / n, net / n * 1e9 / R.dsp_hz, h(r2), note, flag))
    rep.check('t33 end marker', w[1 + 3 * len(T33)], 0xE33D)
    path = os.path.join(rdir, 't33_sdram.txt')
    if not os.path.exists(path):
        rep.info('SDRAM dump', 'missing')
        return
    mem = {}
    lines = open(path).read().splitlines()
    rep.info('SDRAM block', lines[0])
    for ln in lines[1:]:
        off, vals = ln.split(':')
        for k, v in enumerate(vals.split()):
            mem[int(off, 16) + 2 * k] = int(v, 16)
    bad = 0
    for off, n, pat in T33_EXPECT:
        for k in range(n):
            got, exp = mem.get(off + 2 * k), pat[k % len(pat)]
            if got != exp:
                bad += 1
                if bad <= 20:
                    print('    SDRAM +%03X: %s, expect %s' % (off + 2 * k, h(got) if got is not None else 'none', h(exp)))
    if all(v == 0xA5A5 for v in mem.values()):
        # hwstub stages USB transfers in its buffer, so the block is overwritten
        # by the result collection after the run; t34_sdbuf checks the data
        # from the DSP side instead.
        rep.info('SDRAM block', 'overwritten by hwstub transfers after the run (see t34_sdbuf)')
    else:
        rep.check('SDRAM contents match the stores', bad, 0)


def t34_sdbuf(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T34
    rep.info('progress (case)', h(R.extra[0]))
    w = out_words(R, 1 + 5 * len(T34) + 1)
    rep.info('EPR read back', h(w[0]))
    print('  %-9s %7s %8s %7s %8s  %-5s %-5s %s' % ('case', 'stores', 'per st', 'drain', 'total/st', 'first', 'last', 'check'))
    for i, (op, n) in enumerate(T34):
        st, dr, first, last, tag = w[1 + 5 * i:6 + 5 * i]
        if tag != 0xC000 + i:
            print('  %2d x %-4s not reached' % (n, op))
            continue
        st = (st - 1) & 0xffff               # minus the timer read
        ef, el = (0x1000 + i, 0x1000 + i) if op == 'st' else (0x2000 + i, 0x3000 + i)
        ok = 'OK' if (first, last) == (ef, el) else 'WRONG (expect %s %s)' % (h(ef), h(el))
        print('  %2d x %-4s %7d %8.2f %7d %8.1f  %s  %s  %s' % (n, op, st, st / n, dr, (st + dr) / n, h(first), h(last), ok))
    rep.check('t34 end marker', w[1 + 5 * len(T34)], 0xE34D)


def t35_dma(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T35
    rep.info('progress (case)', h(R.extra[0]))
    w = out_words(R, 14 * len(T35) + 1)
    print('  %-5s %-7s %8s %7s %6s  %-4s %-4s %-4s %-4s %-4s %-9s %4s  %-4s %-4s %s' % (
        'words', 'cycles', 'cyc/wd', 'MB/s', 'poll', 'Raw', 'Err', 'ChEn', 'Cfg', 'BlkT', 'DAR', 'bad', 'd[0]', 'd[-1]', 'note'))
    for i, (words, ctl, bts, note) in enumerate(T35):
        cyc, l0, l1, raw, err, chen, cfg, blk, dlo, dhi, bad, d0, dl, tag = w[14 * i:14 * i + 14]
        if tag != 0xC000 + i:
            print('  %-5d not reached' % words)
            continue
        gave_up = l0 == 0xFFFF and l1 == 0xFFFF     # both agn counters exhausted
        cyc = (cyc - 1) & 0xffff
        over = cyc >= 0xFFF0 or gave_up
        print('  %-5d %7s %8s %7s %6s  %s %s %s %s %s %04X%04X %4d  %s %s %s' % (
            words, '>65535' if over else cyc, '' if over else '%.2f' % (cyc / words),
            '' if over else '%.1f' % (words * 4 / (cyc / R.dsp_hz) / 1e6),
            'GAVEUP' if gave_up else '%X/%X' % (l1, l0), h(raw), h(err), h(chen), h(cfg), h(blk),
            dhi, dlo, bad, h(d0), h(dl), note))
    rep.check('t35 end marker', w[14 * len(T35)], 0xE35D)


def t36_dma2(R, rep, rdir):
    sys.path.insert(0, HERE)
    from gen_t21 import T36
    rep.info('progress (case)', h(R.extra[0]))
    w = out_words(R, 8 * len(T36) + 1)
    print('  %-48s %7s %7s %6s %6s  %-4s %-4s %4s' % ('case', 'total', 'bus ops', 'cyc/wd', 'MB/s', 'Raw', 'ChEn', 'bad'))
    for i, (kind, words, ctl, note) in enumerate(T36):
        tot, bus, l1, l0, raw, chen, bad, tag = w[8 * i:8 * i + 8]
        if tag != 0xC000 + i:
            print('  %-48s not reached' % note)
            continue
        if kind in ('ld', 'st'):
            print('  %-48s %7s %7d %6.1f' % (note, '', bus, bus / words))
            continue
        gave_up = l0 == 0xFFFF and l1 == 0xFFFF
        tot = (tot - 1) & 0xffff
        print('  %-48s %7s %7s %6s %6s  %s %s %4d' % (
            note, 'GAVEUP' if gave_up else tot, bus if bus else '',
            '' if gave_up else '%.2f' % (tot / words),
            '' if gave_up else '%.1f' % (words * 4 / (tot / R.dsp_hz) / 1e6), h(raw), h(chen), bad))
    rep.check('t36 end marker', w[8 * len(T36)], 0xE36D)
    path = os.path.join(rdir, 't36_sdram.txt')
    if os.path.exists(path):
        lines = open(path).read().splitlines()
        rep.info('block B (ARM view)', lines[0])
        mem = []
        for ln in lines[1:]:
            mem += [int(x, 16) for x in ln.split(':')[1].split()]
        # words 0..63 were overwritten by the '+st' cases (5A5A A5A5); the rest
        # still hold the last DMEM -> SDRAM copy (halfword k = k)
        bad = sum(1 for k in range(128, 2048) if mem[k] != k)
        st = sum(1 for k in range(128) if mem[k] != (0x5A5A if k % 2 == 0 else 0xA5A5))
        rep.info('B halfwords 128..2047 = k (DMA)', '%d wrong' % bad)
        rep.info('B halfwords 0..127 = 5A5A/A5A5 (std)', '%d wrong' % st)


def t37_dmairq(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T37
    rep.info('progress (case)', h(R.extra[0]))
    if R.extra[0x3C] == 0x5B00:
        rep.info('SPURIOUS interrupt', '%%ireq %s %%tpc %s' % (h(R.extra[0x3D]), h(R.extra[0x3E])))
    w = out_words(R, 9 * len(T37) + 1)
    for i, words in enumerate(T37):
        ent, cyc, ireq, tpc, last, raw, bad, imask, tag = w[9 * i:9 * i + 9]
        if tag != 0xC000 + i:
            rep.info('%d words' % words, 'not reached')
            continue
        rep.info('%d words' % words, 'entries %d, start -> handler %d cycles, %%ireq at entry %s, %%tpc %s, '
                 'dest[last] at entry %s (expect %s), RawTfr after %s, bad %d, %%imask after %s' % (
                     ent, cyc, h(ireq), h(tpc), h(last), h(2 * words - 1), h(raw), bad, h(imask)))
    rep.check('t37 end marker', w[9 * len(T37)], 0xE37D)


def t38_armirq(R, rep, rdir):
    rep.info('phase reached', h(R.extra[1]))
    w = out_words(R, 6)
    rep.info('CHIPCFG read by the DSP', '%04X%04X' % (w[1], w[0]))
    rep.info('high half written (0 = skipped)', h(w[2]))
    rep.info('CHIPCFG read back', '%04X%04X' % (w[4], w[3]))
    rep.check('t38 end marker', w[5], 0xE38D)
    path = os.path.join(rdir, 't38_arm.txt')
    if os.path.exists(path):
        for ln in open(path).read().splitlines():
            print('    ' + ln)


def t39_circ(R, rep):
    n = R.raw.get((OUT_OFF + 0x3F0) * R.unit)
    rep.info('progress (index)', h(R.extra[0]))
    rep.check('t39 end marker', R.raw.get((OUT_OFF + 0x3F1) * R.unit), 0xE39D)
    if n is None or n == 0xA5A5:
        return
    rep.info('%amode values with wrapping', '%d' % n)
    w = out_words(R, 3 * min(n, 300))
    for k in range(min(n, 300)):
        v, m1, m2 = w[3 * k:3 * k + 3]
        if k < 60:
            print('    %%amode %s: ldu wraps %s, ldxu wraps %s' % (h(v), bin(m1)[2:].zfill(7), bin(m2)[2:].zfill(7)))


def t40_circ2(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T40_V
    rep.check('t40 end marker', R.raw.get(OUT_OFF * R.unit), 0xE40D)
    base = 0x800 * R.unit
    k = 0
    for v in T40_V:
        odd = []
        for r in range(8):
            for j in range(4):
                x = R.raw.get(base + k * R.unit)
                k += 1
                if x != 6:
                    odd.append('a%d/cb%d=%s' % (r, j, 'none' if x is None else x - 0x10000 if x & 0x8000 else x))
        print('    %%amode %04X: %s' % (v, ', '.join(odd) if odd else 'all linear (+6)'))


def t41_circ3(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T41_V, T41_REGS, T41_LEN
    rep.check('t41 end marker', R.raw.get(OUT_OFF * R.unit), 0xE41D)
    base = 0x800 * R.unit
    k = 0
    for v in T41_V:
        print('  %%amode %04X' % v)
        for r in T41_REGS:
            for j in range(4):
                tr = []
                for _ in range(11):
                    x = R.raw.get(base + k * R.unit)
                    k += 1
                    tr.append('?' if x is None else str(x - 0x10000 if x & 0x8000 else x))
                print('    a%d from cb%d (len %d): %s' % (r, j, T41_LEN[j], ' '.join(tr)))


def t42_brev(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T42
    rep.check('t42 end marker', R.raw.get(OUT_OFF * R.unit), 0xE42D)
    base = 0x800 * R.unit
    k = 0
    for r, w, extra in T42:
        tr = []
        for _ in range(16):
            x = R.raw.get(base + k * R.unit)
            k += 1
            tr.append(x)
        # expected: bit-reversed counter of width w, i.e. rev_w(i+1)
        exp = [int(bin((i + 1) % (1 << w))[2:].zfill(w)[::-1], 2) if w else 0 for i in range(16)]
        ok = 'matches w-bit reversal' if tr == exp else 'DIFFERS'
        print('    a%d %%amode %04X (w=%d): %s  [%s]' % (r, 1 | w << 2 | extra, w,
              ' '.join('?' if x is None else str(x) for x in tr), ok))


def t43_circ4(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T43_V, T43_MODES
    rep.check('t43 end marker', R.raw.get(OUT_OFF * R.unit), 0xE43D)
    base = 0x800 * R.unit
    k = 0
    found = 0
    for v in T43_V:
        odd = []
        for r in range(8):
            for name, op, st0, lin in T43_MODES:
                x = R.raw.get(base + k * R.unit)
                k += 1
                x = None if x is None else (x - 0x10000 if x & 0x8000 else x)
                if x != lin:
                    odd.append('a%d %s: %s (linear %d)' % (r, name, x, lin))
        found += len(odd)
        print('    %%amode %04X: %s' % (v, '; '.join(odd) if odd else 'all linear'))
    rep.info('non-linear results', found)


def t44_brevst(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T44
    rep.check('t44 end marker', R.raw.get(OUT_OFF * R.unit), 0xE44D)

    def rev(x, w):
        return int(bin(x % (1 << w))[2:].zfill(w)[::-1], 2)
    base = 0x800 * R.unit
    k = 0
    for c, (name, r, w, dest, n, copy) in enumerate(T44):
        tr = [R.raw.get(base + (k + i) * R.unit) for i in range(n)]
        k += n
        exp_tr = [rev(i + 1, w) for i in range(n)]
        # store i goes to dest + rev(i) (the address before its post-modify)
        val = [(0x200 if copy else 0x100 + 0x10 * c) + i for i in range(n)]
        mem = [R.raw.get((dest + j) * R.unit) for j in range(1 << w)]
        exp_mem = [None] * (1 << w)
        for i in range(n):
            exp_mem[rev(i, w)] = val[i]
        rep.check('%s: address trace' % name, tuple(tr) == tuple(exp_tr), True)
        rep.check('%s: memory in bit-reversed order' % name, mem == exp_mem, True)
        print('      trace  %s' % ' '.join(str(x) for x in tr))
        print('      memory %s' % ' '.join('?' if x is None else '%X' % x for x in mem))


def t45_reti_smode(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T45
    if R.extra[0x3C] == 0x5B00:
        rep.info('SPURIOUS interrupt', '%%ireq %s %%tpc %s' % (h(R.extra[0x3D]), h(R.extra[0x3E])))
    w = out_words(R, 6 * len(T45) + 1)
    print('  %-42s %-8s %-8s | %-8s %-8s %-6s' % ('case', 'smode@in', 'psmode@in', 'smode', 'psmode', 'imask'))
    for i, (name, sm, act) in enumerate(T45):
        a, b, c, d, e, tag = w[6 * i:6 * i + 6]
        if tag != 0xC000 + i:
            print('  %-42s not reached' % name)
            continue
        print('  %-42s %-8s %-9s | %-8s %-8s %s   (smode before: %s)' % (name, h(a), h(b), h(c), h(d), h(e), h(sm)))
    rep.check('t45 end marker', w[6 * len(T45)], 0xE45D)


def t46_prio(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T46
    rep.info('progress (case)', h(R.extra[0]))
    if R.extra[0x3C] == 0x5B00:
        rep.info('SPURIOUS interrupt', '%%ireq %s %%tpc %s' % (h(R.extra[0x3D]), h(R.extra[0x3E])))
    out = OUT_OFF * R.unit
    rep.check('t46 end marker', R.raw.get((OUT_OFF) * R.unit), 0xE46D)
    base = 0x800 * R.unit
    default = None
    for i, (ip0, ip1) in enumerate(T46):
        w = [R.raw.get(base + (16 * i + k) * R.unit) for k in range(16)]
        n = w[0] if w[0] is not None and w[0] <= 15 else 0
        order = w[1:1 + n]
        if i == 0:
            default = order
        mark = '' if i == 0 else ('' if order == default else '  <-- differs')
        print('    ip0 %04X ip1 %04X: %2d entries: %s%s' % (ip0, ip1, n, ' '.join(str(x) for x in order), mark))


def t47_prio2(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T47, ip_regs
    rep.info('progress (case)', h(R.extra[0]))
    if R.extra[0x3C] == 0x5B00:
        rep.info('SPURIOUS interrupt', '%%ireq %s %%tpc %s' % (h(R.extra[0x3D]), h(R.extra[0x3E])))
    rep.check('t47 end marker', R.raw.get(OUT_OFF * R.unit), 0xE47D)
    base = 0x800 * R.unit
    for i, (name, lv, top, mask, late) in enumerate(T47):
        w = [R.raw.get(base + (16 * i + k) * R.unit) for k in range(16)]
        n = w[0] if w[0] is not None and w[0] <= 14 else 0
        ip0, ip1 = ip_regs(lv, top)
        print('    %-44s ip0 %04X ip1 %04X: %2d: %s   (%%ireq left %s)' % (
            name, ip0, ip1, n, ' '.join(str(x) for x in w[1:1 + n]), h(w[15]) if w[15] is not None else '?'))


def t48_src14(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T48
    rep.info('progress (bit)', h(R.extra[0]))
    rep.check('t48 end marker', R.raw.get(OUT_OFF * R.unit), 0xE48D)
    base = 0x800 * R.unit
    for i, (reg, b) in enumerate(T48):
        ireq, back, tag = [R.raw.get(base + (3 * i + k) * R.unit) for k in range(3)]
        if tag != 0xC000 + i:
            print('    %s bit %2d: not reached' % (reg, b))
            continue
        flag = '  <-- %ireq bit 14' if ireq is not None and ireq & 0x4000 else ''
        print('    %s bit %2d: %%ireq %s, read back %s%s' % (reg, b, h(ireq), h(back), flag))


def t49_stdshort(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T49
    rep.check('t49 end marker', R.raw.get(OUT_OFF * R.unit), 0xE49D)
    for base, c, r, lo, hi in T49:
        addr = (0xA00 if base == 6 else 0xA40) + c
        got = (R.raw.get(addr * R.unit), R.raw.get((addr + 1) * R.unit))
        rep.check('std r%d,a%d,%d -> [%03X] lo, [%03X] hi' % (r, base, c, addr, addr + 1), int(got == (lo, hi)), 1)


def t50_nest(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T50
    rep.info('progress (case)', h(R.extra[0]))
    if R.extra[0x3C] == 0x5B00:
        rep.info('SPURIOUS interrupt', '%%ireq %s %%tpc %s' % (h(R.extra[0x3D]), h(R.extra[0x3E])))
    rep.check('t50 end marker', R.raw.get(OUT_OFF * R.unit), 0xE50D)
    names = {1: 'outer entry', 2: 'INNER entry', 3: 'outer after raise', 4: 'main after'}
    base = 0x800 * R.unit
    for i, (name, src) in enumerate(T50):
        print('    %s' % name)
        for k in range(5):
            eid, im, tpc = [R.raw.get(base + (16 * i + 3 * k + j) * R.unit) for j in range(3)]
            if eid not in names:
                break
            print('      %-18s %%imask %s %%tpc %s' % (names[eid], h(im), h(tpc)))


def t51_circ5(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T51
    rep.check('t51 end marker', R.raw.get(OUT_OFF * R.unit), 0xE51D)
    base = 0x800 * R.unit
    k = 0
    for reg, v in T51:
        odd = []
        for r in range(8):
            x = R.raw.get(base + k * R.unit)
            k += 1
            if x != 6:
                odd.append('a%d=%s' % (r, x))
        print('    %s %04X: %s' % (reg, v, ', '.join(odd) if odd else 'all linear'))


def t52_cexe(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T52
    rep.info('progress (case)', h(R.extra[0]))
    if R.extra[0x3C] == 0x5B00:
        rep.info('SPURIOUS interrupt', '%%ireq %s %%tpc %s' % (h(R.extra[0x3D]), h(R.extra[0x3E])))
    rep.check('t52 end marker', R.raw.get(OUT_OFF * R.unit), 0xE52D)
    base = 0x800 * R.unit
    for i, ((fname, op), m, sc) in enumerate(T52):
        w = [R.raw.get(base + (8 * i + k) * R.unit) for k in range(8)]
        if w[7] != 0xC000 + i:
            print('    %-6s %-2s %s: not reached' % (fname, m, sc))
            continue
        if sc == 'A':
            print('    %-6s %-2s A interrupt in shadow: %%tpc = cexe%+d, r5 at entry %d, r5 at end %d, %%imask at entry %s, entries %d' % (
                fname, m, w[0] - 0x10000 if w[0] & 0x8000 else w[0], w[1], w[2], h(w[3]), w[4]))
        else:
            print('    %-6s %-2s %s %s: %%hwflag %s %%shwflag %s r5 %d' % (
                fname, m, sc, 'cond false' if sc == 'B' else 'cond true ', h(w[0]), h(w[1]), w[2]))


def t53_vxor(rdir, rep):
    names = {v: os.path.join(rdir, 't53_vxor_%s.txt' % v) for v in 'abc'}
    if not all(os.path.exists(p) for p in names.values()):
        rep.info('t53', 'needs t53_vxor_a, _b and _c')
        return
    R = {v: Result(p) for v, p in names.items()}
    for v in 'abc':
        rep.info('t53_vxor_%s' % v, 'done' if R[v].done else 'TIMEOUT')
    labels = ['r%d' % k for k in range(16)] + ['a%d.%d' % (k // 2, k % 2) for k in range(14)] + \
        ['n%d' % k for k in range(8)] + ['%%%s' % c for c in CREGS] + ['g1:g0', 'g3:g2', 'g5:g4', 'g7:g6']
    extra = {0x60: '%vitr after vxor', 0x61: '%vitr after vit_a', 0x62: '%vitr after vit_b'}
    print('    %-18s %-6s %-6s %-6s' % ('', 'vxor', 'vxor.e', 'nop'))
    diff = 0
    for k, lab in enumerate(labels):
        vals = [R[v].w.get(k) for v in 'abc']
        mark = '  <--' if len(set(vals)) > 1 else ''
        diff += bool(mark)
        if mark or lab in ('r2', 'r3', 'r4', 'r5', 'r6', '%vitr', '%hwflag'):
            print('    %-18s %s %s %s%s' % (lab, *[h(x) if x is not None else '----' for x in vals], mark))
    for off, lab in extra.items():
        vals = [R[v].w.get(off) for v in 'abc']
        mark = '  <--' if len(set(vals)) > 1 else ''
        diff += bool(mark)
        print('    %-18s %s %s %s%s' % (lab, *[h(x) if x is not None else '----' for x in vals], mark))
    raw = sum(1 for o in R['a'].raw if not (0x200 <= o < 0x400) and
              (R['a'].raw.get(o), R['b'].raw.get(o)) != (R['c'].raw.get(o), R['c'].raw.get(o)))
    rep.info('register/flag differences between the three images', diff)
    rep.info('DMEM differences outside the result block', raw)


def t54_smode(R, rep):
    e = R.extra
    names = {5: 'timer0 (source 5)', 15: 'NMI'}
    if e[0] not in names:
        rep.info('%s' % R.name, 'never woke up (EXTRA+0 = %s), after-write marker %s, spin-end marker %s' % (
            h(e[0]), h(e[8]), h(e[9])))
        return
    rep.info('%s woke by' % R.name, names[e[0]])
    rep.info('  %smode write at, %tpc at entry', '%s, %s (%+d)' % (h(e[7]), h(e[1]), ((e[1] - e[7]) + 0x8000) % 0x10000 - 0x8000))
    rep.info('  cycles from write to entry (timer1)', '%d' % ((e[6] - e[2]) & 0xffff))
    rep.info('  %smode / %psmode / %imask at entry', '%s / %s / %s' % (h(e[3]), h(e[4]), h(e[5])))
    rep.info('  instruction after the write ran', 'yes' if e[8] == 0x600D else 'no (%s)' % h(e[8]))


def t55_hwflag2(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T55
    rep.check('t55 end marker', R.raw.get(OUT_OFF * R.unit), 0xE55D)
    names = {2: 'b2', 3: 'Z', 4: 'GT', 5: 'GE', 6: 'C', 8: 'SV', 10: 'V'}

    def fl(x):
        if x is None:
            return '----'
        bits = [names.get(b, 'b%d' % b) for b in range(16) if x >> b & 1]
        return '%s %s%s' % (h(x), ' '.join(bits), '   <-- bit 2' if x & 4 else '')
    g = lambda a: R.raw.get(a * R.unit)
    for i, (name, setup, body) in enumerate(T55):
        print('    %-32s %s' % (name, fl(g(0x800 + i))))
        if name.startswith('inside agn'):
            print('    %-32s %s' % ('  (last pass inside the loop)', fl(g(0x840 + i))))
    n = len(T55)
    print('    %-32s %s' % ('handler entry, straight code', fl(g(0x880))))
    print('    %-32s %s' % ('after reti, straight code', fl(g(0x800 + n + 0x20))))
    print('    %-32s %s' % ('handler entry, inside agn loop', fl(g(0x881))))
    print('    %-32s %s' % ('after the loop', fl(g(0x800 + n + 0x21))))


def t56_hwflag2b(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T56
    rep.check('t56 end marker', R.raw.get(OUT_OFF * R.unit), 0xE56D)
    for i, (name, setup, body) in enumerate(T56):
        x = R.raw.get((0x800 + i) * R.unit)
        print('    %-36s %s%s' % (name, h(x) if x is not None else '----', '   <-- bit 2' if x is not None and x & 4 else ''))


def t57_hwflag2c(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T57
    rep.check('t57 end marker', R.raw.get(OUT_OFF * R.unit), 0xE57D)
    for i, (name, body) in enumerate(T57):
        x = R.raw.get((0x800 + i) * R.unit)
        print('    %-36s %s%s' % (name, h(x) if x is not None else '----', '   <-- bit 2' if x is not None and x & 4 else ''))


def t58_hwflag2d(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T58
    rep.check('t58 end marker', R.raw.get(OUT_OFF * R.unit), 0xE58D)

    def show(name, x):
        print('    %-36s %s%s' % (name, h(x) if x is not None else '----', '   <-- bit 2' if x is not None and x & 4 else ''))
    for i, (name, setup, body) in enumerate(T58):
        show(name, R.raw.get((0x800 + i) * R.unit))
    show('software interrupt entry', R.raw.get(0x880 * R.unit))
    show('NMI entry', R.raw.get(0x881 * R.unit))
    rep.info('NMI arrived during the spin', 'yes' if R.raw.get(0x882 * R.unit) == 1 else 'no')


def t59_bisect(R, rep):
    g = lambda a: R.raw.get(a * R.unit)
    rep.info('final %hwflag (dump)', h(R.creg['hwflag']))
    names = ['after prologue', 'after markers', 'before phase 1 loop', 'after phase 1',
             'before phase 2 loop', 'after phase 2', 'after end marker']
    for k, n in enumerate(names):
        x = R.w.get(0x70 + k)
        print('    %-22s %s%s' % (n, h(x) if x is not None else '----', '   <-- bit 2' if x is not None and x & 4 else ''))
    for ph, (n, step, log) in enumerate([(64, 0x800, 0x900), (512, 0x8000, 0x980)], 1):
        first = None
        for k in range(n):
            a, b = g(log + 2 * k), g(log + 2 * k + 1)
            if first is None and ((a or 0) & 4 or (b or 0) & 4):
                first = (k, a, b)
        if first:
            k, a, b = first
            print('    phase %d: bit 2 first at iteration %d (address %06X): after ld %s, after add %s' % (
                ph, k, 0x7F0 + k * step, h(a), h(b)))
            for j in range(max(0, k - 2), min(n, k + 3)):
                print('      iter %3d addr %06X: after ld %s, after add %s' % (
                    j, 0x7F0 + j * step, h(g(log + 2 * j)), h(g(log + 2 * j + 1))))
        else:
            print('    phase %d: bit 2 never set (%s ... %s)' % (ph, h(g(log)), h(g(log + 2 * n - 1))))


def t60_hwflag2e(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T60
    g = lambda a: R.raw.get(a * R.unit)
    rep.check('t60 end marker', R.raw.get(OUT_OFF * R.unit), 0xE60D)

    def pair(a):
        i, f = g(a), g(a + 1)
        return '%%ireq %s %%hwflag %s%s' % (h(i), h(f), '  <-- bit 2' if f is not None and f & 4 else '')
    k = 0x800
    for name, v in T60:
        if v == 0x8000:
            continue
        print('    %-36s set:   %s' % (name, pair(k)))
        print('    %-36s clear: %s' % ('', pair(k + 2)))
        k += 4
    print('    USB request, pass before:            %s' % pair(0x840))
    print('    USB request, pass it appeared:       %s' % pair(0x842))
    print('    USB wait loops left (64 x 64K)        %s' % h(g(0x844)))
    print('    timer0 ran out (masked):             %s' % pair(0x850))
    print('    after clearing %%ireq:                %s' % pair(0x852))


def t61_cexe2(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T61
    g = lambda a: R.raw.get(a * R.unit)
    rep.check('t61 end marker', R.raw.get(OUT_OFF * R.unit), 0xE61D)
    print('    %-6s %-2s %-5s  a0+  a1+  r1    [S]   r5' % ('form', 'm', 'cond'))
    for i, (form, m, cond) in enumerate(T61):
        w = [g(0x800 + 8 * i + k) for k in range(8)]
        if w[7] != 0xC000 + i:
            print('    %-6s %-2s %-5s  not reached' % (form, m, cond))
            continue
        print('    %-6s %-2s %-5s  %3d  %3d  %s  %s  %d' % (form, m, cond, w[0], w[1], h(w[2]), h(w[3]), w[4]))


def t62_nestprio(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T62
    g = lambda a: R.raw.get(a * R.unit)
    rep.info('progress (case)', h(R.extra[0]))
    if R.extra[0x3C] == 0x5B00:
        rep.info('SPURIOUS interrupt', '%%ireq %s %%tpc %s' % (h(R.extra[0x3D]), h(R.extra[0x3E])))
    rep.check('t62 end marker', R.raw.get(OUT_OFF * R.unit), 0xE62D)
    res = {0: 'waited', 1: 'PREEMPTED', 0xFFFF: 'never ran'}
    for src in (13, 11):
        print('    inner source %d: rows = outer (12) level, columns = inner level' % src)
        print('             %s' % '  '.join('Li=%d      ' % li for li in range(4)))
        for lo in range(4):
            row = []
            for li in range(4):
                i = T62.index((src, lo, li))
                w = [g(0x800 + 4 * i + k) for k in range(4)]
                row.append('%-11s' % (res.get(w[0], h(w[0])) if w[3] == 0xC000 + i else 'n/r'))
            print('      Lo=%d   %s' % (lo, ' '.join(row)))
    i0 = [g(0x800 + 4 * i + 1) for i in range(len(T62))]
    i1 = [g(0x800 + 4 * i + 2) for i in range(len(T62))]
    print('    %%ip0 at outer entry (first 8 cases): %s' % ' '.join(h(x) for x in i0[:8]))
    print('    %%ip0 at inner entry (first 8 cases): %s' % ' '.join(h(x) for x in i1[:8]))


def t63_circ6(R, rep):
    sys.path.insert(0, HERE)
    from gen_t21 import T63_V
    g = lambda a: R.raw.get(a * R.unit)
    rep.check('t63 end marker', R.raw.get(OUT_OFF * R.unit), 0xE63D)
    lin = [0xC000 + k for k in range(6)] + [0xC005, 0xC006]
    k = 0
    found = 0
    for v in T63_V:
        odd = []
        for r in range(8):
            w = [g(0x800 + k + j) for j in range(8)]
            k += 8
            if w != lin:
                odd.append('a%d: %s' % (r, ' '.join('%X' % (x & 0xFFF) if x is not None else '?' for x in w)))
        found += len(odd)
        print('    %%amode %04X: %s' % (v, '; '.join(odd) if odd else 'all linear'))
    rep.info('cases reading other than linear data', found)


def t24_space(R, rep):
    w = out_words(R, 577)
    rep.info('progress: phase, k', '%s %s' % (h(R.extra[0]), h(R.extra[1])))
    def report(name, addrs, vals, expect):
        odd = [(a, v) for a, v, x in zip(addrs, vals, expect) if v != x]
        rep.info(name, '%d reads, %d differ from a DMEM mirror' % (len(vals), len(odd)))
        for a, v in odd[:40]:
            print('    %06X: %s' % (a, 'none' if v is None else h(v)))
        if len(odd) > 40:
            print('    ...')
    a1 = [0x7F0 + k * 0x800 for k in range(64)]
    report('phase 1 (0..0x1FFFF step 0x800)', a1, w[:64], [0x5A00 + (k % 16) for k in range(64)])
    a2 = [0x7F0 + k * 0x8000 for k in range(512)]
    report('phase 2 (whole 24-bit space step 0x8000)', a2, w[64:576], [0x5A00] * 512)
    rep.check('t24 end marker', w[576], 0xE24D)


CHECKS = {'t0c_units': t0c_units, 't1_call': t1_call, 't2_branch': t2_branch, 't2b_bgt32': t2b_bgt32,
          't3_regs': t3_regs, 't4_mem': t4_mem, 't5_imm': t5_imm, 't6_creg': t6_creg,
          't7_xbits': t7_xbits, 't8_alu': t8_alu, 't9_cexe': t9_cexe, 't12_hazards': t12_hazards, 't13_inferred': t13_inferred, 't14_settle': t14_settle, 't15_rest': t15_rest, 't16_flags': t16_flags,
          't17_circ': t17_circ, 't17b_circ': t17b_circ, 't20_final': t20_final,
          't21_ireq': t21_ireq, 't24_space': t24_space}


CHECKS2 = {'t48_src14': t48_src14, 't49_stdshort': t49_stdshort, 't50_nest': t50_nest, 't51_circ5': t51_circ5}


def main():
    rdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'results')
    only = sys.argv[2:]
    rep = Report()
    for fn in sorted(os.listdir(rdir)):
        name = fn[:-4]
        if not fn.endswith('.txt') or (only and name not in only) or name == 't7_xbits_ref':
            continue
        if not open(os.path.join(rdir, fn)).readline().startswith('# t'):
            continue        # bench.txt, ARM-side dumps
        R = Result(os.path.join(rdir, fn))
        print('%s: %s' % (name, 'done' if R.done else 'TIMEOUT'))
        if name == 't37_dmairq':
            t37_dmairq(R, rep)
            continue
        if name == 't38_armirq':
            t38_armirq(R, rep, rdir)
            continue
        if name == 't61_cexe2':
            t61_cexe2(R, rep)
            continue
        if name == 't62_nestprio':
            t62_nestprio(R, rep)
            continue
        if name == 't63_circ6':
            t63_circ6(R, rep)
            continue
        if name == 't60_hwflag2e':
            t60_hwflag2e(R, rep)
            continue
        if name == 't59_bisect':
            t59_bisect(R, rep)
            continue
        if name == 't58_hwflag2d':
            t58_hwflag2d(R, rep)
            continue
        if name == 't57_hwflag2c':
            t57_hwflag2c(R, rep)
            continue
        if name == 't56_hwflag2b':
            t56_hwflag2b(R, rep)
            continue
        if name == 't55_hwflag2':
            t55_hwflag2(R, rep)
            continue
        if name.startswith('t54_'):
            t54_smode(R, rep)
            continue
        if name == 't52_cexe':
            t52_cexe(R, rep)
            continue
        if name.startswith('t53_vxor_'):
            if name == 't53_vxor_c':
                t53_vxor(rdir, rep)
            continue
        if name in ('t48_src14', 't49_stdshort', 't50_nest', 't51_circ5'):
            CHECKS2[name](R, rep)
            continue
        if name == 't47_prio2':
            t47_prio2(R, rep)
            continue
        if name == 't46_prio':
            t46_prio(R, rep)
            continue
        if name == 't45_reti_smode':
            t45_reti_smode(R, rep)
            continue
        if name == 't44_brevst':
            t44_brevst(R, rep)
            continue
        if name == 't42_brev':
            t42_brev(R, rep)
            continue
        if name == 't43_circ4':
            t43_circ4(R, rep)
            continue
        if name == 't41_circ3':
            t41_circ3(R, rep)
            continue
        if name == 't40_circ2':
            t40_circ2(R, rep)
            continue
        if name == 't39_circ':
            t39_circ(R, rep)
            continue
        if name == 't36_dma2':
            t36_dma2(R, rep, rdir)
            continue
        if name == 't35_dma':
            t35_dma(R, rep)
            continue
        if name == 't34_sdbuf':
            t34_sdbuf(R, rep)
            continue
        if name == 't33_sdstore':
            t33_sdstore(R, rep, rdir)
            continue
        if name.startswith('t32_'):
            t32_repro(R, rep)
            continue
        if name == 't31_bususe':
            t31_bususe(R, rep)
            continue
        if name == 't30_bustime':
            t30_bustime(R, rep)
            continue
        if name == 't29_mbox':
            t29_mbox(R, rep)
            continue
        if name == 't28_mbox':
            t28_mbox(R, rep)
            continue
        if name == 't27_ext':
            t27_ext(R, rep)
            continue
        if name.startswith('t26_'):
            t26_ext(R, rep)
            continue
        if name.startswith('t25_smode_'):
            t25_smode(R, rep)
            continue
        if name.startswith(('t22_vec_', 't23_nmi')):
            t22_vec(R, rep)
            continue
        if name in ('t21_ireq', 't24_space') and not R.done:
            CHECKS[name](R, rep)      # partial results show how far it got
            continue
        if name.startswith('t19_jump_'):
            rep.info('jump', 'before=%s fell-through=%s target=%s rpc=%s tpc=%s' % tuple(h(x) for x in (R.extra[0], R.extra[1], R.extra[2], R.r[10], R.r[11])))
            continue
        if name.startswith(('t10_', 't18_cp_')):
            t10(R, rep)
            continue
        if not R.done and name != 't2b_bgt32':
            rep.info('no MAGIC', 'register dump incomplete; skipping checks')
            continue
        f = CHECKS.get(name)
        if f is t7_xbits:
            f(R, rep, rdir)
        elif f:
            f(R, rep)
    print('\n%(PASS)d pass, %(FAIL)d fail, %(INFO)d info' % rep.counts)


if __name__ == '__main__':
    main()
