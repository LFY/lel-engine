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

// reference frame class:
// position / fwd / up

#include "matrix.h"

namespace LEL {

class RefFrame {
public:
    RefFrame() : pos(makevector4(0,0,0,1)),
                 fwd(makevector4(1,0,0,0)),
                 up(makevector4(0,0,1,0)) {}
   
	RefFrame(const vector4& _pos,
		     const vector4& _fwd,
		     const vector4& _up) :
		pos(_pos), fwd(v4normed(_fwd)), up(v4normed(_up)),
		mInitialPointing(v4normed(v4normed(fwd))) { }

	RefFrame(
            float px, float py, float pz,
            float fx, float fy, float fz,
            float ux, float uy, float uz) :
        RefFrame(
                makevector4(px, py, pz, 1.0f),
                makevector4(fx, fy, fz, 0.0f),
                makevector4(ux, uy, uz, 1.0f)) { }

    static RefFrame identity() {
        return RefFrame(makevector4(0,0,0,1),
                        makevector4(0,0,-1,0),
                        makevector4(0,1,0,0));
    }
    
    void updatePos(const vector4& _pos) {
        pos = _pos;
    }

    void updateAxes(const vector4& _fwd,
                    const vector4& _up) {
        fwd = _fwd;
        up = _up;
    }

    matrix4 getMatrix() const {
        return makeFrameChange(pos, v4normed(fwd), v4normed(up)) * scaling(scale.x, scale.y, scale.z);
    }

    RefFrame getInverseFrame() const {

        matrix4 invPose = makeModelview(pos, v4normed(fwd), v4normed(up));
        RefFrame res;
        res.pos = mcol(invPose, 3);
        // res.pos = -1.0f * pos;
        res.fwd = -1.0f * v4normed(mcol(invPose, 2));
        res.up = v4normed(mcol(invPose, 1));
        res.scale = makevector4(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z, 0.0f);
        return res;
    }

    RefFrame preapplyFrame(const RefFrame& other) const {
        matrix4 combined = getMatrix() * other.getMatrix();
        RefFrame res;
        res.pos = combined * makevector4(0,0,0,1);
        res.scale.x = scale.x * other.scale.x;
        res.scale.y = scale.y * other.scale.y;
        res.scale.z = scale.z * other.scale.z;
        res.fwd = -1.0f * v4normed(mcol(combined, 2));
        res.up = v4normed(mcol(combined, 1));
        return res;
    }

	matrix4 getViewMatrix() const {
		return makeModelview(pos, fwd, up);
	}

	vector4 getInitialPointing() const {
		return mInitialPointing;
	}

    // position, rotation, scale    
    vector4 pos; vector4 fwd; vector4 up;
    vector4 scale = { 1.0f, 1.0f, 1.0f, 0.0f };

    // current velocity
    vector4 velocity = { 0.0f, 0.0f, 0.0f, 0.0f };

    // accelerations (not affected by setting velocity)
    // translational accel
    vector4 accel = { 0.0f, 0.0f, 0.0f, 0.0f };
    // rotational accelerations
    float rollAccelCW = 0.0f; // axis = fwd
    float yawAcceLeft = 0.0f; // axis = up
    float pitchAccelUp = 0.0f; // axis = fwd X up (right)

	// These are public-facing settings and do not directly
	// affect the pos/Fwd/Up. They are meant to be an absolute ref.
	// Also this isn't going to work for full 3d rotations.
    // More meant for FPS-style controls.
	float yaw = 0.0f;
	float pitch = 0.0f;
	float roll = 0.0f;

private:
	// We can make it easier to compute absolute rotations if we keep this.
	vector4 mInitialPointing;
};

};
