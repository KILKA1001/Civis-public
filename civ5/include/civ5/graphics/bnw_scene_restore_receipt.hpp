#pragma once

#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/ui/bnw_live_scene_binding.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace civ5::graphics {

// Fields actually decoded for the camera's world art and minimap. This is a
// semantic comparison of the restored observations, not a hash of the input
// container or a claim that the source payload was applied as a live overlay.
struct BnwSceneCellRevision {
    std::uint64_t index=0,x=0,y=0;
    std::optional<std::int64_t> owner,plot_type,terrain_type,feature_type,
        resource_type,improvement_type;
    bool is_city=false;
    std::optional<saves::SavedRiverPlot> rivers;
    std::optional<saves::SavedRoutePlot> route;
    std::optional<std::vector<saves::SavedPlotUnitReference>> units;
    std::optional<std::set<std::int64_t>> revealed_teams;
    bool operator==(const BnwSceneCellRevision &) const = default;
};

struct BnwRendererSceneReceipt {
    std::string scene_file_sha256;
    std::string restored_payload_sha256;
    std::array<std::uint8_t,16> map_guid{};
    std::uint64_t width=0,height=0;
    bool wrap_x=false,wrap_y=false;
    std::int64_t game_turn=-1;
    std::uint64_t active_player=0,active_team=0;
    std::vector<BnwSceneCellRevision> cells;
    std::vector<saves::SavedPlotTeamArtObservation> active_team_art;
    // The current independent renderer has no live-section apply path. This
    // receipt describes immutable map/fog/art, optionally with a separately
    // validated treasury presentation pair that cannot modify those fields.
    std::vector<std::uint32_t> applied_live_sections;
    // The ID10/ID30 pair changes treasury presentation only. Its bytes are
    // bound here; LiveSession must validate/restore the pair before rendering.
    std::string treasury_presentation_sha256;
    bool treasury_presentation_verified=false;
    bool projection_reconciled=false;
};

inline std::optional<std::string> bnw_treasury_presentation_digest(
    const saves::CivisSaveDocument &document) {
    if(ui::unbound_bnw_scene_section(document,
            ui::BnwSceneOverlayPolicy::treasury_presentation))
        return std::nullopt;
    std::string bundle="BNW-TREASURY-PRESENTATION-V1;";
    for(const std::uint32_t id:{10u,30u}) {
        const saves::CivisSaveSection *found=nullptr;
        for(const auto &section:document.sections)
            if(section.id==id) {
                if(found || section.flags!=0) return std::nullopt;
                found=&section;
            }
        if(!found) return std::nullopt;
        bundle+=std::to_string(id)+":"+std::to_string(found->bytes.size())+":"+
            runtime::identify_bytes(found->bytes).sha256+";";
    }
    return runtime::identify_bytes(std::vector<std::uint8_t>(
        bundle.begin(),bundle.end())).sha256;
}

inline BnwSceneCellRevision bnw_scene_cell_revision(
    const saves::SavedMapCellProjection &cell) {
    return {cell.index,cell.x,cell.y,cell.owner,cell.plot_type,
            cell.terrain_type,cell.feature_type,cell.resource_type,
            cell.improvement_type,cell.is_city,cell.rivers,cell.route,
            cell.units,cell.revealed_teams};
}

