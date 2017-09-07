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

typedef struct
{
    unsigned int major;
    unsigned int minor;
} OGLVersion;

static OGLVersion _OGLDriverVersion = {0, 0};
static OpenGLESRenderer *_OGLRenderer = NULL;
static bool BEGINGL()
{
    if(oglrender_beginOpenGL)
        return oglrender_beginOpenGL();
    else return true;
}

static void ENDGL()
{
    if(oglrender_endOpenGL)
        oglrender_endOpenGL();
}

//-------------------------------------------------------------

// Basic functions
OGLEXT(PFNGLGETSTRINGIPROC, glES3GetStringi)

// Shaders
OGLEXT(PFNGLBINDFRAGDATALOCATIONEXTPROC, glESBindFragDataLocationEXT)

// FBO
OGLEXT(PFNGLGENFRAMEBUFFERSPROC, glES3GenFramebuffers)
OGLEXT(PFNGLBINDFRAMEBUFFERPROC, glES3BindFramebuffer)
OGLEXT(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glES3FramebufferRenderbuffer)
OGLEXT(PFNGLFRAMEBUFFERTEXTURE2DPROC, glES3FramebufferTexture2D)
OGLEXT(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glES3CheckFramebufferStatus)
OGLEXT(PFNGLDELETEFRAMEBUFFERSPROC, glES3DeleteFramebuffers)
OGLEXT(PFNGLBLITFRAMEBUFFERPROC, glES3BlitFramebuffer)
OGLEXT(PFNGLGENRENDERBUFFERSPROC, glES3GenRenderbuffers)
OGLEXT(PFNGLBINDRENDERBUFFERPROC, glES3BindRenderbuffer)
OGLEXT(PFNGLRENDERBUFFERSTORAGEPROC, glES3RenderbufferStorage)
OGLEXT(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glES3RenderbufferStorageMultisample)
OGLEXT(PFNGLDELETERENDERBUFFERSPROC, glES3DeleteRenderbuffers)

void OGLLoadEntryPoints()
{
    // Basic functions
    INITOGLEXT(PFNGLGETSTRINGIPROC, glES3GetStringi)

    // Shaders
    INITOGLEXT(PFNGLBINDFRAGDATALOCATIONEXTPROC, glESBindFragDataLocationEXT)

    // FBO
    INITOGLEXT(PFNGLGENFRAMEBUFFERSPROC, glES3GenFramebuffers)
    INITOGLEXT(PFNGLBINDFRAMEBUFFERPROC, glES3BindFramebuffer)
    INITOGLEXT(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glES3FramebufferRenderbuffer)
    INITOGLEXT(PFNGLFRAMEBUFFERTEXTURE2DPROC, glES3FramebufferTexture2D)
    INITOGLEXT(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glES3CheckFramebufferStatus)
    INITOGLEXT(PFNGLDELETEFRAMEBUFFERSPROC, glES3DeleteFramebuffers)
    INITOGLEXT(PFNGLBLITFRAMEBUFFERPROC, glES3BlitFramebuffer)
    INITOGLEXT(PFNGLGENRENDERBUFFERSPROC, glES3GenRenderbuffers)
    INITOGLEXT(PFNGLBINDRENDERBUFFERPROC, glES3BindRenderbuffer)
    INITOGLEXT(PFNGLRENDERBUFFERSTORAGEPROC, glES3RenderbufferStorage)
    INITOGLEXT(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glES3RenderbufferStorageMultisample)
    INITOGLEXT(PFNGLDELETERENDERBUFFERSPROC, glES3DeleteRenderbuffers)
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

    // Set up final output FBO
    glGenRenderbuffers(1, &OGLRef.rboFragColorID);
    glGenRenderbuffers(1, &OGLRef.rboFragDepthStencilID);
    glBindRenderbuffer(GL_RENDERBUFFER, OGLRef.rboFragColorID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, OGLRef.rboFragDepthStencilID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT);

    glGenFramebuffers(1, &OGLRef.fboRenderID);
    glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.fboRenderID);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, OGLRef.rboFragColorID);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, OGLRef.rboFragDepthStencilID);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        INFO("OpenGL ES 3.0: Failed to created FBOs. Some emulation features will be disabled.\n");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &OGLRef.fboClearImageID);
        glDeleteTextures(1, &OGLRef.texClearImageColorID);
        glDeleteTextures(1, &OGLRef.texClearImageDepthStencilID);

        glDeleteFramebuffers(1, &OGLRef.fboRenderID);
        glDeleteRenderbuffers(1, &OGLRef.rboFragColorID);
        glDeleteRenderbuffers(1, &OGLRef.rboFragDepthStencilID);

        OGLRef.fboRenderID = 0;
        return OGLERROR_FBO_CREATE_ERROR;
    }

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    INFO("OpenGL: Successfully created FBOs.\n");

    return OGLERROR_NOERR;
}

