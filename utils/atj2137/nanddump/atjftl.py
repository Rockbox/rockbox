#!/usr/bin/env python3
"""Rebuild logical disks of an ATJ2137 (NAND96R1.DRV FTL) from a raw NAND dump
made by atjnanddump (4096+256 bytes per page, 128 pages per block).

FTL model (reverse engineered from NAND96R1.DRV, checked against E150 dump):
 - superpage  = 2 planes x 4 KiB = 16 sectors; physical pair p = blocks 2p, 2p+1,
   sectors 0-7 of a superpage in block 2p, sectors 8-15 in block 2p+1, same page
 - logical block = 128 superpages = 1 MiB; 954 logical blocks per zone, 4 zones
 - per zone table block (pair 8+z, marker spare ff aa 80|z<<2) written as a log of
   4-page versions: [map][log table][0x55 dirty marker][unused]
     map page: u16[1023] = blockmap[954] + freelist[69], u16 sum at 0x7fe
     log page: 8 x {u16 lblock, u16 log pair, u16 pages used}, u16 sum at 0x7fe
   entries: low 12 bits = physical pair, high nibble = flags
 - log (replacement) block: pages written in order, logical superpage index in
   spare user byte 0 of sector 1; later copies win over earlier ones and over
   the data block
 - all logical disks share one sector space, sizes at LFI header +0x80 (u32[8])
Table pages are always ECC corrected, data pages only with --ecc (see atjbch.py).
"""
import argparse, mmap, struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import atjbch

PAGE = 4096; RAW = 4352; PPB = 128; SPP = 16
LB_PER_ZONE = 954; ZONES = 4; TABLE_PAIR0 = 8
FF16 = b'\xff' * 16

class Dump:
    def __init__(self, path):
        self.f = open(path, 'rb')
        self.m = mmap.mmap(self.f.fileno(), 0, access=mmap.ACCESS_READ)

    def raw(self, blk, page):
        o = (blk * PPB + page) * RAW
        return self.m[o:o + RAW]

    def page_ecc(self, blk, page, nsec=8):
        r = self.raw(blk, page); out = bytearray(); bad = 0
        for s in range(nsec):
            sp = r[PAGE + s * 16:PAGE + s * 16 + 16]
            n, d, _ = atjbch.decode(r[s * 512:s * 512 + 512], sp[:3], sp[3:])
            bad += n < 0; out += d
        return bytes(out), bad

def zero_bits(b):
    return len(b) * 8 - int.from_bytes(b, 'little').bit_count()

def sum16(b, n):
    return sum(struct.unpack_from('<%dH' % n, b)) & 0xffff

def load_zone(dump, z):
    """latest version of zone table with valid checksums"""
    blk = 2 * (TABLE_PAIR0 + z)
    best = None
    for p in range(0, PPB, 4):
        if dump.raw(blk, p)[PAGE:PAGE + 16] == FF16:
            break
        mp, b1 = dump.page_ecc(blk, p, 4)
        lg, b2 = dump.page_ecc(blk, p + 1, 4)
        if b1 or b2:
            continue
        if struct.unpack_from('<H', mp, 0x7fe)[0] != sum16(mp, 0x3ff):
            continue
        if struct.unpack_from('<H', lg, 0x7fe)[0] != sum16(lg, 0x18):
            continue
        best = (p, mp, lg)
    if best is None:
        raise RuntimeError('no valid table for zone %d' % z)
    p, mp, lg = best
    bmap = list(struct.unpack_from('<%dH' % LB_PER_ZONE, mp))
    logs = {}
    for i in range(8):
        lb, pair, used = struct.unpack_from('<3H', lg, i * 6)
        if lb != 0xffff:
            logs[lb] = (pair & 0xfff, used & 0xfff)
    return p, bmap, logs

