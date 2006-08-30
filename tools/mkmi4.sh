#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Purpose of this script:
#
# Inputs: music player model name and a file name
#
# Action: Build a valid mi4 file (prepending and appending magic)
#         Encrypt the file with TEA encryption so that the model's own
#         bootloader accepts this file.
#         Sign the file with a DSA signature the bootloader accepts
#
# Output: A built, encrypted and signed mi4 file image.
#
# Requirement:
#
# This script assumes that you have the mi4code tool in your path, that
# you have the environment variable MI4CODE pointing to the tool or that you
# have it in the same dir that you invoke this script with.
#
# mi4 info and tool are here: http://daniel.haxx.se/sansa/mi4.html
#

mkmi4=$0
target=$1
input=$2
output=$3

# scan the $PATH for the given command
findtool(){
  file="$1"

  IFS=":"
  for path in $PATH
  do
    # echo "checks for $file in $path" >&2
    if test -f "$path/$file"; then
      echo "$path/$file"
      return
    fi
  done
}

help () {
  echo "Usage: mi4fix.sh <e200/h10/h10_5gb/elio> <input> <output>"
  exit
}

if test -z "$output"; then
  help
fi

# sign - if the firmware should be DSA signed with a dummy (only 010301
#        firmwares)
# tea -  name of the TEA crypt key to use for encrypting, but only if ...
# encrypt - is set to "yes" for encrypting the firmware
case $target in
 # fake example)
 #   sign="yes"
 #   encrypt="yes"
 #   tea=targetkey
 #   ;;
  e200)
    sign="yes"
    ;;
  h10)
    sign="yes"
    ;;
  h10_5gb)
    buildopt="-2"
    ;;
  elio)
    buildopt="-2"
    ;;
  *)
    echo "unsupported target"
    help
    ;;
esac

if test -z "$MI4CODE"; then
  tool=`findtool mi4code`
  if test -z "$tool"; then
    # not in path
    tool=`dirname $mkmi4`/mi4code
    if ! test -f $tool; then
      echo "Couldn't find mi4code"
      exit
    fi
  fi
else
  tool=$MI4CODE
fi

# Use full file plaintext length if not encrypting
if test -z "$encrypt"; then
  buildopt="$buildopt -pall"
fi

# build mi4
#echo "$tool build $input $output.raw"
$tool build $buildopt $input $output.raw
# encrypt
if test -n "$encrypt"; then
  #echo "$tool encrypt $output.raw $output.encrypt $tea"
  $tool encrypt $output.raw $output.encrypt $tea
else
  # Even if we don't encrypt we need to do this to ensure the crc gets fixed
  $tool encrypt -pall $output.raw $output.encrypt default
fi
# sign
if test -n "$sign"; then
  #echo "$tool sign $output.encrypt $output"
  $tool sign $output.encrypt $output
else
  mv $output.encrypt $output
fi
