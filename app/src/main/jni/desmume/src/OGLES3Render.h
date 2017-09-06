/*
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


#ifndef OGLES3RENDER_H
#define OGLES3RENDER_H

#include <queue>
#include <set>
#include <string>
#include "render3D.h"
#include "types.h"
#include "OGLES2Render.h"

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>

#define OGLEXT(procPtr, func)		procPtr func = NULL;
#define INITOGLEXT(procPtr, func)	func = (procPtr)eglGetProcAddress(#func);
#define EXTERNOGLEXT(procPtr, func)	extern procPtr func;

#if !defined(GL_ES_VERSION_3_0)
#error OpenGL ES requires v3.0 headers or later.
#endif

extern GPU3DInterface gpu3Dgles3;

class OpenGLES3Renderer : public OpenGLESRenderer {
protected:

public:
    ~OpenGLES3Renderer();
};

#endif //OGLES3RENDER_H
