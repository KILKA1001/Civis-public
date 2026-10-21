#pragma once
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include <stdexcept>
namespace civ5::validation {
inline void verify_actual_end_turn_timer_prefix(runtime::BnwSelectedStockLuaSourceOwner& source,
 const std::shared_ptr<runtime::BnwRetainedTurnCursor>& cursor,const ui::SelectedBnwPreviewHost& host){
 const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};
 check(cursor&&cursor->view().frontier==runtime::BnwRetainedTurnFrontier::game_turn_timer_reset_owner,"Actual EndTurnTimer regression requires reached GameplayTurnChanged return");
 const auto world=host.session().read_state_lease();const auto turn=world->setup.game_turn;const auto revision=world->ui_source_revision;
 const auto timer=cursor->timer_owner();const auto use=timer->current_use();const auto before=timer->read(use);const auto end_before=timer->read_end_turn_timer();
 const auto returned=source.run_game_turn_end_timer_prefix(cursor);const auto after=timer->read(use);const auto end_after=timer->read_end_turn_timer();
 check(returned.frontier==runtime::BnwRetainedTurnFrontier::game_turn_elapsed_increment_owner&&host.session().read_state_lease()==world&&world->setup.game_turn==turn&&world->ui_source_revision==revision,
  "Actual EndTurnTimer runtime reset changed world/turn or wrong next frontier");
 check(end_before.reset_occurrence<UINT64_MAX&&end_after.reset_occurrence==end_before.reset_occurrence+1&&end_after.running&&end_after.semaphore==0&&end_after.stopped_elapsed==0.0f&&end_after.frequency==before.frequency,
  "Actual EndTurnTimer did not issue separate OS Start/semaphore reset");
 check(before.current_start==after.current_start&&before.game_turn_start==after.game_turn_start&&before.operation==after.operation&&before.reset==after.reset&&before.pause_delta==after.pause_delta&&
  before.current_stopped_elapsed==after.current_stopped_elapsed&&before.game_turn_stopped_elapsed==after.game_turn_stopped_elapsed,
  "Actual EndTurnTimer reset restarted current/game timers");
}
template<class Timer,class Cursor>concept PublicEndTurnTimerAck=requires(Timer& timer,const Cursor& cursor){timer.reset_actual_end_turn_timer(cursor);};
static_assert(!PublicEndTurnTimerAck<runtime::BnwGameTurnTimerOwner,runtime::BnwRetainedTurnCursor>);
}
