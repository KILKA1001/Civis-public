#include "civ5/runtime/bnw_team_progress_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool caught=false;try {f();} catch(const std::exception &) {caught=true;}
    check(caught,"Invalid BNW team progress checkpoint accepted");
}
std::string_view name(const char *s) {return s?std::string_view(s):std::string_view();}
const saves::PlotArrayObservation &arr(const saves::TeamObservation &team,
                                       std::string_view wanted) {
    for(const auto &a:team.arrays) if(name(a.name)==wanted) return a;
    throw std::runtime_error("Selected BNW team technology array unavailable");
}
saves::PlotArrayObservation &mutable_arr(saves::TeamObservation &team,
                                         std::string_view wanted) {
    for(auto &a:team.arrays) if(name(a.name)==wanted) return a;
    throw std::runtime_error("Selected BNW team technology array unavailable");
}
std::int64_t &progress(saves::SavedSessionSnapshot &world,
                       std::size_t team,std::size_t row) {
    for(auto &a:world.teams->teams.at(team).arrays)
        if(name(a.name)=="research_progress") return a.values.at(row);
    throw std::runtime_error("Selected BNW research progress unavailable");
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &doc,
                                       std::uint32_t id) {
    for(const auto &s:doc.sections) if(s.id==id) return s;
    throw std::runtime_error("Missing BNW team progress section");
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto input=host.preview_input();
    check(input.frame_sources.database && input.frame_sources.technologies,
          "Selected BNW active technology rules unavailable");
    const auto &rules=*input.frame_sources.technologies;
    const auto types=input.frame_sources.database->cache_info_types();
    const auto &baseline=host.session().state();
    const auto &source=host.session().source().snapshot;
    auto state=baseline;
    check(source.teams && source.teams->teams.size()==64,
          "Selected BNW immutable source team roster unavailable");
    std::size_t team=0,row=0;
    std::int64_t slot=-1;
    for(std::size_t t=0;t<source.teams->teams.size() && slot<0;++t) {
        const auto &hashes=arr(source.teams->teams[t],"tech_hashes").values;
        for(std::size_t r=0;r<hashes.size();++r) {
            if(hashes[r]<=0) continue;
            const auto *entry=types.find_hash(static_cast<std::uint64_t>(hashes[r]));
            if(!entry || entry->id<0 || !rules.find(
                static_cast<std::uint64_t>(entry->id))) continue;
            if(arr(source.teams->teams[t],"research_progress").values[r]>=INT32_MAX-2)
                continue;
            team=t;row=r;slot=entry->id;break;
        }
    }
    check(slot>=0,"Selected BNW has no mapped progress row");
    const auto before=progress(state.world,team,row);
    progress(state.world,team,row)=before+1;
    state.technologies.at(team).set_progress_times100(slot,before+1);
    const auto pair=runtime::make_bnw_team_progress_checkpoint_pair(
        state,source,baseline.setup,baseline.technologies,baseline.player_research,
        rules,types,host.identity());
    check(pair.raw.id==runtime::bnw_team_progress_checkpoint_section_id &&
          pair.owned.id==static_cast<std::uint32_t>(
              saves::CivisSaveSectionId::live_team_research),
          "BNW raw/owned research section identity changed");
    {
        auto &has_tech=mutable_arr(state.world.teams->teams[team],
                                  "has_tech").values.at(row);
        ++has_tech;
        rejects([&] {(void)runtime::make_bnw_team_progress_checkpoint_pair(
            state,source,baseline.setup,baseline.technologies,baseline.player_research,
            rules,types,host.identity());});
        --has_tech;
        ++progress(state.world,team,row);
        rejects([&] {(void)runtime::make_bnw_team_progress_checkpoint_pair(
            state,source,baseline.setup,baseline.technologies,baseline.player_research,
            rules,types,host.identity());});
        --progress(state.world,team,row);
        ++state.player_research.players.begin()->second.overflow_times100;
        rejects([&] {(void)runtime::make_bnw_team_progress_checkpoint_pair(
            state,source,baseline.setup,baseline.technologies,baseline.player_research,
            rules,types,host.identity());});
        --state.player_research.players.begin()->second.overflow_times100;
        ++state.setup.game_turn;
        rejects([&] {(void)runtime::make_bnw_team_progress_checkpoint_pair(
            state,source,baseline.setup,baseline.technologies,
            baseline.player_research,rules,types,host.identity());});
        --state.setup.game_turn;
        ++state.world.teams->teams[team].index;
        rejects([&] {(void)runtime::make_bnw_team_progress_checkpoint_pair(
            state,source,baseline.setup,baseline.technologies,baseline.player_research,
            rules,types,host.identity());});
        --state.world.teams->teams[team].index;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first_path=std::filesystem::current_path()/
        ("bnw-team-progress-"+suffix+"-1.CivisSave");
    const auto second_path=std::filesystem::current_path()/
        ("bnw-team-progress-"+suffix+"-2.CivisSave");
    struct Cleanup {std::filesystem::path first,second;~Cleanup() {
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }} cleanup{first_path,second_path};
    auto document=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    document.sections.push_back(pair.raw);document.sections.push_back(pair.owned);
    saves::write_civis_save(first_path,document);
    const auto reopened=saves::read_civis_save(first_path);
    runtime::BnwTeamProgressCheckpointPair loaded{
        section(reopened,runtime::bnw_team_progress_checkpoint_section_id),
        section(reopened,static_cast<std::uint32_t>(
            saves::CivisSaveSectionId::live_team_research))};
    {
        auto corrupt=loaded;
        corrupt.raw.bytes.pop_back();
        rejects([&] {(void)runtime::restore_bnw_team_progress_checkpoint_pair(
            corrupt,source,baseline.setup,baseline.technologies,
            baseline.player_research,rules,types,host.identity());});
        auto foreign=host.identity();
        foreign.rules_sha256=std::string(64,'0');
        rejects([&] {(void)runtime::restore_bnw_team_progress_checkpoint_pair(
            loaded,source,baseline.setup,baseline.technologies,
            baseline.player_research,rules,types,foreign);});
    }
    auto restored=runtime::restore_bnw_team_progress_checkpoint_pair(
        loaded,source,baseline.setup,baseline.technologies,
        baseline.player_research,rules,types,host.identity());
    check(progress(restored.world,team,row)==before+1 &&
          restored.technologies.at(team).progress_times100(slot)==before+1 &&
          arr(source.teams->teams[team],"research_progress").values[row]==before,
          "BNW raw/owned team progress did not survive write/read");
    {
        auto mixed=loaded;
        auto later=state;
        ++progress(later.world,team,row);
        later.technologies.at(team).set_progress_times100(slot,before+2);
        const auto second=runtime::make_bnw_team_progress_checkpoint_pair(
            later,source,baseline.setup,baseline.technologies,baseline.player_research,
            rules,types,host.identity());
        mixed.owned=second.owned;
        rejects([&] {(void)runtime::restore_bnw_team_progress_checkpoint_pair(
            mixed,source,baseline.setup,baseline.technologies,
            baseline.player_research,rules,types,host.identity());});
    }
    progress(restored.world,team,row)=before+2;
    restored.technologies.at(team).set_progress_times100(slot,before+2);
    state.world=std::move(restored.world);
    state.technologies=std::move(restored.technologies);
    const auto again=runtime::make_bnw_team_progress_checkpoint_pair(
        state,source,baseline.setup,baseline.technologies,baseline.player_research,
        rules,types,host.identity());
    auto next=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    next.sections.push_back(again.raw);next.sections.push_back(again.owned);
    saves::write_civis_save(second_path,next);
    const auto reread=saves::read_civis_save(second_path);
    auto final=runtime::restore_bnw_team_progress_checkpoint_pair(
        {section(reread,runtime::bnw_team_progress_checkpoint_section_id),
         section(reread,static_cast<std::uint32_t>(
             saves::CivisSaveSectionId::live_team_research))},
        source,baseline.setup,baseline.technologies,baseline.player_research,
        rules,types,host.identity());
    check(progress(final.world,team,row)==before+2 &&
          final.technologies.at(team).progress_times100(slot)==before+2 &&
          runtime::identify_file(path).sha256==original.sha256,
          "BNW team progress second save or original source changed");
    std::cout << "PASS BNW raw/owned team progress checkpoint\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW raw/owned team progress: "
                  << error.what() << '\n';return 1;
    }
}
