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

#include "lel.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>

namespace LEL {

uint64_t loadFile(const char* filename, char** out) {
    FILE* fh = NULL;
#ifdef _WIN32
        fopen_s(&fh, filename, "rb");
#else
        fh = fopen(filename, "rb");
#endif
    if (!fh) { return 0; }

    fseek(fh, 0, SEEK_END);
    uint64_t s = ftell(fh);
    rewind(fh);

    char* buf = (char*)malloc(s + 1);
	if (!buf) abort();
    buf[s] = '\0';
    
    fread(buf, s, 1, fh);
    fclose(fh); fh = NULL;
    *out = buf;

    return s;
}

#define MAX_PATH_LEN 4096

std::string currExecutablePath() {
    std::string res;

    char buf[MAX_PATH_LEN];
#ifdef _WIN32

    HMODULE hModule = GetModuleHandle(NULL);
    if (hModule) {
        GetModuleFileName(hModule, buf, sizeof(buf));
        res = buf;
    }
#endif

#ifdef __APPLE__
    uint32_t sz = MAX_PATH_LEN;
    int ret = _NSGetExecutablePath(buf, &sz);
    if (ret) {
        char* buf2 = (char*)malloc(sz);
        _NSGetExecutablePath(buf2, &sz);
        res = buf2;
        free(buf2);
    } else  {
        res = buf;
    }
#endif

    return res;
}

std::string pathCat(const std::string& a, const std::string& b) {
#ifdef _WIN32
    std::string delim = "\\";
#else
    std::string delim = "/";
#endif

    return a + delim + b;
}

std::string pathPop(const std::string& p) {
#ifdef _WIN32
    std::string delim = "\\";
#else
    std::string delim = "/";
#endif

    size_t delimPos = p.find_last_of(delim);
    if (delimPos == std::string::npos) return p;

    return p.substr(0, delimPos - 1);
}

std::string currExecutableDir() {
    return pathPop(currExecutablePath());
}

std::vector<std::string> readLines(const char* filename) {
    char* buf = nullptr;
    loadFile(filename, &buf);

    std::string str(buf);
    free(buf);

    std::vector<std::string> lines;
    const size_t kNull = std::string::npos;

    size_t currStart = 0;
    size_t linePos = str.find("\n");
    bool hasLine = linePos != kNull;

    while (hasLine) {
        lines.push_back(str.substr(currStart, linePos - currStart));
        currStart = linePos + 1;
        linePos = str.find("\n", currStart);
        hasLine = linePos != kNull;
    }

    lines.push_back(str.substr(currStart, kNull));

    return lines;
}

std::vector<std::string> splitBy(const std::string& str, const std::string& delim) {
    std::vector<std::string> elts;
    const size_t kNull = std::string::npos;

    size_t currStart = 0;
    size_t pos = str.find(delim);
    bool has = pos != kNull;

    while (has) {
        elts.push_back(str.substr(currStart, pos - currStart));
        currStart = pos + delim.length();
        pos = str.find(delim, currStart);
        has = pos != kNull;
    }

    // get the last one if currStart is not the last
    if (currStart != str.length() - 1)
        elts.push_back(str.substr(currStart, kNull));

    return elts;
}

static std::chrono::high_resolution_clock::time_point start_time_point;
bool start_time_init = false;

void start_timestamps() {
	start_time_point = std::chrono::high_resolution_clock::now();
	start_time_init = true;
}

uint64_t curr_time_us() {
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now() -
		start_time_point).count();
}

} // namespace LEL
