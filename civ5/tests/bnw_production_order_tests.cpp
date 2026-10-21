#include "civ5/runtime/bnw_production_order.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void check(bool value,const char *message) {
    if (!value) throw std::runtime_error(message);
}

civ5::runtime::LiveSession make_session() {
    using namespace civ5;
    saves::SavedSession source;
    source.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{
        source.snapshot,runtime::ResearchContext{}, {},runtime::PlayerResearchContext{},
        runtime::CityYieldContext{},runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{},runtime::TreasuryContext{},
        runtime::ScienceContext{},runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
    state.setup.players[5].team=7;
    auto &city=state.cities.players[5].cities[9];
    city.owner=5;city.id=9;
    city.production_orders=std::vector<runtime::CityYieldState::ProductionOrder>{
        {4,1,-1,false,false,std::nullopt}};
    city.production_progress.emplace();
    return runtime::LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
} // namespace

int main() {
    try {
        using namespace civ5::runtime;
        ProductionProgressRules rules;
        rules.processes={1};rules.buildings={3};rules.units={4};
        auto game=make_session();
        const auto saved=game.checkpoint();
        const auto choice=BnwProductionChoice{BnwProductionKind::construct,3};
        const auto original_rng=game.state().random.gameplay.state().calls;
        const auto no_services=choose_bnw_city_production(game,5,9,choice,rules,{});
        check(no_services.outcome==LiveActionOutcome::rejected &&
              game.state().cities.players.at(5).cities.at(9).production_orders->front().item==1,
              "Missing services must leave current order intact");

        bool ready=false,core_ok=true,lua_ok=true,throw_effect=false;
        int dirty=0;
        BnwProductionOrderServices services;
        services.lua_ready=[&] {return ready;};
        services.core_can_order_read_only=[&](const LiveSessionState &state,
                std::int64_t owner,std::int64_t city,const BnwProductionChoice &requested) {
            check(owner==5 && city==9 && requested.item==3,
                  "Wrong native production request");
            check(state.cities.players.at(5).cities.at(9).production_orders->empty() &&
                  state.setup.game_turn==123,
                  "Native eligibility must see the cleared detached queue");
            return core_ok;
        };
        services.lua_test_all_read_only=[&](std::string_view event,
                std::int64_t owner,std::int64_t city,std::int64_t item,
                const LiveSessionState &state) -> std::optional<bool> {
            check(event=="CityCanConstruct" && owner==5 && city==9 &&
                  item==3 && state.setup.game_turn==123,
                  "Lua eligibility ran out of BNW source order");
            return lua_ok?std::nullopt:std::optional<bool>{false};
        };
        services.apply_owned_effect=[&](LiveSessionState &state,
                std::int64_t owner,std::int64_t city,
                BnwProductionOwnedEffect effect,
                const CityYieldState::ProductionOrder &order) {
            check(owner==5 && city==9 && (order.item==1 || order.item==3),
                  "Owned production effect addressed wrong city or order");
            int digit=0;
            switch (effect) {
            case BnwProductionOwnedEffect::release_commitment: digit=1;break;
            case BnwProductionOwnedEffect::uncommit_operation: digit=2;break;
            case BnwProductionOwnedEffect::stop_process: digit=3;break;
            case BnwProductionOwnedEffect::start_process: digit=4;break;
            case BnwProductionOwnedEffect::add_commitment:
                if (throw_effect) throw std::runtime_error("making counter unavailable");
                digit=5;break;
            }
            // A stand-in effect ledger stored only on the detached owner.
            state.setup.game_turn=state.setup.game_turn*10+digit;
        };
        services.dirty_after_commit=[&](std::int64_t owner,std::int64_t city) {
            ++dirty;
            check(owner==5 && city==9 && game.state().setup.game_turn==1235 &&
                  game.state().cities.players.at(5).cities.at(9).production_orders->front().item==3,
                  "Dirty notification occurred before production commit");
        };

        const auto unavailable=choose_bnw_city_production(game,5,9,choice,rules,services);
        check(unavailable.outcome==LiveActionOutcome::rejected && dirty==0 &&
              game.state().setup.game_turn==0,
              "Unavailable Lua host must block before any effect");
        ready=true;core_ok=false;
        const auto native_veto=choose_bnw_city_production(game,5,9,choice,rules,services);
        check(native_veto.outcome==LiveActionOutcome::rejected && dirty==0 &&
              game.state().setup.game_turn==0 &&
              game.state().cities.players.at(5).cities.at(9).production_orders->front().item==1,
              "Native veto must roll back old process and queue effects");
        core_ok=true;lua_ok=false;
        const auto lua_veto=choose_bnw_city_production(game,5,9,choice,rules,services);
        check(lua_veto.outcome==LiveActionOutcome::rejected && dirty==0 &&
              game.state().setup.game_turn==0,
              "Lua veto must roll back old order effects");
        lua_ok=true;throw_effect=true;
        bool threw=false;
        try { (void)choose_bnw_city_production(game,5,9,choice,rules,services); }
        catch (const std::runtime_error &) {threw=true;}
        check(threw && dirty==0 && game.state().setup.game_turn==0 &&
              game.state().random.gameplay.state().calls==original_rng,
              "Making-counter failure must roll back entire production action and RNG");
        throw_effect=false;
        const auto committed=choose_bnw_city_production(game,5,9,choice,rules,services);
        const auto &queue=*game.state().cities.players.at(5).cities.at(9).production_orders;
        check(committed.outcome==LiveActionOutcome::committed &&
              queue.size()==1 && queue.front().type==1 && queue.front().item==3 &&
              game.state().setup.game_turn==1235 && dirty==1 &&
              !game.readiness().can_continue(),
              "Production selection did not commit effects and new head in source order");
        game.restore(saved);
        check(game.state().setup.game_turn==0 &&
              game.state().cities.players.at(5).cities.at(9).production_orders->front().item==1,
              "Checkpoint did not restore old production head");
        const auto wrong_info=choose_bnw_city_production(game,5,9,
            {BnwProductionKind::construct,999},rules,services);
        check(wrong_info.outcome==LiveActionOutcome::rejected &&
              game.state().setup.game_turn==0,
              "Unknown BNW production Info must reject before effects");
        std::cout << "PASS BNW production order\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW production order: " << error.what() << '\n';
        return 1;
    }
}
