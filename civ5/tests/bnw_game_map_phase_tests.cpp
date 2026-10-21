#include "civ5/runtime/bnw_game_map_phase.hpp"
#include "civ5/runtime/bnw_plot_unit_live_services.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *why) {
    if(!value) throw std::runtime_error(why);
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view name) {
    for(const auto &entry:plot.fields)
        if(entry.name && std::string_view(entry.name)==name) return entry.value;
    throw std::runtime_error("Missing BNW map test field");
}
runtime::LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.map_header.layout=bnw;
    world.map_header.grid={1,1};
    world.map_plots.emplace();
    world.map_plots->layout=bnw;
    world.map_projection.emplace();
    world.map_projection->grid=world.map_header.grid;
    world.all_players.emplace();
    world.all_players->layout=bnw;
    saves::SavedPlayerObservation player;
    player.saved_player_id=0;
    player.core.fields.push_back({"m_iTotalLandScored",
        saves::PlayerCoreEncoding::i32,0,0,{}, {6},{},{},{}});
    world.all_players->players.push_back(std::move(player));
    saves::PlotObservation plot;
    plot.index=plot.x=plot.y=0;
    plot.version=7;
    plot.fields={{"owner",0},{"plot_type",2},
                 {"ownership_duration",2},{"improvement_duration",4},
                 {"improvement_type_hash",123},
                 {"world_anchor",-1},{"world_anchor_data",5}};
    plot.arrays.push_back({"units",2,{}});
    world.map_plots->plots.push_back(std::move(plot));
    saves::SavedMapCellProjection cell;
    cell.index=cell.x=cell.y=0;
    cell.owner=0;cell.plot_type=2;cell.improvement_type=42;
    cell.units=std::vector<saves::SavedPlotUnitReference>{};
    world.map_projection->cells.push_back(std::move(cell));
    return {std::move(world),runtime::ResearchContext{}, {},
        runtime::PlayerResearchContext{},runtime::CityYieldContext{},
        runtime::BnwUnhappinessContext{},runtime::MaintenanceContext{},
        runtime::TreasuryContext{},runtime::ScienceContext{},
        runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    content::RulesDatabase db;
    db.apply_sql(
        "CREATE TABLE Defines(Name TEXT,Value TEXT);"
        "INSERT INTO Defines VALUES"
        "('OWNERSHIP_SCORE_DURATION_THRESHOLD','3'),"
        "('PLOT_UNIT_LIMIT','1');");
    const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto handler=runtime::make_bnw_game_map_phase_handler(defines);
    check(handler.phase==runtime::BnwTurnPhase::game_map &&
          handler.run && !handler.after_commit,
          "BNW game map phase was not bound");
    auto state=fixture();
    const auto rng=state.random.gameplay.state();
    check(handler.run(state,-1).outcome==runtime::LiveActionOutcome::committed &&
          field(state.world.map_plots->plots[0],"ownership_duration")==3 &&
          field(state.world.map_plots->plots[0],"improvement_duration")==5 &&
          field(state.world.map_plots->plots[0],"world_anchor")==-1 &&
          field(state.world.map_plots->plots[0],"world_anchor_data")==-1 &&
          state.world.all_players->players[0].core.fields[0].values[0]==7 &&
          state.random.gameplay.state()==rng,
          "BNW unitless map turn did not advance plot and land score");
    auto occupied=fixture();
    occupied.world.map_projection->cells[0].units->push_back({0,1});
    check(handler.run(occupied,-1).outcome==
              runtime::LiveActionOutcome::rejected &&
          field(occupied.world.map_plots->plots[0],"ownership_duration")==2,
          "Occupied BNW plot silently skipped unit validity");
    auto anchored=fixture();
    for(auto &entry:anchored.world.map_plots->plots[0].fields)
        if(std::string_view(entry.name)=="world_anchor") entry.value=2;
    check(handler.run(anchored,-1).outcome==
              runtime::LiveActionOutcome::rejected &&
          field(anchored.world.map_plots->plots[0],"ownership_duration")==2,
          "Old BNW world anchor silently skipped engine removal");
    auto later_bad=fixture();
    later_bad.world.map_header.grid={2,1};
    later_bad.world.map_projection->grid={2,1};
    auto second=later_bad.world.map_plots->plots[0];
    second.index=second.x=1;
    for(auto &entry:second.fields)
        if(std::string_view(entry.name)=="improvement_duration")
            entry.value=32767;
    later_bad.world.map_plots->plots.push_back(std::move(second));
    auto second_cell=later_bad.world.map_projection->cells[0];
    second_cell.index=second_cell.x=1;
    later_bad.world.map_projection->cells.push_back(std::move(second_cell));
    check(handler.run(later_bad,-1).outcome==
              runtime::LiveActionOutcome::rejected &&
          field(later_bad.world.map_plots->plots[0],"ownership_duration")==2 &&
          later_bad.world.all_players->players[0].core.fields[0].values[0]==6,
          "Late BNW map rejection leaked earlier plot increments");
    check(handler.run(state,0).outcome==runtime::LiveActionOutcome::rejected,
          "BNW map phase accepted a player turn");
    if(argc==3) {
        auto selected=ui::bootstrap_selected_bnw_preview(
            std::filesystem::path(argv[1]),std::filesystem::path(argv[2]),0);
        if(selected.preview.blocker)
            throw std::runtime_error(selected.preview.blocker->reason);
        check(selected.preview.host!=nullptr,
              "Selected BNW map host unavailable");
        auto real=selected.preview.host->session().state();
        const auto selected_handler=runtime::make_bnw_game_map_phase_handler(
            selected.preview.host->defines());
        std::size_t occupied_plots=0,anchored_plots=0;
        for(const auto &cell:real.world.map_projection->cells)
            if(cell.units && !cell.units->empty()) ++occupied_plots;
        for(const auto &plot:real.world.map_plots->plots)
            if(field(plot,"world_anchor")!=-1) ++anchored_plots;
        const auto original=real.world.map_plots->plots[0].fields;
        const auto selected_rng=real.random.gameplay.state();
        const auto outcome=selected_handler.run(real,-1);
        check(outcome.outcome==runtime::LiveActionOutcome::rejected &&
              real.world.map_plots->plots[0].fields.size()==original.size() &&
              real.random.gameplay.state()==selected_rng,
              "Selected BNW unsupported map branch was silently accepted");
        for(std::size_t i=0;i<original.size();++i)
            check(real.world.map_plots->plots[0].fields[i].value==
                  original[i].value,
                  "Selected BNW rejected map changed a plot");
        check(occupied_plots==7 && anchored_plots==1,
              "Selected Gandhi occupied or anchored map roster changed");
        const auto &database=selected.preview.host->database();
        const auto domain=runtime::load_bnw_plot_domain_rules(
            database,database.cache_info_types());
        const auto probe_hash=field(real.world.map_plots->plots[233],
                                    "improvement_type_hash");
        std::size_t matching_rules=0;
        std::int64_t resolved_id=-1;
        for(const auto &[id,rule]:domain.improvements)
            if(rule.hash==probe_hash) {++matching_rules;resolved_id=id;}
        check(probe_hash==1434118760 && matching_rules==1 && resolved_id==2 &&
              !real.world.map_projection->cells[233].improvement_type,
              "Selected Gandhi goody-hut hash is not uniquely active");
        const auto reads=runtime::make_bnw_plot_unit_no_effect_services(
            domain,&database);
        const auto anchor=runtime::plan_bnw_world_anchor_clear(real,635);
        check(anchor.valid && anchor.removal,
              "Selected Gandhi anchor at plot 635 is unresolved");
        std::vector<runtime::BnwWorldAnchorRemovalEvent> delivered;
        auto full_handler=runtime::make_bnw_game_map_phase_handler(
            selected.preview.host->defines(),domain,reads,
            [&](const auto &event){delivered.push_back(event);});
        auto complete=real;
        const auto passed=full_handler.run(complete,-1);
        if(passed.outcome!=runtime::LiveActionOutcome::committed)
            std::cerr<<"selected full map reject="<<passed.reason<<'\n';
        check(passed.outcome==runtime::LiveActionOutcome::committed &&
              full_handler.after_commit && delivered.empty() &&
              field(complete.world.map_plots->plots[635],"world_anchor")==-1 &&
              field(complete.world.map_plots->plots[635],"world_anchor_data")==-1 &&
              complete.random.gameplay.state()==selected_rng,
              "Selected Gandhi full map phase did not stage atomically");
        for(std::size_t index=0;index<real.world.map_plots->plots.size();++index) {
            const auto &before=real.world.map_plots->plots[index];
            const auto &after=complete.world.map_plots->plots[index];
            const auto owner=field(before,"owner");
            const auto improved=before.version && *before.version>=5?
                field(before,"improvement_type_hash")!=0:
                field(before,"improvement_type")!=-1;
            check(field(after,"ownership_duration")==
                      field(before,"ownership_duration")+(owner!=-1) &&
                  field(after,"improvement_duration")==
                      field(before,"improvement_duration")+improved &&
                  field(after,"world_anchor")==-1 &&
                  field(after,"world_anchor_data")==-1,
                  "Selected Gandhi map plot did not advance in SDK order");
        }
        check(complete.world.map_projection->cells[233].improvement_type==2,
              "Selected Gandhi goody-hut hash did not become live Improvement ID");
        check(full_handler.run(complete,-1).outcome==
                  runtime::LiveActionOutcome::rejected,
              "Map handler discarded an unpublished anchor journal");
        check(full_handler.after_commit(selected.preview.host->session()).outcome==
                  runtime::LiveActionOutcome::rejected && delivered.empty(),
              "Anchor journal published against the original save");
        selected.preview.host->session().mutable_state()=complete;
        check(full_handler.after_commit(selected.preview.host->session()).outcome==
                  runtime::LiveActionOutcome::committed &&
              delivered.size()==1 && delivered.front()==*anchor.removal,
              "Selected Gandhi anchor event was not published once after commit");
        auto absent=real;
        auto missing=runtime::make_bnw_game_map_phase_handler(
            selected.preview.host->defines(),domain,reads,{});
        check(missing.run(absent,-1).outcome==
                  runtime::LiveActionOutcome::rejected &&
              field(absent.world.map_plots->plots[635],"world_anchor")==
                  anchor.removal->old_anchor,
              "Missing GameplayWorldAnchor emitter changed selected map");
        auto effect_reads=reads;
        effect_reads.valid_domain=[](const auto &,auto,auto){return false;};
        auto effect=real;
        auto effect_handler=runtime::make_bnw_game_map_phase_handler(
            selected.preview.host->defines(),domain,std::move(effect_reads),
            [&](const auto &event){delivered.push_back(event);});
        const auto refused=effect_handler.run(effect,-1);
        check(refused.outcome==runtime::LiveActionOutcome::rejected &&
              field(effect.world.map_plots->plots[635],"world_anchor")==
                  anchor.removal->old_anchor && delivered.size()==1,
              "Required unit jump/kill leaked detached map or engine event");
        auto hole_rules=domain;
        hole_rules.improvements.erase(resolved_id);
        auto hole=real;
        auto hole_handler=runtime::make_bnw_game_map_phase_handler(
            selected.preview.host->defines(),hole_rules,reads,
            [&](const auto &event){delivered.push_back(event);});
        const auto missing_rule=hole_handler.run(hole,-1);
        check(missing_rule.outcome==runtime::LiveActionOutcome::rejected &&
              missing_rule.reason.find("absent from active rules")!=std::string::npos &&
              !hole.world.map_projection->cells[233].improvement_type &&
              field(hole.world.map_plots->plots[635],"world_anchor")==
                  anchor.removal->old_anchor && delivered.size()==1,
              "Missing Improvement rule silently changed Gandhi map");
        check(!domain.improvements.contains(1000000),
              "Selected Improvement collision ID is already occupied");
        auto collision_rules=domain;
        collision_rules.improvements.emplace(1000000,
            domain.improvements.at(resolved_id));
        auto collision=real;
        auto collision_handler=runtime::make_bnw_game_map_phase_handler(
            selected.preview.host->defines(),collision_rules,reads,
            [&](const auto &event){delivered.push_back(event);});
        const auto duplicated=collision_handler.run(collision,-1);
        check(duplicated.outcome==runtime::LiveActionOutcome::rejected &&
              duplicated.reason.find("ambiguous")!=std::string::npos &&
              !collision.world.map_projection->cells[233].improvement_type &&
              field(collision.world.map_plots->plots[635],"world_anchor")==
                  anchor.removal->old_anchor && delivered.size()==1,
              "Colliding Improvement hash silently changed Gandhi map");
        std::cout<<"selected BNW map occupied_plots="<<occupied_plots
                 <<" anchored_plots="<<anchored_plots
                 <<" full=PASS legacy_reject="<<outcome.reason<<'\n';
    }
}
