#pragma once
#include "civ5/runtime/bnw_city_current_food_rate.hpp"
#include <optional>
#include <span>
namespace civ5::runtime {
class BnwCityCurrentFoodDifferenceUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Pure native expression only. Arbitrary inputs issue no read owner, source
// receipt, freshness, growth capability or store. Positive is UNMODIFIED.
std::int32_t bnw_food_difference_unmodified_times100(
 bool food_production,std::int32_t rate_times100,std::int32_t consumption,
 bool bottom,std::optional<std::int32_t> population={},std::optional<std::int32_t> food_times100={});
// Pure already-selected source arithmetic, never an owner/capability factory.
std::int32_t bnw_food_growth_religion(std::span<const std::int32_t> majority,
 std::optional<std::int32_t> secondary={});
std::int32_t bnw_food_growth_total(std::optional<std::int32_t> capital,
 std::int32_t city,std::optional<std::int32_t> religion,
 std::optional<std::int32_t> selected_penalty_or_wltkd={});
std::int32_t bnw_food_growth_apply(std::int32_t positive_difference,
 std::int32_t growth_total);
// Pure source predicate only; no world/readiness capability follows.
enum class BnwFoodGrowthUnhappy { none, very_unhappy, unhappy };
BnwFoodGrowthUnhappy bnw_food_growth_unhappy(bool no_happiness,
 std::optional<std::int32_t> excess={},std::optional<std::int32_t> very_threshold={});
}
class BnwCityCurrentFoodDifference final {
 public:
  // Genuine current numeric read, including reached source growth modifiers.
  // Never a completed growth/refresh/turn or food setter receipt.
  std::int32_t value_times100()const;
 private:
  struct Impl;std::shared_ptr<const Impl> impl_;
  struct Growth;
  explicit BnwCityCurrentFoodDifference(std::shared_ptr<const Impl>)noexcept;
  static BnwCityCurrentFoodDifference make(std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwCityFoodRateRules>,std::int64_t,std::int64_t,bool);
  static Growth capture_growth(const Impl&);
  static std::int32_t evaluate(const Impl&,Growth* initial=nullptr);
  friend BnwCityCurrentFoodDifference inspect_bnw_city_current_food_difference(
   std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwCityFoodRateRules>,std::int64_t,std::int64_t,bool);
};
BnwCityCurrentFoodDifference inspect_bnw_city_current_food_difference(
 std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwCityFoodRateRules>,std::int64_t owner,std::int64_t city,bool bottom=true);
}
