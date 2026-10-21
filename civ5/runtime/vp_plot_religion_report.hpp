#pragma once
#include "civ5/runtime/vp_plot_religion.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/resource_visibility.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_plot_religion_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,const runtime::PlayerResearchContext &roster,const std::vector<runtime::TeamTechnologies> &technologies,
    runtime::CityYieldContext &cities,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false,\"full_plot_yield\":false}";
    const auto rules=runtime::load_vp_plot_religion_rules(db,types);runtime::restore_vp_plot_religion(snapshot,rules,cities);
    auto &map=*snapshot.map_projection;const auto pass=runtime::restore_plot_passability(snapshot,db,types);
    const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snapshot);
    runtime::PlotWaterContext water{map,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext context{rules,water};
    const auto rr=runtime::load_resource_visibility_rules(db,types,players.profile);auto rs=runtime::restore_resource_visibility(snapshot,players,rr,types);
    runtime::ResourceVisibilityContext resource{map,rs,rr,players,roster,technologies,types,defines,false};
    const auto active=snapshot.pregame->archive.integers.at("active_player");
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(const auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto matrix=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,y]:v){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(y);out<<']';}out<<']';};
    out<<"{\"applicable\":true,\"full_helpers\":true,\"full_plot_yield\":false,\"holy_city_only\":false,\"load_hooks_complete\":false,\"pillage_filter_applied\":false,\"active_player\":"<<active
       <<",\"unique_for_civ\":"<<options->enabled("BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV")<<",\"resource_slots\":"<<rules.resource_slots<<",\"yields\":";list(rules.yields);
    out<<",\"natural_wonders\":";list(rules.natural_wonders);out<<",\"beliefs\":[";bool comma=false;
    for(const auto &[id,b]:rules.beliefs){if(comma)out<<',';comma=true;out<<'['<<id<<",["<<b.pantheon<<','<<b.founder<<','<<b.follower<<','<<b.enhancer<<','<<b.reformation<<','<<b.improvement<<','<<b.no_improvement<<','<<b.resource<<','<<b.no_feature<<"],"<<b.civilization<<",[";
        bool inner=false;for(const auto *m:{&b.terrain,&b.plot,&b.feature,&b.unimproved_feature,&b.resources,&b.improvements}){if(inner)out<<',';inner=true;matrix(*m);}out<<"],";pairs(b.lake);out<<',';pairs(b.wonder);out<<']';}
    out<<"],\"improvements\":[";comma=false;for(const auto &[id,i]:rules.improvements){if(comma)out<<',';comma=true;out<<'['<<id<<','<<i.connects_all<<','<<i.great_person<<','<<i.cityside<<','<<i.ignore_ownership<<','<<i.created_resource<<',';list(i.trades);out<<',';list(i.valid_resources);out<<']';}
    out<<"],\"religions\":[";comma=false;for(const auto &r:cities.religions){const auto &s=runtime::vp_religion_beliefs(r);if(comma)out<<',';comma=true;out<<'['<<r.religion<<','<<r.founder<<','<<s.belief_religion<<','<<s.holy_x<<','<<s.holy_y<<','<<s.pantheon<<',';list(r.beliefs);out<<',';pairs(s.lookup);out<<']';}
    out<<"],\"players\":[";comma=false;for(const auto &[id,p]:cities.players){if(comma)out<<',';comma=true;out<<'['<<id<<','<<p.civilization<<','<<vp_player_state_religion(p).value()<<','<<p.second_pantheon_count.value()<<']';}
    out<<"],\"cities\":[";comma=false;for(const auto &[owner,p]:cities.players)for(const auto &[id,city]:p.cities){if(comma)out<<',';comma=true;
        const auto selection=runtime::vp_plot_religion_selection(cities,rules,city);const auto selected=selection.majority?selection.majority-cities.religions.data():-1;
        out<<'['<<owner<<','<<id<<','<<vp_city_majority_religion(city).value()<<",[";bool inner=false;for(const auto &r:city.religions){if(inner)out<<',';inner=true;out<<'['<<r.religion<<','<<r.followers<<','<<r.pressure<<']';}out<<"],"<<selected<<','<<selection.secondary<<']';}
    const auto owning=[&](const auto &p)->const runtime::CityYieldState*{const auto &ref=p.yield_city_override&&(p.yield_city_override->owner!=-1||p.yield_city_override->id!=-1)?p.yield_city_override:p.yield_city;
        if(!ref)throw std::runtime_error("Unresolved plot religion owning city");const auto owner=cities.players.find(ref->owner);if(owner==cities.players.end())return nullptr;const auto city=owner->second.cities.find(ref->id);return city==owner->second.cities.end()?nullptr:&city->second;};
    out<<"],\"cases\":[";comma=false;for(const auto &p:map.cells){if(comma)out<<',';comma=true;out<<'[';bool inner=false;const auto *city=owning(p);
        const auto selection=city?runtime::vp_plot_religion_selection(cities,rules,*city):runtime::VpPlotReligionSelection{};
        for(const auto yield:rules.yields)for(int mode=0;mode<5;++mode){auto player=mode==2?std::int64_t{-1}:mode==1?active:p.owner.value();if(mode!=1&&mode!=2&&player==-1&&city)player=city->owner;
            const auto res=runtime::plot_resource_type(resource,p.index,player==-1?-1:players.players.at(player).team);
            const auto improvement=mode==3?-1:p.improvement_type.value(),feature=mode==4?-1:p.feature_type.value();
            const auto nature=runtime::vp_religion_nature_yield(context,p.index,yield,player,improvement,feature,res,city,selection);
            const auto improvement_yield=runtime::vp_religion_improvement_yield(context,yield,player,improvement,res,city,selection);
            if(inner)out<<',';inner=true;out<<nature.total()<<','<<improvement_yield;
        }out<<']';}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
