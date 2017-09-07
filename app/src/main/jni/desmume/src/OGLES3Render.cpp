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
OGLEXT(PFNGLGETSTRINGIPROC, glESGetStringi)

// Shaders
OGLEXT(PFNGLBINDFRAGDATALOCATIONEXTPROC, glBindFragDataLocationEXT)

// FBO
OGLEXT(PFNGLGENFRAMEBUFFERSPROC, glESGenFramebuffers)
OGLEXT(PFNGLBINDFRAMEBUFFERPROC, glESBindFramebuffer)
OGLEXT(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glESFramebufferRenderbuffer)
OGLEXT(PFNGLFRAMEBUFFERTEXTURE2DPROC, glESFramebufferTexture2D)
OGLEXT(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glESCheckFramebufferStatus)
OGLEXT(PFNGLDELETEFRAMEBUFFERSPROC, glESDeleteFramebuffers)
OGLEXT(PFNGLBLITFRAMEBUFFERPROC, glESBlitFramebuffer)
OGLEXT(PFNGLGENRENDERBUFFERSPROC, glESGenRenderbuffers)
OGLEXT(PFNGLBINDRENDERBUFFERPROC, glESBindRenderbuffer)
OGLEXT(PFNGLRENDERBUFFERSTORAGEPROC, glESRenderbufferStorage)
OGLEXT(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glESRenderbufferStorageMultisample)
OGLEXT(PFNGLDELETERENDERBUFFERSPROC, glESDeleteRenderbuffers)

void OGLLoadEntryPoints()
{
    // Basic functions
    INITOGLEXT(PFNGLGETSTRINGIPROC, glESGetStringi)

    // Shaders
    INITOGLEXT(PFNGLBINDFRAGDATALOCATIONEXTPROC, glBindFragDataLocationEXT)

    // FBO
    INITOGLEXT(PFNGLGENFRAMEBUFFERSPROC, glESGenFramebuffers)
    INITOGLEXT(PFNGLBINDFRAMEBUFFERPROC, glESBindFramebuffer)
    INITOGLEXT(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glESFramebufferRenderbuffer)
    INITOGLEXT(PFNGLFRAMEBUFFERTEXTURE2DPROC, glESFramebufferTexture2D)
    INITOGLEXT(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glESCheckFramebufferStatus)
    INITOGLEXT(PFNGLDELETEFRAMEBUFFERSPROC, glESDeleteFramebuffers)
    INITOGLEXT(PFNGLBLITFRAMEBUFFERPROC, glESBlitFramebuffer)
    INITOGLEXT(PFNGLGENRENDERBUFFERSPROC, glESGenRenderbuffers)
    INITOGLEXT(PFNGLBINDRENDERBUFFERPROC, glESBindRenderbuffer)
    INITOGLEXT(PFNGLRENDERBUFFERSTORAGEPROC, glESRenderbufferStorage)
    INITOGLEXT(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glESRenderbufferStorageMultisample)
    INITOGLEXT(PFNGLDELETERENDERBUFFERSPROC, glESDeleteRenderbuffers)
}

// Vertex Shader GLSL 1.50
static const char *vertexShader_150 = {"\
	#version 150 \n\
	\n\
	in vec4 inPosition; \n\
	in vec2 inTexCoord0; \n\
	in vec3 inColor; \n\
	\n\
	uniform float polyAlpha; \n\
	uniform vec2 texScale; \n\
	\n\
	out vec4 vtxPosition; \n\
	out vec2 vtxTexCoord; \n\
	out vec4 vtxColor; \n\
	\n\
	void main() \n\
	{ \n\
		mat2 texScaleMtx	= mat2(	vec2(texScale.x,        0.0), \n\
									vec2(       0.0, texScale.y)); \n\
		\n\
		vtxPosition = inPosition; \n\
		vtxTexCoord = texScaleMtx * inTexCoord0; \n\
		vtxColor = vec4(inColor * 4.0, polyAlpha); \n\
		\n\
		gl_Position = vtxPosition; \n\
	} \n\
"};

