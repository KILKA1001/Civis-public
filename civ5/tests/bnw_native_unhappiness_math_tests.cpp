#include "civ5/runtime/bnw_unhappiness.hpp"
#include <climits>
#include <iostream>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f,std::string_view text){bool denied=false;try{f();}catch(const std::exception& e){denied=true;need(std::string_view(e.what()).find(text)!=std::string_view::npos,"Wrong native unhappiness refusal");}need(denied,"Native unhappiness accepted unsupported/overflow math");}
void run(){
 // All synthetic variation is pure arithmetic; no source owner can be issued.
 content::RulesDatabase db;db.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES('UNHAPPINESS_PER_CITY',2),('UNHAPPINESS_PER_CAPTURED_CITY',5),('UNHAPPINESS_PER_POPULATION',1),('UNHAPPINESS_PER_OCCUPIED_POPULATION',1.34),('FREE_UNIT_HAPPINESS',0);");
 const auto defs=db.cache_global_defines(content::GlobalDefineProfile::bnw);
 BnwUnhappinessRules rules;rules.handicaps[0]={100,100,100};
 BnwUnhappinessContext u;u.world_cities_percent=100;auto& p=u.players[0];p.happiness=12;
 ResearchContext setup;setup.game_handicap=0;auto& r=setup.players[0];r.team=0;r.handicap=0;r.alive=true;r.slot_status=3;
 CityYieldContext world;auto& player=world.players[0];player.capital=8192;
 for(auto [id,pop]:{std::pair{8192,3},std::pair{8193,2}}){auto& c=player.cities[id];c.id=id;c.owner=0;c.population=pop;p.cities[id]={};}
 const auto native=[&]{return detail::bnw_native_unhappiness_math(u,setup,world,rules,defs,0);};
 auto q=native();need(q.normal_cities_times100==400&&q.normal_population_times100==500&&q.total==9&&q.excess_happiness==3,"Native unhappiness source stages differ");
 need(bnw_unhappiness(u,setup,world,rules,defs,0).excess_happiness==3,"Legacy wide unhappiness changed");
 p.cities_modifier=10000000;refuses(native,"int32 intermediate");need(bnw_unhappiness(u,setup,world,rules,defs,0).total<INT32_MAX,"Wide quotient-before-native-overflow fixture no longer meaningful");p.cities_modifier=0;
 p.half_specialists=true;refuses(native,"half-specialist");p.half_specialists=false;
 // Native occupied float*100 rounding then double population conversion.
 p.cities.at(8193).occupied=true;q=native();need(q.captured_cities_times100==500&&q.occupied_population_times100==268,"Native occupied conversion differs");
 player.cities.at(8193).population=INT32_MAX;refuses(native,"conversion");
 p.cities.clear();player.cities.clear();
 // Full IDs sort differently from their real FLTA slots. Native slot0+slot1
 // overflows before slot2's cancellation, although the wide aggregate fits.
 for(auto [id,pop]:{std::pair{8194,-15000000},std::pair{16384,15000000},std::pair{24577,15000000}}){auto& c=player.cities[id];c.id=id;c.owner=0;c.population=pop;p.cities[id]={};}
 refuses(native,"int32 intermediate");need(bnw_unhappiness(u,setup,world,rules,defs,0).total<INT32_MAX,"Wide FLTA-order overflow fixture no longer meaningful");
 std::cout<<"PASS pure native unhappiness checked inner products/conversion/slot sums and explicit half-specialist frontier; legacy wide result preserved; no source authority\n";
}
}
int main(){try{run();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
