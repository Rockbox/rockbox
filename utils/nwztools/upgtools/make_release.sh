#!/bin/bash

if [ $# -ne 1 ];
then
    echo "usage: $0 version"
    exit 1
fi

ver="$1"
echo "version: $ver"

echo "Building windows version..."
make PREFIX=i686-w64-mingw32- EXE_EXT=.exe clean && make PREFIX=i686-w64-mingw32- EXE_EXT=.exe && cp upgtool.exe upgtool-v${ver}.exe
echo "Building linux version..."
make clean && make && cp upgtool upgtool_64-v${ver}
