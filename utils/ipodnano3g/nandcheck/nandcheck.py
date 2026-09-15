#!/usr/bin/env python3
r"""Collect what is needed to validate an iPod Nano 3G's NAND for Rockbox.

1. Put the iPod in DFU mode and run the check image:
   mks5lboot --dfusend nano3g-check.dfu
   The screen shows the chip, its table row and how a read-only mount went.
   Nothing is written to the iPod.
2. Connect USB. The iPod appears as a disk of 2048-byte sectors.
3. Run, with read access to the disk (sudo on Linux/macOS, an
   administrator prompt on Windows):
       nandcheck.py collect
   which finds the disk itself, or name it: /dev/sdX (Linux), /dev/rdiskN
   (macOS), \\.\PhysicalDriveN (Windows).
   This writes nandcheck-<id>-x<banks>.tar.gz: the report, the spare
   metadata of every page and every page that is not file data - the VFL
   and FTL control structures and Apple's bad-block records. No file
   contents are included. It takes about 7 minutes on a 4GB unit.
   Add --full to include every page's data as well (4-8 GB, and it contains
   whatever is stored on the iPod).

The check image serves:
    sector 0                        text report
    1 + block * banks + bank        128 records of 16 bytes: 3 spare words
                                    and a result word, little endian: -1 on
                                    a timeout, else the ECC class (0 clean,
                                    1 corrected, 3 failed) | most corrected
                                    bits in a chunk << 8 | 0x10000 erased
    metaend + page * secperpage     page data, bank by bank

The archive's meta.bin keeps the dump convention the host tests use (result
0 erased, 1 written, 3 failed, -1 timed out); results.bin has every page's
full result word.

  nandcheck.py collect [DISK|auto] [--out DIR] [--full]
  nandcheck.py mkimage DUMPDIR IMAGE     (testing: a sparse check-image
                                          view of a 4GB unit's dump)
"""
import argparse
import hashlib
import io
import mmap
import os
import stat
import struct
import sys
import tarfile
import time

SECTOR = 2048
RECORD = 16
MAGIC = "nano3g-nandcheck 1"
DATA_TYPES = (0x40, 0x41)
ERASED = 0xff


def is_control(kind):
    """Every page but user data and erased ones: FTL control (0x43-0x4f),
    VFL contexts (0x80) and whatever else Apple keeps, such as its bad-block
    records"""
    return kind not in DATA_TYPES and kind != ERASED


WINDOWS = sys.platform == "win32"


class Disk:
    """A disk opened read-only, read in whole sectors"""

    def __init__(self, path):
        flags = os.O_RDONLY | getattr(os, "O_BINARY", 0)
        direct = False
        if not WINDOWS:
            try:
                if stat.S_ISBLK(os.stat(path).st_mode) and \
                        hasattr(os, "O_DIRECT"):
                    flags |= os.O_DIRECT    # no readahead of unwanted sectors
                    direct = True
            except OSError:
                pass
        self.fd = os.open(path, flags)
        self.direct = direct
        self.buf = mmap.mmap(-1, SECTOR * 256)  # page-aligned for O_DIRECT

    def read(self, sector, count):
        out = bytearray()
        while count:
            n = min(count, 256)
            if self.direct:
                view = memoryview(self.buf)[:n * SECTOR]
                got = os.preadv(self.fd, [view], sector * SECTOR)
                data = view[:got]
            else:
                # Windows raw disks and macOS: seek and read whole sectors
                os.lseek(self.fd, sector * SECTOR, os.SEEK_SET)
                data = os.read(self.fd, n * SECTOR)
                got = len(data)
            if got != n * SECTOR:
                raise IOError("short read at sector %d" % sector)
            out += data
            sector += n
            count -= n
        return bytes(out)

    def close(self):
        os.close(self.fd)


def candidate_disks():
    """Whole disks this system might show the iPod as"""
    if WINDOWS:
        return [r"\\.\PhysicalDrive%d" % i for i in range(32)]
    if sys.platform == "darwin":
        return ["/dev/rdisk%d" % i for i in range(32)]
    return sorted("/dev/" + d for d in os.listdir("/sys/block")
                  if d.startswith(("sd", "mmcblk", "vd")))


def find_disk():
    """The disk whose sector 0 is the check image's report"""
    denied = []
    for path in candidate_disks():
        try:
            disk = Disk(path)
        except PermissionError:
            denied.append(path)
            continue
        except OSError:
            continue
        try:
            first = disk.read(0, 1)
        except (OSError, IOError):
            first = b""
        disk.close()
        if first.startswith(MAGIC.encode()):
            return path
    hint = ""
    if denied:
        hint = ("\nno permission to read %s - run as administrator/root, "
                "or grant read access to the disk" % ", ".join(denied))
    sys.exit("no disk carrying the Nano 3G NAND check was found" + hint)


