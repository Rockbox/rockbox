LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := MangoPlayer
LOCAL_SRC_FILES := ibassodualboot.c qdbmp.c
include $(BUILD_EXECUTABLE)
