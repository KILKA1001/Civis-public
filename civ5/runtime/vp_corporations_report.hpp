#pragma once
#include "civ5/saves/vp_player_knowledge_profile.hpp"
#include "civ5/runtime/vp_corporations.hpp"
#include "civ5/runtime/vp_corporation_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_corporations_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::CityYieldContext &cities,const runtime::CityYieldRules &city_rules,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    const auto rules=runtime::load_vp_corporation_rules(db,types);runtime::restore_vp_corporations(snap,cities);
    const auto *profile=runtime::find_vp_corporation_profile(cities);
    if(!profile||!profile->active)throw runtime::CityYieldUnavailable("Missing VP corporation profile report state");
    const auto gr=runtime::load_vp_great_work_rules(db,types);const auto rr=runtime::load_vp_plot_religion_rules(db,types);const auto pass=runtime::restore_plot_passability(snap,db,types);
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};
    struct Services:runtime::VpGreatWorkServices{void unlock_capital_theming_achievement(std::int64_t)override{throw std::runtime_error("Unexpected corporation achievement");}} services;
    runtime::VpGreatWorkContext great{cities,city_rules,gr,religion,services};runtime::VpCorporationContext c{rules,great};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto handle=[&](const runtime::CityYieldState *city){if(city)out<<'['<<city->owner<<','<<city->id<<']';else out<<"null";};
    out<<"{\"applicable\":true,\"trade_city_modifier_complete\":true,\"city_yield_updater_complete\":false,\"corporation_lifecycle_complete\":false,\"yields\":";list(rules.yields);
    out<<",\"rules\":[";bool comma=false;for(const auto &[id,r]:rules.corporations){if(comma)out<<',';comma=true;out<<'['<<id<<','<<r.headquarters<<','<<r.office<<','<<r.franchise<<',';pairs(r.trade_city);out<<']';}
    out<<"],\"source_final_systems\":["<<snap.final_systems.start_offset<<','<<snap.final_systems.end_offset<<"],\"active\":[";comma=false;
    for(const auto &r:*profile->active){if(comma)out<<',';comma=true;out<<'['<<r.type<<','<<r.founder<<','<<r.headquarters_x<<','<<r.headquarters_y<<','<<r.turn_founded<<']';}
    out<<"],\"route_cities\":[";comma=false;for(const auto &r:cities.trade_routes){if(comma)out<<',';comma=true;out<<'[';
        handle(runtime::vp_city_at(cities,*snap.map_projection,r.origin_x,r.origin_y));out<<',';handle(runtime::vp_city_at(cities,*snap.map_projection,r.destination_x,r.destination_y));out<<']';}
    out<<"],\"players\":[";comma=false;for(const auto &sp:snap.all_players->players){const auto id=sp.saved_player_id;const auto &p=cities.players.at(id);const auto &a=profile->players.at(id);
        const auto *knowledge=saves::vp_player_knowledge_dimensions(sp.knowledge);
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"source_knowledge\":["<<sp.knowledge.start_offset<<','<<sp.knowledge.end_offset<<','<<knowledge->techs<<','<<knowledge->flavors<<"],\"state\":[";
        bool inner=false;for(auto n:{a.founded,a.offices,a.franchises,a.additional_franchises,a.additional_franchises_modifier,a.offices_as_franchises,a.random_foreign_franchise_modifier,a.free_franchise_above_popular,a.no_foreign_corporations_in_cities,a.no_franchises_in_foreign_cities}){if(inner)out<<',';inner=true;out<<n;}
        out<<"],\"franchises_per_improvement\":";list(a.franchises_per_improvement);out<<",\"cities\":[";inner=false;
        for(const auto &[cid,city]:p.cities){if(inner)out<<',';inner=true;out<<"{\"id\":"<<cid<<",\"headquarters\":"<<runtime::vp_city_is_headquarters(c,city)<<",\"office\":"<<runtime::vp_city_has_office(c,city)<<",\"franchises\":[";
            bool row=false;for(const auto &[corp,r]:rules.corporations){if(row)out<<',';row=true;out<<'['<<corp<<','<<runtime::vp_city_has_franchise(c,city,corp)<<']';}
            out<<"],\"modifiers\":[";row=false;for(auto y:rules.yields){if(row)out<<',';row=true;out<<'['<<y<<','<<runtime::vp_city_corporation_trade_modifier(c,city,y)<<']';}out<<"]}";
        }out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
