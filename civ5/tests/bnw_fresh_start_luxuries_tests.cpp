#include "civ5/runtime/bnw_fresh_start_luxuries.hpp"
#include <array>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_stock_luxury_weights() {
    using namespace civ5::runtime;
    constexpr std::array names{
        "RESOURCE_WHALE", "RESOURCE_PEARLS", "RESOURCE_GOLD",
        "RESOURCE_SILVER", "RESOURCE_GEMS", "RESOURCE_IVORY",
        "RESOURCE_FUR", "RESOURCE_DYE", "RESOURCE_SPICES",
        "RESOURCE_SILK", "RESOURCE_SUGAR", "RESOURCE_COTTON",
        "RESOURCE_WINE", "RESOURCE_INCENSE", "RESOURCE_COPPER",
        "RESOURCE_SALT", "RESOURCE_CITRUS", "RESOURCE_TRUFFLES",
        "RESOURCE_COCOA", "RESOURCE_CRAB"};
    SimulationRandom map_random(RandomAlgorithm::bnw, {41, 8, 0});
    BnwFreshStartResources resources;
    resources.resource_scan_complete = true;
    resources.selected_rules_sha256 = "selected-bnw-fixture";
    resources.stock_helper_sha256 =
        "a957db6a15138a09aef4221217c654ef146dca65aee17aea701e13b3db3b1b32";
    resources.map_random_before = resources.map_random_after = map_random.state();
    resources.blockers.push_back({BnwStartResourcesBlockerCode::luxury_weights_pending,
                                  "fixture"});
    for (std::size_t i = 0; i < names.size(); ++i) {
        BnwStartResourceRow row{static_cast<std::int32_t>(100 + i), names[i]};
        resources.resources.push_back(row);
        resources.stock_shortcuts.push_back(row);
    }
    const auto before = map_random.state();
    auto weights = draft_bnw_fresh_start_luxury_weights(resources, map_random);
    check(weights.luxury_weights_complete &&
              weights.has_blocker(BnwStartLuxuriesBlockerCode::custom_init_pending) &&
              weights.selected_rules_sha256 == resources.selected_rules_sha256 &&
              weights.map_random_before == before && weights.map_random_after == before &&
              map_random.state() == before,
          "Luxury weight assignment is detached, selected and RNG-free");
    constexpr std::array<std::size_t, 8> source_counts{8, 11, 12, 8, 8, 10, 11, 18};
    for (std::size_t i = 0; i < source_counts.size(); ++i)
        check(weights.regions[i].size() == source_counts[i],
              "Stock luxury region preserves every source pair");
    check(weights.fallback.size() == 20 && weights.city_states.size() == 20 &&
              weights.regions[0][0] == BnwStartLuxuryWeight{106, 40} &&
              weights.regions[0][1] == BnwStartLuxuryWeight{100, 35} &&
              weights.regions[7].back() == BnwStartLuxuryWeight{102, 5} &&
              weights.fallback.front() == BnwStartLuxuryWeight{100, 10} &&
              weights.city_states.back() == BnwStartLuxuryWeight{119, 15},
          "Resource IDs come from selected rows while weights follow Lua order");
    auto missing = resources;
    missing.stock_shortcuts.pop_back(); // RESOURCE_CRAB
    const auto refused = draft_bnw_fresh_start_luxury_weights(missing, map_random);
    check(refused.has_blocker(BnwStartLuxuriesBlockerCode::selected_luxury_missing) &&
              !refused.luxury_weights_complete && refused.fallback.empty() &&
              refused.regions[0].empty(),
          "Missing selected luxury cannot publish partial weights");
    weights.rollback();
    check(!weights.luxury_weights_complete && weights.regions[0].empty() &&
              weights.fallback.empty() && weights.city_states.empty() &&
              map_random.state() == before,
          "Rollback discards the detached luxury weight tables");
}
} // namespace

int main() { test_stock_luxury_weights(); }
