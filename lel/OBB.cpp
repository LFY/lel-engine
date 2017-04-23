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

#include "OBB.h"

#include "lel.h"

#include <sstream>
#include "math.h"

namespace LEL {

OBB makeOBB(
        float ox, float oy, float oz,
        float ax, float ay, float az,
        float bx, float by, float bz,
        float cx, float cy, float cz,
        float sx, float sy, float sz) {

    OBB res;
	res.origin = makevector4(ox, oy, oz, 1.0f);
    res.axes[0] = v4normed(makevector4(ax, ay, az, 0.0f));
    res.axes[1] = v4normed(makevector4(bx, by, bz, 0.0f));
    res.axes[2] = v4normed(makevector4(cx, cy, cz, 0.0f));

    res.dims[0] = sx;
    res.dims[1] = sy;
    res.dims[2] = sz;

    return res;
}

OBB makeOBBNormedAxes(
        float ox, float oy, float oz,
        float ax, float ay, float az,
        float bx, float by, float bz,
        float cx, float cy, float cz,
        float sx, float sy, float sz) {

    OBB res;
	res.origin = makevector4(ox, oy, oz, 1.0f);
    res.axes[0] = makevector4(ax, ay, az, 0.0f);
    res.axes[1] = makevector4(bx, by, bz, 0.0f);
    res.axes[2] = makevector4(cx, cy, cz, 0.0f);

    res.dims[0] = sx;
    res.dims[1] = sy;
    res.dims[2] = sz;

    return res;
}

OBB makeOBB(
	const vector4& origin,
	const vector4& a,
	const vector4& b,
	const vector4& c,
	const vector4& s) {
	return makeOBB(
		origin.x, origin.y, origin.z,
		a.x, a.y, a.z,
		b.x, b.y, b.z,
		c.x, c.y, c.z,
		s.x, s.y, s.z
	);
}

std::string dumpOBB(const OBB& a) {
    std::ostringstream ss;
    ss << "OBB[ origin " << dumpv4(a.origin) << "\n";
    ss << "     axis 0 " << dumpv4(a.axes[0]) << " dim " << a.dims[0] << "\n";
    ss << "     axis 1 " << dumpv4(a.axes[1]) << " dim " << a.dims[1] << "\n";
    ss << "     axis 2 " << dumpv4(a.axes[2]) << " dim " << a.dims[2] << "\n";
    ss << "   ]   \n";

    return ss.str();
}

static std::array<vector4, 8> obb_points(const OBB& a) {
    const vector4& A = a.dims[0] * a.axes[0];
    const vector4& B = a.dims[1] * a.axes[1];
    const vector4& AB = A + B;

    const vector4& C = a.dims[2] * a.axes[2];
    const vector4& AC = A + C;
    const vector4& BC = B + C;
    const vector4& ABC = AB + C;

    return
        {
            a.origin,
            a.origin + A,
            a.origin + B,
            a.origin + C,
            a.origin + AB,
            a.origin + BC,
            a.origin + AC,
            a.origin + ABC,
        };
}

interval2 obb_extents_along_ray(const std::array<vector4, 8>& points,
                                const rayv4& r) {
    float min_interval = 0.0f;
    float max_interval = 0.0f;
	uint32_t i = 0;
    for (const auto& pt : points) {
        float coord = proj_pt_to_ray_coord(pt, r);
		if (i == 0) {
			min_interval = coord;
			max_interval = coord;
		} else {
			if (coord < min_interval) min_interval = coord;
			if (coord > max_interval) max_interval = coord;
		}
		i++;
    }
    return makeinterval2(min_interval, max_interval);
}

bool intersectOBB(const OBB& a, const OBB& b) {
    // test:
    // projection of OBB along 6 (3 a, 3b) axes
    // std::vector<rayv4> tests = {
    //     makerayv4(a.origin, a.axes[0]), // 0
    //     makerayv4(a.origin, a.axes[1]), // 1
    //     makerayv4(a.origin, a.axes[2]), // 2

    //     makerayv4(a.origin, b.axes[0]), // 3
    //     makerayv4(a.origin, b.axes[1]), // 4
    //     makerayv4(a.origin, b.axes[2]), // 5

    //     // Along all unique edge/edge planes
    //     makerayv4(a.origin, v4cross(a.axes[0], b.axes[0])), // 6
    //     makerayv4(a.origin, v4cross(a.axes[0], b.axes[1])), // 7
    //     makerayv4(a.origin, v4cross(a.axes[0], b.axes[2])), // 8
    //     makerayv4(a.origin, v4cross(a.axes[1], b.axes[0])), // 9
    //     makerayv4(a.origin, v4cross(a.axes[1], b.axes[1])), // 10
    //     makerayv4(a.origin, v4cross(a.axes[1], b.axes[2])), // 11
    //     makerayv4(a.origin, v4cross(a.axes[2], b.axes[0])), // 12
    //     makerayv4(a.origin, v4cross(a.axes[2], b.axes[1])), // 13
    //     makerayv4(a.origin, v4cross(a.axes[2], b.axes[2])), // 14
    // };

    rayv4 currTest;
    currTest.pos = a.origin;
#define CASE(x) \
    currTest.dir = x; \

    std::array<vector4, 8> pointsA = obb_points(a);
    std::array<vector4, 8> pointsB = obb_points(b);
    for (int i = 0; i < 15; i++) {
        switch (i) {
        case 0: CASE(a.axes[0]); break;
        case 1: CASE(a.axes[1]); break;
        case 2: CASE(a.axes[2]); break;

        case 3: CASE(b.axes[0]); break;
        case 4: CASE(b.axes[1]); break;
        case 5: CASE(b.axes[2]); break;

        case 6: CASE(v4cross(a.axes[0], b.axes[0])); break;
        case 7: CASE(v4cross(a.axes[0], b.axes[1])); break;
        case 8: CASE(v4cross(a.axes[0], b.axes[2])); break;
        case 9: CASE(v4cross(a.axes[1], b.axes[0])); break;
        case 10: CASE(v4cross(a.axes[1], b.axes[1])); break;
        case 11: CASE(v4cross(a.axes[1], b.axes[2])); break;
        case 12: CASE(v4cross(a.axes[2], b.axes[0])); break;
        case 13: CASE(v4cross(a.axes[2], b.axes[1])); break;
        case 14: CASE(v4cross(a.axes[2], b.axes[2])); break;
        default: break;
        }
        if (!v4closetozero(currTest.dir) &&
            !overlapinterval2(
                obb_extents_along_ray(pointsA, currTest),
                obb_extents_along_ray(pointsB, currTest))) {
            return false;
        }
    }

    return true;
}

static inline std::array<planev4, 6> obb_planes(const OBB& a) {
	const vector4& A = a.dims[0] * a.axes[0];
	const vector4& B = a.dims[1] * a.axes[1];
	const vector4& C = a.dims[2] * a.axes[2];

	return
	{
		makeplanev4(a.origin    , a.axes[0]),
		makeplanev4(a.origin + A, a.axes[0]),
		makeplanev4(a.origin    , a.axes[1]),
		makeplanev4(a.origin + B, a.axes[1]),
		makeplanev4(a.origin    , a.axes[2]),
		makeplanev4(a.origin + C, a.axes[2]),
	};
}

bool intersectOBB(const rayv4& a, const OBB& b, float* tOut) {
	std::array<planev4, 6> planes = obb_planes(b);
	float tMaxEnter = 0.0f;
	float tMinExit = 9999999999.0f;
	float t0a, t0b, t1a, t1b, t2a, t2b;

	t0a = ray_plane_intersect(a, planes[0]);
	if (fabs(t0a) > 0.00001f) {
		t0b = ray_plane_intersect(a, planes[1]);
			if (t0a > t0b) { float tmp = t0a; t0a = t0b; t0b = tmp; }
			if (t0b < tMinExit) tMinExit = t0b;
			if (t0a > tMaxEnter) tMaxEnter = t0a;
	}

	t1a = ray_plane_intersect(a, planes[2]);
	if (fabs(t1a) > 0.00001f) {
		t1b = ray_plane_intersect(a, planes[3]);
		if (t1a > t1b) { float tmp = t1a; t1a = t1b; t1b = tmp; }
		if (t1b < tMinExit) tMinExit = t1b;
		if (t1a > tMaxEnter) tMaxEnter = t1a;
		if (tMaxEnter >= tMinExit) return false;
	}

	t2a = ray_plane_intersect(a, planes[4]);
	if (fabs(t2a) > 0.00001f) {
		t2b = ray_plane_intersect(a, planes[5]);
		if (t2a > t2b) { float tmp = t2a; t2a = t2b; t2b = tmp; }
		if (t2b < tMinExit) tMinExit = t2b;
		if (t2a > tMaxEnter) tMaxEnter = t2a;
		if (tMaxEnter >= tMinExit) return false;
	}

	*tOut = tMaxEnter;
	return true;
}

OBB transformedOBB(const OBB& obb, const matrix4& tr) {
    vector4 origA = obb.axes[0];
    vector4 origB = obb.axes[1];
    vector4 origC = obb.axes[2];

    matrix4 next_obb_props =
        tr * makematrix4(
                 obb.origin.x, origA.x, origB.x, origC.x,
                 obb.origin.y, origA.y, origB.y, origC.y,
                 obb.origin.z, origA.z, origB.z, origC.z,
                 1.0f,             0.0f,   0.0f,    0.0f);
    const float* next = next_obb_props.vals;

    return makeOBBNormedAxes(
            next[0], next[1], next[2],
            next[4], next[5], next[6],
            next[8], next[9], next[10],
            next[12], next[13], next[14],
            obb.dims[0], obb.dims[1], obb.dims[2]);
}

std::vector<float> getOBBVertices(const OBB& obb) {
    const vector4& A = obb.dims[0] * obb.axes[0];
    const vector4& B = obb.dims[1] * obb.axes[1];
    const vector4& C = obb.dims[2] * obb.axes[2];

    const vector4& origin = obb.origin;
    const vector4& apos = origin + A;
    const vector4& bpos = origin + B;
    const vector4& abpos = origin + A + B;

    const vector4& cpos = origin + C;
    const vector4& acpos = origin + A + C;
    const vector4& bcpos = origin + B + C;
    const vector4& abcpos = origin + A + B + C;

    return
    {
        origin.x, origin.y, origin.z,
        apos.x, apos.y, apos.z,
        bpos.x, bpos.y, bpos.z,
        abpos.x, abpos.y, abpos.z,

        cpos.x, cpos.y, cpos.z,
        acpos.x, acpos.y, acpos.z,
        bcpos.x, bcpos.y, bcpos.z,
        abcpos.x, abcpos.y, abcpos.z,
    };
}

void getOBBVertices_inplace(const OBB& obb, float* posVector) {
    const vector4& A = obb.dims[0] * obb.axes[0];
    const vector4& B = obb.dims[1] * obb.axes[1];
    const vector4& C = obb.dims[2] * obb.axes[2];

    uint32_t coordsSize = 3 * sizeof(float);
    const vector4& origin = obb.origin; memcpy(posVector + 0, &origin, coordsSize);
    const vector4& apos = origin + A; memcpy(posVector + 3, &apos, coordsSize);
    const vector4& bpos = origin + B; memcpy(posVector + 6, &bpos, coordsSize);
    const vector4& abpos = apos + B; memcpy(posVector + 9, &abpos, coordsSize);

    const vector4& cpos = origin + C; memcpy(posVector + 12, &cpos, coordsSize);
    const vector4& acpos = apos + C; memcpy(posVector + 15, &acpos, coordsSize);
    const vector4& bcpos = bpos + C; memcpy(posVector + 18, &bcpos, coordsSize);
    const vector4& abcpos = abpos + C; memcpy(posVector + 21, &abcpos, coordsSize);

    // fprintf(stderr, "%s: %s\n", __func__, dumpOBB(obb).c_str());
    // fprintf(stderr, "%s: OBB @ %f %f %f, %f %f %f, %f %f %f, %f %f %f, %f %f %f\n", __func__, origin.x, origin.y, origin.z, obb.axes[0], obb.axes[1], obb.axes[2], obb.dims[0], obb.dims[1], obb.dims[2]);
    // fprintf(stderr, "%s: OBB @ a %f %f %f\n", __func__, apos.x, apos.y, apos.z);
    // fprintf(stderr, "%s: OBB @ b %f %f %f\n", __func__, bpos.x, bpos.y, bpos.z);
    // fprintf(stderr, "%s: OBB @ ab %f %f %f\n", __func__, abpos.x, abpos.y, abpos.z);
    // fprintf(stderr, "%s: OBB @ c %f %f %f\n", __func__, cpos.x, cpos.y, cpos.z);
    // fprintf(stderr, "%s: OBB @ ac %f %f %f\n", __func__, acpos.x, acpos.y, acpos.z);
    // fprintf(stderr, "%s: OBB @ bc %f %f %f\n", __func__, bcpos.x, bcpos.y, bcpos.z);
    // fprintf(stderr, "%s: OBB @ abc %f %f %f\n", __func__, abcpos.x, abcpos.y, abcpos.z);
}

} // namespace LEL
