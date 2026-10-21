#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/base_gameplay.hpp"
#include "civ5/content/rules_database.hpp"

namespace civ5::content {
struct NativeGameplaySourceResult {
    std::string request, source;
    std::optional<std::uint64_t> mount_id;
    runtime::FileIdentity identity;
    bool selected = false, applied = false;
    std::string error;
};
struct NativeGameplayReport {
    NativeGameplayRecipe recipe;
    runtime::FileIdentity schema_identity;
    std::string schema_source;
    bool schema_applied = false;
    std::vector<NativeGameplaySourceResult> sources;
    std::uint64_t selected = 0, missing = 0, applied = 0, failed = 0;
};
// Load the original base engine schema and its verified 111-request sequence
// into the caller's isolated x64 RulesDatabase. The original PE is inspected
// as data only; no original code, DLL, Lua, or Steam state is executed.
// Missing base names are normal. An XML error is retained per source and does
// not stop later requests, matching the native loader's recovery boundary.
NativeGameplayReport apply_native_base_gameplay(RulesDatabase &database,
                                                const std::filesystem::path &game_root,
                                                const assets::ResourceMountTable &resources);
struct DlcGameplayPackageReport {
    std::size_t package = 0;
    std::string id;
    std::uint64_t requested = 0, resolved = 0, missing = 0, applied = 0, failed = 0;
    std::vector<std::string> missing_requests;
};
struct DlcGameplayReport {
    std::vector<DlcGameplayPackageReport> packages;
};
// Apply GameData declarations from exactly the selected DLC manifests, in
// save order. The resource table is shared with rendering/audio lookup; no
// unselected DLC is inferred from its presence on disk.
DlcGameplayReport apply_saved_dlc_gameplay(RulesDatabase &database,
                                           std::span<const std::size_t> package_indices,
                                           const Catalog &catalog,
                                           const assets::ResourceMountTable &resources,
                                           std::uint64_t source_budget = 64ULL * 1024 * 1024);
} // namespace civ5::content
