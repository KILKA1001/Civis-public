#pragma once
#include "civ5/runtime/vp_city_growth.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include <sstream>
#include <iomanip>

namespace civ5::validation {
inline std::string vp_city_growth_report(saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,
    const content::InfoTypeCatalog &types,const runtime::ResearchContext &players,const runtime::PlayerResearchContext &diplomacy,
    const runtime::ScienceContext &science,runtime::CityYieldContext &cities,const content::GlobalDefineCache &defines,const content::CustomModOptions *options){
    if(cities.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    if(!options||!options->all_succeeded())throw std::runtime_error("Growth diagnostic requires resolved options");
    const auto rules=runtime::load_vp_city_growth_rules(db);runtime::restore_vp_city_growth(snapshot,cities);
    const auto rr=runtime::load_vp_plot_religion_rules(db,types);
    const auto pass=runtime::restore_plot_passability(snapshot,db,types);const auto wr=runtime::load_plot_water_rules(db,players.profile);const auto ws=runtime::restore_plot_water(snapshot);
    runtime::PlotWaterContext water{*snapshot.map_projection,ws,wr,types,pass,players,cities,defines,options};runtime::VpPlotReligionContext religion{rr,water};
    runtime::VpCityGrowthContext context{religion,diplomacy,science,rules};std::ostringstream out;out<<std::setprecision(17);
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Missing growth diagnostic source");};
    out<<"{\"applicable\":true,\"city_yield_updater_complete\":false,\"supply_capacity_calculated\":false,\"balance\":"<<options->enabled("BALANCE_VP")
       <<",\"permanent_pantheons\":"<<options->enabled("BALANCE_PERMANENT_PANTHEONS")<<",\"no_happiness\":"<<cities.no_happiness<<",\"food\":"<<rules.food<<",\"defines\":{";
    bool comma=false;for(const char *name:{"UNHAPPY_THRESHOLD","VERY_UNHAPPY_THRESHOLD","GLOBAL_GROWTH_PENALTY_PER_UNHAPPY","LOCAL_HAPPINESS_FOOD_MODIFIER","LOCAL_UNHAPPINESS_FOOD_MODIFIER",
        "VERY_UNHAPPY_GROWTH_PENALTY","UNHAPPY_GROWTH_PENALTY","PUPPET_GROWTH_MODIFIER","WLTKD_GROWTH_MULTIPLIER","BALANCE_FOLLOWER_GROWTH_BONUS","GROWTH_PENALTY_PER_UNIT_OVER_SUPPLY","MAX_UNIT_SUPPLY_GROWTH_MOD"}){
        const auto *d=defines.find(name);if(!d||!d->lookup.error.empty())throw std::runtime_error("Unresolved growth diagnostic define");
        if(comma)out<<',';comma=true;out<<'"'<<name<<"\":";
        if(std::holds_alternative<double>(d->lookup.value))out<<std::get<double>(d->lookup.value);else out<<std::get<std::int64_t>(d->lookup.value);}
    out<<"},\"belief_rules\":[";comma=false;for(const auto &[id,b]:rules.beliefs){if(comma)out<<',';comma=true;out<<'['<<id<<','<<b.modifier<<','<<b.requires_peace<<']';}
    out<<"],\"civ_teams\":";list(diplomacy.civ_teams);out<<",\"teams\":[";comma=false;for(const auto &[id,t]:diplomacy.teams){if(comma)out<<',';comma=true;out<<'['<<id<<','<<t.alive_count<<',';list(t.at_war);out<<']';}
    const auto *profile=runtime::find_vp_city_growth_profile(cities);if(!profile)throw runtime::CityYieldUnavailable("Missing VP growth report profile");
    out<<"],\"players\":[";comma=false;for(const auto &sp:snapshot.all_players->players){const auto owner=sp.saved_player_id;const auto &p=cities.players.at(owner);const auto &g=profile->players.at(owner);
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<owner<<",\"team\":"<<players.players.at(owner).team<<",\"capital\":"<<p.capital<<",\"minor\":"<<p.minor<<",\"barbarian\":"<<science.barbarian_players.contains(owner)
           <<",\"golden_age\":"<<p.golden_age<<",\"happiness\":"<<runtime::vp_science_profile(science.players.at(owner)).cached_happiness_total<<",\"source_core\":["<<sp.core.start_offset<<','<<sp.core.end_offset<<','<<sp.core.yield_slots
           <<"],\"source_traits\":["<<sp.traits.start_offset<<','<<sp.traits.end_offset<<','<<sp.traits.dimensions.yields<<','<<sp.traits.dimensions.terrains<<"],\"growth\":["<<g.city<<','<<g.capital<<','<<g.boon<<','<<g.population_boost_religion
           <<"],\"golden_yields\":";pairs(g.golden_age);out<<",\"trait_golden_yields\":";pairs(g.trait_golden_age);out<<",\"cities\":[";
        bool inner=false;for(const auto &sc:sp.cities.cities){const auto id=named(sc.core.fields,"m_iID").values.at(0);const auto &city=p.cities.at(id);const auto &cg=profile->cities.at({owner,id});
            if(inner)out<<',';inner=true;out<<"{\"id\":"<<id<<",\"source_core\":["<<sc.core.start_offset<<','<<sc.core.end_offset<<"],\"puppet\":"<<city.puppet<<",\"celebration_turns\":"<<city.we_love_king_turns.value()
               <<",\"growth\":["<<cg.happiness_delta<<','<<cg.event<<','<<cg.tourism<<"],\"golden_yields\":";pairs(cg.golden_age);out<<",\"cases\":[";
            bool cc=false;for(const std::int64_t over:{0LL,1LL,3LL,100LL,4294967300LL,9223372036854775807LL})for(const std::int64_t assumed:{-10LL,0LL,10LL}){
                const auto q=runtime::vp_city_growth_modifiers(context,city,over,assumed);if(cc)out<<',';cc=true;
                out<<'['<<over<<','<<assumed<<','<<q.capital<<','<<q.player<<','<<q.golden_city<<','<<q.golden_player<<','<<q.golden_trait<<','<<q.supply<<','<<q.event<<','<<q.tourism<<','<<q.puppet
                   <<','<<q.beliefs<<','<<q.followers<<','<<q.secondary<<','<<q.pantheon<<','<<q.happiness<<','<<q.celebration<<','<<q.total<<','<<q.excess_happiness<<','<<q.at_peace<<','<<q.unhappy<<']';}
            out<<"]}";
        }out<<"]}";
    }out<<"]}";return out.str();
}
} // namespace civ5::validation
