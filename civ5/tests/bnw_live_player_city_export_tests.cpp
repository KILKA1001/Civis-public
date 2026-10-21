#include "civ5/runtime/bnw_live_player_city_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
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
    check(failed,"Invalid BNW player-city export was accepted");
}
constexpr std::array<std::uint32_t,6> ids{
    runtime::bnw_player_strategy_checkpoint_section_id,
    runtime::bnw_player_core_checkpoint_section_id,
    runtime::bnw_city_core_checkpoint_section_id,
    runtime::bnw_city_production_checkpoint_section_id,
    runtime::bnw_city_management_checkpoint_section_id,
    runtime::bnw_city_tail_checkpoint_section_id};
saves::SavedSessionDecodeContext context(bool overlays=false) {
    saves::SavedSessionDecodeContext result;
    result.decode_map_regions=true;result.decode_teams=true;
    result.decode_all_players=true;result.decode_supporting_classes=true;
    result.all_players=saves::AllPlayersContext{};
    result.permit_live_bnw_player_strategy_overlay=overlays;
    result.permit_live_bnw_player_core_overlay=overlays;
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
std::int64_t &player_core(saves::SavedSessionSnapshot &world,
                          std::string_view wanted) {
    for(auto &part:world.all_players->players.at(0).core.fields)
        if(part.name && std::string_view(part.name)==wanted)
            return part.values.at(0);
    throw std::runtime_error("Selected BNW player core field missing");
}
std::int64_t &player_strategy(saves::SavedSessionSnapshot &world,
                              std::string_view block,std::string_view wanted) {
    for(auto &part:world.all_players->players.at(0).strategy.blocks)
        if(part.name && std::string_view(part.name)==block)
            for(auto &field:part.fields)
                if(field.name && std::string_view(field.name)==wanted)
                    return field.value;
    throw std::runtime_error("Selected BNW player strategy field missing");
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
std::int64_t &city_core(saves::SavedCityObservation &city,
                        std::string_view wanted) {
    for(auto &item:city.core.fields)
        if(item.name && std::string_view(item.name)==wanted &&
           item.values.size()==1) return item.values[0];
    throw std::runtime_error("Selected BNW city core field missing");
}
template<class Block> std::int64_t &field(Block &block,std::string_view wanted) {
    for(auto &item:block.fields)
        if(item.name && std::string_view(item.name)==wanted) return item.value;
    throw std::runtime_error("Selected BNW city subsystem field missing");
}
saves::CityProductionRecord &production(saves::SavedCityObservation &city) {
    for(auto &block:city.production.blocks)
        if(block.name && std::string_view(block.name)=="buildings") return block;
    throw std::runtime_error("Selected BNW city production block missing");
}
saves::CityManagementBlock &management(saves::SavedCityObservation &city) {
    for(auto &block:city.management.blocks)
        if(block.name && std::string_view(block.name)=="citizens") return block;
    throw std::runtime_error("Selected BNW city management block missing");
}
std::int64_t &tail_dirty(saves::SavedCityObservation &city) {
    for(auto &block:city.tail.blocks)
        if(block.name && std::string_view(block.name)=="city_ai")
            for(auto &node:block.children)
                if(node.name && std::string_view(node.name)==
                   "m_bChooseProductionDirty") return node.values.at(0);
    throw std::runtime_error("Selected BNW city AI dirty flag missing");
}
const saves::CivisSaveSection &section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &part:document.sections)
        if(part.id==id) {
            check(!found,"Duplicate composed BNW section");found=&part;
        }
    check(found,"Missing composed BNW section");return *found;
}
runtime::BnwCityCheckpointQuadruple city_sections(
    const saves::SavedSession &saved) {
    check(saved.source.live_bnw_city_core &&
          saved.source.live_bnw_city_production &&
          saved.source.live_bnw_city_management &&
          saved.source.live_bnw_city_tail,
          "BNW city sections missing after import");
    return {{runtime::bnw_city_core_checkpoint_section_id,0,
             *saved.source.live_bnw_city_core},
            {runtime::bnw_city_production_checkpoint_section_id,0,
             *saved.source.live_bnw_city_production},
            {runtime::bnw_city_management_checkpoint_section_id,0,
             *saved.source.live_bnw_city_management},
            {runtime::bnw_city_tail_checkpoint_section_id,0,
             *saved.source.live_bnw_city_tail}};
}
saves::SavedSessionSnapshot restore_all(
    const saves::SavedSession &saved,
    const runtime::SessionCheckpointIdentity &identity) {
    check(saved.source.live_bnw_player_core &&
          saved.source.live_bnw_player_strategy,
          "BNW player sections missing after import");
    // Every restore uses the same immutable source. The city quadruple first
    // builds a detached world; ID24 and ID23 then apply to disjoint player
    // trees. Nothing is published if a later section rejects.
    auto world=runtime::restore_bnw_city_checkpoint_quadruple(
        city_sections(saved),saved.snapshot,identity);
    runtime::restore_bnw_player_core_checkpoint(
        {runtime::bnw_player_core_checkpoint_section_id,0,
         *saved.source.live_bnw_player_core},
        world,saved.snapshot,identity);
    runtime::restore_bnw_player_strategy_checkpoint(
        {runtime::bnw_player_strategy_checkpoint_section_id,0,
         *saved.source.live_bnw_player_strategy},
        world,saved.snapshot,identity);
    return world;
}
void selected(const std::filesystem::path &path) {
    const auto source_hash=runtime::identify_file(path).sha256;
    const auto imported=saves::read_saved_session(path,context());
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto state=state_from_world(imported.snapshot);
    const auto selected=choose(state.world);
    ++player_core(state.world,"m_iTotalLandScored");
    auto &explore=player_strategy(state.world,"economic_ai",
                                  "exploration_plots_dirty");
    explore=1-explore;
    auto &target=city(state.world,selected);
    ++city_core(target,"m_iFood");
    ++field(production(target),"m_iBuildingProductionModifier");
    auto &automated=field(management(target),"m_bAutomated");
    automated=1-automated;
    auto &dirty=tail_dirty(target);dirty=1-dirty;
    const auto score=player_core(state.world,"m_iTotalLandScored");
    const auto flag=explore,food=city_core(target,"m_iFood");
    const auto modifier=field(production(target),
                              "m_iBuildingProductionModifier");
    const auto managed=automated,tail=dirty;
    auto first_export=runtime::make_bnw_live_player_city_partial_export(
        state,imported,identity,source_hash);
    check(!first_export.full_persistence &&
          first_export.uncovered_domains.size()>=15,
          "BNW player-city export concealed incomplete persistence");
    for(const auto id:ids)
        check(section(first_export.document,id).flags==0,
              "BNW player-city section flags changed");
    {
        auto overlap=imported;
        overlap.source.live_city_production=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_player_city_partial_export(
            state,overlap,identity,source_hash);});
        overlap=imported;
        overlap.source.live_bnw_player_proximity=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_player_city_partial_export(
            state,overlap,identity,source_hash);});
        overlap=imported;
        overlap.source.live_bnw_exploration_dirty=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_player_city_partial_export(
            state,overlap,identity,source_hash);});
        auto &count=state.world.all_players->players[selected.player]
                        .cities.header.wire_city_count;
        ++count;
        rejects([&]{(void)runtime::make_bnw_live_player_city_partial_export(
            state,imported,identity,source_hash);});
        --count;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first=std::filesystem::current_path()/
        ("bnw-player-city-"+suffix+"-1.CivisSave");
    const auto second=std::filesystem::current_path()/
        ("bnw-player-city-"+suffix+"-2.CivisSave");
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
              "BNW player-city save lost source identity or payload");
        auto restored=restore_all(reopened,identity);
        auto &restored_city=city(restored,selected);
        check(player_core(restored,"m_iTotalLandScored")==score &&
              player_strategy(restored,"economic_ai",
                              "exploration_plots_dirty")==flag &&
              city_core(restored_city,"m_iFood")==food &&
              field(production(restored_city),
                    "m_iBuildingProductionModifier")==modifier &&
              field(management(restored_city),"m_bAutomated")==managed &&
              tail_dirty(restored_city)==tail,
              "BNW six player-city domains did not survive write/reload");
        auto resumed=state_from_world(std::move(restored));
        ++player_core(resumed.world,"m_iTotalLandScored");
        ++city_core(city(resumed.world,selected),"m_iFood");
        auto second_export=runtime::make_bnw_live_player_city_partial_export(
            resumed,reopened,identity,source_hash);
        saves::write_civis_save(second,second_export.document);
    }
    {
        const auto reopened=saves::read_saved_session(second,context(true));
        const auto document=saves::read_civis_save(second);
        for(const auto id:ids) (void)section(document,id);
        auto restored=restore_all(reopened,identity);
        auto &restored_city=city(restored,selected);
        check(player_core(restored,"m_iTotalLandScored")==score+1 &&
              player_strategy(restored,"economic_ai",
                              "exploration_plots_dirty")==flag &&
              city_core(restored_city,"m_iFood")==food+1 &&
              field(production(restored_city),
                    "m_iBuildingProductionModifier")==modifier &&
              field(management(restored_city),"m_bAutomated")==managed &&
              tail_dirty(restored_city)==tail,
              "BNW re-save did not replace all player-city sections");
    }
    check(runtime::identify_file(path).sha256==source_hash,
          "BNW player-city export altered original Civ5Save");
    std::cout << "PASS BNW composed player-city partial export roundtrip\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW composed player-city export: "
                  << error.what() << '\n';
        return 1;
    }
}
