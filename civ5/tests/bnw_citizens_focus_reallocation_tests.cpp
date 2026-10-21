#include "civ5/runtime/bnw_city_entry.hpp"
#include <climits>
#include <stdexcept>
using civ5::runtime::detail::bnw_citizens_forced_demotions;
void need(bool b){if(!b)throw std::runtime_error("forced-working native subtraction mismatch");}
template<class F> void refuses(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}need(rejected);}
int main(){
 need(bnw_citizens_forced_demotions(5,2)==3);
 need(bnw_citizens_forced_demotions(2,5)==-3);
 need(bnw_citizens_forced_demotions(INT32_MAX,INT32_MAX)==0);
 need(bnw_citizens_forced_demotions(INT32_MIN,INT32_MIN)==0);
 need(bnw_citizens_forced_demotions(0,INT32_MAX)==-INT32_MAX);
 refuses([]{(void)bnw_citizens_forced_demotions(INT32_MAX,-1);});
 refuses([]{(void)bnw_citizens_forced_demotions(INT32_MIN,1);});
 // Reached operand widths cannot be repaired by cancellation/final cast.
 refuses([]{(void)bnw_citizens_forced_demotions(static_cast<long long>(INT32_MAX)+1,static_cast<long long>(INT32_MAX)+1);});
 return 0;
}
