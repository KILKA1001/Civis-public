#pragma once
#include "civ5/runtime/vp_player_plot_yield.hpp"
#include "civ5/runtime/vp_player_accomplishments_profile.hpp"
#include "civ5/runtime/vp_city_plot_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_player_plot_yield_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,const runtime::PlayerResearchContext &roster,const std::vector<runtime::TeamTechnologies> &technologies,
    runtime::CityYieldContext &cities,runtime::MaintenanceContext &units,const content::GlobalDefineCache &defines,const content::CustomModOptions *options,
    const std::map<std::int64_t,std::int64_t> &eras){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false,\"full_plot_yield\":false}";
    const auto rules=runtime::load_vp_player_plot_rules(db,types);const auto state=runtime::restore_vp_player_plot_yields(snapshot,rules,cities);
    auto &map=*snapshot.map_projection;const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    auto ms=runtime::restore_plot_movement(snapshot,players,roster);runtime::PlotMovementContext movement{map,pass,players,roster,ms,types,defines};
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snapshot);
    runtime::PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,options};
    const auto rr=runtime::load_resource_visibility_rules(db,types,players.profile);auto rs=runtime::restore_resource_visibility(snapshot,players,rr,types);
    runtime::ResourceVisibilityContext resource{map,rs,rr,players,roster,technologies,types,defines,false};
    const auto ir=runtime::load_improvement_yield_rules(db,players.profile);const auto is=runtime::restore_improvement_yield_state(snapshot,ir,types,cities);
    runtime::ImprovementYieldContext improvements{ir,is,water,resource,eras};
    const auto nr=runtime::load_vp_nature_yield_rules(db);const auto ns=runtime::restore_vp_nature_yields(snapshot,nr,cities,units);
    const auto rel=runtime::load_vp_plot_religion_rules(db,types);runtime::restore_vp_plot_religion(snapshot,rel,cities);
    const auto active=snapshot.pregame->archive.integers.at("active_player");const auto active_team=players.players.at(active).team;
    runtime::VpPlayerPlotContext context{rules,state,improvements,nr,ns,movement,rel,active_team};
    std::ostringstream out;
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(const auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto matrix=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,row]:v){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(row);out<<']';}out<<']';};
    out<<"{\"applicable\":true,\"full_player_helper\":true,\"full_plot_yield\":false,\"load_hooks_complete\":false,\"movement_cache_refreshed\":false,\"active_player\":"<<active<<",\"active_team\":"<<active_team
       <<",\"extra_yield\":"<<std::get<std::int64_t>(defines.find("EXTRA_YIELD")->lookup.value)<<",\"permanent_pantheons\":"<<options->enabled("BALANCE_PERMANENT_PANTHEONS")<<",\"balance_vp\":"<<options->enabled("BALANCE_VP")
       <<",\"monopolies\":"<<options->enabled("BALANCE_RESOURCE_MONOPOLIES")<<",\"yields\":[";bool comma=false;
    for(const auto &[id,y]:rules.yields){if(comma)out<<',';comma=true;out<<'['<<id<<','<<y.golden_threshold<<','<<y.golden<<']';}
    out<<"],\"resources\":[";comma=false;for(const auto &[id,r]:rules.resources){if(comma)out<<',';comma=true;out<<'['<<id<<','<<r.usage<<',';pairs(r.monopoly);out<<']';}
    out<<"],\"accomplishment_rules\":[";comma=false;for(const auto &[id,r]:rules.accomplishments){if(comma)out<<',';comma=true;out<<'['<<id<<',';matrix(r);out<<']';}
    out<<"],\"policies\":[";comma=false;for(const auto &[id,r]:rules.policies){if(comma)out<<',';comma=true;out<<'['<<id<<','<<r.branch<<',';pairs(r.improvement_culture);out<<']';}
    out<<"],\"belief_wonder\":";matrix(rules.belief_wonder_modifier);
    out<<",\"teams\":[";comma=false;for(const auto &[id,t]:state.teams){if(comma)out<<',';comma=true;out<<'['<<id<<',';matrix(t[0]);out<<',';matrix(t[1]);out<<']';}
    out<<"],\"players\":[";comma=false;for(const auto &[id,p]:state.players){if(comma)out<<',';comma=true;out<<'['<<id<<",[";bool inner=false;
        for(const auto &v:p.yields){if(inner)out<<',';inner=true;pairs(v);}out<<"],[";inner=false;for(const auto &m:p.matrices){if(inner)out<<',';inner=true;matrix(m);}out<<"],";
        const auto &era=runtime::vp_city_trait_era(cities,id);pairs(runtime::vp_player_accomplishments(cities,id));out<<',';list(p.blocked_branches);out<<",["<<p.golden_ages<<','<<p.monopoly_flat<<','<<p.wonder_modifier<<','<<p.wltkd_gp_modifier<<','<<era.fractional_era<<','<<p.policy_culture<<','<<p.trade_route_only<<','<<era.era_scaling<<','<<era.odd_era<<"],";
        list(players.players.at(id).policies);out<<']';}
    out<<"],\"leagues\":[";comma=false;for(const auto &l:cities.leagues){if(comma)out<<',';comma=true;out<<'[';list(l.members);out<<',';list(l.culture_per_natural_wonder);out<<']';}
    out<<"],\"cities\":[";comma=false;for(const auto &[owner,p]:cities.players)for(const auto &[id,city]:p.cities){if(comma)out<<',';comma=true;const auto &a=*runtime::vp_city_plot_profile_city(cities,owner,id).bonuses;
        out<<'['<<owner<<','<<id<<",[";bool inner=false;for(const auto *v:{&a.sea,&a.river,&a.lake,&a.sea_resource,&a.connection,&a.luxury}){if(inner)out<<',';inner=true;pairs(*v);}out<<"],[";inner=false;
        for(const auto *v:{&a.terrain,&a.feature,&a.improvement,&a.resource,&a.plot,&a.event_terrain,&a.event_feature,&a.event_improvement,&a.event_resource}){if(inner)out<<',';inner=true;matrix(*v);}out<<"]]";}
    const auto owning=[&](const auto &p)->const runtime::CityYieldState*{const auto &ref=p.yield_city_override&&(p.yield_city_override->owner!=-1||p.yield_city_override->id!=-1)?p.yield_city_override:p.yield_city;
        if(!ref)throw std::runtime_error("Missing player plot owning city");const auto owner=cities.players.find(ref->owner);if(owner==cities.players.end())return nullptr;const auto city=owner->second.cities.find(ref->id);return city==owner->second.cities.end()?nullptr:&city->second;};
    out<<"],\"cases\":[";comma=false;for(const auto &p:map.cells){if(comma)out<<',';comma=true;out<<'[';bool inner=false;const auto *city=owning(p);
        const auto selection=city?runtime::vp_plot_religion_selection(cities,rel,*city):runtime::VpPlotReligionSelection{};
        const auto *pantheon=city?runtime::vp_find_religion(cities,cities.pantheon,city->owner):nullptr;
        for(const auto &[yield,y]:rules.yields)for(int mode=0;mode<6;++mode){auto player=mode==2?std::int64_t{-1}:mode==1?active:p.owner.value();if(mode!=1&&mode!=2&&player==-1&&city)player=city->owner;
            const auto res=runtime::plot_resource_type(resource,p.index,player==-1?-1:players.players.at(player).team);
            const auto n=runtime::vp_player_plot_yield(context,p.index,yield,mode==4?107:0,player,mode==3?-1:p.improvement_type.value(),p.feature_type.value(),res,city,selection,
                mode==4?pantheon:nullptr,mode==1,mode==5?std::optional<std::int64_t>{-1}:std::nullopt);
            if(inner)out<<',';inner=true;out<<n;
        }out<<']';}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
