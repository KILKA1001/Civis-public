#pragma once
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include <limits>
#include <string_view>
namespace civ5::validation {
// Both probes require naturally insolvent installed/imported scenarios. They
// do not change balances, roster, cities, service flags or RNG to obtain a branch.
inline void verify_actual_imported_live_gold_deficit(runtime::BnwSelectedStockLuaSourceOwner& source,
 const runtime::LiveSession& session,std::int64_t actor,const std::shared_ptr<const ui::LiveUiHost>& actual_ui,
 const std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>& actual_core,bool expect_random_frontier){
 const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 const auto& before=session.state();const auto* physical=&before;const auto generation=session.state_generation();
 require(actor>=0&&actor<64&&actual_core&&before.world.all_players&&before.world.all_players->players.size()==64&&
  !before.player_gold_publication,"Deficit probe needs actual original-import session/Core with no prior Gold publication");
 require(runtime::detail::bnw_game_active_player_read(before)!=actor||actual_ui,
  "Active deficit probe needs genuine canonical UI for actual4/6/11 setters");
 const auto& raw=before.world.all_players->players.at(static_cast<std::size_t>(actor));
 require(raw.units.header.wire_unit_count>=0&&static_cast<std::uint64_t>(raw.units.header.wire_unit_count)==raw.units.units.size()&&
  raw.cities.cities.size()<=static_cast<std::size_t>(INT32_MAX),"Deficit independent raw roster/count unavailable");
 std::int32_t combat=0;
 for(const auto& unit:raw.units.units){const saves::UnitCoreField* base=nullptr;
  for(const auto& field:unit.core.fields)if(field.name&&std::string_view(field.name)=="m_iBaseCombat"){
   require(!base,"Deficit oracle duplicate native BaseCombat");base=&field;}
  require(base&&base->encoding==saves::UnitCoreEncoding::i32&&base->values.size()==1&&base->bytes.empty()&&
   base->values.front()>=INT32_MIN&&base->values.front()<=INT32_MAX,"Deficit oracle requires typed native BaseCombat");
  if(base->values.front()>0){require(combat<INT32_MAX,"Deficit oracle combat overflow");++combat;}
 }
 const auto cities=static_cast<std::int32_t>(raw.cities.cities.size());
 require((combat>cities)==expect_random_frontier,"Deficit probe save does not naturally select requested SDK branch");
 const auto rev=before.ui_source_revision;const auto turn=before.setup.game_turn;
 const auto map=before.random.map.state(),game=before.random.gameplay.state();
 const auto next_seed=(game.seed*1103515245ULL+12345ULL)&0xffffffffULL;
 const auto native_roll=(((next_seed>>16)&0xffffULL)*100ULL)/65536ULL;
 const bool military_frontier=expect_random_frontier&&native_roll<50;
 const auto& treasury=before.treasury.players.at(actor);
 const auto old_lifetime=treasury.lifetime_gross_gold;
 const auto old_balances=treasury.balance_history_times100,old_incomes=treasury.income_history_times100;
 std::shared_ptr<const runtime::BnwLivePlayerGoldPending> receipt;bool failed=false;
 try{receipt=source.begin_imported_player_gold_component(actor,actual_ui,actual_core);}
 catch(const runtime::BnwLivePlayerGoldSourceFailure& failure){failed=true;receipt=failure.receipt();}
 require(bool(receipt),"Deficit original call did not issue physical retained Pending");
 const auto view=receipt->view();const auto& after=session.state();
 require(view.deficit_scan_returned&&view.deficit_combat_units==combat&&view.deficit_cities==cities&&
  view.deficit_no_disband_returned==!military_frontier,"Deficit actual branch differs from independent native raw oracle");
 require(&after==physical&&session.state_generation()==generation&&after.setup.game_turn==turn&&after.random.map.state()==map&&
  rev<=UINT64_MAX-view.paired_stores&&after.ui_source_revision==rev+view.paired_stores&&
  (!expect_random_frontier?after.random.gameplay.state()==game:
   (view.deficit_rng_stored&&view.deficit_rng_returned&&view.deficit_rng_civis_trace_retained&&
    view.deficit_rng_original_diagnostic_parity_unknown&&view.deficit_rng_value==native_roll&&
    view.deficit_rng_before==game&&view.deficit_rng_after==after.random.gameplay.state()&&game.calls<UINT32_MAX&&
    after.random.gameplay.state().seed==next_seed&&after.random.gameplay.state().calls==game.calls+1&&
    after.random.gameplay.state().resets==game.resets))&&
  view.revision_after==after.ui_source_revision,"Deficit query unexpectedly changed physical world/clock/RNG/revision");
 const auto& t=after.treasury.players.at(actor);const auto& m=after.maintenance.players.at(actor);
 require(t.balance_times100==0,"Deficit fixture did not actually enter native insolvent balance clamp");
 const auto& admin=after.world.all_players->players.at(static_cast<std::size_t>(actor)).administration.blocks.at(1);
 const auto field=[&](std::string_view name){const saves::NamedSaveInteger* found=nullptr;
  for(const auto& row:admin.fields)if(row.name&&std::string_view(row.name)==name){require(!found,"Deficit raw duplicate");found=&row;}
  require(found,"Deficit raw scalar absent");return found->value;};
 const auto array=[&](std::string_view name)->const std::vector<std::int64_t>&{const saves::PlotArrayObservation* found=nullptr;
  for(const auto& row:admin.arrays)if(row.name&&std::string_view(row.name)==name){require(!found,"Deficit raw duplicate array");found=&row;}
  require(found&&found->columns==1,"Deficit raw history array absent");return found->values;};
 require(field("m_iGold")==t.balance_times100&&field("m_iLifetimeGrossGoldIncome")==t.lifetime_gross_gold&&
  field("m_iExpensePerTurnUnitMaintenance")==m.cached_unit_expense&&field("m_iExpensePerTurnUnitSupply")==m.cached_supply_expense&&
  field("m_GoldBalanceForTurnTimes100_wire_count")==static_cast<std::int64_t>(t.balance_history_times100.size())&&
  field("m_GoldChangeForTurnTimes100_wire_count")==static_cast<std::int64_t>(t.income_history_times100.size())&&
  array("m_GoldBalanceForTurnTimes100")==t.balance_history_times100&&array("m_GoldChangeForTurnTimes100")==t.income_history_times100,
  "Deficit native return/frontier exposes stale raw/owned scalar/history/count mirror");
 if(military_frontier){
  require(failed&&view.phase==runtime::BnwLivePlayerGoldPhase::deficit_military_owner_required&&after.player_gold_publication&&
   after.player_gold_publication->uncertain&&t.lifetime_gross_gold==old_lifetime&&
   t.balance_history_times100==old_balances&&t.income_history_times100==old_incomes,
   "Actual roll<50 bypassed MilitaryAI or executed original gross/history suffix before DoDeficit returned");
 }else{
  require(!failed&&view.phase==runtime::BnwLivePlayerGoldPhase::native_returned&&!after.player_gold_publication,
   "Actual combat<=cities or native roll>=50 did not genuinely return to original DoGold suffix");
 }
 bool refused=false;try{(void)source.begin_imported_player_gold_component(actor,actual_ui,actual_core);}
 catch(const std::runtime_error&){refused=true;}
 require(refused&&session.state().ui_source_revision==view.revision_after&&
  session.state().random.gameplay.state()==(expect_random_frontier?view.deficit_rng_after:game),
  "Deficit same actor/turn failure or return allowed replay");
}
inline void verify_actual_imported_gold_no_disband(runtime::BnwSelectedStockLuaSourceOwner& source,
 const runtime::LiveSession& session,std::int64_t actor,const std::shared_ptr<const ui::LiveUiHost>& actual_ui,
 const std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>& actual_core){
 verify_actual_imported_live_gold_deficit(source,session,actor,actual_ui,actual_core,false);
}
inline void verify_actual_imported_gold_random_frontier(runtime::BnwSelectedStockLuaSourceOwner& source,
 const runtime::LiveSession& session,std::int64_t actor,const std::shared_ptr<const ui::LiveUiHost>& actual_ui,
 const std::shared_ptr<const runtime::BnwNativeUnitAllocatorCore>& actual_core){
 verify_actual_imported_live_gold_deficit(source,session,actor,actual_ui,actual_core,true);
}
}
