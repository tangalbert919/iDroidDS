# Android ndk makefile for ds4droid

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

# ARMv7 module (32-bit)

LOCAL_MODULE := liblightningarm

LOCAL_MODULE_FILENAME := liblightingarm

LOCAL_C_INCLUDES := $(MY_LOCAL_PATH)/include \
                    $(MY_LOCAL_PATH)/include/lightning

LOCAL_SRC_FILES := $(MY_LOCAL_PATH)/lib/jit_arm.c \
                   $(MY_LOCAL_PATH)/lib/jit_arm-cpu.c \
                   $(MY_LOCAL_PATH)/lib/jit_arm-swf.c \
                   $(MY_LOCAL_PATH)/lib/jit_arm-sz.c \
                   $(MY_LOCAL_PATH)/lib/jit_arm-vfp.c \
                   $(MY_LOCAL_PATH)/lib/lightning.c

include $(BUILD_STATIC_LIBRARY)

# ARMv8 module (64-bit)

LOCAL_MODULE := liblightningarm64

LOCAL_MODULE_FILENAME := liblightningarm64

LOCAL_C_INCLUDES := $(MY_LOCAL_PATH)/include \
                    $(MY_LOCAL_PATH)/include/lightning

LOCAL_SRC_FILES := $(MY_LOCAL_PATH)/lib/jit_aarch64.c \
                   $(MY_LOCAL_PATH)/lib/jit_aarch64-cpu.c \
                   $(MY_LOCAL_PATH)/lib/jit_aarch64-fpu.c \
                   $(MY_LOCAL_PATH)/lib/jit_aarch64-sz.c \
                   $(MY_LOCAL_PATH)/lib/lightning.c

include $(BUILD_STATIC_LIBRARY)

