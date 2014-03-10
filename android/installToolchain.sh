#!/bin/bash

# Abort execution as soon as an error is encountered
# That way the script do not let the user think the process completed correctly
# and leave the opportunity to fix the problem and restart compilation where
# it stopped
set -e

SDK_DOWNLOAD_URL="http://developer.android.com/sdk/index.html"
NDK_DOWNLOAD_URL="http://developer.android.com/sdk/ndk/index.html"

find_url() {
    base_url="$1"
    os="$2"
    wget -q -O - $base_url | grep dl.google.com | sed 's/.*"\(http:\/\/.*\)".*/\1/' | grep $os | grep -v bundle | grep -v .exe # Windows hack
}

OS=`uname`
case $OS in
    Linux)
    SDK_URL=$(find_url $SDK_DOWNLOAD_URL linux)
    NDK_URL=$(find_url $NDK_DOWNLOAD_URL linux)
    ANDROID=tools/android
    ;;
    
    Darwin)
    SDK_URL=$(find_url $SDK_DOWNLOAD_URL mac)
    NDK_URL=$(find_url $NDK_DOWNLOAD_URL darwin)
    ANDROID=tools/android
    ;;

    CYGWIN*)
    SDK_URL=$(find_url $SDK_DOWNLOAD_URL windows)
    NDK_URL=$(find_url $NDK_DOWNLOAD_URL windows)
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
    case ${local_file} in
        *.zip)
            unzip -qo -d "$prefix" "$local_file"
            ;;
        *.tgz|*.tar.gz)
            (cd $prefix; tar -xzf "$local_file")
            ;;
        *.tar.bz2)
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
    $SDK_PATH/$ANDROID update sdk --no-ui --filter platform,platform-tool,tool
fi

cat <<EOF
 * All done!

Please set the following environment variables before running tools/configure:
export ANDROID_SDK_PATH=$SDK_PATH
export ANDROID_NDK_PATH=$NDK_PATH

EOF
