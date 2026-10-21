#pragma once
#include "civ5/runtime/vp_city_trait_yields.hpp"
#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_city_trait_yields_report(saves::SavedSessionSnapshot &snap,const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
    runtime::ResearchContext &players,runtime::CityYieldContext &cities,const runtime::TreasuryContext &treasury,
    const std::map<std::int64_t,std::int64_t> &team_eras,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(players.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    const auto rules=runtime::load_vp_city_trait_yield_rules(db,types);runtime::restore_vp_city_trait_yields(snap,rules,cities,treasury);
    runtime::VpCityTraitYieldContext context{rules,players,cities,treasury,team_eras,defines,*options};
    std::ostringstream out;const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto rows=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<',';pairs(n);out<<']';}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Missing city trait source");};
    out<<"{\"applicable\":true,\"contributions_complete\":true,\"city_yield_updater_complete\":false,\"balance_vp\":"<<options->enabled("BALANCE_VP")
        <<",\"improvement_slots\":"<<rules.improvement_slots<<",\"yield_slots\":"<<rules.yield_slots<<",\"culture\":"<<rules.culture<<",\"tourism\":"<<rules.tourism<<",\"production\":"<<cities.production<<",\"yields\":";list(rules.yields);
    out<<",\"connection_defines\":[";bool comma=false;for(const auto *name:{"TRADE_ROUTE_BASE_GOLD","TRADE_ROUTE_CAPITAL_POP_GOLD_MULTIPLIER","TRADE_ROUTE_CITY_POP_GOLD_MULTIPLIER"}){if(comma)out<<',';comma=true;out<<std::get<std::int64_t>(defines.find(name)->lookup.value);}
    out<<"],\"source_trade\":["<<snap.trade.start_offset<<','<<snap.trade.end_offset<<','<<snap.trade.dimensions.major_slots<<','<<snap.trade.dimensions.yield_slots<<"],\"routes\":[";comma=false;
    for(const auto &r:cities.trade_routes){if(comma)out<<',';comma=true;out<<'['<<r.origin_owner<<','<<r.destination_owner<<','<<r.origin_x<<','<<r.origin_y<<','<<r.destination_x<<','<<r.destination_y<<']';}
    out<<"],\"players\":[";comma=false;
    for(const auto &sp:snap.all_players->players){const auto id=sp.saved_player_id;const auto &p=cities.players.at(id);const auto &a=runtime::vp_city_trait_yield_player(cities,id);const auto &te=runtime::vp_city_trait_era(cities,id);const auto &pt=*sp.tail.context;const auto &t=treasury.players.at(id);
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<id<<",\"team\":"<<players.players.at(id).team<<",\"era\":"<<team_eras.at(players.players.at(id).team)
            <<",\"source_core\":["<<sp.core.start_offset<<','<<sp.core.end_offset<<','<<sp.core.yield_slots<<"],\"source_traits\":["<<sp.traits.start_offset<<','<<sp.traits.end_offset<<','<<sp.traits.dimensions.yields<<','<<sp.traits.dimensions.terrains
            <<"],\"source_administration\":["<<sp.administration.start_offset<<','<<sp.administration.end_offset<<"],\"source_tail\":["<<sp.tail.start_offset<<','<<sp.tail.end_offset
            <<"],\"tail_context\":{\"yields\":"<<pt.yields<<",\"players\":"<<pt.players<<",\"major\":"<<pt.major<<",\"player_id\":"<<pt.player_id<<"},\"capital\":"<<p.capital<<",\"golden_age\":"<<p.golden_age
            <<",\"era_scaling\":["<<te.era_scaling<<','<<te.odd_era<<','<<te.fractional_era<<"],\"era_modifier\":"<<runtime::vp_trait_era_modifier(te,team_eras.at(players.players.at(id).team))
            <<",\"connection_modifier\":"<<t.connection_modifier<<",\"connection_change\":"<<t.connection_change<<",\"partners\":"<<runtime::vp_different_trade_partners(cities,id)
            <<",\"capital_only\":"<<a.capital_only<<",\"historic_events\":"<<a.historic_events<<",\"city_culture\":"<<a.city_culture<<",\"golden_age_tourism\":"<<a.golden_age_tourism;
#define FIELD(n) out<<",\"" #n "\":";pairs(a.n)
        FIELD(improvements_built);FIELD(historic);FIELD(per_partner);FIELD(land_connections);FIELD(connection);FIELD(trait_connection);
#undef FIELD
        out<<",\"per_improvement\":";rows(a.per_improvement);out<<",\"improvement_types\":";list(a.improvement_types);out<<",\"cities\":[";bool inner=false;
        for(const auto &sc:sp.cities.cities){const auto cid=named(sc.core.fields,"m_iID").values.at(0);const auto &city=p.cities.at(cid);const auto &v=runtime::vp_city_trait_yield_city(cities,id,cid);
            if(inner)out<<',';inner=true;out<<"{\"id\":"<<cid<<",\"source_core\":["<<sc.core.start_offset<<','<<sc.core.end_offset<<"],\"population\":"<<city.population
                <<",\"connected\":"<<v.connected<<",\"industrial\":"<<v.industrial<<",\"connection_modifier\":"<<v.connection_modifier<<",\"connection_gold\":"<<runtime::vp_city_connection_gold_times100(context,&city)<<",\"cached\":";pairs(runtime::vp_city_cached_yields(cities,id,cid));
#define FIELD(n) out<<",\"" #n "\":";pairs(v.n)
            FIELD(added);FIELD(unimproved);FIELD(permanent_wltkd);
#undef FIELD
            out<<",\"quotes\":[";bool row=false;for(auto y:rules.yields){const auto q=runtime::vp_city_trait_yield(context,city,y);const auto z=runtime::vp_city_connection_yield(context,city,y);if(row)out<<',';row=true;
                out<<'['<<y<<",["<<q.per_turn.improvements<<','<<q.per_turn.historic<<','<<q.per_turn.partners<<','<<q.per_turn.added<<','<<q.per_turn.total<<"],["<<q.traits<<','<<q.unimproved<<','<<q.permanent_wltkd<<','<<q.culture<<','<<q.land_connections<<','<<q.golden_age_tourism<<','<<q.total_times100<<"],["<<z.route<<','<<z.industrial<<','<<z.total_times100<<"]]";}
            out<<"]}";
        }out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
