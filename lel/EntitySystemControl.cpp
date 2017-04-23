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

#include "EntityAction.h"
#include "EntitySystemControl.h"

#include "lel_sdl2_opengl.h"

namespace LEL {

static void sEnqueueWantedPoseUpdate(
        EntitySystemControl* control,
        EntitySystem* sys, entity_handle_t e, GameControllerState* gc);
static void sHandleWantedSystemAction(
        EntitySystemControl* control,
        EntitySystem* sys, entity_handle_t e, GameControllerState* gc);
static void sGetWantedPoseUpdateMatrices(
                EntitySystemControl* control,
                GameControllerState* gc,
                Entity* e,
                matrix4* translation_out,
                matrix4* pitchyaw_out,
                matrix4* roll_out);
static void sHandleWantedPoseUpdate(
        Entity* e, GameControllerState* gc,
        matrix4* tr, matrix4* pitchyaw, matrix4* roll);

void EntitySystemControl::setDefaultKBMBinds() {
    m_fwdTrKeybind = SDLK_w;
    m_backTrKeybind = SDLK_s;
    m_leftTrKeybind = SDLK_a;
    m_rightTrKeybind = SDLK_d;

    m_resetSysKeybind = SDLK_BACKSPACE;
}

void EntitySystemControl::updateControlledEntity() {
    setControlledEntity(mSys->mControlInfo.entity);
}

void EntitySystemControl::update(GameControllerState* gc) {
    sEnqueueWantedPoseUpdate(this, mSys, mControlledEntity, gc);
	sHandleWantedSystemAction(this, mSys, mControlledEntity, gc);
}

static void sGetWantedPoseUpdateMatrices(
                EntitySystemControl* control,
                GameControllerState* gc,
                Entity* e,
                matrix4* translation_out,
                matrix4* pitchyaw_out,
                matrix4* roll_out) {

	RefFrame& frame = e->frame;

	vector4& pointing = frame.fwd;
	vector4& up = frame.up;
    vector4 right = v4normed(v4cross(pointing, up));

	float controlScale = 0.1f;
	float deadzone = controlScale * 0.1f;

    *translation_out = identity4();
    *pitchyaw_out = identity4();
    *roll_out = identity4();

    // keyboard / mousehandling part; takes priority over game controller
    bool kbm = false;
    for (const auto& elt: gc->keyDowns) {
        if (elt == control->m_fwdTrKeybind) {
            *translation_out =
                translation(controlScale * pointing);
            kbm = true;
        }
        if (elt == control->m_backTrKeybind) {
            *translation_out =
                translation(-controlScale * pointing);
            kbm = true;
        }
        if (elt == control->m_leftTrKeybind) {
            *translation_out =
                translation(-controlScale * right);
            kbm = true;
        }
        if (elt == control->m_rightTrKeybind) {
            *translation_out =
                translation(controlScale * right);
            kbm = true;
        }
    }

    if (gc->mouseDowns.size() &&
        (gc->mousedX || gc->mousedY)) {
        kbm = true;

        float mousedX_scaled = gc->mousedX * 0.01;
        float mousedY_scaled = -gc->mousedY * 0.01;

        matrix4 rotR = rotation(right.x, right.y, right.z, mousedY_scaled); // pitch
        matrix4 rotU = rotation(up.x, up.y, up.z, -mousedX_scaled * 0.707); // yaw
        matrix4 pitchYaw = rotU * rotR;
        matrix4 roll = rotation(pointing.x, pointing.y, pointing.z, mousedX_scaled * 0.707); // roll

        *pitchyaw_out = pitchYaw;
        *roll_out = roll;
    }

    if (kbm) return;

    // Game controller part (overriden by kb/m)
	float lt = controlScale * gc->ltrig;
	float rt = controlScale * gc->rtrig;
	float lX = controlScale * gc->lstickX;
	float lY = controlScale * gc->lstickY;
	float rX = controlScale * gc->rstickX;
	float rY = controlScale * gc->rstickY;

	vector4 lxyVec = makevector4(lX, lY, 0.0f, 0.0f);
	vector4 rxyVec = makevector4(rX, rY, 0.0f, 0.0f);
    float lxyOffset = v4len(lxyVec);
    float lxyLive = lxyOffset - deadzone;
	float rxyOffset = v4len(rxyVec);
	float rxyLive = rxyOffset - deadzone;

    if (lxyLive > 0) {
        lxyVec = v4stretch(lxyVec, lxyLive);
	} else {
		lxyVec.x = 0.0f; lxyVec.y = 0.0f;
	}

    if (rxyLive > 0) {
        rxyVec = v4stretch(rxyVec, rxyLive);
	} else {
		rxyVec.x = 0.0f; rxyVec.y = 0.0f;
	}

    matrix4 transTotal = translation(lxyVec.x * right) *
                         translation(-lxyVec.y * up) *
                         translation((rt - lt) * pointing);
    matrix4 rotR = rotation(right.x, right.y, right.z, rxyVec.y); // pitch
    matrix4 rotU = gc->rstick ? identity4() : rotation(up.x, up.y, up.z, -rxyVec.x * 0.707); // yaw
    matrix4 pitchYaw = rotU * rotR;
    matrix4 roll = rotation(pointing.x, pointing.y, pointing.z, rxyVec.x * 0.707); // roll

    *translation_out = transTotal;
    *pitchyaw_out = pitchYaw;
    *roll_out = roll;
}

static void sHandleWantedSystemAction(
        EntitySystemControl* control,
        EntitySystem* sys, entity_handle_t e, GameControllerState* gc) {
    for (const auto& elt: gc->keyDowns) {
        if (elt == control->m_resetSysKeybind) {
            fprintf(stderr, "%s: should reset system!\n", __func__);
			sys->reloadFromFile("level1");
        }
    }
}

static void sHandleWantedPoseUpdate(
        Entity* e, GameControllerState* gc,
        matrix4* tr, matrix4* pitchyaw, matrix4* roll) {
    e->handleWantedPoseUpdate(gc, tr, pitchyaw, roll);
}

static void sEnqueueWantedPoseUpdate(
        EntitySystemControl* control,
        EntitySystem* sys,
        entity_handle_t e,
        GameControllerState* gc) {
    WantedPoseUpdate* act = new WantedPoseUpdate;
    Entity* entity = &sys->mEntities[e];
    sGetWantedPoseUpdateMatrices(
            control, gc, entity,
            &act->translation,
            &act->pitchYaw,
            &act->roll);
    act->action =
        (EntityAction::ActionFunc)sHandleWantedPoseUpdate;
    act->args[0] = entity;
    act->args[1] = gc;

    sys->enqueueAction(0, (EntityAction*)act); // 0: run immediately
}

} // namespace LEL
