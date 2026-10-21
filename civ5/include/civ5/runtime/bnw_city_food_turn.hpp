#pragma once
#include <cstdint>
#include <stdexcept>
namespace civ5::runtime {
struct LiveSessionState;
class BnwCityFoodTurnStep;
// Gameplay component after production-compile instrumentation. Invoke on the
// enclosing detached city-turn state. Only the actual IsRazing early return is
// currently implemented; no caller bool, food delta or saved-yield freshness.
BnwCityFoodTurnStep apply_bnw_city_food_turn(
    LiveSessionState&,std::int64_t owner,std::int64_t city);
class BnwCityFoodTurnUnavailable final : public std::runtime_error {
 public:
  BnwCityFoodTurnUnavailable();
  static constexpr const char* frontier()noexcept {
    return "CvCity::foodDifferenceTimes100: connect actual food-production order/UnitInfo and current food consumption to refreshed food yield; then growth modifiers, threshold, clamp and population effects";
  }
};
class BnwCityFoodTurnStep final {
 public:
  std::int64_t owner()const noexcept{return owner_;}
  std::int32_t city()const noexcept{return city_;}
  std::int32_t razing_turns()const noexcept{return razing_;}
  static constexpr bool returned_for_razing()noexcept{return true;}
  static constexpr bool changed_food_or_population()noexcept{return false;}
  static constexpr bool full_growth_ready()noexcept{return false;}
 private:
  friend BnwCityFoodTurnStep apply_bnw_city_food_turn(
      LiveSessionState&,std::int64_t,std::int64_t);
  BnwCityFoodTurnStep(std::int64_t owner,std::int32_t city,std::int32_t razing)
      noexcept:owner_(owner),city_(city),razing_(razing){}
  std::int64_t owner_;
  std::int32_t city_,razing_;
};
}
