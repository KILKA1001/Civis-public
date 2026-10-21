#include "civ5/runtime/bnw_city_growth_threshold_math.hpp"
#include <array>
#include <cfenv>
#include <climits>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>
using namespace civ5::runtime;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
BnwCityGrowthThresholdMathInputs standard(){
 // Expansion2 GlobalDefines:15,8,1.5,150; normal speed/start era100.
 // This is a math fixture, not activated DB or current player authority.
 return {1,15,8.0f,1.5f,false,150,100,100,true,false,false,100,0,0};
}
void refuses(const BnwCityGrowthThresholdMathInputs& p,std::string_view stage){
 bool denied=false;try{(void)bnw_city_growth_threshold_math(p);}
 catch(const BnwCityGrowthThresholdMathUnavailable& e){denied=true;check(std::string_view(e.what()).find(stage)!=std::string_view::npos,"Wrong source arithmetic frontier");}
 check(denied,"Undefined native intermediate accepted via later cancellation");
}
}
int main(){try{
 check(std::fegetround()==FE_TONEAREST,"Math tests require native default FP mode");
 for(const auto& [population,expected]:std::array<std::pair<std::int32_t,std::int32_t>,7>{{{1,15},{2,24},{3,33},{4,44},{5,55},{10,114},{17,207}}}){
  auto p=standard();p.population=population;check(bnw_city_growth_threshold_math(p)==expected,"BNW base/float-power integer sequence differs");
 }
 auto p=standard();p.population=3;p.minor_civilization=true;p.game_speed_growth_percent=67;p.start_era_growth_percent=90;
 check(bnw_city_growth_threshold_math(p)==28,"Percent stages were combined or reordered"); //33 ->49 ->32 ->28.
 p=standard();p.population=3;p.human=false;p.game_handicap_ai_growth_percent=75;p.game_handicap_ai_per_era_modifier=-5;p.player_current_era=3;
 check(bnw_city_growth_threshold_math(p)==20,"AI game handicap/current era order differs"); //33 ->24 ->20.
 p.game_handicap_ai_per_era_modifier=-100;p.player_current_era=2;check(bnw_city_growth_threshold_math(p)==1,"AI era zero clamp/final minimum differs");
 for(std::int32_t branch=0;branch<3;++branch){
  p=standard();p.population=3;p.human=branch==0;p.ai_teammate_of_human=branch==1;p.barbarian=branch==2;
  p.game_handicap_ai_growth_percent=INT32_MAX;p.game_handicap_ai_per_era_modifier=INT32_MAX;p.player_current_era=INT32_MAX;
  p.minor_civ_growth_percent=INT32_MAX;
  check(bnw_city_growth_threshold_math(p)==33,"Inactive AI/minor operands were consumed");
 }
 p=standard();p.population=2;p.base_city_growth_threshold=20;p.city_growth_multiplier=-0.75f;p.city_growth_exponent=0;
 check(bnw_city_growth_threshold_math(p)==21,"Negative float converted by floor/nearest instead of C++ truncation");
 p.population=2;p.base_city_growth_threshold=20;p.city_growth_multiplier=1.75f;p.city_growth_exponent=0;
 check(bnw_city_growth_threshold_math(p)==22,"Linear float conversion was rounded rather than truncated");
 p=standard();p.population=16777218;p.base_city_growth_threshold=0;p.city_growth_multiplier=1;p.city_growth_exponent=0;
 check(bnw_city_growth_threshold_math(p)==16777217,"Integer-to-float source rounding was widened to double");
 p=standard();p.base_city_growth_threshold=-3;p.city_growth_exponent=1;p.game_speed_growth_percent=-200;
 check(bnw_city_growth_threshold_math(p)==6,"Final minimum was applied before percentages");
 p=standard();p.population=2;p.base_city_growth_threshold=0;p.city_growth_multiplier=2147483520.0f;p.city_growth_exponent=0;p.game_speed_growth_percent=0;
 check(bnw_city_growth_threshold_math(p)==1,"Representable upper float conversion refused");
 p.city_growth_multiplier=2147483648.0f;refuses(p,"linear float-to-int");
 p=standard();p.population=0;refuses(p,"population domain");p.population=INT32_MIN;refuses(p,"population domain");
 p=standard();p.city_growth_multiplier=std::numeric_limits<float>::infinity();refuses(p,"linear float-to-int");
 p=standard();p.city_growth_exponent=-1;refuses(p,"pow-to-int");
 p=standard();p.population=3;p.city_growth_exponent=31;p.game_speed_growth_percent=0;refuses(p,"pow-to-int");
 p=standard();p.population=2;p.base_city_growth_threshold=INT32_MAX;refuses(p,"base sum");
 p=standard();p.population=2;p.base_city_growth_threshold=INT32_MAX;p.city_growth_multiplier=0;refuses(p,"exponent sum");
 p=standard();p.minor_civilization=true;p.minor_civ_growth_percent=INT32_MAX;refuses(p,"minor product");
 p=standard();p.game_speed_growth_percent=INT32_MAX;p.start_era_growth_percent=0;refuses(p,"game speed product");
 p=standard();p.start_era_growth_percent=INT32_MAX;refuses(p,"start era product");
 p=standard();p.human=false;p.game_handicap_ai_growth_percent=INT32_MAX;refuses(p,"AI growth product");
 p=standard();p.human=false;p.game_handicap_ai_per_era_modifier=INT32_MAX;p.player_current_era=2;refuses(p,"AI current era product");
 p.player_current_era=1;refuses(p,"AI current era sum");
 p.game_handicap_ai_per_era_modifier=INT32_MAX-100;p.player_current_era=1;refuses(p,"AI era scale product");
 p=standard();p.population=2;p.city_growth_multiplier=-2147483648.0f;p.base_city_growth_threshold=INT32_MAX;p.city_growth_exponent=0;
 check(bnw_city_growth_threshold_math(p)==1,"Representable INT32_MIN float conversion differs");
 // Ambient FP mode is a typed frontier, never silently changed by the module.
 if(std::fesetround(FE_DOWNWARD)==0){
  bool denied=false;try{(void)bnw_city_growth_threshold_math(standard());}catch(const BnwCityGrowthThresholdMathUnavailable&){denied=true;}
  const auto restored=std::fesetround(FE_TONEAREST);check(restored==0&&denied,"Nondefault FP mode accepted or test failed to restore it");
 }
 std::cout<<"PASS prepared BNW growth threshold float/pow/truncation, ordered minor/speed/start-era/AI branches and reached native overflow; pure math only\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
