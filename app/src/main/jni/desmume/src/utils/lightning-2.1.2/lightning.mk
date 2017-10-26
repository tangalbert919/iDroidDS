# Android ndk makefile for ds4droid

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE := liblightning
LOCAL_C_INCLUDES := $(MY_LOCAL_PATH)/include \
                    $(MY_LOCAL_PATH)/include/lightning

LOCAL_SRC_FILES := $(wildcard $(MY_LOCAL_PATH)/lib/*.c)

include $(BUILD_STATIC_LIBRARY)