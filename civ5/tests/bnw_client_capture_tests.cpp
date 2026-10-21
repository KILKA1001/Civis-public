#include "civ5/graphics/bnw_client_capture.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
namespace {
using namespace civ5;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F action) {
    try { action(); } catch (const std::exception &) { return; }
    throw std::runtime_error("Required rejection missing");
}
runtime::LiveSession session() {
    using namespace runtime;
    LiveSessionState state{saves::SavedSessionSnapshot{}, ResearchContext{}, {},
        PlayerResearchContext{}, CityYieldContext{}, BnwUnhappinessContext{}, MaintenanceContext{},
        TreasuryContext{}, ScienceContext{}, ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw, {5, 0, 0}),
         SimulationRandom(RandomAlgorithm::bnw, {7, 0, 0})}};
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    state.setup.game_turn = 12;
    state.setup.players[7].alive = true;
    state.setup.players[7].team = 3;
    state.maintenance.players[7].units[21].type = 4;
    auto &world = state.world;
    world.game_state.layout = world.map_header.layout = bnw;
    world.map_header.grid = {2, 1};
    world.map_header.guid[0] = 42;
    world.pregame.emplace().archive.integers["active_player"] = 7;
    world.pregame->archive.integers["game_turn"] = 12;
    world.pregame->layout=bnw;
    world.pregame->archive.integer_arrays["team_types"]=std::vector<std::int64_t>(80,-1);
    world.pregame->archive.integer_arrays["team_types"][7]=3;
    auto &map = world.map_projection.emplace(); map.grid = {2, 1};
    auto &plots = world.map_plots.emplace(); plots.layout = bnw; plots.dimensions.teams = 80;
    plots.dimensions.players=80;
    auto &teams = world.teams.emplace(); teams.layout = bnw; teams.dimensions.teams = 64;
    teams.teams.resize(64);
    for (std::uint64_t i = 0; i < 2; ++i) {
        saves::SavedMapCellProjection cell; cell.index = cell.x = i; cell.y = 0;
        cell.terrain_type = 2; cell.revealed_teams = std::set<std::int64_t>{3};
        map.cells.push_back(cell);
        saves::PlotObservation plot; plot.index = plot.x = i; plot.y = 0;
        plot.version=7;
        saves::PlotArrayObservation raw{"visibility", 1, std::vector<std::int64_t>(80, 0)};
        raw.values[3] = i == 0 ? 1 : 0;
        plot.arrays.push_back(std::move(raw));
        plot.arrays.push_back({"revealed_words",1,{8,0,0}});
        plot.arrays.push_back({"revealed_improvement_hashes",1,std::vector<std::int64_t>(80,0)});
        plot.arrays.push_back({"revealed_routes",1,std::vector<std::int64_t>(80,-1)});
        plots.plots.push_back(std::move(plot));
    }
    rebuild_bnw_live_plot_visibility(state);
    saves::SavedSession source; source.snapshot = world; source.source.bytes = {1, 2, 3};
    return LiveSession::from_restored_bnw(std::move(source), std::move(state));
}

graphics::BnwRendererSceneReceipt receipt(const ui::BnwClientFrame& frame) {
    graphics::BnwRendererSceneReceipt out;
    out.scene_file_sha256=out.restored_payload_sha256=std::string(64,'0');
    out.map_guid=frame.stamp.identity.map_guid;
    out.width=2; out.height=1; out.game_turn=12; out.active_player=7; out.active_team=3;
    for(const auto& cell:frame.world.map_projection->cells)
        out.cells.push_back(graphics::bnw_scene_cell_revision(cell));
    out.active_team_art=saves::observe_saved_map_team_art(*frame.world.pregame,*frame.world.map_plots).plots;
    return out;
}
void run(const std::filesystem::path& output) {
    std::filesystem::create_directories(output);
    auto live=session(); ui::BnwClientFrameOwner owner(live,"synthetic-capture-fixture",7);
    terrain::CameraControllerState camera{};
    camera.camera.world={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    camera.camera.field_of_view=35; camera.camera.aspect_ratio=1;
    camera.camera.near_plane=50; camera.camera.far_plane=4000;
    auto frame=owner.capture({2,2,2,2,96},camera); auto scene=receipt(*frame);
    int validations=0, reads=0;
    const auto readback=[&] { ++reads; return std::vector<std::uint8_t>{1,2,3,4,5,6,7,8,9,10,11,12}; };
    graphics::BnwClientCaptureOptions options{output/"frame",[] { return "2026-09-30T00:00:00Z"; },[&] {
        ++validations;
        if(graphics::bnw_renderer_scene_revision_mismatch(scene,live.state().world,live.state().setup.game_turn,7))
            throw std::runtime_error("underlying live state changed");
    },"WorldView",1.25f,&live};
    const auto files=graphics::capture_bnw_client_rgb(owner,frame,scene,2,2,readback,options);
    check(validations==2&&reads==1,"readback not bracketed with semantic validation");
    std::ifstream image(files.image,std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(image)),{});
    const std::string expected=std::string("P6\n2 2\n255\n")+std::string("\x07\x08\x09\x0a\x0b\x0c\x01\x02\x03\x04\x05\x06",12);
    check(bytes==expected,"RGB8 row orientation or pixel format changed");
    rejects([&] { graphics::capture_bnw_client_rgb(owner,frame,scene,2,2,readback,options); });
    check(std::filesystem::file_size(files.image)==bytes.size(),"exclusive capture overwrote evidence");
    options.output_prefix=output/"failed";
    rejects([&] { graphics::capture_bnw_client_rgb(owner,frame,scene,2,2,[]()->std::vector<std::uint8_t>{throw std::runtime_error("GPU read failed");},options); });
    check(!std::filesystem::exists(output/"failed.native.json"),"failed readback published receipt");
    rejects([&] { graphics::capture_bnw_client_rgb(owner,frame,scene,2,2,[]{return std::vector<std::uint8_t>{};},options); });
    rejects([&] { graphics::capture_bnw_client_rgb(owner,frame,scene,1,2,readback,options); });
    auto stale_scene=scene; stale_scene.cells[0].terrain_type=99;
    const auto before_reads=reads;
    rejects([&] { graphics::capture_bnw_client_rgb(owner,frame,stale_scene,2,2,readback,options); });
    check(reads==before_reads,"stale renderer called readback");
    // Same-turn mutation intentionally omits notify_world_changed. Semantic live
    // validator must reject even while the frame owner still reports current.
    rejects([&] { graphics::capture_bnw_client_rgb(owner,frame,scene,2,2,[&] {
        live.mutable_state().world.map_projection->cells[0].terrain_type=99;
        return readback();
    },options); });
    check(owner.is_current(frame),"fixture did not exercise unnotified mutation");
    check(!std::filesystem::exists(output/"failed.native.json"),"mutated state published receipt");
    owner.notify_world_changed();
    rejects([&] { graphics::capture_bnw_client_rgb(owner,frame,scene,2,2,readback,options); });
}
}
int main(int argc,char**argv) {try {
    const auto base=argc==2?std::filesystem::path(argv[1]):std::filesystem::current_path();
    const auto output=base/("synthetic-capture-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    run(output); std::cout<<"PASS native capture receipt\n"; return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n'; return 1;}}
