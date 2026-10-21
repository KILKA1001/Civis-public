#include "civ5/runtime/vp_schmooze_turn.hpp"
#include "civ5/runtime/vp_attack_operations.hpp"
#include "civ5/runtime/vp_army_construction.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    if(defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("VP diplomat define profile mismatch");
    const auto *entry=defines.find(name);
    if(!entry||!entry->lookup.error.empty())
        throw std::runtime_error("Unresolved VP diplomat define: "+std::string(name));
    const auto *value=std::get_if<std::int64_t>(&entry->lookup.value);
    if(!value)throw std::runtime_error("VP diplomat define is not an integer");
    return *value;
}
}
VpSchmoozeStep process_vp_schmooze_without_effect_host(
    const MaintenanceWorld &world,CityYieldContext &cities,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t spy_owner,std::size_t spy_index) {
    constexpr auto vp=content::TechnologyProfile::vp542;
    if(world.setup.profile!=vp||world.diplomacy.profile!=vp||cities.profile!=vp||
       &world.cities!=&cities||!options.all_succeeded()||!options.enabled("BALANCE_VP"))
        throw std::runtime_error("VP diplomat requires one live VP world and enabled rules");
    const auto *profile=find_vp_espionage_profile(world.setup);
    if(!profile||!profile->players.contains(spy_owner)||
       spy_index>=profile->players.at(spy_owner).spies.size())
        throw std::runtime_error("Unresolved VP diplomat spy");
    const auto &spy=profile->players.at(spy_owner).spies.at(spy_index);
    if(spy.state!=7)throw std::runtime_error("VP spy is not schmoozing");
    const auto target=vp_spy_city_binding(cities,spy_owner,spy_index,spy.city_x,spy.city_y);
    if(!target)throw std::runtime_error("VP diplomat city is absent");
    const auto *activity=find_vp_city_espionage(cities,target->owner,target->city);
    if(!activity)throw std::runtime_error("Missing VP diplomat city profile");
    const auto intrigue=activity->value("m_abDiplomatReceiveIntrigues",spy_owner);
    if(intrigue!=0&&intrigue!=1)
        throw std::runtime_error("Invalid VP diplomat intrigue state");
    const auto inputs=bind_vp_network_point_rate_inputs(world,treasury,treasury_rules,
        database,defines,options,spy_owner,spy_index,*target);
    VpSchmoozeStep result;
    result.spy_owner=spy_owner;result.spy_index=spy_index;
    result.target_owner=target->owner;result.city=target->city;
    result.rate=calculate_vp_network_point_rate(inputs,load_vp_network_point_rate_rules(defines));
    if(result.rate.rate>0) {
        const auto rules=load_vp_diplomat_network_point_rules(database,world.setup);
        const auto preview=preview_vp_diplomat_network_points(
            *activity,spy_owner,result.rate.rate,rules);
        if(!preview.safe_without_effect_host)
            throw std::runtime_error("VP diplomat passive bonus requires world effect hosts");
        result.network_points=apply_vp_diplomat_network_points_without_effect_host(
            ensure_vp_city_espionage(cities,target->owner,target->city),
            spy_owner,result.rate.rate,rules);
    }
    if(define(defines,"ESPIONAGE_XP_PER_TURN_DIPLOMAT")!=0)
        result.unresolved="VP diplomat experience requires promotion host";
    else if(intrigue) {
        const auto *operations=find_vp_attack_operations_profile(world.setup);
        const auto *armies=find_vp_army_construction_profile(world.setup);
        const auto owner=world.setup.players.find(target->owner);
        if(owner==world.setup.players.end())
            throw std::runtime_error("Unresolved VP diplomat city owner");
        // CvPlayer::isHuman(ISHUMAN_AI_UNITS) delegates to pregame human status
        // unless VP's AI_CONTROL_UNITS option hands units to the AI.
        result.target_human_for_units=world.setup.is_human(owner->second)&&
            !options.enabled("AI_CONTROL_UNITS");
        result.military_might_branch_skipped=!*result.target_human_for_units;
        if(armies&&armies->players.contains(target->owner)) {
            result.target_army_count=armies->players.at(target->owner).size();
            result.building_land_army=vp_is_building_army(world.setup,target->owner,VpArmyType::land);
            result.building_naval_army=vp_is_building_army(world.setup,target->owner,VpArmyType::naval);
            result.building_combined_army=vp_is_building_army(world.setup,target->owner,VpArmyType::combined);
            if(!*result.target_human_for_units) {
                if(*result.building_land_army)
                    result.building_army_intrigue_candidate="land";
                else if(*result.building_naval_army||*result.building_combined_army)
                    result.building_army_intrigue_candidate="amphibious";
            }
            if(!result.building_army_intrigue_candidate.empty())
                result.building_army_intrigue_duplicate=vp_recent_building_army_intrigue(
                    world.setup,spy_owner,target->owner,
                    result.building_army_intrigue_candidate=="amphibious");
        }
        if(!operations||!operations->players.contains(spy_owner))
            result.unresolved="VP diplomat intrigue requires attack operation profile";
        else if(operations->players.at(spy_owner).empty()) {
            result.no_sneak_attack_operations=true;
            if(!armies||!armies->players.contains(target->owner))
                result.unresolved="VP diplomat intrigue requires army construction profile";
            else if(result.building_army_intrigue_duplicate==false)
                result.unresolved="VP building-army intrigue requires AddIntrigueMessage host";
            else if(!result.military_might_branch_skipped)
                result.unresolved="VP diplomat intrigue requires military history and notification hosts";
            else if(!find_vp_city_intrigue_order_profile(cities))
                result.unresolved="VP diplomat intrigue requires city order profile";
            else {
                result.construction_scan=vp_first_city_construction_intrigue(cities,database,target->owner);
                if(result.construction_scan->candidate)
                    result.unresolved="VP city construction intrigue requires AddIntrigueMessage host";
                else if(!find_vp_diplomacy_intrigue_profile(world.setup))
                    result.unresolved="VP diplomat intrigue requires diplomacy profile";
                else {
                    result.diplomatic_scan=vp_scan_diplomatic_intrigue(world.setup,world.diplomacy,
                        options,spy_owner,target->owner,spy.city_x,spy.city_y);
                    if(result.diplomatic_scan->candidate) {
                        const auto &candidate=*result.diplomatic_scan->candidate;
                        result.diplomatic_intrigue_duplicate=vp_recent_diplomatic_intrigue(
                            world.setup,spy_owner,target->owner,candidate.reported_player,
                            candidate.kind==VpDiplomaticIntrigueKind::coop_war,spy.city_x,spy.city_y);
                        if(!*result.diplomatic_intrigue_duplicate)
                            result.unresolved="VP diplomatic intrigue requires AddIntrigueMessage host";
                    }
                }
            }
        } else
            result.unresolved="VP diplomat intrigue requires diplomacy, map and notification hosts";
    }
    return result;
}
} // namespace civ5::runtime
