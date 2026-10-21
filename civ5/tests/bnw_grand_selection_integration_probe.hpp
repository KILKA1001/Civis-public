#pragma once
#include "civ5/runtime/bnw_game_activation_execution.hpp"
namespace civ5::tests {
// Test observations only. These counters cannot create a runtime authority.
struct BnwGrandSelectionProbeCounters {
 std::uint64_t lua_trace=0,lua_forks=0,lua_publishes=0,session_generation=0;
 bool can_continue=false;
 bool operator==(const BnwGrandSelectionProbeCounters&)const=default;
};
using BnwGrandSelectionProbeObserver=std::function<BnwGrandSelectionProbeCounters()>;
// Called on a mutable clone BEFORE fingerprint/seal/start. No cache permission.
void configure_bnw_grand_selection_zero_tie_sql(content::RulesDatabase&);
// Fixture callback only, before source categories/rolls/pressure. Requires the
// existing test's source met/self + science + Conquest inputs already prepared.
std::int32_t configure_bnw_grand_selection_zero_tie_inputs(runtime::LiveSessionState&,std::int64_t player);
// Reuse an existing successful pressure flow (actual active0/1). All rejected
// attempts must preserve its native prefix and RNG; never fabricate MinAge.
void probe_bnw_grand_selection_missing_minimum(
 runtime::BnwGameActivationExecution&,runtime::BnwTeamTechLuaEpoch&,
 const runtime::SavedContentProfile&,const content::RulesDatabase&,
 const runtime::BnwGrandWorldAverageRules&,const runtime::BnwHappinessSuffixRules&,
 const BnwGrandSelectionProbeObserver&);
// Consumes ONE genuine zero-tie pressure flow. Includes pre-entry stale guards,
// actual adoption+dirty-before-log, post seals, one-shot/replay and no-grant.
void probe_bnw_grand_selection_zero_tie(
 runtime::BnwGameActivationExecution&,runtime::BnwTeamTechLuaEpoch&,
 const runtime::SavedContentProfile&,const content::RulesDatabase&,
 const runtime::BnwGrandWorldAverageRules&,const runtime::BnwHappinessSuffixRules&,
 const BnwGrandSelectionProbeObserver&);
}
