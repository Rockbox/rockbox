#!/bin/sh

BUILDDIR=$1
APK=$2
SDKV=$3

[ -z $ANDROID_SDK_PATH ] && exit 1
[ -z $BUILDDIR ] && exit 1
[ -d $BUILDDIR ] || exit 1

# need to cd into the bin dir and create a symlink to the libraries
# so that aapt puts the libraries with the correct prefix into the apk
cd $BUILDDIR/bin
ln -nfs $BUILDDIR/libs lib
cp resources.ap_ $APK
$ANDROID_SDK_PATH/build-tools/$SDKV/aapt add $APK classes.dex > /dev/null
$ANDROID_SDK_PATH/build-tools/$SDKV/aapt add $APK lib/*/* > /dev/null

exit 0
