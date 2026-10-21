#include "civ5/ui/bnw_client_scene_frame.hpp"
#include <iostream>
#include <stdexcept>
#include "bnw_client_scene_frame_fixture.hpp"
namespace {
using namespace bnw_frame_fixture;
void run() {
    auto live=session();
    const auto scene=receipt(live);
    int captures=0;
    ui::LiveSessionUiSources sources;
    sources.active_notifications=[&](const runtime::LiveSession &) {
        ++captures; return std::optional<ui::LiveWorldViewNotificationFeed>{};
    };
    sources.selected_unit=[] { return std::optional<std::int64_t>{21}; };
    ui::BnwClientSceneFrame client(live,scene,7,sources);
    ui::LiveUiHost host(registry(),client.ui_provider(),nullptr);
    const ui::ClientViewport viewport{800,600,800,600,144};
    terrain::CameraControllerState camera{}; camera.camera.world[12]=34;
    const auto frame=client.begin(viewport,camera);
    check(captures==1 && frame->stamp.selection.unit==21 && frame->camera.camera.world[12]==34,
          "Renderer did not latch one selected camera/world frame");
    const auto ui_frame=host.begin_frame(800,600);
    client.ui_provider()(); client.ui_provider()();
    check(captures==1 && client.accepts(host,ui_frame),"UI independently recaptured live sources");
    int draws=0;
    check(client.present([&](const auto &published) {
        ++draws; check(&published==frame.get(),"Renderer consumed another world");
    }) && draws==1,"Matched static GPU scene did not present");
    auto &state=live.mutable_state(); runtime::rebuild_bnw_live_plot_visibility(state);
    client.begin(viewport,camera);
    const auto new_ui=host.begin_frame(800,600);
    state.world.map_projection->cells[0].terrain_type=99; // same-turn external write, no notify
    check(!client.current() && !client.present([&](const auto &) { ++draws; }),
          "Static scene swapped a same-turn incompatible live map");
    bool action=false;
    check(client.dispatch(host,new_ui,[&] { action=true; return ui::UiCommandResult{
        ui::UiCommandStatus::performed,{}}; }).status==ui::UiCommandStatus::invalid && !action,
        "Unnotified scene mismatch executed input");
    rejects([&] { client.begin(viewport,camera); });
    rejects([&] { client.ui_provider()(); });
    state.world.map_projection->cells[0].terrain_type=2;
    client.begin(viewport,camera);
    const auto current_ui=host.begin_frame(800,600);
    check(!client.present([&](const auto &) {
        state.world.map_projection->cells[0].owner=77;
    }),"Mutation during composition escaped the swap boundary");
    state.world.map_projection->cells[0].owner.reset();
    client.begin(viewport,camera);
    const auto ready_ui=host.begin_frame(800,600);
    int performed=0;
    const auto once=[&] { ++performed; return ui::UiCommandResult{ui::UiCommandStatus::performed,{}}; };
    check(client.dispatch(host,ready_ui,once).status==ui::UiCommandStatus::performed && !client.current(),
          "Performed input retained stale publication");
    check(client.dispatch(host,ready_ui,once).status==ui::UiCommandStatus::invalid && performed==1,
          "Performed input retried against old rendered frame");
    client.begin(viewport,camera);
    const auto throw_ui=host.begin_frame(800,600);
    rejects([&] { client.dispatch(host,throw_ui,[]()->ui::UiCommandResult {
        throw std::runtime_error("uncertain effect"); }); });
    check(!client.current(),"Throwing input retained reusable frame");
    client.begin(viewport,camera);
    state.plot_visibility.counts[3]=0; // alter current derived cache without source/map writes
    check(!client.current(), "Changed derived fog retained a matching stamped frame");
    rejects([&] { client.begin(viewport,camera); });
    state.plot_visibility.counts[3]=1;
    client.begin(viewport,camera);
    state.maintenance.players[7].units.erase(21);
    check(!client.current(), "Disappeared owned selection retained a stamped frame");
    state.maintenance.players[7].units[21].type=4;
    auto wrong=scene; wrong.active_team=4;
    ui::BnwClientSceneFrame other(live,wrong,7,sources);
    rejects([&] { other.begin(viewport,camera); });
    client.begin(viewport,camera);
    const auto checkpoint=live.checkpoint();
    live.restore(checkpoint); // identical semantic map, replaced borrowed native owner
    check(!client.current(), "Restored context reused native scene owners from the old state");
    // Repeated identical restores can reuse the original state allocation.
    // A freshly published UI frame must never authenticate old native owners.
    for (unsigned cycle=0;cycle<16;++cycle) {
        rejects([&] { client.begin(viewport,camera); });
        check(!client.current(), "Fresh frame revived old native scene after restore/ABA");
        live.restore(checkpoint);
    }
    ui::BnwClientSceneFrame rebuilt(live,receipt(live),7,sources);
    check(rebuilt.begin(viewport,camera) && rebuilt.current(),
          "Explicit scene owner rebuild did not bind restored state");
    check(!client.begin({0,0,0,0,96},camera),"Minimized viewport rendered a client frame");
    (void)current_ui;
}
}
int main() {
    try { run(); std::cout<<"BNW renderer client consumption tests passed\n"; return 0; }
    catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
