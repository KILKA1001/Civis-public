#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/saved_terrain.hpp"
#include <algorithm>
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
    check(failed,"Invalid BNW RNG export was accepted");
}
saves::SavedSessionDecodeContext context(
    const saves::SavedTerrainFeatureResolver &features,bool overlays=false) {
    saves::SavedSessionDecodeContext result;
    result.decode_map_regions=true;result.decode_teams=true;
    result.decode_all_players=true;result.decode_supporting_classes=true;
    result.all_players=saves::AllPlayersContext{};
    result.terrain_features=features;
    result.permit_live_bnw_map_plots_overlay=overlays;
    result.permit_live_bnw_player_strategy_overlay=overlays;
    result.permit_live_bnw_player_core_overlay=overlays;
    result.permit_live_bnw_city_core_overlay=overlays;
    result.permit_live_bnw_city_production_overlay=overlays;
    result.permit_live_bnw_city_management_overlay=overlays;
    result.permit_live_bnw_city_tail_overlay=overlays;
    result.permit_live_random_overlay=overlays;
    return result;
}
runtime::LiveSessionState state_from_world(saves::SavedSessionSnapshot world) {
    auto random=runtime::restore_session_random(world.progression);
    return {std::move(world),{},{},{},{},{},{},{},{},{},
            std::move(random),{},{},{},{},{},{},{},{},{},{},{}};
}
std::int64_t &player_core(saves::SavedSessionSnapshot &world,
                          std::size_t player,std::string_view wanted) {
    for(auto &part:world.all_players->players.at(player).core.fields)
        if(part.name && std::string_view(part.name)==wanted)
            return part.values.at(0);
    throw std::runtime_error("Selected BNW player core field missing");
}
std::int64_t &strategy(saves::SavedSessionSnapshot &world,
                       std::string_view wanted) {
    for(auto &part:world.all_players->players.at(0).strategy.blocks)
        if(part.name && std::string_view(part.name)=="economic_ai")
            for(auto &field:part.fields)
                if(field.name && std::string_view(field.name)==wanted)
                    return field.value;
    throw std::runtime_error("Selected BNW strategy field missing");
}
std::int64_t &plot_field(saves::SavedSessionSnapshot &world,std::size_t index,
                         std::string_view wanted) {
    for(auto &field:world.map_plots->plots.at(index).fields)
        if(field.name && std::string_view(field.name)==wanted) return field.value;
    throw std::runtime_error("Selected BNW plot field missing");
}
struct Selection {std::size_t player=0,city=0;};
Selection choose_city(const saves::SavedSessionSnapshot &world) {
    for(std::size_t p=0;p<world.all_players->players.size();++p)
        if(!world.all_players->players[p].cities.cities.empty()) return {p,0};
    throw std::runtime_error("Selected Gandhi save has no founded city");
}
saves::SavedCityObservation &city(saves::SavedSessionSnapshot &world,Selection s) {
    return world.all_players->players.at(s.player).cities.cities.at(s.city);
}
std::int64_t &city_core(saves::SavedCityObservation &city,std::string_view wanted) {
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
    throw std::runtime_error("Selected BNW production block missing");
}
saves::CityManagementBlock &management(saves::SavedCityObservation &city) {
    for(auto &block:city.management.blocks)
        if(block.name && std::string_view(block.name)=="citizens") return block;
    throw std::runtime_error("Selected BNW management block missing");
}
std::int64_t &tail_dirty(saves::SavedCityObservation &city) {
    for(auto &block:city.tail.blocks)
        if(block.name && std::string_view(block.name)=="city_ai")
            for(auto &node:block.children)
                if(node.name && std::string_view(node.name)==
                   "m_bChooseProductionDirty") return node.values.at(0);
    throw std::runtime_error("Selected BNW city AI flag missing");
}
const saves::CivisSaveSection &section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &part:document.sections)
        if(part.id==id) {
            check(!found,"Duplicate BNW section");found=&part;
        }
    check(found,"Missing BNW section");return *found;
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
runtime::LiveSessionState restore_all(
    const saves::SavedSession &saved,
    const runtime::SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    check(saved.source.live_bnw_map_plots &&
          saved.source.live_bnw_player_core &&
          saved.source.live_bnw_player_strategy &&
          saved.source.live_simulation_random,
          "BNW map/player/RNG sections missing after import");
    auto core=saved.snapshot;
    runtime::restore_bnw_player_core_checkpoint(
        {runtime::bnw_player_core_checkpoint_section_id,0,
         *saved.source.live_bnw_player_core},core,saved.snapshot,identity);
    auto world=saved.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(
        {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_map_plots),
         0,*saved.source.live_bnw_map_plots},world,identity,features,
        &rules,&types,&defines);
    for(std::size_t p=0;p<world.all_players->players.size();++p) {
        check(player_core(world,p,"m_iTotalLandScored")==
              player_core(core,p,"m_iTotalLandScored"),
              "BNW map/player score disagreement after reload");
        world.all_players->players[p].core=
            std::move(core.all_players->players[p].core);
    }
    runtime::restore_bnw_player_strategy_checkpoint(
        {runtime::bnw_player_strategy_checkpoint_section_id,0,
         *saved.source.live_bnw_player_strategy},world,saved.snapshot,identity);
    auto cities=runtime::restore_bnw_city_checkpoint_quadruple(
        city_sections(saved),saved.snapshot,identity);
    for(std::size_t p=0;p<world.all_players->players.size();++p)
        world.all_players->players[p].cities=
            std::move(cities.all_players->players[p].cities);
    auto live=state_from_world(std::move(world));
    runtime::restore_simulation_random_checkpoint(
        {static_cast<std::uint32_t>(
             saves::CivisSaveSectionId::live_simulation_random),0,
         *saved.source.live_simulation_random},live.random,identity);
    return live;
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    content::RulesDatabase rules;
    for(const auto &relative:{
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Terrains.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Features.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Improvements.xml",
        "Assets/Gameplay/XML/Misc/CIV5Routes.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/GlobalDefines.xml"})
        rules.apply_xml(content::read_xml(game_root/relative));
    const auto types=rules.cache_info_types();
    const auto defines=rules.cache_global_defines(
        content::GlobalDefineProfile::bnw);
    const auto *threshold=defines.find("OWNERSHIP_SCORE_DURATION_THRESHOLD");
    check(types.report().all_succeeded() && threshold &&
          threshold->lookup.found &&
          std::get<std::int64_t>(threshold->lookup.value)==20,
          "Selected BNW map score rules unavailable");
    const auto features=content::saved_terrain_features(rules,types).resolve;
    const auto source_hash=runtime::identify_file(path).sha256;
    const auto imported=saves::read_saved_session(path,context(features));
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        runtime::identify_bytes(rules.gameplay_fingerprint_data()).sha256};
    auto state=state_from_world(imported.snapshot);
    std::size_t plot=state.world.map_plots->plots.size();
    for(std::size_t i=0;i<plot;++i) {
        const auto &cell=state.world.map_projection->cells[i];
        const auto duration=plot_field(state.world,i,"ownership_duration");
        if(cell.owner==0 && cell.plot_type && *cell.plot_type!=3 &&
           duration>=0 && duration<20 && cell.units && cell.units->empty()) {
            plot=i;break;
        }
    }
    check(plot<state.world.map_plots->plots.size(),
          "Selected Gandhi save has no score-eligible plot");
    plot_field(state.world,plot,"ownership_duration")=20;
    ++player_core(state.world,0,"m_iTotalLandScored");
    auto &explore=strategy(state.world,"exploration_plots_dirty");
    explore=1-explore;
    const auto selected_city=choose_city(state.world);
    auto &target=city(state.world,selected_city);
    ++city_core(target,"m_iFood");
    ++field(production(target),"m_iBuildingProductionModifier");
    auto &automated=field(management(target),"m_bAutomated");
    automated=1-automated;
    auto &dirty=tail_dirty(target);dirty=1-dirty;
    const auto first_food=city_core(target,"m_iFood");
    const auto first_explore=explore;
    (void)state.random.map.draw(37);
    (void)state.random.gameplay.draw(1000);
    const auto map_rng=state.random.map.state();
    const auto gameplay_rng=state.random.gameplay.state();
    auto first_export=runtime::make_bnw_live_rng_partial_export(
        state,imported,identity,source_hash,features,rules,types,defines);
    check(!first_export.full_persistence &&
          first_export.uncovered_domains.size()>=15,
          "BNW RNG export concealed incomplete persistence");
    constexpr auto rng_id=static_cast<std::uint32_t>(
        saves::CivisSaveSectionId::live_simulation_random);
    check(section(first_export.document,rng_id).flags==0,
          "BNW RNG export section flags changed");
    {
        auto overlap=imported;
        overlap.source.live_team_research=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_rng_partial_export(
            state,overlap,identity,source_hash,features,rules,types,defines);});
        overlap=imported;
        overlap.source.live_treasury=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_rng_partial_export(
            state,overlap,identity,source_hash,features,rules,types,defines);});
        ++state.world.progression.random[0].calls;
        rejects([&]{(void)runtime::make_bnw_live_rng_partial_export(
            state,imported,identity,source_hash,features,rules,types,defines);});
        --state.world.progression.random[0].calls;
        auto bad=state.random.gameplay;
        state.random.gameplay.reseed(1);
        rejects([&]{(void)runtime::make_bnw_live_rng_partial_export(
            state,imported,identity,source_hash,features,rules,types,defines);});
        state.random.gameplay=bad;
        const auto before=bad.state();
        state.random.gameplay=runtime::SimulationRandom(
            runtime::RandomAlgorithm::bnw,
            {before.seed^1u,before.calls,before.resets});
        rejects([&]{(void)runtime::make_bnw_live_rng_partial_export(
            state,imported,identity,source_hash,features,rules,types,defines);});
        state.random.gameplay=bad;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first=std::filesystem::current_path()/
        ("bnw-rng-export-"+suffix+"-1.CivisSave");
    const auto second=std::filesystem::current_path()/
        ("bnw-rng-export-"+suffix+"-2.CivisSave");
    struct Cleanup {std::filesystem::path first,second;~Cleanup(){
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }} cleanup{first,second};
    saves::write_civis_save(first,first_export.document);
    first_export.document.sections.clear();state.world={};
    {
        const auto reopened=saves::read_saved_session(first,context(features,true));
        check(reopened.source.source_sha256==source_hash &&
              reopened.source.bytes==imported.source.bytes,
              "BNW RNG save lost source identity or payload");
        auto resumed=restore_all(reopened,identity,features,rules,types,defines);
        check(resumed.random.map.state()==map_rng &&
              resumed.random.gameplay.state()==gameplay_rng &&
              resumed.world.progression.random[0].seed==
                  imported.snapshot.progression.random[0].seed &&
              plot_field(resumed.world,plot,"ownership_duration")==20 &&
              strategy(resumed.world,"exploration_plots_dirty")==first_explore,
              "BNW RNG/world sections did not survive write/reload");
        (void)resumed.random.gameplay.draw(17);
        ++city_core(city(resumed.world,selected_city),"m_iFood");
        auto second_export=runtime::make_bnw_live_rng_partial_export(
            resumed,reopened,identity,source_hash,features,rules,types,defines);
        saves::write_civis_save(second,second_export.document);
    }
    {
        const auto reopened=saves::read_saved_session(second,context(features,true));
        const auto document=saves::read_civis_save(second);
        (void)section(document,rng_id);
        auto resumed=restore_all(reopened,identity,features,rules,types,defines);
        check(resumed.random.map.state()==map_rng &&
              resumed.random.gameplay.state().calls==gameplay_rng.calls+1 &&
              city_core(city(resumed.world,selected_city),"m_iFood")==
                  first_food+1,
              "BNW second save did not replace RNG/world sections");
    }
    check(runtime::identify_file(path).sha256==source_hash,
          "BNW RNG export altered original Civ5Save");
    std::cout << "PASS BNW draw-only RNG partial export roundtrip\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and BNW game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW RNG export: " << error.what() << '\n';
        return 1;
    }
}
