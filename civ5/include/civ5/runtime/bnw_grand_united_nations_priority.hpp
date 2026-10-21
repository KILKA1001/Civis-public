#pragma once
#include "civ5/runtime/bnw_grand_first_priority.hpp"
namespace civ5::runtime {
struct BnwGrandUnitedNationsPriorityQuote {
 bool calculated=false,victory_disabled=false,no_active_leagues=false,starting_votes_pending=false;
 std::int64_t player=-1;
 std::optional<std::int32_t> category,minor_attacks,attack_weight,attack_term,votes_needed;
 std::optional<std::int32_t> active_leagues,era,diplomacy_flavor,flavor_term,league_prefix;
 std::optional<std::int32_t> votes_controlled,votes_delta,unallied_city_states;
 std::optional<std::int32_t> close_weight,comparison_weight,friendship,bonus,combat;
 std::string blocker;
};
// Source GetUnitedNationsPriority. Native disabled/no-active-league branches
// calculate real values. Active leagues stop at the mandatory first numerical
// votes producer, which also mutates vote_sources outside a league session.
// No cached member.votes replacement, caller completion flag or RNG permission.
BnwGrandUnitedNationsPriorityQuote quote_bnw_grand_united_nations_priority(
 const LiveSessionState &,const BnwHappinessSuffixRules &,const content::RulesDatabase &,
 const SessionCheckpointIdentity &,std::int64_t player);
}
