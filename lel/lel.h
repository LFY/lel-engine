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

#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <vector>

#define DEBUG_LEL 0

#if DEBUG_LEL

#ifdef _WIN32

#define DLOG(...) do { \
    FILE* fh = NULL; \
    fopen_s(&fh, "dlog.txt", "a"); \
    fprintf(fh, __VA_ARGS__); fclose(fh); \
} while(0) \

#else

#define DLOG(...) do { \
FILE* fh = NULL; \
fh = fopen("dlog.txt", "a"); \
fprintf(fh, __VA_ARGS__); fclose(fh); \
} while(0) \

#endif

#else
#define DLOG(...)
#endif

namespace LEL {
    
uint64_t loadFile(const char* filename, char** out);
std::vector<std::string> readLines(const char* filename);
std::vector<std::string> splitBy(const std::string& str, const std::string& delim);

std::string pathCat(const std::string& a, const std::string& b);
std::string pathPop(const std::string& p);
 
std::string currExecutablePath();
std::string currExecutableDir();

void start_timestamps();
uint64_t curr_time_us();

} // namespace LEL
