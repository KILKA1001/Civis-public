#include "civ5/runtime/vp_culture_income.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include "civ5/runtime/vp_treasury_income_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
template<class T> const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name&&std::string_view(row.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP culture income field: "+std::string(name));
        found=&row;
    }
    if(!found)throw std::runtime_error("Missing VP culture income field: "+std::string(name));
    return *found;
}
template<class T> std::int64_t scalar(const std::vector<T> &rows,std::string_view name) {
    const auto &v=named(rows,name).values;
    if(v.size()!=1)throw std::runtime_error("VP culture income scalar width mismatch");
    return v.front();
}
template<class T> std::int64_t yield(const std::vector<T> &rows,std::string_view name,std::int64_t id) {
    const auto &v=named(rows,name).values;
    if(id<0||static_cast<std::uint64_t>(id)>=v.size())
        throw std::runtime_error("VP culture yield missing from saved array: "+std::string(name));
    return v.at(static_cast<std::size_t>(id));
}
std::int64_t times100(std::int64_t n){return wide::multiply_divide(n,100,1);}
std::int64_t value(const YieldValues &values,std::int64_t id) {
    const auto it=values.find(id);
    return it==values.end()?0:it->second;
}
std::int64_t i64(std::size_t n) {
    if(n>static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
        throw std::overflow_error("VP culture collection exceeds int64");
    return static_cast<std::int64_t>(n);
}
std::int64_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    const auto *item=defines.find(name);
    if(!item||!item->lookup.error.empty())
        throw std::runtime_error("Unresolved VP culture define: "+std::string(name));
    const auto *value=std::get_if<std::int64_t>(&item->lookup.value);
    if(!value)throw std::runtime_error("VP culture define is not an integer: "+std::string(name));
    return *value;
}
const VpCultureIncomeSavedPlayer &culture_player(const TreasuryTurnContext &context,std::int64_t player) {
    const auto &world=context.world;
    if(context.state.profile!=vp||world.setup.profile!=vp||world.diplomacy.profile!=vp||
       world.cities.profile!=vp||context.maintenance.profile!=vp||context.rules.profile!=vp||
       context.city_rules.profile!=vp||context.maintenance_rules.profile!=vp||
       context.defines.profile()!=content::GlobalDefineProfile::vp542||
       !context.options||!context.options->all_succeeded())
        throw std::runtime_error("VP culture world/rules profile mismatch");
    const auto *profile=find_vp_culture_income_saved_profile(world.setup);
    if(!profile)throw std::runtime_error("Missing VP culture income saved profile");
    return profile->players.at(player);
}
}
VpCultureIncomeSavedProfileState &ensure_vp_culture_income_saved_profile(ResearchContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP culture income requires VP GameCore profile");
    if(auto *p=world.extensions.find<VpCultureIncomeSavedProfileState>())return *p;
    return world.extensions.emplace<VpCultureIncomeSavedProfileState>();
}
const VpCultureIncomeSavedProfileState *find_vp_culture_income_saved_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpCultureIncomeSavedProfileState>():nullptr;
}
void restore_vp_culture_income_saved_profile(const saves::SavedSessionSnapshot &snapshot,
    ResearchContext &world,std::int64_t culture_yield) {
    if(world.profile!=vp||!snapshot.all_players||snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus||culture_yield<0)
        throw std::runtime_error("VP culture income import requires VP players and yield Info");
    VpCultureIncomeSavedProfileState imported;
    for(const auto &saved:snapshot.all_players->players) {
        const auto id=saved.saved_player_id;
        if(!world.players.contains(id)||saved.core.layout!=saves::GameStateLayout::vp542_corpus||
           saved.traits.layout!=saves::GameStateLayout::vp542_corpus||
           saved.tail.layout!=saves::GameStateLayout::vp542_corpus)
            throw std::runtime_error("VP culture income player is not in one VP roster");
        VpCultureIncomeSavedPlayer player;
        player.free_times100=times100(scalar(saved.core.fields,"m_iJONSCulturePerTurnForFree"));
        player.jfd_minors_times100=times100(yield(saved.core.fields,"m_aiYieldFromMinors",culture_yield));
        player.annexed_minors_times100=yield(saved.core.fields,"m_piYieldPerTurnFromAnnexedMinorsTimes100",culture_yield);
        player.incoming_espionage_times100=times100(yield(saved.tail.fields,"m_aiIncomingEspionageYields",culture_yield));
        player.outgoing_espionage_times100=times100(yield(saved.tail.fields,"m_aiOutgoingEspionageYields",culture_yield));
        player.happiness_to_culture=scalar(saved.core.fields,"m_iHappinessToCulture");
        player.per_trade_partner=yield(saved.traits.fields,"m_iYieldChangePerTradePartner",culture_yield);
        player.per_export=yield(saved.traits.fields,"m_iYieldFromExport",culture_yield);
        player.per_import=yield(saved.traits.fields,"m_iYieldFromImport",culture_yield);
        player.temporary_bonus_turns=scalar(saved.core.fields,"m_iCultureBonusTurns");
        player.conquest_bonus_turns=scalar(saved.core.fields,"m_iCultureBonusTurnsConquest");
        player.conquest_modifier=scalar(saved.traits.fields,"m_iCultureBonusModifierConquest");
        player.golden_age_disabled=scalar(saved.core.fields,"m_iGoldenAgeCultureBonusDisabledCount")>0;
        if(!imported.players.emplace(id,player).second)
            throw std::runtime_error("Duplicate VP culture income player");
    }
    if(imported.players.size()!=world.players.size())
        throw std::runtime_error("Incomplete VP culture income roster");
    ensure_vp_culture_income_saved_profile(world)=std::move(imported);
}
VpCultureResourceTraitsQuote vp_culture_resource_traits_times100(
    const TreasuryTurnContext &context,std::int64_t player) {
    const auto &saved=culture_player(context,player);
    const auto &income=vp_treasury_income_profile(context.state.players.at(player));
    VpCultureResourceTraitsQuote q;
    if(!context.options->enabled("BALANCE_VP")) {
        std::set<std::int64_t> partners;
        for(const auto &route:context.state.trade_routes) {
            if(route.origin_x==-1&&route.origin_y==-1&&route.destination_x==-1&&route.destination_y==-1)continue;
            if(route.origin_owner==route.destination_owner)continue;
            if(route.origin_owner==player)partners.insert(route.destination_owner);
            else if(route.destination_owner==player)partners.insert(route.origin_owner);
        }
        q.trade_partners=i64(partners.size());
        q.partner_times100=times100(wide::multiply_divide(saved.per_trade_partner,q.trade_partners,1));
    }
    for(auto resource:context.rules.luxury_resources) {
        if(value(income.exported,resource)>0)q.exported_luxuries=wide::add(q.exported_luxuries,1);
        if(value(income.imported,resource)>0||value(income.from_minors,resource)>0||
           value(income.from_alliances,resource)>0||value(income.siphoned,resource)>0)
            q.imported_luxuries=wide::add(q.imported_luxuries,1);
    }
    const auto team=context.world.setup.players.at(player).team;
    const auto era=std::max<std::int64_t>(1,context.maintenance.team_eras.at(team));
    if(saved.per_export>0)
        q.export_times100=times100(wide::multiply_divide(
            wide::multiply_divide(era,q.exported_luxuries,1),saved.per_export,1));
    if(saved.per_import>0)
        q.import_times100=times100(wide::multiply_divide(
            wide::multiply_divide(era,q.imported_luxuries,1),saved.per_import,1));
    q.total_times100=wide::add(wide::add(q.partner_times100,q.export_times100),q.import_times100);
    return q;
}
VpCultureMinorIncomeQuote vp_culture_minor_income_times100(
    const TreasuryTurnContext &context,std::int64_t player,std::int64_t culture_yield) {
    const auto &saved=culture_player(context,player);
    if(culture_yield<0||static_cast<std::uint64_t>(culture_yield)>=context.world.cities.yield_slots)
        throw std::out_of_range("VP culture yield ID");
    VpCultureMinorIncomeQuote q;
    for(auto minor:context.state.minor_players)
        if(context.world.setup.players.at(minor).alive)
            q.city_states_times100=wide::add(q.city_states_times100,
                minor_yield_times100(context.state,context.world,context.maintenance,context.rules,
                    context.defines,*context.options,minor,player,culture_yield).total_times100);
    if(context.options->enabled("BALANCE_CORE_JFD"))q.jfd_times100=saved.jfd_minors_times100;
    q.total_times100=wide::add(q.city_states_times100,q.jfd_times100);
    return q;
}
std::int64_t vp_culture_happiness_times100(
    const TreasuryTurnContext &context,std::int64_t player) {
    const auto &saved=culture_player(context,player);
    if(context.options->enabled("BALANCE_VP")||context.world.cities.no_happiness||
       saved.happiness_to_culture==0)return 0;
    const auto happiness=context.world.cities.players.at(player).excess_happiness;
    if(!happiness)throw std::runtime_error("Unresolved VP culture excess happiness");
    if(*happiness<=0)return 0;
    // The source getter already returns hundredths; there is no trailing *100.
    return wide::multiply_divide(*happiness,saved.happiness_to_culture,1);
}
VpCultureTurnControls vp_culture_turn_controls(
    const TreasuryTurnContext &context,std::int64_t player,bool no_policies) {
    const auto &saved=culture_player(context,player);
    const auto *history=find_vp_culture_history_profile(context.world.setup);
    if(!history||!history->players.contains(player))
        throw std::runtime_error("Unresolved VP culture history for income");
    VpCultureTurnControls q;
    q.no_policies=no_policies;
    q.anarchy=history->players.at(player).anarchy_turns>0;
    if(q.no_policies||q.anarchy)return q;
    q.golden_age=context.world.cities.players.at(player).golden_age;
    q.golden_age_disabled=saved.golden_age_disabled;
    q.temporary_bonus=saved.temporary_bonus_turns>0;
    q.conquest_bonus=saved.conquest_bonus_turns>0;
    if(q.temporary_bonus)
        q.temporary_boost_modifier=define(context.defines,"TEMPORARY_CULTURE_BOOST_MOD");
    if(q.conquest_bonus)q.conquest_modifier=saved.conquest_modifier;
    if(q.golden_age&&!q.golden_age_disabled)
        q.golden_age_modifier=define(context.defines,"GOLDEN_AGE_CULTURE_MODIFIER");
    return q;
}
VpCultureVassalQuote vp_culture_vassals_times100(
    const TreasuryTurnContext &context,const ScienceContext &science,
    std::int64_t player,bool no_policies,const VpCulturePerTurnService &culture_per_turn) {
    (void)culture_player(context,player);
    VpCultureVassalQuote q;
    if(no_policies)return q;
    if(science.profile!=vp)throw std::runtime_error("VP culture vassals require VP science state");
    const auto master_team=context.world.setup.players.at(player).team;
    for(auto vassal:context.world.diplomacy.major_players) {
        const auto team=context.world.setup.players.at(vassal).team;
        if(context.world.diplomacy.teams.at(team).master!=master_team)continue;
        if(!culture_per_turn)
            throw std::runtime_error("Unresolved complete VP vassal culture income");
        const auto income=culture_per_turn(vassal);
        if(!income||*income<0)
            throw std::runtime_error("Unresolved complete VP vassal culture income");
        if(q.by_vassal_times100.empty())
            q.modifier=wide::add(define(context.defines,"VASSALAGE_FREE_YIELD_FROM_VASSAL_PERCENT"),
                vp_science_profile(science.players.at(player)).vassal_modifier);
        const auto share=wide::multiply_divide(*income,q.modifier,100);
        q.by_vassal_times100.emplace(vassal,share);
        q.total_times100=wide::add(q.total_times100,share);
    }
    return q;
}
VpCultureReligionRules load_vp_culture_religion_rules(const content::RulesDatabase &database) {
    VpCultureReligionRules rules;
    const auto query=database.query_rows(
        {"SELECT ID, PlayerCultureModifier, RequiresPeace FROM Beliefs",{}},
        [&](const auto &row) {
            const auto id=row.integer_at(0);
            if(id<0||!rules.beliefs.emplace(id,VpCultureReligionBeliefRule{
                    row.integer_at(1),row.integer_at(2)!=0}).second)
                throw std::runtime_error("Invalid VP culture belief ID");
        });
    if(!query.all_succeeded())throw std::runtime_error("VP culture belief rules: "+query.error);
    return rules;
}
VpCultureReligionQuote vp_culture_religion(
    const TreasuryTurnContext &context,const VpCultureReligionRules &rules,std::int64_t player) {
    (void)culture_player(context,player);
    VpCultureReligionQuote q;
    const auto &owner=vp_treasury_religion_profile(context.state.players.at(player));
    if(!owner.owns_state_religion||owner.state_religion<=context.world.cities.pantheon)return q;
    q.owned_religion=owner.state_religion;
    const TreasuryReligion *religion=nullptr;
    for(const auto &entry:context.state.religions)
        if(entry.religion==q.owned_religion&&!entry.pantheon) {
            if(religion)throw std::runtime_error("Duplicate owned VP religion");
            religion=&entry;
        }
    if(!religion)return q;
    const auto team=context.world.setup.players.at(player).team;
    const bool at_peace=context.world.diplomacy.teams.at(team).at_war.empty();
    const bool founder_eligible=religion->pantheon&&!religion->holy_plot_owner
        ?religion->founder==player
        :religion->holy_plot_owner&&*religion->holy_plot_owner==player;
    const auto holy=religion->holy_city;
    const auto holy_owner=holy?holy->first:-1;
    const auto holy_city=holy?holy->second:-1;
    const auto majority=holy?vp_treasury_minor_profile(context.state.players.at(holy_owner))
        .city_majorities.at(holy_city):-1;
    const auto capital=context.world.cities.players.at(player).capital;
    for(auto belief:religion->beliefs) {
        const auto rule=rules.beliefs.find(belief);
        const auto flags=context.rules.beliefs.find(belief);
        if(rule==rules.beliefs.end()||flags==context.rules.beliefs.end())
            throw std::runtime_error("Unresolved VP culture belief");
        const auto amount=rule->second.modifier;
        if(!amount||(rule->second.requires_peace&&!at_peace))continue;
        const auto &b=flags->second;
        if((b.founder||b.enhancer)&&(!founder_eligible||(holy&&holy_owner!=player)))continue;
        if((b.follower||b.reformation)&&holy&&(majority!=q.owned_religion||holy_owner!=player))continue;
        if(b.reformation&&!founder_eligible&&owner.state_religion!=q.owned_religion)continue;
        if(b.reformation&&holy&&!founder_eligible&&holy_city!=capital)continue;
        if(b.pantheon) {
            if(!founder_eligible&&q.owned_religion>context.world.cities.pantheon&&
               owner.state_religion!=q.owned_religion)continue;
            if(holy) {
                bool merged=false;
                for(const auto &entry:context.state.religions)
                    if(entry.pantheon&&entry.founder==player&&
                       entry.belief_lookup.contains(belief)&&religion->belief_lookup.contains(belief)) {
                        merged=true;break;
                    }
                if((majority!=q.owned_religion&&!merged)||holy_owner!=player)continue;
                if(founder_eligible&&holy_owner!=player)continue;
                if(!founder_eligible&&holy_city!=capital)continue;
            }
        }
        if(context.options->enabled("BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV")&&
           b.civilization!=-1&&b.civilization!=context.world.cities.players.at(player).civilization)continue;
        q.by_belief[belief]=wide::add(q.by_belief[belief],amount);
        q.modifier=wide::add(q.modifier,amount);
    }
    return q;
}
VpCultureIncomeQuote vp_culture_income_times100(const VpCultureIncomeInputs &in) {
    VpCultureIncomeQuote q;
    if(in.no_policies||in.anarchy)return q;
    const auto add=wide::add;
    const auto percent=[](std::int64_t base,std::int64_t modifier) {
        return wide::multiply_divide(base,modifier,100);
    };
    auto base=add(in.cities_times100,in.happiness_times100);
    base=add(base,in.traits_times100);
    base=add(base,in.free_times100);
    base=add(base,in.minors_times100);
    base=add(base,in.annexed_minors_times100);
    base=add(base,in.incoming_espionage_times100);
    base=wide::subtract(base,in.outgoing_espionage_times100);
    base=add(base,in.vassals_times100);
    base=add(base,in.religion_base_times100);
    q.before_modifiers_times100=base;
    q.religion_extra_times100=percent(base,in.religion_modifier);
    if(in.temporary_bonus)
        q.bonus_turns_times100=percent(base,in.temporary_boost_modifier);
    if(in.conquest_bonus)
        q.bonus_turns_times100=add(q.bonus_turns_times100,percent(base,in.conquest_modifier));
    // The native iTmp still contains GetYieldPerTurnFromVassalsTimes100 here.
    if(in.golden_age&&!in.golden_age_disabled)
        q.golden_age_times100=add(in.vassals_times100,percent(base,in.golden_age_modifier));
    q.total_times100=std::max<std::int64_t>(0,
        add(add(add(base,q.religion_extra_times100),q.bonus_turns_times100),q.golden_age_times100));
    return q;
}
VpCultureResolvedIncome vp_resolve_culture_income_times100(
    const TreasuryTurnContext &context,const ScienceContext &science,
    const VpCultureReligionRules &religion_rules,std::int64_t culture_yield,
    std::int64_t player,bool no_policies) {
    (void)culture_player(context,player);
    if(culture_yield<0||static_cast<std::uint64_t>(culture_yield)>=context.world.cities.yield_slots)
        throw std::out_of_range("VP culture yield ID");
    struct Frame {
        std::int64_t player;
        bool entered=false;
        std::vector<std::int64_t> vassals;
        std::size_t next_vassal=0;
        VpCultureTurnControls controls;
    };
    VpCultureResolvedIncome result;
    std::vector<Frame> stack{{player}};
    std::set<std::int64_t> visiting;
    while(!stack.empty()) {
        auto &frame=stack.back();
        const auto current=frame.player;
        if(!frame.entered) {
            if(!visiting.insert(current).second)
                throw std::runtime_error("Cyclic VP culture vassal dependency");
            frame.controls=vp_culture_turn_controls(context,current,no_policies);
            frame.entered=true;
            if(frame.controls.no_policies||frame.controls.anarchy) {
                VpCultureIncomeInputs inputs;
                inputs.no_policies=frame.controls.no_policies;
                inputs.anarchy=frame.controls.anarchy;
                result.inputs_by_player.emplace(current,inputs);
                result.quotes_by_player.emplace(current,vp_culture_income_times100(inputs));
                visiting.erase(current);
                stack.pop_back();
                continue;
            }
            const auto master_team=context.world.setup.players.at(current).team;
            for(auto vassal:context.world.diplomacy.major_players) {
                const auto team=context.world.setup.players.at(vassal).team;
                if(context.world.diplomacy.teams.at(team).master==master_team)
                    frame.vassals.push_back(vassal);
            }
        }
        if(frame.next_vassal<frame.vassals.size()) {
            const auto child=frame.vassals[frame.next_vassal++];
            if(result.quotes_by_player.contains(child))continue;
            if(visiting.contains(child))
                throw std::runtime_error("Cyclic VP culture vassal dependency");
            stack.push_back({child});
            continue;
        }
        VpCultureIncomeInputs inputs;
        inputs.golden_age=frame.controls.golden_age;
        inputs.golden_age_disabled=frame.controls.golden_age_disabled;
        inputs.temporary_bonus=frame.controls.temporary_bonus;
        inputs.conquest_bonus=frame.controls.conquest_bonus;
        inputs.temporary_boost_modifier=frame.controls.temporary_boost_modifier;
        inputs.conquest_modifier=frame.controls.conquest_modifier;
        inputs.golden_age_modifier=frame.controls.golden_age_modifier;
        for(const auto &[city_id,city]:context.world.cities.players.at(current).cities)
            inputs.cities_times100=wide::add(inputs.cities_times100,city_yield_times100(
                context.world.cities,context.city_rules,context.defines,current,city_id,culture_yield).total_times100);
        inputs.happiness_times100=vp_culture_happiness_times100(context,current);
        inputs.traits_times100=vp_culture_resource_traits_times100(context,current).total_times100;
        const auto &saved=culture_player(context,current);
        inputs.free_times100=saved.free_times100;
        inputs.minors_times100=vp_culture_minor_income_times100(context,current,culture_yield).total_times100;
        inputs.annexed_minors_times100=saved.annexed_minors_times100;
        inputs.incoming_espionage_times100=saved.incoming_espionage_times100;
        inputs.outgoing_espionage_times100=saved.outgoing_espionage_times100;
        inputs.vassals_times100=vp_culture_vassals_times100(context,science,current,no_policies,
            [&](std::int64_t vassal)->std::optional<std::int64_t> {
                const auto it=result.quotes_by_player.find(vassal);
                if(it==result.quotes_by_player.end())return {};
                return it->second.total_times100;
            }).total_times100;
        const auto religion=vp_culture_religion(context,religion_rules,current);
        inputs.religion_base_times100=religion.base_times100;
        inputs.religion_modifier=religion.modifier;
        result.inputs_by_player.emplace(current,inputs);
        result.quotes_by_player.emplace(current,vp_culture_income_times100(inputs));
        visiting.erase(current);
        stack.pop_back();
    }
    result.quote=result.quotes_by_player.at(player);
    return result;
}
VpCultureIncomeTurnResult do_vp_culture_income_after_gold(
    ResearchContext &setup,const TreasuryTurnContext &context,const ScienceContext &science,
    const VpCultureReligionRules &religion_rules,std::int64_t culture_yield,
    std::int64_t player,VpCultureAccrualOptions options) {
    if(&setup!=&context.world.setup)
        throw std::invalid_argument("VP culture income requires one live world");
    VpCultureIncomeTurnResult result;
    result.resolved=vp_resolve_culture_income_times100(context,science,religion_rules,
        culture_yield,player,options.no_policies);
    result.accrual=apply_vp_culture_income(setup,player,
        result.resolved.quote.total_times100,options);
    return result;
}
} // namespace civ5::runtime
