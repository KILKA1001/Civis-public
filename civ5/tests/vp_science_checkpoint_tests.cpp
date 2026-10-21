#include "civ5/runtime/vp_science_checkpoint.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
unsigned checks=0;
void check(bool okay,const char *message){++checks;if(!okay)throw std::runtime_error(message);}
template<class F> void rejects(F action){
    bool caught=false;try{action();}catch(const std::exception &){caught=true;}
    check(caught,"Expected VP science checkpoint rejection");
}
void round_trip(){
    using namespace civ5;
    constexpr std::int64_t wide=4294967301LL;
    runtime::ResearchContext setup;setup.profile=content::TechnologyProfile::vp542;
    setup.players[0]={};setup.players[wide]={};
    runtime::ScienceContext state;state.profile=content::TechnologyProfile::vp542;
    auto &first=runtime::ensure_vp_science_profile(state.players[0]);
    auto &second=runtime::ensure_vp_science_profile(state.players[wide]);
    first.last_turn_times100=wide+2;first.cached_happiness_total=77;
    second.last_turn_times100=-wide;
    const runtime::SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto section=runtime::make_vp_science_checkpoint(state,setup,identity);
    check(section.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_science_profile)&&
        section.flags==0&&section.bytes.size()==180,"Missing versioned VP science section");
    first.last_turn_times100=0;second.last_turn_times100=0;
    runtime::restore_vp_science_checkpoint(section,state,setup,identity);
    check(runtime::vp_science_profile(state.players.at(0)).last_turn_times100==wide+2&&
        runtime::vp_science_profile(state.players.at(wide)).last_turn_times100==-wide&&
        runtime::vp_science_profile(state.players.at(0)).cached_happiness_total==77,
        "VP science checkpoint lost wide live state or replaced other source inputs");
    auto wrong=identity;wrong.rules_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_vp_science_checkpoint(section,state,setup,wrong);});
    wrong=identity;wrong.payload_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_vp_science_checkpoint(section,state,setup,wrong);});
    auto damaged=section;damaged.bytes.pop_back();
    rejects([&]{runtime::restore_vp_science_checkpoint(damaged,state,setup,identity);});
    check(runtime::vp_science_profile(state.players.at(0)).last_turn_times100==wide+2,
        "Truncated VP science checkpoint changed live state");
    damaged=section;
    std::copy_n(damaged.bytes.begin()+148,8,damaged.bytes.begin()+164);
    rejects([&]{runtime::restore_vp_science_checkpoint(damaged,state,setup,identity);});
    state.players.erase(wide);
    rejects([&]{runtime::restore_vp_science_checkpoint(section,state,setup,identity);});
    state.players[wide]={};
    rejects([&]{runtime::restore_vp_science_checkpoint(section,state,setup,identity);});
    runtime::ensure_vp_science_profile(state.players[wide]);
    auto bnw=setup;bnw.profile=content::TechnologyProfile::bnw;
    rejects([&]{runtime::restore_vp_science_checkpoint(section,state,bnw,identity);});
    runtime::VpScienceCheckpointBudget small;small.bytes=175;
    rejects([&]{runtime::make_vp_science_checkpoint(state,setup,identity,small);});

    saves::CivisSaveDocument doc;doc.flags=1;
    doc.sections={{static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload),0,{1,2,3}},
                  saves::make_save_metadata_section({std::string(64,'d'),{}}),section};
    const auto path=std::filesystem::temp_directory_path()/
        ("civis-vp-science-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    saves::write_civis_save(path,doc);
    const auto loaded=saves::read_loaded_save(path);
    const auto *profile=saves::profile_section(loaded.profile_sections,saves::CivisSaveSectionId::vp_science_profile);
    check(profile&&*profile==section.bytes,"Physical Civis container lost VP science section");
    const auto repacked=saves::make_civis_import_document(loaded,std::string(64,'e'));
    check(repacked.sections.size()==doc.sections.size(),"Repack dropped VP science section");
    rejects([&]{saves::observe_loaded_session(loaded,{});});
    saves::SavedSessionDecodeContext wrong_consumer;
    wrong_consumer.permitted_profile_sections.insert(
        static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_research_profile));
    rejects([&]{saves::observe_loaded_session(loaded,wrong_consumer);});
    std::filesystem::remove(path);
}
} // namespace
int main(){
    try{round_trip();std::cout<<"PASS "<<checks<<" VP science checkpoint checks; runtime_bits="<<sizeof(void*)*8<<'\n';return 0;}
    catch(const std::exception &error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}
}
