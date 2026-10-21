#include "civ5/ui/bnw_city_production_backend.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;

void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}

runtime::LiveSession session() {
    saves::SavedSession source;
    source.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{
        source.snapshot, runtime::ResearchContext{}, {}, runtime::PlayerResearchContext{},
        runtime::CityYieldContext{}, runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{}, runtime::TreasuryContext{},
        runtime::ScienceContext{}, runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {5, 0, 0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {7, 0, 0})},
        runtime::ResearchSelectionState{}, runtime::BnwGameClockState{},
        {}, {}, {}, {}, {}, {}, {}, {}, {}};
    state.setup.game_turn = 12;
    state.setup.players[5].alive = true;
    auto &city = state.cities.players[5].cities[9];
    city.owner = 5;
    city.id = 9;
    city.production_orders = std::vector<runtime::CityYieldState::ProductionOrder>{
        {4, 1, -1, false, false, std::nullopt}};
    city.production_progress.emplace();
    return runtime::LiveSession::from_restored_bnw(std::move(source), std::move(state));
}

ui::UiFrameSnapshot frame_from(const runtime::LiveSession &live, std::uint64_t number) {
    const auto &current = live.state().cities.players.at(5).cities.at(9);
    ui::CityScreenFrame city_frame;
    city_frame.player_id = 5;
    city_frame.selected_city_id = 9;
    ui::CityScreenCity city;
    city.owner = 5;
    city.id = 9;
    city.queue_resolved = current.production_orders.has_value();
    for (const auto &order : *current.production_orders)
        city.queue.push_back({order.type, order.item, std::nullopt});
    city_frame.cities.push_back(std::move(city));
    return ui::UiFrameSnapshot(number, 5,
        static_cast<std::int32_t>(live.state().setup.game_turn))
        .with<ui::CityScreenFrame>(
            std::make_shared<const ui::CityScreenFrame>(std::move(city_frame)));
}

ui::UiContextRegistry registry() {
    return ui::UiContextRegistry::from_documents(
        content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView"
                      ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/ProductionPopup"
                      ID="ProductionPopup" Hidden="True"/>
        </Context>)"),
        content::read_xml_metadata("<Context/>"));
}

ui::CityScreenSource popup_source() {
    ui::CityScreenSource source;
    source.context_id = "ProductionPopup";
    source.selected_source_path = "Assets/DLC/Expansion2/UI/InGame/Popups/ProductionPopup.xml";
    source.document = content::read_xml_metadata(R"(<Context>
      <Grid ID="Backdrop" Anchor="L,T" Offset="16,150" Size="380,300">
        <Box Anchor="R,T" Offset="18,46" Size="340,335">
          <ScrollPanel ID="ScrollPanel" Anchor="R,T" Size="340,400">
            <UpButton ID="UpButton" Style="ScrollBarUp" AnchorSide="O.I"/>
            <DownButton ID="DownButton" Style="ScrollBarDown" AnchorSide="O.I"/>
            <Stack ID="StackOStacks" Size="256,32"/>
          </ScrollPanel>
        </Box>
      </Grid>
      <Label ID="CityName"/>
      <Button ID="CloseButton" Anchor="L,T" Size="40,24"/>
      <Instance Name="ProdButton"><Button ID="Button" Size="340,42"/></Instance>
    </Context>)");
    source.control_ids = {"StackOStacks", "ScrollPanel", "UpButton", "DownButton"};
    return source;
}
} // namespace

