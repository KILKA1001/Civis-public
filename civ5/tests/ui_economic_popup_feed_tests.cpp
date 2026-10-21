#include "civ5/ui/economic_overview_popup_feed.hpp"
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
    return runtime::LiveSession::from_restored_bnw(std::move(source), std::move(state));
}
} // namespace

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
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
            assets::resource_keys(base));
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
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
        const auto proof = inspect_economic_close_source(resources, overview);
        require(proof.verified && !proof.lua_sha256.empty(),
            "selected BNW popup source must be verified");

        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, content::read_xml_metadata("<Context/>")));
        auto live = session();
        auto foreign = session();
        EconomicOverviewLifecycle owner(live, 7, screens, proof);
        const auto provider = make_economic_overview_popup_feed_provider(live, owner);
        LiveSessionUiSources frame_sources;
        frame_sources.active_popups = provider;
        require(!provider(live) && !provider(foreign),
            "no owner event means no popup feed");
        UiFrameSnapshot frame(20, 7, 12);
        require(owner.queue(live, frame,
            {"BUTTONPOPUP_ECONOMIC_OVERVIEW", 7, 44, 12, 0, 9}).status ==
                EconomicPopupQueueStatus::queued,
            "selected current event queues popup");
        const auto result = capture_economic_overview_popup_feed(live, owner,
            &frame, 1200, 800);
        require(result.feed && result.feed->player_id == 7 &&
                result.feed->turn == 12 && result.feed->owner_sequence == 9 &&
                result.feed->events.size() == 1 &&
                result.feed->events.front().context_id == "EconomicOverview" &&
                result.feed->events.front().event_id == 44,
            "adapter preserves owner event identity");
        require(frame_sources.active_popups(live) && !provider(foreign) &&
                !capture_economic_overview_popup_feed(foreign, owner).feed,
            "provider is bound to one live session");
        UiFrameSnapshot foreign_player(21, 8, 12);
        require(!capture_economic_overview_popup_feed(live, owner,
                    &foreign_player, 1200, 800).feed,
            "foreign player frame is rejected");
        UiFrameSnapshot stale_turn(21, 7, 13);
        require(!capture_economic_overview_popup_feed(live, owner,
                    &stale_turn, 1200, 800).feed,
            "stale frame turn is rejected");
        require(!capture_economic_overview_popup_feed(live, owner,
                    &frame, 0, 800).feed,
            "missing measured viewport is rejected");
        PopupFrame missing = *owner.active_popups(live);
        missing.owner_sequence = 0;
        require(!project_economic_popup_frame(missing).feed &&
                !project_economic_popup_frame(missing).blocker.empty(),
            "missing required PopupFrame identity has explicit blocker");
        missing = *owner.active_popups(live);
        missing.events.front().event_id = -1;
        require(!project_economic_popup_frame(missing).feed,
            "missing event ID cannot be published");
        live.mutable_state().setup.game_turn = 13;
        require(!provider(live) &&
                !capture_economic_overview_popup_feed(live, owner).feed,
            "stale owner turn is withdrawn");
        live.mutable_state().setup.game_turn = 12;
        const auto current = owner.current_owner(live, frame, 1200, 800);
        require(current.has_value(), "close has current owner");
        EconomicOverviewClose close;
        std::string blocker;
        require(close.bind(frame, *owner.active_popups(live), proof,
                EconomicCloseMeasurement{*current, {100, 700, 150, 32}},
                true, blocker), "measured close binds");
        require(close.activate(*current, EconomicCloseInput::button_click,
                    110, 710, owner.close_service(live, frame, 1200, 800)).status ==
                EconomicCloseStatus::performed && !provider(live),
            "closed owner withdraws popup feed");
        std::cout << "selected BNW EconomicOverview popup feed: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
