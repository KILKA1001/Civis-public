#pragma once

#include "civ5/runtime/bnw_turn_scheduler.hpp"

namespace civ5::runtime {

// CvGame::DoUpdateCachedWorldReligionTechProgress, immediately before
// updateScore in CvGame::doTurn. This is only that cache update; the enclosing
// game phase still requires score, religion, team and engine services.
std::int32_t compute_bnw_world_religion_tech_progress(
    const LiveSessionState &state,
    const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots);

void update_bnw_world_religion_tech_progress(
    LiveSessionState &state,
    const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots);

// Completes CvGame::doUpdateCacheOnTurn (empty in Expansion2), the religion
// cache update, and CvGame::updateScore only when its saved dirty flag is
// clear. A dirty score needs the complete ranking/replay implementation.
// The caller keeps technologies alive while the returned handler is in use.
BnwTurnHandler make_bnw_game_caches_and_score_handler(
    const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots);

// Binds the dirty-score branch to the selected BNW rules. The rules and
// catalogs must outlive the returned handler.
BnwTurnHandler make_bnw_game_caches_and_score_handler(
    const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines);

} // namespace civ5::runtime
