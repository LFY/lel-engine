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

#include <vector>

namespace LEL {

struct GameControllerState {
    bool attached = false;
    // dpad
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;

    // buttons
    bool a = false;
    bool b = false;
    bool x = false;
    bool y = false;

    // start/back
    bool start = false;
    bool back = false;

    // shoulder
    bool leftshoulder = false;
    bool rightshoulder = false;

    // digital sticks
    bool lstick = false;
    bool rstick = false;

    // triggers
    float ltrig = 0.0f;
    float rtrig = 0.0f;

    // analog sticks
    float lstickX = 0.0f;
    float lstickY = 0.0f;
    float rstickX = 0.0f;
    float rstickY = 0.0f;
	float rstickZ = 0.0f;

    // keyboard
    std::vector<uint32_t> keyDowns = {};
    // mouse
    std::vector<uint32_t> mouseDowns = {};
    int32_t mouseX = 0;
    int32_t mouseY = 0;
    int32_t mousedX = 0;
    int32_t mousedY = 0;
    
};

void setupGameControllers();
void cleanupGameControllers();
void pollGameControllerState();

int getMaxNumControllers();
GameControllerState* getGameControllerStates();
GameControllerState* getKBMState();

} // namespace LEL

