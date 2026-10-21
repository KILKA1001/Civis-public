#include "civ5/runtime/bnw_citizen_plot_value_math.hpp"
#include <climits>
#include <stdexcept>
namespace civ5::runtime::detail {
namespace {
std::int32_t native(std::int64_t n){if(n<INT32_MIN||n>INT32_MAX)throw std::overflow_error("Citizens plot value native32 reached overflow");return static_cast<std::int32_t>(n);}
std::int32_t mul(std::int32_t a,std::int32_t b){return native(std::int64_t(a)*b);}
}
std::int32_t bnw_citizen_plot_value_term(std::int32_t d,std::int16_t v){return mul(d,v);}
std::array<std::int32_t,6> bnw_citizen_plot_value_prefix(
 const std::array<std::int32_t,6>& defines,const std::array<std::int16_t,6>& cached){
 std::array<std::int32_t,6> v{};
 for(std::size_t i=0;i<6;++i)v[i]=bnw_citizen_plot_value_term(defines[i],cached[i]);
 return v;
}
std::array<std::int32_t,6> bnw_citizen_plot_value_focus_food(std::array<std::int32_t,6> v,
 std::int32_t excess,bool avoid,std::int32_t focus,bool allow_growth){
 // CvEnums CityAIFocusTypes: NO=-1, FOOD0,PROD1,GOLD2,GP3,SCIENCE4,
 // CULTURE5,PROD_GROWTH6,GOLD_GROWTH7,FAITH8. GP is no yield multiplier.
 if(focus==0)v[0]=mul(v[0],3);
 else if(focus==1)v[1]=mul(v[1],3);
 else if(focus==2)v[2]=mul(v[2],3);
 else if(focus==4)v[3]=mul(v[3],3);
 else if(focus==5)v[4]=mul(v[4],3);
 else if(focus==7){v[0]=mul(v[0],2);v[2]=mul(v[2],2);}
 else if(focus==6){v[0]=mul(v[0],2);v[1]=mul(v[1],2);}
 else if(focus==8)v[5]=mul(v[5],3);
 // Source GetPlotValue uses its native flag only in this first condition.
 if(allow_growth&&excess>=0&&avoid)v[0]=0;
 else{
  if(focus!=-1&&focus!=0&&focus!=6&&focus!=7){
   const auto needed=native(-std::int64_t(excess));
   v[0]=needed>0?mul(v[0],8):v[0]/2;
  }else if(!avoid){
   const auto needed=native(std::int64_t(200)-excess);
   if(needed>0)v[0]=mul(v[0],8);
   else if(focus!=0)v[0]/=2;
  }
 }
 return v;
}
std::array<std::int32_t,6> bnw_citizen_plot_value_focus_food(std::array<std::int32_t,6> v,
 std::int32_t excess,bool avoid,std::int32_t focus){
 return bnw_citizen_plot_value_focus_food(v,excess,avoid,focus,true);
}
bool bnw_citizen_plot_value_choose(std::int32_t best,std::int32_t candidate,bool want_best)noexcept{
 return best==-1||(want_best?candidate>best:candidate<best);
}
std::int32_t bnw_citizen_plot_value_total(std::array<std::int32_t,6> v,bool avoid,std::int32_t focus,std::int32_t population){
 if((focus==-1||focus==6||focus==7)&&!avoid&&population<5)v[0]=mul(v[0],4);
 std::int32_t total=0;for(const auto n:v)total=native(std::int64_t(total)+n);
 return total;
}
std::int32_t bnw_citizen_plot_value_suffix(std::array<std::int32_t,6> v,
 std::int32_t excess,bool avoid,std::int32_t focus,std::int32_t population){
 return bnw_citizen_plot_value_total(bnw_citizen_plot_value_focus_food(v,excess,avoid,focus),avoid,focus,population);
}

}
