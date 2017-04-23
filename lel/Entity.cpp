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

#include "Entity.h"

#include "EntitySystem.h"

namespace LEL {

Entity::Entity(RenderModel* model) :
    mRenderModel(model),
    frame(makevector4(0,0,0,1),
          makevector4(1,0,0,0),
          makevector4(0,0,1,0)) {

} 

void Entity::deriveProps() {
    auto it = intProps.find("menuselector");
    if (it != intProps.end()) {
        if (it->second) {
            isMenuSelector = true;
        }
    }
}

RefFrame Entity::getWorldFrame() const {
    if (transformParent != -1) {
        return EntitySystem::get()->mEntities[transformParent].getWorldFrame().preapplyFrame(frame);
    } else {
        return frame;
    }
}
    
RefFrame Entity::getParentWorldFrame() const {
    if (transformParent != -1) {
        return EntitySystem::get()->mEntities[transformParent].getWorldFrame();
    } else {
        return RefFrame::identity();
    }
}

RefFrame Entity::getParentRotationFrame() const {
    RefFrame all = getParentWorldFrame();
    all.pos = makevector4(0,0,0,1);
    return all;
}

void Entity::resetPos(const vector4& v) {
    frame.pos.x = v.x;
    frame.pos.y = v.y;
    frame.pos.z = v.z;
    frame.pos.w = 1.0f;
    frameDirty = true;
}

void Entity::resetFwd(const vector4& v) {
    frame.fwd.x = v.x;
    frame.fwd.y = v.y;
    frame.fwd.z = v.z;
    frame.fwd.w = 0.0f;
    frameDirty = true;
}

void Entity::resetUp(const vector4& v) {
    frame.up.x = v.x;
    frame.up.y = v.y;
    frame.up.z = v.z;
    frame.up.w = 0.0f;
    frameDirty = true;
}

void Entity::resetPos(float x, float y, float z) {
    frame.pos.x = x;
    frame.pos.y = y;
    frame.pos.z = z;
    frame.pos.w = 1.0f;
    frameDirty = true;
}

void Entity::resetFwd(float x, float y, float z) {
    frame.fwd.x = x;
    frame.fwd.y = y;
    frame.fwd.z = z;
    frame.fwd.w = 0.0f;
    frameDirty = true;
}

void Entity::resetUp(float x, float y, float z) {
    frame.up.x = x;
    frame.up.y = y;
    frame.up.z = z;
    frame.up.w = 0.0f;
    frameDirty = true;
}

void Entity::refreshTransform() {

    if (transformParent != -1) {
        // Can't skip if dirty unless we propagate dirty state from
        // scene graph parent.
        mTransform = EntitySystem::get()->mEntities[transformParent].getWorldFrame().getMatrix() * frame.getMatrix();
    } else {
        if (!frameDirty) return;

        mTransform = frame.getMatrix();
    }

    frameDirty = false;
}

void Entity::setTransformParent(entity_handle_t e) {
    transformParent = e;
}

void Entity::setTransform(const matrix4& tr) {
    mTransform = tr;
}

void Entity::changeToParentCoords() {
	if (transformParent != -1) {
		fprintf(stderr, "%s: %p parent: %d (%s->%s)\n", __func__,
			this, transformParent,
			this->stringProps["__entity_name__"].c_str(),
			EntitySystem::get()->mEntities[transformParent].stringProps["__entity_name__"].c_str());
		frame = (EntitySystem::get()->mEntities[transformParent].getWorldFrame().getInverseFrame()).preapplyFrame(frame);
	}
}

void Entity::setProjection(
        float fov, float aspect,
        float nearclip, float farclip) {
    mProjection = makePerspectiveProj(
            fov, aspect, nearclip, farclip);
}

matrix4 Entity::getModelview() const {
    if (transformParent != -1)
        return (EntitySystem::get()->mEntities[transformParent].frame.preapplyFrame(frame)).getViewMatrix();
    else
        return frame.getViewMatrix();
}

matrix4 Entity::getProjection() const {
	return mProjection;
}

matrix4 Entity::currentCameraMatrix() const {
    return mProjection * getModelview();
}

matrix4 Entity::currentWorldMatrix() const {
    return mTransform;
}

RenderModel* Entity::getRenderModel() {
    return mRenderModel;
}

TextDisplay* Entity::getTextDisplay() {
    return mTextDisplay;
}

void Entity::addTextDisplay() {
    mTextDisplay = new TextDisplay();
}

void Entity::update() {
    if (ttl >= 0.0f) {
        ttl -= 0.01f;
        if (ttl < 0.0f) {
            live = false;
        }
    }

    accVelocity = damping * accVelocity;
    frame.velocity = accVelocity;

    frame.pos = frame.pos + frame.velocity;

    frameDirty |= !v4closetozero(frame.velocity) || frameDirty;

    if (hasCollision) {
        if (transformParent != -1) {
            collision_info.bbox_current =
                transformedOBB(collision_info.bbox_orig, EntitySystem::get()->mEntities[transformParent].frame.getMatrix() * frame.getMatrix());
        } else {
            if (!frameDirty) return;
            collision_info.bbox_current =
                transformedOBB(collision_info.bbox_orig, frame.getMatrix());
        }
    }
}

void Entity::draw() const {
    if (mRenderModel) mRenderModel->draw(mTransform);
}

// Input handlers
void Entity::handleWantedPoseUpdate(GameControllerState* gc, matrix4* tr, matrix4* pitchYaw, matrix4* roll) {
	if (!gc->attached) return;

    if (isMenuSelector) {
        // in this case, use the gc state directly
        bool nextUp = gc->up;
        bool nextDown = gc->down;
        bool nextLeft = gc->left;
        bool nextRight = gc->right;

        bool nextA = gc->a;
        bool nextB = gc->b;
        bool nextX = gc->x;
        bool nextY = gc->y;

        bool changed = false;
        if (discreteButtons.up && !nextUp) {
            discreteY++;
            changed = true;
        }
        if (discreteButtons.down && !nextDown) {
            discreteY--;
            changed = true;
        }
        if (discreteButtons.left && !nextLeft) {
            discreteX--;
            changed = true;
        }
        if (discreteButtons.right && !nextRight) {
            discreteX++;
            changed = true;
        }

		if (discreteButtons.a && !nextA) {
			discreteButtonA++;
			changed = true;
		}
		if (discreteButtons.b && !nextB) {
			discreteButtonB++;
			changed = true;
		}
		if (discreteButtons.x && !nextX) {
			discreteButtonX++;
			changed = true;
		}
		if (discreteButtons.y && !nextY) {
			discreteButtonY++;
			changed = true;
		}

        if (changed) {
            fprintf(stderr, "%s: @ udlr %d %d %d %d abxy %d %d %d %d menu opt %d %d\n",
				    __func__, nextUp, nextDown, nextLeft, nextRight, nextA, nextB, nextX, nextY, discreteX, discreteY);
        }

        discreteButtons.up = nextUp;
        discreteButtons.down = nextDown;
        discreteButtons.left = nextLeft;
        discreteButtons.right = nextRight;

        discreteButtons.a = nextA;
        discreteButtons.b = nextB;
        discreteButtons.x = nextX;
        discreteButtons.y = nextA;
    } else {
		vector4 wantedPos = (*tr) * frame.pos;
		vector4 posDiff = wantedPos - frame.pos;

		wantedAccel = (1.0f / mass) * posDiff;
		accVelocity = accVelocity + wantedAccel;

		resetFwd(v4normed((*pitchYaw) * frame.fwd));
		resetUp(v4normed((*roll) * (*pitchYaw) * frame.up));
		vector4 right = v4normed(v4cross(frame.fwd, frame.up));
		resetUp(v4normed(v4cross(right, frame.fwd)));
	}

}


} // namespace LEL
