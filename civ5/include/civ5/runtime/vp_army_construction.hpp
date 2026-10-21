#pragma once
#include "civ5/runtime/research_context.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <map>
#include <vector>

namespace civ5::runtime {
enum class VpArmyType : std::int64_t { land=0, naval=1, combined=2 };
struct VpArmySlot {
    std::int64_t unit_id=-1;
    bool required=false;
};
struct VpArmyConstruction {
    std::int64_t id=0, owner=0, type=0;
    std::size_t source_index=0;
    std::vector<VpArmySlot> slots;
};
struct VpArmyConstructionProfileState {
    std::map<std::int64_t,std::vector<VpArmyConstruction>> players;
};
VpArmyConstructionProfileState &ensure_vp_army_construction_profile(ResearchContext &world);
const VpArmyConstructionProfileState *find_vp_army_construction_profile(const ResearchContext &world);
void restore_vp_army_construction_profile(const saves::SavedSessionSnapshot &snapshot,
                                          ResearchContext &world);
// Release-5.4.2 CvMilitaryAI::IsBuildingArmy/GetOpenSlots(true): any army of
// the selected type with a required slot whose unit ID is exactly -1.
bool vp_is_building_army(const ResearchContext &world,std::int64_t owner,VpArmyType type);
} // namespace civ5::runtime
