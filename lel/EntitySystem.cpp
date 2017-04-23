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

#include "EntitySystem.h"
#include "EntitySystemControl.h"

#include "GameMode.h"
#include "MenuSelector.h"
#include "Racing.h"
#include "RayShooter.h"

#include "lel.h"

#include <algorithm>

namespace LEL {

entity_handle_t ENTITY_NOT_FOUND = -1;
static EntitySystem* sCurrentSystem = nullptr;

void EntitySystem::setGamemode(const std::string& name) {
    if (name == "racing") {
        fprintf(stderr, "%s: racing gamemode\n", __func__);
        gameMode = new Racing(this);
    }
}

void EntitySystem::loadModel(const std::string& name) {
	if (mModels.find(name) != mModels.end()) return;

    std::string geoPath = LEL::pathCat(mAssetDir, name + ".obj"); 
    std::string diffuseTexPath = LEL::pathCat(mAssetDir, name + "_diffuse.png");
    OBJData objData(geoPath, diffuseTexPath);
    RenderModel* model = new RenderModel(name, objData);
    addModel(model);
	addModelCollider(name, objData.coarseBounds);
}

void EntitySystem::addModel(RenderModel* model) {
	mModels[model->getName()] = model;
}

void EntitySystem::addModelCollider(const std::string& name, const OBB& obb) {
	mModelColliders[name] = obb;
}

static void sDoAnimFrameUpdate(EntitySystem* sys,
                               entity_handle_t* eid,
                               vector4* pos,
                               vector4* fwd,
                               vector4* up,
                               vector4* scale) {
    sys->mEntities[*eid].frame.pos = *pos;
    sys->mEntities[*eid].frame.fwd = *fwd;
    sys->mEntities[*eid].frame.up = *up;
    sys->mEntities[*eid].frame.scale = *scale;
}

void EntitySystem::addAnimFrame(entity_handle_t eid,
                                uint32_t frame,
                                const vector4& pos,
                                const vector4& fwd,
                                const vector4& up,
                                const vector4& scale) {

    SetPose* act = new SetPose;
    act->action = (EntityAction::ActionFunc)sDoAnimFrameUpdate;
    act->args[0] = this;
    act->eid = eid;
    act->pos = pos;
    act->fwd = fwd;
    act->up = up;
    act->scale = scale;

    enqueueAction(frame * 16667, act);
}

void EntitySystem::clearProps() {
    intProps.clear();
    stringProps.clear();
}

void EntitySystem::enqueueAction(uint64_t exec_time, EntityAction* act) {
    mPendingActions.emplace_back(exec_time, act);
}

void EntitySystem::updateState() {
    // Handle all pending actions that have passed expiration date
    uint64_t now = curr_time_us();

    for (auto& act : mPendingActions) {
        if (now >= act.first)
            act.second->doAction();
    }

    // Remove
    mPendingActions.erase(std::remove_if(
            mPendingActions.begin(), mPendingActions.end(),
            [=](const EntitySystem::TimedAction& act) -> bool {
                if (now >= act.first) {
                    delete act.second;
                    return true;
                }
                return false; }),
            mPendingActions.end());

	for (auto& elt : mEntities) { elt.update(); }

	mEntities.erase(
		std::remove_if(mEntities.begin(), mEntities.end(), [](const Entity& e) -> bool { return !e.live; }),
		mEntities.end());

	for (auto& elt : mEntities) { elt.refreshTransform(); }

    // Dynamic colliders are done all-pairs
    mCollidingEntities.clear();
    mCollisionResults.clear();
    mCollisionResults.resize(mEntities.size(), 0);
    for (int i = 0; i < numActiveDynamicColliders; i++) {
        Entity& col = mEntities[mDynamicColliders[i]];
        for (int j = 0; j < i; j++) {
            Entity& col2 = mEntities[mDynamicColliders[j]];
            bool isTouching = intersectOBB(col2.collision_info.bbox_current,
                                           col.collision_info.bbox_current);
            entity_handle_t j_entity = mDynamicColliders[j];
            entity_handle_t i_entity = mDynamicColliders[i];
            mCollisionResults[j_entity] = mCollisionResults[j_entity] || isTouching;
            mCollisionResults[i_entity] = mCollisionResults[i_entity] || isTouching;

            if (isTouching)
                mCollidingEntities.emplace_back(j_entity, i_entity);
        }
    }
    
    // Static colliders are done |Dyn| * |Static|
    for (int i = 0; i < numActiveStaticColliders; i++) {
        Entity& staticCol = mEntities[mStaticColliders[i]];
        for (int j = 0; j < numActiveDynamicColliders; j++) {
            Entity& dynamicCol = mEntities[mDynamicColliders[j]];
            bool isTouching = intersectOBB(staticCol.collision_info.bbox_current,
                                           dynamicCol.collision_info.bbox_current);
            entity_handle_t j_entity = mDynamicColliders[j];
            entity_handle_t i_entity = mStaticColliders[i];

            mCollisionResults[j_entity] = mCollisionResults[j_entity] || isTouching;
            mCollisionResults[i_entity] = mCollisionResults[i_entity] || isTouching;
            if (isTouching)
                mCollidingEntities.emplace_back(j_entity, i_entity);
        }
    }
    
	// do secondary updates
	for (const auto elt : mSecondaryUpdates) {
		elt->update();
	}

    // deal with gamemode conditions
    if (gameMode) gameMode->update();

    // racing: collided with all gates, avoided other collisions
    mLastUpdateTimeUsecs = now;
}

Entity& EntitySystem::getEntityById(entity_handle_t id) {
    return mEntities[id];
}

entity_handle_t EntitySystem::getEntityIdByName(const std::string& name) const {
    const auto entsIt = mNamedEntityMap.find(name);
    if (entsIt != mNamedEntityMap.end()) return entsIt->second;
    return ENTITY_NOT_FOUND;
}

Entity* EntitySystem::getEntityByName(const std::string& name) const {
    const auto entsIt = mNamedEntityMap.find(name);
    if (entsIt != mNamedEntityMap.end()) {
        return (Entity*)&mEntities[entsIt->second];
    }
    return nullptr;
}

bool EntitySystem::hasFocusEntity() const {
	return mCurrentFocus != ENTITY_NOT_FOUND;
}

vector4 EntitySystem::getCurrentFocusPos() const {
	if (!hasFocusEntity()) return makevector4(0, 0, 0, 1);
	return mEntities[mCurrentFocus].frame.pos;
}

void EntitySystem::addDynamicColliderById(uint32_t id) {
    for (int i = 0; i < numActiveDynamicColliders; i++) {
        if (mDynamicColliders[i] == id)
            return;
    }
    numActiveDynamicColliders++;
    mEntities[id].hasCollision = true;
    if (mDynamicColliders.size() < numActiveDynamicColliders)
        mDynamicColliders.resize(numActiveDynamicColliders);
    mDynamicColliders[numActiveDynamicColliders - 1] = id;
}

void EntitySystem::addStaticColliderById(uint32_t id) {
    for (int i = 0; i < numActiveStaticColliders; i++) {
        if (mStaticColliders[i] == id)
            return;
    }
    numActiveStaticColliders++;
    mEntities[id].hasCollision = true;
    if (mStaticColliders.size() < numActiveStaticColliders)
        mStaticColliders.resize(numActiveStaticColliders);
    mStaticColliders[numActiveStaticColliders - 1] = id;
}

void EntitySystem::removeDynamicColliderById(uint32_t id) {
    mEntities[id].hasCollision = false;
    for (int i = 0; i < numActiveDynamicColliders; i++) {
        if (mDynamicColliders[i] == id) {
            // swap the id with last
            mDynamicColliders[i] = mDynamicColliders[numActiveDynamicColliders - 1];
            mDynamicColliders[numActiveDynamicColliders - 1] = id;
            numActiveDynamicColliders--;
            return;
        }
    }
}

void EntitySystem::removeStaticColliderById(uint32_t id) {
    mEntities[id].hasCollision = false;
    for (int i = 0; i < numActiveStaticColliders; i++) {
        if (mStaticColliders[i] == id) {
            // swap the id with last
            mStaticColliders[i] = mStaticColliders[numActiveStaticColliders - 1];
            mStaticColliders[numActiveStaticColliders - 1] = id;
            numActiveStaticColliders--;
            return;
        }
    }
}

void EntitySystem::reset() {

    delete gameMode;
	gameMode = nullptr;

    for (const auto s : mSecondaryUpdates) {
        delete s;
    }
    mSecondaryUpdates.clear();

    delete mControl;
	mControl = nullptr;

    mControlInfo.entity = 0;

    mCurrentCamera = 0;
    numActiveStaticColliders = 0;
    numActiveDynamicColliders = 0;

    mCollidingEntities.clear();
    mCollisionResults.clear();
    mDynamicColliders.clear();
    mStaticColliders.clear();

    mPendingActions.clear();

    stringProps.clear();
    floatProps.clear();
    intProps.clear();

    mNamedEntityMap.clear();
    mEntities.clear();

    mTime = 0.0;
}

static void sParseEntitySystem(const std::string& asset_dir, const std::string& filename, EntitySystem& sys);

void EntitySystem::resetAndReload() {
    reset();
    sParseEntitySystem(mAssetDir, mFileBasename + ".esys", *this);
}

void EntitySystem::reloadFromFile(const std::string& filename) {
    reset();
	mFileBasename = filename;
    sParseEntitySystem(mAssetDir, mFileBasename + ".esys", *this);
}

static void sParseDefineModel(const std::string& line, EntitySystem* sys);
static void sParseDefineEntity(const std::string& line, EntitySystem* sys);
static void sParseDefineCamera(const std::string& line, EntitySystem* sys);

static void sParseSetModel(const std::string& line, EntitySystem* sys);
static void sParseSetEntity(const std::string& line, EntitySystem* sys);
static void sParseSetCamera(const std::string& line, EntitySystem* sys);

static void sParseSetGlobalProp(const std::string& line, EntitySystem* sys);
static void sParseEntityAnim(const std::string& line, EntitySystem* sys);

static void sParsePostProcess(EntitySystem* sys);

static void sParseEntitySystem(const std::string& asset_dir,
                               const std::string& filename,
                               EntitySystem& sys) {

    std::vector<std::string> lines = readLines(pathCat(asset_dir, filename).c_str());
    size_t kNull = std::string::npos;

    for (const auto& line : lines) {
        if (line.find("#") != kNull) continue;

        bool isSet = line.find("set") == 0;
        bool isDefine = line.find("define") == 0;

        if (!(isSet || isDefine)) continue;

#ifndef _WIN32
        auto mysscanf = sscanf;
#else
        auto mysscanf = sscanf_s;
#endif

        if (isDefine) {
            size_t expectedStart = strlen("define") + 1;
            if (line.find("model") == expectedStart) {
                sParseDefineModel(line, &sys);
            }
            if (line.find("entity") == expectedStart) {
                sParseDefineEntity(line, &sys);
            }
            if (line.find("camera") == expectedStart) {
                sParseDefineCamera(line, &sys);
            }
        }

        if (isSet) {
            size_t expectedStart = strlen("set") + 1;
            if (line.find("model") == expectedStart) {
                sParseSetModel(line, &sys);
            }
            if (line.find("entity") == expectedStart) {
                sParseSetEntity(line, &sys);
            }
            if (line.find("camera") == expectedStart) {
                sParseSetCamera(line, &sys);
            }
            if (line.find("prop") == expectedStart) {
                sParseSetGlobalProp(line, &sys);
            }
            if (line.find("entityanim") == expectedStart) {
                sParseEntityAnim(line, &sys);
            }
        }
    }

    sParsePostProcess(&sys);
}

EntitySystem::EntitySystem() {
    sCurrentSystem = this;
}

// static
EntitySystem* EntitySystem::get() {
    return sCurrentSystem;
}

EntitySystem* parseEntitySystem(const std::string& asset_dir,
                                const std::string& file_basename) {

    EntitySystem* res = new EntitySystem();
	res->mAssetDir = asset_dir;
	res->mFileBasename = file_basename;

    sParseEntitySystem(asset_dir, file_basename + ".esys", *res);

    return res;
}

#ifndef _WIN32
        static auto mysscanf = sscanf;
#else
        static auto mysscanf = sscanf_s;
#endif

#define MAX_ESYS_NAME_LEN 1024

static void sParseDefineModel(const std::string& line, EntitySystem* sys) {
    char modelName[MAX_ESYS_NAME_LEN];
#ifdef _WIN32
    sscanf_s(&line[0], "define model %s", modelName, (unsigned int)sizeof(modelName));
#else
    sscanf(&line[0], "define model %s", modelName);
#endif
    DLOG("found model defined as %s", modelName);
    sys->loadModel(std::string(modelName));
}

static void sParseDefineEntity(const std::string& line, EntitySystem* sys) {
    char modelName[MAX_ESYS_NAME_LEN];
    unsigned int entityIndex;
#ifdef _WIN32
    sscanf_s(&line[0], "define entity %s %u", modelName, (unsigned int)sizeof(modelName), &entityIndex);
#else
    sscanf(&line[0], "define entity %s %u", modelName, &entityIndex);
#endif
    DLOG("found entity using model %s index %u", modelName, entityIndex);
    if (sys->mEntities.size() < entityIndex + 1)
        sys->mEntities.resize(entityIndex + 1);
    sys->mEntities[entityIndex] = Entity(sys->mModels[modelName]);
    sys->mEntities[entityIndex].collision_info.bbox_orig = sys->mModelColliders[modelName];
}

static void sParseDefineCamera(const std::string& line, EntitySystem* sys) {
    char cameraName[MAX_ESYS_NAME_LEN];
    unsigned int entityIndex;
#ifdef _WIN32
    sscanf_s(&line[0], "define camera %s %u", cameraName, (unsigned int)sizeof(cameraName), &entityIndex);
#else
    sscanf(&line[0], "define camera %s %u", cameraName, &entityIndex);
#endif
    DLOG("found camera named %s", cameraName);
    Entity camera;
    if (sys->mEntities.size() < entityIndex + 1)
        sys->mEntities.resize(entityIndex + 1);
    sys->mEntities[entityIndex] = camera;
    sys->mNamedEntityMap[cameraName] = entityIndex;
    fprintf(stderr, "%s: camera id: %u\n", __func__, entityIndex);
}

static void sParseSetModel(const std::string& line, EntitySystem* sys) {
    DLOG("undefined");
}

static void sParseSetEntity(const std::string& line, EntitySystem* sys) {
    unsigned int entityIndex;
    float x, y, z;
    float px, py, pz, fx, fy, fz, ux, uy, uz;

    char propName[MAX_ESYS_NAME_LEN];
    unsigned int propNameLen = (unsigned int)sizeof(propName);
    int intprop; float floatprop;
    char strProp[MAX_ESYS_NAME_LEN];
    unsigned int strPropLen = (unsigned int)sizeof(strProp);

    int parsedPos = mysscanf(&line[0], "set entity %u pos %f %f %f", &entityIndex, &x, &y, &z);
    int parsedScale = mysscanf(&line[0], "set entity %u scale %f %f %f", &entityIndex, &x, &y, &z);
    int parsedFrame = mysscanf(&line[0], "set entity %u frame %f %f %f %f %f %f %f %f %f", &entityIndex, &px, &py, &pz, &fx, &fy, &fz, &ux, &uy, &uz);

#ifdef _WIN32
    int parsedIntProp = sscanf_s(&line[0], "set entity %u prop int %s %d", &entityIndex, propName, propNameLen, &intprop);
    int parsedFloatProp = sscanf_s(&line[0], "set entity %u prop float %s %f", &entityIndex, propName, propNameLen, &floatprop);
    int parsedStringProp = sscanf_s(&line[0], "set entity %u prop str %s \"%[^\"]\"", &entityIndex, propName, propNameLen, strProp, strPropLen);
#else
    int parsedIntProp = mysscanf(&line[0], "set entity %u prop int %s %d", &entityIndex, propName, &intprop);
    int parsedFloatProp = mysscanf(&line[0], "set entity %u prop float %s %f", &entityIndex, propName, &floatprop);
    int parsedStringProp = mysscanf(&line[0], "set entity %u prop str %s \"%[^\"]\"", &entityIndex, propName, strProp);
#endif

    if (parsedPos == 4) {
        DLOG("set entity %u pos %f %f %fn", entityIndex, x, y, z);
        sys->mEntities[entityIndex].frame.pos = makevector4(x, y, z, 1.0f);
    }

    if (parsedScale == 4) {
        DLOG("set entity %u scale %f %f %f", entityIndex, x, y, z);
        sys->mEntities[entityIndex].frame.scale.x = x;
        sys->mEntities[entityIndex].frame.scale.y = y;
        sys->mEntities[entityIndex].frame.scale.z = z;
    }

    if (parsedFrame == 10) {
        sys->mEntities[entityIndex].resetPos(px, py, pz);
        sys->mEntities[entityIndex].resetFwd(fx, fy, fz);
        sys->mEntities[entityIndex].resetUp(ux, uy, uz);
    }

    if (parsedIntProp == 3) {
        sys->mEntities[entityIndex].intProps[propName] = intprop;
        sys->mEntities[entityIndex].deriveProps();
    }

    if (parsedFloatProp == 3) {
        sys->mEntities[entityIndex].floatProps[propName] = floatprop;
        sys->mEntities[entityIndex].deriveProps();
    }

    if (parsedStringProp == 3) {
        sys->mEntities[entityIndex].stringProps[propName] = strProp;
        sys->mEntities[entityIndex].deriveProps();
    }
}

static void sParseSetCamera(const std::string& line, EntitySystem* sys) {
    char cameraName[MAX_ESYS_NAME_LEN];
    float x, y, z, w;
    float px, py, pz, fx, fy, fz, ux, uy, uz;
    unsigned int cameraNameLen = sizeof(cameraName);

    char propName[MAX_ESYS_NAME_LEN];
    unsigned int propNameLen = (unsigned int)sizeof(propName);
    int intprop; float floatprop;
    char strProp[MAX_ESYS_NAME_LEN];
    unsigned int strPropLen = (unsigned int)sizeof(strProp);
#ifdef _WIN32
    int parsedProj = sscanf_s(&line[0], "set camera %s proj %f %f %f %f", cameraName, cameraNameLen, &x, &y, &z, &w);
    int parsedPos = sscanf_s(&line[0], "set camera %s pos %f %f %f", cameraName, cameraNameLen, &x, &y, &z);
    int parsedFwd = sscanf_s(&line[0], "set camera %s fwd %f %f %f", cameraName, cameraNameLen, &x, &y, &z);
    int parsedUp = sscanf_s(&line[0], "set camera %s up %f %f %f", cameraName, cameraNameLen, &x, &y, &z);
    int parsedFrame = sscanf_s(&line[0], "set camera %s frame %f %f %f %f %f %f %f %f %f", cameraName, cameraNameLen, &px, &py, &pz, &fx, &fy, &fz, &ux, &uy, &uz);

    int parsedIntProp = sscanf_s(&line[0], "set camera %s prop int %s %d", cameraName, cameraNameLen, propName, propNameLen, &intprop);
    int parsedFloatProp = sscanf_s(&line[0], "set camera %s prop float %s %f", cameraName, cameraNameLen, propName, propNameLen, &floatprop);
    int parsedStringProp = sscanf_s(&line[0], "set camera %s prop str %s \"%[^\"]\"", cameraName, cameraNameLen, propName, propNameLen, strProp, strPropLen);
#else
    int parsedProj = sscanf(&line[0], "set camera %s proj %f %f %f %f", cameraName, &x, &y, &z, &w);
    int parsedPos = sscanf(&line[0], "set camera %s pos %f %f %f", cameraName, &x, &y, &z);
    int parsedFwd = sscanf(&line[0], "set camera %s fwd %f %f %f", cameraName, &x, &y, &z);
    int parsedUp = sscanf(&line[0], "set camera %s up %f %f %f", cameraName, &x, &y, &z);
    int parsedFrame = sscanf(&line[0], "set camera %s frame %f %f %f %f %f %f %f %f %f", cameraName, &px, &py, &pz, &fx, &fy, &fz, &ux, &uy, &uz);

    int parsedIntProp = mysscanf(&line[0], "set camera %s prop int %s %d", cameraName, propName, &intprop);
    int parsedFloatProp = mysscanf(&line[0], "set camera %s prop float %s %f", cameraName, propName, &floatprop);
    int parsedStringProp = mysscanf(&line[0], "set camera %s prop str %s \"%[^\"]\"", cameraName, propName, strProp);
#endif

	Entity& camera = sys->getEntityById(sys->getEntityIdByName(cameraName));

    if (parsedProj == 5) {
        DLOG("set camera %s proj %f %f %f %f", cameraName, x, y, z, w);
        camera.setProjection(x, y, z, w);
    }
    if (parsedPos == 4) {
        DLOG("set camera %s pos %f %f %f", cameraName, x, y, z);
        camera.frame.pos = makevector4(x, y, z, 1.0f);
    }
    if (parsedFwd == 4) {
        DLOG("set camera %s fwd %f %f %f", cameraName, x, y, z);
        camera.frame.fwd = makevector4(x, y, z, 0.0f);
    }
    if (parsedUp == 4) {
        DLOG("set camera %s up %f %f %f", cameraName, x, y, z);
        camera.frame.up = makevector4(x, y, z, 0.0f);
    }
    if (parsedFrame == 10) {
        camera.frame = RefFrame(px, py, pz, fx, fy, fz, ux, uy, uz);
    }

    if (parsedIntProp == 3) {
        fprintf(stderr, "%s: parsed camera porp %s\n", __func__, propName);
        camera.intProps[propName] = intprop;
    }
    if (parsedFloatProp == 3) {
        camera.floatProps[propName] = floatprop;
    }
    if (parsedStringProp == 3) {
        camera.stringProps[propName] = strProp;
    }
}

static void sParseSetGlobalProp(const std::string& line, EntitySystem* sys) {
    char propName[MAX_ESYS_NAME_LEN];
    unsigned int propNameLen = (unsigned int)sizeof(propName);
    int intprop; float floatprop;
    char strProp[MAX_ESYS_NAME_LEN];
    unsigned int strPropLen = (unsigned int)sizeof(strProp);
#ifdef _WIN32
    int parsedIntProp = sscanf_s(&line[0], "set prop int %s %d", propName, propNameLen, &intprop);
    int parsedFloatProp = sscanf_s(&line[0], "set prop float %s %f", propName, propNameLen, &floatprop);
    int parsedStringProp = sscanf_s(&line[0], "set prop str %s \"%[^\"]\"", propName, propNameLen, strProp, strPropLen);
#else
    int parsedIntProp = mysscanf(&line[0], "set prop int %s %d", propName, &intprop);
    int parsedFloatProp = mysscanf(&line[0], "set prop float %s %f", propName, &floatprop);
    int parsedStringProp = mysscanf(&line[0], "set prop str %s \"%[^\"]\"", propName, strProp);
#endif

    if (parsedIntProp == 2) {
        sys->intProps[propName] = intprop;
    }

    if (parsedFloatProp == 2) {
        sys->floatProps[propName] = floatprop;
    }

    if (parsedStringProp == 2) {
        sys->stringProps[propName] = strProp;
    }
}

static void sParseEntityAnim(const std::string& line, EntitySystem* sys) {

    unsigned int eid;
    unsigned int frame;
    float p0, p1, p2, f0, f1, f2, u0, u1, u2, s0, s1, s2;
    int parsed =
        mysscanf(&line[0], "set entityanim %u %u %f %f %f %f %f %f %f %f %f %f %f %f",
                 &frame, &eid,
                 &p0, &p1, &p2, &f0, &f1, &f2, &u0, &u1, &u2, &s0, &s1, &s2);

    if (parsed == 14) {
        sys->addAnimFrame(eid, frame,
                          makevector4(p0, p1, p2, 1.0f),
                          makevector4(f0, f1, f2, 0.0f),
                          makevector4(u0, u1, u2, 0.0f),
                          makevector4(s0, s1, s2, 0.0f));
    }
}


static void sParsePostProcess(EntitySystem* sys) {
    const auto gamemodePropIt = sys->stringProps.find("gamemode");
    if (gamemodePropIt != sys->stringProps.end()) {
        sys->setGamemode(gamemodePropIt->second);
    }

    uint32_t i = 0;

    for (const auto& elt : sys->mEntities) {
        const auto propIt = elt.stringProps.find("__entity_name__");
        if (propIt == elt.stringProps.end()) continue;
        sys->mNamedEntityMap[propIt->second] = i;

        i++;
    }

    for (auto& elt : sys->mEntities) {
        const auto propIt = elt.stringProps.find("__entity_parent__");
        if (propIt == elt.stringProps.end()) continue;
        fprintf(stderr, "%s: found parent entity %s\n", __func__, propIt->second.c_str());
        elt.setTransformParent(sys->getEntityIdByName(propIt->second));
    }

    bool done = false;

    while (!done) {
        done = true;
        for (auto& elt : sys->mEntities) {
            if (elt.needUpdateTransformFromParent) {
                if (elt.transformParent == ENTITY_NOT_FOUND) {
                    elt.needUpdateTransformFromParent = false;
                } else {
                    if (!sys->mEntities[elt.transformParent].needUpdateTransformFromParent) {
                        fprintf(stderr, "%s: set to parent coords!\n", __func__);
                        elt.changeToParentCoords();
                        elt.needUpdateTransformFromParent = false;
                        done = false;
                        break;
                    } else {
                        fprintf(stderr, "%s: still need update transform form parent\n", __func__);
                        done = false;
                    }
                }
            }
        }
    }

    fprintf(stderr, "%s: initializing current camera\n", __func__);
    i = 0;
    for (auto& elt : sys->mEntities) {
        if (elt.intProps.find("currentCamera") != elt.intProps.end()) {
            sys->mCurrentCamera = i;
        }
        i++;
    }

    fprintf(stderr, "%s: initializing control\n", __func__);
    i = 0;
    for (auto& elt : sys->mEntities) {
        if (elt.intProps["currentControl"]) {
            sys->mControlInfo.entity = i;
        }
        i++;
    }
    sys->mControl = new EntitySystemControl(sys);

	fprintf(stderr, "%s: initializing static text displays\n", __func__);
	for (auto& elt : sys->mEntities) {
		auto it = elt.stringProps.find("static_text");
		if (it != elt.stringProps.end()) {
			elt.addTextDisplay();
			elt.getTextDisplay()->text = it->second;
		}
	}

    fprintf(stderr, "%s: initializing menuselector\n", __func__);
    i = 0;
	for (auto& elt : sys->mEntities) {
		auto it = elt.intProps.find("menuselector");
		if (it != elt.intProps.end()) {
            MenuSelector* upd = new MenuSelector;
            upd->init((void*)sys, i);
            sys->mSecondaryUpdates.push_back(upd);
		}
        i++;
	}

    fprintf(stderr, "%s: initializing rayshooter\n", __func__);
    i = 0;
	for (auto& elt : sys->mEntities) {
		auto it = elt.intProps.find("ray_shoot");
		if (it != elt.intProps.end()) {
            RayShooter* upd = new RayShooter;
            upd->init((void*)sys, i);
            sys->mSecondaryUpdates.push_back(upd);
		}
        i++;
	}

	fprintf(stderr, "%s: initializing focusers\n", __func__);
	i = 0;
	for (auto& elt : sys->mEntities) {
		auto it = elt.intProps.find("currentFocus");
		if (it != elt.intProps.end()) {
			sys->mCurrentFocus = i;
		}
		i++;
	}

    fprintf(stderr, "%s: done with postprocess. attempting to init gamemode\n", __func__);
    if (sys->gameMode) sys->gameMode->tryStart();
}

} // namespace LEL
