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
#include "GameControllers.h"

namespace LEL {

typedef void (*EntityControlFunc)(GameControllerState* gc, Entity* e);

// Purpose of this class is to issue the correct actions
// to the |mSys| depending on what input came from player.
class EntitySystemControl {
public:
    EntitySystemControl(EntitySystem* sys) :
        mSys(sys) {
        updateControlledEntity();
        setDefaultKBMBinds();
    }

    void setDefaultKBMBinds();

    void setControlledEntity(entity_handle_t e) {
        mControlledEntity = e;
    }

    void updateControlledEntity();
    void update(GameControllerState* gc);

    uint32_t m_fwdTrKeybind = 0;
    uint32_t m_backTrKeybind = 0;
    uint32_t m_leftTrKeybind = 0;
    uint32_t m_rightTrKeybind = 0;

    uint32_t m_resetSysKeybind = 0;

private:
    entity_handle_t mControlledEntity;
    EntitySystem* mSys = nullptr;

    // Mapping of input primitives to actions
    // (can update from options menu, hopefully)
    //
    // Actions:
    // WantedPoseUpdate,
    // which consists of translation / rotation matrices.
    // Possibly restricted, damped or rescaled by Entity,
    // physics model, or collision system.
    // Entity / EntitySystem has final say
    // on the final update to its RefFrame.
};
} // namespace LEL
