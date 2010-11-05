#!/bin/sh
ADB="$ANDROID_SDK_PATH/tools/adb"

$ADB install rockbox.apk
echo 'am start -W -a android.intent.action.MAIN -n org.rockbox/.RockboxActivity; exit' | $ADB shell
