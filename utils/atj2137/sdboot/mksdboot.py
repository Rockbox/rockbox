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
"""Build ATJ213x boot images from sdboot stages and a payload.

sd   - image to be written to SD card starting at LBA 1:
       dd if=sdboot.img of=/dev/sdX bs=512 seek=1 conv=notrunc
       BROM boots it when NAND boot fails.
adfu - stage2 for adfuload (payload appended):
       adfuload -s1 ADEC_N63.BIN -s2 brecload.bin
"""

import argparse
import os
import struct
import sys

SECTOR = 512
STAGE15_LBA = 2
STAGE15_MAX_SIZE = 0x1600
PAYLOAD_LBA = STAGE15_LBA + STAGE15_MAX_SIZE // SECTOR

SDBOOT_MAGIC = 0x54424453
SDBOOT_SDRAM_INIT = 1 << 0
SDBOOT_PAYLOAD_SD = 1 << 1
HDR_OFFSET = 8

BREC_ADDR = 0xb4048000

MBRC_SIG_OFFSET = 0x1f4
MBRC_SIG = struct.pack('<II', 0x42746341, 0x55aa6d72)  # "ActBrm" 0xaa55


def mbrc_checksum(sector):
    words = struct.unpack('<127I', sector[:0x1fc])
    return (sum(words) + 0x1234) & 0xffffffff


def mbrc_seal(code):
    if len(code) > MBRC_SIG_OFFSET:
        sys.exit('stage1 too big: %d bytes (max %d)' % (len(code), MBRC_SIG_OFFSET))
    sector = bytearray(code.ljust(MBRC_SIG_OFFSET, b'\0')) + MBRC_SIG
    sector += struct.pack('<I', mbrc_checksum(sector))
    return bytes(sector)


def mbrc_valid(sector):
    return (sector[MBRC_SIG_OFFSET:MBRC_SIG_OFFSET + 8] == MBRC_SIG and
            struct.unpack('<I', sector[0x1fc:0x200])[0] == mbrc_checksum(sector))


def set_header(stage, load, entry, size, lba, flags):
    stage = bytearray(stage)
    magic = struct.unpack_from('<I', stage, HDR_OFFSET)[0]
    if magic != SDBOOT_MAGIC:
        sys.exit('stage1.5 header magic not found')
    struct.pack_into('<6I', stage, HDR_OFFSET, SDBOOT_MAGIC, load, entry, size, lba, flags)
    return bytes(stage)


def pad(data, align=SECTOR):
    return data + b'\0' * (-len(data) % align)


def read(path):
    with open(path, 'rb') as f:
        return f.read()


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('mode', choices=['sd', 'adfu', 'check'])
    p.add_argument('payload', help='payload binary (or sector to check)')
    p.add_argument('-o', '--output')
    p.add_argument('--load', type=lambda x: int(x, 0),
                   help='payload load address (default 0xa0000000, BREC: 0x%x)' % BREC_ADDR)
    p.add_argument('--entry', type=lambda x: int(x, 0), help='entry point (default load)')
    p.add_argument('--brec', action='store_true', help='payload is BREC (load/entry 0x%x)' % BREC_ADDR)
    p.add_argument('--sdram-init', action='store_true',
                   help='adfu mode: init SDRAM like MBRC does (sd mode always does)')
    p.add_argument('--stagedir', default=here, help='directory with stage binaries')
    a = p.parse_args()

    payload = read(a.payload)

    if a.mode == 'check':
        ok = mbrc_valid(payload[:SECTOR])
        print('MBRC check: %s' % ('OK' if ok else 'FAILED'))
        return 0 if ok else 1

    if not a.output:
        p.error('--output required')

    load = BREC_ADDR if a.brec else (a.load if a.load is not None else 0xa0000000)
    entry = a.entry if a.entry is not None else load

    if a.mode == 'sd':
        stage1 = mbrc_seal(read(os.path.join(a.stagedir, 'stage1.bin')))
        stage15 = read(os.path.join(a.stagedir, 'stage15_sd.bin'))
        if len(stage15) > STAGE15_MAX_SIZE:
            sys.exit('stage1.5 too big')
        stage15 = set_header(stage15, load, entry, len(payload), PAYLOAD_LBA,
                             SDBOOT_SDRAM_INIT | SDBOOT_PAYLOAD_SD)
        img = stage1 + stage15.ljust(STAGE15_MAX_SIZE, b'\0') + pad(payload)
        last_lba = 1 + len(img) // SECTOR - 1
        print('sd image: LBA 1..%d (%d bytes), payload at LBA %d, load 0x%08x entry 0x%08x'
              % (last_lba, len(img), PAYLOAD_LBA, load, entry))
        print('first partition on the card must start after LBA %d' % last_lba)
    else:
        stage15 = read(os.path.join(a.stagedir, 'stage15_adfu.bin'))
        flags = SDBOOT_SDRAM_INIT if a.sdram_init else 0
        img = set_header(stage15, load, entry, len(payload), 0, flags) + payload
        print('adfu image: %d bytes, load 0x%08x entry 0x%08x' % (len(img), load, entry))

    with open(a.output, 'wb') as f:
        f.write(img)
    return 0


if __name__ == '__main__':
    sys.exit(main())
