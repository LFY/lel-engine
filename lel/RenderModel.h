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

#pragma once

#include "lel_sdl2_opengl.h"

#include "OBJData.h"

#include "matrix.h"

namespace LEL {

struct RenderModelProgramInfo {
    GLuint vshaderId = 0;
    GLuint fshaderId = 0;
    GLuint programId = 0;
    GLint posAttribLoc = 0;
    GLint normalAttribLoc = 1;
    GLint texcoordAttribLoc = 2;
    GLint uMatrixLoc = -1;
};

class RenderModel {
public:
    enum RenderModelShadingType {
        DIFFUSE_ONLY,
        MAX_RENDER_TYPES 
    };

    RenderModel(const std::string& name, const OBJData& objData, RenderModelShadingType = DIFFUSE_ONLY);
    void initGL();
	GLuint getShaderProgramId() const;
    GLuint transformMatrixLoc() const;
	const std::string& getName() const;
    void draw();
    void draw(const matrix4& transform);
    ~RenderModel();

    static void setGlobalTransform(const matrix4& tr);
    static const matrix4& getGlobalTransform();

    // TODO: add Vulkan support
    GLuint mVBO;
    GLuint mIBO;
    GLuint mVAO;
    GLuint mTexture;
    GLsizei mVerts;
    GLenum mIndexType;

    std::string mName;

    RenderModelShadingType mShadingType = RenderModelShadingType::DIFFUSE_ONLY;
    OBJData mOBJData;
    RenderModelProgramInfo* mProgramInfo;

	bool initialized = false;
};

GLuint renderModelProgramId(RenderModel::RenderModelShadingType type);

} // namespace LEL

