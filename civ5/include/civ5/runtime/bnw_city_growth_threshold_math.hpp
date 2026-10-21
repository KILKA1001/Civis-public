#pragma once
#include <cstdint>
#include <stdexcept>
namespace civ5::runtime {
class BnwCityGrowthThresholdMathUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// Supplied arithmetic operands only. These predicates/values authenticate no
// player, city, population, active rules, notification, cursor or doGrowth stage.
// Native define getters MULTIPLIER and EXPONENT are float, not double or int.
struct BnwCityGrowthThresholdMathInputs {
 std::int32_t population;
 std::int32_t base_city_growth_threshold;
 float city_growth_multiplier;
 float city_growth_exponent;
 bool minor_civilization;
 std::int32_t minor_civ_growth_percent;
 std::int32_t game_speed_growth_percent;
 std::int32_t start_era_growth_percent;
 bool human;
 bool ai_teammate_of_human;
 bool barbarian;
 std::int32_t game_handicap_ai_growth_percent;
 std::int32_t game_handicap_ai_per_era_modifier;
 std::int32_t player_current_era;
};
// SDK CvCity::growthThreshold -> CvPlayer::getGrowthThreshold arithmetic only.
// Each native int32 operation/conversion is checked BEFORE later truncation,
// percentage cancellation or final max(1). Requires normal FP rounding mode.
std::int32_t bnw_city_growth_threshold_math(const BnwCityGrowthThresholdMathInputs&);
}
