#pragma once
#include "civ5/content/rules_database.hpp"
#include <memory>
#include <stdexcept>
#include <string_view>
namespace civ5::runtime {
struct LiveSessionState;
class BnwCityCurrentFoodConsumption;
class BnwCityCurrentFoodRate;
class BnwCityCurrentFoodDifference;
class BnwCityCurrentFoodConsumptionUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// Rules are the actual immutable activated BNW database for expected_rules_sha.
// This reuses the existing native-width source calculation; no yield refresh
// or whole doGrowth authority follows from completing this getter.
BnwCityCurrentFoodConsumption inspect_bnw_city_current_food_consumption(
 const LiveSessionState&,const content::RulesDatabase&,std::string_view expected_rules_sha,
 std::int64_t owner,std::int64_t city,std::int64_t extra=0);
class BnwCityCurrentFoodConsumption final {
 public:
  std::int32_t value()const;
  std::int32_t population_with_extra()const;
  std::int32_t specialist_total()const;
  std::int32_t reduction()const;
  bool reads_specialists()const;
  void require_same_inputs(const LiveSessionState&,std::string_view expected_rules_sha)const;
  static constexpr bool consumption_getter_complete()noexcept{return true;}
  static constexpr bool native_gc_cache_owned()noexcept{return false;}
  static constexpr bool refreshed_food_yield()noexcept{return false;}
  static constexpr bool full_growth_ready()noexcept{return false;}
 private:
  friend class BnwCityCurrentFoodDifference;
  friend class BnwCityEntryPending;
  friend BnwCityCurrentFoodConsumption inspect_bnw_city_current_food_consumption(
   const LiveSessionState&,const content::RulesDatabase&,std::string_view,
   std::int64_t,std::int64_t,std::int64_t);
  struct Impl;std::shared_ptr<const Impl> impl_;
  explicit BnwCityCurrentFoodConsumption(std::shared_ptr<const Impl>)noexcept;
  void check_current()const;
  static BnwCityCurrentFoodConsumption from_same_rate(const BnwCityCurrentFoodRate&,std::int64_t,std::int64_t,std::int64_t);
  static BnwCityCurrentFoodConsumption make(const LiveSessionState&,const content::RulesDatabase&,
   std::string_view,std::int64_t,std::int64_t,std::int64_t,const BnwCityCurrentFoodRate*);
};
}
