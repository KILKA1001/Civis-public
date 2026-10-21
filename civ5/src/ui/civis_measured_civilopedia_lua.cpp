#include "civ5/ui/civis_measured_civilopedia_lua.hpp"
#include <limits>

namespace civ5::ui {
namespace {
std::int32_t checked_coordinate(std::int64_t value) {
    if (value < INT32_MIN || value > INT32_MAX)
        throw runtime::ScriptError("Civilopedia geometry exceeds renderer integer range");
    return static_cast<std::int32_t>(value);
}
}
runtime::ScriptUiLayoutProvider bind_civis_measured_civilopedia_layout(
    CivisMeasuredCivilopediaControlTree &controls,
    const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaMeasuredLayout &measured,
    CivisCivilopediaScrollMeasurementProvider renderer_measurements) {
    const auto owner = controls.owner();
    if (!sources.verified || !sources.control_tree.verified ||
        owner.frame_number != measured.frame_number || owner.owner_token != measured.owner_token ||
        owner.player_id != measured.player_id || owner.game_turn != measured.game_turn ||
        owner.payload_sha256 != measured.payload_sha256 || owner.rules_sha256 != measured.rules_sha256 ||
        owner.styles_sha256 != measured.resolved_styles_sha256 ||
        owner.styles_sha256 != sources.styles_sha256 ||
        owner.civilopedia_sha256 != sources.civilopedia_sha256 || owner.xml_sha256 != sources.xml_sha256 ||
        measured.viewport_width <= 0 || measured.viewport_height <= 172)
        throw runtime::ScriptError("Civilopedia Lua binding source or measured owner changed");
    runtime::ScriptUiLayoutProvider result;
    result.context_id = "CivilopediaScreen";
    for (const auto &control : sources.control_tree.controls) {
        if (!controls.get_geometry(owner, control.id).available)
            throw runtime::ScriptError("Civilopedia Lua binding control is unavailable");
        result.controls.push_back({control.id, control.kind, true});
    }
    result.screen_size = [width = measured.viewport_width, height = measured.viewport_height] {
        return std::pair<std::int64_t, std::int64_t>{width, height};
    };
    result.get_size = [&controls, owner](std::string_view id) {
        const auto size = controls.get_size(owner, id);
        if (!size.available) throw runtime::ScriptError(size.blocker);
        return std::pair<std::int64_t, std::int64_t>{size.width, size.height};
    };
    result.set_size = [&controls, owner](std::string_view id, std::int64_t width, std::int64_t height) {
        const auto step = controls.set_size(owner, id, checked_coordinate(width), checked_coordinate(height));
        if (!step.applied) throw runtime::ScriptError(step.blocker);
    };
    result.set_offset = [&controls, owner](std::string_view id, std::int64_t x, std::int64_t y) {
        const auto step = controls.set_offset(owner, id, checked_coordinate(x), checked_coordinate(y));
        if (!step.applied) throw runtime::ScriptError(step.blocker);
    };
    result.calculate_internal_size = [&controls, owner,
        renderer_measurements = std::move(renderer_measurements)](std::string_view id) {
        if (!renderer_measurements)
            throw runtime::ScriptError("CalculateInternalSize requires a resolved renderer measurement provider");
        const auto completed = controls.completed_operations();
        if (completed < 9 || (completed == 9 && id != "ScrollPanel") ||
            (completed == 10 && id != "LeftScrollPanel"))
            throw runtime::ScriptError("Measured Civilopedia Lua calculation is out of source order");
        const auto viewport = controls.get_geometry(owner, id);
        if (!viewport.available) throw runtime::ScriptError(viewport.blocker);
        auto snapshot = renderer_measurements(owner, id, viewport.geometry);
        const auto published = controls.publish(owner, std::move(snapshot));
        if (!published.applied) throw runtime::ScriptError(published.blocker);
        const auto calculated = controls.calculate_internal_size(owner, id);
        if (!calculated.applied) throw runtime::ScriptError(calculated.blocker);
    };
    return result;
}
} // namespace civ5::ui
