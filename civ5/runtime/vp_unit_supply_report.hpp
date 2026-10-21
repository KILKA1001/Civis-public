#pragma once
#include "civ5/saves/vp_team_profile.hpp"
#include "civ5/runtime/vp_unit_supply.hpp"
#include "civ5/runtime/vp_city_growth.hpp"
#include <sstream>
#include <iomanip>

namespace civ5::validation {
// Unique City States shuffles MinorCivilizations.ID on each database load.
// Compare every rule and player binding by the saved Type, retaining live IDs
// internally. This does not alter SQL random() or the mod's rule values.
inline std::string vp_unit_supply_minor_rules_report(const runtime::VpUnitSupplyRules &rules){
    std::map<std::string,std::int64_t> ordered;
    for(const auto &[id,trait]:rules.minor_traits){const auto &type=rules.minor_types.at(id);
        if(type.empty()||!ordered.emplace(type,trait).second)throw std::runtime_error("Supply report needs unique nonempty minor types");}
    std::ostringstream out;out<<'[';bool comma=false;for(const auto &[type,trait]:ordered){if(comma)out<<',';comma=true;out<<'['<<std::quoted(type)<<','<<trait<<']';}out<<']';return out.str();
}
inline std::string vp_unit_supply_minor_type_report(const runtime::VpUnitSupplyRules &rules,std::int64_t id){
    if(id==-1)return "null";std::ostringstream out;
    if(const auto i=rules.minor_types.find(id);i!=rules.minor_types.end())out<<std::quoted(i->second);
    else out<<"{\"unresolved_id\":"<<id<<'}';return out.str();
}
inline std::string vp_unit_supply_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::PlayerResearchContext &dip,const std::vector<runtime::TeamTechnologies> &techs,
    runtime::CityYieldContext &cities,runtime::MaintenanceContext &units,const runtime::ScienceContext &science,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded())throw std::runtime_error("Supply diagnostic requires options");
    const auto rules=runtime::load_vp_unit_supply_rules(db,types);runtime::restore_vp_unit_supply(snap,types,players,dip,cities,units);
    runtime::VpUnitSupplyContext c{units,players,dip,cities,techs,rules,db,defines,*options};
    const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    const auto gr=runtime::load_vp_city_growth_rules(db);const auto pass=runtime::restore_plot_passability(snap,db,types);
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};runtime::VpCityGrowthContext growth{religion,dip,science,gr};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Missing supply diagnostic source");};
    const auto component=[&](auto q){out<<'['<<q.handicap<<','<<q.cities<<','<<q.population<<','<<q.expended<<']';};
    out<<"{\"applicable\":true,\"supply_capacity_calculated\":true,\"city_yield_updater_complete\":false,\"cache_reset_on_import\":true,\"world_city_modifier\":"<<runtime::vp_world_city_supply_modifier(units)<<",\"options\":{";
    bool comma=false;for(const char *name:{"BALANCE_VP","BALANCE_MINOR_UNIT_SUPPLY_HANDICAP","HUMAN_USES_AI_HANDICAP"}){if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<options->enabled(name);}
    out<<"},\"game_options\":{";comma=false;for(const char *name:{"GAMEOPTION_NO_CHANGING_WAR_PEACE","GAMEOPTION_ALWAYS_WAR","GAMEOPTION_ALWAYS_PEACE"}){if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<players.game_option_value(db,name);}
    out<<"},\"defines\":{";comma=false;for(const char *name:{"AI_HANDICAP","UNIT_SUPPLY_BASE_TECH_REDUCTION_PER_ERA","UNIT_SUPPLY_CITIES_TECH_REDUCTION_MULTIPLIER","UNIT_SUPPLY_POPULATION_TECH_REDUCTION_MULTIPLIER",
        "PUPPET_YIELD_AND_SUPPLY_MODIFIER_MULTIPLICATIVE","UNIT_SUPPLY_WAR_WEARINESS_PERCENT_REDUCTION","MINOR_CIV_UNIT_SUPPLY_MODIFIER_CULTURED","MINOR_CIV_UNIT_SUPPLY_MODIFIER_MILITARISTIC","MINOR_CIV_UNIT_SUPPLY_MODIFIER_MARITIME",
        "MINOR_CIV_UNIT_SUPPLY_MODIFIER_MERCANTILE","MINOR_CIV_UNIT_SUPPLY_MODIFIER_RELIGIOUS","MINOR_CIV_UNIT_SUPPLY_MODIFIER_FRIENDLY","MINOR_CIV_UNIT_SUPPLY_MODIFIER_NEUTRAL","MINOR_CIV_UNIT_SUPPLY_MODIFIER_HOSTILE","MINOR_CIV_UNIT_SUPPLY_MODIFIER_IRRATIONAL","MINOR_CIV_UNIT_SUPPLY_MULTIPLIER_PER_EXTRA_CITY"}){
        const auto *d=defines.find(name);if(!d||!d->lookup.error.empty())throw std::runtime_error("Unresolved supply diagnostic define");if(comma)out<<',';comma=true;out<<std::quoted(name)<<':'<<std::get<std::int64_t>(d->lookup.value);}
    out<<"},\"era_base\":";pairs(rules.era_base);out<<",\"minor_traits\":"<<vp_unit_supply_minor_rules_report(rules);out<<",\"handicaps\":[";comma=false;
    for(const auto &[id,h]:rules.handicaps){if(comma)out<<',';comma=true;out<<'['<<id;for(const auto *b:{&h.player,&h.ai,&h.minor})out<<",["<<b->base<<','<<b->city<<','<<b->population<<','<<b->era_flat<<','<<b->era_modifier<<','<<b->percent<<']';out<<']';}
    const auto &td=snap.teams->dimensions;const auto &tr=*saves::vp_team_dimensions(td);
    out<<"],\"team_dimensions\":{\"domains\":"<<td.domains<<",\"major\":"<<td.major<<",\"yields\":"<<td.yields
       <<",\"vote_sources\":"<<tr.vote_sources<<",\"small_awards\":"<<tr.small_awards<<",\"techs\":"<<tr.techs<<",\"routes\":"<<tr.routes<<",\"builds\":"<<tr.builds<<",\"projects\":"<<tr.projects
       <<",\"unit_classes\":"<<tr.unit_classes<<",\"building_classes\":"<<tr.building_classes<<",\"buildings\":"<<tr.buildings<<",\"terrains\":"<<tr.terrains<<",\"features\":"<<tr.features<<",\"improvements\":"<<tr.improvements<<",\"victories\":"<<tr.victories
       <<"},\"major_players\":";list(dip.major_players);out<<",\"teams\":[";comma=false;
    for(const auto &st:snap.teams->teams){const auto id=named(st.fields,"id").value;const auto &t=dip.teams.at(id);if(comma)out<<',';comma=true;
        out<<'['<<id<<','<<st.start_offset<<','<<st.end_offset<<','<<t.master<<',';list(t.met);out<<',';list(t.at_war);out<<',';pairs(t.war_peace->locked_turns);out<<',';list(t.war_peace->permanent);out<<']';}
    out<<"],\"players\":[";comma=false;
    for(const auto &sp:snap.all_players->players){const auto id=sp.saved_player_id;const auto &p=runtime::vp_unit_supply_player(units,id);const auto team=players.players.at(id).team;
        if(p.base_cache||p.war_cache)throw std::runtime_error("Supply import unexpectedly retained unsaved cache");
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"team\":"<<team<<",\"team_era\":"<<units.team_eras.at(team)<<",\"major\":"<<dip.team_is_major(players,team)<<",\"minor\":"<<dip.team_is_minor(players,team)
           <<",\"barbarian\":"<<units.players.at(id).barbarian<<",\"human\":"<<players.is_human(players.players.at(id))<<",\"source_core\":["<<sp.core.start_offset<<','<<sp.core.end_offset<<','<<sp.core.yield_slots
           <<"],\"source_traits\":["<<sp.traits.start_offset<<','<<sp.traits.end_offset<<','<<sp.traits.dimensions.yields<<','<<sp.traits.dimensions.terrains
           <<"],\"source_planning\":["<<sp.planning.start_offset<<','<<sp.planning.end_offset<<','<<sp.planning.dimensions.major<<','<<sp.planning.dimensions.civ_teams<<','<<sp.planning.dimensions.legacy_teams
           <<"],\"state\":["<<p.military<<','<<p.free<<','<<p.extra_flat<<','<<p.city_flat<<','<<p.extra_population<<','<<p.expended<<','<<p.puppet<<','<<p.trait_base<<','<<p.trait_city<<','<<p.trait_population<<','<<p.trait_puppet<<','<<p.war_value<<','<<p.war_modifier<<','<<p.trait_war_modifier<<','<<vp_unit_supply_minor_type_report(rules,p.minor_type)<<','<<p.personality
           <<"],\"war_weariness\":";pairs(p.war_weariness);out<<",\"cities\":[";bool inner=false;
        for(const auto &sc:sp.cities.cities){const auto cid=named(sc.core.fields,"m_iID").values.at(0);const auto &city=cities.players.at(id).cities.at(cid);const auto &city_supply=runtime::vp_city_supply(cities,id,cid);if(inner)out<<',';inner=true;
            out<<'['<<cid<<','<<sc.core.start_offset<<','<<sc.core.end_offset<<','<<city.population<<','<<city.puppet<<','<<dip.players.at(id).cities.at(cid).ignore_happiness<<','<<city_supply.flat<<','<<city_supply.population_modifier<<']';}
        const auto base=runtime::vp_unit_supply_components(c,id),unreduced=runtime::vp_unit_supply_components(c,id,true);const auto war=runtime::vp_highest_war_weariness_percent(c,id);
        out<<"],\"components\":";component(base);out<<",\"unreduced\":";component(unreduced);out<<",\"war_percent\":"<<war;
        const auto q=runtime::vp_unit_supply(c,id);out<<",\"quote\":["<<q.effective_cities<<','<<q.city_reduction<<','<<q.empire_modifier<<','<<q.minor_modifier<<','<<q.extra_city_modifier<<','<<q.base<<','<<q.war_percent<<','<<q.capacity<<','<<q.units<<','<<q.over<<','<<q.unlimited<<','<<q.cached<<','<<q.minor_unready<<']';
        const auto no_war=runtime::vp_unit_supply(c,id,false),cached=runtime::vp_unit_supply(c,id);out<<",\"cache_queries\":["<<no_war.capacity<<','<<no_war.over<<','<<no_war.cached<<','<<cached.capacity<<','<<cached.over<<','<<cached.cached<<"],\"growth\":[";inner=false;
        for(const auto &[cid,city]:cities.players.at(id).cities){const auto g=runtime::vp_city_growth(growth,c,city);if(inner)out<<',';inner=true;out<<'['<<cid<<','<<g.supply<<','<<g.total<<']';}
        out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
