#pragma once
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include <stdexcept>
namespace civ5::validation {
// Invoke with the genuine actual stock source/cursor that reached the NET
// returned frontier through its ordinary producers. This does not fabricate
// the earlier turn phases, a Lua lease, source receipt or renderer readiness.
inline void verify_actual_game_turn_clock_prefix(runtime::BnwSelectedStockLuaSourceOwner& source,
 const std::shared_ptr<runtime::BnwRetainedTurnCursor>& cursor,const ui::SelectedBnwPreviewHost& host){
 const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 check(bool(cursor),"Actual clock regression cursor absent");
 const auto before=host.session().read_state_lease();const auto& saved=host.session().source();
 check(saved.source.header.build=="1.0.3.279 (403694)"&&saved.source.header.mods.empty()&&saved.snapshot.pregame&&
  before->world.pregame&&before->world.map_header.guid==saved.snapshot.map_header.guid,
  "Actual clock regression requires imported compatible BNW source");
 check(cursor->view().frontier==runtime::BnwRetainedTurnFrontier::game_turn_paired_clock_store_owner,
  "Actual clock regression prior mandatory NET/turn frontier is not reached");
 const auto old_turn=before->setup.game_turn;const auto old_revision=before->ui_source_revision;
 const auto timer=cursor->timer_owner();const auto use=timer->current_use();const auto marks=timer->read(use);
 const auto result=source.run_game_turn_clock_prefix(cursor);timer->require_current();const auto after_marks=timer->read(use);
 const auto after=host.session().read_state_lease();
 check(before==after&&after->setup.game_turn==old_turn+1&&after->world.pregame->archive.integers.at("game_turn")==old_turn+1&&
  after->ui_source_revision==old_revision+1&&result.frontier==runtime::BnwRetainedTurnFrontier::game_turn_ui_dirty_owner,
  "Actual clock regression did not earn exact paired native return");
 check(cursor->timer_owner()==timer&&marks.current_start==after_marks.current_start&&marks.game_turn_start==after_marks.game_turn_start&&
  marks.frequency==after_marks.frequency&&marks.operation==after_marks.operation&&marks.reset==after_marks.reset&&
  marks.pause_delta==after_marks.pause_delta&&marks.current_stopped_elapsed==after_marks.current_stopped_elapsed&&
  marks.game_turn_stopped_elapsed==after_marks.game_turn_stopped_elapsed&&after_marks.current_elapsed>=marks.current_elapsed&&
  after_marks.game_turn_elapsed>=marks.game_turn_elapsed,"Actual clock regression timer identity/start/use changed");
}
template<class Timer,class Cursor>concept PublicTimerClockAck=requires(Timer& timer,const Cursor& cursor){timer.adopt_actual_game_turn_clock_return(cursor);};
static_assert(!PublicTimerClockAck<runtime::BnwGameTurnTimerOwner,runtime::BnwRetainedTurnCursor>);
}
