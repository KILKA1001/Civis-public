#include "civ5/runtime/bnw_military_defense.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
void require_count(std::int64_t count) {
    require(count>=0 && count<=std::numeric_limits<std::int32_t>::max(),
            "BNW military defense count is outside the SDK int range");
}
std::int64_t five_quarters(std::int64_t count) {
    // The SDK multiplies signed int before division. Reject values for which
    // its expression would overflow rather than inventing an outcome.
    require(count<=std::numeric_limits<std::int32_t>::max()/5,
            "BNW military defense threshold overflows SDK int");
    return count*5/4;
}
} // namespace

BnwMilitaryDefenseStates derive_bnw_military_defense(
    const BnwMilitaryDefenseBase &base) {
    require_count(base.land_units);
    require_count(base.naval_units);
    require_count(base.mandatory_reserve);
    require_count(base.recommended_military);
    require_count(base.recommended_navy);
    const auto land_neutral_limit=five_quarters(base.recommended_military);
    const auto naval_neutral_limit=five_quarters(base.recommended_navy);
    BnwMilitaryDefenseStates states;
    if (base.land_units<base.mandatory_reserve) states.land=3;
    else if (base.land_units<base.recommended_military) states.land=2;
    else if (base.land_units<land_neutral_limit) states.land=1;
    else states.land=0;
    if (base.naval_units<=base.recommended_navy/2) states.naval=3;
    else if (base.naval_units<=base.recommended_navy) states.naval=2;
    else if (base.naval_units<=naval_neutral_limit) states.naval=1;
    else states.naval=0;
    return states;
}

BnwMilitaryDefenseStates bind_bnw_military_defense(
    const LiveSessionState &state,std::int64_t player,
    const BnwMilitaryDefenseSource &source) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    require(state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.setup.profile==content::TechnologyProfile::bnw,
            "BNW military defense world/profile mismatch");
    require(player>=0 && source.player==player && source.base_data_current &&
            source.navy_recommendation_current && source.base,
            "BNW military defense requires current player-bound AI base data");
    bool found=false;
    for (const auto &candidate:state.world.all_players->players)
        if (candidate.saved_player_id==player) {
            require(!found,"Duplicate BNW military defense player");
            found=true;
        }
    require(found,"BNW military defense player unavailable");
    return derive_bnw_military_defense(*source.base);
}

} // namespace civ5::runtime
