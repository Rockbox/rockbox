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

set -uo pipefail
IFS=$'\n\t'

CMD=$(command -v sysbench)
TESTDURSEC=300 #5 minutes/test
VERBOSITY=5
BLOCKSZ=('test all' 512 1024 4096 16384 65536 1048576)
THREADS=1 #--validate may fail if threads > 1 (some versions)

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
SIZEKB=${2:-'10*1024'} #10Mb file [default]
SIZEBYTE=$((SIZEKB*1024))
BLOCK=${3:-5} #65535 default
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
    if (( $(ulimit -Sn ) < $(($filenum + 100)) )) ;then
      local hard=$(ulimit -Hn) #get hard limit
      echo attempting to increase soft file limit to $hard
      ulimit -Sn $hard
    fi
################################################################################
    sysbench_cmd=(fileio --validate --file-total-size=$SIZEBYTE --file-block-size=$bs --file-num=$filenum --threads=$THREADS --max-requests=0 --time=$TESTDURSEC --file-extra-flags=direct --verbosity=$VERBOSITY --file-fsync-all=on)
    echo "--------------------------------------------------------------------------------"
    echo "Command Lines to be executed:"
    echo "--------------------------------------------------------------------------------"
    echo "Test Prep:"
    echo ${CMD##*/} " fileio --file-total-size=$SIZEBYTE --file-block-size=$bs --file-num=$filenum prepare"
    echo ""
    echo "Sequential Write:"
    echo ${CMD##*/} "${sysbench_cmd[@]}" --file-test-mode=seqwr run
    echo ""
    echo "Random R/W:"
    echo ${CMD##*/} "${sysbench_cmd[@]}" --file-test-mode=rndrw run
    echo ""
    echo "Sequential Read:"
    echo ${CMD##*/} "${sysbench_cmd[@]}" --file-test-mode=seqrd run
    echo "--------------------------------------------------------------------------------"
    echo ""
################################################################################
    echo "Preparing ${filenum} files, ${SIZEKB}KB file Block Size: ${BLOCKSZ[b]}B..."
    $CMD fileio --file-total-size=$SIZEBYTE --file-block-size=$bs --file-num=$filenum --verbosity=0 prepare
    echo ""
    echo "--------------------------------------------------------------------------------"
    echo "SEQUENTIAL WRITE [$DEV_DIR] BLOCK SIZE: $bs"
    echo "--------------------------------------------------------------------------------"
    echo ""
    out=$($CMD "${sysbench_cmd[@]}" --file-test-mode=seqwr run 2>&1) #exec command
    printf "%s\n" "${out#*Threads started!}"
    echo "--------------------------------------------------------------------------------"
    echo ""
    echo "--------------------------------------------------------------------------------"
    echo "RANDOM WRITE/READ [$DEV_DIR] BLOCK SIZE: $bs"
    echo "--------------------------------------------------------------------------------"
    out=$("$CMD" "${sysbench_cmd[@]}" --file-test-mode=rndrw run 2>&1) #exec command
    printf "%s\n" "${out#*Threads started!}"
    echo "--------------------------------------------------------------------------------"
    echo ""
    echo "--------------------------------------------------------------------------------"
    echo "SEQUENTIAL READ [$DEV_DIR] BLOCK SIZE: $bs"
    echo "--------------------------------------------------------------------------------"
    out=$("$CMD" "${sysbench_cmd[@]}" --file-test-mode=seqrd run 2>&1) #exec command
    printf "%s\n" "${out#*Threads started!}"
    echo "--------------------------------------------------------------------------------"
  done
    echo "--------------------------------------------------------------------------------"
    echo "Finished [$DEV_DIR] $(date)"
    echo "--------------------------------------------------------------------------------"
}
################################################################################
echo "Device Filepath: $1"
echo "Testfile Folder: $TEST_PATH"
echo "Filesize: $SIZEKB kB"
echo "Blocksize (bytes): ${BLOCKSZ[BLOCK]}"
echo "Test Duration (SEC) $TESTDURSEC Each"
echo "Tests: Random R/W, Sequential Write, Sequential Read"
echo ""
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
