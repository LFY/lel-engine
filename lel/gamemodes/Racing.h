#pragma once

#include "Compass.h"
#include "GameMode.h"

#include <set>

namespace LEL {

class Racing : public GameMode {
public:
    Racing(EntitySystem* sys) :
        GameMode(sys), mCompass(sys) { }
    virtual void tryStart() override;
    virtual void update() override;
private:
    struct PlayerState {
        entity_handle_t entityId = 0;
        uint32_t toVisitNext = 0;
        uint32_t currLap = 0;
        std::vector<entity_handle_t> visited = {};
		uint64_t lapStartTime;
		uint64_t currLapTime;
		uint64_t penaltyLapTime = 0;

		uint64_t lastLapTime = -1;
		uint64_t bestLapTime = -1;

		bool dead = false;
    };
    entity_handle_t laptimeDisplayId;
    entity_handle_t lastLapTimeDisplayId;
    entity_handle_t bestLapTimeDisplayId;
	entity_handle_t compassDisplayId = 0;
	Compass mCompass;
    void updateWaypointVisitState();
    std::vector<entity_handle_t> waypointEntityIds = {};
    std::vector<entity_handle_t> playerEntityIds = {};
	std::set<entity_handle_t> obstacleEntityIds = {};
    std::vector<PlayerState> playerStates = {};
};

} // namespace LEL
