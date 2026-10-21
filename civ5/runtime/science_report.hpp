#pragma once
#include "civ5/runtime/science.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include <iomanip>
#include <sstream>

namespace civ5::validation {
inline std::string science_report(const runtime::ScienceContext &s,const runtime::MaintenanceWorld &w,
    const runtime::TreasuryContext &t,runtime::MaintenanceContext m,const runtime::TreasuryRules &tr,
    const runtime::CityYieldRules &cr,const runtime::MaintenanceRules &mr,const content::GlobalDefineCache &d,
    const content::CustomModOptions *o) {
    const bool vp=s.profile==content::TechnologyProfile::vp542;
    auto excluded=m;std::ostringstream out;
    const auto list=[&](const auto &v){out<<'[';bool first=true;for(auto n:v){if(!first)out<<',';first=false;out<<n;}out<<']';};
    const auto quote=[&](std::int64_t id,bool skip,runtime::MaintenanceContext &cache) {
        try {
            const auto q=runtime::science_times100(s,w,t,cache,tr,cr,mr,d,o,id,skip);
            out<<"{\"status\":\"resolved\",\"anarchy\":"<<(q.anarchy?"true":"false")<<",\"research_agreements_excluded\":"<<(q.research_agreements_excluded?"true":"false");
#define Q(n) out<<",\"" #n "\":"<<q.n
            Q(cities);Q(religion);Q(traits);Q(other_players);Q(happiness);Q(agreements);Q(budget_deficit);
#undef Q
            if(vp) {
                const auto &profile=runtime::vp_science_quote(q);
#define Q(n) out<<",\"" #n "\":"<<profile.n
                Q(vassals);Q(jfd_minors);Q(minors);Q(annexed_minors);Q(incoming_espionage);Q(outgoing_espionage);Q(passive_spies);
#undef Q
            }
            out<<",\"total_times100\":"<<q.total_times100;
            out<<",\"gold_rate_times100\":";if(q.gold_rate_times100)out<<*q.gold_rate_times100;else out<<"null";out<<'}';
        } catch(const runtime::ScienceUnavailable &e) {out<<"{\"status\":\"unresolved\",\"error\":"<<std::quoted(e.what())<<'}';}
          catch(const runtime::TreasuryUnavailable &e) {out<<"{\"status\":\"unresolved\",\"error\":"<<std::quoted(e.what())<<'}';}
          catch(const runtime::MaintenanceUnavailable &e) {out<<"{\"status\":\"unresolved\",\"error\":"<<std::quoted(e.what())<<'}';}
          catch(const runtime::CityYieldUnavailable &e) {out<<"{\"status\":\"unresolved\",\"error\":"<<std::quoted(e.what())<<'}';}
    };
    out<<"{\"science_is_calculated\":true,\"research_mutated\":false,\"cache_updates_are_disposable\":true,\"no_science\":"<<(s.no_science?"true":"false")<<",\"defines\":{";
    bool first=true;
    for(const auto *name:{"RESEARCH_AGREEMENT_MOD","RESEARCH_AGREEMENT_PLAYER_AVERAGE_YIELD_PERCENT","RESEARCH_AGREEMENT_BOOST_DIVISOR","RESEARCH_AGREEMENT_PER_TURN_YIELD_PERCENT","MINOR_CIV_SCIENCE_BONUS_MULTIPLIER","VASSALAGE_FREE_YIELD_FROM_VASSAL_PERCENT","UNHAPPY_THRESHOLD"}) {
        const auto *v=d.find(name);if(!v)continue;if(!first)out<<',';first=false;out<<std::quoted(name)<<':'<<std::get<std::int64_t>(v->lookup.value);
    }
    out<<"},\"minor_players\":";list(s.minor_players);out<<",\"barbarian_players\":";list(s.barbarian_players);
    out<<",\"civ_teams\":";list(w.diplomacy.civ_teams);out<<",\"research_agreements\":[";first=true;
    for(const auto &[id,v]:s.research_agreements){if(!first)out<<',';first=false;out<<'['<<id<<',';list(v);out<<']';}
    out<<"],\"players\":[";first=true;
    for(const auto &[id,p]:s.players) {
        if(!first)out<<',';first=false;out<<"{\"id\":"<<id;
#define P(n) out<<",\"" #n "\":"<<p.n
        P(happiness_to_science);P(median_tech_percent);P(minor_science_allies);
#undef P
        if(vp){
            const auto &profile=runtime::vp_science_profile(p);
            out<<",\"source_policy_modifiers\":";
            if(profile.source_policy_modifiers)out<<*profile.source_policy_modifiers;else out<<"null";
            out<<",\"source_espionage\":";
            if(profile.source_espionage)out<<'['<<profile.source_espionage->first<<','<<profile.source_espionage->second<<']';else out<<"null";
#define P(n) out<<",\"" #n "\":"<<profile.n
            P(cached_happiness_total);P(last_turn_times100);P(vassal_modifier);
            P(per_trade_partner);P(jfd_minors);P(annexed_minors_times100);
            P(incoming_espionage);P(outgoing_espionage);P(passive_spy_modifier);
#undef P
            out<<",\"spies\":[";bool comma=false;
            for(const auto &spy:profile.spies){if(comma)out<<',';comma=true;out<<'['<<spy.alive<<',';
                if(spy.city)out<<'['<<spy.city->first<<','<<spy.city->second<<']';else out<<"null";out<<']';}
            out<<"],\"passive_spy_percent\":[";comma=false;
            for(const auto &[cid,bonuses]:profile.passive_spy_percent)
                for(auto [player,n]:bonuses){if(comma)out<<',';comma=true;out<<'['<<cid<<','<<player<<','<<n<<']';}
            out<<']';
        }
        out<<",\"major_team\":"<<(w.diplomacy.team_is_major(w.setup,w.setup.players.at(id).team)?"true":"false");
        out<<",\"science_from_players\":";list(p.science_from_players);
        out<<",\"quote\":";quote(id,false,m);out<<",\"without_agreements\":";
        if(vp)quote(id,true,excluded);else out<<"null";out<<'}';
    }
    const auto caches=[&](const auto &cache){out<<'[';bool comma=false;for(const auto &[id,p]:cache.players){if(comma)out<<',';comma=true;out<<'['<<id<<','<<p.cached_unit_expense<<','<<p.cached_supply_expense<<']';}out<<']';};
    out<<"],\"expense_cache_after\":";caches(m);out<<",\"excluded_expense_cache_after\":";if(vp)caches(excluded);else out<<"null";out<<'}';return out.str();
}
} // namespace civ5::validation