inline std::optional<BnwRendererSceneReceipt> capture_bnw_renderer_scene_receipt(
    const std::filesystem::path &scene_path,
    std::string_view scene_file_sha256,
    const saves::SavedSession &restored,
    const saves::SavedMapTeamArtObservation &art,
    ui::BnwSceneOverlayPolicy policy=ui::BnwSceneOverlayPolicy::source_only) {
    if (runtime::identify_file(scene_path).sha256 != scene_file_sha256)
        return std::nullopt;
    std::optional<std::string> treasury_digest;
    if (restored.source.container==saves::SaveContainerKind::civis) {
        const auto document=saves::read_civis_save(scene_path);
        if(ui::unbound_bnw_scene_section(document,policy)) return std::nullopt;
        treasury_digest=bnw_treasury_presentation_digest(document);
        if(ui::unbound_bnw_scene_section(document) && !treasury_digest)
            return std::nullopt;
    }
    const auto &snapshot=restored.snapshot;
    const auto &map=snapshot.map_header;
    if (snapshot.game_state.layout!=saves::GameStateLayout::expansion2_version1 ||
        map.layout!=saves::GameStateLayout::expansion2_version1 ||
        !snapshot.pregame || !snapshot.map_plots ||
        !snapshot.map_projection || !snapshot.map_terrain ||
        map.grid.width==0 || map.grid.height==0 ||
        snapshot.map_plots->plots.size()!=map.grid.size() ||
        snapshot.map_projection->cells.size()!=map.grid.size() ||
        art.plots.size()!=map.grid.size()) return std::nullopt;
    const auto turn=snapshot.pregame->archive.integers.find("game_turn");
    if (turn==snapshot.pregame->archive.integers.end() || turn->second<0)
        return std::nullopt;
    const auto independently_observed=saves::observe_saved_map_team_art(
        *snapshot.pregame,*snapshot.map_plots);
    if (independently_observed.active_player!=art.active_player ||
        independently_observed.active_team!=art.active_team ||
        independently_observed.plots!=art.plots) return std::nullopt;
    BnwRendererSceneReceipt result;
    result.scene_file_sha256=std::string(scene_file_sha256);
    result.restored_payload_sha256=
        runtime::identify_bytes(restored.source.bytes).sha256;
    result.map_guid=map.guid;
    result.width=map.grid.width;
    result.height=map.grid.height;
    result.wrap_x=map.wrap_x;
    result.wrap_y=map.wrap_y;
    result.game_turn=turn->second;
    result.active_player=art.active_player;
    result.active_team=art.active_team;
    result.active_team_art=art.plots;
    if(treasury_digest)
        result.treasury_presentation_sha256=*treasury_digest;
    result.cells.reserve(snapshot.map_projection->cells.size());
    for(const auto &cell:snapshot.map_projection->cells)
        result.cells.push_back(bnw_scene_cell_revision(cell));
    return result;
}

inline std::optional<std::string> bnw_renderer_scene_revision_mismatch(
    const BnwRendererSceneReceipt &receipt,
    const saves::SavedSessionSnapshot &live_world,
    std::int64_t live_game_turn,
    std::int64_t active_player) {
    if (!receipt.applied_live_sections.empty())
        return "renderer has no verified live-section apply path";
    if(!receipt.treasury_presentation_sha256.empty() &&
       !receipt.treasury_presentation_verified)
        return "treasury presentation pair was not restored by the live owner";
    if (active_player<0 || static_cast<std::uint64_t>(active_player)!=
            receipt.active_player) return "active player differs";
    if (live_game_turn!=receipt.game_turn) return "game turn differs";
    if (live_world.game_state.layout!=saves::GameStateLayout::expansion2_version1 ||
        live_world.map_header.layout!=saves::GameStateLayout::expansion2_version1 ||
        live_world.map_header.guid!=receipt.map_guid ||
        live_world.map_header.grid.width!=receipt.width ||
        live_world.map_header.grid.height!=receipt.height ||
        live_world.map_header.wrap_x!=receipt.wrap_x ||
        live_world.map_header.wrap_y!=receipt.wrap_y)
        return "map identity differs";
    if (!live_world.pregame || !live_world.map_plots ||
        !live_world.map_projection ||
        live_world.map_projection->cells.size()!=receipt.cells.size())
        return "decoded map shape differs";
    const auto turn=live_world.pregame->archive.integers.find("game_turn");
    if (turn==live_world.pregame->archive.integers.end() ||
        turn->second!=receipt.game_turn) return "saved pregame turn differs";
    for(std::size_t i=0;i<receipt.cells.size();++i) {
        const auto current=bnw_scene_cell_revision(
            live_world.map_projection->cells[i]);
        const auto &expected=receipt.cells[i];
        if (current==expected) continue;
        const char *field="other";
        if (current.owner!=expected.owner) field="owner";
        else if (current.plot_type!=expected.plot_type) field="plot_type";
        else if (current.terrain_type!=expected.terrain_type) field="terrain_type";
        else if (current.feature_type!=expected.feature_type) field="feature_type";
        else if (current.resource_type!=expected.resource_type) field="resource_type";
        else if (current.improvement_type!=expected.improvement_type)
            field="improvement_type";
        else if (current.is_city!=expected.is_city) field="is_city";
        else if (current.rivers!=expected.rivers) field="rivers";
        else if (current.route!=expected.route) field="route";
        else if (current.units!=expected.units) field="units";
        else if (current.revealed_teams!=expected.revealed_teams)
            field="revealed_teams";
        return "decoded map cell " + std::to_string(i) + " " + field +
            " differs";
    }
    const auto art=saves::observe_saved_map_team_art(
        *live_world.pregame,*live_world.map_plots);
    if (art.active_player!=receipt.active_player ||
        art.active_team!=receipt.active_team ||
        art.plots.size()!=receipt.active_team_art.size())
        return "decoded active-team art owner or shape differs";
    for(std::size_t i=0;i<art.plots.size();++i)
        if (art.plots[i]!=receipt.active_team_art[i])
            return "decoded active-team art plot " + std::to_string(i) +
                " differs";
    return std::nullopt;
}

