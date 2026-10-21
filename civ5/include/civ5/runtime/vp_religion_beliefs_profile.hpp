#pragma once
#include "civ5/runtime/city_yields.hpp"

namespace civ5::runtime {
// VP's serialized belief lookup and holy-city fields have no fixed place in
// the shared religion record. Other GameCore profiles may attach their own data.
struct VpReligionBeliefsProfileState {
    std::int64_t belief_religion = -1, holy_x = -1, holy_y = -1;
    bool pantheon = false;
    // This lookup is independent of the ordered, possibly duplicate belief list.
    YieldValues lookup;
};

inline VpReligionBeliefsProfileState &ensure_vp_religion_beliefs(
    CityYieldReligionBeliefs &religion, content::TechnologyProfile profile) {
    if (profile != content::TechnologyProfile::vp542)
        throw CityYieldUnavailable("VP religion beliefs require VP GameCore profile");
    if (auto *state = religion.extensions.find<VpReligionBeliefsProfileState>())
        return *state;
    return religion.extensions.emplace<VpReligionBeliefsProfileState>();
}

inline VpReligionBeliefsProfileState &vp_religion_beliefs(CityYieldReligionBeliefs &religion) {
    auto *state = religion.extensions.find<VpReligionBeliefsProfileState>();
    if (!state) throw CityYieldUnavailable("Unresolved VP religion beliefs");
    return *state;
}

inline const VpReligionBeliefsProfileState &vp_religion_beliefs(
    const CityYieldReligionBeliefs &religion) {
    const auto *state = religion.extensions.find<VpReligionBeliefsProfileState>();
    if (!state) throw CityYieldUnavailable("Unresolved VP religion beliefs");
    return *state;
}
} // namespace civ5::runtime
