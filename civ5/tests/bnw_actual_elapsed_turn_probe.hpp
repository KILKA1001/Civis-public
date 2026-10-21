#pragma once
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include <stdexcept>
namespace civ5::validation {
inline void verify_actual_elapsed_turn_prefix(runtime::BnwSelectedStockLuaSourceOwner& source,
 const std::shared_ptr<runtime::BnwRetainedTurnCursor>& cursor,const ui::SelectedBnwPreviewHost& host){
 const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};
 check(cursor&&cursor->view().frontier==runtime::BnwRetainedTurnFrontier::game_turn_elapsed_increment_owner,"Actual elapsed regression requires reached EndTurnTimer return");
 const auto world=host.session().read_state_lease();const auto turn=world->setup.game_turn;const auto revision=world->ui_source_revision;const auto old=world->maintenance.elapsed_turns;
 const auto before=std::get<saves::Expansion2PrefixObservation>(world->world.game_state.prefix).scalars;
 const auto result=source.run_game_turn_elapsed_prefix(cursor);const auto after=std::get<saves::Expansion2PrefixObservation>(world->world.game_state.prefix).scalars;
 check(result.frontier==runtime::BnwRetainedTurnFrontier::game_turn_dynamic_turns_owner&&host.session().read_state_lease()==world&&world->setup.game_turn==turn&&world->ui_source_revision==revision+1&&
  world->maintenance.elapsed_turns==old+1&&after[1]==old+1,"Actual elapsed native paired increment/source return differs");
 for(std::size_t i=0;i<before.size();++i)if(i!=1)check(before[i]==after[i],"Actual elapsed increment changed other native GAME scalar");
 cursor->timer_owner()->require_current();
}
}
