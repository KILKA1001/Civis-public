#include "civ5/runtime/bnw_unit_jump_composite.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <climits>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::int64_t &core(saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(auto &part:unit.core.fields)
        if(part.name && wanted==part.name && part.values.size()==1)
            return part.values[0];
    throw std::runtime_error("missing BNW unit core value");
}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(const auto &part:unit.core.fields)
        if(part.name && wanted==part.name && part.values.size()==1)
            return part.values[0];
    throw std::runtime_error("missing BNW unit core value");
}
std::int64_t plot_field(const saves::PlotObservation &plot,std::string_view wanted) {
    for(const auto &part:plot.fields)
        if(part.name && wanted==part.name) return part.value;
    throw std::runtime_error("missing BNW plot field");
}
std::vector<std::int64_t> &plot_units(saves::PlotObservation &plot) {
    for(auto &part:plot.arrays)
        if(part.name && std::string_view(part.name)=="units")
            return part.values;
    throw std::runtime_error("missing BNW plot unit roster");
}
const std::vector<std::int64_t> &plot_units(const saves::PlotObservation &plot) {
    for(const auto &part:plot.arrays)
        if(part.name && std::string_view(part.name)=="units")
            return part.values;
    throw std::runtime_error("missing BNW plot unit roster");
}
std::vector<std::int64_t> &plot_array(
    saves::PlotObservation &plot,std::string_view wanted) {
    for(auto &part:plot.arrays)
        if(part.name && wanted==part.name) return part.values;
    throw std::runtime_error("missing BNW sight plot array");
}
std::vector<std::int64_t> &area_reveals(
    saves::SavedSessionSnapshot &world,std::int64_t wanted) {
    for(auto &container:world.map_regions->containers)
        if(container.name && std::string_view(container.name)=="areas")
            for(auto &record:container.records) {
                bool match=false;
                for(const auto &part:record.fields)
                    if(part.name && std::string_view(part.name)=="id" &&
                       part.value==wanted) match=true;
                if(match)
                    for(auto &part:record.arrays)
                        if(part.name &&
                           std::string_view(part.name)=="revealed_tiles")
                            return part.values;
            }
    throw std::runtime_error("missing BNW area reveal array");
}
struct Selected {
    runtime::BnwUnitJumpCheckpointTarget target;
    runtime::BnwAreaUnitTransfer transfer;
};
Selected choose(const saves::SavedSessionSnapshot &world) {
    const auto &grid=world.map_header.grid;
    const auto &player=world.all_players->players.at(0);
    for(std::size_t u=0;u<player.units.units.size();++u) {
        const auto &unit=player.units.units[u];
        const auto owner=core(unit,"m_eOwner"),id=core(unit,"m_iID");
        const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
        if(owner!=0 || id<0 || x<0 || y<0 ||
           x>=static_cast<std::int64_t>(grid.width) ||
           y>=static_cast<std::int64_t>(grid.height)) continue;
        const auto from=grid.index(x,y);
        if(plot_units(world.map_plots->plots[from])!=
           std::vector<std::int64_t>{owner,id}) continue;
        for(std::uint64_t to=0;to<grid.size();++to) {
            if(to==from || !plot_units(world.map_plots->plots[to]).empty() ||
               plot_field(world.map_plots->plots[to],"area")==
                   plot_field(world.map_plots->plots[from],"area"))
                continue;
            try {
                auto transfer=runtime::plan_bnw_area_unit_transfer(
                    world,owner,from,to);
                return {{0,u,from,to},transfer};
            } catch(const std::exception &) {}
        }
    }
    throw std::runtime_error("Selected Gandhi has no player-0 cross-area unit fixture");
}
void selected_test(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    const auto source=imported.snapshot;
    const auto selected=choose(source);
    auto world=source;
    auto &unit=world.all_players->players[selected.target.player_index]
                    .units.units[selected.target.unit_index];
    const auto xy=world.map_header.grid.coordinates(selected.target.to_plot);
    const auto owner=core(unit,"m_eOwner"),id=core(unit,"m_iID");
    core(unit,"m_iX")=xy[0];core(unit,"m_iY")=xy[1];
    plot_units(world.map_plots->plots[selected.target.from_plot]).clear();
    plot_units(world.map_plots->plots[selected.target.to_plot])={owner,id};
    world.map_projection->cells[selected.target.from_plot].units=
        std::vector<saves::SavedPlotUnitReference>{};
    world.map_projection->cells[selected.target.to_plot].units=
        std::vector<saves::SavedPlotUnitReference>{{owner,id}};
    runtime::apply_bnw_area_unit_transfer(world,selected.transfer);

    std::optional<std::size_t> revealed_plot;
    for(std::size_t i=0;i<world.map_plots->plots.size();++i) {
        auto &plot=world.map_plots->plots[i];
        auto &cell=world.map_projection->cells[i];
        auto &visibility=plot_array(plot,"visibility");
        auto &invisible=plot_array(plot,"invisible_visibility");
        if(!cell.revealed_teams || cell.revealed_teams->contains(0) ||
           visibility[0]!=0 || invisible[0]>=INT16_MAX ||
           plot_field(plot,"area")<0) continue;
        auto &count=area_reveals(world,plot_field(plot,"area"))[0];
        if(count>=INT32_MAX) continue;
        visibility[0]=1;++invisible[0];
        plot_array(plot,"revealed_words")[0]|=1;
        cell.revealed_teams->insert(0);
        ++count;revealed_plot=i;break;
    }
    check(revealed_plot.has_value(),
          "Selected Gandhi has no first-reveal sight fixture");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        std::string(64,'a')};
    const auto sections=runtime::make_bnw_unit_jump_composite(
        world,source,identity,selected.target,selected.transfer,0);
    check(!sections.can_commit() && sections.unit.id==41 &&
          sections.area.id==19 && sections.sight.id==42,
          "BNW jump composition accepted gameplay commit or lost a section");
    auto document=saves::make_civis_import_document(
        imported.source,identity.payload_sha256);
    document.sections.push_back(sections.unit);
    document.sections.push_back(sections.area);
    document.sections.push_back(sections.sight);
    const auto output=path.parent_path()/(
        "bnw-jump-composite-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code error;std::filesystem::remove(path,error);
    }} cleanup{output};
    const auto on_disk=saves::read_civis_save(output);
    runtime::BnwUnitJumpCompositeSections encoded;
    std::size_t seen=0;
    for(const auto &part:on_disk.sections) {
        if(part.id==41) {encoded.unit=part;++seen;}
        if(part.id==19) {encoded.area=part;++seen;}
        if(part.id==42) {encoded.sight=part;++seen;}
    }
    check(seen==3 && encoded.unit.bytes==sections.unit.bytes &&
          encoded.area.bytes==sections.area.bytes &&
          encoded.sight.bytes==sections.sight.bytes,
          "BNW composed jump sections did not survive one CivisSave file");
    auto restored=source;
    runtime::restore_bnw_unit_jump_composite(
        encoded,restored,identity,selected.target,selected.transfer,0);
    check(core(restored.all_players->players[0]
                  .units.units[selected.target.unit_index],"m_iX")==static_cast<std::int64_t>(xy[0]) &&
          restored.map_projection->cells[selected.target.from_plot].units->empty() &&
          restored.map_projection->cells[selected.target.to_plot].units->size()==1 &&
          plot_array(restored.map_plots->plots[*revealed_plot],"visibility")[0]==1 &&
          restored.map_projection->cells[*revealed_plot]
              .revealed_teams->contains(0) &&
          area_reveals(restored,plot_field(restored.map_plots->plots[*revealed_plot],
                                          "area"))[0]==
          area_reveals(world,plot_field(world.map_plots->plots[*revealed_plot],
                                       "area"))[0],
          "Selected Gandhi composed jump lost unit, area, or sight data");
    auto broken=sections;broken.sight.bytes.back()^=1;
    auto intact=source;
    bool rejected=false;
    try {runtime::restore_bnw_unit_jump_composite(
        broken,intact,identity,selected.target,selected.transfer,0);}
    catch(const std::exception &) {rejected=true;}
    check(rejected &&
          core(intact.all_players->players[0]
                   .units.units[selected.target.unit_index],"m_iX")==
          core(source.all_players->players[0]
                    .units.units[selected.target.unit_index],"m_iX") &&
          plot_array(intact.map_plots->plots[*revealed_plot],"visibility")[0]==0,
          "Corrupt sight section partially restored the composed jump");
    std::cout<<"selected Gandhi composed synthetic jump "
             <<selected.target.from_plot<<" -> "<<selected.target.to_plot
             <<", sight "<<*revealed_plot<<" PASS\n";
}
}
int main(int argc,char **argv) {
    check(argc==2,"Supply selected Gandhi BNW save path");
    selected_test(argv[1]);
}
