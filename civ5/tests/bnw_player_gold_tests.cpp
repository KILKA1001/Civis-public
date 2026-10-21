#include "civ5/runtime/bnw_player_gold.hpp"
#include "civ5/content/rules_database.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
LiveSession session() {
    saves::SavedSession source;
    source.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    LiveSessionState state{source.snapshot,ResearchContext{}, {},PlayerResearchContext{},
        CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},
        TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
    constexpr std::int64_t player=5,team=7;
    state.setup.game_turn=1;
    state.setup.game_handicap=0;
    state.setup.start_era=0;
    state.setup.players[player]={team,0,2,0,true};
    state.player_research.players[player]={};
    state.cities.yield_slots=6;
    state.cities.players[player]={};
    state.cities.players[player].excess_happiness=0; // synthetic calculated empire cache
    auto &city=state.cities.players[player].cities[2];
    city.owner=player;city.id=2;city.yields[2].terrain=12;
    state.maintenance.elapsed_turns=1;
    state.maintenance.estimate_end_turn=state.maintenance.default_end_turn=500;
    state.maintenance.team_eras[team]=0;
    state.maintenance.players[player]={};
    state.treasury.players[player].balance_times100=1000;
    state.technologies.emplace_back(content::TechnologyProfile::bnw,team,1);
    return LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
} // namespace

