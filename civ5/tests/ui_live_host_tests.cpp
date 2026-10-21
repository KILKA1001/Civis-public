#include "civ5/ui/live_ui_host.hpp"
#include "civ5/ui/city_screens.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct Choice { int id = 0; };
struct Backend final : ui::UiGameplayBackend {
    int calls = 0;
    ui::UiCommandResult availability(const ui::UiGameplayCommand &command,
                                      const ui::UiFrameSnapshot &) const override {
        const auto *choice = command.as<Choice>();
        return choice && choice->id == 2
            ? ui::UiCommandResult{ui::UiCommandStatus::ready, {}}
            : ui::UiCommandResult{ui::UiCommandStatus::unavailable, "Choice is unavailable"};
    }
    ui::UiCommandResult execute(const ui::UiGameplayCommand &,
                                 const ui::UiFrameSnapshot &) override {
        ++calls;
        return {ui::UiCommandStatus::performed, {}};
    }
};

struct UnitBackend final : ui::UiGameplayBackend {
    int calls = 0;
    ui::UiCommandResult availability(const ui::UiGameplayCommand &command,
                                      const ui::UiFrameSnapshot &) const override {
        return command.as<ui::WorldViewAction>()
            ? ui::UiCommandResult{ui::UiCommandStatus::ready, {}}
            : ui::UiCommandResult{ui::UiCommandStatus::unavailable, "Unknown action"};
    }
    ui::UiCommandResult execute(const ui::UiGameplayCommand &,
                                 const ui::UiFrameSnapshot &) override {
        ++calls;
        return {ui::UiCommandStatus::performed, {}};
    }
};

ui::UiContextRegistry registry() {
    return ui::UiContextRegistry::from_documents(
        content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/TopPanel" ID="TopPanel"/>
          <LuaContext FileName="Assets/UI/InGame/CityView/CityView" ID="CityView" Hidden="True"/>
          <LuaContext FileName="Assets/UI/InGame/TechTree/TechTree" ID="TechTree" Hidden="True"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/SocialPolicyPopup" ID="SocialPolicyPopup" Hidden="True"/>
        </Context>)"),
        content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/ActionInfoPanel" ID="ActionInfoPanel"/>
          <LuaContext FileName="Assets/UI/InGame/WorldView/UnitPanel" ID="UnitPanel"/>
          <LuaContext FileName="Assets/UI/InGame/CityBannerManager" ID="CityBannerManager"/>
          <LuaContext FileName="Assets/UI/InGame/InfoCorner" ID="InfoCorner"/>
          <LuaContext FileName="Assets/UI/InGame/UnitFlagManager" ID="UnitFlagManager"/>
        </Context>)"));
}

ui::TopPanelLayout top_panel_layout() {
    return ui::TopPanelLayout::from_xml(content::read_xml_metadata(R"(<Context>
      <Image Texture="assets\UI\Art\WorldView\TopPanelBar.dds" Anchor="C,T" Size="Full.32"/>
      <Stack ID="TopPanelInfoStack" Offset="0,0" Padding="0" StackGrowth="Right"/>
      <Image Texture="assets\UI\Art\WorldView\TopPanelBar.dds" Anchor="R,T" Size="295.32"/>
      <Stack Anchor="R,T"><Button ID="Menu"/></Stack>
      <ToolTipType Name="TooltipTypeTopPanel"><Grid ID="TopPanelMouseover" Style="GridBlack8">
        <Label ID="TooltipLabel"/></Grid></ToolTipType>
    </Context>)"));
}

} // namespace

