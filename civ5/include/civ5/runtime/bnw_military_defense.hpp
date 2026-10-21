#pragma once

#include "civ5/runtime/live_session.hpp"
#include <cstdint>
#include <optional>

namespace civ5::runtime {

// Inputs produced by the current CvMilitaryAI::UpdateBaseData and
// MilitaryAIHelpers::ComputeRecommendedNavySize, not by the save decoder.
struct BnwMilitaryDefenseBase {
    std::int64_t land_units=-1, naval_units=-1;
    std::int64_t mandatory_reserve=-1, recommended_military=-1;
    std::int64_t recommended_navy=-1;
};
struct BnwMilitaryDefenseSource {
    std::int64_t player=-1;
    bool base_data_current=false;
    bool navy_recommendation_current=false;
    std::optional<BnwMilitaryDefenseBase> base;
};
struct BnwMilitaryDefenseStates {
    // Expansion2 CvMilitaryAI.h: ENOUGH=0, NEUTRAL=1, NEEDED=2, CRITICAL=3.
    std::int64_t land=0, naval=0;
};

// Expansion2 CvMilitaryAI.cpp:2199 UpdateDefenseState. A checked, pure
// translation of its ordered comparisons; invalid/overflowing input rejects.
BnwMilitaryDefenseStates derive_bnw_military_defense(
    const BnwMilitaryDefenseBase &base);

// Saved BNW strategy history is insufficient to reconstruct these states.
// Require fresh, player-bound AI base data; never advance the session RNG.
BnwMilitaryDefenseStates bind_bnw_military_defense(
    const LiveSessionState &state,std::int64_t player,
    const BnwMilitaryDefenseSource &source);

} // namespace civ5::runtime
