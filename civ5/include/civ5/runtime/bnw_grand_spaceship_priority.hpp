#pragma once
#include "civ5/runtime/bnw_grand_first_priority.hpp"
namespace civ5::runtime {
struct BnwGrandSpaceshipPriorityQuote {
 bool calculated=false,victory_disabled=false,apollo_info_absent=false;
 std::int64_t player=-1;
 std::optional<std::int32_t> category,science_flavor,era,flavor_term;
 std::optional<std::int32_t> apollo_project,apollo_count,apollo_weight;
 std::size_t project_rows_visited=0;
 std::string blocker;
};
// Actual source GetSpaceshipPriority, independent read-only category leaf.
// Native GetBase slot2 is reused separately. No earlier slots, RNG or write
// are authorized by a caller-authored or completed pure quote.
BnwGrandSpaceshipPriorityQuote quote_bnw_grand_spaceship_priority(
 const LiveSessionState &,const BnwHappinessSuffixRules &,
 const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t player);
}
