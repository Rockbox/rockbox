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
Generate t13_inferred.s + t13_inferred.json: run the inferred multiply,
divide and bit-field instructions once each on known inputs and record
r0..r3 (whichever the instruction writes) plus the cycle count (%timer0).

Results: 6 halfwords per case at OUT = SCRATCH + 0x100 (DSP 0x400, ARM byte
offset 0x800, inside the raw DMEM dump ZSP.run saves):
    r0, r1, r2, r3, cycles, 0xC000 + case index
"""
import json

FB_OPS = ['imulc', 'mulsu', 'muluu', 'mul', 'muln', 'imul', 'pmul', 'mul2', 'dmul', 'cmulr', 'cmuli',
          'padd', 'psub', 'idmulc', 'macsu', 'macuu', 'mac', 'macn', 'pmac', 'mac2', 'dmac', 'cmacr', 'cmaci']
P9_OPS = ['padd', 'psub', 'pmul', 'idmulc', 'cmulr', 'cmuli', 'mul2', 'pmac', 'mac2', 'cmacr', 'cmaci']

# default register file for every case (overridable per case)
BASE = {'r0': 0x0100, 'r1': 0x0000, 'r2': 0x0200, 'r3': 0x0000,
        'r4': 0xFFFD, 'r5': 0x0005,      # scalar: -3, 5
        'r6': 0x0007, 'r7': 0xFFFE,      # scalar/pair: 7, -2
        'r8': 0x0003, 'r9': 0x0004,      # pair X = (3, 4)
        'r10': 0x0005, 'r11': 0x0006,    # pair Y = (5, 6)
        'r12': 0xA5C3}                   # bit-field source

cases = []


def case(name, insn, note, **regs):
    r = dict(BASE)
    r.update(regs)
    cases.append({'name': name, 'insn': insn if isinstance(insn, list) else [insn], 'note': note, 'regs': r})


# ---- 16-bit multiply forms (8xxx): dest pair r0:r1 (a=0)
for op in ['mul', 'muln', 'imul', 'mac', 'macn']:
    case(op + '16_s', '%s r0,r4,r5' % op, 'scalar -3 x 5, acc r0:r1 = 0x100')
    case(op + '16_p', '%s r0,r8,r10' % op, 'scalar 3 x 5, acc r0:r1 = 0x100')
case('imulc16_2op', 'imulc r0,r4', 'r0 = 0x100, r4 = -3')
case('imulc16_r2', 'imulc r2,r4,r5', 'dest fixed r2, -3 x 5')
case('imulc16_r2p', 'imulc r2,r8,r10', 'dest fixed r2, pairs X, Y')
# ---- 16-bit pair forms (9xxx): dest pair r0, sources pairs X = r8:r9, Y = r10:r11
for op in P9_OPS:
    case(op + '9_xy', '%s r0,r8,r10' % op, 'X = (3,4), Y = (5,6), acc r0:r1 = 0x100')
    case(op + '9_sn', '%s r0,r4,r6' % op, 'X = (-3,5), Y = (7,-2), acc r0:r1 = 0x100')
# ---- 32-bit three-operand forms (FBxx)
for op in FB_OPS:
    case(op + '_s', '%s r0,r4,r5' % op, '-3 x 5 (r4, r5), r0:r1 = 0x100')
    case(op + '_p', '%s r0,r8,r10' % op, 'r8 = 3 (r9 = 4), r10 = 5 (r11 = 6), r0:r1 = 0x100')
# ---- divide step
case('diva_1', 'diva r0,r5', 'r0:r1 = 100, r5 = 5, one step', r0=100, r1=0)
case('diva_16', ['diva r0,r5'] * 16, 'r0:r1 = 100, r5 = 5, 16 steps', r0=100, r1=0)
case('diva_16h', ['diva r0,r5'] * 16, 'r0:r1 = 100 << 16, r5 = 5, 16 steps', r0=0, r1=100)
case('diva_17', ['diva r0,r5'] * 17, 'r0:r1 = 100, r5 = 5, 17 steps', r0=100, r1=0)
# ---- bit field, 32-bit immediate forms; source r12 = 0xA5C3, dest r0 = 0xFFFF (ins) / 0 (ext)
for c, d in [(4, 8), (8, 4), (0, 4), (12, 4), (4, 4), (0, 1), (15, 1)]:
    case('ext_%d_%d' % (c, d), 'ext r0,r12,%d,%d' % (c, d), 'src 0xA5C3', r0=0)
    case('exts_%d_%d' % (c, d), 'exts r0,r12,%d,%d' % (c, d), 'src 0xA5C3', r0=0)
    case('ins_%d_%d' % (c, d), 'ins r0,r12,%d,%d' % (c, d), 'src 0xA5C3 into 0xFFFF', r0=0xFFFF, r1=0xFFFF)
# 32-bit pair source / dest
for c, d in [(4, 8), (16, 8), (8, 16), (0, 20)]:
    case('ext.e_%d_%d' % (c, d), 'ext.e r0,r8,%d,%d' % (c, d), 'src pair r8:r9 = 0x12345678', r0=0, r8=0x5678, r9=0x1234)
    case('exts.e_%d_%d' % (c, d), 'exts.e r0,r8,%d,%d' % (c, d), 'src pair 0x12345678', r0=0, r8=0x5678, r9=0x1234)
    case('ins.e_%d_%d' % (c, d), 'ins.e r0,r12,%d,%d' % (c, d), 'src 0xA5C3 into pair 0xFFFFFFFF', r0=0xFFFF, r1=0xFFFF)
# register field spec (r5)
for spec in [0x0408, 0x0804, 0x0084, 0x0048]:
    case('extr_%04x' % spec, 'ext r0,r12,r5', 'src 0xA5C3, spec r5', r0=0, r5=spec)
    case('extsr_%04x' % spec, 'exts r0,r12,r5', 'src 0xA5C3, spec r5', r0=0, r5=spec)
    case('insr_%04x' % spec, 'ins r0,r12,r5', 'src 0xA5C3 into 0xFFFF, spec r5', r0=0xFFFF, r5=spec)
# 16-bit forms with implied spec
case('ext16', 'ext r0,r12', 'src 0xA5C3, dest pair r0:r1', r0=0, r1=0)
case('exts16', 'exts r0,r12', 'src 0xA5C3, dest pair r0:r1', r0=0, r1=0)
case('ins16', 'ins r0,r8', 'pair r8:r9 = (0xA5C3, 0x0408), dest 0xFFFF', r0=0xFFFF, r8=0xA5C3, r9=0x0408)
case('ext16e', 'ext.e r0,r8', 'pair r8:r9 = (0x5678, 0x1234)', r0=0, r1=0, r8=0x5678, r9=0x1234)
case('exts16e', 'exts.e r0,r8', 'pair r8:r9 = (0x5678, 0x1234)', r0=0, r1=0, r8=0x5678, r9=0x1234)
case('ins16e', 'ins.e r0,r8', 'pair r8:r9 = (0xA5C3, 0x0408), dest pair 0xFFFFFFFF', r0=0xFFFF, r1=0xFFFF,
     r8=0xA5C3, r9=0x0408)


def main():
    L = ['; generated by gen_t13.py: inferred multiply / divide / bit-field instructions',
         '        .equ TEST_ID, 0x0013',
         '        .include "common.inc"',
         'OUT = SCRATCH + 0x100',
         '        .include "prologue.inc"',
         '        mov.l r0,0x0080',
         '        mov %tc,r0               ; start %timer0',
         '        mov a5,lo(OUT)',
         '        movhw a5,hi(OUT)']
    for i, cs in enumerate(cases):
        L.append('; ---- %d %s: %s' % (i, cs['name'], cs['note']))
        for r in ['r%d' % k for k in range(13)]:
            L.append('        mov.l %s,0x%04X' % (r, cs['regs'][r]))
        L += ['        nop'] * 4
        L.append('        mov r13,%timer0')
        L += ['        ' + x for x in cs['insn']]
        L.append('        mov r14,%timer0')
        L += ['        nop'] * 4
        L.append('        sub r13,r14')
        L += ['        stu r%d,a5,1' % k for k in range(4)]
        L += ['        stu r13,a5,1', '        mov.l r14,0x%04X' % (0xC000 + i), '        stu r14,a5,1']
    L.append('        .include "epilogue.inc"')
    open('t13_inferred.s', 'w').write('\n'.join(L) + '\n')
    json.dump(cases, open('t13_inferred.json', 'w'), indent=1)
    print('%d cases' % len(cases))


if __name__ == '__main__':
    main()
