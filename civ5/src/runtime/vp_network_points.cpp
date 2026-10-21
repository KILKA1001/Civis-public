#include "civ5/runtime/vp_network_points.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
// Release-5.4.2 CvSpyState wire values, private to this compatibility module.
constexpr std::int64_t gathering_intel=3,schmooze=7;
std::int64_t scaled(std::int64_t base,std::int64_t enabled,std::int64_t percent) {
    return enabled?wide::multiply_divide(base,percent,100):base;
}
std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    if(defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("VP network point define profile mismatch");
    const auto *entry=defines.find(name);
    if(!entry||!entry->lookup.error.empty())
        throw std::runtime_error("Unresolved VP network point define: "+std::string(name));
    const auto *value=std::get_if<std::int64_t>(&entry->lookup.value);
    if(!value)throw std::runtime_error("VP network point define is not an integer");
    return *value;
}
std::int64_t required(const std::optional<std::int64_t> &value,std::string_view name) {
    if(!value)throw std::runtime_error("Unresolved VP network point input: "+std::string(name));
    return *value;
}
}
VpNetworkPointRateRules load_vp_network_point_rate_rules(const content::GlobalDefineCache &defines) {
    VpNetworkPointRateRules rules;
    rules.base=define(defines,"ESPIONAGE_NP_BASE");
    rules.per_spy_rank=define(defines,"ESPIONAGE_NP_PER_SPY_RANK");
    rules.per_influence_level=define(defines,"ESPIONAGE_NP_CULTURAL_INFLUENCE");
    rules.per_technology_behind=define(defines,"ESPIONAGE_NP_PER_TECHNOLOGY_BEHIND");
    rules.max_technologies_behind=define(defines,"ESPIONAGE_NP_MAX_NUM_TECH");
    rules.reduction_per_security_point=define(defines,"ESPIONAGE_NP_REDUCTION_PER_SECURITY_POINT");
    rules.selected_rules_loaded=true;
    return rules;
}
VpNetworkPointRateQuote calculate_vp_network_point_rate(const VpNetworkPointRateInputs &inputs,
    const VpNetworkPointRateRules &rules) {
    // Source returns zero before touching any city/player rules in these cases.
    if(!inputs.balance_vp||!inputs.city_present||
       (inputs.spy_state!=gathering_intel&&inputs.spy_state!=schmooze))return {};
    if(!rules.selected_rules_loaded)
        throw std::runtime_error("VP network point rate requires selected GameCore rules");
    auto rate=rules.base;
    if(inputs.effective_spy_rank)
        rate=wide::add(rate,wide::multiply_divide(*inputs.effective_spy_rank,rules.per_spy_rank,1));
    const auto influence=required(inputs.influence_level,"influence level");
    rate=wide::add(rate,std::max<std::int64_t>(0,wide::multiply_divide(
        wide::subtract(influence,1),rules.per_influence_level,1)));
    const auto difference=wide::subtract(required(inputs.target_known_techs,"target technologies"),
        required(inputs.own_known_techs,"own technologies"));
    const auto behind=std::max<std::int64_t>(0,std::min(difference,rules.max_technologies_behind));
    rate=wide::add(rate,wide::multiply_divide(behind,rules.per_technology_behind,1));
    rate=wide::add(rate,required(inputs.player_bonus,"player bonus"));
    rate=wide::add(rate,required(inputs.religion_bonus,"religion bonus"));
    if(inputs.spy_state==gathering_intel&&inputs.effective_spy_rank) {
        if(!inputs.counterspy_resolved)
            throw std::runtime_error("Unresolved VP counterspy city lookup");
        if(inputs.counterspy&&inputs.counterspy->focus_reduction>0)
            rate=wide::subtract(rate,wide::multiply_divide(
                inputs.counterspy->focus_reduction,
                wide::add(inputs.counterspy->effective_rank,1),1));
    }
    const auto policy=required(inputs.policy_faster_modifier,"policy modifier");
    if(policy>0)rate=wide::multiply_divide(rate,wide::add(100,policy),100);
    const auto trait=required(inputs.trait_strength_modifier,"trait modifier");
    if(trait!=0)rate=wide::multiply_divide(rate,wide::add(100,trait),100);
    rate=std::max<std::int64_t>(0,rate);
    VpNetworkPointRateQuote quote;
    quote.before_security=rate;
    if(inputs.spy_state==gathering_intel) {
        const auto security=required(inputs.city_security,"city security");
        if(rate>0&&security>0) {
            quote.security_reduction_percent=wide::multiply_divide(
                security,rules.reduction_per_security_point,100);
            rate=wide::multiply_divide(rate,
                wide::subtract(100,quote.security_reduction_percent),100);
        }
    }
    quote.rate=rate;
    return quote;
}
VpNetworkPointRules load_vp_network_point_rules(const content::RulesDatabase &database,
    const ResearchContext &world) {
    if(world.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP network points require VP GameCore profile");
    VpNetworkPointRules rules;
    bool speed_found=false;
    const auto speed=database.query_rows({"SELECT TrainPercent FROM GameSpeeds WHERE ID=?",{world.speed}},
        [&](const auto &row) {
            if(speed_found)throw std::runtime_error("Duplicate selected VP game speed");
            speed_found=true;rules.train_percent=row.integer_at(0);
        });
    if(!speed.all_succeeded()||!speed_found||rules.train_percent<=0)
        throw std::runtime_error("Unresolved VP espionage game speed: "+speed.error);
    const auto passive=database.query_rows({
        "SELECT ID,NetworkPointsNeeded,NetworkPointsScaling,SciencePercentAdded,TilesRevealed,RevealCityScreen "
        "FROM SpyPassiveBonuses ORDER BY ID",{}},
        [&](const auto &row) {
            rules.passive.push_back({row.integer_at(0),
                scaled(row.integer_at(1),row.integer_at(2),rules.train_percent),
                row.integer_at(3),row.integer_at(4),row.integer_at(5)!=0});
        });
    if(!passive.all_succeeded())
        throw std::runtime_error("Unresolved VP passive spy bonuses: "+passive.error);
    const auto missions=database.query_rows({
        "SELECT ID,NetworkPointsNeeded,NetworkPointsScaling FROM CityEventChoices "
        "WHERE IsEspionageMission != 0 ORDER BY ID",{}},
        [&](const auto &row) {
            rules.missions.push_back({row.integer_at(0),
                scaled(row.integer_at(1),row.integer_at(2),rules.train_percent)});
        });
    if(!missions.all_succeeded())
        throw std::runtime_error("Unresolved VP spy mission thresholds: "+missions.error);
    rules.selected_rules_loaded=true;
    return rules;
}
VpNetworkPointPreview preview_vp_network_points(const VpCityEspionageState &city,
    std::int64_t player,std::int64_t rate,const VpNetworkPointRules &rules) {
    if(!rules.selected_rules_loaded||rate<=0)
        throw std::runtime_error("VP network points require selected rules and a positive rate");
    VpNetworkPointPreview quote;
    quote.before=city.value("m_aiAmount",player);
    quote.max_before=city.value("m_aiMaxAmount",player);
    if(quote.before<0||quote.max_before<0)
        throw std::runtime_error("VP network points require initialized city activity");
    const auto next=city.next_passive_bonus.find(player);
    if(next==city.next_passive_bonus.end())
        throw std::runtime_error("Missing VP next passive bonus state");
    quote.rate=rate;
    quote.after=wide::add(quote.before,rate);
    quote.max_after=std::max(quote.max_before,quote.after);
    quote.next_passive_bonus=next->second.threshold;
    if(!next->second.initialized) {
        for(const auto &bonus:rules.passive)
            if(bonus.threshold>quote.max_before&&
               (!quote.next_passive_bonus||bonus.threshold<*quote.next_passive_bonus))
                quote.next_passive_bonus=bonus.threshold;
    }
    if(quote.next_passive_bonus&&quote.max_after>=*quote.next_passive_bonus) {
        quote.next_passive_bonus.reset();
        for(const auto &bonus:rules.passive) {
            if(bonus.threshold>quote.max_after) {
                if(!quote.next_passive_bonus||bonus.threshold<*quote.next_passive_bonus)
                    quote.next_passive_bonus=bonus.threshold;
            } else if(bonus.threshold>quote.max_before)
                quote.passive_bonuses_unlocked.push_back(bonus.id);
        }
    }
    for(const auto &mission:rules.missions)
        if(mission.threshold>quote.before&&mission.threshold<=quote.after)
            quote.missions_unlocked.push_back(mission.id);
    quote.safe_without_effect_host=quote.passive_bonuses_unlocked.empty()&&
        quote.missions_unlocked.empty();
    return quote;
}
VpNetworkPointPreview apply_vp_network_points_without_effect_host(VpCityEspionageState &city,
    std::int64_t player,std::int64_t rate,const VpNetworkPointRules &rules) {
    const auto quote=preview_vp_network_points(city,player,rate,rules);
    if(!quote.safe_without_effect_host)
        throw std::runtime_error("VP network points cross a bonus or mission requiring world effects");
    process_vp_city_spy_activity(city,player,rate);
    city.next_passive_bonus.at(player)={true,quote.next_passive_bonus};
    return quote;
}
VpDiplomatNetworkPointRules load_vp_diplomat_network_point_rules(
    const content::RulesDatabase &database,const ResearchContext &world) {
    if(world.profile!=content::TechnologyProfile::vp542)
        throw std::runtime_error("VP diplomat network points require VP GameCore profile");
    std::optional<std::int64_t> train_percent;
    const auto speed=database.query_rows({"SELECT TrainPercent FROM GameSpeeds WHERE ID=?",
        {world.speed}},[&](const auto &row) {
            if(train_percent)throw std::runtime_error("Duplicate selected VP game speed");
            train_percent=row.integer_at(0);
        });
    if(!speed.all_succeeded()||!train_percent||*train_percent<=0)
        throw std::runtime_error("Unresolved VP diplomat game speed: "+speed.error);
    VpDiplomatNetworkPointRules rules;
    const auto passive=database.query_rows({
        "SELECT ID,NetworkPointsNeeded,NetworkPointsScaling "
        "FROM SpyPassiveBonusesDiplomat ORDER BY ID",{}},[&](const auto &row) {
            rules.passive.push_back({row.integer_at(0),
                scaled(row.integer_at(1),row.integer_at(2),*train_percent)});
        });
    if(!passive.all_succeeded())
        throw std::runtime_error("Unresolved VP diplomat passive bonuses: "+passive.error);
    rules.selected_rules_loaded=true;
    return rules;
}
VpDiplomatNetworkPointPreview preview_vp_diplomat_network_points(
    const VpCityEspionageState &city,std::int64_t player,std::int64_t rate,
    const VpDiplomatNetworkPointRules &rules) {
    if(!rules.selected_rules_loaded||rate<=0)
        throw std::runtime_error("VP diplomat points require selected rules and positive rate");
    VpDiplomatNetworkPointPreview quote;
    quote.before=city.value("m_aiAmount",player);
    quote.max_before=city.value("m_aiMaxAmount",player);
    if(quote.before<0||quote.max_before<0)
        throw std::runtime_error("VP diplomat points require initialized city activity");
    const auto next=city.next_passive_bonus.find(player);
    if(next==city.next_passive_bonus.end())
        throw std::runtime_error("Missing VP diplomat next passive bonus state");
    quote.rate=rate;
    quote.after=wide::add(quote.before,rate);
    quote.max_after=std::max(quote.max_before,quote.after);
    quote.next_passive_bonus=next->second.threshold;
    if(!next->second.initialized)
        for(const auto &bonus:rules.passive)
            if(bonus.threshold>quote.before&&
               (!quote.next_passive_bonus||bonus.threshold<*quote.next_passive_bonus))
                quote.next_passive_bonus=bonus.threshold;
    if(quote.next_passive_bonus&&quote.max_after>=*quote.next_passive_bonus) {
        quote.next_passive_bonus.reset();
        for(const auto &bonus:rules.passive) {
            if(bonus.threshold>quote.max_after) {
                if(!quote.next_passive_bonus||bonus.threshold<*quote.next_passive_bonus)
                    quote.next_passive_bonus=bonus.threshold;
            } else if(bonus.threshold>quote.before)
                quote.passive_bonuses_unlocked.push_back(bonus.id);
        }
    }
    quote.safe_without_effect_host=quote.passive_bonuses_unlocked.empty();
    return quote;
}
VpDiplomatNetworkPointPreview apply_vp_diplomat_network_points_without_effect_host(
    VpCityEspionageState &city,std::int64_t player,std::int64_t rate,
    const VpDiplomatNetworkPointRules &rules) {
    const auto quote=preview_vp_diplomat_network_points(city,player,rate,rules);
    if(!quote.safe_without_effect_host)
        throw std::runtime_error("VP diplomat points cross a passive bonus requiring world effects");
    (void)city.value("m_aiRate",player);
    (void)city.value("m_aiLastProgress",player);
    process_vp_city_spy_activity(city,player,rate);
    city.next_passive_bonus.at(player)={true,quote.next_passive_bonus};
    return quote;
}
} // namespace civ5::runtime
