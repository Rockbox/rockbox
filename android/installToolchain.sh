#!/bin/bash

# Abort execution as soon as an error is encountered
# That way the script do not let the user think the process completed correctly
# and leave the opportunity to fix the problem and restart compilation where
# it stopped
set -e

# http://developer.android.com/sdk/index.html
SDK_URL_LNX="http://dl.google.com/android/android-sdk_r11-linux_x86.tgz"
SDK_URL_MAC="http://dl.google.com/android/android-sdk_r11-mac_x86.zip"
SDK_URL_WIN="http://dl.google.com/android/android-sdk_r11-windows.zip"
# http://developer.android.com/sdk/ndk/index.html
NDK_URL_LNX="http://dl.google.com/android/ndk/android-ndk-r5c-linux-x86.tar.bz2"
NDK_URL_MAC="http://dl.google.com/android/ndk/android-ndk-r5c-darwin-x86.tar.bz2"
NDK_URL_WIN="http://dl.google.com/android/ndk/android-ndk-r5c-windows.zip"

OS=`uname`
case $OS in
    Linux)
    SDK_URL=$SDK_URL_LNX
    NDK_URL=$NDK_URL_LNX
    ANDROID=tools/android
    ;;
    
    Darwin)
    SDK_URL=$SDK_URL_MAC
    NDK_URL=$NDK_URL_MAC
    ANDROID=tools/android
    ;;

    CYGWIN*)
    SDK_URL=$SDK_URL_WIN
    NDK_URL=$NDK_URL_WIN
    ANDROID=tools/android.bat
    ;;
esac

prefix="${INSTALL_PREFIX:-$HOME}"
dldir="${DOWNLOAD_DIR:-/tmp}"

SDK_PATH=$(find $prefix -maxdepth 1 -name "android-sdk-*")
NDK_PATH=$(find $prefix -maxdepth 1 -name "android-ndk-*")

download_and_extract() {
    url="$1"
    name=${url##*/}
    local_file="$dldir/$name"
    if [ \! -f "$local_file" ]; then
        echo " * Downloading $name..."
        wget -O "$local_file" $1
    fi

    echo " * Extracting $name..."
    case ${local_file#*.} in
        zip)
            unzip -qo -d "$prefix" "$local_file"
            ;;
        tgz|tar.gz)
            (cd $prefix; tar -xf "$local_file")
            ;;
        tar.bz2)
            (cd $prefix; tar -xjf "$local_file")
            ;;
        *)
            echo "Couldn't figure out how to extract $local_file" ! 1>&2
            ;;
    esac
}

if [ -z "$SDK_PATH" ]; then
    download_and_extract $SDK_URL
    # OS X doesn't know about realname, use basename instead.
    SDK_PATH=$prefix/$(basename $prefix/android-sdk-*)
fi
if [ -z "$NDK_PATH" ]; then
    download_and_extract $NDK_URL
    NDK_PATH=$prefix/$(basename $prefix/android-ndk-*)
fi

if [ -z "$(find $SDK_PATH/platforms -type d -name 'android-*')" ]; then
    echo " * Installing Android platforms..."
    $SDK_PATH/$ANDROID update sdk --no-ui --filter platform,tool
fi

cat <<EOF
 * All done!

Please set the following environment variables before running tools/configure:
export ANDROID_SDK_PATH=$SDK_PATH
export ANDROID_NDK_PATH=$NDK_PATH

EOF
