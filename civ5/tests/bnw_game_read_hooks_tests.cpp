#include "civ5/runtime/bnw_game_read_hooks.hpp"
#include "civ5/runtime/research_context.hpp"
#include <stdexcept>
#include <variant>

namespace {
void check(bool condition) {
    if(!condition) throw std::runtime_error("BNW CvGame::Read scalar parity failed");
}
}

int main() {
    using namespace civ5;
    saves::GameStateObservation source;
    source.layout=saves::GameStateLayout::expansion2_version1;
    source.controls.push_back({"game_state",1});
    auto &serialized=std::get<saves::Expansion2PrefixObservation>(source.prefix);
    serialized.scalars[21]=42;
    auto owned=source;
    runtime::apply_bnw_game_read_scalar_hooks(owned);
    check(std::get<saves::Expansion2PrefixObservation>(owned.prefix).scalars[21]==1);
    check(owned.controls.front().value==2);
    check(std::get<saves::Expansion2PrefixObservation>(source.prefix).scalars[21]==42);
    check(source.controls.front().value==1);

    saves::SavedSessionSnapshot snapshot;
    snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    snapshot.history.layout=saves::GameStateLayout::expansion2_version1;
    snapshot.pregame.emplace();
    snapshot.pregame->layout=saves::GameStateLayout::expansion2_version1;
    snapshot.history.sessions=7;
    snapshot.pregame->archive.integers["game_type"]=0;
    const auto serialized_snapshot=snapshot;
    runtime::apply_bnw_game_read_session_hook(snapshot);
    check(snapshot.history.sessions==8 && serialized_snapshot.history.sessions==7);
    snapshot.pregame->archive.integers["game_type"]=1;
    runtime::apply_bnw_game_read_session_hook(snapshot);
    check(snapshot.history.sessions==8);
    snapshot.pregame->archive.integers["game_type"]=2;
    runtime::apply_bnw_game_read_session_hook(snapshot);
    check(snapshot.history.sessions==9);

    // CvGame::Read chooses the first human civilization in a solo game,
    // regardless of the saved active player or the player's alive flag.
    snapshot.all_players.emplace();
    snapshot.all_players->layout=saves::GameStateLayout::expansion2_version1;
    snapshot.all_players->context.strategy.civ_players=63;
    snapshot.all_players->players.resize(64);
    runtime::ResearchContext setup;
    setup.game_type=0;
    for(std::int64_t id=0;id<64;++id) {
        snapshot.all_players->players[static_cast<std::size_t>(id)].saved_player_id=id;
        setup.players.emplace(id,runtime::ResearchPlayerState{});
    }
    setup.players.at(2).slot_status=3; // SS_TAKEN
    setup.players.at(2).alive=false;
    setup.players.at(5).slot_status=3;
    snapshot.pregame->archive.integers["game_type"]=0;
    snapshot.pregame->archive.integers["active_player"]=5;
    runtime::apply_bnw_game_read_active_player_hook(snapshot,setup);
    check(snapshot.pregame->archive.integers.at("active_player")==2);
    snapshot.pregame->archive.integers["game_type"]=2;
    setup.game_type=2;
    snapshot.pregame->archive.integers["active_player"]=5;
    runtime::apply_bnw_game_read_active_player_hook(snapshot,setup);
    check(snapshot.pregame->archive.integers.at("active_player")==5);
    snapshot.pregame->archive.integers["game_type"]=0;
    setup.game_type=0;
    snapshot.all_players->players[2].saved_player_id=17;
    bool rejected=false;
    try { runtime::apply_bnw_game_read_active_player_hook(snapshot,setup); }
    catch(const std::invalid_argument &) { rejected=true; }
    check(rejected);

    for(const auto count:{-7,0,1}) {
        auto game=source;
        std::get<saves::Expansion2PrefixObservation>(game.prefix).scalars[21]=count;
        game.controls.front().value=0;
        runtime::apply_bnw_game_read_scalar_hooks(game);
        check(std::get<saves::Expansion2PrefixObservation>(game.prefix).scalars[21]==count);
        check(game.controls.front().value==0);
    }
    auto malformed=source;
    malformed.controls.push_back({"game_state",1});
    try { runtime::apply_bnw_game_read_scalar_hooks(malformed); }
    catch(const std::invalid_argument &) { return 0; }
    throw std::runtime_error("Duplicate game state accepted");
}
