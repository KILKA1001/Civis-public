// Own-GL minimap fog consumption regression; installed assets remain read-only.
// Installed original DDS bytes; explicitly constructed two-plot CPU visibility
// fixture. This proves own GL consumption, not original generated fog pixels,
// production journal notification, native-base equivalence, or full-turn readiness.
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_texture.hpp"
using civ5::graphics::DdsSamplingColor;
using civ5::graphics::GlContext;
using civ5::graphics::GlRenderApi;
using civ5::graphics::GlTexture;
using civ5::graphics::RenderTextureFormat;
#include "../validation/gl_minimap_surface_diagnostic.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/ui/minimap_saved_map.hpp"
#include "bnw_client_scene_frame_fixture.hpp"
#include <iostream>
#include <type_traits>

namespace {
using namespace civ5;
using bnw_frame_fixture::check;
using bnw_frame_fixture::rejects;
using Renderer = GlMinimapSurfaceDiagnostic;
static_assert(!std::is_default_constructible_v<Renderer::AppliedFogReceipt>);
static_assert(!std::is_constructible_v<Renderer::AppliedFogReceipt, bool>);
assets::DdsData original_dds(const std::shared_ptr<assets::FpkSource> &archive,
                             std::string_view name) {
    const assets::FpkEntry *selected = nullptr;
    for (const auto &entry : archive->entries())
        if (assets::normalize_asset_name(assets::resource_basename(entry.original_name)) ==
            assets::normalize_asset_name(std::string(name))) {
            check(!selected, "Original DDS basename is ambiguous");
            selected = &entry;
        }
    check(selected, "Original minimap DDS is absent");
    const auto found = archive->find(assets::AssetKey(selected->original_name));
    const auto *handle = std::get_if<assets::AssetHandle>(&found);
    check(handle && handle->candidate().provenance.archive_relative_path == "Resource/DX9/MiscTextures.fpk",
          "DDS provenance changed");
    return assets::DdsData::parse(handle->read_bytes());
}
void render_fixture_base(Renderer &surface, const ui::MinimapRenderMeshes &meshes,
                         const Renderer::PrefogSource *source) {
    ui::MinimapMountainMarkers mountains{};
    mountains.radius = 1;
    surface.render(meshes, {}, mountains, {}, 8.0f, nullptr,
        {}, {}, {}, {}, {}, {}, 0.5f, {}, {}, {}, {}, {}, {}, source != nullptr, source);
}
void run(const std::filesystem::path &game_root) {
    const auto opened = assets::FpkSource::open(game_root / "Resource/DX9/MiscTextures.fpk",
                                                "Resource/DX9/MiscTextures.fpk");
    const auto *archive = std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
    check(archive && *archive, "Installed original MiscTextures FPK unavailable");
    const std::array<assets::DdsData, 6> textures{
        original_dds(*archive, "MiniMapWater.dds"), original_dds(*archive, "MiniMapShallowWater.dds"),
        original_dds(*archive, "MiniMapGrass.dds"), original_dds(*archive, "MiniMapFog.dds"),
        original_dds(*archive, "MiniMapMountain.dds"), original_dds(*archive, "MiniMapIce.dds")};
    auto live = bnw_frame_fixture::session();
    auto &state = live.mutable_state();
    state.world.map_projection->cells[1].revealed_teams->clear();
    state.world.map_plots->plots[1].arrays[1].values[0] = 0;
    auto &city = state.cities.players[7].cities[9];
    city.owner=7; city.id=9; city.population=5; city.x=0; city.y=0;
    runtime::rebuild_bnw_live_plot_visibility(state);
    const auto scene = bnw_frame_fixture::receipt(live);
    const auto initial = ui::project_saved_minimap_terrain(*live.state().world.map_projection);
    ui::BnwMinimapFogGeometry geometry;
    geometry.window = {0, 0, 1, 0};
    geometry.transform.cube_to_world[0] = geometry.transform.cube_to_world[5] = 1;
    geometry.transform.cube_to_world[10] = geometry.transform.cube_to_world[15] = 1;
    geometry.edge_subdivisions = 4;
    std::function<void()> fence_fault;
    bool fault_armed=false, fault_fired=false;
    ui::LiveSessionUiSources callback_sources;
    callback_sources.city_banner_appearance=[](const auto &,auto active,auto player,auto id,auto frame) {
        ui::BnwCityBannerAppearance r;
        r.active_player_id=active; r.player_id=player; r.city_id=id; r.frame_number=frame; r.turn=12;
        return std::optional(r);
    };
    callback_sources.city_banner_appearance_current=[&](const auto &,const auto &) {
        if(fault_armed) { fault_armed=false; fault_fired=true; fence_fault(); }
        return true;
    };
    ui::BnwClientFrameOwner client(live, "declared-constructed-minimap-source", 7, callback_sources);
    ui::BnwMinimapFogPreparationOwner preparation(live, client, scene, initial, geometry);
    auto packet = preparation.prepare(client.capture({800,600,800,600,96}, {}));
    check(preparation.source_context_matches(scene, geometry), "Real bound CPU context rejected");
    auto foreign_scene = scene;
    ++foreign_scene.active_team;
    check(!preparation.source_context_matches(foreign_scene, geometry), "Foreign team source accepted");
    auto foreign_geometry = geometry;
    foreign_geometry.transform.offset_x = 0.25f;
    check(!preparation.source_context_matches(scene, foreign_geometry), "Different source transform accepted");
    ui::MinimapVisibleBounds visible;
    visible.segments[0] = {-1, -1, 2, 1};
    visible.edge_subdivisions = geometry.edge_subdivisions;
    auto meshes = ui::build_minimap_render_meshes(initial, geometry.window, geometry.transform, visible,
        [](const ui::MinimapEdgeQuery &) -> std::optional<ui::MinimapTerrainEdgeRecord> { return std::nullopt; });
    const Renderer::PrefogSource source{scene, geometry};
    auto context = std::make_shared<GlContext>();
    Renderer surface(context, textures);
    render_fixture_base(surface, meshes, &source);
    auto destination = std::move(surface.output);
    auto partial = surface.apply_current_fog(preparation, packet, destination);
    check(partial.own_gl_applied() && !partial.independent_native_base_proof() &&
        !partial.captured_original_pixel_parity() && partial.prefog_sha256().size() == 64 &&
        surface.current(partial, preparation, packet, destination), "Actual own GL receipt absent");
    const auto partially_hidden_pixels = destination.read_mip_raw(0);
    // The borrowed receipt object itself is destroyed by a real owner fence.
    // The genuine private GPU publication remains current: only a local copy
    // may be used after this callback. This intentionally permits true.
    std::optional<Renderer::AppliedFogReceipt> borrowed_receipt=partial;
    fence_fault=[&]{ borrowed_receipt.reset(); }; fault_armed=true;
    check(surface.current(*borrowed_receipt,preparation,packet,destination) &&
        fault_fired && !borrowed_receipt && surface.current(partial,preparation,packet,destination),
        "Destroyed optional receipt was reread or genuine retained publication rejected");
    fault_fired=false;
    const auto old_packet=packet;
    fence_fault=[&]{ packet.reset(); }; fault_armed=true;
    check(!surface.current(partial,preparation,packet,destination) && fault_fired && !packet,
        "GPU current query accepted caller packet reset during source check");
    packet=old_packet;
    check(surface.current(partial,preparation,packet,destination),
        "Read-only query fault corrupted genuine GPU publication");
    fault_fired=false; fault_armed=true;
    rejects([&]{ surface.apply_current_fog(preparation,packet,destination); });
    check(fault_fired && !packet && destination.read_mip_raw(0)==partially_hidden_pixels &&
        !surface.current(partial,preparation,old_packet,destination),
        "Reset apply input wrote target or retained its previous GPU receipt");
    packet=old_packet;
    partial=surface.apply_current_fog(preparation,packet,destination);
    check(surface.current(partial,preparation,packet,destination),
        "Explicit own GL reapply after callback reset failed");
    // Controlled cache-only fault/recapture, not a production mutator/journal proof.
    state.plot_visibility.counts[1 * 80 + 3] = 1;
    check(!surface.current(partial, preparation, packet, destination), "Unnotified source mutation accepted");
    packet = preparation.prepare(client.capture({800,600,800,600,96}, {}));
    check(packet->fog_mesh().triangles.empty(), "Fixture did not open both cells");
    const auto all_open = surface.apply_current_fog(preparation, packet, destination);
    const auto all_open_pixels = destination.read_mip_raw(0);
    check(all_open_pixels != partially_hidden_pixels && all_open.prefog_sha256() == partial.prefog_sha256(),
        "Empty fog failed to restore retained pre-fog pixels");
    // Replay the same existing renderer's static passes with no fog as an
    // independent no-fog control; no second renderer or fabricated base bytes.
    auto no_fog = meshes;
    no_fog.fog.triangles.clear();
    render_fixture_base(surface, no_fog, nullptr);
    check(destination.read_mip_raw(0) == all_open_pixels,
        "All-open pixels differ from actual static pass control");
    check(!surface.current(all_open, preparation, packet, destination), "Static rerender retained old proof");
    // Retention reads output; return its live wrapper for a fresh base epoch.
    surface.output = std::move(destination);
    render_fixture_base(surface, meshes, &source);
    destination = std::move(surface.output);
    const auto refreshed = surface.apply_current_fog(preparation, packet, destination);
    auto foreign_target = GlTexture::render_target(*context, 320, 190, 1, RenderTextureFormat::rgba8);
    rejects([&] { surface.apply_current_fog(preparation, packet, foreign_target); });
    check(!surface.current(refreshed, preparation, packet, destination),
        "Failed application retained old receipt");
    const auto correct = surface.apply_current_fog(preparation, packet, destination);
    surface.gl.NamedFramebufferTexture(surface.framebuffer, GL_COLOR_ATTACHMENT0, foreign_target.id(), 0);
    check(!surface.current(correct, preparation, packet, destination), "Foreign FBO color attachment accepted");
    surface.gl.NamedFramebufferTexture(surface.framebuffer, GL_COLOR_ATTACHMENT0, destination.id(), 0);
    auto original_stencil = std::move(surface.ice_stencil);
    surface.ice_stencil = GlTexture::render_target(*context, 16, 16, 1, RenderTextureFormat::depth24_stencil8);
    surface.gl.NamedFramebufferTexture(surface.framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, surface.ice_stencil.id(), 0);
    rejects([&] { surface.apply_current_fog(preparation, packet, destination); });
    surface.ice_stencil = std::move(original_stencil);
    surface.gl.NamedFramebufferTexture(surface.framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, surface.ice_stencil.id(), 0);
    const auto restored = surface.apply_current_fog(preparation, packet, destination);
    auto foreign_context = std::make_shared<GlContext>();
    const auto actual_foreign = wglGetCurrentContext();
    check(!surface.current(restored, preparation, packet, destination) &&
        wglGetCurrentContext() == actual_foreign, "Receipt query implicitly switched a foreign WGL context");
    context->check("Explicit test return to owned WGL context");
    foreign_context.reset();
    context->check("Owned WGL context after foreign context destruction");
    check(surface.current(restored, preparation, packet, destination),
        "Owned context/target/source was not current before retirement probe");
    client.notify_world_changed();
    check(!surface.current(restored, preparation, packet, destination) &&
        !preparation.source_context_matches(scene, geometry), "Retired client source accepted");
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 2, "Expected original installed game root");
        run(argv[1]);
        std::cout << "PASS own GL minimap fog consumption; original pixel parity false\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
