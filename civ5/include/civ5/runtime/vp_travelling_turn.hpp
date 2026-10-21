#pragma once
#include "civ5/runtime/vp_city_espionage.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"

namespace civ5::runtime {
struct VpTravellingStep {
    std::int64_t spy_owner=-1,target_owner=-1,city=-1;
    std::size_t spy_index=0;
    std::int64_t before=-1,after=-1,max_before=-1,max_after=-1,rate=-1,goal=-1;
    bool arrival_pending=false;
    std::string unresolved;
};
// VP ProcessSpy's travelling state uses the city-owned Process primitive.
// Arrival itself changes spy state and can trigger missions/notifications;
// the caller stops before that transition until its effect hosts are bound.
VpTravellingStep process_vp_travelling_without_arrival_host(
    const ResearchContext &setup,CityYieldContext &cities,
    std::int64_t spy_owner,std::size_t spy_index);
} // namespace civ5::runtime