int main() {
    try {
        auto live = session();
        runtime::ProductionProgressRules rules;
        rules.units = {2};
        rules.buildings = {3, 4, 5, 6};
        rules.projects = {1};
        rules.processes = {1};
        rules.info_identity["Units"][2] = "UNIT_WARRIOR";
        rules.info_identity["Buildings"][3] = "BUILDING_MARKET";
        rules.info_identity["Buildings"][4] = std::nullopt;
        rules.info_identity["Buildings"][5] = "BUILDING_DUPLICATE";
        rules.info_identity["Buildings"][6] = "BUILDING_DUPLICATE";
        rules.info_identity["Projects"][1] = "PROJECT_UNKNOWN";
        rules.info_identity["Processes"][1] = "PROCESS_WEALTH";
        content::RulesDatabase database;
        database.apply_sql(
            "CREATE TABLE Units (ID INTEGER, Type TEXT, Description TEXT);"
            "CREATE TABLE Buildings (ID INTEGER, Type TEXT, Description TEXT);"
            "CREATE TABLE Projects (ID INTEGER, Type TEXT, Description TEXT);"
            "CREATE TABLE Processes (ID INTEGER, Type TEXT, Description TEXT);"
            "INSERT INTO Units VALUES (2, 'UNIT_WARRIOR', 'TXT_KEY_UNIT_WARRIOR');"
            "INSERT INTO Buildings VALUES (3, 'BUILDING_MARKET', 'TXT_KEY_BUILDING_MARKET');"
            "INSERT INTO Buildings VALUES (5, 'BUILDING_DUPLICATE', 'TXT_KEY_DUPLICATE');"
            "INSERT INTO Buildings VALUES (6, 'BUILDING_DUPLICATE', 'TXT_KEY_DUPLICATE');"
            "INSERT INTO Projects VALUES (1, 'PROJECT_UNKNOWN', 'TXT_KEY_UNKNOWN');"
            "INSERT INTO Processes VALUES (1, 'PROCESS_WEALTH', 'TXT_KEY_PROCESS_WEALTH');");
        runtime::BnwProductionOrderServices services;
        services.lua_ready = [] { return true; };
        services.core_can_order_read_only = [](const auto &, auto, auto, const auto &) {
            return true;
        };
        services.lua_test_all_read_only = [](auto, auto, auto, auto,
            const auto &) -> std::optional<bool> { return std::nullopt; };
        services.default_unit_ai_read_only = [](const auto &, auto) ->
            std::optional<std::int64_t> { return 0; };
        services.apply_owned_effect = [](auto &, auto, auto, auto, const auto &) {};
        services.dirty_after_commit = [](auto, auto) {};
        int previews = 0;
        ui::BnwCityProductionUiBackend backend(live, rules, services,
            [&](const auto &, auto owner, auto city,
                const runtime::BnwProductionChoice &choice) {
                ++previews;
                require(owner == 5 && city == 9, "preview must use owned city");
                return choice.kind == runtime::BnwProductionKind::construct &&
                       choice.item == 3
                    ? ui::UiCommandResult{ui::UiCommandStatus::ready, {}}
                    : ui::UiCommandResult{ui::UiCommandStatus::unavailable,
                                          "Native eligibility unavailable"};
            }, &database, [](std::string_view key) {
                if (key == "TXT_KEY_UNIT_WARRIOR") return std::string("Warrior");
                if (key == "TXT_KEY_BUILDING_MARKET") return std::string("Market");
                if (key == "TXT_KEY_PROCESS_WEALTH") return std::string("Wealth");
                return std::string(key);
            });
        ui::LiveUiSources sources;
        sources.production_backend = &backend;
        auto selected_popup = popup_source();
        sources.production_popup_source = &selected_popup;
        std::uint64_t samples = 0;
        ui::LiveUiHost host(registry(), [&] { return frame_from(live, ++samples); },
                            &backend, sources);
        const auto first = host.begin_frame(1280, 540);
        require(!first.snapshot.find<ui::ProductionPopupChoices>() && previews == 0,
                "closed popup must not query every production Info each frame");
        require(host.dispatch(ui::UiOpenScreen{"ProductionPopup"}, first).status ==
                    ui::UiCommandStatus::navigation,
                "popup navigation should acquire focus");
        const auto focused = host.begin_frame(1280, 540);
        const auto *choices = focused.snapshot.find<ui::ProductionPopupChoices>();
        require(choices && choices->player_id == 5 && choices->city_id == 9 &&
                    choices->turn == 12 && choices->entries.size() == 7 && previews == 3 &&
                    choices->first_visible == 0 && choices->visible_rows == 3,
                "active BNW rules and live city publish complete choice list once");
        require(choices->entries[0].order_type == 0 &&
                    choices->entries[0].authored_type == "UNIT_WARRIOR" &&
                    choices->entries[0].localized_name == "Warrior" &&
                    choices->entries[0].availability.status == ui::UiCommandStatus::unavailable &&
                    choices->entries[1].order_type == 1 &&
                    choices->entries[1].item_id == 3 &&
                    choices->entries[1].localized_name == "Market" &&
                    choices->entries[1].availability.status == ui::UiCommandStatus::ready,
                "read-only eligibility distinguishes active unit and building");
        require(choices->entries[2].availability.reason.find("unresolved") !=
                    std::string::npos &&
                    choices->entries[3].availability.reason.find("ambiguous") !=
                    std::string::npos &&
                    choices->entries[4].availability.reason.find("ambiguous") !=
                    std::string::npos,
                "missing and duplicate authored Types stay visibly unavailable");
        ui::BnwCityProductionUiBackend missing_names(live, rules, services,
            [&](const auto &, auto, auto, const auto &) {
                return ui::UiCommandResult{ui::UiCommandStatus::ready, {}};
            });
        const auto unnamed = missing_names.capture_choices(focused.snapshot);
        require(unnamed && !unnamed->entries[0].localized_name &&
                    unnamed->entries[1].availability.status ==
                        ui::UiCommandStatus::unavailable &&
                    unnamed->entries[1].availability.reason.find("untranslated") !=
                        std::string::npos,
                "production choices without active localization cannot become clickable");
        const auto off_page = host.dispatch(ui::city_choose_production(9, 1, 5), focused);
        const auto disabled_choice = host.dispatch(ui::city_choose_production(9, 0, 2), focused);
        const auto unknown = host.dispatch(ui::city_choose_production(9, 1, 999), focused);
        require(off_page.status == ui::UiCommandStatus::unavailable &&
                    disabled_choice.status == ui::UiCommandStatus::unavailable &&
                    unknown.status == ui::UiCommandStatus::unavailable &&
                    live.state().cities.players.at(5).cities.at(9)
                        .production_orders->front().item == 1,
                "typed production dispatch must honor the visible ready frame rows");
        const auto plan = ui::plan_city_screen(selected_popup, focused.snapshot,
                                                1280, 540, &backend);
        require(plan.hits.size() == 4 && !plan.hits[0].enabled &&
                    plan.hits[0].unavailable_reason == "Native eligibility unavailable" &&
                    plan.hits[1].enabled &&
                    std::any_of(plan.paints.begin(), plan.paints.end(),
                        [](const auto &paint) {
                            return paint.text == "Warrior [unavailable]";
                        }),
                "popup paints only the first selected XML scroll page");
        const auto no_backend = ui::plan_city_screen(selected_popup, focused.snapshot,
                                                      1280, 540);
        require(!no_backend.hits[1].enabled,
                "captured eligibility cannot enable an unbound renderer action");
        ui::CityGameplayBackend fallback;
        const auto wrong_backend = ui::plan_city_screen(selected_popup, focused.snapshot,
                                                         1280, 540, &fallback);
        require(!wrong_backend.hits[1].enabled &&
                    wrong_backend.hits[1].unavailable_reason.find("does not match") !=
                        std::string::npos,
                "captured eligibility cannot authorize a different backend");
        const ui::LiveUiHit down{{700, 300, 24, 24}, "ProductionPopup",
            ui::city_scroll_production_popup(9,
                ui::ProductionPopupScrollDirection::down), true, {}};
        require(host.click(focused, 704, 304, std::span(&down, 1)).command.status ==
                    ui::UiCommandStatus::navigation,
                "measured DownButton hit scrolls focused popup");
        const auto after_click = host.begin_frame(1280, 540);
        require(after_click.snapshot.find<ui::ProductionPopupChoices>()->first_visible == 1,
                "scroll offset advances one row");
        const auto second_plan = ui::plan_city_screen(selected_popup,
            after_click.snapshot, 1280, 540, &backend);
        require(second_plan.hits.size() == 4 && second_plan.hits[0].enabled &&
                    !second_plan.hits[1].enabled &&
                    second_plan.hits[1].unavailable_reason.find("unresolved") !=
                        std::string::npos,
                "next page retains ready and disabled choice identities");
        const auto stable = host.begin_frame(1280, 540);
        require(stable.snapshot.find<ui::ProductionPopupChoices>()->first_visible == 1,
                "new live frame preserves the focused city scroll offset");
        const ui::WorldViewRect measured_scroll_panel{34, 196, 340, 144};
        require(host.wheel_production_popup(stable, 500, 300, -120,
                    measured_scroll_panel).consumed &&
                    host.wheel_production_popup(stable, 500, 300, -120,
                    measured_scroll_panel).command.status == ui::UiCommandStatus::invalid,
                "focused popup consumes wheel outside measured scroll panel");
        require(host.wheel_production_popup(stable, 100, 300, -120,
                    measured_scroll_panel).command.status == ui::UiCommandStatus::navigation,
                "wheel inside measured ScrollPanel advances list");
        require(host.click(stable, 704, 304, std::span(&down, 1)).command.status ==
                    ui::UiCommandStatus::invalid,
                "old frame cannot replay scroll hit");
        auto scrolled = host.begin_frame(1280, 540);
        require(scrolled.snapshot.find<ui::ProductionPopupChoices>()->first_visible == 2,
                "wheel offset survives frame capture");
        for (std::size_t expected = 3; expected <= 4; ++expected) {
            require(host.wheel_production_popup(scrolled, 100, 300, -120,
                        measured_scroll_panel).command.status ==
                        ui::UiCommandStatus::navigation,
                    "wheel should reach later confirmed choices");
            scrolled = host.begin_frame(1280, 540);
            require(scrolled.snapshot.find<ui::ProductionPopupChoices>()->first_visible ==
                        expected,
                    "scroll must advance without skipping a row");
        }
        require(host.wheel_production_popup(scrolled, 100, 300, -120,
                    measured_scroll_panel).command.status ==
                    ui::UiCommandStatus::unavailable,
                "last page must clamp instead of creating a blank page");
        const auto last_plan = ui::plan_city_screen(selected_popup,
            scrolled.snapshot, 1280, 540, &backend);
        require(last_plan.hits.size() == 4 && !last_plan.hits[0].enabled &&
                    !last_plan.hits[1].enabled &&
                    std::get<ui::UiGameplayCommand>(last_plan.hits[0].command)
                        .as<ui::CityChooseProduction>()->item_id == 6,
                "last page shows later disabled choices without enabling them");
        const ui::LiveUiHit disabled{last_plan.hits[0].bounds,
            "ProductionPopup", last_plan.hits[0].command,
            last_plan.hits[0].enabled, last_plan.hits[0].unavailable_reason};
        require(host.click(scrolled, disabled.bounds.x + 2, disabled.bounds.y + 2,
                    std::span(&disabled, 1)).command.status ==
                    ui::UiCommandStatus::unavailable,
                "disabled production row cannot reach gameplay backend");
        const ui::LiveUiHit up{{730, 300, 24, 24}, "ProductionPopup",
            ui::city_scroll_production_popup(9,
                ui::ProductionPopupScrollDirection::up), true, {}};
        require(host.click(scrolled, 734, 304, std::span(&up, 1)).command.status ==
                    ui::UiCommandStatus::navigation,
                "measured UpButton hit scrolls back");
        const auto before_close = host.begin_frame(1280, 540);
        require(before_close.snapshot.find<ui::ProductionPopupChoices>()->first_visible == 3 &&
                    host.dispatch(ui::UiCloseScreen{"ProductionPopup"}, before_close).status ==
                        ui::UiCommandStatus::navigation,
                "popup close retains focus semantics and ends scroll session");
        const auto no_focus = host.begin_frame(1280, 540);
        require(!host.wheel_production_popup(no_focus, 100, 300, -120,
                    measured_scroll_panel).consumed &&
                    host.dispatch(ui::UiOpenScreen{"ProductionPopup"}, no_focus).status ==
                        ui::UiCommandStatus::navigation,
                "wheel cannot scroll a closed popup");
        const auto reopened = host.begin_frame(1280, 540);
        require(reopened.snapshot.find<ui::ProductionPopupChoices>()->first_visible == 0,
                "reopened popup begins at first confirmed choice");
        const auto ready_plan = ui::plan_city_screen(selected_popup,
            reopened.snapshot, 1280, 540, &backend);
        const auto ready_hit = std::find_if(ready_plan.hits.begin(),
            ready_plan.hits.end(), [](const ui::WorldViewHit &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                const auto *choice = gameplay
                    ? gameplay->as<ui::CityChooseProduction>() : nullptr;
                return choice && choice->order_type == 1 &&
                       choice->item_id == 3 && hit.enabled;
            });
        require(ready_hit != ready_plan.hits.end(),
                "measured popup must expose the eligible typed building row");
        const ui::LiveUiHit measured_choice{ready_hit->bounds,
            "ProductionPopup", ready_hit->command,
            ready_hit->enabled, ready_hit->unavailable_reason};
        const auto clicked = host.click(reopened, measured_choice.bounds.x + 2,
            measured_choice.bounds.y + 2, std::span(&measured_choice, 1));
        require(clicked.consumed &&
                    clicked.command.status == ui::UiCommandStatus::performed &&
                    live.state().cities.players.at(5).cities.at(9)
                        .production_orders->front().item == 3,
                "measured typed hit reaches the owned transactional order backend");
        require(!host.screens().focused_screen() &&
                host.click(reopened, measured_choice.bounds.x + 2,
                    measured_choice.bounds.y + 2,
                    std::span(&measured_choice, 1)).command.status ==
                    ui::UiCommandStatus::invalid,
                "committed non-append order closes modal and invalidates its old hit");
        const auto committed = host.begin_frame(1280, 540);
        const auto *committed_city = committed.snapshot.find<ui::CityScreenFrame>();
        require(!committed.snapshot.find<ui::ProductionPopupChoices>() &&
                    committed_city && committed_city->cities.front().queue.size() == 1 &&
                    committed_city->cities.front().queue.front().item == 3,
                "next live frame reflects the committed order with popup closed");
        ui::LiveUiHost mismatched(registry(),
            [&] { return frame_from(live, ++samples); }, &fallback, sources);
        const auto closed = mismatched.begin_frame(1280, 540);
        require(mismatched.dispatch(ui::UiOpenScreen{"ProductionPopup"}, closed).status ==
                    ui::UiCommandStatus::navigation &&
                    !mismatched.begin_frame(1280, 540).snapshot
                        .find<ui::ProductionPopupChoices>(),
                "choice frame requires the same typed backend as dispatch");
        std::cout << "ui BNW production choices tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
