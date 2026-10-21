#pragma once
#include "civ5/content/rules_database.hpp"
#include <memory>
#include <string_view>
namespace civ5::runtime {
struct LiveSessionState;
class BnwCityFoodProductionOrder;
class BnwCityCurrentFoodDifference;
class BnwCityFoodProductionRules final {
 public:
  static std::shared_ptr<const BnwCityFoodProductionRules> load(
      const content::RulesDatabase&,std::string_view expected_rules_sha256);
  void require_current(std::string_view expected_rules_sha256)const;
  ~BnwCityFoodProductionRules();
  BnwCityFoodProductionRules(const BnwCityFoodProductionRules&)=delete;
 private:
  friend class BnwCityFoodProductionOrder;
  friend BnwCityFoodProductionOrder inspect_bnw_city_food_production_order(
      const LiveSessionState&,std::shared_ptr<const BnwCityFoodProductionRules>,
      std::string_view,std::int64_t,std::int64_t);
  struct Impl;std::unique_ptr<Impl> impl_;
  explicit BnwCityFoodProductionRules(std::unique_ptr<Impl>)noexcept;
};
// Completed BNW-owned queue/rules getter, not a food-yield refresh receipt.
// Empty/non-TRAIN heads do not access Units or military-food player state;
// rules may be absent on those native branches.
BnwCityFoodProductionOrder inspect_bnw_city_food_production_order(
    const LiveSessionState&,std::shared_ptr<const BnwCityFoodProductionRules>,
    std::string_view expected_rules_sha256,std::int64_t owner,std::int64_t city);
class BnwCityFoodProductionOrder final {
 public:
  bool food_production()const;
  bool read_unit_info()const;
  bool read_military_counter()const;
  std::int64_t owner()const;
  std::int32_t city()const;
  void require_same_inputs(const LiveSessionState&,std::string_view expected_rules_sha256)const;
  static constexpr bool bnw_queue_rules_getter_complete()noexcept{return true;}
  static constexpr bool native_gc_lifecycle_owned()noexcept{return false;}
  static constexpr bool refreshed_food_yield()noexcept{return false;}
  static constexpr bool full_growth_ready()noexcept{return false;}
 private:
  friend class BnwCityCurrentFoodDifference;
  friend BnwCityFoodProductionOrder inspect_bnw_city_food_production_order(
      const LiveSessionState&,std::shared_ptr<const BnwCityFoodProductionRules>,
      std::string_view,std::int64_t,std::int64_t);
  struct Impl;std::shared_ptr<const Impl> impl_;
  static BnwCityFoodProductionOrder inspect_head_only(const LiveSessionState&,std::int64_t,std::int64_t);
  bool requires_unit_rules()const;
  void complete_from_same_rules(const LiveSessionState&,std::shared_ptr<const BnwCityFoodProductionRules>,std::string_view);
  void check_thread()const;
  explicit BnwCityFoodProductionOrder(std::shared_ptr<const Impl>)noexcept;
};
}
