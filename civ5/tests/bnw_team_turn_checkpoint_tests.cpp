#include "civ5/runtime/bnw_team_turn_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace civ5;
void check(bool valid,const char *why) {
    if(!valid) throw std::runtime_error(why);
}
template<class F> void rejects(F &&run) {
    bool failed=false;
    try {run();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW team turn checkpoint was accepted");
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
saves::PlotArrayObservation &array(saves::TeamObservation &team,
                                    std::string_view wanted) {
    for(auto &value:team.arrays)
        if(name(value.name)==wanted) return value;
    throw std::runtime_error("Missing BNW team array in test");
}
std::int64_t &points(saves::TeamObservation &team) {
    for(auto &value:team.fields) if(name(value.name)=="victory_points") return value.value;
    throw std::runtime_error("Missing codec fixture victory points");
}
std::int64_t points(const saves::TeamObservation &team) {
    for(const auto &value:team.fields) if(name(value.name)=="victory_points") return value.value;
    throw std::runtime_error("Missing selected victory points");
}
saves::TeamHashedArray &awards(saves::TeamObservation &team) {
    for(auto &value:team.hashes) if(name(value.name)=="small_award_achieved") return value;
    throw std::runtime_error("Missing codec fixture full awards");
}
const saves::TeamHashedArray &awards(const saves::TeamObservation &team) {
    for(const auto &value:team.hashes) if(name(value.name)=="small_award_achieved") return value;
    throw std::runtime_error("Missing selected full awards");
}
saves::SavedSessionSnapshot fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.teams.emplace();
    auto &all=*world.teams;
    all.layout=bnw;
    all.dimensions.teams=64;
    all.dimensions.civ_teams=63;
    all.teams.resize(64);
    for(std::size_t t=0;t<64;++t) {
        auto &team=all.teams[t];
        team.index=t;
        team.fields.push_back({"id",static_cast<std::int64_t>(t)});
        team.fields.push_back({"tech_version",1});
        team.fields.push_back({"saved_techs",3});
        // Explicit detached codec DTO fields, not genuine SmallAwards authority.
        team.fields.push_back({"victory_points",0});
        team.hashes.push_back({"small_award_achieved",3,
                              {{0x111u,{0}},{0,{}},{0x222u,{0}}}});
        team.arrays.push_back({"turns_at_war",1,
                              std::vector<std::int64_t>(64)});
        team.arrays.push_back({"turns_locked_at_war",1,
                              std::vector<std::int64_t>(64)});
        team.arrays.push_back({"tech_hashes",1,{11,22,33}});
        team.arrays.push_back({"no_trade_tech",1,{1,0,1}});
        team.arrays.push_back({"research_progress",1,{10,20,30}});
    }
    return world;
}
void synthetic() {
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto source=fixture();
    auto live=source;
    array(live.teams->teams[0],"turns_at_war").values[1]=9;
    array(live.teams->teams[0],"turns_locked_at_war").values[1]=4;
    array(live.teams->teams[62],"turns_at_war").values[0]=17;
    array(live.teams->teams[0],"no_trade_tech").values[0]=0;
    array(live.teams->teams[63],"no_trade_tech").values[2]=0;
    points(live.teams->teams[0])=21;
    points(live.teams->teams[62])=34;
    awards(live.teams->teams[0]).rows[0].values[0]=1;
    awards(live.teams->teams[62]).rows[2].values[0]=1;
    const auto section=runtime::make_bnw_team_turn_checkpoint(
        live,source,identity);
    check(section.id==runtime::bnw_team_turn_checkpoint_section_id &&
          section.flags==0,"BNW team turn section identity changed");
    auto restored=source;
    array(restored.teams->teams[0],"research_progress").values[1]=77;
    runtime::restore_bnw_team_turn_checkpoint(
        section,restored,source,identity);
    check(array(restored.teams->teams[0],"turns_at_war").values[1]==9 &&
          array(restored.teams->teams[0],"turns_locked_at_war").values[1]==4 &&
          array(restored.teams->teams[62],"turns_at_war").values[0]==17 &&
          array(restored.teams->teams[0],"no_trade_tech").values[0]==0 &&
          array(restored.teams->teams[63],"no_trade_tech").values[2]==0 &&
          array(restored.teams->teams[0],"research_progress").values[1]==77,
          "BNW team turn fields did not compose with ID31 progress");
    check(points(restored.teams->teams[0])==21 &&
          points(restored.teams->teams[62])==34 &&
          awards(restored.teams->teams[0]).wire_count==3 &&
          awards(restored.teams->teams[0]).rows[0].type_hash==0x111u &&
          awards(restored.teams->teams[0]).rows[0].values[0]==1 &&
          awards(restored.teams->teams[0]).rows[1].type_hash==0 &&
          awards(restored.teams->teams[0]).rows[1].values.empty() &&
          awards(restored.teams->teams[62]).rows[2].values[0]==1,
          "BNW V2 did not preserve complete points/award/zero-hole state");
    check(runtime::make_bnw_team_turn_checkpoint(
              restored,source,identity).bytes==section.bytes,
          "BNW team turn checkpoint changed across reload");
    auto corrupt=section;corrupt.bytes.pop_back();
    auto atomic=source;
    rejects([&] {runtime::restore_bnw_team_turn_checkpoint(
        corrupt,atomic,source,identity);});
    check(array(atomic.teams->teams[0],"turns_at_war").values[1]==0 &&
          array(atomic.teams->teams[0],"no_trade_tech").values[0]==1,
          "Rejected BNW team turn checkpoint partially changed world");
    // Last-team malformed destination must not publish earlier valid stores.
    auto late=source;
    late.teams->teams[63].fields.push_back({"victory_points",0});
    rejects([&] {runtime::restore_bnw_team_turn_checkpoint(section,late,source,identity);});
    late.teams->teams[63].fields.pop_back();
    check(runtime::make_bnw_team_turn_checkpoint(late,source,identity).bytes==
          runtime::make_bnw_team_turn_checkpoint(source,source,identity).bytes,
          "Late-team V2 preflight failure partially changed owned state");
    corrupt=section;corrupt.bytes[12]^=1;
    rejects([&] {auto state=source;
        runtime::restore_bnw_team_turn_checkpoint(
            corrupt,state,source,identity);});
    corrupt=section;corrupt.bytes[8+4+128+4+10]^=1;
    rejects([&] {auto state=source;
        runtime::restore_bnw_team_turn_checkpoint(
            corrupt,state,source,identity);});
    auto wrong=source;
    array(wrong.teams->teams[0],"no_trade_tech").values[0]=0;
    rejects([&] {runtime::restore_bnw_team_turn_checkpoint(
        section,wrong,source,identity);});
    auto invalid=live;
    array(invalid.teams->teams[0],"turns_at_war").values[63]=1;
    rejects([&] {(void)runtime::make_bnw_team_turn_checkpoint(
        invalid,source,identity);});
    invalid=live;
    array(invalid.teams->teams[63],"turns_locked_at_war").values[0]=1;
    rejects([&] {(void)runtime::make_bnw_team_turn_checkpoint(
        invalid,source,identity);});
    invalid=live;
    array(invalid.teams->teams[1],"no_trade_tech").values[0]=2;
    rejects([&] {(void)runtime::make_bnw_team_turn_checkpoint(
        invalid,source,identity);});
    invalid=live;
    array(invalid.teams->teams[0],"tech_hashes").values[0]=44;
    rejects([&] {(void)runtime::make_bnw_team_turn_checkpoint(
        invalid,source,identity);});
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &root) {
    auto boot=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(boot.preview.blocker)
        throw std::runtime_error(boot.preview.blocker->reason);
    check(boot.preview.host!=nullptr,"Selected BNW host unavailable");
    const auto &host=*boot.preview.host;
    const auto &source=host.session().source().snapshot;
    auto live=host.session().state().world;
    const auto unchanged=runtime::make_bnw_team_turn_checkpoint(
        source,source,host.identity());
    auto copy=source;
    runtime::restore_bnw_team_turn_checkpoint(
        unchanged,copy,source,host.identity());
    check(runtime::make_bnw_team_turn_checkpoint(
              copy,source,host.identity()).bytes==unchanged.bytes,
          "Selected BNW source team turn did not round-trip");
    auto &team=live.teams->teams.at(0);
    auto &war=array(team,"turns_at_war").values.at(1);
    auto &locked=array(team,"turns_locked_at_war").values.at(1);
    war=war==INT32_MAX?war-1:war+1;
    locked=locked==INT32_MAX?locked-1:locked+1;
    auto &no_trade=array(team,"no_trade_tech").values.at(0);
    no_trade=no_trade?0:1;
    // Actual decoded DTO fields only; ordinary BNW may have zero award rows.
    auto &selected_points=points(team);
    selected_points=selected_points==INT32_MAX?selected_points-1:selected_points+1;
    auto &selected_awards=awards(team);
    for(auto &row:selected_awards.rows) if(row.type_hash) {
        row.values.at(0)=row.values.at(0)?0:1;break;
    }
    const auto changed=runtime::make_bnw_team_turn_checkpoint(
        live,source,host.identity());
    auto restored=source;
    runtime::restore_bnw_team_turn_checkpoint(
        changed,restored,source,host.identity());
    check(points(restored.teams->teams[0])==selected_points &&
          points(source.teams->teams[0])!=selected_points &&
          awards(restored.teams->teams[0]).wire_count==selected_awards.wire_count &&
          awards(restored.teams->teams[0]).rows.size()==selected_awards.rows.size(),
          "Selected raw V2 did not retain actual points/full award span");
    check(runtime::make_bnw_team_turn_checkpoint(
              restored,source,host.identity()).bytes==changed.bytes,
          "Selected BNW changed team turn did not round-trip");
    std::cout<<"selected BNW teams="<<source.teams->teams.size()
             <<" team_turn_bytes="<<changed.bytes.size()<<'\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        synthetic();
        if(argc==3) selected(argv[1],argv[2]);
        return 0;
    } catch(const std::exception &e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
