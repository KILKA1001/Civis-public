#include "civ5/runtime/bnw_team_turn_pair.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool caught=false;
    try {f();} catch(const std::exception &) {caught=true;}
    check(caught,"Invalid BNW team turn pair accepted");
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
saves::PlotArrayObservation &array(saves::TeamObservation &team,
                                    std::string_view wanted) {
    for(auto &entry:team.arrays) if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Missing selected BNW technology array");
}
const saves::PlotArrayObservation &array(const saves::TeamObservation &team,
                                         std::string_view wanted) {
    for(const auto &entry:team.arrays) if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Missing selected BNW technology array");
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
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &document,
                                       std::uint32_t id) {
    for(const auto &entry:document.sections) if(entry.id==id) return entry;
    throw std::runtime_error("Missing BNW team turn pair section");
}
void synthetic() {
    content::RulesDatabase database;
    database.apply_sql("CREATE TABLE Technologies(ID INTEGER PRIMARY KEY,Type TEXT UNIQUE);"
                       "INSERT INTO Technologies VALUES(0,'TECH_ZERO');"
                       "INSERT INTO Technologies VALUES(1,'TECH_ONE');"
                       "INSERT INTO Technologies VALUES(2,'TECH_TWO');");
    const auto types=database.cache_info_types();
    content::TechnologyRules rules;
    rules.profile=content::TechnologyProfile::bnw;
    rules.collection.slots=3;
    for(std::uint64_t i=0;i<3;++i) rules.entries[i].base.type=
        i==0?"TECH_ZERO":i==1?"TECH_ONE":"TECH_TWO";
    saves::SavedSessionSnapshot source;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    source.game_state.layout=bnw;
    source.teams.emplace();
    source.teams->layout=bnw;
    source.teams->dimensions.teams=64;
    source.teams->dimensions.civ_teams=63;
    source.teams->teams.resize(64);
    for(std::size_t team=0;team<64;++team) {
        auto &saved=source.teams->teams[team];
        saved.index=team;
        saved.fields={{"id",static_cast<std::int64_t>(team)},
                      {"tech_version",1},{"saved_techs",3},{"last_tech",-1}};
        // Complete detached codec fixture, not a retained action capability.
        saved.fields.push_back({"victory_points",0});
        saved.hashes.push_back({"small_award_achieved",3,
                               {{0x111u,{0}},{0,{}},{0x222u,{0}}}});
        saved.arrays.push_back({"turns_at_war",1,std::vector<std::int64_t>(64)});
        saved.arrays.push_back({"turns_locked_at_war",1,
                                std::vector<std::int64_t>(64)});
        saved.arrays.push_back({"tech_hashes",1,{
            content::saved_info_type_hash("TECH_ZERO"),
            content::saved_info_type_hash("TECH_ONE"),
            content::saved_info_type_hash("TECH_TWO")}});
        saved.arrays.push_back({"has_tech",1,{0,0,0}});
        saved.arrays.push_back({"no_trade_tech",1,{1,0,1}});
        saved.arrays.push_back({"research_progress",1,{10,20,30}});
        saved.arrays.push_back({"tech_count",1,{0,0,0}});
    }
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    runtime::LiveSessionState state{source,runtime::ResearchContext{},
        {},runtime::PlayerResearchContext{},runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{},runtime::MaintenanceContext{},
        runtime::TreasuryContext{},runtime::ScienceContext{},
        runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
    state.setup.profile=content::TechnologyProfile::bnw;
    for(std::int64_t team=0;team<64;++team)
        state.setup.team_members[team]=0;
    state.technologies=runtime::restore_team_technologies(*source.teams,rules,types);
    state.player_research.profile=content::TechnologyProfile::bnw;
    const auto baseline_technologies=state.technologies;
    const auto baseline_players=state.player_research;
    array(state.world.teams->teams[0],"no_trade_tech").values[1]=1;
    state.technologies[0].set_no_trade(1,true);
    array(state.world.teams->teams[0],"turns_at_war").values[1]=5;
    array(state.world.teams->teams[62],"turns_locked_at_war").values[0]=3;
    array(state.world.teams->teams[63],"no_trade_tech").values[2]=0;
    state.technologies[63].set_no_trade(2,false);
    points(state.world.teams->teams[0])=35;
    points(state.world.teams->teams[62])=49;
    awards(state.world.teams->teams[0]).rows[0].values[0]=1;
    awards(state.world.teams->teams[62]).rows[2].values[0]=1;
    const auto pair=runtime::make_bnw_team_turn_checkpoint_pair(state,source,
        baseline_technologies,baseline_players,rules,types,identity);
    const auto restored=runtime::restore_bnw_team_turn_checkpoint_pair(pair,
        source,state.setup,baseline_technologies,baseline_players,rules,types,
        identity);
    check(restored.technologies[0].at(1).no_trade &&
          !restored.technologies[63].at(2).no_trade &&
          array(restored.world.teams->teams[0],"turns_at_war").values[1]==5 &&
          array(restored.world.teams->teams[62],"turns_locked_at_war").values[0]==3,
          "Synthetic BNW team turn pair did not roundtrip all 64 teams");
    check(points(restored.world.teams->teams[0])==35 &&
          points(restored.world.teams->teams[62])==49 &&
          awards(restored.world.teams->teams[0]).rows[0].values[0]==1 &&
          awards(restored.world.teams->teams[0]).rows[1].type_hash==0 &&
          awards(restored.world.teams->teams[0]).rows[1].values.empty() &&
          awards(restored.world.teams->teams[62]).rows[2].values[0]==1,
          "Paired V2 transaction lost points/full award rows");
    auto resumed=state;
    resumed.world=restored.world;resumed.technologies=restored.technologies;
    resumed.player_research=restored.player_research;
    const auto saved_again=runtime::make_bnw_team_turn_checkpoint_pair(resumed,source,
        baseline_technologies,baseline_players,rules,types,identity);
    check(saved_again.raw.bytes==pair.raw.bytes && saved_again.owned.bytes==pair.owned.bytes,
          "Paired V2 raw/owned state changed across restore and re-save");
    auto mismatch=pair;
    mismatch.owned=runtime::make_research_checkpoint(baseline_technologies,
        baseline_players,state.setup,rules,identity).common;
    rejects([&] {(void)runtime::restore_bnw_team_turn_checkpoint_pair(
        mismatch,source,state.setup,baseline_technologies,baseline_players,
        rules,types,identity);});
    mismatch=pair;
    mismatch.raw.bytes.pop_back();
    rejects([&] {(void)runtime::restore_bnw_team_turn_checkpoint_pair(
        mismatch,source,state.setup,baseline_technologies,baseline_players,
        rules,types,identity);});
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto input=host.preview_input();
    check(input.frame_sources.database && input.frame_sources.technologies,
          "Selected BNW rules unavailable");
    const auto &rules=*input.frame_sources.technologies;
    const auto types=input.frame_sources.database->cache_info_types();
    const auto &baseline=host.session().state();
    const auto &source=host.session().source().snapshot;
    check(source.teams && source.teams->teams.size()==64,
          "Selected BNW 64-team roster unavailable");
    auto state=baseline;
    std::size_t team=0,row=0;
    std::int64_t slot=-1;
    for(std::size_t t=0;t<64 && slot<0;++t) {
        const auto &hashes=array(source.teams->teams[t],"tech_hashes").values;
        for(std::size_t r=0;r<hashes.size();++r) {
            if(hashes[r]<=0) continue;
            const auto *entry=types.find_hash(static_cast<std::uint64_t>(hashes[r]));
            if(!entry || entry->id<0 || !rules.find(
                static_cast<std::uint64_t>(entry->id))) continue;
            team=t;row=r;slot=entry->id;break;
        }
    }
    check(slot>=0,"Selected BNW has no mapped technology row");
    auto &flag=array(state.world.teams->teams[team],"no_trade_tech").values[row];
    check(flag==0 || flag==1,"Selected BNW no-trade flag is invalid");
    const auto old=flag;
    flag=1-old;
    state.technologies[team].set_no_trade(slot,flag!=0);
    // Actual imported fields; never invent SmallAwards rows for empty BNW.
    auto &selected_points=points(state.world.teams->teams[team]);
    selected_points=selected_points==INT32_MAX?selected_points-1:selected_points+1;
    auto &selected_awards=awards(state.world.teams->teams[team]);
    for(auto &award:selected_awards.rows) if(award.type_hash) {
        award.values.at(0)=award.values.at(0)?0:1;break;
    }
    auto make=[&](const runtime::LiveSessionState &value) {
        return runtime::make_bnw_team_turn_checkpoint_pair(value,source,
            baseline.technologies,baseline.player_research,rules,types,
            host.identity());
    };
    auto pair=make(state);
    check(pair.raw.id==runtime::bnw_team_turn_checkpoint_section_id &&
          pair.owned.id==static_cast<std::uint32_t>(
              saves::CivisSaveSectionId::live_team_research),
          "BNW team turn pair IDs changed");
    // Synthetic independent mutations: either half alone, another ID8 field,
    // and the competing ID31 progress owner must all be rejected.
    {
        auto mismatch=state;
        array(mismatch.world.teams->teams[team],"no_trade_tech").values[row]=old;
        rejects([&] {(void)make(mismatch);});
        mismatch=state;
        mismatch.technologies[team].set_no_trade(slot,old!=0);
        rejects([&] {(void)make(mismatch);});
        mismatch=state;
        mismatch.technologies[team].set_progress_times100(slot,
            mismatch.technologies[team].progress_times100(slot)+1);
        rejects([&] {(void)make(mismatch);});
        mismatch=state;
        ++mismatch.player_research.players.begin()->second.overflow_times100;
        rejects([&] {(void)make(mismatch);});
        mismatch=state;
        ++array(mismatch.world.teams->teams[team],"research_progress").values[row];
        rejects([&] {(void)make(mismatch);});
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto output=std::filesystem::current_path()/
        ("bnw-team-turn-pair-"+suffix+".CivisSave");
    struct Cleanup {std::filesystem::path file;~Cleanup() {
        std::error_code error;std::filesystem::remove(file,error);
    }} cleanup{output};
    auto document=saves::make_civis_import_document(
        host.session().source().source,runtime::identify_file(path).sha256);
    document.sections.push_back(pair.raw);
    document.sections.push_back(pair.owned);
    saves::write_civis_save(output,document);
    const auto reopened=saves::read_civis_save(output);
    const runtime::BnwTeamTurnCheckpointPair loaded{
        section(reopened,runtime::bnw_team_turn_checkpoint_section_id),
        section(reopened,static_cast<std::uint32_t>(
            saves::CivisSaveSectionId::live_team_research))};
    auto restore=[&](const runtime::BnwTeamTurnCheckpointPair &value) {
        return runtime::restore_bnw_team_turn_checkpoint_pair(value,source,
            baseline.setup,baseline.technologies,baseline.player_research,
            rules,types,host.identity());
    };
    auto restored=restore(loaded);
    check(array(restored.world.teams->teams[team],"no_trade_tech").values[row]==flag &&
          restored.technologies[team].at(slot).no_trade==(flag!=0) &&
          array(source.teams->teams[team],"no_trade_tech").values[row]==old,
          "BNW team turn pair failed write/read/restore");
    check(points(restored.world.teams->teams[team])==selected_points &&
          points(source.teams->teams[team])!=selected_points &&
          awards(restored.world.teams->teams[team]).wire_count==selected_awards.wire_count &&
          awards(restored.world.teams->teams[team]).rows.size()==selected_awards.rows.size(),
          "Selected paired V2 file restore lost actual points/award span");
    auto resumed=baseline;
    resumed.world=restored.world;resumed.technologies=restored.technologies;
    resumed.player_research=restored.player_research;
    const auto saved_again=make(resumed);
    check(saved_again.raw.bytes==loaded.raw.bytes && saved_again.owned.bytes==loaded.owned.bytes,
          "Selected paired V2 file restore/re-save changed raw/owned payload");
    {
        auto bad=loaded;
        bad.raw.bytes.pop_back();
        rejects([&] {(void)restore(bad);});
        bad=loaded;
        bad.owned.bytes.pop_back();
        rejects([&] {(void)restore(bad);});
        bad=loaded;
        auto foreign=host.identity();
        foreign.rules_sha256=std::string(64,'0');
        rejects([&] {(void)runtime::restore_bnw_team_turn_checkpoint_pair(
            bad,source,baseline.setup,baseline.technologies,
            baseline.player_research,rules,types,foreign);});
    }
    // War counters alone still carry ID8, even though its owned values remain
    // at baseline. This is the section-pair contract used by the common loader.
    auto war=baseline;
    auto &counter=array(war.world.teams->teams[team],"turns_at_war").values;
    check(!counter.empty(),"Selected BNW war counters unavailable");
    const auto opponent=(team+1)%63;
    ++counter[opponent];
    auto war_pair=make(war);
    auto war_restored=restore(war_pair);
    check(war_restored.technologies[team].at(slot)==baseline.technologies[team].at(slot) &&
          array(war_restored.world.teams->teams[team],"turns_at_war").values[opponent]==
              counter[opponent],
          "BNW war-only pair failed");
    std::cout << "PASS BNW team turn raw/owned pair selected and synthetic\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        synthetic();
        if(argc==1) return 0;
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW team turn pair: " << error.what() << '\n';
        return 1;
    }
}
