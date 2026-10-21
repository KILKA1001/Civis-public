#pragma once
#include "civ5/runtime/player_economy_research_stage.hpp"
#include "civ5/runtime/vp_culture_income.hpp"
#include "civ5/runtime/vp_culture_history_turn.hpp"
#include "civ5/runtime/vp_policy_cost.hpp"
#include "civ5/runtime/vp_anarchy_turn.hpp"
#include "civ5/runtime/vp_policy_phase.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/runtime/vp_attack_operations.hpp"
#include "civ5/runtime/vp_army_construction.hpp"
#include "civ5/runtime/vp_city_espionage.hpp"
#include "civ5/runtime/vp_city_construction_intrigue.hpp"
#include "civ5/runtime/vp_diplomacy_intrigue.hpp"
#include "civ5/runtime/vp_network_points.hpp"
#include "civ5/runtime/vp_network_point_binding.hpp"
#include "civ5/runtime/vp_gathering_intel_turn.hpp"
#include "civ5/runtime/vp_espionage_player_turn.hpp"
#include "civ5/runtime/vp_vassal_tax_turn.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "research_turn_report.hpp"
#include "treasury_turn_report.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace civ5::validation {
// Run an ordered subset of player-turn steps on one disposable imported world.
// An unresolved mandatory host service stops the segment; partial mutations
// are reported, never written back as a continuation checkpoint. The VP
// espionage probe runs only on a second disposable city copy. It never marks
// the incomplete player turn as complete or writes its city changes to a save.
inline std::string player_economy_research_stage_report(
    const saves::SavedSessionSnapshot &snapshot,
    const content::RulesDatabase &database,
    const runtime::ResearchTurnState &source_turn,
    const runtime::PlayerResearchContext &source_players,
    const runtime::ResearchContext &setup,
    const content::ResearchRules &research_rules,
    const content::TechnologyRules &technologies,
    const content::GlobalDefineCache &defines,
    const content::CustomModOptions *options,
    const std::vector<runtime::TeamTechnologies> &source_teams,
    const runtime::ScienceContext &source_science,
    const runtime::CityYieldContext &source_cities,
    const runtime::TreasuryContext &source_treasury,
    const runtime::MaintenanceContext &source_maintenance,
    const runtime::TreasuryRules &treasury_rules,
    const runtime::CityYieldRules &city_rules,
    const runtime::MaintenanceRules &maintenance_rules,
    std::int64_t culture_yield,
    std::int64_t active_player) {
    std::ostringstream out;
    if (!setup.players.contains(active_player) || !source_players.players.contains(active_player) ||
        !source_treasury.players.contains(active_player) || !source_turn.players.contains(active_player)) {
        out << "{\"full_turn\":false,\"disposable_copy\":true,\"status\":\"unresolved\","
               "\"error\":\"Active player missing from imported world\"}";
        return out.str();
    }
    auto setup_copy=setup;
    std::string error;
    if(setup_copy.profile==content::TechnologyProfile::vp542) {
        try {
            runtime::restore_vp_vassal_tax_profile(snapshot,setup_copy);
            runtime::restore_vp_culture_history_profile(snapshot,setup_copy);
            runtime::restore_vp_culture_income_saved_profile(snapshot,setup_copy,culture_yield);
            runtime::restore_vp_policy_cost_saved_profile(snapshot,setup_copy);
            if(const auto *live=runtime::find_vp_espionage_profile(setup_copy)) {
                if(live->players.size()!=setup_copy.players.size())
                    throw std::runtime_error("Partial VP player espionage state in live world");
                for(const auto &[id,player]:setup_copy.players) {
                    (void)player;
                    if(!live->players.contains(id))
                        throw std::runtime_error("Mismatched VP player espionage roster");
                }
            } else
                runtime::restore_vp_espionage_profile(snapshot,setup_copy);
            if(const auto *live=runtime::find_vp_attack_operations_profile(setup_copy)) {
                if(live->players.size()!=setup_copy.players.size())
                    throw std::runtime_error("Partial VP attack operation state in live world");
                for(const auto &[id,player]:setup_copy.players) {
                    (void)player;
                    if(!live->players.contains(id))
                        throw std::runtime_error("Mismatched VP attack operation roster");
                }
            } else
                runtime::restore_vp_attack_operations_profile(snapshot,setup_copy);
            if(const auto *live=runtime::find_vp_army_construction_profile(setup_copy)) {
                if(live->players.size()!=setup_copy.players.size())
                    throw std::runtime_error("Partial VP army construction state in live world");
                for(const auto &[id,player]:setup_copy.players) {
                    (void)player;
                    if(!live->players.contains(id))
                        throw std::runtime_error("Mismatched VP army construction roster");
                }
            } else
                runtime::restore_vp_army_construction_profile(snapshot,setup_copy);
            if(const auto *live=runtime::find_vp_diplomacy_intrigue_profile(setup_copy)) {
                if(live->major_order.empty()||live->players.size()!=live->major_order.size())
                    throw std::runtime_error("Partial VP intrigue diplomacy profile in live world");
            } else
                runtime::restore_vp_diplomacy_intrigue_profile(snapshot,setup_copy);
        }
        catch(const std::runtime_error &e) { error=e.what(); }
    }
    bool no_policies=false,timer_enabled=false;
    if(setup_copy.profile==content::TechnologyProfile::vp542&&error.empty()) {
        try {
            no_policies=setup_copy.game_option_value(database,"GAMEOPTION_NO_POLICIES")==1;
            timer_enabled=setup_copy.game_option_value(database,"GAMEOPTION_END_TURN_TIMER_ENABLED")==1;
        }
        catch(const std::runtime_error &e) { error=e.what(); }
    }
    auto state=source_turn;
    auto players=source_players;
    auto science=source_science;
    auto cities=source_cities;
    if(setup_copy.profile==content::TechnologyProfile::vp542&&error.empty()) {
        try {
            bool imported=false,missing=false;
            for(const auto &[owner,player]:cities.players)
                for(const auto &[city_id,city]:player.cities) {
                    (void)city;
                    if(runtime::find_vp_city_espionage(cities,owner,city_id))imported=true;
                    else missing=true;
                }
            if(imported&&missing)
                throw std::runtime_error("Partial VP city espionage state in live world");
            if(!imported&&missing)
                runtime::restore_vp_city_espionage(snapshot,cities);
            if(!runtime::find_vp_city_intrigue_order_profile(cities))
                runtime::restore_vp_city_intrigue_order_profile(snapshot,cities);
        }
        catch(const std::runtime_error &e) { error=e.what(); }
    }
    auto treasury=source_treasury;
    auto maintenance=source_maintenance;
    auto teams=source_teams;
    runtime::MaintenanceWorld world{setup_copy,players,cities,teams};
    ResearchTurnProbeWorld research_host;
    TreasuryTurnProbeWorld gold_host;
    const auto active_team=setup_copy.players.at(active_player).team;
    runtime::ResearchProgressContext progress{players,setup_copy,research_rules,technologies,defines,
        options,teams,research_host,active_team,science.no_science};
    runtime::TreasuryTurnContext gold{treasury,world,maintenance,treasury_rules,city_rules,
        maintenance_rules,defines,options,gold_host,active_player,{}};
    runtime::ResearchTurnContext research{state,progress,science,world,treasury,maintenance,
        treasury_rules,city_rules,maintenance_rules,research_host,active_player,{}};
    const auto before_balance=treasury.players.at(active_player).balance_times100;
    const auto technology=players.players.at(active_player).current_research();
    const auto progress_at=[&](const auto &values)->std::optional<std::int64_t> {
        if (technology<0) return {};
        for (const auto &team:values)
            if (team.team()==active_team)
                return team.progress_times100(technology);
        throw std::runtime_error("Active player's research team is missing");
    };
    const auto before_progress=progress_at(source_teams);
    std::optional<runtime::PlayerEconomyResearchStageResult> result;
    std::optional<runtime::VpVassalTaxTurnResult> vp_vassal_tax;
    std::optional<std::int64_t> vp_culture_history_delta;
    std::optional<std::int64_t> vp_culture_cities_times100;
    std::string vp_culture_cities_error;
    std::optional<runtime::VpCultureResourceTraitsQuote> vp_culture_traits;
    std::optional<runtime::VpCultureMinorIncomeQuote> vp_culture_minors;
    std::optional<std::int64_t> vp_culture_happiness;
    std::optional<runtime::VpCultureTurnControls> vp_culture_controls;
    std::optional<runtime::VpCultureVassalQuote> vp_culture_vassals;
    std::optional<runtime::VpCultureReligionRules> vp_culture_religion_rules;
    std::optional<runtime::VpCultureReligionQuote> vp_culture_religion;
    std::optional<runtime::VpCultureResolvedIncome> vp_culture_full;
    std::optional<runtime::VpCultureAccrualResult> vp_culture_accrual;
    std::optional<runtime::VpPaidPolicyCount> vp_paid_policies;
    std::optional<runtime::VpPolicyCityModifierQuote> vp_policy_city_modifier;
    std::optional<runtime::VpPolicyCostSubtotal> vp_policy_subtotal;
    std::optional<runtime::VpPolicyIdeologyQuote> vp_policy_ideology;
    std::optional<runtime::VpPolicyFinalQuote> vp_policy_final;
    std::optional<runtime::VpPolicyCostUpdateResult> vp_policy_update;
    std::optional<runtime::VpAnarchyTurnResult> vp_anarchy;
    std::optional<runtime::VpPolicyPhaseQuote> vp_policy_phase;
    std::optional<runtime::VpEspionageTurnFrontier> vp_espionage_frontier;
    std::optional<runtime::VpSpyCityBinding> vp_espionage_city;
    std::string vp_espionage_city_error;
    std::optional<runtime::VpNetworkPointPreview> vp_network_points_saved_rate;
    std::string vp_network_points_saved_rate_error;
    std::optional<runtime::VpNetworkPointRateInputs> vp_network_point_live_inputs;
    std::optional<runtime::VpNetworkPointRateQuote> vp_network_point_live_rate;
    std::string vp_network_point_live_rate_error;
    std::optional<runtime::VpNetworkPointPreview> vp_network_points_live_rate_preview;
    std::string vp_network_points_live_rate_preview_error;
    std::optional<std::int64_t> vp_saved_next_policy_cost;
    std::string vp_culture_traits_error,vp_culture_minors_error,vp_culture_happiness_error;
    std::string vp_culture_controls_error,vp_culture_vassals_error,vp_culture_religion_error;
    std::string vp_culture_full_error;
    std::string vp_paid_policies_error;
    std::string vp_policy_city_modifier_error;
    std::string vp_policy_subtotal_error;
    std::string vp_policy_ideology_error;
    std::string vp_policy_final_error;
    std::string vp_policy_update_error;
    std::string vp_anarchy_error;
    std::string vp_policy_phase_error;
    if(const auto *history=runtime::find_vp_culture_history_profile(setup_copy);
       history&&history->players.contains(active_player))
        vp_saved_next_policy_cost=history->players.at(active_player).next_policy_cost;
    if(setup_copy.profile==content::TechnologyProfile::vp542) {
        try { vp_culture_religion_rules=runtime::load_vp_culture_religion_rules(database); }
        catch(const std::exception &e) {
            vp_culture_religion_error=e.what();
            vp_culture_full_error=e.what();
        }
    }
    runtime::GameCoreAfterGoldStep after_gold{setup_copy.profile,
        [&](runtime::TreasuryTurnContext &context,std::int64_t player) {
            if(setup_copy.profile==content::TechnologyProfile::vp542) {
                vp_vassal_tax=runtime::do_vp_vassal_tax_after_gold(context,player);
                vp_culture_history_delta=runtime::do_vp_culture_history_after_gold(setup_copy,player);
                try {
                    std::int64_t cities_total=0;
                    for(const auto &entry:context.world.cities.players.at(player).cities)
                        cities_total=runtime::wide::add(cities_total,runtime::city_yield_times100(
                            context.world.cities,city_rules,defines,player,entry.first,culture_yield).total_times100);
                    vp_culture_cities_times100=cities_total;
                } catch(const std::exception &e) { vp_culture_cities_error=e.what(); }
                try { vp_culture_traits=runtime::vp_culture_resource_traits_times100(context,player); }
                catch(const std::exception &e) { vp_culture_traits_error=e.what(); }
                try { vp_culture_minors=runtime::vp_culture_minor_income_times100(context,player,culture_yield); }
                catch(const std::exception &e) { vp_culture_minors_error=e.what(); }
                try { vp_culture_happiness=runtime::vp_culture_happiness_times100(context,player); }
                catch(const std::exception &e) { vp_culture_happiness_error=e.what(); }
                try { vp_culture_controls=runtime::vp_culture_turn_controls(context,player,no_policies); }
                catch(const std::exception &e) { vp_culture_controls_error=e.what(); }
                try { vp_culture_vassals=runtime::vp_culture_vassals_times100(context,science,player,no_policies); }
                catch(const std::exception &e) { vp_culture_vassals_error=e.what(); }
                if(vp_culture_religion_rules) {
                    try { vp_culture_religion=runtime::vp_culture_religion(context,*vp_culture_religion_rules,player); }
                    catch(const std::exception &e) { vp_culture_religion_error=e.what(); }
                    try {
                        const bool minor=std::find(treasury.minor_players.begin(),
                            treasury.minor_players.end(),player)!=treasury.minor_players.end();
                        auto turn=runtime::do_vp_culture_income_after_gold(setup_copy,context,science,
                            *vp_culture_religion_rules,culture_yield,player,
                            {no_policies,timer_enabled,minor});
                        vp_culture_full=std::move(turn.resolved);
                        vp_culture_accrual=turn.accrual;
                        vp_culture_vassals=runtime::vp_culture_vassals_times100(context,science,player,no_policies,
                            [&](std::int64_t vassal)->std::optional<std::int64_t> {
                                const auto it=vp_culture_full->quotes_by_player.find(vassal);
                                if(it==vp_culture_full->quotes_by_player.end())return {};
                                return it->second.total_times100;
                            });
                        vp_culture_vassals_error.clear();
                    }
                    catch(const std::exception &e) { vp_culture_full_error=e.what(); }
                }
                try { vp_paid_policies=runtime::vp_paid_policy_count(setup_copy,database,player); }
                catch(const std::exception &e) { vp_paid_policies_error=e.what(); }
                try {
                    if(!options||!options->all_succeeded())
                        throw std::runtime_error("Unresolved VP policy cost custom options");
                    vp_policy_city_modifier=runtime::vp_policy_city_modifier_times100(
                        setup_copy,players,defines,options->enabled("BALANCE_VP"),player);
                } catch(const std::exception &e) { vp_policy_city_modifier_error=e.what(); }
                try {
                    if(!options||!options->all_succeeded())
                        throw std::runtime_error("Unresolved VP policy cost custom options");
                    vp_policy_subtotal=runtime::vp_policy_cost_subtotal(
                        setup_copy,players,database,defines,options->enabled("BALANCE_VP"),player);
                } catch(const std::exception &e) { vp_policy_subtotal_error=e.what(); }
                if(vp_policy_subtotal)try {
                    vp_policy_ideology=runtime::vp_policy_ideology_cost(
                        setup_copy,database,defines,options->enabled("BALANCE_VP"),
                        player,vp_policy_subtotal->after_speed);
                } catch(const std::exception &e) { vp_policy_ideology_error=e.what(); }
                if(vp_policy_subtotal&&vp_policy_ideology)try {
                    vp_policy_final=runtime::vp_policy_final_cost(
                        setup_copy,players,database,defines,
                        options->enabled("HUMAN_USES_AI_HANDICAP"),player,
                        vp_policy_ideology->after_ideology,vp_policy_subtotal->counted_policies);
                } catch(const std::exception &e) { vp_policy_final_error=e.what(); }
                if(vp_policy_final&&vp_culture_accrual)try {
                    vp_policy_update=runtime::do_vp_policy_cost_after_culture(
                        setup_copy,player,*vp_policy_final);
                } catch(const std::exception &e) { vp_policy_update_error=e.what(); }
                if(vp_policy_update)try {
                    vp_anarchy=runtime::do_vp_anarchy_after_policy_cost(setup_copy,context,player);
                } catch(const std::exception &e) {
                    vp_anarchy_error=e.what();
                    throw;
                }
                if(!vp_culture_accrual||!vp_policy_update||!vp_anarchy)
                    throw std::runtime_error("VP culture or policy phase is unresolved before research");
                try {
                    if(!options||!options->all_succeeded())
                        throw std::runtime_error("Unresolved VP policy phase custom options");
                    vp_policy_phase=runtime::vp_policy_phase_without_host(
                        setup_copy,players,database,*options,player);
                    if(!vp_policy_phase->safe_no_effect)
                        throw std::runtime_error(vp_policy_phase->unresolved);
                } catch(const std::exception &e) {
                    vp_policy_phase_error=e.what();
                    throw;
                }
            }
        }};
    if(error.empty()) {
        try { result=runtime::do_player_economy_research_stage(gold,research,active_player,after_gold); }
        catch (const runtime::TreasuryUnavailable &e) { error=e.what(); }
        catch (const runtime::ResearchTurnUnavailable &e) { error=e.what(); }
        catch (const runtime::ScienceUnavailable &e) { error=e.what(); }
        catch (const runtime::MaintenanceUnavailable &e) { error=e.what(); }
        catch (const runtime::CityYieldUnavailable &e) { error=e.what(); }
        catch (const std::runtime_error &e) { error=e.what(); }
    }
    if(result&&setup_copy.profile==content::TechnologyProfile::vp542) {
        vp_espionage_frontier=runtime::vp_espionage_turn_frontier(setup_copy,active_player);
        if(vp_espionage_frontier->first_spy_needing_host)try {
            const auto &spy=runtime::find_vp_espionage_profile(setup_copy)->players.at(active_player)
                .spies.at(*vp_espionage_frontier->first_spy_needing_host);
            vp_espionage_city=runtime::vp_spy_city_binding(cities,active_player,
                *vp_espionage_frontier->first_spy_needing_host,spy.city_x,spy.city_y);
            if(!vp_espionage_city)vp_espionage_city_error="VP spy target city is absent";
        } catch(const std::exception &e) { vp_espionage_city_error=e.what(); }
        if(vp_espionage_city&&vp_espionage_frontier->first_spy_state==3&&
           vp_espionage_city->rate>0)try {
            const auto rules=runtime::load_vp_network_point_rules(database,setup_copy);
            const auto *activity=runtime::find_vp_city_espionage(cities,
                vp_espionage_city->owner,vp_espionage_city->city);
            vp_network_points_saved_rate=runtime::preview_vp_network_points(
                *activity,active_player,vp_espionage_city->rate,rules);
        } catch(const std::exception &e) { vp_network_points_saved_rate_error=e.what(); }
        if(vp_espionage_city&&(vp_espionage_frontier->first_spy_state==3||
                               vp_espionage_frontier->first_spy_state==7))try {
            if(!options)throw std::runtime_error("Missing selected VP custom options");
            vp_network_point_live_inputs=runtime::bind_vp_network_point_rate_inputs(
                world,treasury,treasury_rules,database,defines,*options,
                active_player,*vp_espionage_frontier->first_spy_needing_host,
                *vp_espionage_city);
            vp_network_point_live_rate=runtime::calculate_vp_network_point_rate(
                *vp_network_point_live_inputs,runtime::load_vp_network_point_rate_rules(defines));
        } catch(const std::exception &e) { vp_network_point_live_rate_error=e.what(); }
        if(vp_network_point_live_rate&&vp_espionage_frontier->first_spy_state==3&&
           vp_network_point_live_rate->rate>0)try {
            const auto rules=runtime::load_vp_network_point_rules(database,setup_copy);
            const auto *activity=runtime::find_vp_city_espionage(cities,
                vp_espionage_city->owner,vp_espionage_city->city);
            vp_network_points_live_rate_preview=runtime::preview_vp_network_points(
                *activity,active_player,vp_network_point_live_rate->rate,rules);
        } catch(const std::exception &e) { vp_network_points_live_rate_preview_error=e.what(); }
    }
    std::optional<runtime::VpGatheringIntelStep> vp_gathering_intel_safe_step;
    std::string vp_gathering_intel_safe_step_error;
    if(result&&setup_copy.profile==content::TechnologyProfile::vp542&&
       vp_espionage_frontier&&vp_espionage_frontier->first_spy_needing_host&&
       vp_espionage_frontier->first_spy_state==3)try {
        if(!options)throw std::runtime_error("Missing selected VP custom options");
        auto spy_cities=cities;
        const runtime::MaintenanceWorld spy_world{setup_copy,players,spy_cities,teams};
        vp_gathering_intel_safe_step=runtime::process_vp_gathering_intel_without_effect_host(
            spy_world,spy_cities,treasury,treasury_rules,database,defines,*options,
            active_player,*vp_espionage_frontier->first_spy_needing_host);
    } catch(const std::exception &e) { vp_gathering_intel_safe_step_error=e.what(); }
    std::optional<runtime::VpEspionagePlayerTurnSegment> vp_espionage_segment;
    std::string vp_espionage_segment_error;
    if(result&&setup_copy.profile==content::TechnologyProfile::vp542)try {
        if(!options)throw std::runtime_error("Missing selected VP custom options");
        auto spy_setup=setup_copy;
        auto spy_cities=cities;
        vp_espionage_segment=runtime::process_vp_espionage_player_turn_segment(
            spy_setup,spy_cities,players,teams,treasury,treasury_rules,database,defines,
            *options,active_player);
    } catch(const std::exception &e) { vp_espionage_segment_error=e.what(); }
    out << "{\"full_turn\":false,\"intervening_turn_phases_complete\":false,\"world_effects_complete\":false,"
           "\"lua_income_services_bound\":false,\"disposable_copy\":true,\"steam_called\":false,"
           "\"active_player\":" << active_player << ",\"status\":"
        << std::quoted(result?"applied":"unresolved") << ",\"error\":" << std::quoted(error)
        << ",\"gold_before_times100\":" << before_balance
        << ",\"gold_after_times100\":" << treasury.players.at(active_player).balance_times100
        << ",\"technology\":" << technology << ",\"progress_before_times100\":";
    if(before_progress)out<<*before_progress;else out<<"null";
    out<<",\"progress_after_times100\":";
    if(const auto after=progress_at(teams))out<<*after;else out<<"null";
    out<<",\"gold_net_times100\":";
    if(result)out<<result->gold.net_times100;else out<<"null";
    out<<",\"vp_vassal_tax_expense_times100\":";
    if(result&&vp_vassal_tax)out<<vp_vassal_tax->expense_times100;
    else out<<"null";
    out<<",\"vp_culture_history_delta_times100\":";
    if(result&&vp_culture_history_delta)out<<*vp_culture_history_delta;
    else out<<"null";
    out<<",\"vp_culture_income_components\":";
    const auto *saved_culture=runtime::find_vp_culture_income_saved_profile(setup_copy);
    if(saved_culture&&saved_culture->players.contains(active_player)) {
        const auto &saved=saved_culture->players.at(active_player);
        out<<"{\"cities_times100\":";
        if(vp_culture_cities_times100)out<<*vp_culture_cities_times100;else out<<"null";
        out<<",\"cities_error\":"<<std::quoted(vp_culture_cities_error)
           <<",\"traits_times100\":";
        if(vp_culture_traits)out<<vp_culture_traits->total_times100;else out<<"null";
        out<<",\"traits_error\":"<<std::quoted(vp_culture_traits_error)
           <<",\"minor_total_times100\":";
        if(vp_culture_minors)out<<vp_culture_minors->total_times100;else out<<"null";
        out<<",\"minor_error\":"<<std::quoted(vp_culture_minors_error)
           <<",\"happiness_times100\":";
        if(vp_culture_happiness)out<<*vp_culture_happiness;else out<<"null";
        out<<",\"happiness_error\":"<<std::quoted(vp_culture_happiness_error)
           <<",\"turn_controls\":";
        if(vp_culture_controls) {
            const auto &v=*vp_culture_controls;
            out<<"{\"no_policies\":"<<(v.no_policies?"true":"false")
               <<",\"anarchy\":"<<(v.anarchy?"true":"false")
               <<",\"golden_age\":"<<(v.golden_age?"true":"false")
               <<",\"golden_age_disabled\":"<<(v.golden_age_disabled?"true":"false")
               <<",\"temporary_bonus\":"<<(v.temporary_bonus?"true":"false")
               <<",\"conquest_bonus\":"<<(v.conquest_bonus?"true":"false")
               <<",\"temporary_boost_modifier\":"<<v.temporary_boost_modifier
               <<",\"conquest_modifier\":"<<v.conquest_modifier
               <<",\"golden_age_modifier\":"<<v.golden_age_modifier<<'}';
        } else out<<"null";
        out<<",\"turn_controls_error\":"<<std::quoted(vp_culture_controls_error)
           <<",\"vassals_times100\":";
        if(vp_culture_vassals)out<<vp_culture_vassals->total_times100;else out<<"null";
        out<<",\"vassals_error\":"<<std::quoted(vp_culture_vassals_error)
           <<",\"religion_base_times100\":";
        if(vp_culture_religion)out<<vp_culture_religion->base_times100;else out<<"null";
        out<<",\"religion_modifier\":";
        if(vp_culture_religion)out<<vp_culture_religion->modifier;else out<<"null";
        out<<",\"religion_error\":"<<std::quoted(vp_culture_religion_error)
           <<",\"full_quote_times100\":";
        if(vp_culture_full)out<<vp_culture_full->quote.total_times100;else out<<"null";
        out<<",\"full_quote_player_count\":";
        if(vp_culture_full)out<<vp_culture_full->quotes_by_player.size();else out<<"null";
        out<<",\"full_quote_error\":"<<std::quoted(vp_culture_full_error)
           <<",\"full_quote_applied\":"<<(vp_culture_accrual?"true":"false")
           <<",\"culture_accrual\":";
        if(vp_culture_accrual) {
            const auto &a=*vp_culture_accrual;
            out<<"{\"credited_times100\":"<<a.credited_times100
               <<",\"balance_before_times100\":"<<a.balance_before_times100
               <<",\"balance_after_times100\":"<<a.balance_after_times100
               <<",\"lifetime_before_times100\":"<<a.lifetime_before_times100
               <<",\"lifetime_after_times100\":"<<a.lifetime_after_times100
               <<",\"timer_blocked\":"<<(a.timer_blocked?"true":"false")<<'}';
        } else out<<"null";
        out<<",\"free_times100\":"<<saved.free_times100
           <<",\"jfd_minors_times100\":"<<saved.jfd_minors_times100
           <<",\"annexed_minors_times100\":"<<saved.annexed_minors_times100
           <<",\"incoming_espionage_times100\":"<<saved.incoming_espionage_times100
           <<",\"outgoing_espionage_times100\":"<<saved.outgoing_espionage_times100
           <<",\"happiness_to_culture\":"<<saved.happiness_to_culture
           <<",\"per_trade_partner\":"<<saved.per_trade_partner
           <<",\"per_export\":"<<saved.per_export
           <<",\"per_import\":"<<saved.per_import
           <<",\"temporary_bonus_turns\":"<<saved.temporary_bonus_turns
           <<",\"conquest_bonus_turns\":"<<saved.conquest_bonus_turns
           <<",\"conquest_modifier\":"<<saved.conquest_modifier
           <<",\"golden_age_disabled\":"<<(saved.golden_age_disabled?"true":"false")<<'}';
    } else out<<"null";
    out<<",\"vp_policy_cost_inputs\":";
    const auto *policy_profile=runtime::find_vp_policy_cost_saved_profile(setup_copy);
    if(policy_profile&&policy_profile->players.contains(active_player)) {
        const auto &saved=policy_profile->players.at(active_player);
        out<<"{\"owned\":";
        if(vp_paid_policies)out<<vp_paid_policies->owned;else out<<"null";
        out<<",\"free\":";
        if(vp_paid_policies)out<<vp_paid_policies->free;else out<<"null";
        out<<",\"dummy\":";
        if(vp_paid_policies)out<<vp_paid_policies->dummy;else out<<"null";
        out<<",\"paid\":";
        if(vp_paid_policies)out<<vp_paid_policies->paid;else out<<"null";
        out<<",\"error\":"<<std::quoted(vp_paid_policies_error)
           <<",\"saved_num_policies\":"<<saved.saved_num_policies
           <<",\"free_policies\":"<<saved.free_policies
           <<",\"free_policies_ever\":"<<saved.free_policies_ever
           <<",\"free_tenets\":"<<saved.free_tenets
           <<",\"policy_cost_modifier\":"<<saved.policy_cost_modifier
           <<",\"city_cost_discount\":"<<saved.city_cost_discount
           <<",\"world_city_modifier_times100\":"<<policy_profile->world_city_modifier_times100
           <<",\"city_modifier_times100\":";
        if(vp_policy_city_modifier)out<<vp_policy_city_modifier->total_times100;else out<<"null";
        out<<",\"effective_cities\":";
        if(vp_policy_city_modifier)out<<vp_policy_city_modifier->effective_cities;else out<<"null";
        out<<",\"city_modifier_error\":"<<std::quoted(vp_policy_city_modifier_error)
           <<",\"subtotal\":";
        if(vp_policy_subtotal) {
            const auto &q=*vp_policy_subtotal;
            out<<"{\"counted_policies\":"<<q.counted_policies
               <<",\"charged_policies\":"<<q.charged_policies
               <<",\"exponent_input\":"<<q.exponent_input
               <<",\"exponent_cost\":"<<q.exponent_cost
               <<",\"base_cost\":"<<q.base_cost
               <<",\"after_cities\":"<<q.after_cities
               <<",\"after_player_modifier\":"<<q.after_player_modifier
               <<",\"speed_percent\":"<<q.speed_percent
               <<",\"after_speed\":"<<q.after_speed<<'}';
        } else out<<"null";
        out<<",\"subtotal_error\":"<<std::quoted(vp_policy_subtotal_error)
           <<",\"ideology\":";
        if(vp_policy_ideology) {
            const auto &q=*vp_policy_ideology;
            out<<"{\"tier1\":"<<q.tier1<<",\"tier2\":"<<q.tier2
               <<",\"tier3\":"<<q.tier3
               <<",\"penalty_percent\":"<<q.penalty_percent
               <<",\"after_ideology\":"<<q.after_ideology<<'}';
        } else out<<"null";
        out<<",\"ideology_error\":"<<std::quoted(vp_policy_ideology_error)
           <<",\"final_quote\":";
        if(vp_policy_final) {
            const auto &q=*vp_policy_final;
            out<<"{\"major\":"<<(q.major?"true":"false")
               <<",\"human_for_handicap\":"<<(q.human_for_handicap?"true":"false")
               <<",\"player_handicap\":"<<q.player_handicap
               <<",\"game_handicap\":"<<q.game_handicap
               <<",\"player_percent\":"<<q.player_percent
               <<",\"player_era_factor\":"<<q.player_era_factor
               <<",\"after_player_handicap\":"<<q.after_player_handicap
               <<",\"ai_percent\":"<<q.ai_percent
               <<",\"ai_era_factor\":"<<q.ai_era_factor
               <<",\"after_ai_handicap\":"<<q.after_ai_handicap
               <<",\"catchup_modifier\":"<<q.catchup_modifier
               <<",\"higher_players\":"<<q.higher_players
               <<",\"possible_players\":"<<q.possible_players
               <<",\"catchup_factor\":"<<q.catchup_factor
               <<",\"after_catchup\":"<<q.after_catchup
               <<",\"visible_divisor\":"<<q.visible_divisor
               <<",\"cost\":"<<q.cost<<'}';
        } else out<<"null";
        out<<",\"final_quote_error\":"<<std::quoted(vp_policy_final_error)
           <<",\"saved_next_policy_cost\":";
        if(vp_saved_next_policy_cost)out<<*vp_saved_next_policy_cost;else out<<"null";
        out<<",\"updated_next_policy_cost\":";
        if(vp_policy_update)out<<vp_policy_update->after;else out<<"null";
        out<<",\"update_error\":"<<std::quoted(vp_policy_update_error)
           <<",\"recalculated\":"<<(vp_policy_update?"true":"false")<<'}';
    } else out<<"null";
    out<<",\"vp_anarchy\":";
    if(vp_anarchy) {
        const auto &a=*vp_anarchy;
        out<<"{\"before\":"<<a.before<<",\"after\":"<<a.after
           <<",\"changed\":"<<(a.changed?"true":"false")
           <<",\"event_ended\":"<<(a.event_ended?"true":"false")
           <<",\"ui_dirty\":"<<(a.ui_dirty?"true":"false")<<'}';
    } else out<<"null";
    out<<",\"vp_anarchy_error\":"<<std::quoted(vp_anarchy_error);
    out<<",\"vp_policy_phase\":";
    if(vp_policy_phase) {
        const auto &p=*vp_policy_phase;
        out<<"{\"outer_guard\":"<<(p.outer_guard?"true":"false")
           <<",\"human_policy_choice\":"<<(p.human_policy_choice?"true":"false")
           <<",\"human_diplomacy\":"<<(p.human_diplomacy?"true":"false")
           <<",\"vassal\":"<<(p.vassal?"true":"false")
           <<",\"affordable\":"<<(p.affordable?"true":"false")
           <<",\"ideology_branch\":"<<p.ideology_branch
           <<",\"culture_balance_times100\":"<<p.culture_balance_times100
           <<",\"next_policy_cost\":"<<p.next_policy_cost
           <<",\"safe_no_effect\":"<<(p.safe_no_effect?"true":"false")
           <<",\"unresolved\":"<<std::quoted(p.unresolved)<<'}';
    } else out<<"null";
    out<<",\"vp_policy_phase_error\":"<<std::quoted(vp_policy_phase_error);
    out<<",\"vp_attack_operations\":";
    if(const auto *operations=runtime::find_vp_attack_operations_profile(setup_copy)) {
        const auto &attacks=operations->players.at(active_player);
        out<<"{\"profile\":\"vp542\",\"source\":\"saved_operations\",\"attacks\":[";
        for(std::size_t i=0;i<attacks.size();++i) {
            if(i)out<<',';
            const auto &attack=attacks[i];
            out<<"{\"source_index\":"<<attack.source_index<<",\"id\":"<<attack.id
               <<",\"type\":"<<attack.type<<",\"owner\":"<<attack.owner
               <<",\"enemy\":"<<attack.enemy<<",\"state\":"<<attack.state
               <<",\"target_x\":"<<attack.target_x<<",\"target_y\":"<<attack.target_y<<'}';
        }
        out<<"]}";
    } else out<<"null";
    out<<",\"vp_espionage_frontier\":";
    if(vp_espionage_frontier) {
        const auto &e=*vp_espionage_frontier;
        out<<"{\"spy_count\":"<<e.spy_count
           <<",\"queued_messages\":"<<e.queued_messages
           <<",\"queued_intrigue\":"<<e.queued_intrigue
           <<",\"first_spy_needing_host\":";
        if(e.first_spy_needing_host)out<<*e.first_spy_needing_host;else out<<"null";
        out<<",\"first_spy_state\":"<<e.first_spy_state
           <<",\"safe_no_gameplay_effect\":"<<(e.safe_no_gameplay_effect?"true":"false")
           <<",\"unresolved\":"<<std::quoted(e.unresolved)<<'}';
    } else out<<"null";
    out<<",\"vp_espionage_city\":";
    if(vp_espionage_city) {
        const auto &c=*vp_espionage_city;
        out<<"{\"owner\":"<<c.owner<<",\"city\":"<<c.city
           <<",\"x\":"<<c.x<<",\"y\":"<<c.y
           <<",\"assignment\":"<<c.assignment
           <<",\"amount\":"<<c.amount<<",\"max_amount\":"<<c.max_amount
           <<",\"rate\":"<<c.rate<<",\"goal\":"<<c.goal<<'}';
    } else out<<"null";
    out<<",\"vp_espionage_city_error\":"<<std::quoted(vp_espionage_city_error);
    out<<",\"vp_network_points_saved_rate_preview\":";
    if(vp_network_points_saved_rate) {
        const auto &q=*vp_network_points_saved_rate;
        out<<"{\"diagnostic_only\":true,\"rate\":"<<q.rate
           <<",\"before\":"<<q.before<<",\"after\":"<<q.after
           <<",\"max_before\":"<<q.max_before<<",\"max_after\":"<<q.max_after
           <<",\"next_passive_bonus\":";
        if(q.next_passive_bonus)out<<*q.next_passive_bonus;else out<<"null";
        out<<",\"passive_bonuses_unlocked\":[";
        for(std::size_t i=0;i<q.passive_bonuses_unlocked.size();++i) {
            if(i)out<<',';out<<q.passive_bonuses_unlocked[i];
        }
        out<<"],\"missions_unlocked\":[";
        for(std::size_t i=0;i<q.missions_unlocked.size();++i) {
            if(i)out<<',';out<<q.missions_unlocked[i];
        }
        out<<"],\"safe_without_effect_host\":"
           <<(q.safe_without_effect_host?"true":"false")<<'}';
    } else out<<"null";
    out<<",\"vp_network_points_saved_rate_error\":"
       <<std::quoted(vp_network_points_saved_rate_error);
    out<<",\"vp_network_point_live_rate\":";
    if(vp_network_point_live_rate) {
        const auto &q=*vp_network_point_live_rate;
        out<<"{\"diagnostic_only\":true,\"rate\":"<<q.rate
           <<",\"before_security\":"<<q.before_security
           <<",\"security_reduction_percent\":"<<q.security_reduction_percent
           <<",\"saved_rate\":"<<vp_espionage_city->rate
           <<",\"same_as_saved_rate\":"<<(q.rate==vp_espionage_city->rate?"true":"false")
           <<'}';
    } else out<<"null";
    out<<",\"vp_network_point_live_inputs\":";
    if(vp_network_point_live_inputs) {
        const auto &i=*vp_network_point_live_inputs;
        const auto emit=[&](const std::optional<std::int64_t> &value){if(value)out<<*value;else out<<"null";};
        out<<"{\"spy_state\":"<<i.spy_state<<",\"effective_spy_rank\":";emit(i.effective_spy_rank);
        out<<",\"influence_level\":";emit(i.influence_level);
        out<<",\"own_known_techs\":";emit(i.own_known_techs);
        out<<",\"target_known_techs\":";emit(i.target_known_techs);
        out<<",\"player_bonus\":";emit(i.player_bonus);
        out<<",\"religion_bonus\":";emit(i.religion_bonus);
        out<<",\"policy_faster_modifier\":";emit(i.policy_faster_modifier);
        out<<",\"trait_strength_modifier\":";emit(i.trait_strength_modifier);
        out<<",\"counterspy_resolved\":"<<(i.counterspy_resolved?"true":"false")
           <<",\"counterspy_focus_reduction\":";
        if(i.counterspy)out<<i.counterspy->focus_reduction;else out<<"null";
        out<<",\"city_security\":";emit(i.city_security);
        out<<'}';
    } else out<<"null";
    out<<",\"vp_network_point_live_rate_error\":"<<std::quoted(vp_network_point_live_rate_error);
    out<<",\"vp_network_points_live_rate_preview\":";
    if(vp_network_points_live_rate_preview) {
        const auto &q=*vp_network_points_live_rate_preview;
        out<<"{\"diagnostic_only\":true,\"rate\":"<<q.rate
           <<",\"before\":"<<q.before<<",\"after\":"<<q.after
           <<",\"max_before\":"<<q.max_before<<",\"max_after\":"<<q.max_after
           <<",\"safe_without_effect_host\":"
           <<(q.safe_without_effect_host?"true":"false")<<'}';
    } else out<<"null";
    out<<",\"vp_network_points_live_rate_preview_error\":"
       <<std::quoted(vp_network_points_live_rate_preview_error);
    out<<",\"vp_gathering_intel_safe_step\":";
    if(vp_gathering_intel_safe_step) {
        const auto &step=*vp_gathering_intel_safe_step;
        out<<"{\"diagnostic_only\":true,\"spy_owner\":"<<step.spy_owner
           <<",\"spy_index\":"<<step.spy_index
           <<",\"target_owner\":"<<step.target_owner
           <<",\"city\":"<<step.city
           <<",\"rate\":"<<step.rate.rate
           <<",\"before\":";
        if(step.network_points)out<<step.network_points->before;else out<<"null";
        out<<",\"after\":";
        if(step.network_points)out<<step.network_points->after;else out<<"null";
        out<<'}';
    } else out<<"null";
    out<<",\"vp_gathering_intel_safe_step_error\":"
       <<std::quoted(vp_gathering_intel_safe_step_error);
    out<<",\"vp_espionage_segment\":";
    if(vp_espionage_segment) {
        const auto &segment=*vp_espionage_segment;
        out<<"{\"diagnostic_only\":true,\"complete\":"
           <<(segment.complete?"true":"false")
           <<",\"processed_spies\":"<<segment.processed_spies
           <<",\"blocking_spy\":";
        if(segment.blocking_spy)out<<*segment.blocking_spy;else out<<"null";
        out<<",\"blocking_state\":"<<segment.blocking_state
           <<",\"unresolved\":"<<std::quoted(segment.unresolved)
           <<",\"gathering_steps\":[";
        for(std::size_t i=0;i<segment.gathering_steps.size();++i) {
            const auto &step=segment.gathering_steps[i];
            if(i)out<<',';
            out<<"{\"spy_index\":"<<step.spy_index
               <<",\"city\":"<<step.city<<",\"rate\":"<<step.rate.rate
               <<",\"before\":";
            if(step.network_points)out<<step.network_points->before;else out<<"null";
            out<<",\"after\":";
            if(step.network_points)out<<step.network_points->after;else out<<"null";
            out<<'}';
        }
        out<<"],\"diplomat_steps\":[";
        for(std::size_t i=0;i<segment.diplomat_steps.size();++i) {
            const auto &step=segment.diplomat_steps[i];
            if(i)out<<',';
            out<<"{\"spy_index\":"<<step.spy_index
               <<",\"city\":"<<step.city<<",\"rate\":"<<step.rate.rate
               <<",\"no_sneak_attack_operations\":"
               <<(step.no_sneak_attack_operations?"true":"false")
               <<",\"target_army_count\":";
            if(step.target_army_count)out<<*step.target_army_count;else out<<"null";
            const auto boolean=[&](const char *name,const std::optional<bool> &value) {
                out<<",\""<<name<<"\":";
                if(value)out<<(*value?"true":"false");else out<<"null";
            };
            boolean("building_land_army",step.building_land_army);
            boolean("building_naval_army",step.building_naval_army);
            boolean("building_combined_army",step.building_combined_army);
            boolean("target_human_for_units",step.target_human_for_units);
            out<<",\"military_might_branch_skipped\":"
               <<(step.military_might_branch_skipped?"true":"false");
            out<<",\"building_army_intrigue_candidate\":";
            if(step.building_army_intrigue_candidate.empty())out<<"null";
            else out<<std::quoted(step.building_army_intrigue_candidate);
            boolean("building_army_intrigue_duplicate",step.building_army_intrigue_duplicate);
            out<<",\"construction_cities_scanned\":";
            if(step.construction_scan)out<<step.construction_scan->scanned_cities;
            else out<<"null";
            out<<",\"construction_candidate\":";
            if(step.construction_scan&&step.construction_scan->candidate) {
                const auto &candidate=*step.construction_scan->candidate;
                out<<"{\"city\":"<<candidate.city<<",\"x\":"<<candidate.x
                   <<",\"y\":"<<candidate.y<<",\"order_type\":"<<candidate.order_type
                   <<",\"item\":"<<candidate.item<<'}';
            } else out<<"null";
            out<<",\"target_human_diplomacy\":";
            if(step.diplomatic_scan)out<<(step.diplomatic_scan->target_human_diplomacy?"true":"false");
            else out<<"null";
            out<<",\"diplomatic_active_majors\":";
            if(step.diplomatic_scan)out<<step.diplomatic_scan->active_major_count;else out<<"null";
            out<<",\"coop_pairs_examined\":";
            if(step.diplomatic_scan)out<<step.diplomatic_scan->coop_pairs_examined;else out<<"null";
            out<<",\"deception_players_examined\":";
            if(step.diplomatic_scan)out<<step.diplomatic_scan->deception_players_examined;else out<<"null";
            out<<",\"diplomatic_candidate\":";
            if(step.diplomatic_scan&&step.diplomatic_scan->candidate) {
                const auto &candidate=*step.diplomatic_scan->candidate;
                out<<"{\"kind\":"<<std::quoted(candidate.kind==runtime::VpDiplomaticIntrigueKind::coop_war?
                    "coop_war":"deception")<<",\"other_player\":"<<candidate.other_player
                   <<",\"diplomacy_player\":"<<candidate.diplomacy_player
                   <<",\"reported_player\":"<<candidate.reported_player<<'}';
            } else out<<"null";
            boolean("diplomatic_intrigue_duplicate",step.diplomatic_intrigue_duplicate);
            out
               <<",\"before\":";
            if(step.network_points)out<<step.network_points->before;else out<<"null";
            out<<",\"after\":";
            if(step.network_points)out<<step.network_points->after;else out<<"null";
            out<<",\"unresolved\":"<<std::quoted(step.unresolved)<<'}';
        }
        out<<"],\"travelling_steps\":[";
        for(std::size_t i=0;i<segment.travelling_steps.size();++i) {
            const auto &step=segment.travelling_steps[i];
            if(i)out<<',';
            out<<"{\"spy_index\":"<<step.spy_index<<",\"city\":"<<step.city
               <<",\"before\":"<<step.before<<",\"after\":"<<step.after
               <<",\"rate\":"<<step.rate<<",\"goal\":"<<step.goal
               <<",\"max_before\":"<<step.max_before<<",\"max_after\":"<<step.max_after
               <<",\"arrival_pending\":"<<(step.arrival_pending?"true":"false")
               <<",\"unresolved\":"<<std::quoted(step.unresolved)<<'}';
        }
        out<<"]}";
    } else out<<"null";
    out<<",\"vp_espionage_segment_error\":"<<std::quoted(vp_espionage_segment_error);
    out<<",\"research_beakers_times100\":";
    if(result&&result->research.beakers_times100)out<<*result->research.beakers_times100;
    else out<<"null";
    out<<",\"gold_events\":[";
    for(std::size_t i=0;i<gold_host.events.size();++i){if(i)out<<',';out<<std::quoted(gold_host.events[i]);}
    out<<"],\"research_events\":[";
    for(std::size_t i=0;i<research_host.events.size();++i){if(i)out<<',';out<<std::quoted(research_host.events[i]);}
    out<<"]}";
    return out.str();
}
} // namespace civ5::validation
