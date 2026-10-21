#include "civ5/runtime/bnw_citizen_plot_value_math.hpp"
#include <climits>
#include <iostream>
#include <stdexcept>
using namespace civ5::runtime::detail;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool failed=false;try{f();}catch(const std::overflow_error&){failed=true;}need(failed,"Reached native plot value overflow accepted");}
}
int main(){try{
 std::array<std::int32_t,6> d{12,8,10,6,4,3};std::array<std::int16_t,6> cache{2,1,1,0,0,0};
 const auto base=bnw_citizen_plot_value_prefix(d,cache);
 need(base==std::array<std::int32_t,6>{24,8,10,0,0,0},"Six native products differ");
 need(bnw_citizen_plot_value_suffix(base,0,false,-1,1)==786,"Small default city food bias differs");
 need(bnw_citizen_plot_value_suffix(base,200,false,-1,5)==30,"Sufficient-food half value differs");
 need(bnw_citizen_plot_value_suffix(base,-100,false,1,5)==226,"Production focus/negative-food bias differs");
 need(bnw_citizen_plot_value_suffix(base,0,true,1,5)==34,"AvoidGrowth zero-food path differs");
 // Even subsequently zeroed food must first execute its reached native focus product.
 auto edge=base;edge[0]=INT32_MAX;
 refuses([&]{(void)bnw_citizen_plot_value_suffix(edge,0,true,0,1);});
 refuses([&]{(void)bnw_citizen_plot_value_suffix(base,INT32_MIN,false,1,5);});
 refuses([&]{(void)bnw_citizen_plot_value_suffix(base,INT32_MIN,false,-1,5);});
 need(bnw_citizen_plot_value_suffix(base,INT32_MIN,true,-1,5)==42,"Avoid=true negative-food skipped wrong branch");
 edge.fill(0);edge[1]=INT32_MAX;edge[2]=1;edge[3]=-1;
 refuses([&]{(void)bnw_citizen_plot_value_suffix(edge,200,false,-1,5);});
 d[0]=INT32_MAX;cache[0]=2;refuses([&]{(void)bnw_citizen_plot_value_prefix(d,cache);});
 // SDK false mode still reads avoid-growth but bypasses food-zeroing.
 const auto false_mode=bnw_citizen_plot_value_focus_food(base,100,true,2,false);
 need(false_mode==std::array<std::int32_t,6>{12,8,30,0,0,0},"Native false mode skipped positive-food half branch");
 need(bnw_citizen_plot_value_total(false_mode,true,2,1)==50,"Native false-mode total/order differs");
 need(bnw_citizen_plot_value_focus_food(base,100,true,2,true)==
  bnw_citizen_plot_value_focus_food(base,100,true,2),"Legacy true mode changed");
 need(!bnw_citizen_plot_value_choose(20,20,false),"Worst tie replaced first native slot");
 need(bnw_citizen_plot_value_choose(-1,0,false),"Native -1 sentinel replaced by optional-min semantics");
 need(bnw_citizen_plot_value_choose(20,-5,false),"Signed worst score comparison changed");
 need(bnw_citizen_plot_value_choose(INT32_MAX,INT32_MIN,false)&&!bnw_citizen_plot_value_choose(INT32_MIN,INT32_MAX,false),"Signed native comparator narrowed an edge");
 const std::array<std::int32_t,4> ordered{4,4,-1,9};std::int32_t score=-1;std::size_t index=99;
 for(std::size_t i=0;i<ordered.size();++i)if(bnw_citizen_plot_value_choose(score,ordered[i],false)){score=ordered[i];index=i;}
 need(index==3&&score==9,"Native genuine -1 score sentinel replaced by sorted/optional minimum");
 // Duplicate forward occurrences are deliberately visited, not deduplicated.
 score=-1;index=99;for(std::size_t i=0;i<2;++i)if(bnw_citizen_plot_value_choose(score,4,false)){score=4;index=i;}
 need(index==0,"Equal wrapped occurrences no longer preserve first native slot");

 refuses([&]{(void)bnw_citizen_plot_value_focus_food(base,INT32_MIN,true,2,false);});
 std::cout<<"Citizens plot value pure source cases passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
