#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include <span>

namespace civ5::content {
struct LooseResourcePlan {
    assets::ResourceMountTable resources;
    std::filesystem::path root;
    std::uint64_t indexed_files;
};
// An explicit diagnostic mount snapshot, not native package discovery or an
// entitlement/activation decision. Every source uses the shared AssetStream.
LooseResourcePlan read_loose_resource_plan(const std::filesystem::path &path);
// Build the same loose Common/Gameplay VFS from the save-selected DLCs. Each
// package owns its scan root, so verified profile overlays can live outside
// the Steam installation without making unrelated DLC or mods visible.
LooseResourcePlan build_selected_loose_resource_plan(
    const std::filesystem::path &game_root, std::span<const std::size_t> enabled_dlc,
    const Catalog &catalog);
} // namespace civ5::content
