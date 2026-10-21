#pragma once
#include "civ5/runtime/vp_city_food.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include "civ5/saves/vp_city_management_profile.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string vp_city_food_report(const saves::SavedSessionSnapshot &snapshot,const content::RulesDatabase &db,
    runtime::CityYieldContext &cities,const runtime::ResearchContext &players,const std::map<std::int64_t,std::int64_t> &eras,
    const content::GlobalDefineCache &defines){
    if(cities.profile!=content::TechnologyProfile::vp542)return "{\"applicable\":false}";
    const auto rules=runtime::load_vp_city_food_rules(db);runtime::restore_vp_city_food(snapshot,rules,cities);
    runtime::VpCityFoodContext context{cities,players,eras,rules,defines};std::ostringstream out;
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(const auto &[id,n]:v){if(comma)out<<',';comma=true;out<<'['<<id<<','<<n<<']';}out<<']';};
    const auto named=[](const auto &v,const char *name)->const auto&{for(const auto &x:v)if(std::string_view(x.name)==name)return x;throw std::runtime_error("Missing food diagnostic source");};
    const auto define=[&](const char *name){const auto *v=defines.find(name);
        if(!v||!v->lookup.error.empty()||!std::holds_alternative<std::int64_t>(v->lookup.value))throw std::runtime_error("Unresolved food diagnostic define");
        return std::get<std::int64_t>(v->lookup.value);};
    out<<"{\"applicable\":true,\"city_yield_updater_complete\":false,\"fresh_food_income_calculated\":false,\"specialist_slots\":"<<rules.specialist_slots
       <<",\"promotion_wire_slots\":"<<rules.promotion_slots<<",\"food_per_population\":"<<define("FOOD_CONSUMPTION_PER_POPULATION")
       <<",\"default_specialist\":"<<define("DEFAULT_SPECIALIST")<<",\"era_extra\":";pairs(rules.specialist_extra_by_era);
    out<<",\"unit_rules\":[";bool comma=false;for(const auto &[id,u]:rules.units){if(comma)out<<',';comma=true;out<<'['<<id<<','<<u.food<<','<<u.military<<']';}
    out<<"],\"players\":[";comma=false;
    const auto *profile=runtime::find_vp_city_food_profile(cities);if(!profile)throw runtime::CityYieldUnavailable("Missing VP food report profile");
    for(const auto &sp:snapshot.all_players->players){const auto owner=sp.saved_player_id;const auto &p=cities.players.at(owner);const auto &f=profile->players.at(owner);
        if(comma)out<<',';comma=true;out<<"{\"id\":"<<owner<<",\"team\":"<<players.players.at(owner).team<<",\"era\":"<<eras.at(players.players.at(owner).team)<<",\"capital\":"<<p.capital
            <<",\"source_core\":["<<sp.core.start_offset<<','<<sp.core.end_offset<<','<<sp.core.yield_slots<<"],\"source_traits\":["<<sp.traits.start_offset<<','<<sp.traits.end_offset<<','<<sp.traits.dimensions.yields<<','<<sp.traits.dimensions.terrains
            <<"],\"food\":["<<f.non_specialist_change_times100<<','<<f.specialist_change<<','<<f.half_specialist_count<<','<<f.half_specialist_capital_count<<','<<f.military_production_count<<"],\"cities\":[";
        bool inner=false;for(const auto &sc:sp.cities.cities){const auto id=named(sc.core.fields,"m_iID").values.at(0);const auto &city=p.cities.at(id);const auto &cf=profile->cities.at({owner,id});const auto rates=runtime::vp_city_food_rates(context,city);
            if(inner)out<<',';inner=true;const auto &mc=*saves::vp_city_management_dimensions(sc.management.context);
            out<<"{\"id\":"<<id<<",\"population\":"<<city.population<<",\"source_core\":["<<sc.core.start_offset<<','<<sc.core.end_offset
               <<"],\"source_production\":["<<sc.production.start_offset<<','<<sc.production.end_offset<<','<<*saves::vp_city_production_building_slots(sc.production.context)
               <<"],\"source_management\":["<<sc.management.start_offset<<','<<sc.management.end_offset<<','<<mc.flavors<<','<<mc.strategies<<','<<mc.promotions<<','<<mc.buildings
               <<"],\"food\":["<<cf.stored_times100<<','<<cf.additional<<','<<cf.no_starvation_non_specialist<<','<<cf.minimum<<"],\"specialists\":";pairs(cf.specialists);
            out<<",\"orders\":[";bool order_comma=false;for(const auto &order:*city.production_orders){if(order_comma)out<<',';order_comma=true;out<<'['<<order.type<<','<<order.item<<','<<order.secondary<<','<<order.save<<','<<order.rush<<']';}
            out<<"],\"food_production\":"<<runtime::vp_city_food_production(context,city)<<",\"rates\":["<<rates.specialists<<','<<rates.non_specialists<<','<<rates.era_extra<<','<<rates.per_non_specialist_times100<<','<<rates.per_specialist_times100<<"],\"cases\":[";
            bool case_comma=false;for(const std::int64_t before:{-1000LL,0LL,99LL,100LL,199LL,200LL,400LL,10000LL,4294967300LL})for(int assumed=0;assumed<2;++assumed){
                const auto q=runtime::vp_city_food_consumption(context,city,before,assumed!=0);if(case_comma)out<<',';case_comma=true;
                out<<'['<<before<<','<<assumed<<','<<q.non_specialists_times100<<','<<q.specialists_times100<<','<<q.total_times100<<','<<q.protected_non_specialists<<','<<q.protected_size_one<<']';}
            out<<"]}";
        }out<<"]}";
    }
    out<<"],\"conversion\":[";comma=false;for(const std::int64_t food:{-1000LL,0LL,1LL,199LL,200LL,201LL,202LL,399LL,400LL,401LL,404LL,4294967300LL,9223372036854775807LL}){
        if(comma)out<<',';comma=true;out<<'['<<food<<','<<runtime::vp_food_to_production_times100(food)<<']';}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
