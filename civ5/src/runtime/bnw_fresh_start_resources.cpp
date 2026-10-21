#include "civ5/runtime/bnw_fresh_start_resources.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include <algorithm>
#include <exception>
#include <iterator>
#include <limits>
#include <string_view>
#include <unordered_set>

namespace civ5::runtime {
namespace {
constexpr std::string_view stock_starting_plots_sha256 =
    "a957db6a15138a09aef4221217c654ef146dca65aee17aea701e13b3db3b1b32";
bool selected_stock_starting_plots(
    std::span<const BnwMapIncludedSource> includes) {
    bool found = false;
    for (const auto &source : includes) {
        if (source.name != "AssignStartingPlots" &&
            source.name != "AssignStartingPlots.lua") continue;
        if (source.sha256 != stock_starting_plots_sha256) return false;
        found = true;
    }
    return found;
}
// Exact resource Type branches in Expansion2 AssignStartingPlots:__Init.
constexpr std::string_view stock_shortcut_types[] = {
    "RESOURCE_WHEAT", "RESOURCE_COW", "RESOURCE_DEER", "RESOURCE_BANANA",
    "RESOURCE_FISH", "RESOURCE_SHEEP", "RESOURCE_STONE",
    "RESOURCE_IRON", "RESOURCE_HORSE", "RESOURCE_COAL", "RESOURCE_OIL",
    "RESOURCE_ALUMINUM", "RESOURCE_URANIUM",
    "RESOURCE_WHALE", "RESOURCE_PEARLS", "RESOURCE_IVORY", "RESOURCE_FUR",
    "RESOURCE_SILK", "RESOURCE_DYE", "RESOURCE_SPICES", "RESOURCE_SUGAR",
    "RESOURCE_COTTON", "RESOURCE_WINE", "RESOURCE_INCENSE", "RESOURCE_GOLD",
    "RESOURCE_SILVER", "RESOURCE_GEMS", "RESOURCE_MARBLE",
    "RESOURCE_COPPER", "RESOURCE_SALT", "RESOURCE_CITRUS",
    "RESOURCE_TRUFFLES", "RESOURCE_CRAB", "RESOURCE_COCOA", "RESOURCE_BISON",
};
void block(BnwFreshStartResources &result, BnwStartResourcesBlockerCode code,
           std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
} // namespace

bool BnwFreshStartResources::has_blocker(BnwStartResourcesBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshStartResources::rollback() noexcept {
    resources.clear();
    stock_shortcuts.clear();
    resource_scan_complete = false;
}

BnwFreshStartResources draft_bnw_fresh_start_resources(
    const BnwFreshGameContentResult &selected,
    const BnwFreshStartCoastTables &coastal,
    const SimulationRandom &map_random,
    std::span<const BnwMapIncludedSource> selected_includes,
    std::uint64_t maximum_resource_rows) {
    BnwFreshStartResources result;
    result.map_random_before = result.map_random_after = map_random.state();
    if (!coastal.coastal_tables_complete || coastal.coastal.empty() ||
        coastal.coastal.size() != coastal.next_to_coast.size() ||
        !coastal.has_blocker(BnwStartCoastBlockerCode::start_placement_pending)) {
        block(result, BnwStartResourcesBlockerCode::coastal_tables_pending,
              "Source-order coastal and second-ring tables must precede the resource loop");
        return result;
    }
    if (!selected.content_ready() || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        map_random.algorithm() != RandomAlgorithm::bnw ||
        coastal.map_random_after != map_random.state()) {
        block(result, BnwStartResourcesBlockerCode::selected_rules_unverified,
              "Starting resource loop requires the same selected BNW content and map RNG");
        return result;
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != coastal.selected_rules_sha256) {
        block(result, BnwStartResourcesBlockerCode::selected_rules_unverified,
              "Coastal and resource stages must use identical activated rules");
        return result;
    }
    if (!selected_stock_starting_plots(selected_includes)) {
        block(result, BnwStartResourcesBlockerCode::stock_helper_unverified,
              "Selected AssignStartingPlots helper differs from verified Expansion2 stock bytes");
        return result;
    }
    result.stock_helper_sha256 = stock_starting_plots_sha256;
    bool invalid_row = false;
    std::unordered_set<std::int32_t> seen_ids;
    std::unordered_set<std::string> seen_types;
    // GameInfo.Resources() calls SELECT * FROM Resources with no sorting.
    bool query_failed = false;
    try {
        const auto report = selected.rules->query_rows(
            {"SELECT * FROM Resources", {}},
            [&](const content::RulesRowView &row) {
                if (invalid_row) return;
                const auto id = row.integer("ID");
                const auto type = row.text("Type");
                if (!row.has_column("ID") || id < 0 ||
                    id > std::numeric_limits<std::int32_t>::max() ||
                    !type || type->empty() ||
                    !seen_ids.insert(static_cast<std::int32_t>(id)).second ||
                    !seen_types.insert(*type).second) {
                    invalid_row = true;
                    return;
                }
                result.resources.push_back({static_cast<std::int32_t>(id), *type});
                if (std::find(std::begin(stock_shortcut_types),
                              std::end(stock_shortcut_types), *type) !=
                    std::end(stock_shortcut_types))
                    result.stock_shortcuts.push_back(result.resources.back());
            }, maximum_resource_rows);
        query_failed = !report.all_succeeded();
    } catch (const std::exception &) {
        query_failed = true;
    }
    if (invalid_row || query_failed || result.resources.empty()) {
        result.rollback();
        block(result, BnwStartResourcesBlockerCode::resource_rows_unavailable,
              "Selected GameInfo.Resources rows are missing, invalid or exceed the resource budget");
        return result;
    }
    result.resource_scan_complete = true;
    block(result, BnwStartResourcesBlockerCode::luxury_weights_pending,
          "AssignStartingPlots.__InitLuxuryWeights and region placement remain pending");
    return result;
}
} // namespace civ5::runtime
