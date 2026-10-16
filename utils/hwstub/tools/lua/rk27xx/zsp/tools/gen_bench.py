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
Generate DSP timing benchmarks: tests/bench/<case>_u<U>.s + tests/bench/cases.json

Each program repeats a short instruction group U times inside a loop:

    outer:  r15 = outer count (from DMEM, set by the host)  -- software loop
    inner:  %loop0 = INNER-1, agn0                          -- hardware loop
              group x U

Two builds per case (U = U1, U2). The host times both with the ARM timer;
cycles per group = (time/outer(U2) - time/outer(U1)) / (INNER * (U2-U1)),
which cancels loop and polling overhead.

Body register rules: r0 is reloaded every outer pass, r15 and a7 are reserved.
Initial state: r1..r14 = 1..14, a0..a3 = SCRATCH + 0/0x20/0x40/0x60,
a4 = SCRATCH + 1, a5 = SCRATCH + 0x4000, a6 = SCRATCH + 0x100 (stack),
n1 = 1, r12 = address of a 'ret' stub, Z flag set by cmp r1,r1 per pass.
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'tests', 'bench')
INNER = 1000
U1, U2 = 8, 24

# name: (category, group of instructions, number of instructions in the group, note)
CASES = [
    # ---- issue rate
    ('nop',          'alu',    ['nop'], 'empty slot'),
    ('add_dep',      'alu',    ['add r1,r1'], 'dependent chain'),
    ('add_indep4',   'alu',    ['add r1,r9', 'add r2,r9', 'add r3,r9', 'add r4,r9'], '4 independent'),
    ('add_imm4',     'alu',    ['add r1,1'], '16-bit immediate'),
    ('add3',         'alu',    ['add r1,r2,r3'], '32-bit 3-operand'),
    ('mov_imm16',    'alu',    ['mov.l r1,0x1234'], '32-bit immediate'),
    ('shll_imm',     'alu',    ['shll r1,3'], 'shift by immediate'),
    ('shll_reg',     'alu',    ['shll r1,r2'], 'shift by register'),
    ('mul16',        'mac',    ['mul r0,r1,r2'], '16-bit multiply'),
    ('mac_dep',      'mac',    ['mac r0,r1,r2'], 'accumulate chain'),
    ('mac_indep2',   'mac',    ['mac r0,r1,r3', 'mac r2,r4,r5'], 'two accumulators'),
    ('mov_a',        'alu',    ['mov a3,a1'], 'address register move'),
    ('add_a_imm',    'alu',    ['add a3,1'], 'address register add'),
    # ---- memory (DMEM)
    ('ld',           'mem',    ['ld r1,a0'], 'load halfword'),
    ('ld_indep4',    'mem',    ['ld r1,a0', 'ld r2,a1', 'ld r3,a2', 'ld r4,a3'], '4 loads'),
    ('ldd',          'mem',    ['ldd r2,a0'], 'load pair (32 bit)'),
    ('ldq',          'mem',    ['ldq r4,a0'], 'load quad (64 bit)'),
    ('ld_short',     'mem',    ['ld r1,a6,3'], 'a6 short offset'),
    ('ld_disp16',    'mem',    ['ld r1,a0,100'], '32-bit displacement'),
    ('ldu_pm',       'mem',    ['ldu r1,a0,1', 'ldu r2,a0,-1'], 'post-modify +1/-1'),
    ('ldx',          'mem',    ['ldx r1,a0,n1'], 'indexed'),
    ('st',           'mem',    ['st r1,a0'], 'store halfword'),
    ('st_indep4',    'mem',    ['st r1,a0', 'st r2,a1', 'st r3,a2', 'st r4,a3'], '4 stores'),
    ('std',          'mem',    ['std r2,a0'], 'store pair'),
    ('stq',          'mem',    ['stq r4,a0'], 'store quad'),
    ('stu_pm',       'mem',    ['stu r1,a0,1', 'stu r2,a0,-1'], 'post-modify +1/-1'),
    ('ld_ld_near',   'mem',    ['ld r1,a0', 'ld r2,a4'], 'two loads, adjacent addresses'),
    ('ld_ld_far',    'mem',    ['ld r1,a0', 'ld r2,a5'], 'two loads, 0x4000 apart'),
    ('ld_st',        'mem',    ['ld r1,a0', 'st r2,a1'], 'load + store'),
    ('push_pop',     'mem',    ['push r1,a6', 'pop r2,a6'], 'stack'),
    # ---- hazards: *_use vs *_nouse differ only in the dependency
    ('load_use',     'hazard', ['ld r1,a0', 'add r2,r1'], 'loaded value used next'),
    ('load_nouse',   'hazard', ['ld r1,a0', 'add r2,r3'], 'reference'),
    ('aload_use',    'hazard', ['mov a0,a1', 'ld r1,a0'], 'address register used next'),
    ('aload_nouse',  'hazard', ['mov a3,a1', 'ld r1,a0'], 'reference'),
    ('st_ld_same',   'hazard', ['st r1,a0', 'ld r2,a0'], 'load after store, same address'),
    ('st_ld_diff',   'hazard', ['st r1,a0', 'ld r2,a1'], 'reference'),
    ('mac_use',      'hazard', ['mac r0,r1,r2', 'add r3,r0'], 'accumulator used next'),
    ('mac_nouse',    'hazard', ['mac r0,r1,r2', 'add r3,r4'], 'reference'),
    ('mul_use',      'hazard', ['mul r0,r1,r2', 'add r3,r0'], 'product used next'),
    ('mul_nouse',    'hazard', ['mul r0,r1,r2', 'add r3,r4'], 'reference'),
    ('creg_use',     'hazard', ['mov %loop1,r1', 'mov r2,%loop1'], 'control register read after write'),
    ('creg_nouse',   'hazard', ['mov %loop1,r1', 'mov r2,r3'], 'reference'),
    # ---- control flow
    ('br_next',      'flow',   ['br .+2'], '32-bit branch taken to next'),
    ('bz_taken',     'flow',   ['cmp r1,r1', 'bz .+1'], 'cmp + 16-bit branch taken'),
    ('bz_nottaken',  'flow',   ['cmp r1,r1', 'bnz .+1'], 'cmp + 16-bit branch not taken'),
    ('cmp_only',     'flow',   ['cmp r1,r1', 'nop'], 'reference for bz_*'),
    ('call_ret',     'flow',   ['call r12'], 'call + ret'),
    ('cexe_true',    'flow',   ['cmp r1,r1', 'cexe (z,nu) { } !1', 'add r2,r3'], 'condition true'),
    ('cexe_false',   'flow',   ['cmp r1,r1', 'cexe (nz,nu) { } !1', 'add r2,r3'], 'condition false'),
    # ---- 32-bit pair ALU and mulsu (as in a DCT on register pairs)
    ('adde_dep',     'alu',    ['add.e r2,r4'], 'pair add, dependent chain'),
    ('adde_indep2',  'alu',    ['add.e r2,r4', 'add.e r6,r8'], 'two pair adds'),
    ('shrae',        'alu',    ['shra.e r10,15'], 'pair shift, dependent chain'),
    ('move_indep2',  'alu',    ['mov.e r2,r4', 'mov.e r6,r8'], 'two pair moves'),
    ('move_add2',    'alu',    ['mov.e r2,r4', 'mov.e r6,r8', 'add r1,r9', 'add r3,r9'],
                               'pair moves + 16-bit adds: issue slots or pair limit'),
    ('move_add2b',   'alu',    ['mov.e r2,r4', 'mov.e r6,r8', 'add r1,r9', 'add r11,r9'],
                               'as move_add2, no 16-bit op on half of a pair'),
    ('adde_add',     'alu',    ['add.e r2,r4', 'add r11,r9'], 'pair add beside a 16-bit add'),
    ('mulsu',        'mac',    ['mulsu r10,r9,r12'], 'signed x unsigned multiply'),
    # ---- mixes from an MP3 subband synthesis on the DSP (window taps,
    # DCT); post-modify steps are undone within the group so the pointers
    # stay in place
    ('ldu_2areg',    'mix',    ['ldu r1,a0,1', 'ldu r2,a1,1', 'ldu r1,a0,-1', 'ldu r2,a1,-1'],
                               'post-modify loads, two a-registers'),
    ('lddu_ldu',     'mix',    ['lddu r8,a0,2', 'ldu r10,a2,1', 'lddu r8,a0,-2', 'ldu r10,a2,-1'],
                               'window tap loads'),
    ('ld2_mac2',     'mix',    ['ldd r8,a0', 'ld r10,a2', 'mac r0,r9,r10', 'mac r2,r8,r10'],
                               'window tap without post-modify'),
    ('win_tap',      'mix',    ['lddu r8,a0,2', 'ldu r10,a2,1', 'mac r0,r9,r10', 'mac r2,r8,r10',
                                'lddu r8,a0,-2', 'ldu r10,a2,-1', 'mac r0,r9,r10', 'mac r2,r8,r10'],
                               'two window taps (win8a0.inc)'),
    ('mac_ld',       'mix',    ['mac r0,r1,r2', 'ld r3,a0'], 'MAC beside a load'),
    ('ldd_std',      'mix',    ['ldd r4,a0,4', 'std r2,a6,38'], 'DCT load + spill'),
    ('mulsu_chain',  'mix',    ['mulsu r10,r13,r8', 'shra.e r10,15', 'add.e r2,r10'],
                               'DCT rotation term'),
    ('dct_rot',      'mix',    ['ldd r12,a5,36', 'mul r2,r9,r13', 'mulsu r10,r9,r12', 'shll.e r2,1',
                                'shra.e r10,15', 'add.e r2,r10', 'mulsu r10,r13,r8', 'shra.e r10,15',
                                'add.e r2,r10', 'mov.e r8,r0', 'add.e r8,r2', 'sub.e r0,r2'],
                               'dct32.inc rotation + butterfly'),
    ('dct_bfly',     'mix',    ['ldd r4,a0,4', 'std r2,a6,38', 'ldd r2,a0,58', 'std r6,a6,40',
                                'mov.e r6,r4', 'add.e r6,r2', 'sub.e r4,r2'],
                               'dct32.inc loads, spills, butterfly'),
]


