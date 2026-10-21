#include "civ5/runtime/bnw_unit_sight_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
saves::PlotArrayObservation &array(saves::PlotObservation &plot,std::string_view name) {
    for(auto &part:plot.arrays)
        if(part.name && std::string_view(part.name)==name) return part;
    throw std::runtime_error("missing sight plot array");
}
saves::MapRegionRecord &area(saves::SavedSessionSnapshot &world) {
    for(auto &container:world.map_regions->containers)
        if(container.name && std::string_view(container.name)=="areas")
            return container.records.at(0);
    throw std::runtime_error("missing sight area");
}
saves::PlotArrayObservation &revealed_area(saves::MapRegionRecord &record) {
    for(auto &part:record.arrays)
        if(part.name && std::string_view(part.name)=="revealed_tiles")
            return part;
    throw std::runtime_error("missing area reveals");
}
saves::SavedSessionSnapshot synthetic() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=world.map_header.layout=bnw;
    world.map_header.grid={2,1};
    world.map_plots.emplace();world.map_plots->layout=bnw;
    world.map_projection.emplace();world.map_projection->grid=world.map_header.grid;
    world.map_regions.emplace();world.map_regions->layout=bnw;
    saves::MapRegionContainer areas;
    areas.name="areas";areas.wire_count=1;
    saves::MapRegionRecord record;
    record.fields.push_back({"id",1});
    record.arrays.push_back({"revealed_tiles",1,std::vector<std::int64_t>(80)});
    areas.records.push_back(record);
    world.map_regions->containers.push_back(areas);
    for(std::uint64_t index=0;index<2;++index) {
        saves::PlotObservation plot;
        plot.index=plot.x=index;plot.y=0;
        plot.arrays.push_back({"visibility",1,std::vector<std::int64_t>(80)});
        plot.arrays.push_back({"invisible_visibility",1,
                               std::vector<std::int64_t>(80)});
        plot.arrays.push_back({"revealed_words",1,
                               std::vector<std::int64_t>(4)});
        world.map_plots->plots.push_back(plot);
        saves::SavedMapCellProjection cell;
        cell.index=cell.x=index;cell.y=0;
        cell.revealed_teams=std::set<std::int64_t>{};
        world.map_projection->cells.push_back(cell);
    }
    return world;
}
void synthetic_test() {
    const auto source=synthetic();
    auto target=source;
    auto &plot=target.map_plots->plots[1];
    array(plot,"visibility").values[3]=1;
    array(plot,"invisible_visibility").values[3]=1;
    array(plot,"revealed_words").values[0]=8;
    target.map_projection->cells[1].revealed_teams->insert(3);
    revealed_area(area(target)).values[3]=1;
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto section=runtime::make_bnw_unit_sight_checkpoint(
        target,source,identity,3);
    check(section.id==runtime::bnw_unit_sight_checkpoint_section_id,
          "BNW sight section ID changed");
    auto restored=source;
    runtime::restore_bnw_unit_sight_checkpoint(section,restored,identity);
    check(array(restored.map_plots->plots[1],"visibility").values[3]==1 &&
          array(restored.map_plots->plots[1],"invisible_visibility").values[3]==1 &&
          restored.map_projection->cells[1].revealed_teams->contains(3) &&
          revealed_area(area(restored)).values[3]==1,
          "BNW sight checkpoint lost counters or reveal");
    auto stale=source;
    array(stale.map_plots->plots[1],"visibility").values[3]=2;
    bool rejected=false;
    try {runtime::restore_bnw_unit_sight_checkpoint(section,stale,identity);}
    catch(const std::exception &) {rejected=true;}
    check(rejected && array(stale.map_plots->plots[1],"visibility").values[3]==2,
          "BNW sight stale source changed or was accepted");
    auto corrupt=section;
    corrupt.bytes.back()^=1;
    auto intact=source;
    rejected=false;
    try {runtime::restore_bnw_unit_sight_checkpoint(corrupt,intact,identity);}
    catch(const std::exception &) {rejected=true;}
    check(rejected && array(intact.map_plots->plots[1],"visibility").values[3]==0,
          "BNW sight corrupt target changed or was accepted");
    auto foreign=target;
    array(foreign.map_plots->plots[1],"visibility").values[4]=1;
    rejected=false;
    try {(void)runtime::make_bnw_unit_sight_checkpoint(
        foreign,source,identity,3);}
    catch(const std::exception &) {rejected=true;}
    check(rejected,"BNW sight codec accepted another team's change");
}
void selected_test(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_all_players=true;
    context.decode_teams=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    const auto source=imported.snapshot;
    auto changed=source;
    std::optional<std::size_t> chosen;
    for(std::size_t i=0;i<changed.map_plots->plots.size();++i) {
        auto &values=array(changed.map_plots->plots[i],"visibility").values;
        if(values[0]>=0 && values[0]<32767) {++values[0];chosen=i;break;}
    }
    check(chosen.has_value(),"selected Gandhi has no sight counter fixture");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        std::string(64,'a')};
    const auto section=runtime::make_bnw_unit_sight_checkpoint(
        changed,source,identity,0);
    auto restored=source;
    runtime::restore_bnw_unit_sight_checkpoint(section,restored,identity);
    check(array(restored.map_plots->plots[*chosen],"visibility").values[0]==
          array(changed.map_plots->plots[*chosen],"visibility").values[0],
          "selected Gandhi sight codec failed");
    std::cout<<"selected Gandhi sight codec plot "<<*chosen<<" PASS\n";
}
}
int main(int argc,char **argv) {
    check(argc==1 || argc==2,"Expected optional selected Gandhi save");
    synthetic_test();
    if(argc==2) selected_test(argv[1]);
    std::cout<<"BNW sight checkpoint synthetic PASS\n";
}
