#pragma once
#include "civ5/assets/asset_source.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <map>
#include <optional>

namespace civ5::content {
// Inno component choices, not limits on native Civis collections.
enum class VpComponent { core, core_43, full, full_eui, full_43, full_43_eui };
VpComponent vp_component(const std::string &name);
std::string vp_component_name(VpComponent component);
bool vp_has_eui(VpComponent component);
bool vp_has_full_rules(VpComponent component);
bool vp_has_43_layout(VpComponent component);
// Pure projection of one pinned installer's decoded path. Unselected component
// files are omitted. Uninstall resources are never part of a gameplay profile.
std::optional<std::string> vp_542_destination(const std::string &decoded_relative, VpComponent component);
struct ProfileFile {
    std::string logical_path;
    std::filesystem::path source;
    runtime::FileIdentity identity;
    assets::AssetStream open() const;
};
struct ProfileMod {
    PackageManifest manifest; // Original, unchanged .modinfo.
    std::string logical_root;
    std::vector<std::string> omitted_eui_lua;
    bool installer_replaced_gamecore = false;
    bool installer_relocated_gamecore = false;
};
struct ProfileAction {
    std::string requested_path;
    const ProfileFile *resolved = nullptr;
};
struct VpProfile {
    VpComponent component;
    std::filesystem::path bundle_root;
    std::map<std::string, ProfileFile> files; // Validated, normalized logical path.
    std::vector<ProfileMod> installed_mods;   // Availability is not activation.
    std::uint64_t total_bytes = 0;
    const ProfileFile &file(const std::string &logical_path) const;
    const ProfileMod &mod(const std::string &id) const;
    const ProfileFile &mod_file(const ProfileMod &mod, const std::string &relative) const;
    // Explicit category selection preserves manifest ordering. The caller owns
    // phase dispatch; this does not run other actions or a mod's DLL/Lua.
    std::vector<ProfileAction> action_plan(const ProfileMod &mod, const std::string &phase,
                                           const std::string &action) const;
    // Strict convenience API: a missing action source is never silently dropped.
    std::vector<const ProfileFile *> action_files(const ProfileMod &mod, const std::string &phase,
                                                  const std::string &action) const;
};
VpProfile build_vp_542_profile(const std::filesystem::path &bundle_root, VpComponent component);
// Publish one immutable, verified package/VFS view of the chosen installer
// component. All files are copied from this same profile binding; the original
// game and MODS directories are never changed. An existing view is reverified.
std::filesystem::path publish_vp_542_profile_view(const VpProfile &profile,
                                                   const std::filesystem::path &view_root);
// Use installer-supplied game package descriptors without replacing any file
// in the original Steam installation. Returns the number of selected overlays.
std::uint64_t apply_vp_542_catalog_overlays(Catalog &catalog, const VpProfile &profile,
                                            const std::filesystem::path &game_root,
                                            const std::filesystem::path &view_root);
// Atomic diagnostic convenience API, not the native mod action dispatcher.
void apply_profile_database_file(RulesDatabase &database, const ProfileFile &file);
enum class DatabaseActionStatus { applied, missing_source, unsupported_extension, xml_error, sql_error };
std::string database_action_status_name(DatabaseActionStatus status);
struct DatabaseActionResult {
    std::string requested_path;
    DatabaseActionStatus status = DatabaseActionStatus::applied;
    std::string error;
    std::optional<ModSqlResult> sql;
};
struct DatabaseActionReport {
    std::vector<DatabaseActionResult> actions;
    bool all_succeeded = true;
};
// Caller-supplied order, including repetitions. A failed handler does not skip
// later actions. Integrity/read-budget failures throw; they are not native SQL
// errors and must not be hidden as a successful or tolerable game load.
DatabaseActionReport apply_database_actions(RulesDatabase &database, std::span<const ProfileAction> actions,
                                            std::uint64_t source_budget = 64ULL * 1024 * 1024);
} // namespace civ5::content
