#!/usr/bin/env python3
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (C) 2026 by Marcin Bukat
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
"""Parse a Linux usbmon pcap (DLT_USB_LINUX_MMAPPED) of an ADFU session and
reconstruct the sequence of CBW commands with their data phases.

Used to analyse ProductTool reflash captures and to produce a replay script
for atjflash.

usage: usbmon_adfu.py capture.pcap               - list commands
       usbmon_adfu.py capture.pcap script.bin    - write atjflash replay script
"""

import struct
import sys

PCAP_HDR = struct.Struct('<IHHiIII')
REC_HDR = struct.Struct('<IIII')
# struct usbmon_packet (64 bytes, mmapped variant)
MON_HDR = struct.Struct('<QcBBBHccqiiII8siiII')

CBW_SIG = 0x43425355
CSW_SIG = 0x53425355
XFER_BULK = 3


class Urb:
    __slots__ = ('ts', 'dev', 'ep', 'data')

    def __init__(self, ts, dev, ep, data):
        self.ts, self.dev, self.ep, self.data = ts, dev, ep, data


def read_urbs(path):
    """Yield bulk transfers with payload: OUT data from submissions,
    IN data from completions."""
    with open(path, 'rb') as f:
        hdr = f.read(PCAP_HDR.size)
        magic, _, _, _, _, _, linktype = PCAP_HDR.unpack(hdr)
        if magic != 0xa1b2c3d4 or linktype != 220:
            raise ValueError('not a little-endian usbmon mmapped pcap')
        while True:
            rh = f.read(REC_HDR.size)
            if len(rh) < REC_HDR.size:
                break
            _, _, incl, _ = REC_HDR.unpack(rh)
            rec = f.read(incl)
            (_, typ, xfer, epnum, dev, _, _, _, sec, usec, status, length,
             len_cap, _, _, _, _, _) = MON_HDR.unpack_from(rec)
            if xfer != XFER_BULK:
                continue
            data = rec[MON_HDR.size:MON_HDR.size + len_cap]
            if len_cap != len(data):
                raise ValueError('truncated record')
            is_in = bool(epnum & 0x80)
            if typ == b'S' and not is_in and data:
                yield Urb(sec + usec / 1e6, dev, epnum, data)
            elif typ == b'C' and is_in and data and status == 0:
                yield Urb(sec + usec / 1e6, dev, epnum, data)


class Cmd:
    def __init__(self, cbw, ts, dev):
        self.ts = ts
        self.dev = dev
        self.raw = cbw
        (_, self.tag, self.xfer_len, self.flags, self.lun,
         self.cblen) = struct.unpack_from('<IIIBBB', cbw)
        self.cb = cbw[15:31]
        self.out = bytearray()
        self.inp = bytearray()
        self.csw = None
        self.chunk = 0

    def describe(self):
        cb = self.cb
        op = cb[0]
        a = struct.unpack_from('<I', cb, 1)[0]
        b = struct.unpack_from('<I', cb, 5)[0]
        c = struct.unpack_from('<I', cb, 9)[0]
        s = 'op=%02x a=%08x b=%08x c=%08x' % (op, a, b, c)
        if op == 0xb0:
            s += ' sub=%04x' % (c & 0xffff)
        s += ' out=%d in=%d' % (len(self.out), len(self.inp))
        if self.csw is not None:
            s += ' csw=%d' % self.csw
        return s


def parse(path):
    cmds = []
    cur = None
    for u in read_urbs(path):
        d = u.data
        if (u.ep & 0x80) == 0 and len(d) == 31 and struct.unpack_from('<I', d)[0] == CBW_SIG:
            cur = Cmd(d, u.ts, u.dev)
            cmds.append(cur)
        elif (u.ep & 0x80) and len(d) == 13 and struct.unpack_from('<I', d)[0] == CSW_SIG:
            if cur is not None:
                cur.csw = d[12]
                cur = None
        elif cur is not None:
            if not (u.ep & 0x80) and cur.chunk == 0:
                cur.chunk = len(d)
            (cur.inp if u.ep & 0x80 else cur.out).extend(d)
    return cmds


# replay script format (little endian):
#   "ATJRPL01", u32 count, then count records:
#   u8 cbw[31], u8 dir (0 none, 1 out, 2 in), u8 csw, u16 reserved,
#   u32 chunk, u32 len, data[len]  (out data or expected in data)
def write_script(cmds, path, last=None):
    with open(path, 'wb') as f:
        sel = cmds if last is None else cmds[:last + 1]
        f.write(b'ATJRPL01' + struct.pack('<I', len(sel)))
        for c in sel:
            if c.csw is None:
                raise ValueError('command without CSW: ' + c.describe())
            if c.out and c.inp:
                raise ValueError('bidirectional data: ' + c.describe())
            if c.out:
                d, data = 1, bytes(c.out)
            elif c.inp:
                d, data = 2, bytes(c.inp)
            else:
                d, data = 0, b''
            if len(data) != c.xfer_len:
                raise ValueError('data length mismatch: ' + c.describe())
            f.write(c.raw + struct.pack('<BBHII', d, c.csw, 0, c.chunk, len(data)) + data)


def main():
    cmds = parse(sys.argv[1])
    if len(sys.argv) > 2:
        write_script(cmds, sys.argv[2])
        print('wrote %d commands to %s' % (len(cmds), sys.argv[2]))
        return
    devs = sorted(set(c.dev for c in cmds))
    print('# %d commands, devices %s' % (len(cmds), devs))
    for i, c in enumerate(cmds):
        print('%4d %10.3f dev%-3d %s' % (i, c.ts - cmds[0].ts, c.dev, c.describe()))


if __name__ == '__main__':
    main()
