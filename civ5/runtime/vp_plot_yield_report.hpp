#pragma once
#include "civ5/runtime/vp_plot_yield.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_plot_yield_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,const runtime::PlayerResearchContext &roster,const std::vector<runtime::TeamTechnologies> &technologies,
    runtime::CityYieldContext &cities,runtime::MaintenanceContext &units,const runtime::MaintenanceRules &unit_rules,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false,\"full_plot_yield\":false}";
    const auto pr=runtime::load_vp_player_plot_rules(db,types);const auto ps=runtime::restore_vp_player_plot_yields(snapshot,pr,cities);
    auto &map=*snapshot.map_projection;const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    auto ms=runtime::restore_plot_movement(snapshot,players,roster);runtime::PlotMovementContext movement{map,pass,players,roster,ms,types,defines};
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snapshot);
    runtime::PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,options};
    const auto rr=runtime::load_resource_visibility_rules(db,types,players.profile);auto rs=runtime::restore_resource_visibility(snapshot,players,rr,types);
    runtime::ResourceVisibilityContext resource{map,rs,rr,players,roster,technologies,types,defines,false};
    const auto ir=runtime::load_improvement_yield_rules(db,players.profile);const auto is=runtime::restore_improvement_yield_state(snapshot,ir,types,cities);
    runtime::ImprovementYieldContext improvements{ir,is,water,resource,units.team_eras};
    const auto nr=runtime::load_vp_nature_yield_rules(db);const auto ns=runtime::restore_vp_nature_yields(snapshot,nr,cities,units);
    const auto rel=runtime::load_vp_plot_religion_rules(db,types);runtime::restore_vp_plot_religion(snapshot,rel,cities);
    const auto active=snapshot.pregame->archive.integers.at("active_player");const auto active_team=players.players.at(active).team;
    runtime::VpNatureYieldContext nature{nr,ns,water,movement,cities,units,unit_rules,active_team};
    runtime::VpPlayerPlotContext player{pr,ps,improvements,nr,ns,movement,rel,active_team};runtime::VpPlotYieldContext context{nature,player,active};
    std::ostringstream out;
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    out<<"{\"applicable\":true,\"full_plot_yield\":true,\"yield_update_applied\":false,\"load_hooks_complete\":false,\"movement_cache_refreshed\":false,\"monopoly_cache_refreshed\":false,\"extra_cache_restored_empty\":true"
       <<",\"active_player\":"<<active<<",\"active_team\":"<<active_team<<",\"no_yield_ice\":"<<options->enabled("CORE_NO_YIELD_ICE")<<",\"modes\":[\"normal\",\"display\",\"assume_no_improvement\",\"debug_display\",\"display_assume_no_improvement\"],\"cells\":[";
    bool comma=false;for(const auto &p:map.cells){const auto *profile=saves::find_vp_plot_projection(p);
        if(!p.potential_city_work||!p.cached_yields||!profile||!profile->revealed_types)throw std::runtime_error("Missing VP full plot yield state");
        if(p.cached_yields->size()!=pr.yield_slots||profile->revealed_types->size()!=snapshot.map_plots->dimensions.teams||!profile->extra_yields.empty())throw std::runtime_error("Full plot source spans/reset mismatch");
        if(comma)out<<',';comma=true;const auto &known=profile->revealed_types->at(active_team);out<<'['<<*p.potential_city_work<<',';pairs(*p.cached_yields);
        out<<",["<<known.owner<<','<<known.improvement<<','<<known.route<<"],";pairs(profile->extra_yields);out<<']';}
    out<<"],\"cases\":[";comma=false;for(const auto &p:map.cells){if(comma)out<<',';comma=true;out<<'[';bool inner=false;
        for(const auto &[yield,y]:pr.yields)for(int mode=0;mode<5;++mode){resource.debug_mode=mode==3;
            const auto q=runtime::vp_plot_yield(context,p.index,yield,mode==1||mode==3||mode==4,nullptr,mode==2||mode==4);
            if(inner)out<<',';inner=true;out<<q.total;}out<<']';}resource.debug_mode=false;
    out<<"],\"final_garrisons\":[";comma=false;for(const auto &[owner,p]:cities.players)for(const auto &[id,city]:p.cities){if(comma)out<<',';comma=true;out<<'['<<owner<<','<<id<<','<<runtime::vp_city_plot_profile_city(cities,owner,id).yields->garrison<<']';}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
