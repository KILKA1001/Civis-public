#include "civ5/runtime/bnw_city_food_production_conversion.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <string_view>
#include <utility>
namespace {
void check(bool v,const char* reason){if(!v)throw std::runtime_error(reason);}
void refuses(std::int32_t input,std::string_view operation){bool failed=false;try{
 (void)civ5::runtime::bnw_city_food_production_from_excess(input);
 }catch(const civ5::runtime::BnwCityFoodProductionConversionUnavailable& e){failed=true;check(std::string_view(e.what()).find(operation)!=std::string_view::npos,"Wrong native intermediate refusal");}
 check(failed,"Overflow was accepted after narrowing or final division");
}
}
int main(){try{
 using civ5::runtime::bnw_city_food_production_from_excess;
 constexpr std::array<std::pair<std::int32_t,std::int32_t>,19> cases{{
  {INT32_MIN,0},{-100,0},{-1,0},{0,0},{1,1},{2,2},{3,2},{4,3},
  {5,3},{6,3},{7,3},{8,4},{9,4},{10,4},{11,4},{12,5},{20,7},{100,27},{1000,252}
 }};
 for(const auto& [food,production]:cases)check(bnw_city_food_production_from_excess(food)==production,"BNW whole-food branch/quantization differs");
 // Four additional food produce one additional production in the last band;
 // integer division must not turn this native whole-food getter into VP's
 // centifood converter or preserve fractional production.
 for(std::int32_t food=5;food<64;++food)check(bnw_city_food_production_from_excess(food+4)==bnw_city_food_production_from_excess(food)+1,"Final-band native whole production step differs");
 constexpr std::int32_t last_sum_safe=(INT32_MAX-300)/25+4;
 check(bnw_city_food_production_from_excess(last_sum_safe)==21474836,"Last representable native sum changed");
 refuses(last_sum_safe+1,"sum overflow");
 constexpr std::int32_t last_product_safe=INT32_MAX/25+4;
 refuses(last_product_safe,"sum overflow");
 refuses(last_product_safe+1,"product overflow");
 refuses(INT32_MAX,"product overflow");
 // The mathematical final quotient for last_sum_safe+1 fits int32. The
 // source sum does not: validating only a wide final answer is insufficient.
 std::cout<<"PASS BNW GetFoodProduction whole-food thresholds, native truncation, inactive signed endpoints and product/sum-before-division overflow guards; no rate/freshness/growth authority\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
