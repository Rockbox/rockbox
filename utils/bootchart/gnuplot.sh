#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2010 by Maurus Cuelenaere
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
#
# Invoke this as "./gnuplot.sh < logf.txt"

TMP=`tempfile`

awk 'BEGIN {
    FS=","
    i=1
}

/^BC:/ {
    # BC:<function name>,<line number>,<elapsed ticks>
    printf "%d\t%d\t\"%s\"\n", i, $3, substr($1, 4)
    i=i+1
}' > $TMP

echo "plot \"$TMP\" u 1:2:3 w labels left rotate by 90 offset 0,0.5 notitle, \"$TMP\" u 1:2 w linespoints notitle" | gnuplot -persist

rm $TMP
