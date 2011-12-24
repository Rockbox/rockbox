#!/bin/bash

######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   * Script to generate a Samsung YP-R0 firmware file (R0.ROM) */
######################################################################
#
# This file was oringally called NewPack.sh, its origin is the R0 open source
# package from Samsung.
#
# Muon Platform
# Copyright (c) 2004-2009 Samsung Electronics, Inc.
# All rights reserved.
#
# Rom Packaging Script
# It needs sudoer privilege of rm, mkdir, cp, mkcramfs.
# You can configure it in the /etc/sudoer file.
# This script is very dangerous. Be careful to use.
#
# SangMan Sim<sangman.sim@samsung.com>

# bail out early
set -e

DIR=${2:-"."}
DIR=${DIR%/}
REVISION="$DIR/RevisionInfo.txt"
CRAMFS="$DIR/cramfs-fsl.rom"
SYSDATA="$DIR/SYSDATA.bin"
MBOOT="$DIR/MBoot.bin"
MBOOT_TMP="${TMP_DIR:-$DIR}/MBoot.tmp"
LINUX="$DIR/zImage"
R0ROM=$1

# some sanity checks
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage $0 <rom file> [path to image files]"
    exit 1
fi

if [ ! -f ./MuonEncrypt ]; then
    echo "Couldn't find MuonEncrypt binary (try 'make')"
    exit 1
fi

if [ ! -e $REVISION ]; then
    cat >$REVISION <<EOF
Version : V2.30
Target : KR
EOF
fi


function WriteImage {
    echo "Adding $1 to $R0ROM"
	#HEAD_STR=[`stat -c%s $1`/`md5sum $1 | cut -d " " -f 1`]
	#HEAD_SIZE=`echo $HEAD_STR | wc -c`
	#PACK_SIZE=`expr 44 - $HEAD_SIZE`

	#while [ $PACK_SIZE -gt 0 ]
	#do
		#PACK_SIZE=`expr $PACK_SIZE - 1`
		#echo -n 0
	#done

	./MuonEncrypt $1 >> $R0ROM
	#cat $MBOOT >> $R0ROM
}

function Pack4Byte {
	FILE_SIZE=`stat -c%s $R0ROM`
	PACK_SIZE=`expr 4 - $FILE_SIZE % 4`

	if [ $PACK_SIZE != 4 ]
	then
		while [ $PACK_SIZE -gt 0 ]
		do
			PACK_SIZE=`expr $PACK_SIZE - 1` || true
			echo -en $1 >> $R0ROM
		done
	fi

}

echo Make $R0ROM

cat $REVISION > $R0ROM
echo User : $USER >> $R0ROM
echo Dir : $PWD >> $R0ROM
echo BuildTime : `date "+%y/%m/%d %H:%M:%S"` >> $R0ROM
echo MBoot : size\(`stat -c%s $MBOOT`\),checksum\(`md5sum $MBOOT | cut -d " " -f 1`\) >> $R0ROM
echo Linux : size\(`stat -c%s $LINUX`\),checksum\(`md5sum $LINUX | cut -d " " -f 1`\) >> $R0ROM
echo RootFS : size\(`stat -c%s $CRAMFS`\),checksum\(`md5sum $CRAMFS | cut -d " " -f 1`\) >> $R0ROM
echo Sysdata : size\(`stat -c%s $SYSDATA`\),checksum\(`md5sum $SYSDATA | cut -d " " -f 1`\) >> $R0ROM

Pack4Byte "\\n"


dd if=$MBOOT of=$MBOOT_TMP bs=96 count=1 2> /dev/null

echo `stat -c%s $MBOOT`:`md5sum $MBOOT | cut -d " " -f 1` >> $MBOOT_TMP
echo `stat -c%s $LINUX`:`md5sum $LINUX | cut -d " " -f 1` >> $MBOOT_TMP
echo `stat -c%s $CRAMFS`:`md5sum $CRAMFS | cut -d " " -f 1` >> $MBOOT_TMP
echo `stat -c%s $SYSDATA`:`md5sum $SYSDATA | cut -d " " -f 1` >> $MBOOT_TMP

dd if=$MBOOT of=$MBOOT_TMP bs=1088 skip=1 seek=1 2> /dev/null
WriteImage $MBOOT_TMP

#rm $MBOOT_TMP

Pack4Byte "0"

WriteImage $LINUX

Pack4Byte "0"

WriteImage $CRAMFS

Pack4Byte "0"

WriteImage $SYSDATA

echo $R0ROM : `stat -c%s $R0ROM`, `md5sum $R0ROM | cut -d " " -f 1`
#head -9 $R0ROM

echo "Done"
