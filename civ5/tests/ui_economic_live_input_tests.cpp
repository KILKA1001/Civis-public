#include "civ5/ui/economic_overview_live_input.hpp"
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
        const auto tab_source = inspect_economic_tab_source(resources, overview);
        require(open_source.verified && close_source.verified &&
                tab_source.verified &&
                open_source.economic_lua_sha256 == close_source.lua_sha256 &&
                tab_source.lua_sha256 == close_source.lua_sha256,
            "selected BNW open, tab and close callbacks share Lua source");

        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, content::read_xml_metadata("<Context/>")));
        auto live = session();
        auto foreign = session();
        EconomicOverviewLifecycle *lifecycle_ptr = nullptr;
        int processed_calls = 0;
        EconomicOverviewLifecycle lifecycle(live, 7, screens, close_source,
            [&](const EconomicPopupProcessedEvent &event) {
                require(lifecycle_ptr && !screens.visible("EconomicOverview") &&
                        lifecycle_ptr->turn_timer_semaphore() == 0 &&
                        event.player_id == 7 && event.event_id == 42,
                    "Processed follows dequeue, hide and timer decrement");
                ++processed_calls;
            });
        lifecycle_ptr = &lifecycle;
        EconomicOverviewInteraction interaction(live, 7, lifecycle,
                                                open_source, close_source);
        EconomicOverviewActiveTabs tabs(lifecycle, tab_source, close_source);
        EconomicOverviewLiveInput input(live, lifecycle, tabs, interaction);
        int host = 0, foreign_host = 0;
        LiveUiFrame frame{UiFrameSnapshot(20, 7, 12), 1, &host,
                          {}, {}, {}, {}, {}, {}, {}, {}, false};
        EconomicGoldClick gold{7, 42, 12, 20, 1, 1200, 800,
            {40, 4, 110, 30}, open_source.top_panel_lua_sha256,
            open_source.top_panel_xml_sha256};
        require(interaction.gold_click(live, frame.snapshot, gold, 50, 10).status ==
                    EconomicPopupQueueStatus::queued,
            "current selected GoldPerTurn event opens owner popup");
        const auto owner = lifecycle.current_owner(live, frame.snapshot, 1200, 800);
        require(owner.has_value(), "popup exposes measured owner identity");
        EconomicOverviewInputRects rects{*owner,
            {100, 100, 200, 30}, {350, 100, 200, 30}, {100, 700, 150, 30}};
        std::string blocker;
        require(!input.bind(live, frame, rects, true, blocker),
            "owner popup absent from sampled UI frame blocks input");
        frame.snapshot = frame.snapshot.with(std::make_shared<const PopupFrame>(
            *lifecycle.active_popups(live)));
        auto overlap = rects;
        overlap.close_button = overlap.general_button;
        require(!input.bind(live, frame, overlap, true, blocker) && !blocker.empty(),
            "ambiguous measured controls cannot bind");
        require(!input.bind(foreign, frame, rects, true, blocker),
            "foreign session cannot bind live hit geometry");
        require(input.bind(live, frame, rects, std::nullopt, blocker),
            "current owner frame binds selected measured controls");

        const auto outside = input.click(live, frame, 900, 500, 900, 500);
        require(outside.consumed && outside.status ==
                    EconomicOverviewInputStatus::unavailable &&
                tabs.view(live, frame.snapshot).selected == EconomicTab::general,
            "modal outside click is consumed without camera or tab effect");
        const auto mismatch = input.click(live, frame, 110, 110, 360, 110);
        require(mismatch.consumed && mismatch.status ==
                    EconomicOverviewInputStatus::unavailable &&
                tabs.view(live, frame.snapshot).selected == EconomicTab::general,
            "press and release on different controls cannot dispatch");
        const auto released_outside = input.click(live, frame, 110, 110, 900, 500);
        require(released_outside.consumed && released_outside.status ==
                    EconomicOverviewInputStatus::unavailable,
            "release outside hit region cannot dispatch or reach camera");
        LiveUiFrame later{frame.snapshot, 2, &host,
                          {}, {}, {}, {}, {}, {}, {}, {}, false};
        later.snapshot = UiFrameSnapshot(21, 7, 12).with(
            std::make_shared<const PopupFrame>(*lifecycle.active_popups(live)));
        require(input.click(live, later, 110, 110, 110, 110).consumed &&
                input.click(live, later, 110, 110, 110, 110).status ==
                    EconomicOverviewInputStatus::unavailable,
            "new frame sequence invalidates old measured geometry");
        LiveUiFrame wrong_host{frame.snapshot, 1, &foreign_host,
                               {}, {}, {}, {}, {}, {}, {}, {}, false};
        require(input.click(live, wrong_host, 110, 110, 110, 110).consumed &&
                input.click(live, wrong_host, 110, 110, 110, 110).status ==
                    EconomicOverviewInputStatus::unavailable,
            "another host cannot replay measured geometry");
        const auto disabled = input.click(live, frame, 360, 110, 360, 110);
        require(disabled.consumed && disabled.action ==
                    EconomicOverviewInputAction::happiness &&
                disabled.status == EconomicOverviewInputStatus::unavailable,
            "unknown live happiness game option blocks tab callback");
        require(input.bind(live, frame, rects, true, blocker),
            "owner-observed happiness option permits current tab binding");
        const auto happiness = input.click(live, frame, 360, 110, 360, 110);
        require(happiness.consumed && happiness.action ==
                    EconomicOverviewInputAction::happiness &&
                happiness.status == EconomicOverviewInputStatus::performed &&
                tabs.view(live, frame.snapshot).selected == EconomicTab::happiness,
            "measured Happiness callback performs local authored SetHide transition");
        const auto general = input.click(live, frame, 110, 110, 110, 110);
        require(general.consumed && general.status ==
                    EconomicOverviewInputStatus::performed &&
                tabs.view(live, frame.snapshot).selected == EconomicTab::general,
            "measured General callback restores local view");
        const auto closed = input.click(live, frame, 110, 710, 110, 710);
        require(closed.consumed && closed.action ==
                    EconomicOverviewInputAction::close &&
                closed.status == EconomicOverviewInputStatus::performed &&
                !screens.visible("EconomicOverview") &&
                lifecycle.turn_timer_semaphore() == 0 &&
                lifecycle.processed_events().size() == 1 && processed_calls == 1 &&
                lifecycle.processed_events().front().event_id == 42,
            "Close dispatches dequeue, hide, timer and Processed in source order");
        require(!input.click(live, frame, 110, 710, 110, 710).consumed &&
                lifecycle.processed_events().size() == 1,
            "closed popup cannot replay close or suppress camera as modal");
        std::cout << "selected BNW EconomicOverview live input: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
