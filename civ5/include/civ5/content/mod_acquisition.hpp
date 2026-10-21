#pragma once
#include "civ5/content/manifest.hpp"
#include "civ5/content/mod_archive.hpp"
#include "civ5/platform/http_download.hpp"
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>

namespace civ5::content {
struct ModTarget {
    std::string id, name;
    std::uint64_t version = 0;
};
struct ModSource {
    ModTarget target;
    std::uint64_t workshop_id = 0; // Distinct from the authored mod ID.
    std::string archive_url, sha256;
    std::string installer_bundle; // Retained separately until activation profile
                                  // is resolved.
    std::string archive_member; // Optional member of a provider ZIP, pinned by sha256.
    std::string package_root; // Optional selected mod directory inside a multi-package archive.
    bool allow_stale_manifest_md5 = false; // Explicitly pinned author archive only.
    bool allow_missing_manifest_assets = false; // Pinned archive; never scripts or native code.
};
struct ModDependency {
    std::string id, name;
    std::uint64_t minimum_version = 0;
    std::uint64_t maximum_version = std::numeric_limits<std::uint64_t>::max();
};
std::vector<ModSource> read_mod_sources(const std::filesystem::path &file);
// Read the manifest dependency closure without selecting a package. Game/DLC
// requirements are retained by the manifest reader but are not Workshop mod
// acquisition targets; unknown dependency kinds fail closed.
std::vector<ModDependency> read_mod_dependencies(const PackageManifest &package);
struct WorkshopFile {
    std::uint64_t id = 0, bytes = 0, content_handle = 0;
    std::string title, url, filename;
};
WorkshopFile parse_workshop_details(std::string text, std::uint64_t expected_id);
struct AcquiredMod {
    PackageManifest manifest;
    std::filesystem::path cache_package;
    std::string archive_sha256;
    std::uint64_t archive_bytes = 0, verified_files = 0;
    bool reused = false;
    std::vector<std::string> stale_manifest_md5_files;
    struct StaleManifestHash {
        std::string path, declared_md5, actual_md5;
    };
    std::vector<StaleManifestHash> stale_manifest_hashes;
    struct MissingManifestAsset {
        std::string path, declared_md5;
    };
    std::vector<MissingManifestAsset> missing_manifest_assets;
};
struct AmbiguousCachedMod : std::runtime_error { using std::runtime_error::runtime_error; };
// Find one independently installed, fully verified cache package by the exact
// save ID/version. A save does not record its archive hash, so two different
// packages with that identity are ambiguous and are never chosen silently.
std::optional<AcquiredMod> find_cached_mod_by_identity(const std::filesystem::path &cache,
                                                        const ModTarget &target);
// Verify the exact discovered cache package, including its pinned archive,
// receipt and every extracted file. Returns nullopt for packages outside cache.
std::optional<AcquiredMod> verify_cached_mod_package(const std::filesystem::path &cache,
                                                     const PackageManifest &package);
enum class ModAcquisitionStatus {
    source_unresolved,
    version_unavailable,
    download_required,
    installer_profile_required,
    cached_verified,
    cache_ambiguous,
    cache_invalid
};
const char *mod_acquisition_status_name(ModAcquisitionStatus status);
struct ModAcquisitionItem {
    ModTarget target;
    ModAcquisitionStatus status = ModAcquisitionStatus::source_unresolved;
    std::optional<std::size_t> source_index;
    std::optional<AcquiredMod> cached;
    std::string reason;
};
struct ModAcquisitionPlan {
    std::vector<ModAcquisitionItem> items;
    bool all_verified = true;
};
struct SavedModAcquisitionReport {
    std::uint64_t acquired = 0;
    std::uint64_t reused = 0;
    std::uint64_t retained_installer_bundles = 0;
    std::vector<std::string> diagnostics;
    bool all_acquired = true;
};
// Read-only, offline check of exact target versions and pinned cache packages.
// Keeps target order; an empty target list needs neither sources nor a cache.
// A source match is not proof of remote availability or gameplay compatibility.
ModAcquisitionPlan plan_mod_acquisition(std::span<const ModTarget> targets,
                                        std::span<const ModSource> sources,
                                        const std::filesystem::path &cache = {});
// Acquire the exact saved mod versions and recursively queue manifest Mod
// dependencies. Game/DLC dependencies are deliberately left to the save
// profile resolver. Every failure is reported in diagnostics and the original
// save is never opened for writing.
SavedModAcquisitionReport acquire_saved_mods(
    std::span<const ModTarget> targets, std::span<const ModSource> sources,
    const std::filesystem::path &cache, const std::filesystem::path &decoder = {},
    const std::filesystem::path &local_installer = {}, platform::DownloadProgress progress = {},
    ArchiveBudget budget = {});
// Own cache only. Published packages include archive, receipt and extracted
// content; staging never participates in content discovery/activation.
void initialize_mod_cache(const std::filesystem::path &root);
AcquiredMod acquire_mod(const ModSource &source, const std::filesystem::path &cache,
                        platform::DownloadProgress progress = {}, ArchiveBudget budget = {});
// Explicit HTTPS source for an uncatalogued mod. With no expected hash, the
// downloaded bytes are still checked against the exact target manifest and
// recorded by SHA-256 in the cache receipt; this does not prove which revision
// the original save author used. Nothing is activated by this operation.
AcquiredMod acquire_mod_from_url(const ModTarget &target, const std::string &url,
                                 const std::filesystem::path &cache,
                                 const std::string &expected_sha256 = {},
                                 const std::string &archive_member = {},
                                 const std::string &package_root = {},
                                 platform::DownloadProgress progress = {}, ArchiveBudget budget = {});
// Resolve a public Workshop item at acquisition time. The item ID is a source
// locator, while the save's mod ID/version and manifest decide whether the
// downloaded archive is usable. No Steam subscription is changed.
AcquiredMod acquire_mod_from_workshop(const ModTarget &target, std::uint64_t workshop_id,
                                      const std::filesystem::path &cache,
                                      const std::string &expected_sha256 = {},
                                      platform::DownloadProgress progress = {}, ArchiveBudget budget = {});
// Offline counterpart for archives supplied by the user or a future provider.
AcquiredMod install_mod_archive(const std::filesystem::path &archive, const ModTarget &target,
                                const std::filesystem::path &cache, const std::string &expected_sha256 = {},
                                ArchiveBudget budget = {}, const std::string &package_root = {},
                                bool allow_stale_manifest_md5 = false,
                                bool allow_missing_manifest_assets = false);
} // namespace civ5::content
