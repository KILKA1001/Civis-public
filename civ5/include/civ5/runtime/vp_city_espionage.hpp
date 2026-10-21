#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace civ5::runtime {
// City espionage belongs to the selected VP GameCore, on the same live city
// object used by graphics and all common gameplay systems. Player IDs are map
// keys; the source's fixed wire array length is not a host capacity.
struct VpCityEspionageState {
    // City-owned VP security inputs, mutable after buildings, missions and
    // local happiness change. No shared CityYieldState field is required.
    std::int64_t security_modifier=0,security_per_x_pop=0;
    std::int64_t previous_spy_missions=0,happiness_delta=0;
    struct NextPassiveBonus {
        bool initialized=false;
        // nullopt after initialization means there is no further threshold.
        // The original INT_MAX marker is decoded, never used as a live cap.
        std::optional<std::int64_t> threshold;
    };
    std::map<std::string,std::map<std::int64_t,std::int64_t>,std::less<>> fields;
    std::map<std::int64_t,NextPassiveBonus> next_passive_bonus;
    std::int64_t value(std::string_view field,std::int64_t player) const;
    void set(std::string_view field,std::int64_t player,std::int64_t value);
};
VpCityEspionageState &ensure_vp_city_espionage(CityYieldContext &world,
    std::int64_t owner,std::int64_t city);
const VpCityEspionageState *find_vp_city_espionage(const CityYieldContext &world,
    std::int64_t owner,std::int64_t city);
void restore_vp_city_espionage(const saves::SavedSessionSnapshot &snapshot,CityYieldContext &world);

struct VpSpyCityBinding {
    std::int64_t owner=-1,city=-1,x=-1,y=-1;
    std::int64_t assignment=-1,amount=-1,max_amount=-1,rate=-1,goal=-1;
};
// Binds one saved spy location to the live city and its selected VP activity.
// A missing city is a source-consistent absent target; a mismatched assignment
// is an invalid imported world, never silently repaired.
std::optional<VpSpyCityBinding> vp_spy_city_binding(const CityYieldContext &world,
    std::int64_t spy_owner,std::size_t spy_index,std::int64_t x,std::int64_t y);

struct VpCitySpyProcessResult {
    std::int64_t before=-1,after=-1,max_before=-1,max_after=-1,rate=0;
};
// Source CvCityEspionage::Process primitive, used by AddNetworkPoints. The
// caller supplies the already calculated rate and remains responsible for
// passive bonuses, visibility, notifications, missions and spy experience.
VpCitySpyProcessResult process_vp_city_spy_activity(VpCityEspionageState &city,
    std::int64_t player,std::int64_t rate);
} // namespace civ5::runtime