int main() {
    try {
        content::RulesDatabase database;
        database.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
            "('DEFICIT_UNIT_DISBANDING_THRESHOLD',-5),"
            "('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),"
            "('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),"
            "('INITIAL_FREE_OUTSIDE_UNITS',2),"
            "('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50);");
        const auto defines=database.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        TreasuryRules treasury;
        CityYieldRules city_rules;
        MaintenanceRules maintenance;
        maintenance.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};
        maintenance.era_building_percent[0]=100;
        BnwPlayerGoldRules rules{treasury,city_rules,maintenance,defines,-1};
        auto game=session();
        std::vector<TreasuryDirty> published;
        BnwPlayerGoldServices services;
        services.lua_ready=[] {return true;};
        services.founder_benefits_read_only=[](std::int64_t,
            const LiveSessionState &)->std::optional<std::int64_t> {return {};};
        services.dirty_after_commit=[&](std::int64_t,TreasuryDirty kind) {
            check(game.state().treasury.players.at(5).balance_times100==3400,
                  "Gold UI callback ran before transaction commit");
            published.push_back(kind);
        };
        auto handler=make_bnw_player_gold_handler(rules,services);
        check(handler.phase==BnwTurnPhase::player_gold && handler.run,
              "Gold factory did not bind player_gold");
        check(run_bnw_player_turn_with_gold(game,5,{},rules,services).outcome==
                  LiveActionOutcome::rejected &&
              game.state().treasury.players.at(5).balance_times100==1000,
              "Gold wrapper accepted missing mandatory player phases");
        const auto first=apply_bnw_player_gold(game,5,rules,services);
        check(first.outcome==LiveActionOutcome::committed &&
              game.state().treasury.players.at(5).balance_times100==2200 &&
              game.state().treasury.players.at(5).lifetime_gross_gold==12 &&
              game.state().treasury.players.at(5).balance_history_times100==
                  std::vector<std::int64_t>{2200} &&
              game.state().treasury.players.at(5).income_history_times100==
                  std::vector<std::int64_t>{1200} && published.empty(),
              "Inactive BNW player DoGold did not accrue correct owned gold/history");

        rules.active_player=5;
        const auto active=apply_bnw_player_gold(game,5,rules,services);
        check(active.outcome==LiveActionOutcome::committed &&
              game.state().treasury.players.at(5).balance_times100==3400 &&
              published==std::vector<TreasuryDirty>{TreasuryDirty::misc_buttons,
                  TreasuryDirty::selection_buttons,TreasuryDirty::game_data} &&
              !game.state().player_gold_publication,
              "Active gold UI intents were not published after commit in SDK order");
        auto reentrant=session();
        auto reentrant_services=services;
        std::vector<TreasuryDirty> reentrant_published;
        reentrant_services.dirty_after_commit=[&](std::int64_t,TreasuryDirty kind) {
            check(reentrant.state().player_gold_publication &&
                  reentrant.state().player_gold_publication->uncertain,
                  "In-flight gold UI step was exposed as available");
            check(publish_bnw_player_gold(reentrant,reentrant_services).outcome==
                      LiveActionOutcome::rejected,
                  "Reentrant gold UI publication replayed an in-flight step");
            reentrant_published.push_back(kind);
        };
        check(apply_bnw_player_gold(reentrant,5,rules,reentrant_services).outcome==
                  LiveActionOutcome::committed &&
              reentrant_published==std::vector<TreasuryDirty>{
                  TreasuryDirty::misc_buttons,TreasuryDirty::selection_buttons,
                  TreasuryDirty::game_data} &&
              !reentrant.state().player_gold_publication,
              "Gold UI publication did not complete once per step after reentry");
        auto no_ui=services;no_ui.dirty_after_commit={};
        const auto before=game.state().treasury.players.at(5).balance_times100;
        check(apply_bnw_player_gold(game,5,rules,no_ui).outcome==
                  LiveActionOutcome::rejected &&
              game.state().treasury.players.at(5).balance_times100==before,
              "Missing active UI service advanced treasury");
        auto no_lua=services;no_lua.founder_benefits_read_only={};
        check(apply_bnw_player_gold(game,5,rules,no_lua).outcome==
                  LiveActionOutcome::rejected &&
              game.state().treasury.players.at(5).balance_times100==before,
              "Missing mandatory read-only Lua income query advanced treasury");
        auto uncertain=session();
        auto broken=services;
        broken.dirty_after_commit=[](std::int64_t,TreasuryDirty) {
            throw std::runtime_error("engine UI unavailable");
        };
        const auto published_failure=apply_bnw_player_gold(uncertain,5,rules,broken);
        check(published_failure.outcome==LiveActionOutcome::committed &&
              uncertain.state().treasury.players.at(5).balance_times100==2200 &&
              uncertain.state().player_gold_publication &&
              uncertain.state().player_gold_publication->uncertain &&
              apply_bnw_player_gold(uncertain,5,rules,services).outcome==
                  LiveActionOutcome::rejected,
              "Failed external UI callback was replayed or rolled back after commit");
        auto achievement=session();
        achievement.mutable_state().setup.players[5].slot_status=3;
        auto &trade_city=achievement.mutable_state().cities.players[5].cities[2];
        trade_city.x=1;trade_city.y=2;
        saves::TradeRouteObservation trade{};
        trade.origin_owner=5;trade.origin_x=1;trade.origin_y=2;
        trade.destination_owner=8;trade.origin_yields={0,0,20099};
        achievement.mutable_state().cities.trade_routes.push_back(trade);
        const auto steam_excluded=apply_bnw_player_gold(
            achievement,5,BnwPlayerGoldRules{treasury,city_rules,maintenance,
                defines,-1},services);
        check(steam_excluded.outcome==LiveActionOutcome::committed &&
              !achievement.state().player_gold_publication,
              "Steam-only achievement branch blocked offline BNW DoGold");

        // Source: Expansion2 CvTreasury.cpp:70 DoGold and CvPlayer.cpp:16848
        // DoDeficit. A deficit requires gameplay RNG, military AI, unit scrap,
        // notification/Lua effects not yet owned by the live session.
        rules.active_player=-1;
        game.mutable_state().cities.players[5].cities[2].yields[2].terrain=0;
        game.mutable_state().maintenance.players[5].base_building=5;
        game.mutable_state().treasury.players[5].balance_times100=0;
        const auto rng=game.state().random.gameplay.state().calls;
        const auto history=game.state().treasury.players.at(5).income_history_times100;
        check(apply_bnw_player_gold(game,5,rules,services).outcome==
                  LiveActionOutcome::rejected &&
              game.state().treasury.players.at(5).balance_times100==0 &&
              game.state().treasury.players.at(5).income_history_times100==history &&
              game.state().random.gameplay.state().calls==rng,
              "DoDeficit failure changed owned treasury/history/RNG");

        // CvPlayer::DoDeficit returns immediately when combat units do not
        // exceed cities. A current roster proves that branch needs no RNG,
        // military AI, notification or Lua unit hook.
        auto quiet=session();
        quiet.mutable_state().cities.players[5].cities[2].yields[2].terrain=0;
        quiet.mutable_state().maintenance.players[5].base_building=5;
        quiet.mutable_state().treasury.players[5].balance_times100=0;
        auto &roster=quiet.mutable_state().world.all_players.emplace();
        roster.layout=saves::GameStateLayout::expansion2_version1;
        auto &owner=roster.players.emplace_back();
        owner.saved_player_id=5;
        owner.cities.layout=roster.layout;
        owner.cities.header.wire_city_count=1;
        auto &saved_city=owner.cities.cities.emplace_back();
        saved_city.core.layout=roster.layout;
        for (const auto &[name,value]:
             {std::pair{"m_iID",std::int64_t{2}},
              std::pair{"m_eOwner",std::int64_t{5}}}) {
            saves::CityCoreField field{};
            field.name=name;
            field.encoding=saves::CityCoreEncoding::i32;
            field.values={value};
            saved_city.core.fields.push_back(std::move(field));
        }
        owner.units.layout=roster.layout;
        auto current=services;
        current.deficit_unit_roster_current=true;
        current.deficit_city_roster_current=true;
        const auto quiet_rng=quiet.state().random.gameplay.state().calls;
        check(apply_bnw_player_gold(quiet,5,rules,current).outcome==
                  LiveActionOutcome::committed &&
              quiet.state().treasury.players.at(5).balance_times100==0 &&
              quiet.state().random.gameplay.state().calls==quiet_rng,
              "BNW proven no-disband deficit failed or consumed RNG");
        auto &combat_unit=quiet.mutable_state().world.all_players->players.front()
                              .units.units.emplace_back();
        combat_unit.core.layout=saves::GameStateLayout::expansion2_version1;
        const auto put=[&](const char *name,std::int64_t value) {
            saves::UnitCoreField field{};
            field.name=name;
            field.encoding=saves::UnitCoreEncoding::i32;
            field.values={value};
            combat_unit.core.fields.push_back(std::move(field));
        };
        put("m_iID",1);put("m_eOwner",5);put("m_iBaseCombat",10);
        auto second=combat_unit;
        for (auto &field:second.core.fields)
            if (std::string_view(field.name)=="m_iID") field.values={2};
        quiet.mutable_state().world.all_players->players.front().units.units
            .push_back(std::move(second));
        quiet.mutable_state().world.all_players->players.front().units.header
            .wire_unit_count=2;
        const auto before_combat=quiet.state().treasury.players.at(5).balance_times100;
        check(apply_bnw_player_gold(quiet,5,rules,current).outcome==
                  LiveActionOutcome::rejected &&
              quiet.state().treasury.players.at(5).balance_times100==before_combat &&
              quiet.state().random.gameplay.state().calls==quiet_rng,
              "BNW disband branch advanced without unit and Lua services");

        game.mutable_state().treasury.profile=content::TechnologyProfile::vp542;
        check(apply_bnw_player_gold(game,5,rules,services).outcome==
                  LiveActionOutcome::rejected &&
              game.state().random.gameplay.state().calls==rng,
              "Profile mismatch executed BNW gold");
        game.mutable_state().treasury.profile=content::TechnologyProfile::bnw;
        game.mutable_state().cities.players[5].cities[2].yields[2].terrain=12;
        game.mutable_state().maintenance.players[5].base_building=0;
        const auto old_balance=game.state().treasury.players.at(5).balance_times100;
        const auto active_handler=make_bnw_player_gold_handler(
            BnwPlayerGoldRules{treasury,city_rules,maintenance,defines,5},services);
        const auto rollback=apply_live_action(game,[&](LiveSessionState &detached) {
            const auto result=active_handler.run(detached,5);
            check(result.outcome==LiveActionOutcome::committed,
                  "Gold handler failed to stage detached state and UI intent");
            check(detached.player_gold_publication.has_value(),
                  "Active gold did not stage UI intent");
            (void)detached.random.gameplay.draw(10);
            return LiveActionResult{LiveActionOutcome::rejected,"later phase failed"};
        });
        check(rollback.outcome==LiveActionOutcome::rejected &&
              game.state().treasury.players.at(5).balance_times100==old_balance &&
              game.state().random.gameplay.state().calls==rng &&
              !game.state().player_gold_publication && published.size()==3,
              "Later rejection leaked gold/RNG/UI publication");
        std::cout << "PASS BNW player gold\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW player gold: " << error.what() << '\n';
        return 1;
    }
}
