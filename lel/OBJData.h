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

#include <string>
#include <vector>
#include <map>

#include "OBB.h"

namespace LEL {

class OBJData {
public:
	OBJData(const std::string& filename,
		    const std::string& diffuseTextureName = "");

	struct OBJVertex {
		float pos[3];
		float norm[3];
		float texcoord[2];
	};

	struct VertexDataKey {
		unsigned int pos;
		unsigned int norm;
		unsigned int texcoord;
	};

	struct VertexDataKeyCompare {
		bool operator()(const VertexDataKey& a, const VertexDataKey& b) const {
			if (a.pos != b.pos)
				return a.pos < b.pos;
			if (a.norm != b.norm)
				return a.norm < b.norm;
			if (a.texcoord != b.texcoord)
				return a.texcoord < b.texcoord;
			return false;
		}
	};

	std::vector<std::vector<float> > obj_v;
	std::vector<std::vector<float> > obj_vt;
	std::vector<std::vector<float> > obj_vn;
	std::vector<std::vector<uint32_t> > obj_f;

	std::map<VertexDataKey, OBJVertex, VertexDataKeyCompare> actualVertexData;
	std::map<VertexDataKey, uint32_t, VertexDataKeyCompare> actualIndexMap;

	std::vector<OBJVertex> vertexData;
	std::vector<uint32_t> indexData;

	std::string diffuseTextureName = "";
	unsigned int diffuseTextureWidth = 0;
	unsigned int diffuseTextureHeight = 0;

	std::vector<unsigned char> diffuseTextureRGBA;

	OBB coarseBounds;
};

} // namespace LEL
