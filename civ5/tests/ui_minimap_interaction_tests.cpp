#include "civ5/ui/minimap_ui.hpp"
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
}

int main() {
    using namespace civ5::ui;
    const auto in_game = civ5::content::read_xml_metadata(R"(<Context>
      <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
    </Context>)");
    const auto world_view = civ5::content::read_xml_metadata(R"(<Context>
      <LuaContext FileName="Assets/UI/InGame/WorldView/MiniMapPanel" ID="MiniMapPanel"/>
    </Context>)");
    UiScreenController controller(UiContextRegistry::from_documents(in_game, world_view));
    MinimapPanelLayout layout;
    layout.frame = {-8, -39, 355, 276};
    layout.map = {18, 43, 320, 190};
    layout.background = {0, 0, 128, 224};
    layout.strategic = {346, 1, 45, 45};
    layout.options = layout.strategic;
    layout.options_panel = {377, -39, 300, 300};
    layout.map_options = {
        {MinimapMapOption::hide_recommendations, "HideRecommendation", "TXT_KEY_MAP_OPTIONS_RECOMMENDATIONS"},
        {MinimapMapOption::show_resources, "ShowResources", "TXT_KEY_MAP_OPTIONS_RESOURCE_ICONS"},
        {MinimapMapOption::show_yield, "ShowYield", "TXT_KEY_MAP_OPTIONS_YIELD_ICONS"},
        {MinimapMapOption::show_grid, "ShowGrid", "TXT_KEY_MAP_OPTIONS_HEX_GRID"},
        {MinimapMapOption::show_trade, "ShowTrade", "TXT_KEY_MAP_OPTIONS_TRADE_ROUTES"}};
    MinimapUiState state;
    state.visible.segments[0] = {0, 0, 320, 190};
    state.visible.texture = {0, 320, 320.f / 190.f};
    state.visible_bounds_confirmed = true;
    state.strategic_view = false;
    state.options_visible = false;
    const UiFrameSnapshot frame = UiFrameSnapshot(1, 0, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    const auto plan = plan_minimap_ui(layout, 1280, 960, frame);
    require(plan.panel.map.x == 951 && plan.panel.map.y == 766,
            "BNW minimap map rectangle");
    require(plan.panel.options_panel.x == 603 && plan.panel.options_panel.y == 699,
            "authored map options shell anchor");
    require(plan.hits.size() == 3 && plan.viewport_rings.size() == 1,
            "minimap hit and viewport plan");
    require(plan.strategic_button.status == UiCommandStatus::unavailable &&
                plan.options_button.status == UiCommandStatus::unavailable,
            "buttons are disabled when no owner backend is supplied");
    auto map = minimap_command_at(plan, frame, plan.panel.map.x + 160,
                                  plan.panel.map.y + 95);
    require(map && std::get<UiGameplayCommand>(*map).as<MinimapPanToWorld>(),
            "map click emits typed pan");
    const auto *pan = std::get<UiGameplayCommand>(*map).as<MinimapPanToWorld>();
    require(std::abs(pan->x - 160.5f) < .001f &&
                std::abs(pan->y - 94.5f) < .001f,
            "screen to world click projection");
    require(!minimap_command_at(plan, frame, 0, 0), "outside hit ignored");
    auto strategic = minimap_command_at(plan, frame, plan.panel.strategic.x + 10,
                                         plan.panel.strategic.y + 10);
    require(strategic && std::get<UiGameplayCommand>(*strategic)
                             .as<MinimapSetStrategicView>()->enabled,
            "strategic button emits typed toggle");
    auto options = minimap_command_at(plan, frame, plan.panel.options.x + 10,
                                       plan.panel.options.y + 10);
    require(options && std::get<UiGameplayCommand>(*options)
                           .as<MinimapSetOptionsVisible>()->visible,
            "options button emits typed toggle");
    require(controller.dispatch(*map, frame).status == UiCommandStatus::unavailable,
            "missing backend cannot report success");
    MinimapUiBackend unavailable({});
    require(controller.dispatch(*options, frame, &unavailable).status ==
                UiCommandStatus::unavailable,
            "unsupported options cannot report success");
    int pans = 0;
    MinimapUiBackend backend({.pan_to_world = [&](float x, float y) {
        ++pans;
        return x == pan->x && y == pan->y;
    }});
    require(controller.dispatch(*map, frame, &backend).status == UiCommandStatus::performed &&
                pans == 1,
            "camera action executes once");
    state.visible_bounds_confirmed = false;
    const auto provisional = UiFrameSnapshot(3, 0, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    const auto provisional_result = controller.dispatch(*map, provisional, &backend);
    require(provisional_result.status == UiCommandStatus::unavailable && pans == 1 &&
                provisional_result.reason ==
                    "Native post-load minimap bounds are unconfirmed",
            "provisional bounds cannot move the camera or report success");
    int option_changes = 0;
    MinimapUiBackend option_backend({.set_options_visible = [&](bool visible) {
        ++option_changes;
        return visible;
    }});
    const auto option_plan = plan_minimap_ui(layout, 1280, 960, provisional,
                                             &option_backend);
    require(option_plan.strategic_button.status == UiCommandStatus::unavailable &&
                option_plan.options_button.status == UiCommandStatus::ready,
            "only the connected options button is enabled");
    require(controller.dispatch(*options, provisional, &option_backend).status ==
                UiCommandStatus::performed && option_changes == 1,
            "provisional map bounds do not disable an independent map button");
    state.options_visible = true;
    const auto opened = UiFrameSnapshot(4, 0, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    const auto opened_plan = plan_minimap_ui(layout, 1280, 960, opened, &option_backend);
    require(opened_plan.hits.size() == 8 &&
                opened_plan.map_display_buttons[static_cast<std::size_t>(
                    MinimapMapOption::show_grid)].status == UiCommandStatus::unavailable,
            "open options shell exposes five disabled rows without live states");
    require(controller.dispatch(*options, opened, &option_backend).status ==
                UiCommandStatus::unavailable && option_changes == 1,
            "stale options toggle cannot report success twice");
    const auto grid_row = minimap_map_display_row(opened_plan.panel.options_panel, 3);
    require(!minimap_command_at(opened_plan, opened, grid_row.x + 4, grid_row.y + 4),
            "unknown grid state cannot produce an action");
    state.map_display[static_cast<std::size_t>(MinimapMapOption::show_grid)] = false;
    const auto known_grid = UiFrameSnapshot(6, 0, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    const auto known_plan = plan_minimap_ui(layout, 1280, 960, known_grid, &option_backend);
    const auto grid_command = minimap_command_at(known_plan, known_grid,
                                                 grid_row.x + 4, grid_row.y + 4);
    require(grid_command && std::get<UiGameplayCommand>(*grid_command)
                                .as<MinimapSetMapDisplay>()->enabled &&
                controller.dispatch(*grid_command, known_grid, &option_backend).status ==
                    UiCommandStatus::unavailable,
            "known grid state still needs its real display owner");
    int grid_changes = 0;
    MinimapUiBackend grid_backend({.set_map_display = [&](MinimapMapOption option, bool enabled) {
        ++grid_changes;
        return option == MinimapMapOption::show_grid && enabled;
    }});
    require(plan_minimap_ui(layout, 1280, 960, known_grid, &grid_backend)
                .map_display_buttons[static_cast<std::size_t>(
                    MinimapMapOption::show_grid)].status == UiCommandStatus::ready &&
                controller.dispatch(*grid_command, known_grid, &grid_backend).status ==
                    UiCommandStatus::performed && grid_changes == 1,
            "typed map display command executes only through its owner");
    state.map_display[static_cast<std::size_t>(MinimapMapOption::show_grid)] = true;
    const auto grid_on = UiFrameSnapshot(7, 0, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    require(controller.dispatch(*grid_command, grid_on, &grid_backend).status ==
                UiCommandStatus::invalid && grid_changes == 1,
            "stale grid command cannot report success twice");
    state.map_display[static_cast<std::size_t>(MinimapMapOption::show_grid)] = false;
    const auto other_player = UiFrameSnapshot(6, 1, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    require(controller.dispatch(*grid_command, other_player, &grid_backend).status ==
                UiCommandStatus::invalid && grid_changes == 1,
            "map display command cannot cross an active-player change");
    state.options_visible.reset();
    const auto unknown = UiFrameSnapshot(5, 0, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    require(plan_minimap_ui(layout, 1280, 960, unknown, &option_backend)
                .options_button.status == UiCommandStatus::unavailable &&
                !minimap_command_at(plan, unknown, plan.panel.options.x + 10,
                                    plan.panel.options.y + 10) &&
                controller.dispatch(*options, unknown, &option_backend).status ==
                    UiCommandStatus::unavailable && option_changes == 1,
            "unknown options visibility cannot enable or mutate the button");
    state.visible.texture = {65, 189, 189.f / 190.f};
    const auto narrow = UiFrameSnapshot(2, 0, 1).with<MinimapUiState>(
        std::make_shared<const MinimapUiState>(state));
    const auto narrow_plan = plan_minimap_ui(layout, 1280, 960, narrow);
    require(!minimap_command_at(narrow_plan, narrow, narrow_plan.panel.map.x + 1,
                                 narrow_plan.panel.map.y + 95),
            "horizontal texture padding is not clickable");
    std::cout << "Minimap UI interaction checks PASS\n";
}
