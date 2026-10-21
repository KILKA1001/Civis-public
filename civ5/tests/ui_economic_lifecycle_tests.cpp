#include "civ5/ui/economic_overview_lifecycle.hpp"
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
    source.snapshot.game_state.layout =
        saves::GameStateLayout::expansion2_version1;
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
}

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        require(argc == 2, "installed game root required");
        const std::filesystem::path root(argv[1]);
        const auto lua_path = root / "Assets/UI/InGame/Popups/EconomicOverview.lua";
        const auto xml_path = root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml";
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->add_file(lua_path);
        bnw->add_file(xml_path);
        assets::ResourceMountTable mounts(assets::resource_lookup(base), {},
                                           assets::resource_keys(base));
        mounts.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
                      assets::resource_keys(bnw)});
        const auto xml = content::read_xml(xml_path);
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
        collect(xml);
        const auto proof = inspect_economic_close_source(mounts, overview);
        require(proof.verified, "selected BNW close source verified");
        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        const auto world_view = content::read_xml_metadata("<Context/>");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, world_view));
        auto live = session();
        auto foreign = session();
        int delivered = 0;
        EconomicOverviewLifecycle owner(live, 7, screens, proof,
            [&](const EconomicPopupProcessedEvent &event) {
                require(event.player_id == 7 && event.processed_result == 0,
                        "synchronous BNW Processed payload");
                ++delivered;
            });
        UiFrameSnapshot frame(1, 7, 12);
        EconomicPopupRequest request{"BUTTONPOPUP_ECONOMIC_OVERVIEW",
                                     7, 30, 12, 0, 1};
        require(!owner.active_popups(live) && !screens.visible("EconomicOverview"),
                "saved state alone does not open popup");
        require(owner.queue(foreign, frame, request).status ==
                    EconomicPopupQueueStatus::blocked,
                "foreign live session rejected");
        auto wrong_type = request;
        wrong_type.button_popup_type = "BUTTONPOPUP_CHOOSETECH";
        require(owner.queue(live, frame, wrong_type).status ==
                    EconomicPopupQueueStatus::blocked,
                "typed non-Economic popup rejected");
        require(owner.queue(live, frame, request).status ==
                    EconomicPopupQueueStatus::queued &&
                screens.visible("EconomicOverview") &&
                owner.turn_timer_semaphore() == 1 &&
                owner.current_priority() == EconomicPopupPriority::economic_overview,
                "queue mutates local UIManager, visibility and timer");
        const auto popups = owner.active_popups(live);
        require(popups && popups->owner_sequence == 1 &&
                popups->events.size() == 1 &&
                popups->events.front().event_id == 30 &&
                !owner.active_popups(foreign),
                "owner publishes only current session event");
        const auto active = owner.current_owner(live, frame, 1200, 800);
        require(active && !owner.current_owner(foreign, frame, 1200, 800),
                "measured owner identity binds to live session");
        EconomicOverviewClose close;
        EconomicCloseMeasurement measurement{*active, {100, 700, 150, 32}};
        std::string blocker;
        require(close.bind(frame, *popups, proof, measurement, true, blocker),
                "selected popup close hit binds");
        const auto performed = close.activate(*active,
            EconomicCloseInput::button_click, 110, 710,
            owner.close_service(live, frame, 1200, 800));
        require(performed.status == EconomicCloseStatus::performed &&
                !screens.visible("EconomicOverview") &&
                owner.turn_timer_semaphore() == 0 &&
                owner.processed_events().size() == 1 && delivered == 1 &&
                !owner.active_popups(live),
                "close executes dequeue, hidden, timer and Processed in owner");
        require(owner.queue(live, frame, request).status ==
                    EconomicPopupQueueStatus::blocked,
                "completed sequence cannot replay");

        auto next = request;
        next.event_id = 31;
        next.owner_sequence = 2;
        next.data1 = 1;
        UiFrameSnapshot next_frame(2, 7, 12);
        require(owner.queue(live, next_frame, next).status ==
                    EconomicPopupQueueStatus::queued &&
                owner.current_priority() == EconomicPopupPriority::in_game_utmost,
                "Data1=1 opens at authored high priority when hidden");
        auto toggle = next;
        toggle.event_id = 32;
        toggle.owner_sequence = 3;
        require(owner.queue(live, next_frame, toggle).status ==
                    EconomicPopupQueueStatus::blocked &&
                owner.turn_timer_semaphore() == 1,
                "visible Data1=1 toggle is an explicit blocked close boundary");
        const auto active2 = owner.current_owner(live, next_frame, 1200, 800);
        const auto popups2 = owner.active_popups(live);
        EconomicOverviewClose close2;
        require(active2 && popups2 &&
                close2.bind(next_frame, *popups2, proof,
                    EconomicCloseMeasurement{*active2, {100, 700, 150, 32}},
                    true, blocker),
                "second event owns its own close");
        require(close2.activate(*active2, EconomicCloseInput::escape_key,
            0, 0, owner.close_service(live, next_frame, 1200, 800)).status ==
                EconomicCloseStatus::performed &&
                owner.processed_events().size() == 2 && delivered == 2,
                "second owner close publishes exactly one new Processed event");

        UiScreenController failing_screens(UiContextRegistry::from_documents(
            in_game, world_view));
        EconomicOverviewLifecycle failing(live, 7, failing_screens, proof,
            [](const EconomicPopupProcessedEvent &) {
                throw std::runtime_error("listener may have observed Processed");
            });
        auto partial = request;
        partial.event_id = 40;
        partial.owner_sequence = 5;
        UiFrameSnapshot partial_frame(3, 7, 12);
        require(failing.queue(live, partial_frame, partial).status ==
                    EconomicPopupQueueStatus::queued,
                "independent partial owner queues");
        const auto partial_owner = failing.current_owner(live, partial_frame, 1200, 800);
        const auto partial_popup = failing.active_popups(live);
        EconomicOverviewClose partial_close;
        require(partial_owner && partial_popup &&
                partial_close.bind(partial_frame, *partial_popup, proof,
                    EconomicCloseMeasurement{*partial_owner, {100, 700, 150, 32}},
                    true, blocker),
                "partial owner close binds");
        require(partial_close.activate(*partial_owner,
            EconomicCloseInput::button_click, 110, 710,
            failing.close_service(live, partial_frame, 1200, 800)).status ==
                EconomicCloseStatus::uncertain &&
                failing.turn_timer_semaphore() == 0 &&
                failing.processed_events().size() == 1 &&
                !failing_screens.visible("EconomicOverview"),
                "throw after journal delivery is uncertain, never performed");
        auto another = partial;
        another.event_id = 41;
        another.owner_sequence = 6;
        require(failing.queue(live, partial_frame, another).status ==
                    EconomicPopupQueueStatus::blocked &&
                !failing.close_service(live, partial_frame, 1200, 800),
                "partial owner remains occupied and one-shot");
        std::cout << "selected BNW economic lifecycle fixture passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
