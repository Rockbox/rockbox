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
Generate t15_rest.s + t15_rest.json: every instruction still marked Inferred,
run once on known state, recording the registers / memory it may affect.

Per case: standard state, extra setup, %timer0, instruction(s), %timer0, then
record: cycles, <items>, 0xC000 + case index (stu to a5, SCRATCH + 0x100 on).

Standard state (S = SCRATCH):
  r0..r12 = BASE below, a0 = S+0x10, a1 = S+0x80, a2 = S+0x20, a3 = S+0x30,
  a4 = S+0x50, a6 = S+0x40, n0..n7 = 1..8, g0..g7 = 0x10..0x17
  memory S+i = 0xB000+i for i < 0x80 (loads), S+0x80..0x9F = 0xA5A5 before
  every case that stores (stores go there; a1 points at S+0x80)
Reserved: r13 (scratch), r14 (timer), a5 (output), a7 (RES).
Record items: rN, aNl/aNh (low/high half), nN, gN (pair gN+1:gN), %creg,
  mXX (memory S+0xXX).
"""
import json

S = 'SCRATCH'
BASE = {'r0': 0x0100, 'r1': 0x0000, 'r2': 0x0200, 'r3': 0x0000, 'r4': 0xFFFD, 'r5': 0x0005,
        'r6': 0x0007, 'r7': 0xFFFE, 'r8': 0x0003, 'r9': 0x0004, 'r10': 0x0005, 'r11': 0x0006,
        'r12': 0xA5C3}
AREG = {'a0': 0x10, 'a1': 0x80, 'a2': 0x20, 'a3': 0x30, 'a4': 0x50, 'a6': 0x40}
CREG_E864 = {'%cb0_beg', '%cb0_end', '%cb1_beg', '%cb1_end', '%cb2_beg', '%cb2_end', '%cb3_beg', '%cb3_end'}

cases = []


def case(name, insns, rec, note='', setup=(), stores=False, regs=None):
    r = dict(BASE)
    r.update(regs or {})
    cases.append({'name': name, 'insn': insns if isinstance(insns, list) else [insns], 'rec': rec,
                  'note': note, 'setup': list(setup), 'stores': stores, 'regs': r})


M80 = ['m80', 'm81', 'm82', 'm83', 'm84']

# ------------------------------------------------------------------ data movement
case('swap_a', 'swap a2,a3', ['a2l', 'a3l'])
case('swap_n', 'swap n1,n2', ['n1', 'n2'])
case('swap_r', 'swap r4,r5', ['r4', 'r5'])
case('movhw_a_r', 'movhw a2,r4', ['a2l', 'a2h'])
case('movlw_a_r', 'movlw a2,r4', ['a2l', 'a2h'])
case('movlw_a_imm', 'movlw a2,0x1111', ['a2l', 'a2h'])
case('mov_a_imm_hi', 'mov a2,0x1234', ['a2l', 'a2h'], 'does mov a,imm clear the high half?',
     setup=['movhw a2,0x5555'])
case('mov_vitr_r', 'mov %vitr,r4', ['%vitr'], '79xx index 2')
case('mov_hwflag_r', 'mov %shwflag,r5', ['%shwflag'], '79xx index 1')
case('mov.e_vitr_r', ['mov.e %vitr,r8', 'mov.e r0,%vitr'], ['r0', 'r1', '%vitr'], '7A01 then 7Cxx read back')
case('mov.e_a_cb', ['mov.l r15,0x1234', 'mov %cb1_beg,r15', 'nop', 'nop', 'mov.e a2,%cb1_beg'], ['a2l', 'a2h'])
case('mov.e_r_a', 'mov.e r0,a2', ['r0', 'r1'], 'a2 = S+0x20')
case('mov.e_cb_a', ['mov.e %cb2_beg,a3', 'nop', 'nop', 'mov r15,%cb2_beg', 'mov r12,r15'], ['r12', '%cb2_beg'],
     'cb2_beg <- a3 (S+0x30)')
case('mov_r_n', 'mov r4,n3', ['r4'])
case('mov.e_rr', 'mov.e r0,r8', ['r0', 'r1'])
case('mov.f_rr', 'mov.f r0,r8', ['r0', 'r1', 'g0'])
case('movlw.f', 'movlw.f r0,-2', ['r0', 'r1', 'g0'])
case('movhw.f', 'movhw.f r0,0x1234', ['r0', 'r1', 'g0'])
case('movl_r', 'movl r4,0x12', ['r4'], 'r4 = 0xFFFD')
case('movh_r', 'movh r4,0x34', ['r4'], 'r4 = 0xFFFD')
case('mov_g_r', 'mov g0,r12', ['g0', 'g2'], 'r12 = 0xA5C3')
# ------------------------------------------------------------------ loads (pattern S+i = 0xB000+i)
for ins, rec in [('lddu r4,a0,2', ['r4', 'r5', 'a0l']), ('lddu r4,a0,-2', ['r4', 'r5', 'a0l']),
                 ('ldqu r4,a0,4', ['r4', 'r5', 'r6', 'r7', 'a0l']), ('lddu a2,a0,2', ['a2l', 'a2h', 'a0l']),
                 ('ldxu r4,a0', ['r4', 'a0l']), ('lddxu a2,a0', ['a2l', 'a2h', 'a0l']),
                 ('ldxu n4,a0', ['n4', 'a0l']), ('lddxu r4,a0', ['r4', 'r5', 'a0l']),
                 ('ldqxu r4,a0', ['r4', 'r5', 'r6', 'r7', 'a0l']),
                 ('ldd a2,a6,3', ['a2l', 'a2h']), ('ldd r4,a6,3', ['r4', 'r5']), ('ld n4,a0', ['n4']),
                 ('ldx r4,a0', ['r4', 'a0l']), ('lddx r4,a0', ['r4', 'r5']), ('ldqx r4,a0', ['r4', 'r5', 'r6', 'r7']),
                 ('lddx a2,a0', ['a2l', 'a2h']), ('ldx n4,a0', ['n4']),
                 ('ldd a2,a0,2', ['a2l', 'a2h']), ('ld n4,a0,2', ['n4']), ('ldd r4,a0,2', ['r4', 'r5']),
                 ('ldf r4,a0,2', ['r4', 'r5', 'g4']), ('ldfu r4,a0,3', ['r4', 'r5', 'g4', 'a0l']),
                 ('ldxu r4,a0,n2', ['r4', 'a0l']), ('lddxu a2,a0,n2', ['a2l', 'a2h', 'a0l']),
                 ('ldxu n4,a0,n2', ['n4', 'a0l']), ('lddxu r4,a0,n2', ['r4', 'r5', 'a0l']),
                 ('ldqxu r4,a0,n2', ['r4', 'r5', 'r6', 'r7', 'a0l']), ('lddx a2,a0,n2', ['a2l', 'a2h']),
                 ('ldx n4,a0,n2', ['n4']), ('lddx r4,a0,n2', ['r4', 'r5']), ('ldqx r4,a0,n2', ['r4', 'r5', 'r6', 'r7'])]:
    case('L ' + ins, ins, rec, 'a0 = S+0x10 ([a0] = 0xB010), n0..n7 = 1..8')
# r-register base: r6 = data address S+0x12
RB = {'r6': None}
for ins, rec in [('ldu r4,r6,1', ['r4', 'r6']), ('lddu r4,r6,2', ['r4', 'r5', 'r6']), ('ld r4,r6', ['r4']),
                 ('ldx r4,r6', ['r4', 'r6', 'r7']), ('ldxu r4,r6', ['r4', 'r6', 'r7'])]:
    case('L ' + ins, ['mov.l r6,lo(%s + 0x12)' % S, 'mov.l r7,2', 'nop', 'nop'] + [ins], rec,
         'r6 = S+0x12 ([r6] = 0xB012), r7 = 2')
# pops from a6 = S+0x40 (pattern)
for ins, rec in [('popd r4,a6', ['r4', 'r5', 'a6l']), ('popd a2,a6', ['a2l', 'a2h', 'a6l']),
                 ('pop n4,a6', ['n4', 'a6l']), ('popq r4,a6', ['r4', 'r5', 'r6', 'r7', 'a6l']),
                 ('popq a2,a6', ['a2l', 'a2h', 'a3l', 'a3h', 'a6l']), ('popd n4,a6', ['n4', 'n5', 'a6l']),
                 ('popq n4,a6', ['n4', 'n5', 'n6', 'n7', 'a6l']), ('popd g4,a6', ['g4', 'g6', 'a6l']),
                 ('popq g0,a6', ['g0', 'g2', 'g4', 'g6', 'a6l']), ('popf r4,a6', ['r4', 'r5', 'g4', 'a6l'])]:
    case('P ' + ins, ins, rec, 'a6 = S+0x40 ([a6] = 0xB040)')
# ------------------------------------------------------------------ stores (to S+0x80.., a1 = S+0x80)
ST = M80 + ['m85', 'm86', 'm87']
for ins, rec in [('stdu r4,a1,2', ST[:4] + ['a1l']), ('stqu r4,a1,4', ST[:6] + ['a1l']),
                 ('stdu a2,a1,2', ST[:4] + ['a1l']), ('stxu r4,a1', ST[:3] + ['a1l']),
                 ('stdxu a2,a1', ST[:3] + ['a1l']), ('stxu n4,a1', ST[:3] + ['a1l']),
                 ('stdxu r4,a1', ST[:3] + ['a1l']), ('stqxu r4,a1', ST[:5] + ['a1l']),
                 ('std r4,a1', ST[:3]), ('stq r4,a1', ST[:5]), ('std a2,a1', ST[:3]), ('st n4,a1', ST[:2]),
                 ('stx r4,a1', ST), ('stdx r4,a1', ST), ('stqx r4,a1', ST), ('stdx a2,a1', ST), ('stx n4,a1', ST),
                 ('std a2,a1,1', ST[:4]), ('std r4,a1,1', ST[:4]), ('stf r4,a1,1', ST[:5]),
                 ('stfu r4,a1,3', ST[:5] + ['a1l']), ('stxu r4,a1,n2', ST[:3] + ['a1l']),
                 ('stdxu a2,a1,n2', ST[:3] + ['a1l']), ('stxu n4,a1,n2', ST[:3] + ['a1l']),
                 ('stdxu r4,a1,n2', ST[:3] + ['a1l']), ('stqxu r4,a1,n2', ST[:5] + ['a1l']),
                 ('stx r4,a1,n2', ST), ('stdx a2,a1,n2', ST), ('stx n4,a1,n2', ST), ('stdx r4,a1,n2', ST),
                 ('stqx r4,a1,n2', ST)]:
    case('S ' + ins, ins, rec, 'a1 = S+0x80, r4..r7 = FFFD 0005 0007 FFFE, a2 = S+0x20, n4 = 5',
         stores=True, regs={'r6': 0x0007})
for ins, rec in [('stu r4,r6,1', ST[:2] + ['r6']), ('stdu r4,r6,2', ST[:3] + ['r6']), ('st r4,r6', ST[:2]),
                 ('stx r4,r6', ST + ['r6', 'r7']), ('st r4,r6,1', ST[:3]), ('stxu r4,r6', ST + ['r6', 'r7'])]:
    case('S ' + ins, ['mov.l r6,lo(%s + 0x80)' % S, 'mov.l r7,2', 'nop', 'nop'] + [ins], rec,
         'r6 = S+0x80, r7 = 2', stores=True)
# pushes: a6 = S+0x88
for ins, rec in [('pushd r4,a6', ST + ['a6l']), ('pushd a2,a6', ST + ['a6l']), ('push n4,a6', ST + ['a6l']),
                 ('pushq r4,a6', ST + ['m88', 'a6l']), ('pushq a2,a6', ST + ['m88', 'a6l']),
                 ('pushd n4,a6', ST + ['a6l']), ('pushq n4,a6', ST + ['m88', 'a6l']),
                 ('pushd g4,a6', ST + ['a6l']), ('pushq g0,a6', ST + ['m88', 'a6l']),
                 ('pushf r4,a6', ST + ['m88', 'a6l'])]:
    case('U ' + ins, ['mov a6,lo(%s + 0x88)' % S, 'movhw a6,hi(%s + 0x88)' % S, 'nop', 'nop', ins], rec,
         'a6 = S+0x88; written words show the push order', stores=True)
# ------------------------------------------------------------------ ALU on a/n registers
for ins, rec in [('shll n1,3', ['n1']), ('shrl n4,1', ['n4']), ('add a2,r5', ['a2l', 'a2h']),
                 ('sub a2,r5', ['a2l', 'a2h']), ('add.e a2,r8', ['a2l', 'a2h']), ('sub.e a2,r8', ['a2l', 'a2h']),
                 ('add n1,r5', ['n1']), ('sub n1,r5', ['n1']), ('add a2,a3', ['a2l', 'a2h']),
                 ('sub a2,a3', ['a2l', 'a2h']), ('add n1,n2', ['n1']), ('sub n1,n2', ['n1']),
                 ('add a2,n3', ['a2l', 'a2h']), ('sub a2,n3', ['a2l', 'a2h']), ('add n1,1000', ['n1'])]:
    case('A ' + ins, ins, rec, 'a2 = S+0x20, a3 = S+0x30, n1..n4 = 2..5, r5 = 5, r8:r9 = 0x00040003')
# compares: record flags
for ins in ['cmp r4,r5', 'cmp.e r8,1', 'cmp a2,0', 'cmp n1,1', 'cmp n1,r5', 'cmp.e a2,r8', 'cmp.e r8,r10',
            'cmp.f r8,r10', 'cmp r4,1000', 'cmp.e r8,1000']:
    case('C ' + ins, ins, ['%hwflag', '%shwflag'], 'flags after compare')
# ------------------------------------------------------------------ pair / 40-bit ALU
PE = {'r0': 0xFFFF, 'r1': 0x0001, 'r2': 0x0001, 'r3': 0x0000}   # r0:r1 = 0x0001FFFF, r2:r3 = 1
for op in ['add.e', 'sub.e', 'addc.e', 'subc.e', 'min.e', 'max.e', 'iadd.e', 'isub.e', 'and.e', 'or.e', 'xor.e',
           'add.f', 'sub.f']:
    case('E ' + op, '%s r0,r2' % op, ['r0', 'r1', 'g0', '%hwflag'], 'r0:r1 = 0x0001FFFF, r2:r3 = 1', regs=PE)
for ins in ['sxtn.f r0', 'zxtn.f r0', 'sat.f r0', 'norm.e r4,r0', 'abs.e r2,r0', 'neg.e r2,r0', 'round.e r2,r0',
            'not.e r2,r0', 'mov.e r2,r0', 'mov.f r2,r0', 'iadd.e r0,-2', 'iadd.e r0,1000',
            'shll.e r0,4', 'shrl.e r0,4', 'shla.e r0,4', 'shra.e r0,4', 'shrl.f r0,4', 'shll.f r0,4',
            'shll.e r0,r5', 'shrl.e r0,r5', 'shla.e r0,r5', 'shra.e r0,r5', 'shbl.e r0,r7', 'shba.e r0,r7',
            'shbl.f r0,r7', 'shbl r4,r7', 'shba r4,r7']:
    case('E ' + ins, ins, ['r0', 'r1', 'r2', 'r3', 'r4', 'g0', 'g2'],
         'r0:r1 = 0x8001FFFF (g0 = 0x10), r2:r3 = 1, r5 = 5, r7 = -2, r4 = 0xFFFD',
         regs={'r0': 0xFFFF, 'r1': 0x8001, 'r2': 0x0001, 'r3': 0x0000})
for ins, rec in [('xor r4,0x00FF', ['r4']), ('iadd r4,1000', ['r4']), ('and g1,0x0F', ['g0']),
                 ('or g1,0xF0', ['g0']), ('xor g1,0xFF', ['g0']), ('revb r12,4', ['r12']), ('revb r12,15', ['r12']),
                 ('revb r12,r5', ['r12'])]:
    case('I ' + ins, ins, rec, 'r4 = 0xFFFD, g1:g0 = 0x1110, r12 = 0xA5C3, r5 = 5')
# ------------------------------------------------------------------ bit operations
for ins, rec in [('bits %vitr,3', ['%vitr']), ('bitc %vitr,0', ['%vitr']), ('biti %vitr,1', ['%vitr']),
                 ('bits %ip0,4', ['%ip0']), ('bitc %ip0,4', ['%ip0']), ('biti %ip0,5', ['%ip0']),
                 ('bitt g1,0', ['%hwflag', 'r4']), ('bitt g1,1', ['%hwflag', 'r4']),
                 ('bitt %vitr,0', ['%hwflag', 'r4']), ('bitt %vitr,2', ['%hwflag', 'r4']),
                 ('bitt r12,0', ['%hwflag', 'r4']), ('bitt r12,2', ['%hwflag', 'r4'])]:
    tail = ['mov.l r4,0', 'cexe (z,nu) { } !1', 'mov.l r4,1'] if 'bitt' in ins else []
    case('B ' + ins, [ins] + tail, rec, '%vitr = 0x0005, %ip0 = 0x0021, g1 = 0x11, r12 = 0xA5C3; r4 = 1 if Z after bitt',
         setup=['mov.l r13,0x0005', 'mov %vitr,r13', 'mov.l r13,0x0021', 'mov %ip0,r13'])
# ------------------------------------------------------------------ Viterbi
for ins in ['vit_a r2,r8,r10', 'vit_b r2,r8,r10', 'vxor r12', 'vxor.e r8']:
    case('V ' + ins, ins, ['r2', 'r3', 'r8', 'r9', 'r12', '%vitr'],
         'r8:r9 = (3,4), r10:r11 = (5,6), %vitr = 0x0005, r12 = 0xA5C3',
         setup=['mov.l r13,0x0005', 'mov %vitr,r13'])
case('V vit_a x2', ['vit_a r2,r8,r10', 'vit_a r2,r8,r10'], ['r2', 'r3', '%vitr'], 'two steps',
     setup=['mov.l r13,0x0000', 'mov %vitr,r13'])
case('V vit_a neg', 'vit_a r2,r8,r10', ['r2', 'r3', '%vitr'], 'r8:r9 = (7,1), r10:r11 = (1,7)',
     setup=['mov.l r13,0x0000', 'mov %vitr,r13'], regs={'r8': 7, 'r9': 1, 'r10': 1, 'r11': 7})
# ------------------------------------------------------------------ control flow
for cond, neg in [('z', False), ('nz', True)]:
    for kind in ['cexe', 'cexes']:
        for u in ['u', 'nu']:
            case('X %s (%s,%s)' % (kind, cond, u),
                 ['mov.l r4,0', 'mov.l r5,0', 'cmp r4,0', '%s (%s,%s) { } !2' % (kind, cond, u), 'add r4,1',
                  'cmp r4,1', 'mov r6,%hwflag'],
                 ['r4', 'r6', '%hwflag'],
                 'r4 = 1 if executed; r6 = flags after the shadowed cmp')
for cond in ['z', 'nz']:
    for u in ['u', 'nu']:
        case('X cexer (r5,%s,%s)' % (cond, u),
             ['mov.l r4,0', 'mov.l r5,0', 'cexer (r5,%s,%s) { } !2' % (cond, u), 'add r4,1', 'cmp r4,1',
              'mov r6,%hwflag'], ['r4', 'r6', '%hwflag'], 'r5 = 0; r4 = 1 if executed')


def rec_code(item):
    if item[0] == 'r':
        return ['stu %s,a5,1' % item]
    if item[0] == 'a':
        return ['%s r13,%s' % ('movlw' if item[2] == 'l' else 'movhw', item[:2]), 'stu r13,a5,1']
    if item[0] == 'n':
        return ['mov r13,%s' % item, 'stu r13,a5,1']
    if item[0] == 'g':
        return ['mov r13,%s' % item, 'stu r13,a5,1']
    if item[0] == '%':
        return ['mov r13,%s' % item, 'stu r13,a5,1']
    if item[0] == 'm':
        return ['ld r13,a7,0x%X' % (0x200 + int(item[1:], 16)), 'stu r13,a5,1']
    raise ValueError(item)


def main():
    L = ['; generated by gen_t15.py: remaining inferred instructions',
         '        .equ TEST_ID, 0x0015',
         '        .include "common.inc"',
         'OUT = SCRATCH + 0x100',
         '        .include "prologue.inc"',
         '        mov.l r0,0x0080',
         '        mov %tc,r0',
         '        mov a5,lo(OUT)',
         '        movhw a5,hi(OUT)',
         '; memory pattern S+i = 0xB000+i, i < 0x80',
         '        mov a0,lo(SCRATCH)', '        movhw a0,hi(SCRATCH)',
         '        mov.l r1,0xB000',
         '        mov.l r0,0x7F', '        mov %loop0,r0',
         'fill:   stu r1,a0,1', '        add r1,1', '        agn0 fill']
    off = 0
    for i, cs in enumerate(cases):
        L.append('; ---- %d %s: %s' % (i, cs['name'], cs['note']))
        if cs['stores']:
            L.append('        mov.l r13,0xA5A5')
            L += ['        st r13,a7,0x%X' % (0x280 + k) for k in range(0x20)]
        for r in ['r%d' % k for k in range(13)]:
            L.append('        mov.l %s,0x%04X' % (r, cs['regs'][r]))
        for a, o in AREG.items():
            L += ['        mov %s,lo(%s + 0x%X)' % (a, S, o), '        movhw %s,hi(%s + 0x%X)' % (a, S, o)]
        L += ['        mov n%d,%d' % (k, k + 1) for k in range(8)]
        L += ['        mov g%d,0x%02X' % (k, 0x10 + k) for k in range(8)]
        lab = 'c%d' % i
        L += ['        ' + x.replace('{L}', lab) for x in cs['setup']]
        L += ['        nop'] * 4
        L.append('        mov r13,%timer0')
        for x in cs['insn']:
            x = x.replace('{L}', lab)
            L.append(x if x.startswith(lab + ':') else '        ' + x)
        L.append('        mov r14,%timer0')
        L += ['        nop'] * 4
        L.append('        sub r13,r14')
        L.append('        stu r13,a5,1')
        for it in cs['rec']:
            L += ['        ' + x for x in rec_code(it)]
        L += ['        mov.l r14,0x%04X' % (0xC000 + i), '        stu r14,a5,1']
        cs['off'] = off
        off += len(cs['rec']) + 2
    L.append('        .include "epilogue.inc"')
    open('t15_rest.s', 'w').write('\n'.join(L) + '\n')
    json.dump(cases, open('t15_rest.json', 'w'), indent=1)
    print('%d cases, %d result words (DSP 0x400..0x%X)' % (len(cases), off, 0x400 + off))


if __name__ == '__main__':
    main()
