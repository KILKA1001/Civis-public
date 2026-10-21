#pragma once
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/live_session.hpp"
#include <array>
#include <stdexcept>
namespace civ5::validation {
// Integration fixture: real imported canonical session, selected stock owner,
// actual VM accumulator and its actual result. No synthetic listener/value.
inline std::array<std::optional<std::int64_t>,2> verify_actual_treasury_same_vm_income(
 runtime::BnwSelectedStockLuaSourceOwner& source,const runtime::LiveSession& session,std::int64_t actor){
 const auto& before=session.state();const auto* address=&before;
 const auto revision=before.ui_source_revision;const auto turn=before.setup.game_turn;
 const auto map_rng=before.random.map.state(),game_rng=before.random.gameplay.state();
 std::array<std::optional<std::int64_t>,2> actual;
 for(auto& returned:actual){
  returned=source.query_treasury_founder_benefits(actor,session.state());
  const auto& after=session.state();
  if(&after!=address||after.ui_source_revision!=revision||after.setup.game_turn!=turn||
   after.random.map.state()!=map_rng||after.random.gameplay.state()!=game_rng)
   throw std::runtime_error("Actual Treasury income fixture changed canonical world/clock/RNG");
 }
 // Values are real callback results; no no-listeners/founded-religion answer
 // is manufactured. Both invocations must pass actual Source/VM currentness.
 return actual;
}
}
