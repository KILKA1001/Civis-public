#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include <climits>
#include <iostream>
#include <array>
using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCityCurrentFoodDifferenceUnavailable&){denied=true;}need(denied,"Native FOOD difference accepted unavailable operand/intermediate");}
void run(){
 using detail::bnw_food_difference_unmodified_times100;
 const auto calc=[](bool prod,int rate,int consume,bool bottom=false,
                    std::optional<std::int32_t> pop={},std::optional<std::int32_t> food={}){
  return bnw_food_difference_unmodified_times100(prod,rate,consume,bottom,pop,food);
 };
 need(calc(true,199,1)==0&&calc(false,199,1)==99,"Production whole rate division source order differs");
 need(calc(true,-199,0)==0,"Negative native division/clamp differs");
 for(int excess=-5;excess<=8;++excess)need(calc(true,excess*100,0)==0,"Food production nonpositive clamp differs");
 need(calc(false,0,1)==-100&&calc(false,199,2)==-1,"Ordinary centi units differ");
 // Positive detail values are unmodified pure arithmetic, never a result owner.
 need(calc(false,201,2)==1,"Unmodified positive arithmetic differs");
 need(calc(false,0,1,true,2)==-100,"Bottom skipped stock for population != 1");
 for(int food:{-99,0,99})need(calc(false,0,1,true,1,food)==0,"Bottom must use signed stock /100, not exact centi zero");
 for(int food:{-100,100})need(calc(false,0,1,true,1,food)==-100,"Bottom nonzero whole stock changed");
 need(calc(false,-1,0,false)==-1,"Bottom=false read missing operands");
 refuses([&]{(void)calc(false,0,1,true);});
 refuses([&]{(void)calc(false,0,1,true,1);});
 // Every reached native32 intermediate is checked before mathematical clamp
 // or cancellation can hide an overflow in the original signed-int source.
 refuses([&]{(void)calc(true,0,-INT32_MAX);});
 refuses([&]{(void)calc(true,INT32_MIN,INT32_MAX);});
 refuses([&]{(void)calc(false,0,INT32_MAX);});
 refuses([&]{(void)calc(false,0,INT32_MIN);});
 refuses([&]{(void)calc(false,INT32_MAX,-1);});
 refuses([&]{(void)calc(false,INT32_MIN,1);});
 refuses([&]{(void)calc(false,INT32_MAX,INT32_MAX,true,1,0);});
 need(calc(false,INT32_MIN,0)==INT32_MIN&&calc(false,INT32_MAX,0)==INT32_MAX,"Native finite boundaries differ");
 using detail::bnw_food_growth_religion;using detail::bnw_food_growth_total;using detail::bnw_food_growth_apply;
 const std::array<std::int32_t,3> beliefs{10,-5,7};
 need(bnw_food_growth_religion(beliefs,13)==25,"Nested majority then secondary sum differs");
 need(bnw_food_growth_total(20,10,25,25)==180,"Capital/city/religion/WLTKD native order differs");
 need(bnw_food_growth_total({},10,{},-75)==35,"Absent capital/religion source branch differs");
 need(bnw_food_growth_apply(199,125)==248,"Native positive fraction truncation differs");
 need(bnw_food_growth_apply(199,-125)==-248,"Negative growth total must remain negative");
 need(bnw_food_growth_apply(1,-99)==0&&bnw_food_growth_apply(1,0)==0,"Signed truncation/zero total differs");
 need(bnw_food_growth_apply(1,INT32_MIN)==INT32_MIN/100,"Finite negative growth endpoint differs");
 refuses([&]{(void)bnw_food_growth_apply(0,100);});
 refuses([&]{(void)bnw_food_growth_apply(-1,100);});
 refuses([&]{(void)bnw_food_growth_apply(INT32_MAX,100);}); // quotient fits; native product does not
 refuses([&]{(void)bnw_food_growth_apply(2,INT32_MIN);});
 const std::array<std::int32_t,3> cancelling{INT32_MAX,1,-1};
 refuses([&]{(void)bnw_food_growth_religion(cancelling);});
 const std::array<std::int32_t,1> max_belief{INT32_MAX};
 refuses([&]{(void)bnw_food_growth_religion(max_belief,1);});
 refuses([&]{(void)bnw_food_growth_total(INT32_MAX,-100,{});});
 refuses([&]{(void)bnw_food_growth_total({},INT32_MAX,-100);});
 refuses([&]{(void)bnw_food_growth_total({},0,INT32_MAX,-100);});
 refuses([&]{(void)bnw_food_growth_total({},0,{},INT32_MAX);});
 need(bnw_food_growth_total({},0,{},-100)==0&&bnw_food_growth_total({},0,{},-175)==-75,"Penalty totals were clamped");
 using U=detail::BnwFoodGrowthUnhappy;
 need(detail::bnw_food_growth_unhappy(true)==U::none,"NO_HAPPINESS demanded skipped excess/threshold operands");
 need(detail::bnw_food_growth_unhappy(false,-10,-10)==U::very_unhappy,"Very unhappy inclusive threshold differs");
 need(detail::bnw_food_growth_unhappy(false,-9,-10)==U::unhappy,"Unhappy negative branch differs");
 need(detail::bnw_food_growth_unhappy(false,0,-10)==U::none,"Zero excess must permit WLTKD branch");
 need(detail::bnw_food_growth_unhappy(false,5,5)==U::very_unhappy,"Configured positive very-unhappy threshold was clamped");
 refuses([&]{(void)detail::bnw_food_growth_unhappy(false);});
 refuses([&]{(void)detail::bnw_food_growth_unhappy(false,0);});
 std::cout<<"PASS pure FOOD difference source order/units/division/bBottom/positive native nested modifiers/product-before-division; no session/refresh/growth authority\n";
}
}
int main(){try{run();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
