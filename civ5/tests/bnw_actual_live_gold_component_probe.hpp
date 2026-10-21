#pragma once
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include <string_view>
namespace civ5::validation {
// Actual imported Source and its physical session are supplied by the real
// bootstrap harness. No fabricated world, listener, receipt or numeric answer.
inline void verify_actual_inactive_live_gold_component(runtime::BnwSelectedStockLuaSourceOwner& source,
 const runtime::LiveSession& session,std::int64_t actor){
 const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 const auto& before=session.state();const auto* world=&before;const auto generation=session.state_generation();
 require(runtime::detail::bnw_game_active_player_read(before)!=actor,"Live Gold oracle needs actual inactive source actor");
 const auto revision=before.ui_source_revision;const auto turn=before.setup.game_turn;
 const auto map=before.random.map.state(),game=before.random.gameplay.state();
 const auto receipt=source.begin_imported_player_gold_component(actor);
 require(bool(receipt),"Actual Live Gold receipt missing");const auto view=receipt->view();const auto& after=session.state();
 require(view.phase==runtime::BnwLivePlayerGoldPhase::native_returned&&view.standalone_component&&view.paired_stores>0,
  "Live Gold oracle needs actual changed native component; no phase/frontier synthetic PASS");
 require(&after==world&&session.state_generation()==generation&&after.setup.game_turn==turn&&
  after.random.map.state()==map&&after.random.gameplay.state()==game&&!after.player_gold_publication&&
  view.revision_before==revision&&view.revision_after==revision+view.paired_stores&&after.ui_source_revision==view.revision_after,
  "Actual Live Gold replaced world/VM clock/RNG or lost exact per-pair adoption");
 const auto& owned=after.treasury.players.at(actor);const auto& costs=after.maintenance.players.at(actor);
 const auto& raw=after.world.all_players->players.at(static_cast<std::size_t>(actor)).administration.blocks.at(1);
 const auto scalar=[&](std::string_view name){const saves::NamedSaveInteger* field=nullptr;
  for(const auto& row:raw.fields)if(row.name&&std::string_view(row.name)==name){require(!field,"Live Gold raw field duplicate");field=&row;}
  require(field,"Live Gold raw field missing");return field->value;};
 const auto array=[&](std::string_view name)->const std::vector<std::int64_t>&{const saves::PlotArrayObservation* field=nullptr;
  for(const auto& row:raw.arrays)if(row.name&&std::string_view(row.name)==name){require(!field,"Live Gold raw array duplicate");field=&row;}
  require(field&&field->columns==1,"Live Gold raw array missing");return field->values;};
 require(scalar("m_iGold")==owned.balance_times100&&scalar("m_iLifetimeGrossGoldIncome")==owned.lifetime_gross_gold&&
  scalar("m_iExpensePerTurnUnitMaintenance")==costs.cached_unit_expense&&scalar("m_iExpensePerTurnUnitSupply")==costs.cached_supply_expense&&
  scalar("m_GoldBalanceForTurnTimes100_wire_count")==static_cast<std::int64_t>(owned.balance_history_times100.size())&&
  scalar("m_GoldChangeForTurnTimes100_wire_count")==static_cast<std::int64_t>(owned.income_history_times100.size())&&
  array("m_GoldBalanceForTurnTimes100")==owned.balance_history_times100&&array("m_GoldChangeForTurnTimes100")==owned.income_history_times100,
  "Actual canonical Live Gold raw/owned fields/history/counts not paired");
 bool replay_refused=false;try{(void)source.begin_imported_player_gold_component(actor);}catch(const std::runtime_error&){replay_refused=true;}
 require(replay_refused&& &session.state()==world&&session.state().ui_source_revision==view.revision_after,
  "Actual Live Gold component replay was allowed/changed world");
}
}
