#!/bin/sh
rootdir=`dirname $0`

cat $rootdir/targets.txt | (
    while read target model
    do
        rm -f $rootdir/checkwps.$model
    done
)
