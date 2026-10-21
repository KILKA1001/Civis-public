#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <map>
#include <optional>
#include <vector>

namespace civ5::runtime {
// CvPlayer::firstCity/nextCity order for VP's any-city construction intrigue.
// The live list can later be updated when cities are founded or removed.
struct VpCityIntrigueOrderProfileState {
    std::map<std::int64_t,std::vector<std::int64_t>> players;
};
VpCityIntrigueOrderProfileState &ensure_vp_city_intrigue_order_profile(CityYieldContext &world);
const VpCityIntrigueOrderProfileState *find_vp_city_intrigue_order_profile(const CityYieldContext &world);
void restore_vp_city_intrigue_order_profile(const saves::SavedSessionSnapshot &snapshot,
                                             CityYieldContext &world);
struct VpConstructionIntrigueCandidate {
    std::int64_t city=-1,x=-1,y=-1,order_type=-1,item=-1;
};
struct VpConstructionIntrigueScan {
    std::size_t scanned_cities=0;
    std::optional<VpConstructionIntrigueCandidate> candidate;
};
// VP UncoverIntrigue scans a city's current order: nonrepeatable project,
// world-wonder building, or unit linked to a spaceship project. Current
// modded XML/SQL tables decide classifications; no mod-specific C++ branch.
VpConstructionIntrigueScan vp_first_city_construction_intrigue(
    const CityYieldContext &world,const content::RulesDatabase &database,std::int64_t owner);
} // namespace civ5::runtime
