#pragma once
#include "civ5/saves/vp_city_tail_profile.hpp"
#include "civ5/runtime/vp_city_building_yields.hpp"
#include "civ5/runtime/vp_player_accomplishments_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_city_building_yields_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::CityYieldContext &cities,const runtime::CityYieldRules &city_rules,
    const std::map<std::int64_t,std::int64_t> &team_eras,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    const auto rules=runtime::load_vp_city_building_yield_rules(db,types);runtime::restore_vp_city_building_yields(snap,rules,cities);
    const auto gr=runtime::load_vp_great_work_rules(db,types);const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    const auto pass=runtime::restore_plot_passability(snap,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};
    struct Services:runtime::VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected achievement in building income");}} services;
    runtime::VpGreatWorkContext great{cities,city_rules,gr,religion,services};runtime::VpCityBuildingYieldContext c{rules,great,team_eras};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto rows=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(n);out<<']';}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Missing building income source");};
    out<<"{\"applicable\":true,\"building_contribution_complete\":true,\"city_yield_updater_complete\":false,\"building_slots\":"<<rules.building_slots<<",\"yields\":";list(rules.yields);
    out<<",\"buildings\":[";bool comma=false;for(const auto &[id,b]:rules.buildings){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.faith_only<<',';rows(b.accomplishments);out<<']';}
    out<<"],\"faith_building_tourism\":";pairs(rules.faith_building_tourism);out<<",\"players\":[";comma=false;
    for(const auto &sp:snap.all_players->players){const auto id=sp.saved_player_id;const auto &p=cities.players.at(id);const auto &pg=runtime::vp_city_building_yield_player(cities,id);const auto &pt=*sp.tail.context;
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"team\":"<<players.players.at(id).team<<",\"era\":"<<team_eras.at(players.players.at(id).team)
            <<",\"source_core\":["<<sp.core.start_offset<<','<<sp.core.end_offset<<','<<sp.core.yield_slots<<"],\"source_tail\":["<<sp.tail.start_offset<<','<<sp.tail.end_offset
            <<"],\"tail_context\":{\"yields\":"<<pt.yields<<",\"players\":"<<pt.players<<",\"major\":"<<pt.major<<",\"player_id\":"<<pt.player_id<<"},\"strategic_from_minors\":"<<pg.strategic_from_minors<<",\"accomplishments\":";pairs(runtime::vp_player_accomplishments(cities,id));
        out<<",\"per_religion\":";pairs(pg.per_religion_times100);out<<",\"cities\":[";bool inner=false;
        for(const auto &sc:sp.cities.cities){const auto cid=named(sc.core.fields,"m_iID").values.at(0);const auto &city=p.cities.at(cid);const auto &a=runtime::vp_city_building_yield_city(cities,id,cid);const auto &ct=*saves::vp_city_tail_dimensions(sc.tail.context);
            if(inner)out<<',';inner=true;out<<"{\"id\":"<<cid<<",\"position\":["<<city.x<<','<<city.y<<"],\"source_core\":["<<sc.core.start_offset<<','<<sc.core.end_offset
                <<"],\"source_production\":["<<sc.production.start_offset<<','<<sc.production.end_offset<<"],\"source_tail\":["<<sc.tail.start_offset<<','<<sc.tail.end_offset
                <<"],\"tail_context\":{\"vp_rows\":{\"yields\":"<<ct.yields<<",\"emphases\":"<<ct.emphases<<"}},\"buildings\":"<<a.buildings<<",\"plots\":";list(a.plots);
            out<<",\"era_scaling\":";pairs(a.era_scaling_times100);out<<",\"passing_trade\":";pairs(a.passing_trade);out<<",\"fractions\":[";bool row=false;
            for(const auto &v:a.fractions){if(row)out<<',';row=true;out<<'[';bool f=false;for(const auto &[y,n]:v){if(f)out<<',';f=true;out<<'['<<y<<','<<n.numerator()<<','<<n.denominator()<<']';}out<<']';}
            out<<"],\"terrain\":";rows(a.per_x_terrain);out<<",\"features\":";rows(a.per_x_feature);
            runtime::YieldValues base,per_religion;for(const auto &[y,v]:city.yields){if(v.buildings)base[y]=v.buildings;if(v.per_religion)per_religion[y]=v.per_religion;}
            out<<",\"base\":";pairs(base);out<<",\"per_religion\":";pairs(per_religion);out<<",\"quotes\":[";row=false;
            for(auto y:rules.yields){const auto q=runtime::vp_city_building_yield(c,city,y);if(row)out<<',';row=true;
                out<<'['<<y<<','<<q.base<<','<<q.era<<','<<q.per_building<<','<<q.per_tile<<','<<q.passing_trade<<','<<q.terrain<<','<<q.features<<','<<q.minor_resources<<','<<q.accomplishments<<','<<q.faith<<','<<q.religions<<','<<q.local_themes<<','<<q.total_times100<<']';}
            out<<"]}";
        }out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
