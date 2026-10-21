#pragma once
#include "civ5/content/rules_database.hpp"

namespace civ5::content {
struct CustomModOption {
    std::int64_t authored_value = 0, value = 0;
    bool requires_updates = false, updates_present = false;
};
struct CustomModOptionError { std::string name, message; int sqlite_code = 0; };
struct CustomModOptions {
    GameplayPostprocessReport prefetch;
    RulesQueryReport query;
    std::map<std::string, CustomModOption, std::less<>> entries;
    std::uint64_t errors = 0, rejected_updates = 0;
    std::vector<CustomModOptionError> error_samples;
    bool all_succeeded() const { return prefetch.all_succeeded && query.all_succeeded() && errors == 0; }
    std::int64_t value(std::string_view name) const;
    // MOD_OPT_CACHE compares with one; it does not use nonzero/GetBool.
    bool enabled(std::string_view name) const { return value(name) == 1; }
};
struct CustomModOptionsBudget {
    std::uint64_t rows = 1024 * 1024, error_samples = 16;
    GameplayPostprocessBudget prefetch;
};
// VP preloadCache, before GC's InfoType and Info collections. This mutates only
// the session's private database through CustomModPostDefines. Last duplicate
// Name wins; DbUpdates uses direct evidence OR both _SQL and _XML evidence.
CustomModOptions load_custom_mod_options(RulesDatabase &rules, const CustomModOptionsBudget &budget = {});
} // namespace civ5::content
