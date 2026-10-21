#include "civ5/runtime/bnw_live_city_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool failed=false;
    try {f();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW city export was accepted");
}
saves::SavedSessionDecodeContext context(bool overlays=false) {
    saves::SavedSessionDecodeContext result;
    result.decode_map_regions=true;result.decode_teams=true;
    result.decode_all_players=true;result.decode_supporting_classes=true;
    result.all_players=saves::AllPlayersContext{};
    result.permit_live_bnw_city_core_overlay=overlays;
    result.permit_live_bnw_city_production_overlay=overlays;
    result.permit_live_bnw_city_management_overlay=overlays;
    result.permit_live_bnw_city_tail_overlay=overlays;
    return result;
}
runtime::LiveSessionState state_from_world(saves::SavedSessionSnapshot world) {
    auto random=runtime::restore_session_random(world.progression);
    return {std::move(world),{},{},{},{},{},{},{},{},{},
            std::move(random),{},{},{},{},{},{},{},{},{},{},{}};
}
struct Selection {std::size_t player=0,city=0;};
Selection choose(const saves::SavedSessionSnapshot &world) {
    for(std::size_t p=0;p<world.all_players->players.size();++p)
        if(!world.all_players->players[p].cities.cities.empty()) return {p,0};
    throw std::runtime_error("Selected Gandhi save has no founded city");
}
saves::SavedCityObservation &city(saves::SavedSessionSnapshot &world,Selection s) {
    return world.all_players->players.at(s.player).cities.cities.at(s.city);
}
std::int64_t &core(saves::SavedCityObservation &city,std::string_view wanted) {
    for(auto &field:city.core.fields)
        if(field.name && std::string_view(field.name)==wanted &&
           field.values.size()==1) return field.values[0];
    throw std::runtime_error("Selected BNW city core field missing");
}
template<class Block> std::int64_t &field(Block &block,std::string_view wanted) {
    for(auto &item:block.fields)
        if(item.name && std::string_view(item.name)==wanted) return item.value;
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
std::int64_t &tail_dirty(saves::SavedCityObservation &city) {
    auto &ai=tail(city,"city_ai");
    for(auto &node:ai.children)
        if(node.name && std::string_view(node.name)=="m_bChooseProductionDirty") {
            check(node.values.size()==1,"Selected BNW city AI flag has no value");
            return node.values[0];
        }
    throw std::runtime_error("Selected BNW city AI dirty flag missing");
}
const saves::CivisSaveSection &section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    const saves::CivisSaveSection *result=nullptr;
    for(const auto &part:document.sections)
        if(part.id==id) {
            check(!result,"Duplicate BNW city section");result=&part;
        }
    check(result,"Missing BNW city section");return *result;
}
runtime::BnwCityCheckpointQuadruple sections(const saves::SavedSession &saved) {
    check(saved.source.live_bnw_city_core &&
          saved.source.live_bnw_city_production &&
          saved.source.live_bnw_city_management &&
          saved.source.live_bnw_city_tail,
          "Composed BNW city sections missing after import");
    return {{runtime::bnw_city_core_checkpoint_section_id,0,
             *saved.source.live_bnw_city_core},
            {runtime::bnw_city_production_checkpoint_section_id,0,
             *saved.source.live_bnw_city_production},
            {runtime::bnw_city_management_checkpoint_section_id,0,
             *saved.source.live_bnw_city_management},
            {runtime::bnw_city_tail_checkpoint_section_id,0,
             *saved.source.live_bnw_city_tail}};
}
void selected(const std::filesystem::path &path) {
    const auto source_hash=runtime::identify_file(path).sha256;
    const auto imported=saves::read_saved_session(path,context());
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto state=state_from_world(imported.snapshot);
    const auto selected=choose(state.world);
    auto &target=city(state.world,selected);
    auto &food=core(target,"m_iFood");
    auto &modifier=field(production(target,"buildings"),
                         "m_iBuildingProductionModifier");
    auto &automated=field(management(target,"citizens"),"m_bAutomated");
    auto &dirty=tail_dirty(target);
    ++food;++modifier;automated=1-automated;dirty=1-dirty;
    const auto first_food=food,first_modifier=modifier;
    const auto first_automated=automated,first_dirty=dirty;
    auto first_export=runtime::make_bnw_live_city_partial_export(
        state,imported,identity,source_hash);
    check(!first_export.full_persistence &&
          first_export.uncovered_domains.size()>=15,
          "BNW city export concealed incomplete world coverage");
    for(const auto id:{runtime::bnw_city_core_checkpoint_section_id,
                       runtime::bnw_city_production_checkpoint_section_id,
                       runtime::bnw_city_management_checkpoint_section_id,
                       runtime::bnw_city_tail_checkpoint_section_id})
        check(section(first_export.document,id).flags==0,
              "BNW city export section flags changed");
    {
        auto overlap=imported;
        overlap.source.live_city_production=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_city_partial_export(
            state,overlap,identity,source_hash);});
        auto &cities=state.world.all_players->players[selected.player].cities;
        ++cities.header.wire_city_count;
        rejects([&]{(void)runtime::make_bnw_live_city_partial_export(
            state,imported,identity,source_hash);});
        --cities.header.wire_city_count;
        const auto saved_id=core(target,"m_iID");
        core(target,"m_iID")=saved_id+1;
        rejects([&]{(void)runtime::make_bnw_live_city_partial_export(
            state,imported,identity,source_hash);});
        core(target,"m_iID")=saved_id;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first=std::filesystem::current_path()/
        ("bnw-live-city-"+suffix+"-1.CivisSave");
    const auto second=std::filesystem::current_path()/
        ("bnw-live-city-"+suffix+"-2.CivisSave");
    struct Cleanup {std::filesystem::path first,second;~Cleanup(){
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }} cleanup{first,second};
    saves::write_civis_save(first,first_export.document);
    first_export.document.sections.clear();state.world={};
    {
        const auto reopened=saves::read_saved_session(first,context(true));
        check(reopened.source.source_sha256==source_hash &&
              reopened.source.bytes==imported.source.bytes,
              "BNW city save lost source identity or payload");
        auto restored=runtime::restore_bnw_city_checkpoint_quadruple(
            sections(reopened),reopened.snapshot,identity);
        auto &value=city(restored,selected);
        check(core(value,"m_iFood")==first_food &&
              field(production(value,"buildings"),
                    "m_iBuildingProductionModifier")==first_modifier &&
              field(management(value,"citizens"),"m_bAutomated")==
                  first_automated && tail_dirty(value)==first_dirty,
              "BNW city quadruple did not survive write/exit/load");
        auto resumed=state_from_world(std::move(restored));
        ++core(city(resumed.world,selected),"m_iFood");
        ++field(production(city(resumed.world,selected),"buildings"),
                "m_iBuildingProductionModifier");
        auto second_export=runtime::make_bnw_live_city_partial_export(
            resumed,reopened,identity,source_hash);
        saves::write_civis_save(second,second_export.document);
    }
    {
        const auto reopened=saves::read_saved_session(second,context(true));
        const auto document=saves::read_civis_save(second);
        for(const auto id:{runtime::bnw_city_core_checkpoint_section_id,
                           runtime::bnw_city_production_checkpoint_section_id,
                           runtime::bnw_city_management_checkpoint_section_id,
                           runtime::bnw_city_tail_checkpoint_section_id})
            (void)section(document,id);
        auto restored=runtime::restore_bnw_city_checkpoint_quadruple(
            sections(reopened),reopened.snapshot,identity);
        auto &value=city(restored,selected);
        check(core(value,"m_iFood")==first_food+1 &&
              field(production(value,"buildings"),
                    "m_iBuildingProductionModifier")==first_modifier+1 &&
              field(management(value,"citizens"),"m_bAutomated")==
                  first_automated && tail_dirty(value)==first_dirty,
              "BNW second city export failed to replace prior ID25-28");
    }
    check(runtime::identify_file(path).sha256==source_hash,
          "BNW city export altered original Civ5Save");
    std::cout << "PASS BNW live city ID25-28 partial export roundtrip\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW live city export: " << error.what() << '\n';
        return 1;
    }
}
