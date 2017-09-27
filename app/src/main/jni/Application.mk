# Android ndk makefile for nds4droid

APP_STL := gnustl_static
APP_ABI := armeabi-v7a x86 arm64-v8a x86_64

# For releases. The first CFLAG line is for GCC, and the second one is Clang.
APP_CFLAGS := -Ofast -fmodulo-sched -fmodulo-sched-allow-regmoves -fsingle-precision-constant -fvariable-expansion-in-unroller -fomit-frame-pointer -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fdata-sections -fbranch-target-load-optimize2 -fexceptions -fno-stack-protector -fforce-addr -ftree-loop-im -ftree-loop-ivcanon -fivopts -ftree-loop-if-convert-stores -floop-strip-mine -ftree-loop-distribution -floop-interchange -ftree-loop-linear -floop-block -Wno-psabi
#APP_CFLAGS := -O2 -fmodulo-sched -fmodulo-sched-allow-regmoves -fsingle-precision-constant -fvariable-expansion-in-unroller -fomit-frame-pointer -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fdata-sections -fexceptions -fno-stack-protector -fforce-addr -fivopts
# For debugging only.
#APP_CFLAGS := -Wno-psabi

NDK_TOOLCHAIN_VERSION := 4.9
APP_PLATFORM := android-21
