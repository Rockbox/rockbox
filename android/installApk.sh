#!/bin/sh
ADB="$ANDROID_SDK_PATH/tools/adb"

if [ "$1" == "-r" ]; then
    echo 'pm uninstall org.rockbox; exit' | $ADB shell
fi
$ADB install rockbox.apk
echo 'am start -W -a android.intent.action.MAIN -n org.rockbox/.RockboxActivity; exit' | $ADB shell