def program(name, group, u):
    L = ['; generated by tools/gen_bench.py: %s, group x %d' % (name, u),
         '        .include "../common.inc"',
         'PARAM = RES + 0x20               ; outer count - 1, written by the host',
         '        .org 0',
         '        mov a7,lo(RES)',
         '        movhw a7,hi(RES)']
    for i in range(1, 15):
        if i != 12:
            L.append('        mov.l r%d,%d' % (i, i))
    L += ['        mov.l r12,ret_stub',
          '        mov a0,lo(SCRATCH)', '        movhw a0,hi(SCRATCH)',
          '        mov a1,lo(SCRATCH + 0x20)', '        movhw a1,hi(SCRATCH + 0x20)',
          '        mov a2,lo(SCRATCH + 0x40)', '        movhw a2,hi(SCRATCH + 0x40)',
          '        mov a3,lo(SCRATCH + 0x60)', '        movhw a3,hi(SCRATCH + 0x60)',
          '        mov a4,lo(SCRATCH + 1)', '        movhw a4,hi(SCRATCH + 1)',
          '        mov a5,lo(SCRATCH + 0x4000)', '        movhw a5,hi(SCRATCH + 0x4000)',
          '        mov a6,lo(SCRATCH + 0x100)', '        movhw a6,hi(SCRATCH + 0x100)',
          '        mov n1,1',
          '        ld r15,a7,0x20             ; outer count - 1',
          'outer:  mov.l r0,%d' % (INNER - 1),
          '        mov %loop0,r0',
          '        cmp r1,r1                  ; Z = 1 for the flow cases',
          'inner:']
    for _ in range(u):
        L += ['        ' + g for g in group]
    L += ['        agn0 inner',
          '        add r15,-1',
          '        cmp r15,-1',
          '        bnz outer',
          '        mov.l r0,MAGIC',
          '        st r0,a7,63',
          'spin:   br spin',
          'ret_stub:',
          '        ret']
    return '\n'.join(L) + '\n'


def main():
    os.makedirs(OUT, exist_ok=True)
    meta = {'inner': INNER, 'u1': U1, 'u2': U2, 'cases': []}
    for name, cat, group, note in CASES:
        for u in (U1, U2):
            open(os.path.join(OUT, '%s_u%d.s' % (name, u)), 'w').write(program(name, group, u))
        meta['cases'].append({'name': name, 'cat': cat, 'group': group, 'n': len(group), 'note': note})
    json.dump(meta, open(os.path.join(OUT, 'cases.json'), 'w'), indent=1)
    print('%d cases, %d programs' % (len(CASES), 2 * len(CASES)))


if __name__ == '__main__':
    main()
