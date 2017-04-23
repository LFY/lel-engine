// Copyright 2017 Lingfeng Yang
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//         SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "RenderModel.h"

#include "lel.h"

#include <unordered_map>

#include <stdio.h>

namespace LEL {

static const char* const sDiffuseOnlyVShaderSrc = R"(
#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
uniform mat4 matrix;
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 v3NormalIn;
layout(location = 2) in vec2 v2TexCoordsIn;
out vec2 v2TexCoord;
void main() {
	v2TexCoord = v2TexCoordsIn;
	gl_Position = matrix * vec4(position.xyz, 1);
})";

static const char* const sDiffuseOnlyFShaderSrc = R"(
#version 150 core
uniform sampler2D diffuse;
in vec2 v2TexCoord;
out vec4 outputColor;
void main() {
    vec4 outColor = texture(diffuse, v2TexCoord);
    outputColor.rgb = outColor.rgb;
    outputColor.a = outColor.a;
})";

static std::unordered_map<int, RenderModelProgramInfo*> sRenderModelPrograms;

GLuint renderModelProgramId(RenderModel::RenderModelShadingType type) {
    return sRenderModelPrograms[type]->programId;
}

static RenderModelProgramInfo* sLazyProgramInit(RenderModel::RenderModelShadingType type) {
    if (sRenderModelPrograms[type]) return sRenderModelPrograms[type];

    GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vshadersrc = sDiffuseOnlyVShaderSrc;
    const char* fshadersrc = sDiffuseOnlyFShaderSrc;

    switch (type) {
	case RenderModel::RenderModelShadingType::DIFFUSE_ONLY:
        vshadersrc = sDiffuseOnlyVShaderSrc;
        fshadersrc = sDiffuseOnlyFShaderSrc;
        break;
    default:
        break;
    }

    GLint vshaderLen = (GLint)(strlen(vshadersrc) + 1);
    GLint fshaderLen = (GLint)(strlen(fshadersrc) + 1);
    glShaderSource(vsh, 1, &vshadersrc, &vshaderLen);
    glShaderSource(fsh, 1, &fshadersrc, &fshaderLen);

    glCompileShader(vsh);
    glCompileShader(fsh);

#define INFOLOG_MAX_LEN 2048
    char infologbuf[INFOLOG_MAX_LEN];

    glGetShaderInfoLog(vsh, INFOLOG_MAX_LEN, nullptr, infologbuf);
    fprintf(stderr, "%s: INFOLOG vsh %s\n", __func__, infologbuf);
    glGetShaderInfoLog(fsh, INFOLOG_MAX_LEN, nullptr, infologbuf);
    fprintf(stderr, "%s: INFOLOG fsh %s\n", __func__, infologbuf);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vsh);
    glAttachShader(prog, fsh);
    glLinkProgram(prog);

    GLint linkstatus;
    glGetProgramiv(prog, GL_LINK_STATUS, &linkstatus);

    fprintf(stderr, "%s: linkstatus %d\n", __func__, linkstatus);

    sRenderModelPrograms[type] = new RenderModelProgramInfo;
    sRenderModelPrograms[type]->vshaderId = vsh;
    sRenderModelPrograms[type]->fshaderId = fsh;
    sRenderModelPrograms[type]->programId = prog;
    sRenderModelPrograms[type]->uMatrixLoc =
        glGetUniformLocation(prog, "matrix");

    return sRenderModelPrograms[type];
}

RenderModel::RenderModel(const std::string& name,
                         const OBJData& objData,
                         RenderModelShadingType type) :
    mName(name), mOBJData(objData), mShadingType(type) { }

void RenderModel::initGL() {
    mProgramInfo = sLazyProgramInit(mShadingType);

    mIndexType = GL_UNSIGNED_INT;
    mVerts = (uint32_t)mOBJData.indexData.size();

    void* interleavedVertexAttribs = (void*)&mOBJData.vertexData[0];
    unsigned int attribDataLen = sizeof(OBJData::OBJVertex) * (unsigned int)mOBJData.vertexData.size();

    void* indexData = (void*)&mOBJData.indexData[0];
    unsigned int indexDataLen = sizeof(unsigned int) * (unsigned int)mOBJData.indexData.size();

    void* rgbaDiffuseTextureData = (void*)&mOBJData.diffuseTextureRGBA[0];
    unsigned int texWidth = mOBJData.diffuseTextureWidth;
    unsigned int texHeight = mOBJData.diffuseTextureHeight;

    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mIBO);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, attribDataLen, interleavedVertexAttribs, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataLen, indexData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    unsigned int floatSz = 4;
    unsigned int posOffset = 0;
    unsigned int normOffset = posOffset + floatSz * 3;
    unsigned int texCoordOffset = normOffset + floatSz * 3;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * (3 + 3 + 2), (void*)(uintptr_t)posOffset);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 4 * (3 + 3 + 2), (void*)(uintptr_t)normOffset);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * (3 + 3 + 2), (void*)(uintptr_t)texCoordOffset);

    glBindVertexArray(0);

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 texWidth, texHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgbaDiffuseTextureData);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

    GLfloat fLargest;
    glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest );

    glBindTexture( GL_TEXTURE_2D, 0 );

	initialized = true;
}

const std::string& RenderModel::getName() const {
	return mName;
}

GLuint RenderModel::getShaderProgramId() const {
	return mProgramInfo->programId;
}

GLuint RenderModel::transformMatrixLoc() const {
    return mProgramInfo->uMatrixLoc;
}

void RenderModel::draw() {
	glBindVertexArray( mVAO );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, mTexture );
	glDrawElements( GL_TRIANGLES, mVerts, mIndexType, 0 );
	glBindVertexArray( 0 );
}

void RenderModel::draw(const matrix4& transform) {
    glUniformMatrix4fv(mProgramInfo->uMatrixLoc,
                       1, GL_FALSE,
                       (RenderModel::getGlobalTransform() * transform).vals);
    draw();
}

RenderModel::~RenderModel() {
    glDeleteBuffers(1, &mIBO);
    glDeleteBuffers(1, &mVBO);
    glDeleteVertexArrays(1, &mVAO);
}

static matrix4 sGlobalTransform = identity4();

void RenderModel::setGlobalTransform(const matrix4& tr) {
    sGlobalTransform = tr;
}

const matrix4& RenderModel::getGlobalTransform() {
    return sGlobalTransform;
}

} // namespace LEL
