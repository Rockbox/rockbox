#!/bin/sh

IMAGE=disk.img
MOUNT=/mnt/dummy
RESULT=result.txt

fail() {
    echo "!! Test failed. Look in $RESULT for test logs."
    exit
}

check() {
    /sbin/dosfsck -r $IMAGE | tee -a $RESULT
    [ $RETVAL -ne 0 ] && fail
}

try() {
    ./fat $1 $2 $3 2>> $RESULT
    RETVAL=$?
    [ $RETVAL -ne 0 ] && fail
}

buildimage() {
    /sbin/mkdosfs -F 32 -s $1 $IMAGE > /dev/null
    mount -o loop $IMAGE $MOUNT
    echo "Filling it with /etc files"
    find /etc -type f -maxdepth 1 -exec cp {} $MOUNT \;
    umount $MOUNT
}

runtests() {
    rm $RESULT

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
    try chkfile /apa.txt
    try chkfile /bpa.txt

    echo ---Test: create 10 1k files
    for i in `seq 1 10`;
    do
        echo ---Test: $i/10 ---
        try mkfile /rockbox.$i
        check
        try chkfile /bpa.txt
    done

}

echo "Building test image (1 sector/cluster)"
buildimage 1
runtests

echo "Building test image (4 sector/cluster)"
buildimage 4
runtests

echo "Building test image (8 sectors/cluster)"
buildimage 8
runtests

echo "Building test image (32 sectors/cluster)"
buildimage 32
runtests

echo "Building test image (128 sectors/cluster)"
buildimage 128
runtests

echo "== Test completed sucessfully =="
