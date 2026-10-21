#pragma once
#include "civ5/runtime/research_context.hpp"
#include "civ5/runtime/player_research.hpp"
#include <cstdint>
#include <map>
#include <vector>

namespace civ5::runtime {
// Inputs owned by VP's player-policy GameCore. Shared policy ownership stays
// in ResearchPlayerState; only VP's free-policy flags and cost counters live here.
struct VpPolicyCostSavedPlayer {
    std::int64_t saved_num_policies=0,free_policies=0,free_policies_ever=0,free_tenets=0;
    std::int64_t policy_cost_modifier=0,city_cost_discount=0;
    std::vector<std::uint8_t> free_policy,branch_unlocked;
};
struct VpPolicyCostSavedProfileState {
    std::int64_t world_city_modifier_times100=0;
    std::map<std::int64_t,VpPolicyCostSavedPlayer> players;
};
VpPolicyCostSavedProfileState &ensure_vp_policy_cost_saved_profile(ResearchContext &world);
const VpPolicyCostSavedProfileState *find_vp_policy_cost_saved_profile(const ResearchContext &world);
void restore_vp_policy_cost_saved_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world);
struct VpPaidPolicyCount {
    std::int64_t owned=0,free=0,dummy=0,paid=0;
};
// CvPlayerPolicies::GetNumPoliciesOwned(false, true): read selected SQL rule
// metadata and live shared ownership. Source branch openers are excluded.
VpPaidPolicyCount vp_paid_policy_count(const ResearchContext &world,
    const content::RulesDatabase &database,std::int64_t player);
struct VpPolicyCityModifierQuote {
    std::int64_t effective_cities=0,policy_cities=0;
    std::int64_t base_times100=0,scaling_times100=0,pairs=0,total_times100=0;
};
// VP 5.4.2 GetPolicyCityModifierTimes100, using the shared live city roster
// and the saved world-size value. BALANCE_VP selects the first-city branch.
VpPolicyCityModifierQuote vp_policy_city_modifier_times100(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::GlobalDefineCache &defines,bool balance_vp,std::int64_t player);
struct VpPolicyCostSubtotal {
    std::int64_t counted_policies=0,charged_policies=0,exponent_input=0;
    std::int64_t exponent_cost=0,base_cost=0,after_cities=0;
    std::int64_t after_player_modifier=0,speed_percent=0,after_speed=0;
};
// Source-ordered GetNextPolicyCost prefix through GameSpeed.CulturePercent.
// Later multipliers are applied by the ideology/final quote functions.
VpPolicyCostSubtotal vp_policy_cost_subtotal(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    bool balance_vp,std::int64_t player);
struct VpPolicyIdeologyQuote {
    std::int64_t tier1=0,tier2=0,tier3=0;
    std::int64_t penalty_percent=0,after_ideology=0;
};
// VP 5.4.2 tenet multiplier, applied after the game-speed subtotal. SQL
// supplies Levels and PurchaseByLevel; ownership comes from common state.
VpPolicyIdeologyQuote vp_policy_ideology_cost(
    const ResearchContext &world,const content::RulesDatabase &database,
    const content::GlobalDefineCache &defines,bool balance_vp,
    std::int64_t player,std::int64_t cost_after_speed);
struct VpPolicyFinalQuote {
    bool major=false,human_for_handicap=false;
    std::int64_t player_handicap=-1,game_handicap=-1;
    std::int64_t player_percent=0,player_era_factor=0,after_player_handicap=0;
    std::int64_t ai_percent=0,ai_era_factor=0,after_ai_handicap=0;
    std::int64_t catchup_modifier=0,higher_players=0,possible_players=0;
    std::int64_t catchup_factor=100,after_catchup=0;
    std::int64_t visible_divisor=0,cost=0;
};
// Completes GetNextPolicyCost after the ideology multiplier using selected
// HandicapInfos, live major-civ ownership, and VP's handicap custom option.
VpPolicyFinalQuote vp_policy_final_cost(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    bool human_uses_ai_handicap,std::int64_t player,
    std::int64_t cost_after_ideology,std::int64_t actual_policy_count);
struct VpPolicyCostUpdateResult {
    std::int64_t before=0,after=0;
};
// CvPlayer::DoUpdateNextPolicyCost, after culture accrual in doTurn. The
// profile-owned saved field is updated only on the caller's disposable world.
VpPolicyCostUpdateResult do_vp_policy_cost_after_culture(
    ResearchContext &world,std::int64_t player,const VpPolicyFinalQuote &quote);
} // namespace civ5::runtime
