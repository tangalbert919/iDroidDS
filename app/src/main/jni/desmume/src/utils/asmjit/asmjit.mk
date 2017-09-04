# Android ndk makefile for ds4droid

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE    		:= 	libasmjit

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
                            x86/x86assembler.cpp \
                            x86/x86builder.cpp \
                            x86/x86compiler.cpp \
                            x86/x86inst.cpp \
                            x86/x86instimpl.cpp \
                            x86/x86internal.cpp \
                            x86/x86logging.cpp \
                            x86/x86operand.cpp \
                            x86/x86rapass.cpp \
                            x86/x86settoavxpass.cpp

LOCAL_ARM_NEON 			:= false
LOCAL_CFLAGS			:= -DCOMPRESS_MT -DASMJIT_BUILD_X86

include $(BUILD_STATIC_LIBRARY)