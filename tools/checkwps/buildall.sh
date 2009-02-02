#!/bin/sh
rootdir=`dirname $0`

cat $rootdir/targets.txt | (
    while read target model
    do
        rm -f checkwps.$model
        make -s -C $rootdir MODEL=$model TARGET=$target checkwps
    done
)
