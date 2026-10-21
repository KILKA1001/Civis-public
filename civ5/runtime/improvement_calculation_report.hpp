#pragma once
#include "civ5/runtime/improvement_yield_calculation.hpp"
#include "civ5/runtime/vp_improvement_yield_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string improvement_calculation_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,
    const content::InfoTypeCatalog &types,runtime::ResearchContext &players,const runtime::PlayerResearchContext &roster,
    const std::vector<runtime::TeamTechnologies> &technologies,runtime::CityYieldContext &cities,
    const std::map<std::int64_t,std::int64_t> &eras,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    const bool vp=players.profile==content::TechnologyProfile::vp542;auto &map=*snapshot.map_projection;
    const auto rules=runtime::load_improvement_yield_rules(db,players.profile);
    const auto state=runtime::restore_improvement_yield_state(snapshot,rules,types,cities);
    const auto rr=runtime::load_resource_visibility_rules(db,types,players.profile);
    auto rs=runtime::restore_resource_visibility(snapshot,players,rr,types);
    const runtime::ResourceVisibilityContext resource{map,rs,rr,players,roster,technologies,types,defines,false};
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snapshot);
    const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    runtime::PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,options};
    // Earlier plot_water diagnostic initialized and refreshed these same caches.
    const auto beliefs=vp?runtime::ImprovementReligionRules{}:runtime::load_improvement_religion_rules(db);
    runtime::ImprovementYieldContext context{rules,state,water,resource,eras,vp?nullptr:&beliefs};
    const auto resolve=[&](const auto &id,const auto &hash){if(id)return *id;if(hash){if(!*hash)return std::int64_t{-1};if(const auto *t=types.find_hash(*hash))return t->id;}throw std::runtime_error("Unresolved improvement diagnostic identity");};
    const auto city=[&](const auto &ref)->const runtime::CityYieldState*{if(!ref)throw std::runtime_error("Unresolved improvement diagnostic city reference");
        const auto p=cities.players.find(ref->owner);if(p==cities.players.end())return nullptr;const auto v=p->second.cities.find(ref->id);return v==p->second.cities.end()?nullptr:&v->second;};
    std::ostringstream out;
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto matrix=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,y]:v){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(y);out<<']';}out<<']';};
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(const auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto ref=[&](const auto &v){if(!v)throw std::runtime_error("Missing diagnostic city reference");out<<'['<<v->owner<<','<<v->id<<']';};
    out<<"{\"full_improvement_helper\":true,\"full_plot_yield\":false,\"load_hooks_complete\":false,\"source_pillage_filter_applied\":false,\"automatic_route\":"<<context.automatic_route
       <<",\"balance_vp\":"<<(options&&options->enabled("BALANCE_VP")?"true":"false")<<",\"players\":[";
    bool comma=false;for(const auto &[id,p]:state.players){if(comma)out<<',';comma=true;const auto &base=cities.players.at(id);
        out<<'['<<id<<','<<base.golden_age<<','<<base.second_pantheon_count.value()<<',';matrix(p.own);out<<',';matrix(p.traits);out<<',';
        if(vp){const auto &connections=runtime::vp_improvement_yield_player(state,id);
            list(connections.connections);out<<',';list(connections.industrial_connections);}
        else{list(std::vector<std::int64_t>{});out<<',';list(std::vector<std::int64_t>{});}
        out<<']';}
    out<<"],\"teams\":[";comma=false;for(const auto &[id,t]:state.teams){if(comma)out<<',';comma=true;out<<'['<<id<<','<<eras.at(id);
        for(const auto &m:t){out<<',';matrix(m);}out<<']';}
    out<<"],\"cities\":[";comma=false;for(const auto &[owner,p]:cities.players)for(const auto &[id,c]:p.cities){if(comma)out<<',';comma=true;
        out<<'['<<owner<<','<<id<<','<<c.we_love_king_turns.value()<<",[";bool inner=false;for(const auto &r:c.religions){if(inner)out<<',';inner=true;out<<'['<<r.religion<<','<<r.followers<<','<<r.pressure<<']';}out<<"]]";}
    out<<"],\"religions\":[";comma=false;for(const auto &r:cities.religions){if(comma)out<<',';comma=true;out<<'['<<r.religion<<','<<r.founder<<',';list(r.beliefs);out<<']';}
    out<<"],\"beliefs\":[";comma=false;for(const auto &[id,b]:beliefs.beliefs){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.pantheon<<',';matrix(b.improvements);out<<']';}
    out<<"],\"cells\":[";comma=false;for(const auto &p:map.cells){if(!p.route||!p.rivers)throw std::runtime_error("Unresolved diagnostic route/river");
        if(comma)out<<',';comma=true;out<<'['<<p.owner.value()<<','<<p.plot_type.value()<<','<<p.terrain_type.value()<<','<<resolve(p.feature_type,p.feature_type_hash)<<','
            <<runtime::plot_resource_type(resource,p.index)<<','<<resolve(p.improvement_type,p.improvement_type_hash)<<','<<p.artifact_era.value()<<','<<p.route->type<<','<<p.route->pillaged<<','
            <<p.rivers->is_river<<','<<p.river_crossing_bits.value()<<',';if(vp)out<<p.trade_route_units.value();else out<<"null";
        out<<','<<runtime::plot_is_fresh_water(water,p.index)<<','<<runtime::plot_is_coastal_land(water,p.index)<<','<<(vp?p.is_city:city(p.plot_city)!=nullptr)<<',';ref(p.plot_city);out<<',';ref(p.yield_city);out<<',';ref(p.yield_city_override);out<<']';}
    if(rules.improvements.empty())throw std::runtime_error("Improvement diagnostic needs an active improvement Info");
    const auto *farm=types.find("IMPROVEMENT_FARM");const auto fallback=farm&&rules.improvements.contains(farm->id)?farm->id:rules.improvements.begin()->first;
    out<<"],\"bnw_preview_improvement\":"<<fallback<<",\"cases\":[";comma=false;
    for(const auto &p:map.cells){auto improvement=resolve(p.improvement_type,p.improvement_type_hash);if(improvement==-1){if(vp)continue;improvement=fallback;}
        for(const auto y:rules.yields)for(int mode=0;mode<3;++mode){const auto player=mode==2?std::int64_t{-1}:p.owner.value();const bool optimal=mode==1;
            runtime::ImprovementYieldQuote q;
            if(vp){const auto actual_resource=runtime::plot_resource_type(resource,p.index,player==-1?-1:players.players.at(player).team);
                // VP IDInfo is invalid only when BOTH owner and ID are sentinels.
                const auto &override=p.yield_city_override;const auto *owning=city(override&&(override->owner!=-1||override->id!=-1)?override:p.yield_city);
                q=runtime::vp_improvement_yield(context,p.index,improvement,y,player,p.route->type,resolve(p.feature_type,p.feature_type_hash),actual_resource,owning,optimal);
            }else q=runtime::bnw_improvement_yield(context,p.index,improvement,y,player,optimal);
            if(comma)out<<',';comma=true;out<<'['<<p.index<<','<<improvement<<','<<y<<','<<mode<<','<<q.total()<<']';
        }
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
