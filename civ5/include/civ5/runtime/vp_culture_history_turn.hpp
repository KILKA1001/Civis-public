#pragma once
#include "civ5/runtime/research_context.hpp"

namespace civ5::runtime {
// CvPlayerCulture's saved VP history is owned by its GameCore module. Values
// are widened after source decoding; source i32 fields are not host limits.
struct VpCultureHistoryPlayer {
    std::int64_t lifetime_times100=0,last_lifetime_times100=0,last_cpt_times100=0;
    std::int64_t balance_times100=0,next_policy_cost=0,anarchy_turns=0;
};
struct VpCultureHistoryProfileState {
    std::map<std::int64_t,VpCultureHistoryPlayer> players;
};
VpCultureHistoryProfileState &ensure_vp_culture_history_profile(ResearchContext &world);
const VpCultureHistoryProfileState *find_vp_culture_history_profile(const ResearchContext &world);
void restore_vp_culture_history_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world);
// First culture operation in VP CvPlayer::doTurn, after vassal tax and before
// culture income. No policy cost, anarchy or income is applied by this step.
std::int64_t do_vp_culture_history_after_gold(ResearchContext &world,std::int64_t player);
struct VpCultureAccrualOptions {
    bool no_policies=false,timer_enabled=false,minor=false;
};
struct VpCultureAccrualResult {
    std::int64_t quoted_times100=0,credited_times100=0;
    std::int64_t balance_before_times100=0,balance_after_times100=0;
    std::int64_t lifetime_before_times100=0,lifetime_after_times100=0;
    bool timer_blocked=false;
};
// Apply a complete, externally calculated culture-per-turn quote. The VP
// formula that produces the quote is a separate required step; this function
// never substitutes a partial city-only amount for it.
VpCultureAccrualResult apply_vp_culture_income(ResearchContext &world,std::int64_t player,
    std::int64_t quoted_times100,VpCultureAccrualOptions options);
} // namespace civ5::runtime
