#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW city management checkpoint was accepted");
}
struct Selection {std::size_t player=0,city=0;};
Selection choose(const saves::SavedSessionSnapshot &source) {
    for(std::size_t p=0;p<source.all_players->players.size();++p)
        if(!source.all_players->players[p].cities.cities.empty()) return {p,0};
    throw std::runtime_error("Selected Gandhi save has no founded city");
}
std::int64_t &city_field(saves::SavedCityObservation &city,
                         std::string_view wanted) {
    for(auto &item:city.core.fields)
        if(item.name && std::string_view(item.name)==wanted &&
           item.values.size()==1) return item.values[0];
    throw std::runtime_error("Selected BNW city core field missing");
}
saves::CityProductionRecord &production(saves::SavedCityObservation &city,
                                         std::string_view wanted) {
    for(auto &item:city.production.blocks)
        if(item.name && std::string_view(item.name)==wanted) return item;
    throw std::runtime_error("Selected BNW production block missing");
}
saves::CityManagementBlock &management(saves::SavedCityObservation &city,
                                        std::string_view wanted) {
    for(auto &item:city.management.blocks)
        if(item.name && std::string_view(item.name)==wanted) return item;
    throw std::runtime_error("Selected BNW management block missing");
}
template<class Block> std::int64_t &field(Block &block,std::string_view wanted) {
    for(auto &item:block.fields)
        if(item.name && std::string_view(item.name)==wanted) return item.value;
    throw std::runtime_error("Selected BNW city subsystem field missing");
}
void selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.all_players &&
          imported.snapshot.all_players->players.size()==64,
          "Selected Gandhi save has no complete city roster");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto source=std::move(imported.snapshot);
    const auto chosen=choose(source);
    runtime::BnwCityCoreProductionManagementCheckpointTriple triple;
    std::int64_t food_after=0,production_after=0,automated_after=0;
    {
        auto world=source;
        auto &city=world.all_players->players[chosen.player].cities.cities[chosen.city];
        auto &food=city_field(city,"m_iFood");
        auto &modifier=field(production(city,"buildings"),
                             "m_iBuildingProductionModifier");
        auto &automated=field(management(city,"citizens"),"m_bAutomated");
        ++food;++modifier;automated=1-automated;
        food_after=food;production_after=modifier;automated_after=automated;
        triple=runtime::make_bnw_city_core_production_management_checkpoint_triple(
            world,source,identity);
        check(triple.core.id==runtime::bnw_city_core_checkpoint_section_id &&
              triple.production.id==runtime::bnw_city_production_checkpoint_section_id &&
              triple.management.id==runtime::bnw_city_management_checkpoint_section_id,
              "BNW city checkpoint triple section IDs changed");
        rejects([&]{(void)runtime::make_bnw_city_core_checkpoint(
            world,source,identity);});
        rejects([&]{(void)runtime::make_bnw_city_production_checkpoint(
            world,source,identity);});
        auto wrong=world;
        auto &citizens=management(wrong.all_players->players[chosen.player]
            .cities.cities[chosen.city],"citizens");
        check(!citizens.arrays.empty(),
              "Selected BNW citizens have no fixed plot array");
        citizens.arrays.front().values.push_back(1);
        rejects([&]{(void)runtime::make_bnw_city_core_production_management_checkpoint_triple(
            wrong,source,identity);});
        wrong=world;
        check(!wrong.all_players->players[chosen.player].cities.cities[chosen.city]
                   .tail.blocks.empty(),
              "Selected BNW city has no tail block for companion guard");
        wrong.all_players->players[chosen.player].cities.cities[chosen.city]
            .tail.blocks.pop_back();
        rejects([&]{(void)runtime::make_bnw_city_core_production_management_checkpoint_triple(
            wrong,source,identity);});
    }
    const auto output=std::filesystem::current_path()/(
        "bnw-city-management-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    {
        auto document=saves::make_civis_import_document(imported.source,original.sha256);
        document.sections.push_back(triple.core);
        document.sections.push_back(triple.production);
        document.sections.push_back(triple.management);
        saves::write_civis_save(output,document);
    }
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code ignored;std::filesystem::remove(path,ignored);
    }} cleanup{output};
    {
        const auto reopened=saves::read_civis_save(output);
        runtime::BnwCityCoreProductionManagementCheckpointTriple read;
        for(const auto &section:reopened.sections) {
            if(section.id==triple.core.id) read.core=section;
            if(section.id==triple.production.id) read.production=section;
            if(section.id==triple.management.id) read.management=section;
        }
        check(read.core.bytes==triple.core.bytes &&
              read.production.bytes==triple.production.bytes &&
              read.management.bytes==triple.management.bytes,
              "Atomic CivisSave write/read lost city checkpoint triple");
        auto restored=runtime::restore_bnw_city_core_production_management_checkpoint_triple(
            read,source,identity);
        auto &city=restored.all_players->players[chosen.player]
            .cities.cities[chosen.city];
        check(city_field(city,"m_iFood")==food_after &&
              field(production(city,"buildings"),
                    "m_iBuildingProductionModifier")==production_after &&
              field(management(city,"citizens"),"m_bAutomated")==automated_after,
              "BNW city core/production/management values did not round-trip");
        auto missing=read;missing.management={};
        rejects([&]{(void)runtime::restore_bnw_city_core_production_management_checkpoint_triple(
            missing,source,identity);});
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW city management checkpoint changed source Civ5Save");
    auto destination=source;
    auto bad=triple.management;bad.bytes.pop_back();
    rejects([&]{runtime::restore_bnw_city_management_checkpoint(
        bad,destination,source,identity);});
    auto &city=destination.all_players->players[chosen.player].cities.cities[chosen.city];
    auto &old=source.all_players->players[chosen.player].cities.cities[chosen.city];
    check(field(management(city,"citizens"),"m_bAutomated")==
          field(management(old,"citizens"),"m_bAutomated"),
          "Late management rejection partially changed destination");
    bad=triple.management;bad.bytes[8]=2;
    rejects([&]{runtime::restore_bnw_city_management_checkpoint(
        bad,destination,source,identity);});
    auto foreign=identity;foreign.rules_sha256[0]='b';
    rejects([&]{runtime::restore_bnw_city_management_checkpoint(
        triple.management,destination,source,foreign);});
    std::cout << "PASS BNW CityManagement checkpoint triple\n";
}
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW management checkpoint: " << error.what() << '\n';
        return 1;
    }
}
