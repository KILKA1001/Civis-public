#include "civ5/ui/minimap_ui.hpp"
#include <cmath>
#include <stdexcept>

namespace civ5::ui {
namespace {
bool contains(MinimapRect rect, std::int32_t x, std::int32_t y) {
    return x >= rect.x && y >= rect.y &&
           static_cast<std::int64_t>(x) < static_cast<std::int64_t>(rect.x) + rect.width &&
           static_cast<std::int64_t>(y) < static_cast<std::int64_t>(rect.y) + rect.height;
}
bool valid(MinimapWorldBounds bounds) {
    return std::isfinite(bounds.min_x) && std::isfinite(bounds.min_y) &&
           std::isfinite(bounds.max_x) && std::isfinite(bounds.max_y) &&
           bounds.max_x > bounds.min_x && bounds.max_y > bounds.min_y;
}
} // namespace

MinimapRect minimap_map_display_row(MinimapRect panel, std::size_t index) {
    if (index >= minimap_map_option_count || panel.width < 280 || panel.height < 260)
        throw std::invalid_argument("Invalid minimap map display row");
    return {panel.x + 28, panel.y + 54 + static_cast<std::int32_t>(index) * 29,
            panel.width - 56, 29};
}

MinimapUiPlan plan_minimap_ui(const MinimapPanelLayout &layout,
                             std::uint32_t viewport_width, std::uint32_t viewport_height,
                             const UiFrameSnapshot &frame,
                             const UiGameplayBackend *backend) {
    MinimapUiPlan result;
    result.panel = plan_minimap_panel(layout, viewport_width, viewport_height);
    // The controls can overlap the authored frame; test them before the map.
    result.hits = {{MinimapHitKind::strategic, result.panel.strategic},
                   {MinimapHitKind::options, result.panel.options},
                   {MinimapHitKind::map, result.panel.map}};
    for (auto &option : result.map_display_buttons)
        option = {UiCommandStatus::unavailable, "Map display owner is unavailable"};
    if (const auto *state = frame.find<MinimapUiState>()) {
        if (!valid(state->visible.segments[0]) ||
            state->visible.texture.active_width == 0 ||
            state->visible.texture.padding_x + state->visible.texture.active_width > 320 ||
            state->visible.segment_count == 0 || state->visible.segment_count > 2 ||
            state->camera_corners.size() > 3)
            throw std::invalid_argument("Invalid live minimap UI state");
        for (std::uint32_t segment = 0; segment < state->visible.segment_count; ++segment) {
            if (!valid(state->visible.segments[segment]))
                throw std::invalid_argument("Invalid minimap world segment");
            result.viewport_rings.push_back(build_minimap_viewport_ring(
                state->camera_corners, state->visible.segments[segment],
                state->visible.texture.active_width));
        }
        if (backend && state->strategic_view)
            result.strategic_button = backend->availability(
                UiGameplayCommand::make("MiniMapPanel",
                    MinimapSetStrategicView{!*state->strategic_view}), frame);
        if (backend && state->options_visible)
            result.options_button = backend->availability(
                UiGameplayCommand::make("MiniMapPanel",
                    MinimapSetOptionsVisible{!*state->options_visible}), frame);
        if (state->options_visible && *state->options_visible)
            for (const auto &source : layout.map_options) {
                result.hits.push_back({MinimapHitKind::map_display,
                    minimap_map_display_row(result.panel.options_panel,
                                             static_cast<std::size_t>(source.option)),
                    source.option});
                if (const auto command = minimap_map_display_command(frame, source.option))
                    if (backend)
                        result.map_display_buttons[static_cast<std::size_t>(source.option)] =
                            backend->availability(std::get<UiGameplayCommand>(*command), frame);
            }
    }
    return result;
}

std::optional<UiCommand> minimap_command_at(const MinimapUiPlan &plan,
                                            const UiFrameSnapshot &frame,
                                            std::int32_t x, std::int32_t y) {
    const auto *state = frame.find<MinimapUiState>();
    if (!state) return std::nullopt;
    for (const auto &hit : plan.hits) {
        if (!contains(hit.bounds, x, y)) continue;
        if (hit.kind == MinimapHitKind::strategic) {
            if (!state->strategic_view) return std::nullopt;
            return UiGameplayCommand::make("MiniMapPanel",
                MinimapSetStrategicView{!*state->strategic_view});
        }
        if (hit.kind == MinimapHitKind::options) {
            if (!state->options_visible) return std::nullopt;
            return UiGameplayCommand::make("MiniMapPanel",
                MinimapSetOptionsVisible{!*state->options_visible});
        }
        if (hit.kind == MinimapHitKind::map_display)
            return minimap_map_display_command(frame, hit.option);
        const auto &texture = state->visible.texture;
        const float texture_x = (static_cast<float>(x - hit.bounds.x) + .5f) *
                                320.0f / static_cast<float>(hit.bounds.width);
        if (texture_x < texture.padding_x ||
            texture_x >= texture.padding_x + texture.active_width)
            return std::nullopt;
        const auto &bounds = state->visible.segments[0];
        const float u = (texture_x - texture.padding_x) /
                        static_cast<float>(texture.active_width);
        const float v = (static_cast<float>(y - hit.bounds.y) + .5f) /
                        static_cast<float>(hit.bounds.height);
        return UiGameplayCommand::make("MiniMapPanel", MinimapPanToWorld{
            bounds.min_x + u * (bounds.max_x - bounds.min_x),
            bounds.max_y - v * (bounds.max_y - bounds.min_y)});
    }
    return std::nullopt;
}

std::optional<UiCommand> minimap_map_display_command(const UiFrameSnapshot &frame,
                                                      MinimapMapOption option) {
    const auto index = static_cast<std::size_t>(option);
    const auto *state = frame.find<MinimapUiState>();
    if (!state || !frame.active_player_id() || !frame.turn() ||
        !state->options_visible || !*state->options_visible ||
        index >= minimap_map_option_count || !state->map_display[index])
        return std::nullopt;
    return UiGameplayCommand::make("MiniMapPanel",
        MinimapSetMapDisplay{option, !*state->map_display[index],
                             *frame.active_player_id(), *frame.turn(), frame.frame()});
}

UiCommandResult MinimapUiBackend::availability(const UiGameplayCommand &command,
                                                const UiFrameSnapshot &frame) const {
    if (command.source_id() != "MiniMapPanel")
        return {UiCommandStatus::invalid, "Not a minimap command"};
    const auto *state = frame.find<MinimapUiState>();
    if (!state)
        return {UiCommandStatus::unavailable, "Live minimap state is missing"};
    if (const auto *pan = command.as<MinimapPanToWorld>()) {
        if (!state->visible_bounds_confirmed)
            return {UiCommandStatus::unavailable,
                    "Native post-load minimap bounds are unconfirmed"};
        return callbacks_.pan_to_world && std::isfinite(pan->x) && std::isfinite(pan->y)
            ? UiCommandResult{UiCommandStatus::ready, {}}
            : UiCommandResult{UiCommandStatus::unavailable, "Camera pan is unavailable"};
    }
    if (const auto *strategic = command.as<MinimapSetStrategicView>()) {
        if (!state->strategic_view || strategic->enabled == *state->strategic_view)
            return {UiCommandStatus::unavailable,
                    "Strategic view state is unknown or already selected"};
        return callbacks_.set_strategic_view
            ? UiCommandResult{UiCommandStatus::ready, {}}
            : UiCommandResult{UiCommandStatus::unavailable, "Strategic view is unavailable"};
    }
    if (const auto *options = command.as<MinimapSetOptionsVisible>()) {
        if (!state->options_visible || options->visible == *state->options_visible)
            return {UiCommandStatus::unavailable,
                    "Map options state is unknown or already selected"};
        return callbacks_.set_options_visible
            ? UiCommandResult{UiCommandStatus::ready, {}}
            : UiCommandResult{UiCommandStatus::unavailable, "Map options are unavailable"};
    }
    if (const auto *display = command.as<MinimapSetMapDisplay>()) {
        const auto index = static_cast<std::size_t>(display->option);
        if (index >= minimap_map_option_count)
            return {UiCommandStatus::invalid, "Unknown map display option"};
        if (!frame.active_player_id() || !frame.turn() ||
            display->player_id != *frame.active_player_id() ||
            display->turn != *frame.turn() || display->frame != frame.frame())
            return {UiCommandStatus::invalid, "Map display player or frame is stale"};
        if (!state->options_visible || !*state->options_visible ||
            !state->map_display[index] ||
            display->enabled == *state->map_display[index])
            return {UiCommandStatus::unavailable,
                    "Map display state is hidden, unknown or already selected"};
        return callbacks_.set_map_display
            ? UiCommandResult{UiCommandStatus::ready, {}}
            : UiCommandResult{UiCommandStatus::unavailable,
                              "Map display owner is unavailable"};
    }
    return {UiCommandStatus::invalid, "Unknown minimap command"};
}

UiCommandResult MinimapUiBackend::execute(const UiGameplayCommand &command,
                                           const UiFrameSnapshot &frame) {
    const auto available = availability(command, frame);
    if (available.status != UiCommandStatus::ready) return available;
    bool succeeded = false;
    if (const auto *pan = command.as<MinimapPanToWorld>())
        succeeded = callbacks_.pan_to_world(pan->x, pan->y);
    else if (const auto *strategic = command.as<MinimapSetStrategicView>())
        succeeded = callbacks_.set_strategic_view(strategic->enabled);
    else if (const auto *options = command.as<MinimapSetOptionsVisible>())
        succeeded = callbacks_.set_options_visible(options->visible);
    else if (const auto *display = command.as<MinimapSetMapDisplay>())
        succeeded = callbacks_.set_map_display(display->option, display->enabled);
    return succeeded ? UiCommandResult{UiCommandStatus::performed, {}}
                     : UiCommandResult{UiCommandStatus::unavailable, "Minimap action was rejected"};
}

} // namespace civ5::ui
