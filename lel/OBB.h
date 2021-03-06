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

#include "matrix.h"

#include <array>
#include <string>
#include <vector>

// Oriented bounding boxes---specification, intersection tests

namespace LEL {

struct OBB {
    vector4 origin;
    vector4 axes[3];
    float dims[3];
};

OBB makeOBB(
        float ox, float oy, float oz,
        float ax, float ay, float az,
        float bx, float by, float bz,
        float cx, float cy, float cz,
        float sx, float sy, float sz);

OBB makeOBB(
	const vector4& origin,
	const vector4& a,
	const vector4& b,
	const vector4& c,
	const vector4& s);

std::string dumpOBB(const OBB& a);

interval2 obb_extents_along_ray(const std::array<vector4, 8>& points,
                                const rayv4& r);
bool intersectOBB(const OBB& a, const OBB& b);
bool intersectOBB(const rayv4& a, const OBB& b, float* tOut);

OBB transformedOBB(const OBB& obb,
                   const matrix4& tr);

std::vector<float> getOBBVertices(const OBB& obb);
void getOBBVertices_inplace(const OBB& obb, float* posVector);

} // namespace LEL
