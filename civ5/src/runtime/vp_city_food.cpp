#include "civ5/runtime/vp_city_food.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("City food ID exceeds int64");return static_cast<std::int64_t>(n);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate city food field");found=&x;}
    if(!found)throw std::runtime_error("Missing city food field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &n=named(v,name).values;if(n.size()!=1)throw std::runtime_error("Invalid city food scalar");return n[0];}
std::int64_t define(const VpCityFoodContext &c,const char *name){const auto *v=c.defines.find(name);
    if(!v||!v->lookup.error.empty()||!std::holds_alternative<std::int64_t>(v->lookup.value))throw std::runtime_error("Unresolved city food define: "+std::string(name));
    return std::get<std::int64_t>(v->lookup.value);}
const CityYieldPlayer &validate(const VpCityFoodContext &c,const CityYieldState &city){
    if(c.cities.profile!=vp||c.players.profile!=vp||c.defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("City food requires VP world");
    const auto &p=c.cities.players.at(city.owner);
    if(&p.cities.at(city.id)!=&city)throw std::runtime_error("City food requires resolved live city/player");
    (void)vp_city_food_player(c.cities,city.owner);(void)vp_city_food_city(c.cities,city.owner,city.id);
    return p;
}
}
VpCityFoodRules load_vp_city_food_rules(const content::RulesDatabase &db){
    VpCityFoodRules r;
    const auto load=[&](const char *table,const auto &fn){const auto q=db.prefetch_info(table,fn);if(!q.all_succeeded())throw std::runtime_error("City food Info: "+q.error);return q.slots;};
    load("Eras",[&](auto id,auto,const auto &row){r.specialist_extra_by_era[id64(id)]=row.integer("SpecialistExtraFoodCost");return true;});
    load("Units",[&](auto id,auto,const auto &row){r.units[id64(id)]={row.integer("Food")!=0,row.integer("MilitaryProduction")!=0};return true;});
    r.specialist_slots=load("Specialists",[](auto,auto,const auto &){return true;});
    r.promotion_slots=load("UnitPromotions",[](auto,auto,const auto &){return true;});return r;
}
void restore_vp_city_food(const saves::SavedSessionSnapshot &s,const VpCityFoodRules &rules,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout||rules.specialist_slots>rules.promotion_slots)
        throw std::runtime_error("City food import requires VP specialist wire domain");
    auto &profile=ensure_vp_city_food_profile(world);std::set<std::int64_t> players;
    for(const auto &source:s.all_players->players){const auto owner=source.saved_player_id;auto &player=world.players.at(owner);
        if(!players.insert(owner).second||source.core.layout!=layout||source.traits.layout!=layout||source.cities.layout!=layout)
            throw std::runtime_error("City food player binding mismatch");
        VpCityFoodProfileState::Player p;
        p.non_specialist_change_times100=scalar(source.traits.fields,"m_iNonSpecialistFoodChange");
        p.specialist_change=scalar(source.core.fields,"m_iSpecialistFoodChange");
        p.half_specialist_count=scalar(source.core.fields,"m_iHalfSpecialistFoodCount");
        p.half_specialist_capital_count=scalar(source.core.fields,"m_iHalfSpecialistFoodCapitalCount");
        p.military_production_count=scalar(source.core.fields,"m_iMilitaryFoodProductionCount");profile.players[owner]=p;
        std::set<std::int64_t> cities;
        for(const auto &sc:source.cities.cities){const auto id=scalar(sc.core.fields,"m_iID");auto &city=player.cities.at(id);
            if(!cities.insert(id).second||sc.core.layout!=layout||sc.management.layout!=layout||sc.production.layout!=layout||
               city.id!=id||city.owner!=owner||scalar(sc.core.fields,"m_eOwner")!=owner||
               city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})
                throw std::runtime_error("City food city binding mismatch");
            VpCityFoodProfileState::City f;
            f.stored_times100=scalar(sc.core.fields,"m_iFood");f.additional=scalar(sc.core.fields,"m_iAdditionalFood");
            f.no_starvation_non_specialist=scalar(sc.core.fields,"m_iNoStarvationNonSpecialist");f.minimum=scalar(sc.core.fields,"m_iMinimumFood");
            const auto &counts=named(named(sc.management.blocks,"citizens").arrays,"m_aiSpecialistCounts");
            if(counts.columns!=1||counts.values.size()!=rules.promotion_slots)throw std::runtime_error("City specialist wire span mismatch");
            for(std::size_t i=0;i<rules.specialist_slots;++i)if(counts.values[i])f.specialists[id64(i)]=counts.values[i];
            profile.cities[{owner,id}]=std::move(f);
            std::vector<CityYieldState::ProductionOrder> orders;
            for(const auto &o:named(sc.production.blocks,"queue").children){
                const auto value=[&](const char *name){return named(o.fields,name).value;};
                orders.push_back({value("eOrderType"),value("iData1"),value("iData2"),value("bSave")!=0,value("bRush")!=0,std::nullopt});}
            city.production_orders=std::move(orders);
        }
        if(cities.size()!=player.cities.size())throw std::runtime_error("Incomplete city food cities");
    }
    if(players.size()!=world.players.size())throw std::runtime_error("Incomplete city food players");
}
std::int64_t vp_total_specialists(const VpCityFoodContext &c,const CityYieldState &city){
    validate(c,city);const auto default_specialist=define(c,"DEFAULT_SPECIALIST");std::int64_t total=0;
    const auto &food=vp_city_food_city(c.cities,city.owner,city.id);
    for(const auto &[id,n]:food.specialists){if(id<0||static_cast<std::uint64_t>(id)>=c.rules.specialist_slots)throw std::out_of_range("City food specialist index");
        if(id!=default_specialist)total=wide::add(total,n);}
    return total;
}
VpCityFoodRates vp_city_food_rates(const VpCityFoodContext &c,const CityYieldState &city){
    const auto &player=validate(c,city);const auto &p=vp_city_food_player(c.cities,city.owner);const auto &f=vp_city_food_city(c.cities,city.owner,city.id);VpCityFoodRates q;
    const auto base=define(c,"FOOD_CONSUMPTION_PER_POPULATION");q.specialists=vp_total_specialists(c,city);
    q.non_specialists=std::max<std::int64_t>(0,wide::subtract(city.population,q.specialists));
    q.per_non_specialist_times100=std::max<std::int64_t>(0,wide::add(wide::add(wide::multiply_divide(base,100,1),wide::multiply_divide(f.additional,100,1)),p.non_specialist_change_times100));
    const auto era=c.team_eras.at(c.players.players.at(city.owner).team);
    // Equivalent to the native descending integer scan, including absent Info
    // entries and negative defaults, without iterating through billions of gaps.
    auto it=c.rules.specialist_extra_by_era.upper_bound(era);
    while(it!=c.rules.specialist_extra_by_era.begin()){--it;if(it->first<0)break;if(it->second>=0){q.era_extra=it->second;break;}}
    q.per_specialist_times100=wide::add(wide::multiply_divide(wide::add(base,q.era_extra),100,1),wide::multiply_divide(p.specialist_change,100,1));
    if(p.half_specialist_count>0)q.per_specialist_times100/=2;
    if(p.half_specialist_capital_count>0&&city.id==player.capital)q.per_specialist_times100/=2;
    q.per_specialist_times100=std::max<std::int64_t>(100,q.per_specialist_times100);return q;
}
VpCityFoodConsumption vp_city_food_consumption(const VpCityFoodContext &c,const CityYieldState &city,std::int64_t food,bool assume_no_reduction){
    VpCityFoodConsumption q;q.rates=vp_city_food_rates(c,city);const auto &f=vp_city_food_city(c.cities,city.owner,city.id);
    q.non_specialists_times100=wide::multiply_divide(q.rates.per_non_specialist_times100,q.rates.non_specialists,1);
    if(f.no_starvation_non_specialist>0&&!assume_no_reduction){q.protected_non_specialists=true;q.non_specialists_times100=std::min(food,q.non_specialists_times100);}
    q.specialists_times100=wide::multiply_divide(q.rates.per_specialist_times100,q.rates.specialists,1);
    q.total_times100=std::max<std::int64_t>(100,wide::add(q.non_specialists_times100,q.specialists_times100));
    if(city.population==1&&f.stored_times100==0){q.protected_size_one=true;q.total_times100=std::min(q.total_times100,food);}return q;
}
bool vp_unit_food_production(const VpCityFoodContext &c,std::int64_t owner,std::int64_t unit){
    if(c.cities.profile!=vp||c.players.profile!=vp)throw std::runtime_error("Food production requires VP world");
    const auto u=c.rules.units.find(unit);if(u==c.rules.units.end())return false;if(u->second.food)return true;
    return vp_city_food_player(c.cities,owner).military_production_count>0&&u->second.military;
}
bool vp_city_food_production(const VpCityFoodContext &c,const CityYieldState &city){
    validate(c,city);if(!city.production_orders)throw std::runtime_error("Unresolved city production queue");
    const auto &orders=*city.production_orders;return !orders.empty()&&orders.front().type==0&&vp_unit_food_production(c,city.owner,orders.front().item);
}
std::int64_t vp_food_to_production_times100(std::int64_t food){
    if(food<=0)return 0;if(food<=200)return food;
    if(food<=400)return wide::add(200,wide::multiply_divide(wide::subtract(food,200),50,100));
    return wide::add(300,wide::multiply_divide(wide::subtract(food,400),25,100));
}
} // namespace civ5::runtime
