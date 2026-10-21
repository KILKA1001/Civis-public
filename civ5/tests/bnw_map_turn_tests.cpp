#include "civ5/runtime/bnw_map_turn.hpp"
#include "civ5/content/rules_database.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
std::int64_t value(const saves::PlotObservation &plot, std::string_view name) {
    for (const auto &field : plot.fields)
        if (field.name && name == field.name) return field.value;
    throw std::runtime_error("Missing test plot field");
}
std::int64_t &mutable_value(saves::PlotObservation &plot, std::string_view name) {
    for (auto &field : plot.fields)
        if (field.name && name == field.name) return field.value;
    throw std::runtime_error("Missing mutable test plot field");
}
LiveSession session() {
    saves::SavedSession source;
    auto &world = source.snapshot;
    world.game_state.layout = saves::GameStateLayout::expansion2_version1;
    world.map_header.layout = world.game_state.layout;
    world.map_header.grid = {3, 1};
    world.map_plots.emplace();
    world.map_plots->layout = world.game_state.layout;
    world.map_projection.emplace();
    world.map_projection->grid = world.map_header.grid;
    world.all_players.emplace();
    world.all_players->layout = world.game_state.layout;
    saves::SavedPlayerObservation player;
    player.saved_player_id = 0;
    player.core.fields.push_back({"m_iTotalLandScored", saves::PlayerCoreEncoding::i32,
                                  0, 0, {}, {6}, {}, {}, {}});
    world.all_players->players.push_back(std::move(player));
    for (std::uint64_t index = 0; index < 3; ++index) {
        saves::PlotObservation plot;
        plot.index = plot.x = index;
        plot.y = 0;
        plot.version = 7;
        plot.fields = {{"owner", index == 2 ? -1 : 0},
                       {"plot_type", index == 1 ? 3 : 2},
                       {"ownership_duration", index == 2 ? 0 : 2},
                       {"improvement_duration", index == 0 ? 4 : 0},
                       {"improvement_type_hash", index == 0 ? 123 : 0},
                       {"world_anchor", index == 0 ? 2 : -1},
                       {"world_anchor_data", index == 0 ? 5 : -1}};
        world.map_plots->plots.push_back(std::move(plot));
        saves::SavedMapCellProjection cell;
        cell.index = cell.x = index;
        cell.y = 0;
        cell.owner = index == 2 ? -1 : 0;
        cell.plot_type = index == 1 ? 3 : 2;
        cell.improvement_type = index == 0 ? 42 : -1;
        cell.units = std::vector<saves::SavedPlotUnitReference>{};
        world.map_projection->cells.push_back(std::move(cell));
    }
    LiveSessionState state{world, ResearchContext{}, {}, PlayerResearchContext{},
        CityYieldContext{}, BnwUnhappinessContext{}, MaintenanceContext{},
        TreasuryContext{}, ScienceContext{}, ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw, {5, 0, 0}),
         SimulationRandom(RandomAlgorithm::bnw, {7, 0, 0})}};
    return LiveSession::from_restored_bnw(std::move(source), std::move(state));
}
std::int64_t score(const LiveSession &game) {
    return game.state().world.all_players->players.front().core.fields.front().values.front();
}
}

