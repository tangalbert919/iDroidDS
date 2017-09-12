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

#if defined(GL_ES_VERSION_3_2)
#include <GLES3/gl32.h>
#elif defined(GL_ES_VERSION_3_1)
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#define OGLEXT(procPtr, func)		procPtr func = NULL;
#define INITOGLEXT(procPtr, func)	func = (procPtr)eglGetProcAddress(#func);
#define EXTERNOGLEXT(procPtr, func)	extern procPtr func;

#if !defined(GL_ES_VERSION_3_0)
#error OpenGL ES requires v3.0 headers or later.
#endif

#include "OGLES2Render.h"

void OGLES3LLoadEntryPoints();
void OGLES3CreateRenderer(OpenGLESRenderer **pGLESRenderer);

class OpenGLES3Renderer : public OpenGLES2Renderer {
protected:

    bool isPBOSupported;
    bool isMultisampledFBOSupported;
    virtual Render3DError InitExtensions();
    virtual Render3DError CreateFBOs();
    virtual void DestroyFBOs();
    virtual Render3DError CreateMultisampledFBO();
    virtual void DestroyMultisampledFBO();
    virtual Render3DError CreateVAOs();
    virtual void DestroyVAOs();
    virtual Render3DError CreatePBOs();
    virtual void DestroyPBOs();

    virtual Render3DError LoadShaderPrograms(std::string *outVertexShaderProgram, std::string *outFragmentShaderProgram);
    virtual Render3DError SetupShaderIO();

    virtual void GetExtensionSet(std::set<std::string> *oglExtensionSet);
    virtual Render3DError EnableVertexAttributes(const VERTLIST *vertlist, const GLushort *indexBuffer, const size_t vertIndexCount);
    virtual Render3DError DisableVertexAttributes();
    virtual Render3DError SelectRenderingFramebuffer();
    virtual Render3DError DownsampleFBO();
    virtual Render3DError ReadBackPixels();

    virtual Render3DError ClearUsingImage() const;
public:
    ~OpenGLES3Renderer();
};

#endif //OGLES3RENDER_H
