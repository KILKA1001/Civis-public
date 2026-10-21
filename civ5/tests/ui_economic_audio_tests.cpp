#include "civ5/ui/economic_overview_audio.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
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
        check(argc == 2, "installed BNW root required");
        const std::filesystem::path root(argv[1]);
        const auto economic_xml = root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml";
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        for (const char *relative : {
                 "Assets/UI/InGame/TopPanel.xml",
                 "Assets/UI/InGame/Popups/EconomicOverview.lua",
                 "Assets/Sounds/XML/ButtonPopupSounds.xml",
                 "Assets/Sounds/XML/NotificationSounds.xml"})
            base->add_file(root / relative);
        bnw->add_file(root / "Assets/DLC/Expansion2/UI/InGame/TopPanel.lua");
        bnw->add_file(economic_xml);
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
                                              assets::resource_keys(base));
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
                         assets::resource_keys(bnw)});
        const auto open_source = inspect_economic_gold_source(resources);
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
        auto catalog = UiAudioCatalog::from_resources(resources);
        const auto audio_source = inspect_economic_audio_source(
            resources, catalog, open_source, close_source);
        check(open_source.verified && close_source.verified &&
                  audio_source.verified && audio_source.popup_is_silent &&
                  !audio_source.popup_audio_xml_sha256.empty() &&
                  catalog.find(UiAudioKind::popup,
                    "BUTTONPOPUP_ECONOMIC_OVERVIEW")->script_id ==
                    "AS2D_PLACEHOLDER_BLANK",
              "selected BNW popup audio row is authored silent");
        auto changed = open_source;
        changed.economic_lua_sha256 = "wrong";
        check(!inspect_economic_audio_source(resources, catalog, changed,
                                             close_source).verified,
              "changed popup Lua cannot verify silence");

        auto live = session();
        auto foreign = session();
        auto ui = screens();
        EconomicOverviewLifecycle owner(live, 7, ui, close_source);
        EconomicOverviewInteraction interaction(live, 7, owner,
                                                open_source, close_source);
        EconomicOverviewAudioInteraction audio(interaction, std::move(catalog),
                                                audio_source);
        UiFrameSnapshot frame(20, 7, 12);
        EconomicGoldClick gold{7, 42, 12, 20, 1, 1200, 800,
            {40, 4, 110, 30}, open_source.top_panel_lua_sha256,
            open_source.top_panel_xml_sha256};
        check(audio.pending_audio() == 0 && audio.drain_audio().empty(),
              "render/source inspection does not emit audio");
        const auto foreign_click = audio.gold_click(foreign, frame, gold, 50, 10);
        check(foreign_click.popup.status == EconomicPopupQueueStatus::blocked &&
                  !foreign_click.audio && audio.pending_audio() == 0,
              "foreign click emits neither popup nor sound");
        const auto opened = audio.gold_click(live, frame, gold, 50, 10);
        check(opened.popup.status == EconomicPopupQueueStatus::queued &&
                  opened.audio == UiAudioEmitStatus::silent &&
                  audio.pending_audio() == 0 && audio.drain_audio().empty() &&
                  ui.visible("EconomicOverview"),
              "real popup transition reports authored silence, no cue");
        const auto replay = audio.gold_click(live, frame, gold, 50, 10);
        check(replay.popup.status == EconomicPopupQueueStatus::blocked &&
                  !replay.audio && audio.drain_audio().empty(),
              "repeated click cannot emit audio");
        const auto current = owner.current_owner(live, frame, 1200, 800);
        check(current.has_value(), "current popup owner");
        const auto closed = audio.close(live, frame,
            EconomicCloseMeasurement{*current, {100, 700, 150, 32}},
            EconomicCloseInput::button_click, 110, 710);
        check(closed.status == EconomicCloseStatus::performed &&
                  owner.processed_events().size() == 1 &&
                  audio.drain_audio().empty(),
              "authored close has no invented cue");
        std::cout << "selected BNW EconomicOverview authored silence: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
