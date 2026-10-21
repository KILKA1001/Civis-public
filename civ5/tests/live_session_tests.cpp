#include "civ5/runtime/live_session.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    bool caught=false;
    try { action(); } catch (const std::invalid_argument &) { caught=true; }
    check(caught,"Expected invalid live-session import or checkpoint to be rejected");
}
template<class F> void rejects_reason(F action, const char *expected) {
    try { action(); }
    catch (const std::invalid_argument &error) {
        check(std::string(error.what()).find(expected)!=std::string::npos,
              "Live-session import rejected before the intended identity guard");
        return;
    }
    check(false,"Expected live-session identity mismatch to be rejected");
}
template<class F> void rejects_any(F action) {
    bool caught=false;
    try { action(); } catch (const std::exception &) { caught=true; }
    check(caught,"Expected BNW cache rebuild failure");
}
civ5::saves::SavedSession source() {
    civ5::saves::SavedSession result;
    result.snapshot.game_state.layout=civ5::saves::GameStateLayout::expansion2_version1;
    result.source.bytes={1,2,3};
    result.snapshot.payload_end_offset=result.source.bytes.size();
    return result;
}
civ5::runtime::LiveSessionState state(const civ5::saves::SavedSession &saved) {
    using namespace civ5::runtime;
    return {saved.snapshot,ResearchContext{},std::vector<TeamTechnologies>{},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},MaintenanceContext{},TreasuryContext{},
        ScienceContext{},ResearchTurnState{},
        SessionRandomStreams{SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
                             SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
}
}
int main() {
    try {
        using namespace civ5;
        static_assert(std::is_const_v<std::remove_reference_t<decltype(
            std::declval<const runtime::LiveSession &>().source())>>);
        auto imported=source();
        auto session=runtime::LiveSession::from_restored_bnw(imported,state(imported));
        rejects([&] { (void)session.bnw_game_debug_mode(); });
        check(!session.readiness().can_continue() &&
              session.readiness().blockers.size()==6 &&
              session.readiness().blockers.front()==runtime::LiveSessionBlocker::bnw_query_caches_dirty,
              "Diagnostic import must not advertise playable continuation");
        auto lease=session.read_state_lease();
        std::weak_ptr<const runtime::LiveSessionState> retired=lease;
        const auto lease_checkpoint=session.checkpoint();
        session.mutable_state().setup.game_turn=17;
        check(lease->setup.game_turn==17,"Read lease falsely froze in-place writes");
        session.restore(lease_checkpoint);
        rejects([&] { (void)session.bnw_game_debug_mode(); });
        check(lease.get()!=&session.state() && lease->setup.game_turn==17 &&
              session.state().setup.game_turn==0 && !retired.expired(),
              "Replaced state did not survive its active read lease");
        lease.reset();
        check(retired.expired(),"Retired state leaked after final read lease release");
        const auto before=session.checkpoint();
        session.mutable_state().setup.game_turn=4294967300LL;
        session.mutable_state().cities.players[8].cities[9].population=42;
        session.mutable_state().world.payload_end_offset=99;
        (void)session.mutable_state().random.gameplay.draw(10);
        check(session.source().snapshot.payload_end_offset==3 &&
              session.source().source.bytes==std::vector<std::uint8_t>({1,2,3}),
              "Mutable world must not change imported source");
        session.restore(before);
        check(session.state().setup.game_turn==0 &&
              session.state().cities.players.empty() &&
              session.state().world.payload_end_offset==3 &&
              session.state().random.gameplay.state().calls==0 &&
              !session.bnw_unhappiness_current(),
              "Rollback must restore every common mutable domain and RNG");
        content::RulesDatabase happiness_db;
        happiness_db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
            "('UNHAPPINESS_PER_CITY',3),('UNHAPPINESS_PER_CAPTURED_CITY',5),"
            "('UNHAPPINESS_PER_POPULATION',1),('UNHAPPINESS_PER_OCCUPIED_POPULATION',1),"
            "('FREE_UNIT_HAPPINESS',20),('DEFAULT_SPECIALIST',0);");
        const auto happiness_defines=happiness_db.cache_global_defines(content::GlobalDefineProfile::bnw);
        runtime::BnwUnhappinessRules happiness_rules;
        happiness_rules.handicaps[0]={100,100,100};
        auto happiness_source=source();
        auto happiness_state=state(happiness_source);
        happiness_state.setup.game_handicap=0;
        auto &player=happiness_state.setup.players[0];
        player.team=0;player.handicap=0;player.slot_status=3;player.alive=true;
        auto &city=happiness_state.cities.players[0].cities[1];
        city.owner=0;city.id=1;city.population=5;
        happiness_state.cities.players[0].capital=1;
        happiness_state.unhappiness.world_cities_percent=100;
        happiness_state.unhappiness.players[0].happiness=10;
        happiness_state.unhappiness.players[0].cities[1]={};
        // Only the nearby-city index is stale. It must still prevent a clean
        // query-cache readiness report after the happiness cache is rebuilt.
        happiness_state.city_flavor_recipients.current=true;
        happiness_state.player_flavor_recipients.current=true;
        happiness_state.plot_impassability.current=true;
        happiness_state.adjacent_land.current=true;
        happiness_state.plot_visibility.current=true;
        auto happiness=runtime::LiveSession::from_restored_bnw(
            std::move(happiness_source),std::move(happiness_state));
        happiness.rebuild_bnw_unhappiness_cache(happiness_rules,happiness_defines);
        check(happiness.bnw_unhappiness_current() &&
              happiness.state().cities.players.at(0).excess_happiness==2,
              "BNW load cache must derive happiness from the owned live city");
        check(happiness.readiness().blockers.front()==
                  runtime::LiveSessionBlocker::bnw_query_caches_dirty,
              "Stale nearby-city index must block BNW query readiness");
        const auto clean=happiness.checkpoint();
        happiness.mutable_state().cities.players.at(0).cities.at(1).population=8;
        check(!happiness.bnw_unhappiness_current() &&
              happiness.readiness().blockers.front()==runtime::LiveSessionBlocker::bnw_query_caches_dirty,
              "Mutable world access must invalidate the dependent BNW query cache");
        const auto dirty=happiness.checkpoint();
        happiness.rebuild_bnw_unhappiness_cache(happiness_rules,happiness_defines);
        check(happiness.state().cities.players.at(0).excess_happiness==-1,
              "BNW cache refresh must use changed live population");
        happiness.restore(dirty);
        check(!happiness.bnw_unhappiness_current() &&
              happiness.state().cities.players.at(0).excess_happiness==2,
              "Rollback must preserve the dirty cache state without pretending it was rebuilt");
        happiness.restore(clean);
        check(happiness.bnw_unhappiness_current() &&
              happiness.state().cities.players.at(0).cities.at(1).population==5,
              "Rollback must restore both live inputs and cache freshness");
        happiness.mutable_state().setup.players.at(0).handicap=99;
        rejects_any([&] { happiness.rebuild_bnw_unhappiness_cache(
            happiness_rules,happiness_defines); });
        check(!happiness.bnw_unhappiness_current() &&
              happiness.state().cities.players.at(0).excess_happiness==2,
              "Failed refresh must keep old cache and report it stale");
        auto another_source=source();
        auto another=runtime::LiveSession::from_restored_bnw(another_source,state(another_source));
        rejects([&] { another.restore(before); });
        check(another.state().setup.game_turn==0,"Foreign checkpoint must leave state intact");
        auto mixed=source();
        auto bad=state(mixed);
        bad.science.profile=content::TechnologyProfile::vp542;
        rejects([&] { (void)runtime::LiveSession::from_restored_bnw(std::move(mixed),std::move(bad)); });
        content::RulesDatabase db;
        const auto types=db.cache_info_types();
        const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);
        content::TechnologyRules technologies;
        runtime::CityYieldRules city_rules;
        runtime::ProductionProgressRules production;
        runtime::MaintenanceRules maintenance;
        runtime::TreasuryRules treasury;
        runtime::LiveSessionImportRules rules{db,types,technologies,defines,city_rules,
            production,maintenance,treasury,{"payload","rules"}};
        rejects([&] { (void)runtime::LiveSession::restore_bnw(source(),rules); });
        auto complete=source();
        complete.snapshot.pregame.emplace();
        complete.snapshot.map_plots.emplace();
        complete.snapshot.map_regions.emplace();
        complete.snapshot.teams.emplace();
        complete.snapshot.all_players.emplace();
        complete.snapshot.supporting_classes.emplace();
        const auto actual_payload=runtime::identify_bytes(complete.source.bytes).sha256;
        const auto actual_rules=runtime::identify_bytes(db.gameplay_fingerprint_data()).sha256;
        rules.identity={std::string(64,'a'),actual_rules};
        rejects_reason([&] { (void)runtime::LiveSession::restore_bnw(complete,rules); },
                       "payload identity");
        rules.identity={actual_payload,std::string(64,'b')};
        rejects_reason([&] { (void)runtime::LiveSession::restore_bnw(complete,rules); },
                       "rules identity");
        std::cout << "PASS live session foundation\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL live session foundation: " << error.what() << '\n';
        return 1;
    }
}
