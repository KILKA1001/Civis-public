#include "civ5/runtime/vp_gathering_intel_turn.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    if(defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("VP gathering-intel define profile mismatch");
    const auto *entry=defines.find(name);
    if(!entry||!entry->lookup.error.empty())
        throw std::runtime_error("Unresolved VP gathering-intel define: "+std::string(name));
    const auto *value=std::get_if<std::int64_t>(&entry->lookup.value);
    if(!value)throw std::runtime_error("VP gathering-intel define is not an integer");
    return *value;
}
bool can_discover_wonder(const CityYieldState &city,const content::RulesDatabase &database) {
    if(!city.production_orders)
        throw std::runtime_error("VP wonder discovery needs the live production queue");
    if(city.production_orders->empty())return false;
    const auto &order=city.production_orders->front();
    if(order.type!=1&&order.type!=2)return false;
    std::optional<std::int64_t> property;
    const auto query=database.query_rows({order.type==1?
        "SELECT BuildingClasses.MaxGlobalInstances FROM Buildings "
        "JOIN BuildingClasses ON BuildingClasses.Type=Buildings.BuildingClass "
        "WHERE Buildings.ID=?":
        "SELECT IsRepeatable FROM Projects WHERE ID=?",{order.item}},
        [&](const auto &row) {
            if(property)throw std::runtime_error("Ambiguous VP wonder production Info");
            property=row.integer_at(0);
        });
    if(!query.all_succeeded()||!property)
        throw std::runtime_error("Unresolved VP wonder production Info: "+query.error);
    return order.type==1?*property>=0:*property==0;
}
}
VpGatheringIntelStep process_vp_gathering_intel_without_effect_host(
    const MaintenanceWorld &world,CityYieldContext &cities,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t spy_owner,std::size_t spy_index) {
    constexpr auto vp=content::TechnologyProfile::vp542;
    if(world.setup.profile!=vp||world.diplomacy.profile!=vp||cities.profile!=vp||
       &world.cities!=&cities||!options.all_succeeded()||!options.enabled("BALANCE_VP"))
        throw std::runtime_error("VP gathering intel requires one live VP world and enabled rules");
    const auto *profile=find_vp_espionage_profile(world.setup);
    if(!profile||!profile->players.contains(spy_owner)||
       spy_index>=profile->players.at(spy_owner).spies.size())
        throw std::runtime_error("Unresolved VP gathering-intel spy");
    const auto &spy=profile->players.at(spy_owner).spies.at(spy_index);
    if(spy.state!=3)throw std::runtime_error("VP spy is not gathering intel");
    const auto target=vp_spy_city_binding(cities,spy_owner,spy_index,spy.city_x,spy.city_y);
    if(!target)throw std::runtime_error("VP gathering-intel city is absent");
    const auto &live_city=cities.players.at(target->owner).cities.at(target->city);
    const auto *activity=find_vp_city_espionage(cities,target->owner,target->city);
    if(!activity)throw std::runtime_error("Missing VP gathering-intel city profile");
    const auto reveal=activity->value("m_abRevealCityScreen",spy_owner);
    if(reveal!=0&&reveal!=1)
        throw std::runtime_error("Invalid VP city-screen reveal state");
    if(target->owner!=spy_owner&&reveal&&can_discover_wonder(live_city,database))
        throw std::runtime_error("VP wonder discovery requires intrigue and notification hosts");
    if(define(defines,"ESPIONAGE_XP_PER_TURN_OFFENSIVE")!=0)
        throw std::runtime_error("VP spy experience requires promotion and notification hosts");
    const auto inputs=bind_vp_network_point_rate_inputs(world,treasury,treasury_rules,
        database,defines,options,spy_owner,spy_index,*target);
    VpGatheringIntelStep result;
    result.spy_owner=spy_owner;result.spy_index=spy_index;
    result.target_owner=target->owner;result.city=target->city;
    result.rate=calculate_vp_network_point_rate(inputs,load_vp_network_point_rate_rules(defines));
    if(result.rate.rate>0) {
        const auto rules=load_vp_network_point_rules(database,world.setup);
        const auto preview=preview_vp_network_points(*activity,spy_owner,result.rate.rate,rules);
        if(!preview.safe_without_effect_host)
            throw std::runtime_error("VP spy network points require passive or mission effect hosts");
        (void)activity->value("m_aiRate",spy_owner);
        (void)activity->value("m_aiLastProgress",spy_owner);
        result.network_points=apply_vp_network_points_without_effect_host(
            ensure_vp_city_espionage(cities,target->owner,target->city),
            spy_owner,result.rate.rate,rules);
    }
    return result;
}
} // namespace civ5::runtime
