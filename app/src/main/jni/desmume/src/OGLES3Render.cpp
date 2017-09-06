/*
	Copyright (C) 2017 The nds4droid Team

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

#include "OGLES3Render.h"

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "gfx3d.h"
#include "NDSSystem.h"
#include "texcache.h"

//-------------------------------------------------------------

// Basic functions


typedef struct {
    unsigned int major;
    unsigned int minor;
} OGLVersion;

OpenGLES3Renderer::~OpenGLES3Renderer() {

    glFinish();

    DestroyVAOs();
    DestroyFBOs();
}
