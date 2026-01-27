LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := bsdiff
LOCAL_SRC_FILES := ../../bsdiff_unity.c ../../../bsdiff.c ../../../bspatch.c
LOCAL_CFLAGS := -O2 -Wall -fvisibility=hidden
include $(BUILD_SHARED_LIBRARY)