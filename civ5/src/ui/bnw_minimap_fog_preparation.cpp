#include "civ5/ui/bnw_minimap_fog_preparation.hpp"
#include "civ5/ui/minimap_saved_map.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <thread>

namespace civ5::ui {
namespace {
void require(bool value, const char *message) {
    if (!value) throw std::invalid_argument(message);
}
void u32(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}
void string(std::vector<std::uint8_t> &bytes, const std::string &value) {
    require(value.size() <= std::numeric_limits<std::uint32_t>::max(), "Minimap identity too long");
    u32(bytes, static_cast<std::uint32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
}
bool sha(const std::string &value) {
    if (value.size() != 64) return false;
    for (char c : value) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
std::string grid_digest(const MinimapTileGrid &grid) {
    std::vector<std::uint8_t> bytes;
    u32(bytes, grid.width()); u32(bytes, grid.height()); u32(bytes, grid.wrap_x());
    const auto append = [&](const MinimapTileRecord &r) {
        bytes.insert(bytes.end(), {r.byte0, r.byte1, r.byte2, r.byte3});
    };
    for (const auto &r : grid.records()) append(r);
    append(grid.sentinel());
    return runtime::identify_bytes(bytes).sha256;
}
void validate_geometry(const BnwMinimapFogGeometry &geometry) {
    const auto &w = geometry.window;
    const auto width = static_cast<std::int64_t>(w.right) - w.left + 1;
    const auto height = static_cast<std::int64_t>(w.bottom) - w.top + 1;
    // Civis preparation budgets, not claims about a native viewport limit.
    require(width > 0 && height > 0 && width <= 1024 && height <= 1024 &&
        width * height <= 65536 && w.left >= -32768 && w.top >= -32768 &&
        w.right <= 32767 && w.bottom <= 32767, "Invalid or excessive minimap fog window");
    require(geometry.edge_subdivisions >= 2 && geometry.edge_subdivisions <= 256 &&
        width * height * (6 * (static_cast<std::int64_t>(geometry.edge_subdivisions) + 1)) * 3 <= 4000000,
        "Invalid or excessive minimap fog tessellation");
    const auto &t = geometry.transform;
    for (float value : t.cube_to_world) require(std::isfinite(value), "Nonfinite minimap transform");
    require(std::isfinite(t.scale_x) && std::isfinite(t.scale_y) &&
        std::isfinite(t.offset_x) && std::isfinite(t.offset_y) && t.scale_x != 0 && t.scale_y != 0,
        "Invalid minimap scale/offset");
}
std::string context_digest(const std::string &grid, const graphics::BnwRendererSceneReceipt &scene,
    const BnwMinimapFogGeometry &geometry) {
    std::vector<std::uint8_t> bytes;
    string(bytes, "civis-minimap-fog-preparation-v1"); string(bytes, grid);
    string(bytes, scene.scene_file_sha256); string(bytes, scene.restored_payload_sha256);
    bytes.insert(bytes.end(), scene.map_guid.begin(), scene.map_guid.end());
    const auto &w = geometry.window;
    for (auto value : {w.left, w.top, w.right, w.bottom}) u32(bytes, static_cast<std::uint32_t>(value));
    const auto &t = geometry.transform;
    for (float value : t.cube_to_world) u32(bytes, std::bit_cast<std::uint32_t>(value));
    for (auto value : t.map_to_cube) u32(bytes, static_cast<std::uint32_t>(value));
    for (float value : {t.scale_x, t.scale_y, t.offset_x, t.offset_y}) u32(bytes, std::bit_cast<std::uint32_t>(value));
    u32(bytes, geometry.edge_subdivisions);
    return runtime::identify_bytes(bytes).sha256;
}
MinimapRenderLayer fog_mesh(const MinimapTileGrid &grid, const BnwMinimapFogGeometry &geometry) {
    MinimapRenderLayer result;
    const auto cells = build_minimap_draw_batches(grid, geometry.window).fog;
    const auto append = [&](const auto &mesh) {
        for (auto index : mesh.indices) {
            const auto point = mesh.vertices.at(index);
            require(std::isfinite(point.x) && std::isfinite(point.y), "Nonfinite prepared minimap geometry");
            result.triangles.push_back(point);
        }
    };
    for (const auto &cell : cells) {
        if (cell.edge_mask == 0)
            append(build_minimap_plain_hex_mesh(std::span<const MinimapDrawCell>(&cell, 1), geometry.transform));
        else append(build_minimap_synthetic_masked_hex_mesh(cell, geometry.transform, geometry.edge_subdivisions));
    }
    return result;
}
std::string payload_digest(const MinimapTileGrid &grid, const MinimapRenderLayer &mesh,
    const WorldViewMapVisibility &visibility) {
    std::vector<std::uint8_t> bytes;
    string(bytes, grid_digest(grid));
    for (auto mode : visibility.plots) u32(bytes, static_cast<std::uint32_t>(mode));
    u32(bytes, static_cast<std::uint32_t>(mesh.triangles.size()));
    for (const auto &point : mesh.triangles) {
        u32(bytes, std::bit_cast<std::uint32_t>(point.x)); u32(bytes, std::bit_cast<std::uint32_t>(point.y));
    }
    return runtime::identify_bytes(bytes).sha256;
}
struct Busy {
    bool &flag;
    explicit Busy(bool &value) : flag(value) { flag = true; }
    ~Busy() { flag = false; }
};
}
struct BnwMinimapFogPreparationOwner::Impl {
    const runtime::LiveSession &session;
    const BnwClientFrameOwner &client;
    const graphics::BnwRendererSceneReceipt scene;
    const MinimapTileGrid initial;
    const BnwMinimapFogGeometry geometry;
    const std::thread::id thread = std::this_thread::get_id();
    const std::uint64_t generation = session.state_generation();
    const runtime::LiveSessionState *state = &session.state();
    std::string source_digest, context;
    std::uint64_t sequence = 0;
    mutable bool busy = false;
    mutable std::shared_ptr<const BnwMinimapFogPacket> published;
    std::shared_ptr<const BnwClientFrame> frame;
    Impl(const runtime::LiveSession &s, const BnwClientFrameOwner &c,
        graphics::BnwRendererSceneReceipt r, MinimapTileGrid grid, BnwMinimapFogGeometry g)
        : session(s), client(c), scene(std::move(r)), initial(std::move(grid)), geometry(std::move(g)) {
        validate_geometry(geometry);
        require(sha(scene.scene_file_sha256) && session.state().world.map_projection,
            "Minimap preparation requires a selected scene/map");
        require(graphics::matches_bnw_renderer_scene_receipt(scene, session.state().world,
            session.state().setup.game_turn, static_cast<std::int64_t>(scene.active_player)),
            "Minimap preparation scene does not match current session");
        source_digest = grid_digest(initial);
        // This exact existing projector establishes pristine fog, terrain edges,
        // unused records and sentinel. Do not invent a generic sentinel reset.
        require(source_digest == grid_digest(project_saved_minimap_terrain(*session.state().world.map_projection)),
            "Minimap initial grid is not the pristine selected terrain projection");
        context = context_digest(source_digest, scene, geometry);
    }
    bool matches_live(const std::shared_ptr<const BnwClientFrame> &candidate) const {
        if (!candidate || session.state_generation() != generation ||
            &session.state() != state) return false;
        const auto lease = session.read_state_lease();
        const auto &live = *lease;
        const auto &id = candidate->stamp.identity;
        const auto &v = candidate->visibility;
        if (!graphics::matches_bnw_renderer_scene_receipt(scene, candidate->world, id.turn, id.player) ||
            !graphics::matches_bnw_renderer_scene_receipt(scene, live.world, live.setup.game_turn, id.player) ||
            id.map_guid != scene.map_guid || id.team < 0 || static_cast<std::uint64_t>(id.team) != scene.active_team ||
            v.player_id != id.player || v.team_id != id.team || v.width != initial.width() ||
            v.height != initial.height() || v.plots.size() != static_cast<std::size_t>(v.width) * v.height ||
            !live.world.map_projection || !live.world.pregame) return false;
        const auto player = live.setup.players.find(id.player);
        const auto active = live.world.pregame->archive.integers.find("active_player");
        if (player == live.setup.players.end() || !player->second.alive || player->second.team != id.team ||
            active == live.world.pregame->archive.integers.end() || active->second != id.player) return false;
        const auto &cache = runtime::bnw_live_plot_visibility_cache(live);
        if (cache.grid.width != v.width || cache.grid.height != v.height ||
            static_cast<std::uint64_t>(id.team) >= cache.team_slots ||
            live.world.map_projection->cells.size() != v.plots.size()) return false;
        for (std::size_t plot = 0; plot < v.plots.size(); ++plot) {
            const auto &cell = live.world.map_projection->cells.at(plot);
            const auto count = cache.counts.at(plot * cache.wire_slots + static_cast<std::uint64_t>(id.team));
            if (count < 0 || !cell.revealed_teams) return false;
            const auto expected = count > 0 ? WorldViewPlotFog::visible :
                cell.revealed_teams->contains(id.team) ? WorldViewPlotFog::explored : WorldViewPlotFog::hidden;
            if (v.plots[plot] != expected) return false;
        }
        return session.state_generation() == generation && &session.state() == state;
    }
    bool validates(const std::shared_ptr<const BnwClientFrame> &candidate) const {
        // Owner fences can change in-place cache values too. Requery semantics
        // after the final callback; generation alone does not detect those writes.
        return candidate && client.is_current(candidate) && matches_live(candidate) &&
            client.is_current(candidate) && matches_live(candidate);
    }
};
BnwMinimapFogPacket::BnwMinimapFogPacket(BnwMinimapFogIdentity identity, MinimapTileGrid grid,
    MinimapRenderLayer mesh, BnwMinimapFogGeometry geometry, std::size_t opened)
    : identity_(std::move(identity)), grid_(std::move(grid)), fog_mesh_(std::move(mesh)),
      geometry_(std::move(geometry)), opened_(opened) {}
BnwMinimapFogPreparationOwner::BnwMinimapFogPreparationOwner(const runtime::LiveSession &session,
    const BnwClientFrameOwner &client, graphics::BnwRendererSceneReceipt scene,
    MinimapTileGrid initial, BnwMinimapFogGeometry geometry)
    : impl_(std::make_unique<Impl>(session, client, std::move(scene), std::move(initial), std::move(geometry))) {}
BnwMinimapFogPreparationOwner::~BnwMinimapFogPreparationOwner() = default;
std::shared_ptr<const BnwMinimapFogPacket> BnwMinimapFogPreparationOwner::prepare(
    const std::shared_ptr<const BnwClientFrame> &frame) {
    // The shared_ptr storage object must outlive this call. Callback reset/
    // replacement is allowed, but must not retarget this operation.
    const auto frame_pin = frame;
    auto &i = *impl_;
    require(std::this_thread::get_id() == i.thread && !i.busy, "Wrong-thread or reentrant minimap preparation");
    Busy busy(i.busy);
    i.published.reset(); i.frame.reset();
    require(frame == frame_pin && i.validates(frame_pin) && frame == frame_pin,
        "Stale or replaced minimap preparation frame/scene/visibility");
    require(i.sequence != std::numeric_limits<std::uint64_t>::max(), "Minimap preparation sequence exhausted");
    auto grid = i.initial;
    const auto opened = apply_live_bnw_minimap_fog(grid, frame_pin->visibility);
    auto mesh = fog_mesh(grid, i.geometry);
    const auto payload = payload_digest(grid, mesh, frame_pin->visibility);
    auto result = std::shared_ptr<const BnwMinimapFogPacket>(new BnwMinimapFogPacket(
        {frame_pin->stamp.identity, i.generation, i.sequence + 1, i.source_digest, i.context, payload},
        std::move(grid), std::move(mesh), i.geometry, opened));
    require(frame == frame_pin && i.validates(frame_pin) && frame == frame_pin,
        "Minimap owner or frame argument changed during preparation");
    ++i.sequence; i.frame = frame_pin; i.published = result;
    return result;
}
bool BnwMinimapFogPreparationOwner::current(
    const std::shared_ptr<const BnwMinimapFogPacket> &packet) const noexcept {
    const auto packet_pin = packet;
    auto &i = *impl_;
    if (std::this_thread::get_id() != i.thread || i.busy || !packet_pin ||
        i.published != packet_pin) return false;
    const auto frame_pin = i.frame;
    Busy busy(i.busy);
    try {
        if (i.validates(frame_pin) && packet == packet_pin &&
            i.published == packet_pin && i.frame == frame_pin) return true;
    } catch (...) {}
    i.published.reset();
    return false;
}
bool BnwMinimapFogPreparationOwner::source_context_matches(
    const graphics::BnwRendererSceneReceipt &scene,
    const BnwMinimapFogGeometry &geometry) const noexcept {
    auto &i = *impl_;
    if (std::this_thread::get_id() != i.thread || i.busy || !i.published || !i.frame)
        return false;
    Busy busy(i.busy);
    try {
        // Declared render-source binding only; this cannot attest how a GPU
        // base was generated or establish original/native pixel equivalence.
        if (!i.validates(i.frame) || scene.scene_file_sha256 != i.scene.scene_file_sha256 ||
            scene.restored_payload_sha256 != i.scene.restored_payload_sha256 ||
            scene.map_guid != i.scene.map_guid || scene.width != i.scene.width ||
            scene.height != i.scene.height || scene.wrap_x != i.scene.wrap_x ||
            scene.wrap_y != i.scene.wrap_y || scene.game_turn != i.scene.game_turn ||
            scene.active_player != i.scene.active_player || scene.active_team != i.scene.active_team ||
            scene.cells != i.scene.cells || scene.active_team_art != i.scene.active_team_art ||
            context_digest(i.source_digest, scene, geometry) != i.context)
            return false;
        const auto lease = i.session.read_state_lease();
        return graphics::matches_bnw_renderer_scene_receipt(scene, lease->world,
            lease->setup.game_turn, i.frame->stamp.identity.player) && i.validates(i.frame);
    } catch (...) { return false; }
}

} // namespace civ5::ui
