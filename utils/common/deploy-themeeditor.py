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
import sys

deploy.program = "rbthemeeditor"
deploy.project = "utils/themeeditor/themeeditor.pro"
deploy.svnserver = "svn://svn.rockbox.org/rockbox/"
deploy.svnpaths = \
           [ "utils/themeeditor/",
             "lib/skin_parser/",
             "docs/COPYING" ]
deploy.useupx = False
deploy.bundlecopy = {
    "resources/windowicon.icns" : "Contents/Resources/",
    "Info.plist"          : "Contents/"
}
# Windows nees some special treatment. Differentiate between program name
# and executable filename.
if sys.platform == "win32":
    deploy.progexe = "Release/" + deploy.program + ".exe"
    deploy.make = "mingw32-make"
elif sys.platform == "darwin":
    deploy.progexe = deploy.program + ".app"
    # OS X 10.6 defaults to gcc 4.2. Building universal binaries that are
    # compatible with 10.4 requires using gcc-4.0.
    if not "QMAKESPEC" in deploy.environment:
        deploy.environment["QMAKESPEC"] = "macx-g++40"
else:
    deploy.progexe = deploy.program
# all files of the program. Will get put into an archive after building
# (zip on w32, tar.bz2 on Linux). Does not apply on Mac which uses dmg.
deploy.programfiles = [ deploy.progexe ]
deploy.nsisscript = "utils/themeeditor/themeeditor.nsi"

deploy.deploy()

