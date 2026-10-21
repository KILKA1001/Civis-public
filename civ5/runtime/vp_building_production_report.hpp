#pragma once
#include "civ5/runtime/vp_building_production.hpp"
#include "civ5/runtime/vp_policy_branch_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_building_production_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::PlayerResearchContext &dip,const std::vector<runtime::TeamTechnologies> &techs,
    runtime::CityYieldContext &cities,const runtime::CityYieldRules &city_rules,runtime::MaintenanceContext &units,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded())throw std::runtime_error("Building production requires resolved options");
    const auto rules=runtime::load_vp_building_production_rules(db,types);runtime::restore_vp_building_production(snap,rules,cities,players,units);
    const auto sr=runtime::load_vp_unit_supply_rules(db,types);const auto fr=runtime::load_vp_city_food_rules(db);const auto pr=runtime::load_vp_city_production_rules(db);
    const auto gr=runtime::load_vp_great_work_rules(db,types);const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    const auto pass=runtime::restore_plot_passability(snap,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};
    struct Services:runtime::VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected production achievement");}} services;
    runtime::VpGreatWorkContext great{cities,city_rules,gr,religion,services};runtime::VpUnitSupplyContext supply{units,players,dip,cities,techs,sr,db,defines,*options};
    runtime::VpCityFoodContext food{cities,players,units.team_eras,fr,defines};runtime::VpCityProductionContext production{supply,food,pr};runtime::VpBuildingProductionContext c{rules,production,great};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto player_quote=[&](const runtime::VpPlayerBuildingProductionQuote &q){out<<'['<<q.general.total<<','<<q.policy<<','<<q.wonder<<','<<q.limited<<','<<q.converted_wonder<<','<<q.converted_policy<<','<<q.building_class<<','<<q.capital<<','<<q.total<<']';};
    out<<"{\"applicable\":true,\"building_modifiers_complete\":true,\"production_progress_updated\":false,\"city_yield_updater_complete\":false,\"resource_slots\":"<<rules.resource_slots<<",\"wonder_routes\":"<<options->enabled("TRADE_WONDER_RESOURCE_ROUTES")<<",\"buildings\":[";
    bool comma=false;for(const auto &[id,b]:rules.buildings){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.building_class<<','<<b.technology<<']';}
    out<<"],\"classes\":[";comma=false;for(const auto &[id,b]:rules.classes){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.world<<','<<b.team<<','<<b.national<<']';}
    out<<"],\"technology_eras\":";pairs(rules.technology_eras);out<<",\"improvements\":";pairs(rules.improvements);out<<",\"free_policies\":";pairs(rules.free_policies);
    out<<",\"policies\":[";comma=false;for(const auto &[id,p]:rules.policies){if(comma)out<<',';comma=true;out<<'['<<id<<','<<p.branch<<',';pairs(p.classes);out<<']';}
    const auto bonuses=[&](const char *name,const auto &rows){out<<",\""<<name<<"\":[";bool comma=false;for(const auto &[id,b]:rows){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.modifier<<','<<b.obsolete<<']';}out<<']';};out<<']';bonuses("resources",rules.resources);bonuses("beliefs",rules.beliefs);
    std::set<std::int64_t> eras{-1};for(const auto &[id,e]:rules.technology_eras)eras.insert(e);out<<",\"eras\":";list(eras);
    const auto &profile=*runtime::find_vp_building_production_profile(cities);
    out<<",\"players\":[";comma=false;for(const auto &[owner,p]:cities.players){if(comma)out<<',';comma=true;const auto &s=profile.players.at(owner);
        out<<"{\"id\":"<<owner<<",\"state\":["<<s.wonder<<','<<s.world<<','<<s.team<<','<<s.national<<','<<s.golden_age<<','<<s.to_building<<','<<s.capital_discount<<','<<s.conquest_era<<','<<s.past_era<<','<<s.policy_wonder<<','<<s.policy_building<<"],\"blocked_branches\":";list(runtime::vp_blocked_policy_branches(players,owner));
        out<<",\"queries\":[";bool inner=false;for(const auto &[bid,b]:rules.buildings){if(inner)out<<',';inner=true;out<<'['<<bid<<',';player_quote(runtime::vp_player_building_production(c,owner,bid));out<<']';}
        out<<"],\"units\":[";inner=false;for(const auto &[id,u]:units.players.at(owner).units){if(inner)out<<',';inner=true;out<<'['<<id<<','<<u.source_core->first<<','<<u.source_core->second<<','<<profile.wonder_units.at({owner,id})<<']';}
        out<<"],\"cities\":[";inner=false;for(const auto &[id,city]:p.cities){if(inner)out<<',';inner=true;const auto &a=profile.cities.at({owner,id});
            out<<"{\"id\":"<<id<<",\"state\":["<<a.wonder<<','<<a.building<<"],\"local_resources\":";pairs(a.local_resources);out<<",\"unimproved_resources\":";pairs(a.unimproved_resources);
            out<<",\"wonder_units\":"<<runtime::vp_wonder_unit_production(c,city)<<",\"wonder_improvements\":"<<runtime::vp_wonder_improvement_production(c,city)<<",\"era_queries\":[";bool row=false;
            for(auto e:eras){if(row)out<<',';row=true;const auto resource=runtime::vp_wonder_resource_production(c,city,e);const auto rel=runtime::vp_wonder_religion_production(c,city,e);
                out<<'['<<e<<','<<resource.incoming<<','<<resource.outgoing<<','<<resource.total<<','<<rel.majority<<','<<rel.secondary<<','<<rel.pantheon<<','<<rel.total<<']';}
            out<<"],\"queries\":[";row=false;for(const auto &[bid,b]:rules.buildings){if(row)out<<',';row=true;const auto q=runtime::vp_city_building_production(c,city,bid);
                out<<'['<<bid<<",["<<q.general.total<<','<<q.player.total<<','<<q.era<<','<<q.conversion<<','<<q.wonder<<','<<q.resources<<','<<q.religion<<','<<q.units<<','<<q.improvements<<','<<q.building<<','<<q.past_eras<<','<<q.total<<"]]";}
            out<<"]}";
        }out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
