#!/bin/bash

######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   * Script to unpack a Samsung YP-R0 firmware file (R0.ROM) */
######################################################################

# The file was originally called MuonDecrypt.sh
#
# I'm not sure about the original author of this file, as it wasn't included in Samsung package.
# But I guess it was done by JeanLouis, an Italian user of the Hardware Upgrade Forum. If needed, we should search throug old posts for that...
#


# bail out early
set -e

# some sanity checks
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage $0 <rom file> [out dir]"
    exit 1
fi


ROM=$1
DIR=${2:-"."}
DIR=${DIR%/}
MBOOT="$DIR/MBoot.bin"
MBOOT_TMP="${TMP_DIR:-$DIR}/MBoot.tmp"
LINUX="$DIR/zImage"
CRAMFS="$DIR/cramfs-fsl.rom"
SYSDATA="$DIR/SYSDATA.bin"
MD5SUMS="$DIR/MD5SUMS"
TMP="${TMP_DIR:-$DIR}/_$$.tmp"


if [ ! -f ./extract_section ]; then
    echo "Couldn't find extract_section binary (try 'make')"
    exit 1
fi

if [ ! -f ./MuonEncrypt ]; then
    echo "Couldn't find MuonEncrypt binary (try 'make')"
    exit 1
fi

mkdir -p $DIR

if [ ! -w $DIR ]; then
    echo "Target dir not writable"
    exit 1
fi

ExtractAndDecrypt() {
	START=$(expr $START - $2)
	echo "Extracting $1..."
	./extract_section $ROM $TMP $START $2
	echo "Decrypt $1..."
	./MuonEncrypt $TMP > $1
}

size=( `head -n 9 $ROM | tail -n 4 | while read LINE; do echo $LINE | cut -d\( -f 2 | cut -d\) -f 1; done`)
checksum=( `head -n 9 $ROM | tail -n 4 | while read LINE; do echo $LINE | cut -d\( -f 3 | cut -d\) -f 1; done`)

echo "${checksum[0]}  $MBOOT" > $MD5SUMS
echo "${checksum[1]}  $LINUX" >> $MD5SUMS
echo "${checksum[2]}  $CRAMFS" >> $MD5SUMS
echo "${checksum[3]}  $SYSDATA" >> $MD5SUMS

START=`stat -c%s $ROM`

ExtractAndDecrypt $SYSDATA ${size[3]}
ExtractAndDecrypt $CRAMFS ${size[2]}
ExtractAndDecrypt $LINUX ${size[1]}
ExtractAndDecrypt $MBOOT_TMP ${size[0]}

rm $TMP
echo "Create $MBOOT..."
dd if=$MBOOT_TMP of=$MBOOT bs=96 count=1 2>/dev/null
dd if=$MBOOT_TMP of=$MBOOT bs=1088 skip=1 seek=1 2>/dev/null
rm $MBOOT_TMP

echo "Check integrity:"
md5sum -c $MD5SUMS
