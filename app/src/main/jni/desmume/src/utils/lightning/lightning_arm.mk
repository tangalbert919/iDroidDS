# Android ndk file for nds4droid
# Lightning JIT 2.1.0. Makefile built by Albert Tang.

LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE     := liblightning-v7
LOCAL_C_INCLUDES := $(LOCAL_PATH)/lib
LOCAL_SRC_FILES  := lib/jit_arm.c \
                    lib/jit_arm-cpu.c \
                    lib/jit_arm-cpu.c \
                    lib/jit_arm-swf.c \
                    lib/jit_arm-sz.c \
                    lib/jit_arm-vtp.c \
                    lib/lightning.c

LOCAL_ARM_NEON 			:= false
LOCAL_ARM_MODE          := arm
LOCAL_CFLAGS            := -marm -march=armv7-a -mfloat-abi=softfp

include $(BUILD_SHARED_LIBRARY)
