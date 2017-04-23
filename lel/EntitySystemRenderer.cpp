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

#include "EntitySystemRenderer.h"

#include "lel_sdl2_opengl.h"
#include "lel.h"
#include "lodepng.h"

#define GL_ERR_CHECK() do { \
    GLint err = glGetError(); \
    if (err) fprintf(stderr, "%s:%d GL error 0x%x\n", __func__, __LINE__, err); \
} while(0) \

#define GL_ERR_CHECK_INT(v) do { \
    GLint err = glGetError(); \
    if (err) { fprintf(stderr, "%s:%d GL error 0x%x (val %d)\n", __func__, __LINE__, err, v); abort(); } \
} while(0) \

namespace LEL {

static const char* const sColliderVShaderSrc = R"(
#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
uniform mat4 matrix;
layout(location = 0) in vec4 position;
void main() {
	gl_Position = matrix * vec4(position.xyz, 1);
})";

static const char* const sColliderFShaderSrc = R"(
#version 150 core
out vec4 outputColor;
uniform vec3 color;
void main() {
    outputColor = vec4(color.xyz, 0.3);
})";

static const char* const sFullscreenQuadVShaderSrc = R"(
#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
uniform mat4 matrix;
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoordIn;
out vec2 texcoordOut;
void main() {
	gl_Position = vec4(position.xy, 0, 1);
    texcoordOut = texcoordIn;
})";

static const char* const sFullscreenQuadFShaderSrc = R"(
#version 150 core
uniform sampler2D img;
uniform float brightness;
in vec2 texcoordOut;
out vec4 outputColor;
void main() {
    vec4 color = texture(img, texcoordOut);
    outputColor.rgb = brightness * color.rgb;
    outputColor.a = color.a;
})";

static const uint32_t kFontTextureDim = 1024;

