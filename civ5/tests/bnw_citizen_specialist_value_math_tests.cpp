#include "civ5/runtime/bnw_citizen_specialist_value_math.hpp"
#include "civ5/runtime/bnw_citizen_plot_value_math.hpp"
#include <climits>
#include <iostream>
#include <stdexcept>
using namespace civ5::runtime;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refuses(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}need(failed,"Missing reached native overflow refusal");}
}
int main(){try{
 need(detail::bnw_specialist_focus_yield(3)==-1&&detail::bnw_specialist_focus_yield(-1)==-1&&
  detail::bnw_specialist_focus_yield(4)==3&&detail::bnw_specialist_focus_yield(7)==0,
  "Native default-specialist focus selection differs");
 need(detail::bnw_specialist_weighted_yield(7,-3)==-21,"Native specialist weighted product differs");
 need(detail::bnw_specialist_weighted_yield(-1,7)==-7&&detail::bnw_specialist_weighted_yield(-1,-7)==7,"Signed process operand multiplication differs");
 refuses([]{(void)detail::bnw_specialist_weighted_yield(-1,INT32_MIN);});

 refuses([]{(void)detail::bnw_specialist_weighted_yield(INT32_MAX,2);});
 const auto building=detail::bnw_specialist_building_value(3,99);
 need(building.adjusted==118&&building.unmodified==99,"Source building slack adjustment/truncation differs");
 refuses([]{(void)detail::bnw_specialist_building_value(INT32_MIN,0);});
 refuses([]{(void)detail::bnw_specialist_building_value(2,INT32_MAX);});
 civ5::saves::TeamHashedArray rows{"m_aiSpecialistCounts",4,{{0,{}},{7,{3}},{123,{9}},{7,{5}}}};
 auto changed=detail::bnw_specialist_known_hash_projection_math(rows,7,6);
 need(changed.rows.size()==4&&changed.rows[1].values.front()==3&&changed.rows[3].values.front()==6&&
  changed.rows[2].type_hash==123&&changed.rows[2].values.front()==9&&changed.rows[0].values.empty(),
  "Native known-hash last assignment lost original row multiplicity/unknown/null");
 changed=detail::bnw_specialist_known_hash_projection_math(rows,7,-1);
 need(changed.rows[1].values.front()==3&&changed.rows[3].values.front()==-1&&changed.rows[2].values.front()==9,
  "Signed count projection changed earlier duplicate/unknown instead of native last SET");
 changed=detail::bnw_specialist_known_hash_projection_math(rows,8,1);
 need(changed.wire_count==5&&changed.rows.size()==5&&changed.rows.back().type_hash==8&&
  changed.rows.back().values.front()==1,"Genuine known-hash projected row append differs");
 refuses([&]{(void)detail::bnw_specialist_known_hash_projection_math(rows,0,1);});
 rows.wire_count=3;refuses([&]{(void)detail::bnw_specialist_known_hash_projection_math(rows,7,6);});
 std::array<std::int32_t,6> yields{1,2,3,4,5,6};
 const auto focused=detail::bnw_citizen_plot_value_focus_food(yields,300,false,3);
 need(detail::bnw_specialist_total(focused,false,3,0,18,10)==48,
  "Source GP focus/food half/eight-term specialist sum differs");
 // Native individual additions reject overflow before a later cancelling term.
 yields={INT32_MAX,1,-1,0,0,0};refuses([&]{(void)detail::bnw_specialist_total(yields,true,3,0,0,0);});
 yields={INT32_MAX,0,0,0,0,0};refuses([&]{(void)detail::bnw_specialist_total(yields,true,3,0,1,-1);});
 // Zero small-city population affects only the native default/growth branch.
 yields={2,0,0,0,0,0};need(detail::bnw_specialist_total(yields,false,-1,1,0,0)==8,"Small city specialist food multiplier differs");
 std::cout<<"bnw specialist arithmetic tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
