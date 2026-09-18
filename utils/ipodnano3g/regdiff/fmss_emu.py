#!/usr/bin/env python3
"""fmss_emu.py -- run an Apple FMSS sequencer program against a mock flash
controller and print the register trace.

The mock controller always reports ready and pass, and the ECC encoder
always done, so the trace is the success path. Parameters are set up as
Apple's m1fmssWriteSequentialPages (0x082d430c) sets them for a run of pages.

  fmss_emu.py program2 NPAGES     two-plane program 0x2200ac38 (mode 8)
  fmss_emu.py program1 NPAGES     single-plane program 0x22009e98
"""
import struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fmss_dis import OSOS, reg, EVENTS, ALU

PROGRAMS = {"read": 0x22009030, "read-corrected": 0x22009030,
            "program2": 0x2200ac38,
            "program1": 0x22009e98}


class Emu:
    def __init__(self, start, ecc_count=0):
        self.d = open(OSOS, "rb").read()
        self.start = start
        self.r = [0] * 16
        self.io = {}                 # FMC and SEQ registers
        self.mem = {}                # parameter memory
        self.trace = []
        self.raw = []                # (offset, value) of every FMC write
        self.ecc_count = ecc_count

    def mem32(self, a):
        return self.mem.get(a, 0)

    def rd(self, off):
        if off == 0x810:             # ECC result: bit 0 fail, bits 16..19 count
            return self.ecc_count << 16
        if off == 0x07c:             # encode/decode busy: always done
            return 0
        if off == 0x04c:             # FMSYND0: NAND status ready, pass
            return 0x40
        if off == 0x048:             # FMCSTAT
            return 0xffffffff
        return self.io.get(off, 0)

    def wr(self, off, v):
        self.io[off] = v & 0xffffffff
        if off < 0xc00:
            self.trace.append("%-10s = 0x%x" % (reg(off), v & 0xffffffff))
            self.raw.append((off, v & 0xffffffff))

    def run(self, limit=200000):
        pc = self.start
        steps = 0
        while steps < limit:
            steps += 1
            w0, w1 = struct.unpack_from("<II", self.d, pc - 0x22000000)
            op, a, b, o = w0 >> 24, (w0 >> 16) & 0xff, w0 & 0xff, w0 & 0xffff
            r = self.r
            nxt = pc + 8
            if op == 0x00:
                return
            elif op == 0x01:
                self.wr(o, w1)
            elif op == 0x02:
                self.wr(o, r[a])
            elif op == 0x03:
                r[a] = self.mem32(r[b])
            elif op == 0x04:
                r[a] = self.rd(o) & w1
            elif op == 0x05:
                r[a] = w1
            elif op == 0x06:
                r[a] = r[b]
            elif op == 0x07:
                self.trace.append("wait %s" % EVENTS.get(a, a))
            elif op == 0x11:
                self.mem[r[b]] = r[a] & 0xffffffff
            elif op == 0x18:
                r[a] = self.io.get(r[b], 0)
            elif op == 0x19:
                self.io[r[b]] = r[a] & 0xffffffff
            elif op in ALU:
                f = {0x0a: lambda x, y: x & y, 0x0b: lambda x, y: x | y,
                     0x0c: lambda x, y: x + y, 0x0d: lambda x, y: x - y,
                     0x13: lambda x, y: x << y, 0x14: lambda x, y: x >> y}[op]
                r[a] = (f(r[b], w1) if w1 else f(r[a], r[b])) & 0xffffffff
            elif op == 0x0e:
                if r[a]:
                    nxt = self.start + w1
            elif op == 0x17:
                if not r[a]:
                    nxt = self.start + w1
            else:
                raise ValueError("op %02x at %08x" % (op, pc))
            pc = nxt
        raise RuntimeError("step limit")


def setup_list(e, pages, base_ctrl0=0, units=1):
    """pages: list of (bank, page) in the order the program is given them;
    units: 2KiB units per page (1 on 2KiB-page chips, 2 on 4KiB)"""
    base = 0x40000000
    pagelist, banklist, bufs, spares = base, base + 0x10000, base + 0x20000, base + 0x30000
    cetab, results = base + 0x40000, base + 0x50000
    for b in range(8):
        e.mem[cetab + 4 * b] = b
    for i, (bank, page) in enumerate(pages):
        e.mem[pagelist + 4 * i] = page
        e.mem[banklist + 4 * i] = bank
        # One buffer per 2KiB unit, as the FILs build the list (0xD28 units)
        for u in range(units):
            e.mem[bufs + 4 * (i * units + u)] = (0x08000000 + i * 2048 * units
                                                 + u * 2048)
        for w in range(3):
            e.mem[spares + 4 * (i * 3 + w)] = 0x11110000 | (i << 4) | w
    e.io.update({0xd04: 8, 0xd0c: pagelist, 0xd10: banklist,
                 0xd14: cetab, 0xd18: len(pages), 0xd1c: spares, 0xd20: bufs,
                 0xd24: results, 0xd28: units, 0xd48: base_ctrl0,
                 0xd4c: 0, 0xd00: 64})


def setup_write(e, npages, banks=4, planes=2):
    """Pages in Apple's bank-major order for two-plane programming: for each
    row, bank 0 plane 0, bank 0 plane 1, bank 1 plane 0, ..."""
    base = 0x40000000
    pagelist, banklist, bufs, spares = base, base + 0x10000, base + 0x20000, base + 0x30000
    cetab = base + 0x40000
    per_row = banks * planes
    for b in range(banks):
        e.mem[cetab + 4 * b] = b                    # bank -> chip enable
    for i in range(npages):
        row, k = divmod(i, per_row)
        bank, plane = divmod(k, planes)
        block = 1000 + plane                        # a plane pair of blocks
        e.mem[pagelist + 4 * i] = block * 128 + row
        e.mem[banklist + 4 * i] = bank
        for c in range(4):
            e.mem[bufs + 4 * (i * 4 + c)] = 0x08000000 + i * 2048 + c * 512
        for w in range(3):
            e.mem[spares + 4 * (i * 3 + w)] = 0x11110000 | (i << 4) | w
    e.io.update({0xd04: per_row, 0xd0c: pagelist, 0xd10: banklist,
                 0xd14: cetab, 0xd18: npages, 0xd1c: spares, 0xd20: bufs,
                 0xd24: 0, 0xd28: 4, 0xd48: 0, 0xd4c: 0, 0xd00: 128 // 2})


if __name__ == "__main__":
    name = sys.argv[1] if len(sys.argv) > 1 else "program2"
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    e = Emu(PROGRAMS[name], ecc_count=3 if name == "read-corrected" else 0)
    if name.startswith("read"):
        setup_list(e, [(i % 4, 1000 + i // 4) for i in range(n)])
    else:
        setup_write(e, n)
    e.run()
    for t in e.trace:
        print(t)
    if name.startswith("read"):
        results = e.io[0xd24] - 4 * n
        print("results:", " ".join("0x%x" % e.mem[results + 4 * i]
                                    for i in range(n)))
