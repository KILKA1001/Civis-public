// Explicit owner-fence fault probes using the real immutable packet API.
// No GPU, original fog-generator, or full-turn claim.
#include "civ5/ui/bnw_minimap_fog_preparation.hpp"
#include "civ5/ui/minimap_saved_map.hpp"
#include "bnw_client_scene_frame_fixture.hpp"
#include <functional>
#include <iostream>

namespace {
using namespace civ5;
using bnw_frame_fixture::check;
using bnw_frame_fixture::rejects;
ui::BnwMinimapFogGeometry geometry() {
    ui::BnwMinimapFogGeometry g;
    g.window={0,0,1,0};
    g.transform.cube_to_world[0]=g.transform.cube_to_world[5]=1;
    g.transform.cube_to_world[10]=g.transform.cube_to_world[15]=1;
    g.edge_subdivisions=4;
    return g;
}
void run(bool current_only) {
    auto live=bnw_frame_fixture::session();
    auto &city=live.mutable_state().cities.players[7].cities[9];
    city.owner=7;city.id=9;city.population=5;city.x=0;city.y=0;
    runtime::rebuild_bnw_live_plot_visibility(live.mutable_state());
    std::function<void()> fault;
    bool armed=false, fired=false;
    ui::LiveSessionUiSources sources;
    sources.city_banner_appearance=[](const auto &,auto active,auto player,auto id,auto frame) {
        ui::BnwCityBannerAppearance r;
        r.active_player_id=active;r.player_id=player;r.city_id=id;r.frame_number=frame;r.turn=12;
        return std::optional(r);
    };
    sources.city_banner_appearance_current=[&](const auto &,const auto &) {
        if(armed) { armed=false;fired=true;fault(); }
        return true;
    };
    ui::BnwClientFrameOwner client(live,"callback-handle-pinning",7,sources);
    const ui::ClientViewport viewport{800,600,800,600,96};
    const auto scene=bnw_frame_fixture::receipt(live);
    const auto initial=ui::project_saved_minimap_terrain(*live.state().world.map_projection);
    ui::BnwMinimapFogPreparationOwner owner(live,client,scene,initial,geometry());
    auto frame=client.capture(viewport,{});
    auto packet=owner.prepare(frame);
    const auto retained=packet;
    check(owner.current(packet),"Current callback-pin fixture missing");
    fault=[&]{ packet.reset(); };armed=true;
    const bool accepted=owner.current(packet);
    check(fired && !packet,"Owner fence did not reset original caller packet storage");
    check(!accepted,"current accepted a packet argument reset by its source callback");
    check(!owner.current(retained),"Failed input bracket retained the previous packet");
    if(current_only) return;

    fired=false;frame=client.capture(viewport,{});packet=owner.prepare(frame);
    const auto before_prepare=packet;
    fault=[&]{ frame.reset(); };armed=true;
    rejects([&]{ owner.prepare(frame); });
    check(fired && !frame && !owner.current(before_prepare),
        "prepare reset fault crashed, published, or retained its previous packet");

    fired=false;frame=client.capture(viewport,{});packet=owner.prepare(frame);
    ui::BnwMinimapFogPreparationOwner foreign(live,client,scene,initial,geometry());
    const auto foreign_packet=foreign.prepare(frame);
    const auto own=packet;
    fault=[&]{ packet=foreign_packet; };armed=true;
    check(!owner.current(packet) && fired && packet==foreign_packet,
        "Current query silently retargeted to a foreign packet after source callback");
    check(!owner.current(own),"Retargeted query retained its old publication");

    armed=false;frame=client.capture(viewport,{});packet=owner.prepare(frame);
    check(owner.current(packet),"Explicit recapture after faults failed");
}
}
int main(int argc,char **) {
    try {run(argc>1);std::cout<<"PASS minimap caller handle reset/retarget source-fence probes\n";}
    catch(const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
