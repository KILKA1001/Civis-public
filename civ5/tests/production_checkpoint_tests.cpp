#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/production_checkpoint.hpp"
#include <iostream>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <type_traits>

using namespace civ5::content;
namespace {
unsigned checks=0;
void check(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F action){
    bool caught=false;try{action();}catch(const std::exception&){caught=true;}
    check(caught,"Expected production checkpoint rejection");
}
}
#include "production_progress_tests.hpp"
void production_checkpoint_file_tests(){
    using namespace civ5;
    content::RulesDatabase db;
    db.apply_sql("CREATE TABLE Units(ID,Type);INSERT INTO Units VALUES(7,'UNIT7');"
                 "CREATE TABLE Buildings(ID,Type);CREATE TABLE Projects(ID,Type);"
                 "CREATE TABLE Processes(ID,Type);CREATE TABLE Specialists(ID,Type);");
    const auto types=db.cache_info_types();
    const auto rules=runtime::load_production_progress_rules(db,content::TechnologyProfile::bnw);
    constexpr std::int64_t big=4294967300LL;
    runtime::CityYieldContext changed;changed.profile=content::TechnologyProfile::bnw;
    auto &city=changed.players[big].cities[big+1];city.owner=big;city.id=big+1;
    city.source_core={{100,200}};city.production_progress.emplace();
    city.production_progress->source_profile=changed.profile;
    city.production_progress->units[7]=big+100;city.production_orders.emplace();
    const auto section=runtime::make_production_checkpoint(changed,rules,types);
    saves::CivisSaveDocument doc;doc.flags=1;
    doc.sections={{static_cast<std::uint32_t>(saves::CivisSaveSectionId::imported_payload),0,{1,2,3}},
                  saves::make_save_metadata_section({std::string(64,'a'),{}}),section};
    const auto path=std::filesystem::temp_directory_path()/
        ("civis-production-checkpoint-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    saves::write_civis_save(path,doc);
    const auto loaded=saves::read_loaded_save(path);
    check(loaded.live_city_production==section.bytes&&loaded.bytes==std::vector<std::uint8_t>({1,2,3}),
        "Physical Civis file retains live production beside imported payload");
    auto fresh=changed;fresh.players.at(big).cities.at(big+1).production_progress->units[7]=0;
    runtime::restore_production_checkpoint(
        {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_city_production),0,*loaded.live_city_production},
        rules,types,fresh);
    check(fresh.players.at(big).cities.at(big+1).production_progress->units.at(7)==big+100,
        "Reopened Civis file applies changed 64-bit production to the live city");
    std::filesystem::remove(path);
}
int main(){
    try{production_progress_tests();production_checkpoint_file_tests();
        std::cout<<"PASS "<<checks<<" production checkpoint checks; runtime_bits="
        <<sizeof(void*)*8<<'\n';return 0;}
    catch(const std::exception &error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}
}
