#!/bin/bash

# Abort execution as soon as an error is encountered
# That way the script do not let the user think the process completed correctly
# and leave the opportunity to fix the problem and restart compilation where
# it stopped
set -e

SDK_DOWNLOAD_URL="https://developer.android.com/studio"
SDK_DOWNLOAD_KEYWORD="commandlinetools"
NDK_DOWNLOAD_URL="https://github.com/android/ndk/wiki/Unsupported-Downloads"
NDK_DOWNLOAD_KEYWORD="r10e"

find_url() {
    base_url="$1"
    keyword="$2"
    os="$3"
    wget -q -O - $base_url | grep dl.google.com | sed 's/.*"\(https:\/\/dl.google.com\/.*\.zip\)".*/\1/' | grep $os | grep $keyword | grep -v bundle | grep -v .exe
}

OS=`uname`
case $OS in
    Linux)
    SDK_URL=$(find_url $SDK_DOWNLOAD_URL $SDK_DOWNLOAD_KEYWORD linux)
    NDK_URL=$(find_url $NDK_DOWNLOAD_URL $NDK_DOWNLOAD_KEYWORD linux)
    ANDROID=cmdline-tools/latest/bin/sdkmanager
    ;;
    
    Darwin)
    SDK_URL=$(find_url $SDK_DOWNLOAD_URL $SDK_DOWNLOAD_KEYWORD mac)
    NDK_URL=$(find_url $NDK_DOWNLOAD_URL $NDK_DOWNLOAD_KEYWORD darwin)
    ANDROID=cmdline-tools/latest/bin/sdkmanager
    ;;

    CYGWIN*)
    SDK_URL=$(find_url $SDK_DOWNLOAD_URL $SDK_DOWNLOAD_KEYWORD windows)
    NDK_URL=$(find_url $NDK_DOWNLOAD_URL $NDK_DOWNLOAD_KEYWORD windows)
    ANDROID=cmdline-tools/latest/bin/sdkmanager.exe
    ;;
esac

prefix="${INSTALL_PREFIX:-$HOME}"
dldir="${DOWNLOAD_DIR:-/tmp}"

SDK_PATH=${ANDROID_HOME:-$(find $prefix -maxdepth 1 -name "android-sdk")}
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
    unzip -qo -d "$prefix" "$local_file"
}

if [ -z "$SDK_PATH" ]; then
    mkdir -p "$prefix/android-sdk/cmdline-tools"
    download_and_extract $SDK_URL
    mv "$prefix/cmdline-tools" "$prefix/android-sdk/cmdline-tools/latest"
    # OS X doesn't know about realname, use basename instead.
    SDK_PATH=$prefix/$(basename $prefix/android-sdk)
fi
if [ -z "$NDK_PATH" ]; then
    download_and_extract $NDK_URL
    NDK_PATH=$prefix/$(basename $prefix/android-ndk-*)
fi

if [ ! -d "$SDK_PATH/platforms/android-19" ] || [ ! -d "$SDK_PATH/build-tools/19.1.0" ]; then
    echo " * Installing Android platforms..."
    $SDK_PATH/$ANDROID --install "platforms;android-19" "build-tools;19.1.0"
fi

cat <<EOF
 * All done!

Please set the following environment variables before running tools/configure:
export ANDROID_SDK_PATH=$SDK_PATH
export ANDROID_NDK_PATH=$NDK_PATH

EOF
