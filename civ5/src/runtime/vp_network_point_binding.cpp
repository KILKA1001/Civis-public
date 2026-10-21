#include "civ5/runtime/vp_network_point_binding.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/saves/vp_game_trade_profile.hpp"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr std::int64_t gathering_intel=3,counter_intel=5,schmooze=7;
std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    if(defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("VP spy rate define profile mismatch");
    const auto *entry=defines.find(name);
    if(!entry||!entry->lookup.error.empty())
        throw std::runtime_error("Unresolved VP spy rate define: "+std::string(name));
    const auto *value=std::get_if<std::int64_t>(&entry->lookup.value);
    if(!value)throw std::runtime_error("Invalid VP spy rate integer define");
    return *value;
}
std::int64_t selected_integer(const content::RulesDatabase &db,const char *sql,std::int64_t id) {
    std::optional<std::int64_t> result;
    const auto query=db.query_rows({sql,{id}},[&](const auto &row) {
        if(result)throw std::runtime_error("Duplicate VP spy rate selected rule");
        result=row.integer_at(0);
    });
    if(!query.all_succeeded()||!result)
        throw std::runtime_error("Unresolved VP spy rate selected rule: "+query.error);
    return *result;
}
const TeamTechnologies &team_techs(const MaintenanceWorld &world,std::int64_t team) {
    const TeamTechnologies *found=nullptr;
    for(const auto &entry:world.technologies)if(entry.team()==team) {
        if(found)throw std::runtime_error("Duplicate VP spy rate team technologies");
        found=&entry;
    }
    if(!found)throw std::runtime_error("Missing VP spy rate team technologies");
    return *found;
}
std::int64_t influence_level(const VpEspionagePlayer &spy_player,
    const MaintenanceWorld &world,const content::GlobalDefineCache &defines,
    std::int64_t spy_owner,std::int64_t target_owner) {
    if(spy_owner==target_owner)return -1;
    const auto spy_team=world.setup.players.at(spy_owner).team;
    const auto target_team=world.setup.players.at(target_owner).team;
    if(!world.diplomacy.teams.at(target_team).met.contains(spy_team))return -1;
    const auto influence=spy_player.cultural_influence_times100.find(target_owner);
    if(influence==spy_player.cultural_influence_times100.end())
        throw std::runtime_error("Missing live VP culture influence target");
    const auto *history=find_vp_culture_history_profile(world.setup);
    if(!history||!history->players.contains(target_owner))
        throw std::runtime_error("Missing VP target lifetime culture");
    const auto lifetime=history->players.at(target_owner).lifetime_times100;
    const auto percent=lifetime>0?wide::multiply_divide(
        influence->second,100,lifetime):0;
    if(percent>=define(defines,"CULTURE_LEVEL_DOMINANT"))return 5;
    if(percent>=define(defines,"CULTURE_LEVEL_INFLUENTIAL"))return 4;
    if(percent>=define(defines,"CULTURE_LEVEL_POPULAR"))return 3;
    if(percent>=define(defines,"CULTURE_LEVEL_FAMILIAR"))return 2;
    if(percent>=define(defines,"CULTURE_LEVEL_EXOTIC"))return 1;
    return 0;
}
std::int64_t religion_bonus(const TreasuryContext &treasury,const TreasuryRules &rules,
    const MaintenanceWorld &world,const content::RulesDatabase &database,
    const content::CustomModOptions &options,std::int64_t player) {
    const auto religion_id=vp_treasury_religion_profile(treasury.players.at(player)).state_religion;
    // GetReligion(religion, NO_PLAYER) never selects a player's pantheon.
    if(religion_id<=0)return 0;
    const TreasuryReligion *religion=nullptr;
    for(const auto &entry:treasury.religions)
        if(entry.religion==religion_id&&!entry.pantheon) {religion=&entry;break;}
    if(!religion)return 0;
    const auto belief_religion_id=religion->belief_religion;
    const TreasuryReligion *belief_religion=nullptr;
    for(const auto &entry:treasury.religions)
        if(entry.religion==belief_religion_id&&!entry.pantheon) {belief_religion=&entry;break;}
    const auto holy=religion->holy_city;
    const auto holy_owner=holy?holy->first:-1;
    const auto majority=holy?vp_treasury_minor_profile(treasury.players.at(holy_owner))
        .city_majorities.at(holy->second):-1;
    const bool founder_eligible=belief_religion&&belief_religion->holy_plot_owner&&
        *belief_religion->holy_plot_owner==player;
    std::int64_t result=0;
    for(const auto belief:religion->beliefs) {
        const auto amount=selected_integer(database,
            "SELECT EspionageNetworkPoints FROM Beliefs WHERE ID=?",belief);
        if(!amount)continue;
        const auto rule=rules.beliefs.find(belief);
        if(rule==rules.beliefs.end())throw std::runtime_error("Unresolved VP spy rate belief");
        const auto &b=rule->second;
        if(belief_religion_id!=-1&&(b.founder||b.enhancer)&&
           (!founder_eligible||(holy&&holy_owner!=player)))continue;
        if(b.follower&&holy&&(majority!=belief_religion_id||holy_owner!=player))continue;
        if(b.reformation&&((holy&&majority!=belief_religion_id)||
           (!founder_eligible&&vp_treasury_religion_profile(treasury.players.at(player))
               .state_religion!=belief_religion_id)))continue;
        if(b.pantheon&&holy) {
            bool merged=false;
            if(majority!=belief_religion_id)
                for(const auto &entry:treasury.religions)
                    if(entry.pantheon&&entry.founder==player&&
                       entry.belief_lookup.contains(belief)&&belief_religion&&
                       belief_religion->belief_lookup.contains(belief)) {
                        merged=true;break;
                    }
            if((majority!=belief_religion_id&&!merged)||holy_owner!=player)continue;
        }
        if(options.enabled("BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV")&&
           b.civilization!=-1&&b.civilization!=world.cities.players.at(player).civilization)continue;
        result=wide::add(result,amount);
    }
    return result;
}
struct CounterspyInfo {std::optional<VpCounterspyNetworkReduction> reduction;std::int64_t security=0;};
CounterspyInfo counterspy(const MaintenanceWorld &world,const content::RulesDatabase &database,
    const VpCityEspionageState &activity,std::int64_t city_owner) {
    CounterspyInfo result;
    if(!world.diplomacy.major_players.contains(city_owner))return result;
    const auto assigned=activity.value("m_aiSpyAssignment",city_owner);
    if(assigned==-1)return result;
    const auto *profile=find_vp_espionage_profile(world.setup);
    if(!profile||!profile->players.contains(city_owner))
        throw std::runtime_error("Missing VP counterspy player");
    const auto &spies=profile->players.at(city_owner).spies;
    if(assigned<0||static_cast<std::uint64_t>(assigned)>=spies.size())
        throw std::runtime_error("Invalid VP counterspy assignment");
    const auto &spy=spies.at(static_cast<std::size_t>(assigned));
    if(spy.state!=counter_intel||spy.focus==-1)return result;
    std::optional<std::pair<std::int64_t,std::int64_t>> modifiers;
    const auto query=database.query_rows({
        "SELECT CounterspyNPRateReduction,SpySecurityModifier FROM CityEventChoices WHERE ID=?",
        {spy.focus}},[&](const auto &row) {
            if(modifiers)throw std::runtime_error("Duplicate VP counterspy focus");
            modifiers={{row.integer_at(0),row.integer_at(1)}};
        });
    if(!query.all_succeeded()||!modifiers)
        throw std::runtime_error("Unresolved VP counterspy focus: "+query.error);
    const auto &owner=profile->players.at(city_owner);
    const auto rank=wide::add(spy.rank,owner.extra_rank_bonus);
    result.reduction=VpCounterspyNetworkReduction{modifiers->first,rank};
    result.security=modifiers->second;
    return result;
}
std::int64_t city_security(const MaintenanceWorld &world,const TreasuryContext &treasury,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const VpCityEspionageState &activity,
    std::int64_t owner,std::int64_t city_id,
    const CounterspyInfo &counter) {
    if(!world.diplomacy.major_players.contains(owner))return -1;
    const auto &live=world.cities.players.at(owner).cities.at(city_id);
    const auto *profile=find_vp_espionage_profile(world.setup);
    if(!profile||!profile->players.contains(owner))
        throw std::runtime_error("Missing live VP city owner espionage state");
    const auto add=wide::add;
    const auto mul=[](std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);};
    auto security=define(defines,"ESPIONAGE_SECURITY_BASE");
    security=add(security,selected_integer(database,
        "SELECT SpySecurityModifier FROM HandicapInfos WHERE ID=?",
        world.setup.players.at(owner).handicap));
    if(!world.setup.is_human(world.setup.players.at(owner)))
        security=add(security,selected_integer(database,
            "SELECT SpySecurityModifier FROM HandicapInfos WHERE ID=?",world.setup.game_handicap));
    const auto not_all=define(defines,"ESPIONAGE_SECURITY_NOT_ALL_HAVE_SPIES");
    if(not_all) {
        const auto *profile=find_vp_espionage_profile(world.setup);
        if(!profile)throw std::runtime_error("Missing VP espionage roster for city security");
        for(const auto player:world.diplomacy.major_players)
            if(world.setup.players.at(player).alive&&profile->players.at(player).spies.empty()) {
                security=add(security,not_all);break;
            }
    }
    security=add(security,activity.security_modifier);
    const auto scale=define(defines,"ESPIONAGE_SECURITY_PER_POPULATION_BUILDING_SCALER");
    if(scale)security=add(security,wide::multiply_divide(live.population,
        activity.security_per_x_pop,scale));
    security=add(security,profile->players.at(owner).security_modifier);
    const auto previous=define(defines,"ESPIONAGE_SECURITY_PREVIOUS_CITY_MISSIONS");
    if(previous) {
        const auto speed=selected_integer(database,
            "SELECT SpyRatePercent FROM GameSpeeds WHERE ID=?",world.setup.speed);
        security=add(security,wide::multiply_divide(mul(
            activity.previous_spy_missions,previous),100,speed));
    }
    security=add(security,counter.security);
    security=add(security,mul(live.population,define(defines,"ESPIONAGE_SECURITY_PER_POPULATION")));
    std::int64_t routes=0;
    for(const auto &route:treasury.trade_routes) {
        const auto *vp_route=saves::find_vp_trade_route(route);
        if(!vp_route)throw std::runtime_error("Missing VP route identity for city security");
        if(vp_route->origin_id==-1||vp_route->destination_id==-1||
           route.origin_owner==-1||route.destination_owner==-1)continue;
        if((route.origin_owner==owner&&vp_route->origin_id==city_id)||
           (route.destination_owner==owner&&vp_route->destination_id==city_id))
            routes=add(routes,1);
        if(route.origin_owner==owner&&route.destination_owner==owner&&
           vp_route->origin_id==city_id&&vp_route->destination_id==city_id)
            routes=add(routes,1);
    }
    security=add(security,mul(routes,define(defines,"ESPIONAGE_SECURITY_PER_TRADE_ROUTE")));
    const auto happiness=activity.happiness_delta;
    if(happiness<0)security=add(security,mul(wide::subtract(0,happiness),
        define(defines,"ESPIONAGE_SECURITY_PER_EXCESS_UNHAPPINESS")));
    return std::max<std::int64_t>(0,std::min(define(defines,"ESPIONAGE_MAX_NUM_SECURITY_POINTS"),security));
}
} // namespace

