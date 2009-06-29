#!/bin/bash
#
# Create a tarball of rbutil from a tagged release
# Doess a full checkout, then shaves off everything unneeded.

if [ -z "$1" ]; then
    printf "Usage: `basename $0` 1.2.2"
    exit
else
    version="$1"
fi

svn checkout svn://svn.rockbox.org/rockbox/tags/rbutil_$version rbutil-$version
if [ ! $? ]; then
    echo "Something went wrong with checking out"
    exit 22
fi

cd rbutil-$version
# Delete top-level dirs that aren't used
rm -rf bootloader firmware fonts icons uisimulator wps backdrops docs flash gdb manual utils

# Clean out tools and restore the bits that we need
rm -rf tools/*
svn update tools/{iriver.c,Makefile,mkboot.h,voicefont.c,VOICE_PAUSE.wav,wavtrim.h,iriver.h,mkboot.c,rbspeex,voicefont.h,wavtrim.c}
# Just in case
mkdir -p tools/rbspeex/build

# Clean out apps and restore apps/codecs/libspeex
rm -rf apps/*
svn update apps/codecs
rm -rf apps/codecs/*
svn update apps/codecs/libspeex
cd ..

tar cvvj --exclude-vcs -f rbutil-$version.tar.bz2 rbutil-$version
rm -rf rbutil-$version
