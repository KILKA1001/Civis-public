#include "civ5/ui/economic_overview_active_tabs.hpp"
#include "civ5/ui/economic_overview_interaction.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void check(bool yes, const char *message) {
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
        check(argc == 2, "installed BNW game root required");
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
        OverviewSource selected{"EconomicOverview",
            "Assets/UI/InGame/Popups/EconomicOverview.xml",
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml", {}};
        std::function<void(const content::XmlNode &)> collect;
        collect = [&](const content::XmlNode &node) {
            const auto id = node.attribute("ID");
            if (!id.empty()) selected.controls.push_back(
                OverviewControl{id, node.name, {}, {}, {}, {}, {}, {}});
            for (const auto &child : node.children) collect(child);
        };
        collect(content::read_xml(xml_path));
        const auto open_source = inspect_economic_gold_source(resources);
        const auto close_source = inspect_economic_close_source(resources, selected);
        const auto tab_source = inspect_economic_tab_source(resources, selected);
        check(open_source.verified && close_source.verified &&
                  tab_source.verified &&
                  tab_source.lua_sha256 == close_source.lua_sha256,
              "selected callback, tabs and popup share source identity");

        auto live = session();
        auto foreign = session();
        auto ui = screens();
        EconomicOverviewLifecycle lifecycle(live, 7, ui, close_source);
        EconomicOverviewInteraction popup(live, 7, lifecycle,
                                          open_source, close_source);
        EconomicOverviewActiveTabs tabs(lifecycle, tab_source, close_source);
        UiFrameSnapshot frame(20, 7, 12);
        check(!tabs.view(live, frame).available,
              "no popup event means no active tab");
        EconomicGoldClick gold{7, 42, 12, 20, 1, 1200, 800,
            {40, 4, 110, 30}, open_source.top_panel_lua_sha256,
            open_source.top_panel_xml_sha256};
        check(popup.gold_click(live, frame, gold, 50, 10).status ==
                    EconomicPopupQueueStatus::queued,
              "measured gold callback opens EconomicOverview owner");
        const auto owner = lifecycle.current_owner(live, frame, 1200, 800);
        check(owner.has_value(), "opened popup has an owner");
        EconomicTabMeasurement measured{*owner,
            {100, 100, 280, 32}, {400, 100, 280, 32}};
        std::string blocker;
        check(!tabs.bind(foreign, frame, measured, true, blocker) &&
                  !tabs.view(foreign, frame).available,
              "foreign session cannot bind tab geometry");
        check(tabs.bind(live, frame, measured, std::nullopt, blocker),
              "real popup binds measured selected BNW tabs");
        auto view = tabs.view(live, frame);
        check(view.available && view.selected == EconomicTab::general &&
                  view.general_visible && view.general_highlight &&
                  !view.happiness_visible,
              "authored OnGeneralInfoButton is initial active tab");
        const auto disabled = tabs.click(live, frame, 410, 110);
        check(disabled.consumed && !disabled.performed &&
                  tabs.view(live, frame).general_visible,
              "unknown happiness game option blocks Happiness callback");
        check(tabs.click(live, frame, 110, 110).performed &&
                  tabs.view(live, frame).general_visible,
              "measured General callback performs real local SetHide state");
        check(!tabs.click(live, UiFrameSnapshot(21, 7, 12), 110, 110).performed,
              "stale frame cannot select a tab");
        check(tabs.bind(live, frame, measured, true, blocker),
              "owner-supplied happiness option allows rebind");
        check(tabs.click(live, frame, 410, 110).performed &&
                  tabs.view(live, frame).happiness_visible,
              "authored Happiness callback changes four local visible states");
        UiFrameSnapshot next_frame(21, 7, 12);
        measured.owner.frame = 21;
        check(tabs.bind(live, next_frame, measured, true, blocker) &&
                  tabs.view(live, next_frame).happiness_visible &&
                  !tabs.view(live, frame).available,
              "same popup keeps tab across fresh frame and retires old frame");
        check(tabs.click(live, next_frame, 110, 110).performed &&
                  tabs.view(live, next_frame).general_visible,
              "General callback restores initial panel");
        const auto current = lifecycle.current_owner(live, next_frame, 1200, 800);
        check(current.has_value(), "close uses current frame owner");
        check(popup.close(live, next_frame,
                    EconomicCloseMeasurement{*current, {100, 700, 150, 32}},
                    EconomicCloseInput::button_click, 110, 710).status ==
                    EconomicCloseStatus::performed &&
                    !tabs.view(live, next_frame).available &&
                    !tabs.click(live, next_frame, 110, 110).performed,
              "closed popup withdraws all tab hits");
        std::cout << "selected BNW active EconomicOverview tab: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
