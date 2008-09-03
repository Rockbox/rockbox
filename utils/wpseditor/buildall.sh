#!/bin/sh
qmake && make
cd libwps
./buildall.sh 2> /dev/null
