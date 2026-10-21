#pragma once
#include "civ5/runtime/treasury.hpp"
#include "civ5/runtime/vp_treasury_income_profile.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include <iomanip>
#include <sstream>

namespace civ5::validation {
inline std::string treasury_report(const runtime::TreasuryContext &s,const runtime::MaintenanceWorld &w,
    runtime::MaintenanceContext maintenance,const runtime::TreasuryRules &rules,const runtime::CityYieldRules &city_rules,
    const runtime::MaintenanceRules &maintenance_rules,const content::GlobalDefineCache &d,const content::CustomModOptions *o){
    std::ostringstream out;
    const auto attempt=[](auto &&f){try{f();return std::string();}
        catch(const runtime::TreasuryUnavailable &e){return std::string(e.what());}
        catch(const runtime::MaintenanceUnavailable &e){return std::string(e.what());}
        catch(const runtime::CityYieldUnavailable &e){return std::string(e.what());}};
    const auto list=[&](const auto &v){out << '[';bool first=true;for(auto n:v){if(!first)out << ',';first=false;out << n;}out << ']';};
    const auto pairs=[&](const auto &v){out << '[';bool first=true;for(auto [id,n]:v){if(!first)out << ',';first=false;out << '[' << id << ',' << n << ']';}out << ']';};
    // Unique City States intentionally randomizes MinorCivilizations.ID.
    // Compare every rule and binding by Type, as CvPreGame restores them.
    std::map<std::string,const runtime::MinorYieldRules *> minor_types;
    for(const auto &[id,m]:rules.minors){
        if(m.type.empty()||!minor_types.emplace(m.type,&m).second)
            throw std::runtime_error("Treasury report needs unique nonempty minor types");
    }
    const auto minor_type=[&](std::int64_t id){
        if(id==-1)out << "null";
        else if(const auto i=rules.minors.find(id);i!=rules.minors.end())out << std::quoted(i->second.type);
        else out << "{\"unresolved_id\":" << id << '}';
    };
    const auto income=[&](const runtime::TreasuryIncomeQuote &q){out << '{';
        out << "\"cities\":" << q.cities;
#define FIELD(n) out << ",\"" #n "\":" << q.n
        FIELD(diplomacy);FIELD(connections);FIELD(religion);FIELD(traits);
#undef FIELD
        if(const auto *profile=q.profile_extensions.find<runtime::VpTreasuryIncomeQuote>()) {
#define FIELD(n) out << ",\"" #n "\":" << profile->n
            FIELD(internal_trade);FIELD(vassal_taxes);FIELD(minors);FIELD(jfd_minors);FIELD(annexed_minors);FIELD(espionage);
#undef FIELD
        }
        out << ",\"total_times100\":" << q.total_times100;
        out << '}';};
    out << "{\"net_income_calculated\":true,\"balances_mutated\":false,\"cache_updates_are_disposable\":true,\"lua_events_executed\":false,\"options\":{";
    bool first=true;for(const auto *name:{"BALANCE_VP","BALANCE_CORE_JFD","BALANCE_CITY_STATE_SCALE","BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV"}){
        if(!first)out << ',';first=false;out << std::quoted(name) << ':' << (o&&o->enabled(name)?"true":"false");}
    out << "},\"city_state_scale_per_city\":";
    const auto *scale=d.find("CITY_STATE_SCALE_PER_CITY_MOD");out << (scale?std::get<std::int64_t>(scale->lookup.value):0);
    out << ",\"minor_players\":";list(s.minor_players);out << ",\"luxury_resources\":";list(rules.luxury_resources);
    out << ",\"belief_infos\":[";first=true;
    for(const auto &[id,b]:rules.beliefs){if(!first)out << ',';first=false;
        out << '[' << id << ',' << b.gold_per_city << ',' << b.followers_per_gold << ',' << b.city_state_modifier << ',' << b.civilization
            << ',' << b.founder << ',' << b.enhancer << ',' << b.follower << ',' << b.reformation << ',' << b.pantheon << ']';}
    out << "],\"minor_rules\":[";first=true;
    for(const auto &[type,m]:minor_types)for(bool ally:{false,true})for(const auto &[y,eras]:ally?m->allies:m->friends)for(auto [era,n]:eras){
        if(!first)out << ',';first=false;out << '[' << std::quoted(type) << ',' << ally << ',' << y << ',' << era << ',' << n << ']';}
    out << "],\"minor_info_types\":[";first=true;for(const auto &[type,m]:minor_types){if(!first)out << ',';first=false;out << std::quoted(type);}
    out << "],\"religions\":[";first=true;
    for(const auto &r:s.religions){if(!first)out << ',';first=false;out << "{\"id\":" << r.religion << ",\"founder\":" << r.founder << ",\"belief_religion\":" << r.belief_religion << ",\"pantheon\":" << (r.pantheon?"true":"false") << ",\"holy_plot_owner\":";
        if(r.holy_plot_owner)out << *r.holy_plot_owner;else out << "null";out << ",\"holy_city\":";
        if(r.holy_city)out << '[' << r.holy_city->first << ',' << r.holy_city->second << ']';else out << "null";
        out << ",\"beliefs\":";list(r.beliefs);out << ",\"belief_lookup\":";list(r.belief_lookup);out << '}';}
    out << "],\"trade_routes\":[";first=true;for(const auto &r:s.trade_routes){if(!first)out << ',';first=false;
        out << '[' << r.origin_owner << ',' << r.destination_owner << ',' << r.origin_x << ',' << r.origin_y << ',' << r.destination_x << ',' << r.destination_y << ']';}
    out << "],\"players\":[";first=true;
    for(const auto &[id,p]:s.players){if(!first)out << ',';first=false;out << "{\"id\":" << id;
        for(const auto &[name,range]:std::initializer_list<std::pair<const char *,std::optional<std::pair<std::uint64_t,std::uint64_t>>>>{{"source_core",p.source_core},{"source_administration",p.source_administration}}){
            out << ',' << std::quoted(name) << ':';if(range)out << '[' << range->first << ',' << range->second << ']';else out << "null";
        }
#define FIELD(n) out << ",\"" #n "\":" << p.n
        FIELD(balance_times100);FIELD(connections_times100);
        FIELD(lifetime_gross_gold);
        FIELD(gold_per_partner);
        const auto *minor=s.profile==content::TechnologyProfile::vp542?&runtime::vp_treasury_minor_profile(p):nullptr;
        out << ",\"trait_cs_modifier\":" << (minor?minor->trait_cs_modifier:0);
        out << ",\"cs_modifier\":" << (minor?minor->cs_modifier:0);
        out << ",\"state_religion\":" << (s.profile==content::TechnologyProfile::vp542?runtime::vp_treasury_religion_profile(p).state_religion:-1);
        FIELD(ally);
        out << ",\"permanent_ally\":" << (minor?minor->permanent_ally:-1);
        FIELD(anarchy);
        out << ",\"owns_state_religion\":" << (s.profile==content::TechnologyProfile::vp542?runtime::vp_treasury_religion_profile(p).owns_state_religion:false);
        out << ",\"no_ally\":" << (minor?minor->no_ally:false);
#undef FIELD
        if(s.profile==content::TechnologyProfile::vp542){
            const auto &income=runtime::vp_treasury_income_profile(p);
#define FIELD(n) out << ",\"" #n "\":" << income.n
            FIELD(internal_trade);FIELD(incoming_espionage);FIELD(jfd_minors);FIELD(annexed_minors_times100);
            FIELD(gold_per_export);FIELD(gold_per_import);
#undef FIELD
        }
        out << ",\"minor_type\":";minor_type(minor?minor->minor_type:-1);
        out << ",\"city_majorities\":";if(minor)pairs(minor->city_majorities);else out << "[]";
        if(s.profile==content::TechnologyProfile::vp542){
            const auto &income=runtime::vp_treasury_income_profile(p);
#define PAIRS(n) out << ",\"" #n "\":";pairs(income.n)
            PAIRS(exported);PAIRS(imported);PAIRS(from_minors);PAIRS(from_alliances);PAIRS(siphoned);
#undef PAIRS
        }
        out << ",\"friends\":";if(minor)list(minor->friends);else out << "[]";
        out << ",\"balance_history_times100\":";list(p.balance_history_times100);
        out << ",\"income_history_times100\":";list(p.income_history_times100);
        std::optional<runtime::TreasuryIncomeQuote> gross;
        const auto gross_error=attempt([&]{gross=runtime::gross_gold_times100(s,w,maintenance,rules,city_rules,d,o,id);});
        out << ",\"gross\":";if(gross)income(*gross);else out << "null";out << ",\"gross_error\":" << std::quoted(gross_error);
        std::optional<runtime::TreasuryNetQuote> net;
        const auto net_error=attempt([&]{net=runtime::net_gold_times100(s,w,maintenance,rules,city_rules,maintenance_rules,d,o,id);});
        out << ",\"net\":";
        if(net){const auto &q=*net;out << "{\"anarchy\":" << (q.anarchy?"true":"false") << ",\"net_times100\":" << q.net_times100 << ",\"income\":";income(q.income);
            out << ",\"expenses\":";if(q.expenses)out << q.expenses->total;else out << "null";out << '}';}
        else out << "null";
        out << ",\"net_error\":" << std::quoted(net_error) << ",\"unit_expense_after\":" << maintenance.players.at(id).cached_unit_expense << ",\"supply_expense_after\":" << maintenance.players.at(id).cached_supply_expense << '}';
    }
    out << "]}";return out.str();
}
} // namespace civ5::validation
