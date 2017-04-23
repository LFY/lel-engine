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

#include "Racing.h"

#include "lel.h"

#include <algorithm>
#include <sstream>

namespace LEL {

void Racing::tryStart() {
    // check that the system has defined:
    // entities with property:
    // racetrackSeq n (at least 2)
    int numWaypoints = 0;
    uint32_t k = 0;
    for (auto& it : entitySystem->mEntities) {
        auto notfound = it.intProps.end();
        if (it.intProps.find("racetrackSeq") != notfound) {
            numWaypoints++;
            waypointEntityIds.push_back(k);
            entitySystem->addStaticColliderById(k);
            entitySystem->mEntities[k].passthroughRays = true;
			fprintf(stderr, "%s: add collider %u\n", __func__, k);
        }
        if (it.intProps.find("dynamic_text_lap_time") != notfound) {
            fprintf(stderr, "%s: ADDING TEXT PROP\n", __func__);
            it.addTextDisplay();
            it.getTextDisplay()->text = "welp. f";
            laptimeDisplayId = k;
        }
        if (it.intProps.find("dynamic_text_last_lap") != notfound) {
            fprintf(stderr, "%s: ADDING TEXT PROP\n", __func__);
            it.addTextDisplay();
            it.getTextDisplay()->text = "Last:";
            lastLapTimeDisplayId = k;
        }
        if (it.intProps.find("dynamic_text_best_lap") != notfound) {
            fprintf(stderr, "%s: ADDING TEXT PROP\n", __func__);
            it.addTextDisplay();
            it.getTextDisplay()->text = "Best:";
            bestLapTimeDisplayId = k;
        }
        if (it.intProps.find("player") != notfound) {
            PlayerState initial_state;
            initial_state.entityId = k;
			initial_state.lapStartTime = curr_time_us();
            playerStates.push_back(initial_state);
            entitySystem->addDynamicColliderById(k);
            entitySystem->mEntities[k].isUi = true;
        }
		if (it.intProps.find("racing_point_at_next") != notfound) {
			compassDisplayId = k;
            entitySystem->mEntities[k].isUi = true;
		}
		if (it.intProps.find("racing_static_collide") != notfound) {
			entitySystem->addStaticColliderById(k);
			obstacleEntityIds.insert(k);
		}
        k++;
    }

    // sort waypoint ids by their racetrackSeq
    std::sort(waypointEntityIds.begin(), waypointEntityIds.end(),
              [this](uint32_t a, uint32_t b) {
                  return entitySystem->mEntities[a].intProps["racetrackSeq"] <
                         entitySystem->mEntities[b].intProps["racetrackSeq"];
              });

    fprintf(stderr, "%s: racetrack: found %d waypoints %zu players\n", __func__, numWaypoints, playerEntityIds.size());

    // then check if entity system
    // has already defined "maxLaps"
    // or "currLap".
    entitySystem->intProps["currLap"] = 0;

    auto maxLapsIt = entitySystem->intProps.find("maxLaps");
    if (maxLapsIt == entitySystem->intProps.end())
        entitySystem->intProps["maxLaps"] = 3;
}

static bool sMatchCollision(
                const std::pair<entity_handle_t, entity_handle_t>& col,
                entity_handle_t a,
                entity_handle_t b) {
    return (a == col.first && b == col.second) ||
           (b == col.first && a == col.second);
}


void Racing::update() {
    for (const auto& collisionPair : entitySystem->mCollidingEntities) {
        for (auto& playerState : playerStates) {
            if (sMatchCollision(
                        collisionPair,
                        playerState.entityId,
                        waypointEntityIds[playerState.toVisitNext])) {
                fprintf(stderr, "%s: player visited %u!\n", __func__,
                        waypointEntityIds[playerState.toVisitNext]);
                playerState.visited.push_back(
                        waypointEntityIds[playerState.toVisitNext]);
                playerState.toVisitNext++;
                if (playerState.toVisitNext >= waypointEntityIds.size())
                    playerState.toVisitNext = 0;
            }

			bool playerCollideFirst =
				collisionPair.first == playerState.entityId;
			bool playerCollideSecond =
				collisionPair.second == playerState.entityId;

			if (playerCollideFirst) {
				if (obstacleEntityIds.find(collisionPair.second) != obstacleEntityIds.end()) {
					fprintf(stderr, "%s: player collided!\n", __func__);
					playerState.penaltyLapTime += 20000000; // 20 seconds
					playerState.dead = true;
					entitySystem->enqueueAction(4000000, new ResetEntitySystem(entitySystem));
				}
			}

			if (playerCollideSecond) {
				if (obstacleEntityIds.find(collisionPair.first) != obstacleEntityIds.end()) {
					fprintf(stderr, "%s: player collided!\n", __func__);
					playerState.penaltyLapTime += 20000000; // 20 seconds
					playerState.dead = true;
					entitySystem->enqueueAction(4000000, new ResetEntitySystem(entitySystem));
				}
			}
        }
    }

    for (auto& playerState : playerStates) {
        bool missing = false;
        for (const auto wpId : waypointEntityIds) {
            if (std::find(playerState.visited.begin(),
                          playerState.visited.end(),
                          wpId) == playerState.visited.end())
                missing = true;
        }

        if (!missing) {
            if (std::count_if(playerState.visited.begin(),
                              playerState.visited.end(),
                              [this](entity_handle_t visit) {
                                  return visit == waypointEntityIds[0];
                                  }) == 2) {
                playerState.currLap++;
                playerState.visited.clear();
                playerState.visited.push_back(waypointEntityIds[0]);
				playerState.lastLapTime = curr_time_us() - playerState.lapStartTime + playerState.penaltyLapTime;
                fprintf(stderr, "%s: just did a lap. curr lap: %u laptime: %f s\n", __func__, playerState.currLap,
                        playerState.lastLapTime / 1000.0f / 1000.0f);
				playerState.lapStartTime = curr_time_us();
				playerState.penaltyLapTime = 0;

                if (playerState.bestLapTime > playerState.lastLapTime ||
					playerState.bestLapTime == -1)
                    playerState.bestLapTime = playerState.lastLapTime;
            }
        }

        playerState.currLapTime = curr_time_us() - playerState.lapStartTime;

		if (playerState.dead) {
			std::stringstream ssCurr;
			ssCurr << "YOU ARE DEAD";
			entitySystem->mEntities[laptimeDisplayId].getTextDisplay()->text = ssCurr.str();
		}
		else {
			std::stringstream ssCurr;
			ssCurr << "current: " << playerState.currLapTime / 1000000.0f;
			entitySystem->mEntities[laptimeDisplayId].getTextDisplay()->text = ssCurr.str();
		}

        std::stringstream ssLast;
        ssLast << "last: " << playerState.lastLapTime / 1000000.0f;
        if (playerState.lastLapTime != -1)
            entitySystem->mEntities[lastLapTimeDisplayId].getTextDisplay()->text = ssLast.str();

        std::stringstream ssBest;
        ssBest << "best: " << playerState.bestLapTime / 1000000.0f;
        if (playerState.bestLapTime != -1)
            entitySystem->mEntities[bestLapTimeDisplayId].getTextDisplay()->text = ssBest.str();

		// update compass
		if (compassDisplayId) {
			mCompass.setEntities(compassDisplayId, waypointEntityIds[playerState.toVisitNext]);
			mCompass.update();
		}
    }

}

void Racing::updateWaypointVisitState() {

}

} // namespace LEL
