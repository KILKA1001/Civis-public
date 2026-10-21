#include "civ5/runtime/research_turn_checkpoint.hpp"
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
    check(caught,"Expected research-turn checkpoint rejection");
}
void round_trip(civ5::content::TechnologyProfile profile){
    using namespace civ5;
    constexpr std::int64_t wide=4294967301LL;
    content::TechnologyRules technologies;technologies.profile=profile;
    technologies.collection.slots=wide+10;
    technologies.entries[7].base.type="TECH_SEVEN";
    technologies.entries[wide+2].base.type="TECH_WIDE";
    runtime::ResearchContext setup;setup.profile=profile;setup.players[wide].team=wide+1;
    runtime::ResearchTurnState state;state.profile=profile;
    auto &player=state.players[wide];
    player.cities_founded=wide+3;
    player.agreement_counters={{wide+4,wide+5},{wide+6,-wide}};
    player.researching={7,wide+2};
    player.founded_first_city=true;
    player.leader_type="LEADER_ORIGINAL";
    player.unmapped_researching=17;
    const runtime::SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto section=runtime::make_research_turn_checkpoint(state,setup,technologies,identity);
    check(section.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_research_turn)&&
        section.flags==0&&section.bytes.size()>100,"Missing common research-turn section");
    player.cities_founded=0;player.agreement_counters.clear();player.researching.clear();
    player.founded_first_city=false;player.leader_type="LEADER_CHANGED";
    runtime::restore_research_turn_checkpoint(section,state,setup,technologies,identity);
    const auto &restored=state.players.at(wide);
    check(restored.cities_founded==wide+3&&
        restored.agreement_counters==runtime::YieldValues({{wide+4,wide+5},{wide+6,-wide}})&&
        restored.researching==std::set<std::int64_t>({7,wide+2})&&
        !restored.founded_first_city&&restored.leader_type=="LEADER_CHANGED"&&
        restored.unmapped_researching==17,
        "Research-turn checkpoint lost wide live data or changed source-only fields");
    auto wrong=identity;wrong.rules_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_research_turn_checkpoint(section,state,setup,technologies,wrong);});
    wrong=identity;wrong.payload_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_research_turn_checkpoint(section,state,setup,technologies,wrong);});
    auto damaged=section;damaged.bytes.pop_back();
    auto before=state;
    rejects([&]{runtime::restore_research_turn_checkpoint(damaged,state,setup,technologies,identity);});
    check(state.players.at(wide).agreement_counters==before.players.at(wide).agreement_counters,
        "Truncated research-turn section partially changed live state");
    state.players.erase(wide);
    rejects([&]{runtime::restore_research_turn_checkpoint(section,state,setup,technologies,identity);});
    state=std::move(before);
    auto wrong_catalog=technologies;wrong_catalog.collection.slots=wide+2;
    rejects([&]{runtime::restore_research_turn_checkpoint(section,state,setup,wrong_catalog,identity);});
    runtime::ResearchTurnCheckpointBudget small;small.entries=3;
    rejects([&]{runtime::make_research_turn_checkpoint(state,setup,technologies,identity,small);});

    saves::CivisSaveDocument doc;doc.flags=1;
    doc.sections={{static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload),0,{1,2,3}},
                  saves::make_save_metadata_section({std::string(64,'d'),{}}),section};
    const auto path=std::filesystem::temp_directory_path()/
        ("civis-research-turn-checkpoint-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    saves::write_civis_save(path,doc);
    const auto loaded=saves::read_loaded_save(path);
    check(loaded.live_research_turn==section.bytes,"Physical Civis container lost research-turn state");
    const auto repacked=saves::make_civis_import_document(loaded,std::string(64,'e'));
    check(repacked.sections.size()==doc.sections.size(),"Repack dropped research-turn section");
    rejects([&]{saves::observe_loaded_session(loaded,{});});
    std::filesystem::remove(path);
}
} // namespace
int main(){
    try{
        round_trip(civ5::content::TechnologyProfile::bnw);
        round_trip(civ5::content::TechnologyProfile::vp542);
        std::cout<<"PASS "<<checks<<" research-turn checkpoint checks; runtime_bits="<<sizeof(void*)*8<<'\n';
        return 0;
    }catch(const std::exception &error){
        std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;
    }
}
