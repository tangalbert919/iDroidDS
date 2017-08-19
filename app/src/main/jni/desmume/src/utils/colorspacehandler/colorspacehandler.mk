# Android ndk makefile for colorspacehandler

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE := libcolorspacehandler

LOCAL_SRC_FILES := colorspacehandler.cpp \
					colorspacehandler_AltiVec.cpp \
					colorspacehandler_AVX2.cpp \
					colorspacehandler_SSE2.cpp

LOCAL_ARM_MODE 			:= 	arm
LOCAL_ARM_NEON 			:= 	false
LOCAL_CFLAGS			:= -DCOMPRESS_MT

include $(BUILD_STATIC_LIBRARY)