void EntitySystemRenderer::initGL() {
    for (const auto& it: mSys->mModels) {
        it.second->initGL();
    }

    mColliderProgramId =
        createShaderProgram(
                sColliderVShaderSrc,
                sColliderFShaderSrc, nullptr, nullptr);
    mColliderMatrixLoc = glGetUniformLocation(mColliderProgramId, "matrix");
    mColliderColorLoc = glGetUniformLocation(mColliderProgramId, "color");

    glGenBuffers(1, &mColliderVBO);
    glGenBuffers(1, &mColliderIBO);
    glGenVertexArrays(1, &mColliderVAO);

    glBindVertexArray(mColliderVAO);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, mColliderVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mColliderIBO);

    glBufferData(GL_ARRAY_BUFFER, 8 * 3 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

    std::vector<uint32_t> colliderIBO = {
        // top, bottom
        0, 2, 1, 3, 1, 2,
        4, 6, 5, 7, 5, 6,

        // front corner
        0, 1, 4, 4, 1, 5,
        0, 4, 2, 2, 4, 6,

        // back corner
        7, 5, 1, 7, 1, 3,
        7, 2, 6, 2, 7, 3,
    };

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(uint32_t), &colliderIBO[0], GL_STATIC_DRAW);
    glBindVertexArray(0);

    fprintf(stderr, "%s: loading text display\n", __func__);

    // Also initialize TextDisplay's
    // first, initialize the font texture
    glGenTextures(1, &mTextDisplayRenderState.texture);
    glBindTexture(GL_TEXTURE_2D, mTextDisplayRenderState.texture);
    std::vector<unsigned char> fontTextureRGBABytes;
    unsigned int fontTexWidth = kFontTextureDim;
    unsigned int fontTexHeight = kFontTextureDim;
    lodepng::decode(
            fontTextureRGBABytes, fontTexWidth, fontTexHeight,
            LEL::pathCat(
                mSys->mAssetDir,
                "Inconsolata.png"));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kFontTextureDim, kFontTextureDim, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, &fontTextureRGBABytes[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kFontTextureDim, kFontTextureDim, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, &fontTextureRGBABytes[0]);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLint err = glGetError();
    if (err) fprintf(stderr, "%s: GL err 0x%x\n", __func__, err);
	TextDisplay::initGL(mTextDisplayRenderState.texture, &mTextDisplayRenderState);
    err = glGetError();
    if (err) fprintf(stderr, "%s: GL err 0x%x\n", __func__, err);

    {

        mFullscreenQuadRenderState.depthTest = false;
        mFullscreenQuadRenderState.texture = 0;
        mFullscreenQuadRenderState.program =
            createShaderProgram(
                    sFullscreenQuadVShaderSrc,
                    sFullscreenQuadFShaderSrc, nullptr, nullptr);
        mFullscreenQuadRenderState.worldMatrixLoc =
            glGetUniformLocation(mFullscreenQuadRenderState.program, "img");
        mBrightnessUniformLoc =
            glGetUniformLocation(mFullscreenQuadRenderState.program, "brightness");

        glGenVertexArrays(1, &mFullscreenQuadRenderState.vao);
        glGenBuffers(1, &mFullscreenQuadRenderState.vbo);
        GLuint ibo;
        glGenBuffers(1, &ibo);
        glGenTextures(1, &mFullscreenQuadRenderState.texture);
        mFullscreenQuadRenderState.numVerts = 6;

        glBindVertexArray(mFullscreenQuadRenderState.vao);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        std::vector<uint32_t> indices = {
            0, 1, 2, 0, 2, 3,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 6, &indices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, mFullscreenQuadRenderState.vbo);
        std::vector<float> vertexAttrs = {
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 0.0f, 1.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &vertexAttrs[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(uintptr_t)(sizeof(float) * 2));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    mCameraState.numUpdates = 0;
}

void EntitySystemRenderer::makeCurrent() {
    mClearColor = { 0.3f, 0.5f, 0.7f, 0.0f };
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EntitySystemRenderer::preDraw() {
	const Entity& currentCamera =
		mSys->getEntityById(mSys->mCurrentCamera);

    mCameraState.worldFrame = currentCamera.getWorldFrame();
    mCameraState.projection = currentCamera.getProjection();
    mCameraState.focusTarget = mSys->getCurrentFocusPos();
    updateCameraFocalDist();
    mCameraState.numUpdates++;

	std::vector<LEL::Entity>& entities = mSys->mEntities;
    mWorldRenderStates.resize(2 * entities.size());

    size_t k = 0;
    for (auto& ent : entities) {

        matrix4 worldMatrix = ent.currentWorldMatrix();

        RenderModel* renderModel = ent.getRenderModel();

        if (!renderModel || ent.isUi || ent.getTextDisplay()) continue;
        if (k == mWorldRenderStates.size())
            mWorldRenderStates.resize(k + 1);
		if (!renderModel->initialized) renderModel->initGL();

        mWorldRenderStates[k].depthTest = true;
        mWorldRenderStates[k].program = renderModel->getShaderProgramId();
        mWorldRenderStates[k].vao = renderModel->mVAO;
        mWorldRenderStates[k].vbo = renderModel->mVBO;
        mWorldRenderStates[k].texture = renderModel->mTexture;
        mWorldRenderStates[k].numVerts = renderModel->mVerts;
        mWorldRenderStates[k].worldMatrixLoc = renderModel->transformMatrixLoc();
        mWorldRenderStates[k].worldMatrix = worldMatrix;
		mWorldRenderStates[k].vboData = nullptr;
		mWorldRenderStates[k].vboDataBytes = 0;
        k++;
    }

    mWorldRenderStates.resize(k);

    k = 0;
    for (auto& ent : entities) {

        matrix4 worldMatrix = ent.currentWorldMatrix();
        TextDisplay* textDisplay = ent.getTextDisplay();
        RenderModel* renderModel = ent.getRenderModel();

        if ((!textDisplay && !ent.isUi) || !renderModel) continue;
		if (!renderModel->initialized) renderModel->initGL();
        if (k == mUiRenderStates.size())
            mUiRenderStates.resize(k + 1);

        if (textDisplay) {
            textDisplay->updateText();

            mUiRenderStates[k] = mTextDisplayRenderState;
            mUiRenderStates[k].program = mTextDisplayRenderState.program;
            mUiRenderStates[k].worldMatrix = worldMatrix;
            mUiRenderStates[k].numVerts = textDisplay->numChars * 6;
            mUiRenderStates[k].vboData = (void*)(&textDisplay->vertData[0]);
            mUiRenderStates[k].vboDataBytes = sizeof(float) * (unsigned int)textDisplay->vertData.size();
        } else if (renderModel) {
            mUiRenderStates[k].depthTest = true;
            mUiRenderStates[k].program = renderModel->getShaderProgramId();
            mUiRenderStates[k].vao = renderModel->mVAO;
            mUiRenderStates[k].vbo = renderModel->mVBO;
            mUiRenderStates[k].texture = renderModel->mTexture;
            mUiRenderStates[k].numVerts = renderModel->mVerts;
            mUiRenderStates[k].worldMatrixLoc = renderModel->transformMatrixLoc();
            mUiRenderStates[k].worldMatrix = worldMatrix;
            mUiRenderStates[k].vboData = nullptr;
            mUiRenderStates[k].vboDataBytes = 0;
        }

        k++;
    }
    mUiRenderStates.resize(k);

    if (mDofRenderInfo.enabled) {
        if (!mDofRenderInfo.initialized) {
            initializeDofRenderer();
        }
        updateDofRenderer();
    }
}

void EntitySystemRenderer::updateRenderState(const GL32RenderState& state) {
    if (state.depthTest != lastRenderState.depthTest) {
        if (state.depthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    if (state.program != lastRenderState.program) {
        glUseProgram(state.program);
    }

    if (state.vbo != lastRenderState.vbo) {
        glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
    }

    if (state.vao != lastRenderState.vao) {
        glBindVertexArray(state.vao);
    }

    if (state.texture != lastRenderState.texture) {
        glBindTexture(GL_TEXTURE_2D, state.texture);
    }

    lastRenderState = state;
}

void EntitySystemRenderer::initializeDofRenderer() {
    DofRenderInfo& info = mDofRenderInfo;

    glGenFramebuffers(1, &info.fbo);
    glGenFramebuffers(1, &info.highresFbo);

    glGenTextures(1, &info.texture);
    glGenTextures(1, &info.highresTexture);

    glGenRenderbuffers(1, &info.rboDs);
    glGenRenderbuffers(1, &info.highresRboDs);

    glBindFramebuffer(GL_FRAMEBUFFER, info.fbo);

    glBindTexture(GL_TEXTURE_2D, info.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, info.xres, info.yres, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glGenerateMipmap(GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, info.texture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, info.rboDs);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, info.xres, info.yres);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, info.rboDs);

    fprintf(stderr, "%s: fb complete? 0x%x\n", __func__, glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glBindFramebuffer(GL_FRAMEBUFFER, info.highresFbo);
    glBindTexture(GL_TEXTURE_2D, info.highresTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, info.xresHigh, info.yresHigh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glGenerateMipmap(GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, info.highresTexture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, info.highresRboDs);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, info.xresHigh, info.yresHigh);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, info.highresRboDs);

    fprintf(stderr, "%s: highres fb complete? 0x%x\n", __func__, glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    info.initialized = true;
}

void EntitySystemRenderer::updateDofRenderer() {
    DofRenderInfo& info = mDofRenderInfo;
    info.adjustedCameraFrames.clear();

    const RefFrame& currFrame = mCameraState.worldFrame;
    vector4 cameraFocusPos =
        currFrame.pos + currFrame.fwd * mCameraState.focalDist;

    {
        float angleStep = 2.0f * M_PI / (float)info.numPoints;
        vector4 cameraSampleOffset = info.radius * currFrame.up;

        info.adjustedCameraFrames.push_back(mCameraState.worldFrame);

        for (uint32_t i = 0; i < info.numPoints; i++) {
            vector4 currOffset =
                rotation(currFrame.fwd.x, currFrame.fwd.y, currFrame.fwd.z, i * angleStep) *
                cameraSampleOffset;
            RefFrame adjusted = currFrame;
            adjusted.pos = adjusted.pos + currOffset;
            adjusted.fwd = v4normed(cameraFocusPos - adjusted.pos);
            info.adjustedCameraFrames.push_back(adjusted);
        }
    }

    {
        float angleStep = 2.0f * M_PI / (float)info.numPoints2;
        vector4 cameraSampleOffset = info.radius2 * currFrame.up;

        for (uint32_t i = 0; i < info.numPoints; i++) {
            vector4 currOffset =
                rotation(currFrame.fwd.x, currFrame.fwd.y, currFrame.fwd.z, i * angleStep) *
                cameraSampleOffset;
            RefFrame adjusted = currFrame;
            adjusted.pos = adjusted.pos + currOffset;
            adjusted.fwd = v4normed(cameraFocusPos - adjusted.pos);
            info.adjustedCameraFrames.push_back(adjusted);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, info.fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EntitySystemRenderer::deinitializeDofRenderer() {
    DofRenderInfo& info = mDofRenderInfo;
    mDofRenderInfo.initialized = false;
    mDofRenderInfo.adjustedCameraFrames.clear();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteFramebuffers(1, &info.fbo);
    glDeleteTextures(1, &info.texture);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EntitySystemRenderer::accumulateDofFbo(GLuint tex) {
    float clearColorFactor =
        (float) mDofRenderInfo.adjustedCameraFrames.size();
    glViewport(0, 0, mXres, mYres);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mFullscreenQuadRenderState.texture = mDofRenderInfo.texture;
    updateRenderState(mFullscreenQuadRenderState);
    glUniform1f(mBrightnessUniformLoc, 1.0f / clearColorFactor);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
    glDrawElements(GL_TRIANGLES, mFullscreenQuadRenderState.numVerts, GL_UNSIGNED_INT, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EntitySystemRenderer::updateCameraFocalDist() {
    const RefFrame& frame = mCameraState.worldFrame;
    float target = v4dot(mCameraState.focusTarget - frame.pos, frame.fwd);

    if (target < 5.0f) target = 5.0f;
    if (target > 300.0f) target = 300.0f;

    if (mCameraState.numUpdates < 2) {
        mCameraState.focalDist = target;
    } else {
        // don't abrupty change focal dist
        float delta = target - mCameraState.focalDist;
        mCameraState.focalDist += delta * 1.0f / 10.0f;
    }
}

void EntitySystemRenderer::draw() {

    mCameraState.modelView = mCameraState.worldFrame.getViewMatrix();
    mCameraState.cameraMatrix = mCameraState.projection * mCameraState.modelView;

    if (mDofRenderInfo.enabled) {
        float clearColorFactor =
            (float) mDofRenderInfo.adjustedCameraFrames.size();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(mClearColor[0] , mClearColor[1] , mClearColor[2], 0.0f);
        // high res part
        {
            glViewport(0, 0, mDofRenderInfo.xresHigh, mDofRenderInfo.yresHigh);
            glBindFramebuffer(GL_FRAMEBUFFER, mDofRenderInfo.highresFbo);
            doRenderPass(mWorldRenderStates, mCameraState.projection *
                         (mDofRenderInfo.adjustedCameraFrames[0]).getViewMatrix(),
                         true /* reset color and depth */);

            accumulateDofFbo(mDofRenderInfo.highresTexture);
        }
        // blurred samples
        for (uint32_t i = 1; i < mDofRenderInfo.adjustedCameraFrames.size(); i++) {
            glViewport(0, 0, mDofRenderInfo.xres, mDofRenderInfo.yres);
            glBindFramebuffer(GL_FRAMEBUFFER, mDofRenderInfo.fbo);
            doRenderPass(mWorldRenderStates, mCameraState.projection *
                    (mDofRenderInfo.adjustedCameraFrames[i]).getViewMatrix(),
                    true /* reset color and depth */);

            accumulateDofFbo(mDofRenderInfo.texture);
        }
    } else {
        glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        doRenderPass(mWorldRenderStates, mCameraState.cameraMatrix);
    }

    doRenderPass(mUiRenderStates, mCameraState.cameraMatrix);

    // Debug drawing stuff below
	if (!mDebugDrawOBBS) return;

	GL32RenderState obbState = {
		false, // disable depth test
		mColliderProgramId,
		mColliderVAO,
		mColliderVBO,
		0,
		36,
		(GLint)mColliderMatrixLoc,
		identity4(),
		nullptr,
		0
	};

	updateRenderState(obbState);

    // Debug draw collision OBBs
	const Entity& currentCamera =
		mSys->getEntityById(mSys->mCurrentCamera);
    std::vector<float> obbPos(sizeof(float) * 3 * 8);
    for (int i = 0; i < mSys->numActiveDynamicColliders; i++) {
        Entity& col = mSys->mEntities[mSys->mDynamicColliders[i]];

        getOBBVertices_inplace(col.collision_info.bbox_current, &obbPos[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 8, &obbPos[0]);

        glUniformMatrix4fv(mColliderMatrixLoc, 1, GL_FALSE, (currentCamera.currentCameraMatrix().vals));
        if (mSys->mCollisionResults[mSys->mDynamicColliders[i]]) {
            glUniform3f(mColliderColorLoc, 0.7, 0.2, 0.2);
        } else {
            glUniform3f(mColliderColorLoc, 0.2, 0.7, 0.4);
        }
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

}

void EntitySystemRenderer::doRenderPass(const std::vector<GL32RenderState>& renderStates,
                                        const matrix4& cameraMatrix, bool resetDepth) {
    if (resetDepth) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	int i = 0;
	for (const auto& renderState : renderStates) {
		i++;
		updateRenderState(renderState);
		glUniformMatrix4fv(renderState.worldMatrixLoc, 1, GL_FALSE,
			(cameraMatrix * renderState.worldMatrix).vals);
		if (renderState.vboData) {
			glBufferSubData(GL_ARRAY_BUFFER, 0, renderState.vboDataBytes, renderState.vboData);
		}
		glDrawElements(GL_TRIANGLES, renderState.numVerts, GL_UNSIGNED_INT, 0);
	}
}

} // namespace LEL
