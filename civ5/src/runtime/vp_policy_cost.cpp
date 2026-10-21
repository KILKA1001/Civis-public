#include "civ5/runtime/vp_policy_cost.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
template<class T> const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name&&std::string_view(row.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP policy-cost field: "+std::string(name));
        found=&row;
    }
    if(!found)throw std::runtime_error("Missing VP policy-cost field: "+std::string(name));
    return *found;
}
std::int64_t scalar(const saves::PlayerCoreObservation &core,std::string_view name) {
    const auto &values=named(core.fields,name).values;
    if(values.size()!=1)throw std::runtime_error("VP policy-cost scalar width mismatch");
    return values.front();
}
std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    if(defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("VP policy-cost define profile mismatch");
    const auto *item=defines.find(name);
    if(!item||!item->lookup.error.empty())
        throw std::runtime_error("Unresolved VP policy-cost define: "+std::string(name));
    const auto *value=std::get_if<std::int64_t>(&item->lookup.value);
    if(!value)throw std::runtime_error("VP policy-cost define is not an integer");
    return *value;
}
double float_define(const content::GlobalDefineCache &defines,std::string_view name) {
    if(defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("VP policy-cost define profile mismatch");
    const auto *item=defines.find(name);
    if(!item||!item->lookup.error.empty())
        throw std::runtime_error("Unresolved VP policy-cost define: "+std::string(name));
    const auto *value=std::get_if<double>(&item->lookup.value);
    if(!value||!std::isfinite(*value))
        throw std::runtime_error("Invalid VP policy-cost float define");
    return *value;
}
std::int64_t source_truncate(double value) {
    if(!std::isfinite(value)||value < -0x1p63||value >= 0x1p63)
        throw std::overflow_error("VP policy cost cannot fit signed 64-bit state");
    return static_cast<std::int64_t>(value);
}
struct PolicyHandicapRules {
    std::int64_t policy_percent=0,policy_per_era=0,ai_policy_percent=0;
    std::int64_t ai_policy_per_era=0,policy_catchup=0,ai_policy_catchup=0;
};
PolicyHandicapRules handicap(const content::RulesDatabase &database,std::int64_t id) {
    PolicyHandicapRules result;
    bool found=false;
    const auto query=database.query_rows({
        "SELECT PolicyPercent,PolicyPerEraModifier,AIPolicyPercent,"
        "AIPolicyPerEraModifier,PolicyCatchUpMod,AIPolicyCatchUpMod "
        "FROM HandicapInfos WHERE ID = ?",{id}},[&](const auto &row) {
            if(found)throw std::runtime_error("Duplicate selected VP policy handicap");
            found=true;
            result={row.integer_at(0),row.integer_at(1),row.integer_at(2),
                    row.integer_at(3),row.integer_at(4),row.integer_at(5)};
        });
    if(!query.all_succeeded())throw std::runtime_error("VP policy handicap rules: "+query.error);
    if(!found)throw std::runtime_error("Missing selected VP policy handicap");
    return result;
}
}
VpPolicyCostSavedProfileState &ensure_vp_policy_cost_saved_profile(ResearchContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP policy cost requires VP GameCore profile");
    if(auto *p=world.extensions.find<VpPolicyCostSavedProfileState>())return *p;
    return world.extensions.emplace<VpPolicyCostSavedProfileState>();
}
const VpPolicyCostSavedProfileState *find_vp_policy_cost_saved_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpPolicyCostSavedProfileState>():nullptr;
}
void restore_vp_policy_cost_saved_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world) {
    if(world.profile!=vp||!snapshot.pregame||!snapshot.all_players||
       snapshot.pregame->layout!=saves::GameStateLayout::vp542_corpus||
       snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP policy-cost import requires VP players");
    VpPolicyCostSavedProfileState imported;
    imported.world_city_modifier_times100=snapshot.pregame->world.values.integers.at(
        "num_cities_policy_cost_mod");
    for(const auto &saved:snapshot.all_players->players) {
        const auto id=saved.saved_player_id;
        if(!world.players.contains(id)||saved.core.layout!=saves::GameStateLayout::vp542_corpus||
           saved.strategy.layout!=saves::GameStateLayout::vp542_corpus)
            throw std::runtime_error("VP policy-cost player is not in one VP roster");
        VpPolicyCostSavedPlayer player;
        player.saved_num_policies=scalar(saved.core,"m_iNumPolicies");
        player.free_policies=scalar(saved.core,"m_iNumFreePolicies");
        player.free_policies_ever=scalar(saved.core,"m_iNumFreePoliciesEver");
        player.free_tenets=scalar(saved.core,"m_iNumFreeTenets");
        player.policy_cost_modifier=scalar(saved.core,"m_iPolicyCostModifier");
        player.city_cost_discount=scalar(saved.core,"m_iNumCitiesPolicyCostDiscount");
        const auto &policies=named(saved.strategy.blocks,"policies");
        const auto &free=named(policies.arrays,"free_policy");
        const auto &owned=named(policies.arrays,"has_policy");
        const auto &branches=named(policies.arrays,"branch_unlocked");
        if(free.columns!=1||owned.columns!=1||branches.columns!=1||
           free.values.size()!=owned.values.size())
            throw std::runtime_error("VP policy-cost saved policy dimensions differ");
        player.free_policy.reserve(free.values.size());
        for(std::size_t i=0;i<free.values.size();++i) {
            if(free.values[i]<0||free.values[i]>1||owned.values[i]<0||owned.values[i]>1)
                throw std::runtime_error("Invalid VP saved policy flag");
            player.free_policy.push_back(static_cast<std::uint8_t>(free.values[i]));
        }
        player.branch_unlocked.reserve(branches.values.size());
        for(const auto flag:branches.values) {
            if(flag<0||flag>1)throw std::runtime_error("Invalid VP saved policy branch flag");
            player.branch_unlocked.push_back(static_cast<std::uint8_t>(flag));
        }
        if(!imported.players.emplace(id,std::move(player)).second)
            throw std::runtime_error("Duplicate VP policy-cost player");
    }
    if(imported.players.size()!=world.players.size())
        throw std::runtime_error("Incomplete VP policy-cost roster");
    ensure_vp_policy_cost_saved_profile(world)=std::move(imported);
}
VpPaidPolicyCount vp_paid_policy_count(const ResearchContext &world,
    const content::RulesDatabase &database,std::int64_t player) {
    if(world.profile!=vp)throw std::runtime_error("VP paid policies require VP GameCore profile");
    const auto *profile=find_vp_policy_cost_saved_profile(world);
    if(!profile)throw std::runtime_error("Missing VP policy-cost saved profile");
    const auto &saved=profile->players.at(player);
    const auto &live=world.players.at(player);
    if(!live.policies_resolved)throw std::runtime_error("Unresolved live VP policy ownership");
    std::map<std::int64_t,bool> dummy;
    const auto query=database.query_rows({"SELECT ID, IsDummy FROM Policies",{}},
        [&](const auto &row) {
            const auto id=row.integer_at(0);
            if(id<0||!dummy.emplace(id,row.integer_at(1)!=0).second)
                throw std::runtime_error("Invalid VP policy rule ID");
        });
    if(!query.all_succeeded())throw std::runtime_error("VP policy rules: "+query.error);
    VpPaidPolicyCount result;
    for(auto policy:live.policies) {
        if(policy<0||static_cast<std::uint64_t>(policy)>=saved.free_policy.size())
            throw std::runtime_error("Live VP policy is outside saved policy flags");
        const auto rule=dummy.find(policy);
        if(rule==dummy.end())throw std::runtime_error("Owned VP policy has no selected rule");
        result.owned=wide::add(result.owned,1);
        if(saved.free_policy[static_cast<std::size_t>(policy)])result.free=wide::add(result.free,1);
        else if(rule->second)result.dummy=wide::add(result.dummy,1);
        else result.paid=wide::add(result.paid,1);
    }
    return result;
}
VpPolicyCityModifierQuote vp_policy_city_modifier_times100(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::GlobalDefineCache &defines,bool balance_vp,std::int64_t player) {
    if(world.profile!=vp||players.profile!=vp)
        throw std::runtime_error("VP policy city cost requires VP GameCore profile");
    const auto *profile=find_vp_policy_cost_saved_profile(world);
    if(!profile)throw std::runtime_error("Missing VP policy-cost saved profile");
    const auto &saved=profile->players.at(player);
    const auto &live=players.players.at(player);
    VpPolicyCityModifierQuote result;
    const auto discount=wide::add(100,saved.city_cost_discount);
    result.base_times100=wide::multiply_divide(profile->world_city_modifier_times100,discount,100);
    result.scaling_times100=wide::multiply_divide(
        define(defines,"NUM_CITIES_COST_MOD_SCALING"),discount,100);
    result.effective_cities=live.cities.empty()?0:vp_effective_cities(live,true);
    result.policy_cities=std::max<std::int64_t>(0,wide::subtract(result.effective_cities,
        balance_vp?0:1));
    const auto n=result.policy_cities;
    result.pairs=n%2==0?wide::multiply_divide(n/2,wide::subtract(n,1),1):
        wide::multiply_divide(n,(n-1)/2,1);
    result.total_times100=wide::add(
        wide::multiply_divide(result.base_times100,n,1),
        wide::multiply_divide(result.scaling_times100,result.pairs,1));
    return result;
}
VpPolicyCostSubtotal vp_policy_cost_subtotal(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    bool balance_vp,std::int64_t player) {
    if(world.profile!=vp||players.profile!=vp)
        throw std::runtime_error("VP policy cost requires VP GameCore profile");
    const auto *profile=find_vp_policy_cost_saved_profile(world);
    if(!profile)throw std::runtime_error("Missing VP policy-cost saved profile");
    const auto &saved=profile->players.at(player);
    VpPolicyCostSubtotal result;
    result.counted_policies=vp_paid_policy_count(world,database,player).paid;
    result.charged_policies=wide::subtract(result.counted_policies,
        wide::subtract(wide::subtract(saved.free_policies_ever,saved.free_policies),saved.free_tenets));
    const auto coefficient=static_cast<float>(define(defines,"POLICY_COST_INCREASE_TO_BE_EXPONENTED"))+
        static_cast<float>(float_define(defines,"POLICY_COST_EXTRA_VALUE"));
    result.exponent_input=source_truncate(static_cast<float>(result.charged_policies)*coefficient);
    result.exponent_cost=source_truncate(std::pow(static_cast<double>(result.exponent_input),
        float_define(defines,"POLICY_COST_EXPONENT")));
    result.base_cost=wide::add(result.exponent_cost,define(defines,"BASE_POLICY_COST"));
    const auto city=vp_policy_city_modifier_times100(world,players,defines,balance_vp,player);
    result.after_cities=wide::multiply_divide(result.base_cost,
        wide::add(10000,city.total_times100),10000);
    result.after_player_modifier=wide::multiply_divide(result.after_cities,
        wide::add(100,saved.policy_cost_modifier),100);
    bool found=false;
    const auto query=database.query_rows({"SELECT CulturePercent FROM GameSpeeds WHERE ID = ?",{world.speed}},
        [&](const auto &row) {
            if(found)throw std::runtime_error("Duplicate selected VP game speed");
            found=true;
            result.speed_percent=row.integer_at(0);
        });
    if(!query.all_succeeded())throw std::runtime_error("VP game speed: "+query.error);
    if(!found)throw std::runtime_error("Missing selected VP game speed");
    result.after_speed=wide::multiply_divide(result.after_player_modifier,result.speed_percent,100);
    return result;
}
VpPolicyIdeologyQuote vp_policy_ideology_cost(
    const ResearchContext &world,const content::RulesDatabase &database,
    const content::GlobalDefineCache &defines,bool balance_vp,
    std::int64_t player,std::int64_t cost_after_speed) {
    if(world.profile!=vp)throw std::runtime_error("VP ideology cost requires VP GameCore profile");
    VpPolicyIdeologyQuote result;
    result.after_ideology=cost_after_speed;
    if(!balance_vp)return result;
    const auto &live=world.players.at(player);
    if(!live.policies_resolved)throw std::runtime_error("Unresolved live VP ideology tenets");
    std::map<std::int64_t,std::int64_t> levels;
    const auto query=database.query_rows({
        "SELECT p.ID,p.Level,b.PurchaseByLevel FROM Policies p "
        "LEFT JOIN PolicyBranchTypes b ON b.Type=p.PolicyBranchType",{}},
        [&](const auto &row) {
            const auto id=row.integer_at(0);
            if(id<0||!levels.emplace(id,row.integer_at(2)!=0?row.integer_at(1):0).second)
                throw std::runtime_error("Invalid VP ideology policy rule ID");
        });
    if(!query.all_succeeded())throw std::runtime_error("VP ideology rules: "+query.error);
    for(const auto policy:live.policies) {
        const auto found=levels.find(policy);
        if(found==levels.end())throw std::runtime_error("Owned VP tenet has no selected rule");
        switch(found->second) {
        case 1: result.tier1=wide::add(result.tier1,1);break;
        case 2: result.tier2=wide::add(result.tier2,1);break;
        case 3: result.tier3=wide::add(result.tier3,1);break;
        default: break;
        }
    }
    const auto exponent=static_cast<float>(float_define(defines,"POLICY_COST_EXPONENT"));
    for(const auto [level,count]:{std::pair{1,result.tier1},std::pair{2,result.tier2},
                                   std::pair{3,result.tier3}})
        result.penalty_percent=wide::add(result.penalty_percent,
            wide::multiply_divide(count,source_truncate(exponent*static_cast<float>(level)),1));
    result.after_ideology=wide::multiply_divide(cost_after_speed,
        wide::add(100,result.penalty_percent),100);
    return result;
}
VpPolicyFinalQuote vp_policy_final_cost(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::RulesDatabase &database,const content::GlobalDefineCache &defines,
    bool human_uses_ai_handicap,std::int64_t player,
    std::int64_t cost_after_ideology,std::int64_t actual_policy_count) {
    if(world.profile!=vp||players.profile!=vp)
        throw std::runtime_error("VP final policy cost requires VP GameCore profile");
    const auto &live=world.players.at(player);
    VpPolicyFinalQuote result;
    result.major=players.major_players.contains(player);
    result.after_player_handicap=result.after_ai_handicap=result.after_catchup=cost_after_ideology;
    if(result.major) {
        const auto raw_human=world.is_human(live);
        result.human_for_handicap=raw_human&&!human_uses_ai_handicap;
        result.player_handicap=raw_human&&human_uses_ai_handicap?
            define(defines,"AI_HANDICAP"):live.handicap;
        const auto player_rules=handicap(database,result.player_handicap);
        result.player_percent=player_rules.policy_percent;
        result.player_era_factor=std::max<std::int64_t>(0,wide::add(100,
            wide::multiply_divide(player_rules.policy_per_era,world.current_era,1)));
        result.after_player_handicap=wide::multiply_divide(cost_after_ideology,
            result.player_percent,100);
        result.after_player_handicap=wide::multiply_divide(result.after_player_handicap,
            result.player_era_factor,100);
        result.after_ai_handicap=result.after_player_handicap;
        std::int64_t extra_catchup=player_rules.policy_catchup;
        if(!result.human_for_handicap) {
            result.game_handicap=world.game_handicap;
            const auto game_rules=handicap(database,result.game_handicap);
            result.ai_percent=game_rules.ai_policy_percent;
            result.ai_era_factor=std::max<std::int64_t>(0,wide::add(100,
                wide::multiply_divide(game_rules.ai_policy_per_era,world.current_era,1)));
            result.after_ai_handicap=wide::multiply_divide(result.after_ai_handicap,
                result.ai_percent,100);
            result.after_ai_handicap=wide::multiply_divide(result.after_ai_handicap,
                result.ai_era_factor,100);
            extra_catchup=wide::add(extra_catchup,game_rules.ai_policy_catchup);
        }
        result.catchup_modifier=extra_catchup;
        result.after_catchup=result.after_ai_handicap;
        if(extra_catchup>0) {
            for(const auto other:players.major_players) {
                if(other==player)continue;
                const auto it=world.players.find(other);
                if(it==world.players.end())
                    throw std::runtime_error("VP major policy-cost roster differs");
                if(!it->second.alive)continue;
                result.possible_players=wide::add(result.possible_players,1);
                if(vp_paid_policy_count(world,database,other).paid>actual_policy_count)
                    result.higher_players=wide::add(result.higher_players,1);
            }
            const auto numerator=wide::multiply_divide(
                wide::multiply_divide(result.higher_players,extra_catchup,1),
                world.current_era,std::max<std::int64_t>(1,result.possible_players));
            result.catchup_factor=wide::subtract(100,numerator);
            result.after_catchup=wide::multiply_divide(result.after_ai_handicap,
                result.catchup_factor,100);
        }
    }
    result.visible_divisor=define(defines,"POLICY_COST_VISIBLE_DIVISOR");
    if(result.visible_divisor<=0)throw std::runtime_error("Invalid VP policy visible divisor");
    result.cost=wide::multiply_divide(
        wide::multiply_divide(result.after_catchup,1,result.visible_divisor),
        result.visible_divisor,1);
    return result;
}
VpPolicyCostUpdateResult do_vp_policy_cost_after_culture(
    ResearchContext &world,std::int64_t player,const VpPolicyFinalQuote &quote) {
    if(world.profile!=vp)throw std::runtime_error("VP policy update requires VP GameCore profile");
    auto *history=world.extensions.find<VpCultureHistoryProfileState>();
    if(!history)throw std::runtime_error("Missing VP policy-cost live field");
    auto &saved=history->players.at(player);
    const VpPolicyCostUpdateResult result{saved.next_policy_cost,quote.cost};
    saved.next_policy_cost=result.after;
    return result;
}
} // namespace civ5::runtime
