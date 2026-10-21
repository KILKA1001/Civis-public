#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include <climits>
#include <iostream>
#include <type_traits>
using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCitySpecialistCountsUnavailable&){denied=true;}need(denied,"Specialist pure math admitted unsupported native operation");}
void run(){
 using detail::bnw_specialist_reset_read_math;
 auto reset=bnw_specialist_reset_read_math(5,99,{});need(reset.dense==std::vector<std::int32_t>(5,0)&&reset.total==0,"Actual Reset zero loop differs");
 // Hole/null slots are allocated zero; null has no value, unknown consumes its
 // value; repeated mapped assignments keep the LAST wire value, not a sum.
 auto rows=bnw_specialist_reset_read_math(5,0,{{{},{}},{{},7},{1,4},{1,-2},{4,9},{-1,77},{99,88}});
 need(rows.dense==std::vector<std::int32_t>({0,-2,0,0,9})&&rows.total==7,"Reset/ordered Read/consume-discard differs");
 need(bnw_specialist_reset_read_math(3,1,{{1,INT32_MAX},{2,-3}}).total==-3,"Native default-specialist exclusion differs");
 need(bnw_specialist_reset_read_math(3,2,{{1,4}}).total==4,"Hole default index differs");
 need(bnw_specialist_reset_read_math(0,-1,{{99,7},{},{-1,3}}).total==0,"Zero span/native out-of-range consume differs");
 refuses([&]{(void)bnw_specialist_reset_read_math(-1,0,{});});
 refuses([&]{(void)bnw_specialist_reset_read_math(INT32_MAX,0,{});});
 refuses([&]{(void)bnw_specialist_reset_read_math(2,0,{{1,{}}});});
 refuses([&]{(void)bnw_specialist_reset_read_math(2,0,{{-2,3}});});
 // Native ascending prefix overflow cannot be hidden by a later cancellation.
 refuses([&]{(void)bnw_specialist_reset_read_math(3,-1,{{0,INT32_MAX},{1,1},{2,INT32_MIN}});});
 refuses([&]{(void)bnw_specialist_reset_read_math(3,-1,{{0,INT32_MIN},{1,-1},{2,INT32_MAX}});});
 need(bnw_specialist_reset_read_math(2,-1,{{0,INT32_MAX},{1,-INT32_MAX}}).total==0,"Safe signed cancellation differs");
 // A pure remap DTO result cannot call the private source factory or replace
 // the native-half typed frontier on the existing generic unhappiness path.
 std::cout<<"PASS pure native Reset/Read span/holes/null/unknown/duplicate/default/negative/prefix boundaries; no source issuer\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCitySpecialistCounts>);
static_assert(!std::is_constructible_v<BnwCitySpecialistCounts,detail::BnwSpecialistMathResult>);
int main(){try{run();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
