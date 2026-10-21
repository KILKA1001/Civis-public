#pragma once
#include "civ5/runtime/plot_movement.hpp"
#include "civ5/runtime/vp_plot_movement_profile.hpp"
#include "civ5/runtime/vp_plot_passability_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string plot_movement_report(const saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,
    const content::InfoTypeCatalog &types,const runtime::ResearchContext &players,
    const runtime::PlayerResearchContext &diplomacy,const content::GlobalDefineCache &defines){
    if(players.profile!=content::TechnologyProfile::vp542)
        return "{\"applicable\":false,\"full_nature_yield\":false,\"unit_moves_executed\":false}";
    const auto &map=*snapshot.map_projection;const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    auto state=runtime::restore_plot_movement(snapshot,players,diplomacy);
    runtime::PlotMovementContext c{map,pass,players,diplomacy,state,types,defines};
    const auto define=[&](const char *name){const auto *d=defines.find(name);
        if(!d||!d->lookup.error.empty()||!std::holds_alternative<std::int64_t>(d->lookup.value))throw std::runtime_error("Missing movement diagnostic define");
        return std::get<std::int64_t>(d->lookup.value);};
    const auto feature=[&](const auto &p){if(p.feature_type)return *p.feature_type;if(p.feature_type_hash){if(!*p.feature_type_hash)return std::int64_t{-1};
        if(const auto *t=types.find_hash(*p.feature_type_hash))return t->id;}throw std::runtime_error("Missing movement diagnostic feature");};
    std::vector<std::int64_t> ids{-1},teams;for(const auto &[id,p]:state.players)ids.push_back(id);for(const auto &[id,t]:state.teams)teams.push_back(id);
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(const auto id:v){if(comma)out<<',';comma=true;out<<id;}out<<']';};
    out<<"{\"applicable\":true,\"full_nature_yield\":false,\"unit_moves_executed\":false,\"load_hooks_complete\":false,\"water_terrains\":["<<define("DEEP_WATER_TERRAIN")<<','<<define("SHALLOW_WATER_TERRAIN")<<"],\"player_ids\":";
    list(ids);out<<",\"team_ids\":";list(teams);out<<",\"players\":[";bool comma=false;
    for(const auto &[id,p]:state.players){if(comma)out<<',';comma=true;out<<'['<<id<<','<<players.players.at(id).team<<','<<p.embarked_all_water<<','<<p.mountain_pass<<','<<p.mountains_after_general<<','<<p.crosses_ice<<','<<p.generals_created<<','<<diplomacy.players.at(id).pregame_minor<<",["
        <<runtime::player_can_embark(c,id)<<','<<runtime::player_can_cross_ocean(c,id)<<','<<runtime::player_can_cross_mountain(c,id)<<','<<runtime::player_can_cross_ice(c,id)<<"]]";}
    out<<"],\"teams\":[";comma=false;for(const auto &[id,t]:state.teams){if(comma)out<<',';comma=true;const auto &d=diplomacy.teams.at(id);
        out<<'['<<id<<','<<t.embark_count<<','<<t.all_water_count<<','<<d.master<<','<<d.liberated_by<<',';list(t.open_borders);out<<',';list(d.at_war);out<<',';list(d.members);out<<"]";}
    out<<"],\"cells\":[";comma=false;for(const auto &p:map.cells){if(!p.route)throw std::runtime_error("Unresolved movement route");
        if(comma)out<<',';comma=true;out<<'['<<p.owner.value()<<','<<p.plot_type.value()<<','<<p.terrain_type.value()<<','<<feature(p)<<','<<p.route->type<<','<<p.route->pillaged<<','<<p.is_city<<','<<pass.plots.at(p.index).impassable<<",\"";
        for(const auto team:teams)out<<(runtime::vp_plot_team_impassable(pass,p.index,team)?'1':'0');out<<"\"]";}
    out<<"],\"passes\":[";
    for(int round=0;round<2;++round){if(round)out<<',';
        if(round)for(const auto team:teams)runtime::refresh_plot_movement_team_status(c,team);
        out<<"{\"minor_teams\":[";comma=false;for(const auto team:teams)if(runtime::vp_plot_movement_minor(state,team)){if(comma)out<<',';comma=true;out<<team;}
        out<<"],\"open_borders\":[";comma=false;for(const auto from:teams){if(comma)out<<',';comma=true;out<<'\"';for(const auto to:teams)out<<(runtime::movement_open_borders(c,from,to)?'1':'0');out<<'\"';}
        out<<"],\"plots\":[";comma=false;for(const auto &p:map.cells){if(comma)out<<',';comma=true;out<<'[';
            for(int territory=0;territory<2;++territory){if(territory)out<<',';out<<'\"';for(const auto player:ids)out<<(runtime::plot_is_valid_move(c,p.index,player,territory!=0)?'1':'0');out<<'\"';}out<<']';}
        out<<"]}";
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
