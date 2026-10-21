#include "civ5/content/mod_activation.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return {};
    return value.substr(first, value.find_last_not_of(" \t\n\r") - first + 1);
}
bool has_declared_file(const PackageManifest &package, const std::string &path) {
    const auto requested = assets::normalize_asset_name(path);
    return std::any_of(package.files.begin(), package.files.end(), [&](const auto &file) {
        return assets::normalize_asset_name(file.path) == requested;
    });
}
bool matches_declared_source(const PackageManifest &package,const std::string &path,
                             const std::string &text) {
    const auto requested=assets::normalize_asset_name(path);
    bool found=false;
    std::optional<std::string> digest;
    for(const auto &file:package.files){
        if(assets::normalize_asset_name(file.path)!=requested)continue;
        found=true;
        if(file.md5.empty())continue;
        if(!digest){
            const auto bytes=std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size());
            digest=runtime::bytes_md5(bytes);
        }
        if(*digest!=runtime::ascii_lower(file.md5))return false;
    }
    return found;
}
std::string read_source(const fs::path &path, std::uint64_t budget) {
    const auto before = runtime::identify_file(path);
    require(before.bytes <= budget, "Database action source exceeds reader budget");
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "Cannot open database action source");
    std::string text(static_cast<std::size_t>(before.bytes), '\0');
    input.read(text.data(), static_cast<std::streamsize>(text.size()));
    require(static_cast<std::uint64_t>(input.gcount()) == before.bytes &&
                input.peek() == std::char_traits<char>::eof(),
            "Truncated database action source");
    require(runtime::identify_file(path).sha256 == before.sha256,
            "Database action source changed during read");
    return text;
}
} // namespace
std::vector<std::size_t> order_mod_actions(std::span<const std::size_t> active,
                                         const Catalog &catalog) {
    using Indices = std::vector<std::size_t>;
    const auto count = active.size();
    std::set<std::pair<std::string, std::uint64_t>> identities;
    for (const auto index : active) {
        const auto &p = catalog.packages.at(index);
        require(p.kind == PackageKind::mod, "Action order requires mod packages");
        require(identities.emplace(runtime::ascii_lower(p.id), p.version).second,
                "Duplicate active mod identity in action order");
    }
    std::vector<Indices> direct(count), expanded(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto &p = catalog.packages.at(active[i]);
        // EXE 0x7bc860: type 2 before type 1, each query in authored order;
        // each row scans the active list and retains every matching version.
        for (const auto *group : {"Dependencies", "References"}) {
            const auto *rows = p.document.child(group);
            if (!rows) continue;
            for (const auto &row : rows->children) {
                if (row.name != "Mod") continue;
                const auto id = runtime::ascii_lower(row.attribute("id"));
                require(!id.empty(), "Empty mod relationship identity");
                const auto lo = row.attribute("minversion"), hi = row.attribute("maxversion");
                const auto minimum = lo.empty() ? std::uint64_t{0} : decimal(lo);
                const auto maximum = hi.empty() ? UINT64_MAX : decimal(hi);
                for (std::size_t j = 0; j < count; ++j) {
                    const auto &other = catalog.packages.at(active[j]);
                    if (runtime::ascii_lower(other.id) == id && other.version >= minimum && other.version <= maximum)
                        direct[i].push_back(j);
                }
            }
        }
    }
    // The retail closure is direct relationships, then each child's closure,
    // followed by adjacent-only list::unique. An explicit stack removes the
    // source call-stack limit and detects cycles, including optional references.
    struct Frame { std::size_t node, next = 0; };
    std::vector<unsigned char> state(count);
    for (std::size_t root = 0; root < count; ++root) {
        if (state[root] == 2) continue;
        std::vector<Frame> stack{{root}};
        state[root] = 1;
        expanded[root] = direct[root];
        while (!stack.empty()) {
            auto &frame = stack.back();
            if (frame.next == direct[frame.node].size()) {
                auto &values = expanded[frame.node];
                values.erase(std::unique(values.begin(), values.end()), values.end());
                state[frame.node] = 2;
                stack.pop_back();
                continue;
            }
            const auto child = direct[frame.node][frame.next];
            require(state[child] != 1, "Cyclic mod action dependency/reference");
            if (state[child] == 0) {
                state[child] = 1;
                expanded[child] = direct[child];
                stack.push_back({child});
                continue;
            }
            auto &values = expanded[frame.node];
            values.insert(values.end(), expanded[child].begin(), expanded[child].end());
            ++frame.next;
        }
    }
    Indices ordered;
    std::vector<bool> emitted(count);
    std::vector<std::int64_t> remaining(count);
    const auto emit = [&](std::size_t i) { ordered.push_back(active[i]); emitted[i] = true; };
    for (std::size_t i = 0; i < count; ++i) {
        require(expanded[i].size() <= INT64_MAX, "Mod relationship count exceeds host counter");
        remaining[i] = static_cast<std::int64_t>(expanded[i].size());
        if (remaining[i] == 0) emit(i);
    }
    // Preserve the native single scan, including negative counters. Replacing
    // it with DFS or a conventional topological queue changes database IDs.
    for (const auto &values : expanded) for (const auto i : values) {
        remaining[i] = runtime::wide::add(remaining[i], -1);
        if (remaining[i] == 0) emit(i);
    }
    for (std::size_t i = 0; i < count; ++i) if (!emitted[i]) emit(i);
    return ordered;
}
const char *database_activation_status_name(DatabaseActivationStatus status) {
    switch (status) {
    case DatabaseActivationStatus::ready:
        return "ready";
    case DatabaseActivationStatus::missing_source:
        return "missing_source";
    case DatabaseActivationStatus::unsupported_extension:
        return "unsupported_extension";
    }
    throw std::runtime_error("Invalid database activation status");
}
DatabaseActivationPlan plan_saved_database_actions(const runtime::SavedContentProfile &profile,
                                                   const Catalog &catalog) {
    DatabaseActivationPlan result;
    result.all_sources_present = profile.all_requirements_located;
    result.ordered_mods = order_mod_actions(profile.enabled_mods, catalog);
    for (const auto package_index : result.ordered_mods) {
        const auto &package = catalog.packages.at(package_index);
        const auto *actions = package.document.child("Actions");
        if (!actions)
            continue;
        const auto *phase = actions->child("OnModActivated");
        if (!phase)
            continue;
        for (const auto &node : phase->children) {
            if (node.name != "UpdateDatabase")
                continue;
            require(node.children.empty() && !trim(node.text).empty(),
                    "Malformed UpdateDatabase action");
            DatabaseActivationAction action{package.id, package.version, trim(node.text), package_index,
                                            DatabaseActivationStatus::missing_source};
            const auto extension = action.requested_path.size() >= 4
                                       ? runtime::ascii_lower(action.requested_path.substr(
                                             action.requested_path.size() - 4))
                                       : std::string{};
            if (extension != ".xml" && extension != ".sql")
                action.status = DatabaseActivationStatus::unsupported_extension;
            else if (has_declared_file(package, action.requested_path)) {
                // package_file applies the same traversal/reparse checks used
                // by discovery and confirms that the selected package owns the
                // action source.
                try {
                    (void)package_file(package.path.parent_path(), action.requested_path);
                    action.status = DatabaseActivationStatus::ready;
                } catch (const std::exception &) {
                    action.status = DatabaseActivationStatus::missing_source;
                }
            }
            if (action.status != DatabaseActivationStatus::ready)
                result.all_sources_present = false;
            result.actions.push_back(std::move(action));
        }
    }
    return result;
}
DatabaseActivationReport apply_saved_database_actions(RulesDatabase &database,
                                                      const DatabaseActivationPlan &plan,
                                                      const Catalog &catalog,
                                                      std::uint64_t source_budget,
                                                      std::span<const std::size_t> use_installed_bytes,
                                                      std::span<const runtime::VerifiedManifestHash> verified_hashes) {
    DatabaseActivationReport report;
    report.all_succeeded = plan.all_sources_present;
    for (const auto &planned : plan.actions) {
        DatabaseActivationResult result{planned, false, {}};
        if (planned.status != DatabaseActivationStatus::ready) {
            result.error = database_activation_status_name(planned.status);
        } else {
            require(planned.package.has_value(), "Ready database action has no package");
            const auto &package = catalog.packages.at(*planned.package);
            const auto source = package_file(package.path.parent_path(), planned.requested_path);
            const auto text = read_source(source, source_budget);
            const auto extension = runtime::ascii_lower(assets::path_utf8(
                fs::path(std::u8string(planned.requested_path.begin(), planned.requested_path.end())).extension()));
            result.manifest_md5_match=matches_declared_source(package,planned.requested_path,text);
            const auto bytes = std::span(reinterpret_cast<const std::uint8_t *>(text.data()), text.size());
            const auto actual_md5 = result.manifest_md5_match ? std::string{} : runtime::bytes_md5(bytes);
            const auto pinned_match = std::any_of(verified_hashes.begin(), verified_hashes.end(),
                [&](const runtime::VerifiedManifestHash &verified) {
                    return verified.package == *planned.package &&
                           assets::normalize_asset_name(verified.path) ==
                               assets::normalize_asset_name(planned.requested_path) &&
                           verified.actual_md5 == actual_md5 &&
                           std::any_of(package.files.begin(), package.files.end(),
                               [&](const ModFile &file) {
                                   return file.path == verified.path &&
                                          runtime::ascii_lower(file.md5) == verified.declared_md5;
                               });
                });
            if(!result.manifest_md5_match && !pinned_match &&
               std::find(use_installed_bytes.begin(),use_installed_bytes.end(),*planned.package)==use_installed_bytes.end())
                result.error="manifest MD5 mismatch";
            else if (extension == ".xml") {
                try {
                    database.apply_mod_xml(read_game_data_xml_text(text));
                    result.applied = true;
                } catch (const std::runtime_error &error) {
                    result.error = error.what();
                }
            } else if (extension == ".sql") {
                const auto sql = database.apply_mod_sql(text);
                // Native SQL action preserves ExecuteMultiple's bool across
                // EndTransaction. A failed RELEASE remains a diagnostic even
                // when source COMMIT/ROLLBACK already ended the savepoint.
                result.applied = sql.success;
                if (!result.applied || !sql.release_error.empty())
                    result.error = sql.error.empty() ? sql.release_error : sql.error;
            } else
                result.error = database_activation_status_name(DatabaseActivationStatus::unsupported_extension);
        }
        report.all_succeeded &= result.applied;
        report.actions.push_back(std::move(result));
    }
    return report;
}
} // namespace civ5::content