def parse_report(sector0):
    text = sector0.split(b"\0", 1)[0].decode("ascii", "replace")
    lines = text.splitlines()
    if not lines or lines[0] != MAGIC:
        sys.exit("not the Nano 3G NAND check image (sector 0 is %r)"
                 % lines[:1])
    rep = {}
    for line in lines[1:]:
        key, _, value = line.partition(" ")
        rep[key] = value
    return text, rep


def ftl_meaning(rc):
    """What ftl_init()'s result says about the mount"""
    try:
        rc = int(rc)
    except ValueError:
        return "not tried"
    fixed = {0: "mounted", -1: "no geometry",
             -2: "geometry beyond the FTL's limits",
             -3: "no VFL context found",
             -4: "layout or page size not supported",
             -5: "VFL context does not fit the chip table row"}
    if rc in fixed:
        return fixed[rc]
    if -20 < rc <= -10:
        return "no FTL context in the control blocks"
    if rc <= -20:
        return "FTL context or restore inconsistent"
    return "unknown"


def progress(done, total, start, what):
    rate = done / max(time.time() - start, 1e-3)
    eta = (total - done) / rate if rate else 0
    sys.stderr.write("\r%s %5.1f%%  %d/%d  eta %dm%02ds   " % (
        what, 100.0 * done / total, done, total, eta // 60, eta % 60))


def collect(args):
    # before the minutes of reading, not after
    os.makedirs(args.out, exist_ok=True)
    path = find_disk() if args.disk == "auto" else args.disk
    print("reading %s" % path)
    disk = Disk(path)
    text, rep = parse_report(disk.read(0, 1))
    print(text.rstrip())
    ident = rep.get("ids", "unknown").split()[0]
    name = "nandcheck-%s-x%s" % (ident, rep.get("banks", "0"))
    out = os.path.join(args.out, name + ".tar.gz")
    files = {"report.txt": text.encode()}

    if rep.get("row", "none") == "none":
        print("\nThe chip is not in Apple's table; only the report is saved.")
    else:
        banks, blocks = int(rep["banks"]), int(rep["blocks"])
        ppb, pagesize = int(rep["ppb"]), int(rep["pagesize"])
        spp = pagesize // SECTOR
        metaend = 1 + blocks * banks
        ftl = rep.get("ftl", "?")
        print("\nread-only mount: %s (%s)" % (ftl, ftl_meaning(ftl)))

        # Spare records of every page, stored bank by bank like a dump
        meta = bytearray(banks * blocks * ppb * RECORD)
        results = bytearray(banks * blocks * ppb * 4)
        start = time.time()
        step = 8 * banks
        for s in range(1, metaend, step):
            n = min(step, metaend - s)
            data = disk.read(s, n)
            for i in range(n):
                block, bank = divmod(s - 1 + i, banks)
                off = (bank * blocks + block) * ppb * RECORD
                meta[off:off + ppb * RECORD] = data[i * SECTOR:
                                                    (i + 1) * SECTOR]
            progress(s - 1 + n, metaend - 1, start, "spare records")
        sys.stderr.write("\n")

        # The control pages, and a summary of what the medium holds
        counts = {"data": 0, "erased": 0, "ftl": 0, "vfl": 0, "other": 0,
                  "ecc-failed": 0, "timeout": 0,
                  "erased-flag-but-written": 0, "erased-type-no-flag": 0}
        corrected = [0] * 9
        control = []
        for g in range(banks * blocks * ppb):
            off = g * RECORD
            word = struct.unpack_from("<i", meta, off + 12)[0]
            results[g * 4:(g + 1) * 4] = meta[off + 12:off + 16]
            kind = meta[off + 9]
            if word < 0:
                counts["timeout"] += 1
                norm = -1
            else:
                erased = bool(word & 0x10000)
                failed = (word & 0xff) == 3
                if failed:
                    counts["ecc-failed"] += 1
                if erased and kind != ERASED:
                    counts["erased-flag-but-written"] += 1
                if kind == ERASED and not erased:
                    counts["erased-type-no-flag"] += 1
                if not erased:
                    corrected[min((word >> 8) & 0xff, 8)] += 1
                norm = 0 if erased else 3 if failed else 1
            struct.pack_into("<i", meta, off + 12, norm)
            if kind in DATA_TYPES:
                counts["data"] += 1
            elif kind == ERASED:
                counts["erased"] += 1
            else:
                counts["ftl" if 0x43 <= kind <= 0x4f else
                       "vfl" if kind == 0x80 else "other"] += 1
                control.append(g)
        pages = io.BytesIO()
        start = time.time()
        for i, g in enumerate(control):
            bank, page = divmod(g, blocks * ppb)
            pages.write(struct.pack("<II", bank, page))
            pages.write(disk.read(metaend + g * spp, spp))
            if i % 256 == 0 or i + 1 == len(control):
                progress(i + 1, len(control), start, "control pages")
        sys.stderr.write("\n")
        files["meta.bin"] = bytes(meta)
        files["results.bin"] = bytes(results)
        files["pages.bin"] = pages.getvalue()
        summary = "".join("%s %d\n" % kv for kv in counts.items())
        summary += "corrected-bits %s\n" % " ".join(
            "%d:%d" % (b, c) for b, c in enumerate(corrected) if c)
        files["summary.txt"] = summary.encode()
        print(summary.rstrip())

        if args.full:
            path = os.path.join(args.out, name + "-data.bin")
            total = banks * blocks * ppb
            start = time.time()
            with open(path, "wb") as f:
                for g in range(0, total, 64):
                    n = min(64, total - g)
                    f.write(disk.read(metaend + g * spp, n * spp))
                    if g % 6400 == 0:
                        progress(g + n, total, start, "page data")
            sys.stderr.write("\nfull data: %s\n" % path)

    manifest = "".join("%s  %s\n" % (hashlib.sha256(v).hexdigest(), k)
                       for k, v in files.items())
    files["SHA256SUMS"] = manifest.encode()
    with tarfile.open(out, "w:gz") as tar:
        for k, v in files.items():
            info = tarfile.TarInfo(name + "/" + k)
            info.size = len(v)
            info.mtime = int(time.time())
            tar.addfile(info, io.BytesIO(v))
    print("\nwrote %s - please send this file" % out)


def mkimage(args):
    """A sparse image in the check image's layout, from a 4-bank, 4096-block,
    2KiB dump (nand_data_4banks.bin + meta_all.bin). Only control pages get
    data, which is all collect reads without --full."""
    banks, blocks, ppb, pagesize = 4, 4096, 128, 2048
    metaend = 1 + blocks * banks
    meta = open(os.path.join(args.dumpdir, "meta_all.bin"), "rb").read()
    data = open(os.path.join(args.dumpdir, "nand_data_4banks.bin"), "rb")
    report = ("%s\nversion mkimage\nids a514d3ad a514d3ad a514d3ad a514d3ad\n"
              "banks 4\nrow 3\nmode 8\npagesize 2048\nblocks 4096\nppb 128\n"
              "userblocks 3872\nplanes 2\nvflspares 89\nvalidated 1\nftl 0\n"
              "sectors 1982464\nverdict OK, validated chip\n"
              % MAGIC).encode()
    with open(args.image, "wb") as f:
        f.write(report.ljust(SECTOR, b"\0"))
        for block in range(blocks):
            for bank in range(banks):
                off = (bank * blocks + block) * ppb * RECORD
                recs = bytearray(meta[off:off + ppb * RECORD])
                for p in range(ppb):
                    rc = struct.unpack_from("<i", recs, p * RECORD + 12)[0]
                    word = (0x10000 if rc == 0 else
                            rc | (1 << 8) if rc == 1 else rc)
                    struct.pack_into("<i", recs, p * RECORD + 12, word)
                f.write(recs)
        for g in range(banks * blocks * ppb):
            if is_control(meta[g * RECORD + 9]):
                data.seek(g * pagesize)
                f.seek((metaend + g) * SECTOR)
                f.write(data.read(pagesize))
        f.truncate((metaend + banks * blocks * ppb) * SECTOR)
    print("wrote", args.image)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("collect", help="collect the validation data")
    c.add_argument("disk", nargs="?", default="auto",
                   help="the iPod's disk: /dev/sdX (Linux), /dev/rdiskN "
                        "(macOS), \\\\.\\PhysicalDriveN (Windows), or "
                        "auto (default) to find it")
    c.add_argument("--out", default=".")
    c.add_argument("--full", action="store_true")
    m = sub.add_parser("mkimage", help="testing: image from a dump")
    m.add_argument("dumpdir")
    m.add_argument("image")
    args = ap.parse_args()
    (collect if args.cmd == "collect" else mkimage)(args)


if __name__ == "__main__":
    main()
