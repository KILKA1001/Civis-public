#include "civ5/ui/economic_overview_interaction.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool yes, const char *message) {
    if (!yes) throw std::runtime_error(message);
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
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    state.setup.game_turn = 12;
    state.setup.players[7].alive = true;
    return runtime::LiveSession::from_restored_bnw(std::move(source),
                                                   std::move(state));
}

civ5::ui::UiScreenController screens() {
    using namespace civ5;
    const auto in_game = content::read_xml_metadata(R"(<Context>
      <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
      <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
    </Context>)");
    return ui::UiScreenController(ui::UiContextRegistry::from_documents(
        in_game, content::read_xml_metadata("<Context/>")));
}
} // namespace

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        require(argc == 2, "installed BNW root required");
        const std::filesystem::path root(argv[1]);
        const auto economic_xml = root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml";
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->add_file(root / "Assets/UI/InGame/TopPanel.xml");
        base->add_file(root / "Assets/UI/InGame/Popups/EconomicOverview.lua");
        bnw->add_file(root / "Assets/DLC/Expansion2/UI/InGame/TopPanel.lua");
        bnw->add_file(economic_xml);
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
                                              assets::resource_keys(base));
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
                         assets::resource_keys(bnw)});
        const auto open_source = inspect_economic_gold_source(resources);
        require(open_source.verified, "selected gold event source");
        OverviewSource economic{"EconomicOverview",
            "Assets/UI/InGame/Popups/EconomicOverview.xml",
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml", {}};
        std::function<void(const content::XmlNode &)> collect;
        collect = [&](const content::XmlNode &node) {
            const auto id = node.attribute("ID");
            if (!id.empty()) economic.controls.push_back(
                OverviewControl{id, node.name, {}, {}, {}, {}, {}, {}});
            for (const auto &child : node.children) collect(child);
        };
        collect(content::read_xml(economic_xml));
        const auto close_source = inspect_economic_close_source(resources, economic);
        require(close_source.verified &&
                    close_source.lua_sha256 == open_source.economic_lua_sha256,
                "selected popup listener and close source share one payload");

        auto live = session();
        auto foreign = session();
        auto ui = screens();
        int delivered = 0;
        EconomicOverviewLifecycle owner(live, 7, ui, close_source,
            [&](const EconomicPopupProcessedEvent &event) {
                require(event.player_id == 7 && event.event_id == 42 &&
                        event.owner_sequence == 1 && event.processed_result == 0,
                        "processed preserves gold click identity");
                ++delivered;
            });
        EconomicOverviewInteraction interaction(live, 7, owner,
                                                open_source, close_source);
        UiFrameSnapshot frame(21, 7, 12);
        EconomicGoldClick gold{7, 42, 12, 21, 1, 1200, 800,
            {40, 4, 110, 30}, open_source.top_panel_lua_sha256,
            open_source.top_panel_xml_sha256};
        require(!owner.active_popups(live) && !ui.visible("EconomicOverview"),
                "owner does not open popup before the measured event");
        require(interaction.gold_click(foreign, frame, gold, 50, 10).status ==
                    EconomicPopupQueueStatus::blocked &&
                    !ui.visible("EconomicOverview"),
                "foreign session cannot emit gold popup");
        require(interaction.gold_click(live, UiFrameSnapshot(22, 7, 12),
                    gold, 50, 10).status == EconomicPopupQueueStatus::blocked &&
                    !ui.visible("EconomicOverview"),
                "stale measured gold frame cannot emit popup");
        require(interaction.gold_click(live, frame, gold, 50, 10).status ==
                    EconomicPopupQueueStatus::queued &&
                    ui.visible("EconomicOverview") &&
                    owner.turn_timer_semaphore() == 1,
                "measured authored gold click opens one local popup");
        require(interaction.gold_click(live, frame, gold, 50, 10).status ==
                    EconomicPopupQueueStatus::blocked,
                "gold event cannot replay");
        const auto current = owner.current_owner(live, frame, 1200, 800);
        require(current && current->owner_sequence == 1,
                "close owner follows the gold click event");
        EconomicCloseMeasurement close_measurement{*current,
            {100, 700, 150, 32}};
        require(interaction.close(foreign, frame, close_measurement,
                    EconomicCloseInput::button_click, 110, 710).status ==
                    EconomicCloseStatus::unavailable &&
                    owner.turn_timer_semaphore() == 1,
                "foreign session cannot close popup");
        require(interaction.close(live, UiFrameSnapshot(22, 7, 12),
                    close_measurement, EconomicCloseInput::button_click,
                    110, 710).status == EconomicCloseStatus::unavailable &&
                    owner.turn_timer_semaphore() == 1,
                "stale close frame cannot mutate owner");
        require(interaction.close(live, frame, close_measurement,
                    EconomicCloseInput::button_click, 110, 710).status ==
                    EconomicCloseStatus::performed &&
                    !ui.visible("EconomicOverview") &&
                    owner.turn_timer_semaphore() == 0 &&
                    owner.processed_events().size() == 1 && delivered == 1,
                "close dequeues, hides, decrements timer, publishes once");
        require(interaction.close(live, frame, close_measurement,
                    EconomicCloseInput::button_click, 110, 710).status ==
                    EconomicCloseStatus::unavailable &&
                    owner.processed_events().size() == 1 && delivered == 1,
                "repeat close cannot republish Processed");

        auto partial_ui = screens();
        int partial_deliveries = 0;
        EconomicOverviewLifecycle partial_owner(live, 7, partial_ui,
            close_source, [&](const EconomicPopupProcessedEvent &) {
                ++partial_deliveries;
                throw std::runtime_error("sink observed event before failure");
            });
        EconomicOverviewInteraction partial(live, 7, partial_owner,
                                            open_source, close_source);
        require(partial.gold_click(live, frame, gold, 50, 10).status ==
                    EconomicPopupQueueStatus::queued,
                "independent owner opens for partial-effect test");
        const auto partial_current = partial_owner.current_owner(live, frame,
                                                                   1200, 800);
        require(partial_current.has_value(), "partial close owner present");
        const EconomicCloseMeasurement partial_measurement{*partial_current,
            {100, 700, 150, 32}};
        require(partial.close(live, frame, partial_measurement,
                    EconomicCloseInput::button_click, 110, 710).status ==
                    EconomicCloseStatus::uncertain &&
                    partial_owner.processed_events().size() == 1 &&
                    partial_deliveries == 1 &&
                    partial_owner.turn_timer_semaphore() == 0 &&
                    !partial_ui.visible("EconomicOverview"),
                "throw after synchronous journal delivery stays uncertain");
        require(partial.close(live, frame, partial_measurement,
                    EconomicCloseInput::button_click, 110, 710).status ==
                    EconomicCloseStatus::unavailable &&
                    partial_owner.processed_events().size() == 1 &&
                    partial_deliveries == 1,
                "partial close cannot deliver Processed twice");
        auto later = gold;
        later.event_id = 43;
        later.owner_sequence = 2;
        require(partial.gold_click(live, frame, later, 50, 10).status ==
                    EconomicPopupQueueStatus::blocked,
                "partial owner cannot reopen over uncertain close");
        std::cout << "selected BNW economic interaction: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
