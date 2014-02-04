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
#   Copyright (C) 2013 Lorenzo Miori
######################################################################
# bail out early
set -e

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: $0 <files path> [path to unpacked rom]"
    echo "\t<files path> is expected to have a special resources and rootfs layout"
    exit 1
fi

MODEL="unknown"
FILES=${1%/}
FILES=${FILES:-"/"}
COMMON_FILES="$FILES/common"
DIR=${2:-"."}
DIR=${DIR%/}
ROOTFS=$DIR/rootfs
CRAMFS=$DIR/cramfs-fsl.rom

# sanity checks

for subdir in common r0 r1
do
    if [ ! -d "$FILES/$subdir" ]
    then
        echo "Missing $FILES/$subdir. Invalid $FILES layout."
        exit 1
    fi
done

if [ ! -e "$FILES/r1/etc/safemode/cable_detect" ]
then
    echo "Couldn't find cable_detect binary (try 'make' or select a valid layout directory)"
    exit 1
fi

for image in pre_smode.raw post_smode.raw safemode.raw
do
    if [ ! -e "$FILES/r1/etc/safemode/$image" ]
    then
        echo "Missing r1 .raw image file (try 'make'): $image"
        exit 1
    fi
    if [ ! -e "$FILES/r0/etc/safemode/$image" ]
    then
        echo "Missing r0 .raw image file (try 'make'): $image"
        exit 1
    fi
done

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

[ ! -e $ROOTFS ] || rm -R $ROOTFS
cramfs-1.1/cramfsck -x $ROOTFS $CRAMFS

# now we can detect player version
# NOTE: order is important here, since ironically
# r1's ROM contains also r0's executables
if [ -e "$ROOTFS/usr/local/bin/r1" ]
then
    MODEL="r1"
else
    if [ -e "$ROOTFS/usr/local/bin/r0" ]
    then
        MODEL="r0"
    fi
fi

echo "$MODEL ROM found."

echo "Patching rootfs (common files)"
echo "cp -r $COMMON_FILES/* $ROOTFS/"
cp -r $COMMON_FILES/.rockbox $ROOTFS/
cp -r $COMMON_FILES/* $ROOTFS/

echo "Patching rootfs ($MODEL files)"
MODEL_FILES="$FILES/$MODEL"
echo "cp -r $MODEL_FILES/* $ROOTFS/"
cp -r $MODEL_FILES/* $ROOTFS/
rm -rf $ROOTFS/dev/ttyGS0
mknod $ROOTFS/dev/ttyGS0 c 127 0

echo "Packing new cramfs image"
cramfs-1.1/mkcramfs $ROOTFS $CRAMFS
