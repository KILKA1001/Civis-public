#include "civ5/runtime/bnw_unit_jump_detached.hpp"
#include "civ5/runtime/bnw_unit_jump_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
saves::UnitCoreField &field(saves::SavedUnitObservation &unit,
                            std::string_view name) {
    for(auto &entry:unit.core.fields)
        if(entry.name && std::string_view(entry.name)==name) return entry;
    throw std::runtime_error("missing synthetic unit field");
}
const saves::UnitCoreField &field(const saves::SavedUnitObservation &unit,
                                  std::string_view name) {
    for(const auto &entry:unit.core.fields)
        if(entry.name && std::string_view(entry.name)==name) return entry;
    throw std::runtime_error("missing synthetic unit field");
}
runtime::LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=world.map_header.layout=bnw;
    world.map_header.grid={3,1};
    world.map_plots.emplace();
    world.map_plots->layout=bnw;
    world.map_projection.emplace();
    world.map_projection->grid=world.map_header.grid;
    world.all_players.emplace();
    world.all_players->layout=bnw;
    saves::SavedPlayerObservation player;
    player.saved_player_id=0;
    player.units.layout=player.units.header.layout=bnw;
    player.units.header.wire_unit_count=1;
    saves::SavedUnitObservation unit;
    unit.core.layout=unit.tail.layout=bnw;
    unit.core.version=7;unit.tail.context.bnw_unit_version=7;
    unit.core.fields={
        {"m_eOwner",saves::UnitCoreEncoding::i32,0,0,{}, {0},{}},
        {"m_iID",saves::UnitCoreEncoding::i32,0,0,{}, {7},{}},
        {"m_iX",saves::UnitCoreEncoding::i32,0,0,{}, {0},{}},
        {"m_iY",saves::UnitCoreEncoding::i32,0,0,{}, {0},{}},
        {"m_iLastMoveTurn",saves::UnitCoreEncoding::i32,0,0,{}, {1},{}},
        {"m_iFortifyTurns",saves::UnitCoreEncoding::i32,0,0,{}, {3},{}},
        {"m_bSetUpForRangedAttack",saves::UnitCoreEncoding::boolean,0,0,{}, {1},{}},
        {"m_eFacingDirection",saves::UnitCoreEncoding::i32,0,0,{}, {0},{}},
        {"m_eActivityType",saves::UnitCoreEncoding::i32,0,0,{}, {2},{}}
    };
    unit.tail.blocks.push_back({"m_missionQueue","counted_missions",0,0,1,
                                {1,2,3,4,5},{},{}});
    player.units.units.push_back(unit);
    world.all_players->players.push_back(player);
    for(std::uint64_t index=0;index<3;++index) {
        saves::PlotObservation plot;
        plot.index=plot.x=index;plot.y=0;plot.version=7;
        plot.arrays.push_back({"units",2,index==0?
            std::vector<std::int64_t>{0,7}:std::vector<std::int64_t>{}});
        world.map_plots->plots.push_back(plot);
        saves::SavedMapCellProjection cell;
        cell.index=cell.x=index;cell.y=0;
        cell.units=index==0?
            std::vector<saves::SavedPlotUnitReference>{{0,7}}:
            std::vector<saves::SavedPlotUnitReference>{};
        world.map_projection->cells.push_back(cell);
    }
    return {std::move(world),runtime::ResearchContext{}, {},
        runtime::PlayerResearchContext{},runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{},runtime::MaintenanceContext{},
        runtime::TreasuryContext{},runtime::ScienceContext{},
        runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
}
runtime::BnwJumpReadServices reads() {
    runtime::BnwJumpReadServices result;
    result.nearest_city_plot=[](const auto &,auto,auto){return std::int64_t{-1};};
    result.valid_domain=[](const auto &,auto,auto){return true;};
    result.can_move_into=[](const auto &,auto index,auto){return index!=0;};
    result.friendly_units_of_type=[](const auto &,auto,auto){return std::int64_t{0};};
    result.can_enter_territory=[](const auto &,auto,auto){return true;};
    result.barbarian=[](const auto &,auto,auto){return false;};
    result.enemy=[](const auto &,auto,auto){return false;};
    result.mountain=[](const auto &,auto,auto){return false;};
    result.air=[](const auto &,auto,auto){return false;};
    result.sea=[](const auto &,auto,auto){return false;};
    result.revealed=[](const auto &,auto,auto){return true;};
    result.area_id=[](const auto &,auto,auto){return std::int64_t{1};};
    result.plot_distance=[](const auto &,auto a,auto b){
        return static_cast<std::int64_t>(a>b?a-b:b-a);
    };
    return result;
}
std::int64_t x(const runtime::LiveSessionState &state) {
    for(const auto &entry:state.world.all_players->players[0].units.units[0].core.fields)
        if(entry.name && std::string_view(entry.name)=="m_iX")
            return entry.values.at(0);
    throw std::runtime_error("missing x");
}
void selected_codec(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    const auto &source=imported.snapshot;
    check(source.all_players && source.map_plots && source.map_projection,
          "selected Gandhi unit/map decode unavailable");
    std::optional<runtime::BnwUnitJumpCheckpointTarget> target;
    for(std::size_t p=0;p<source.all_players->players.size() && !target;++p)
        for(std::size_t u=0;u<source.all_players->players[p].units.units.size() && !target;++u) {
            const auto &unit=source.all_players->players[p].units.units[u];
            const auto owner=field(unit,"m_eOwner")
                                .values.at(0);
            const auto id=field(unit,"m_iID")
                             .values.at(0);
            const auto ux=field(unit,"m_iX")
                             .values.at(0);
            const auto uy=field(unit,"m_iY")
                             .values.at(0);
            if(ux<0 || uy<0 || ux>=static_cast<std::int64_t>(source.map_header.grid.width) ||
               uy>=static_cast<std::int64_t>(source.map_header.grid.height)) continue;
            const auto from=source.map_header.grid.index(ux,uy);
            const auto &from_plot=source.map_plots->plots[from];
            bool singleton=false;
            for(const auto &array:from_plot.arrays)
                if(array.name && std::string_view(array.name)=="units" &&
                   array.values==std::vector<std::int64_t>{owner,id}) singleton=true;
            if(!singleton) continue;
            for(std::uint64_t to=0;to<source.map_header.grid.size();++to) {
                if(to==from) continue;
                bool empty=false;
                for(const auto &array:source.map_plots->plots[to].arrays)
                    if(array.name && std::string_view(array.name)=="units" &&
                       array.values.empty()) empty=true;
                if(empty) {target={{p,u,from,to}};break;}
            }
        }
    check(target.has_value(),"selected Gandhi has no synthetic jump fixture");
    auto moved=source;
    auto &unit=moved.all_players->players[target->player_index]
                    .units.units[target->unit_index];
    const auto xy=moved.map_header.grid.coordinates(target->to_plot);
    const auto owner=field(unit,"m_eOwner").values.at(0);
    const auto id=field(unit,"m_iID").values.at(0);
    field(unit,"m_iX").values[0]=xy[0];
    field(unit,"m_iY").values[0]=xy[1];
    for(auto index:{target->from_plot,target->to_plot})
        for(auto &array:moved.map_plots->plots[index].arrays)
            if(array.name && std::string_view(array.name)=="units")
                array.values=index==target->from_plot?
                    std::vector<std::int64_t>{}:std::vector<std::int64_t>{owner,id};
    moved.map_projection->cells[target->from_plot].units=
        std::vector<saves::SavedPlotUnitReference>{};
    moved.map_projection->cells[target->to_plot].units=
        std::vector<saves::SavedPlotUnitReference>{{owner,id}};
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    const auto section=runtime::make_bnw_unit_jump_checkpoint(
        moved,source,identity,*target);
    auto document=saves::make_civis_import_document(
        imported.source,identity.payload_sha256);
    document.sections.push_back(section);
    const auto output=path.parent_path()/(
        "bnw-jump-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code error;std::filesystem::remove(path,error);
    }} cleanup{output};
    const auto on_disk=saves::read_civis_save(output);
    const saves::CivisSaveSection *encoded=nullptr;
    for(const auto &part:on_disk.sections)
        if(part.id==runtime::bnw_unit_jump_checkpoint_section_id) {
            check(!encoded,"duplicated BNW jump section on disk");encoded=&part;
        }
    check(encoded && encoded->bytes==section.bytes,
          "BNW jump section did not survive CivisSave write/read");
    auto restored=source;
    runtime::restore_bnw_unit_jump_checkpoint(*encoded,restored,identity);
    check(field(restored.all_players->players[target->player_index]
                         .units.units[target->unit_index],"m_iX").values[0]==xy[0] &&
          restored.map_projection->cells[target->from_plot].units->empty() &&
          restored.map_projection->cells[target->to_plot].units->size()==1,
          "selected Gandhi synthetic jump checkpoint roundtrip failed");
    std::cout<<"selected Gandhi jump codec plot "<<target->from_plot
             <<" -> "<<target->to_plot<<" PASS\n";
}
}
int main(int argc,char **argv) {
    check(argc==1 || argc==2,"Expected optional selected Gandhi save");

    const auto source=fixture();
    constexpr saves::SavedPlotUnitReference handle{0,7};
    const auto original_rng=source.random.gameplay.state();
    auto callbacks=reads();
    const auto chosen=runtime::select_bnw_nearest_valid_plot(
        source,0,handle,1,callbacks);
    check(chosen.outcome==runtime::BnwJumpOutcome::destination &&
          chosen.destination_plot==1 && chosen.best_value==2 &&
          chosen.scanned_plots==3 && x(source)==0,
          "SDK jump scan did not choose closest plot");
    callbacks.plot_distance=[](const auto &,auto a,auto b){
        return std::int64_t{a==b?0:1};
    };
    const auto tied=runtime::select_bnw_nearest_valid_plot(
        source,0,handle,1,callbacks);
    check(tied.destination_plot==1 && tied.best_value==2,
          "SDK strict less-than did not keep first plot on tie");
    auto missing=callbacks;
    missing.can_enter_territory={};
    const auto refused=runtime::select_bnw_nearest_valid_plot(
        source,0,handle,1,missing);
    check(refused.outcome==runtime::BnwJumpOutcome::unresolved &&
          refused.reason.find("canEnterTerritory")!=std::string::npos &&
          x(source)==0,"unknown jump callback did not fail closed");
    auto blocked=callbacks;
    blocked.can_move_into=[](const auto &,auto,auto){return false;};
    const auto absent=runtime::select_bnw_nearest_valid_plot(
        source,0,handle,1,blocked);
    check(absent.outcome==runtime::BnwJumpOutcome::no_destination,
          "jump without destination did not demand kill");
    const auto death=runtime::project_bnw_jump_detached(
        source,0,handle,1,blocked,{});
    check(!death.detached &&
          death.reason.find("kill(false)")!=std::string::npos,
          "failed jump silently skipped kill");

    std::vector<std::string> order;
    runtime::BnwJumpDetachedServices effects;
    effects.set_xy=[&](auto &state,auto unit,std::uint64_t destination){
        order.push_back("setXY");
        auto &from=state.world.map_projection->cells[0].units.value();
        auto &to=state.world.map_projection->cells[destination].units.value();
        from.clear();to.push_back(unit);
        state.world.map_plots->plots[0].arrays[0].values.clear();
        state.world.map_plots->plots[destination].arrays[0].values=
            {unit.owner,unit.id};
        auto &selected=state.world.all_players->players[0].units.units[0];
        field(selected,"m_iX").values[0]=static_cast<std::int64_t>(destination);
        field(selected,"m_iLastMoveTurn").values[0]=2;
        field(selected,"m_bSetUpForRangedAttack").values[0]=0;
        field(selected,"m_eFacingDirection").values[0]=1;
        return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,{}};
    };
    effects.clear_mission_queue=[&](auto &state,auto){
        order.push_back("ClearMissionQueue");
        auto &mission=state.world.all_players->players[0].units.units[0]
                          .tail.blocks[0];
        mission.wire_count=0;mission.values.clear();
        return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,{}};
    };
    effects.set_activity_awake=[&](auto &state,auto){
        order.push_back("SetActivityType");
        auto &selected=state.world.all_players->players[0].units.units[0];
        field(selected,"m_eActivityType").values[0]=0;
        field(selected,"m_iFortifyTurns").values[0]=0;
        return runtime::LiveActionResult{runtime::LiveActionOutcome::committed,{}};
    };
    const auto projected=runtime::project_bnw_jump_detached(
        source,0,handle,1,callbacks,effects);
    check(projected.detached && !projected.can_commit() &&
          x(*projected.detached)==1 && x(source)==0 &&
          projected.detached->world.map_projection->cells[0].units->empty() &&
          projected.detached->world.map_projection->cells[1].units->size()==1 &&
          source.random.gameplay.state()==original_rng &&
          order==std::vector<std::string>{"setXY","ClearMissionQueue",
                                          "SetActivityType"},
          "detached jump projection leaked or changed SDK call order");
    const runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    const auto section=runtime::make_bnw_unit_jump_checkpoint(
        projected.detached->world,source.world,identity,{0,0,0,1});
    check(section.id==runtime::bnw_unit_jump_checkpoint_section_id,
          "BNW jump checkpoint section ID changed");
    auto reloaded=source.world;
    runtime::restore_bnw_unit_jump_checkpoint(section,reloaded,identity);
    check(reloaded.all_players->players[0].units.units[0].core.fields[2].values[0]==1 &&
          reloaded.map_projection->cells[0].units->empty() &&
          reloaded.map_projection->cells[1].units->size()==1 &&
          reloaded.all_players->players[0].units.units[0].tail.blocks[0]
              .values.empty() &&
          field(reloaded.all_players->players[0].units.units[0],
                "m_iFortifyTurns").values[0]==0,
          "BNW jump checkpoint roundtrip lost unit or roster");
    auto stale=source.world;
    stale.all_players->players[0].units.units[0].core.fields[2].values[0]=2;
    bool rejected=false;
    try {runtime::restore_bnw_unit_jump_checkpoint(section,stale,identity);}
    catch(const std::exception &) {rejected=true;}
    check(rejected && stale.all_players->players[0].units.units[0]
                          .core.fields[2].values[0]==2,
          "BNW jump checkpoint accepted stale source or leaked mutation");
    auto tampered=section;
    tampered.bytes.back()^=1;
    auto intact=source.world;
    rejected=false;
    try {runtime::restore_bnw_unit_jump_checkpoint(tampered,intact,identity);}
    catch(const std::exception &) {rejected=true;}
    check(rejected && intact.all_players->players[0].units.units[0]
                         .core.fields[2].values[0]==0,
          "BNW jump checkpoint accepted corrupt target or leaked mutation");
    order.clear();
    effects.clear_mission_queue=[&](auto &,auto){
        order.push_back("ClearMissionQueue");
        return runtime::LiveActionResult{runtime::LiveActionOutcome::rejected,
                                         "mission unavailable"};
    };
    const auto rollback=runtime::project_bnw_jump_detached(
        source,0,handle,1,callbacks,effects);
    check(!rollback.detached && x(source)==0 &&
          rollback.reason.find("mission unavailable")!=std::string::npos,
          "failed detached jump leaked state");
    if(argc==2) selected_codec(argv[1]);
    std::cout<<"BNW jump selection and detached effect harness PASS\n";
}
