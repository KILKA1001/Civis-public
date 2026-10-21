#include "civ5/runtime/vp_vassal_tax_turn.hpp"
#include "civ5/runtime/vp_maintenance_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"

namespace civ5::runtime {
namespace {
const auto vp=content::TechnologyProfile::vp542;
template<class T> const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name&&std::string_view(row.name)==name) {
        if(found)throw TreasuryUnavailable("Duplicate VP vassal tax field");
        found=&row;
    }
    if(!found)throw TreasuryUnavailable("Missing VP vassal tax field: "+std::string(name));
    return *found;
}
}
VpVassalTaxProfileState &ensure_vp_vassal_tax_profile(ResearchContext &world) {
    if(world.profile!=vp)throw TreasuryUnavailable("VP vassal taxes require VP GameCore profile");
    if(auto *profile=world.extensions.find<VpVassalTaxProfileState>())return *profile;
    return world.extensions.emplace<VpVassalTaxProfileState>();
}
const VpVassalTaxProfileState *find_vp_vassal_tax_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpVassalTaxProfileState>():nullptr;
}
void restore_vp_vassal_tax_profile(const saves::SavedSessionSnapshot &snapshot,ResearchContext &world) {
    if(world.profile!=vp||!snapshot.teams||snapshot.teams->layout!=saves::GameStateLayout::vp542_corpus)
        throw TreasuryUnavailable("VP vassal tax import requires VP teams");
    VpVassalTaxProfileState imported;
    const auto width=snapshot.teams->dimensions.major;
    for(const auto &team:snapshot.teams->teams) {
        const auto id=named(team.fields,"id").value;
        if(!world.team_members.contains(id))throw TreasuryUnavailable("VP vassal tax team is not in shared roster");
        const auto &rates=named(team.arrays,"vassal_tax").values;
        if(rates.size()!=width)throw TreasuryUnavailable("VP vassal tax wire width mismatch");
        auto [it,added]=imported.rates_by_master_team.try_emplace(id);
        if(!added)throw TreasuryUnavailable("Duplicate VP vassal tax team");
        auto &row=it->second;
        for(std::size_t i=0;i<rates.size();++i) {
            if(i>INT64_MAX)throw TreasuryUnavailable("VP vassal tax player ID exceeds int64");
            row.emplace(static_cast<std::int64_t>(i),rates[i]);
        }
    }
    if(imported.rates_by_master_team.size()!=world.team_members.size())
        throw TreasuryUnavailable("Incomplete VP vassal tax team roster");
    ensure_vp_vassal_tax_profile(world)=std::move(imported);
}
VpVassalTaxTurnResult do_vp_vassal_tax_after_gold(TreasuryTurnContext &context,std::int64_t player) {
    const auto &world=context.world;
    if(context.state.profile!=vp||world.setup.profile!=vp||world.diplomacy.profile!=vp||
       world.cities.profile!=vp||context.maintenance.profile!=vp||context.rules.profile!=vp||
       context.city_rules.profile!=vp||context.maintenance_rules.profile!=vp||
       context.defines.profile()!=content::GlobalDefineProfile::vp542||
       !context.options||!context.options->all_succeeded())
        throw TreasuryUnavailable("VP vassal tax turn requires one VP world");
    VpVassalTaxTurnResult result;
    const auto team=world.setup.players.at(player).team;
    result.master=world.diplomacy.teams.at(team).master;
    auto &taxes=vp_maintenance_profile(context.maintenance.players.at(player)).vassal_taxes_times100;
    if(result.master==-1) {taxes=0;return result;}
    result.vassal=true;
    result.gross_times100=gross_gold_times100(context.state,world,context.maintenance,context.rules,
        context.city_rules,context.defines,context.options,player,context.income_services).total_times100;
    if(result.gross_times100<=0) {taxes=0;return result;}
    const auto *profile=find_vp_vassal_tax_profile(world.setup);
    if(!profile||!profile->rates_by_master_team.contains(result.master)||
       !profile->rates_by_master_team.at(result.master).contains(player))
        throw TreasuryUnavailable("Unresolved VP master tax rate");
    result.rate=profile->rates_by_master_team.at(result.master).at(player);
    result.expense_times100=wide::multiply_divide(result.gross_times100,result.rate,100);
    taxes=result.expense_times100;
    return result;
}
} // namespace civ5::runtime
