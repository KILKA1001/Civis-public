#pragma once
#include <array>
#include <cstdint>
#include <memory>
namespace civ5::runtime {
struct LiveSessionState;
class BnwCityFoodBaseYield;
// Exact native-width getBaseYieldRate(YIELD_FOOD) on current raw/owned inputs.
// This does not replace getYieldRateTimes100 or assert a next-turn cache refresh.
BnwCityFoodBaseYield inspect_bnw_city_food_base_yield(
    const LiveSessionState&,std::int64_t owner,std::int64_t city);
class BnwCityFoodBaseYield final {
 public:
  std::int32_t value()const;
  std::array<std::int32_t,5> components()const;
  void require_same_inputs(const LiveSessionState&)const;
  static constexpr bool base_getter_complete()noexcept{return true;}
  static constexpr bool refreshed_food_yield()noexcept{return false;}
  static constexpr bool full_growth_ready()noexcept{return false;}
 private:
  friend BnwCityFoodBaseYield inspect_bnw_city_food_base_yield(
      const LiveSessionState&,std::int64_t,std::int64_t);
  struct Impl;std::shared_ptr<const Impl> impl_;
  explicit BnwCityFoodBaseYield(std::shared_ptr<const Impl>)noexcept;
  void check_thread()const;
};
}
