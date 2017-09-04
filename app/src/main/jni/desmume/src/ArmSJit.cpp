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

#include "types.h"

//#ifdef HAVE_JIT
#include "ArmSJit.h"

#include "armcpu.h"
#include "instructions.h"
#include "instruction_attributes.h"
#include "Disassembler.h"
#include "MMU.h"
#include "MMU_timing.h"

#ifdef MAPPED_JIT_FUNCS
CACHE_ALIGN JitStackless JIT;

uintptr_t *JitStackless::JIT_MEM[2][0x4000] = {{0}};

static uintptr_t *JIT_MEM[2][32] = {
        //arm9
        {
                /* 0X*/	DUP2(JIT.ARM9_ITCM),
                /* 1X*/	DUP2(JIT.ARM9_ITCM), // mirror
                /* 2X*/	DUP2(JIT.MAIN_MEM),
                /* 3X*/	DUP2(JIT.SWIRAM),
                /* 4X*/	DUP2(NULL),
                /* 5X*/	DUP2(NULL),
                /* 6X*/		 NULL,
                        JIT.ARM9_LCDC,	// Plain ARM9-CPU Access (LCDC mode) (max 656KB)
                /* 7X*/	DUP2(NULL),
                /* 8X*/	DUP2(NULL),
                /* 9X*/	DUP2(NULL),
                /* AX*/	DUP2(NULL),
                /* BX*/	DUP2(NULL),
                /* CX*/	DUP2(NULL),
                /* DX*/	DUP2(NULL),
                /* EX*/	DUP2(NULL),
                /* FX*/	DUP2(JIT.ARM9_BIOS)
        },
        //arm7
        {
                /* 0X*/	DUP2(JIT.ARM7_BIOS),
                /* 1X*/	DUP2(NULL),
                /* 2X*/	DUP2(JIT.MAIN_MEM),
                /* 3X*/	     JIT.SWIRAM,
                             JIT.ARM7_ERAM,
                /* 4X*/	     NULL,
                             JIT.ARM7_WIRAM,
                /* 5X*/	DUP2(NULL),
                /* 6X*/		 JIT.ARM7_WRAM,		// VRAM allocated as Work RAM to ARM7 (max. 256K)
                           NULL,
                /* 7X*/	DUP2(NULL),
                /* 8X*/	DUP2(NULL),
                /* 9X*/	DUP2(NULL),
                /* AX*/	DUP2(NULL),
                /* BX*/	DUP2(NULL),
                /* CX*/	DUP2(NULL),
                /* DX*/	DUP2(NULL),
                /* EX*/	DUP2(NULL),
                /* FX*/	DUP2(NULL)
        }
};

static u32 JIT_MASK[2][32] = {
        //arm9
        {
                /* 0X*/	DUP2(0x00007FFF),
                /* 1X*/	DUP2(0x00007FFF),
                /* 2X*/	DUP2(0x003FFFFF), // FIXME _MMU_MAIN_MEM_MASK
                /* 3X*/	DUP2(0x00007FFF),
                /* 4X*/	DUP2(0x00000000),
                /* 5X*/	DUP2(0x00000000),
                /* 6X*/		 0x00000000,
                             0x000FFFFF,
                /* 7X*/	DUP2(0x00000000),
                /* 8X*/	DUP2(0x00000000),
                /* 9X*/	DUP2(0x00000000),
                /* AX*/	DUP2(0x00000000),
                /* BX*/	DUP2(0x00000000),
                /* CX*/	DUP2(0x00000000),
                /* DX*/	DUP2(0x00000000),
                /* EX*/	DUP2(0x00000000),
                /* FX*/	DUP2(0x00007FFF)
        },
        //arm7
        {
                /* 0X*/	DUP2(0x00003FFF),
                /* 1X*/	DUP2(0x00000000),
                /* 2X*/	DUP2(0x003FFFFF),
                /* 3X*/	     0x00007FFF,
                             0x0000FFFF,
                /* 4X*/	     0x00000000,
                             0x0000FFFF,
                /* 5X*/	DUP2(0x00000000),
                /* 6X*/		 0x0003FFFF,
                             0x00000000,
                /* 7X*/	DUP2(0x00000000),
                /* 8X*/	DUP2(0x00000000),
                /* 9X*/	DUP2(0x00000000),
                /* AX*/	DUP2(0x00000000),
                /* BX*/	DUP2(0x00000000),
                /* CX*/	DUP2(0x00000000),
                /* DX*/	DUP2(0x00000000),
                /* EX*/	DUP2(0x00000000),
                /* FX*/	DUP2(0x00000000)
        }
};

static void init_jit_mem() {
    static bool inited = false;
    if(inited)
        return;
    inited = true;
    for (int proc = 0; proc < 2; proc++)
        for (int i = 0; i < 0x4000; i++)
            JIT.JIT_MEM[proc][i] = (uintptr_t) (JIT_MEM[proc][i >> 9] + (((i << 14) & JIT_MASK[proc][i >> 9]) >> 1));
}
#else
DS_ALIGN(4096) uintptr_t g_CompiledFuncs[1<<26] = {0};
#endif

//#endif

// There should be some stuff from SLJIT here.