class FTL:
    def __init__(self, dump, ecc=False):
        self.d = dump
        self.ecc = ecc
        self.stats = {'corrected': 0, 'bits': 0, 'uncorrectable': 0}
        self.zones = [load_zone(dump, z) for z in range(ZONES)]
        self.logmaps = {}

    def phys(self, blk, page):
        r = self.d.raw(blk, page)
        if not self.ecc or r == b'\xff' * RAW:
            return r[:PAGE]
        out = bytearray()
        for s in range(8):
            d = r[s * 512:s * 512 + 512]; sp = r[PAGE + s * 16:PAGE + s * 16 + 16]
            if sp == FF16 and zero_bits(d) <= atjbch.T:
                # erased sector (no ECC), possibly with a few bit flips
                out += b'\xff' * 512; continue
            n, d2, _ = atjbch.check(d, sp[:3], sp[3:])
            if n < 0:
                self.stats['uncorrectable'] += 1
            elif n:
                self.stats['corrected'] += 1; self.stats['bits'] += n
            out += d2
        return bytes(out)

    def log_pages(self, z, lb):
        key = (z, lb)
        if key not in self.logmaps:
            pair, used = self.zones[z][2][lb]
            pm = {}
            for p in range(used):
                sp = self.d.raw(2 * pair, p)[PAGE:PAGE + 32]
                idx = sp[16]
                if idx < PPB:
                    pm[idx] = p
            self.logmaps[key] = (pair, pm)
        return self.logmaps[key]

    def superpage(self, sp):
        lbg, page = divmod(sp, PPB)
        z, lb = divmod(lbg, LB_PER_ZONE)
        _, bmap, logs = self.zones[z]
        pair, ppage = None, page
        if lb in logs:
            lpair, pm = self.log_pages(z, lb)
            if page in pm:
                pair, ppage = lpair, pm[page]
        if pair is None:
            e = bmap[lb]
            if (e & 0xfff) == 0xfff:
                return b'\xff' * (SPP * 512)
            pair = e & 0xfff
        return self.phys(2 * pair, ppage) + self.phys(2 * pair + 1, ppage)

    def read(self, sector, count):
        out = bytearray()
        while count:
            sp, off = divmod(sector, SPP)
            n = min(count, SPP - off)
            out += self.superpage(sp)[off * 512:(off + n) * 512]
            sector += n; count -= n
        return bytes(out)

def disks(ftl):
    # disk table from LFI copy 1 header (copy 0 header is damaged on E150)
    for start in (0, 90112):
        h = ftl.read(start, 1)
        if h[:4] == b'\x55\xaa\xf2\x0f':
            sizes = struct.unpack_from('<8I', h, 0x80)
            res = []; o = 0
            for s in sizes:
                res.append((o, s)); o += s
            return res
    raise RuntimeError('no valid LFI header')

def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('dump')
    ap.add_argument('-l', '--list', action='store_true', help='list zones and disks')
    ap.add_argument('-d', '--disk', type=int, help='disk number to extract')
    ap.add_argument('-n', '--sectors', type=int, help='limit sectors to extract')
    ap.add_argument('-o', '--out', help='output file')
    ap.add_argument('-e', '--ecc', action='store_true', help='ECC correct data pages')
    a = ap.parse_args()
    ftl = FTL(Dump(a.dump), a.ecc)
    dl = disks(ftl)
    if a.list or a.disk is None:
        for z, (p, bmap, logs) in enumerate(ftl.zones):
            used = sum(1 for e in bmap if (e & 0xfff) != 0xfff)
            print(f'zone {z}: table page {p}, mapped {used}/{LB_PER_ZONE}, log blocks {len(logs)}')
        for i, (o, s) in enumerate(dl):
            print(f'disk {i}: start {o} size {s} ({s * 512 / 2**20:.1f} MiB)')
        return
    start, size = dl[a.disk]
    n = min(size, a.sectors or size)
    with open(a.out, 'wb') as f:
        step = 2048
        for s in range(0, n, step):
            f.write(ftl.read(start + s, min(step, n - s)))
            if s % (step * 64) == 0:
                print(f'\r{s * 100 // n}%', end='', file=sys.stderr)
    print('\rdone', ftl.stats if a.ecc else '', file=sys.stderr)

if __name__ == '__main__':
    main()
