#pragma once
#include "civ5/runtime/bnw_unit_death_xp_quote.hpp"

namespace civ5::runtime {

struct BnwUnitCaptureDeathQuote {
    BnwDeathXpQuote xp;
    std::int64_t old_player=-1,original_owner=-1,old_type=-1;
    std::int64_t capturing_player=-1,capture_type=-1;
    std::int64_t x=-1,y=-1,religion=-1,religious_strength=0,
                 spreads_left=0;
    bool embarked=false,captured_as_is=false,capture_valid=false;
    // The quote is observational only. Earlier kill effects, any XP setter,
    // and the following setXY/removeUnit/resource/roster effects are unported.
    bool can_commit_kill=false;
};

// Expansion2 CvUnit.cpp:1439-1493. This proves only the NO_PLAYER capture
// branch of getCaptureDefinition; capture conversion needs civilization rules,
// AI and unit creation. No gameplay state is changed.
BnwUnitCaptureDeathQuote quote_bnw_unit_capture_death_no_player(
    const saves::SavedSessionSnapshot &source,
    const BnwDeathXpQuote &xp,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules);

} // namespace civ5::runtime
