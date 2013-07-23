#!/bin/bash

######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   * Script to generate raw image resource files */
######################################################################
#

set -e

IMG_PATH="files/images"

for model in r0 r1
do
    echo $model
    for image in `ls "$IMG_PATH/$model"`
    do
        echo $image
        ../../tools/bmp2rb "$IMG_PATH/$model/$image" > "files/$model/etc/safemode/${image%.*}.raw"
    done
done
