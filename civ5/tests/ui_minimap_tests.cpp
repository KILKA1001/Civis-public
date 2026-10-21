#include "civ5/assets/asset_source.hpp"
#include "civ5/assets/dds.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/assets/ui_block_atlas.hpp"
#include "civ5/content/info_types.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/ui/minimap_draw_batches.hpp"
#include "civ5/ui/minimap_city_markers.hpp"
#include "civ5/ui/minimap_coast_edge_candidates.hpp"
#include "civ5/ui/minimap_hex_geometry.hpp"
#include "civ5/ui/minimap_ice_regions.hpp"
#include "civ5/ui/minimap_ice_probe_mesh.hpp"
#include "civ5/ui/minimap_mountain_markers.hpp"
#include "civ5/ui/minimap_notification_pulse.hpp"
#include "civ5/ui/minimap_native_bounds_fixture.hpp"
#include "civ5/ui/minimap_masked_hex_mesh.hpp"
#include "civ5/ui/minimap_panel.hpp"
#include "civ5/ui/minimap_plain_hex_mesh.hpp"
#include "civ5/ui/minimap_projection.hpp"
#include "civ5/ui/minimap_saved_map.hpp"
#include "civ5/ui/minimap_captured_gandhi_culture.hpp"
#include "civ5/ui/minimap_saved_ice_plots.hpp"
#include "civ5/ui/minimap_saved_reveal.hpp"
#include "civ5/ui/minimap_scan_window.hpp"
#include "civ5/ui/minimap_tile_grid.hpp"
#include "civ5/ui/minimap_visible_bounds.hpp"
#include "civ5/ui/minimap_viewport_ring.hpp"
#include "civ5/ui/minimap_world_bridge.hpp"
#include "civ5/ui/minimap_render_meshes.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
using namespace civ5;
namespace {
int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
template <class Action> void rejects(Action action) {
    bool failed = false;
    try { action(); } catch (const std::exception &) { failed = true; }
    check(failed, "Invalid MiniMapPanel source or viewport was accepted");
}
} // namespace

