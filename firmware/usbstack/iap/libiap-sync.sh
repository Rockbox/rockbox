#!/bin/bash

set -e

url=https://github.com/mojyack/libiap.git
src=/tmp/libiap
dst=$PWD/libiap

if [[ ! -e $src ]]; then
    git clone "$url" "$src"
else
    git -C "$src" pull
fi

rsync --recursive --delete "$src/src/iap/" "$dst/" --exclude /README.rockbox
cp "$src/LICENSE" "$dst/LICENSE"
git -C "$src" rev-parse HEAD > "$dst/HEAD"
