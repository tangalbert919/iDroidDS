# Android ndk makefile for ds4droid

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)


LOCAL_MODULE    		:= 	libdesmumearm64
LOCAL_C_INCLUDES		:= 	$(LOCAL_PATH)/desmume/src \
							$(LOCAL_PATH)/desmume/src/android \
							$(LOCAL_PATH)/desmume/src/android/7z/CPP \
							$(LOCAL_PATH)/desmume/src/android/7z/CPP/include_windows
						   
LOCAL_SRC_FILES			:= 	desmume/src/addons/*.cpp \
							desmume/src/utils/decrypt/*.cpp \
							desmume/src/utils/libfat/*.cpp \
							desmume/src/utils/tinyxml/*.cpp \
							desmume/src/utils/advanscene.cpp \
							desmume/src/utils/ConvertUTF.c \
							desmume/src/utils/*.cpp \
							desmume/src/metaspu/metaspu.cpp \
							desmume/src/filter/*.cpp \
							desmume/src/arm_instructions.cpp \
							desmume/src/armcpu.cpp \
							desmume/src/bios.cpp \
							desmume/src/cheatSystem.cpp \
							desmume/src/common.cpp \
							desmume/src/cp15.cpp \
							desmume/src/debug.cpp \
							desmume/src/Disassembler.cpp \
							desmume/src/driver.cpp \
							desmume/src/emufile.cpp \
							desmume/src/encrypt.cpp \
							desmume/src/FIFO.cpp \
							desmume/src/firmware.cpp \
							desmume/src/fs-linux.cpp \
							desmume/src/gfx3d.cpp \
							desmume/src/GPU.cpp \
							desmume/src/GPU_osd_stub.cpp \
							desmume/src/matrix.cpp \
							desmume/src/mc.cpp \
							desmume/src/MMU.cpp \
							desmume/src/movie.cpp \
							desmume/src/NDSSystem.cpp \
							desmume/src/OGLES2Render.cpp \
							desmume/src/path.cpp \
							desmume/src/rasterize.cpp \
							desmume/src/readwrite.cpp \
							desmume/src/render3D.cpp \
							desmume/src/ROMReader.cpp \
							desmume/src/rtc.cpp \
							desmume/src/saves.cpp \
							desmume/src/slot1.cpp \
							desmume/src/slot2.cpp \
							desmume/src/SPU.cpp \
							desmume/src/texcache.cpp \
							desmume/src/thumb_instructions.cpp \
							desmume/src/version.cpp \
							desmume/src/wifi.cpp \
							desmume/src/android/mic.cpp \
							desmume/src/android/throttle.cpp \
							desmume/src/android/main.cpp \
							desmume/src/android/OpenArchive.cpp \
							desmume/src/android/7zip.cpp \
							desmume/src/android/sndopensl.cpp \
							desmume/src/android/draw.cpp 
							
LOCAL_ARM_NEON 			:= true
LOCAL_ARM_MODE 			:= arm
LOCAL_CFLAGS			:= -DANDROID -DHAVE_LIBZ -DNO_MEMDEBUG -DNO_GPUDEBUG -march=armv8-a -mtune=cortex-a53 -fpermissive
LOCAL_STATIC_LIBRARIES 	:= sevenzip lightningarm64
LOCAL_LDLIBS 			:= -llog -lz -lEGL -lGLESv2 -lGLESv3 -ljnigraphics -lOpenSLES -landroid

#To check for speed improvements
#LOCAL_CFLAGS += -DMEASURE_FIRST_FRAMES

include $(BUILD_SHARED_LIBRARY)
