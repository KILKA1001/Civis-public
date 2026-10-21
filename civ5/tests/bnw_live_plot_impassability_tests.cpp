#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_action_transaction.hpp"
#include "civ5/runtime/live_session.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    try {action();} catch(const std::exception &) {return;}
    throw std::runtime_error("Missing BNW plot impassability blocker");
}
LiveSessionState empty_state() {
    return {saves::SavedSessionSnapshot{},ResearchContext{}, {},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{0,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{0,0,0})},
        ResearchSelectionState{},BnwGameClockState{},std::nullopt,std::nullopt,{}, {}, {}, {}, {}, {}};
}
SessionCheckpointIdentity identity(const content::RulesDatabase &db) {
    return {"test-payload",identify_bytes(db.gameplay_fingerprint_data()).sha256};
}
void synthetic() {
    content::RulesDatabase db;
    db.apply_sql("CREATE TABLE Terrains(ID INTEGER,Type TEXT,Impassable BOOLEAN);"
        "INSERT INTO Terrains VALUES(0,'TERRAIN_BLOCKED',1);"
        "CREATE TABLE Features(ID INTEGER,Type TEXT,Impassable BOOLEAN);"
        "INSERT INTO Features VALUES(0,'FEATURE_PASSABLE',0),"
        "(1,'FEATURE_BLOCKED',1);");
    const auto types=db.cache_info_types();
    auto state=empty_state();
    state.world.game_state.layout=state.world.map_header.layout=bnw;
    state.world.map_header.grid={3,1};
    auto &plots=state.world.map_plots.emplace();plots.layout=bnw;
    auto &map=state.world.map_projection.emplace();map.grid={3,1};
    for(std::uint64_t i=0;i<3;++i) {
        saves::SavedMapCellProjection cell;
        cell.index=i;cell.x=i;cell.y=0;
        cell.terrain_type=0;cell.feature_type=i==0?-1:i==1?0:1;
        map.cells.push_back(cell);
        saves::PlotObservation saved;saved.index=i;saved.x=i;saved.y=0;
        plots.plots.push_back(saved);
    }
    const auto stamp=identity(db);
    rebuild_bnw_live_plot_impassability(state,db,types,stamp);
    const auto &pass=bnw_live_plot_impassability(state);
    check(pass.plots.size()==3 && pass.plots[0].impassable &&
          !pass.plots[1].impassable && pass.plots[2].impassable,
          "CvPlot::updateImpassable feature-over-terrain rule changed");
    auto changed=state;
    changed.world.map_projection->cells[1].feature_type=-1;
    rejects([&]{(void)bnw_live_plot_impassability(changed);});
    auto unknown=state;
    unknown.world.map_projection->cells[1].feature_type=99;
    rejects([&]{rebuild_bnw_live_plot_impassability(unknown,db,types,stamp);});
    check(!unknown.plot_impassability.current,
          "Failed rebuild retained a current impassability cache");
    auto stale_rules=stamp;stale_rules.rules_sha256="wrong";
    rejects([&]{rebuild_bnw_live_plot_impassability(unknown,db,types,stale_rules);});
    auto vp=state;vp.world.game_state.layout=saves::GameStateLayout::vp542_corpus;
    rejects([&]{rebuild_bnw_live_plot_impassability(vp,db,types,stamp);});
    saves::SavedSession source;source.snapshot=state.world;
    auto session=LiveSession::from_restored_bnw(std::move(source),std::move(state));
    const auto checkpoint=session.checkpoint();
    const auto declined=apply_live_action(session,[](LiveSessionState &) {
        return LiveActionResult{LiveActionOutcome::rejected,"declined"};
    });
    check(declined.outcome==LiveActionOutcome::rejected &&
          session.state().plot_impassability.current,
          "Rejected action invalidated unchanged plot cache");
    (void)apply_live_action(session,[](LiveSessionState &) {
        return LiveActionResult{LiveActionOutcome::committed,{}};
    });
    check(!session.state().plot_impassability.current,
          "Committed action retained potentially stale plot cache");
    session.restore(checkpoint);
    check(bnw_live_plot_impassability(session.state()).plots[0].impassable,
          "Checkpoint failed to restore plot passability");
    (void)session.mutable_state();
    check(!session.state().plot_impassability.current,
          "Arbitrary world edit retained plot passability");
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    content::RulesDatabase db;
    // Expansion2 supplies its own complete terrain/feature collections.
    for(const auto &relative:{
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Terrains.xml",
        "Assets/DLC/Expansion2/Gameplay/XML/Terrain/CIV5Features.xml"})
        db.apply_xml(content::read_xml(game_root/relative));
    const auto types=db.cache_info_types();
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto source=saves::read_saved_session(save,context);
    auto state=empty_state();state.world=std::move(source.snapshot);
    check(state.world.game_state.layout==bnw && state.world.map_projection,
          "Selected Gandhi save lacks a decoded BNW map");
    rebuild_bnw_live_plot_impassability(state,db,types,identity(db));
    const auto &pass=bnw_live_plot_impassability(state);
    std::size_t blocked=0,feature_override=0;
    for(std::size_t i=0;i<pass.plots.size();++i) {
        blocked+=pass.plots[i].impassable;
        const auto &cell=state.world.map_projection->cells[i];
        if((cell.feature_type && *cell.feature_type!=-1) ||
           (cell.feature_type_hash && *cell.feature_type_hash!=0))
            ++feature_override;
    }
    std::cout << "selected BNW plots=" << pass.plots.size()
              << " impassable=" << blocked
              << " featured=" << feature_override << '\n';
    check(blocked>0 && feature_override>0 &&
          pass.plots.size()==state.world.map_projection->cells.size(),
          "Selected save did not exercise complete terrain/feature passability");
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and game root");
        synthetic();
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW CvPlot::updateImpassable live load hook\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW CvPlot::updateImpassable live load hook: "
                  << error.what() << '\n';
        return 1;
    }
}
