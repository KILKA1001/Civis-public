#include "civ5/runtime/bnw_city_religion_yield_owner.hpp"
#include <array>
#include <cassert>
#include <climits>
#include <iostream>
#include <type_traits>
using namespace civ5::runtime;
template<class T> concept PublicReligionIssuer=requires {T::after_city_yield_child();};
template<class T> concept PublicReligionAdvancement=requires(T& owner) {owner.prepare_next();};
static_assert(!std::is_default_constructible_v<BnwCityReligionYieldOwner>);
static_assert(!std::is_copy_constructible_v<BnwCityReligionYieldOwner>);
static_assert(!PublicReligionIssuer<BnwCityReligionYieldOwner>);
static_assert(!PublicReligionAdvancement<BnwCityReligionYieldOwner>);
template<class F>void refuses(F&& call){bool caught=false;try{call();}catch(const BnwCityReligionYieldUnavailable&){caught=true;}assert(caught);}
int main(){
 const std::array rows{CityYieldReligion{0,6,0},CityYieldReligion{-1,1,0},CityYieldReligion{1,2,0},CityYieldReligion{1,3,0}};
 // Counts native records, not distinct IDs; pantheon/nonfollowers excluded.
 assert(detail::bnw_city_religion_centi_math(rows,125)==2);
 assert(detail::bnw_city_religion_centi_math(rows,50)==1);
 assert(detail::bnw_city_religion_centi_math(rows,1)==0);
 const std::array invalid_unreached{CityYieldReligion{INT64_MAX,INT64_MAX,0}};
 assert(detail::bnw_city_religion_centi_math(invalid_unreached,0)==0);
 assert(detail::bnw_city_religion_centi_math(invalid_unreached,-1)==0);
 refuses([&]{(void)detail::bnw_city_religion_centi_math(rows,INT32_MAX);});
 const std::array zero_followers{CityYieldReligion{INT64_MAX,0,0}};
 assert(detail::bnw_city_religion_centi_math(zero_followers,100)==0);
 const std::array<std::int32_t,3> ordinary{4,-2,7};
 assert(detail::bnw_city_religion_sum_math(ordinary)==9);
 const std::array<std::int32_t,3> reached_overflow{INT32_MAX,1,-1};
 refuses([&]{(void)detail::bnw_city_religion_sum_math(reached_overflow);});
 const std::array<std::int32_t,2> reached_underflow{INT32_MIN,-1};
 refuses([&]{(void)detail::bnw_city_religion_sum_math(reached_underflow);});
 std::cout<<"Prepared city religion arithmetic/API tests; genuine child/owner/writes UNOBSERVED\n";
}
