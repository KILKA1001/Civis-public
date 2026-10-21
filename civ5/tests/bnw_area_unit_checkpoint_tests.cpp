#include "civ5/runtime/bnw_area_unit_checkpoint.hpp"
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
#include "civ5/runtime/bnw_city_production_checkpoint.hpp"
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
#include "civ5/runtime/production_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/saved_terrain.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW area unit transition was accepted");
}
std::int64_t &unit_field(saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(auto &field:unit.core.fields)
        if(field.name && wanted==field.name &&
           field.encoding==saves::UnitCoreEncoding::i32 && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW unit field unavailable");
}
std::int64_t unit_field(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(const auto &field:unit.core.fields)
        if(field.name && wanted==field.name &&
           field.encoding==saves::UnitCoreEncoding::i32 && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW unit field unavailable");
}
std::int64_t &city_core_field(saves::SavedCityObservation &city,std::string_view wanted) {
    for(auto &field:city.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW city core field unavailable");
}
std::int64_t city_core_field(const saves::SavedCityObservation &city,std::string_view wanted) {
    for(const auto &field:city.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW city core field unavailable");
}
std::int64_t &city_production_field(saves::SavedCityObservation &city,
                                    std::string_view block_name,std::string_view field_name) {
    for(auto &block:city.production.blocks)
        if(block.name && block_name==block.name)
            for(auto &field:block.fields)
                if(field.name && field_name==field.name) return field.value;
    throw std::runtime_error("Selected BNW city production field unavailable");
}
std::int64_t city_production_field(const saves::SavedCityObservation &city,
                                   std::string_view block_name,std::string_view field_name) {
    for(const auto &block:city.production.blocks)
        if(block.name && block_name==block.name)
            for(const auto &field:block.fields)
                if(field.name && field_name==field.name) return field.value;
    throw std::runtime_error("Selected BNW city production field unavailable");
}
std::int64_t &city_management_field(saves::SavedCityObservation &city,
                                    std::string_view block_name,std::string_view field_name) {
    for(auto &block:city.management.blocks)
        if(block.name && block_name==block.name)
            for(auto &field:block.fields)
                if(field.name && field_name==field.name) return field.value;
    throw std::runtime_error("Selected BNW city management field unavailable");
}
std::int64_t city_management_field(const saves::SavedCityObservation &city,
                                   std::string_view block_name,std::string_view field_name) {
    for(const auto &block:city.management.blocks)
        if(block.name && block_name==block.name)
            for(const auto &field:block.fields)
                if(field.name && field_name==field.name) return field.value;
    throw std::runtime_error("Selected BNW city management field unavailable");
}
std::int64_t &city_ai_dirty(saves::SavedCityObservation &city) {
    for(auto &block:city.tail.blocks)
        if(block.name && std::string_view(block.name)=="city_ai")
            for(auto &child:block.children)
                if(child.name && std::string_view(child.name)=="m_bChooseProductionDirty" &&
                   child.values.size()==1) return child.values.front();
    throw std::runtime_error("Selected BNW CityAI dirty flag unavailable");
}
std::int64_t city_ai_dirty(const saves::SavedCityObservation &city) {
    for(const auto &block:city.tail.blocks)
        if(block.name && std::string_view(block.name)=="city_ai")
            for(const auto &child:block.children)
                if(child.name && std::string_view(child.name)=="m_bChooseProductionDirty" &&
                   child.values.size()==1) return child.values.front();
    throw std::runtime_error("Selected BNW CityAI dirty flag unavailable");
}
std::vector<std::int64_t> &plot_units(saves::PlotObservation &plot) {
    for(auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="units" && array.columns==2)
            return array.values;
    throw std::runtime_error("Selected BNW plot unit list unavailable");
}
const std::vector<std::int64_t> &plot_units(const saves::PlotObservation &plot) {
    for(const auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="units" && array.columns==2)
            return array.values;
    throw std::runtime_error("Selected BNW plot unit list unavailable");
}
std::int64_t plot_area(const saves::PlotObservation &plot) {
    for(const auto &field:plot.fields)
        if(field.name && std::string_view(field.name)=="area") return field.value;
    throw std::runtime_error("Selected BNW plot area unavailable");
}
struct Selection {
    std::size_t player=0,unit=0;
    std::uint64_t old_plot=0,new_plot=0;
    runtime::BnwAreaUnitTransfer transfer;
};
Selection select_cross_area(saves::SavedSessionSnapshot &world) {
    const auto &grid=world.map_header.grid;
    for(std::size_t p=0;p<world.all_players->players.size();++p)
        for(std::size_t u=0;u<world.all_players->players[p].units.units.size();++u) {
            auto &unit=world.all_players->players[p].units.units[u];
            const auto owner=unit_field(unit,"m_eOwner"),id=unit_field(unit,"m_iID");
            const auto x=unit_field(unit,"m_iX"),y=unit_field(unit,"m_iY");
            if(owner!=world.all_players->players[p].saved_player_id || owner<0 || id<0 ||
               x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            const auto old_plot=grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
            if(plot_units(world.map_plots->plots[old_plot])!=
               std::vector<std::int64_t>{owner,id}) continue;
            for(std::uint64_t target=0;target<grid.size();++target) {
                if(target==old_plot ||
                   plot_area(world.map_plots->plots[target])==
                       plot_area(world.map_plots->plots[old_plot]) ||
                   !plot_units(world.map_plots->plots[target]).empty()) continue;
                try {
                    auto transfer=runtime::plan_bnw_area_unit_transfer(
                        world,owner,old_plot,target);
                    return {p,u,old_plot,target,transfer};
                } catch(const std::exception &) {}
            }
        }
    throw std::runtime_error("Selected Gandhi save lacks a cross-area accounting fixture");
}
void test_selected(const std::filesystem::path &path,
                   const std::optional<std::filesystem::path> &game_root) {
    const auto original=runtime::identify_file(path);
    // Projection must use the SAME activated feature catalog as LiveSession's
    // subsequent source-bound v6 map overlay validation. Bootstrap before decode.
    std::optional<ui::SelectedBnwBootstrapResult> bootstrap;
    saves::SavedTerrainFeatureResolver features;
    if(game_root) {
        bootstrap.emplace(ui::bootstrap_selected_bnw_preview(path,*game_root,0,true));
        check(bootstrap->preview.host && !bootstrap->preview.blocker,
              "Selected BNW source bootstrap did not activate rules");
        features=content::saved_terrain_features(bootstrap->preview.host->database(),
                                                bootstrap->preview.host->types()).resolve;
    }
    const auto *database=bootstrap ? &bootstrap->preview.host->database() : nullptr;
    saves::SavedSessionDecodeContext context;
    context.terrain_features=features;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.map_regions && imported.snapshot.map_plots &&
          imported.snapshot.map_projection && imported.snapshot.all_players,
          "Selected Gandhi save lacks decoded BNW areas/units");
    const auto selected=select_cross_area(imported.snapshot);
    auto live=imported.snapshot;
    auto &unit=live.all_players->players[selected.player].units.units[selected.unit];
    const auto xy=live.map_header.grid.coordinates(selected.new_plot);
    check(xy[0]<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) &&
          xy[1]<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
          "Selected target coordinate exceeds signed unit field");
    const auto target_x=static_cast<std::int64_t>(xy[0]);
    const auto target_y=static_cast<std::int64_t>(xy[1]);
    unit_field(unit,"m_iX")=target_x;unit_field(unit,"m_iY")=target_y;
    const auto owner=selected.transfer.owner,id=unit_field(unit,"m_iID");
    plot_units(live.map_plots->plots[selected.old_plot]).clear();
    plot_units(live.map_plots->plots[selected.new_plot])={owner,id};
    live.map_projection->cells[selected.old_plot].units=
        std::vector<saves::SavedPlotUnitReference>{};
    live.map_projection->cells[selected.new_plot].units=
        std::vector<saves::SavedPlotUnitReference>{{owner,id}};
    runtime::apply_bnw_area_unit_transfer(live,selected.transfer);
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        database ? database->gameplay_fingerprint_sha256() :
                   std::string(64,'a')};
    const auto triplet=runtime::make_bnw_map_unit_area_checkpoint_triplet(
        live,imported.snapshot,identity,selected.transfer,features);
    std::int64_t core_proximity_after=0;
    bool changed_core=false;
    for(auto &part:live.all_players->players[0].core.fields)
        if(part.name && std::string_view(part.name)=="m_aiProximityToPlayer" &&
           part.values.size()>1) {
            part.values[1]=part.values[1]==3?2:3;
            core_proximity_after=part.values[1];
            changed_core=true;
        }
    check(changed_core,"Selected BNW player proximity core field unavailable");
    const auto core_section=runtime::make_bnw_player_core_checkpoint(
        live,imported.snapshot,identity);
    std::size_t city_player=64;
    for(std::size_t p=0;p<live.all_players->players.size();++p)
        if(!live.all_players->players[p].cities.cities.empty()) {
            city_player=p;break;
        }
    check(city_player<64,"Selected BNW city checkpoint fixture has no city");
    auto &changed_city=live.all_players->players[city_player].cities.cities[0];
    const auto city_food_after=++city_core_field(changed_city,"m_iFood");
    const auto city_overflow_after=++city_core_field(
        changed_city,"m_iOverflowProduction");
    const auto building_modifier_after=++city_production_field(
        changed_city,"buildings","m_iBuildingProductionModifier");
    auto &automated=city_management_field(changed_city,"citizens","m_bAutomated");
    automated=1-automated;
    const auto city_automated_after=automated;
    auto &city_dirty=city_ai_dirty(changed_city);
    city_dirty=1-city_dirty;
    const auto city_dirty_after=city_dirty;
    const auto city_quadruple=runtime::make_bnw_city_checkpoint_quadruple(
        live,imported.snapshot,identity);
    check(triplet.area.id==runtime::bnw_area_unit_checkpoint_section_id &&
          triplet.area.bytes.at(8)==1,"BNW area checkpoint ID or version changed");
    auto bad=imported.snapshot;
    auto &regions=*bad.map_regions;
    bool changed=false;
    for(auto &container:regions.containers)
        if(container.name && std::string_view(container.name)=="areas")
            for(auto &record:container.records) {
                bool old=false;
                for(const auto &field:record.fields)
                    if(field.name && std::string_view(field.name)=="id" &&
                       field.value==selected.transfer.from_area) old=true;
                if(!old) continue;
                for(auto &array:record.arrays)
                    if(array.name && std::string_view(array.name)=="units_per_player") {
                        array.values[static_cast<std::size_t>(owner)]=0;
                        changed=true;
                    }
            }
    check(changed,"Selected BNW area counter unavailable");
    rejects([&]{(void)runtime::plan_bnw_area_unit_transfer(
        bad,owner,selected.old_plot,selected.new_plot);});
    auto wrong=identity;wrong.rules_sha256=std::string(64,'b');
    rejects([&]{auto copy=imported.snapshot;
        runtime::restore_bnw_area_unit_checkpoint(triplet.area,copy,wrong);});
    auto unsupported=live;
    for(auto &container:unsupported.map_regions->containers)
        if(container.name && std::string_view(container.name)=="areas") {
            for(auto &field:container.records.front().fields)
                if(field.name && std::string_view(field.name)=="cities") ++field.value;
            break;
        }
    rejects([&]{(void)runtime::make_bnw_area_unit_checkpoint(
        unsupported,imported.snapshot,identity,selected.transfer);});
    auto document=saves::make_civis_import_document(imported.source,original.sha256);
    document.sections.push_back(triplet.map);
    document.sections.push_back(triplet.units);
    document.sections.push_back(triplet.area);
    document.sections.push_back(core_section);
    document.sections.push_back(city_quadruple.core);
    document.sections.push_back(city_quadruple.production);
    document.sections.push_back(city_quadruple.management);
    document.sections.push_back(city_quadruple.tail);
    const auto output=std::filesystem::current_path()/(
        "bnw-area-unit-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code error;std::filesystem::remove(path,error);
    }} cleanup{output};
    const auto reloaded=saves::read_civis_save(output);
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_map_plots_overlay=true;
    context.permit_live_bnw_units_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_area_units_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_player_core_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_city_core_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_city_production_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_city_management_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_city_tail_overlay=true;
    auto imported_triplet=saves::read_saved_session(output,context);
    check(imported_triplet.source.live_bnw_map_plots==triplet.map.bytes &&
          imported_triplet.source.live_bnw_units==triplet.units.bytes &&
          imported_triplet.source.live_bnw_area_units==triplet.area.bytes &&
          imported_triplet.source.live_bnw_player_core==core_section.bytes &&
          imported_triplet.source.live_bnw_city_core==city_quadruple.core.bytes &&
          imported_triplet.source.live_bnw_city_production==city_quadruple.production.bytes &&
          imported_triplet.source.live_bnw_city_management==city_quadruple.management.bytes &&
          imported_triplet.source.live_bnw_city_tail==city_quadruple.tail.bytes,
          "Overlay-aware import lost the BNW area, unit, map, player or city sections");
    const saves::CivisSaveSection *map=nullptr,*units=nullptr,*area=nullptr;
    for(const auto &section:reloaded.sections) {
        if(section.id==triplet.map.id) map=&section;
        if(section.id==triplet.units.id) units=&section;
        if(section.id==triplet.area.id) area=&section;
    }
    check(map && units && area && map->bytes==triplet.map.bytes &&
          units->bytes==triplet.units.bytes && area->bytes==triplet.area.bytes,
          "Atomic CivisSave lost BNW area/unit/map sections");
    auto restored=imported.snapshot;
    rejects([&]{runtime::restore_bnw_map_unit_area_checkpoint_triplet(
        map,units,nullptr,restored,identity,features);});
    check(plot_units(restored.map_plots->plots[selected.old_plot])==
          plot_units(imported.snapshot.map_plots->plots[selected.old_plot]),
          "Missing area section changed owned world");
    auto corrupt=*area;corrupt.bytes.back()^=1;
    rejects([&]{runtime::restore_bnw_map_unit_area_checkpoint_triplet(
        map,units,&corrupt,restored,identity,features);});
    check(plot_units(restored.map_plots->plots[selected.old_plot])==
          plot_units(imported.snapshot.map_plots->plots[selected.old_plot]),
          "Failed area section published partial unit relocation");
    bool found_same_area=false;
    for(std::uint64_t other=0;other<imported.snapshot.map_header.grid.size();++other) {
        if(other==selected.old_plot ||
           plot_area(imported.snapshot.map_plots->plots[other])!=
               selected.transfer.from_area) continue;
        auto mismatched=*area;
        constexpr std::size_t from_plot_offset=8+4+64+64+8+8+8;
        for(unsigned byte=0;byte<8;++byte)
            mismatched.bytes[from_plot_offset+byte]=
                static_cast<std::uint8_t>(other>>(8*byte));
        rejects([&]{runtime::restore_bnw_map_unit_area_checkpoint_triplet(
            map,units,&mismatched,restored,identity,features);});
        found_same_area=true;
        break;
    }
    check(found_same_area,"Selected BNW source area lacks alternate plot for binding test");
    runtime::restore_bnw_map_unit_area_checkpoint_triplet(
        map,units,area,restored,identity,features);
    check(unit_field(restored.all_players->players[selected.player].units.units[selected.unit],
                     "m_iX")==target_x &&
          plot_units(restored.map_plots->plots[selected.old_plot]).empty() &&
          plot_units(restored.map_plots->plots[selected.new_plot])==
              std::vector<std::int64_t>{owner,id} &&
          runtime::identify_file(path).sha256==original.sha256,
          "BNW area/unit/map sections did not round-trip atomically");
    const auto reversed=runtime::plan_bnw_area_unit_transfer(
        restored,owner,selected.new_plot,selected.old_plot);
    check(reversed.from_units==selected.transfer.to_units+1 &&
          reversed.to_units==selected.transfer.from_units-1 &&
          reversed.from_owner_units==selected.transfer.to_owner_units+1 &&
          reversed.to_owner_units==selected.transfer.from_owner_units-1,
          "BNW area totals or per-player counters did not restore");
    if(database) {
        auto types=database->cache_info_types();
        auto defines=database->cache_global_defines(content::GlobalDefineProfile::bnw);
        auto technologies=content::load_technology_rules(*database,types,
            content::TechnologyProfile::bnw,5);
        auto city_yields=runtime::load_city_yield_rules(*database,
            content::TechnologyProfile::bnw);
        auto production=runtime::load_production_progress_rules(*database,
            content::TechnologyProfile::bnw);
        auto maintenance=runtime::load_maintenance_rules(*database,
            content::TechnologyProfile::bnw,types);
        auto treasury=runtime::load_treasury_rules(*database,
            content::TechnologyProfile::bnw,types);
        runtime::LiveSessionImportRules rules{*database,types,technologies,defines,
            city_yields,production,maintenance,treasury,identity};
        auto inconsistent=imported_triplet;
        const auto old_progress=runtime::make_production_checkpoint(
            bootstrap->preview.host->session().state().cities,production,types);
        inconsistent.source.live_city_production=old_progress.bytes;
        rejects([&]{(void)runtime::LiveSession::restore_bnw(
            std::move(inconsistent),rules);});
        auto consistent=imported_triplet;
        auto session=runtime::LiveSession::restore_bnw(std::move(imported_triplet),rules);
        const auto &world=session.state().world;
        check(world.history.sessions==consistent.snapshot.history.sessions+1 &&
              session.source().snapshot.history.sessions==
                  consistent.snapshot.history.sessions,
              "BNW CvGame::Read session count was not applied to the owned world");
        const auto &restored_city=world.all_players->players[city_player].cities.cities[0];
        std::int64_t restored_proximity=-1;
        for(const auto &part:world.all_players->players[0].core.fields)
            if(part.name && std::string_view(part.name)=="m_aiProximityToPlayer" &&
               part.values.size()>1) restored_proximity=part.values[1];
        check(unit_field(
                  world.all_players->players[selected.player].units.units[selected.unit],
                  "m_iX")==target_x &&
              plot_units(world.map_plots->plots[selected.old_plot]).empty() &&
              restored_proximity==core_proximity_after &&
              city_core_field(restored_city,"m_iFood")==city_food_after &&
              city_core_field(restored_city,"m_iOverflowProduction")==
                  city_overflow_after &&
              city_production_field(restored_city,"buildings",
                                    "m_iBuildingProductionModifier")==building_modifier_after &&
              city_management_field(restored_city,"citizens","m_bAutomated")==
                  city_automated_after &&
              city_ai_dirty(restored_city)==city_dirty_after &&
              !session.readiness().can_continue(),
              "LiveSession did not compose the BNW map, unit, player and city checkpoints");
        consistent.source.live_city_production=runtime::make_production_checkpoint(
            session.state().cities,production,types).bytes;
        auto reloaded=runtime::LiveSession::restore_bnw(std::move(consistent),rules);
        check(city_core_field(
                  reloaded.state().world.all_players->players[city_player]
                      .cities.cities[0],"m_iOverflowProduction")==city_overflow_after &&
              !reloaded.readiness().can_continue(),
              "Consistent raw and live production sections did not restore");
    }
    std::cout << "selected BNW area " << selected.transfer.from_area << " -> " <<
        selected.transfer.to_area << " for unit " << owner << ':' << id << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2 || argc==3,"Supply selected Gandhi save and optional game root");
        test_selected(argv[1],argc==3 ?
            std::optional<std::filesystem::path>(argv[2]) : std::nullopt);
        std::cout << "PASS BNW area unit checkpoint\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW area unit checkpoint: " << error.what() << '\n';
        return 1;
    }
}
