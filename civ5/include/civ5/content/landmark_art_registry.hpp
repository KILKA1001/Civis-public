#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/terrain/scene_polygon_plot_store.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::content {
class RulesDatabase;
struct LandmarkArtRow {
    std::string improvement, resource, era, state;
    std::string scale, model, layout_handler, terrain_contour, tech;
};
struct LandmarkTypeRow {
    std::string type, landmark_type, friendly_name;
    bool operator==(const LandmarkTypeRow &) const = default;
};
struct ActiveLandmarkArtRow {
    std::string improvement, resource, era, state, layout_handler;
    double scale=1.0;
    std::optional<std::string> model, tech;
    std::int64_t terrain_contour=0;
    bool operator==(const ActiveLandmarkArtRow &) const = default;
};

// Parse the original LandmarkArtInfos document without changing row order.
// The caller supplies documents in the active mount/import order.
std::vector<LandmarkArtRow> read_landmark_art_rows(std::span<const std::uint8_t> xml);
std::vector<LandmarkTypeRow> read_landmark_type_rows(std::span<const std::uint8_t> xml);

struct LandmarkArtRegistry {
    // IDs are the zero-based result positions of retail 47F5C0's UNION query.
    std::vector<std::string> art_types;
    // Record order is the row order of retail's ArtDefine_Landmarks SELECT.
    std::vector<terrain::ScenePolygonMaterialSource> materials;
    std::uint32_t id(std::string_view name) const;
};

LandmarkArtRegistry build_landmark_art_registry(std::span<const LandmarkArtRow> rows);

struct LandmarkArtDocument {
    std::string request;
    std::optional<std::size_t> package;
    std::optional<std::uint64_t> mount_id;
    std::size_t first_row=0, row_count=0;
};
struct LandmarkArtLoadResult {
    LandmarkArtRegistry registry;
    std::vector<LandmarkArtRow> rows;
    std::vector<LandmarkTypeRow> type_rows;
    std::vector<LandmarkArtDocument> documents;
    std::size_t type_documents=0;
};
struct SavedLandmarkArtLoadResult {
    LandmarkArtLoadResult art;
    Catalog catalog;
    std::size_t selected_dlc=0;
    std::uint64_t indexed_loose_files=0;
    std::string save_sha256, mount_plan_sha256;
};
struct SavedModLandmarkArtFailure {
    std::string source, error;
};
struct SavedModLandmarkArtLoadResult {
    LandmarkArtRegistry registry;
    std::vector<ActiveLandmarkArtRow> active_rows;
    std::size_t selected_mods=0, actions=0;
    std::uint64_t landmark_type_rows=0;
    std::vector<SavedModLandmarkArtFailure> failures;
    bool profile_verified=false;
};
// Base document first, then exactly the selected DLC packages in caller order.
// Category activation is explicit: the loader does not infer ownership from
// installed packages or choose unselected DLC. Every declared file is resolved
// through the shared resource table, and missing/malformed active art fails.
LandmarkArtLoadResult load_landmark_art_registry(
    const assets::AssetHandle &base,const assets::AssetHandle &base_types,
    std::span<const std::size_t> selected_dlc,
    const Catalog &catalog,const assets::ResourceMountTable &resources,
    std::array<bool,3> enabled_categories,
    std::uint64_t maximum_source_bytes=1024*1024);
// Resolve the save's exact installed DLC selection through an explicit loose
// mount snapshot. Mod art and SQL edits are outside this loader.
SavedLandmarkArtLoadResult load_saved_landmark_art_registry(
    const std::filesystem::path &save,const std::filesystem::path &game_root,
    const std::filesystem::path &mount_plan,
    std::array<bool,3> enabled_categories,
    std::uint64_t maximum_source_bytes=1024*1024);
// Seed the game's art table before saved mod UpdateDatabase actions, then
// read the final rowid order after those actions. All authored base/DLC art
// columns are seeded. Active rows retain the full metadata, while model
// loading and drawing remain a separate stage.
void seed_landmark_art_rows(RulesDatabase &rules,std::span<const LandmarkArtRow> rows);
void seed_landmark_type_rows(RulesDatabase &rules,std::span<const LandmarkTypeRow> rows);
std::vector<ActiveLandmarkArtRow> read_active_landmark_art_rows(const RulesDatabase &rules);
LandmarkArtRegistry read_landmark_art_registry(const RulesDatabase &rules);
// Diagnostic replay of selected mod database actions after the exact saved
// DLC art seed. Incomplete rules/native mod support remains visible in the
// result and must not be interpreted as full visual parity.
SavedModLandmarkArtLoadResult load_saved_mod_landmark_art_registry(
    const SavedLandmarkArtLoadResult &dlc_art,
    const std::filesystem::path &save,const std::filesystem::path &game_root,
    const std::filesystem::path &mount_plan,
    std::span<const std::filesystem::path> mod_roots);
} // namespace civ5::content
