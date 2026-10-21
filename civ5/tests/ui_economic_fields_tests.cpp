#include "civ5/ui/economic_overview_fields.hpp"
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

const civ5::ui::EconomicFieldValue *field(
    const civ5::ui::EconomicBoundFields &view, std::string_view id) {
    for (const auto &entry : view.fields)
        if (entry.control_id == id) return &entry;
    return nullptr;
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
        const auto parent_xml = root /
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml";
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        base->add_file(root / "Assets/UI/InGame/TopPanel.xml");
        base->add_file(root / "Assets/UI/InGame/Popups/EconomicOverview.lua");
        for (const char *name : {"EconomicOverview.xml",
                                  "EconomicGeneralInfo.xml",
                                  "EconomicGeneralInfo.lua", "HappinessInfo.xml",
                                  "HappinessInfo.lua"})
            bnw->add_file(root / "Assets/DLC/Expansion2/UI/InGame/Popups" / name);
        // TopPanel.lua resides one directory above the popup contexts.
        bnw->add_file(root / "Assets/DLC/Expansion2/UI/InGame/TopPanel.lua");
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
                                              assets::resource_keys(base));
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
                         assets::resource_keys(bnw)});
        OverviewSource parent{"EconomicOverview",
            "Assets/UI/InGame/Popups/EconomicOverview.xml",
            "Assets/DLC/Expansion2/UI/InGame/Popups/EconomicOverview.xml", {}};
        std::function<void(const content::XmlNode &)> collect;
        collect = [&](const content::XmlNode &node) {
            const auto id = node.attribute("ID");
            if (!id.empty()) parent.controls.push_back(
                OverviewControl{id, node.name, {}, {}, {}, {}, {}, {}});
            for (const auto &child : node.children) collect(child);
        };
        collect(content::read_xml(parent_xml));
        const auto open_source = inspect_economic_gold_source(resources);
        const auto close_source = inspect_economic_close_source(resources, parent);
        const auto tab_source = inspect_economic_tab_source(resources, parent);
        const auto field_source = inspect_economic_field_source(resources,
                                                                tab_source);
        check(open_source.verified && close_source.verified &&
                  tab_source.verified && field_source.verified &&
                  field_source.general_lua_bound.contains("TotalGoldValue") &&
                  field_source.positive_happiness_binding,
              "selected parent and child XML/Lua field sources verified");
        auto invalid_tab_source = tab_source;
        invalid_tab_source.lua_sha256 = "wrong";
        check(!inspect_economic_field_source(resources,
                                              invalid_tab_source).verified,
              "changed parent Lua cannot publish child fields");

        auto live = session();
        auto foreign = session();
        auto ui = screens();
        EconomicOverviewLifecycle lifecycle(live, 7, ui, close_source);
        EconomicOverviewInteraction popup(live, 7, lifecycle,
                                          open_source, close_source);
        EconomicOverviewActiveTabs tabs(lifecycle, tab_source, close_source);
        EconomicOverviewFields fields(lifecycle, tabs, field_source);
        OverviewFrameState data;
        data.player_id = 7;
        data.pages["EconomicGeneralInfo"].rows = {
            {"TotalGoldValue", "17.50"},
            {"GrossGoldValue", "3.00"}};
        // The current HappinessInfo snapshot has only an excess diagnostic.
        // It must not be relabelled as GetHappiness()/TotalHappinessValue.
        data.pages["HappinessInfo"].rows = {{"Overall happiness", "8"}};
        UiFrameSnapshot frame = UiFrameSnapshot(20, 7, 12).with(
            std::make_shared<const OverviewFrameState>(data));
        check(!fields.capture(live, frame, 1200, 800).available,
              "live values alone do not create a popup field view");
        EconomicGoldClick gold{7, 42, 12, 20, 1, 1200, 800,
            {40, 4, 110, 30}, open_source.top_panel_lua_sha256,
            open_source.top_panel_xml_sha256};
        check(popup.gold_click(live, frame, gold, 50, 10).status ==
                    EconomicPopupQueueStatus::queued,
              "measured gold event opens owner");
        const auto current = lifecycle.current_owner(live, frame, 1200, 800);
        check(current.has_value(), "current popup owner");
        EconomicTabMeasurement measured{*current,
            {100, 100, 280, 32}, {400, 100, 280, 32}};
        std::string blocker;
        check(tabs.bind(live, frame, measured, true, blocker),
              "selected tab controls bind to owner");
        const auto general = fields.capture(live, frame, 1200, 800);
        check(general.available && general.tab == EconomicTab::general &&
                  general.owner_sequence == 1 && general.player_id == 7 &&
                  field(general, "TotalGoldValue") &&
                  field(general, "TotalGoldValue")->value == "17.50" &&
                  field(general, "NetGoldValue") &&
                  !field(general, "NetGoldValue")->value &&
                  !field(general, "NetGoldValue")->unavailable_reason.empty(),
              "General shows only completed current-frame values");
        check(!fields.capture(foreign, frame, 1200, 800).available &&
                  !fields.capture(live, UiFrameSnapshot(21, 7, 12),
                                  1200, 800).available,
              "foreign session and stale frame publish no fields");
        check(tabs.click(live, frame, 410, 110).performed,
              "authored Happiness tab callback runs locally");
        const auto happiness = fields.capture(live, frame, 1200, 800);
        check(happiness.available && happiness.tab == EconomicTab::happiness &&
                  field(happiness, "TotalHappinessValue") &&
                  !field(happiness, "TotalHappinessValue")->value &&
                  !field(happiness, "TotalHappinessValue")->unavailable_reason.empty(),
              "excess diagnostic does not invent frame-bound GetHappiness");
        check(popup.close(live, frame,
                    EconomicCloseMeasurement{*current, {100, 700, 150, 32}},
                    EconomicCloseInput::button_click, 110, 710).status ==
                    EconomicCloseStatus::performed &&
                    !fields.capture(live, frame, 1200, 800).available,
              "close withdraws all field values");
        std::cout << "selected BNW EconomicOverview bound fields: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
