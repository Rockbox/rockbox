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
  - DWDMA channels: writing ChEnReg copies the programmed block at once, but
    RawTfr (and DSP source 3) follow only after the transfer's time;
  - interrupts (vector 0x80 - 8n, %imask, %tpc, reti), wait mode
    (%smode bit 13/14), %timer0/%timer1, hardware loops, cexe/cexes/cexer.

Timing (see TIMING below) is a model of the in-order 4-issue core in whole
cycles, fitted to the RK27xx benchmarks; bus, write buffer and DMA times are
in DSP and HCLK cycles (Sim(dsp_hz=..., hclk_hz=...)). %timer0/%timer1
count these cycles.
"""
import collections
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
        # placeholders: (field, mul, add, kind, isreg, letter), isreg when
        # the placeholder is a register number (r{a}, a{b+6}, ...) of the
        # register file letter ('r', 'a', 'n', 'g')
        self.phs = []
        pos = 0
        for m in PH_RE.finditer(syntax):
            lit = syntax[pos:m.start()]
            if lit.endswith('0x'):
                lit = lit[:-2]
            kind = m.group('kind') or ''
            isreg = (len(lit) >= 1 and lit[-1] in 'rang' and
                     (len(lit) == 1 or not lit[-2].isalnum()) and kind == '')
            self.phs.append((m.group('f'), m.group('mul'), m.group('add'), kind, isreg,
                             lit[-1] if isreg else ''))
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


# ---------------------------------------------------------------- timing
# In-order 4-issue superscalar, in whole cycles. Up to 4 instructions issue
# per cycle, those on register pairs or quads within the same budget; on
# top of that each unit has a capacity per cycle (CAP) of which an
# instruction takes a share. An instruction issues in the first cycle, not
# before the previous instruction's, where its slots and units have room
# and its sources are ready. Fitted to the cycle benchmarks of the DSP test
# suite (utils/hwstub/tools/lua/rk27xx/zsp, tools/gen_bench.py; measured at
# 160 MHz) and the manual's sections 7.1-7.3; the
# comments give the measurements (cycles per instruction or group, or per
# pass of 8 groups + agn0).
ISSUE_SLOTS = 4                 # nop 0.25; 8 x ldd + agn0 4.07
# The ALU capacity is shared: a 16-bit op takes 2 of 6, one on pairs (.e) 3
# (add x 4 0.33 each; mov.e x 2 1.01 per pair; add.e + add 1.01;
# mov.e x 2 + add x 2 2.02, the adds not beside the mov.es; dct_bfly 2.15).
CAP = {'alu': 6,
       'agu': 2,                # ld 0.5, mov a,a 0.5, mov a + ld 1 per pair
       'st': 1,                 # st 1, ld + st 1 per pair
       'mac': 1}                # mac with two accumulators 1.5 (latency)
# class: (units as (unit, share), result latency, block). block: the next
# instruction issues at least this many cycles later.
TIMING = {
    'nop':   ((), 0, 0),
    'alu':   ((('alu', 2),), 1, 0),         # dependent add 1
    'alue':  ((('alu', 3),), 1, 0),         # dependent add.e 1
    'agu':   ((('agu', 1),), 1, 0),         # add a,imm chain 1, mov a then ld no stall
    'mac':   ((('mac', 1),), 2, 0),         # mac chain 2, mul r0,r1,r2 2 (reads r1)
    'ld':    ((('agu', 1),), 1, 0),         # load-use no stall; ldu chain on a 1
    'st':    ((('agu', 1), ('st', 1)), 0, 0),
    'stack': ((('agu', 1), ('st', 1)), 1, 3),   # push/pop 3
    'br':    ((), 0, 1),                    # br 1
    'brc':   ((), 0, 1),                    # cmp + bz taken or not 1
    'loop':  ((), 0, 1),                    # agnN: nop 3.09, cmp+nop 5.09, cexe 9.18
    'call':  ((), 0, 2),                    # call + ret 5
    'ret':   ((), 0, 3),
    'cexe':  ((), 0, 1),                    # cmp; cexe; add 1.2
    'cwr':   ((), 4, 2),                    # creg write 2, write then read 4
}
ST_LD_SAME = 5                  # st then ld of the same address: 5 per pair
                                # (not for push/pop: 3 each)
AGN_AFTER_BRC = 6               # agnN just after a conditional branch: bz 14.27
# The core's own IMEM and DMEM are tightly coupled: an access takes the
# core cycles of TIMING above at any DSP clock and HCLK (t30 int_ld 0.44,
# int_st 0.5 per access at every clock measured). Everything else goes
# over the AHB and costs part DSP cycles, part HCLK cycles (the AHB clock,
# which the SDRAM runs at), fitted to t30_bustime, t35_dma and t36_dma2 run
# at DSP clocks of 18..160 MHz with HCLK 100 and 50 MHz (rk27xx/zsp.lua
# ZSP.sweep(), the suite's tools/bus_sweep.py): the HCLK part doubles when
# HCLK halves.
# Comments: measured at DSP 18 / 160 MHz with HCLK 50 (and 100).
#
# Regions (Bus.region()): 'dmem_ahb' is DMEM reached through the AHB at
# bus address 0x30000000, through its slave port as the ARM and the DMA
# reach it: a DSP access through the EPR bus window, or a DMA transfer to
# or from DMEM. Not the core's own DMEM port, which is free of all this.
# 'mbox' the mailbox, 'dmareg' the DWDMA registers, 'sdram' anything else.
#
# load through the bus, the core waits for it: (DSP cycles, HCLK cycles)
BUS_LOAD = {'dmem_ahb': (23.5, 15),         # ldd x16: 28.8 / 70.4 (26.2 / 47.8)
            'mbox': (12.7, 8.3),            # ldd x16: 15.7 / 39.1 (13.9 / 26.0)
            'dmareg': (12.7, 8.3),          # DWDMA registers: as the mailbox
            'sdram': (9, 66)}               # ldd x16: 34.4 / 202.8 (25.0 / 124.4);
                                            # ld x16: 38.4 / 257.6 (27.1 / 137.2)
# stores: 2.2-2.5 DSP cycles at any clock while the write buffer has room;
# an entry drains in (HCLK cycles): DMEM std x16 9.9 per store at 160 MHz,
# SDRAM std x64 176.8 at 160 MHz (94.7 at HCLK 100)
WB_ENTRIES = 16                 # write buffer
WB_STORE = 2                    # DSP cycles per store while the buffer has room
WB_DRAIN = {'dmem_ahb': 3, 'mbox': 1, 'dmareg': 1, 'sdram': 58}
# DWDMA, 32-bit words: start-up (DSP, HCLK cycles) and per word
# max(floor DSP cycles, DSP cycles + HCLK cycles). The DMEM side of a
# transfer is DMEM's AHB slave port, clocked by the DSP: writing it takes
# at least 2 DSP cycles per word (the floor of DMA_IN, which sets the time
# at low DSP clocks), reading it 3.3 (DMA_OUT); the AHB/SDRAM side takes
# HCLK cycles.
DMA_IN = ((20, 10), (2.0, 0, 5.75))         # SDRAM -> DMEM, 1024 words: 2128 / 18558
                                            # (2128 / 10138; 1908 at 24 MHz)
DMA_OUT = ((35, 20), (0, 3.3, 3.2))         # DMEM -> SDRAM, 1024 words: 4477 / 13622
                                            # (3960 / 8502)

MULS = {'mul', 'mulsu', 'mac', 'macsu', 'macn'}
WRITE_ONLY = {'mov', 'mov.e', 'ld', 'ldd', 'ldu', 'lddu', 'mul', 'mulsu', 'pop', 'popd'}
NO_DST = {'st', 'std', 'stu', 'stdu', 'push', 'pushd', 'cmp', 'cmp.e', 'cexer', 'call'}
POSTMOD_OPS = {'ldu', 'lddu', 'stu', 'stdu', 'push', 'pop', 'pushd', 'popd'}
REGBASE = {'r': 0, 'a': 16, 'n': 24, 'g': 32}


def timing_info(sim, ins):
    """(class, issue slots, units, latency, block, sources, destinations) of
    an instruction; registers are 0-15 r, 16-23 a, 24-31 n, 32- g, control
    registers by name."""
    syn = ins.spec.syntax
    m = syn.split()[0]
    regs = []
    for (f, mul, add, kind, isreg, letter), v in zip(ins.spec.phs, sim.ops(ins)):
        if isreg:
            k = REGBASE[letter] + v
            regs.append([k, k + 1] if mul == '2' else [k])
        elif kind.startswith('creg'):
            regs.append([v])
    if m in MULS and regs:
        regs[0] = [regs[0][0], regs[0][0] + 1]      # 32-bit result, a pair
    first = regs[0] if regs else []
    rest = [k for r in regs[1:] for k in r]
    if m in NO_DST:
        dsts, srcs = [], first + rest
    elif m in WRITE_ONLY:
        dsts, srcs = list(first), rest
    else:
        dsts, srcs = list(first), first + rest
    if m in POSTMOD_OPS and len(regs) > 1:
        dsts += regs[1]
    creg_dst = bool(first) and isinstance(first[0], str) and m not in NO_DST
    if m == 'nop':
        cls = 'nop'
    elif m.startswith('{c:br_cond}'):
        cls = 'loop' if sim.ops(ins)[0].startswith('agn') else 'brc'
    elif m == 'br':
        cls = 'br'
    elif m == 'call':
        cls = 'call'
    elif m in ('ret', 'reti'):
        cls = 'ret'
    elif m.startswith('cexe'):
        cls = 'cexe'
    elif creg_dst or m in ('bits', 'bitc'):
        cls = 'cwr'
    elif m in MULS:
        cls = 'mac'
    elif m.startswith('ld'):
        cls = 'ld'
    elif m.startswith('st'):
        cls = 'st'
    elif m in ('push', 'pop', 'pushd', 'popd'):
        cls = 'stack'
    elif first and isinstance(first[0], int) and 16 <= first[0] < 32:
        cls = 'agu'
    else:
        cls = 'alu'
    if cls == 'alu' and m.endswith('.e'):
        cls = 'alue'
    slots = 1
    units, lat, block = TIMING[cls]
    return cls, slots, units, lat, block, srcs, dsts


class Inst:
    __slots__ = ('spec', 'f', 'size', 'key', 'word', 'tm')


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
        # the data is there at once; completion is seen after the transfer
        (sc, sh), (floor, pc, ph) = DMA_IN if self.region(dar) == 'dmem_ahb' else DMA_OUT
        f = self.sim.dsp_hz / self.sim.hclk_hz   # DSP cycles per HCLK cycle
        t = sc + sh * f + n * max(floor, pc + ph * f)
        self.chen |= 1 << ch
        self.sim.events.append((self.sim.tnext + max(1, round(t)), ch))

    def dma_done(self, ch):
        self.chen &= ~(1 << ch)
        self.raw_tfr |= 1 << ch
        if self.dma.get(0x310, 0) & (1 << ch):   # MaskTfr unmasked
            self.sim.ireq |= 1 << 3

    def region(self, a):
        """Timing class of a bus address (BUS_LOAD, WB_DRAIN): 'dmem_ahb'
        is DMEM through its AHB slave port (bus window, DMA), not the
        core's tightly coupled port."""
        if self.DMEM_BUS <= a < self.DMEM_BUS + 0x10000:
            return 'dmem_ahb'
        if self.MBOX <= a < self.MBOX + 0x100:
            return 'mbox'
        if self.DWDMA <= a < self.DWDMA + 0x400:
            return 'dmareg'
        return 'sdram'


