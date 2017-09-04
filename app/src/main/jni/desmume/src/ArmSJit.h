/*
	Copyright (C) 2017 nds4droid team

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

#ifndef ARM_SJIT
#define ARM_SJIT

#include "common.h"
#include "CpuBase.h"

#if defined(_WINDOWS) || defined(DESMUME_COCOA) || defined(ANDROID)
#define MAPPED_JIT_FUNCS
#endif

#ifdef MAPPED_JIT_FUNCS
struct JitStackless
{
    uintptr_t MAIN_MEM[16*1024*1024/2];
    uintptr_t SWIRAM[0x8000/2];
    uintptr_t ARM9_ITCM[0x8000/2];
    uintptr_t ARM9_LCDC[0xA4000/2];
    uintptr_t ARM9_BIOS[0x8000/2];
    uintptr_t ARM7_BIOS[0x4000/2];
    uintptr_t ARM7_ERAM[0x10000/2];
    uintptr_t ARM7_WIRAM[0x10000/2];
    uintptr_t ARM7_WRAM[0x40000/2];

    static uintptr_t JIT_MEM[2][0x4000];

};

extern CACHE_ALIGN JitStackless g_SlJit;
#define SLJIT_COMPILE_FUNC(adr, PROCNUM) g_SlJit.JIT_MEM[PROCNUM][((adr)&0x0FFFC000)>>14][((adr)&0x0003FFE)>>1]
#define SLJIT_COMPILE_FUNC_PREMASKED(adr, PROCNUM, ofs) g_SlJit.JIT_MEM[PROCNUM][(adr)>>14][(((adr)0x0003FFE)>>1)+ofs]
#define SLJIT_COMPILE_FUNC_KNOWNBANK(adr, bank, mask, ofs) g.SlJit.bank[(((adr)&(mask))>>1)+ofs]
#define SLJIT_MAPPED(adr, PROCNUM) g_SlJit.JIT_MEM[PROCNUM][(adr)>>14]
#else
extern uintptr_t g_CompiledFuncs[];
#define SLJIT_COMPILE_FUNC(adr, PROCNUM) g_CompiledFuncs[((adr) & 0x7FFFFFE) >> 1]
#define SLJIT_COMPILE_FUNC_PREMASKED(adr, PROCNUM, ofs) SLJIT_COMPILE_FUNC(adr, PROCNUM)
#define SLJIT_COMPILE_FUNC_KNOWNBANK(adr, bank, mask, ofs) SLJIT_COMPILE_FUNC(adr, PROCNUM)
#define SLJIT_MAPPED(adr, PROCNUM) true
#endif

extern CpuBase arm_sjit;
#endif //ARM_SJIT
