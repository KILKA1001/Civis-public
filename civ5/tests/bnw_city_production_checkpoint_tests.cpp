#include "civ5/runtime/bnw_city_production_checkpoint.hpp"
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
    check(rejected,"Invalid BNW city production checkpoint was accepted");
}
struct Selection {std::size_t player=0,city=0;};
Selection choose(const saves::SavedSessionSnapshot &source) {
    for(std::size_t p=0;p<source.all_players->players.size();++p)
        if(!source.all_players->players[p].cities.cities.empty()) return {p,0};
    throw std::runtime_error("Selected Gandhi save has no founded city");
}
saves::CityProductionRecord &block(saves::SavedCityObservation &city,
                                    std::string_view wanted) {
    for(auto &part:city.production.blocks)
        if(part.name && std::string_view(part.name)==wanted) return part;
    throw std::runtime_error("Selected BNW production block missing");
}
std::int64_t &field(saves::CityProductionRecord &record,std::string_view wanted) {
    for(auto &part:record.fields)
        if(part.name && std::string_view(part.name)==wanted) return part.value;
    throw std::runtime_error("Selected BNW production field missing");
}
std::int64_t &field(saves::SavedCityObservation &city,std::string_view wanted) {
    for(auto &part:city.core.fields)
        if(part.name && std::string_view(part.name)==wanted &&
           part.values.size()==1) return part.values[0];
    throw std::runtime_error("Selected BNW city core field missing");
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
    runtime::BnwCityCoreProductionCheckpointPair pair;
    std::int64_t food_after=0,building_modifier_after=0;
    {
        auto world=source;
        auto &city=world.all_players->players[chosen.player].cities.cities[chosen.city];
        auto &food=field(city,"m_iFood");
        auto &building_modifier=field(block(city,"buildings"),
                                      "m_iBuildingProductionModifier");
        ++food;++building_modifier;
        food_after=food;building_modifier_after=building_modifier;
        pair=runtime::make_bnw_city_core_production_checkpoint_pair(
            world,source,identity);
        check(pair.core.id==runtime::bnw_city_core_checkpoint_section_id &&
              pair.production.id==runtime::bnw_city_production_checkpoint_section_id,
              "BNW city core/production pair IDs changed");
        rejects([&]{(void)runtime::make_bnw_city_core_checkpoint(
            world,source,identity);});
        auto wrong=world;
        block(wrong.all_players->players[chosen.player].cities.cities[chosen.city],
              "queue").children.push_back({});
        rejects([&]{(void)runtime::make_bnw_city_core_production_checkpoint_pair(
            wrong,source,identity);});
        wrong=world;
        auto &management=wrong.all_players->players[chosen.player].cities
            .cities[chosen.city].management;
        check(!management.blocks.empty(),
              "Selected BNW city has no management companion block");
        management.blocks.pop_back();
        rejects([&]{(void)runtime::make_bnw_city_core_production_checkpoint_pair(
            wrong,source,identity);});
    }
    const auto output=std::filesystem::current_path()/(
        "bnw-city-production-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    {
        auto document=saves::make_civis_import_document(imported.source,original.sha256);
        document.sections.push_back(pair.core);
        document.sections.push_back(pair.production);
        saves::write_civis_save(output,document);
    }
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code ignored;std::filesystem::remove(path,ignored);
    }} cleanup{output};
    {
        const auto reopened=saves::read_civis_save(output);
        runtime::BnwCityCoreProductionCheckpointPair read_pair;
        for(const auto &section:reopened.sections) {
            if(section.id==pair.core.id) read_pair.core=section;
            if(section.id==pair.production.id) read_pair.production=section;
        }
        check(read_pair.core.bytes==pair.core.bytes &&
              read_pair.production.bytes==pair.production.bytes,
              "Atomic CivisSave read/write lost BNW city checkpoint pair");
        auto restored=runtime::restore_bnw_city_core_production_checkpoint_pair(
            read_pair,source,identity);
        auto &city=restored.all_players->players[chosen.player]
            .cities.cities[chosen.city];
        check(field(city,"m_iFood")==food_after &&
              field(block(city,"buildings"),"m_iBuildingProductionModifier")==
                  building_modifier_after,
              "BNW city core/production pair did not round-trip");
        auto missing=read_pair;missing.production={};
        rejects([&]{(void)runtime::restore_bnw_city_core_production_checkpoint_pair(
            missing,source,identity);});
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW production checkpoint changed original Civ5Save");
    auto destination=source;
    auto bad=pair.production;bad.bytes.pop_back();
    rejects([&]{runtime::restore_bnw_city_production_checkpoint(
        bad,destination,source,identity);});
    auto &city=destination.all_players->players[chosen.player].cities.cities[chosen.city];
    auto &old=source.all_players->players[chosen.player].cities.cities[chosen.city];
    check(field(block(city,"buildings"),"m_iBuildingProductionModifier")==
          field(block(old,"buildings"),"m_iBuildingProductionModifier"),
          "Truncated BNW production section partially changed destination");
    bad=pair.production;bad.bytes[8]=2;
    rejects([&]{runtime::restore_bnw_city_production_checkpoint(
        bad,destination,source,identity);});
    auto foreign=identity;foreign.rules_sha256[0]='b';
    rejects([&]{runtime::restore_bnw_city_production_checkpoint(
        pair.production,destination,source,foreign);});
    std::cout << "PASS BNW CityProduction core pair checkpoint\n";
}
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW production checkpoint: " << error.what() << '\n';
        return 1;
    }
}
