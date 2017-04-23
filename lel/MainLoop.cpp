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

#include "MainLoop.h"

#include "Audio.h"
#include "EntitySystemControl.h"

#include "lel.h"
#include "lel_sdl2_opengl.h"

#include <algorithm>

namespace LEL {

#define MAX_CONTROLLERS 4

int MainLoop::run() {
    SDL_Event e;
    bool quit = false;

    setupGameControllers();
    int maxNumControllers = getMaxNumControllers();
    GameControllerState* controllerStates = getGameControllerStates();

    start_timestamps();
    while (!quit) {

        // Input
        
        // Game controller / joystick handling
        pollGameControllerState();
        for (int i = 0; i < maxNumControllers; i++) {
            GameControllerState* curr = controllerStates + i;
            mSystem->mControl->update(curr);
            for (const auto& it : mControllerHandlers) {
                it.second(curr);
            }
        }
        
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            GameControllerState* kbm = getKBMState();
            
            // Keyboard handling
            if (e.type == SDL_KEYDOWN ||
                e.type == SDL_KEYUP) {
                if (e.type == SDL_KEYDOWN) {
                    kbm->keyDowns.push_back((uint32_t)(e.key.keysym.sym));
                }
                if (e.type == SDL_KEYUP) {
                    kbm->keyDowns.erase(
                        std::remove_if(
                            kbm->keyDowns.begin(), kbm->keyDowns.end(),
                            [&e](uint32_t code) { return (uint32_t)(e.key.keysym.sym) == code; }),
                            kbm->keyDowns.end());
                }
                mSystem->mControl->update(kbm);
                for (const auto& it : mKeyboardHandlers) {
                    it.second(e.type == SDL_KEYDOWN, e.key.keysym.sym);
                }
            }
            // Mouse handling
            if (e.type == SDL_MOUSEBUTTONDOWN ||
                e.type == SDL_MOUSEBUTTONUP) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    kbm->mouseDowns.push_back((uint32_t)(e.button.button));
                }
                if (e.type == SDL_MOUSEBUTTONUP) {
                    kbm->mouseDowns.erase(
                        std::remove_if(
                            kbm->mouseDowns.begin(), kbm->mouseDowns.end(),
                            [&e](uint32_t code) { return (uint32_t)(e.button.button) == code; }),
                            kbm->mouseDowns.end());
                }
                mSystem->mControl->update(kbm);
            }
            
            if (e.type == SDL_MOUSEMOTION) {
                kbm->mouseX = e.motion.x;
                kbm->mouseY = e.motion.y;
                kbm->mousedX = e.motion.xrel;
                kbm->mousedY = e.motion.yrel;
                mSystem->mControl->update(kbm);
            }
        }

        // Update state
        mSystem->updateState();
        mRenderer->preDraw();
        mRenderer->draw();

        // Draw callbacks
        for (const auto& it : mDraws) {
            it.second();
        }

        mWindow->post();
		Audio::get()->update();
    }

    cleanupGameControllers();

    return 0;
}

} // namespace LEL
