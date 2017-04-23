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

#include "GameControllers.h"

#include <string.h>
#include <stdio.h>

namespace LEL {

#define MAX_CONTROLLERS 4

static SDL_GameController* sControllers[MAX_CONTROLLERS];
static SDL_Joystick* sJoysticks[MAX_CONTROLLERS];

static GameControllerState sControllerStates[MAX_CONTROLLERS];
static int sMaxControllers = 0;
void setupGameControllers();
void cleanupGameControllers();

static GameControllerState sKBMState;

void setupGameControllers() {
    sMaxControllers = SDL_NumJoysticks();
    for (int ji = 0; ji < sMaxControllers; ji++) {
		if (SDL_IsGameController(ji)) {
			sControllers[ji] = SDL_GameControllerOpen(ji);
		}
		else {
			// is a generic joystick device.
			sJoysticks[ji] = SDL_JoystickOpen(ji);
			if (sJoysticks[ji]) {
				fprintf(stderr, "%s: Opened joystick %d\n", __func__, ji);
				fprintf(stderr, "%s: Name: %s\n", __func__, SDL_JoystickNameForIndex(ji));
				fprintf(stderr, "%s: # axes: %d\n", __func__, SDL_JoystickNumAxes(sJoysticks[ji]));
				fprintf(stderr, "%s: # buttons: %d\n", __func__, SDL_JoystickNumButtons(sJoysticks[ji]));
				fprintf(stderr, "%s: # balls: %d\n", __func__, SDL_JoystickNumBalls(sJoysticks[ji]));
			}

		}

        if (ji >= 4) break;

    }
}

void cleanupGameControllers() {
    for (int ji = 0; ji < sMaxControllers; ji++) {
		if (ji >= 4) break;
		if (SDL_IsGameController(ji)) {
			if (sControllers[ji]) SDL_GameControllerClose(sControllers[ji]);
		} else {
			if (sJoysticks[ji]) SDL_JoystickClose(sJoysticks[ji]);
		}
    }
}

int getMaxNumControllers() {
    return MAX_CONTROLLERS;
}

GameControllerState* getGameControllerStates() {
    return sControllerStates;
}

GameControllerState* getKBMState() {
    return &sKBMState;
}

void pollGameControllerState() {
    // Handle input
    for (int i = 0; i < sMaxControllers; i++) {
        GameControllerState* curr = sControllerStates + i;

        if (sControllers[i] && SDL_GameControllerGetAttached(sControllers[i])) {

            curr->attached = true;

            curr->up = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_DPAD_UP);
            curr->down = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            curr->left = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            curr->right = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

            curr->a = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_A);
            curr->b = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_B);
            curr->x = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_X);
            curr->y = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_Y);

            curr->start = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_START);
            curr->back = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_BACK);

            curr->leftshoulder = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            curr->rightshoulder = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

            curr->lstick = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_LEFTSTICK);
            curr->rstick = SDL_GameControllerGetButton(sControllers[i], SDL_CONTROLLER_BUTTON_RIGHTSTICK);

            int16_t ltrig = SDL_GameControllerGetAxis(sControllers[i], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
            int16_t rtrig = SDL_GameControllerGetAxis(sControllers[i], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

            int16_t lstickX = SDL_GameControllerGetAxis(sControllers[i], SDL_CONTROLLER_AXIS_LEFTX);
            int16_t lstickY = SDL_GameControllerGetAxis(sControllers[i], SDL_CONTROLLER_AXIS_LEFTY);

            int16_t rstickX = SDL_GameControllerGetAxis(sControllers[i], SDL_CONTROLLER_AXIS_RIGHTX);
            int16_t rstickY = SDL_GameControllerGetAxis(sControllers[i], SDL_CONTROLLER_AXIS_RIGHTY);

            curr->ltrig = ltrig / 32767.0;
            curr->rtrig = rtrig / 32767.0;

            curr->lstickX = lstickX / 32767.0;
            curr->lstickY = lstickY / 32767.0;

            curr->rstickX = rstickX / 32767.0;
            curr->rstickY = rstickY / 32767.0;

            break;
        } else if (sJoysticks[i] && SDL_JoystickGetAttached(sJoysticks[i])) {
            
			curr->attached = true;

			int16_t rstickX = SDL_JoystickGetAxis(sJoysticks[i], 0);
			int16_t rstickY = SDL_JoystickGetAxis(sJoysticks[i], 1);
			curr->rstickX = rstickX / 32767.0;
			curr->rstickY = -rstickY / 32767.0;

			int16_t rawThrottleAxis = SDL_JoystickGetAxis(sJoysticks[i], 2);
			curr->rtrig = rawThrottleAxis < 0 ? (-rawThrottleAxis / 32767.0) : 0.0f;
			curr->ltrig = rawThrottleAxis > 0 ? (rawThrottleAxis / 32767.0) : 0.0f;
			curr->rstickZ = SDL_JoystickGetAxis(sJoysticks[i], 4) / 32767.0;

            curr->a = SDL_JoystickGetButton(sJoysticks[i], 0);
            curr->b = SDL_JoystickGetButton(sJoysticks[i], 2);

        } else { curr->attached = false; }
    }
}

} // namespace LEL
