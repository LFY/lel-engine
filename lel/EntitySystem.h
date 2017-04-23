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

#include "Camera.h"
#include "Collider.h"
#include "Entity.h"
#include "EntityAction.h"
#include "GameControllers.h"
#include "OBB.h"
#include "OBJData.h"
#include "RenderModel.h"
#include "SecondaryUpdate.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace LEL {

class GameMode;
class EntitySystemControl;
enum EntityControlType {
    ENTITY_CONTROL_TYPE_DIRECT,
    ENTITY_CONTROL_TYPE_MAX,
};

extern entity_handle_t ENTITY_NOT_FOUND;

class EntitySystem {
using TimedAction = std::pair<uint64_t, EntityAction*>;
public:
    EntitySystem();

    static EntitySystem* get();

    void setGamemode(const std::string& name);
    void loadModel(const std::string& name);
	void addModel(RenderModel* model);
	void addModelCollider(const std::string& name, const OBB& obb);
    void addAnimFrame(entity_handle_t eid,
                      uint32_t frame,
                      const vector4& pos,
                      const vector4& fwd,
                      const vector4& up,
                      const vector4& scale);

    void clearProps();

    void enqueueAction(uint64_t exec_time, EntityAction* act);
	void updateState();

    Entity& getEntityById(entity_handle_t id);
    entity_handle_t getEntityIdByName(const std::string& name) const;
    Entity* getEntityByName(const std::string& name) const;

	bool hasFocusEntity() const;
	vector4 getCurrentFocusPos() const;

    void addDynamicColliderById(uint32_t id);
    void addStaticColliderById(uint32_t id);
    void removeDynamicColliderById(uint32_t id);
    void removeStaticColliderById(uint32_t id);

    // Resets the system to the initial state, o
    // reload the system from a given file.
    void reset();
    void resetAndReload();
    void reloadFromFile(const std::string& file);
    
    std::string mAssetDir;
    std::string mFileBasename;

    float mTime;
    uint64_t mLastUpdateTimeUsecs = 0ULL;
    std::unordered_map<std::string, RenderModel*> mModels;
    std::vector<Entity> mEntities;
    std::unordered_map<std::string, entity_handle_t> mNamedEntityMap;

    std::unordered_map<std::string, int> intProps;
    std::unordered_map<std::string, int> floatProps;
    std::unordered_map<std::string, std::string> stringProps;

    std::vector<TimedAction> mPendingActions;

    // Collision system
	std::unordered_map<std::string, OBB> mModelColliders;

    // All-pairs eager collision for now.
	std::vector<entity_handle_t> mDynamicColliders;
	std::vector<entity_handle_t> mStaticColliders;
	std::vector<entity_handle_t> mCollisionResults; // tracking entities that collide
    std::vector<std::pair<entity_handle_t, entity_handle_t> > mCollidingEntities;
    uint32_t numActiveDynamicColliders = 0;
    uint32_t numActiveStaticColliders = 0;

	entity_handle_t mCurrentCamera = 0;
	entity_handle_t mCurrentFocus = ENTITY_NOT_FOUND;

    // Player input
    struct ControlledEntityInfo {
        entity_handle_t entity;
        EntityControlType controlType;
    };

    ControlledEntityInfo mControlInfo;

    EntitySystemControl* mControl;

    // Other stuff
	std::vector<SecondaryUpdate*> mSecondaryUpdates;
    // specific to game:
    GameMode* gameMode = nullptr;
};

EntitySystem* parseEntitySystem(const std::string& asset_dir,
                                const std::string& file_basename);

class ResetEntitySystem : public EntityAction {
public:
	ResetEntitySystem(EntitySystem* _sys) : sys(_sys) { }

	virtual void doAction() override {
		fprintf(stderr, "%s: start\n", __func__);
		if (sys) {
			fprintf(stderr, "%s: reset\n", __func__);
			sys->resetAndReload();
		}
	}
	EntitySystem* sys = nullptr;
};

class ChangeEntitySystem : public EntityAction {
public:
	ChangeEntitySystem(EntitySystem* _sys,
                       const std::string& _basename) :
        sys(_sys), basename(_basename) { }

	virtual void doAction() override {
		fprintf(stderr, "%s: start\n", __func__);
		if (sys) {
			fprintf(stderr, "%s: reset\n", __func__);
			sys->reloadFromFile(basename);
		}
	}

	EntitySystem* sys = nullptr;
    std::string basename = {};
};

} // namespace LEL
