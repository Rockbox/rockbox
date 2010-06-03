#!/bin/sh

version="3.6"

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

# Only GNU cp accepts --long-options (and --parents)
# If the system cp is POSIX cp, try gcp (works on OSX)
CP=cp
$CP --help >/dev/null 2>&1 || CP=gcp
svn ls -R | xargs -Imoo $CP --parents moo $rbdir 2>/dev/null

cd $tempdir

rm -f $outfile

# 7zip the entire directory
7zr a $outfile rockbox*

# world readable please
chmod a+r $outfile

# remove temporary files
rm -rf $tempdir
