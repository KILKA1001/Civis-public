#include "civ5/ui/bnw_city_production_backend.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
using namespace civ5;

void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}

runtime::LiveSession make_session() {
    saves::SavedSession source;
    source.snapshot.game_state.layout = saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{
        source.snapshot, runtime::ResearchContext{}, {}, runtime::PlayerResearchContext{},
        runtime::CityYieldContext{}, runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{}, runtime::TreasuryContext{},
        runtime::ScienceContext{}, runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {5, 0, 0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw, {7, 0, 0})},
        {}, {}, {}, {}};
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

ui::UiFrameSnapshot make_frame() {
    ui::CityScreenFrame city;
    city.player_id = 5;
    city.selected_city_id = 9;
    ui::CityScreenCity selected;
    selected.owner = 5;
    selected.id = 9;
    selected.queue_resolved = true;
    selected.queue.push_back({4, 1, std::nullopt});
    city.cities.push_back(selected);
    return ui::UiFrameSnapshot(1, 5, 12).with<ui::CityScreenFrame>(
        std::make_shared<const ui::CityScreenFrame>(std::move(city)));
}
} // namespace

int main() {
    try {
        auto session = make_session();
        auto frame = make_frame();
        runtime::ProductionProgressRules rules;
        rules.processes.insert(1);
        rules.buildings.insert(3);
        const auto command = std::get<ui::UiGameplayCommand>(
            ui::city_choose_production(9, 1, 3));
        int preview_calls = 0, dirty_calls = 0;
        bool lua_ready = false, native_ok = true, lua_ok = true, effect_throws = false;
        runtime::BnwProductionOrderServices services;
        services.lua_ready = [&] { return lua_ready; };
        services.core_can_order_read_only = [&](const runtime::LiveSessionState &state,
            std::int64_t owner, std::int64_t city,
            const runtime::BnwProductionChoice &choice) {
            require(owner == 5 && city == 9 && choice.item == 3 &&
                    state.cities.players.at(5).cities.at(9).production_orders->empty() &&
                    state.cities.players.at(5).cities.at(9)
                        .production_progress->overflow_times100 == 123,
                    "native query must see detached cleared queue and effects");
            return native_ok;
        };
        services.lua_test_all_read_only = [&](std::string_view event,
            std::int64_t owner, std::int64_t city, std::int64_t item,
            const runtime::LiveSessionState &state) -> std::optional<bool> {
            require(event == "CityCanConstruct" && owner == 5 && city == 9 &&
                    item == 3 && state.setup.game_turn == 12,
                    "Lua query must match typed BNW order");
            return lua_ok ? std::nullopt : std::optional<bool>{false};
        };
        services.apply_owned_effect = [&](runtime::LiveSessionState &state,
            std::int64_t owner, std::int64_t city,
            runtime::BnwProductionOwnedEffect effect,
            const runtime::CityYieldState::ProductionOrder &) {
            require(owner == 5 && city == 9, "effect must address selected city");
            auto &ledger = state.cities.players.at(owner).cities.at(city)
                .production_progress->overflow_times100;
            int digit = 0;
            switch (effect) {
            case runtime::BnwProductionOwnedEffect::release_commitment: digit = 1; break;
            case runtime::BnwProductionOwnedEffect::uncommit_operation: digit = 2; break;
            case runtime::BnwProductionOwnedEffect::stop_process: digit = 3; break;
            case runtime::BnwProductionOwnedEffect::start_process: digit = 4; break;
            case runtime::BnwProductionOwnedEffect::add_commitment:
                if (effect_throws) throw std::runtime_error("effect unavailable");
                digit = 5; break;
            }
            ledger = ledger * 10 + digit;
        };
        services.dirty_after_commit = [&](std::int64_t owner, std::int64_t city) {
            ++dirty_calls;
            require(owner == 5 && city == 9 &&
                    session.state().cities.players.at(5).cities.at(9)
                        .production_orders->front().item == 3,
                    "dirty event must follow commit");
        };
        ui::BnwProductionPreview preview = [&](const runtime::LiveSessionState &state,
            std::int64_t owner, std::int64_t city,
            const runtime::BnwProductionChoice &choice) {
            ++preview_calls;
            require(state.setup.game_turn == 12 && owner == 5 && city == 9 &&
                    choice.kind == runtime::BnwProductionKind::construct && choice.item == 3,
                    "preview must receive current typed choice");
            return ui::UiCommandResult{ui::UiCommandStatus::ready, {}};
        };
        ui::BnwCityProductionUiBackend missing(session, rules, {}, preview);
        require(missing.availability(command, frame).status == ui::UiCommandStatus::unavailable &&
                missing.execute(command, frame).status == ui::UiCommandStatus::unavailable &&
                preview_calls == 0,
                "missing native, Lua and effect services must fail closed");
        ui::BnwCityProductionUiBackend no_preview(session, rules, services, {});
        require(no_preview.availability(command, frame).status == ui::UiCommandStatus::unavailable,
                "missing observational eligibility preview must fail closed");
        ui::BnwCityProductionUiBackend backend(session, rules, services, preview);
        require(backend.availability(command, frame).status == ui::UiCommandStatus::unavailable &&
                preview_calls == 0,
                "unavailable Lua host must block before preview");
        lua_ready = true;
        require(backend.availability(command, frame).status == ui::UiCommandStatus::ready &&
                preview_calls == 1,
                "complete services and preview expose choice");
        require(backend.availability(std::get<ui::UiGameplayCommand>(
                    ui::city_choose_production(9, 1, 999)), frame).status ==
                    ui::UiCommandStatus::unavailable,
                "unknown production Info must not become selectable");
        rules.processes.clear();
        require(backend.availability(command, frame).reason ==
                    "Existing BNW production Info is unavailable",
                "unresolved current order must block selection before preview");
        rules.processes.insert(1);
        native_ok = false;
        require(backend.execute(command, frame).status == ui::UiCommandStatus::unavailable &&
                dirty_calls == 0 &&
                session.state().cities.players.at(5).cities.at(9)
                    .production_orders->front().item == 1,
                "native veto must roll back old order and effects");
        native_ok = true;
        lua_ok = false;
        require(backend.execute(command, frame).status == ui::UiCommandStatus::unavailable &&
                dirty_calls == 0 &&
                session.state().cities.players.at(5).cities.at(9)
                    .production_progress->overflow_times100 == 0,
                "Lua veto must roll back detached effects");
        lua_ok = true;
        effect_throws = true;
        require(backend.execute(command, frame).status == ui::UiCommandStatus::unavailable &&
                dirty_calls == 0 &&
                session.state().cities.players.at(5).cities.at(9)
                    .production_progress->overflow_times100 == 0,
                "effect exception must leave live world intact");
        effect_throws = false;
        require(backend.execute(command, frame).status == ui::UiCommandStatus::performed &&
                dirty_calls == 1 &&
                session.state().cities.players.at(5).cities.at(9)
                    .production_progress->overflow_times100 == 1235,
                "typed UI choice must commit transactional BNW order once");
        require(backend.availability(command, frame).status == ui::UiCommandStatus::unavailable,
                "old UI frame must not reuse production queue after commit");
        std::cout << "ui BNW city production backend tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
