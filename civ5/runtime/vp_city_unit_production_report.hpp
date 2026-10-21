#pragma once
#include "civ5/runtime/vp_city_unit_production.hpp"
#include <algorithm>
#include <sstream>

namespace civ5::validation {
inline std::string vp_city_unit_production_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,
    const content::InfoTypeCatalog &types,runtime::ResearchContext &players,const runtime::ResearchTurnState &turn,
    runtime::PlayerResearchContext &dip,const std::vector<runtime::TeamTechnologies> &techs,
    runtime::CityYieldContext &cities,const runtime::CityYieldRules &city_rules,runtime::MaintenanceContext &units,
    const runtime::ScienceContext &science,const runtime::TreasuryContext &treasury,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded()||!snap.map_projection)throw std::runtime_error("VP city unit production requires resolved map and options");
    const auto rules=runtime::load_vp_city_unit_production_rules(db,types);
    runtime::restore_vp_city_unit_production(snap,rules,cities,units);
    const auto ur=runtime::load_vp_player_unit_production_rules(db,types);
    const auto tr=runtime::load_vp_unit_trait_rules(db,types);
    const auto sr=runtime::load_vp_unit_supply_rules(db,types);
    const auto fr=runtime::load_vp_city_food_rules(db);
    const auto pr=runtime::load_vp_city_production_rules(db);
    const auto gr=runtime::load_vp_great_work_rules(db,types);
    const auto cr=runtime::load_vp_city_growth_rules(db);
    const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    const auto pass=runtime::restore_plot_passability(snap,db,types);
    const auto wr=runtime::load_plot_water_rules(db,players.profile);
    const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};
    runtime::VpPlotReligionContext religion{rr,water};
    runtime::VpCityReligionMinorContext minor{religion,treasury};
    runtime::VpUnitTraitContext traits{tr,players,turn,dip,techs,minor};
    runtime::VpUnitSupplyContext supply{units,players,dip,cities,techs,sr,db,defines,*options};
    runtime::VpCityFoodContext food{cities,players,units.team_eras,fr,defines};
    runtime::VpCityProductionContext production{supply,food,pr};
    runtime::VpPlayerUnitProductionContext player{ur,production,traits};
    struct Services:runtime::VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected production achievement");}} services;
    runtime::VpGreatWorkContext great{cities,city_rules,gr,religion,services};
    runtime::VpCityGrowthContext growth{religion,dip,science,cr};
    runtime::VpCityUnitProductionContext c{rules,player,great,growth};
    std::vector<std::int64_t> probes;
    const auto probe=[&](const auto &predicate){for(const auto &[id,u]:rules.units)if(predicate(u)){
        if(std::find(probes.begin(),probes.end(),id)==probes.end())probes.push_back(id);break;}};
    probe([](const auto &u){return u.military;});probe([](const auto &u){return u.found;});
    probe([](const auto &u){return u.spaceship!=-1;});
    probe([](const auto &u){return !u.required_resources.empty()||!u.total_resources.empty();});
    probe([](const auto &u){return !u.buildings.empty();});
    std::ostringstream out;out<<"{\"applicable\":true,\"city_overload_complete\":true,\"turn_continuation_verified\":false,\"units\":"<<rules.units.size()<<",\"players\":[";
    const auto quote=[&](const runtime::CityYieldState &city,std::int64_t query){const auto q=runtime::vp_city_unit_production(c,city,query);
        out<<'['<<query<<','<<q.city.total<<','<<q.player.total<<','<<q.capital_settler<<','<<q.unit_class<<','<<q.domain<<','<<q.pledges
           <<','<<q.combat<<','<<q.corporation<<','<<q.unhappiness<<','<<q.military_city<<','<<q.military_units<<','<<q.space_city
           <<','<<q.space_player<<','<<q.space_leagues<<','<<q.religion_unit<<','<<q.religion_combat<<','<<q.buildings<<','<<q.total<<']';};
    const auto &profile=*runtime::find_vp_unit_production_profile(cities);
    bool first_player=true;for(const auto &[owner,p]:cities.players){if(!first_player)out<<',';first_player=false;
        const auto &ps=profile.city_players.at(owner);
        out<<"{\"id\":"<<owner<<",\"capital_settler\":"<<ps.capital_settler<<",\"cities\":[";
        bool first_city=true;for(const auto &[id,city]:p.cities){if(!first_city)out<<',';first_city=false;
            const auto &cs=profile.cities.at({owner,id});out<<"{\"id\":"<<id<<",\"military\":"<<cs.military<<",\"queue\":";
            const auto queued=city.production_orders&&!city.production_orders->empty()&&city.production_orders->front().type==0?city.production_orders->front().item:-1;
            out<<queued<<",\"query\":";
            const auto query=queued!=-1&&rules.units.contains(queued)?queued:rules.units.empty()?-1:rules.units.begin()->first;
            if(query==-1)out<<"null";else quote(city,query);
            out<<",\"probes\":[";bool first_probe=true;
            for(auto id:probes)if(id!=query){if(!first_probe)out<<',';first_probe=false;quote(city,id);}
            out<<']';
            out<<'}';}out<<"]}";}out<<"]}";return out.str();
}
} // namespace civ5::validation
