#!/bin/sh

IMAGE=disk.img
MOUNT=/mnt/dummy
DIR=$MOUNT/q
RESULT=result.txt

fail() {
    echo "!! Test failed $RETVAL. Look in $RESULT for test logs."
    chmod a+rw $RESULT
    exit
}

check() {
    /sbin/dosfsck -r $IMAGE | tee -a $RESULT
    [ $RETVAL -ne 0 ] && fail
}

try() {
    echo COMMAND: fat $1 "$2" "$3"
    echo COMMAND: fat $1 "$2" "$3" >> $RESULT
    ./fat $1 "$2" "$3" 2>> $RESULT
    RETVAL=$?
    [ $RETVAL -ne 0 ] && fail
}

buildimage() {
    /sbin/mkdosfs -F 16 -s $1 $IMAGE > /dev/null;
    mount -o loop,fat=16 $IMAGE $MOUNT;
    echo "Filling it with /etc files";
    mkdir $DIR;
    find /etc -type f -maxdepth 1 -exec cp {} $DIR \;
    for i in `seq 1 120`;
    do
        echo apa > "$DIR/very $i long test filename so we can make sure they.work";
    done;
    umount $MOUNT;
}

runtests() {
    rm $RESULT

    echo ---Test: create a long name directory in the root
    try mkdir "/very long subdir name"
    check
    try mkdir "/very long subdir name/apa.monkey.me.now"
    check

    echo ---Test: create a directory called "dir"
    try mkdir "/dir"
    check

    echo ---Test: create a 10K file
    try mkfile "/really long filenames rock" 10
    check

    try mkfile /dir/apa.monkey.me.now 10
    check
    try chkfile "/really long filenames rock" 10
    try chkfile /dir/apa.monkey.me.now 8

    echo ---Test: create a 1K file
    try mkfile /bpa.rock 1
    check
    try chkfile /bpa.rock 1

    echo ---Test: create a 40K file
    try mkfile /cpa.rock 40
    check
    try chkfile /cpa.rock 40

    echo ---Test: create a 400K file
    try mkfile /dpa.rock 400
    check
    try chkfile /dpa.rock 400

    echo ---Test: create a 1200K file
    try mkfile /epa.rock 1200
    check
    try chkfile /epa.rock 1200

    echo ---Test: rewrite first 20K of a 40K file
    try mkfile /cpa.rock 20
    check
    try chkfile /cpa.rock 20

    echo ---Test: rewrite first sector of 40K file
    try mkfile /cpa.rock 0
    check
    try chkfile /cpa.rock
    try chkfile /bpa.rock

    LOOP=25
    SIZE=700

    try del "/really long filenames rock"

    echo ---Test: create $LOOP $SIZE k files
    for i in `seq 1 $LOOP`;
    do
        echo ---Test: $i/$LOOP ---
        try mkfile "/q/rockbox rocks.$i" $SIZE
        check
        try chkfile "/q/rockbox rocks.$i" $SIZE
        check
        try del "/q/rockbox rocks.$i"
        check
        try mkfile "/q/rockbox rocks.$i" $SIZE
        check
        try ren "/q/rockbox rocks.$i" "/q/$i is a new long filename!"
        check
    done

}

echo "--------------------------------------"
echo "Building test image (4 sector/cluster)"
echo "--------------------------------------"
buildimage 4
runtests

echo "--------------------------------------"
echo "Building test image (8 sectors/cluster)"
echo "--------------------------------------"
buildimage 8
runtests

echo "----------------------------------------"
echo "Building test image (64 sectors/cluster)"
echo "----------------------------------------"
buildimage 16
runtests

echo "== Test completed successfully =="
chmod a+rw $RESULT
