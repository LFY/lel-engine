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

#include "OBJData.h"

#include "lel.h"
#include "lodepng.h"

#include <stdio.h>

namespace LEL {
    
OBJData::OBJData(const std::string& filename,
	const std::string& _diffuseTextureName) :
	diffuseTextureName(_diffuseTextureName) {
        
    char* buf = nullptr;
    loadFile(filename.c_str(), &buf);

	std::string str(buf);
	if (buf) free(buf);
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

	for (const auto& line : lines) {
		size_t commentPos = line.find("#");
		if (commentPos != kNull) continue;

		size_t fstSpacePos = line.find(" ");
		if (fstSpacePos == kNull) continue;

		std::string tag = line.substr(0, fstSpacePos);

		float v0, v1, v2;
		unsigned int p0, t0, n0, p1, t1, n1, p2, t2, n2;
        
#ifndef _WIN32
        auto mysscanf = sscanf;
#else
        auto mysscanf = sscanf_s;
#endif
		if (tag == "v") {
			mysscanf(&line[0], "v %f %f %f", &v0, &v1, &v2);
			obj_v.push_back({ v0, v1, v2 });
		}
		else if (tag == "vt") {
			mysscanf(&line[0], "vt %f %f", &v0, &v1);
			// .objs from Blender come out with flipped tex coords.
			obj_vt.push_back({ v0, 1.0f - v1 });
		}
		else if (tag == "vn") {
			mysscanf(&line[0], "vn %f %f %f", &v0, &v1, &v2);
			obj_vn.push_back({ v0, v1, v2 });
		}
		else if (tag == "f") {
			mysscanf(&line[0], "f %u/%u/%u %u/%u/%u %u/%u/%u",
				&p0, &t0, &n0,
				&p1, &t1, &n1,
				&p2, &t2, &n2);
			obj_f.push_back({
				p0, t0, n0,
				p1, t1, n1,
				p2, t2, n2 });
		}
	}

	for (size_t i = 0; i < obj_f.size(); i++) {
		unsigned int vertA = obj_f[i][0] - 1;
		unsigned int vertB = obj_f[i][3] - 1;
		unsigned int vertC = obj_f[i][6] - 1;

		unsigned int normA = obj_f[i][2] - 1;
		unsigned int normB = obj_f[i][5] - 1;
		unsigned int normC = obj_f[i][8] - 1;

		unsigned int texA = obj_f[i][1] - 1;
		unsigned int texB = obj_f[i][4] - 1;
		unsigned int texC = obj_f[i][7] - 1;

		VertexDataKey keyA, keyB, keyC;

		keyA.pos = vertA;
		keyA.norm = normA;
		keyA.texcoord = texA;

		keyB.pos = vertB;
		keyB.norm = normB;
		keyB.texcoord = texB;

		keyC.pos = vertC;
		keyC.norm = normC;
		keyC.texcoord = texC;

		OBJVertex& currDataA = actualVertexData[keyA];
		OBJVertex& currDataB = actualVertexData[keyB];
		OBJVertex& currDataC = actualVertexData[keyC];

		actualIndexMap[keyA] = 0;
		actualIndexMap[keyB] = 0;
		actualIndexMap[keyC] = 0;

		memcpy(&currDataA.pos, &obj_v[vertA][0], sizeof(float) * 3);
		memcpy(&currDataB.pos, &obj_v[vertB][0], sizeof(float) * 3);
		memcpy(&currDataC.pos, &obj_v[vertC][0], sizeof(float) * 3);

		memcpy(&currDataA.norm, &obj_vn[normA][0], sizeof(float) * 3);
		memcpy(&currDataB.norm, &obj_vn[normB][0], sizeof(float) * 3);
		memcpy(&currDataC.norm, &obj_vn[normC][0], sizeof(float) * 3);

		memcpy(&currDataA.texcoord, &obj_vt[texA][0], sizeof(float) * 2);
		memcpy(&currDataB.texcoord, &obj_vt[texB][0], sizeof(float) * 2);
		memcpy(&currDataC.texcoord, &obj_vt[texC][0], sizeof(float) * 2);
	}

	unsigned int actualIndexMapIndex = 0;
	for (auto it = actualVertexData.begin(); it != actualVertexData.end(); ++it) {
		actualIndexMap[it->first] = actualIndexMapIndex;
		actualIndexMapIndex++;
		vertexData.push_back(it->second);
	}

	for (size_t i = 0; i < obj_f.size(); i++) {
		unsigned int vertA = obj_f[i][0] - 1;
		unsigned int vertB = obj_f[i][3] - 1;
		unsigned int vertC = obj_f[i][6] - 1;

		unsigned int normA = obj_f[i][2] - 1;
		unsigned int normB = obj_f[i][5] - 1;
		unsigned int normC = obj_f[i][8] - 1;

		unsigned int texA = obj_f[i][1] - 1;
		unsigned int texB = obj_f[i][4] - 1;
		unsigned int texC = obj_f[i][7] - 1;

		VertexDataKey keyA, keyB, keyC;

		keyA.pos = vertA;
		keyA.norm = normA;
		keyA.texcoord = texA;

		keyB.pos = vertB;
		keyB.norm = normB;
		keyB.texcoord = texB;

		keyC.pos = vertC;
		keyC.norm = normC;
		keyC.texcoord = texC;

		indexData.push_back(actualIndexMap[keyA]);
		indexData.push_back(actualIndexMap[keyB]);
		indexData.push_back(actualIndexMap[keyC]);
	}

	if (diffuseTextureName != "") {
		lodepng::decode(
			diffuseTextureRGBA,
			diffuseTextureWidth, diffuseTextureHeight,
			diffuseTextureName);
	}

	DLOG("%s: texture w h %u %u\n", __func__, diffuseTextureWidth, diffuseTextureHeight);
	for (size_t i = 0; i < vertexData.size(); i++) {
		DLOG("%s: pos %f %f %f n %f %f %f t %f %f\n", __func__,
			vertexData[i].pos[0],
			vertexData[i].pos[1],
			vertexData[i].pos[2],
			vertexData[i].norm[0],
			vertexData[i].norm[1],
			vertexData[i].norm[2],
			vertexData[i].texcoord[0],
			vertexData[i].texcoord[1]);
	}

	DLOG("%s: index elts %zu\n", __func__, indexData.size());
	for (size_t i = 0; i < indexData.size(); i += 3) {
		DLOG("%s: triangle %u %u %u\n", __func__, indexData[i], indexData[i + 1], indexData[i + 2]);
	}

	// Populate coarse bounds as AABB
	if (!vertexData.size()) return;

	float xmin, xmax, ymin, ymax, zmin, zmax;
    xmin = vertexData[0].pos[0];
    ymin = vertexData[0].pos[1];
    zmin = vertexData[0].pos[2];
    xmax = vertexData[0].pos[0];
    ymax = vertexData[0].pos[1];
    zmax = vertexData[0].pos[2];

    for (size_t i = 1; i < vertexData.size(); i++) {
        float x = vertexData[i].pos[0];
        float y = vertexData[i].pos[1];
        float z = vertexData[i].pos[2];

        xmin = x < xmin ? x : xmin;
        ymin = y < ymin ? y : ymin;
        zmin = z < zmin ? z : zmin;

        xmax = x > xmax ? x : xmax;
        ymax = y > ymax ? y : ymax;
        zmax = z > zmax ? z : zmax;
    }

    coarseBounds = makeOBB(
            xmin, ymin, zmin,
            1,0,0,0,1,0,0,0,1,
            xmax - xmin,
            ymax - ymin,
            zmax - zmin);
}

} // namespace LEL
