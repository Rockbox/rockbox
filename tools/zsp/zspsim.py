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
Functional ZSP500 simulator for testing DSP code before it runs on the RK27xx.

It decodes with the same zsp500.isa table as zspasm.pl and implements the
behaviour documented in the ZSP500 DSP programming manual,
https://www.rockbox.org/wiki/pub/Main/Rockchip27xx/ZSP500_DSP_Programming_Manual.pdf,
for the instructions listed in OPS (anything else stops with "not
simulated"). It also models the parts of the system that RK27xx DSP code
talks to:

  - DMEM (32K halfwords) and IMEM, the EPR bus window (0xD00000), the EPR
    registers (0xCE0000);
  - "bus memory" (SDRAM, a dict of 32-bit words keyed by byte address);
  - the mailbox (H2C0/H2C0b -> DSP source 4, C2H0/C2H0b -> on_c2h callback);
  - DWDMA channels: writing ChEnReg runs the programmed block transfer at
    once and sets RawTfr (no burst/latency model);
  - interrupts (vector 0x80 - 8n, %imask, %tpc, reti), wait mode
    (%smode bit 13/14), %timer0/%timer1, hardware loops, cexe/cexes/cexer.

Timing is one cycle per instruction plus a fixed cost per bus access; it is
only a rough guide, not a cycle model.
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_ISA = os.path.join(HERE, 'zsp500.isa')
IMAGE_MAGIC = b'ZSP1'           # zspasm.pl image_bytes()
SECT_IMEM, SECT_DMEM = 0, 1

M16, M24, M32 = 0xffff, 0xffffff, 0xffffffff

# ---------------------------------------------------------------- ISA table
# The decoding half of zspasm.pl's reading of zsp500.isa (see the format
# there).

PH_RE = re.compile(r'\{(?P<mul>[24]?)(?P<f>[a-z])(?P<add>\+\d+)?(?::(?P<kind>[\w]+(?:\+\d+)?))?\}')


class Spec:
    def __init__(self, pattern, syntax):
        bits = pattern.replace(' ', '')
        self.size = len(bits) // 16
        self.syntax = syntax
        self.mask = 0
        self.val = 0
        self.fields = {}
        for i, ch in enumerate(bits):
            # chars 0..15 -> inst bits 15..0, chars 16..31 -> inst bits 31..16
            bit = 15 - i if i < 16 else 31 - (i - 16)
            if ch in '01':
                self.mask |= 1 << bit
                self.val |= int(ch) << bit
            elif ch != 'x':
                self.fields.setdefault(ch, []).append(bit)
        # placeholders: (field, mul, add, kind, isreg), isreg when the
        # placeholder is a register number (r{a}, a{b+6}, ...)
        self.phs = []
        pos = 0
        for m in PH_RE.finditer(syntax):
            lit = syntax[pos:m.start()]
            if lit.endswith('0x'):
                lit = lit[:-2]
            kind = m.group('kind') or ''
            isreg = (len(lit) >= 1 and lit[-1] in 'rang' and
                     (len(lit) == 1 or not lit[-2].isalnum()) and kind == '')
            self.phs.append((m.group('f'), m.group('mul'), m.group('add'), kind, isreg))
            pos = m.end()


def load_isa(path):
    specs = []
    tables = {}
    for ln in open(path).read().split('\n'):
        ln = ln.strip()
        m = re.match(r'@(\w+)\s*=\s*(.*)$', ln)
        if m:
            tables[m.group(1)] = m.group(2).split()
        elif ln and not ln.startswith('#'):
            pat, syn = [x.strip() for x in ln.split('|', 1)]
            specs.append(Spec(pat, syn))
    specs.sort(key=lambda s: s.size)
    return specs, tables



def s16(v):
    v &= M16
    return v - 0x10000 if v & 0x8000 else v


def s32(v):
    v &= M32
    return v - (1 << 32) if v >> 31 else v


class SimError(Exception):
    pass


class Inst:
    __slots__ = ('spec', 'f', 'size', 'key', 'word')


class Bus:
    """Bus memory (SDRAM etc.) + mailbox + DWDMA, as seen at bus addresses."""
    DMEM_BUS = 0x30000000
    MBOX = 0x18088000
    DWDMA = 0x182F0000

    def __init__(self, sim):
        self.sim = sim
        self.mem = {}                 # byte address (aligned 4) -> 32-bit word
        self.h2c = [0] * 4
        self.h2cb = [0] * 4
        self.h2c_sta = 0
        self.c2h = [0] * 4
        self.on_c2h = None
        self.dma = {}
        self.raw_tfr = 0
        self.chen = 0

    # ---------------------------------------------------- word access
    def read32(self, a):
        if self.DMEM_BUS <= a < self.DMEM_BUS + 0x10000:
            i = (a - self.DMEM_BUS) // 2
            return self.sim.dmem[i] | (self.sim.dmem[i + 1] << 16)
        if self.MBOX <= a < self.MBOX + 0x100:
            return self._mbox_read(a - self.MBOX)
        if self.DWDMA <= a < self.DWDMA + 0x400:
            return self._dma_read(a - self.DWDMA)
        return self.mem.get(a & ~3, 0)

    def write32(self, a, v):
        v &= M32
        if self.DMEM_BUS <= a < self.DMEM_BUS + 0x10000:
            i = (a - self.DMEM_BUS) // 2
            self.sim.dmem[i] = v & M16
            self.sim.dmem[i + 1] = v >> 16
            return
        if self.MBOX <= a < self.MBOX + 0x100:
            return self._mbox_write(a - self.MBOX, v)
        if self.DWDMA <= a < self.DWDMA + 0x400:
            return self._dma_write(a - self.DWDMA, v)
        self.mem[a & ~3] = v

    # ---------------------------------------------------- mailbox (ARM offsets)
    def arm_post(self, ch, data, cmd):
        """ARM writes H2Cx data then H2Cxb: raises DSP source 4."""
        self.h2c[ch] = data & M32
        self.h2cb[ch] = cmd & M32
        self.h2c_sta |= 1 << ch
        self.sim.ireq |= 1 << 4

    def _mbox_read(self, off):
        if off == 0x00:
            return 0xA7
        if off == 0x10:
            return self.h2c_sta
        if 0x20 <= off < 0x40:
            ch, cmd = (off - 0x20) // 8, (off - 0x20) % 8
            if cmd:
                self.h2c_sta &= ~(1 << ch)
                if not self.h2c_sta:
                    self.sim.ireq &= ~(1 << 4)
                return self.h2cb[ch]
            return self.h2c[ch]
        return 0

    def _mbox_write(self, off, v):
        if 0x50 <= off < 0x70:
            ch, cmd = (off - 0x50) // 8, (off - 0x50) % 8
            if cmd:
                if self.on_c2h:
                    self.on_c2h(ch, self.c2h[ch], v)
            else:
                self.c2h[ch] = v

    # ---------------------------------------------------- DWDMA (32-bit regs)
    def _dma_read(self, off):
        if off == 0x2C0:
            return self.raw_tfr
        if off == 0x3A0:
            return self.chen
        return self.dma.get(off, 0)

    def _dma_write(self, off, v):
        if off == 0x338:                         # ClearTfr
            self.raw_tfr &= ~v
            return
        if off == 0x3A0:                         # ChEnReg: bits 15:8 write enable
            for ch in range(4):
                if v & (0x100 << ch):
                    if v & (1 << ch):
                        self._run(ch)
                    else:
                        self.chen &= ~(1 << ch)
            return
        self.dma[off] = v

    def _run(self, ch):
        base = 0x58 * ch
        sar = self.dma.get(base + 0x00, 0)
        dar = self.dma.get(base + 0x08, 0)
        ctl_l = self.dma.get(base + 0x18, 0)
        n = self.dma.get(base + 0x1C, 0) & 0xfff
        if ctl_l & (1 << 17):
            raise SimError('DWDMA ch%d: source gather set (channel would stall)' % ch)
        if (ctl_l >> 1) & 7 != 2 or (ctl_l >> 4) & 7 != 2:
            raise SimError('DWDMA ch%d: only 32-bit transfers are simulated' % ch)
        for i in range(n):
            self.write32(dar + 4 * i, self.read32(sar + 4 * i))
        self.sim.cycles += 10 * n
        self.raw_tfr |= 1 << ch
        if self.dma.get(0x310, 0) & (1 << ch):   # MaskTfr unmasked
            self.sim.ireq |= 1 << 3


class Sim:
    def __init__(self, isa=DEFAULT_ISA):
        self.specs, self.tables = load_isa(isa)
        self.dec16 = {}
        self.dmem = [0] * 0x8000
        self.imem = [0] * 0x8000
        self.bus = Bus(self)
        self.epr = [0, 1, 2, 4, 8, 0x10, 0x20, 0x40]
        self.reset()
        self.cache = {}

    def reset(self):
        self.r = [0] * 16
        self.g = [0] * 8
        self.a = [0] * 8
        self.n = [0] * 8
        self.c = {k: 0 for k in ('%loop0', '%loop1', '%loop2', '%loop3', '%hwflag', '%shwflag',
                                 '%imask', '%ireq', '%smode', '%psmode', '%amode', '%fmode', '%tc',
                                 '%ip0', '%ip1', '%rpc', '%tpc', '%vitr', '%ded', '%dei')}
        self.c['%timer0'] = self.c['%timer1'] = 0xffff
        self.pc = 0
        self.cycles = 0
        self.insts = 0
        self.shadow = 0          # instructions left in a cexe shadow
        self.skip = False
        self.update = False
        self.waiting = False

    @property
    def ireq(self):
        return self.c['%ireq']

    @ireq.setter
    def ireq(self, v):
        self.c['%ireq'] = v & M16

    # ------------------------------------------------------------ loading
    def load_image(self, data):
        import struct
        if data[:4] != IMAGE_MAGIC:
            raise SimError('not a ZSP1 image')
        _, nsect, _ = struct.unpack_from('<HHI', data, 4)
        pos = 12
        for _ in range(nsect):
            kind, _, start, count = struct.unpack_from('<HHII', data, pos)
            pos += 12
            words = struct.unpack_from('<%dH' % count, data, pos)
            pos += 2 * count
            pos = (pos + 3) & ~3
            mem = self.imem if kind == SECT_IMEM else self.dmem
            mem[start:start + count] = list(words)
        self.cache = {}

    # ------------------------------------------------------------ decode
    def decode(self, pc):
        hit = self.cache.get(pc)
        if hit is not None:
            return hit
        w0 = self.imem[pc & 0x7fff]
        w1 = self.imem[(pc + 1) & 0x7fff]
        size = 2 if (w0 >> 12) == 0xF else 1
        word = w0 | (w1 << 16) if size == 2 else w0
        # the most specific pattern wins (e.g. 'br' over '{c:br_cond}' with c = 0)
        best = None
        for s in self.specs:
            if s.size == size and (word & s.mask) == s.val:
                if best is None or bin(s.mask).count('1') > bin(best.mask).count('1'):
                    best = s
        if best is None or (best.syntax.startswith('{c:br_cond}') and
                            self.tables['br_cond'][self._field(best, word, 'c')] == '-'):
            raise SimError('invalid opcode %04X at %04X' % (w0, pc))
        s = best
        f = {}
        for ch, bits in s.fields.items():
            v = 0
            for b in bits:
                v = (v << 1) | ((word >> b) & 1)
            f[ch] = v
        ins = Inst()
        ins.spec, ins.f, ins.size, ins.word = s, f, size, word
        ins.key = (s.syntax, size)
        self.cache[pc] = ins
        return ins

    @staticmethod
    def _field(spec, word, ch):
        v = 0
        for b in spec.fields[ch]:
            v = (v << 1) | ((word >> b) & 1)
        return v

    def ops(self, ins):
        """Operand values in syntax order: ints, register numbers, table names."""
        out = []
        for f, mul, add, kind, isreg in ins.spec.phs:
            v = ins.f[f]
            n = len(ins.spec.fields[f])
            if kind and kind != 's' and not kind.startswith('rel'):
                tname, _, off = kind.partition('+')
                out.append(self.tables[tname][v + int(off or 0)])
                continue
            if kind == 's':
                v = v - (1 << n) if v >> (n - 1) else v
            if mul:
                v *= int(mul)
            if add:
                v += int(add[1:])
            out.append(v)
        return out

    # ------------------------------------------------------------ data space
    def dread(self, addr):
        addr &= M24
        self.cycles += 0 if addr < 0x8000 else 40
        if addr < 0x8000:
            return self.dmem[addr]
        if 0xCE0000 <= addr < 0xD00000:
            return self.epr[addr & 7]
        if 0xD00000 <= addr < 0xE00000:
            ba = self.busaddr(addr)
            w = self.bus.read32(ba & ~3)
            return (w >> 16) & M16 if ba & 2 else w & M16
        return 0

    def dwrite(self, addr, v):
        addr &= M24
        v &= M16
        if addr < 0x8000:
            self.dmem[addr] = v
        elif 0xCE0000 <= addr < 0xD00000:
            self.epr[addr & 7] = v & 0x7ff
        elif 0xD00000 <= addr < 0xE00000:
            ba = self.busaddr(addr)
            w = self.bus.read32(ba & ~3)
            w = (w & 0xffff0000) | v if not ba & 2 else (w & 0xffff) | (v << 16)
            self.bus.write32(ba & ~3, w)

    def busaddr(self, addr):
        off = addr - 0xD00000
        return (self.epr[(off >> 17) & 7] << 21) | (off << 1)

    def dread32(self, addr):
        addr &= M24
        if 0xD00000 <= addr < 0xE00000:      # one 32-bit bus access
            self.cycles += 40
            return self.bus.read32(self.busaddr(addr))
        return self.dread(addr) | (self.dread(addr + 1) << 16)

    def dwrite32(self, addr, v):
        addr &= M24
        if 0xD00000 <= addr < 0xE00000:
            self.bus.write32(self.busaddr(addr), v)
            return
        self.dwrite(addr, v)
        self.dwrite(addr + 1, v >> 16)

    # ------------------------------------------------------------ registers
    def rp(self, k):                  # 32-bit pair r(k+1):r(k)
        return s32(self.r[k] | (self.r[k + 1] << 16))

    def setrp(self, k, v):
        self.r[k] = v & M16
        self.r[k + 1] = (v >> 16) & M16

    def creg_get(self, name):
        if name == '%pc':
            return self.pc
        return self.c[name]

    def creg_set(self, name, v):
        if name == '%pc':
            self.next_pc = v & M24
            return
        if name in ('%vitr',):
            return
        if name in ('%rpc', '%tpc'):
            v &= M24
        else:
            v &= M16
        if name == '%ireq':
            v |= self.c['%ireq'] & 0     # software can set/clear all bits
        self.c[name] = v

    def flags_cmp(self, a, b, bits):
        mask = (1 << bits) - 1
        sa = a - ((a & (1 << (bits - 1))) << 1)
        sb = b - ((b & (1 << (bits - 1))) << 1)
        fl = self.c['%hwflag'] & ~((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6))
        if (a & mask) == (b & mask):
            fl |= 1 << 3
        if sa > sb:
            fl |= 1 << 4
        if sa >= sb:
            fl |= 1 << 5
        if (a & mask) >= (b & mask):
            fl |= 1 << 6
        self.c['%hwflag'] = fl

    def flags_res(self, v, bits):
        sv = v - ((v & (1 << (bits - 1))) << 1)
        fl = self.c['%hwflag'] & ~((1 << 3) | (1 << 4) | (1 << 5))
        if sv == 0:
            fl |= 1 << 3
        if sv > 0:
            fl |= 1 << 4
        if sv >= 0:
            fl |= 1 << 5
        self.c['%hwflag'] = fl

    def cond(self, name, flags=None):
        fl = self.c['%hwflag'] if flags is None else flags
        z, gt, ge, c, v = (fl >> 3) & 1, (fl >> 4) & 1, (fl >> 5) & 1, (fl >> 6) & 1, (fl >> 10) & 1
        sv = (fl >> 8) & 1
        return {'z': z, 'nz': not z, 'ge': ge, 'lt': not ge, 'gt': gt, 'le': not gt,
                'v': v, 'nv': not v, 'c': c, 'nc': not c, 'sv': sv, 'nsv': not sv,
                'gv': 0, 'ngv': 1, 'gsv': 0, 'ngsv': 1}[name]

    # ------------------------------------------------------------ run
    def take_interrupt(self):
        req = self.c['%ireq']
        if req & 0x8000:
            src = 15
        else:
            if not self.c['%imask'] & 0x8000:
                return False
            pend = req & self.c['%imask'] & 0x7fff
            if not pend:
                return False
            src = max(i for i in range(15) if pend >> i & 1)
        self.c['%tpc'] = self.pc
        self.c['%ireq'] &= ~(1 << src)
        im = self.c['%imask']
        self.c['%imask'] = (im & ~0xC000) | ((im >> 1) & 0x4000)
        self.c['%psmode'] = self.c['%smode'] & 0x0433
        self.c['%smode'] &= ~0x0433
        self.pc = 0x80 - 8 * src if src < 15 else 0x08
        self.waiting = False
        self.cycles += 4
        return True

    def tick_timers(self, n):
        for t, bit, irq in (('%timer0', 0x80, 5), ('%timer1', 0x8000, 6)):
            if self.c['%tc'] & bit and self.c[t]:
                v = self.c[t] - n
                if v <= 0:
                    self.c[t] = 0
                    self.c['%tc'] &= ~bit
                    self.c['%ireq'] |= 1 << irq
                else:
                    self.c[t] = v

    def run(self, max_insts=50_000_000, until=None):
        """Run until wait mode with nothing to wake it, until(sim) is true,
        or a 'br .' self-loop. Returns the reason."""
        n = 0
        while n < max_insts:
            if until and until(self):
                return 'until'
            if self.shadow == 0 and self.take_interrupt():
                pass
            elif self.waiting:
                # idle: the caller decides what wakes the core next
                return 'wait'
            pc = self.pc
            ins = self.decode(pc)
            self.next_pc = (pc + ins.size) & M24
            if self.shadow:
                self.shadow -= 1
                if self.skip:
                    if self.update:
                        self.exec_postmod_only(ins)
                    self.pc = self.next_pc
                    n += 1
                    continue
            h = OPS.get(ins.key[0])
            if h is None:
                raise SimError('not simulated: %s (%04X) at %04X' % (ins.spec.syntax, ins.word & M16, pc))
            h(self, ins, self.ops(ins))
            n += 1
            self.insts += 1
            self.cycles += 1
            self.tick_timers(1)
            if self.next_pc == pc and not self.shadow:
                self.pc = pc
                return 'halt'
            self.pc = self.next_pc
        return 'limit'

    def exec_postmod_only(self, ins):
        h = POSTMOD.get(ins.key[0])
        if h:
            h(self, ins, self.ops(ins))


# ================================================================ semantics
OPS = {}
POSTMOD = {}


def op(*syntaxes):
    def deco(fn):
        for s in syntaxes:
            OPS[s] = fn
        return fn
    return deco


def postmod(*syntaxes):
    def deco(fn):
        for s in syntaxes:
            POSTMOD[s] = fn
        return fn
    return deco


@op('nop')
def _nop(s, i, o):
    pass


# ---- branches
@op('{c:br_cond} {o:rel8}', '{c:br_cond} {o:rel16}')
def _bcc(s, i, o):
    name = o[0]
    raw = i.f['o']
    if i.size == 1:
        off = raw - 256 if name.startswith('agn') else (raw - 256 if raw & 0x80 else raw)
    else:
        off = raw - 65536 if name.startswith('agn') or raw & 0x8000 else raw
    tgt = (s.pc + off) & M24
    if name.startswith('agn'):
        k = '%loop' + name[3]
        if s.c[k]:
            s.c[k] -= 1
            s.next_pc = tgt
        return
    if s.cond({'bz': 'z', 'bnz': 'nz', 'bge': 'ge', 'blt': 'lt', 'bgt': 'gt', 'ble': 'le',
               'bov': 'v', 'bnov': 'nv', 'bc': 'c', 'bnc': 'nc'}[name]):
        s.next_pc = tgt


@op('br {o:rel16}')
def _br(s, i, o):
    raw = i.f['o']
    s.next_pc = (s.pc + (raw - 65536 if raw & 0x8000 else raw)) & M24


@op('call {o:rel16}')
def _call(s, i, o):
    raw = i.f['o']
    s.c['%rpc'] = s.next_pc
    s.next_pc = (s.pc + (raw - 65536 if raw & 0x8000 else raw)) & M24


@op('call r{a}')
def _callr(s, i, o):
    s.c['%rpc'] = s.next_pc
    s.next_pc = s.r[o[0]]


@op('call a{a}')
def _calla(s, i, o):
    s.c['%rpc'] = s.next_pc
    s.next_pc = s.a[o[0]] & M16


@op('ret')
def _ret(s, i, o):
    s.next_pc = s.c['%rpc']


@op('reti')
def _reti(s, i, o):
    s.next_pc = s.c['%tpc']
    s.c['%tpc'] = 0
    im = s.c['%imask']
    s.c['%imask'] = (im & ~0xC000) | ((im << 1) & 0x8000)
    s.c['%smode'] = (s.c['%smode'] & ~0x433) | s.c['%psmode']
    s.c['%psmode'] = 0


# ---- conditional execution
def _shadow(s, n, run, upd):
    s.shadow = n
    s.skip = not run
    s.update = upd


@op('cexe ({c:cexe_cond},u) { } !{n+1}', 'cexe ({c:cexe_cond},nu) { } !{n+1}')
def _cexe(s, i, o):
    s.c['%shwflag'] = s.c['%hwflag']
    _shadow(s, o[1], s.cond(o[0]), ',u)' in i.spec.syntax)


@op('cexes ({c:cexe_cond},u) { } !{n+1}', 'cexes ({c:cexe_cond},nu) { } !{n+1}')
def _cexes(s, i, o):
    _shadow(s, o[1], s.cond(o[0], s.c['%shwflag']), ',u)' in i.spec.syntax)


@op('cexer (r{a},z,u) { } !{n+1}', 'cexer (r{a},z,nu) { } !{n+1}',
    'cexer (r{a},nz,u) { } !{n+1}', 'cexer (r{a},nz,nu) { } !{n+1}')
def _cexer(s, i, o):
    zero = s.r[o[0]] == 0
    run = zero if ',z,' in i.spec.syntax else not zero
    _shadow(s, o[1], run, ',u)' in i.spec.syntax)


# ---- moves
@op('mov r{a},r{b}')
def _movrr(s, i, o):
    s.r[o[0]] = s.r[o[1]]


@op('mov r{a},0x{b}', 'mov r{a},0x{b:s}')
def _movri(s, i, o):
    s.r[o[0]] = o[1] & M16


@op('mov.e r{2a},r{2b}')
def _move(s, i, o):
    s.setrp(o[0], s.rp(o[1]))


@op('mov a{a},0x{b}')
def _movai(s, i, o):
    s.a[o[0]] = o[1] & M16


@op('movhw a{a},0x{b}')
def _movhwai(s, i, o):
    s.a[o[0]] = (s.a[o[0]] & M16) | ((o[1] & 0xff) << 16)


@op('movlw a{a},0x{b}')
def _movlwai(s, i, o):
    s.a[o[0]] = (s.a[o[0]] & 0xff0000) | (o[1] & M16)


@op('movhw a{a},r{b}')
def _movhwar(s, i, o):
    s.a[o[0]] = (s.a[o[0]] & M16) | ((s.r[o[1]] & 0xff) << 16)


@op('movlw a{a},r{b}')
def _movlwar(s, i, o):
    s.a[o[0]] = (s.a[o[0]] & 0xff0000) | s.r[o[1]]


@op('movlw r{a},a{b}')
def _movlwra(s, i, o):
    s.r[o[0]] = s.a[o[1]] & M16


@op('movhw r{a},a{b}')
def _movhwra(s, i, o):
    s.r[o[0]] = (s.a[o[1]] >> 16) & 0xff


@op('mov.e a{a},r{2b}')
def _movear(s, i, o):
    s.a[o[0]] = s.rp(o[1]) & M24


@op('mov.e r{2a},a{b}')
def _movera(s, i, o):
    s.setrp(o[0], s.a[o[1]])


@op('mov a{a},a{b}')
def _movaa(s, i, o):
    s.a[o[0]] = s.a[o[1]]


@op('mov n{a},r{b}')
def _movnr(s, i, o):
    s.n[o[0]] = s.r[o[1]]


@op('mov n{a},0x{b}')
def _movni(s, i, o):
    s.n[o[0]] = o[1] & M16


@op('mov r{a},n{b}')
def _movrn(s, i, o):
    s.r[o[0]] = s.n[o[1]]


@op('mov r{a},{b:creg_e7e4}')
def _movrc(s, i, o):
    s.r[o[0]] = s.creg_get(o[1]) & M16


@op('mov {a:creg_e724},r{b}', 'mov {a:creg_e664},r{b}')
def _movcr(s, i, o):
    s.creg_set(o[0], s.r[o[1]])


@op('mov {a:creg_e684},0x{b}')
def _movci(s, i, o):
    s.creg_set(o[0], o[1])


@op('mov.e r{2a},{b:creg_e6c4}')
def _moverc(s, i, o):
    s.setrp(o[0], s.creg_get(o[1]))


@op('mov.e {a:creg_e6c4},r{2b}')
def _movecr(s, i, o):
    s.creg_set(o[0], s.rp(o[1]) & M24)


@op('mov {a:creg_e674},n{b}')
def _movloopn(s, i, o):
    s.creg_set(o[0], s.n[o[1]])


@op('bits {a:creg_e664},{b}', 'bits {a:creg_e6a4},{b}')
def _bits(s, i, o):
    s.creg_set(o[0], s.creg_get(o[0]) | (1 << o[1]))
    if o[0] == '%smode' and o[1] in (13, 14):
        s.waiting = True
        s.c['%smode'] &= ~(1 << o[1])


@op('bitc {a:creg_e664},{b}', 'bitc {a:creg_e6a4},{b}')
def _bitc(s, i, o):
    s.creg_set(o[0], s.creg_get(o[0]) & ~(1 << o[1]))


# ---- 16-bit ALU
def _alu16(fn, setflags=True):
    def h(s, i, o):
        v = fn(s.r[o[0]], s.r[o[1]] if isinstance(o[1], int) and 'r{b}' in i.spec.syntax else o[1]) & M16
        s.r[o[0]] = v
        if setflags:
            s.flags_res(v, 16)
    return h


OPS['add r{a},r{b}'] = _alu16(lambda a, b: a + b)
OPS['sub r{a},r{b}'] = _alu16(lambda a, b: a - b)
OPS['and r{a},r{b}'] = _alu16(lambda a, b: a & b)
OPS['or r{a},r{b}'] = _alu16(lambda a, b: a | b)
OPS['xor r{a},r{b}'] = _alu16(lambda a, b: a ^ b)
OPS['add r{a},{b:s}'] = _alu16(lambda a, b: a + b)
OPS['and r{a},0x{b}'] = _alu16(lambda a, b: a & b)
OPS['or r{a},0x{b}'] = _alu16(lambda a, b: a | b)
OPS['xor r{a},0x{b}'] = _alu16(lambda a, b: a ^ b)
OPS['shll r{a},{b}'] = _alu16(lambda a, b: a << b)
OPS['shrl r{a},{b}'] = _alu16(lambda a, b: a >> b)
OPS['shra r{a},{b}'] = _alu16(lambda a, b: s16(a) >> b)


@op('cmp r{a},r{b}')
def _cmprr(s, i, o):
    s.flags_cmp(s.r[o[0]], s.r[o[1]], 16)


@op('cmp r{a},{b:cmp_imm}', 'cmp r{a},{b:s}')
def _cmpri(s, i, o):
    s.flags_cmp(s.r[o[0]], int(o[1]) & M16, 16)


# ---- 32-bit pair ALU
def _alu32(fn):
    def h(s, i, o):
        v = fn(s.rp(o[0]), s.rp(o[1]) if 'r{2b}' in i.spec.syntax else o[1])
        s.setrp(o[0], v)
        s.flags_res(v & M32, 32)
    return h


OPS['add.e r{2a},r{2b}'] = _alu32(lambda a, b: a + b)
OPS['sub.e r{2a},r{2b}'] = _alu32(lambda a, b: a - b)
OPS['shll.e r{2a},{b}'] = _alu32(lambda a, b: a << b)
OPS['shra.e r{2a},{b}'] = _alu32(lambda a, b: a >> b)
OPS['shrl.e r{2a},{b}'] = _alu32(lambda a, b: (a & M32) >> b)


@op('cmp.e r{2a},r{2b}')
def _cmpe(s, i, o):
    s.flags_cmp(s.rp(o[0]) & M32, s.rp(o[1]) & M32, 32)


# ---- address / index registers
@op('add a{a},{b:s}')
def _addai(s, i, o):
    s.a[o[0]] = (s.a[o[0]] + o[1]) & M24


@op('add n{a},{b:s}')
def _addni(s, i, o):
    s.n[o[0]] = (s.n[o[0]] + o[1]) & M16


@op('add a{a},r{b}')
def _addar(s, i, o):
    s.a[o[0]] = (s.a[o[0]] + s16(s.r[o[1]])) & M24


@op('add a{a},n{b}')
def _addan(s, i, o):
    s.a[o[0]] = (s.a[o[0]] + s16(s.n[o[1]])) & M24


@op('add a{a},a{b}')
def _addaa(s, i, o):
    s.a[o[0]] = (s.a[o[0]] + s.a[o[1]]) & M24


@op('sub a{a},a{b}')
def _subaa(s, i, o):
    s.a[o[0]] = (s.a[o[0]] - s.a[o[1]]) & M24


# ---- multiply
@op('mul r{a},r{b},r{c}', 'mul r{2a},r{b},r{c}')
def _mul(s, i, o):
    s.setrp(o[0], s16(s.r[o[1]]) * s16(s.r[o[2]]))


@op('mulsu r{a},r{b},r{c}')
def _mulsu(s, i, o):
    s.setrp(o[0], s16(s.r[o[1]]) * s.r[o[2]])


@op('mac r{a},r{b},r{c}', 'mac r{2a},r{b},r{c}')
def _mac(s, i, o):
    s.setrp(o[0], s.rp(o[0]) + s16(s.r[o[1]]) * s16(s.r[o[2]]))


@op('macsu r{a},r{b},r{c}')
def _macsu(s, i, o):
    s.setrp(o[0], s.rp(o[0]) + s16(s.r[o[1]]) * s.r[o[2]])


@op('macn r{a},r{b},r{c}', 'macn r{2a},r{b},r{c}')
def _macn(s, i, o):
    s.setrp(o[0], s.rp(o[0]) - s16(s.r[o[1]]) * s16(s.r[o[2]]))


# ---- loads / stores
MODS = {'2': 2, '1': 1, '-2': -2, '-1': -1, '4': 4, '-4': -4, '3': 3, '-3': -3}


@op('ld r{a},a{b}')
def _ld(s, i, o):
    s.r[o[0]] = s.dread(s.a[o[1]])


@op('ld r{a},a{b},{c:s}', 'ld r{a},a{b+6},{c}')
def _ldo(s, i, o):
    s.r[o[0]] = s.dread(s.a[o[1]] + o[2])


@op('st r{a},a{b}')
def _st(s, i, o):
    s.dwrite(s.a[o[1]], s.r[o[0]])


@op('st r{a},a{b},{c:s}', 'st r{a},a{b+6},{c}')
def _sto(s, i, o):
    s.dwrite(s.a[o[1]] + o[2], s.r[o[0]])


@op('ldd r{2a},a{b}')
def _ldd(s, i, o):
    s.setrp(o[0], s.dread32(s.a[o[1]]))


@op('ldd r{2a},a{b},{c:s}', 'ldd r{2a},a{b+6},{c}')
def _lddo(s, i, o):
    s.setrp(o[0], s.dread32(s.a[o[1]] + o[2]))


@op('std r{2a},a{b}')
def _std(s, i, o):
    s.dwrite32(s.a[o[1]], s.rp(o[0]))


@op('std r{2a},a{b},{c:s}', 'std r{2a},a{b+6},{c}')
def _stdo(s, i, o):
    s.dwrite32(s.a[o[1]] + o[2], s.rp(o[0]))


@op('ldu r{a},a{b},{c:mod_u}')
def _ldu(s, i, o):
    s.r[o[0]] = s.dread(s.a[o[1]])
    _pm(s, i, o)


@op('lddu r{2a},a{b},{c:mod_d}')
def _lddu(s, i, o):
    s.setrp(o[0], s.dread32(s.a[o[1]]))
    _pm(s, i, o)


@op('stu r{a},a{b},{c:mod_u}')
def _stu(s, i, o):
    s.dwrite(s.a[o[1]], s.r[o[0]])
    _pm(s, i, o)


@op('stdu r{2a},a{b},{c:mod_d}')
def _stdu(s, i, o):
    s.dwrite32(s.a[o[1]], s.rp(o[0]))
    _pm(s, i, o)


@postmod('ldu r{a},a{b},{c:mod_u}', 'lddu r{2a},a{b},{c:mod_d}', 'stu r{a},a{b},{c:mod_u}',
         'stdu r{2a},a{b},{c:mod_d}')
def _pm(s, i, o):
    s.a[o[1]] = (s.a[o[1]] + MODS[o[2]]) & M24


@op('push r{a},a{b+6}')
def _push(s, i, o):
    s.dwrite(s.a[o[1]], s.r[o[0]])
    s.a[o[1]] = (s.a[o[1]] - 1) & M24


@op('pop r{a},a{b+6}')
def _pop(s, i, o):
    s.a[o[1]] = (s.a[o[1]] + 1) & M24
    s.r[o[0]] = s.dread(s.a[o[1]])


@op('pushd r{2a},a{b+6}')
def _pushd(s, i, o):
    s.dwrite32(s.a[o[1]] - 1, s.rp(o[0]))
    s.a[o[1]] = (s.a[o[1]] - 2) & M24


@op('popd r{2a},a{b+6}')
def _popd(s, i, o):
    s.setrp(o[0], s.dread32(s.a[o[1]] + 1))
    s.a[o[1]] = (s.a[o[1]] + 2) & M24


@op('pushd a{a},a{b+6}')
def _pushda(s, i, o):
    s.dwrite32(s.a[o[1]] - 1, s.a[o[0]])
    s.a[o[1]] = (s.a[o[1]] - 2) & M24


@op('popd a{a},a{b+6}')
def _popda(s, i, o):
    s.a[o[0]] = s.dread32(s.a[o[1]] + 1) & M24
    s.a[o[1]] = (s.a[o[1]] + 2) & M24


def main():
    import argparse
    ap = argparse.ArgumentParser(description='run a ZSP image until it halts or waits')
    ap.add_argument('image')
    ap.add_argument('-n', type=int, default=10_000_000)
    a = ap.parse_args()
    s = Sim()
    s.load_image(open(a.image, 'rb').read())
    why = s.run(a.n)
    print('stopped (%s) at %04X after %d instructions, %d cycles' % (why, s.pc, s.insts, s.cycles))
    print('r: ' + ' '.join('%04x' % v for v in s.r))
    print('a: ' + ' '.join('%06x' % v for v in s.a))


if __name__ == '__main__':
    main()
