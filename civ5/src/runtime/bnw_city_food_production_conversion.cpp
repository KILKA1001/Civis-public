#include "civ5/runtime/bnw_city_food_production_conversion.hpp"
#include <climits>
namespace civ5::runtime {
namespace {
std::int32_t native(std::int64_t value,const char* operation){
 if(value<INT32_MIN||value>INT32_MAX)throw BnwCityFoodProductionConversionUnavailable(operation);
 return static_cast<std::int32_t>(value);
}
}
std::int32_t bnw_city_food_production_from_excess(std::int32_t food){
 std::int32_t value=0;
 if(food<=0)value=0;
 else if(food<=2)value=native(std::int64_t(food)*100,"BNW food production native product overflow");
 else if(food<=4){
  const auto excess=native(std::int64_t(food)-2,"BNW food production native subtraction overflow");
  const auto product=native(std::int64_t(excess)*50,"BNW food production native product overflow");
  value=native(std::int64_t(200)+product,"BNW food production native sum overflow");
 }else{
  const auto excess=native(std::int64_t(food)-4,"BNW food production native subtraction overflow");
  const auto product=native(std::int64_t(excess)*25,"BNW food production native product overflow");
  value=native(std::int64_t(300)+product,"BNW food production native sum overflow");
 }
 return value/100;
}
}
