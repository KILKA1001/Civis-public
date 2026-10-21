#pragma once
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include "civ5/runtime/vp_great_works.hpp"
#include <sstream>
#include <iomanip>

namespace civ5::validation {
inline std::string vp_great_works_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::CityYieldContext &cities,runtime::CityYieldRules &city_rules,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded())throw std::runtime_error("Great-work diagnostic requires resolved options");
    runtime::load_city_building_replacements(db,city_rules);
    const auto rules=runtime::load_vp_great_work_rules(db,types);runtime::restore_vp_great_works(snap,rules,cities);
    const auto rr=runtime::load_vp_plot_religion_rules(db,types);const auto pass=runtime::restore_plot_passability(snap,db,types);
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};
    struct Services:runtime::VpGreatWorkServices{std::vector<std::int64_t> awards;void unlock_capital_theming_achievement(std::int64_t id)override{awards.push_back(id);}} services;
    runtime::VpGreatWorkContext c{cities,city_rules,rules,religion,services};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto arrays=[&](const auto &v){out<<'[';bool comma=false;for(const auto &n:v){if(comma)out<<',';comma=true;pairs(n);}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Missing great-work diagnostic source");};
    out<<"{\"applicable\":true,\"fresh_yields_calculated\":true,\"city_yield_updater_complete\":false,\"cache_reset_on_import\":true,\"steam_achievements_delivered\":false,\"options\":{";
    bool comma=false;for(const char *name:{"BUILDINGS_THOROUGH_PREREQUISITES","ENABLE_ACHIEVEMENTS","BALANCE_CORE_JFD","BALANCE_PERMANENT_PANTHEONS"}){if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<options->enabled(name);}
    out<<"},\"defines\":{";comma=false;for(const char *name:{"CITY_MAX_NUM_BUILDINGS","BASE_CULTURE_PER_GREAT_WORK","BASE_TOURISM_PER_GREAT_WORK"}){if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<std::get<std::int64_t>(defines.find(name)->lookup.value);}
    out<<"},\"building_slots\":"<<rules.building_slots<<",\"class_slots\":"<<rules.class_slots<<",\"tourism\":"<<rules.tourism<<",\"culture_local\":"<<rules.culture_local<<",\"art\":"<<rules.art<<",\"artifact\":"<<rules.artifact<<",\"slots\":";list(rules.slots);
    out<<",\"yields\":";list(rules.yields);out<<",\"buildings\":[";comma=false;
    for(const auto &[id,b]:rules.buildings){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.building_class<<','<<b.slot<<','<<b.yield<<','<<b.slots<<',';pairs(b.theme_yields);out<<',';list(b.themes);out<<']';}
    out<<"],\"beliefs\":[";comma=false;for(const auto &[id,b]:rules.beliefs){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.minimum_population<<',';pairs(b.yields);out<<']';}
    out<<"],\"civilization_buildings\":[";comma=false;for(const auto &[id,b]:city_rules.civilization_buildings){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(b);out<<']';}
    out<<"],\"source_culture\":["<<snap.religion_culture.culture_offset<<','<<snap.religion_culture.end_offset<<"],\"global_works\":[";comma=false;
    for(const auto &[id,w]:*cities.great_works){if(comma)out<<',';comma=true;out<<'['<<id<<','<<w.type<<','<<w.turn_founded<<','<<w.era<<','<<w.player<<','<<*w.class_type<<']';}
    const auto &sd=snap.all_players->context.strategy;const auto *profile=saves::vp_player_strategy_dimensions(sd);if(!profile)throw runtime::CityYieldUnavailable("VP strategy dimensions unavailable");const auto &dims=*profile;
    out<<"],\"strategy_dimensions\":{\"major\":"<<sd.major<<",\"civ_players\":"<<sd.civ_players<<",\"yields\":"<<sd.yields<<",\"grand\":"<<dims.grand<<",\"policies\":"<<dims.policies<<",\"branches\":"<<dims.branches<<",\"flavors\":"<<dims.flavors<<",\"economic\":"<<dims.economic<<",\"military\":"<<dims.military<<"},\"players\":[";comma=false;
    for(const auto &sp:snap.all_players->players){const auto id=sp.saved_player_id;const auto &p=cities.players.at(id);const auto &pg=runtime::vp_great_work_player(cities,id);if(comma)out<<',';comma=true;
        out<<"{\"id\":"<<id<<",\"capital\":"<<p.capital<<",\"civilization\":"<<p.civilization<<",\"human\":"<<players.is_human(players.players.at(id))<<",\"source_core\":["<<sp.core.start_offset<<','<<sp.core.end_offset<<','<<sp.core.yield_slots
           <<"],\"source_traits\":["<<sp.traits.start_offset<<','<<sp.traits.end_offset<<','<<sp.traits.dimensions.yields<<','<<sp.traits.dimensions.terrains<<"],\"source_strategy\":["<<sp.strategy.start_offset<<','<<sp.strategy.end_offset
           <<"],\"state\":["<<pg.tourism_modifier<<','<<pg.capital_theming_modifier<<','<<pg.theming_policy_modifier<<"],\"per_work\":";pairs(pg.per_work);out<<",\"trait_per_work\":";pairs(pg.trait_per_work);
        out<<",\"type_bonus\":";arrays(pg.type_bonus);out<<",\"trait_type_bonus\":";arrays(pg.trait_type_bonus);out<<",\"cities\":[";bool inner=false;
        for(const auto &sc:sp.cities.cities){const auto cid=named(sc.core.fields,"m_iID").values.at(0);auto &city=cities.players.at(id).cities.at(cid);auto &cg=runtime::vp_great_work_city(cities,id,cid);
            if(cg.building_by_class||!cg.yield_cache.empty())throw std::runtime_error("Great-work import retained unsaved cache");if(inner)out<<',';inner=true;
            out<<"{\"id\":"<<cid<<",\"population\":"<<city.population<<",\"source_core\":["<<sc.core.start_offset<<','<<sc.core.end_offset<<"],\"source_production\":["<<sc.production.start_offset<<','<<sc.production.end_offset<<"],\"tourism_modifier\":"<<cg.tourism_modifier<<",\"per_work\":";pairs(cg.per_work);
            out<<",\"per_local_theme\":";pairs(cg.per_local_theme);out<<",\"themes\":";pairs(cg.theming_indices);out<<",\"real_buildings\":";pairs(city.real_buildings);out<<",\"free_buildings\":";pairs(city.free_buildings);
            out<<",\"existing_buildings\":";list(cg.existing_buildings);out<<",\"works\":[";bool row=false;for(const auto &w:cg.works){if(row)out<<',';row=true;out<<'['<<w.building_class<<','<<w.slot<<','<<w.work<<']';}
            out<<"],\"themed\":"<<runtime::vp_themed_buildings(c,city)<<",\"multiplier\":"<<runtime::vp_theming_multiplier(c,city)<<",\"quotes\":[";row=false;
            for(auto y:rules.yields){const auto q=runtime::vp_great_work_yield(c,city,y);const auto fresh=runtime::vp_cached_great_work_yield(c,city,y),cached=runtime::vp_cached_great_work_yield(c,city,y);
                if(fresh!=q.total_times100||cached!=fresh)throw std::runtime_error("Great-work cache query differs");if(row)out<<',';row=true;
                out<<'['<<y<<','<<q.works<<','<<q.matching_works<<','<<q.per_work<<','<<q.beliefs<<','<<q.secondary<<','<<q.pantheon<<','<<q.base_times100<<','<<q.tourism_times100<<','<<q.themes<<','<<q.type_bonus<<','<<q.extra_themes<<','<<q.total_times100<<','<<q.suppressed<<']';}
            out<<"],\"class_cache\":";if(cg.building_by_class)pairs(*cg.building_by_class);else out<<"null";out<<'}';
        }out<<"]}";
    }out<<"],\"achievement_requests\":";list(services.awards);out<<'}';return out.str();
}
} // namespace civ5::validation
