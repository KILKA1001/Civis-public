#pragma once
#include "civ5/saves/vp_game_trade_profile.hpp"
#include "civ5/runtime/vp_city_happiness.hpp"
#include <sstream>
#include <iomanip>

namespace civ5::validation {
inline std::string vp_city_happiness_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::CityYieldContext &cities,const runtime::CityYieldRules &city_rules,
    const runtime::MaintenanceContext &units,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    const auto rules=runtime::load_vp_city_happiness_rules(db,types);runtime::restore_vp_city_happiness(snap,cities);
    const auto gr=runtime::load_vp_great_work_rules(db,types);const auto rr=runtime::load_vp_plot_religion_rules(db,types);const auto fr=runtime::load_vp_city_food_rules(db);
    const auto pass=runtime::restore_plot_passability(snap,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};
    struct Services:runtime::VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected happiness achievement");}} services;
    runtime::VpGreatWorkContext great{cities,city_rules,gr,religion,services};runtime::VpCityFoodContext food{cities,players,units.team_eras,fr,defines};runtime::VpCityHappinessContext c{rules,great,food,units,players,db};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto policy=[&](const auto &q){out<<'['<<q.garrison<<','<<q.strength<<','<<q.great_works<<','<<q.population_global<<','<<q.capital<<','<<q.policies_from_policies<<','<<q.policies<<','<<q.specialists<<','<<q.per_city<<','<<q.population<<','<<q.trade<<','<<q.total<<']';};
    out<<"{\"applicable\":true,\"local_happiness_complete\":true,\"policy_contribution_complete\":true,\"city_yield_updater_complete\":false,\"trade_cache_unresolved_on_import\":true,\"turn_slice\":"<<*cities.turn_slice<<",\"culture\":"<<rules.culture<<",\"crime\":"<<rules.crime<<",\"yields\":";list(rules.yields);
    out<<",\"options\":{";bool comma=false;for(const char *name:{"BALANCE_VP","BALANCE_CORE_JFD","HUMAN_USES_AI_HANDICAP","HUMAN_USES_AI_MECHANICS"}){if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<options->enabled(name);}
    out<<"},\"defines\":{";comma=false;for(const char *name:{"CITY_STRENGTH_THRESHOLD_FOR_BONUSES","UNHAPPINESS_PER_SPECIALIST","AI_HANDICAP"}){if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<std::get<std::int64_t>(defines.find(name)->lookup.value);}
    out<<"},\"one_city_challenge\":"<<players.game_option_value(db,"GAMEOPTION_ONE_CITY_CHALLENGE")<<",\"game_handicap\":"<<players.game_handicap<<",\"handicaps\":[";comma=false;
    for(const auto &[id,h]:rules.handicaps){if(comma)out<<',';comma=true;out<<'['<<id<<','<<h.capital<<','<<h.ai_capital<<']';}
    out<<"],\"dummy_policies\":";pairs(rules.dummy_policies);out<<",\"routes\":[";comma=false;for(const auto &r:cities.trade_routes){const auto *profile=saves::find_vp_trade_route(r);if(!profile)throw runtime::CityYieldUnavailable("VP route has no city ID fields");if(comma)out<<',';comma=true;out<<'['<<profile->origin_id<<','<<profile->destination_id<<','<<r.origin_owner<<','<<r.destination_owner<<']';}
    out<<"],\"players\":[";comma=false;
    for(auto &[id,p]:cities.players){const auto &h=runtime::vp_city_happiness_player(cities,id);const auto &sp=players.players.at(id);const auto &trade=runtime::vp_city_happiness_trade_stats(cities,id);if(trade.turn_slice)throw std::runtime_error("Imported trade cache unexpectedly warm");
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"capital\":"<<p.capital<<",\"minor\":"<<p.minor<<",\"human\":"<<players.is_human(sp)<<",\"handicap\":"<<sp.handicap<<",\"no_annexing\":"<<h.no_annexing<<",\"policies\":";list(sp.policies);
        out<<",\"modifiers\":["<<h.garrison<<','<<h.strength<<','<<h.per_x_works<<','<<h.specialists<<','<<h.per_city<<','<<h.per_x_population<<','<<h.per_route<<','<<h.to_culture<<','<<h.policy_capital<<','<<h.policy_city<<"],\"fractions\":[";bool inner=false;
        for(const auto &f:{h.per_population,h.per_policy_from_policies,h.per_policy}){if(inner)out<<',';inner=true;out<<'['<<f.numerator()<<','<<f.denominator()<<']';}
        out<<"],\"cities\":[";inner=false;for(auto &[cid,city]:p.cities){const auto &a=runtime::vp_city_happiness_city(cities,id,cid);if(inner)out<<',';inner=true;
            out<<"{\"id\":"<<cid<<",\"population\":"<<city.population<<",\"state\":["<<city.puppet<<','<<city.razing<<','<<city.resistance<<','<<a.occupied<<','<<a.no_occupied_unhappiness<<"],\"base\":["<<a.empire<<','<<a.buildings<<','<<a.building_classes<<','<<a.religion<<','<<a.unmodded_buildings<<"],\"policies\":";pairs(a.policies);out<<",\"crime\":";pairs(a.crime);out<<",\"development\":";pairs(a.development);
            const auto &plot_yields=*runtime::vp_city_plot_profile_city(cities,id,cid).yields;
            out<<",\"works\":["<<runtime::vp_city_great_works(great,city)<<','<<runtime::vp_city_great_works(great,city,false)<<"],\"specialists\":"<<runtime::vp_total_specialists(food,city)<<",\"strength\":"<<plot_yields.strength<<",\"garrison_before\":["<<plot_yields.garrison<<','<<plot_yields.garrison_override<<"],\"policy_happiness\":";policy(runtime::vp_city_policy_happiness(c,city));
            out<<",\"happiness\":[";bool row=false;for(auto pop:{-1,0,1})for(bool exclude:{false,true}){const auto q=runtime::vp_city_local_happiness(c,city,pop,exclude);if(row)out<<',';row=true;
                out<<'['<<pop<<','<<exclude<<',';policy(q.policies);out<<",["<<q.empire<<','<<q.buildings<<','<<q.building_classes<<','<<q.religion<<','<<q.unmodded_buildings<<','<<q.handicap<<','<<q.crime<<','<<q.development<<"],"<<q.total<<','<<q.suppressed<<']';}
            out<<"],\"yields\":[";row=false;for(auto y:rules.yields){const auto q=runtime::vp_city_policy_yield(c,city,y);if(row)out<<',';row=true;out<<'['<<y<<','<<q.base<<','<<q.happiness<<','<<q.total_times100<<']';}
            out<<"],\"garrison_after\":"<<plot_yields.garrison<<'}';
        }
        runtime::vp_update_trade_stats(cities,id);const auto &s=runtime::vp_city_happiness_trade_stats(cities,id);out<<"],\"trade_stats\":{\"slice\":"<<*s.turn_slice<<",\"counts\":["<<s.international_out<<','<<s.international_in<<','<<s.internal<<','<<s.minors<<"],\"from\":";pairs(s.from_city);out<<",\"to\":";pairs(s.to_city);out<<"}}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
