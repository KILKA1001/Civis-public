#include "civ5/content/custom_mod_options.hpp"

namespace civ5::content {
std::int64_t CustomModOptions::value(std::string_view name) const {
    const auto it = entries.find(name.substr(0, name.find('\0')));
    return it == entries.end() ? 0 : it->second.value;
}
CustomModOptions load_custom_mod_options(RulesDatabase &rules, const CustomModOptionsBudget &budget) {
    CustomModOptions result;
    result.prefetch = rules.prefetch_custom_mod_options(budget.prefetch);
    const auto error = [&](const std::string &name, const std::string &message, int code = 0) {
        ++result.errors;
        if (result.error_samples.size() < budget.error_samples)
            result.error_samples.push_back({name, message, code});
    };
    // Native SelectAt embeds the name in an unescaped expression. Preserve SQL
    // syntax/failure behavior inside the isolated DB; remove its 512-byte buffer.
    const auto evidence = [&](const std::string &name) -> std::optional<bool> {
        bool found = false;
        const auto query = rules.query_rows({"SELECT * FROM CustomModDbUpdates WHERE Name='" +
            name + "' AND Value=1 LIMIT 1", {}}, [&](const auto &) { found = true; }, 1);
        if (!query.all_succeeded()) { error(name, query.error, query.sqlite_code); return {}; }
        return found;
    };
    result.query = rules.query_rows({"SELECT * FROM CustomModOptions", {}}, [&](const RulesRowView &row) {
        const auto name = row.text("Name");
        if (!name) { error("", "Null CustomModOptions.Name"); return; }
        CustomModOption option;
        option.authored_value = option.value = row.integer("Value");
        option.requires_updates = row.integer("DbUpdates") != 0;
        if (option.value && option.requires_updates) {
            const auto direct = evidence(*name);
            option.updates_present = direct.value_or(false);
            if (direct && !*direct) {
                const auto sql = evidence(*name + "_SQL");
                if (sql.value_or(false)) option.updates_present = evidence(*name + "_XML").value_or(false);
            }
            if (!option.updates_present) { option.value = 0; ++result.rejected_updates; }
        }
        result.entries[*name] = option;
    }, budget.rows);
    return result;
}
} // namespace civ5::content
