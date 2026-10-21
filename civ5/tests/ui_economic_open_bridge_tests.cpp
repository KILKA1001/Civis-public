#include "civ5/ui/economic_overview_open_bridge.hpp"
#include <filesystem>
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
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};
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
        require(argc == 2, "installed game root required");
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->add_file(root / "Assets/UI/InGame/TopPanel.xml");
        base->add_file(root / "Assets/UI/InGame/Popups/EconomicOverview.lua");
        bnw->add_file(root / "Assets/DLC/Expansion2/UI/InGame/TopPanel.lua");
        assets::ResourceMountTable mounts(assets::resource_lookup(base), {},
                                          assets::resource_keys(base));
        mounts.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
                      assets::resource_keys(bnw)});
        const auto proof = inspect_economic_gold_source(mounts);
        require(proof.verified && !proof.top_panel_lua_sha256.empty() &&
                    proof.top_panel_lua_path.find("Expansion2") != std::string::npos &&
                    !proof.economic_lua_sha256.empty(),
                "selected BNW gold callback and EconomicOverview listener verified");
        auto missing = std::make_shared<assets::LooseResourceIndex>(root);
        assets::ResourceMountTable no_source(assets::resource_lookup(missing));
        require(!inspect_economic_gold_source(no_source).verified,
                "missing authored callback stays blocked");

        const auto in_game = content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/EconomicOverview" ID="EconomicOverview" Hidden="1"/>
        </Context>)");
        UiScreenController screens(UiContextRegistry::from_documents(
            in_game, content::read_xml_metadata("<Context/>")));
        auto live = session();
        auto foreign = session();
        EconomicCloseSourceProof close_source;
        close_source.verified = true;
        close_source.lua_sha256 = proof.economic_lua_sha256;
        close_source.xml_path = "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml";
        EconomicOverviewLifecycle lifecycle(live, 7, screens, close_source);
        EconomicOverviewOpenBridge bridge(live, 7, lifecycle, proof);
        UiFrameSnapshot frame(20, 7, 12);
        EconomicGoldClick click{7, 33, 12, 20, 1, 1200, 800,
            {40, 4, 110, 30}, proof.top_panel_lua_sha256,
            proof.top_panel_xml_sha256};
        require(!lifecycle.active_popups(live) && !screens.visible("EconomicOverview"),
                "source and saved frame do not fabricate a popup");
        require(!bridge.click(live, frame, click, 200, 10).consumed &&
                    !screens.visible("EconomicOverview"),
                "outside click does not emit event");
        require(bridge.click(foreign, frame, click, 50, 10).status ==
                    EconomicPopupQueueStatus::blocked &&
                    !screens.visible("EconomicOverview"),
                "foreign session cannot emit event");
        require(bridge.click(live, UiFrameSnapshot(21, 7, 12), click, 50, 10).status ==
                    EconomicPopupQueueStatus::blocked &&
                    !screens.visible("EconomicOverview"),
                "stale measured frame cannot emit event");
        auto wrong_source = click;
        wrong_source.top_panel_lua_sha256 = "wrong";
        require(bridge.click(live, frame, wrong_source, 50, 10).status ==
                    EconomicPopupQueueStatus::blocked &&
                    !screens.visible("EconomicOverview"),
                "changed selected Lua cannot emit event");
        const auto opened = bridge.click(live, frame, click, 50, 10);
        require(opened.consumed && opened.status == EconomicPopupQueueStatus::queued &&
                    screens.visible("EconomicOverview") &&
                    lifecycle.turn_timer_semaphore() == 1,
                "actual owner click queues popup in Civis UI owner");
        const auto popup = lifecycle.active_popups(live);
        require(popup && popup->events.size() == 1 &&
                    popup->events.front().event_id == 33 &&
                    popup->owner_sequence == 1,
                "popup publication preserves click event order and identity");
        require(bridge.click(live, frame, click, 50, 10).status ==
                    EconomicPopupQueueStatus::blocked &&
                    lifecycle.turn_timer_semaphore() == 1,
                "same click sequence never replays");
        std::cout << "selected BNW EconomicOverview open bridge: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
