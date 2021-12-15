LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := MangoPlayer
LOCAL_SRC_FILES := ibassodualboot.c qdbmp.c

TARGET_ARCH=arm
TARGET_PLATFORM=android-14
TARGET_ARCH_ABI=armeabi

#LOCAL_CFLAGS := -DDEBUG
#LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)
