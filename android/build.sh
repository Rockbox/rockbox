#!/bin/sh
############################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2010 by Maurus Cuelenaere
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

# TODO: convert this into a Makefile

ROCKBOX_DIR=`dirname $0`

if [ -z "$ANDROID_SDK_PATH" ]; then
    echo "Please set \$ANDROID_SDK_PATH!"
    exit 0
fi

if [ -z "$ANDROID_PLATFORM_VERSION" ]; then
    ANDROID_PLATFORM_VERSION=8
fi

ANDROID_PLATFORM="$ANDROID_SDK_PATH/platforms/android-$ANDROID_PLATFORM_VERSION"
AAPT="$ANDROID_PLATFORM/tools/aapt"
DX="$ANDROID_PLATFORM/tools/dx"
APKBUILDER="$ANDROID_SDK_PATH/tools/apkbuilder"

if [ \! -d "$ANDROID_PLATFORM" ]; then
    echo "Can't find Android platform v$ANDROID_PLATFORM_VERSION!"
    exit 0
fi

if [ -d "$ROCKBOX_DIR/bin" ]; then
    echo "[CLEAN]      bin/"
    rm -rf $ROCKBOX_DIR/bin
fi

mkdir $ROCKBOX_DIR/bin

echo "[AAPT]       bin/resources.ap_"
$AAPT package -f -m -J $ROCKBOX_DIR/gen -M $ROCKBOX_DIR/AndroidManifest.xml -S $ROCKBOX_DIR/res -I $ANDROID_PLATFORM/android.jar -F $ROCKBOX_DIR/bin/resources.ap_

for file in `find $ROCKBOX_DIR \( -wholename '*src/*' -o -wholename '*gen/*' \) -a -name '*.java'`; do
    echo "[JAVAC]      `echo $file | sed 's/'$ROCKBOX_DIR'\///'`"
    javac -d $ROCKBOX_DIR/bin -classpath $ANDROID_PLATFORM/android.jar:$ROCKBOX_DIR/bin -sourcepath $ROCKBOX_DIR/src:$ROCKBOX_DIR/gen $file
done

echo "[DEX]        bin/classes.dex"
$DX --dex --output=$ROCKBOX_DIR/bin/classes.dex $ROCKBOX_DIR/bin

echo "[APKBUILDER] bin/Rockbox.apk"
$APKBUILDER $ROCKBOX_DIR/bin/Rockbox.apk -u -z $ROCKBOX_DIR/bin/resources.ap_ -f $ROCKBOX_DIR/bin/classes.dex -nf $ROCKBOX_DIR/libs
