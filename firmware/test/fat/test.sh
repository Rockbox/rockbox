#!/bin/sh

IMAGE=disk.img
MOUNT=/mnt/dummy

fail() {
    echo "!! Test failed. Look in result.txt for test log."
    exit
}

check() {
    /sbin/dosfsck -r $IMAGE | tee -a result.txt
    [ $RETVAL -ne 0 ] && fail
}

try() {
    ./fat $1 $2 $3 2> result.txt
    RETVAL=$?
    [ $RETVAL -ne 0 ] && fail
}

buildimage() {
    umount $MOUNT
    /sbin/mkdosfs -F 32 -s $1 disk.img >/dev/null
    mount -o loop $IMAGE $MOUNT
    echo "Filling it with /etc files"
    find /etc -type f -maxdepth 1 -exec cp {} $MOUNT \;
}

runtests() {

    echo ---Test: create a 10K file
    try mkfile /apa.txt 10
    check
    try chkfile /apa.txt

    echo ---Test: create a 1K file
    try mkfile /bpa.txt 1
    check
    try chkfile /bpa.txt

    echo ---Test: create a 40K file
    try mkfile /cpa.txt 40
    check
    try chkfile /cpa.txt

    echo ---Test: truncate previous 40K file to 20K
    try mkfile /cpa.txt 20
    check
    try chkfile /cpa.txt

    echo ---Test: truncate previous 20K file to 0K
    try mkfile /cpa.txt 0
    check
    try chkfile /cpa.txt

    echo ---Test: create 20 1k files
    for i in `seq 1 10`;
    do
        echo -n $i
        try mkfile /rockbox.$i
        check
    done

}

echo "Building test image A (2 sectors/cluster)"
buildimage 2
runtests

echo "Building test image B (8 sectors/cluster)"
buildimage 8
runtests

echo "Building test image B (1 sector/cluster)"
buildimage 1
runtests

umount $MOUNT

echo "-- Test complete --"
