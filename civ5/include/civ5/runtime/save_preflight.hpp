#pragma once
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/save_header.hpp"
#include <optional>
#include <span>

namespace civ5::runtime {
struct SaveCandidate {
    std::filesystem::path path;
    saves::SaveHeader header;
};
struct SaveSelection {
    std::vector<SaveCandidate> candidates;
    std::optional<std::size_t> selected;
    std::vector<std::string> diagnostics;
};
// Highest saved mod count, then latest saved turn, then lexical path.
SaveSelection select_save(const std::filesystem::path &path);
enum class RequirementStatus { located, missing, version_mismatch, ambiguous };
struct ContentRequirement {
    content::PackageKind kind;
    std::string id, name;
    std::uint32_t wire_value = 0;
    RequirementStatus status = RequirementStatus::missing;
    std::optional<std::size_t> package;
    std::vector<std::size_t> candidates;
};
enum class ContentProfileKind { vanilla, dlc, modded };
const char *content_profile_kind_name(ContentProfileKind kind);
// Read-only activation plan derived only from the save header and the
// discovered package catalog. Extra installed DLC/mods are deliberately not
// selected. This is a content profile check, not game continuation.
struct SavedContentProfile {
    ContentProfileKind kind = ContentProfileKind::vanilla;
    std::vector<ContentRequirement> requirements;
    std::vector<std::size_t> enabled_dlc;
    std::vector<std::size_t> enabled_mods;
    bool all_requirements_located = false;
    bool files_complete = true;
    bool native_mods_require_port = false;
    bool can_activate = false;
};
// Retail 403694 corpus: the mod wire value matches the installed manifest
// version for all ten selected mods. Require an exact match, no upgrading.
std::vector<ContentRequirement> locate_saved_content(const saves::SaveHeader &header,
                                                     const content::Catalog &catalog);
// Resolve an explicit same-ID/version source choice from discovered manifests.
// A pin must match a mod requested by this save; unrelated or wrong-version
// packages are rejected. Other variants of that exact identity are removed
// before activation, rather than silently preferring local or cached bytes.
void select_saved_mod_sources(content::Catalog &catalog,const saves::SaveHeader &header,
                              std::span<const std::filesystem::path> manifests);
struct NativeModFile {
    std::string path;
    FileIdentity identity;
    std::uint16_t machine = 0;
};
struct PackageInspection {
    std::size_t package;
    std::uint64_t present_files = 0;
    std::uint64_t profile_omitted_files = 0;
    std::vector<NativeModFile> native_files;
    std::vector<std::string> diagnostics;
};
// An optional compatibility profile can account for a deliberately absent
// manifest file. The identity must match one selected declaration exactly.
// Generic preflight does not decide which profile, if any, is trustworthy.
struct ManifestFileOmission {
    std::string mod_id, path, md5;
    std::uint64_t version = 0;
};
// One authored MD5 replaced by the actual MD5 of a fully verified, pinned
// archive. The caller must obtain this from cache receipt verification.
struct VerifiedManifestHash {
    std::size_t package = 0;
    std::string path, declared_md5, actual_md5;
};
PackageInspection inspect_mod_files(const content::Catalog &catalog, std::size_t package,
                                    std::span<const ManifestFileOmission> omissions = {},
                                    std::span<const VerifiedManifestHash> verified_hashes = {});
SavedContentProfile resolve_saved_content_profile(const saves::SaveHeader &header,
                                                  const content::Catalog &catalog,
                                                  std::span<const ManifestFileOmission> omissions = {},
                                                  std::span<const VerifiedManifestHash> verified_hashes = {});
// Build an explicit profile for a new game or a caller-selected mod set. The
// selected package indices are authoritative; mod dependencies are resolved
// from their manifest in dependency order and are never inferred from display
// names or from unrelated installed packages. An empty selection is the
// vanilla profile, while selected DLC without mods is the DLC profile.
SavedContentProfile resolve_selected_content_profile(std::span<const std::size_t> selected_dlc,
                                                     std::span<const std::size_t> selected_mods,
                                                     const content::Catalog &catalog,
                                                     std::span<const ManifestFileOmission> omissions = {},
                                                     std::span<const VerifiedManifestHash> verified_hashes = {});
} // namespace civ5::runtime
