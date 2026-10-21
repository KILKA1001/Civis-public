#include "civ5/runtime/vp_policy_phase.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include <stdexcept>

namespace civ5::runtime {
VpPolicyPhaseQuote vp_policy_phase_without_host(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::RulesDatabase &database,const content::CustomModOptions &options,
    std::int64_t player) {
    constexpr auto vp=content::TechnologyProfile::vp542;
    if(world.profile!=vp||players.profile!=vp||!options.all_succeeded())
        throw std::runtime_error("VP policy phase requires VP GameCore rules and options");
    const auto &live=world.players.at(player);
    const auto &research=players.players.at(player);
    VpPolicyPhaseQuote result;
    result.outer_guard=live.alive&&players.major_players.contains(player)&&!research.cities.empty();
    if(!result.outer_guard) {
        result.safe_no_effect=true;
        return result;
    }
    const auto *saved=find_vp_policy_cost_saved_profile(world);
    const auto *history=find_vp_culture_history_profile(world);
    if(!saved||!saved->players.contains(player)||!history||!history->players.contains(player))
        throw std::runtime_error("Missing VP policy phase player state");
    const auto &branch_flags=saved->players.at(player).branch_unlocked;
    const auto query=database.query_rows({
        "SELECT ID,PurchaseByLevel FROM PolicyBranchTypes ORDER BY ID",{}},
        [&](const auto &row) {
            const auto id=row.integer_at(0);
            if(id<0||static_cast<std::uint64_t>(id)>=branch_flags.size())
                throw std::runtime_error("VP ideology branch outside saved flags");
            if(result.ideology_branch<0&&row.integer_at(1)!=0&&
               branch_flags[static_cast<std::size_t>(id)]!=0)
                result.ideology_branch=id;
        });
    if(!query.all_succeeded())throw std::runtime_error("VP policy branches: "+query.error);
    const auto &culture=history->players.at(player);
    result.culture_balance_times100=culture.balance_times100;
    result.next_policy_cost=culture.next_policy_cost;
    if(result.culture_balance_times100<0)
        throw std::runtime_error("Negative VP policy culture balance");
    result.affordable=result.next_policy_cost<=0||
        result.culture_balance_times100/100>=result.next_policy_cost;
    const auto human=world.is_human(live);
    result.human_policy_choice=human&&!options.enabled("AI_CONTROL_POLICY_CHOICE");
    result.human_diplomacy=human&&!options.enabled("AI_CONTROL_DIPLOMACY");
    result.vassal=players.teams.at(live.team).master>=0;
    if(!result.human_policy_choice)
        result.unresolved="VP policy AI is not bound";
    else if(result.affordable)
        result.unresolved="VP affordable-policy notification is not bound";
    else if(result.ideology_branch<0)
        result.unresolved="VP ideology choice eligibility is not bound";
    else if(result.vassal)
        result.unresolved="VP vassal ideology switch is not bound";
    else if(!result.human_diplomacy)
        result.unresolved="VP AI diplomacy ideology switch is not bound";
    else
        result.safe_no_effect=true;
    return result;
}
} // namespace civ5::runtime
