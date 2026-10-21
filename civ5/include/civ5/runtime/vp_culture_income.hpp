#pragma once
#include "civ5/runtime/research_context.hpp"
#include "civ5/runtime/science.hpp"
#include "civ5/runtime/treasury_turn.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include <cstdint>
#include <functional>

namespace civ5::runtime {
struct VpCultureIncomeSavedPlayer {
    std::int64_t free_times100=0,jfd_minors_times100=0,annexed_minors_times100=0;
    std::int64_t incoming_espionage_times100=0,outgoing_espionage_times100=0;
    std::int64_t happiness_to_culture=0,per_trade_partner=0,per_export=0,per_import=0;
    std::int64_t temporary_bonus_turns=0,conquest_bonus_turns=0,conquest_modifier=0;
    bool golden_age_disabled=false;
};
struct VpCultureIncomeSavedProfileState {
    std::map<std::int64_t,VpCultureIncomeSavedPlayer> players;
};
VpCultureIncomeSavedProfileState &ensure_vp_culture_income_saved_profile(ResearchContext &world);
const VpCultureIncomeSavedProfileState *find_vp_culture_income_saved_profile(const ResearchContext &world);
// The culture yield identity comes from the selected XML/SQL Info catalog.
// The original source-array width is checked only while importing the save.
void restore_vp_culture_income_saved_profile(const saves::SavedSessionSnapshot &snapshot,
    ResearchContext &world,std::int64_t culture_yield);

struct VpCultureResourceTraitsQuote {
    std::int64_t trade_partners=0,exported_luxuries=0,imported_luxuries=0;
    std::int64_t partner_times100=0,export_times100=0,import_times100=0,total_times100=0;
};
struct VpCultureMinorIncomeQuote {
    std::int64_t city_states_times100=0,jfd_times100=0,total_times100=0;
};
// Resolve VP-only trait and minor-civilization culture from the live world,
// selected SQL rules and saved VP extensions. Neither quote mutates the turn.
VpCultureResourceTraitsQuote vp_culture_resource_traits_times100(
    const TreasuryTurnContext &context,std::int64_t player);
VpCultureMinorIncomeQuote vp_culture_minor_income_times100(
    const TreasuryTurnContext &context,std::int64_t player,std::int64_t culture_yield);
// CP's happiness-to-culture branch remains lazy: VP or NO_HAPPINESS returns
// zero without demanding an unresolved empire happiness calculation.
std::int64_t vp_culture_happiness_times100(
    const TreasuryTurnContext &context,std::int64_t player);
struct VpCultureTurnControls {
    bool no_policies=false,anarchy=false,golden_age=false,golden_age_disabled=false;
    bool temporary_bonus=false,conquest_bonus=false;
    std::int64_t temporary_boost_modifier=0,conquest_modifier=0,golden_age_modifier=0;
};
// no_policies is resolved from the selected game's GameOptions before this
// query. Remaining VP controls use live state, saved extensions and Defines.
VpCultureTurnControls vp_culture_turn_controls(
    const TreasuryTurnContext &context,std::int64_t player,bool no_policies);
struct VpCultureVassalQuote {
    std::int64_t modifier=0,total_times100=0;
    std::map<std::int64_t,std::int64_t> by_vassal_times100;
};
using VpCulturePerTurnService=std::function<std::optional<std::int64_t>(std::int64_t)>;
// The original getter asks each vassal for its complete culture quote. Until
// that recursive query is bound, a missing service is an unresolved dependency.
VpCultureVassalQuote vp_culture_vassals_times100(
    const TreasuryTurnContext &context,const ScienceContext &science,
    std::int64_t player,bool no_policies,const VpCulturePerTurnService &culture_per_turn={});
struct VpCultureReligionBeliefRule {
    std::int64_t modifier=0;
    bool requires_peace=false;
};
struct VpCultureReligionRules {
    std::map<std::int64_t,VpCultureReligionBeliefRule> beliefs;
};
VpCultureReligionRules load_vp_culture_religion_rules(const content::RulesDatabase &database);
struct VpCultureReligionQuote {
    std::int64_t owned_religion=-1,base_times100=0,modifier=0;
    std::map<std::int64_t,std::int64_t> by_belief;
};
// VP 5.4.2's GetYieldPerTurnFromReligion returns zero; founder beliefs can
// still modify the total culture quote after its other sources are added.
VpCultureReligionQuote vp_culture_religion(
    const TreasuryTurnContext &context,const VpCultureReligionRules &rules,std::int64_t player);

// Components are resolved from live cities, selected rules, saved VP profile
// state and mod services before this source-order arithmetic is called.
struct VpCultureIncomeInputs {
    bool no_policies=false,anarchy=false,golden_age=false,golden_age_disabled=false;
    bool temporary_bonus=false,conquest_bonus=false;
    std::int64_t cities_times100=0,happiness_times100=0,traits_times100=0;
    std::int64_t free_times100=0,minors_times100=0,annexed_minors_times100=0;
    std::int64_t incoming_espionage_times100=0,outgoing_espionage_times100=0;
    std::int64_t vassals_times100=0,religion_base_times100=0;
    std::int64_t religion_modifier=0,temporary_boost_modifier=0;
    std::int64_t conquest_modifier=0,golden_age_modifier=0;
};
struct VpCultureIncomeQuote {
    std::int64_t before_modifiers_times100=0,religion_extra_times100=0;
    std::int64_t bonus_turns_times100=0,golden_age_times100=0,total_times100=0;
};
// VP CvPlayer::GetTotalJONSCulturePerTurnTimes100 arithmetic, retaining the
// source's live iTmp carry into the Golden Age branch without i32 wrap.
VpCultureIncomeQuote vp_culture_income_times100(const VpCultureIncomeInputs &inputs);
struct VpCultureResolvedIncome {
    VpCultureIncomeQuote quote;
    std::map<std::int64_t,VpCultureIncomeInputs> inputs_by_player;
    std::map<std::int64_t,VpCultureIncomeQuote> quotes_by_player;
};
// Resolve complete VP 5.4.2 culture income on one live world. Vassal queries
// are evaluated bottom-up with a heap stack and reject dependency cycles.
VpCultureResolvedIncome vp_resolve_culture_income_times100(
    const TreasuryTurnContext &context,const ScienceContext &science,
    const VpCultureReligionRules &religion_rules,std::int64_t culture_yield,
    std::int64_t player,bool no_policies);
struct VpCultureIncomeTurnResult {
    VpCultureResolvedIncome resolved;
    VpCultureAccrualResult accrual;
};
// After the separate saved-history update, quote the complete VP culture
// income and credit it to VP-owned live culture state. The quote is resolved
// before either balance or lifetime culture can change.
VpCultureIncomeTurnResult do_vp_culture_income_after_gold(
    ResearchContext &setup,const TreasuryTurnContext &context,const ScienceContext &science,
    const VpCultureReligionRules &religion_rules,std::int64_t culture_yield,
    std::int64_t player,VpCultureAccrualOptions options);
} // namespace civ5::runtime