int main() {
    try {
        auto layout = top_panel_layout();
        ui::WorldViewSkin world_skin;
        ui::ProgressionScreenSource tech_source;
        tech_source.selected_source_path = "selected/TechTree.xml";
        tech_source.document = content::read_xml_metadata("<Context/>");
        ui::LiveUiSources sources;
        sources.world_skin = &world_skin;
        sources.top_panel_layout = &layout;
        sources.progression[0] = &tech_source;

        int sampled = 0;
        Backend backend;
        ui::LiveUiHost host(registry(), [&] {
            return ui::UiFrameSnapshot(static_cast<std::uint64_t>(++sampled), 7, 12);
        }, &backend, sources);

        auto first = host.begin_frame(1280, 960);
        require(sampled == 1 && first.snapshot.frame() == 1, "provider sampled once");
        require(first.world_view && first.world_view->paints.empty() &&
                    first.top_panel_bars && !first.top_panel && !first.progression,
                "plans retain missing live values");
        require(!host.click(first, 500, 500).consumed, "unclaimed world click reaches camera");

        const ui::LiveUiHit top_hit{{10, 0, 100, 32}, "TopPanel",
            ui::UiOpenScreen{"TechTree"}};
        const auto opened = host.click(first, 20, 16, std::span(&top_hit, 1));
        require(opened.consumed && opened.command.status == ui::UiCommandStatus::navigation,
                "measured top panel hit opens screen");
        require(host.click(first, 20, 16, std::span(&top_hit, 1)).command.status ==
                    ui::UiCommandStatus::invalid, "old frame cannot repeat click");

        auto tech_frame = host.begin_frame(1280, 960);
        require(sampled == 2 && tech_frame.progression &&
                    tech_frame.progression->selected_source_path == "selected/TechTree.xml" &&
                    !tech_frame.progression->unresolved.empty(),
                "focused progression uses selected source and exposes absent data");
        const ui::LiveUiHit unavailable{{30, 50, 50, 50}, "TechTree",
            ui::UiGameplayCommand::make("TechTree", Choice{1})};
        const auto refused = host.click(tech_frame, 40, 60, std::span(&unavailable, 1));
        require(refused.consumed && refused.command.status == ui::UiCommandStatus::unavailable &&
                    backend.calls == 0, "unavailable choice does not mutate game");
        const ui::LiveUiHit mismatched{{30, 50, 50, 50}, "TechTree",
            ui::UiGameplayCommand::make("WorldView", Choice{2})};
        require(host.click(tech_frame, 40, 60, std::span(&mismatched, 1)).command.status ==
                    ui::UiCommandStatus::invalid && backend.calls == 0,
                "measured hit cannot impersonate another gameplay context");
        const ui::LiveUiHit available{{30, 50, 50, 50}, "TechTree",
            ui::UiGameplayCommand::make("TechTree", Choice{2})};
        const auto performed = host.click(tech_frame, 40, 60, std::span(&available, 1));
        require(performed.command.status == ui::UiCommandStatus::performed && backend.calls == 1,
                "typed choice executes exactly once");

        auto next = host.begin_frame(1280, 960);
        require(host.dispatch(ui::UiOpenScreen{"SocialPolicyPopup"}, next).status ==
                    ui::UiCommandStatus::navigation, "open modal popup");
        auto modal_frame = host.begin_frame(1280, 960);
        require(host.click(modal_frame, 40, 60, std::span(&available, 1)).command.status ==
                    ui::UiCommandStatus::invalid && backend.calls == 1,
                "modal screen blocks background hit");
        require(host.click(modal_frame, 500, 500).consumed,
                "modal screen retains pointer focus outside its buttons");
        require(host.dispatch_top_panel(ui::TopPanelAction::open_game_menu, modal_frame).status ==
                    ui::UiCommandStatus::invalid, "modal screen blocks TopPanel action");
        require(host.dispatch(ui::UiCloseScreen{"SocialPolicyPopup"}, modal_frame).status ==
                    ui::UiCommandStatus::navigation, "close focused modal");
        auto after_modal = host.begin_frame(1280, 960);
        require(!host.click(after_modal, 500, 500).consumed, "camera input restored");

        ui::LiveUiHost disconnected(registry(), [] { return ui::UiFrameSnapshot(1, 7, 1); },
                                    nullptr);
        auto no_backend = disconnected.begin_frame(1280, 960);
        const ui::LiveUiHit world_action{{0, 0, 10, 10}, "WorldView",
            ui::UiGameplayCommand::make("WorldView", Choice{2})};
        require(disconnected.click(no_backend, 2, 2, std::span(&world_action, 1)).command.status ==
                    ui::UiCommandStatus::unavailable, "no backend never claims gameplay success");

        std::optional<ui::WorldViewUnit> selected_unit;
        std::vector<ui::WorldViewNotification> notifications;
        std::vector<ui::WorldViewActionItem> info_corner;
        std::vector<ui::WorldViewIcon> unit_flags;
        UnitBackend unit_backend;
        ui::LiveUiHost unit_host(registry(), [&] {
            ui::WorldViewState world;
            world.player_id = 7;
            world.selected_unit = selected_unit;
            world.notifications = notifications;
            world.info_corner = info_corner;
            world.unit_flags = unit_flags;
            return ui::UiFrameSnapshot(1, 7, 12).with<ui::WorldViewState>(
                std::make_shared<const ui::WorldViewState>(std::move(world)));
        }, &unit_backend);
        const auto unit_command = [](std::int64_t action, std::int64_t unit) {
            return ui::UiCommand(ui::UiGameplayCommand::make("UnitPanel",
                ui::WorldViewAction{ui::WorldViewActionKind::unit_action, action, unit}));
        };
        auto unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(unit_command(11, 3), unit_frame).status ==
                    ui::UiCommandStatus::unavailable && unit_backend.calls == 0,
                "no selected unit cannot execute an action through a ready backend");
        selected_unit = ui::WorldViewUnit{3, "Warrior", "2/2", "8",
            {{11, "Move", {}, {}, false}}};
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(unit_command(11, 3), unit_frame).status ==
                    ui::UiCommandStatus::unavailable &&
                    unit_host.dispatch(unit_command(12, 3), unit_frame).status ==
                    ui::UiCommandStatus::unavailable &&
                    unit_host.dispatch(unit_command(11, 4), unit_frame).status ==
                    ui::UiCommandStatus::unavailable && unit_backend.calls == 0,
                "disabled, absent and wrong-unit actions never reach backend");
        selected_unit->actions.front().available = true;
        auto stale_frame = unit_frame;
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(unit_command(11, 3), stale_frame).status ==
                    ui::UiCommandStatus::invalid &&
                    unit_host.dispatch(unit_command(11, 3), unit_frame).status ==
                    ui::UiCommandStatus::performed && unit_backend.calls == 1,
                "only a current selected-unit action executes once");
        const auto notification_command = [](std::string source, std::int64_t id) {
            return ui::UiCommand(ui::UiGameplayCommand::make(std::move(source),
                ui::WorldViewAction{ui::WorldViewActionKind::notification, id}));
        };
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(notification_command("ActionInfoPanel", 41),
                    unit_frame).status == ui::UiCommandStatus::unavailable &&
                    unit_backend.calls == 1,
                "a ready backend cannot execute a notification absent from the live queue");
        notifications.push_back({41, "Current event", {}, true});
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(notification_command("UnitPanel", 41),
                    unit_frame).status == ui::UiCommandStatus::unavailable &&
                    unit_host.dispatch(notification_command("ActionInfoPanel", 42),
                    unit_frame).status == ui::UiCommandStatus::unavailable &&
                    unit_host.dispatch(notification_command("ActionInfoPanel", 41),
                    unit_frame).status == ui::UiCommandStatus::performed &&
                    unit_backend.calls == 2,
                "only a current notification from ActionInfoPanel reaches backend");
        const auto corner_command = [](std::int64_t id) {
            return ui::UiCommand(ui::UiGameplayCommand::make("InfoCorner",
                ui::WorldViewAction{ui::WorldViewActionKind::info_corner, id}));
        };
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(corner_command(5), unit_frame).status ==
                    ui::UiCommandStatus::unavailable && unit_backend.calls == 2,
                "a ready backend cannot execute an absent InfoCorner action");
        info_corner.push_back({5, "Current corner", {}, {}, false});
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(corner_command(5), unit_frame).status ==
                    ui::UiCommandStatus::unavailable && unit_backend.calls == 2,
                "disabled InfoCorner action stays disabled");
        info_corner.front().available = true;
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(corner_command(5), unit_frame).status ==
                    ui::UiCommandStatus::performed && unit_backend.calls == 3,
                "current backend-owned InfoCorner action executes once");
        const auto flag_command = [](std::int64_t id) {
            return ui::UiCommand(ui::UiGameplayCommand::make("UnitFlagManager",
                ui::WorldViewAction{ui::WorldViewActionKind::select_unit, id}));
        };
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(flag_command(3), unit_frame).status ==
                    ui::UiCommandStatus::unavailable && unit_backend.calls == 3,
                "an absent projected unit flag cannot select a unit");
        unit_flags.push_back({3, {300, 400}, "Warrior", {}, {}, true});
        unit_frame = unit_host.begin_frame(1280, 960);
        require(unit_host.dispatch(flag_command(3), unit_frame).status ==
                    ui::UiCommandStatus::performed && unit_backend.calls == 4,
                "a current projected flag reaches its gameplay owner once");

        ui::LiveUiHost city_host(registry(), [] {
            ui::WorldViewState world;
            world.player_id = 7;
            world.city_banners.push_back({{8192, {400, 450}, "Delhi", {}, {}, true},
                                          1, true});
            ui::CityScreenFrame cities;
            cities.player_id = 7;
            cities.cities.push_back({7, 8192, 32, 15, 1, std::string("Delhi")});
            return ui::UiFrameSnapshot(1, 7, 12)
                .with<ui::WorldViewState>(
                    std::make_shared<const ui::WorldViewState>(std::move(world)))
                .with<ui::CityScreenFrame>(
                    std::make_shared<const ui::CityScreenFrame>(std::move(cities)));
        }, nullptr);
        const auto city_action = [](std::int64_t id) {
            return ui::UiCommand(ui::UiGameplayCommand::make("CityBannerManager",
                ui::WorldViewAction{ui::WorldViewActionKind::open_city, id}));
        };
        auto city_frame = city_host.begin_frame(1280, 960);
        require(city_host.dispatch(ui::UiOpenScreen{"CityView"}, city_frame).status ==
                    ui::UiCommandStatus::unavailable,
                "CityView cannot open without selected live city");
        require(city_host.dispatch(city_action(42), city_frame).status ==
                    ui::UiCommandStatus::unavailable,
                "unmatched city banner cannot select a city");
        const ui::LiveUiHit city_hit{{345, 430, 110, 40}, "CityBannerManager",
                                     city_action(8192)};
        require(city_host.click(city_frame, 400, 450,
                    std::span(&city_hit, 1)).command.status ==
                    ui::UiCommandStatus::navigation &&
                    city_host.screens().focused_screen() ==
                        std::optional<std::string>{"CityView"},
                "confirmed Delhi banner opens CityView without gameplay backend");
        city_frame = city_host.begin_frame(1280, 960);
        require(ui::selected_city(city_frame.snapshot) &&
                    ui::selected_city(city_frame.snapshot)->id == 8192 &&
                    ui::selected_city(city_frame.snapshot)->name == "Delhi",
                "CityView retains confirmed Delhi identity across live frames");
        require(city_host.dispatch(ui::UiCloseScreen{"CityView"}, city_frame).status ==
                    ui::UiCommandStatus::navigation,
                "CityView closes through typed navigation");
        city_frame = city_host.begin_frame(1280, 960);
        require(!ui::selected_city(city_frame.snapshot),
                "closing CityView clears UI-owned city selection");
        std::cout << "live UI host tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
