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

#include "Audio.h"
#include "Collider.h"
#include "EntitySystem.h"
#include "EntitySystemRenderer.h"
#include "GameControllers.h"
#include "MainLoop.h"
#include "matrix.h"
#include "OBB.h"
#include "RenderWindow.h"

#include "lel_sdl2_opengl.h"
#include "lel.h"

#include <unordered_map>
#include <vector>

#include <stdio.h>

static void run_tests();
int main(int argc, char* argv[]) {
	fprintf(stderr, "%s: welcome to the LEL game engine hehehehheheheh :}\n", __func__);

    if (argc == 2 && !strcmp(argv[1], "test")) {
        run_tests();
        return 0;
    }

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);

	LEL::RenderWindow window(1280, 720);

	LEL::start_timestamps();
	LEL::MainLoop mainLoop(&window);

	LEL::EntitySystem* system = LEL::parseEntitySystem("ludum_dare_38_assets", "mainmenu");
    LEL::EntitySystemRenderer* renderer = new LEL::EntitySystemRenderer(system, 1280, 720);

    LEL::Audio* audio = LEL::Audio::get();
    audio->playMusic("ludum_dare_38_assets", "track1");

    renderer->initGL();
    renderer->makeCurrent();

    mainLoop.setSystem(system, renderer);

	int mainLoopExit =
		mainLoop.run();

    return mainLoopExit;
}

void run_tests() {
    LEL::OBB a = LEL::makeOBB(
            -4.78497, -3.53994, -1.49341,
            0,-1,0,
            0,0,1,
            -1,0,0,
            2,2,2);

    LEL::OBB b = LEL::makeOBB(
            -5.91586, -4.1716, -2.22533,
            0,-1,0,
            0,0,1,
            -1,0,0,
            2,2,2);

    fprintf(stderr, "%s: intersects? %d\n", __func__, LEL::intersectOBB(a, b));

    LEL::RefFrame frame;
    frame.pos = LEL::makevector4(1,0,0,1);
    frame.fwd = LEL::makevector4(0,1,0,1);
    frame.up = LEL::makevector4(0,1,1,1);

    LEL::RefFrame inv = frame.getInverseFrame();
    fprintf(stderr, "%s: inverse frame: pos %s fwd %s up %s\n",
            __func__,
            dumpv4(inv.pos).c_str(),
            dumpv4(inv.fwd).c_str(),
            dumpv4(inv.up).c_str());

    fprintf(stderr, "%s: inverse tform: %s\n",
            __func__,
            dumpm4(frame.getMatrix() * inv.getMatrix()).c_str());

    LEL::RefFrame app = frame.preapplyFrame(inv);
    fprintf(stderr, "%s: invapplied frame: pos %s fwd %s up %s\n",
            __func__,
            dumpv4(app.pos).c_str(),
            dumpv4(app.fwd).c_str(),
            dumpv4(app.up).c_str());
}
