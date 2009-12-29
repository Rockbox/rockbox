#!/bin/sh
rootdir=`dirname $0`
outdir=$rootdir/output

make clean # make clean the build dir first
rm -f autoconf.h
rm -f Makefile
awk -f $rootdir/parse_configure.awk $rootdir/../configure | (
    while read target model
    do
        rm -f $outdir/checkwps.$model # then delete any output/checkwps.*
        rmdir $outdir > /dev/null 2>&1
    done
)
