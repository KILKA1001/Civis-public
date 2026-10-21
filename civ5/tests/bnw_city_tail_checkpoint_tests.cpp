#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
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
    check(rejected,"Invalid BNW city tail checkpoint was accepted");
}
struct Selection {std::size_t player=0,city=0;};
Selection choose(const saves::SavedSessionSnapshot &source) {
    for(std::size_t p=0;p<source.all_players->players.size();++p)
        if(!source.all_players->players[p].cities.cities.empty()) return {p,0};
    throw std::runtime_error("Selected Gandhi save has no founded city");
}
saves::SavedCityObservation &city(saves::SavedSessionSnapshot &world,Selection s) {
    return world.all_players->players[s.player].cities.cities[s.city];
}
std::int64_t &core(saves::SavedCityObservation &city,std::string_view wanted) {
    for(auto &field:city.core.fields)
        if(field.name && std::string_view(field.name)==wanted &&
           field.values.size()==1) return field.values[0];
    throw std::runtime_error("Selected BNW city core field missing");
}
template<class Block> auto &field(Block &block,std::string_view wanted) {
    for(auto &field:block.fields)
        if(field.name && std::string_view(field.name)==wanted) return field.value;
    throw std::runtime_error("Selected BNW city subsystem field missing");
}
saves::CityProductionRecord &production(saves::SavedCityObservation &city,
                                         std::string_view wanted) {
    for(auto &block:city.production.blocks)
        if(block.name && std::string_view(block.name)==wanted) return block;
    throw std::runtime_error("Selected BNW city production block missing");
}
saves::CityManagementBlock &management(saves::SavedCityObservation &city,
                                       std::string_view wanted) {
    for(auto &block:city.management.blocks)
        if(block.name && std::string_view(block.name)==wanted) return block;
    throw std::runtime_error("Selected BNW city management block missing");
}
saves::CityTailNode &tail(saves::SavedCityObservation &city,
                          std::string_view wanted) {
    for(auto &block:city.tail.blocks)
        if(block.name && std::string_view(block.name)==wanted) return block;
    throw std::runtime_error("Selected BNW city tail block missing");
}
saves::CityTailNode &child(saves::CityTailNode &node,std::string_view wanted) {
    for(auto &entry:node.children)
        if(entry.name && std::string_view(entry.name)==wanted) return entry;
    throw std::runtime_error("Selected BNW city tail child missing");
}
std::int64_t &tail_value(saves::SavedCityObservation &city) {
    auto &value=child(tail(city,"city_ai"),"m_bChooseProductionDirty");
    check(value.values.size()==1,"Selected BNW city AI dirty flag absent");
    return value.values[0];
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
    runtime::BnwCityCheckpointQuadruple quadruple;
    std::int64_t food_after=0,production_after=0,automated_after=0,tail_after=0;
    {
        auto world=source;
        auto &target=city(world,chosen);
        auto &food=core(target,"m_iFood");
        auto &modifier=field(production(target,"buildings"),
                             "m_iBuildingProductionModifier");
        auto &automated=field(management(target,"citizens"),"m_bAutomated");
        auto &dirty=tail_value(target);
        ++food;++modifier;automated=1-automated;dirty=1-dirty;
        food_after=food;production_after=modifier;
        automated_after=automated;tail_after=dirty;
        quadruple=runtime::make_bnw_city_checkpoint_quadruple(
            world,source,identity);
        check(quadruple.core.id==runtime::bnw_city_core_checkpoint_section_id &&
              quadruple.production.id==runtime::bnw_city_production_checkpoint_section_id &&
              quadruple.management.id==runtime::bnw_city_management_checkpoint_section_id &&
              quadruple.tail.id==runtime::bnw_city_tail_checkpoint_section_id,
              "BNW city checkpoint quadruple section IDs changed");
        rejects([&]{(void)runtime::make_bnw_city_tail_checkpoint(
            world,source,identity);});
        auto wrong=world;
        auto &ai=tail(city(wrong,chosen),"city_ai");
        check(!ai.children.empty(),"Selected BNW city AI has no decoded children");
        ai.children.pop_back();
        rejects([&]{(void)runtime::make_bnw_city_checkpoint_quadruple(
            wrong,source,identity);});
        wrong=world;
        wrong.all_players->players[chosen.player].cities.header.wire_city_count++;
        rejects([&]{(void)runtime::make_bnw_city_checkpoint_quadruple(
            wrong,source,identity);});
    }
    const auto output=std::filesystem::current_path()/(
        "bnw-city-tail-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code ignored;std::filesystem::remove(path,ignored);
    }} cleanup{output};
    {
        auto document=saves::make_civis_import_document(imported.source,original.sha256);
        document.sections.push_back(quadruple.core);
        document.sections.push_back(quadruple.production);
        document.sections.push_back(quadruple.management);
        document.sections.push_back(quadruple.tail);
        saves::write_civis_save(output,document);
    }
    {
        const auto reopened=saves::read_civis_save(output);
        runtime::BnwCityCheckpointQuadruple read;
        for(const auto &section:reopened.sections) {
            if(section.id==quadruple.core.id) read.core=section;
            if(section.id==quadruple.production.id) read.production=section;
            if(section.id==quadruple.management.id) read.management=section;
            if(section.id==quadruple.tail.id) read.tail=section;
        }
        check(read.core.bytes==quadruple.core.bytes &&
              read.production.bytes==quadruple.production.bytes &&
              read.management.bytes==quadruple.management.bytes &&
              read.tail.bytes==quadruple.tail.bytes,
              "Atomic CivisSave write/read lost city checkpoint quadruple");
        auto restored=runtime::restore_bnw_city_checkpoint_quadruple(
            read,source,identity);
        auto &target=city(restored,chosen);
        check(core(target,"m_iFood")==food_after &&
              field(production(target,"buildings"),
                    "m_iBuildingProductionModifier")==production_after &&
              field(management(target,"citizens"),"m_bAutomated")==automated_after &&
              tail_value(target)==tail_after,
              "BNW city values did not round-trip");
        auto missing=read;missing.tail={};
        rejects([&]{(void)runtime::restore_bnw_city_checkpoint_quadruple(
            missing,source,identity);});
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW city tail checkpoint changed source Civ5Save");
    auto destination=source;
    auto bad=quadruple.tail;bad.bytes.pop_back();
    rejects([&]{runtime::restore_bnw_city_tail_checkpoint(
        bad,destination,source,identity);});
    check(tail_value(city(destination,chosen))==tail_value(city(source,chosen)),
          "Late BNW city tail rejection partially changed destination");
    bad=quadruple.tail;bad.bytes[8]=2;
    rejects([&]{runtime::restore_bnw_city_tail_checkpoint(
        bad,destination,source,identity);});
    auto foreign=identity;foreign.rules_sha256[0]='b';
    rejects([&]{runtime::restore_bnw_city_tail_checkpoint(
        quadruple.tail,destination,source,foreign);});
    std::cout << "PASS BNW CityTail checkpoint quadruple\n";
}
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW city tail checkpoint: " << error.what() << '\n';
        return 1;
    }
}