void OpenGLES3Renderer::DestroyFBOs() {
    if (!this->isFBOSupported)
    {
        return;
    }

    OGLESRenderRef &OGLRef = *this->ref;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &OGLRef.fboClearImageID);
    glDeleteTextures(1, &OGLRef.texClearImageColorID);
    glDeleteTextures(1, &OGLRef.texClearImageDepthStencilID);

    glDeleteFramebuffers(1, &OGLRef.fboRenderID);
    glDeleteRenderbuffers(1, &OGLRef.rboFragColorID);
    glDeleteRenderbuffers(1, &OGLRef.rboFragDepthStencilID);

    this->isFBOSupported = false;
}

Render3DError OpenGLES3Renderer::CreateMultisampledFBO() {
    // Check the maximum number of samples that the GPU supports and use that.
    // Since our target resolution is only 256x192 pixels, using the most samples
    // possible is the best thing to do.
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    if (maxSamples < 2)
    {
        INFO("OpenGL: GPU does not support at least 2x multisampled FBOs. Multisample antialiasing will be disabled.\n");
        return OGLERROR_FEATURE_UNSUPPORTED;
    }
    else if (maxSamples > OGLRENDER_MAX_MULTISAMPLES)
    {
        maxSamples = OGLRENDER_MAX_MULTISAMPLES;
    }

    OGLESRenderRef &OGLRef = *this->ref;

    // Set up FBO render targets
    glGenRenderbuffers(1, &OGLRef.rboMSFragColorID);
    glGenRenderbuffers(1, &OGLRef.rboMSFragDepthStencilID);

    glBindRenderbuffer(GL_RENDERBUFFER, OGLRef.rboMSFragColorID);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, maxSamples, GL_RGBA, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, OGLRef.rboMSFragDepthStencilID);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, maxSamples, GL_DEPTH24_STENCIL8, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT);

    // Set up multisampled rendering FBO
    glGenFramebuffers(1, &OGLRef.fboMSIntermediateRenderID);
    glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.fboMSIntermediateRenderID);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, OGLRef.rboMSFragColorID);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, OGLRef.rboMSFragDepthStencilID);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &OGLRef.fboMSIntermediateRenderID);
        glDeleteRenderbuffers(1, &OGLRef.rboMSFragColorID);
        glDeleteRenderbuffers(1, &OGLRef.rboMSFragDepthStencilID);

        INFO("OpenGL ES 3.0: Failed to create multisampled FBO. Multisample antialiasing will be disabled.\n");
        return OGLERROR_FBO_CREATE_ERROR;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.fboRenderID);
    INFO("OpenGL ES 3.0: Successfully created multisampled FBO.\n");

    return OGLERROR_NOERR;
}

void OpenGLES3Renderer::DestroyMultisampledFBO() {
    if (!this->isMultisampledFBOSupported)
    {
        return;
    }

    OGLESRenderRef &OGLRef = *this->ref;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &OGLRef.fboMSIntermediateRenderID);
    glDeleteRenderbuffers(1, &OGLRef.rboMSFragColorID);
    glDeleteRenderbuffers(1, &OGLRef.rboMSFragDepthStencilID);

    this->isMultisampledFBOSupported = false;
}

Render3DError OpenGLES3Renderer::CreateVAOs() {
    OGLESRenderRef &OGLRef = *this->ref;

    glGenVertexArrays(1, &OGLRef.vaoMainStatesID);
    glBindVertexArray(OGLRef.vaoMainStatesID);

    glBindBuffer(GL_ARRAY_BUFFER, OGLRef.vboVertexID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OGLRef.iboIndexID);

    glEnableVertexAttribArray(OGLVertexAttributeID_Position);
    glEnableVertexAttribArray(OGLVertexAttributeID_TexCoord0);
    glEnableVertexAttribArray(OGLVertexAttributeID_Color);

    glVertexAttribPointer(OGLVertexAttributeID_Position, 4, GL_FLOAT, GL_FALSE, sizeof(VERT), (const GLvoid *)offsetof(VERT, coord));
    glVertexAttribPointer(OGLVertexAttributeID_TexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(VERT), (const GLvoid *)offsetof(VERT, texcoord));
    glVertexAttribPointer(OGLVertexAttributeID_Color, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VERT), (const GLvoid *)offsetof(VERT, color));

    glBindVertexArray(0);

    return OGLERROR_NOERR;
}

