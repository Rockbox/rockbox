#!/bin/sh


######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   Script to patch an unpacked Samsung firmware file (YP-Q2)
#   Copyright (C) 2017 Lorenzo Miori
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
Q2_FILES="$FILES/q2"
DIR=${2:-"."}
DIR=${DIR%/}
APPFS=$DIR/appfs
ROOTFS=$DIR/rootfs
CRAMFS=$DIR/appfs.cramfs
SQUASHFS=$DIR/rootfs.sqfs

# sanity checks

for subdir in q2
do
    if [ ! -d "$FILES/$subdir" ]
    then
        echo "Missing $FILES/$subdir. Invalid $FILES layout."
        exit 1
    fi
done

#if [ ! -e "$FILES/r1/etc/safemode/cable_detect" ]
#then
#    echo "Couldn't find cable_detect binary (try 'make' or select a valid layout directory)"
#    exit 1
#fi

#for image in pre_smode.raw post_smode.raw safemode.raw
#do
#    if [ ! -e "$FILES/r1/etc/safemode/$image" ]
#    then
#        echo "Missing r1 .raw image file (try 'make'): $image"
#        exit 1
#    fi
#    if [ ! -e "$FILES/r0/etc/safemode/$image" ]
#    then
#        echo "Missing r0 .raw image file (try 'make'): $image"
#        exit 1
#    fi
#done

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

if [ -z $ROOTFS ] || [ -z $APPFS ] || [ -z $FILES ]; then
    echo "Invalid input directories"
    exit 1
fi

if [ ! -e $CRAMFS ]; then
    echo "$CRAMFS image not found (did you extract the firmware?)"
    exit 1
fi

if [ ! -e $SQUASHFS ]; then
    echo "$SQUASHFS image not found (did you extract the firmware?)"
    exit 1
fi

echo "Extracting cramfs image"

[ ! -e $APPFS ] || rm -R $APPFS
cramfs-1.1/cramfsck -x $APPFS $CRAMFS

echo "Extracting the squashfs image"
[ ! -e $ROOTFS ] || rm -R $ROOTFS
../squashfs-tools/unsquashfs -d $ROOTFS $SQUASHFS

# now we can detect player version
# NOTE: order is important here, since ironically
# r1's ROM contains also r0's executables

#if [ -e "$ROOTFS/usr/local/bin/r1" ]
#then
#    MODEL="r1"
#else
#    if [ -e "$ROOTFS/usr/local/bin/r0" ]
#    then
#        MODEL="r0"
#    fi
#fi


echo "Patching rootfs partition"
echo "cp -r $Q2_FILES/rootfs/* $ROOTFS/"
#cp -r $Q2_FILES/rootfs/.rockbox $ROOTFS/
cp -r $Q2_FILES/rootfs/* $ROOTFS/

echo "Patching appfs partition"
echo "cp -r $Q2_FILES/appfs/* $APPFS/"
cp -r $Q2_FILES/appfs/* $APPFS/

echo "Packing new cramfs image"
cramfs-1.1/mkcramfs $APPFS $CRAMFS

echo "Packing new squashfs image"
../squashfs-tools/mksquashfs $ROOTFS $SQUASHFS -noappend
