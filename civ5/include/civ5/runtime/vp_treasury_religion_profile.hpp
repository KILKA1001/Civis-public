#pragma once
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
// These saved PlayerReligions fields belong to the VP GameCore. Other profiles
// can keep their own religion ownership model without changing TreasuryPlayer.
struct VpTreasuryReligionProfile {
    std::int64_t state_religion=-1;
    bool owns_state_religion=false;
};
inline VpTreasuryReligionProfile &ensure_vp_treasury_religion_profile(TreasuryPlayer &player){
    if(auto *state=player.profile_extensions.find<VpTreasuryReligionProfile>())return *state;
    return player.profile_extensions.emplace<VpTreasuryReligionProfile>();
}
inline const VpTreasuryReligionProfile &vp_treasury_religion_profile(const TreasuryPlayer &player){
    const auto *state=player.profile_extensions.find<VpTreasuryReligionProfile>();
    if(!state)throw TreasuryUnavailable("Missing VP treasury religion profile");
    return *state;
}
} // namespace civ5::runtime
