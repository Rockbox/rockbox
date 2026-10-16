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
Generate t12_hazards.s + t12_hazards.json: hazard correctness and timing.

Each case runs its sequence twice: back to back ('imm') and with 6 nops between
the instructions ('dly'). Both results and both cycle counts (from %timer0,
a down-counter at the DSP clock) are stored:

    EXTRA + 4k + 0: result, imm      EXTRA + 4k + 2: result, dly
    EXTRA + 4k + 1: cycles, imm      EXTRA + 4k + 3: cycles, dly

Data in SCRATCH (S): S+0 = 0x1111, S+1 = 0x2222, S+2 = 0x3333, S+4 = 0x4444,
S+8/S+9 = pointer S+2 (low, high). Every case re-initialises its registers
and is separated from the previous one by nops.
"""
import json

S = 'SCRATCH'
SETUP_COMMON = ['mov a0,lo(%s)' % S, 'movhw a0,hi(%s)' % S,
                'mov a1,lo(%s + 1)' % S, 'movhw a1,hi(%s + 1)' % S]

# name, setup, sequence, result register, fresh value (None = compare imm/dly only), stale value, note
CASES = [
    ('empty', [], [], 'r2', None, None, 'baseline: two timer reads'),
    ('ld_add', ['mov.l r1,0xAAAA', 'mov.l r2,0'], ['ld r1,a0', 'add r2,r1'], 'r2', 0x1111, 0xAAAA,
     'load, then ALU use'),
    ('ld_mov', ['mov.l r1,0xAAAA'], ['ld r1,a0', 'mov r2,r1'], 'r2', 0x1111, 0xAAAA, 'load, then move'),
    ('ldd_mov', ['mov.l r2,0xAAAA', 'mov.l r3,0xAAAA'], ['ldd r2,a0', 'mov r4,r3'], 'r4', 0x2222, 0xAAAA,
     'pair load, odd register used'),
    ('ld_st', ['mov.l r1,0xAAAA', 'mov a2,lo(%s + 5)' % S, 'movhw a2,hi(%s + 5)' % S],
     ['ld r1,a0', 'st r1,a2', 'ld r2,a2'], 'r2', 0x1111, 0xAAAA, 'load, then store of the loaded register'),
    ('amov_ld', [], ['mov a0,a1', 'ld r2,a0'], 'r2', 0x2222, 0x1111, 'a-register move, then load through it'),
    ('aadd_ld', [], ['add a0,2', 'ld r2,a0'], 'r2', 0x3333, 0x1111, 'a-register add, then load'),
    ('amovlw_ld', ['mov.l r5,lo(%s + 1)' % S], ['movlw a0,r5', 'ld r2,a0'], 'r2', 0x2222, 0x1111,
     'a-register low half from r, then load'),
    ('aldd_ld', ['mov a3,lo(%s + 8)' % S, 'movhw a3,hi(%s + 8)' % S], ['ldd a0,a3', 'ld r2,a0'], 'r2',
     0x3333, 0x1111, 'pointer loaded from memory, then used'),
    ('ldu_ld', [], ['ldu r3,a0,1', 'ld r2,a0'], 'r2', 0x2222, 0x1111, 'post-modify, then load through the updated register'),
    ('n_ldx', ['mov n1,0', 'mov.l r6,1'], ['mov n1,r6', 'ldx r2,a0,n1'], 'r2', 0x2222, 0x1111,
     'index register written, then used'),
    ('st_ld', ['mov a2,lo(%s + 4)' % S, 'movhw a2,hi(%s + 4)' % S, 'mov.l r1,0x4444', 'st r1,a2',
               'nop', 'nop', 'nop', 'nop', 'nop', 'nop', 'mov.l r1,0x5555'],
     ['st r1,a2', 'ld r2,a2'], 'r2', 0x5555, 0x4444, 'store, then load of the same address'),
    ('mul_lo', ['mov.l r3,3', 'mov.l r4,5', 'mov.l r0,0xAAAA', 'mov.l r1,0xAAAA'],
     ['mul r0,r3,r4', 'mov r2,r0'], 'r2', None, 0xAAAA, 'multiply, then use r0'),
    ('mul_hi', ['mov.l r3,3', 'mov.l r4,5', 'mov.l r0,0xAAAA', 'mov.l r1,0xAAAA'],
     ['mul r0,r3,r4', 'mov r2,r1'], 'r2', None, 0xAAAA, 'multiply, then use r1'),
    ('mac_lo', ['mov.l r3,3', 'mov.l r4,5', 'mov.l r0,0', 'mov.l r1,0'],
     ['mac r0,r3,r4', 'mov r2,r0'], 'r2', None, 0, 'MAC, then use r0'),
    ('mac_hi', ['mov.l r3,3', 'mov.l r4,5', 'mov.l r0,0', 'mov.l r1,0'],
     ['mac r0,r3,r4', 'mov r2,r1'], 'r2', None, 0, 'MAC, then use r1'),
    ('mac_mac', ['mov.l r3,3', 'mov.l r4,5', 'mov.l r0,0', 'mov.l r1,0'],
     ['mac r0,r3,r4', 'mac r0,r3,r4', 'nop', 'nop', 'nop', 'nop', 'mov r2,r1'], 'r2', None, None,
     'two dependent MACs'),
    ('add_add', ['mov.l r1,1'], ['add r1,r1', 'add r1,r1', 'mov r2,r1'], 'r2', 4, 1, 'ALU chain'),
    ('creg_rd', ['mov.l r1,0x0055', 'mov.l r2,0'], ['mov %loop1,r1', 'mov r2,%loop1'], 'r2', 0x0055, None,
     'control register write, then read'),
    ('creg_imm', ['mov.l r2,0'], ['mov %loop1,0x0077', 'mov r2,%loop1'], 'r2', 0x0077, None,
     'control register immediate write, then read'),
    ('g_rd', ['mov g0,0', 'mov g1,0'], ['mov g1,0x55', 'mov r2,g0'], 'r2', 0x5500, 0x0000,
     'guard register write, then pair read'),
    ('loop_agn', ['mov.l r0,2', 'mov.l r2,0'], ['mov %loop0,r0', 'L%d: add r2,1', 'agn0 L%d'], 'r2', 3, None,
     'loop counter written right before the loop'),
]


def body(seq, delay, tag):
    out = []
    for i, ins in enumerate(seq):
        if '%d' in ins:
            ins = ins % tag
        if i and delay:
            out += ['nop'] * 6
        out.append(ins)
    return out


def main():
    L = ['; generated by gen_t12.py: hazard correctness and timing',
         '        .equ TEST_ID, 0x0012',
         '        .include "common.inc"',
         '        .include "prologue.inc"',
         '        mov.l r0,0x0080',
         '        mov %tc,r0               ; start %timer0',
         '        mov a0,lo(SCRATCH)', '        movhw a0,hi(SCRATCH)']
    for off, v in [(0, 0x1111), (1, 0x2222), (2, 0x3333), (4, 0x4444)]:
        L += ['        mov.l r1,0x%04X' % v, '        st r1,a0,%d' % off]
    L += ['        mov a3,lo(SCRATCH + 2)', '        movhw a3,hi(SCRATCH + 2)', '        std a3,a0,8']
    meta = []
    slot = 0
    for name, setup, seq, res, fresh, stale, note in CASES:
        for v, delay in (('imm', False), ('dly', True)):
            tag = '%s_%s' % (name, v)
            L.append('; ---- %s (%s): %s' % (name, v, note))
            L += ['        ' + x for x in SETUP_COMMON + setup]
            L += ['        nop'] * 8
            L.append('        mov r13,%timer0')
            for ins in body(seq, delay, len(meta) * 2 + (1 if delay else 0)):
                L.append(('' if ins.startswith('L') else '        ') + ins)
            L.append('        mov r14,%timer0')
            L += ['        nop'] * 4
            L.append('        sub r13,r14              ; elapsed cycles (down-counter)')
            L.append('        st %s,a7,0x%X' % (res, 0x40 + slot))
            L.append('        st r13,a7,0x%X' % (0x40 + slot + 1))
            slot += 2
        meta.append({'name': name, 'slot': 4 * len(meta), 'fresh': fresh, 'stale': stale, 'note': note,
                     'seq': [s.replace('L%d: ', '') for s in seq]})
    L.append('        .include "epilogue.inc"')
    open('t12_hazards.s', 'w').write('\n'.join(L) + '\n')
    json.dump(meta, open('t12_hazards.json', 'w'), indent=1)
    print('%d cases, %d result slots' % (len(CASES), slot))


if __name__ == '__main__':
    main()
