#!/bin/sh

######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   Script to test packer and unpacker
#   Copyright (C) 2013 Lorenzo Miori
######################################################################

cleanup()
{
    echo "$1"
    rm "MBoot.bin" "zImage" "cramfs-fsl.rom" "SYSDATA.bin" "TEST_MD5SUMS" "RevisionInfo.txt"
    exit $2
}

if [ $# -lt 1 ]
then
    cleanup "Missing parameter! Run with: test.sh <path to working rom to test>" 1
fi

# be sure we have the executables up-to-date
make

./fwdecrypt $1
if [ $? -ne 0 ]
then
    cleanup "Error while decrypting ROM file" 1
fi

md5sum MBoot.bin zImage cramfs-fsl.rom SYSDATA.bin RevisionInfo.txt > "TEST_MD5SUMS"

./fwcrypt $1
if [ $? -ne 0 ]
then
    cleanup "Error while decrypting ROM file" 1
fi

md5sum --strict -c "TEST_MD5SUMS"
if [ $? -ne 0 ]
then
    cleanup "MD5SUM mismatch!" 1
fi

cleanup "OK: test completed without errors." 0
