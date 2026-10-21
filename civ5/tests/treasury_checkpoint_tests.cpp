#include "civ5/runtime/treasury_checkpoint.hpp"
#include "civ5/runtime/vp_treasury_income_profile.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
unsigned checks=0;
void check(bool condition,const char *message){++checks;if(!condition)throw std::runtime_error(message);}
template<class F> void rejects(F action){
    bool caught=false;try{action();}catch(const std::exception &){caught=true;}
    check(caught,"Expected treasury checkpoint rejection");
}
void round_trip(civ5::content::TechnologyProfile profile){
    using namespace civ5;
    constexpr std::int64_t wide=4294967301LL;
    runtime::ResearchContext setup;setup.profile=profile;setup.players[wide].team=wide+1;
    runtime::TreasuryContext state;state.profile=profile;
    auto &player=state.players[wide];
    player.source_core={{wide+10,wide+20}};
    player.source_administration={{wide+30,wide+40}};
    player.balance_times100=wide+50;player.lifetime_gross_gold=wide+60;
    player.balance_history_times100={wide+70,-wide};
    player.income_history_times100={INT64_MAX,-wide-80};
    player.connections_times100=wide+90;
    if(profile==content::TechnologyProfile::vp542)
        runtime::ensure_vp_treasury_income_profile(player).jfd_minors=wide+91;
    const runtime::SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto section=runtime::make_treasury_checkpoint(state,setup,identity);
    check(section.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_treasury)&&
        section.flags==0&&section.bytes.size()>100,"Missing common treasury section");
    state.players[wide].balance_times100=0;
    state.players[wide].lifetime_gross_gold=0;
    state.players[wide].balance_history_times100.clear();
    state.players[wide].income_history_times100.clear();
    if(profile==content::TechnologyProfile::vp542)
        runtime::ensure_vp_treasury_income_profile(state.players[wide]).jfd_minors=wide+101;
    runtime::restore_treasury_checkpoint(section,state,setup,identity);
    const auto &restored=state.players.at(wide);
    check(restored.balance_times100==wide+50&&restored.lifetime_gross_gold==wide+60&&
        restored.balance_history_times100==std::vector<std::int64_t>({wide+70,-wide})&&
        restored.income_history_times100==std::vector<std::int64_t>({INT64_MAX,-wide-80})&&
        restored.connections_times100==wide+90,
        "Treasury checkpoint lost x64 live history or changed source-only inputs");
    check(profile==content::TechnologyProfile::vp542?
        runtime::vp_treasury_income_profile(restored).jfd_minors==wide+101:
        !restored.profile_extensions.find<runtime::VpTreasuryIncomeProfile>(),
        "Common treasury checkpoint replaced GameCore-owned income state");
    auto wrong=identity;wrong.rules_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_treasury_checkpoint(section,state,setup,wrong);});
    wrong=identity;wrong.payload_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_treasury_checkpoint(section,state,setup,wrong);});
    auto damaged=section;damaged.bytes.pop_back();
    auto before=state;
    rejects([&]{runtime::restore_treasury_checkpoint(damaged,state,setup,identity);});
    check(state.players.at(wide).balance_history_times100==before.players.at(wide).balance_history_times100,
        "Truncated treasury section partially changed live state");
    state.players[wide].source_administration={{wide+30,wide+41}};
    rejects([&]{runtime::restore_treasury_checkpoint(section,state,setup,identity);});
    state.players[wide].source_administration=before.players.at(wide).source_administration;
    state.players.erase(wide);
    rejects([&]{runtime::restore_treasury_checkpoint(section,state,setup,identity);});
    state=std::move(before);
    runtime::TreasuryCheckpointBudget small;small.history_entries=3;
    rejects([&]{runtime::make_treasury_checkpoint(state,setup,identity,small);});

    saves::CivisSaveDocument doc;doc.flags=1;
    doc.sections={{static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload),0,{1,2,3}},
                  saves::make_save_metadata_section({std::string(64,'d'),{}}),section};
    const auto path=std::filesystem::temp_directory_path()/
        ("civis-treasury-checkpoint-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    saves::write_civis_save(path,doc);
    const auto loaded=saves::read_loaded_save(path);
    check(loaded.live_treasury==section.bytes,"Physical Civis container lost treasury state");
    const auto repacked=saves::make_civis_import_document(loaded,std::string(64,'e'));
    check(repacked.sections.size()==doc.sections.size(),"Repack dropped treasury section");
    rejects([&]{saves::observe_loaded_session(loaded,{});});
    std::filesystem::remove(path);
}
} // namespace
int main(){
    try{
        round_trip(civ5::content::TechnologyProfile::bnw);
        round_trip(civ5::content::TechnologyProfile::vp542);
        std::cout<<"PASS "<<checks<<" treasury checkpoint checks; runtime_bits="<<sizeof(void*)*8<<'\n';
        return 0;
    }catch(const std::exception &error){
        std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;
    }
}
