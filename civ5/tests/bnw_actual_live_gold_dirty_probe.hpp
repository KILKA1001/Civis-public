#pragma once
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include <string_view>
namespace civ5::validation {
inline void verify_actual_active_live_gold_dirty_return(runtime::BnwSelectedStockLuaSourceOwner& source,
 const runtime::LiveSession& session,std::int64_t actor,const std::shared_ptr<const ui::LiveUiHost>& actual_ui){
 const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 const auto& before=session.state();const auto* world=&before;const auto generation=session.state_generation();
 require(actual_ui&&runtime::detail::bnw_game_active_player_read(before)==actor&&!before.player_gold_publication,
  "Gold UI natural fixture needs genuine active imported actor/current actual UI");
 const auto turn=before.setup.game_turn,old_balance=before.treasury.players.at(actor).balance_times100;
 const auto rev=before.ui_source_revision;const auto map=before.random.map.state(),game=before.random.gameplay.state();
 const auto receipt=source.begin_imported_player_gold_component(actor,actual_ui);
 require(bool(receipt),"Gold UI actual issued owner absent");const auto view=receipt->view();const auto& after=session.state();
 require(view.phase==runtime::BnwLivePlayerGoldPhase::native_returned&&view.dirty_setters==3&&
  !view.required_dirty&&view.dirty_client_revisions[0]<UINT64_MAX-2&&
  view.dirty_client_revisions[1]==view.dirty_client_revisions[0]+1&&view.dirty_client_revisions[2]==view.dirty_client_revisions[0]+2,
  "Gold UI genuine original4->6->11 setters did not all return in exact order");
 require(&after==world&&session.state_generation()==generation&&after.setup.game_turn==turn&&after.random.map.state()==map&&
  after.random.gameplay.state()==game&&after.ui_source_revision==rev+view.paired_stores&&!after.player_gold_publication&&
  after.treasury.players.at(actor).balance_times100!=old_balance,
  "Gold UI fixture did not execute genuine changed balance or altered unrelated session/clock/RNG");
 const auto& t=after.treasury.players.at(actor);const auto& m=after.maintenance.players.at(actor);
 const auto& raw=after.world.all_players->players.at(static_cast<std::size_t>(actor)).administration.blocks.at(1);
 const auto field=[&](std::string_view name){const saves::NamedSaveInteger* found=nullptr;
  for(const auto& row:raw.fields)if(row.name&&std::string_view(row.name)==name){require(!found,"Gold UI raw duplicate");found=&row;}
  require(found,"Gold UI raw field absent");return found->value;};
 const auto array=[&](std::string_view name)->const std::vector<std::int64_t>&{const saves::PlotArrayObservation* found=nullptr;
  for(const auto& row:raw.arrays)if(row.name&&std::string_view(row.name)==name){require(!found,"Gold UI raw duplicate array");found=&row;}
  require(found&&found->columns==1,"Gold UI raw array absent");return found->values;};
 require(field("m_iGold")==t.balance_times100&&field("m_iLifetimeGrossGoldIncome")==t.lifetime_gross_gold&&
  field("m_iExpensePerTurnUnitMaintenance")==m.cached_unit_expense&&field("m_iExpensePerTurnUnitSupply")==m.cached_supply_expense&&
  field("m_GoldBalanceForTurnTimes100_wire_count")==static_cast<std::int64_t>(t.balance_history_times100.size())&&
  field("m_GoldChangeForTurnTimes100_wire_count")==static_cast<std::int64_t>(t.income_history_times100.size())&&
  array("m_GoldBalanceForTurnTimes100")==t.balance_history_times100&&array("m_GoldChangeForTurnTimes100")==t.income_history_times100,
  "Gold UI completed native return still exposes stale raw scalar/history/count mirror");
 bool refused=false;try{(void)source.begin_imported_player_gold_component(actor,actual_ui);}catch(const std::runtime_error&){refused=true;}
 require(refused&&session.state().ui_source_revision==view.revision_after,"Gold UI same actor/turn replay was allowed");
}
}
