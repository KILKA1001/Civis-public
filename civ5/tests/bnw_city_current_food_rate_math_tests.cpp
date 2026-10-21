#include "civ5/runtime/city_yields.hpp"
#include <climits>
#include <iostream>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f,std::string_view match){bool denied=false;try{f();}catch(const CityYieldUnavailable& e){denied=true;need(std::string_view(e.what()).find(match)!=std::string_view::npos,"Wrong native arithmetic refusal");}need(denied,"Native math accepted overflow/unimplemented recursion");}
void run(){
 // Synthetic contexts below are only operands of a pure read-math function.
 // They cannot issue a source result, refresh cursor, scalar store or turn.
 content::GlobalDefineCache defines;CityYieldRules rules;CityYieldContext s;s.yield_slots=6;
 auto& player=s.players[0];player.excess_happiness=0;auto& city=player.cities[8192];city.id=8192;city.owner=0;city.population=3;city.area=-1;
 auto& values=city.yields[0];values.terrain=2;values.buildings=3;values.specialists=-1;values.misc=4;values.religion=2;
 values.per_population=15;values.per_religion=13;values.rate_modifier=20;values.resource_modifier=-5;
 city.religions={{1,3,10},{2,1,1}};rules.belief_follower_maximum[0][0]=4;s.religions={{1,0,{0},{}}};
 // 1000 + 45 + 26 = 1071; (100 + 20 - 5 + 3) = 118;
 // native 1071*118/100 truncates to 1263.
 need(detail::bnw_native_food_rate_math(s,rules,defines,0,8192)==1263,"Native FOOD source term/division order differs");
 need(city_yield_times100(s,rules,defines,0,8192,0).total_times100==1263,"Wide existing calculation changed");
 saves::TradeRouteObservation route;route.origin_owner=0;route.destination_owner=0;route.origin_yields={5,0,0,0,0,0};route.destination_yields={7,0,0,0,0,0};s.trade_routes={route};
 need(detail::bnw_native_food_rate_math(s,rules,defines,0,8192)==1275,"Native origin then destination trade additions differ");
 route.origin_x=route.origin_y=route.destination_x=route.destination_y=-1;s.trade_routes={route};
 need(detail::bnw_native_food_rate_math(s,rules,defines,0,8192)==1263,"Native empty coordinate predicate differs");
 s.trade_routes.clear();values={};city.religions.clear();s.religions.clear();values.terrain=10000;values.rate_modifier=2900;
 need(city_yield_times100(s,rules,defines,0,8192,0).total_times100==30000000,"Wide quotient fixture differs");
 refuses([&]{(void)detail::bnw_native_food_rate_math(s,rules,defines,0,8192);},"int32 intermediate");
 values={};values.terrain=INT32_MAX;values.buildings=1;values.specialists=-1;
 refuses([&]{(void)detail::bnw_native_food_rate_math(s,rules,defines,0,8192);},"int32 intermediate");
 values={};values.terrain=-1;values.per_population=-1;city.population=1;values.rate_modifier=-50;
 need(detail::bnw_native_food_rate_math(s,rules,defines,0,8192)==-50,"Native signed truncation differs");
 values={};values.process_modifier=1;
 refuses([&]{(void)detail::bnw_native_food_rate_math(s,rules,defines,0,8192);},"nonzero process");
 values={};values.rate_modifier=INT32_MAX;
 refuses([&]{(void)detail::bnw_native_food_rate_math(s,rules,defines,0,8192);},"int32 intermediate");
 values={};player.excess_happiness=INT32_MIN;
 refuses([&]{(void)detail::bnw_native_food_rate_math(s,rules,defines,0,8192);},"int32 intermediate");
 s.no_happiness=true;need(detail::bnw_native_food_rate_math(s,rules,defines,0,8192)==0,"NO_HAPPINESS reached unused native excess negation");s.no_happiness=false;player.excess_happiness=0;
 values={};route={};route.origin_yields={INT32_MAX,0,0,0,0,0};route.destination_yields={1,0,0,0,0,0};s.trade_routes={route};
 refuses([&]{(void)detail::bnw_native_food_rate_math(s,rules,defines,0,8192);},"int32 intermediate");
 std::cout<<"PASS pure native FOOD math term ordering, checked products before division, signed truncation, trade and explicit process frontier; no source capability\n";
}
}
int main(){try{run();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
