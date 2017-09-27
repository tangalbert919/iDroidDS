/*	sndsuperpowered.h
	Copyright (C) 2017 The nds4droid team

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

#ifndef SNDSUPERPOWERED_H
#define SNDSUPERPOWERED_H

#include "../SPU.h"

#define SNDCORE_SUPERPOWERED 1

extern SoundInterface_struct SNDSuperpowered;

void SNDSuperpoweredPaused(bool paused);

#endif //SNDSUPERPOWERED_H
