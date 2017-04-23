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

#include "EntitySystem.h"
#include "GL32RenderState.h"

#include <vector>

namespace LEL {

// OpenGL 3.2 renderer

class EntitySystemRenderer {
public:
    EntitySystemRenderer(EntitySystem* sys,
                         uint32_t xres,
                         uint32_t yres) :
        mSys(sys), mXres(xres), mYres(yres) { }

    void initGL();
    void makeCurrent();
    
	void preDraw();
	void updateRenderState(const GL32RenderState& state);
    void draw();

	GL32RenderState currRenderState() const {
		return lastRenderState;
	}

private:
	void doRenderPass(const std::vector<GL32RenderState>& renderStates,
                      const matrix4& cameraMatrix, bool resetDepth = false);

    uint32_t mXres;
    uint32_t mYres;
    std::vector<float> mClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

	GL32RenderState lastRenderState;
	std::vector<GL32RenderState> mWorldRenderStates;
	std::vector<GL32RenderState> mUiRenderStates;

    GLuint mCurrProgramId = 0;
    EntitySystem* mSys;

    // Collision box rendering
    GLuint mColliderProgramId;
    GLuint mColliderMatrixLoc;
    GLuint mColliderColorLoc;
    GLuint mColliderVAO;
    GLuint mColliderVBO;
    GLuint mColliderIBO;

    // Font rendering
	GL32RenderState mTextDisplayRenderState;

	// Auxiliary state for effects
    struct CameraState {
        RefFrame worldFrame;
		matrix4 projection;
        vector4 focusTarget;
		float focalDist;
        uint32_t numUpdates;
        // to be updated later derived from above
		matrix4 modelView;
		matrix4 cameraMatrix;
    };
    CameraState mCameraState;

	// Draw collision OBBS
	bool mDebugDrawOBBS = false;

	// Effects : DOF
	struct DofRenderInfo {
        bool enabled = true;

        // constant params (or they should be)
        float radius = 0.2f;
		uint32_t numPoints = 4;

        float radius2 = 0.4f;
		uint32_t numPoints2 = 8;

        GLint xres = 640;
        GLint yres = 360;
		GLint xresHigh = 1280;
		GLint yresHigh = 720;

        // dynamically updated/initialized
        bool initialized = false;
        std::vector<RefFrame> adjustedCameraFrames;
        GLuint fbo;
        GLuint texture;
        GLuint rboDs;
        GLuint highresFbo;
        GLuint highresTexture;
		GLuint highresRboDs;
	};

    GLint mBrightnessUniformLoc;
    DofRenderInfo mDofRenderInfo;
    void initializeDofRenderer();
    void updateDofRenderer();
    void deinitializeDofRenderer();
    void accumulateDofFbo(GLuint tex);
    void updateCameraFocalDist();

    // Fullscreen quad drawing
    GL32RenderState mFullscreenQuadRenderState;
};

} // namespace LEL
