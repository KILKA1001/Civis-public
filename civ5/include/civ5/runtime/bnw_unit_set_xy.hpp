#pragma once
#include "civ5/saves/session_snapshot.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

struct BnwSetXYRequest {
    std::int64_t owner=-1,unit_id=-1,to_x=-1,to_y=-1;
};

// Expansion2 CvUnit.cpp:12918-13692. These are in source execution order for
// the restricted default-layer, empty-destination, non-city candidate path.
// Conditional branches still require services that can prove and run them.
enum class BnwSetXYPhase : std::uint8_t {
    popup_guard_and_ranged_setup,
    destination_collision,
    old_plot_remove,
    old_plot_sight_area_and_move_clock,
    coordinates_and_facing,
    new_plot_fortify_sight_roster_and_area,
    city_transport_meeting_and_adjacent_effects,
    graphics_and_cargo_follow,
    goody_camp_and_territory,
    capture_and_ui_dirty,
    lua_unit_set_xy,
    popup_release_and_garrison_ui,
};

enum class BnwSetXYBlocker : std::uint8_t {
    unsupported_candidate,
    unit_rules_and_combat,
    plot_sight_and_revelation,
    area_unit_counters,
    facing_and_fortify,
    diplomacy_and_adjacent_effects,
    ui_publication,
    lua_unit_set_xy,
    complete_unit_persistence,
};

struct BnwSetXYPreflight {
    bool simple_candidate=false;
    std::string reason;
    std::int64_t from_x=-1,from_y=-1,to_x=-1,to_y=-1;
    std::int64_t source_last_move_turn=-1,next_last_move_turn=-1;
    bool clears_ranged_setup=false;
    bool resets_fortify=false;
    std::vector<BnwSetXYPhase> sdk_order;
    std::vector<BnwSetXYBlocker> blockers;
    bool can_commit() const noexcept {return blockers.empty();}
};

// Read-only preflight. It never calls CvUnit::setXY, changes a session, emits
// Lua/UI events, or treats the diagnostic unit checkpoint as a game action.
BnwSetXYPreflight preflight_bnw_set_xy(
    const saves::SavedSessionSnapshot &world,const BnwSetXYRequest &request);

} // namespace civ5::runtime
