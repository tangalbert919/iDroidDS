# Android ndk makefile for ds4droid

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE    		:= 	libasmjitarm

LOCAL_SRC_FILES			:=  base/arch.cpp \
                            base/assembler.cpp \
                            base/codebuilder.cpp \
                            base/codecompiler.cpp \
                            base/codeemitter.cpp \
                            base/codeholder.cpp \
                            base/constpool.cpp \
                            base/cpuinfo.cpp \
                            base/func.cpp \
                            base/globals.cpp \
                            base/inst.cpp \
                            base/intutils.cpp \
                            base/logging.cpp \
                            base/operand.cpp \
                            base/osutils.cpp \
                            base/ralocal.cpp \
                            base/rapass.cpp \
                            base/rastack.cpp \
                            base/runtime.cpp \
                            base/string.cpp \
                            base/utils.cpp \
                            base/virtmem.cpp \
                            base/zone.cpp \
                            arm/armassembler.cpp \
                            arm/armbuilder.cpp \
                            arm/armcompiler.cpp \
                            arm/arminst.cpp \
                            arm/arminternal.cpp \
                            arm/armlogging.cpp \
                            arm/armoperand.cpp \
                            arm/armoperand_regs.cpp \
                            arm/armrapass.cpp

LOCAL_ARM_NEON 			:= false
LOCAL_CFLAGS			:= -DCOMPRESS_MT -DASMJIT_BUILD_ARM

include $(BUILD_STATIC_LIBRARY)