int main() {
    try {
        content::RulesDatabase selected_rules;
        selected_rules.apply_sql(
            "CREATE TABLE Defines(Name TEXT, Value TEXT);"
            "INSERT INTO Defines VALUES('OWNERSHIP_SCORE_DURATION_THRESHOLD','3');"
            "INSERT INTO Defines VALUES('PLOT_UNIT_LIMIT','1');");
        const auto selected_defines = selected_rules.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        const auto selected_map_rules = resolve_bnw_map_turn_rules(selected_defines);
        check(selected_map_rules.ownership_score_duration_threshold == 3 &&
              selected_map_rules.plot_unit_limit == 1,
              "Map turn did not bind the activated BNW defines");
        content::RulesDatabase missing_rules;
        missing_rules.apply_sql("CREATE TABLE Defines(Name TEXT, Value TEXT)");
        bool missing_rejected = false;
        try {
            (void)resolve_bnw_map_turn_rules(missing_rules.cache_global_defines(
                content::GlobalDefineProfile::bnw));
        } catch (const std::runtime_error &) { missing_rejected = true; }
        check(missing_rejected, "Map turn accepted absent source defines");
        auto game = session();
        auto run = [&](const BnwMapTurnServices &services) {
            return apply_live_action(game, [&](LiveSessionState &state) {
                return run_bnw_map_turn_phase(state,
                    selected_map_rules.ownership_score_duration_threshold, services);
            });
        };
        check(run({}).outcome == LiveActionOutcome::rejected && score(game) == 6,
              "Map phase accepted missing mandatory services");
        game.mutable_state().world.map_projection->cells.at(1).units.reset();
        BnwMapTurnServices roster_services;
        roster_services.verify_units = [](LiveSessionState &, std::uint64_t) {
            return LiveActionResult{LiveActionOutcome::committed, {}};
        };
        roster_services.stage_anchor_removal = [](LiveSessionState &, std::uint64_t,
                                                  std::int64_t) {
            return LiveActionResult{LiveActionOutcome::committed, {}};
        };
        check(run(roster_services).outcome == LiveActionOutcome::rejected &&
              score(game) == 6 &&
              value(game.state().world.map_plots->plots.front(),
                    "ownership_duration") == 2,
              "Map phase advanced an unresolved unit roster");
        game.mutable_state().world.map_projection->cells.at(1).units =
            std::vector<saves::SavedPlotUnitReference>{};
        std::vector<std::string> order;
        bool reject_second = true;
        BnwMapTurnServices services;
        services.verify_units = [&](LiveSessionState &state, std::uint64_t index) {
            const auto &plot = state.world.map_plots->plots.at(index);
            check(state.world.map_projection->cells.at(index).units->empty(),
                  "Test unit validity service received occupied plot");
            if (index == 0)
                check(value(plot, "ownership_duration") == 3 &&
                      value(plot, "improvement_duration") == 5 &&
                      state.world.all_players->players.front().core.fields.front()
                          .values.front() == 7 && score(game) == 6,
                      "Duration setters did not precede unit verification");
            order.push_back("verify:" + std::to_string(index));
            if (reject_second && index == 1)
                return LiveActionResult{LiveActionOutcome::rejected, "unit mismatch"};
            return LiveActionResult{LiveActionOutcome::committed, {}};
        };
        services.stage_anchor_removal = [&](LiveSessionState &state,
                                            std::uint64_t index, std::int64_t old) {
            auto &plot=state.world.map_plots->plots.at(index);
            check(index == 0 && old == 2 &&
                  value(plot, "world_anchor") == 2 &&
                  value(plot, "world_anchor_data") == 5,
                  "Anchor removal did not see the old wire fields");
            order.push_back("anchor:" + std::to_string(index));
            mutable_value(plot,"world_anchor")=-1;
            mutable_value(plot,"world_anchor_data")=-1;
            return LiveActionResult{LiveActionOutcome::committed, {}};
        };
        const auto rejected = run(services);
        check(rejected.outcome == LiveActionOutcome::rejected && score(game) == 6 &&
              value(game.state().world.map_plots->plots.front(), "ownership_duration") == 2 &&
              value(game.state().world.map_plots->plots.front(), "world_anchor") == 2,
              "Rejected map phase leaked plot or score mutation");
        check(order == std::vector<std::string>({"verify:0", "anchor:0", "verify:1"}),
              "BNW map phase source order changed");
        order.clear();
        reject_second = false;
        const auto committed = run(services);
        check(committed.outcome == LiveActionOutcome::committed && score(game) == 7 &&
              value(game.state().world.map_plots->plots.at(0), "ownership_duration") == 3 &&
              value(game.state().world.map_plots->plots.at(0), "improvement_duration") == 5 &&
              value(game.state().world.map_plots->plots.at(1), "ownership_duration") == 3 &&
              value(game.state().world.map_plots->plots.at(2), "ownership_duration") == 0 &&
              value(game.state().world.map_plots->plots.at(0), "world_anchor") == -1,
              "BNW map plot transition differs from source order");
        check(order == std::vector<std::string>({"verify:0", "anchor:0", "verify:1", "verify:2"}),
              "BNW map did not visit plots in index order");
        check(make_bnw_map_turn_handler(3, services).phase == BnwTurnPhase::game_map,
              "Map handler registered at the wrong game phase");

        auto empty_map=session();
        for (auto &plot:empty_map.mutable_state().world.map_plots->plots)
            for (auto &member:plot.fields)
                if (member.name && std::string_view(member.name)=="world_anchor")
                    member.value=-1;
        BnwMapTurnServices empty_services;
        empty_services.verify_units=[&](LiveSessionState &state,std::uint64_t index) {
            return verify_bnw_plot_units(state,index,
                selected_map_rules.plot_unit_limit,{});
        };
        auto anchored_map=session();
        check(apply_live_action(anchored_map,[&](LiveSessionState &state) {
                  return run_bnw_map_turn_phase(state,
                      selected_map_rules.ownership_score_duration_threshold,
                      empty_services);
              }).outcome==LiveActionOutcome::rejected && score(anchored_map)==6,
              "Present world anchor still requires staged engine removal");
        check(apply_live_action(empty_map,[&](LiveSessionState &state) {
                  return run_bnw_map_turn_phase(state,
                      selected_map_rules.ownership_score_duration_threshold,
                      empty_services);
              }).outcome==LiveActionOutcome::committed && score(empty_map)==7 &&
              value(empty_map.state().world.map_plots->plots.at(0),
                    "world_anchor_data")==-1,
              "Empty map phase must commit without unit or anchor engine calls");

        auto units_game = session();
        units_game.mutable_state().world.map_projection->cells.at(0).units->push_back({0, 9});
        BnwUnitValidityServices unit_services;
        const auto present = [](const LiveSessionState &state, std::uint64_t index,
                                saves::SavedPlotUnitReference handle) -> std::optional<bool> {
            const auto &roster = *state.world.map_projection->cells.at(index).units;
            return std::any_of(roster.begin(), roster.end(), [&](const auto &entry) {
                return entry.owner == handle.owner && entry.id == handle.id;
            });
        };
        unit_services.exists = present;
        unit_services.at_plot = present;
        unit_services.delayed_death = unit_services.cargo = unit_services.in_combat =
            unit_services.invisible = [](const LiveSessionState &, std::uint64_t,
                                         saves::SavedPlotUnitReference) { return false; };
        unit_services.valid_domain = unit_services.can_enter_territory =
            unit_services.visible_enemy = [](const LiveSessionState &, std::uint64_t,
                                             saves::SavedPlotUnitReference) { return true; };
        std::int64_t friendly = 1;
        unit_services.friendly_units_of_type = [&](const LiveSessionState &, std::uint64_t,
                                                   saves::SavedPlotUnitReference) {
            return std::optional<std::int64_t>{friendly};
        };
        unit_services.unit_team = [](const LiveSessionState &, std::uint64_t,
                                     saves::SavedPlotUnitReference) {
            return std::optional<std::int64_t>{2};
        };
        unit_services.plot_team = [](const LiveSessionState &, std::uint64_t,
                                     saves::SavedPlotUnitReference) {
            return std::optional<std::int64_t>{3};
        };
        bool jump_succeeds = true;
        std::vector<std::string> unit_order;
        unit_services.jump = [&](LiveSessionState &state, std::uint64_t index,
                                 saves::SavedPlotUnitReference) -> std::optional<bool> {
            unit_order.push_back("jump");
            if (jump_succeeds) state.world.map_projection->cells.at(index).units->clear();
            return jump_succeeds;
        };
        unit_services.kill = [&](LiveSessionState &state, std::uint64_t index,
                                 saves::SavedPlotUnitReference) {
            unit_order.push_back("kill");
            state.world.map_projection->cells.at(index).units->clear();
            return LiveActionResult{LiveActionOutcome::committed, {}};
        };
        const auto valid = [&](const BnwUnitValidityServices &binding) {
            return apply_live_action(units_game, [&](LiveSessionState &state) {
                return verify_bnw_plot_units(state, 0,
                    selected_map_rules.plot_unit_limit, binding);
            });
        };
        auto empty_game=session();
        check(apply_live_action(empty_game,[&](LiveSessionState &state) {
                  return verify_bnw_plot_units(state,0,
                      selected_map_rules.plot_unit_limit,{});
              }).outcome==LiveActionOutcome::committed,
              "Empty SDK plot roster must not require unit GameCore services");
        check(valid({}).outcome == LiveActionOutcome::rejected &&
              units_game.state().world.map_projection->cells.at(0).units->size() == 1,
              "Unit validity accepted missing mandatory GameCore services");
        check(valid(unit_services).outcome == LiveActionOutcome::committed &&
              unit_order == std::vector<std::string>{"jump"} &&
              units_game.state().world.map_projection->cells.at(0).units->empty(),
              "Enemy exclusion did not relocate in the second source pass");
        units_game.mutable_state().world.map_projection->cells.at(0).units->push_back({0, 9});
        friendly = 2;
        jump_succeeds = false;
        unit_order.clear();
        check(valid(unit_services).outcome == LiveActionOutcome::committed &&
              unit_order == std::vector<std::string>({"jump", "kill"}) &&
              units_game.state().world.map_projection->cells.at(0).units->empty(),
              "Stack overflow did not kill after failed jump in the first pass");
        std::cout << "BNW map plot turn PASS\n";
    } catch (const std::exception &error) {
        std::cerr << "BNW map plot turn FAIL: " << error.what() << '\n';
        return 1;
    }
}
