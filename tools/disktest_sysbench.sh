#!/bin/bash
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2020 William Wilgus
################################################################################
#
# chmod +x disktest_sysbench.sh
#
# apt install sysbench
#
# ./disktest_sysbench.sh /device path filesz(kb) block > ./disklog.txt
#
################################################################################
echo "Disktest Benchmarking [] $(date)"
set -eu

CMD=$(command -v sysbench)
TESTDURSEC=240
VERBOSITY=4
BLOCKSZ=('test all' 512 1024 4096 16384 65535 1048576)
SYSBENCHCMD="--file-fsync-all[=on"
if [ -z "$CMD"  ]; then
 echo "This script requires 'sysbench' try: 'apt install sysbench' or your system package handler"
 exit 1
else
 echo "using sysbench found @ $CMD"
 echo ""
fi

if [ $# -lt 1 ]; then
  echo "Usage: $0 <filepath_to_device>, <filesize_in_kB> <block 0-${#BLOCKSZ[@]}>"
  exit 1
fi

DEV_DIR=$1
TEST_PATH="$DEV_DIR/disktest_sysbench"
SIZEKB=${2:-'1024'}
SIZEBYTE=$((SIZEKB*1024))
BLOCK=${3:-4}
if (($BLOCK > ${#BLOCKSZ[@]})) ;then
  $BLOCK = 0
fi

BeginDiskTest() {
  local start=$BLOCK
  local blocks=$BLOCK+1
  local bs=0
  local cwd="/"
  local filenum=0
  local sysbench_cmd
  local out

  if (( blocks == 1 )) ;then
    start=1
    blocks=${#BLOCKSZ[@]}
  fi

  for (( b=$start; b<$blocks; b++ ))
  do
    bs=${BLOCKSZ[b]}
    filenum=$(( SIZEBYTE / bs ))
    # note try bigger block sizes if you run out of file handles
    if (( $(ulimit -Sn ) < $(($filenum * 2)) )) ;then
      ulimit -Sn $(($filenum * 2))
    fi
    sysbench_cmd=(fileio --file-total-size=$SIZEBYTE --file-block-size=$bs --file-num=$filenum --threads=2 --max-requests=0 --time=$TESTDURSEC --file-extra-flags=direct --verbosity=$VERBOSITY --file-fsync-all=on )
    echo "--------------------------------------------------------------------------------"
    echo "Preparing ${filenum} files, ${SIZEKB}KB file Block Size: ${BLOCKSZ[b]}B..."
    echo "sysbench fileio --file-total-size=$SIZEBYTE --file-block-size=$bs --file-num=$filenum prepare"
    $CMD fileio --file-total-size=$SIZEBYTE --file-block-size=$bs --file-num=$filenum --verbosity=2 prepare

    echo "--------------------------------------------------------------------------------"
    echo "RANDOM WRITE/READ [$TEST_PATH] BLOCK SIZE: $bs"
    echo "--------------------------------------------------------------------------------"

    echo ${CMD##*/} "${sysbench_cmd[@]}" --file-test-mode=rndrw run
    out=$($CMD "${sysbench_cmd[@]}" --file-test-mode=rndrw run)
    echo "--------------------------------------------------------------------------------"
    echo "--------------------------------------------------------------------------------"
    echo "SEQUENTIAL WRITE [$TEST_PATH] BLOCK SIZE: $bs"
    echo "--------------------------------------------------------------------------------"

    echo ${CMD##*/} "${sysbench_cmd[@]}" --file-test-mode=seqwr run
    out=$($CMD "${sysbench_cmd[@]}" --file-test-mode=seqwr run)
    echo "--------------------------------------------------------------------------------"

    echo "--------------------------------------------------------------------------------"
    echo "SEQUENTIAL READ [$TEST_PATH] BLOCK SIZE: $bs"
    echo "--------------------------------------------------------------------------------"
    echo ${CMD##*/} "${sysbench_cmd[@]}" --file-test-mode=seqrd run
    out=$($CMD "${sysbench_cmd[@]}" --file-test-mode=seqrd run)
    echo "--------------------------------------------------------------------------------"
  done
    echo "--------------------------------------------------------------------------------"
    echo "Finished [$TEST_PATH] $(date)"
    echo "--------------------------------------------------------------------------------"
}
################################################################################
echo "Device Filepath: $1"
echo "Testfile Folder: $TEST_PATH"
echo "Filesize: $SIZEKB kB"
echo "Blocksize (bytes): ${BLOCKSZ[BLOCK]}"
echo "Test Duration (SEC) $TESTDURSEC"

echo "Ready to create test files on device @ $TEST_PATH"
echo Continue Y/n?
read prompt
if [ "$prompt" != "${prompt#[Yy]}" ] ;then
  [ -d $TEST_PATH ] || mkdir $TEST_PATH
  cd "$TEST_PATH"
  cwd=$(pwd -P)
  if [ "$cwd" = "$TEST_PATH" ] ;then
    BeginDiskTest
  else
    echo "couldn't cd to device directory "
    exit 2
  fi
else
    echo "exiting"
    exit 0
fi

exit 0