class Sim:
    def __init__(self, isa=DEFAULT_ISA, dsp_hz=160_000_000, hclk_hz=100_000_000):
        self.dsp_hz = dsp_hz
        self.hclk_hz = hclk_hz          # AHB/SDRAM: Rockbox 50 MHz, boosted and hwstub 100
        self.profile = None             # {pc: cycles} when profiling
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
        self.cycles = 0                 # cycles elapsed (timers follow it)
        self.tnext = 0                  # earliest issue cycle of the next instruction
        self.tcyc = 0                   # cycle the last instruction issued in
        self.slots = 0                  # issue slots / units used in tcyc
        self.used = dict.fromkeys(CAP, 0)
        self.ready = {}                 # register -> cycle its value is ready
        self.st_dmem = {}               # DMEM address -> cycle of the last store
        self.acc = []                   # this instruction's accesses: (store, region, addr)
        self.wb = collections.deque()   # write buffer: drain cycle of each entry
        self.events = []                # (cycle, DMA channel) completions
        self.prev_cls = None            # timing class of the last instruction
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

    def load_raw(self, data):
        """A flat little-endian IMEM image from address 0 (zspasm.pl -o)."""
        import struct
        n = len(data) // 2
        self.imem[:n] = list(struct.unpack_from('<%dH' % n, data))
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
        ins.tm = None
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
        for f, mul, add, kind, _, _ in ins.spec.phs:
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
        if addr < 0x8000:               # tightly coupled DMEM: core cycles only
            self.acc.append((False, 'dmem', addr))
            return self.dmem[addr]
        if 0xCE0000 <= addr < 0xD00000:
            return self.epr[addr & 7]
        if 0xD00000 <= addr < 0xE00000:
            ba = self.busaddr(addr)
            self.acc.append((False, self.bus.region(ba), ba))
            w = self.bus.read32(ba & ~3)
            return (w >> 16) & M16 if ba & 2 else w & M16
        return 0

    def dwrite(self, addr, v):
        addr &= M24
        v &= M16
        if addr < 0x8000:
            self.acc.append((True, 'dmem', addr))
            self.dmem[addr] = v
        elif 0xCE0000 <= addr < 0xD00000:
            self.epr[addr & 7] = v & 0x7ff
        elif 0xD00000 <= addr < 0xE00000:
            ba = self.busaddr(addr)
            self.acc.append((True, self.bus.region(ba), ba))
            w = self.bus.read32(ba & ~3)
            w = (w & 0xffff0000) | v if not ba & 2 else (w & 0xffff) | (v << 16)
            self.bus.write32(ba & ~3, w)

    def busaddr(self, addr):
        off = addr - 0xD00000
        return (self.epr[(off >> 17) & 7] << 21) | (off << 1)

    def dread32(self, addr):
        addr &= M24
        if 0xD00000 <= addr < 0xE00000:      # one 32-bit bus access
            ba = self.busaddr(addr)
            self.acc.append((False, self.bus.region(ba), ba))
            return self.bus.read32(ba)
        return self.dread(addr) | (self.dread(addr + 1) << 16)

    def dwrite32(self, addr, v):
        addr &= M24
        if 0xD00000 <= addr < 0xE00000:
            ba = self.busaddr(addr)
            self.acc.append((True, self.bus.region(ba), ba))
            self.bus.write32(ba, v)
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
        self.tnext += 4
        return True

    # ------------------------------------------------------------ timing
    def hclk(self, n):
        """Whole DSP cycles for n HCLK cycles (at least 1)."""
        return max(1, round(n * self.dsp_hz / self.hclk_hz))

    def issue(self, ins):
        """Issue ins (after it ran) in the timing model and advance tnext."""
        tm = ins.tm
        if tm is None:
            tm = ins.tm = timing_info(self, ins)
        cls, slots, units, lat, block, srcs, dsts = tm
        c = self.tnext
        ready = self.ready
        for s in srcs:
            r = ready.get(s, 0)
            if r > c:
                c = r
        acc = self.acc
        if cls == 'ld':
            for store, region, addr in acc:
                if region == 'dmem':
                    t = self.st_dmem.get(addr)
                    if t is not None and t + ST_LD_SAME > c:
                        c = t + ST_LD_SAME
        elif cls == 'loop' and self.prev_cls == 'brc':
            block = AGN_AFTER_BRC
        used = self.used
        if c > self.tcyc:
            self.tcyc, self.slots = c, 0
            for u in used:
                used[u] = 0
        while (self.slots + slots > ISSUE_SLOTS or
               any(used[u] + n > CAP[u] for u, n in units)):
            self.tcyc += 1
            self.slots = 0
            for u in used:
                used[u] = 0
        c = self.tcyc
        self.prev_cls = cls
        self.slots += slots
        for u, n in units:
            used[u] += n
        for d in dsts:
            ready[d] = c + lat
        nxt = c + block
        if acc:
            for store, region, addr in acc:
                if region == 'dmem':
                    if store and cls == 'st':
                        self.st_dmem[addr] = c
                elif store:
                    nxt = max(nxt, self.wb_store(c, region))
                else:                   # the core waits for the data
                    cyc, hc = BUS_LOAD[region]
                    nxt = max(nxt, c + max(1, round(cyc + hc * self.dsp_hz / self.hclk_hz)))
            acc.clear()
        self.tnext = max(nxt, c)

    def wb_store(self, c, region):
        """A bus store at cycle c into the write buffer; returns the cycle
        the core can go on."""
        wb = self.wb
        while wb and wb[0] <= c:
            wb.popleft()
        if len(wb) >= WB_ENTRIES:       # full: wait for an entry to drain
            c = wb.popleft()
        start = max(c, wb[-1]) if wb else c
        wb.append(start + self.hclk(WB_DRAIN[region]))
        return c + WB_STORE

    def advance(self):
        """Fire due DMA completions and let the timers follow the cycles."""
        if self.events:
            self.events.sort()
            while self.events and self.events[0][0] <= self.tnext:
                _, ch = self.events.pop(0)
                self.bus.dma_done(ch)
        d = self.tnext - self.cycles
        if d > 0:
            self.tick_timers(d)
            self.cycles = self.tnext

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
                self.advance()
            elif self.waiting:
                if self.events:         # sleep until the next DMA completion
                    self.tnext = max(self.tnext, min(self.events)[0])
                    self.advance()
                    continue
                # idle: the caller decides what wakes the core next
                return 'wait'
            pc = self.pc
            ins = self.decode(pc)
            self.next_pc = (pc + ins.size) & M24
            t0 = self.tnext
            if self.shadow:
                self.shadow -= 1
                if self.skip:
                    if self.update:
                        self.exec_postmod_only(ins)
                    self.acc.clear()
                    self.issue(ins)
                    self.advance()
                    self.pc = self.next_pc
                    n += 1
                    continue
            h = OPS.get(ins.key[0])
            if h is None:
                raise SimError('not simulated: %s (%04X) at %04X' % (ins.spec.syntax, ins.word & M16, pc))
            h(self, ins, self.ops(ins))
            n += 1
            self.insts += 1
            self.issue(ins)
            self.advance()
            if self.profile is not None:
                self.profile[pc] = self.profile.get(pc, 0) + self.tnext - t0
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
    ap.add_argument('image', help='ZSP1 image (zspasm.pl --image) or, with --raw, a flat IMEM image')
    ap.add_argument('-n', type=int, default=10_000_000)
    ap.add_argument('--raw', action='store_true')
    ap.add_argument('--mhz', type=float, default=160, help='DSP clock')
    ap.add_argument('--hclk', type=float, default=100, help='AHB/SDRAM clock (MHz)')
    ap.add_argument('--profile', type=int, metavar='N', default=0,
                    help='print the N instructions that took the most cycles')
    a = ap.parse_args()
    s = Sim(dsp_hz=a.mhz * 1e6, hclk_hz=a.hclk * 1e6)
    data = open(a.image, 'rb').read()
    s.load_raw(data) if a.raw else s.load_image(data)
    if a.profile:
        s.profile = {}
    why = s.run(a.n)
    print('stopped (%s) at %04X after %d instructions, %d cycles' % (why, s.pc, s.insts, s.cycles))
    print('r: ' + ' '.join('%04x' % v for v in s.r))
    print('a: ' + ' '.join('%06x' % v for v in s.a))
    if a.profile:
        print_profile(s, a.profile)


def print_profile(s, top):
    """Cycles per class and the instructions that took the most."""
    per_cls = {}
    for pc, cyc in s.profile.items():
        ins = s.decode(pc)
        cls = ins.tm[0] if ins.tm else '?'
        per_cls[cls] = per_cls.get(cls, 0) + cyc
    total = sum(per_cls.values()) or 1
    print('cycles by class: ' + ', '.join('%s %d (%.0f%%)' % (k, v, 100 * v / total) for k, v in
                                          sorted(per_cls.items(), key=lambda x: -x[1])))
    for pc, cyc in sorted(s.profile.items(), key=lambda x: -x[1])[:top]:
        print('  %04X %8d  %s' % (pc, cyc, s.decode(pc).spec.syntax))


if __name__ == '__main__':
    main()
