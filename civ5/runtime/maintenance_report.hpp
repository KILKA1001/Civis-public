#pragma once
#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/vp_maintenance_profile.hpp"
#include <iomanip>
#include <limits>
#include <sstream>

namespace civ5::validation {
inline std::string maintenance_report(runtime::MaintenanceContext state,const runtime::MaintenanceWorld &w,
    const runtime::MaintenanceRules &rules,const content::GlobalDefineCache &defines,const content::CustomModOptions *options) {
    std::ostringstream out;out << std::setprecision(std::numeric_limits<double>::max_digits10);
    const auto list=[&](const auto &v){out << '[';bool first=true;for(auto n:v){if(!first)out << ',';first=false;out << n;}out << ']';};
    const auto pairs=[&](const auto &v){out << '[';bool first=true;for(auto [id,n]:v){if(!first)out << ',';first=false;out << '[' << id << ',' << n << ']';}out << ']';};
    out << "{\"net_income_calculated\":false,\"cache_updates_are_disposable\":true,\"elapsed_turns\":" << state.elapsed_turns
        << ",\"estimate_end_turn\":" << state.estimate_end_turn << ",\"default_end_turn\":" << state.default_end_turn
        << ",\"start_era\":" << w.setup.start_era << ",\"game_handicap\":" << w.setup.game_handicap
        << ",\"barbarian_team\":" << w.diplomacy.barbarian_team << ",\"options\":{";
    bool first=true;for(const auto *name:{"BALANCE_VP","BALANCE_CORE_JFD","HUMAN_USES_AI_HANDICAP","BALANCE_RESOURCE_SHORTAGE_BUILDING_REFUND"}) {
        if(!first)out << ',';first=false;out << std::quoted(name) << ':' << (options&&options->enabled(name)?"true":"false");
    }
    out << "},\"defines\":{";first=true;
    for(const auto *name:{"UNIT_MAINTENANCE_GAME_MULTIPLIER","UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR","INITIAL_FREE_OUTSIDE_UNITS",
                         "INITIAL_OUTSIDE_UNIT_GOLD_PERCENT","VASSALAGE_VASSAL_UNIT_MAINT_COST_PERCENT","VASSALAGE_VASSAL_CITY_POP_EXPONENT","AI_HANDICAP"}) {
        const auto *d=defines.find(name);if(!d)continue;if(!first)out << ',';first=false;out << std::quoted(name) << ':';
        if(const auto *v=std::get_if<std::int64_t>(&d->lookup.value))out << *v;else out << std::get<double>(d->lookup.value);
    }
    out << "},\"handicaps\":[";first=true;
    for(const auto &[id,h]:rules.handicaps){if(!first)out << ',';first=false;
        out << '[' << id << ',' << h.free_units << ',' << h.ai_free_units << ',' << h.unit << ',' << h.ai_unit << ',' << h.building
            << ',' << h.ai_building << ',' << h.improvement << ',' << h.ai_improvement << ',' << h.inflation << ',' << h.ai_inflation << ',' << h.ai_per_era << ']';}
    out << "],\"unit_infos\":[";first=true;
    for(const auto &[id,u]:rules.units){if(!first)out << ',';first=false;out << '[' << id << ',' << u.domain << ',' << u.combat << ',' << u.range << ',' << u.nuke << ',' << u.no_maintenance << ']';}
    out << "],\"era_building_percent\":";pairs(rules.era_building_percent);out << ",\"combat_classes\":";list(rules.combat_classes);
    out << ",\"contract_types\":";
    if(rules.profile==content::TechnologyProfile::vp542)list(runtime::vp_maintenance_rules_profile(rules).contracts);
    else list(std::set<std::int64_t>{});
    out << ",\"major_players\":";list(w.diplomacy.major_players);
    out << ",\"contracts\":[";first=true;
    if(state.profile==content::TechnologyProfile::vp542)
        for(const auto &c:runtime::vp_maintenance_context_profile(state).contracts){if(!first)out << ',';first=false;out << '[' << c.type << ',' << c.holder << ',' << c.maintenance << ']';}
    out << "],\"teams\":[";first=true;
    for(const auto &[id,t]:w.diplomacy.teams){if(!first)out << ',';first=false;
        out << "{\"id\":" << id << ",\"alive\":" << t.alive_count << ",\"master\":" << t.master << ",\"era\":" << state.team_eras.at(id) << ",\"members\":";list(t.members);
        for(const auto &tech:w.technologies)if(tech.team()==id)out << ",\"known_techs\":" << tech.known_count() << ",\"tech_slots\":" << tech.slots();
        out << '}';}
    out << "],\"players\":[";first=true;
    for(auto &[id,p]:state.players) {
        if(!first)out << ',';first=false;const auto &b=w.setup.players.at(id);
        const auto *vp_profile=state.profile==content::TechnologyProfile::vp542?&runtime::vp_maintenance_profile(p):nullptr;
        out << "{\"id\":" << id << ",\"team\":" << b.team << ",\"handicap\":" << b.handicap
            << ",\"human\":" << (w.setup.is_human(b)?"true":"false") << ",\"alive\":" << (b.alive?"true":"false")
            << ",\"major\":" << (w.diplomacy.team_is_major(w.setup,b.team)?"true":"false");
#define FIELD(n) out << ",\"" #n "\":" << p.n
        FIELD(base_free);FIELD(gold_per_unit_times100);FIELD(extra_unit_cost);FIELD(outside_units);
        FIELD(unit_modifier);FIELD(supply_modifier);FIELD(building_modifier);FIELD(improvement_modifier);
        FIELD(land_modifier);FIELD(naval_modifier);FIELD(base_building);FIELD(base_improvement);
        FIELD(cached_unit_expense);FIELD(cached_supply_expense);
        out << ",\"vassal_modifier\":" << (vp_profile?vp_profile->vassal_modifier:0)
            << ",\"vassal_taxes_times100\":" << (vp_profile?vp_profile->vassal_taxes_times100:0)
            << ",\"outgoing_espionage_gold\":" << (vp_profile?vp_profile->outgoing_espionage_gold:0);
        FIELD(diplomacy_gold);
#undef FIELD
        out << ",\"barbarian\":" << (p.barbarian?"true":"false") << ",\"free_garrison\":" << (p.free_garrison?"true":"false") << ",\"combat_modifiers\":";pairs(p.combat_modifiers);
        out << ",\"active_contracts\":";
        if(vp_profile)list(vp_profile->active_contracts);else list(std::set<std::int64_t>{});
        out << ",\"units\":[";bool first_unit=true;
        for(const auto &[uid,u]:p.units){if(!first_unit)out << ',';first_unit=false;out << '[' << uid << ',' << u.type << ',' << u.domain << ',' << u.combat << ','
            << u.military << ',' << u.civilian << ',' << u.no_maintenance << ',' << (vp_profile&&vp_profile->contract_units.contains(uid)) << ',';
            if(u.garrison)out << *u.garrison;else out << "null";out << ']';}
        out << "],\"unit_source_cores\":[";first_unit=true;
        for(const auto &[uid,u]:p.units)if(u.source_core){if(!first_unit)out << ',';first_unit=false;out << '[' << uid << ',' << u.source_core->first << ',' << u.source_core->second << ']';}
        out << "],\"cities\":[";bool first_city=true;
        for(const auto &[cid,c]:w.cities.players.at(id).cities){if(!first_city)out << ',';first_city=false;
            const auto extra=vp_profile?vp_profile->extra_building_percent.find(cid):runtime::YieldValues::const_iterator{};
            out << '[' << cid << ',' << c.population << ',' << (vp_profile&&extra!=vp_profile->extra_building_percent.end()?extra->second:0) << ']';}
        out << "],\"quote\":{";
        try {
            const auto q=runtime::maintenance_costs(state,w,rules,defines,options,id);
            out << "\"status\":\"resolved\",\"free_units\":" << q.units.free_units << ",\"paid_units\":" << q.units.paid_units
                << ",\"base_times100\":" << q.units.base_times100 << ",\"extra_times100\":" << q.units.extra_times100
                << ",\"multiplier\":" << q.units.multiplier << ",\"exponent\":" << q.units.exponent << ",\"units\":" << q.units.total;
#define FIELD(n) out << ",\"" #n "\":" << q.n
            FIELD(supply);FIELD(buildings);FIELD(improvements);
#undef FIELD
            if(rules.profile==content::TechnologyProfile::vp542) {
                const auto &profile=runtime::vp_maintenance_quote(q);
#define FIELD(n) out << ",\"" #n "\":" << profile.n
                FIELD(vassals);FIELD(taxes);FIELD(contracts);FIELD(espionage);FIELD(diplomacy);
#undef FIELD
            }
            out << ",\"total\":" << q.total;
        } catch(const runtime::MaintenanceUnavailable &e){out << "\"status\":\"unavailable\",\"reason\":" << std::quoted(e.what());}
        out << "},\"unit_expense_after\":" << p.cached_unit_expense << ",\"supply_expense_after\":" << p.cached_supply_expense << '}';
    }
    out << "]}";return out.str();
}
} // namespace civ5::validation
