#pragma once

#include "civ5/runtime/bnw_nearby_cities.hpp"
#include <cstdint>
#include <map>
#include <vector>

namespace civ5::runtime {

struct LiveSessionState;

// CvCity::read registers each loaded CityStrategyAI with its owner's
// CvFlavorManager. This records the source order and identities, not pointers
// to the unavailable native AI objects or completed flavor propagation.
struct BnwCityFlavorRecipients {
    bool current=false;
    std::map<std::int64_t,std::vector<BnwCityHandle>> by_owner;
};

// Expansion2 CvCity.cpp:13724-13728. The hook uses
// AddFlavorRecipient(strategy, false), which appends without sending flavors.
// Unknown owner, missing strategy/manager, or divergent live city blocks.
void rebuild_bnw_city_flavor_recipients(LiveSessionState &state);

// Rechecks the owned source/live identities so a stale registration cannot be
// used after an untracked city change.
const std::vector<BnwCityHandle> &bnw_city_flavor_recipients(
    const LiveSessionState &state,std::int64_t owner);

} // namespace civ5::runtime
