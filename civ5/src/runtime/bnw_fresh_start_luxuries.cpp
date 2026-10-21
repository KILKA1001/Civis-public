#include "civ5/runtime/bnw_fresh_start_luxuries.hpp"
#include <algorithm>
#include <span>
#include <string_view>

namespace civ5::runtime {
namespace {
using namespace std::literals;
constexpr std::string_view stock_starting_plots_sha256 =
    "a957db6a15138a09aef4221217c654ef146dca65aee17aea701e13b3db3b1b32";
struct WeightKey { std::string_view type; std::uint8_t weight; };
// Expansion2 AssignStartingPlots.lua:601-754. The helper's exact selected
// bytes were checked by draft_bnw_fresh_start_resources.
constexpr WeightKey tundra[] = {
    {"RESOURCE_FUR"sv,40},{"RESOURCE_WHALE"sv,35},{"RESOURCE_CRAB"sv,30},
    {"RESOURCE_SILVER"sv,25},{"RESOURCE_COPPER"sv,15},{"RESOURCE_SALT"sv,15},
    {"RESOURCE_GEMS"sv,5},{"RESOURCE_DYE"sv,5}};
constexpr WeightKey jungle[] = {
    {"RESOURCE_COCOA"sv,35},{"RESOURCE_CITRUS"sv,35},{"RESOURCE_SPICES"sv,30},
    {"RESOURCE_GEMS"sv,20},{"RESOURCE_SUGAR"sv,20},{"RESOURCE_PEARLS"sv,20},
    {"RESOURCE_COPPER"sv,5},{"RESOURCE_TRUFFLES"sv,5},{"RESOURCE_CRAB"sv,5},
    {"RESOURCE_SILK"sv,5},{"RESOURCE_DYE"sv,5}};
constexpr WeightKey forest[] = {
    {"RESOURCE_DYE"sv,30},{"RESOURCE_SILK"sv,30},{"RESOURCE_TRUFFLES"sv,30},
    {"RESOURCE_FUR"sv,10},{"RESOURCE_SPICES"sv,10},{"RESOURCE_CITRUS"sv,5},
    {"RESOURCE_SALT"sv,5},{"RESOURCE_COPPER"sv,5},{"RESOURCE_COCOA"sv,5},
    {"RESOURCE_CRAB"sv,10},{"RESOURCE_WHALE"sv,10},{"RESOURCE_PEARLS"sv,10}};
constexpr WeightKey desert[] = {
    {"RESOURCE_INCENSE"sv,35},{"RESOURCE_SALT"sv,15},{"RESOURCE_GOLD"sv,25},
    {"RESOURCE_COPPER"sv,10},{"RESOURCE_COTTON"sv,15},{"RESOURCE_SUGAR"sv,15},
    {"RESOURCE_PEARLS"sv,5},{"RESOURCE_CITRUS"sv,5}};
constexpr WeightKey hills[] = {
    {"RESOURCE_GOLD"sv,30},{"RESOURCE_SILVER"sv,30},{"RESOURCE_COPPER"sv,30},
    {"RESOURCE_GEMS"sv,15},{"RESOURCE_PEARLS"sv,15},{"RESOURCE_SALT"sv,10},
    {"RESOURCE_CRAB"sv,10},{"RESOURCE_WHALE"sv,10}};
constexpr WeightKey plains[] = {
    {"RESOURCE_IVORY"sv,35},{"RESOURCE_WINE"sv,35},{"RESOURCE_SALT"sv,25},
    {"RESOURCE_INCENSE"sv,10},{"RESOURCE_SPICES"sv,5},{"RESOURCE_WHALE"sv,5},
    {"RESOURCE_PEARLS"sv,5},{"RESOURCE_CRAB"sv,5},{"RESOURCE_TRUFFLES"sv,5},
    {"RESOURCE_GOLD"sv,5}};
constexpr WeightKey grass[] = {
    {"RESOURCE_COTTON"sv,30},{"RESOURCE_SILVER"sv,20},{"RESOURCE_SUGAR"sv,20},
    {"RESOURCE_COPPER"sv,20},{"RESOURCE_CRAB"sv,20},{"RESOURCE_PEARLS"sv,10},
    {"RESOURCE_WHALE"sv,10},{"RESOURCE_COCOA"sv,10},{"RESOURCE_TRUFFLES"sv,5},
    {"RESOURCE_SPICES"sv,5},{"RESOURCE_GEMS"sv,5}};
constexpr WeightKey hybrid[] = {
    {"RESOURCE_IVORY"sv,15},{"RESOURCE_COTTON"sv,15},{"RESOURCE_WINE"sv,15},
    {"RESOURCE_SILVER"sv,10},{"RESOURCE_SALT"sv,15},{"RESOURCE_COPPER"sv,20},
    {"RESOURCE_WHALE"sv,20},{"RESOURCE_PEARLS"sv,20},{"RESOURCE_CRAB"sv,20},
    {"RESOURCE_TRUFFLES"sv,10},{"RESOURCE_COCOA"sv,10},{"RESOURCE_SPICES"sv,5},
    {"RESOURCE_SUGAR"sv,5},{"RESOURCE_CITRUS"sv,5},{"RESOURCE_INCENSE"sv,5},
    {"RESOURCE_SILK"sv,5},{"RESOURCE_GEMS"sv,5},{"RESOURCE_GOLD"sv,5}};
constexpr WeightKey fallback[] = {
    {"RESOURCE_WHALE"sv,10},{"RESOURCE_PEARLS"sv,10},{"RESOURCE_GOLD"sv,10},
    {"RESOURCE_SILVER"sv,5},{"RESOURCE_GEMS"sv,10},{"RESOURCE_IVORY"sv,5},
    {"RESOURCE_FUR"sv,10},{"RESOURCE_DYE"sv,5},{"RESOURCE_SPICES"sv,5},
    {"RESOURCE_SILK"sv,5},{"RESOURCE_SUGAR"sv,5},{"RESOURCE_COTTON"sv,5},
    {"RESOURCE_WINE"sv,5},{"RESOURCE_INCENSE"sv,5},{"RESOURCE_COPPER"sv,5},
    {"RESOURCE_SALT"sv,5},{"RESOURCE_CITRUS"sv,5},{"RESOURCE_TRUFFLES"sv,5},
    {"RESOURCE_COCOA"sv,5},{"RESOURCE_CRAB"sv,10}};
constexpr WeightKey city_states[] = {
    {"RESOURCE_WHALE"sv,15},{"RESOURCE_PEARLS"sv,15},{"RESOURCE_GOLD"sv,10},
    {"RESOURCE_SILVER"sv,10},{"RESOURCE_GEMS"sv,10},{"RESOURCE_IVORY"sv,10},
    {"RESOURCE_FUR"sv,15},{"RESOURCE_DYE"sv,10},{"RESOURCE_SPICES"sv,15},
    {"RESOURCE_SILK"sv,15},{"RESOURCE_SUGAR"sv,10},{"RESOURCE_COTTON"sv,10},
    {"RESOURCE_WINE"sv,10},{"RESOURCE_INCENSE"sv,15},{"RESOURCE_COPPER"sv,10},
    {"RESOURCE_SALT"sv,10},{"RESOURCE_CITRUS"sv,15},{"RESOURCE_TRUFFLES"sv,15},
    {"RESOURCE_COCOA"sv,10},{"RESOURCE_CRAB"sv,15}};

bool append_weights(std::vector<BnwStartLuxuryWeight> &output,
                    std::span<const WeightKey> authored,
                    const BnwFreshStartResources &resources) {
    output.reserve(authored.size());
    for (const auto &entry : authored) {
        const auto found = std::find_if(resources.stock_shortcuts.begin(),
                                        resources.stock_shortcuts.end(),
            [&](const auto &row) { return row.type == entry.type; });
        if (found == resources.stock_shortcuts.end()) return false;
        output.push_back({found->id, entry.weight});
    }
    return true;
}
} // namespace

bool BnwFreshStartLuxuryWeights::has_blocker(BnwStartLuxuriesBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshStartLuxuryWeights::rollback() noexcept {
    for (auto &region : regions) region.clear();
    fallback.clear();
    city_states.clear();
    luxury_weights_complete = false;
}

BnwFreshStartLuxuryWeights draft_bnw_fresh_start_luxury_weights(
    const BnwFreshStartResources &resources,
    const SimulationRandom &map_random) {
    BnwFreshStartLuxuryWeights result;
    result.map_random_before = result.map_random_after = map_random.state();
    if (!resources.resource_scan_complete || resources.resources.empty() ||
        resources.selected_rules_sha256.empty() ||
        resources.stock_helper_sha256 != stock_starting_plots_sha256 ||
        !resources.has_blocker(BnwStartResourcesBlockerCode::luxury_weights_pending) ||
        resources.map_random_after != map_random.state() ||
        map_random.algorithm() != RandomAlgorithm::bnw) {
        result.blockers.push_back({BnwStartLuxuriesBlockerCode::resource_scan_pending,
                                   "Verified source-order BNW resource scan must precede luxury weights"});
        return result;
    }
    result.selected_rules_sha256 = resources.selected_rules_sha256;
    result.stock_helper_sha256 = resources.stock_helper_sha256;
    constexpr std::array<std::span<const WeightKey>, 8> authored_regions{
        tundra, jungle, forest, desert, hills, plains, grass, hybrid};
    bool complete = true;
    for (std::size_t region = 0; region < authored_regions.size(); ++region)
        complete &= append_weights(result.regions[region],
                                   authored_regions[region], resources);
    complete &= append_weights(result.fallback, fallback, resources);
    complete &= append_weights(result.city_states, city_states, resources);
    if (!complete) {
        result.rollback();
        result.blockers.push_back({BnwStartLuxuriesBlockerCode::selected_luxury_missing,
                                   "A stock luxury Type is absent from selected GameInfo.Resources"});
        return result;
    }
    result.luxury_weights_complete = true;
    result.blockers.push_back({BnwStartLuxuriesBlockerCode::custom_init_pending,
                               "AssignStartingPlots.__CustomInit and GenerateRegions remain pending"});
    return result;
}
} // namespace civ5::runtime
