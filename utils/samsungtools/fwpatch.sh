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
DIR=${2:-"."}
DIR=${DIR%/}
APPFS_TARGET=$DIR/appfs
ROOTFS_TARGET=$DIR/rootfs

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

if [ -z $ROOTFS_TARGET ] || [ -z $APPFS_TARGET ] || [ -z $FILES ]; then
    echo "Invalid input directories"
    exit 1
fi

if [ -e "$DIR/z5-fw.ver" ]
then
    MODEL="z5"
    APPFS_IMAGE=$DIR/Oasis-big.cramfs
    ROOTFS_IMAGE=$DIR/rootfs.cramfs
else
#    if [ -e "$ROOTFS/usr/local/bin/r0" ]
#    then
# TODO detect Q2
        MODEL="q2"
    APPFS_IMAGE=$DIR/appfs.cramfs
    ROOTFS_IMAGE=$DIR/rootfs.sqfs
#    fi
fi

PATCH_FILES="$FILES/$MODEL"

echo "Firmware for $MODEL has been detected"

if [ ! -e $APPFS_IMAGE ]; then
    echo "$APPFS_IMAGE image not found (did you extract the firmware?)"
    exit 1
fi

if [ ! -e $ROOTFS_IMAGE ]; then
    echo "$ROOTFS_IMAGE image not found (did you extract the firmware?)"
    exit 1
fi

unpack_image()
{
    if [ "${1##*.}" = "cramfs" ]
    then
        echo "(cramfs type)"
        cramfs-1.1/cramfsck -x $2 $1
    elif [ "${1##*.}" = "sqfs" ]
    then
        echo "(squashfs type)"
        ../squashfs-tools/unsquashfs -d $2 $1
    fi
}

pack_image()
{
    if [ "${1##*.}" = "cramfs" ]
    then
        echo "(cramfs type)"
        cramfs-1.1/mkcramfs $2 $1
    elif [ "${1##*.}" = "sqfs" ]
    then
        echo "(squashfs type)"
        ../squashfs-tools/mksquashfs $2 $1 -noappend
    fi
}

echo "Extracting appfs image"
[ ! -e $APPFS_TARGET ] || rm -R $APPFS_TARGET
unpack_image $APPFS_IMAGE $APPFS_TARGET

echo "Extracting rootfs image"
[ ! -e $ROOTFS_TARGET ] || rm -R $ROOTFS_TARGET
unpack_image $ROOTFS_IMAGE $ROOTFS_TARGET

echo "Patching rootfs partition"
echo "cp -r $PATCH_FILES/rootfs/* $ROOTFS_TARGET/"
#cp -r $PATCH_FILES/rootfs/.rockbox $ROOTFS_TARGET/
cp -r $PATCH_FILES/rootfs/* $ROOTFS_TARGET/

echo "Patching appfs partition"
echo "cp -r $PATCH_FILES/appfs/* $APPFS_TARGET/"
cp -r $PATCH_FILES/appfs/* $APPFS_TARGET/

echo "Packing new appfs image"
pack_image $APPFS_IMAGE $APPFS_TARGET

echo "Packing new rootfs image"
pack_image $ROOTFS_IMAGE $ROOTFS_TARGET
