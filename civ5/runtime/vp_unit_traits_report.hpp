#pragma once
#include "civ5/runtime/vp_unit_traits.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_unit_traits_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,const runtime::ResearchTurnState &turn,const runtime::PlayerResearchContext &dip,
    const std::vector<runtime::TeamTechnologies> &teams,runtime::CityYieldContext &cities,const runtime::TreasuryContext &treasury,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded()||!snap.map_projection)throw std::runtime_error("VP unit traits require resolved options and map");
    const auto rules=runtime::load_vp_unit_trait_rules(db,types);runtime::restore_vp_unit_traits(rules,players,turn,dip);
    const auto pass=runtime::restore_plot_passability(snap,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);
    const auto ws=runtime::restore_plot_water(snap);const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};
    runtime::VpPlotReligionContext religion{rr,water};runtime::VpCityReligionMinorContext minor{religion,treasury};
    runtime::VpUnitTraitContext c{rules,players,turn,dip,teams,minor};
    std::ostringstream out;
    const auto quote=[&](std::string_view s){out<<'"';constexpr char hex[]="0123456789abcdef";for(unsigned char ch:s){
        if(ch=='"'||ch=='\\')out<<'\\'<<static_cast<char>(ch);
        else if(ch<0x20)out<<"\\u00"<<hex[ch>>4]<<hex[ch&15];else out<<static_cast<char>(ch);}
        out<<'"';};
    const auto ids=[&](const auto &v){out<<'[';bool comma=false;for(auto id:v){if(comma)out<<',';comma=true;out<<id;}out<<']';};
    std::set<std::int64_t> belief_ids;out<<"{\"applicable\":true,\"trait_gates_complete\":true,\"unit_modifiers_complete\":false,\"trait_slots\":"<<rules.trait_slots<<",\"leader_slots\":"<<rules.leader_slots<<",\"traits\":[";
    bool comma=false;for(const auto &[id,r]:rules.traits){if(comma)out<<',';comma=true;out<<'['<<id<<','<<r.obsolete_tech<<','<<r.prereq_tech<<','<<r.obsolete_belief<<','<<r.prereq_belief<<','<<r.obsolete_policy<<','<<r.prereq_policy<<']';
        if(r.obsolete_belief!=-1)belief_ids.insert(r.obsolete_belief);if(r.prereq_belief!=-1)belief_ids.insert(r.prereq_belief);}
    out<<"],\"leaders\":[";comma=false;for(const auto &[leader,traits]:rules.leader_traits){if(comma)out<<',';comma=true;out<<'[';quote(leader);out<<',';ids(traits);out<<']';}
    out<<"],\"players\":[";comma=false;for(const auto &[id,p]:players.players){if(comma)out<<',';comma=true;
        out<<"{\"id\":"<<id<<",\"team\":"<<p.team<<",\"major\":"<<dip.major_players.contains(id)<<",\"alive\":"<<p.alive<<",\"leader\":";
        const auto &leader=turn.players.at(id).leader_type;if(leader)quote(*leader);else out<<"null";
        out<<",\"policies\":";ids(p.policies);out<<",\"owned_religion\":"<<runtime::vp_owned_religion(minor,id)
           <<",\"created_with_pantheon\":"<<runtime::vp_created_religion(water,id,true)
           <<",\"potential\":";const auto &potential=runtime::vp_potential_unit_traits(c,id);ids(potential);out<<",\"active\":[";
        bool row=false;for(auto trait:potential)if(runtime::vp_unit_trait_active(c,id,trait)){if(row)out<<',';row=true;out<<trait;}out<<"],\"beliefs\":[";
        row=false;for(auto belief:belief_ids){if(row)out<<',';row=true;out<<'['<<belief<<','<<runtime::vp_unit_trait_player_has_belief(c,id,belief)<<']';}
        out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