VpNetworkPointRateInputs bind_vp_network_point_rate_inputs(
    const MaintenanceWorld &world,
    const TreasuryContext &treasury,const TreasuryRules &treasury_rules,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,std::int64_t spy_owner,
    std::size_t spy_index,const VpSpyCityBinding &target) {
    if(world.setup.profile!=vp||world.diplomacy.profile!=vp||world.cities.profile!=vp||
       treasury.profile!=vp||treasury_rules.profile!=vp||
       defines.profile()!=content::GlobalDefineProfile::vp542||!options.all_succeeded())
        throw std::runtime_error("VP spy rate needs one selected VP world and rules");
    VpNetworkPointRateInputs inputs;
    inputs.balance_vp=options.enabled("BALANCE_VP");
    if(!inputs.balance_vp)return inputs;
    const auto *profile=find_vp_espionage_profile(world.setup);
    if(!profile||!profile->players.contains(spy_owner)||
       spy_index>=profile->players.at(spy_owner).spies.size())
        throw std::runtime_error("Missing live VP spy for network rate");
    const auto &player=profile->players.at(spy_owner);
    const auto &spy=player.spies.at(spy_index);
    inputs.spy_state=spy.state;
    const auto binding=vp_spy_city_binding(world.cities,spy_owner,spy_index,spy.city_x,spy.city_y);
    if(!binding||binding->owner!=target.owner||binding->city!=target.city)
        throw std::runtime_error("VP spy rate target is not the live bound city");
    inputs.city_present=true;
    if(spy.state!=gathering_intel&&spy.state!=schmooze)return inputs;
    inputs.effective_spy_rank=wide::add(spy.rank,player.extra_rank_bonus);
    inputs.influence_level=influence_level(player,world,defines,spy_owner,target.owner);
    inputs.own_known_techs=team_techs(world,world.setup.players.at(spy_owner).team).known_count();
    inputs.target_known_techs=team_techs(world,world.setup.players.at(target.owner).team).known_count();
    inputs.player_bonus=player.network_points_bonus;
    inputs.religion_bonus=religion_bonus(treasury,treasury_rules,world,database,options,spy_owner);
    inputs.policy_faster_modifier=player.policy_faster_modifier;
    inputs.trait_strength_modifier=player.offensive_strength_modifier;
    if(spy.state==gathering_intel) {
        const auto *activity=find_vp_city_espionage(world.cities,target.owner,target.city);
        if(!activity)throw std::runtime_error("Missing VP target city espionage state");
        const auto counter=counterspy(world,database,*activity,target.owner);
        inputs.counterspy_resolved=true;
        inputs.counterspy=counter.reduction;
        inputs.city_security=city_security(world,treasury,database,defines,
            *activity,target.owner,target.city,counter);
    }
    return inputs;
}
} // namespace civ5::runtime
