/*
	Copyright (C) 2012 Jeffrey Quesnelle

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cpu-features.h>
#include <jni.h>

#define JNI_NOARGS(X) Java_com_opendoorstudios_ds4droid_DeSmuME_##X(JNIEnv* env, jclass* clazz)

#define CPUTYPE_V7 0
#define CPUTYPE_NEON 1
#define CPUTYPE_X86 2
#define CPUTYPE_ARM64 3
#define CPUTYPE_X64 4

extern "C"
{

jint JNI_NOARGS(getCPUType)
{
	AndroidCpuFamily cpuFamily = android_getCpuFamily();
	uint64_t cpuFeatures = android_getCpuFeatures();
    if ((cpuFeatures & ANDROID_CPU_ARM_FEATURE_NEON) != 0)
        return CPUTYPE_NEON;
	else if (cpuFamily == ANDROID_CPU_FAMILY_X86)
		return CPUTYPE_X86;
	else if (cpuFamily == ANDROID_CPU_FAMILY_X86_64)
		return CPUTYPE_X64;
    else if (cpuFamily == ANDROID_CPU_FAMILY_ARM64)
        return CPUTYPE_ARM64;
	else
		return CPUTYPE_NEON;
}

}
