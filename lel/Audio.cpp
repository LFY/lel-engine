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

#ifdef __APPLE__
#include <GLEW/glew.h>
#include <SDL2/SDL.h>
#include <SDL2_mixer/SDL_mixer.h>
#else
#include <gl\glew.h>
#include <gl\glu.h>
#include <SDL.h>
#include <SDL_mixer.h>
#endif

#include "Audio.h"

#include "lel.h"

namespace LEL {

static Audio* sAudio = nullptr;

// static
Audio* Audio::get() {
    if (!sAudio) sAudio = new Audio();
    return sAudio;
}

Audio::Audio() {
    int result = Mix_Init(MIX_INIT_OGG);
    if (result != MIX_INIT_OGG) {
        fprintf(stderr, "%s: could not initialize mp3 player (0x%x) err %s\n",
                __func__, result, Mix_GetError());
    }
    Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 640);
}

void Audio::playMusic(const std::string& asset_dir,
                      const std::string& basename) {
    if (currMusic) {
        Mix_FreeMusic((Mix_Music*)currMusic);
    }
    Mix_Music* to_play = Mix_LoadMUS(pathCat(asset_dir, basename + ".ogg").c_str());
	fprintf(stderr, "%s: to_play: %p err %s\n", __func__, to_play, Mix_GetError());
    Mix_PlayMusic(to_play, 1);
    currMusic = to_play;
}

} // namespace LEL
