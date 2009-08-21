#!/bin/sh
rootdir=`dirname $0`

cat $rootdir/targets.txt | (
    while read target model
    do
        rm -f checkwps.$model
        make -s -C $rootdir MODELNAME=$model TARGETNAME=$target checkwps
        mv $rootdir/checkwps.$model ./checkwps.$model > /dev/null 2>&1
    done
)
