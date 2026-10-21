#pragma once
#include <cstdint>
#include <stdexcept>
namespace civ5::runtime {
class BnwCityGrowthFoodSuffixMathUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// Arithmetic operands AFTER mandatory starving-notification resolution and the
// existing changeFoodTimes100(diff), changeFoodKept(diff/100) scalar prefix.
// No operand authenticates a city/food yield/citizens owner or effects execution.
struct BnwCityGrowthFoodSuffixMathInputs {
 std::int32_t food_times100_after_difference;
 std::int32_t food_kept_after_difference;
 std::int32_t growth_threshold;
 std::int32_t max_food_kept_percent;
 bool forced_avoid_growth;
 // Only reached by starvation AFTER changeFood(-getFood()). Growth must re-read
 // current population inside the real changePopulation handler, not this value.
 std::int32_t population_for_starvation;
};
enum class BnwCityGrowthFoodSuffixBranch {
 no_population_change,
 forced_avoid_growth,
 growth_population_required,
 starvation_floor_one,
 starvation_population_required
};
enum class BnwCityGrowthPopulationEffectsRequirement {
 none,
 // Actual changePopulation(+1,true), its SetPopulation/DoPopulationChange effects.
 grow_then_religion,
 // Actual changePopulation(-1,true) and SetPopulation/DoPopulationChange effects.
 starve_then_religion
};
enum class BnwCityGrowthNotificationRequirement {
 none_from_this_suffix,
 // AFTER actual changePopulation and its Lua/religion/citizens effects, re-read
 // getPopulation; if <=5, resolve current notifications pointer and genuine Add.
 current_post_population_small_city_notification_branch
};
struct BnwCityGrowthFoodSuffixMathResult {
 // Mathematical scalar results ONLY. Applying them needs the real current city
 // mutation owner; no stores, callbacks, events or population changes occur here.
 std::int32_t food_kept_after_cap;
 std::int32_t food_times100_after_scalar_branch;
 BnwCityGrowthFoodSuffixBranch branch;
 BnwCityGrowthPopulationEffectsRequirement required_population_effects;
 BnwCityGrowthNotificationRequirement required_notification;
 std::int32_t requested_population_change;
 static constexpr bool growth_executed()noexcept{return false;}
 static constexpr bool food_freshness_granted()noexcept{return false;}
};
// CvCity::doGrowth 12877..12921, before/through scalar branches and population
// REQUESTS. Each reached native32 product/add/subtract/negation is checked before
// division, clamp or cancellation. The post-population Lua/notification boundary
// is not simulated. Source threshold getter is positive; other signed inputs
// preserve the production source arithmetic, including negative high range caps.
BnwCityGrowthFoodSuffixMathResult bnw_city_growth_food_suffix_math(
 const BnwCityGrowthFoodSuffixMathInputs&);
}
