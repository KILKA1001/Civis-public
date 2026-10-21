#include "civ5/runtime/bnw_game_score_checkpoint.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {
    if(!ok) throw std::runtime_error(why);
}
template<class F> void rejects(F &&run) {
    bool failed=false;
    try {run();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW score checkpoint was accepted");
}
saves::PlayerTailNode names() {
    saves::PlayerTailNode result;
    result.name="m_ReplayDataSets";
    result.encoding="strings";
    result.wire_count=1;
    saves::PlayerTailNode item;
    item.name="name";
    item.encoding="string32";
    const std::string score="REPLAYDATASET_SCORE";
    item.bytes.assign(score.begin(),score.end());
    item.wire_count=static_cast<std::int64_t>(item.bytes.size());
    result.children.push_back(std::move(item));
    return result;
}
saves::PlayerTailNode values() {
    saves::PlayerTailNode result;
    result.name="m_ReplayDataSetValues";
    result.encoding="turn_sets";
    result.wire_count=1;
    saves::PlayerTailNode item;
    item.name="turns";
    item.encoding="turn_data";
    item.wire_count=1;
    item.values={1,10};
    result.children.push_back(std::move(item));
    return result;
}
saves::SavedSessionSnapshot fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.game_state.roster.emplace();
    auto &roster=*world.game_state.roster;
    roster.dimensions.player_slots=64;
    roster.dimensions.team_slots=64;
    for(auto &array:roster.arrays) array.resize(64);
    world.pregame.emplace();
    world.pregame->layout=bnw;
    world.pregame->archive.integer_arrays["team_types"].resize(64);
    world.all_players.emplace();
    world.all_players->layout=bnw;
    world.all_players->context.player_count=64;
    world.all_players->players.resize(64);
    for(std::size_t p=0;p<64;++p) {
        auto &player=world.all_players->players[p];
        player.index=p;
        player.saved_player_id=static_cast<std::int64_t>(p);
        player.tail.layout=bnw;
        player.tail.fields.push_back(names());
        player.tail.fields.push_back(values());
        if(p==1) {
            player.tail.fields[0].children.clear();
            player.tail.fields[0].wire_count=0;
            player.tail.fields[1].children.clear();
            player.tail.fields[1].wire_count=0;
        }
    }
    return world;
}
} // namespace

void run_tests() {
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto source=fixture();
    auto live=source;
    live.game_state.roster->arrays[1][0]=4;
    live.game_state.roster->arrays[2][0]=3;
    live.game_state.roster->arrays[3][0]=77;
    live.game_state.roster->arrays[4][0]=5;
    live.game_state.roster->arrays[5][0]=6;
    live.game_state.roster->arrays[6][0]=88;
    auto &turns=live.all_players->players[0].tail.fields[1].children[0];
    turns.values.insert(turns.values.end(),{9,77});
    turns.wire_count=2;
    // The source has no score data set for player 1. SDK appends a name and
    // an empty TurnData map, then writes the current turn.
    auto &player=live.all_players->players[1];
    auto name=names().children.front();
    auto value=values().children.front();
    value.values={9,19};
    player.tail.fields[0].children.push_back(std::move(name));
    player.tail.fields[1].children.push_back(std::move(value));
    player.tail.fields[0].wire_count=1;
    player.tail.fields[1].wire_count=1;
    const auto checkpoint=runtime::make_bnw_game_score_checkpoint(
        live,source,identity);
    check(checkpoint.id==runtime::bnw_game_score_checkpoint_section_id &&
          checkpoint.flags==0,"BNW score section ID or flags changed");
    auto restored=source;
    runtime::restore_bnw_game_score_checkpoint(
        checkpoint,restored,source,identity);
    check(restored.game_state.roster->arrays[3][0]==77 &&
          restored.game_state.roster->arrays[6][0]==88 &&
          restored.all_players->players[0].tail.fields[1]
              .children[0].values==std::vector<std::int64_t>({1,10,9,77}) &&
          restored.all_players->players[1].tail.fields[1]
              .children[0].values==std::vector<std::int64_t>({9,19}),
          "BNW score and replay did not round-trip");
    // Re-exporting after restoring from the immutable payload keeps the same
    // complete score/replay section, including the newly added data set.
    check(runtime::make_bnw_game_score_checkpoint(
              restored,source,identity).bytes==checkpoint.bytes,
          "BNW score checkpoint changed across reload");
    auto corrupt=checkpoint;
    corrupt.bytes.pop_back();
    auto unchanged=source;
    rejects([&] {runtime::restore_bnw_game_score_checkpoint(
        corrupt,unchanged,source,identity);});
    check(unchanged.game_state.roster->arrays[3][0]==0 &&
          unchanged.all_players->players[0].tail.fields[1]
              .children[0].values==std::vector<std::int64_t>({1,10}),
          "Rejected BNW score checkpoint changed the destination");
    corrupt=checkpoint;corrupt.bytes[12]^=1;
    rejects([&] {auto world=source;
        runtime::restore_bnw_game_score_checkpoint(
            corrupt,world,source,identity);});
    corrupt=checkpoint;
    // The old frame begins after magic, version, two SHA-256 strings and
    // its byte length. A changed old ranking must not be accepted.
    corrupt.bytes[8+4+128+4+8+10]^=1;
    rejects([&] {auto world=source;
        runtime::restore_bnw_game_score_checkpoint(
            corrupt,world,source,identity);});
    auto wrong_base=source;
    wrong_base.game_state.roster->arrays[3][0]=99;
    rejects([&] {runtime::restore_bnw_game_score_checkpoint(
        checkpoint,wrong_base,source,identity);});
    auto illegal=live;
    illegal.game_state.roster->arrays[3][63]=1;
    rejects([&] {(void)runtime::make_bnw_game_score_checkpoint(
        illegal,source,identity);});
    illegal=live;
    illegal.all_players->players[63].tail.fields[1]
        .children[0].values[1]=999;
    rejects([&] {(void)runtime::make_bnw_game_score_checkpoint(
        illegal,source,identity);});
    illegal=live;
    illegal.all_players->players[0].tail.fields[1]
        .children[0].values={9,77,1,10};
    rejects([&] {(void)runtime::make_bnw_game_score_checkpoint(
        illegal,source,identity);});
}

int main() {
    try {run_tests();return 0;}
    catch(const std::exception &e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
