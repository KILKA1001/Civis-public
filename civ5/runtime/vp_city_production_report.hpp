#pragma once
#include "civ5/runtime/vp_city_production.hpp"
#include <sstream>
#include <iomanip>

namespace civ5::validation {
inline std::string vp_city_production_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::PlayerResearchContext &dip,const std::vector<runtime::TeamTechnologies> &techs,
    runtime::CityYieldContext &cities,runtime::MaintenanceContext &units,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded())throw std::runtime_error("Production diagnostic requires resolved options");
    const auto rules=runtime::load_vp_city_production_rules(db);runtime::restore_vp_city_production(snap,cities);
    const auto sr=runtime::load_vp_unit_supply_rules(db,types);const auto fr=runtime::load_vp_city_food_rules(db);
    runtime::VpUnitSupplyContext supply{units,players,dip,cities,techs,sr,db,defines,*options};runtime::VpCityFoodContext food{cities,players,units.team_eras,fr,defines};
    runtime::VpCityProductionContext c{supply,food,rules};std::ostringstream out;
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto player_quote=[&](const runtime::VpPlayerProductionQuote &q){out<<'['<<q.supply_over<<','<<q.supply<<','<<q.conquest<<','<<q.space<<','<<q.total<<']';};
    const auto general=[&](const runtime::VpCityGeneralProductionQuote &q){out<<'['<<q.industrial<<','<<q.puppet<<','<<q.occupied<<','<<q.specialists<<','<<q.total<<']';};
    const auto city_quote=[&](const runtime::VpCityProductionQuote &q){out<<'[';general(q.city);out<<',';player_quote(q.player);out<<','<<q.space<<','<<q.second_conquest<<','<<q.total<<']';};
    out<<"{\"applicable\":true,\"general_project_process_complete\":true,\"unit_building_modifiers_complete\":false,\"production_progress_updated\":false,\"city_yield_updater_complete\":false,\"defines\":{";
    bool comma=false;for(const char *name:{"INDUSTRIAL_ROUTE_PRODUCTION_MOD","MAX_UNIT_SUPPLY_PRODMOD","PRODUCTION_PENALTY_PER_UNIT_OVER_SUPPLY"}){if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<std::get<std::int64_t>(defines.find(name)->lookup.value);}
    out<<"},\"projects\":[";comma=false;for(const auto &[id,space]:rules.projects){if(comma)out<<',';comma=true;out<<'['<<id<<','<<space<<']';}out<<"],\"processes\":";list(rules.processes);
    const auto &profile=*runtime::find_vp_general_production_profile(cities);
    out<<",\"players\":[";comma=false;for(const auto &[owner,p]:cities.players){const auto &s=profile.players.at(owner);if(comma)out<<',';comma=true;
        out<<"{\"id\":"<<owner<<",\"state\":["<<s.puppet<<','<<s.occupied<<','<<s.conquest_turns<<','<<s.conquest<<','<<s.space<<','<<s.from_specialists<<"],\"general\":";player_quote(runtime::vp_player_general_production(c,owner));
        out<<",\"projects\":[";bool inner=false;for(const auto &[id,space]:rules.projects){if(inner)out<<',';inner=true;out<<'['<<id<<',';player_quote(runtime::vp_player_project_production(c,owner,id));out<<']';}
        out<<"],\"processes\":[";inner=false;for(auto id:rules.processes){if(inner)out<<',';inner=true;out<<'['<<id<<',';player_quote(runtime::vp_player_process_production(c,owner,id));out<<']';}
        out<<"],\"cities\":[";inner=false;for(const auto &[id,city]:p.cities){if(inner)out<<',';inner=true;out<<"{\"id\":"<<id<<",\"space\":"<<profile.cities.at({owner,id}).space<<",\"general\":";general(runtime::vp_city_general_production(c,city));
            out<<",\"projects\":[";bool row=false;for(const auto &[pid,space]:rules.projects){if(row)out<<',';row=true;out<<'['<<pid<<',';city_quote(runtime::vp_city_project_production(c,city,pid));out<<']';}
            out<<"],\"processes\":[";row=false;for(auto pid:rules.processes){if(row)out<<',';row=true;out<<'['<<pid<<',';city_quote(runtime::vp_city_process_production(c,city,pid));out<<']';}out<<"]}";
        }out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
