#include "civ5/runtime/vp_culture_history_turn.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
const auto vp=content::TechnologyProfile::vp542;
template<class T> const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name&&std::string_view(row.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP culture history field: "+std::string(name));
        found=&row;
    }
    if(!found)throw std::runtime_error("Missing VP culture history field: "+std::string(name));
    return *found;
}
std::int64_t scalar(const saves::PlayerCoreObservation &core,std::string_view name) {
    const auto &field=named(core.fields,name);
    if(field.values.size()!=1)throw std::runtime_error("VP culture history scalar width mismatch");
    return field.values.front();
}
}
VpCultureHistoryProfileState &ensure_vp_culture_history_profile(ResearchContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP culture history requires VP GameCore profile");
    if(auto *profile=world.extensions.find<VpCultureHistoryProfileState>())return *profile;
    return world.extensions.emplace<VpCultureHistoryProfileState>();
}
const VpCultureHistoryProfileState *find_vp_culture_history_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpCultureHistoryProfileState>():nullptr;
}
void restore_vp_culture_history_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world) {
    if(world.profile!=vp||!snapshot.all_players||snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP culture history import requires VP players");
    VpCultureHistoryProfileState imported;
    for(const auto &saved:snapshot.all_players->players) {
        const auto id=saved.saved_player_id;
        if(!world.players.contains(id)||saved.core.layout!=saves::GameStateLayout::vp542_corpus||
           saved.trade_culture.layout!=saves::GameStateLayout::vp542_corpus)
            throw std::runtime_error("VP culture history player is not in one VP roster");
        const auto &culture=named(saved.trade_culture.blocks,"player_culture");
        VpCultureHistoryPlayer player;
        player.lifetime_times100=scalar(saved.core,"m_lJONSCultureEverGeneratedTimes100");
        player.last_lifetime_times100=named(culture.fields,"m_lLastTurnLifetimeCultureTimes100").value;
        player.last_cpt_times100=named(culture.fields,"m_iLastTurnCPT").value;
        player.balance_times100=scalar(saved.core,"m_iJONSCultureTimes100");
        player.next_policy_cost=scalar(saved.core,"m_iCostNextPolicy");
        player.anarchy_turns=scalar(saved.core,"m_iAnarchyNumTurns");
        if(!imported.players.emplace(id,player).second)
            throw std::runtime_error("Duplicate VP culture history player");
    }
    if(imported.players.size()!=world.players.size())
        throw std::runtime_error("Incomplete VP culture history roster");
    ensure_vp_culture_history_profile(world)=std::move(imported);
}
std::int64_t do_vp_culture_history_after_gold(ResearchContext &world,std::int64_t player) {
    if(world.profile!=vp)throw std::runtime_error("VP culture history requires VP GameCore profile");
    auto *profile=world.extensions.find<VpCultureHistoryProfileState>();
    if(!profile||!world.players.contains(player)||!profile->players.contains(player))
        throw std::runtime_error("Unresolved VP culture history player");
    auto &state=profile->players.at(player);
    const auto delta=wide::subtract(state.lifetime_times100,state.last_lifetime_times100);
    state.last_cpt_times100=delta;
    state.last_lifetime_times100=state.lifetime_times100;
    return delta;
}
VpCultureAccrualResult apply_vp_culture_income(ResearchContext &world,std::int64_t player,
    std::int64_t quoted_times100,VpCultureAccrualOptions options) {
    if(world.profile!=vp||quoted_times100<0)
        throw std::runtime_error("VP culture income requires a nonnegative VP quote");
    auto *profile=world.extensions.find<VpCultureHistoryProfileState>();
    if(!profile||!world.players.contains(player)||!profile->players.contains(player))
        throw std::runtime_error("Unresolved VP culture income player");
    auto &state=profile->players.at(player);
    VpCultureAccrualResult result;
    result.quoted_times100=quoted_times100;
    result.balance_before_times100=state.balance_times100;
    result.lifetime_before_times100=state.lifetime_times100;
    const auto visible_balance=(options.minor||options.no_policies)?0:state.balance_times100;
    // GetTotalJONSCulturePerTurnTimes100 returns zero for no-policies/anarchy.
    const auto income=(options.no_policies||state.anarchy_turns>0)?0:quoted_times100;
    if(options.timer_enabled&&visible_balance>=wide::multiply_divide(state.next_policy_cost,100,1))
        result.timer_blocked=true;
    else if(income!=0) {
        // CvPlayer::changeJONSCultureTimes100 passes the getter's visible
        // balance to setJONSCultureTimes100. Minor civs expose zero there.
        const auto proposed=wide::add(visible_balance,income);
        if(proposed>state.balance_times100)
            state.lifetime_times100=wide::add(state.lifetime_times100,
                wide::subtract(proposed,state.balance_times100));
        state.balance_times100=std::max<std::int64_t>(0,proposed);
        result.credited_times100=income;
    }
    result.balance_after_times100=state.balance_times100;
    result.lifetime_after_times100=state.lifetime_times100;
    return result;
}
} // namespace civ5::runtime
