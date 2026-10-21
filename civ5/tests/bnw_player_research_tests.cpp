#include "civ5/runtime/bnw_player_research.hpp"
#include "civ5/content/rules_database.hpp"
#include <iostream>
#include <stdexcept>
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
    state.setup.game_turn=10;
    state.setup.game_handicap=state.setup.speed=state.setup.start_era=0;
    state.setup.world_percent=100;
    state.setup.players[player]={team,0,2,0,true};
    state.setup.team_members[team]=1;
    state.player_research.players[player].queue={0};
    state.player_research.players[player].cities[2]={};
    state.player_research.players[player].max_effective_cities=1;
    state.player_research.major_players={player};
    state.player_research.civ_teams={team};
    state.player_research.teams[team].members={player};
    state.player_research.teams[team].alive_count=1;
    state.cities.yield_slots=6;
    state.cities.no_happiness=true;
    state.cities.players[player].excess_happiness=0;
    auto &city=state.cities.players[player].cities[2];
    city.owner=player;city.id=2;city.yields[3].terrain=10;
    city.yields[2].terrain=12;
    state.maintenance.elapsed_turns=4;
    state.maintenance.estimate_end_turn=state.maintenance.default_end_turn=500;
    state.maintenance.team_eras[team]=0;
    state.maintenance.players[player]={};
    state.treasury.players[player].balance_times100=1000;
    state.science.players[player]={};
    state.science.research_agreements[team]={};
    state.research_turn.players[player].founded_first_city=true;
    state.research_turn.players[player].researching={0};
    state.research_turn.players[player].leader_type="LEADER_TEST";
    state.technologies.emplace_back(content::TechnologyProfile::bnw,team,1);
    return LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
} // namespace

