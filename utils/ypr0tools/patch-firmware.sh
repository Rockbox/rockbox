#!/bin/sh


######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   * Script to patch an unpacked Samsung YP-R0 firmware file */
#   Copyright (C) 2011 Thomas Martitz
######################################################################
# bail out early
set -e

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: $0 <files path> [path to unpacked rom]"
    echo "\t<files path> is expected to have a rootfs layout and to contain"
    echo "\tonly the files to overwrite (plain cp -r is used)"
    exit 1
fi

FILES=${1%/}
FILES=${FILES:-"/"}
DIR=${2:-"."}
DIR=${DIR%/}
ROOTFS=$DIR/rootfs
CRAMFS=$DIR/cramfs-fsl.rom

# sanity checks

# this needs to be run as root!
if [ $(whoami) != "root" ]
then
  echo "This needs to be run as root"
  exit 1
fi

if [ ! -e $1 ] || [ ! -e $2 ]; then
    echo "$1 or $2 does not exist"
    exit 1
fi

if [ -z $ROOTFS ] || [ -z $FILES ]; then
    echo "Invalid input directories"
    exit 1
fi

if [ ! -e $CRAMFS ]; then
    echo "Cramfs image not found (did you extract the firmware?)"
    exit 1
fi

echo "Extracting cramfs image"

[ ! -e $ROOTFS ] || rmdir -p $ROOTFS
cramfs-1.1/cramfsck -x $ROOTFS $CRAMFS

echo "Patching rootfs"
echo "cp -r $FILES/* $ROOTFS/"
cp -r $FILES/.rockbox $ROOTFS/
cp -r $FILES/* $ROOTFS/

echo "Packing new cramfs image"
cramfs-1.1/mkcramfs $ROOTFS $CRAMFS
