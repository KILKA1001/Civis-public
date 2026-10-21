#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <span>

namespace civ5::content {
enum class DatabaseActivationStatus { ready, missing_source, unsupported_extension };
const char *database_activation_status_name(DatabaseActivationStatus status);
struct DatabaseActivationAction {
    std::string mod_id;
    std::uint64_t mod_version = 0;
    std::string requested_path;
    std::optional<std::size_t> package;
    DatabaseActivationStatus status = DatabaseActivationStatus::missing_source;
};
struct DatabaseActivationPlan {
    std::vector<DatabaseActivationAction> actions;
    bool all_sources_present = false;
    std::vector<std::size_t> ordered_mods;
};
// Retail action dispatch orders active mods separately from discovery and the
// saved mod list, using selected Dependencies and References. IDs and versions
// retain their full host width; cyclic relationships are diagnosed.
std::vector<std::size_t> order_mod_actions(std::span<const std::size_t> active,
                                         const Catalog &catalog);
// Apply retail mod ordering, preserving authored actions within each mod.
// This planner never executes XML, SQL, Lua, or a native DLL.
DatabaseActivationPlan plan_saved_database_actions(const runtime::SavedContentProfile &profile,
                                                   const Catalog &catalog);
struct DatabaseActivationResult {
    DatabaseActivationAction action;
    bool applied = false;
    std::string error;
    bool manifest_md5_match = true;
};
struct DatabaseActivationReport {
    std::vector<DatabaseActivationResult> actions;
    bool all_succeeded = true;
};
// Apply only ready XML/SQL actions to the caller's isolated RulesDatabase.
// Files are read from the selected package roots. Verified cache exceptions
// name the exact authored and actual MD5 for each allowed file. A separate
// diagnostic option may replay locally installed bytes with stale MD5 values.
// I/O or changed-during-read failures throw; handler errors remain per-action.
DatabaseActivationReport apply_saved_database_actions(RulesDatabase &database,
                                                      const DatabaseActivationPlan &plan,
                                                      const Catalog &catalog,
                                                      std::uint64_t source_budget = 64ULL * 1024 * 1024,
                                                      std::span<const std::size_t> use_installed_bytes = {},
                                                      std::span<const runtime::VerifiedManifestHash> verified_hashes = {});
} // namespace civ5::content
