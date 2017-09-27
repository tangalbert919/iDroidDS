# Android ndk makefile for ds4droid

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE := superpoweredAndroid

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES := SuperpoweredNBandEQ.cpp \
                   AndroidIO/SuperpoweredAndroidAudioIO.cpp

LOCAL_LDLIBS := -lOpenSLES
include $(BUILD_STATIC_LIBRARY)