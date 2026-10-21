#include "civ5/runtime/bnw_game_supporting_checkpoint.hpp"
#include <stdexcept>
#include <string>

namespace {
using namespace civ5;
void check(bool value,const char *why) {
    if(!value) throw std::runtime_error(why);
}
template<class F> void rejects(F &&run) {
    bool failed=false;
    try {run();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW game supporting checkpoint was accepted");
}
saves::SavedSessionSnapshot fixture() {
    saves::SavedSessionSnapshot world;
    world.game_state.layout=saves::GameStateLayout::expansion2_version1;
    world.map_header.layout=world.game_state.layout;
    world.map_header.grid={3,1};
    std::get<saves::Expansion2PrefixObservation>(
        world.game_state.prefix).scalars[23]=17;
    world.supporting_classes.emplace();
    auto &saved=*world.supporting_classes;
    saved.layout=world.game_state.layout;
    saved.game_version=saved.barbarians_version=saved.goody_huts_version=1;
    saved.plot_count=3;
    saved.major_players=2;
    saved.barbarians.resize(3);
    saved.barbarians[0].counter=2;
    saved.barbarians[0].units_spawned=1;
    saved.barbarians[1].counter=-3;
    saved.barbarians[1].units_spawned=-1;
    saved.barbarians[2].counter=-1;
    saved.goody_hut_results={{{1,2,3}},{{4,5,6}}};
    return world;
}
} // namespace

int main() {
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto source=fixture();
    auto live=source;
    std::get<saves::Expansion2PrefixObservation>(
        live.game_state.prefix).scalars[23]=27;
    auto &saved=*live.supporting_classes;
    saved.barbarians[0].counter=1;
    saved.barbarians[1].counter=-2;
    saved.barbarians[2].counter=-1;
    saved.barbarians[2].units_spawned=4;
    saved.goody_hut_results[1][2]=99;
    const auto checkpoint=runtime::make_bnw_game_supporting_checkpoint(
        live,source,identity);
    check(checkpoint.id==static_cast<std::uint32_t>(
              saves::CivisSaveSectionId::live_bnw_game_supporting),
          "BNW game supporting section ID changed");
    auto restored=source;
    runtime::restore_bnw_game_supporting_checkpoint(
        checkpoint,restored,identity);
    check(std::get<saves::Expansion2PrefixObservation>(
              restored.game_state.prefix).scalars[23]==27 &&
          restored.supporting_classes->barbarians[0].counter==1 &&
          restored.supporting_classes->barbarians[1].counter==-2 &&
          restored.supporting_classes->barbarians[2].units_spawned==4 &&
          restored.supporting_classes->goody_hut_results[1][2]==99,
          "BNW game supporting state did not round-trip");
    auto corrupt=checkpoint;
    constexpr std::size_t first_source_goody=8+4+128+16+8+3*8;
    corrupt.bytes[first_source_goody]^=1;
    rejects([&] {auto state=source;
        runtime::restore_bnw_game_supporting_checkpoint(corrupt,state,identity);});
    corrupt=checkpoint;
    corrupt.bytes.pop_back();
    auto atomic=source;
    rejects([&] {runtime::restore_bnw_game_supporting_checkpoint(
        corrupt,atomic,identity);});
    check(atomic.supporting_classes->barbarians[0].counter==2 &&
          std::get<saves::Expansion2PrefixObservation>(
              atomic.game_state.prefix).scalars[23]==17,
          "Rejected BNW checkpoint partially changed the world");
    corrupt=checkpoint;
    corrupt.bytes[12]^=1;
    rejects([&] {auto state=source;
        runtime::restore_bnw_game_supporting_checkpoint(corrupt,state,identity);});
    auto changed=source;
    changed.supporting_classes->barbarians[1].counter=5;
    rejects([&] {runtime::restore_bnw_game_supporting_checkpoint(
        checkpoint,changed,identity);});
    auto invalid=live;
    invalid.supporting_classes->barbarians[0].counter=32768;
    rejects([&] {(void)runtime::make_bnw_game_supporting_checkpoint(
        invalid,source,identity);});
}
