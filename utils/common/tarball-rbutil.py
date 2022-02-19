#!/usr/bin/python3
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (c) 2022 Dominik Riebeling
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

import os
import sys
import gitscraper

if len(sys.argv) < 2:
    print("Usage: %s <version|hash>" % sys.argv[0])
    sys.exit()

repository = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "../.."))

if '.' in sys.argv[1]:
    version = sys.argv[1]
    basename = f"RockboxUtility-v{version}-src"
    ref = f"refs/tags/rbutil_{version}"
    refs = gitscraper.get_refs(repository)
    if ref in refs:
        tree = refs[ref]
    else:
        print("Could not find hash for version!")
        sys.exit()
else:
    tree = gitscraper.parse_rev(repository, sys.argv[1])
    basename = f"RockboxUtility-{tree}"

filelist = ["utils/rbutilqt",
            "utils/ipodpatcher",
            "utils/sansapatcher",
            "utils/mkamsboot",
            "utils/chinachippatcher",
            "utils/mks5lboot",
            "utils/mkmpioboot",
            "utils/mkimxboot",
            "utils/mktccboot",
            "utils/bspatch",
            "utils/bzip2",
            "utils/cmake",
            "utils/CMakeLists.txt",
            "tools/ucl",
            "tools/rbspeex",
            "utils/imxtools",
            "utils/nwztools",
            "utils/tomcrypt",
            "lib/rbcodec/codecs/libspeex",
            "docs/COPYING",
            "docs/gpl-2.0.html",
            "docs/logo/rockbox-clef.svg",
            "docs/logo/rockbox-logo.svg",
            "docs/CREDITS",
            "tools/iriver.c",
            "tools/Makefile",
            "tools/mkboot.h",
            "tools/voicefont.c",
            "tools/VOICE_PAUSE.wav",
            "tools/voice-corrections.txt",
            "tools/wavtrim.h",
            "tools/iriver.h",
            "tools/mkboot.c",
            "tools/telechips.c",
            "tools/telechips.h",
            "tools/voicefont.h",
            "tools/wavtrim.c",
            "tools/sapi_voice.vbs"]

print(f"Getting git revision {tree}")
gitscraper.archive_files(repository, tree, filelist, basename, archive="tbz")

print(f"Created {basename}.tar.bz2.")
