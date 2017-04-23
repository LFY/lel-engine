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

#include "RayShooter.h"

#include "lel.h"

#include <string>
#include <vector>

namespace LEL {

void RayShooter::init(void* opaque, entity_handle_t _rayshooter) {
	fprintf(stderr, "%s: call\n", __func__);
    sys = (EntitySystem*)opaque;
    if (!sys) abort();
    rayShooter = _rayshooter;
    shootFrom =
        sys->getEntityIdByName(
            sys->mEntities[rayShooter].stringProps["ray_shoot_entity"]);
}

void RayShooter::deinit() {

}

void RayShooter::update() {
    EntitySystem& system = *sys;
    const Entity& from = system.mEntities[shootFrom];
    Entity& mark = system.mEntities[rayShooter];
    rayv4 ray = makerayv4(from.frame.pos, from.frame.fwd);
    float firstIntersect = 99999999.9f;
    float intersect = 99999999.9f;
    bool hadCollision = false;

    for (int i = 0; i < system.numActiveDynamicColliders; i++) {
		if (system.mDynamicColliders[i] == shootFrom) continue;
        const Entity& col = system.mEntities[system.mDynamicColliders[i]];
        if (col.passthroughRays) continue;
        hadCollision |= intersectOBB(ray, col.collision_info.bbox_current, &intersect);
        if (intersect < firstIntersect) firstIntersect = intersect;
    }

    for (int i = 0; i < system.numActiveStaticColliders; i++) {
        const Entity& col = system.mEntities[system.mStaticColliders[i]];
//        if (col.passthroughRays) continue;
        hadCollision |= intersectOBB(ray, col.collision_info.bbox_current, &intersect);
        if (intersect < firstIntersect) firstIntersect = intersect;
    }

    if (hadCollision) {
		mark.resetPos(from.frame.pos + firstIntersect * from.frame.fwd);
    }
}

} // namespace LEL
