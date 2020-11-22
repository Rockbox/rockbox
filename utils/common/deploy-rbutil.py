#!/usr/bin/python
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (c) 2010 Dominik Riebeling
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

import deploy

deploy.program = "RockboxUtility"
deploy.project = "rbutil/rbutilqt/rbutilqt.pro"
deploy.svnserver = "svn://svn.rockbox.org/rockbox/"
deploy.svnpaths = \
           ["rbutil/",
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
deploy.useupx = False
deploy.bundlecopy = {
    "icons/rbutilqt.icns" : "Contents/Resources/",
    "Info.plist"          : "Contents/"
}
deploy.progexe = {
    "win32"    : "release/RockboxUtility.exe",
    "darwin"   : "RockboxUtility.app",
    "linux2"   : "RockboxUtility",
    "linux"    : "RockboxUtility"
}
deploy.regreplace = {
    "rbutil/rbutilqt/version.h"  : [["\$Rev\$", "%REVISION%"],
                                    ["(^#define BUILDID).*", "\\1 \"%BUILDID%\""]],
    "rbutil/rbutilqt/Info.plist" : [["\$Rev\$", "%REVISION%"]],
}
# OS X 10.6 defaults to gcc 4.2. Building universal binaries that are
# compatible with 10.4 requires using gcc-4.0.
deploy.qmakespec = {
    "win32"    : "",
    "darwin"   : "macx-g++40",
    "linux2"   : "",
    "linux"    : ""
}
deploy.make = {
    "win32"    : "mingw32-make",
    "darwin"   : "make",
    "linux2"   : "make",
    "linux"    : "make"
}

# all files of the program. Will get put into an archive after building
# (zip on w32, tar.bz2 on Linux). Does not apply on Mac which uses dmg.
# progexe will get added automatically.
deploy.programfiles = list()
deploy.nsisscript = ""

deploy.deploy()
