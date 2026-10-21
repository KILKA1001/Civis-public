#include "civ5/ui/economic_overview_gold_input.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

civ5::runtime::LiveSession session() {
    using namespace civ5;
    saves::SavedSession source;
    source.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{source.snapshot, runtime::ResearchContext{},
        std::vector<runtime::TeamTechnologies>{}, runtime::PlayerResearchContext{},
        runtime::CityYieldContext{}, runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{}, runtime::TreasuryContext{},
        runtime::ScienceContext{}, runtime::ResearchTurnState{},
        runtime::SessionRandomStreams{
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {5, 0, 0}),
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {7, 0, 0})},
        runtime::ResearchSelectionState{}, runtime::BnwGameClockState{},
        {}, {}, {}, {}, {}, {}, {}, {}, {}};
    state.setup.game_turn = 12;
    state.setup.players[7].alive = true;
    return runtime::LiveSession::from_restored_bnw(std::move(source),
                                                    std::move(state));
}
} // namespace

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        require(argc == 2, "installed BNW game root required");
        const std::filesystem::path root(argv[1]);
        const auto xml_path = root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml";
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->add_file(root / "Assets/UI/InGame/TopPanel.xml");
        base->add_file(root / "Assets/UI/InGame/Popups/EconomicOverview.lua");
        bnw->add_file(root / "Assets/DLC/Expansion2/UI/InGame/TopPanel.lua");
        bnw->add_file(xml_path);
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
            assets::resource_keys(base));
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
            assets::resource_keys(bnw)});
        OverviewSource overview{"EconomicOverview",
            "Assets/UI/InGame/Popups/EconomicOverview.xml",
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml", {}};
        std::function<void(const content::XmlNode &)> collect;
        collect = [&](const content::XmlNode &node) {
            const auto id = node.attribute("ID");
            if (!id.empty()) overview.controls.push_back(
                OverviewControl{id, node.name, {}, {}, {}, {}, {}, {}});
            for (const auto &child : node.children) collect(child);
        };
        collect(content::read_xml(xml_path));
        const auto open_source = inspect_economic_gold_source(resources);
        const auto close_source = inspect_economic_close_source(resources, overview);
        require(open_source.verified && close_source.verified &&
                open_source.economic_lua_sha256 == close_source.lua_sha256,
            "selected BNW GoldPerTurn and popup callback sources agree");

        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, content::read_xml_metadata("<Context/>")));
        auto live = session();
        auto foreign = session();
        EconomicOverviewLifecycle lifecycle(live, 7, screens, close_source);
        EconomicOverviewOpenBridge bridge(live, 7, lifecycle, open_source);
        EconomicOverviewGoldInput input(live, screens, bridge, lifecycle);
        int host = 0, other_host = 0;
        LiveUiFrame frame{UiFrameSnapshot(20, 7, 12), 4, &host,
                          {}, {}, {}, {}, {}, {}, {}, {}, false};
        frame.top_panel.emplace();
        frame.top_panel->show_info_stack = true;
        frame.top_panel->gold_text = "[ICON_GOLD]10";
        EconomicGoldInputMeasurement measured{
            {7, 33, 12, 20, 9, 1200, 800, {40, 4, 110, 30},
             open_source.top_panel_lua_sha256,
             open_source.top_panel_xml_sha256},
            &host, 4};
        require(!lifecycle.active_popups(live),
            "save and selected source do not synthesize an open popup");
        const auto outside = input.click(live, frame, measured, 1200, 800,
                                         500, 500, 500, 500);
        require(!outside.consumed && !outside.popup,
            "pointer wholly outside GoldPerTurn remains available to camera route");
        const auto release_outside = input.click(live, frame, measured, 1200, 800,
                                                 50, 10, 500, 500);
        require(release_outside.consumed && !release_outside.popup &&
                !lifecycle.active_popups(live),
            "release outside consumes TopPanel input without opening popup");
        const auto press_outside = input.click(live, frame, measured, 1200, 800,
                                               500, 500, 50, 10);
        require(press_outside.consumed && !press_outside.popup,
            "release into TopPanel cannot trigger opening or camera click");
        auto stale = measured;
        stale.frame_sequence = 3;
        const auto old_frame = input.click(live, frame, stale, 1200, 800,
                                           50, 10, 50, 10);
        require(old_frame.consumed && old_frame.status ==
                    EconomicPopupQueueStatus::blocked && !old_frame.popup,
            "stale presenter frame is consumed but cannot open popup");
        const auto resized = input.click(live, frame, measured, 1100, 800,
                                         50, 10, 50, 10);
        require(resized.consumed && !resized.popup,
            "changed viewport blocks stale geometry and isolates camera");
        const auto clipped = input.click(live, frame, measured, 80, 800,
                                         50, 10, 50, 10);
        require(clipped.consumed && !clipped.popup,
            "visible part of resized UI control still isolates camera");
        LiveUiFrame another{frame.snapshot, 4, &other_host,
                            {}, {}, {}, {}, {}, {}, {}, {}, false};
        another.top_panel = frame.top_panel;
        require(input.click(live, another, measured, 1200, 800,
                    50, 10, 50, 10).consumed &&
                !lifecycle.active_popups(live),
            "another frame owner cannot emit EconomicOverview event");
        require(input.click(foreign, frame, measured, 1200, 800,
                    50, 10, 50, 10).consumed &&
                !lifecycle.active_popups(live),
            "foreign live session cannot emit event or send click to camera");
        const auto opened = input.click(live, frame, measured, 1200, 800,
                                        50, 10, 50, 10);
        require(opened.consumed && opened.status ==
                    EconomicPopupQueueStatus::queued && opened.popup &&
                opened.popup->player_id == 7 && opened.popup->turn == 12 &&
                opened.popup->owner_sequence == 9 &&
                opened.popup->events.size() == 1 &&
                opened.popup->events.front().event_id == 33 &&
                screens.visible("EconomicOverview") &&
                lifecycle.turn_timer_semaphore() == 1,
            "same measured press/release opens exactly the current popup owner");
        const auto again = input.click(live, frame, measured, 1200, 800,
                                       50, 10, 50, 10);
        require(again.consumed && again.status ==
                    EconomicPopupQueueStatus::blocked && !again.popup &&
                lifecycle.turn_timer_semaphore() == 1,
            "focused modal and same event cannot open or replay another popup");
        std::cout << "selected BNW EconomicOverview GoldPerTurn input: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
