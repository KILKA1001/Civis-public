#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/research_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/vp_rules_fingerprint_profile.hpp"
#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/runtime/vp_team_technologies_profile.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
unsigned checks=0;
void check(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F action){
    bool caught=false;try{action();}catch(const std::exception &){caught=true;}
    check(caught,"Expected research checkpoint rejection");
}
void round_trip(civ5::content::TechnologyProfile profile){
    using namespace civ5;
    constexpr std::int64_t big=4294967300LL;
    content::TechnologyRules rules;rules.profile=profile;rules.collection.slots=big+10;
    rules.entries[7].base.type="TECH7";rules.entries[big+2].base.type="TECH_WIDE";
    runtime::ResearchContext setup;setup.profile=profile;
    setup.players[big].team=big+1;setup.team_members[big+1]=1;
    runtime::PlayerResearchContext players;players.profile=profile;
    auto &player=players.players[big];player.queue={7,big+2};
    player.overflow_times100=big+50;player.max_effective_cities=big+60;
    if(profile==content::TechnologyProfile::vp542)
        player.profile_extensions.emplace<runtime::VpPlayerResearchProfileState>().science_from_league=big+70;
    std::vector<runtime::TeamTechnologies> teams;
    teams.emplace_back(profile,big+1,rules.collection.slots);
    teams[0].set_progress_times100(7,big+10);
    teams[0].set_has_tech(big+2,true);
    teams[0].set_no_trade(big+2,true);
    teams[0].increment_count(big+2);
    if(profile==content::TechnologyProfile::vp542){
        vp_team_set_eureka(teams[0],big+2,big+5);vp_team_change_known_count(teams[0],big+6);
    }
    const runtime::ResearchCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto codec=runtime::research_checkpoint_profile_codec(profile);
    check(codec.wire_id==(profile==content::TechnologyProfile::vp542?2U:1U)&&
        (codec.make!=nullptr)==(profile==content::TechnologyProfile::vp542)&&
        (codec.restore!=nullptr)==(profile==content::TechnologyProfile::vp542),
        "Research checkpoint codec crossed the GameCore profile boundary");
    const auto bundle=runtime::make_research_checkpoint(teams,players,setup,rules,identity);
    check(bundle.common.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_team_research)&&
        bundle.profile.has_value()==(profile==content::TechnologyProfile::vp542),
        "Common and VP checkpoint sections crossed their profile boundary");
    if(bundle.profile)
        check(bundle.common.bytes.size()==300&&bundle.profile->bytes.size()==152&&
            runtime::identify_bytes(bundle.common.bytes).sha256==
                "da883c91a642202eae5473b073217318f387e8849968ae0b8c23bc6f815a3426"&&
            runtime::identify_bytes(bundle.profile->bytes).sha256==
                "538caeda66e2d6a90113d8d9176c7bef1673870aec44a9a3fbb9d26a18aa735f",
            "VP research checkpoint bytes changed from the existing v1 wire format");
    auto original_teams=teams;auto original_players=players;
    teams[0].set_progress_times100(7,0);teams[0].set_has_tech(big+2,false);
    players.players.at(big).queue.clear();players.players.at(big).overflow_times100=0;
    players.players.at(big).max_effective_cities=1;
    if(profile==content::TechnologyProfile::vp542){
        vp_team_set_eureka(teams[0],big+2,0);
        vp_team_change_known_count(teams[0],-(big+6));
        runtime::vp_player_research_profile(players.players.at(big)).science_from_league=0;
    }
    runtime::restore_research_checkpoint(bundle.common,bundle.profile,teams,players,setup,rules,identity);
    check(teams[0].progress_times100(7)==big+10&&teams[0].has_tech(big+2)&&
        teams[0].at(big+2).no_trade&&teams[0].at(big+2).count==1&&
        players.players.at(big).queue==std::vector<std::int64_t>({7,big+2})&&
        players.players.at(big).overflow_times100==big+50&&
        players.players.at(big).max_effective_cities==big+60,
        "Research checkpoint did not restore shared wide live state");
    if(profile==content::TechnologyProfile::vp542)
        check(vp_team_eureka(teams[0],big+2)==big+5&&teams[0].known_count()==big+6&&
            runtime::vp_player_research_profile(players.players.at(big)).science_from_league==big+70,
            "VP profile checkpoint did not restore Eureka or cached league science");
    else check(players.players.at(big).profile_extensions.find<runtime::VpPlayerResearchProfileState>()==nullptr,
        "BNW checkpoint incorrectly created a VP player profile");
    auto wrong=identity;wrong.rules_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_research_checkpoint(bundle.common,bundle.profile,teams,players,setup,rules,wrong);});
    wrong=identity;wrong.payload_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_research_checkpoint(bundle.common,bundle.profile,teams,players,setup,rules,wrong);});
    auto damaged=bundle.common;damaged.bytes.pop_back();
    rejects([&]{runtime::restore_research_checkpoint(damaged,bundle.profile,teams,players,setup,rules,identity);});
    auto missing=players;missing.players.erase(big);
    rejects([&]{runtime::restore_research_checkpoint(bundle.common,bundle.profile,teams,missing,setup,rules,identity);});
    if(bundle.profile){
        auto corrupt=*bundle.profile;corrupt.bytes.pop_back();
        teams[0].set_progress_times100(7,-123);
        players.players.at(big).queue={7};
        auto unchanged=teams;auto player_unchanged=players;
        rejects([&]{runtime::restore_research_checkpoint(bundle.common,corrupt,teams,players,setup,rules,identity);});
        check(teams[0].progress_times100(7)==unchanged[0].progress_times100(7)&&
            vp_team_eureka(teams[0],big+2)==vp_team_eureka(unchanged[0],big+2)&&
            players.players.at(big).queue==player_unchanged.players.at(big).queue,
            "Invalid VP section partially changed common research state");
        rejects([&]{runtime::restore_research_checkpoint(bundle.common,std::nullopt,teams,players,setup,rules,identity);});
    }else{
        saves::CivisSaveSection extra{static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_research_profile),0,{1}};
        rejects([&]{runtime::restore_research_checkpoint(bundle.common,extra,teams,players,setup,rules,identity);});
    }
    check(original_teams[0].progress_times100(7)==big+10&&
        original_players.players.at(big).queue.size()==2,
        "Checkpoint construction changed the input world");

    saves::CivisSaveDocument doc;doc.flags=1;
    doc.sections={{static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload),0,{1,2,3}},
                  saves::make_save_metadata_section({std::string(64,'d'),{}}),bundle.common};
    if(bundle.profile)doc.sections.push_back(*bundle.profile);
    const auto path=std::filesystem::temp_directory_path()/
        ("civis-research-checkpoint-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    saves::write_civis_save(path,doc);
    const auto loaded=saves::read_loaded_save(path);
    const auto *profile_bytes=saves::profile_section(
        loaded.profile_sections,saves::CivisSaveSectionId::vp_research_profile);
    check(loaded.live_team_research==bundle.common.bytes&&
        (bundle.profile?profile_bytes&&*profile_bytes==bundle.profile->bytes:profile_bytes==nullptr),
        "Physical Civis container lost the research checkpoint sections");
    const auto repacked=saves::make_civis_import_document(loaded,std::string(64,'e'));
    check(repacked.sections.size()==doc.sections.size(),
        "Repacking an imported research checkpoint dropped a section");
    rejects([&]{saves::observe_loaded_session(loaded,{});});
    std::filesystem::remove(path);
}
void gameplay_image_test(){
    civ5::content::RulesDatabase db;
    db.apply_sql("CREATE TABLE ModRules(ID INTEGER PRIMARY KEY,Value TEXT);"
                 "INSERT INTO ModRules VALUES(4294967300,'original');");
    const auto first=db.gameplay_fingerprint_data();
    check(first.size()>=100&&first==db.gameplay_fingerprint_data(),
        "Activated gameplay rules fingerprint changed on a read-only repeat");
    db.apply_sql("UPDATE ModRules SET Value=Value WHERE ID=4294967300;");
    check(first==db.gameplay_fingerprint_data(),
        "Physical SQLite writes changed an identical logical rules fingerprint");
    db.apply_sql("UPDATE ModRules SET Value='changed' WHERE ID=4294967300;");
    check(first!=db.gameplay_fingerprint_data(),
        "Gameplay fingerprint did not bind live mod SQL changes");
    db.apply_sql("CREATE TABLE MinorCivilizations(ID INTEGER PRIMARY KEY,Type TEXT UNIQUE,Trait TEXT);"
                 "INSERT INTO MinorCivilizations VALUES(1,'MINOR_A','CULTURED');"
                 "INSERT INTO MinorCivilizations VALUES(2,'MINOR_B','MERCANTILE');");
    const auto profile=civ5::runtime::vp_rules_fingerprint_normalizations();
    const auto stable=db.gameplay_fingerprint_data(profile);
    const auto raw=db.gameplay_fingerprint_data();
    db.apply_sql("UPDATE MinorCivilizations SET ID=3 WHERE Type='MINOR_A';"
                 "UPDATE MinorCivilizations SET ID=1 WHERE Type='MINOR_B';"
                 "UPDATE MinorCivilizations SET ID=2 WHERE Type='MINOR_A';");
    check(stable==db.gameplay_fingerprint_data(profile)&&raw!=db.gameplay_fingerprint_data(),
        "VP generated city-state IDs changed the stable rules fingerprint");
    db.apply_sql("UPDATE MinorCivilizations SET Trait='MILITARISTIC' WHERE Type='MINOR_A';");
    check(stable!=db.gameplay_fingerprint_data(profile),
        "VP fingerprint ignored a changed city-state rule");
}
} // namespace
int main(){
    try{
        gameplay_image_test();
        round_trip(civ5::content::TechnologyProfile::bnw);
        round_trip(civ5::content::TechnologyProfile::vp542);
        std::cout<<"PASS "<<checks<<" research checkpoint checks; runtime_bits="<<sizeof(void*)*8<<'\n';
        return 0;
    }catch(const std::exception &error){
        std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;
    }
}
