#!/bin/sh
# Usage: resync.sh PUZZLES_PATH
#
# Automatic resync tool. Removes the current source snapshot in src/
# and copies just the source files we need from the puzzles source
# tree. Handles help generation as well. Stages changes in git.
#
# Expects a modified Halibut (https://www.fwei.tk/git/halibut) to be
# installed in $PATH. Also requires host CC and lz4 library to be
# available


if [ $# -ne 1 ]
then
    echo -e "Usage: $0 PUZZLES_PATH\n"
    echo "Automatically resync with upstream."
    echo "PUZZLES_PATH is the path to a puzzles source tree."
    exit
fi

echo "=== POTENTIALLY DANGEROUS OPERATION ==="
echo "Are you sure you want to remove all files in src/?"
echo -n "If so, type \"yes\" in all caps: "
read ans
if [ "YES" == $ans ]
then
    pushd "$(dirname "$0")" > /dev/null

    echo "[1/5] Removing current src/ directory"
    rm -rf src
    echo "[2/5] Copying new sources"
    mkdir src
    cp -r "$1"/{*.c,*.h,*.R,*.but,LICENCE,README} src
    echo "[3/5] Regenerating help"
    ./genhelp.sh

    echo "[4/5] Staging for commit"
    git add src help
    echo "[5/5] Successfully resynced with upstream"

    popd > /dev/null
else
    echo "Did nothing."
fi
