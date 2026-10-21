#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/content/manifest.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {
// Optional, data-authored repair for an incomplete mod save. No GameCore
// profile or mod-specific key is built into the common runtime.
struct SavedValueFallbackPlan {
    struct LuaPreset {
        std::vector<std::string> scripts;
        std::string call;
    };
    std::string mod_id, user_data_name;
    std::uint64_t mod_version=0;
    std::int64_t user_data_version=0;
    std::optional<LuaPreset> lua_preset;
    std::vector<std::string> keys;
};
struct SavedValueFallbackReport {
    std::uint64_t copied=0,present=0,present_null=0,matching_source=0,missing_source=0;
    std::vector<std::string> missing_keys;
};
SavedValueFallbackPlan read_saved_value_fallback_plan(const std::filesystem::path&);
SavedValueFallbackPlan parse_saved_value_fallback_plan(const content::XmlNode&);
// Reads the named original ModUserData file into a private x64 SQLite copy.
// The source file is never opened for writing.
std::shared_ptr<content::RulesDatabase> load_saved_value_fallback_source(
    const std::filesystem::path &user_data_root,const SavedValueFallbackPlan&);
// Runs only manifest-declared, MD5-verified Lua from the selected mod in a
// private x64 VM/database. An explicit plan chooses the preset; save bytes do
// not reveal which authored preset the player used.
std::shared_ptr<content::RulesDatabase> load_saved_value_fallback_lua_source(
    const content::PackageManifest&,const content::RulesDatabase &activated_rules,
    const SavedValueFallbackPlan&,std::string_view ui_language={});
// Copies only explicitly listed, absent, non-NULL values. All inserts commit
// atomically to the private saved database. Existing NULLs remain NULL; a
// conflicting non-NULL source value rejects the repair before any write.
SavedValueFallbackReport fill_missing_saved_values(content::RulesDatabase &saved,
    const content::RulesDatabase &user_data,const SavedValueFallbackPlan&);
} // namespace civ5::runtime
