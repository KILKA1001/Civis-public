#pragma once
#include "civ5/saves/vp_city_tail_profile.hpp"
#include "civ5/runtime/vp_city_religion_minor_yields.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_city_religion_minor_yields_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::CityYieldContext &cities,const runtime::TreasuryContext &treasury,
    const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    runtime::restore_vp_city_religion_minor_yields(snap,cities,treasury);const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    const auto pass=runtime::restore_plot_passability(snap,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snap);
    runtime::PlotWaterContext water{*snap.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};runtime::VpCityReligionMinorContext context{religion,treasury};
    std::ostringstream out;const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto rows=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(n);out<<']';}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Missing city religion/minor source");};
    out<<"{\"applicable\":true,\"contributions_complete\":true,\"city_yield_updater_complete\":false,\"balance_vp\":"<<options->enabled("BALANCE_VP")<<",\"city_state_scale\":"<<options->enabled("BALANCE_CITY_STATE_SCALE")
        <<",\"scale_per_city\":"<<std::get<std::int64_t>(defines.find("CITY_STATE_SCALE_PER_CITY_MOD")->lookup.value)<<",\"yields\":[";bool comma=false;for(auto y:rr.yields){if(comma)out<<',';comma=true;out<<y;}
    out<<"],\"players\":[";comma=false;
    for(const auto &sp:snap.all_players->players){const auto id=sp.saved_player_id;const auto &p=cities.players.at(id);const auto &a=runtime::vp_city_religion_minor_player(cities,id);
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"source_core\":["<<sp.core.start_offset<<','<<sp.core.end_offset<<','<<sp.core.yield_slots
            <<"],\"source_traits\":["<<sp.traits.start_offset<<','<<sp.traits.end_offset<<','<<sp.traits.dimensions.yields<<','<<sp.traits.dimensions.terrains
            <<"],\"allies\":"<<a.allies<<",\"friends\":"<<a.friends<<",\"owned_religion\":"<<runtime::vp_owned_religion(context,id)<<",\"created_religion\":"<<runtime::vp_created_religion(water,id)
            <<",\"created_with_pantheon\":"<<runtime::vp_created_religion(water,id,true)<<",\"created_pantheon\":"<<runtime::vp_created_pantheon(cities,id)<<",\"own_pantheon\":";pairs(a.own_pantheon);
        out<<",\"cities\":[";bool inner=false;
        for(const auto &sc:sp.cities.cities){const auto cid=named(sc.core.fields,"m_iID").values.at(0);const auto &city=p.cities.at(cid);const auto &v=runtime::vp_city_religion_minor_city(cities,id,cid);const auto &ct=*saves::vp_city_tail_dimensions(sc.tail.context);
            if(inner)out<<',';inner=true;out<<"{\"id\":"<<cid<<",\"source_core\":["<<sc.core.start_offset<<','<<sc.core.end_offset<<"],\"source_tail\":["<<sc.tail.start_offset<<','<<sc.tail.end_offset
                <<"],\"tail_context\":{\"vp_rows\":{\"yields\":"<<ct.yields<<",\"emphases\":"<<ct.emphases<<"}},\"base_religion\":";
            runtime::YieldValues base;for(const auto &[y,z]:city.yields)if(z.religion)base[y]=z.religion;pairs(base);
#define FIELD(n) out<<",\"" #n "\":";pairs(v.n)
            FIELD(special_religion);FIELD(alliance);FIELD(friendship);FIELD(per_ally);FIELD(per_friend);FIELD(from_minors);
#undef FIELD
            out<<",\"terrain_religion\":";rows(v.terrain_religion);out<<",\"feature_religion\":";rows(v.feature_religion);out<<",\"quotes\":[";bool row=false;
            for(auto y:rr.yields){const auto r=runtime::vp_city_religion_yield(context,city,y);const auto m=runtime::vp_city_minor_yield(context,city,y);if(row)out<<',';row=true;
                out<<'['<<y<<",["<<r.base<<','<<r.terrain<<','<<r.features<<','<<r.own_pantheon<<','<<r.special<<','<<r.total_times100<<"],["<<m.modifier<<','<<m.city_scaling<<','<<m.alliance_base<<','<<m.friendship_base<<','<<m.per_ally<<','<<m.per_friend<<','<<m.from_minors<<','<<m.total_times100<<"]]";}
            out<<"]}";
        }out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
