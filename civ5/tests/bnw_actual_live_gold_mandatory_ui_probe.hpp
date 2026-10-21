#pragma once
#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include <string_view>
namespace civ5::validation {
// A REAL active-player/changed-balance fixture only. The bootstrap harness
// chooses actual imported data that reaches this SDK call; no input is changed
// to manufacture its branch. Unknown prerequisites must fail the test.
inline void verify_actual_live_gold_missing_ui_receipt(runtime::BnwSelectedStockLuaSourceOwner& source,
 const runtime::LiveSession& session,std::int64_t actor){
 const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 const auto& before=session.state();const auto* world=&before;const auto generation=session.state_generation();
 require(runtime::detail::bnw_game_active_player_read(before)==actor&&!before.player_gold_publication,
  "Live Gold mandatory UI oracle requires actual active source actor and no pending phase");
 const auto revision=before.ui_source_revision;const auto turn=before.setup.game_turn;
 const auto map=before.random.map.state(),game=before.random.gameplay.state();
 const auto old=before.treasury.players.at(actor); // Gold-target image only
 std::shared_ptr<const runtime::BnwLivePlayerGoldPending> receipt;
 try{(void)source.begin_imported_player_gold_component(actor);}
 catch(const runtime::BnwLivePlayerGoldSourceFailure& actual){receipt=actual.receipt();}
 require(bool(receipt),"Actual reached missing UI did not retain typed issued Gold receipt");
 const auto view=receipt->view();const auto& after=session.state();const auto& owned=after.treasury.players.at(actor);
 require(view.phase==runtime::BnwLivePlayerGoldPhase::dirty_owner_required&&
  view.required_dirty==runtime::TreasuryDirty::misc_buttons&&view.paired_stores>0&&view.standalone_component,
  "Actual Gold did not stop at first native MiscButtons4 callback; missing gate cannot PASS");
 require(&after==world&&session.state_generation()==generation&&after.setup.game_turn==turn&&
  after.random.map.state()==map&&after.random.gameplay.state()==game&&
  after.ui_source_revision==revision+view.paired_stores&&view.revision_after==after.ui_source_revision&&
  owned.balance_times100!=old.balance_times100&&owned.lifetime_gross_gold==old.lifetime_gross_gold&&
  owned.balance_history_times100==old.balance_history_times100&&owned.income_history_times100==old.income_history_times100,
  "Actual missing UI lost prior balance store or executed unreachable gross/history suffix");
 require(after.player_gold_publication&&after.player_gold_publication->player==actor&&
  after.player_gold_publication->uncertain&&after.player_gold_publication->dirty_kinds.empty()&&
  after.player_gold_publication->next_step==0,"Actual missing UI forged publication/delivery or cleared partial phase");
 const auto& raw=after.world.all_players->players.at(static_cast<std::size_t>(actor)).administration.blocks.at(1);
 const auto field=[&](std::string_view name){const saves::NamedSaveInteger* target=nullptr;
  for(const auto& row:raw.fields)if(row.name&&std::string_view(row.name)==name){require(!target,"Live Gold failure raw field duplicated");target=&row;}
  require(target,"Live Gold failure raw field missing");return target->value;};
 const auto& costs=after.maintenance.players.at(actor);
 require(field("m_iGold")==owned.balance_times100&&field("m_iLifetimeGrossGoldIncome")==owned.lifetime_gross_gold&&
  field("m_iExpensePerTurnUnitMaintenance")==costs.cached_unit_expense&&field("m_iExpensePerTurnUnitSupply")==costs.cached_supply_expense,
  "Actual missing UI exposed stale raw mirror to native callback");
 bool refused=false;try{(void)source.begin_imported_player_gold_component(actor);}catch(const std::runtime_error&){refused=true;}
 require(refused&&session.state().ui_source_revision==view.revision_after&&receipt->view().phase==view.phase,
  "Actual failed Gold replay changed native history/world");
}
}
