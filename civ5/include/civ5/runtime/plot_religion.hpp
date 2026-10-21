#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"

namespace civ5::runtime {
// BNW plot helpers share this selection. Majority bonuses scan each Info once;
// the secondary pantheon is the first matching belief in saved religion order.
// A belief may therefore contribute once as majority and again as secondary.
template<class Beliefs>
std::vector<const typename Beliefs::mapped_type*> bnw_plot_beliefs(
    const CityYieldContext &context,const CityYieldState &city,const Beliefs &infos){
    if(context.profile!=content::TechnologyProfile::bnw)throw std::runtime_error("BNW plot religion profile mismatch");
    std::vector<const typename Beliefs::mapped_type*> result;
    std::int64_t total=0,most=-1,pressure=0,major=-1;
    for(const auto &r:city.religions){total=wide::add(total,r.followers);
        if(r.followers>most||(r.followers==most&&r.pressure>pressure)){most=r.followers;pressure=r.pressure;major=r.religion;}}
    if(major==-1||wide::multiply_divide(most,2,1)<total)return result;
    const auto religion=[&](std::int64_t id)->const CityYieldReligionBeliefs*{
        for(const auto &r:context.religions)if(r.religion==id&&(id!=context.pantheon||r.founder==city.owner))return &r;return nullptr;};
    const auto *r=religion(major);if(!r)return result;
    for(const auto id:std::set<std::int64_t>(r->beliefs.begin(),r->beliefs.end())){
        const auto found=infos.find(id);if(found!=infos.end())result.push_back(&found->second);}
    const auto &second=context.players.at(city.owner).second_pantheon_count;
    if(!second)throw std::runtime_error("Unresolved BNW secondary pantheon counter");
    if(*second>0){
        std::int64_t secondary=-1;most=-1;pressure=-1;
        for(const auto &s:city.religions)if(s.religion!=major&&s.religion>context.pantheon&&s.followers>0&&
            (s.followers>most||(s.followers==most&&s.pressure>pressure))){secondary=s.religion;most=s.followers;pressure=s.pressure;}
        if(secondary!=-1)if(const auto *s=religion(secondary))for(const auto id:s->beliefs){
            const auto found=infos.find(id);if(found!=infos.end()&&found->second.pantheon){result.push_back(&found->second);break;}}
    }
    return result;
}
} // namespace civ5::runtime
