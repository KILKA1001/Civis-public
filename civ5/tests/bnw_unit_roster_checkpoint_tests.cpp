#include "civ5/runtime/bnw_unit_roster_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW unit roster deletion was accepted");
}
std::int64_t header(const saves::UnitCollectionHeader &h,std::string_view wanted) {
    for(const auto &field:h.fields)
        if(field.name && wanted==field.name) return field.value;
    throw std::runtime_error("Selected BNW unit free-list field missing");
}
void set_header(saves::UnitCollectionHeader &h,std::string_view wanted,
                std::int64_t value) {
    for(auto &field:h.fields)
        if(field.name && wanted==field.name) {field.value=value;return;}
    throw std::runtime_error("Selected BNW unit free-list field missing");
}
std::int64_t area_field(const saves::MapRegionRecord &area,std::string_view wanted) {
    for(const auto &field:area.fields)
        if(field.name && wanted==field.name) return field.value;
    throw std::runtime_error("Selected BNW area field missing");
}
void set_area_field(saves::MapRegionRecord &area,std::string_view wanted,
                    std::int64_t value) {
    for(auto &field:area.fields)
        if(field.name && wanted==field.name) {field.value=value;return;}
    throw std::runtime_error("Selected BNW area field missing");
}
saves::MapRegionContainer &areas(saves::SavedSessionSnapshot &world) {
    for(auto &container:world.map_regions->containers)
        if(container.name && std::string_view(container.name)=="areas") return container;
    throw std::runtime_error("Selected BNW areas missing");
}
runtime::BnwUnitRemoval choose(const saves::SavedSessionSnapshot &world) {
    for(std::size_t p=0;p<world.all_players->players.size();++p)
        for(std::size_t u=0;u<world.all_players->players[p].units.units.size();++u)
            try {return runtime::plan_bnw_unit_removal(world,p,u);}
            catch(const std::exception &) {}
    throw std::runtime_error("Selected Gandhi save has no source-bound unit removal fixture");
}
void synthetic() {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    saves::SavedSessionSnapshot source{};
    source.game_state.layout=bnw;source.map_header.layout=bnw;
    source.map_header.grid={1,1};
    source.map_plots.emplace();source.map_plots->layout=bnw;
    source.map_plots->plots.emplace_back();
    source.map_plots->plots[0].fields.push_back({"area",0});
    source.map_plots->plots[0].arrays.push_back({"units",2,{0,0}});
    source.map_projection.emplace();source.map_projection->grid={1,1};
    source.map_projection->cells.emplace_back();
    source.map_projection->cells[0].units=
        std::vector<saves::SavedPlotUnitReference>{{0,0}};
    source.map_regions.emplace();source.map_regions->layout=bnw;
    source.map_regions->containers.emplace_back();
    auto &container=source.map_regions->containers.back();
    container.name="areas";
    container.wire_count=1;container.records.emplace_back();
    auto &area=container.records.back();
    area.fields={{"id",0},{"units",1}};
    auto owners=std::vector<std::int64_t>(64);owners[0]=1;
    area.arrays.push_back({"units_per_player",1,std::move(owners)});
    source.all_players.emplace();source.all_players->layout=bnw;
    source.all_players->players.resize(64);
    for(std::size_t p=0;p<64;++p) {
        auto &player=source.all_players->players[p];
        player.index=p;player.saved_player_id=p;
        player.units.layout=bnw;player.units.header.layout=bnw;
    }
    auto &units=source.all_players->players[0].units;
    units.header.source_slots=1;
    units.header.fields={{"num_slots",1},{"last_index",0},
        {"free_list_head",-1},{"free_list_count",0},{"current_id",8192}};
    units.header.next_free_indices={-1};units.header.wire_unit_count=1;
    units.units.emplace_back();
    auto &unit=units.units.back();unit.core.layout=bnw;unit.tail.layout=bnw;
    for(const auto &[name,value]:std::array<std::pair<const char*,std::int64_t>,4>{
            {{"m_iID",0},{"m_eOwner",0},{"m_iX",0},{"m_iY",0}}}) {
        saves::UnitCoreField field{};
        field.name=name;field.encoding=saves::UnitCoreEncoding::i32;
        field.values.push_back(value);unit.core.fields.push_back(std::move(field));
    }
    const auto removal=runtime::plan_bnw_unit_removal(source,0,0);
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto section=runtime::make_bnw_unit_roster_removal_checkpoint(
        source,identity,removal);
    auto result=runtime::restore_bnw_unit_roster_removal_checkpoint(
        section,source,identity);
    check(result.all_players->players[0].units.units.empty() &&
          result.all_players->players[0].units.header.wire_unit_count==0 &&
          header(result.all_players->players[0].units.header,"free_list_head")==0 &&
          result.map_plots->plots[0].arrays[0].values.empty() &&
          area_field(areas(result).records[0],"units")==0,
          "Synthetic last-unit deletion did not update roster/plot/area");
    check(source.all_players->players[0].units.units.size()==1,
          "Synthetic source was mutated");
    source.all_players->players[0].units.header.next_free_indices[0]=0;
    rejects([&]{(void)runtime::plan_bnw_unit_removal(source,0,0);});
}
void selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.map_plots && imported.snapshot.map_projection &&
          imported.snapshot.map_regions && imported.snapshot.all_players,
          "Selected Gandhi save lacks full unit/plot/area snapshot");
    const auto identity=runtime::SessionCheckpointIdentity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto source=std::move(imported.snapshot);
    const auto removal=choose(source);
    const auto &before=source.all_players->players[removal.player_index].units;
    const auto old_count=before.units.size();
    const auto old_head=header(before.header,"free_list_head");
    const auto old_free=header(before.header,"free_list_count");
    const auto old_serial=header(before.header,"current_id");
    const auto old_last=header(before.header,"last_index");
    const auto old_area=area_field(areas(source).records[removal.area_index],"units");
    auto checkpoint=runtime::make_bnw_unit_roster_removal_checkpoint(
        source,identity,removal);
    check(checkpoint.id==runtime::bnw_unit_roster_checkpoint_section_id,
          "BNW unit roster checkpoint section ID changed");
    {
        auto projected=runtime::project_bnw_unit_removal(source,removal);
        const auto &after=projected.all_players->players[removal.player_index].units;
        check(after.units.size()+1==old_count &&
              after.header.wire_unit_count+1==before.header.wire_unit_count &&
              header(after.header,"free_list_head")==
                  (removal.id&8191) &&
              header(after.header,"free_list_count")==old_free+1 &&
              header(after.header,"current_id")==old_serial &&
              header(after.header,"last_index")==old_last &&
              after.header.next_free_indices[removal.id&8191]==old_head,
              "SDK RemoveAt free-list or unit roster projection changed");
        check(area_field(areas(projected).records[removal.area_index],"units")==
                  old_area-1,
              "BNW area unit count was not decremented");
        check(projected.map_projection->cells[removal.plot_index].units->size()+1==
                  source.map_projection->cells[removal.plot_index].units->size(),
              "BNW plot unit handle was not removed");
        check(before.units.size()==old_count &&
              header(before.header,"free_list_head")==old_head,
              "Detached BNW projection changed source snapshot");
    }
    const auto output=std::filesystem::current_path()/(
        "bnw-unit-roster-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code ignored;std::filesystem::remove(path,ignored);
    }} cleanup{output};
    {
        auto document=saves::make_civis_import_document(imported.source,original.sha256);
        document.sections.push_back(checkpoint);
        saves::write_civis_save(output,document);
    }
    {
        const auto reopened=saves::read_civis_save(output);
        const saves::CivisSaveSection *read=nullptr;
        for(const auto &section:reopened.sections)
            if(section.id==checkpoint.id) read=&section;
        check(read && read->bytes==checkpoint.bytes,
              "Atomic CivisSave unit roster round-trip failed");
        runtime::reject_bnw_unit_removal_companion_overlap(reopened.sections);
        auto restored=runtime::restore_bnw_unit_roster_removal_checkpoint(
            *read,source,identity);
        check(restored.all_players->players[removal.player_index].units.units.size()+1==
                  old_count &&
              area_field(areas(restored).records[removal.area_index],"units")==
                  old_area-1,
              "BNW unit deletion projection did not restore");
        auto conflict=reopened.sections;
        for(const auto id:{17u,18u,19u}) {
            conflict.push_back({id,0,{1}});
            rejects([&]{runtime::reject_bnw_unit_removal_companion_overlap(conflict);});
            conflict.pop_back();
        }
        conflict.push_back(checkpoint);
        rejects([&]{runtime::reject_bnw_unit_removal_companion_overlap(conflict);});
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW unit checkpoint changed original Civ5Save");
    {
        auto bad=checkpoint;bad.bytes.pop_back();
        rejects([&]{(void)runtime::restore_bnw_unit_roster_removal_checkpoint(
            bad,source,identity);});
        bad=checkpoint;bad.bytes[8]=2;
        rejects([&]{(void)runtime::restore_bnw_unit_roster_removal_checkpoint(
            bad,source,identity);});
        auto foreign=identity;foreign.rules_sha256[0]='b';
        rejects([&]{(void)runtime::restore_bnw_unit_roster_removal_checkpoint(
            checkpoint,source,foreign);});
    }
    {
        auto bad_source=source;
        auto &h=bad_source.all_players->players[removal.player_index].units.header;
        set_header(h,"current_id",old_serial-1);
        rejects([&]{(void)runtime::plan_bnw_unit_removal(
            bad_source,removal.player_index,removal.unit_index);});
        bad_source=source;
        set_area_field(areas(bad_source).records[removal.area_index],"units",0);
        rejects([&]{(void)runtime::plan_bnw_unit_removal(
            bad_source,removal.player_index,removal.unit_index);});
        bad_source=source;
        bad_source.map_projection->cells[removal.plot_index].units->push_back(
            {removal.owner,removal.id});
        rejects([&]{(void)runtime::make_bnw_unit_roster_removal_checkpoint(
            bad_source,identity,removal);});
    }
    std::cout << "PASS BNW unit roster removal checkpoint projection\n";
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        synthetic();
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW unit roster checkpoint: " << error.what() << '\n';
        return 1;
    }
}