// CvCityManager::OnCityCreated runs during the live import and makes
// CvPlot::isCity resolve the restored city roster. The renderer's raw saved
// projection predates that hook. All raw fields other than is_city must agree
// before the renderer adopts the complete, already validated live projection.
// The returned receipt is derived from the projection actually passed to the
// camera renderer after this assignment.
inline bool reconcile_bnw_renderer_scene_projection(
    saves::SavedMapProjection &renderer_projection,
    BnwRendererSceneReceipt &receipt,
    const runtime::LiveSessionState &live) {
    const auto &world=live.world;
    if (!world.map_projection || !world.pregame || !world.map_plots ||
        !live.nearby_cities.current ||
        renderer_projection.grid.width!=receipt.width ||
        renderer_projection.grid.height!=receipt.height ||
        renderer_projection.wrap_x!=receipt.wrap_x ||
        renderer_projection.wrap_y!=receipt.wrap_y ||
        renderer_projection.cells.size()!=receipt.cells.size() ||
        world.map_projection->cells.size()!=receipt.cells.size() ||
        live.nearby_cities.grid.width!=receipt.width ||
        live.nearby_cities.grid.height!=receipt.height ||
        live.nearby_cities.wrap_x!=receipt.wrap_x ||
        live.nearby_cities.wrap_y!=receipt.wrap_y ||
        world.map_header.guid!=receipt.map_guid ||
        world.map_header.grid.width!=receipt.width ||
        world.map_header.grid.height!=receipt.height)
        return false;
    std::vector<bool> live_city_mask(receipt.cells.size(),false);
    std::size_t live_city_count=0;
    for(const auto &[owner,player]:live.cities.players) {
        (void)owner;
        live_city_count+=player.cities.size();
    }
    if(live_city_count!=live.nearby_cities.locations.size()) return false;
    for(const auto &[handle,location]:live.nearby_cities.locations) {
        try { (void)runtime::bnw_nearby_cities(live,handle); }
        catch(const std::exception &) { return false; }
        if(location.x<0 || location.y<0 ||
           static_cast<std::uint64_t>(location.x)>=receipt.width ||
           static_cast<std::uint64_t>(location.y)>=receipt.height)
            return false;
        const auto index=static_cast<std::uint64_t>(location.y)*receipt.width+
            static_cast<std::uint64_t>(location.x);
        const auto &cell=world.map_projection->cells[index];
        if(live_city_mask[index] || !cell.plot_city ||
           cell.plot_city->owner!=handle.owner ||
           cell.plot_city->id!=handle.id) return false;
        live_city_mask[index]=true;
    }
    for(std::size_t i=0;i<receipt.cells.size();++i) {
        const auto raw=bnw_scene_cell_revision(renderer_projection.cells[i]);
        if(raw!=receipt.cells[i]) return false;
        auto expected=receipt.cells[i];
        expected.is_city=live_city_mask[i];
        if(world.map_projection->cells[i].is_city!=live_city_mask[i] ||
           bnw_scene_cell_revision(world.map_projection->cells[i])!=expected)
            return false;
    }
    const auto live_art=saves::observe_saved_map_team_art(
        *world.pregame,*world.map_plots);
    if(live_art.active_player!=receipt.active_player ||
       live_art.active_team!=receipt.active_team ||
       live_art.plots!=receipt.active_team_art)
        return false;
    auto replacement=*world.map_projection;
    renderer_projection=std::move(replacement);
    receipt.cells.clear();
    receipt.cells.reserve(renderer_projection.cells.size());
    for(const auto &cell:renderer_projection.cells)
        receipt.cells.push_back(bnw_scene_cell_revision(cell));
    receipt.projection_reconciled=true;
    return !bnw_renderer_scene_revision_mismatch(
        receipt,world,live.setup.game_turn,
        static_cast<std::int64_t>(receipt.active_player));
}

inline bool matches_bnw_renderer_scene_receipt(
    const BnwRendererSceneReceipt &receipt,
    const saves::SavedSessionSnapshot &live_world,
    std::int64_t live_game_turn,
    std::int64_t active_player) {
    return !bnw_renderer_scene_revision_mismatch(
        receipt,live_world,live_game_turn,active_player);
}

} // namespace civ5::graphics
