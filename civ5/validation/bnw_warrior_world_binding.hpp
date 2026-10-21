#pragma once

#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace civ5::graphics {

struct BnwWarriorWorldBinding {
    std::uint64_t plot_index=0, x=0, y=0;
    std::int64_t owner=0, unit_id=0;
    bool visible=false;
    // No GR2 mesh, skeleton or animation decoder has been verified yet.
    bool model_drawable=false;
};

// Source-side selection is intentionally separate so removal/hiding can be
// checked without constructing a fake model or a renderer.
inline std::optional<BnwWarriorWorldBinding> selected_gandhi_warrior_from_scene(
    const BnwRendererSceneReceipt &receipt, bool current_plot_visible) {
    constexpr std::uint64_t plot=592;
    constexpr saves::SavedPlotUnitReference warrior{0,16385};
    if (!receipt.projection_reconciled || receipt.active_player!=0 ||
        receipt.active_team!=0 || receipt.game_turn!=0 ||
        receipt.cells.size()<=plot || receipt.active_team_art.size()<=plot)
        return std::nullopt;
    const auto &cell=receipt.cells[plot];
    if (cell.index!=plot || cell.x!=32 || cell.y!=14 || !cell.units ||
        std::find(cell.units->begin(),cell.units->end(),warrior)==cell.units->end() ||
        !current_plot_visible || !receipt.active_team_art[plot].revealed.value_or(false) ||
        receipt.active_team_art[plot].visibility_count<=0)
        return std::nullopt;
    return BnwWarriorWorldBinding{plot,cell.x,cell.y,warrior.owner,warrior.id,true,false};
}

inline std::optional<BnwWarriorWorldBinding> selected_gandhi_warrior_from_live_scene(
    const BnwRendererSceneReceipt &receipt, const runtime::LiveSessionState &live,
    std::int64_t active_player) {
    if (const auto mismatch=bnw_renderer_scene_revision_mismatch(
            receipt,live.world,receipt.game_turn,active_player))
        throw std::runtime_error("Warrior scene revision differs: "+*mismatch);
    const bool visible=runtime::bnw_plot_is_visible(live,592,
        static_cast<std::int64_t>(receipt.active_team));
    return selected_gandhi_warrior_from_scene(receipt,visible);
}

} // namespace civ5::graphics
