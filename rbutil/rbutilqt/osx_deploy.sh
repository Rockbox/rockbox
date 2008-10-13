#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (c) 2007 Barry Wardell
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
#
# Purpose of this script: Prepare Mac OS X version of rbutil for distribution.
#
# Inputs: None
#
# Action: Copy necessary Qt frameworks into the rbutilqt application bundle.
#         Update the linking of rbutilqt and the frameworks to refer to the new
#         new location of the framework files (inside the app bundle).
#         Create a compressed disk image from rbutilqt.app.
#
# Output: A compressed disk image called rbutilqt.dmg
#
# Requirement: This script assumes that you have rbutilqt.app in the current
#         directory and a framework build of Qt in your $PATH
#
# See http://doc.trolltech.com/4.3/deployment-mac.html for an explanation of
# what we're doing here.

# scan the $PATH for the given command
findtool(){
  file="$1"

  IFS=":"
  for path in $PATH
  do
    # echo "checks for $file in $path" >&2
    if test -f "$path/$file"; then
      echo "$path/$file"
      return
    fi
  done
}

# Try to find Qt
QTDIR=`findtool qmake`
if test -z "$QTDIR"; then
    # not in path
    echo "Error: Couldn't find Qt. Make sure it is in your \$PATH"
    exit
else
    QTDIR=`dirname \`dirname $QTDIR\``
    echo "Found Qt: $QTDIR"
fi

# Check Qt was built as frameworks
if test ! -d "$QTDIR/lib/QtCore.framework"; then
    echo "Error: Unable to find Qt frameworks in $QTDIR"
    echo "       Make sure you built Qt as frameworks and not statically."
    exit
fi

mkdir -p rbutilqt.app/Contents/Frameworks/{QtCore,QtGui,QtNetwork}.framework/Versions/4

echo "Copying frameworks"
cp $QTDIR/lib/QtCore.framework/Versions/4/QtCore  rbutilqt.app/Contents/Frameworks/QtCore.framework/Versions/4/
cp $QTDIR/lib/QtGui.framework/Versions/4/QtGui  rbutilqt.app/Contents/Frameworks/QtGui.framework/Versions/4/
cp $QTDIR/lib/QtNetwork.framework/Versions/4/QtNetwork  rbutilqt.app/Contents/Frameworks/QtNetwork.framework/Versions/4/

echo "Fixing framework linking"
install_name_tool -id @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore rbutilqt.app/Contents/Frameworks/QtCore.framework/Versions/4/QtCore
install_name_tool -id @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui rbutilqt.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui
install_name_tool -id @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork rbutilqt.app/Contents/Frameworks/QtNetwork.framework/Versions/4/QtNetwork
install_name_tool -change $QTDIR/lib/QtCore.framework/Versions/4/QtCore  @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore rbutilqt.app/Contents/MacOS/rbutilqt 
install_name_tool -change $QTDIR/lib/QtGui.framework/Versions/4/QtGui  @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui rbutilqt.app/Contents/MacOS/rbutilqt 
install_name_tool -change $QTDIR/lib/QtNetwork.framework/Versions/4/QtNetwork  @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork rbutilqt.app/Contents/MacOS/rbutilqt 
install_name_tool -change $QTDIR/lib/QtCore.framework/Versions/4/QtCore  @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore rbutilqt.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui
install_name_tool -change $QTDIR/lib/QtCore.framework/Versions/4/QtCore  @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore rbutilqt.app/Contents/Frameworks/QtNetwork.framework/Versions/4/QtNetwork

echo "insert accessibility plugins"

mkdir -p rbutilqt.app/Contents/plugins/accessible

cp $QTDIR/plugins/accessible/*.dylib rbutilqt.app/Contents/plugins/accessible

install_name_tool -change $QTDIR/lib/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore rbutilqt.app/Contents/plugins/accessible/libqtaccessiblewidgets.dylib
install_name_tool -change $QTDIR/lib/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui rbutilqt.app/Contents/plugins/accessible/libqtaccessiblewidgets.dylib
install_name_tool -change $QTDIR/lib/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore rbutilqt.app/Contents/plugins/accessible/libqtaccessiblecompatwidgets.dylib
install_name_tool -change $QTDIR/lib/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui rbutilqt.app/Contents/plugins/accessible/libqtaccessiblecompatwidgets.dylib

echo "Creating disk image"
hdiutil create -srcfolder rbutilqt.app -ov rbutilqt.dmg
