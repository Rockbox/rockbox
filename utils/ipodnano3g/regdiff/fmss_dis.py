#!/usr/bin/env python3
"""fmss_dis.py <iram_start> [iram_end] -- disassemble an FMSS sequencer program.

Apple's FIL runs programs on the S5L8702 flash controller's sequencer. They
live in IRAM (0x2200xxxx = osos file offset 0xxxxx). Instruction set as
decoded in ../decode/APPLE-FTL-READ.md:
8 bytes, word0 = op<<24 | rA<<16 | offset-or-rB, word1 = value.
"""
import struct, sys, os

OSOS = os.environ.get("NANO3G_OSOS", os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "osos-1.1.3.bin"))

FMC = {
    0x000: "FMCTRL0", 0x004: "FMCTRL1", 0x008: "FMCMD", 0x00c: "FMADDR0",
    0x010: "FMADDR1", 0x014: "FMADDR2", 0x018: "FMADDR3", 0x01c: "FMADDR4",
    0x020: "FMADDR5", 0x024: "FMADDR6", 0x028: "FMADDR7", 0x02c: "FMANUM",
    0x030: "FMDNUM", 0x034: "FMDATAW0", 0x038: "FMDATAW1", 0x03c: "FMDATAW2",
    0x040: "FMDATAW3", 0x044: "FMDATAW4?", 0x048: "FMCSTAT", 0x04c: "FMSYND0",
    0x06c: "FMSYND5", 0x070: "FMSYND6", 0x074: "FMSYND7", 0x078: "FMUNK78",
    0x07c: "FMUNK7C", 0x080: "FMFIFO", 0x80c: "FMTRANS0", 0x810: "FMUNK810",
    0x814: "FMTRANS1", 0x840: "FMTRANSSTAT",
}
EVENTS = {0: "CMDDONE(bit1)", 1: "ADDRDONE(bit2)", 2: "bit20", 3: "TRANSDONE(bit3)",
          4: "FMTRANS", 5: "STATUSREADY"}
ALU = {0x0a: "&", 0x0b: "|", 0x0c: "+", 0x0d: "-", 0x13: "<<", 0x14: ">>"}


def reg(off):
    if off >= 0xc00:
        return "SEQ+0x%03x" % off
    return FMC.get(off, "FMC+0x%03x" % off)


def dis(start, end):
    d = open(OSOS, "rb").read()
    pc = start
    while pc < end:
        off = pc - 0x22000000
        w0, w1 = struct.unpack_from("<II", d, off)
        op = w0 >> 24
        a = (w0 >> 16) & 0xff
        b = w0 & 0xff
        o = w0 & 0xffff
        rel = pc - start
        if op == 0x00:
            txt = "end"
        elif op == 0x01:
            txt = "%s = 0x%x" % (reg(o), w1)
        elif op == 0x02:
            txt = "%s = r%d" % (reg(o), a)
        elif op == 0x03:
            txt = "r%d = mem32[r%d]" % (a, b)
        elif op == 0x04:
            txt = "r%d = %s & 0x%x" % (a, reg(o), w1)
        elif op == 0x05:
            txt = "r%d = 0x%x" % (a, w1)
        elif op == 0x06:
            txt = "r%d = r%d" % (a, b)
        elif op == 0x07:
            txt = "wait %s" % EVENTS.get(a, "event %d" % a)
        elif op == 0x11:
            txt = "mem32[r%d] = r%d" % (b, a)
        elif op == 0x18:
            txt = "r%d = seq32[r%d]" % (a, b)
        elif op == 0x19:
            txt = "seq32[r%d] = r%d" % (b, a)
        elif op in ALU:
            txt = ("r%d = r%d %s 0x%x" % (a, b, ALU[op], w1) if w1
                   else "r%d %s= r%d" % (a, ALU[op], b))
        elif op == 0x0e:
            txt = "if r%d != 0 goto +0x%x (0x%08x)" % (a, w1, start + w1)
        elif op == 0x17:
            txt = "if r%d == 0 goto +0x%x (0x%08x)" % (a, w1, start + w1)
        else:
            txt = "op %02x a%d b%d off 0x%x val 0x%x" % (op, a, b, o, w1)
        print("%08x +%04x  %08x %08x  %s" % (pc, rel, w0, w1, txt))
        pc += 8


if __name__ == "__main__":
    s = int(sys.argv[1], 0)
    e = int(sys.argv[2], 0) if len(sys.argv) > 2 else s + 0x400
    dis(s, e)
