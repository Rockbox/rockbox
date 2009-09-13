#!/bin/sh
rootdir=`dirname $0`
outdir=$rootdir/output

make clean # make clean the build dir first
rm -f autoconf.h
rm -f Makefile
cat $rootdir/targets.txt | (
    while read target model
    do
        rm -f $outdir/checkwps.$model # then delete any output/checkwps.*
        rmdir $outdir > /dev/null 2>&1
    done
)
