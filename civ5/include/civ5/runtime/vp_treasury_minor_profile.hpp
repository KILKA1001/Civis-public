#pragma once
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
// VP's saved city-state diplomacy and yield inputs. The ordinary ally ID is
// shared with BNW and stays on TreasuryPlayer.
struct VpTreasuryMinorProfile {
    std::int64_t trait_cs_modifier=0,cs_modifier=0,minor_type=-1,permanent_ally=-1;
    bool no_ally=false;
    YieldValues city_majorities;
    std::set<std::int64_t> friends;
};
inline VpTreasuryMinorProfile &ensure_vp_treasury_minor_profile(TreasuryPlayer &player){
    if(auto *state=player.profile_extensions.find<VpTreasuryMinorProfile>())return *state;
    return player.profile_extensions.emplace<VpTreasuryMinorProfile>();
}
inline const VpTreasuryMinorProfile &vp_treasury_minor_profile(const TreasuryPlayer &player){
    const auto *state=player.profile_extensions.find<VpTreasuryMinorProfile>();
    if(!state)throw TreasuryUnavailable("Missing VP treasury minor profile");
    return *state;
}
} // namespace civ5::runtime
