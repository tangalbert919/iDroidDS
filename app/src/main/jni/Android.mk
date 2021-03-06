# Android ndk makefile for ds4droid

LOCAL_BUILD_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
include $(LOCAL_BUILD_PATH)/desmume_neon.mk
include $(LOCAL_BUILD_PATH)/desmume_v7.mk
endif

ifeq ($(TARGET_ARCH_ABI),x86)
include $(LOCAL_BUILD_PATH)/desmume_x86.mk
endif

ifeq ($(TARGET_ARCH_ABI),x86_64)
include $(LOCAL_BUILD_PATH)/desmume_x86_64.mk
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
include $(LOCAL_BUILD_PATH)/desmume_arm64.mk
endif

include $(LOCAL_BUILD_PATH)/cpudetect/cpudetect.mk
include $(LOCAL_BUILD_PATH)/android/7z/7z.mk