void OpenGLES3Renderer::DestroyVAOs() {
    if (!this->isVAOSupported)
    {
        return;
    }

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &this->ref->vaoMainStatesID);

    this->isVAOSupported = false;
}

Render3DError OpenGLES3Renderer::LoadShaderPrograms(std::string *outVertexShaderProgram,
                                                    std::string *outFragmentShaderProgram) {
    *outVertexShaderProgram = std::string(vertexShader_150);
    *outFragmentShaderProgram = std::string(fragmentShader_150);

    return OGLERROR_NOERR;
}

Render3DError OpenGLES3Renderer::SetupShaderIO() {
    OGLESRenderRef &OGLRef = *this->ref;

    glBindAttribLocation(OGLRef.shaderProgram, OGLVertexAttributeID_Position, "inPosition");
    glBindAttribLocation(OGLRef.shaderProgram, OGLVertexAttributeID_TexCoord0, "inTexCoord0");
    glBindAttribLocation(OGLRef.shaderProgram, OGLVertexAttributeID_Color, "inColor");
    glBindFragDataLocationEXT(OGLRef.shaderProgram, 0, "outFragColor");

    return OGLERROR_NOERR;
}

Render3DError OpenGLES3Renderer::CreatePBOs() {
    OGLESRenderRef &OGLRef = *this->ref;


}

void OpenGLES3Renderer::DestroyPBOs() {

}

void OpenGLES3Renderer::GetExtensionSet(std::set<std::string> *oglExtensionSet) {
    GLint extensionCount = 0;

    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    for (size_t i = 0; i < extensionCount; i++)
    {
        std::string extensionName = std::string((const char *)glGetStringi(GL_EXTENSIONS, i));
        oglExtensionSet->insert(extensionName);
    }
}

Render3DError OpenGLES3Renderer::EnableVertexAttributes(const VERTLIST *vertlist, const GLushort *indexBuffer, const size_t vertIndexCount) {
    OGLESRenderRef &OGLRef = *this->ref;

    glBindVertexArray(OGLRef.vaoMainStatesID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VERT) * vertlist->count, vertlist);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, vertIndexCount * sizeof(GLushort), indexBuffer);

    return OGLERROR_NOERR;
}

Render3DError OpenGLES3Renderer::DisableVertexAttributes() {
    glBindVertexArray(0);
    return OGLERROR_NOERR;
}

Render3DError OpenGLES3Renderer::SelectRenderingFramebuffer() {
    OGLESRenderRef &OGLRef = *this->ref;
    static const GLenum drawDirect[1] = {GL_COLOR_ATTACHMENT0};

    OGLRef.selectedRenderingFBO = (CommonSettings.GFX3D_Renderer_Multisample) ? OGLRef.fboMSIntermediateRenderID : OGLRef.fboRenderID;
    glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.selectedRenderingFBO);
    glDrawBuffers(1, &drawDirect[0]);

    return OGLERROR_NOERR;
}

Render3DError OpenGLES3Renderer::DownsampleFBO() {
    OGLESRenderRef &OGLRef = *this->ref;

    if (OGLRef.selectedRenderingFBO != OGLRef.fboMSIntermediateRenderID)
    {
        return OGLERROR_NOERR;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, OGLRef.selectedRenderingFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, OGLRef.fboRenderID);
    glBlitFramebuffer(0, 0, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT, 0, 0, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.fboRenderID);

    return OGLERROR_NOERR;
}

Render3DError OpenGLES3Renderer::ClearUsingImage() const {
    OGLESRenderRef &OGLRef = *this->ref;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, OGLRef.fboClearImageID);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, OGLRef.selectedRenderingFBO);
    glBlitFramebuffer(0, 0, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT, 0, 0, GFX3D_FRAMEBUFFER_WIDTH, GFX3D_FRAMEBUFFER_HEIGHT, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.selectedRenderingFBO);

    return OGLERROR_NOERR;
}