int main(int argc, char **argv) {
    const bool saved_run = (argc == 3 || argc == 5) &&
                           std::string_view(argv[1]) == "--save" &&
                           (argc == 3 || std::string_view(argv[3]) == "--grid-output" ||
                            std::string_view(argv[3]) == "--revealed-grid-output" ||
                            std::string_view(argv[3]) == "--ice-mesh-output" ||
                            std::string_view(argv[3]) == "--surface-mesh-output" ||
                            std::string_view(argv[3]) == "--city-candidate-output");
    check(argc == 1 || saved_run,
          "MiniMapPanel test accepts --save path [--grid-output|--revealed-grid-output|--ice-mesh-output|--surface-mesh-output|--city-candidate-output path]");
    auto root = fs::current_path();
    for (unsigned i = 0; i != 8 && !fs::exists(root / "civ5-assets/Assets/UI/InGame/WorldView/MiniMapPanel.xml"); ++i)
        root = root.parent_path();
    check(fs::exists(root / "civ5-assets/Assets/UI/InGame/WorldView/MiniMapPanel.xml"),
          "Retail MiniMapPanel XML is unavailable");
    const auto asset_root = root / "civ5-assets";
    constexpr std::string_view gandhi_sha =
        "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab";
    constexpr std::string_view retail_exe_sha =
        "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686";
    constexpr std::string_view g0_tga_sha =
        "5f588c60ccf9919545c0684401374ca7dcfe5e04f54ebd490abaec1aebe1cd87";
    // The private managed worktree can have a non-ASCII path under Windows;
    // the test override points at a byte-identical ASCII-path copy there.
    const auto native_bounds_path = std::getenv("CIVIS_MINIMAP_BOUNDS_FIXTURE")
        ? fs::path(std::getenv("CIVIS_MINIMAP_BOUNDS_FIXTURE"))
        : root / "tools/civ5_architecture/fixtures/"
                 "gandhi-b3a54d5c-native-minimap-bounds.bin";
    const auto native_bounds = ui::MinimapNativeBoundsFixture::read(
        native_bounds_path, gandhi_sha, retail_exe_sha, g0_tga_sha);
    const auto &native_box = native_bounds.visible.segments[0];
    check(native_bounds.visible.segment_count == 1 &&
          native_bounds.visible.texture.padding_x == 3 &&
          native_bounds.visible.texture.active_width == 313 &&
          std::abs(native_box.min_x - 230.368469f) < .001f &&
          std::abs(native_box.min_y + 108.865364f) < .001f &&
          std::abs(native_box.max_x - 1593.562988f) < .001f &&
          std::abs(native_box.max_y - 718.632874f) < .001f &&
          std::abs(native_bounds.plane_height - 10.11673164f) < 1e-5f &&
          std::abs(native_bounds.reference_marker_map_local[0] - 141.1328125f) < .001f &&
          std::abs(native_bounds.reference_marker_map_local[1] - 126.796875f) < .001f,
          "Passive native minimap bounds differ from same-save DX9 reference");
    {
        using namespace std::chrono_literals;
        using Status = ui::MinimapNotificationStatus;
        auto now = std::chrono::steady_clock::time_point{};
        bool render_target_ready = false;
        ui::MinimapNotificationPulseContext context;
        context.map_width = 10;
        context.map_height = 10;
        context.player_id = 7;
        context.turn = 12;
        context.lifetime = 1500ms;
        context.transform.cube_to_world = {1, 0, 0, 0, 0, 1, 0, 0,
                                            0, 0, 1, 0, 0, 0, 0, 1};
        context.visible.segments[0] = {0, 0, 10, 10};
        context.visible.texture = native_bounds.visible.texture;
        ui::MinimapNotificationPulseOwner pulses(context,
            [&] { return render_target_ready; }, [&] { return now; });
        check(pulses.emit_minimap_notification(2, 3, 5, 7, 12, 1).status ==
                  Status::unavailable && pulses.active().empty(),
              "A missing minimap render target must not report a queued pulse");
        check(pulses.emit_minimap_notification(-1, -1, 5, 7, 12, 1).status ==
                  Status::no_spatial_marker && pulses.active().empty(),
              "BNW technology notification sentinel must not create a dot");
        auto no_style = context;
        no_style.lifetime = 0ms;
        ui::MinimapNotificationPulseOwner nonspatial(no_style);
        check(nonspatial.emit_minimap_notification(-1, -1, 5, 7, 12, 1).status ==
                  Status::no_spatial_marker,
              "Nonspatial BNW notification must not depend on unknown dot timing");
        render_target_ready = true;
        const auto result = pulses.emit_minimap_notification(2, 3, 6, 7, 12, 2);
        const auto active = pulses.active();
        check(result.accepted() && result.status == Status::queued_visible &&
                  active.size() == 1 && active[0].marker_id == 6 &&
                  active[0].player_id == 7 && active[0].turn == 12 &&
                  active[0].owner_sequence == 2 &&
                  std::abs(active[0].texture_pixel.x - 34.3f) < .01f &&
                  std::abs(active[0].texture_pixel.y - 133.0f) < .01f,
              "A spatial notification must project through the active minimap bounds");
        check(pulses.emit_minimap_notification(2, 3, 6, 7, 12, 3).status ==
                  Status::unavailable && pulses.active().size() == 1 &&
                  pulses.emit_minimap_notification(2, 3, 7, 8, 12, 3).status ==
                  Status::stale &&
                  pulses.emit_minimap_notification(2, -1, 7, 7, 12, 3).status ==
                  Status::invalid,
              "Duplicate, foreign and mixed-sentinel notifications must fail closed");
        now += 1499ms;
        check(pulses.active().size() == 1,
              "Minimap notification expired before its configured lifetime");
        now += 1ms;
        check(pulses.active().empty(),
              "Minimap notification did not expire at its configured lifetime");
        auto partial = context;
        partial.visible.segments[0] = {0, 0, .5f, .5f};
        ui::MinimapNotificationPulseOwner outside(partial, [] { return true; });
        check(outside.emit_minimap_notification(2, 3, 8, 7, 12, 1).status ==
                  Status::queued_outside_view && outside.active().size() == 1 &&
                  !outside.active()[0].inside_view,
              "A valid offscreen plot must retain its event without a visible dot");
    }
    rejects([&] { (void)ui::MinimapNativeBoundsFixture::read(
        native_bounds_path, retail_exe_sha, gandhi_sha, g0_tga_sha); });
    rejects([&] { (void)ui::MinimapNativeBoundsFixture::read(
        native_bounds_path, gandhi_sha, retail_exe_sha, gandhi_sha); });
    std::ifstream native_bounds_input(native_bounds_path, std::ios::binary);
    std::vector<std::uint8_t> native_bytes((std::istreambuf_iterator<char>(native_bounds_input)),
                                            std::istreambuf_iterator<char>());
    native_bytes[200] = 0;
    rejects([&] { (void)ui::MinimapNativeBoundsFixture::parse(
        native_bytes, gandhi_sha, retail_exe_sha, g0_tga_sha); });
    const auto vanilla_key = std::string("Assets/UI/InGame/WorldView/MiniMapPanel.xml");
    const auto bnw_key = std::string("Assets/DLC/Expansion2/UI/InGame/WorldView/MiniMapPanel.xml");
    auto base_source = std::make_shared<assets::LooseResourceIndex>(asset_root);
    base_source->add_file(asset_root / vanilla_key);
    auto bnw_source = std::make_shared<assets::LooseResourceIndex>(asset_root);
    bnw_source->add_file(asset_root / bnw_key);
    assets::ResourceMountTable vfs(assets::resource_lookup(base_source), {},
                                   assets::resource_keys(base_source));

    const auto vanilla = ui::read_minimap_panel(vfs);
    vfs.mount({200, 1, 0, assets::resource_lookup(bnw_source), true, true,
               assets::resource_lookup_all(bnw_source), assets::resource_keys(bnw_source)});
    const auto bnw = ui::read_minimap_panel(vfs);
    const auto vanilla_file = ui::read_minimap_panel(asset_root / vanilla_key);
    check(vanilla.frame.x == -5 && vanilla.frame.y == -39 &&
              vanilla_file.frame.x == vanilla.frame.x && vanilla_file.map.width == vanilla.map.width,
          "Base MiniMapPanel layout differs between file and VFS");
    check(bnw.source_key == bnw_key && vanilla.source_key == vanilla_key &&
              bnw.frame.x == -8 && bnw.frame.y == -39 &&
              bnw.corner_texture == "assets/DLC/Expansion/UI/Art/Controls/LowerRightCorner_MiniMap.dds" &&
              vanilla.corner_texture == "TopFrame2.dds",
          "BNW MiniMapPanel skin override was lost");
    check(bnw.frame_style == "Grid9DetailSix140" && bnw.background.width == 128 &&
              bnw.background.height == 224 && bnw.map.width == 320 && bnw.map.height == 190 &&
              bnw.strategic.width == 45 && bnw.options.height == 45 &&
              bnw.corner.x == 3 && bnw.corner.y == -33 &&
              bnw.corner.width == 101 && bnw.corner.height == 72,
          "MiniMapPanel authored sizes changed");
    check(bnw.options_panel_style == "Grid9DetailSix140" &&
              bnw.map_options.size() == 5 && vanilla.map_options.size() == 4 &&
              bnw.map_options[0].id == "HideRecommendation" &&
              bnw.map_options[4].id == "ShowTrade" &&
              bnw.map_options[4].text_key == "TXT_KEY_MAP_OPTIONS_TRADE_ROUTES",
          "BNW options shell or five authored map display controls changed");
    const auto plan = ui::plan_minimap_panel(bnw, 1280, 960);
    check(plan.frame.x == 933 && plan.frame.y == 723 &&
              plan.map.x == 951 && plan.map.y == 766 &&
              plan.background.x == 815 && plan.background.y == 736 &&
              plan.strategic.x == 889 && plan.strategic.y == 914 &&
              plan.options.x == 889 && plan.options.y == 869 &&
              plan.corner.x == 1184 && plan.corner.y == 690,
          "BNW right-bottom anchoring or top-growing button stack changed");
    check(plan.options_panel.x == 603 && plan.options_panel.y == 699 &&
              plan.options_panel.width == 300 && plan.options_panel.height == 300,
          "BNW options shell source rectangle changed");
    const auto square = ui::plan_minimap_texture_geometry(100.0f, 100.0f);
    const auto moderate = ui::plan_minimap_texture_geometry(150.0f, 100.0f);
    const auto almost_full = ui::plan_minimap_texture_geometry(168.0f, 100.0f);
    const auto full = ui::plan_minimap_texture_geometry(200.0f, 100.0f);
    const auto exact_aspect = ui::plan_minimap_texture_geometry(320.0f, 190.0f);
    check(square.padding_x == 65 && square.active_width == 189 &&
              moderate.padding_x == 18 && moderate.active_width == 284 &&
              almost_full.padding_x == 0 && almost_full.active_width == 319 &&
              full.padding_x == 0 && full.active_width == 320 &&
              exact_aspect.padding_x == 0 && exact_aspect.active_width == 320,
          "Native minimap horizontal padding or x87 truncation changed");
    check(square.active_aspect == 189.0f / 190.0f &&
              full.active_aspect == 320.0f / 190.0f,
          "Native minimap active aspect changed");
    rejects([&] { ui::plan_minimap_texture_geometry(0.0f, 100.0f); });
    rejects([&] { ui::plan_minimap_texture_geometry(100.0f, 0.0f); });
    rejects([&] { ui::plan_minimap_texture_geometry(
                    std::numeric_limits<float>::infinity(), 100.0f); });
    ui::MinimapTileGrid tile_grid(40, 24, true);
    check(tile_grid.records().size() == 256u * 256u &&
              tile_grid.records().front().byte0 == 0 &&
              tile_grid.records().front().byte1 == 3 &&
              tile_grid.records().front().byte2 == 0 &&
              tile_grid.records().front().byte3 == 0,
          "Native minimap tile allocation or initial record bytes changed");
    tile_grid.at(0, 1).byte0 = 0x12;
    tile_grid.at(1, 0).byte0 = 0x34;
    check(tile_grid.records()[256].byte0 == 0x12 &&
              tile_grid.records()[1].byte0 == 0x34 &&
              tile_grid.at(-40, 1).byte0 == 0x12 &&
              tile_grid.at(41, 0).byte0 == 0x34,
          "Native minimap row stride or horizontal wrap changed");
    check(&tile_grid.at(0, -1) == &tile_grid.sentinel() &&
              &tile_grid.at(0, 24) == &tile_grid.sentinel(),
          "Native minimap vertical bounds changed");
    ui::MinimapTileGrid no_wrap(40, 24, false);
    check(&no_wrap.at(-1, 0) == &no_wrap.sentinel() &&
              &no_wrap.at(40, 0) == &no_wrap.sentinel(),
          "Native minimap nonwrapping horizontal bounds changed");
    check(tile_grid.populate_terrain_cell(4, 6, 0) &&
              tile_grid.populate_terrain_cell(5, 6, 5) &&
              tile_grid.populate_terrain_cell(6, 6, 6) &&
              tile_grid.at(4, 6).byte1 == 0 &&
              tile_grid.at(5, 6).byte1 == 1 &&
              tile_grid.at(6, 6).byte1 == 2 &&
              tile_grid.at(4, 6).byte2 == 1 &&
              tile_grid.at(5, 6).byte2 == 1 &&
              tile_grid.at(6, 6).byte2 == 1 &&
              tile_grid.sentinel().byte2 == 1,
          "Native minimap land, coast, ocean, or initial fog classification changed");
    tile_grid.at(5, 6).byte0 = 7;
    tile_grid.at(5, 6).byte3 = 0xfc;
    check(tile_grid.populate_terrain_cell(5, 6, 3) &&
              tile_grid.at(5, 6).byte0 == 0 &&
              tile_grid.at(5, 6).byte1 == 0 &&
              tile_grid.at(5, 6).byte3 == 0 &&
              !tile_grid.populate_terrain_cell(-1, 6, 5) &&
              !tile_grid.populate_terrain_cell(40, 6, 5) &&
              tile_grid.sentinel().byte1 == 3,
          "Native minimap terrain population did not reset the plot or reject invalid cells");
    ui::MinimapTileGrid terrain_edges(5, 5, false);
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            check(terrain_edges.populate_terrain_cell(x, y, 0),
                  "Native minimap terrain population rejected an in-bounds plot");
    terrain_edges.populate_terrain_cell(2, 2, 5);
    terrain_edges.build_initial_terrain_edges();
    check(terrain_edges.at(2, 2).byte1 == 0xfd &&
              (terrain_edges.at(2, 3).byte1 & 0x20) != 0 &&
              (terrain_edges.at(1, 2).byte1 & 0x40) != 0 &&
              (terrain_edges.at(2, 1).byte1 & 0x80) != 0 &&
              (terrain_edges.at(1, 1).byte1 & 0x04) != 0 &&
              (terrain_edges.at(3, 2).byte1 & 0x08) != 0 &&
              (terrain_edges.at(1, 3).byte1 & 0x10) != 0 &&
              (terrain_edges.at(2, 0).byte1 & 0x30) == 0x30,
          "Native minimap coastline edge directions or map-boundary sentinel changed");
    ui::MinimapTileGrid wrapping_edges(2, 3, true);
    ui::MinimapTileGrid bounded_edges(2, 3, false);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 2; ++x) {
            wrapping_edges.populate_terrain_cell(x, y, 0);
            bounded_edges.populate_terrain_cell(x, y, 0);
        }
    wrapping_edges.build_initial_terrain_edges();
    bounded_edges.build_initial_terrain_edges();
    check((wrapping_edges.at(0, 1).byte1 & 0x08) == 0 &&
              (bounded_edges.at(0, 1).byte1 & 0x08) != 0,
          "Native minimap terrain borders did not respect horizontal wrap");
    saves::SavedMapProjection saved_minimap{};
    saved_minimap.grid = {3, 3};
    saved_minimap.cells.resize(9);
    for (std::uint64_t y = 0; y < 3; ++y)
        for (std::uint64_t x = 0; x < 3; ++x) {
            auto &cell = saved_minimap.cells[static_cast<std::size_t>(y * 3 + x)];
            cell.index = y * 3 + x;
            cell.x = x;
            cell.y = y;
            cell.terrain_type = 0;
        }
    saved_minimap.cells[4].terrain_type = 5;
    saved_minimap.cells[4].owner = 0;
    const auto saved_tiles = ui::project_saved_minimap_terrain(saved_minimap);
    check(saved_tiles.at(1, 1).byte1 == 0xfd &&
              saved_tiles.at(1, 1).byte0 == 0 &&
              saved_tiles.at(1, 1).byte2 == 1 &&
              (saved_tiles.at(0, 1).byte1 & 0x40) != 0,
          "Saved map minimap terrain projection or coastline classification changed");
    saves::SavedMapTeamArtObservation synthetic_art;
    synthetic_art.plots.resize(9);
    for (std::size_t index = 0; index < synthetic_art.plots.size(); ++index) {
        synthetic_art.plots[index].plot_index = index;
        synthetic_art.plots[index].revealed = false;
    }
    synthetic_art.plots[4].revealed = true;
    auto revealed_tiles = saved_tiles;
    check(ui::apply_saved_minimap_reveals(revealed_tiles, synthetic_art) == 1 &&
              revealed_tiles.at(1, 1).byte2 == 0xfc &&
              (revealed_tiles.at(1, 0).byte2 & 0x04) != 0 &&
              (revealed_tiles.at(0, 1).byte2 & 0x40) != 0,
          "Saved active-team reveal did not replay native minimap fog edges");
    synthetic_art.plots[8].revealed.reset();
    auto unchanged_tiles = saved_tiles;
    rejects([&] { ui::apply_saved_minimap_reveals(unchanged_tiles, synthetic_art); });
    check(unchanged_tiles.at(1, 1).byte2 == 1,
          "Incomplete saved reveal modified minimap fog before rejection");
    synthetic_art.plots[8].revealed = false;
    auto draw_tiles = saved_tiles;
    draw_tiles.at(0, 0).byte0 = 2;
    draw_tiles.at(0, 0).byte3 = 0xfc;
    draw_tiles.at(0, 2).byte0 = 2;
    draw_tiles.at(2, 2).byte0 = 5;
    draw_tiles.at(1, 1).byte2 = 0;
    const auto batches = ui::build_minimap_draw_batches(draw_tiles, {0, 0, 2, 2});
    check(batches.fog.size() == 8 && batches.coast.size() == 1 &&
              batches.land.size() == 8 && batches.culture.size() == 2 &&
              batches.fog[0] == ui::MinimapDrawCell{0, 0, 0} &&
              batches.coast[0] == ui::MinimapDrawCell{1, 1, 0x3f} &&
              batches.culture[0].payload == 2 && batches.culture[0].cells.size() == 2 &&
              batches.culture[0].cells[0] == ui::MinimapDrawCell{0, 0, 0x3f} &&
              batches.culture[0].cells[1].x == 0 && batches.culture[0].cells[1].y == 2 &&
              batches.culture[1].payload == 5 && batches.culture[1].cells.size() == 1,
          "Native minimap four scans, edge masks, or culture groups changed");
    const auto border_batches = ui::build_minimap_draw_batches(draw_tiles, {-1, -1, 3, 3});
    check(border_batches.fog.size() == 24 && border_batches.coast.size() == 1 &&
              border_batches.land.size() == 8 &&
              border_batches.fog.front() == ui::MinimapDrawCell{-1, -1, 0},
          "Native minimap out-of-bounds sentinel scan changed");
    rejects([&] { ui::build_minimap_draw_batches(draw_tiles, {2, 0, 1, 2}); });
    constexpr std::array<std::array<std::uint32_t, 4>, 6> native_corner_bits{{
        {0x3f2aaaabu, 0xbeaaaaabu, 0xbeaaaaabu, 0x3f800000u},
        {0x3eaaaaabu, 0x3eaaaaabu, 0xbf2aaaabu, 0x3f800000u},
        {0xbeaaaaabu, 0x3f2aaaabu, 0xbeaaaaabu, 0x3f800000u},
        {0xbf2aaaabu, 0x3eaaaaabu, 0x3eaaaaabu, 0x3f800000u},
        {0xbeaaaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0x3f800000u},
        {0x3eaaaaabu, 0xbf2aaaabu, 0x3eaaaaabu, 0x3f800000u},
    }};
    for (std::size_t corner = 0; corner < native_corner_bits.size(); ++corner)
        for (std::size_t lane = 0; lane < native_corner_bits[corner].size(); ++lane)
            check(std::bit_cast<std::uint32_t>(ui::minimap_cube_corners[corner][lane]) ==
                      native_corner_bits[corner][lane],
                  "Native DX9 minimap hex-corner float literal changed");
    ui::MinimapWorldTransform hex_transform;
    hex_transform.cube_to_world = {1, 0, 0, 0, 0, 1, 0, 0,
                                   0, 0, 1, 0, 0, 0, 0, 1};
    const auto hexes = ui::project_minimap_hexes(saved_tiles, hex_transform);
    check(hexes.size() == 9 && hexes[0].cube == std::array<std::int32_t, 3>{0, 0, 0} &&
              hexes[5].cube == std::array<std::int32_t, 3>{2, 1, -3} &&
              hexes[7].cube == std::array<std::int32_t, 3>{0, 2, -2} &&
              hexes[5].center.x == 2.0f && hexes[5].center.y == 1.0f &&
              std::abs(hexes[5].corners[0].x - 8.0f / 3.0f) < 1e-6f &&
              std::abs(hexes[5].corners[0].y - 2.0f / 3.0f) < 1e-6f,
          "Native minimap cube geometry or saved row traversal changed");
    const auto fog_mesh = ui::build_minimap_plain_hex_mesh(batches.fog, hex_transform);
    const auto clip_projection = ui::minimap_clip_projection(-10.0f, -5.0f,
                                                            30.0f, 15.0f);
    const auto projected_bottom_left = clip_projection.project({-10.0f, -5.0f});
    const auto projected_top_right = clip_projection.project({30.0f, 15.0f});
    const auto projected_center = clip_projection.project({10.0f, 5.0f});
    const auto water_bottom_left = clip_projection.project_visible_quad(
        {0.0f, 0.0f}, {-6.0f, -3.0f}, {22.0f, 13.0f});
    const auto water_top_right = clip_projection.project_visible_quad(
        {1.0f, 1.0f}, {-6.0f, -3.0f}, {22.0f, 13.0f});
    const auto water_interior = clip_projection.project_visible_quad(
        {0.25f, 0.75f}, {-6.0f, -3.0f}, {22.0f, 13.0f});
    check(clip_projection.offset.x == 0.5f && clip_projection.offset.y == 0.5f &&
              clip_projection.scale.x == 0.05f && clip_projection.scale.y == 0.1f &&
              projected_bottom_left.clip.x == -1.0f &&
              projected_bottom_left.clip.y == -1.0f &&
              projected_bottom_left.fog_uv.x == 0.0f &&
              projected_bottom_left.fog_uv.y == 1.0f &&
              projected_top_right.clip.x == 1.0f &&
              projected_top_right.clip.y == 1.0f &&
              projected_top_right.fog_uv.x == 1.0f &&
              projected_top_right.fog_uv.y == 0.0f &&
              projected_center.world_uv.x == 10.0f &&
              projected_center.world_uv.y == 5.0f &&
              projected_center.fog_uv.x == 0.5f &&
              projected_center.fog_uv.y == 0.5f,
          "Native minimap clip projection or fog Y orientation changed");
    rejects([&] { ui::minimap_clip_projection(0.0f, 0.0f, 0.0f, 1.0f); });
    check(water_bottom_left.world_uv.x == -6.0f &&
              water_bottom_left.world_uv.y == -3.0f &&
              water_top_right.world_uv.x == 22.0f &&
              water_top_right.world_uv.y == 13.0f &&
              water_interior.world_uv.x == 1.0f &&
              water_interior.world_uv.y == 9.0f &&
              std::abs(water_interior.clip.x + 0.45f) < 1e-6f &&
              std::abs(water_interior.clip.y - 0.4f) < 1e-6f &&
              std::abs(water_interior.fog_uv.y - 0.3f) < 1e-6f,
          "Native minimap water quad world interpolation or Y orientation changed");
    const auto water_uv = ui::minimap_water_texture_uv(water_interior.world_uv);
    check(std::bit_cast<std::uint32_t>(ui::minimap_water_texture_tiling.x) ==
                  0x3b03126fu &&
              water_uv.x == 100.002f && water_uv.y == 100.018f,
          "Native minimap water texture tiling changed");
    check(std::bit_cast<std::uint32_t>(ui::minimap_culture_texture_tiling.x) ==
                  0x3b03126fu &&
              std::bit_cast<std::uint32_t>(ui::minimap_culture_texture_tiling.y) ==
                  0x43530000u &&
              std::bit_cast<std::uint32_t>(ui::minimap_ice_texture_tiling.x) ==
                  0x3b03126fu &&
              std::bit_cast<std::uint32_t>(ui::minimap_ice_texture_tiling.y) ==
                  0x43300000u,
          "Native minimap culture or ice texture tiling changed");
    check(ui::minimap_sampler_2 ==
              std::array<std::uint32_t, 8>{1, 1, 1, 2, 2, 2, 1, 1},
          "Native minimap sampler 2 wrap, filtering or sRGB decode changed");
    constexpr std::array<std::uint32_t, 18> native_plain_indices{
        0, 2, 3, 0, 4, 5, 0, 6, 1, 0, 5, 6, 0, 1, 2, 0, 3, 4};
    check(fog_mesh.vertices.size() == 8 * 7 && fog_mesh.indices.size() == 8 * 18 &&
              fog_mesh.vertices[0].x == 0.0f && fog_mesh.vertices[0].y == 0.0f &&
              fog_mesh.indices[0] == native_plain_indices[0] &&
              std::equal(native_plain_indices.begin(), native_plain_indices.end(),
                         fog_mesh.indices.begin()),
          "Native minimap plain-hex vertex count or triangle fan changed");
    rejects([&] { ui::build_minimap_plain_hex_mesh(batches.coast, hex_transform); });
    const std::vector<ui::MinimapPoint2> native_curve_points{
        {-2.0f, 0.0f}, {-1.0f, 2.0f}, {1.0f, 2.0f}, {2.0f, 0.0f}};
    const auto curve_start = ui::sample_minimap_boundary_curve(native_curve_points, 0.0f);
    const auto curve_quarter = ui::sample_minimap_boundary_curve(native_curve_points, 0.5f);
    const auto curve_middle = ui::sample_minimap_boundary_curve(native_curve_points, 1.0f);
    const auto curve_end = ui::sample_minimap_boundary_curve(native_curve_points, 2.0f);
    check(curve_start.x == -1.5f && curve_start.y == 1.0f &&
              curve_quarter.x == -0.875f && curve_quarter.y == 1.75f &&
              curve_middle.x == 0.0f && curve_middle.y == 2.0f &&
              curve_end.x == 1.5f && curve_end.y == 1.0f,
          "Native minimap three-point boundary interpolation changed");
    std::array<std::vector<ui::MinimapPoint2>, 6> boundary_curves{};
    boundary_curves[4] = native_curve_points;
    constexpr std::array<std::array<std::uint8_t, 3>, 6> native_masked_edges{{
        {4, 0, 1}, {0, 1, 2}, {5, 2, 3},
        {1, 3, 4}, {3, 4, 5}, {2, 5, 0},
    }};
    check(ui::minimap_masked_edges == native_masked_edges,
          "Native minimap edge direction and corner lookup table changed");
    const auto isolated_queries = ui::minimap_boundary_edge_queries({0, 0, 0}, 4, 1u << 4);
    const auto connected_queries = ui::minimap_boundary_edge_queries(
        {0, 0, 0}, 4, (1u << 4) | (1u << 2) | 1u);
    check(isolated_queries == std::array<ui::MinimapEdgeQuery, 3>{{
                  {{{1, -1, 0}}, 0}, {{{0, 0, 0}}, 4}, {{{0, 1, -1}}, 2}}} &&
              connected_queries == std::array<ui::MinimapEdgeQuery, 3>{{
                  {{{0, 0, 0}}, 2}, {{{0, 0, 0}}, 4}, {{{0, 0, 0}}, 0}}},
          "Native minimap adjacent-edge lookup or neighbor cube steps changed");
    const auto synthetic_edge = ui::minimap_synthetic_edge_polyline(
        ui::MinimapEdgeQuery{{0, 0, 0}, 4}, hex_transform);
    check(std::abs(synthetic_edge[0].x - 5.0f / 12.0f) < 1e-6f &&
              std::abs(synthetic_edge[0].y - 1.0f / 6.0f) < 1e-6f &&
              std::abs(synthetic_edge[1].x - 7.0f / 12.0f) < 1e-6f &&
              std::abs(synthetic_edge[1].y + 1.0f / 6.0f) < 1e-6f,
          "Native minimap synthetic quarter-edge endpoints changed");
    auto edge_owner = hex_transform;
    edge_owner.scale_x = 2.0f;
    edge_owner.scale_y = 3.0f;
    edge_owner.offset_x = 5.0f;
    edge_owner.offset_y = 7.0f;
    const std::array<ui::MinimapPoint2, 2> preceding_points{{{-2, 0}, {-1, 1}}};
    const std::array<ui::MinimapPoint2, 2> current_points{{{0, 4}, {1, 9}}};
    const std::array<ui::MinimapPoint2, 2> following_points{{{2, 16}, {3, 25}}};
    const auto smooth_points = ui::smooth_minimap_edge_control_points(
        preceding_points, current_points, following_points, edge_owner);
    check(smooth_points.size() == 4 && smooth_points[0].x == -7.0f &&
              smooth_points[0].y == -2.5f && smooth_points[1].x == -5.0f &&
              smooth_points[1].y == 6.5f && smooth_points[2].x == -3.0f &&
              smooth_points[2].y == 21.5f && smooth_points[3].x == -1.0f &&
              smooth_points[3].y == 42.5f,
          "Native minimap edge smoothing or owner projection changed");
    const auto synthetic_mesh = ui::build_minimap_synthetic_masked_hex_mesh(
        ui::MinimapDrawCell{0, 0, 1u << 4}, hex_transform, 3);
    check(synthetic_mesh.vertices.size() == 14 && synthetic_mesh.indices.size() == 39 &&
              std::abs(synthetic_mesh.vertices[1].x - 37.0f / 48.0f) < 1e-6f &&
              std::abs(synthetic_mesh.vertices[1].y + 11.0f / 48.0f) < 1e-6f &&
              std::abs(synthetic_mesh.vertices[2].x - 19.0f / 32.0f) < 1e-6f &&
              std::abs(synthetic_mesh.vertices[2].y) < 1e-6f &&
              std::abs(synthetic_mesh.vertices[3].x - 13.0f / 24.0f) < 1e-6f &&
              std::abs(synthetic_mesh.vertices[3].y - 11.0f / 48.0f) < 1e-6f,
          "Native minimap synthetic fog-edge mesh is invalid");
    const auto owned_edge = ui::minimap_owned_terrain_edge(
        ui::MinimapEdgeQuery{{0, 0, 0}, 4});
    check(owned_edge == ui::MinimapEdgeQuery{{1, 0, -1}, 7} &&
              ui::minimap_owned_terrain_edge(
                  ui::MinimapEdgeQuery{{0, 0, 0}, 1}) ==
                  ui::MinimapEdgeQuery{{0, 0, 0}, 1},
          "Native terrain edge ownership or reverse slot changed");
    constexpr std::array<ui::MinimapEdgeQuery, 6> native_owned_edges{{
        {{{0, 0, 0}}, 0}, {{{0, 0, 0}}, 1}, {{{0, 0, 0}}, 2},
        {{{0, -1, 1}}, 6}, {{{1, 0, -1}}, 7}, {{{-1, 1, 0}}, 8},
    }};
    for (std::uint8_t direction = 0; direction < 6; ++direction)
        check(ui::minimap_owned_terrain_edge(
                  ui::MinimapEdgeQuery{{0, 0, 0}, direction}) ==
                  native_owned_edges[direction],
              "Native terrain record key differs from direction ownership table");
    const auto coast_grid = terrain::build_world_grid_layout(40, 24, 1, {}).source_grid;
    ui::MinimapCoastEdgeCandidate coast_guard{};
    coast_guard.table_cube = {0, 23, -23};
    coast_guard.direction = 6;
    check(!ui::minimap_coast_edge_registration_allowed(coast_guard, coast_grid, true),
          "Native coast registration accepted the out-of-height neighbor");
    coast_guard.table_cube = {13, 0, -13};
    coast_guard.direction = 7;
    check(ui::minimap_coast_edge_registration_allowed(coast_guard, coast_grid, true),
          "Native coast registration rejected an interior edge");
    coast_guard.table_cube = {0, 0, 0};
    coast_guard.direction = 1;
    check(ui::minimap_coast_edge_registration_allowed(coast_guard, coast_grid, true) &&
              !ui::minimap_coast_edge_registration_allowed(coast_guard, coast_grid, false),
          "Native coast registration did not apply the non-wrapping X guard");
    const auto no_terrain_record = [](const ui::MinimapEdgeQuery &)
        -> std::optional<ui::MinimapTerrainEdgeRecord> { return std::nullopt; };
    auto retained_candidate = coast_guard;
    retained_candidate.worker_cube = {13, 0, -13};
    retained_candidate.table_cube = retained_candidate.worker_cube;
    retained_candidate.direction = 7;
    retained_candidate.world_points = {{{10, 0}, {11, 0}}};
    auto rejected_candidate = retained_candidate;
    rejected_candidate.worker_cube = {12, -1, -11};
    rejected_candidate.table_cube = {0, 23, -23};
    rejected_candidate.direction = 6;
    const std::array coast_records_input{rejected_candidate, retained_candidate};
    const auto coast_records = ui::MinimapCoastEdgeRecordTable::build(
        coast_records_input, coast_grid, true, hex_transform);
    check(coast_records.accepted_pairs() == 1 && coast_records.distinct_slots() == 2 &&
              !coast_records.lookup({{0, 23, -23}, 6}),
          "Rejected coast edge entered the native record table");
    const auto coast_forward = ui::minimap_terrain_edge_polyline(
        ui::MinimapEdgeQuery{{12, 0, -12}, 4}, hex_transform,
        [&](const ui::MinimapEdgeQuery &query) { return coast_records.lookup(query); });
    const auto coast_reverse = coast_records.lookup({{13, 0, -13}, 1});
    const auto coast_wrapped = coast_records.lookup({{53, 0, -53}, 7});
    const auto coast_canonical = coast_records.lookup({{13, 0, -13}, 7});
    check(coast_forward.size() == 2 &&
              std::abs(coast_forward[0].x - 10.0f) < 1e-5f &&
              std::abs(coast_forward[1].x - 11.0f) < 1e-5f &&
              coast_reverse && coast_reverse->relative_world_points.size() == 2 &&
              coast_reverse->relative_world_points[0].x >
                  coast_reverse->relative_world_points[1].x &&
              coast_wrapped && coast_canonical &&
              coast_wrapped->relative_world_points[0].x ==
                  coast_canonical->relative_world_points[0].x,
          "Native coast record ownership, reversal or wrapped lookup changed");
    auto later_candidate = retained_candidate;
    later_candidate.world_points = {{{20, 0}, {21, 0}}};
    const std::array replaced_coast_input{retained_candidate, later_candidate};
    const auto replaced_coast_records = ui::MinimapCoastEdgeRecordTable::build(
        replaced_coast_input, coast_grid, true, hex_transform);
    const auto latest_coast = ui::minimap_terrain_edge_polyline(
        ui::MinimapEdgeQuery{{12, 0, -12}, 4}, hex_transform,
        [&](const ui::MinimapEdgeQuery &query) { return replaced_coast_records.lookup(query); });
    check(replaced_coast_records.accepted_pairs() == 2 &&
              replaced_coast_records.distinct_slots() == 2 &&
              latest_coast.size() == 2 && std::abs(latest_coast[0].x - 20.0f) < 1e-5f,
          "Later coast registration did not replace the recorded slot");
    const auto missing_edge = ui::minimap_terrain_edge_polyline(
        ui::MinimapEdgeQuery{{0, 0, 0}, 4}, hex_transform, no_terrain_record);
    check(missing_edge.size() == 2 &&
              std::abs(missing_edge[0].x - synthetic_edge[0].x) < 1e-6f &&
              std::abs(missing_edge[1].y - synthetic_edge[1].y) < 1e-6f,
          "Missing native terrain edge did not use the synthetic fallback");
    const std::array<ui::MinimapPoint2, 2> recorded_world_points{{{10, 0}, {11, 0}}};
    const auto forward_record = ui::make_minimap_terrain_edge_record(
        {1, 0, -1}, recorded_world_points, hex_transform, false);
    const auto reverse_record = ui::make_minimap_terrain_edge_record(
        {1, 0, -1}, recorded_world_points, hex_transform, true);
    check(forward_record.relative_world_points.size() == 2 &&
              forward_record.relative_world_points[0].x == 9.0f &&
              forward_record.relative_world_points[1].x == 10.0f &&
              reverse_record.relative_world_points[0].x == 10.0f &&
              reverse_record.relative_world_points[1].x == 9.0f,
          "Native minimap terrain-edge registration changed order or anchor");
    // Original 0x424BD0, stopped before its table write, on three worker cubes
    // from the edited Gandhi coast. These bits are independent of the
    // controlled-random point coordinates in the candidate stream.
    ui::MinimapWorldTransform native_record_transform;
    native_record_transform.cube_to_world = {
        std::bit_cast<float>(0x427ffe91u), std::bit_cast<float>(0x3a760002u), 0, 0,
        std::bit_cast<float>(0x42000000u), std::bit_cast<float>(0x428000f7u), 0, 0,
        0, 0, 0, 0,
        std::bit_cast<float>(0x420000b9u), std::bit_cast<float>(0x430000d8u), 0, 1};
    struct NativeRecordCenterCase {
        std::array<std::int32_t, 3> cube;
        std::array<std::uint32_t, 2> original_center_bits;
    };
    constexpr std::array native_record_center_cases{
        NativeRecordCenterCase{{12, -1, -11}, {0x443ffef9u, 0x4280067du}},
        NativeRecordCenterCase{{28, 13, -41}, {0x450bff63u, 0x44700376u}},
        NativeRecordCenterCase{{3, 20, -23}, {0x4457ffc7u, 0x44b00167u}}};
    const std::array<ui::MinimapPoint2, 2> zero_record_points{};
    for (const auto &native_case : native_record_center_cases) {
        const auto record = ui::make_minimap_terrain_edge_record(
            native_case.cube, zero_record_points, native_record_transform, false);
        check(record.relative_world_points.size() == 2 &&
                  std::bit_cast<std::uint32_t>(record.relative_world_points[0].x) ==
                      (native_case.original_center_bits[0] ^ 0x80000000u) &&
                  std::bit_cast<std::uint32_t>(record.relative_world_points[0].y) ==
                      (native_case.original_center_bits[1] ^ 0x80000000u),
              "Compiled coast record center differs from original EXE bits");
    }
    struct NativeRecordPointCase {
        std::array<std::int32_t, 3> cube;
        std::array<std::uint32_t, 4> input_bits;
        std::array<std::uint32_t, 4> relative_bits;
    };
    constexpr std::array native_record_point_cases{
        NativeRecordPointCase{{12, -1, -11},
                              {0x4442b077u, 0x42d0bf90u, 0x44462afau, 0x42c2b1a5u},
                              {0x412c5f80u, 0x42217226u, 0x41c58020u, 0x42055650u}},
        NativeRecordPointCase{{28, 13, -41},
                              {0x450a43d1u, 0x446cd23eu, 0x450a6d18u, 0x44716a0eu},
                              {0xc1ddc900u, 0xc14c4e00u, 0xc1c92580u, 0x40b34c00u}},
        NativeRecordPointCase{{3, 20, -23},
                              {0x445a9b2eu, 0x44b50cfau, 0x445e56e3u, 0x44b4670cu},
                              {0x4126d9c0u, 0x42217260u, 0x41cae380u, 0x420cb4a0u}}};
    for (const auto &native_case : native_record_point_cases) {
        const std::array<ui::MinimapPoint2, 2> input{{
            {std::bit_cast<float>(native_case.input_bits[0]),
             std::bit_cast<float>(native_case.input_bits[1])},
            {std::bit_cast<float>(native_case.input_bits[2]),
             std::bit_cast<float>(native_case.input_bits[3])}}};
        for (const bool reverse : {false, true}) {
            const auto record = ui::make_minimap_terrain_edge_record(
                native_case.cube, input, native_record_transform, reverse);
            check(record.relative_world_points.size() == 2,
                  "Native coast writer point count changed");
            for (std::size_t point = 0; point < 2; ++point) {
                const auto expected = reverse ? 1 - point : point;
                check(std::bit_cast<std::uint32_t>(record.relative_world_points[point].x) ==
                          native_case.relative_bits[2 * expected] &&
                          std::bit_cast<std::uint32_t>(record.relative_world_points[point].y) ==
                          native_case.relative_bits[2 * expected + 1],
                      "Compiled coast record point differs from original EXE bits");
            }
        }
    }
    rejects([&] { ui::make_minimap_terrain_edge_record(
        {1, 0, -1}, std::span<const ui::MinimapPoint2>{}, hex_transform, false); });
    std::vector<ui::MinimapEdgeQuery> terrain_queries;
    const auto recorded_edge = [&](const ui::MinimapEdgeQuery &query)
        -> std::optional<ui::MinimapTerrainEdgeRecord> {
        terrain_queries.push_back(query);
        if (query == owned_edge)
            return forward_record;
        return std::nullopt;
    };
    const auto expanded_record = ui::minimap_terrain_edge_polyline(
        ui::MinimapEdgeQuery{{0, 0, 0}, 4}, hex_transform, recorded_edge);
    check(expanded_record.size() == 2 && expanded_record[0].x == 10.0f &&
              expanded_record[1].x == 11.0f && expanded_record[0].y == 0.0f,
          "Native minimap terrain record did not restore its world-space anchor");
    const std::array<ui::MinimapPoint2, 2> wrapped_world_points{{{0.25f, 0}, {0.5f, 0}}};
    const auto wrapped_record = ui::make_minimap_terrain_edge_record(
        {0, 0, 0}, wrapped_world_points, hex_transform, false);
    const auto wrapped_edge = ui::minimap_terrain_edge_polyline(
        ui::MinimapEdgeQuery{{40, 0, -40}, 1}, hex_transform,
        [&](const ui::MinimapEdgeQuery &query)
            -> std::optional<ui::MinimapTerrainEdgeRecord> {
            check(query == ui::MinimapEdgeQuery{{40, 0, -40}, 1},
                  "Wrapped terrain lookup discarded its unwrapped query");
            return wrapped_record; // provider aliases this query to the canonical table cell
        });
    check(wrapped_edge.size() == 2 && wrapped_edge[0].x == 40.25f &&
              wrapped_edge[1].x == 40.5f,
          "Native minimap wrapped edge used the lookup cell as world anchor");
    rejects([&] { ui::minimap_terrain_edge_polyline(
        ui::MinimapEdgeQuery{{0, 0, 0}, 4}, hex_transform,
        [&](const ui::MinimapEdgeQuery &)
            -> std::optional<ui::MinimapTerrainEdgeRecord> {
            return ui::MinimapTerrainEdgeRecord{{{9, 0}}};
        }); });
    terrain_queries.clear();
    const auto terrain_masked_mesh = ui::build_minimap_terrain_masked_hex_mesh(
        ui::MinimapDrawCell{0, 0, 1u << 4}, hex_transform, 3, recorded_edge);
    check(terrain_queries == std::vector<ui::MinimapEdgeQuery>{
                  ui::minimap_owned_terrain_edge(isolated_queries[0]),
                  ui::minimap_owned_terrain_edge(isolated_queries[1]),
                  ui::minimap_owned_terrain_edge(isolated_queries[2])} &&
              terrain_masked_mesh.vertices.size() == synthetic_mesh.vertices.size() &&
              terrain_masked_mesh.indices == synthetic_mesh.indices &&
              terrain_masked_mesh.vertices[2].x > synthetic_mesh.vertices[2].x + 1.0f,
          "Recorded native terrain edge did not replace the synthetic boundary");
    const auto fallback_mesh = ui::build_minimap_terrain_masked_hex_mesh(
        ui::MinimapDrawCell{0, 0, 1u << 4}, hex_transform, 3, no_terrain_record);
    check(fallback_mesh.indices == synthetic_mesh.indices &&
              fallback_mesh.vertices.size() == synthetic_mesh.vertices.size(),
          "Terrain minimap fallback mesh topology differs from synthetic fog geometry");
    for (std::size_t vertex = 0; vertex < fallback_mesh.vertices.size(); ++vertex)
        check(std::abs(fallback_mesh.vertices[vertex].x -
                       synthetic_mesh.vertices[vertex].x) < 1e-6f &&
                  std::abs(fallback_mesh.vertices[vertex].y -
                           synthetic_mesh.vertices[vertex].y) < 1e-6f,
              "Terrain minimap fallback moved a synthetic vertex");
    const auto masked_mesh = ui::build_minimap_masked_hex_mesh(
        ui::MinimapDrawCell{0, 0, 1u << 4}, hex_transform, boundary_curves, 3);
    const auto masked_hex = ui::minimap_hex_geometry(0, 0, hex_transform);
    for (std::size_t edge_index = 1; edge_index < native_masked_edges.size(); ++edge_index)
        for (std::size_t corner = 0; corner < 2; ++corner) {
            const auto actual = masked_mesh.vertices[4 + 2 * (edge_index - 1) + corner];
            const auto expected = masked_hex.corners[native_masked_edges[edge_index][corner + 1]];
            check(std::abs(actual.x - expected.x) < 1e-6f &&
                      std::abs(actual.y - expected.y) < 1e-6f,
                  "Native minimap unmarked edge chose the wrong hex corners");
        }
    check(masked_mesh.vertices.size() == 14 && masked_mesh.indices.size() == 39 &&
              masked_mesh.vertices[1].x == -1.5f &&
              masked_mesh.vertices[2].x == 0.0f &&
              masked_mesh.vertices[3].x == 1.5f &&
              masked_mesh.indices[0] == 0 && masked_mesh.indices[1] == 1 &&
              masked_mesh.indices[2] == 2 && masked_mesh.indices[3] == 0 &&
              masked_mesh.indices[4] == 2 && masked_mesh.indices[5] == 3 &&
              masked_mesh.indices[6] == 0 && masked_mesh.indices[7] == 3 &&
              masked_mesh.indices[8] == 4 &&
              masked_mesh.indices[36] == 0 && masked_mesh.indices[37] == 13 &&
              masked_mesh.indices[38] == 1,
          "Native minimap masked-edge fan or closing triangles changed");
    rejects([&] { ui::build_minimap_masked_hex_mesh(
        ui::MinimapDrawCell{0, 0, 1u << 4}, hex_transform,
        std::array<std::vector<ui::MinimapPoint2>, 6>{}, 3); });
    rejects([&] { ui::build_minimap_masked_hex_mesh(
        ui::MinimapDrawCell{0, 0, 0}, hex_transform, boundary_curves, 3); });
    hex_transform.cube_to_world[8] = 0.25f;
    hex_transform.cube_to_world[12] = 10.0f;
    hex_transform.cube_to_world[13] = 20.0f;
    hex_transform.scale_x = 2.0f;
    hex_transform.scale_y = 3.0f;
    hex_transform.offset_x = 5.0f;
    hex_transform.offset_y = 7.0f;
    const auto shifted_hex = ui::minimap_hex_geometry(2, 1, hex_transform);
    check(shifted_hex.center.x == 17.5f && shifted_hex.center.y == 56.0f &&
              std::abs(shifted_hex.corners[0].x - (17.5f + 7.0f / 6.0f)) < 1e-5f &&
              std::abs(shifted_hex.corners[0].y - 55.0f) < 1e-5f,
          "Native minimap world matrix, scale, or translation changed");
    terrain::WorldLayout world_layout{};
    world_layout.source_grid.width = 3;
    world_layout.source_grid.height = 3;
    world_layout.worker_grid.width = 4;
    world_layout.worker_grid.height = 4;
    world_layout.source_grid.second_matrix = hex_transform.map_to_cube;
    world_layout.source_grid.second_matrix[0] = 0;
    world_layout.source_grid.second_matrix[1] = 1;
    world_layout.source_grid.second_matrix[4] = 1;
    world_layout.source_grid.second_matrix[5] = 0;
    world_layout.source_grid.second_matrix[8] = 1;
    world_layout.source_grid.second_matrix[12] = 4;
    world_layout.source_grid.second_matrix[13] = -2;
    std::copy(hex_transform.cube_to_world.begin(), hex_transform.cube_to_world.end(),
              world_layout.transforms.begin() + 16);
    terrain::HeightOwnerParameters height_owner{};
    height_owner.width = 256;
    height_owner.height = 256;
    height_owner.scale_x = hex_transform.scale_x;
    height_owner.scale_y = hex_transform.scale_y;
    height_owner.center_x = hex_transform.offset_x;
    height_owner.center_y = hex_transform.offset_y;
    const auto world_bridge = ui::minimap_world_transform(world_layout, height_owner);
    const auto mapped_hex = ui::minimap_hex_geometry(2, 1, world_bridge);
    check(world_bridge.cube_to_world == hex_transform.cube_to_world &&
              world_bridge.map_to_cube == world_layout.source_grid.second_matrix &&
              mapped_hex.cube == std::array<std::int32_t, 3>{2, 0, -2} &&
              mapped_hex.center.x == 18.0f && mapped_hex.center.y == 53.0f,
          "Native minimap world-layout or height-owner bridge changed");
    height_owner.width = 255;
    rejects([&] { ui::minimap_world_transform(world_layout, height_owner); });
    height_owner.width = 256;
    height_owner.world_span_x = 256.0f;
    height_owner.world_span_y = 200.0f;
    height_owner.center_y = 100.0f;
    const auto synthetic_bounds = ui::plan_minimap_visible_bounds(
        world_bridge, height_owner, 0, 0, 2, 2, false);
    check(synthetic_bounds.segment_count == 1 &&
              synthetic_bounds.segments[0].min_x < synthetic_bounds.segments[0].max_x &&
              synthetic_bounds.segments[0].min_y < synthetic_bounds.segments[0].max_y &&
              std::abs((synthetic_bounds.segments[0].max_x - synthetic_bounds.segments[0].min_x) /
                           (synthetic_bounds.segments[0].max_y - synthetic_bounds.segments[0].min_y) -
                       synthetic_bounds.texture.active_aspect) < 1e-5f &&
              synthetic_bounds.edge_subdivisions >= 2,
          "Native minimap visible bounds or active aspect changed");
    // Two stable original DX9 reads of this selected Gandhi save capture the
    // inverse matrix, owner transform and post-load visible bounds. The
    // consumer's one-cell expansion crosses both the X and Y grid edges.
    terrain::WorldLayout captured_layout{};
    const std::array<float, 16> captured_inverse{
        0.01562545821070671f, -2.2910535335540771e-7f,
        -0.015625229105353355f, 0.0f,
        -0.007812499534338713f, 0.015624656341969967f,
        -0.007812156807631254f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, -2.0f, 1.5f, 1.0f};
    std::copy(captured_inverse.begin(), captured_inverse.end(),
              captured_layout.transforms.begin());
    captured_layout.source_grid.first_matrix = {
        0, -1, -1, 0, -1, 0, -1, 0,
        -1, -1, 0, 0, 0, 0, 0, 1};
    terrain::HeightOwnerParameters captured_owner{};
    captured_owner.scale_x = 1.0f;
    captured_owner.scale_y = 0.8659999966621399f;
    captured_owner.center_x = 1280.0f;
    captured_owner.center_y = 775.9359741210938f;
    const auto captured_scan = ui::minimap_scan_window(
        {230.36846923828125f, -108.86536407470703f,
         1593.56298828125f, 718.6328735351562f},
        captured_layout, captured_owner);
    check(captured_scan.left == 22 && captured_scan.top == 9 &&
              captured_scan.right == 45 && captured_scan.bottom == 26,
          "Captured retail minimap fog scan window changed");
    check(ui::detail::minimap_round_cube({0.5f, 0.5f, -1.0f}) ==
              std::array<std::int32_t, 3>{0, 1, -1} &&
              ui::detail::minimap_round_cube({-0.5f, -0.5f, 1.0f}) ==
              std::array<std::int32_t, 3>{0, -1, 1},
          "Native minimap cube rounding changed at half-cell ties");
    const std::array<ui::MinimapViewportCorners, 1> ring_corners{{{{
        {-100.0f, -70.0f, 0.0f}, {-100.0f, 70.0f, 0.0f},
        {100.0f, -70.0f, 0.0f}, {100.0f, 70.0f, 0.0f}}}}};
    const auto ring = ui::build_minimap_viewport_ring(
        ring_corners, {-1280.0f, -814.376f, 1311.96f, 759.018f}, 313);
    check(!ui::minimap_needs_viewport_ring({0.0f, 0.0f, 100.0f, 80.0f},
                                           100.0f, 80.0f) &&
              ui::minimap_needs_viewport_ring({0.0f, 0.0f, 101.0f, 80.0f},
                                              100.0f, 80.0f),
          "Retail kind-8 aspect/visible-world guard changed");
    check(ring.vertices.size() == 12 && ring.indices.size() == 48 &&
              std::equal(ring.indices.begin(), ring.indices.end(),
                         ui::minimap_viewport_ring_indices.begin()),
          "Native kind-8 viewport-ring topology changed");
    const std::array<float, 12> native_ring_x{{
        -100.0f, -100.0f, 100.0f, 100.0f,
        -66.0796127319f, -66.0796127319f, 66.0796127319f, 66.0796127319f,
        -133.920379639f, -133.920379639f, 133.920379639f, 133.920379639f}};
    const std::array<float, 12> native_ring_y{{
        -70.0f, 70.0f, -70.0f, 70.0f,
        -46.2557296753f, 46.2557296753f, -46.2557296753f, 46.2557296753f,
        -93.7442703247f, 93.7442703247f, -93.7442703247f, 93.7442703247f}};
    for (std::size_t n = 0; n < ring.vertices.size(); ++n)
        check(std::abs(ring.vertices[n].position[0] - native_ring_x[n]) < 0.0001f &&
                  std::abs(ring.vertices[n].position[1] - native_ring_y[n]) < 0.0001f &&
                  ring.vertices[n].color == (n < 4 ? 0xFFFFFFFFu : 0u),
              "Native kind-8 viewport-ring position or scalar color changed");
    const std::array<ui::MinimapViewportCorners, 1> oblique_corners{{{{
        {-80.0f, -40.0f, 2.0f}, {-60.0f, 75.0f, 1.0f},
        {110.0f, -55.0f, 3.0f}, {130.0f, 80.0f, 4.0f}}}}};
    const auto oblique_ring = ui::build_minimap_viewport_ring(
        oblique_corners, {-500.0f, -300.0f, 900.0f, 500.0f}, 284);
    check(std::abs(oblique_ring.vertices[4].position[0] - (-59.5139389038f)) < 0.0001f &&
              std::abs(oblique_ring.vertices[4].position[1] - (-28.293680191f)) < 0.0001f &&
              oblique_ring.vertices[4].position[2] == 2.0f &&
              std::abs(oblique_ring.vertices[10].position[1] - (-69.042678833f)) < 0.0001f,
          "Oblique kind-8 viewport-ring expansion differs from retail execution");
    const terrain::ViewFrustum axis_frustum{{
        {0, 0, 1, -400}, {0, 0, -1, 10},
        {-1, 0, 0, 100}, {1, 0, 0, 100},
        {0, 1, 0, 70}, {0, -1, 0, 70}}};
    const std::array<terrain::ViewFrustum, 1> axis_frusta{{axis_frustum}};
    const auto sourced_axis_corners = ui::minimap_viewport_corners(axis_frusta, 0.0f);
    check(sourced_axis_corners.size() == 1 && sourced_axis_corners[0] == ring_corners[0],
          "Retail camera event did not supply the axis-aligned kind-8 source corners");
    constexpr float ux = 0.9563047559630354f, uy = 0.29237170472273677f;
    terrain::ViewFrustum rotated_frustum{{
        {0, 0, 1, -400}, {0, 0, -1, 10},
        {-ux, -uy, 0, 120}, {ux, uy, 0, 80},
        {-uy, ux, 0, 60}, {uy, -ux, 0, 50}}};
    auto shifted_frustum = rotated_frustum;
    for (auto &plane : shifted_frustum)
        plane[3] -= plane[0] * 300.0f;
    const std::array<terrain::ViewFrustum, 2> rotated_frusta{{rotated_frustum, shifted_frustum}};
    const auto sourced_rotated_corners = ui::minimap_viewport_corners(rotated_frusta, 23.5f);
    const std::array<ui::MinimapViewportCorners, 2> native_camera_corners{{{{
        {-58.96208572387695f, -80.76802825927734f, 23.5f},
        {-91.12297058105469f, 24.42550277709961f, 23.5f},
        {132.29888916015625f, -22.293684005737305f, 23.5f},
        {100.13800048828125f, 82.89984893798828f, 23.5f}}}, {{
        {241.03794860839844f, -80.76802825927734f, 23.5f},
        {208.87705993652344f, 24.425506591796875f, 23.5f},
        {432.2989196777344f, -22.29368782043457f, 23.5f},
        {400.1380310058594f, 82.89984893798828f, 23.5f}}}}};
    check(sourced_rotated_corners.size() == native_camera_corners.size(),
          "Retail camera event view count changed");
    for (std::size_t view = 0; view < native_camera_corners.size(); ++view)
        for (std::size_t corner = 0; corner < 4; ++corner)
            for (std::size_t axis = 0; axis < 3; ++axis)
                check(std::abs(sourced_rotated_corners[view][corner][axis] -
                               native_camera_corners[view][corner][axis]) < 0.0002f,
                      "Kind-8 source corner differs from retail camera callback");
    const auto camera_viewports = ui::minimap_camera_viewports(rotated_frusta, 23.5f);
    const std::array<std::array<float, 2>, 2> native_low{{
        {-91.12297058105469f, -80.76802825927734f},
        {208.87705993652344f, -80.76802825927734f}}};
    const std::array<std::array<float, 2>, 2> native_high{{
        {132.29888916015625f, 82.89984893798828f},
        {432.2989196777344f, 82.89984893798828f}}};
    check(camera_viewports.size() == 2, "Retail camera viewport count changed");
    for (std::size_t view = 0; view < camera_viewports.size(); ++view)
        for (std::size_t axis = 0; axis < 2; ++axis)
            check(std::abs(camera_viewports[view].low[axis] - native_low[view][axis]) < 0.0002f &&
                      std::abs(camera_viewports[view].high[axis] - native_high[view][axis]) < 0.0002f,
                  "Kind-8 camera viewport bounds differ from retail callback");
    const auto sourced_rotated_ring = ui::build_minimap_viewport_ring(
        sourced_rotated_corners, {-500.0f, -300.0f, 900.0f, 500.0f}, 284);
    check(sourced_rotated_ring.vertices.size() == 24 &&
              sourced_rotated_ring.indices.size() == 96 &&
              sourced_rotated_ring.vertices[0].position == sourced_rotated_corners[0][0] &&
              sourced_rotated_ring.vertices[12].position == sourced_rotated_corners[1][0],
          "Native camera corners do not feed both kind-8 viewport rings");
    // Original 0x006294F0 on the selected preview's source-1 camera and
    // full-precision minimap bounds. This catches early Float32 rounding of
    // the retail x87 fringe normalization.
    const auto native_float = [](std::uint32_t bits) { return std::bit_cast<float>(bits); };
    const std::array<ui::MinimapViewportCorners, 1> precise_kind8_corners{{{{
        {native_float(0x442d676fu), native_float(0xc2b30088u), native_float(0x424c0000u)},
        {native_float(0x43e2cc30u), native_float(0x43d43d8fu), native_float(0x424bffffu)},
        {native_float(0x44816f02u), native_float(0xc225d3b0u), native_float(0x424c0000u)},
        {native_float(0x448c909eu), native_float(0x4401b17cu), native_float(0x424c0001u)}}}}};
    const auto precise_kind8_ring = ui::build_minimap_viewport_ring(
        precise_kind8_corners,
        {native_float(0xc49ffff6u), native_float(0xc44b9812u),
         native_float(0x44a3fec2u), native_float(0x443dc128u)}, 313);
    const std::array<std::array<std::uint32_t, 2>, 8> native_fringe_xy{{
        {0x4433633du, 0xc25edbb6u}, {0x43f2f528u, 0x43c74cddu},
        {0x447ac988u, 0xc179388eu}, {0x448992b7u, 0x43f27e4du},
        {0x44276ba1u, 0xc2f69335u}, {0x43d2a338u, 0x43e12e41u},
        {0x44857940u, 0xc286ac9eu}, {0x448f8e85u, 0x440a23d2u}}};
    check(precise_kind8_ring.vertices.size() == 12 &&
              precise_kind8_ring.indices.size() == 48,
          "Selected-preview kind-8 retail mesh extent changed");
    for (std::size_t vertex = 0; vertex < native_fringe_xy.size(); ++vertex)
        for (std::size_t axis = 0; axis < 2; ++axis)
            check(std::bit_cast<std::uint32_t>(
                      precise_kind8_ring.vertices[vertex + 4].position[axis]) ==
                      native_fringe_xy[vertex][axis],
                  "Selected-preview kind-8 fringe differs from retail vertex bits");
    rejects([&] { ui::minimap_viewport_corners(rotated_frusta,
                                               std::numeric_limits<float>::quiet_NaN()); });
    rejects([&] { ui::build_minimap_viewport_ring(
        ring_corners, {-1280.0f, -814.376f, 1311.96f, 759.018f}, 0); });
    const auto synthetic_render_meshes = ui::build_minimap_render_meshes(
        draw_tiles, {0, 0, 2, 2}, world_bridge, synthetic_bounds, no_terrain_record);
    check(synthetic_render_meshes.coast.triangle_count() > 6 &&
              synthetic_render_meshes.land.triangle_count() >= 8 * 6 &&
              synthetic_render_meshes.fog.triangle_count() >= 8 * 6 &&
              synthetic_render_meshes.culture.size() == 2 &&
              synthetic_render_meshes.culture[0].payload == 2 &&
              synthetic_render_meshes.culture[1].payload == 5 &&
              synthetic_render_meshes.culture[0].mesh.triangle_count() > 12 &&
              synthetic_render_meshes.culture[1].mesh.triangle_count() == 6,
          "Reusable minimap geometry lost a native draw group or culture order");
    auto invalid_render_bounds = synthetic_bounds;
    invalid_render_bounds.edge_subdivisions = 1;
    rejects([&] { ui::build_minimap_render_meshes(
        draw_tiles, {0, 0, 2, 2}, world_bridge, invalid_render_bounds,
        no_terrain_record); });
    const std::array<ui::MinimapIceRegion, 5> ice_regions{{
        {{5, 5, 8, 8}}, {{-95, 2, -92, 8}}, {{105, 2, 108, 8}},
        {{10, 10, 12, 12}}, {{20, 20, 22, 22}}}};
    const auto ice_draws = ui::plan_minimap_ice_draws(
        {0, 0, 10, 10}, 100, ice_regions);
    check(ice_draws.size() == 4 &&
              ice_draws[0].region_index == 0 && ice_draws[0].vertex_offset_x == 0 &&
              ice_draws[1].region_index == 3 && ice_draws[1].vertex_offset_x == 0 &&
              ice_draws[2].region_index == 1 && ice_draws[2].vertex_offset_x == 100 &&
              ice_draws[2].visible_bounds.min_x == -100 &&
              ice_draws[3].region_index == 2 && ice_draws[3].vertex_offset_x == -100 &&
              ice_draws[3].visible_bounds.min_x == 100,
          "Native kind-9 ice region order, touching edge or wrap offset changed");
    rejects([&] { ui::plan_minimap_ice_draws({0, 0, 10, 10}, 0, ice_regions); });
    rejects([&] { ui::plan_minimap_ice_draws(
        {0, 0, 10, 10}, 100,
        std::array<ui::MinimapIceRegion, 1>{{{{0, 0, NAN, 10}}}}); });
    saves::SavedMapProjection mountain_map;
    mountain_map.grid = {2, 2};
    mountain_map.cells.resize(4);
    for (std::uint32_t y = 0; y < 2; ++y)
        for (std::uint32_t x = 0; x < 2; ++x) {
            auto &cell = mountain_map.cells[y * 2 + x];
            cell.index = y * 2 + x;
            cell.x = x;
            cell.y = y;
            cell.plot_type = 2;
        }
    mountain_map.cells[1].plot_type = 0;
    mountain_map.cells[2].plot_type = 0;
    ui::MinimapWorldTransform mountain_transform;
    mountain_transform.cube_to_world = {
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    terrain::HeightOwnerParameters mountain_owner{};
    mountain_owner.world_span_x = 4;
    mountain_owner.world_span_y = 6;
    ui::MinimapVisibleBounds mountain_visible;
    mountain_visible.segments[0] = {-1, -1, 2, 2};
    mountain_visible.segment_count = 1;
    const auto mountain_markers = ui::build_minimap_mountain_markers(
        mountain_map, mountain_transform, mountain_owner, mountain_visible);
    check(mountain_markers.source.size() == 2 && mountain_markers.radius == 1.5f &&
              mountain_markers.source[0].map_x == 0 &&
              mountain_markers.source[0].map_y == 1 &&
              mountain_markers.source[0].center.x == 0 &&
              mountain_markers.source[0].center.y == 1 &&
              mountain_markers.source[1].map_x == 1 &&
              mountain_markers.source[1].map_y == 0 &&
              mountain_markers.visible_indices[0] == std::vector<std::size_t>({0, 1}),
          "Native mountain scan order, cube projection or radius changed");
    check(ui::minimap_stored_marker_radius(mountain_owner, 2, 3) == 1.0f,
          "Native marker radius must use the larger map dimension");
    const std::array<std::array<std::uint8_t, 4>, 3> city_colors{{
        {{255, 153, 49, 255}}, {{18, 135, 6, 255}}, {{0, 0, 255, 255}}}};
    const auto color_table = ui::MinimapCityMarkerList::ColorTable{city_colors};
    ui::MinimapCityMarkerList city_markers;
    check(ui::minimap_city_cube_from_map(32, 15) ==
              std::array<std::int16_t, 2>{25, 15},
          "Saved city staggered coordinates did not convert to callback cube q/r");
    city_markers.created(25, 15, 0, color_table, mountain_transform);
    check(city_markers.records().size() == 1 &&
              city_markers.records()[0].world_x == 25 &&
              city_markers.records()[0].world_y == 15 &&
              city_markers.records()[0].rgba == city_colors[0],
          "Native first kind-5 callback projection or color changed");
    city_markers.created(25, 15, 1, color_table, mountain_transform);
    check(city_markers.records().size() == 1 &&
              city_markers.records()[0].rgba == city_colors[1],
          "Duplicate city callback must recolor in place");
    city_markers.created(-2, 1, 2, color_table, mountain_transform);
    city_markers.created(4, -3, 0, color_table, mountain_transform);
    city_markers.destroyed(-2, 1);
    check(city_markers.records().size() == 2 &&
              city_markers.records()[1].cube_q == 4 &&
              city_markers.records()[1].cube_r == -3,
          "Native city removal must swap the last marker into the hole");
    city_markers.recolored(4, -3, 1, color_table);
    check(city_markers.records()[1].rgba == city_colors[1],
          "Native city recolor callback changed");
    city_markers.destroyed(100, 100);
    check(city_markers.records().size() == 2,
          "Unknown city removal changed the marker list");
    rejects([&] { ui::minimap_city_cube_from_map(0, 32768); });
    rejects([&] { city_markers.created(1, 2, 3, color_table, mountain_transform); });
    auto captured_city_map = mountain_map;
    saves::SavedMapTeamArtObservation captured_city_art;
    captured_city_art.plots.resize(captured_city_map.cells.size());
    for (std::size_t index = 0; index < captured_city_art.plots.size(); ++index)
        captured_city_art.plots[index].plot_index = index;
    check(ui::reconcile_native_minimap_city_markers(
              captured_city_map, captured_city_art, mountain_transform, {}).empty(),
          "City-free profile synthesized a native marker");
    captured_city_map.cells[2].plot_city = saves::SavedPlotCityReference{0, 8192};
    captured_city_art.plots[2].revealed = true;
    const std::array<ui::MinimapCityMarker, 1> captured_city{{
        {0.0f, 1.0f, 0, 1, city_colors[0]}}};
    check(ui::reconcile_native_minimap_city_markers(
              captured_city_map, captured_city_art, mountain_transform,
              captured_city).size() == 1,
          "Captured city marker failed native/saved-state reconciliation");
    captured_city_art.plots[2].revealed = false;
    rejects([&] { ui::reconcile_native_minimap_city_markers(
        captured_city_map, captured_city_art, mountain_transform, captured_city); });
    captured_city_art.plots[2].revealed = true;
    auto displaced_city = captured_city;
    displaced_city[0].world_x += 1.0f;
    rejects([&] { ui::reconcile_native_minimap_city_markers(
        captured_city_map, captured_city_art, mountain_transform, displaced_city); });
    mountain_visible.segments[0] = {0, 1, 0, 1};
    const auto clipped_mountains = ui::build_minimap_mountain_markers(
        mountain_map, mountain_transform, mountain_owner, mountain_visible);
    check(clipped_mountains.visible_indices[0] == std::vector<std::size_t>({0}),
          "Native mountain marker segment culling changed");
    mountain_map.cells[0].plot_type.reset();
    rejects([&] { ui::build_minimap_mountain_markers(
        mountain_map, mountain_transform, mountain_owner, mountain_visible); });
    mountain_map.cells[0].feature_type = 0;
    mountain_map.cells[3].feature_type_hash = content::saved_info_type_hash("FEATURE_ICE");
    const auto ice_plots = ui::project_saved_minimap_ice_plots(
        mountain_map, mountain_transform);
    check(ice_plots.size() == 2 && ice_plots[0].plot_index == 0 &&
              ice_plots[0].map_x == 0 && ice_plots[0].map_y == 0 &&
              ice_plots[1].plot_index == 3 && ice_plots[1].map_x == 1 &&
              ice_plots[1].map_y == 1 &&
              ice_plots[0].bounds.min_x < ice_plots[0].center.x &&
              ice_plots[0].center.x < ice_plots[0].bounds.max_x,
          "Saved numeric/hash ice identity or hex projection changed");
    mountain_map.cells[0].feature_type_hash = content::saved_info_type_hash("FEATURE_ICE");
    rejects([&] { ui::project_saved_minimap_ice_plots(mountain_map,
                                                       mountain_transform); });
    mountain_map.cells[0].feature_type_hash.reset();
    rejects([&] { ui::plan_minimap_visible_bounds(
        world_bridge, height_owner, 2, 0, 0, 2, false); });
    auto invalid_hex = hex_transform;
    invalid_hex.scale_y = 0;
    rejects([&] { ui::project_minimap_hexes(saved_tiles, invalid_hex); });
    saved_minimap.cells[4].terrain_type.reset();
    rejects([&] { ui::project_saved_minimap_terrain(saved_minimap); });
    saved_minimap.cells[4].terrain_type = 5;
    saved_minimap.wrap_y = true;
    rejects([&] { ui::project_saved_minimap_terrain(saved_minimap); });
    if (saved_run) {
        saves::SavedSessionDecodeContext context;
        context.decode_map_regions = true;
        context.decode_teams = true;
        context.decode_all_players = true;
        context.decode_supporting_classes = true;
        context.all_players = saves::AllPlayersContext{};
        const auto session = saves::read_saved_session(argv[2], context);
        check(session.snapshot.map_projection.has_value(),
              "Selected save did not decode a minimap terrain projection");
        const auto &map = *session.snapshot.map_projection;
        const auto ice_hash = content::saved_info_type_hash("FEATURE_ICE");
        const auto saved_ice_plots = std::count_if(
            map.cells.begin(), map.cells.end(), [&](const auto &cell) {
                return cell.feature_type == 0 || cell.feature_type_hash == ice_hash;
            });
        std::cout << "Saved minimap ice plots=" << saved_ice_plots << '\n';
        const auto tiles = ui::project_saved_minimap_terrain(map);
        const auto saved_world = terrain::build_world_layout(
            static_cast<std::int32_t>(map.grid.width),
            static_cast<std::int32_t>(map.grid.height),
            terrain::game_world_grid_mode(map.wrap_x), {},
            [](float) { return std::bit_cast<float>(0x45800001u); });
        const auto saved_owner = terrain::HeightOwnerParameters::from_world(saved_world);
        const auto saved_transform = ui::minimap_world_transform(saved_world, saved_owner);
        {
            using namespace std::chrono_literals;
            ui::MinimapNotificationPulseContext context;
            context.map_width = static_cast<std::int32_t>(map.grid.width);
            context.map_height = static_cast<std::int32_t>(map.grid.height);
            context.player_id = 0;
            context.turn = 0;
            context.transform = saved_transform;
            context.visible = native_bounds.visible;
            context.lifetime = 1500ms;
            ui::MinimapNotificationPulseOwner pulses(context, [] { return true; });
            const auto result = pulses.emit_minimap_notification(32, 15, 1, 0, 0, 1);
            const auto projected = pulses.active();
            check(result.status == ui::MinimapNotificationStatus::queued_visible &&
                      projected.size() == 1 &&
                      std::abs(projected[0].texture_pixel.x -
                               native_bounds.reference_marker_map_local[0]) < .03f &&
                      std::abs(projected[0].texture_pixel.y -
                               native_bounds.reference_marker_map_local[1]) < .03f,
                  "Notification plot projection differs from same-save native minimap marker");
            std::cout << std::setprecision(9)
                      << "Saved notification plot (32,15) minimap="
                      << projected[0].texture_pixel.x << ','
                      << projected[0].texture_pixel.y << " native_marker="
                      << native_bounds.reference_marker_map_local[0] << ','
                      << native_bounds.reference_marker_map_local[1] << '\n';
        }
        check(session.snapshot.pregame.has_value() &&
                  session.snapshot.map_plots.has_value(),
              "Selected save lacks active-team minimap reveal inputs");
        const auto team_art = saves::observe_saved_map_team_art(
            *session.snapshot.pregame, *session.snapshot.map_plots);
        check(team_art.plots.size() == map.cells.size(),
              "Selected save active-team plot count differs from map cells");
        if (argc == 5 && std::string_view(argv[3]) == "--city-candidate-output") {
            std::ofstream output(argv[4]);
            check(output.good(), "Cannot write selected-save city candidate output");
            output << std::setprecision(9)
                   << "{\"schema\":\"civ5-minimap-saved-city-projection-v2\","
                      "\"eventObserved\":false,\"activeTeam\":"
                   << team_art.active_team << ",\"cities\":[";
            bool first_city = true;
            for (std::size_t index = 0; index < map.cells.size(); ++index) {
                const auto &cell = map.cells[index];
                if (!cell.plot_city || cell.plot_city->owner < 0 ||
                    cell.plot_city->id < 0) continue;
                check(cell.x < map.grid.width && cell.y < map.grid.height,
                      "Saved city plot lies outside the map");
                const auto cube = ui::minimap_city_cube_from_map(
                    static_cast<std::uint32_t>(cell.x),
                    static_cast<std::uint32_t>(cell.y));
                const auto world = ui::minimap_project_cube(
                    saved_transform,
                    {static_cast<float>(cube[0]), static_cast<float>(cube[1]),
                     static_cast<float>(-static_cast<std::int32_t>(cube[0]) - cube[1]),
                     1.0f});
                check(std::isfinite(world.x) && std::isfinite(world.y),
                      "Saved city candidate projection is invalid");
                check(team_art.plots[index].revealed.has_value(),
                      "Saved city plot lacks active-team reveal bit");
                if (!first_city) output << ',';
                first_city = false;
                output << "{\"owner\":" << cell.plot_city->owner
                       << ",\"cityId\":" << cell.plot_city->id
                       << ",\"map\":[" << cell.x << ',' << cell.y << ']'
                       << ",\"cube\":[" << cube[0] << ',' << cube[1] << ']'
                       << ",\"world\":[" << world.x << ',' << world.y << ']'
                       << ",\"revealed\":"
                       << (*team_art.plots[index].revealed ? "true" : "false")
                       << ",\"visibilityCount\":"
                       << team_art.plots[index].visibility_count << '}';
            }
            output << "]}\n";
            check(output.good(), "Selected-save city candidate output failed");
        }
        const auto saved_ice_source = ui::project_saved_minimap_ice_plots(
            map, saved_transform);
        check(saved_ice_source.size() == static_cast<std::size_t>(saved_ice_plots),
              "Selected save ice source differs from numeric/hash plot features");
        const auto saved_bounds = ui::plan_minimap_visible_bounds(
            saved_transform, saved_owner, 0, 0,
            static_cast<std::int32_t>(tiles.width()) - 1,
            static_cast<std::int32_t>(tiles.height()) - 1, map.wrap_x);
        const auto saved_mountains = ui::build_minimap_mountain_markers(
            map, saved_transform, saved_owner, saved_bounds);
        const auto saved_mountain_plots = std::count_if(
            map.cells.begin(), map.cells.end(), [](const auto &cell) {
                return cell.plot_type == 0;
            });
        check(saved_mountains.source.size() ==
                  static_cast<std::size_t>(saved_mountain_plots) &&
                  saved_mountains.radius > 0,
              "Selected save mountain marker source differs from plot types");
        std::cout << "Saved minimap mountains=" << saved_mountains.source.size()
                  << " radius=" << saved_mountains.radius
                  << " visible first=" << saved_mountains.visible_indices[0].size()
                  << " second=" << saved_mountains.visible_indices[1].size() << '\n';
        const auto &visible = saved_bounds.segments[0];
        check(map.grid.width == 40 && map.grid.height == 24 && map.wrap_x &&
                  saved_bounds.texture.active_width == 313 &&
                  saved_bounds.texture.padding_x == 3 &&
                  saved_bounds.segment_count == 2 &&
                  saved_bounds.edge_subdivisions == 2 &&
                  std::abs(visible.min_x - (-1279.99878f)) < 0.01f &&
                  std::abs(visible.min_y - (-814.376099f)) < 0.01f &&
                  std::abs(visible.max_x - 1311.96118f) < 0.01f &&
                  std::abs(visible.max_y - 759.018066f) < 0.01f &&
                  std::abs((visible.max_x - visible.min_x) /
                               (visible.max_y - visible.min_y) -
                           saved_bounds.texture.active_aspect) < 1e-4f,
              "Selected save minimap visible world bounds are invalid");
        check(ui::minimap_needs_viewport_ring(
                  visible, saved_owner.world_span_x, saved_owner.world_span_y),
              "Selected save should enter the retail kind-8 viewport-ring pass");
        std::cout << std::setprecision(9)
                  << "Saved minimap visible bounds=" << visible.min_x << ',' << visible.min_y
                  << ',' << visible.max_x << ',' << visible.max_y
                  << " segments=" << saved_bounds.segment_count
                  << " active_width=" << saved_bounds.texture.active_width
                  << " edge_subdivisions=" << saved_bounds.edge_subdivisions << '\n';
        auto revealed_save_tiles = tiles;
        const auto revealed_count = ui::apply_saved_minimap_reveals(revealed_save_tiles, team_art);
        std::size_t verified_reveals = 0;
        std::size_t revealed_ice = 0, revealed_snow = 0;
        for (std::size_t index = 0; index < team_art.plots.size(); ++index) {
            const auto x = static_cast<std::int32_t>(index % tiles.width());
            const auto y = static_cast<std::int32_t>(index / tiles.width());
            check(team_art.plots[index].revealed.has_value() &&
                      ((revealed_save_tiles.at(x, y).byte2 & 1u) == 0) ==
                          *team_art.plots[index].revealed,
                  "Selected save minimap fog does not match active-team reveal bits");
            verified_reveals += *team_art.plots[index].revealed;
            if (*team_art.plots[index].revealed) {
                revealed_ice += map.cells[index].feature_type == 0 ||
                                map.cells[index].feature_type_hash == ice_hash;
                revealed_snow += map.cells[index].terrain_type == 4;
            }
        }
        std::cout << "Saved minimap revealed ice=" << revealed_ice
                  << " snow=" << revealed_snow << '\n';
        std::size_t projected_revealed_ice = 0;
        for (const auto &ice : saved_ice_source) {
            if (team_art.plots[ice.plot_index].revealed != true) continue;
            ++projected_revealed_ice;
            std::cout << "Saved minimap revealed ice plot=" << ice.map_x
                      << ',' << ice.map_y << " world=" << ice.center.x
                      << ',' << ice.center.y << " bounds=" << ice.bounds.min_x
                      << ',' << ice.bounds.min_y << ',' << ice.bounds.max_x
                      << ',' << ice.bounds.max_y << '\n';
        }
        check(projected_revealed_ice == revealed_ice,
              "Projected saved ice source differs from active-team reveal bits");
        if (argc == 5 && std::string_view(argv[3]) == "--ice-mesh-output") {
            const auto first_revealed = std::find_if(
                saved_ice_source.begin(), saved_ice_source.end(), [&](const auto &ice) {
                    return team_art.plots[ice.plot_index].revealed == true;
                });
            check(first_revealed != saved_ice_source.end(),
                  "Selected save has no revealed ice for the shader mesh control");
            ui::MinimapIceProbeMesh probe;
            probe.box = {first_revealed->bounds.min_x, first_revealed->bounds.min_y,
                         first_revealed->bounds.max_x, first_revealed->bounds.max_y};
            const auto normalized = [&](ui::MinimapPoint2 point) {
                return ui::MinimapPoint2{
                    2 * (point.x - probe.box[0]) / (probe.box[2] - probe.box[0]) - 1,
                    2 * (point.y - probe.box[1]) / (probe.box[3] - probe.box[1]) - 1};
            };
            for (std::size_t corner = 0; corner < first_revealed->corners.size(); ++corner) {
                const auto next = (corner + 1) % first_revealed->corners.size();
                probe.normalized_triangles.push_back(normalized(first_revealed->center));
                probe.normalized_triangles.push_back(normalized(first_revealed->corners[corner]));
                probe.normalized_triangles.push_back(normalized(first_revealed->corners[next]));
            }
            ui::write_minimap_ice_probe_mesh(argv[4], probe);
            const auto reread = ui::read_minimap_ice_probe_mesh(argv[4]);
            check(reread.box == probe.box &&
                      reread.normalized_triangles.size() == 18,
                  "Saved ice shader probe mesh did not round-trip");
        }
        check(revealed_count == verified_reveals && revealed_count > 0 &&
                  revealed_count < map.grid.size(),
              "Selected save minimap reveal coverage is invalid");
        const auto revealed_batches = ui::build_minimap_draw_batches(
            revealed_save_tiles, {0, 0, static_cast<std::int32_t>(tiles.width()) - 1,
                                  static_cast<std::int32_t>(tiles.height()) - 1});
        const auto masked_fog = std::find_if(
            revealed_batches.fog.begin(), revealed_batches.fog.end(),
            [](const auto &cell) { return cell.edge_mask != 0; });
        check(masked_fog != revealed_batches.fog.end(),
              "Selected save has no masked fog boundary after reveal replay");
        const auto saved_fog_boundary = ui::build_minimap_synthetic_masked_hex_mesh(
            *masked_fog, ui::MinimapWorldTransform{
                             {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}},
            3);
        check(saved_fog_boundary.vertices.size() >= 14 &&
                  saved_fog_boundary.indices.size() >= 39 &&
                  std::all_of(saved_fog_boundary.vertices.begin(),
                              saved_fog_boundary.vertices.end(),
                              [](const auto &point) {
                                  return std::isfinite(point.x) && std::isfinite(point.y);
                              }),
              "Selected save native synthetic fog boundary is invalid");
        std::cout << "Saved minimap revealed=" << revealed_count << '\n';
        const auto saved_batches = ui::build_minimap_draw_batches(
            tiles, {0, 0, static_cast<std::int32_t>(tiles.width()) - 1,
                    static_cast<std::int32_t>(tiles.height()) - 1});
        check(saved_batches.fog.size() == map.grid.size() &&
                  saved_batches.coast.size() == 316 &&
                  saved_batches.land.size() == 350 &&
                  saved_batches.culture.empty(),
              "Selected save minimap draw groups differ from initial terrain records");
        auto captured_culture_tiles = tiles;
        ui::apply_gandhi_native_culture(captured_culture_tiles);
        const auto captured_culture_batches = ui::build_minimap_draw_batches(
            captured_culture_tiles, {22, 9, 45, 26});
        check(captured_culture_batches.culture.size() == 1 &&
                  captured_culture_batches.culture[0].payload == 1 &&
                  captured_culture_batches.culture[0].cells.size() == 6 &&
                  ui::gandhi_native_culture_rgb ==
                      std::array<std::uint8_t, 3>{18, 135, 6} &&
                  captured_culture_tiles.at(32, 14).byte3 == 0x38 &&
                  captured_culture_tiles.at(33, 16).byte3 == 0xd4,
              "Captured Gandhi culture group or native color binding changed");
        const auto masked_coast = std::find_if(
            saved_batches.coast.begin(), saved_batches.coast.end(),
            [](const auto &cell) { return cell.edge_mask != 0; });
        check(masked_coast != saved_batches.coast.end(),
              "Selected save has no terrain boundary for the coast pass");
        const auto fallback_coast = ui::build_minimap_terrain_masked_hex_mesh(
            *masked_coast, ui::MinimapWorldTransform{
                               {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}},
            3, no_terrain_record);
        check(fallback_coast.vertices.size() >= 14 &&
                  fallback_coast.indices.size() >= 39,
              "Selected save terrain pass cannot tessellate a missing-record coast edge");
        const auto saved_fog_mesh = ui::build_minimap_plain_hex_mesh(
            saved_batches.fog, ui::MinimapWorldTransform{
                {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}});
        check(saved_fog_mesh.vertices.size() == map.grid.size() * 7 &&
                  saved_fog_mesh.indices.size() == map.grid.size() * 18,
              "Selected save minimap initial fog mesh is incomplete");
        const auto saved_hexes = ui::project_minimap_hexes(tiles, ui::MinimapWorldTransform{
            {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}});
        check(saved_hexes.size() == map.grid.size() &&
                  saved_hexes.front().cube == std::array<std::int32_t, 3>{0, 0, 0} &&
                  saved_hexes.back().map_x == static_cast<std::int32_t>(tiles.width()) - 1 &&
                  saved_hexes.back().map_y == static_cast<std::int32_t>(tiles.height()) - 1,
              "Selected save minimap hex traversal differs from terrain plot order");
        std::array<std::size_t, 3> classes{};
        std::size_t edges = 0, initial_fog = 0;
        for (std::uint32_t y = 0; y < tiles.height(); ++y)
            for (std::uint32_t x = 0; x < tiles.width(); ++x) {
                const auto &cell = tiles.at(static_cast<std::int32_t>(x),
                                             static_cast<std::int32_t>(y));
                check((cell.byte1 & 3u) < classes.size() && cell.byte0 == 0 &&
                          cell.byte2 == 1 && cell.byte3 == 0,
                      "Selected save minimap has invalid initial terrain records");
                ++classes[cell.byte1 & 3u];
                edges += std::popcount(static_cast<unsigned>(cell.byte1 >> 2));
                initial_fog += cell.byte2 & 1u;
            }
        check(classes[0] + classes[1] + classes[2] == map.grid.size() &&
                  initial_fog == map.grid.size(),
              "Selected save minimap plot coverage or initial fog changed");
        std::cout << "Saved minimap " << tiles.width() << 'x' << tiles.height()
                  << " land=" << classes[0] << " coast=" << classes[1]
                  << " ocean=" << classes[2] << " terrain_edges=" << edges << '\n';
        if (argc == 5 && std::string_view(argv[3]) == "--surface-mesh-output") {
            const auto meshes = ui::build_minimap_render_meshes(
                revealed_save_tiles,
                {0, 0, static_cast<std::int32_t>(tiles.width()) - 1,
                 static_cast<std::int32_t>(tiles.height()) - 1},
                saved_transform, saved_bounds, no_terrain_record);
            check(meshes.coast.triangle_count() == 3786 &&
                      meshes.land.triangle_count() == 3522 &&
                      meshes.fog.triangle_count() == 5754 &&
                      meshes.culture.empty(),
                  "Selected save reusable minimap meshes differ from the original export");
            std::ofstream output(argv[4]);
            check(output.good(), "MiniMapPanel surface mesh output could not be opened");
            output << std::setprecision(9)
                   << "{\"schema\":\"civ5-minimap-saved-surface-mesh-v1\",\"grid\":["
                   << tiles.width() << ',' << tiles.height() << "],\"bounds\":["
                   << visible.min_x << ',' << visible.min_y << ','
                   << visible.max_x << ',' << visible.max_y << "],\"activeWidth\":"
                   << saved_bounds.texture.active_width << ",\"paddingX\":"
                   << saved_bounds.texture.padding_x << ",\"revealedCells\":"
                   << revealed_count << ",\"fogCells\":"
                   << revealed_batches.fog.size() << ",\"layers\":{";
            const auto emit_layer = [&](std::string_view name,
                                        const ui::MinimapRenderLayer &layer) {
                output << '"' << name << "\":[";
                for (std::size_t index = 0; index < layer.triangles.size(); index += 3) {
                    if (index) output << ',';
                    output << '[';
                    for (std::size_t corner = 0; corner < 3; ++corner) {
                        if (corner) output << ',';
                        const auto &point = layer.triangles[index + corner];
                        output << '[' << point.x << ',' << point.y << ']';
                    }
                    output << ']';
                }
                output << ']';
            };
            emit_layer("coast", meshes.coast);
            output << ',';
            emit_layer("land", meshes.land);
            output << ',';
            emit_layer("fog", meshes.fog);
            output << "}}\n";
            check(output.good(), "MiniMapPanel surface mesh output is incomplete");
        }
        if (argc == 5 && (std::string_view(argv[3]) == "--grid-output" ||
                          std::string_view(argv[3]) == "--revealed-grid-output")) {
            const auto &output_tiles = std::string_view(argv[3]) == "--revealed-grid-output"
                                           ? revealed_save_tiles : tiles;
            std::ofstream output(argv[4], std::ios::binary);
            check(output.good(), "MiniMapPanel diagnostic grid output could not be opened");
            const std::array<char, 12> header{
                'M', 'M', 'A', 'P',
                static_cast<char>(output_tiles.width() & 0xffu),
                static_cast<char>((output_tiles.width() >> 8) & 0xffu), 0, 0,
                static_cast<char>(output_tiles.height() & 0xffu),
                static_cast<char>((output_tiles.height() >> 8) & 0xffu), 0, 0};
            output.write(header.data(), header.size());
            for (std::uint32_t y = 0; y < output_tiles.height(); ++y)
                for (std::uint32_t x = 0; x < output_tiles.width(); ++x) {
                    const auto &record = output_tiles.at(static_cast<std::int32_t>(x),
                                                         static_cast<std::int32_t>(y));
                    output.write(reinterpret_cast<const char *>(&record), sizeof(record));
                }
            check(output.good(), "MiniMapPanel diagnostic grid output is incomplete");
        }
    }
    rejects([&] { ui::MinimapTileGrid invalid(257, 24, false); });
    const ui::MinimapTileProjection projection{{1, 0, 0}, {0, 1, 0}};
    check(projection.cell({4, 5, -9}) == std::pair{6, 5} &&
              projection.cell({4, -1, -3}) == std::pair{3, -1},
          "Native minimap cube-to-cell projection or negative half-row changed");
    ui::MinimapTileGrid borders(40, 24, false);
    borders.at(6, 4).byte1 = 0;
    check(borders.apply_cell_payload(4, 4, 2, projection) &&
              borders.at(6, 4).byte0 == 3 && borders.at(6, 4).byte3 == 0xfc &&
              borders.at(6, 5).byte3 == 0x20 &&
              borders.at(5, 4).byte3 == 0x40 &&
              borders.at(6, 3).byte3 == 0x80 &&
              borders.at(5, 3).byte3 == 0x04 &&
              borders.at(7, 4).byte3 == 0x08 &&
              borders.at(5, 5).byte3 == 0x10,
          "Native minimap six-neighbor border bit pairing changed");
    borders.at(6, 5).byte1 = 0;
    check(borders.apply_cell_payload(4, 5, 2, projection) &&
              borders.at(6, 4).byte3 == 0xf8 &&
              (borders.at(6, 5).byte3 & 0x20) == 0,
          "Native minimap equal-neighbor border clearing changed");
    check(!borders.apply_cell_payload(3, 4, 9, projection) &&
              borders.at(5, 4).byte0 == 0 && borders.at(5, 4).byte3 == 0x40,
          "Native minimap record-state gate changed");
    borders.at(12, 10).byte1 = 0;
    check(borders.apply_cell_payload(7, 10, 255, projection) &&
              borders.at(12, 10).byte0 == 0,
          "Native minimap payload-byte increment no longer wraps");
    ui::MinimapTileGrid fog_edges(5, 5, false);
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            fog_edges.populate_terrain_cell(x, y, 0);
    fog_edges.clear_cell_fog(1, 2, projection);
    check(fog_edges.at(2, 2).byte2 == 0xfc &&
              fog_edges.at(2, 3).byte2 == 0x21 &&
              fog_edges.at(1, 2).byte2 == 0x41 &&
              fog_edges.at(2, 1).byte2 == 0x81 &&
              fog_edges.at(1, 1).byte2 == 0x05 &&
              fog_edges.at(3, 2).byte2 == 0x09 &&
              fog_edges.at(1, 3).byte2 == 0x11,
          "Native minimap fog clear or six reciprocal boundary bits changed");
    fog_edges.clear_cell_fog(1, 3, projection);
    check((fog_edges.at(2, 2).byte2 & 0x04) == 0 &&
              (fog_edges.at(2, 3).byte2 & 0x20) == 0 &&
              (fog_edges.at(2, 3).byte2 & 1) == 0,
          "Native minimap fog boundary did not clear between open cells");
    rejects([&] { ui::plan_minimap_panel(bnw, 300, 180); });
    rejects([&] { ui::read_minimap_panel(vfs, "Missing.xml"); });
    rejects([&] { ui::MinimapPanelLayout::from_xml(content::read_xml_metadata("<Wrong/>"), bnw_key); });

    const auto open_archive = [&](const char *name) {
        const auto path = asset_root / "Resource/DX9" / name;
        const auto opened = assets::FpkSource::open(path, std::string("Resource/DX9/") + name);
        check(std::holds_alternative<std::shared_ptr<assets::FpkSource>>(opened),
              "Retail UI art archive could not be opened");
        return std::get<std::shared_ptr<assets::FpkSource>>(opened);
    };
    const auto base_art = open_archive("UITextures.fpk");
    const auto misc_art = open_archive("MiscTextures.fpk");
    const auto expansion_art = open_archive("Expansion1UITextures.fpk");
    const auto bnw_art = open_archive("Expansion2UITextures.fpk");
    assets::ResourceMountTable art(assets::resource_lookup(base_art));
    art.mount({3, 0, 0, assets::resource_lookup(misc_art)});
    art.mount({1, 1, 0, assets::resource_lookup(expansion_art)});
    art.mount({2, 1, 0, assets::resource_lookup(bnw_art)});
    std::vector<std::pair<std::uint32_t, std::uint32_t>> background_candidates;
    for (const auto &candidate : art.resolve_all(assets::AssetKey("BottomRight128x224.dds"))) {
        const auto *handle = std::get_if<assets::AssetHandle>(&candidate.asset);
        if (!handle) continue;
        const auto dds = assets::DdsData::parse(handle->read_bytes());
        background_candidates.emplace_back(dds.width(), dds.height());
    }
    check(background_candidates ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{{268, 12}, {72, 60}, {80, 72}}),
          "Retail minimap background override corpus changed");
    const std::vector<std::pair<std::string, std::pair<std::uint32_t, std::uint32_t>>> textures{
        {"Grid9DetailSix140.dds", {256, 256}}, {bnw.background_texture, {80, 72}},
        {bnw.strategic_texture, {45, 45}}, {bnw.options_texture, {45, 45}},
        {bnw.corner_texture, {74, 37}}};
    for (const auto &[texture, expected_size] : textures) {
        const auto selected = art.resolve(assets::AssetKey(assets::resource_basename(texture))).asset;
        const auto *handle = std::get_if<assets::AssetHandle>(&selected);
        if (!handle) throw std::runtime_error("MiniMapPanel art is missing or ambiguous: " + texture);
        check(handle->candidate().provenance.kind == assets::SourceKind::fpk &&
                  handle->candidate().provenance.archive_relative_path ==
                      "Resource/DX9/Expansion2UITextures.fpk",
              "MiniMapPanel art did not come from the selected BNW UI archive");
        const auto dds = assets::DdsData::parse(handle->read_bytes());
        check(std::pair{dds.width(), dds.height()} == expected_size,
              "MiniMapPanel selected BNW DDS dimensions changed");
    }
    const auto indexed_image = [&](const std::shared_ptr<assets::FpkSource> &archive,
                                   const char *atlas_name, const char *index_name) {
        assets::ResourceMountTable only(assets::resource_lookup(archive));
        const auto read = [&](const char *name) {
            const auto selected = only.resolve(assets::AssetKey(name)).asset;
            const auto *handle = std::get_if<assets::AssetHandle>(&selected);
            check(handle != nullptr, "Retail indexed UI art is missing");
            return assets::DdsData::parse(handle->read_bytes());
        };
        return assets::expand_ui_block_atlas(read(atlas_name), read(index_name));
    };
    const auto pixel_hash = [](const assets::Rgba8Level &image) {
        std::uint64_t hash = 14695981039346656037ULL;
        for (const auto byte : image.rgba) hash = (hash ^ byte) * 1099511628211ULL;
        return hash;
    };
    for (const auto &[archive, expected_hash] :
         {std::pair{base_art, 0xa0d4f77f6ba5029ULL},
          std::pair{expansion_art, 0x8e440e88740c04f5ULL},
          std::pair{bnw_art, 0xe1c2ccbb3b742deaULL}}) {
        const auto image = indexed_image(archive, "BottomRight128x224.dds",
                                         "BottomRight128x224-index.dds");
        check(image.width == 128 && image.height == 224 &&
                  image.rgba.size() == 128u * 224u * 4u,
              "Retail indexed minimap background dimensions changed");
        check(pixel_hash(image) == expected_hash,
              "Retail FTXTInterface block expansion changed reconstructed pixels");
    }
    for (const auto &[archive, expected_hash] :
         {std::pair{expansion_art, 0x187bb2cc521d04caULL},
          std::pair{bnw_art, 0x80da8596bdd33675ULL}}) {
        const auto image = indexed_image(archive, "LowerRightCorner_MiniMap.dds",
                                         "LowerRightCorner_MiniMap-index.dds");
        check(image.width == 101 && image.height == 72 &&
                  image.rgba.size() == 101u * 72u * 4u,
              "Retail indexed minimap corner dimensions changed");
        check(pixel_hash(image) == expected_hash,
              "Retail indexed minimap corner pixels changed");
    }
    const std::vector<std::pair<std::string, std::pair<std::uint32_t, std::uint32_t>>> map_layers{
        {"MiniMapGrass.dds", {256, 256}}, {"MiniMapWater.dds", {256, 256}},
        {"MiniMapFog.dds", {256, 256}}, {"MiniMapCulture.dds", {256, 256}},
        {"MiniMapShallowWater.dds", {256, 256}},
        {"MiniMapMountain.dds", {128, 128}}, {"MiniMapIce.dds", {128, 128}}};
    for (const auto &[texture, expected_size] : map_layers) {
        const auto selected = art.resolve(assets::AssetKey(texture)).asset;
        const auto *handle = std::get_if<assets::AssetHandle>(&selected);
        check(handle && handle->candidate().provenance.archive_relative_path ==
                            "Resource/DX9/MiscTextures.fpk",
              "Retail minimap layer did not come from MiscTextures.fpk");
        const auto dds = assets::DdsData::parse(handle->read_bytes());
        check(std::pair{dds.width(), dds.height()} == expected_size,
              "Retail minimap layer dimensions changed");
    }
    std::cout << "PASS: " << checks << " native MiniMapPanel skin/layout checks; "
              << "BNW map=" << plan.map.x << ',' << plan.map.y << ','
              << plan.map.width << ',' << plan.map.height << '\n';
}
