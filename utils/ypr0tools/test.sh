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

ROM_FILE="$1"
TMP_FOLDER=""

goto_temp()
{
    if [ -n "$TMP_FOLDER" ]
    then
        cd $TMP_FOLDER
    fi
}

cleanup()
{
    echo "$1"
    OLD_DIR=`pwd`
    goto_temp
    rm -f "$ROM_FILE"_TEST_CRYPT "MBoot.bin" "zImage" "cramfs-fsl.rom" "SYSDATA.bin" "TEST_MD5SUMS" "RevisionInfo.txt" > /dev/null
    cd $OLD_DIR
    if [ -n "$TMP_FOLDER" ]
    then
        rmdir $TMP_FOLDER
    fi
    make clean
    exit $2
}

if [ $# -lt 1 ]
then
    cleanup "FAIL: Missing parameter! Run with: test.sh <path to working rom to test> <optional: destination to temporary files>" 1
fi

if [ $# -eq 2 ]
then
    TMP_FOLDER="$2/"
    mkdir $TMP_FOLDER
    if [ $? -ne 0 ]
    then
        echo "FAIL: temporary directory exists!"
    fi
fi

# be sure we have the executables up-to-date
make clean
make

./fwdecrypt $1 $TMP_FOLDER
if [ $? -ne 0 ]
then
    cleanup "FAIL: Error while decrypting ROM file" 1
fi

./fwcrypt $TMP_FOLDER$1_TEST_CRYPT $TMP_FOLDER
if [ $? -ne 0 ]
then
    cleanup "FAIL: Error while decrypting ROM file" 1
fi

OLD_DIR=`pwd`
goto_temp

md5sum MBoot.bin zImage cramfs-fsl.rom SYSDATA.bin RevisionInfo.txt > "TEST_MD5SUMS"

md5sum --strict -c "TEST_MD5SUMS"
if [ $? -ne 0 ]
then
    cleanup "FAIL: MD5SUM mismatch!" 1
fi

cd $OLD_DIR

cleanup "OK: test completed without errors." 0
