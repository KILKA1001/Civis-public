#include "civ5/runtime/bnw_world_religion_progress.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_game_score_checkpoint.hpp"
#include <algorithm>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *why) {
    if(!value) throw std::runtime_error(why);
}
template<class F> void rejects(F &&run) {
    bool failed=false;
    try { run(); } catch(const std::runtime_error &) { failed=true; }
    check(failed,"Unsafe BNW religion progress was accepted");
}
runtime::LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    world.game_state.layout=saves::GameStateLayout::expansion2_version1;
    world.game_state.controls.push_back({"religion_tech",2});
    world.game_state.flags.push_back({"score_dirty",false});
    world.pregame.emplace();
    world.pregame->layout=saves::GameStateLayout::expansion2_version1;
    world.pregame->archive.flag_arrays["minor_nation_civs"].resize(64);
    world.pregame->archive.flag_arrays["minor_nation_civs"][22]=true;
    world.teams.emplace();
    world.teams->layout=saves::GameStateLayout::expansion2_version1;
    world.teams->dimensions.civ_teams=63;
    world.teams->teams.resize(64);
    runtime::ResearchContext setup;
    std::vector<runtime::TeamTechnologies> owned;
    owned.reserve(64);
    for(std::int64_t id=0;id<64;++id) {
        world.teams->teams[static_cast<std::size_t>(id)].fields={
            {"id",id},{"alive_count",id==0 || id==1 || id==22?1:0}};
        runtime::ResearchPlayerState player;
        player.team=id;
        player.alive=id==0 || id==1 || id==22;
        setup.players.emplace(id,player);
        owned.emplace_back(content::TechnologyProfile::bnw,id,3);
    }
    owned[0].set_has_tech(1,true);
    return {std::move(world),std::move(setup),std::move(owned),
        runtime::PlayerResearchContext{},runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{},runtime::MaintenanceContext{},
        runtime::TreasuryContext{},runtime::ScienceContext{},
        runtime::ResearchTurnState{},
        runtime::SessionRandomStreams{
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
            runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
}
content::TechnologyRules rules() {
    content::TechnologyRules result;
    for(std::uint64_t id=0;id<3;++id) {
        auto &tech=result.entries[id];
        tech.integers["Cost"]=10*static_cast<std::int64_t>(id+1);
        tech.and_prerequisites.extent=2;
        tech.and_prerequisites.default_value=-1;
    }
    result.entries[1].and_prerequisites.values[0]=0;
    result.entries[2].and_prerequisites.values[0]=1;
    result.entries[2].and_prerequisites.values[1]=0;
    return result;
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    auto state=fixture();
    auto technology=rules();
    auto handler=runtime::make_bnw_game_caches_and_score_handler(
        technology,2);
    check(handler.phase==runtime::BnwTurnPhase::game_caches_and_score &&
          handler.run && !handler.after_commit,
          "BNW religion cache did not bind the whole clean-score game phase");
    const auto rng_calls=state.random.gameplay.state().calls;
    check(handler.run(state,-1).outcome==runtime::LiveActionOutcome::committed &&
          std::get<saves::Expansion2PrefixObservation>(
              state.world.game_state.prefix).scalars[23]==55 &&
          state.random.gameplay.state().calls==rng_calls,
          "BNW clean-score phase did not update religion cache without RNG");
    auto &dirty=state.world.game_state.flags.front();
    dirty.value=true;
    std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix).scalars[23]=777;
    check(handler.run(state,-1).outcome==runtime::LiveActionOutcome::rejected &&
          std::get<saves::Expansion2PrefixObservation>(
              state.world.game_state.prefix).scalars[23]==777 && dirty.value,
          "Dirty BNW score was silently accepted or changed the religion cache");
    dirty.value=false;
    check(handler.run(state,0).outcome==runtime::LiveActionOutcome::rejected &&
          std::get<saves::Expansion2PrefixObservation>(
              state.world.game_state.prefix).scalars[23]==777,
          "BNW game phase accepted a player turn");
    state.world.game_state.flags.push_back({"score_dirty",false});
    check(handler.run(state,-1).outcome==runtime::LiveActionOutcome::rejected &&
          std::get<saves::Expansion2PrefixObservation>(
              state.world.game_state.prefix).scalars[23]==777,
          "BNW phase accepted ambiguous score flags");
    state.world.game_state.flags.pop_back();
    technology.entries[1].and_prerequisites.values[0]=2;
    check(handler.run(state,-1).outcome==runtime::LiveActionOutcome::rejected &&
          std::get<saves::Expansion2PrefixObservation>(
              state.world.game_state.prefix).scalars[23]==777,
          "Invalid religion rules mutated the BNW game phase");
    technology=rules();
    // Team 0 owns B: C costs 30+0+10=40. Team 1 still needs
    // C+B+A+A=70. Minor team 22 is excluded. Integer average is 55.
    check(runtime::compute_bnw_world_religion_tech_progress(
              state,technology,2)==55,
          "BNW religion technology average or root-tech source order changed");
    runtime::update_bnw_world_religion_tech_progress(state,technology,2);
    check(std::get<saves::Expansion2PrefixObservation>(
              state.world.game_state.prefix).scalars[23]==55,
          "BNW religion progress did not update the owned Game state");
    state.world.game_state.controls.front().value=-1;
    runtime::update_bnw_world_religion_tech_progress(state,technology,2);
    check(std::get<saves::Expansion2PrefixObservation>(
              state.world.game_state.prefix).scalars[23]==-1,
          "Missing BNW religion tech must clear its cached progress");
    state.world.game_state.controls.front().value=2;
    technology.entries[1].and_prerequisites.values[0]=2;
    rejects([&] { (void)runtime::compute_bnw_world_religion_tech_progress(
        state,technology,2); });
    technology=rules();
    technology.entries[2].integers["Cost"]=INT32_MAX;
    rejects([&] { (void)runtime::compute_bnw_world_religion_tech_progress(
        state,technology,2); });
    technology=rules();
    state.world.teams->teams[0].fields[1].value=0;
    state.world.teams->teams[1].fields[1].value=0;
    rejects([&] { (void)runtime::compute_bnw_world_religion_tech_progress(
        state,technology,2); });
    if(argc==3) {
        auto selected=ui::bootstrap_selected_bnw_preview(
            std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
        if(selected.preview.blocker)
            throw std::runtime_error(selected.preview.blocker->reason);
        check(selected.preview.host!=nullptr,"Selected BNW host unavailable");
        auto &host=*selected.preview.host;
        auto real=host.session().state();
        const auto &flags=real.world.game_state.flags;
        auto found=std::find_if(flags.begin(),flags.end(),[](const auto &flag) {
            return flag.name && std::string_view(flag.name)=="score_dirty";
        });
        check(found!=flags.end(),"Selected BNW score flag unavailable");
        const auto was_dirty=found->value;
        const auto before=std::get<saves::Expansion2PrefixObservation>(
            real.world.game_state.prefix).scalars[23];
        const auto calls=real.random.gameplay.state().calls;
        auto bound=runtime::make_bnw_game_caches_and_score_handler(
            host.technologies(),5);
        const auto outcome=bound.run(real,-1);
        check(real.random.gameplay.state().calls==calls,
              "Selected BNW cache phase consumed gameplay RNG");
        if(was_dirty)
            check(outcome.outcome==runtime::LiveActionOutcome::rejected &&
                  std::get<saves::Expansion2PrefixObservation>(
                      real.world.game_state.prefix).scalars[23]==before,
                  "Selected BNW dirty score was silently accepted");
        else
            check(outcome.outcome==runtime::LiveActionOutcome::committed &&
                  std::get<saves::Expansion2PrefixObservation>(
                      real.world.game_state.prefix).scalars[23]==
                  runtime::compute_bnw_world_religion_tech_progress(
                      real,host.technologies(),5),
                  "Selected BNW religion cache did not match source calculation");
        for(auto &flag:real.world.game_state.flags)
            if(flag.name && std::string_view(flag.name)=="score_dirty")
                flag.value=true;
        auto complete=runtime::make_bnw_game_caches_and_score_handler(
            host.technologies(),5,host.database(),host.types(),host.defines());
        check(complete.run(real,-1).outcome==runtime::LiveActionOutcome::committed,
              "Selected BNW dirty cache and score phase failed");
        auto cleared=std::find_if(real.world.game_state.flags.begin(),
            real.world.game_state.flags.end(),[](const auto &flag) {
                return flag.name && std::string_view(flag.name)=="score_dirty";
            });
        check(cleared!=real.world.game_state.flags.end() && !cleared->value,
              "Selected BNW score dirty flag was not cleared");
        const auto checkpoint=runtime::make_bnw_game_score_checkpoint(
            real.world,host.session().source().snapshot,host.identity());
        auto reloaded=host.session().source().snapshot;
        runtime::restore_bnw_game_score_checkpoint(
            checkpoint,reloaded,host.session().source().snapshot,host.identity());
        check(reloaded.game_state.roster->arrays==
                  real.world.game_state.roster->arrays &&
              runtime::make_bnw_game_score_checkpoint(
                  reloaded,host.session().source().snapshot,
                  host.identity()).bytes==checkpoint.bytes,
              "Selected BNW score and replay changed after reload");
        std::cout<<"selected BNW score_dirty="<<was_dirty
                 <<" phase="<<(outcome.outcome==runtime::LiveActionOutcome::committed?
                    "committed":"rejected")<<'\n';
    }
}
