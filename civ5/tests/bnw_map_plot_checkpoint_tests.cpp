#include "civ5/runtime/bnw_map_plot_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try { f(); } catch(const std::exception &) { rejected=true; }
    check(rejected,"Malformed map checkpoint was accepted");
}
std::int64_t &raw_field(saves::PlotObservation &plot,std::string_view name) {
    for(auto &field:plot.fields)
        if(field.name && name==field.name) return field.value;
    throw std::runtime_error("Selected plot lacks requested field");
}
std::vector<std::int64_t> &raw_array(saves::PlotObservation &plot,std::string_view name) {
    for(auto &array:plot.arrays)
        if(array.name && name==array.name) return array.values;
    throw std::runtime_error("Selected plot lacks requested array");
}
constexpr std::size_t old_map_header=8+4+64+64+8*10;
constexpr std::size_t yield_header=old_map_header+16+8+64+8;
std::uint64_t wire64(const std::vector<std::uint8_t>& bytes,std::size_t at) {
    check(at<=bytes.size()&&bytes.size()-at>=8,"Test section uint64 truncated");
    std::uint64_t value=0;for(unsigned i=0;i<8;++i)value|=std::uint64_t(bytes[at+i])<<(8*i);return value;
}
void wire64(std::vector<std::uint8_t>& bytes,std::size_t at,std::uint64_t value) {
    check(at<=bytes.size()&&bytes.size()-at>=8,"Test section uint64 truncated");
    for(unsigned i=0;i<8;++i)bytes[at+i]=static_cast<std::uint8_t>(value>>(8*i));
}
saves::CivisSaveSection as_v5(saves::CivisSaveSection section) {
    check(section.bytes.at(8)==6&&wire64(section.bytes,yield_header-8)==0,
          "Legacy fixture cannot discard changed yield cache");
    section.bytes.erase(section.bytes.begin()+static_cast<std::ptrdiff_t>(old_map_header),
                        section.bytes.begin()+static_cast<std::ptrdiff_t>(yield_header));
    section.bytes[8]=5;return section;
}
std::int64_t &player_land_score(saves::SavedSessionSnapshot &world,std::int64_t id) {
    for(auto &player:world.all_players->players)
        if(player.saved_player_id==id)
            for(auto &field:player.core.fields)
                if(field.name && std::string_view(field.name)=="m_iTotalLandScored" &&
                   field.encoding==saves::PlayerCoreEncoding::i32 && field.values.size()==1)
                    return field.values.front();
    throw std::runtime_error("Selected player land score unavailable");
}
void test_map_turn_scalars() {
    content::RulesDatabase rules;
    rules.apply_sql("CREATE TABLE Defines(Name TEXT,Value INTEGER);"
                    "INSERT INTO Defines VALUES('OWNERSHIP_SCORE_DURATION_THRESHOLD',20);");
    const auto defines=rules.cache_global_defines(content::GlobalDefineProfile::bnw);
    saves::SavedSessionSnapshot source;
    source.game_state.layout=source.map_header.layout=
        saves::GameStateLayout::expansion2_version1;
    source.map_header.grid={1,1};
    source.map_header.land_plots=source.map_header.owned_plots=1;
    source.map_plots.emplace();
    source.map_plots->layout=source.game_state.layout;
    source.map_plots->dimensions.players=64;
    source.map_plots->dimensions.teams=1;
    source.map_plots->dimensions.yields=6;
    saves::PlotObservation plot;
    plot.version=7;
    plot.fields={{"owner",0},{"plot_type",2},{"terrain_type",0},
        {"feature_type_hash",0},{"improvement_type_hash",123},
        {"route_type",-1},{"route_responsible",-1},{"route_pillaged",0},
        {"improvement_pillaged",0},{"rough_feature",0},
        {"feature_variety",0},{"improvement_builder",-1},
        {"improvement_responsible",-1},{"ownership_duration",19},
        {"improvement_duration",4},{"world_anchor",2},{"world_anchor_data",5},
        {"builder_scratch_player",0},{"builder_scratch_turn",0},
        {"builder_scratch_value",0},{"builder_scratch_route",-1}};
    plot.arrays.push_back({"visibility",1,{1}});
    plot.arrays.push_back({"yields",1,{0,0,0,0,0,0}});
    source.map_plots->plots.push_back(plot);
    source.map_projection=saves::project_saved_map(source.map_header,*source.map_plots);
    source.map_terrain=saves::project_saved_map_terrain(*source.map_projection);
    source.all_players.emplace();source.all_players->layout=source.game_state.layout;
    saves::SavedPlayerObservation player;
    player.saved_player_id=0;player.core.layout=source.game_state.layout;
    player.core.fields.push_back({"m_iTotalLandScored",saves::PlayerCoreEncoding::i32,
                                  0,0,{}, {6}, {}, {}, {}});
    source.all_players->players.push_back(player);
    auto live=source;
    auto &changed=live.map_plots->plots.front();
    raw_field(changed,"ownership_duration")=20;
    raw_field(changed,"improvement_duration")=5;
    raw_field(changed,"world_anchor")=-1;
    raw_field(changed,"world_anchor_data")=-1;
    player_land_score(live,0)=7;
    const runtime::SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
    const auto section=runtime::make_bnw_map_plot_checkpoint(
        live,source,identity,{},nullptr,nullptr,&defines);
    auto restored=source;
    runtime::restore_bnw_map_plot_checkpoint(
        section,restored,identity,{},nullptr,nullptr,&defines);
    check(raw_field(restored.map_plots->plots.front(),"ownership_duration")==20 &&
          raw_field(restored.map_plots->plots.front(),"improvement_duration")==5 &&
          raw_field(restored.map_plots->plots.front(),"world_anchor")==-1 &&
          raw_field(restored.map_plots->plots.front(),"world_anchor_data")==-1 &&
          player_land_score(restored,0)==7,
          "BNW map-turn plot fields or score failed v3 round-trip");
    auto bad=live;
    raw_field(bad.map_plots->plots.front(),"improvement_duration")=INT16_MAX+1LL;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        bad,source,identity,{},nullptr,nullptr,&defines);});
    bad=live;
    raw_field(bad.map_plots->plots.front(),"world_anchor")=128;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        bad,source,identity,{},nullptr,nullptr,&defines);});
}
void test_selected(const std::filesystem::path &path) {
    const auto original_identity=runtime::identify_file(path);
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.map_plots && imported.snapshot.map_projection &&
          imported.snapshot.map_terrain,"Selected Gandhi save lacks complete BNW map");
    auto live=imported.snapshot;
    const auto &baseline=*imported.snapshot.map_projection;
    std::size_t changed=baseline.cells.size();
    for(std::size_t i=0;i<baseline.cells.size();++i) {
        const auto &cell=baseline.cells[i];
        if(cell.plot_type==2 && cell.terrain_type && *cell.terrain_type>=0 &&
           *cell.terrain_type<=4 && cell.feature_type_hash==0) {
            changed=i;break;
        }
    }
    check(changed<baseline.cells.size(),"Selected Gandhi save has no simple land plot");
    auto &plot=live.map_plots->plots.at(changed);
    raw_field(plot,"plot_type")=1; // PLOT_HILLS remains land but changes terrain mask.
    live.map_projection=saves::project_saved_map(live.map_header,*live.map_plots);
    live.map_terrain=saves::project_saved_map_terrain(*live.map_projection);
    check(live.map_projection->cells[changed].native_terrain_mask!=
          baseline.cells[changed].native_terrain_mask,
          "Topography action did not change map classification");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto section=runtime::make_bnw_map_plot_checkpoint(live,imported.snapshot,identity,{});
    auto corrupt=section;
    corrupt.bytes[8+4+64+64+8*3]=2; // Record count 1 -> 2.
    rejects([&]{ auto copy=imported.snapshot;
        runtime::restore_bnw_map_plot_checkpoint(corrupt,copy,identity,{}); });
    corrupt=section;
    std::fill(corrupt.bytes.begin()+static_cast<std::ptrdiff_t>(yield_header),
              corrupt.bytes.begin()+static_cast<std::ptrdiff_t>(yield_header+8),0xff); // Invalid v6 plot ID.
    rejects([&]{ auto copy=imported.snapshot;
        runtime::restore_bnw_map_plot_checkpoint(corrupt,copy,identity,{}); });
    auto legacy=section;
    legacy.bytes.resize(8+4+64+64+8*4);
    legacy.bytes[8]=1;
    const auto append64=[&](std::uint64_t value) {
        for(unsigned i=0;i<8;++i)
            legacy.bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    };
    append64(changed);
    append64(2);
    append64(static_cast<std::uint64_t>(raw_field(imported.snapshot.map_plots->plots[changed],"terrain_type")));
    append64(1);
    append64(static_cast<std::uint64_t>(raw_field(live.map_plots->plots[changed],"terrain_type")));
    auto old_restored=imported.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(legacy,old_restored,identity,{});
    check(old_restored.map_projection->cells[changed].plot_type==1,
          "Version 1 topography checkpoint no longer restores");
    auto wrong_identity=identity;wrong_identity.rules_sha256=std::string(64,'b');
    rejects([&]{ auto copy=imported.snapshot;
        runtime::restore_bnw_map_plot_checkpoint(section,copy,wrong_identity,{}); });
    auto unsupported=live;
    raw_field(unsupported.map_plots->plots[changed],"owner")=4;
    rejects([&]{ (void)runtime::make_bnw_map_plot_checkpoint(
        unsupported,imported.snapshot,identity,{}); });
    unsupported=live;
    unsupported.map_projection->cells[changed].owner=4;
    rejects([&]{ (void)runtime::make_bnw_map_plot_checkpoint(
        unsupported,imported.snapshot,identity,{}); });
    unsupported=live;
    ++raw_field(unsupported.map_plots->plots[changed],"ownership_duration");
    rejects([&]{ (void)runtime::make_bnw_map_plot_checkpoint(
        unsupported,imported.snapshot,identity,{}); });
    unsupported=live;
    raw_field(unsupported.map_plots->plots[changed],"plot_type")=3;
    raw_field(unsupported.map_plots->plots[changed],"terrain_type")=5;
    unsupported.map_projection=saves::project_saved_map(unsupported.map_header,*unsupported.map_plots);
    unsupported.map_terrain=saves::project_saved_map_terrain(*unsupported.map_projection);
    rejects([&]{ (void)runtime::make_bnw_map_plot_checkpoint(
        unsupported,imported.snapshot,identity,{}); });

    auto document=saves::make_civis_import_document(imported.source,original_identity.sha256);
    const auto plain=std::filesystem::current_path()/(
        "bnw-map-plain-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(plain,document);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove(path,e); } } plain_cleanup{plain};
    auto no_overlay=saves::read_saved_session(plain,context);
    check(!no_overlay.source.live_bnw_map_plots &&
          no_overlay.snapshot.map_projection->cells[changed].plot_type==2,
          "Plain Civis import changed without map overlay");
    document.sections.push_back(section);
    const auto filename="bnw-map-plot-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave";
    const auto output=std::filesystem::current_path()/filename;
    saves::write_civis_save(output,document);
    Cleanup cleanup{output};
    rejects([&]{ (void)saves::read_saved_session(output,context); });
    context.permit_live_bnw_map_plots_overlay=true;
    auto reloaded=saves::read_saved_session(output,context);
    check(reloaded.source.live_bnw_map_plots &&
          *reloaded.source.live_bnw_map_plots==section.bytes,
          "Civis reader lost live BNW map section");
    // LiveSession::restore_bnw invokes this before city and map load hooks.
    auto restored=reloaded.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(section,restored,identity,{});
    runtime::LiveSessionState state{
        .world=restored,
        .setup={},
        .technologies={},
        .player_research={},
        .cities={},
        .unhappiness={},
        .maintenance={},
        .treasury={},
        .science={},
        .research_turn={},
        .random={runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{0,0,0}),
                 runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{0,0,0})},
        .research_selection={},
        .game_clock={},
        .player_gold_publication={},
        .player_research_publication={},
        .player_early_publication={},
        .nearby_cities={},
        .city_flavor_recipients={},
        .city_strategy_runtime={},
        .player_flavor_recipients={},
        .plot_impassability={},
        .adjacent_land={},
        .plot_visibility={},
        .diplomacy_scratchpad={},
        .notification_load_transients={},
        .player_plot_lists={},
        .player_read_tail={},
        .game_plot_extras={},
        .player_culture_bomb_timers={},
        .military_request_counters={},
        .city_dirty_events={},.city_dirty_next_occurrence=1,.specific_city_info_events={},.ui_source_revision=1,
        .plot_gameplay_events={},.plot_gameplay_next_occurrence=1,
        .standalone_services={},.bnw_unit_mission_continuation_quarantine={}};
    auto owned=runtime::LiveSession::from_restored_bnw(std::move(reloaded),std::move(state));
    check(owned.state().world.map_projection->cells[changed].plot_type==1 &&
          owned.source().snapshot.map_projection->cells[changed].plot_type==2 &&
          !owned.readiness().can_continue(),
          "Owned LiveSession did not separate restored map from immutable source");
    check(raw_field(restored.map_plots->plots[changed],"plot_type")==1 &&
          raw_field(restored.map_plots->plots[changed],"terrain_type")==
              raw_field(live.map_plots->plots[changed],"terrain_type"),
          "Civis reload did not restore raw plot topography");
    check(restored.map_projection->cells[changed].plot_type==1 &&
          restored.map_terrain->known_terrain_masks==live.map_terrain->known_terrain_masks &&
          restored.map_projection->cells[changed].native_terrain_mask==
              live.map_projection->cells[changed].native_terrain_mask,
          "Civis reload did not rebuild derived map views");
    check(runtime::identify_file(path).sha256==original_identity.sha256,
          "Original Civ5Save changed during map checkpoint round-trip");
    std::cout << "selected BNW plot=" << changed << " map=" << baseline.cells.size() << '\n';
}
void test_selected_v2(const std::filesystem::path &path,
                      const std::filesystem::path &game_root) {
    const auto original_identity=runtime::identify_file(path);
    content::RulesDatabase rules;
    for(const auto &relative:{
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Terrains.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Features.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Improvements.xml",
        "Assets/Gameplay/XML/Misc/CIV5Routes.xml"})
        rules.apply_xml(content::read_xml(game_root/relative));
    const auto types=rules.cache_info_types();
    check(types.report().all_succeeded(),"Test BNW map type catalog failed");
    const auto features=content::saved_terrain_features(rules,types).resolve;
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    context.terrain_features=features;
    auto imported=saves::read_saved_session(path,context);
    const auto &cells=imported.snapshot.map_projection->cells;
    std::size_t changed=cells.size();
    for(std::size_t i=0;i<cells.size();++i) {
        const auto &c=cells[i];
        if(c.plot_type==2 && c.owner==-1 && c.feature_type_hash==0 &&
           c.improvement_type_hash==0 && c.route && c.route->type==-1 &&
           !c.is_city && c.units && c.units->empty()) { changed=i;break; }
    }
    check(changed<cells.size(),"Selected Gandhi save lacks a simple unowned empty plot");
    auto live=imported.snapshot;
    auto &plot=live.map_plots->plots.at(changed);
    raw_field(plot,"owner")=0;
    raw_field(plot,"feature_type_hash")=content::saved_info_type_hash("FEATURE_FOREST");
    raw_field(plot,"rough_feature")=1;
    raw_field(plot,"improvement_type_hash")=content::saved_info_type_hash("IMPROVEMENT_FARM");
    raw_field(plot,"route_type")=0;
    ++live.map_header.owned_plots;
    live.map_projection=saves::project_saved_map(live.map_header,*live.map_plots,features);
    live.map_terrain=saves::project_saved_map_terrain(*live.map_projection);
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        runtime::identify_bytes(rules.gameplay_fingerprint_data()).sha256};
    rejects([&]{ (void)runtime::make_bnw_map_plot_checkpoint(
        live,imported.snapshot,identity,features); });
    auto section=runtime::make_bnw_map_plot_checkpoint(
        live,imported.snapshot,identity,features,&rules,&types);
    check(section.bytes.at(8)==6,"Extended BNW map overlay schema is not version 6");
    auto legacy_v2=as_v5(section);
    legacy_v2.bytes.erase(legacy_v2.bytes.begin()+8+4+64+64+8*8,
                          legacy_v2.bytes.begin()+8+4+64+64+8*10);
    legacy_v2.bytes[8]=2;
    auto old_restored=imported.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(
        legacy_v2,old_restored,identity,features,&rules,&types);
    check(old_restored.map_projection->cells.at(changed).owner==0 &&
          old_restored.map_projection->cells.at(changed).route->type==0,
          "Version 2 typed plot checkpoint no longer restores");
    auto bad=section;bad.bytes.at(8+4+64+64+8*4+8*3)^=1;
    rejects([&]{ auto copy=imported.snapshot;
        runtime::restore_bnw_map_plot_checkpoint(bad,copy,identity,features,&rules,&types); });
    auto unknown=live;
    raw_field(unknown.map_plots->plots[changed],"improvement_type_hash")=123456789;
    unknown.map_projection=saves::project_saved_map(unknown.map_header,*unknown.map_plots,features);
    unknown.map_terrain=saves::project_saved_map_terrain(*unknown.map_projection);
    rejects([&]{ (void)runtime::make_bnw_map_plot_checkpoint(
        unknown,imported.snapshot,identity,features,&rules,&types); });
    auto document=saves::make_civis_import_document(
        imported.source,runtime::identify_file(path).sha256);
    document.sections.push_back(section);
    const auto output=std::filesystem::current_path()/(
        "bnw-map-v3-types-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove(path,e); } } cleanup{output};
    context.permit_live_bnw_map_plots_overlay=true;
    auto reloaded=saves::read_saved_session(output,context);
    check(reloaded.source.live_bnw_map_plots &&
          *reloaded.source.live_bnw_map_plots==section.bytes,
          "Atomic CivisSave lost typed map overlay");
    auto restored=reloaded.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(section,restored,identity,features,&rules,&types);
    const auto &cell=restored.map_projection->cells.at(changed);
    check(cell.owner==0 &&
          cell.feature_type_hash==content::saved_info_type_hash("FEATURE_FOREST") &&
          cell.improvement_type_hash==content::saved_info_type_hash("IMPROVEMENT_FARM") &&
          cell.route && cell.route->type==0 &&
          restored.map_header.owned_plots==imported.snapshot.map_header.owned_plots+1 &&
          restored.map_terrain->known_terrain_masks==live.map_terrain->known_terrain_masks &&
          reloaded.snapshot.map_projection->cells.at(changed).owner==-1,
          "V2 map overlay failed to restore typed plot and derived views");
    const auto passability=runtime::restore_plot_passability(restored,rules,types);
    check(passability.plots.size()==cells.size() &&
          !passability.plots.at(changed).impassable,
          "Post-load BNW impassability did not consume restored feature");
    check(runtime::identify_file(path).sha256==original_identity.sha256,
          "Selected original save changed");
    std::cout << "selected BNW v2 plot=" << changed << '\n';
}
void test_selected_map_turn(const std::filesystem::path &path,
                            const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path);
    content::RulesDatabase rules;
    for(const auto &relative:{
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Terrains.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Features.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Improvements.xml",
        "Assets/Gameplay/XML/Misc/CIV5Routes.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/GlobalDefines.xml"})
        rules.apply_xml(content::read_xml(game_root/relative));
    const auto types=rules.cache_info_types();
    const auto defines=rules.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto *threshold_entry=defines.find("OWNERSHIP_SCORE_DURATION_THRESHOLD");
    check(threshold_entry && threshold_entry->lookup.found &&
          std::get<std::int64_t>(threshold_entry->lookup.value)==20,
          "Selected BNW ownership threshold differs from active XML");
    const auto features=content::saved_terrain_features(rules,types).resolve;
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    context.terrain_features=features;
    auto imported=saves::read_saved_session(path,context);
    std::size_t changed=imported.snapshot.map_plots->plots.size();
    for(std::size_t i=0;i<changed;++i) {
        const auto &cell=imported.snapshot.map_projection->cells[i];
        auto &plot=imported.snapshot.map_plots->plots[i];
        if(cell.owner==0 && cell.plot_type && *cell.plot_type!=3 &&
           raw_field(plot,"ownership_duration")>=0 &&
           raw_field(plot,"ownership_duration")<20 &&
           cell.units && cell.units->empty()) { changed=i;break; }
    }
    check(changed<imported.snapshot.map_plots->plots.size(),
          "Selected Gandhi save lacks an owned score-eligible plot");
    auto live=imported.snapshot;
    raw_field(live.map_plots->plots[changed],"ownership_duration")=20;
    ++player_land_score(live,0);
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        runtime::identify_bytes(rules.gameplay_fingerprint_data()).sha256};
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        live,imported.snapshot,identity,features,&rules,&types);});
    auto section=runtime::make_bnw_map_plot_checkpoint(
        live,imported.snapshot,identity,features,&rules,&types,&defines);
    check(section.bytes.at(8)==6,"Map-turn checkpoint did not emit v6 schema");
    auto legacy_v3=as_v5(section);
    legacy_v3.bytes.erase(legacy_v3.bytes.begin()+8+4+64+64+8*9,
                          legacy_v3.bytes.begin()+8+4+64+64+8*10);
    legacy_v3.bytes[8]=3;
    auto legacy_restored=imported.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(
        legacy_v3,legacy_restored,identity,features,&rules,&types,&defines);
    check(raw_field(legacy_restored.map_plots->plots[changed],"ownership_duration")==20,
          "Version 3 map-turn checkpoint no longer restores");
    auto bad=section;
    std::fill(bad.bytes.end()-24,bad.bytes.end()-16,0xff); // Unknown player ID.
    rejects([&]{auto copy=imported.snapshot;
        runtime::restore_bnw_map_plot_checkpoint(
            bad,copy,identity,features,&rules,&types,&defines);});
    auto wrong=live;++player_land_score(wrong,0);
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        wrong,imported.snapshot,identity,features,&rules,&types,&defines);});
    auto no_score=live;--player_land_score(no_score,0);
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        no_score,imported.snapshot,identity,features,&rules,&types,&defines);});
    auto document=saves::make_civis_import_document(imported.source,original.sha256);
    document.sections.push_back(section);
    const auto output=std::filesystem::current_path()/(
        "bnw-map-v3-turn-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove(path,e); } } cleanup{output};
    context.permit_live_bnw_map_plots_overlay=true;
    auto reloaded=saves::read_saved_session(output,context);
    auto restored=reloaded.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(
        section,restored,identity,features,&rules,&types,&defines);
    check(raw_field(restored.map_plots->plots[changed],"ownership_duration")==20 &&
          player_land_score(restored,0)==player_land_score(live,0) &&
          player_land_score(reloaded.snapshot,0)==player_land_score(imported.snapshot,0) &&
          restored.map_projection->cells[changed].owner==0,
          "Atomic CivisSave lost BNW map-turn duration or player score");
    check(runtime::identify_file(path).sha256==original.sha256,
          "Selected original save changed during map-turn checkpoint");
    std::cout << "selected BNW score-crossing plot=" << changed << '\n';
}
void test_selected_visibility(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    const auto teams=imported.snapshot.map_plots->dimensions.teams;
    std::size_t plot_id=imported.snapshot.map_plots->plots.size(),team_id=0;
    for(std::size_t i=0;i<plot_id;++i) {
        auto &counts=raw_array(imported.snapshot.map_plots->plots[i],"visibility");
        check(counts.size()==teams,"Selected BNW plot visibility dimensions differ");
        for(std::size_t team=0;team<counts.size();++team)
            if(counts[team]>0 && counts[team]<INT16_MAX) {
                plot_id=i;team_id=team;break;
            }
    }
    check(plot_id<imported.snapshot.map_plots->plots.size(),
          "Selected Gandhi save lacks a visible BNW plot");
    auto live=imported.snapshot;
    const auto base=raw_array(live.map_plots->plots[plot_id],"visibility")[team_id];
    raw_array(live.map_plots->plots[plot_id],"visibility")[team_id]=base+1;
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto section=runtime::make_bnw_map_plot_checkpoint(live,imported.snapshot,identity,{});
    check(section.bytes.at(8)==6,"Visibility checkpoint did not use v6");
    auto legacy_v4=as_v5(section);
    legacy_v4.bytes[8]=4;
    auto old_restored=imported.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(legacy_v4,old_restored,identity,{});
    check(raw_array(old_restored.map_plots->plots[plot_id],"visibility")[team_id]==base+1,
          "Version 4 visibility checkpoint no longer restores");
    auto bad=section;
    bad.bytes[8+4+64+64+8*9]=2; // One visibility record -> two.
    rejects([&]{auto copy=imported.snapshot;
        runtime::restore_bnw_map_plot_checkpoint(bad,copy,identity,{});});
    auto bad_world=live;
    raw_array(bad_world.map_plots->plots[plot_id],"visibility")[team_id]=0;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        bad_world,imported.snapshot,identity,{});});
    bad_world=live;
    raw_array(bad_world.map_plots->plots[plot_id],"revealed_words")[0]^=1;
    bad_world.map_projection=saves::project_saved_map(bad_world.map_header,*bad_world.map_plots);
    bad_world.map_terrain=saves::project_saved_map_terrain(*bad_world.map_projection);
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        bad_world,imported.snapshot,identity,{});});
    auto document=saves::make_civis_import_document(imported.source,original.sha256);
    document.sections.push_back(section);
    const auto output=std::filesystem::current_path()/(
        "bnw-map-v4-visibility-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code e;std::filesystem::remove(path,e);
    } } cleanup{output};
    context.permit_live_bnw_map_plots_overlay=true;
    auto reloaded=saves::read_saved_session(output,context);
    check(reloaded.source.live_bnw_map_plots &&
          *reloaded.source.live_bnw_map_plots==section.bytes,
          "Atomic CivisSave lost BNW visibility section");
    auto restored=reloaded.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(section,restored,identity,{});
    check(raw_array(restored.map_plots->plots[plot_id],"visibility")[team_id]==base+1 &&
          raw_array(reloaded.snapshot.map_plots->plots[plot_id],"visibility")[team_id]==base &&
          runtime::identify_file(path).sha256==original.sha256,
          "Selected BNW visibility checkpoint did not round-trip");
    std::cout << "selected BNW visibility plot=" << plot_id << " team=" << team_id << '\n';
}
void test_selected_builder_scratch(const std::filesystem::path &path,
                                   const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path);
    content::RulesDatabase rules;
    rules.apply_xml(content::read_xml(game_root/
        "Assets/Gameplay/XML/Misc/CIV5Routes.xml"));
    const auto types=rules.cache_info_types();
    check(types.report().all_succeeded(),"Selected route type catalog failed");
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.map_plots && !imported.snapshot.map_plots->plots.empty() &&
          imported.snapshot.all_players,"Selected Gandhi scratchpad source missing");
    auto live=imported.snapshot;
    const std::size_t plot_id=12;
    check(plot_id<live.map_plots->plots.size(),"Selected Gandhi plot ID missing");
    auto &plot=live.map_plots->plots[plot_id];
    raw_field(plot,"builder_scratch_player")=0;
    raw_field(plot,"builder_scratch_turn")=1;
    raw_field(plot,"builder_scratch_value")=1000;
    raw_field(plot,"builder_scratch_route")=0;
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        runtime::identify_bytes(rules.gameplay_fingerprint_data()).sha256};
    auto section=runtime::make_bnw_map_plot_checkpoint(
        live,imported.snapshot,identity,{},&rules,&types);
    check(section.bytes.at(8)==6,"Builder scratch checkpoint did not use v6");
    auto wrong=live;
    raw_field(wrong.map_plots->plots[plot_id],"builder_scratch_route")=INT32_MAX;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        wrong,imported.snapshot,identity,{},&rules,&types);});
    wrong=live;
    raw_field(wrong.map_plots->plots[plot_id],"builder_scratch_player")=100;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        wrong,imported.snapshot,identity,{},&rules,&types);});
    wrong=live;
    raw_field(wrong.map_plots->plots[plot_id],"builder_scratch_value")=INT16_MAX+1LL;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        wrong,imported.snapshot,identity,{},&rules,&types);});
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        live,imported.snapshot,identity,{});});
    auto corrupt=as_v5(section);
    corrupt.bytes[8]=4; // v4 mask cannot accept builder scratch fields.
    rejects([&]{auto copy=imported.snapshot;
        runtime::restore_bnw_map_plot_checkpoint(
            corrupt,copy,identity,{},&rules,&types);});
    auto document=saves::make_civis_import_document(imported.source,original.sha256);
    document.sections.push_back(section);
    const auto output=std::filesystem::current_path()/(
        "bnw-map-v5-builder-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code e;std::filesystem::remove(path,e);
    } } cleanup{output};
    context.permit_live_bnw_map_plots_overlay=true;
    auto reloaded=saves::read_saved_session(output,context);
    check(reloaded.source.live_bnw_map_plots &&
          *reloaded.source.live_bnw_map_plots==section.bytes,
          "Atomic CivisSave lost BNW builder scratchpad section");
    auto restored=reloaded.snapshot;
    runtime::restore_bnw_map_plot_checkpoint(
        section,restored,identity,{},&rules,&types);
    check(raw_field(restored.map_plots->plots[plot_id],"builder_scratch_player")==0 &&
          raw_field(restored.map_plots->plots[plot_id],"builder_scratch_turn")==1 &&
          raw_field(restored.map_plots->plots[plot_id],"builder_scratch_value")==1000 &&
          raw_field(restored.map_plots->plots[plot_id],"builder_scratch_route")==0 &&
          raw_field(reloaded.snapshot.map_plots->plots[plot_id],"builder_scratch_route")==
              raw_field(imported.snapshot.map_plots->plots[plot_id],"builder_scratch_route") &&
          runtime::identify_file(path).sha256==original.sha256,
          "Selected BNW builder scratchpad did not round-trip");
    std::cout << "selected BNW builder scratch plot=" << plot_id << '\n';
}
void test_selected_yield_cache(const std::filesystem::path& path) {
    // Arbitrary signed-short values here prove scalar-cache preservation only.
    // They do not certify calculateYield freshness or export a pending worked
    // plot operation: city terrain deltas and mandatory UI/history are absent.
    const auto original=runtime::identify_file(path).sha256;
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    const auto& source=imported.snapshot;
    check(source.map_plots&&source.map_projection&&source.map_plots->plots.size()>2,"Decoded six-yield map absent");
    const runtime::SessionCheckpointIdentity identity{runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto live=source;
    const std::array<std::array<std::int64_t,6>,2> values{{{{INT16_MIN,-1,0,1,INT16_MAX,1234}},{{1,2,3,-4,-5,-6}}}};
    for(std::size_t i=0;i<values.size();++i) {
        auto& raw=raw_array(live.map_plots->plots[i],"yields");check(raw.size()==6,"Selected short array differs");
        for(std::size_t y=0;y<6;++y){raw[y]=values[i][y];live.map_projection->cells[i].cached_yields->at(static_cast<std::int64_t>(y))=values[i][y];}
    }
    const auto section=runtime::make_bnw_map_plot_checkpoint(live,source,identity,{});
    check(section.bytes.at(8)==6&&section.bytes.size()==yield_header+2*(8+6*2*2)&&wire64(section.bytes,yield_header-8)==2,"v6 six-short cache section shape differs");
    auto restored=source;runtime::restore_bnw_map_plot_checkpoint(section,restored,identity,{});
    for(std::size_t i=0;i<values.size();++i)for(std::size_t y=0;y<6;++y)
        check(raw_array(restored.map_plots->plots[i],"yields").at(y)==values[i][y]&&restored.map_projection->cells[i].cached_yields->at(static_cast<std::int64_t>(y))==values[i][y],"Six native short/raw projection values did not roundtrip");
    check(raw_array(restored.map_plots->plots[2],"yields")==raw_array(imported.snapshot.map_plots->plots[2],"yields"),"Unchanged cache mutated");
    // Exercise the accepted post-CvCity::Read projection shape using only the
    // actual selected city roster and matching raw plot-city handles. This is
    // a codec fixture, not a claim that all production load hooks have run.
    auto hooked_source=source;
    std::size_t hooked_city_count=0;
    const auto city_scalar=[](const auto& core,std::string_view name) {
        const saves::CityCoreField* found=nullptr;
        for(const auto& field:core.fields)if(field.name&&name==field.name) {
            check(!found,"Duplicate selected city identity");found=&field;
        }
        check(found&&found->values.size()==1,"Selected city identity unavailable");
        return found->values.front();
    };
    for(const auto& player:hooked_source.all_players->players)
        for(const auto& city:player.cities.cities) {
            const auto owner=city_scalar(city.core,"m_eOwner");
            const auto id=city_scalar(city.core,"m_iID");
            const auto x=city_scalar(city.core,"m_iX"),y=city_scalar(city.core,"m_iY");
            check(owner==player.saved_player_id&&id>=0&&x>=0&&y>=0,
                  "Selected live-city identity mismatch");
            const auto index=hooked_source.map_header.grid.index(
                static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
            auto& cell=hooked_source.map_projection->cells.at(static_cast<std::size_t>(index));
            check(cell.plot_city&&cell.plot_city->owner==owner&&cell.plot_city->id==id,
                  "Selected live-city raw link mismatch");
            cell.is_city=true;++hooked_city_count;
        }
    check(hooked_city_count>0,"Selected live-city fixture is empty");
    auto hooked_live=live;
    for(std::size_t i=0;i<hooked_live.map_projection->cells.size();++i)
        hooked_live.map_projection->cells[i].is_city=hooked_source.map_projection->cells[i].is_city;
    const auto hooked_section=runtime::make_bnw_map_plot_checkpoint(hooked_live,hooked_source,identity,{});
    auto hooked_restored=hooked_source;
    runtime::restore_bnw_map_plot_checkpoint(hooked_section,hooked_restored,identity,{});
    for(std::size_t i=0;i<hooked_restored.map_projection->cells.size();++i)
        check(hooked_restored.map_projection->cells[i].is_city==
              hooked_source.map_projection->cells[i].is_city,
              "v6 yield-only restoration lost validated live city flags");
    auto malformed_city=hooked_source;
    for(auto& cell:malformed_city.map_projection->cells)if(!cell.is_city) {
        cell.is_city=true;break;
    }
    rejects([&]{runtime::restore_bnw_map_plot_checkpoint(hooked_section,malformed_city,identity,{});});
    const auto rejected_section=[&](const auto& bad) {
        auto copy=source;
        rejects([&]{runtime::restore_bnw_map_plot_checkpoint(bad,copy,identity,{});});
        for(std::size_t i=0;i<source.map_plots->plots.size();++i)
            check(raw_array(copy.map_plots->plots[i],"yields")==
                  raw_array(imported.snapshot.map_plots->plots[i],"yields")&&
                  copy.map_projection->cells[i].cached_yields==
                  imported.snapshot.map_projection->cells[i].cached_yields,
                  "Malformed section partially applied a raw/projected cache");
    };
    auto bad=section;wire64(bad.bytes,yield_header-8,source.map_plots->plots.size()+1);rejected_section(bad);
    bad=section;wire64(bad.bytes,yield_header,source.map_plots->plots.size());rejected_section(bad);
    bad=section;wire64(bad.bytes,yield_header+32,0);rejected_section(bad); // duplicate cache plot
    bad=section;bad.bytes.at(yield_header+8)^=1;rejected_section(bad); // before-image, baseline SHA still valid
    bad=section;std::copy(bad.bytes.begin()+static_cast<std::ptrdiff_t>(yield_header+8),bad.bytes.begin()+static_cast<std::ptrdiff_t>(yield_header+20),bad.bytes.begin()+static_cast<std::ptrdiff_t>(yield_header+20));rejected_section(bad); // unchanged record
    bad=section;bad.bytes.pop_back();rejected_section(bad);
    bad=section;bad.bytes.push_back(0);rejected_section(bad);
    bad=section;bad.bytes.at(old_map_header)^=1;rejected_section(bad); // GUID
    bad=section;bad.bytes.at(old_map_header+16)^=4;rejected_section(bad); // unsupported wrap bit
    bad=section;bad.bytes.at(old_map_header+24)^=1;rejected_section(bad); // full baseline SHA
    bad=section;wire64(bad.bytes,yield_header,1);wire64(bad.bytes,yield_header+32,0);
    rejected_section(bad); // descending indices
    for(const auto overflow:{std::int64_t(INT16_MIN)-1,std::int64_t(INT16_MAX)+1}) {
        auto wrong=live;raw_array(wrong.map_plots->plots.front(),"yields")[0]=overflow;wrong.map_projection->cells.front().cached_yields->at(0)=overflow;
        rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    }
    auto wrong=live;wrong.map_projection->cells.front().cached_yields->at(0)=1;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    auto wrong_source=source;wrong_source.map_projection->cells.front().cached_yields->at(0)=INT16_MAX;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(live,wrong_source,identity,{});});
    wrong=source;raw_array(wrong.map_plots->plots[2],"yields")[0]=-123;wrong.map_projection->cells[2].cached_yields->at(0)=-123;
    rejects([&]{runtime::restore_bnw_map_plot_checkpoint(section,wrong,identity,{});}); // unchanged-plot baseline digest is also mandatory
    wrong=source;wrong.map_projection->cells[0].cached_yields->at(0)=INT16_MAX;
    rejects([&]{runtime::restore_bnw_map_plot_checkpoint(section,wrong,identity,{});});
    wrong=live;++wrong.map_header.guid[0];rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong=live;raw_array(wrong.map_plots->plots[0],"units").push_back(1);
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});}); // unsupported array remains refused
    wrong=live;for(auto& a:wrong.map_plots->plots[0].arrays)if(a.name&&std::string_view(a.name)=="yields")a.columns=2;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong=live;wrong.map_projection->cells[0].cached_yields->erase(5);
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong=live;wrong.map_plots->plots[0].arrays.push_back({"yields",1,{0,0,0,0,0,0}});
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong=live;wrong.map_plots->dimensions.yields=5;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong=live;wrong.map_projection->wrap_x=!wrong.map_header.wrap_x;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong=live;++wrong.map_plots->start_offset;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong=live;++wrong.map_plots->dimensions.players;
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(wrong,source,identity,{});});
    wrong_source=source;wrong_source.map_plots->plots[0].arrays.push_back({"yields",1,{0,0,0,0,0,0}});
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(live,wrong_source,identity,{});});
    rejects([&]{runtime::restore_bnw_map_plot_checkpoint(section,wrong_source,identity,{});});
    const auto zero=runtime::make_bnw_map_plot_checkpoint(source,source,identity,{});
    for(std::uint8_t version=3;version<=5;++version) {
        auto legacy=as_v5(zero);legacy.bytes[8]=version;
        if(version==3)legacy.bytes.erase(legacy.bytes.begin()+static_cast<std::ptrdiff_t>(old_map_header-8),legacy.bytes.begin()+static_cast<std::ptrdiff_t>(old_map_header));
        auto copy=source;runtime::restore_bnw_map_plot_checkpoint(legacy,copy,identity,{});
        check(raw_array(copy.map_plots->plots[0],"yields")==raw_array(imported.snapshot.map_plots->plots[0],"yields"),"Old schema changed untouched yields");
    }
    auto document=saves::make_civis_import_document(imported.source,original);document.sections.push_back(section);
    const auto output=std::filesystem::current_path()/("bnw-map-v6-yields-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove(path,e);}}cleanup{output};
    saves::write_civis_save(output,document);context.permit_live_bnw_map_plots_overlay=true;auto reloaded=saves::read_saved_session(output,context);
    check(reloaded.source.live_bnw_map_plots&&*reloaded.source.live_bnw_map_plots==section.bytes,"CivisSave lost exact v6 yield payload");
    runtime::restore_bnw_map_plot_checkpoint(section,reloaded.snapshot,identity,{});
    for(std::size_t i=0;i<values.size();++i)for(std::size_t y=0;y<6;++y)
        check(raw_array(reloaded.snapshot.map_plots->plots[i],"yields")[y]==values[i][y]&&reloaded.snapshot.map_projection->cells[i].cached_yields->at(static_cast<std::int64_t>(y))==values[i][y],"File section roundtrip lost a short yield");
    check(runtime::identify_file(path).sha256==original,"Original BNW save changed");
    std::cout<<"selected BNW v6 exact six-short cache and legacy1..5/malformed/raw-mirror/before-image/map-identity checks\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root paths");
        test_map_turn_scalars();
        test_selected(argv[1]);
        test_selected_v2(argv[1],argv[2]);
        test_selected_map_turn(argv[1],argv[2]);
        test_selected_visibility(argv[1]);
        test_selected_builder_scratch(argv[1],argv[2]);
        test_selected_yield_cache(argv[1]);
        std::cout << "PASS BNW map plot CivisSave checkpoint\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW map plot CivisSave checkpoint: " << error.what() << '\n';
        return 1;
    }
}
