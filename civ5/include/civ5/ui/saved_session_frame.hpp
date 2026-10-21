#pragma once

#include "civ5/saves/session_snapshot.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/top_panel_saved_state.hpp"

namespace civ5::ui {

// A frozen BNW save can supply observed UI values, but it is not a running
// LiveSession. The resulting frames intentionally have no gameplay backend.
// The partial saved projection is kept under its own type so consumers cannot
// mistake it for a complete TopPanelLiveFrame.
LiveUiHost::FrameProvider make_saved_session_frame_provider(
    const saves::SavedSessionSnapshot &snapshot, std::int64_t active_player_id);

} // namespace civ5::ui
