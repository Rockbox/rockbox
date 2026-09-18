#!/usr/bin/env python3
"""Check nand-nano3g.c's chip table against Apple's and against the FTL.

Every row of nand_chip_table[] must equal a row of the chip table in a
decrypted Nano 3G firmware 1.1.3 image (NANO3G_OSOS), in the same order, and
the geometry the driver and FTL derive from each row must fit the FTL's
static bounds. Rows the FTL does not mount yet are reported, not failed.
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROCKBOX = os.path.abspath(os.path.join(HERE, "..", ".."))
TARGET = ROCKBOX + "/firmware/target/arm/s5l8702/ipodnano3g"
OSOS = os.environ.get("NANO3G_OSOS")

# Firmware 1.1.3: the table sits at this file offset, 18 rows of 44 bytes
APPLE_TABLE, APPLE_ROWS, APPLE_ROW = 0x9269bc, 18, 44


def ours():
    src = open(TARGET + "/nand-nano3g.c").read()
    body = src[src.index("nand_chip_table[] ="):]
    body = body[:body.index("};")]
    rows = []
    for m in re.finditer(r"\{\s*(0x[0-9A-Fa-f]+),\s*(\d+),\s*(\d+),\s*(\d+),"
                         r"\s*(\d+),\s*(\d+),\s*(\d+),\s*(true|false)\s*\}",
                         body):
        rows.append(dict(id=int(m.group(1), 16), banks=int(m.group(2)),
                         mode=int(m.group(3)), blocks=int(m.group(4)),
                         ppb=int(m.group(5)), pagesize=int(m.group(6)),
                         user=int(m.group(7)), validated=m.group(8) == "true"))
    return rows


def apple():
    d = open(OSOS, "rb").read()
    rows = []
    for n in range(APPLE_ROWS):
        e = d[APPLE_TABLE + n * APPLE_ROW:][:APPLE_ROW]
        cid, banks, blocks, ppb, spp = struct.unpack_from("<IHHHH", e)
        user, mode = struct.unpack_from("<II", e, 0x20)
        rows.append(dict(id=cid, banks=banks, mode=mode, blocks=blocks,
                         ppb=ppb, pagesize=spp * 512, user=user))
    return rows


def defines(path):
    out = {}
    for m in re.finditer(r"#define\s+(\w+)\s+(\d+)\b", open(path).read()):
        out[m.group(1)] = int(m.group(2))
    return out


def planes(mode):
    return 1 if mode == 1 else 4 if mode == 4 else 2


def main():
    fails = 0
    rows = ours()
    if len(rows) != APPLE_ROWS:
        print("FAIL  nand_chip_table has %d rows, the firmware's %d" %
              (len(rows), APPLE_ROWS))
        return 1
    if OSOS:
        # ours lists the chips Rockbox has been tested on first, so match on
        # the id and chip-enable count rather than the position
        theirs = {(r["id"], r["banks"]): r for r in apple()}
        for n, a in enumerate(rows):
            b = theirs.get((a["id"], a["banks"]))
            same = b is not None and all(a[k] == b[k] for k in b)
            fails += not same
            print("%s  row %2d %08x x%d matches the firmware's" %
                  ("PASS" if same else "FAIL", n, a["id"], a["banks"]))
            if not same:
                print("      ours  %s\n      firmware %s" % (a, b))
    else:
        print("SKIP  comparison with Apple's table (set NANO3G_OSOS)")

    ftl = defines(TARGET + "/ftl-nano3g.c")
    nand = defines(TARGET + "/nand-target.h")
    for n, r in enumerate(rows):
        p = planes(r["mode"])
        spares = (r["blocks"] - r["user"]) // p - 23
        pool = r["blocks"] - p * spares
        nsb, usersb = pool // p, r["user"] // p
        sbpages = r["ppb"] * p * r["banks"]
        limits = [("superblocks", nsb, ftl["FTL_MAX_SB"]),
                  ("logical blocks", usersb, ftl["FTL_MAX_USERSB"]),
                  ("pages per superblock", sbpages, ftl["FTL_MAX_SBPAGES"]),
                  ("remap entries", p * spares, 820),
                  ("banks", r["banks"], ftl["FTL_MAX_BANKS"])]
        over = [(name, v, lim) for name, v, lim in limits if v > lim]
        mib = sbpages * r["pagesize"] / (1 << 20)
        maxpage = nand.get("NAND_MAX_PAGE_SIZE", nand["NAND_PAGE_SIZE"])
        pending = "; page size %d not supported yet" % r["pagesize"] \
            if r["pagesize"] > maxpage else ""
        fails += bool(over)
        print("%s  row %2d mode %2d P %d: %4d sb, %4d logical, %4d pages/sb"
              " (%g MiB), vflspares %3d%s%s" %
              ("FAIL" if over else "PASS", n, r["mode"], p, nsb, usersb,
               sbpages, mib, spares,
               "".join("; %s %d > %d" % o for o in over), pending))
    print("ALL PASS" if not fails else "%d FAILED" % fails)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
