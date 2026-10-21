#include "civ5/runtime/bnw_city_growth_food_suffix_math.hpp"
#include <climits>
#include <iostream>
using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCityGrowthFoodSuffixMathUnavailable&){denied=true;}need(denied,"Reached native growth overflow accepted");}
BnwCityGrowthFoodSuffixMathInputs input(std::int32_t food,std::int32_t kept=0){return {food,kept,15,50,false,3};}
void run(){
 // Existing scalar prefix supplies food_before+diff and kept_before+diff/100.
 // These hand-derived POST-prefix inputs do not call a live owner or grant yield
 // freshness: old1490/kept4 + diff49 ->1539/4, growth deducts11*100.
 auto p=input(1539,4);auto r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==4&&r.food_times100_after_scalar_branch==439&&
  r.branch==BnwCityGrowthFoodSuffixBranch::growth_population_required&&r.requested_population_change==1&&
  r.required_population_effects==BnwCityGrowthPopulationEffectsRequirement::grow_then_religion&&
  r.required_notification==BnwCityGrowthNotificationRequirement::current_post_population_small_city_notification_branch,
  "Growth must preserve fractional overflow and require real population+notification owner");
 p=input(1500,900);r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==7&&r.food_times100_after_scalar_branch==700,"Cap product truncation must precede growth cost");
 p=input(3999,99);r=bnw_city_growth_food_suffix_math(p);
 need(r.food_times100_after_scalar_branch==3199&&r.requested_population_change==1,"Huge overflow must request only ONE population growth");
 p=input(1599,900);p.forced_avoid_growth=true;p.population_for_starvation=INT32_MIN;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_times100_after_scalar_branch==1500&&r.food_kept_after_cap==7&&r.requested_population_change==0&&
  r.required_population_effects==BnwCityGrowthPopulationEffectsRequirement::none&&r.required_notification==BnwCityGrowthNotificationRequirement::none_from_this_suffix,
  "Avoid growth must assign exact threshold centi and discard overflow without population");
 p=input(1499,12);r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==7&&r.food_times100_after_scalar_branch==1499&&r.requested_population_change==0,"Whole-food comparison rounded centi upward");
 // Negative fractions are not floored. -99/100 is0: no starvation population.
 p=input(-99,-5);p.population_for_starvation=INT32_MAX;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==0&&r.food_times100_after_scalar_branch==-99&&r.branch==BnwCityGrowthFoodSuffixBranch::no_population_change,"Negative fractional food was floored/repaired early");
 p=input(-199,-1);p.forced_avoid_growth=true;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_times100_after_scalar_branch==-99&&r.requested_population_change==-1&&
  r.required_population_effects==BnwCityGrowthPopulationEffectsRequirement::starve_then_religion&&r.required_notification==BnwCityGrowthNotificationRequirement::none_from_this_suffix,"Source changeFood(-getFood()) must retain negative fraction");
 p=input(-200,8);p.population_for_starvation=1;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_times100_after_scalar_branch==0&&r.requested_population_change==0&&r.branch==BnwCityGrowthFoodSuffixBranch::starvation_floor_one,"Population1 starvation must repair scalar but not decrement");
 p=input(INT32_MIN,0);r=bnw_city_growth_food_suffix_math(p);
 need(r.food_times100_after_scalar_branch==-48&&r.requested_population_change==-1,"INT32_MIN centi division/repair widened or rejected incorrectly");
 // Kept food can exceed threshold when source max-kept percentage is>100;
 // source max(0,threshold-kept) preserves complete centi stock.
 p=input(1533,22);p.max_food_kept_percent=150;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==22&&r.food_times100_after_scalar_branch==1533&&r.requested_population_change==1,"Source zero growth deduction differs");
 // Production range is low-first even for a negative high (debug assertion
 // does not change its arithmetic). Avoid std::clamp with invalid bounds.
 p=input(0,-1);p.max_food_kept_percent=-100;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==0,"Native range low-first order differs");
 p.food_kept_after_difference=0;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==-15,"Native range high branch differs");
 // Cap multiply is reached before range/compare, even if kept is already0.
 p=input(0);p.growth_threshold=INT32_MAX;p.max_food_kept_percent=2;
 refuses([&]{(void)bnw_city_growth_food_suffix_math(p);});
 p=input(0);p.growth_threshold=INT32_MAX;p.max_food_kept_percent=1;r=bnw_city_growth_food_suffix_math(p);
 need(r.food_kept_after_cap==0&&r.food_times100_after_scalar_branch==0,"Native INT32_MAX cap endpoint refused");
 // Reached deduction*100 can overflow despite final mathematical cancellation.
 // Negative max-kept produces high=-21474836; threshold1 - high =21474837,
 // delta*100=-2147483700 below INT32_MIN, while huge positive stock could cancel.
 p=input(INT32_MAX,0);p.growth_threshold=1;p.max_food_kept_percent=INT32_MIN;
 refuses([&]{(void)bnw_city_growth_food_suffix_math(p);});
 // The unconsumed population supplied value is not turned into a fake final
 // population after SetPopulation Lua; real handler/native addition remains.
 p=input(1500);p.population_for_starvation=INT32_MAX;r=bnw_city_growth_food_suffix_math(p);
 need(r.requested_population_change==1&&r.required_population_effects!=BnwCityGrowthPopulationEffectsRequirement::none,"Post-Lua population guessed or handler skipped");
 p=input(0);p.growth_threshold=0;refuses([&]{(void)bnw_city_growth_food_suffix_math(p);});
 need(!r.growth_executed()&&!r.food_freshness_granted(),"Pure scalar decision issued execution capability");
}
}
int main(){try{run();std::cout<<"PASS BNW supplied post-prefix growth food arithmetic/source branches; population/Lua/citizens/notifications remain required\n";return 0;}
 catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
