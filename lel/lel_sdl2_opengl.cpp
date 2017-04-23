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

#include "lel_sdl2_opengl.h"

#include <stdio.h>
#include <string.h>

namespace LEL {

GLuint createShaderProgram(
        const char* vshader_src,
        const char* fshader_src,
        GLuint* vshader_out,
        GLuint* fshader_out) {

    GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);

    GLint vshaderLen = (GLint)(strlen(vshader_src) + 1);
    GLint fshaderLen = (GLint)(strlen(fshader_src) + 1);

    glShaderSource(vsh, 1, &vshader_src, &vshaderLen);
    glShaderSource(fsh, 1, &fshader_src, &fshaderLen);

    glCompileShader(vsh);
    glCompileShader(fsh);

#define INFOLOG_MAX_LEN 2048
    char infologbuf[INFOLOG_MAX_LEN];

    glGetShaderInfoLog(vsh, INFOLOG_MAX_LEN, nullptr, infologbuf);
    fprintf(stderr, "%s: INFOLOG vsh %s\n", __func__, infologbuf);
    glGetShaderInfoLog(fsh, INFOLOG_MAX_LEN, nullptr, infologbuf);
    fprintf(stderr, "%s: INFOLOG fsh %s\n", __func__, infologbuf);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vsh);
    glAttachShader(prog, fsh);
    glLinkProgram(prog);

    GLint linkstatus;
    glGetProgramiv(prog, GL_LINK_STATUS, &linkstatus);
    glGetProgramInfoLog(prog, INFOLOG_MAX_LEN, nullptr, infologbuf);
    fprintf(stderr, "%s: INFOLOG prog %s\n", __func__, infologbuf);

    fprintf(stderr, "%s: linkstatus %d\n", __func__, linkstatus);

    if (vshader_out) *vshader_out = vsh;
    if (vshader_out) *fshader_out = fsh;
    return prog;
}

} // namespace LEL
