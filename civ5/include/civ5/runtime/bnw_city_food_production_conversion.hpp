#pragma once
#include <cstdint>
#include <stdexcept>
namespace civ5::runtime {
class BnwCityFoodProductionConversionUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// CvCity::GetFoodProduction: the native argument is WHOLE excess food, and
// the result is WHOLE production. This pure getter neither obtains a current
// food rate nor authenticates next-turn freshness or doGrowth permission.
// Refuses overflowing native signed32 intermediates before the final /100.
std::int32_t bnw_city_food_production_from_excess(std::int32_t source_excess);
}
