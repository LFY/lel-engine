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

#include "Entity.h"

#include <functional>
#include <string>
#include <unordered_map>

namespace LEL {

class EntityAction {
public:
using ActionFunc = void (*)(void*, void*, void*, void*, void*, void*, void*, void*);
    EntityAction() {
        args.resize(8, nullptr);
    }
    virtual void doAction() {
        action(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
    }

    ActionFunc action = nullptr;
    std::vector<void*> args;
};

// Some building-block actions.
class WantedPoseUpdate : public EntityAction {
public:
    virtual void doAction() override {
        action(args[0], args[1], &translation, &pitchYaw, &roll,
               nullptr, nullptr, nullptr);
    }

    // this needs to live in the action for
    // thread safety / safe destruction
    matrix4 translation;
    matrix4 pitchYaw;
    matrix4 roll;
};

class SetPose : public EntityAction {
public:
    SetPose() { }
            
    virtual void doAction() override {
        action(args[0], &eid, &pos, &fwd, &up, &scale, nullptr, nullptr);
    }

    uint32_t eid;
    vector4 pos;
    vector4 fwd;
    vector4 up;
    vector4 scale;
};

} // namespace LEL
