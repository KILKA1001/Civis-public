#include "civ5/runtime/bnw_city_growth_threshold_math.hpp"
#include <cfenv>
#include <climits>
#include <cmath>
#include <limits>
namespace civ5::runtime {
namespace {
static_assert(std::numeric_limits<float>::is_iec559&&std::numeric_limits<float>::digits==24&&sizeof(float)==4);
static_assert(std::numeric_limits<double>::is_iec559&&std::numeric_limits<double>::digits==53&&sizeof(double)==8);
void need(bool ok,const char* why){if(!ok)throw BnwCityGrowthThresholdMathUnavailable(why);}
std::int32_t native(std::int64_t n,const char* why){need(n>=INT32_MIN&&n<=INT32_MAX,why);return static_cast<std::int32_t>(n);}
std::int32_t cast_integer(double n,const char* why){
 need(std::isfinite(n),why);
 // C++ (int) truncates toward zero. Checking the truncated value permits
 // representable negative/positive fractions without a nearest-integer guess.
 const auto truncated=std::trunc(n);
 need(truncated>=static_cast<double>(INT32_MIN)&&truncated<=static_cast<double>(INT32_MAX),why);
 return static_cast<std::int32_t>(truncated);
}
std::int32_t percent(std::int32_t n,std::int32_t scale,const char* why){
 return native(std::int64_t(n)*scale,why)/100;
}
}
std::int32_t bnw_city_growth_threshold_math(const BnwCityGrowthThresholdMathInputs& p){
 need(p.population>0,"BNW growth threshold native population domain unavailable");
 const auto extra_population=native(std::int64_t(p.population)-1,"BNW growth threshold population subtraction overflow");
 need(std::fegetround()==FE_TONEAREST,"BNW growth threshold nondefault FP rounding mode unavailable");
 // SDK int((population-1) * FLOAT getter): the integer becomes binary32
 // BEFORE multiplication. Store the product as binary32 before int conversion.
 const float population_float=static_cast<float>(extra_population);
 volatile float product=population_float*p.city_growth_multiplier;
 const auto linear=cast_integer(static_cast<double>(product),"BNW growth threshold linear float-to-int unavailable");
 auto threshold=native(std::int64_t(p.base_city_growth_threshold)+linear,"BNW growth threshold base sum overflow");
 // Native pow uses double(integer), double(FLOAT exponent), then truncates.
 const auto powered=std::pow(static_cast<double>(extra_population),static_cast<double>(p.city_growth_exponent));
 const auto nonlinear=cast_integer(powered,"BNW growth threshold pow-to-int unavailable");
 threshold=native(std::int64_t(threshold)+nonlinear,"BNW growth threshold exponent sum overflow");
 if(p.minor_civilization)threshold=percent(threshold,p.minor_civ_growth_percent,"BNW growth threshold minor product overflow");
 threshold=percent(threshold,p.game_speed_growth_percent,"BNW growth threshold game speed product overflow");
 threshold=percent(threshold,p.start_era_growth_percent,"BNW growth threshold start era product overflow");
 if(!p.human&&!p.ai_teammate_of_human&&!p.barbarian){
  threshold=percent(threshold,p.game_handicap_ai_growth_percent,"BNW growth threshold AI growth product overflow");
  const auto era_product=native(std::int64_t(p.game_handicap_ai_per_era_modifier)*p.player_current_era,
   "BNW growth threshold AI current era product overflow");
  const auto era_sum=native(std::int64_t(era_product)+100,"BNW growth threshold AI current era sum overflow");
  const auto scale=era_sum>0?era_sum:std::int32_t{0};
  threshold=percent(threshold,scale,"BNW growth threshold AI era scale product overflow");
 }
 return threshold>1?threshold:std::int32_t{1};
}
}
