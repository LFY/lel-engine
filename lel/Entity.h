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

#include "Collider.h"
#include "GameControllers.h"
#include "RenderModel.h"
#include "TextDisplay.h"
#include "RefFrame.h"

#include "matrix.h"

#include <unordered_map>

typedef uint32_t entity_handle_t;

namespace LEL {

class Entity {
public:
    Entity() = default;
    Entity(RenderModel* model);
	virtual void update();
    void deriveProps();
	void draw() const;    

    void resetPos(const vector4& v);
    void resetFwd(const vector4& v);
    void resetUp(const vector4& v);
    void resetPos(float x, float y, float z);
    void resetFwd(float x, float y, float z);
    void resetUp(float x, float y, float z);

	void refreshTransform();
	void setTransform(const matrix4& tr);
    void setTransformParent(entity_handle_t e);
    void changeToParentCoords();

    // Camera-related methods
    void setProjection(float fov, float aspect, float nearclip, float farclip);
    matrix4 getModelview() const;
	matrix4 getProjection() const;
    matrix4 currentCameraMatrix() const;
    matrix4 currentWorldMatrix() const;
    RefFrame getWorldFrame() const;
    RefFrame getParentWorldFrame() const;
    RefFrame getParentRotationFrame() const;

    RenderModel* getRenderModel();
    TextDisplay* getTextDisplay();
    void addTextDisplay();

    // Large pieces of data:
	RefFrame frame; // reference frame
    Collider collision_info; // collision info

	// Basic state updatey stuff for testing
	// Time to live
	float ttl = -1.0f; 
	bool live = true;
	bool hasCollision = false;

    // Input handlers
    virtual void handleWantedPoseUpdate(GameControllerState* gc, matrix4* tr, matrix4* pitchYaw, matrix4* roll);

    // Transform hierarchy
    entity_handle_t transformParent = -1;
    bool needUpdateTransformFromParent = true;

    // Properties
    std::unordered_map<std::string, int> intProps;
    std::unordered_map<std::string, float> floatProps;
    std::unordered_map<std::string, std::string> stringProps;

	vector4 lastPos;
	vector4 estVelocity;
	float mass = 10.0f;
	float damping = 0.98f;

    // Direct gamecontroller state
    struct DiscreteButtonState {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;

        bool a = false;
        bool b = false;
        bool x = false;
        bool y = false;
    };

    DiscreteButtonState discreteButtons;

    // Derived props
    bool isMenuSelector = false;
    int discreteX = 0;
    int discreteY = 0;
	int discreteButtonA = 0;
	int discreteButtonB = 0;
	int discreteButtonX = 0;
	int discreteButtonY = 0;

    // Other stuff
    bool passthroughRays = false;
    bool isUi = false;
private:
    bool frameDirty = true;

	vector4 wantedAccel = { 0.0f, 0.0f, 0.0f, 0.0f };
	vector4 accVelocity = { 0.0f, 0.0f, 0.0f, 0.0f };

    matrix4 mTransform;
    matrix4 mProjection;
    RenderModel* mRenderModel = nullptr;
    TextDisplay* mTextDisplay = nullptr;
};

} // namespace LEL
