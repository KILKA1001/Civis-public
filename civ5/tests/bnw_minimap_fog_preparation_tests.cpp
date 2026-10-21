#include "civ5/ui/bnw_minimap_fog_preparation.hpp"
#include "civ5/ui/minimap_saved_map.hpp"
#include "bnw_client_scene_frame_fixture.hpp"
#include <iostream>
#include <limits>
#include <thread>
#include <type_traits>

namespace {
using namespace civ5;
using bnw_frame_fixture::check;
using bnw_frame_fixture::rejects;
ui::BnwMinimapFogGeometry geometry() {
    ui::BnwMinimapFogGeometry result;
    result.window = {0, 0, 1, 0};
    result.transform.cube_to_world[0] = 1;
    result.transform.cube_to_world[5] = 1;
    result.transform.cube_to_world[10] = 1;
    result.transform.cube_to_world[15] = 1;
    result.edge_subdivisions = 4;
    return result;
}
void run() {
    static_assert(!std::is_copy_constructible_v<ui::BnwMinimapFogPacket>);
    auto live = bnw_frame_fixture::session();
    auto &state = live.mutable_state();
    // Declared constructed fixture: plot1 is genuinely unknown to this team.
    state.world.map_projection->cells[1].revealed_teams->clear();
    state.world.map_plots->plots[1].arrays[1].values[0] = 0;
    runtime::rebuild_bnw_live_plot_visibility(state);
    const auto scene = bnw_frame_fixture::receipt(live);
    const auto initial = ui::project_saved_minimap_terrain(*live.state().world.map_projection);
    ui::BnwClientFrameOwner client(live, "minimap-fixture", 7);
    ui::BnwMinimapFogPreparationOwner owner(live, client, scene, initial, geometry());
    const ui::ClientViewport viewport{800,600,800,600,96};
    const auto frame = client.capture(viewport, {});
    const auto first = owner.prepare(frame);
    check(owner.current(first) && first->opened_cells() == 1,
        "Current native-cache frame did not prepare minimap fog");
    check(owner.source_context_matches(scene, geometry()), "Actual pinned source context rejected");
    auto wrong_source = scene; wrong_source.active_team++;
    check(!owner.source_context_matches(wrong_source, geometry()), "Foreign source team accepted");
    wrong_source = scene; wrong_source.scene_file_sha256[0] = wrong_source.scene_file_sha256[0]=='0' ? '1' : '0';
    check(!owner.source_context_matches(wrong_source, geometry()), "Foreign source file SHA accepted");
    auto wrong_transform = geometry(); wrong_transform.transform.offset_x = 0.25f;
    check(!owner.source_context_matches(scene, wrong_transform), "Foreign source transform accepted");
    wrong_transform = geometry(); wrong_transform.transform.offset_x = -0.0f;
    check(!owner.source_context_matches(scene, wrong_transform), "Bit-distinct transform context accepted");
    check(owner.current(first), "Rejected declared source revoked current genuine packet");
    check(!(first->grid().at(0,0).byte2 & 1) && (first->grid().at(1,0).byte2 & 1) &&
        first->fog_mesh().triangle_count() > 6,
        "Partial reveal did not retain a masked hidden hex");
    check(initial.at(0,0).byte2 == 1 && initial.sentinel().byte2 == 1 &&
        (first->grid().sentinel().byte2 & 1) && first->grid().sentinel().byte2 != 1,
        "Preparation mutated its template or suppressed source sentinel edge writes");
    check(first->identity().client == frame->stamp.identity &&
        first->identity().state_generation == live.state_generation() &&
        first->identity().source_grid_sha256.size() == 64 &&
        first->identity().context_sha256.size() == 64 &&
        first->identity().fog_payload_sha256.size() == 64,
        "Prepared packet lost source/client/generation identities");
    // No source plot mutation or notify: actual same-generation cache changed.
    state.plot_visibility.counts[1 * 80 + 3] = 1;
    check(!owner.current(first), "Unnotified cache mutation accepted an old packet");
    check(!owner.source_context_matches(scene, geometry()), "Stale in-place source context accepted");
    rejects([&] { owner.prepare(frame); });
    const auto fresh = client.capture(viewport, {});
    const auto both = owner.prepare(fresh);
    check(owner.current(both) && both->opened_cells() == 2 && both->fog_mesh().triangles.empty(),
        "Fresh cache-only visibility did not remove hidden-cell fog");
    check(first->grid().at(1,0).byte2 & 1, "Published immutable packet was rewritten");
    state.plot_visibility.counts[1 * 80 + 3] = 0;
    const auto hidden_again = owner.prepare(client.capture(viewport, {}));
    check(owner.current(hidden_again) && (hidden_again->grid().at(1,0).byte2 & 1) &&
        hidden_again->identity().fog_payload_sha256 == first->identity().fog_payload_sha256,
        "Replay accumulated clears instead of starting with the pristine grid");
    ui::BnwMinimapFogPreparationOwner foreign(live, client, scene, initial, geometry());
    const auto foreign_packet = foreign.prepare(client.current_frame());
    check(!owner.current(foreign_packet) && owner.current(hidden_again),
        "Foreign packet accepted or revoked a valid packet");
    bool thread_rejected = false;
    std::thread worker([&] {
        thread_rejected = !owner.current(hidden_again) && !owner.source_context_matches(scene, geometry());
        rejects([&] { owner.prepare(client.current_frame()); });
    });
    worker.join();
    check(thread_rejected && owner.current(hidden_again), "Wrong-thread preparation affected publication");
    // Retained aliases are independently checked, even without a new frame stamp.
    state.plot_visibility.counts[3] = -1;
    check(!owner.current(hidden_again), "Negative current native count accepted");
    state.plot_visibility.counts[3] = 1;
    check(!owner.current(hidden_again), "Rejected packet revived after cache repair");
    const auto valid = owner.prepare(client.capture(viewport, {}));
    state.world.map_projection->cells[0].terrain_type = 6;
    check(!owner.current(valid), "Raw scene mutation did not require rebuilding minimap owner");
    rejects([&] { owner.prepare(client.capture(viewport, {})); });
    state.world.map_projection->cells[0].terrain_type = 2;
    auto altered = initial;
    altered.at(0,0).byte1 ^= 4;
    rejects([&] { ui::BnwMinimapFogPreparationOwner bad(live,client,scene,altered,geometry()); });
    altered = initial; altered.at(-1,-1).byte2 = 0;
    rejects([&] { ui::BnwMinimapFogPreparationOwner bad(live,client,scene,altered,geometry()); });
    altered = initial; altered.at(0,0).byte2 = 0;
    rejects([&] { ui::BnwMinimapFogPreparationOwner bad(live,client,scene,altered,geometry()); });
    auto bad_geometry = geometry(); bad_geometry.window = {INT32_MIN,0,INT32_MAX,0};
    rejects([&] { ui::BnwMinimapFogPreparationOwner bad(live,client,scene,initial,bad_geometry); });
    bad_geometry = geometry(); bad_geometry.transform.scale_x = std::numeric_limits<float>::infinity();
    rejects([&] { ui::BnwMinimapFogPreparationOwner bad(live,client,scene,initial,bad_geometry); });
    bad_geometry = geometry(); bad_geometry.edge_subdivisions = UINT32_MAX;
    rejects([&] { ui::BnwMinimapFogPreparationOwner bad(live,client,scene,initial,bad_geometry); });
    auto different = geometry(); different.window.right = 0;
    ui::BnwMinimapFogPreparationOwner narrower(live,client,scene,initial,different);
    const auto old_generation = owner.prepare(client.capture(viewport, {}));
    const auto narrow_packet = narrower.prepare(client.current_frame());
    check(narrow_packet->identity().context_sha256 != old_generation->identity().context_sha256,
        "Full scan extents were omitted from preparation context identity");
    const auto checkpoint = live.checkpoint();
    live.restore(checkpoint);
    check(!owner.current(old_generation), "Identical restoration retained old preparation owner");
    check(!owner.source_context_matches(scene, geometry()), "Replaced native owner retained source binding");
    const auto after_restore = client.capture(viewport, {});
    rejects([&] { owner.prepare(after_restore); });
    const auto restored_scene = bnw_frame_fixture::receipt(live);
    ui::BnwMinimapFogPreparationOwner rebound(live,client,restored_scene,
        ui::project_saved_minimap_terrain(*live.state().world.map_projection),geometry());
    check(rebound.current(rebound.prepare(after_restore)), "Explicit restored-owner rebuild did not prepare");
}
void final_callback_mutation() {
    auto live = bnw_frame_fixture::session();
    auto &state = live.mutable_state();
    auto &city = state.cities.players[7].cities[9];
    city.owner=7; city.id=9; city.population=5; city.x=0; city.y=0;
    runtime::rebuild_bnw_live_plot_visibility(state);
    int checks=0, trigger=2; bool armed=false;
    ui::LiveSessionUiSources sources;
    sources.city_banner_appearance=[](const auto &,auto active,auto player,auto id,auto frame) {
        ui::BnwCityBannerAppearance receipt;
        receipt.active_player_id=active; receipt.player_id=player; receipt.city_id=id;
        receipt.frame_number=frame; receipt.turn=12;
        return std::optional(receipt);
    };
    sources.city_banner_appearance_current=[&](const auto &, const auto &) {
        if (armed && ++checks == trigger) state.plot_visibility.counts[3]=0;
        return true;
    };
    ui::BnwClientFrameOwner client(live,"callback-minimap-fixture",7,sources);
    const auto frame = client.capture({800,600,800,600,96},{});
    ui::BnwMinimapFogPreparationOwner owner(live,client,bnw_frame_fixture::receipt(live),
        ui::project_saved_minimap_terrain(*live.state().world.map_projection),geometry());
    const auto accepted=owner.prepare(frame);
    check(owner.current(accepted), "Callback fixture did not establish a current packet");
    armed=true;
    rejects([&] { owner.prepare(frame); });
    check(checks==2 && !owner.current(accepted),
        "Mutation at final owner callback escaped semantic requery or preserved previous publication");
    armed=false; state.plot_visibility.counts[3]=1;
    const auto fresh=client.capture({800,600,800,600,96},{});
    const auto retained=owner.prepare(fresh);
    // The fourth check occurs after mesh/hash preparation, at the final
    // publication bracket. A failed bracket must publish neither candidate
    // nor the previously accepted packet.
    checks=0; trigger=4; armed=true;
    rejects([&] { owner.prepare(fresh); });
    check(checks==4 && !owner.current(retained),
        "Post-preparation callback mutation published CPU fog or retained its old packet");
    armed=false; state.plot_visibility.counts[3]=1;
    const auto source_frame=client.capture({800,600,800,600,96},{});
    const auto source_packet=owner.prepare(source_frame);
    checks=0; trigger=4; armed=true;
    check(!owner.source_context_matches(bnw_frame_fixture::receipt(live),geometry()) && checks==4,
        "Final source-context callback mutation escaped actual semantic requery");
    check(!owner.current(source_packet), "Source-context mutation left a valid CPU packet");
}
}
int main() {
    try { run(); final_callback_mutation(); std::cout << "PASS immutable current-client minimap fog preparation (constructed fixture; CPU only)\n"; }
    catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
