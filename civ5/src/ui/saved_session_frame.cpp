#include "civ5/ui/saved_session_frame.hpp"
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace civ5::ui {

LiveUiHost::FrameProvider make_saved_session_frame_provider(
    const saves::SavedSessionSnapshot &snapshot, std::int64_t active_player_id) {
    if (snapshot.game_state.layout != saves::GameStateLayout::expansion2_version1 ||
        !snapshot.pregame || active_player_id < 0)
        throw std::invalid_argument("Saved UI frame requires a BNW player and pregame state");
    const auto turn = snapshot.pregame->archive.integers.find("game_turn");
    if (turn == snapshot.pregame->archive.integers.end() || turn->second < 0 ||
        turn->second > std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("Saved UI frame requires a valid game turn");
    auto projection = std::make_shared<const TopPanelSavedProjection>(
        project_top_panel_saved_player(snapshot, active_player_id));
    // A saved game can identify the active player, but cannot execute a turn.
    // Publish only the authored WorldView control that is safe to show without
    // a running gameplay session or a camera-projected selection.
    WorldViewState world_view;
    world_view.player_id = active_player_id;
    world_view.end_turn_unavailable_reason = "A saved snapshot cannot execute End Turn";
    auto world = std::make_shared<const WorldViewState>(std::move(world_view));
    const auto game_turn = static_cast<std::int32_t>(turn->second);
    return [active_player_id, game_turn, projection, world,
            frame_number = std::uint64_t{0}]() mutable -> UiFrameSnapshot {
        if (frame_number == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("Saved UI frame number exhausted");
        return UiFrameSnapshot(++frame_number, active_player_id, game_turn)
            .with<TopPanelSavedProjection>(projection)
            .with<WorldViewState>(world);
    };
}

} // namespace civ5::ui