int main() {
    try {
        content::RulesDatabase database;
        database.apply_sql("CREATE TABLE HandicapInfos(ID,Type,ResearchPercent,"
            "ResearchPerEraModifier,AIResearchPercent,AIResearchPerEraModifier);"
            "INSERT INTO HandicapInfos VALUES(0,'H0',100,0,100,0);"
            "CREATE TABLE GameSpeeds(ID,Type,ResearchPercent);"
            "INSERT INTO GameSpeeds VALUES(0,'S0',100);"
            "CREATE TABLE Eras(ID,Type,ResearchPercent);"
            "INSERT INTO Eras VALUES(0,'E0',100);"
            "CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
            "('TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER',0),"
            "('TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER',0),"
            "('TECH_COST_KNOWN_PREREQ_MODIFIER',0),"
            "('NUM_CITIES_COST_MOD_SCALING',100),"
            "('RESEARCH_AGREEMENT_MOD',0),"
            "('DEFICIT_UNIT_DISBANDING_THRESHOLD',-5),"
            "('UNIT_MAINTENANCE_GAME_MULTIPLIER',8),"
            "('UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR',7),"
            "('INITIAL_FREE_OUTSIDE_UNITS',2),"
            "('INITIAL_OUTSIDE_UNIT_GOLD_PERCENT',50);");
        const auto research=content::load_research_rules(
            database,content::TechnologyProfile::bnw);
        const auto defines=database.cache_global_defines(
            content::GlobalDefineProfile::bnw);
        content::TechnologyRules technologies;
        technologies.collection.slots=1;
        technologies.entries[0].integers={{"Cost",10000}};
        technologies.entries[0].booleans={{"Repeat",false}};
        technologies.entries[0].or_prerequisites.default_value=-1;
        TreasuryRules treasury;
        CityYieldRules city_rules;
        MaintenanceRules maintenance;
        maintenance.handicaps[0]={0,0,100,100,100,100,100,100,100,100,0};
        maintenance.era_building_percent[0]=100;
        BnwUnhappinessRules unhappiness;
        BnwPlayerResearchRules rules{research,technologies,defines,unhappiness,
            treasury,city_rules,maintenance,-1,-1};
        auto game=session();
        std::vector<BnwResearchDirty> published;
        BnwPlayerResearchServices services;
        services.lua_ready=[] {return true;};
        services.founder_benefits_read_only=[](std::int64_t,
            const LiveSessionState &)->std::optional<std::int64_t> {return {};};
        services.choose_ai_on_detached=[](std::int64_t,LiveSessionState &) {
            throw std::runtime_error("AI choice should not run with queued tech");
        };
        services.dirty_after_commit=[&](std::int64_t,BnwResearchDirty kind) {
            published.push_back(kind);
        };
        const auto handler=make_bnw_player_research_handler(rules,services);
        check(handler.phase==BnwTurnPhase::player_research && handler.run,
              "Research handler did not bind player_research");
        check(run_bnw_player_turn_with_research(game,5,{},rules,services).outcome==
                  LiveActionOutcome::rejected,
              "Research wrapper accepted missing player phases");
        const auto first=apply_bnw_player_research(game,5,rules,services);
        check(first.outcome==LiveActionOutcome::committed &&
              game.state().technologies.front().progress_times100(0)==1000 &&
              !game.state().technologies.front().has_tech(0) &&
              game.state().research_turn.players.at(5).agreement_counters.empty() &&
              published.empty(),
              "BNW research did not accrue science without granting technology");
        auto active=session();
        auto active_rules=rules;active_rules.active_team=7;
        auto active_services=services;
        active_services.dirty_after_commit=[&](std::int64_t team,
                                                BnwResearchDirty kind) {
            check(team==7 && kind==BnwResearchDirty::game_data_and_score &&
                  active.state().technologies.front().progress_times100(0)==1000,
                  "Research UI callback ran before owned progress committed");
            check(active.state().player_research_publication &&
                  active.state().player_research_publication->uncertain &&
                  publish_bnw_player_research(active,active_services).outcome==
                      LiveActionOutcome::rejected,
                  "Reentrant research UI publication replayed an in-flight step");
            published.push_back(kind);
        };
        check(apply_bnw_player_research(active,5,active_rules,active_services).outcome==
                  LiveActionOutcome::committed &&
              published==std::vector<BnwResearchDirty>{
                  BnwResearchDirty::game_data_and_score} &&
              !active.state().player_research_publication,
              "Research dirty effect was not published after commit");
        published.clear();
        auto ai=session();
        ai.mutable_state().player_research.players[5].queue.clear();
        ai.mutable_state().research_turn.players[5].researching.clear();
        ai.mutable_state().maintenance.elapsed_turns=5;
        auto ai_services=services;
        ai_services.choose_ai_on_detached=[](std::int64_t player,
                                             LiveSessionState &detached) {
            check(player==5,"AI chose research for wrong player");
            detached.player_research.players[player].queue.push_back(0);
            detached.research_turn.players[player].researching.insert(0);
            (void)detached.random.gameplay.draw(10);
        };
        check(apply_bnw_player_research(ai,5,rules,ai_services).outcome==
                  LiveActionOutcome::committed &&
              ai.state().technologies.front().progress_times100(0)==1000 &&
              ai.state().player_research.players.at(5).queue.empty() &&
              ai.state().research_turn.players.at(5).researching.empty() &&
              ai.state().random.gameplay.state().calls==1,
              "BNW forced AI choice did not use detached queue/RNG and clear in SDK order");
        auto no_lua=services;no_lua.lua_ready={};
        const auto old=game.state().technologies.front().progress_times100(0);
        check(apply_bnw_player_research(game,5,rules,no_lua).outcome==
                  LiveActionOutcome::rejected &&
              game.state().technologies.front().progress_times100(0)==old,
              "Missing mandatory Lua service advanced research");
        auto no_ai=services;no_ai.choose_ai_on_detached={};
        check(apply_bnw_player_research(game,5,rules,no_ai).outcome==
                  LiveActionOutcome::rejected,
              "Missing mandatory AI service advanced research");

        auto grant=session();
        grant.mutable_state().technologies.front().set_progress_times100(0,999500);
        const auto rng=grant.state().random.gameplay.state().calls;
        const auto blocked=apply_bnw_player_research(grant,5,rules,services);
        check(blocked.outcome==LiveActionOutcome::rejected &&
              blocked.reason.find("grant")!=std::string::npos &&
              grant.state().technologies.front().progress_times100(0)==999500 &&
              !grant.state().technologies.front().has_tech(0) &&
              grant.state().random.gameplay.state().calls==rng,
              "Technology completion committed progress or RNG without SetHasTech");
        grant.mutable_state().research_turn.profile=content::TechnologyProfile::vp542;
        check(apply_bnw_player_research(grant,5,rules,services).outcome==
                  LiveActionOutcome::rejected,
              "Mismatched GameCore profile executed BNW research");
        grant.mutable_state().research_turn.profile=content::TechnologyProfile::bnw;
        grant.mutable_state().research_turn.players[5].unmapped_researching=1;
        check(apply_bnw_player_research(grant,5,rules,services).outcome==
                  LiveActionOutcome::rejected,
              "Unmapped saved researching flag executed BNW research");

        auto rollback=apply_live_action(game,[&](LiveSessionState &detached) {
            const auto phase=handler.run(detached,5);
            check(phase.outcome==LiveActionOutcome::committed,
                  "Research phase failed on detached owned state");
            (void)detached.random.gameplay.draw(10);
            return LiveActionResult{LiveActionOutcome::rejected,"later phase failed"};
        });
        check(rollback.outcome==LiveActionOutcome::rejected &&
              game.state().technologies.front().progress_times100(0)==old &&
              game.state().random.gameplay.state().calls==0 &&
              !game.state().player_research_publication,
              "Later phase rejection leaked research or RNG");

        // The gold and research phase owners share one detached player turn.
        // Synthetic handlers here represent only the other source phases;
        // production still requires their real GameCore implementations.
        auto combined=session();
        BnwPlayerGoldRules gold_rules{treasury,city_rules,maintenance,defines,5};
        auto combined_research_rules=rules;
        combined_research_rules.active_team=7;
        std::vector<std::string> ui_order;
        BnwPlayerGoldServices gold_services;
        gold_services.lua_ready=[] {return true;};
        gold_services.founder_benefits_read_only=services.founder_benefits_read_only;
        gold_services.dirty_after_commit=[&](std::int64_t,TreasuryDirty) {
            check(combined.state().treasury.players.at(5).balance_times100>1000 &&
                  combined.state().technologies.front().progress_times100(0)>0,
                  "Gold UI was published before both phases committed");
            ui_order.emplace_back("gold");
        };
        auto combined_research_services=services;
        combined_research_services.dirty_after_commit=[&](std::int64_t,BnwResearchDirty) {
            ui_order.emplace_back("research");
        };
        std::vector<BnwTurnHandler> remaining;
        for (const auto phase:bnw_player_turn_order()) {
            if (phase==BnwTurnPhase::player_achievements ||
                phase==BnwTurnPhase::player_gold ||
                phase==BnwTurnPhase::player_research) continue;
            remaining.push_back({phase,[](LiveSessionState &,std::int64_t) {
                return LiveActionResult{LiveActionOutcome::committed,{}};
            }});
        }
        remaining.front().after_commit=[&](LiveSession &) {
            ui_order.emplace_back("early");
            return LiveActionResult{LiveActionOutcome::committed,{}};
        };
        auto missing_combined=run_bnw_player_turn_with_gold_and_research(
            combined,5,{},gold_rules,gold_services,combined_research_rules,
            combined_research_services);
        check(missing_combined.outcome==LiveActionOutcome::rejected &&
              combined.state().treasury.players.at(5).balance_times100==1000 &&
              ui_order.empty(),"Combined player turn skipped mandatory preflight");
        auto blocked_remaining=remaining;
        for (auto &entry:blocked_remaining)
            if (entry.phase==BnwTurnPhase::player_policies)
                entry.run=[](LiveSessionState &,std::int64_t) {
                    return LiveActionResult{LiveActionOutcome::rejected,"policy unavailable"};
                };
        auto blocked_combined=run_bnw_player_turn_with_gold_and_research(
            combined,5,blocked_remaining,gold_rules,gold_services,
            combined_research_rules,combined_research_services);
        check(blocked_combined.outcome==LiveActionOutcome::rejected &&
              combined.state().treasury.players.at(5).balance_times100==1000 &&
              combined.state().technologies.front().progress_times100(0)==0 &&
              !combined.state().player_gold_publication && ui_order.empty(),
              "Later player phase failed to roll back owned gold and UI intent");
        auto complete_combined=run_bnw_player_turn_with_gold_and_research(
            combined,5,remaining,gold_rules,gold_services,
            combined_research_rules,combined_research_services);
        check(complete_combined.outcome==LiveActionOutcome::committed &&
              combined.state().treasury.players.at(5).balance_times100>1000 &&
              combined.state().technologies.front().progress_times100(0)>0 &&
              ui_order==std::vector<std::string>{"early","gold","gold","gold","research"} &&
              !combined.state().player_gold_publication &&
              !combined.state().player_research_publication,
              "Combined gold/research turn did not commit and publish in source order");
        auto uncertain_combined=session();
        auto throwing_gold=gold_services;
        throwing_gold.dirty_after_commit=[](std::int64_t,TreasuryDirty) {
            throw std::runtime_error("UI device unavailable");
        };
        ui_order.clear();
        const auto uncertain_result=run_bnw_player_turn_with_gold_and_research(
            uncertain_combined,5,remaining,gold_rules,throwing_gold,
            combined_research_rules,combined_research_services);
        check(uncertain_result.outcome==LiveActionOutcome::committed &&
              uncertain_result.reason.find("uncertain")!=std::string::npos &&
              uncertain_combined.state().player_gold_publication &&
              uncertain_combined.state().player_gold_publication->uncertain &&
              !uncertain_combined.state().player_research_publication &&
              ui_order==std::vector<std::string>{"early","research"} &&
              run_bnw_player_turn_with_gold_and_research(
                  uncertain_combined,5,remaining,gold_rules,gold_services,
                  combined_research_rules,combined_research_services).outcome==
                  LiveActionOutcome::rejected,
              "Failed gold UI publication replayed a committed player turn");
        std::cout << "PASS BNW player research\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW player research: " << error.what() << '\n';
        return 1;
    }
}
