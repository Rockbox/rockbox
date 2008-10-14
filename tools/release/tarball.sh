#!/bin/sh

version="3.0"

srcdir=.
tempdir=rockbox-temp
outfile=rockbox-$version.7z

# remove previous leftovers
rm -rf $tempdir

cd $srcdir

# create the dir name based on revision number
rbdir=$tempdir/rockbox-$version

# create new temp dir
mkdir -p $rbdir

# copy everything to the temp dir
svn ls -R | xargs -Imoo cp --parents moo $rbdir 2>/dev/null

cd $tempdir

rm -f $outfile

# 7zip the entire directory
7zr a $outfile rockbox*

# world readable please
chmod a+r $outfile

# remove temporary files
rm -rf $tempdir