static const char *fragmentShader_150 = {"\
	#version 150 \n\
	\n\
	in vec4 vtxPosition; \n\
	in vec2 vtxTexCoord; \n\
	in vec4 vtxColor; \n\
	\n\
	uniform sampler2D texMainRender; \n\
	uniform sampler1D texToonTable; \n\
	\n\
	uniform int stateToonShadingMode; \n\
	uniform bool stateEnableAlphaTest; \n\
	uniform bool stateUseWDepth; \n\
	uniform float stateAlphaTestRef; \n\
	\n\
	uniform int polyMode; \n\
	uniform int polyID; \n\
	\n\
	uniform bool polyEnableTexture; \n\
	\n\
	out vec4 outFragColor; \n\
	\n\
	void main() \n\
	{ \n\
		vec4 mainTexColor = (polyEnableTexture) ? texture(texMainRender, vtxTexCoord) : vec4(1.0, 1.0, 1.0, 1.0); \n\
		vec4 tempFragColor = mainTexColor; \n\
		\n\
		if(polyMode == 0) \n\
		{ \n\
			tempFragColor = vtxColor * mainTexColor; \n\
		} \n\
		else if(polyMode == 1) \n\
		{ \n\
			tempFragColor.rgb = (polyEnableTexture) ? (mainTexColor.rgb * mainTexColor.a) + (vtxColor.rgb * (1.0 - mainTexColor.a)) : vtxColor.rgb; \n\
			tempFragColor.a = vtxColor.a; \n\
		} \n\
		else if(polyMode == 2) \n\
		{ \n\
			vec3 toonColor = vec3(texture(texToonTable, vtxColor.r).rgb); \n\
			tempFragColor.rgb = (stateToonShadingMode == 0) ? mainTexColor.rgb * toonColor.rgb : min((mainTexColor.rgb * vtxColor.rgb) + toonColor.rgb, 1.0); \n\
			tempFragColor.a = mainTexColor.a * vtxColor.a; \n\
		} \n\
		else if(polyMode == 3) \n\
		{ \n\
			if (polyID != 0) \n\
			{ \n\
				tempFragColor = vtxColor; \n\
			} \n\
		} \n\
		\n\
		if (tempFragColor.a == 0.0 || (stateEnableAlphaTest && tempFragColor.a < stateAlphaTestRef)) \n\
		{ \n\
			discard; \n\
		} \n\
		\n\
		float vertW = (vtxPosition.w == 0.0) ? 0.00000001 : vtxPosition.w; \n\
		gl_FragDepth = (stateUseWDepth) ? vtxPosition.w/4096.0 : clamp((vtxPosition.z/vertW) * 0.5 + 0.5, 0.0, 1.0); \n\
		outFragColor = tempFragColor; \n\
	} \n\
"};

void OGLCreateRenderer(OpenGLESRenderer **pGLESRenderer)
{
    if (IsVersionSupported(3, 0))
    {
        *pGLESRenderer = new OpenGLES3Renderer;
        (*pGLESRenderer)->SetVersion(3,0);
    }
}

OpenGLES3Renderer::~OpenGLES3Renderer() {

    glFinish();

    DestroyVAOs();
    DestroyFBOs();
    DestroyMultisampledFBO();
}

Render3DError OpenGLES3Renderer::InitExtensions() {
    Render3DError error = OGLERROR_NOERR;
    OGLESRenderRef &OGLRef = *this->ref;

    // Get OpenGL extensions
    std::set<std::string> oglExtensionSet;
    this->GetExtensionSet(&oglExtensionSet);

    // Initialize OpenGL ES 3.0
    this->InitTables();

    // Load and create shaders. This is mandatory.
    this->isShaderSupported = true;

    std::string vertexShaderProgram;
    std::string fragmentShaderProgram;
    error = this->LoadShaderPrograms(&vertexShaderProgram, &fragmentShaderProgram);
    if (error != OGLERROR_NOERR) {
        this->isShaderSupported = false;
        return error;
    }

    error = this->CreateShaders(&vertexShaderProgram, &fragmentShaderProgram);
    if (error != OGLERROR_NOERR) {
        this->isShaderSupported = false;
        return error;
    }

    this->CreateToonTable();

    this->isVBOSupported = true;
    this->CreateVBOs();

    this->isPBOSupported = true;
    this->CreatePBOs();

    this->isVAOSupported = true;
    this->CreateVAOs();

    this->isFBOSupported = true;
    error = this->CreateFBOs();
    if (error != OGLERROR_NOERR) {
        OGLRef.fboRenderID = 0;
        this->isFBOSupported = false;
        return error;
    }

    this->isMultisampledFBOSupported = true;
    error = this->CreateMultisampledFBO();
    if (error != OGLERROR_NOERR) {
        OGLRef.selectedRenderingFBO = 0;
        this->isMultisampledFBOSupported = false;

        if (error == OGLERROR_FBO_CREATE_ERROR)
            return error;
    }

    this->InitTextures();
    this->InitFinalRenderStates(&oglExtensionSet);

    return OGLERROR_NOERR;
}

Render3DError OpenGLES3Renderer::CreateFBOs() {
    OGLESRenderRef &OGLRef = *this->ref;

    // Set up FBO render targets
    glGenTextures(1, &OGLRef.texClearImageColorID);
    glGenTextures(1, &OGLRef.texClearImageDepthStencilID);

    glBindTexture(GL_TEXTURE_2D, OGLRef.texClearImageColorID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_6_5, NULL);

    glBindTexture(GL_TEXTURE_2D, OGLRef.texClearImageDepthStencilID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Set up FBOs
    glGenFramebuffers(1, &OGLRef.fboClearImageID);
    glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.fboClearImageID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, OGLRef.texClearImageColorID, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, OGLRef.texClearImageDepthStencilID, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        INFO("OpenGL ES 3.0: Failed to create FBOs. Some emulation features will be disabled.\n");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &OGLRef.fboClearImageID);
        glDeleteTextures(1, &OGLRef.texClearImageColorID);
        glDeleteTextures(1, &OGLRef.texClearImageDepthStencilID);

        return OGLERROR_FBO_CREATE_ERROR;
    }

}
