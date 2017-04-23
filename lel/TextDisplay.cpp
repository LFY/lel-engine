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

#include "TextDisplay.h"

#include <vector>

static const char* const sTextDisplayVShaderSrc = R"(
#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
uniform mat4 matrix;
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;
out vec2 v2TexCoord;
void main() {
    v2TexCoord = texcoord;
	gl_Position = matrix * vec4(position.xy, 0, 1);
})";

static const char* const sTextDisplayFShaderSrc = R"(
#version 150 core
uniform sampler2D diffuse;
in vec2 v2TexCoord;
out vec4 outputColor;
void main() {
    outputColor = texture(diffuse, v2TexCoord);
})";

static const uint32_t kMaxNumCharacters = 64;

namespace LEL {

TextDisplay::TextDisplay() { }

// static
void TextDisplay::initGL(GLuint fontTextureId, GL32RenderState* outRenderState) {
    outRenderState->texture = fontTextureId;

    fprintf(stderr, "TextDisplay::%s\n", __func__);
    GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vshadersrc = sTextDisplayVShaderSrc;
    const char* fshadersrc = sTextDisplayFShaderSrc;

    GLint vshaderLen = (GLint)(strlen(vshadersrc) + 1);
    GLint fshaderLen = (GLint)(strlen(fshadersrc) + 1);
    glShaderSource(vsh, 1, &vshadersrc, &vshaderLen);
    glShaderSource(fsh, 1, &fshadersrc, &fshaderLen);

    glCompileShader(vsh);
    glCompileShader(fsh);

#define INFOLOG_MAX_LEN 2048
    char infologbuf[INFOLOG_MAX_LEN];

    glGetShaderInfoLog(vsh, INFOLOG_MAX_LEN, nullptr, infologbuf);
    fprintf(stderr, "TextDisplay::%s: INFOLOG vsh %s\n", __func__, infologbuf);
    glGetShaderInfoLog(fsh, INFOLOG_MAX_LEN, nullptr, infologbuf);
    fprintf(stderr, "TextDisplay::%s: INFOLOG fsh %s\n", __func__, infologbuf);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vsh);
    glAttachShader(prog, fsh);
    glLinkProgram(prog);

    GLint linkstatus;
    glGetProgramiv(prog, GL_LINK_STATUS, &linkstatus);

    fprintf(stderr, "TextDisplay::%s: linkstatus %d\n", __func__, linkstatus);

    outRenderState->program = prog;
    outRenderState->worldMatrixLoc = glGetUniformLocation(prog, "matrix");

    // Vertex/index data
    outRenderState->numVerts = 6 * kMaxNumCharacters;
    // each one gets x,y coordinate, 4 per character, and there are 4x s,t texture coords as well.
    // uint32_t vertexDataBytes = numChars * sizeof(float) * 2 * 4 * 2;
    // two triangles per character
    // uint32_t indexDataBytes = numChars * sizeof(uint32_t) * 2 * 3;

    std::vector<float> initialVertexData;
    float xoff = 0.0f;
    float yoff = 0.0f;
	float charWidth = 1.0f;
	float charHeight = 2.0f;
	float charSpacing = 0.2f;
    for (uint32_t i = 0; i < kMaxNumCharacters; i++) {
        initialVertexData.push_back(xoff + 0.0f);
        initialVertexData.push_back(yoff + 0.0f);

        initialVertexData.push_back(0.0f);
        initialVertexData.push_back(1.0f);

        initialVertexData.push_back(xoff + charWidth);
        initialVertexData.push_back(yoff + 0.0f);

        initialVertexData.push_back(1.0f);
        initialVertexData.push_back(1.0f);

        initialVertexData.push_back(xoff + charWidth);
        initialVertexData.push_back(yoff + charHeight);

        initialVertexData.push_back(1.0f);
        initialVertexData.push_back(0.0f);

        initialVertexData.push_back(xoff + 0.0f);
        initialVertexData.push_back(yoff + charHeight);

        initialVertexData.push_back(0.0f);
        initialVertexData.push_back(0.0f);

        xoff += charWidth + charSpacing;
    }

    std::vector<uint32_t> initialIndexData;
    for (uint32_t i = 0; i < kMaxNumCharacters; i++) {
        initialIndexData.push_back(i * 4 + 0);
        initialIndexData.push_back(i * 4 + 1);
        initialIndexData.push_back(i * 4 + 2);
        initialIndexData.push_back(i * 4 + 0);
        initialIndexData.push_back(i * 4 + 2);
        initialIndexData.push_back(i * 4 + 3);
    }

	GLuint ibo;
    glGenBuffers(1, &outRenderState->vbo);
    glGenBuffers(1, &ibo);
    glGenVertexArrays(1, &outRenderState->vao);

    glBindVertexArray(outRenderState->vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, outRenderState->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    glBufferData(GL_ARRAY_BUFFER,
                 initialVertexData.size() * sizeof(float),
                 &initialVertexData[0], GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 initialIndexData.size() * sizeof(uint32_t),
                 &initialIndexData[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(uintptr_t)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void TextDisplay::updateText() {
	numChars = (uint32_t)text.size();
    vertData.resize(numChars * 16);
    // assume: 32 width, 64 height, so:
    // 32 / 512 : 1 / 16 s step (16 per row)
    // 64 / 512 : 1 / 8 t step
    float sStep = 0.0625f;
    float tStep = 0.125f;
    unsigned char offset = 32;
    float xoff = 0.0f;
    float yoff = 0.0f;
    for (size_t i = 0; i < numChars; i++) {
        // get ascii code of the char
        unsigned char c = *(unsigned char*)&text[i];
        c = c - offset;
        float sTotal = sStep * (c % 16);
        float tTotal = tStep * (c / 16);

        vertData[16 * i + 0] = xoff + 0.0f;
        vertData[16 * i + 1] = yoff + 0.0f;

        vertData[16 * i + 2] = sTotal;
        vertData[16 * i + 3] = tTotal + tStep;

        vertData[16 * i + 4 + 0] = xoff + charWidth;
        vertData[16 * i + 4 + 1] = yoff + 0.0f;

        vertData[16 * i + 4 + 2] = sTotal + sStep;
        vertData[16 * i + 4 + 3] = tTotal + tStep;

        vertData[16 * i + 8 + 0] = xoff + charWidth;
        vertData[16 * i + 8 + 1] = yoff + charHeight;

        vertData[16 * i + 8 + 2] = sTotal + sStep;
        vertData[16 * i + 8 + 3] = tTotal;

        vertData[16 * i + 12 + 0] = xoff + 0.0f;
        vertData[16 * i + 12 + 1] = yoff + charHeight;

        vertData[16 * i + 12 + 2] = sTotal;
        vertData[16 * i + 12 + 3] = tTotal;
        xoff += charWidth + charSpacing;
    }
}

void TextDisplay::draw() {
    /*updateText();
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, numChars * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);*/
}

}
