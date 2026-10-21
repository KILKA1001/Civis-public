#pragma once
#include "civ5/runtime/vp_player_unit_production.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_player_unit_production_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,const runtime::ResearchTurnState &turn,runtime::PlayerResearchContext &dip,
    const std::vector<runtime::TeamTechnologies> &techs,runtime::CityYieldContext &cities,runtime::MaintenanceContext &units,
    const runtime::TreasuryContext &treasury,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded()||!snap.map_projection)throw std::runtime_error("VP player unit production requires resolved map and options");
    const auto rules=runtime::load_vp_player_unit_production_rules(db,types);
    const auto trait_rules=runtime::load_vp_unit_trait_rules(db,types);
    if(rules.trait_slots!=trait_rules.trait_slots)throw std::runtime_error("Unit production trait Info span mismatch");
    runtime::restore_vp_player_unit_production(snap,rules,cities);runtime::restore_vp_unit_traits(trait_rules,players,turn,dip);
    const auto sr=runtime::load_vp_unit_supply_rules(db,types);const auto fr=runtime::load_vp_city_food_rules(db);
    const auto pr=runtime::load_vp_city_production_rules(db);
    runtime::VpUnitSupplyContext supply{units,players,dip,cities,techs,sr,db,defines,*options};
    runtime::VpCityFoodContext food{cities,players,units.team_eras,fr,defines};runtime::VpCityProductionContext production{supply,food,pr};
    const auto pass=runtime::restore_plot_passability(snap,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);
    const auto ws=runtime::restore_plot_water(snap);const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};
    runtime::VpPlotReligionContext religion{rr,water};runtime::VpCityReligionMinorContext minor{religion,treasury};
    runtime::VpUnitTraitContext traits{trait_rules,players,turn,dip,techs,minor};runtime::VpPlayerUnitProductionContext c{rules,production,traits};
    std::ostringstream out;const auto vec=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto quote=[&](const runtime::VpPlayerUnitProductionQuote &q){out<<'['<<q.general.supply_over<<','<<q.general.supply<<','<<q.general.conquest<<','<<q.general.total
        <<','<<q.major_wars<<','<<q.military<<','<<q.war_military<<','<<q.settler<<','<<q.combat<<','<<q.unit_traits<<','<<q.special_traits<<','<<q.total<<']';};
    out<<"{\"applicable\":true,\"player_overload_complete\":true,\"city_overload_complete\":false,\"continuation_verified\":false,\"trait_slots\":"<<rules.trait_slots
       <<",\"combat_slots\":"<<rules.combat_slots<<",\"units\":[";
    bool comma=false;for(const auto &[id,u]:rules.units){if(comma)out<<',';comma=true;out<<'['<<id<<','<<u.military<<','<<u.found<<','<<u.combat<<','<<u.special<<',';vec(u.trait_modifiers);out<<']';}
    out<<"],\"specials\":[";comma=false;for(const auto &[id,v]:rules.special_trait_modifiers){if(comma)out<<',';comma=true;out<<'['<<id<<',';vec(v);out<<']';}
    const auto &profile=*runtime::find_vp_unit_production_profile(cities);
    out<<"],\"players\":[";comma=false;for(const auto &[id,p]:cities.players){if(comma)out<<',';comma=true;const auto &s=profile.players.at(id);
        out<<"{\"id\":"<<id<<",\"state\":["<<s.military<<','<<s.military_per_major_war<<','<<s.settler<<"],\"combat\":[";
        bool inner=false;for(const auto &[combat,value]:s.combat){if(inner)out<<',';inner=true;out<<'['<<combat<<','<<value<<']';}
        out<<"],\"potential\":";vec(runtime::vp_potential_unit_traits(traits,id));out<<",\"quotes\":[";
        inner=false;for(const auto &[unit,u]:rules.units){if(inner)out<<',';inner=true;out<<'['<<unit<<',';quote(runtime::vp_player_unit_production(c,id,unit));out<<']';}out<<"]}";}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
