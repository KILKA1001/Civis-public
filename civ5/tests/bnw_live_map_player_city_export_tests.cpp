#include "civ5/runtime/bnw_live_map_player_city_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/content/saved_terrain.hpp"
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
    check(failed,"Invalid BNW map/player/city export was accepted");
}
constexpr std::array<std::uint32_t,7> ids{
    static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_map_plots),
    runtime::bnw_player_strategy_checkpoint_section_id,
    runtime::bnw_player_core_checkpoint_section_id,
    runtime::bnw_city_core_checkpoint_section_id,
    runtime::bnw_city_production_checkpoint_section_id,
    runtime::bnw_city_management_checkpoint_section_id,
    runtime::bnw_city_tail_checkpoint_section_id};
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
    result.permit_live_bnw_game_clock_overlay=overlays;
    result.permit_live_random_overlay=overlays;
    return result;
}
runtime::LiveSessionState state_from_world(saves::SavedSessionSnapshot world) {
    auto random=runtime::restore_session_random(world.progression);
    return {.world=std::move(world),.setup={},.technologies={},.player_research={},
            .cities={},.unhappiness={},.maintenance={},.treasury={},.science={},
            .research_turn={},.random=std::move(random),.research_selection={},.game_clock={},
            .player_gold_publication={},.player_research_publication={},.player_early_publication={},
            .nearby_cities={},.city_flavor_recipients={},.city_strategy_runtime={},
            .player_flavor_recipients={},.plot_impassability={},.adjacent_land={},.plot_visibility={},
            .diplomacy_scratchpad={},.notification_load_transients={},.player_plot_lists={},
            .player_read_tail={},.game_plot_extras={},.player_culture_bomb_timers={},
            .military_request_counters={},.city_dirty_events={},.city_dirty_next_occurrence=1,
            .specific_city_info_events={},.ui_source_revision=1,.plot_gameplay_events={},
            .plot_gameplay_next_occurrence=1,.standalone_services={}};
}
std::int64_t &player_core(saves::SavedSessionSnapshot &world,
                          std::size_t player,std::string_view wanted) {
    for(auto &part:world.all_players->players.at(player).core.fields)
        if(part.name && std::string_view(part.name)==wanted)
            return part.values.at(0);
    throw std::runtime_error("Selected BNW player core field missing");
}
std::int64_t &strategy(saves::SavedSessionSnapshot &world,
                       std::string_view block,std::string_view wanted) {
    for(auto &part:world.all_players->players.at(0).strategy.blocks)
        if(part.name && std::string_view(part.name)==block)
            for(auto &field:part.fields)
                if(field.name && std::string_view(field.name)==wanted)
                    return field.value;
    throw std::runtime_error("Selected BNW player strategy field missing");
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
    const runtime::SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    check(saved.source.live_bnw_map_plots &&
          saved.source.live_bnw_player_core &&
          saved.source.live_bnw_player_strategy,
          "BNW map/player sections missing after import");
    // LiveSession restores ID24 on a detached source, ID17 on the main world,
    // checks their land-score overlap, then applies ID23 and city trees.
    auto core= saved.snapshot;
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
              "BNW ID17/ID24 land score disagrees after reload");
        world.all_players->players[p].core=
            std::move(core.all_players->players[p].core);
    }
    runtime::restore_bnw_player_strategy_checkpoint(
        {runtime::bnw_player_strategy_checkpoint_section_id,0,
         *saved.source.live_bnw_player_strategy},
        world,saved.snapshot,identity);
    auto cities=runtime::restore_bnw_city_checkpoint_quadruple(
        city_sections(saved),saved.snapshot,identity);
    for(std::size_t p=0;p<world.all_players->players.size();++p)
        world.all_players->players[p].cities=
            std::move(cities.all_players->players[p].cities);
    return world;
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
          "Selected BNW map score rules are unavailable");
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
    auto &explore=strategy(state.world,"economic_ai","exploration_plots_dirty");
    explore=1-explore;
    const auto selected_city=choose_city(state.world);
    auto &target=city(state.world,selected_city);
    ++city_core(target,"m_iFood");
    ++field(production(target),"m_iBuildingProductionModifier");
    auto &automated=field(management(target),"m_bAutomated");
    automated=1-automated;
    auto &dirty=tail_dirty(target);dirty=1-dirty;
    const auto score=player_core(state.world,0,"m_iTotalLandScored");
    const auto flag=explore,food=city_core(target,"m_iFood");
    const auto modifier=field(production(target),
                              "m_iBuildingProductionModifier");
    const auto managed=automated,tail=dirty;
    auto first_export=runtime::make_bnw_live_map_player_city_partial_export(
        state,imported,identity,source_hash,features,rules,types,defines);
    check(!first_export.full_persistence &&
          first_export.uncovered_domains.size()>=15,
          "BNW map/player/city export concealed incomplete persistence");
    for(const auto id:ids)
        check(section(first_export.document,id).flags==0,
              "BNW composed section flags changed");
    {
        auto overlapping=imported;
        overlapping.source.live_bnw_units=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,overlapping,identity,source_hash,features,rules,types,defines);});
        overlapping=imported;
        overlapping.source.live_bnw_area_units=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,overlapping,identity,source_hash,features,rules,types,defines);});
        overlapping=imported;
        overlapping.source.live_bnw_team_meet=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,overlapping,identity,source_hash,features,rules,types,defines);});
        overlapping=imported;
        overlapping.source.live_city_production=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,overlapping,identity,source_hash,features,rules,types,defines);});
        overlapping=imported;
        overlapping.source.live_bnw_player_proximity=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,overlapping,identity,source_hash,features,rules,types,defines);});
        overlapping=imported;
        overlapping.source.live_bnw_exploration_dirty=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,overlapping,identity,source_hash,features,rules,types,defines);});
        auto foreign_rules=identity;
        foreign_rules.rules_sha256[0]=
            foreign_rules.rules_sha256[0]=='0'?'1':'0';
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,imported,foreign_rules,source_hash,features,rules,types,defines);});
        ++player_core(state.world,0,"m_iTotalLandScored");
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,imported,identity,source_hash,features,rules,types,defines);});
        --player_core(state.world,0,"m_iTotalLandScored");
        auto &count=state.world.all_players->players[selected_city.player]
                        .cities.header.wire_city_count;
        ++count;
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_partial_export(
            state,imported,identity,source_hash,features,rules,types,defines);});
        --count;
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first=std::filesystem::current_path()/
        ("bnw-map-player-city-"+suffix+"-1.CivisSave");
    const auto second=std::filesystem::current_path()/
        ("bnw-map-player-city-"+suffix+"-2.CivisSave");
    const auto third=std::filesystem::current_path()/
        ("bnw-map-player-city-"+suffix+"-3.CivisSave");
    const auto fourth=std::filesystem::current_path()/
        ("bnw-map-player-city-"+suffix+"-4.CivisSave");
    struct Cleanup {std::filesystem::path first,second,third,fourth;~Cleanup(){
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
        std::filesystem::remove(third,error);
        std::filesystem::remove(fourth,error);
    }} cleanup{first,second,third,fourth};
    saves::write_civis_save(first,first_export.document);
    first_export.document.sections.clear();state.world={};
    {
        const auto reopened=saves::read_saved_session(first,context(features,true));
        check(reopened.source.source_sha256==source_hash &&
              reopened.source.bytes==imported.source.bytes,
              "BNW map/player/city save lost source identity or payload");
        auto restored=restore_all(reopened,identity,features,rules,types,defines);
        auto &restored_city=city(restored,selected_city);
        check(plot_field(restored,plot,"ownership_duration")==20 &&
              player_core(restored,0,"m_iTotalLandScored")==score &&
              strategy(restored,"economic_ai","exploration_plots_dirty")==flag &&
              city_core(restored_city,"m_iFood")==food &&
              field(production(restored_city),
                    "m_iBuildingProductionModifier")==modifier &&
              field(management(restored_city),"m_bAutomated")==managed &&
              tail_dirty(restored_city)==tail,
              "BNW map/player/city sections did not survive write/reload");
        auto resumed=state_from_world(std::move(restored));
        ++city_core(city(resumed.world,selected_city),"m_iFood");
        auto second_export=runtime::make_bnw_live_map_player_city_partial_export(
            resumed,reopened,identity,source_hash,features,rules,types,defines);
        saves::write_civis_save(second,second_export.document);
    }
    {
        const auto reopened=saves::read_saved_session(second,context(features,true));
        const auto document=saves::read_civis_save(second);
        for(const auto id:ids) (void)section(document,id);
        auto restored=restore_all(reopened,identity,features,rules,types,defines);
        auto &restored_city=city(restored,selected_city);
        check(plot_field(restored,plot,"ownership_duration")==20 &&
              player_core(restored,0,"m_iTotalLandScored")==score &&
              strategy(restored,"economic_ai","exploration_plots_dirty")==flag &&
              city_core(restored_city,"m_iFood")==food+1 &&
              field(production(restored_city),
                    "m_iBuildingProductionModifier")==modifier &&
              field(management(restored_city),"m_bAutomated")==managed &&
              tail_dirty(restored_city)==tail,
              "BNW second save did not replace ID17/23-28");
    }

    // The turn owner, decoded pregame mirror and elapsed GameCore mirror must
    // travel together with the same map/player/city document through two
    // save/reload cycles. A pending external publication cannot be saved.
    auto clock_state=state_from_world(imported.snapshot);
    auto &clock_pregame=clock_state.world.pregame->archive.integers.at("game_turn");
    auto &clock_prefix=std::get<saves::Expansion2PrefixObservation>(
        clock_state.world.game_state.prefix);
    const auto original_turn=clock_pregame;
    const auto original_elapsed=clock_prefix.scalars[1];
    clock_state.setup.profile=content::TechnologyProfile::bnw;
    clock_state.setup.game_turn=original_turn+1;
    clock_state.maintenance.elapsed_turns=original_elapsed+1;
    ++clock_pregame;
    ++clock_prefix.scalars[1];
    clock_state.game_clock.sent_auto_moves=false;
    // Real generators advance through their production mutators. The saved
    // payload's old RNG remains unchanged and is deliberately insufficient.
    clock_state.random.map.reseed(clock_state.random.map.state().seed);
    (void)clock_state.random.map.draw(0);
    (void)clock_state.random.map.draw(37);
    (void)clock_state.random.gameplay.draw(11);
    (void)clock_state.random.gameplay.draw_float();
    const auto first_map_random=clock_state.random.map.state();
    const auto first_game_random=clock_state.random.gameplay.state();
    auto uninterrupted=clock_state.random;
    const auto next_map=uninterrupted.map.draw(113);
    const auto next_game=uninterrupted.gameplay.draw(19);
    const auto second_map_random=uninterrupted.map.state();
    const auto second_game_random=uninterrupted.gameplay.state();
    {
        auto pending=clock_state;
        pending.game_clock.pending=runtime::BnwGameClockPublication{
            original_turn+1,0,false};
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
            pending,imported,identity,source_hash,features,rules,types,defines);});
        pending=clock_state;
        ++pending.maintenance.elapsed_turns;
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
            pending,imported,identity,source_hash,features,rules,types,defines);});
    }
    auto clock_export=runtime::make_bnw_live_map_player_city_clock_partial_export(
        clock_state,imported,identity,source_hash,features,rules,types,defines);
    constexpr auto clock_id=static_cast<std::uint32_t>(
        saves::CivisSaveSectionId::live_bnw_game_clock);
    constexpr auto random_id=static_cast<std::uint32_t>(
        saves::CivisSaveSectionId::live_simulation_random);
    const auto &first_random_section=section(clock_export.document,random_id);
    check(first_random_section.flags==0 && first_random_section.bytes.size()==196 &&
          first_random_section.bytes[8]==1,
          "BNW composed current RNG section absent or changed version");
    {
        auto restored_random=runtime::restore_session_random(imported.snapshot.progression);
        runtime::restore_simulation_random_checkpoint(first_random_section,restored_random,identity);
        check(restored_random.map.state()==first_map_random &&
              restored_random.gameplay.state()==first_game_random,
              "BNW exporter copied old payload RNG rather than current streams");
        const auto before_map=restored_random.map.state();
        const auto before_game=restored_random.gameplay.state();
        auto damaged=first_random_section;damaged.bytes.pop_back();
        rejects([&]{runtime::restore_simulation_random_checkpoint(damaged,restored_random,identity);});
        damaged=first_random_section;damaged.flags=1;
        rejects([&]{runtime::restore_simulation_random_checkpoint(damaged,restored_random,identity);});
        auto wrong_identity=identity;wrong_identity.rules_sha256[0]=wrong_identity.rules_sha256[0]=='a'?'b':'a';
        rejects([&]{runtime::restore_simulation_random_checkpoint(first_random_section,restored_random,wrong_identity);});
        // First stream parses successfully, second algorithm fails. Neither
        // destination generator may change before the complete pair validates.
        damaged=first_random_section;damaged.bytes[168]=2;
        rejects([&]{runtime::restore_simulation_random_checkpoint(damaged,restored_random,identity);});
        check(restored_random.map.state()==before_map && restored_random.gameplay.state()==before_game,
              "Malformed RNG checkpoint partially changed the stream pair");
    }
    check(!clock_export.full_persistence &&
          section(clock_export.document,clock_id).flags==0,
          "BNW composed clock export falsely claimed full persistence");
    for(const auto id:ids) {
        const auto &part=section(clock_export.document,id);
        if(id!=static_cast<std::uint32_t>(
                saves::CivisSaveSectionId::live_bnw_map_plots))
            check(part.bytes.size()>=12 && part.bytes[8]==
                      (id==runtime::bnw_city_core_checkpoint_section_id?4:2) &&
                  part.bytes[9]==0 && part.bytes[10]==0 && part.bytes[11]==0,
                  "BNW unchanged core baseline/player/city snapshot used wrong version");
    }
    {
        auto mixed=clock_state;
        ++city_core(city(mixed.world,selected_city),"m_iFood");
        const auto mixed_export=
            runtime::make_bnw_live_map_player_city_clock_partial_export(
                mixed,imported,identity,source_hash,features,rules,types,defines);
        check(section(mixed_export.document,
                      runtime::bnw_city_core_checkpoint_section_id).bytes[8]==3 &&
              section(mixed_export.document,
                      runtime::bnw_city_production_checkpoint_section_id).bytes[8]==2 &&
              section(mixed_export.document,
                      runtime::bnw_city_management_checkpoint_section_id).bytes[8]==2 &&
              section(mixed_export.document,
                      runtime::bnw_city_tail_checkpoint_section_id).bytes[8]==2,
              "BNW mixed changed/unchanged city trees used wrong versions");
        auto mislabeled=section(clock_export.document,
                                runtime::bnw_city_core_checkpoint_section_id);
        mislabeled.bytes[8]=3;
        auto baseline=imported.snapshot;
        rejects([&]{runtime::restore_bnw_city_core_checkpoint(
            mislabeled,baseline,imported.snapshot,identity);});
        auto altered_source=imported.snapshot;
        ++city_core(city(altered_source,selected_city),"m_iFood");
        auto altered_destination=altered_source;
        const auto food_before=city_core(city(altered_destination,selected_city),"m_iFood");
        const auto& genuine=section(clock_export.document,runtime::bnw_city_core_checkpoint_section_id);
        rejects([&]{runtime::restore_bnw_city_core_checkpoint(
            genuine,altered_destination,altered_source,identity);});
        check(city_core(city(altered_destination,selected_city),"m_iFood")==food_before,
              "General exported strict baseline failure partially changed destination");
    }
    saves::write_civis_save(third,clock_export.document);
    {
        const auto reopened=saves::read_saved_session(third,context(features,true));
        check(reopened.source.live_bnw_game_clock.has_value() &&
              reopened.source.live_simulation_random.has_value(),
              "BNW composed clock/RNG sections missing after import");
        auto restored_random=runtime::restore_session_random(reopened.snapshot.progression);
        runtime::restore_simulation_random_checkpoint(
            {random_id,0,*reopened.source.live_simulation_random},restored_random,identity);
        check(restored_random.map.state()==first_map_random &&
              restored_random.gameplay.state()==first_game_random &&
              restored_random.map.draw(113)==next_map &&
              restored_random.gameplay.draw(19)==next_game &&
              restored_random.map.state()==second_map_random &&
              restored_random.gameplay.state()==second_game_random,
              "Save/reload did not preserve the next actual map/game draws");
        auto restored=restore_all(reopened,identity,features,rules,types,defines);
        runtime::BnwGameClockState clock;
        runtime::restore_bnw_game_clock_checkpoint(
            {clock_id,0,*reopened.source.live_bnw_game_clock},
            restored,clock,identity);
        check(restored.pregame->archive.integers.at("game_turn")==original_turn+1 &&
              std::get<saves::Expansion2PrefixObservation>(
                  restored.game_state.prefix).scalars[1]==original_elapsed+1 &&
              !clock.pending,
              "BNW composed clock did not survive save/reload");
        auto resumed=state_from_world(std::move(restored));
        resumed.random=std::move(restored_random);
        resumed.setup.profile=content::TechnologyProfile::bnw;
        resumed.setup.game_turn=original_turn+1;
        resumed.maintenance.elapsed_turns=original_elapsed+1;
        auto regressed=resumed;
        --regressed.setup.game_turn;
        --regressed.maintenance.elapsed_turns;
        --regressed.world.pregame->archive.integers.at("game_turn");
        --std::get<saves::Expansion2PrefixObservation>(
            regressed.world.game_state.prefix).scalars[1];
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
            regressed,reopened,identity,source_hash,features,rules,types,defines);});
        auto corrupted=reopened;
        corrupted.source.live_bnw_game_clock->front()^=1;
        rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
            resumed,corrupted,identity,source_hash,features,rules,types,defines);});
        {
            auto rng_regressed=resumed;
            rng_regressed.random=runtime::restore_session_random(imported.snapshot.progression);
            rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
                rng_regressed,reopened,identity,source_hash,features,rules,types,defines);});
            auto reset_regressed=resumed;
            const auto reset_state=reset_regressed.random.map.state();
            check(reset_state.resets>0,"Actual reseed history absent in fixture");
            reset_regressed.random.map=runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,
                {reset_state.seed,reset_state.calls,reset_state.resets-1});
            rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
                reset_regressed,reopened,identity,source_hash,features,rules,types,defines);});
            auto same_history=resumed;
            same_history.random=runtime::restore_session_random(imported.snapshot.progression);
            runtime::restore_simulation_random_checkpoint(
                {random_id,0,*reopened.source.live_simulation_random},same_history.random,identity);
            const auto actual=same_history.random.map.state();
            same_history.random.map=runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,
                {actual.seed^1,actual.calls,actual.resets});
            rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
                same_history,reopened,identity,source_hash,features,rules,types,defines);});
            auto wrong_algorithm=resumed;
            wrong_algorithm.random.gameplay=runtime::SimulationRandom(
                runtime::RandomAlgorithm::vp_reference,resumed.random.gameplay.state());
            rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
                wrong_algorithm,reopened,identity,source_hash,features,rules,types,defines);});
            auto bad_source=reopened;bad_source.source.live_simulation_random->front()^=1;
            rejects([&]{(void)runtime::make_bnw_live_map_player_city_clock_partial_export(
                resumed,bad_source,identity,source_hash,features,rules,types,defines);});
        }
        ++resumed.setup.game_turn;
        ++resumed.maintenance.elapsed_turns;
        ++resumed.world.pregame->archive.integers.at("game_turn");
        ++std::get<saves::Expansion2PrefixObservation>(
            resumed.world.game_state.prefix).scalars[1];
        auto second_clock=runtime::make_bnw_live_map_player_city_clock_partial_export(
            resumed,reopened,identity,source_hash,features,rules,types,defines);
        saves::write_civis_save(fourth,second_clock.document);
    }
    {
        const auto reopened=saves::read_saved_session(fourth,context(features,true));
        check(reopened.source.live_simulation_random.has_value(),"Second composed RNG section missing");
        auto restarted=runtime::restore_session_random(reopened.snapshot.progression);
        runtime::restore_simulation_random_checkpoint(
            {random_id,0,*reopened.source.live_simulation_random},restarted,identity);
        check(restarted.map.state()==second_map_random && restarted.gameplay.state()==second_game_random &&
              restarted.map.draw(65536)==uninterrupted.map.draw(65536) &&
              restarted.gameplay.draw(997)==uninterrupted.gameplay.draw(997),
              "Second save retained stale inherited ID13 or lost next draws");
        auto restored=restore_all(reopened,identity,features,rules,types,defines);
        runtime::BnwGameClockState clock;
        runtime::restore_bnw_game_clock_checkpoint(
            {clock_id,0,*reopened.source.live_bnw_game_clock},
            restored,clock,identity);
        check(restored.pregame->archive.integers.at("game_turn")==original_turn+2 &&
              std::get<saves::Expansion2PrefixObservation>(
                  restored.game_state.prefix).scalars[1]==original_elapsed+2,
              "BNW second composed save lost clock progression");
    }
    check(runtime::identify_file(path).sha256==source_hash,
          "BNW export altered original Civ5Save");
    std::cout << "PASS BNW map/player/city partial export roundtrip\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and BNW game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW map/player/city export: " << error.what() << '\n';
        return 1;
    }
}
