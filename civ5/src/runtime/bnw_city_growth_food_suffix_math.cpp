#include "civ5/runtime/bnw_city_growth_food_suffix_math.hpp"
#include <climits>
namespace civ5::runtime {
namespace {
std::int32_t native(std::int64_t n){
 if(n<INT32_MIN||n>INT32_MAX)
  throw BnwCityGrowthFoodSuffixMathUnavailable("BNW growth food suffix reached native signed32 overflow");
 return static_cast<std::int32_t>(n);
}
std::int32_t add(std::int32_t a,std::int32_t b){return native(std::int64_t(a)+b);}
std::int32_t subtract(std::int32_t a,std::int32_t b){return native(std::int64_t(a)-b);}
std::int32_t multiply(std::int32_t a,std::int32_t b){return native(std::int64_t(a)*b);}
std::int32_t negate(std::int32_t n){return native(-std::int64_t(n));}
// CvGameCoreUtils::range: do NOT replace with std::clamp. Its production branch
// order handles a negative high as low first, then high, despite debug assertion.
std::int32_t range_zero(std::int32_t n,std::int32_t high){
 if(n<0)return 0;
 if(n>high)return high;
 return n;
}
std::int32_t change_food(std::int32_t food_times100,std::int32_t delta){
 // CvCity::changeFood: multiply first, THEN add. Cancellation cannot make a
 // reached overflowing native product available.
 const auto centi=multiply(100,delta);return add(food_times100,centi);
}
}
BnwCityGrowthFoodSuffixMathResult bnw_city_growth_food_suffix_math(
 const BnwCityGrowthFoodSuffixMathInputs& p){
 if(p.growth_threshold<1)
  throw BnwCityGrowthFoodSuffixMathUnavailable("BNW growth food suffix requires positive source growthThreshold");
 const auto cap=multiply(p.growth_threshold,p.max_food_kept_percent)/100;
 BnwCityGrowthFoodSuffixMathResult r{range_zero(p.food_kept_after_difference,cap),
  p.food_times100_after_difference,BnwCityGrowthFoodSuffixBranch::no_population_change,
  BnwCityGrowthPopulationEffectsRequirement::none,BnwCityGrowthNotificationRequirement::none_from_this_suffix,0};
 const auto whole=p.food_times100_after_difference/100;
 if(whole>=p.growth_threshold){
  if(p.forced_avoid_growth){
   r.food_times100_after_scalar_branch=multiply(p.growth_threshold,100);
   r.branch=BnwCityGrowthFoodSuffixBranch::forced_avoid_growth;
  }else{
   const auto difference=subtract(p.growth_threshold,r.food_kept_after_cap);
   const auto retained_cost=difference>0?difference:0;
   r.food_times100_after_scalar_branch=change_food(p.food_times100_after_difference,negate(retained_cost));
   r.branch=BnwCityGrowthFoodSuffixBranch::growth_population_required;
   r.required_population_effects=BnwCityGrowthPopulationEffectsRequirement::grow_then_religion;
   r.required_notification=BnwCityGrowthNotificationRequirement::current_post_population_small_city_notification_branch;
   r.requested_population_change=1;
  }
 }else if(whole<0){
  r.food_times100_after_scalar_branch=change_food(p.food_times100_after_difference,negate(whole));
  if(p.population_for_starvation>1){
   r.branch=BnwCityGrowthFoodSuffixBranch::starvation_population_required;
   r.required_population_effects=BnwCityGrowthPopulationEffectsRequirement::starve_then_religion;
   r.requested_population_change=-1;
  }else r.branch=BnwCityGrowthFoodSuffixBranch::starvation_floor_one;
 }
 return r;
}
}
