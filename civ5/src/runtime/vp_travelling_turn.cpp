#include "civ5/runtime/vp_travelling_turn.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
VpTravellingStep process_vp_travelling_without_arrival_host(
    const ResearchContext &setup,CityYieldContext &cities,
    std::int64_t spy_owner,std::size_t spy_index) {
    constexpr auto vp=content::TechnologyProfile::vp542;
    if(setup.profile!=vp||cities.profile!=vp)
        throw std::runtime_error("VP spy travel requires VP GameCore profile");
    const auto *profile=find_vp_espionage_profile(setup);
    if(!profile||!profile->players.contains(spy_owner)||
       spy_index>=profile->players.at(spy_owner).spies.size())
        throw std::runtime_error("Unresolved VP travelling spy");
    const auto &spy=profile->players.at(spy_owner).spies.at(spy_index);
    if(spy.state!=1)throw std::runtime_error("VP spy is not travelling");
    const auto target=vp_spy_city_binding(cities,spy_owner,spy_index,spy.city_x,spy.city_y);
    if(!target)throw std::runtime_error("VP travelling spy city is absent");
    if(target->amount<0||target->max_amount<0||target->rate<0||target->goal<0||
       target->max_amount<target->amount)
        throw std::runtime_error("VP travelling spy activity is not initialized");
    VpTravellingStep result;
    result.spy_owner=spy_owner;result.spy_index=spy_index;
    result.target_owner=target->owner;result.city=target->city;
    result.before=target->amount;result.max_before=target->max_amount;
    result.rate=target->rate;result.goal=target->goal;
    result.after=wide::add(result.before,result.rate);
    result.max_after=std::max(result.max_before,result.after);
    if(result.after>=result.goal) {
        result.arrival_pending=true;
        result.unresolved="VP spy arrival requires state transition and mission hosts";
        return result;
    }
    auto &activity=ensure_vp_city_espionage(cities,target->owner,target->city);
    activity.set("m_aiAmount",spy_owner,result.after);
    activity.set("m_aiMaxAmount",spy_owner,result.max_after);
    return result;
}
} // namespace civ5::runtime
