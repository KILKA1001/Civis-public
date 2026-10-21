#include "civ5/runtime/vp_city_happiness.hpp"
#include "civ5/saves/vp_game_trade_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("City happiness ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate city happiness field");found=&x;}
    if(!found)throw std::runtime_error("Missing city happiness field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid city happiness scalar");return a[0];}
void validate(const VpCityHappinessContext &c,const CityYieldState &city){const auto &w=c.great_works.religion.water;const auto &cities=c.great_works.cities;
    if(cities.profile!=vp||c.players.profile!=vp||c.units.profile!=vp||w.defines.profile()!=content::GlobalDefineProfile::vp542||!w.options||!w.options->all_succeeded()||
       &w.cities!=&cities||&w.players!=&c.players||&c.food.cities!=&cities||&c.food.players!=&c.players||&c.food.defines!=&w.defines||
       &cities.players.at(city.owner).cities.at(city.id)!=&city)
        throw CityYieldUnavailable("City happiness requires resolved shared VP state");
    (void)vp_city_happiness_city(cities,city.owner,city.id);(void)vp_city_happiness_player(cities,city.owner);
}
bool option(const VpCityHappinessContext &c,const char *name){return c.great_works.religion.water.options->enabled(name);}
std::int64_t define(const VpCityHappinessContext &c,const char *name){const auto *d=c.great_works.religion.water.defines.find(name);
    if(!d||!d->lookup.error.empty())throw CityYieldUnavailable("Unresolved city happiness define: "+std::string(name));return std::get<std::int64_t>(d->lookup.value);}
std::int64_t policies(const VpCityHappinessContext &c,std::int64_t owner){const auto &p=c.players.players.at(owner);if(!p.policies_resolved)throw CityYieldUnavailable("Unresolved shared policy ownership");
    std::int64_t n=0;for(auto id:p.policies)if(!c.rules.dummy_policies.at(id))n=add(n,1);return n;}
VpCityHappinessRules::Handicap handicap(const VpCityHappinessContext &c,std::int64_t id){const auto i=c.rules.handicaps.find(id);return i==c.rules.handicaps.end()?VpCityHappinessRules::Handicap{}:i->second;}
}
VpCityHappinessRules load_vp_city_happiness_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpCityHappinessRules r;const auto y=db.prefetch_info("Yields",[&](auto id,auto,const auto &){r.yields.insert(id64(id));return true;});
    const auto h=db.prefetch_info("HandicapInfos",[&](auto id,auto,const auto &row){r.handicaps[id64(id)]={row.integer("HappinessDefaultCapital"),row.integer("AIHappinessDefaultCapital")};return true;});
    const auto p=db.prefetch_info("Policies",[&](auto id,auto,const auto &row){r.dummy_policies[id64(id)]=row.integer("IsDummy")!=0;return true;});
    if(!y.all_succeeded()||!h.all_succeeded()||!p.all_succeeded())throw CityYieldUnavailable("Unresolved city happiness Info");r.yield_slots=y.slots;
    const auto *culture=types.find("YIELD_CULTURE"),*crime=types.find("YIELD_JFD_CRIME");if(!culture)throw CityYieldUnavailable("Missing culture yield identity");r.culture=culture->id;r.crime=crime?crime->id:-1;return r;
}
void restore_vp_city_happiness(const saves::SavedSessionSnapshot &s,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout||s.game_state.layout!=layout)throw CityYieldUnavailable("City happiness import requires VP state");
    const auto &prefix=std::get<saves::VpPrefixObservation>(s.game_state.prefix);
    for(std::size_t i=0;i<prefix.scalar_names.size();++i)if(std::string_view(prefix.scalar_names[i])=="turn_slice")world.turn_slice=prefix.scalars[i];
    const auto vector=[&](const auto &fields,const char *name){const auto &a=named(fields,name).values;if(a.size()!=world.yield_slots)throw std::runtime_error("City happiness yield span mismatch");YieldValues v;for(std::size_t i=0;i<a.size();++i)if(a[i])v[id64(i)]=a[i];return v;};
    std::set<std::int64_t> players;auto &extension=ensure_vp_city_happiness_profile(world);
    for(const auto &sp:s.all_players->players){const auto id=sp.saved_player_id;auto &p=world.players.at(id);
        if(!players.insert(id).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.strategy.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=id)
            throw std::runtime_error("City happiness player binding mismatch");
        auto &a=extension.players[id];a.no_annexing=scalar(sp.traits.fields,"m_bNoAnnexing")!=0;
        a.garrison=scalar(sp.core.fields,"m_iHappinessPerGarrisonedUnitCount");a.strength=scalar(sp.core.fields,"m_iHappinessPerCityOverStrengthThreshold");a.per_x_works=scalar(sp.core.fields,"m_iHappinessPerXGreatWorks");
        a.specialists=scalar(sp.core.fields,"m_iHappfromXSpecialists");a.per_city=scalar(sp.core.fields,"m_iHappinessPerCity");a.per_x_population=scalar(sp.core.fields,"m_iHappinessPerXPopulation");
        a.per_route=scalar(sp.core.fields,"m_iHappinessPerActiveTradeRoute");a.to_culture=scalar(sp.core.fields,"m_iHappinessToCulture");
        const auto fraction=[&](const char *name){const auto &v=named(sp.core.fields,name).values;if(v.size()!=2)throw std::runtime_error("Invalid happiness fraction");return YieldFraction(v[0],v[1]);};
        a.per_population=fraction("m_fHappinessPerXPopulationGlobal");a.per_policy_from_policies=fraction("m_fExtraHappinessPoliciesFromPolicies");a.per_policy=fraction("m_fHappinessPolicies");
        const auto &modifiers=named(named(sp.strategy.blocks,"policies").arrays,"policy_modifiers").values;
        if(modifiers.size()!=60)throw std::runtime_error("Invalid VP policy modifier wire layout");a.policy_capital=modifiers[0];a.policy_city=modifiers[1];extension.trade_stats[id]={};
        std::set<std::int64_t> cities;for(const auto &sc:sp.cities.cities){const auto cid=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(cid);
            if(!cities.insert(cid).second||sc.core.layout!=layout||city.owner!=id||city.id!=cid||scalar(sc.core.fields,"m_eOwner")!=id||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})throw std::runtime_error("City happiness city binding mismatch");
            auto &b=extension.cities[{id,cid}];b.occupied=scalar(sc.core.fields,"m_bOccupied")!=0;b.no_occupied_unhappiness=scalar(sc.core.fields,"m_iNoOccupiedUnhappinessCount");
            b.empire=scalar(sc.core.fields,"m_iHappinessFromEmpire");b.buildings=scalar(sc.core.fields,"m_iBaseHappinessFromBuildings");b.building_classes=scalar(sc.core.fields,"m_iBuildingClassHappiness");
            b.religion=scalar(sc.core.fields,"m_iReligionHappiness");b.unmodded_buildings=scalar(sc.core.fields,"m_iUnmoddedHappinessFromBuildings");
            b.policies=vector(sc.core.fields,"m_aiBaseYieldRateFromPolicies");b.crime=vector(sc.core.fields,"m_aiYieldFromCrime");b.development=vector(sc.core.fields,"m_aiYieldFromDevelopment");
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete city happiness cities");
    }if(players.size()!=world.players.size())throw std::runtime_error("Incomplete city happiness players");
}
void vp_reset_trade_stats(CityYieldContext &world,std::int64_t owner){auto &s=vp_city_happiness_trade_stats(world,owner);s={};s.turn_slice=0;}
void vp_update_trade_stats(CityYieldContext &world,std::int64_t owner){
    if(world.profile!=vp||!world.turn_slice)throw CityYieldUnavailable("Trade statistics require a shared VP world and turn slice");auto &s=vp_city_happiness_trade_stats(world,owner);
    if(s.turn_slice==world.turn_slice)return;vp_reset_trade_stats(world,owner);
    auto &stats=vp_city_happiness_trade_stats(world,owner);
    for(const auto &r:world.trade_routes){const auto *vp_route=saves::find_vp_trade_route(r);
        if(!vp_route)throw CityYieldUnavailable("VP route has no city ID fields");
        if(vp_route->origin_id==-1||vp_route->destination_id==-1||r.origin_owner==-1||r.destination_owner==-1)continue;
        const bool from=r.origin_owner==owner,to=r.destination_owner==owner;
        const auto &destination=world.players.at(r.destination_owner);
        if(from&&destination.minor)stats.minors=add(stats.minors,1);if(from&&to)stats.internal=add(stats.internal,1);
        if(from&&!to)stats.international_out=add(stats.international_out,1);if(!from&&to)stats.international_in=add(stats.international_in,1);
        if(from)stats.from_city[vp_route->origin_id]=add(value(stats.from_city,vp_route->origin_id),1);if(to)stats.to_city[vp_route->destination_id]=add(value(stats.to_city,vp_route->destination_id),1);
    }stats.turn_slice=world.turn_slice;
}
std::int64_t vp_trade_routes_from_city(CityYieldContext &world,std::int64_t owner,const CityYieldState *city){
    if(!city)return 0;if(&world.players.at(city->owner).cities.at(city->id)!=city)throw CityYieldUnavailable("Trade statistics require a live city");vp_update_trade_stats(world,owner);
    return vp_city_happiness_trade_stats(world,owner).from_city[city->id];
}
VpCityPolicyHappinessQuote vp_city_policy_happiness(const VpCityHappinessContext &c,CityYieldState &city,std::int64_t population_change){
    validate(c,city);auto &world=c.great_works.cities;const auto &owner=world.players.at(city.owner);const auto &p=vp_city_happiness_player(world,city.owner);VpCityPolicyHappinessQuote q;
    if(p.garrison>0&&vp_city_garrison(world,c.units,city))q.garrison=p.garrison;
    if(p.strength>0){const auto &plot_yields=vp_city_plot_profile_city(c.great_works.cities,city.owner,city.id).yields;if(!plot_yields)throw CityYieldUnavailable("Unresolved city strength");if(plot_yields->strength>=mul(define(c,"CITY_STRENGTH_THRESHOLD_FOR_BONUSES"),100))q.strength=p.strength;}
    if(p.per_x_works)q.great_works=vp_city_great_works(c.great_works,city)/p.per_x_works;
    const auto pop=add(city.population,population_change);const bool capital=owner.capital==city.id;
    if((option(c,"BALANCE_VP")||capital)&&p.per_population.numerator()>0&&pop>0)q.population_global=p.per_population.scaled_truncate(pop);
    if(capital){q.capital=p.policy_capital;if(p.per_policy_from_policies.numerator()>0)q.policies_from_policies=p.per_policy_from_policies.scaled_truncate(policies(c,city.owner));
        if(p.per_policy.numerator()>0)q.policies=p.per_policy.scaled_truncate(policies(c,city.owner));}
    if(p.specialists>0){const auto n=vp_total_specialists(c.food,city);if(n>0){const auto rate=define(c,"UNHAPPINESS_PER_SPECIALIST");if(rate>0)q.specialists=wide::multiply_divide(std::min(n,p.specialists),rate,100);}}
    q.per_city=add(p.policy_city,p.per_city);if(p.per_x_population>0&&!city.puppet)q.population=pop/p.per_x_population;
    if(p.per_route>0)q.trade=mul(vp_trade_routes_from_city(world,city.owner,&city),p.per_route);
    for(auto n:{q.garrison,q.strength,q.great_works,q.population_global,q.capital,q.policies_from_policies,q.policies,q.specialists,q.per_city,q.population,q.trade})q.total=add(q.total,n);return q;
}
VpCityHappinessQuote vp_city_local_happiness(const VpCityHappinessContext &c,CityYieldState &city,std::int64_t population_change,bool exclude_empire){
    validate(c,city);const auto &owner=c.great_works.cities.players.at(city.owner);const auto &a=vp_city_happiness_city(c.great_works.cities,city.owner,city.id);const auto &player_happiness=vp_city_happiness_player(c.great_works.cities,city.owner);VpCityHappinessQuote q;
    if(option(c,"BALANCE_VP")&&((city.puppet&&!player_happiness.no_annexing)||city.razing||city.resistance||(a.occupied&&!city.puppet&&a.no_occupied_unhappiness<=0))){q.suppressed=true;return q;}
    if(!exclude_empire)q.empire=a.empire;q.buildings=a.buildings;q.building_classes=a.building_classes;q.policies=vp_city_policy_happiness(c,city,population_change);q.religion=a.religion;q.unmodded_buildings=a.unmodded_buildings;
    if(owner.capital==city.id){const auto &p=c.players.players.at(city.owner);const bool human=c.players.is_human(p),ai_handicap=option(c,"HUMAN_USES_AI_HANDICAP");
        const auto h=handicap(c,human&&ai_handicap?define(c,"AI_HANDICAP"):p.handicap);q.handicap=h.capital;
        if(option(c,"BALANCE_VP")&&c.players.game_option_value(c.database,"GAMEOPTION_ONE_CITY_CHALLENGE")==1&&human&&!option(c,"HUMAN_USES_AI_MECHANICS"))q.handicap=mul(q.handicap,2);
        else if(!human||ai_handicap)q.handicap=add(q.handicap,handicap(c,c.players.game_handicap).ai_capital);
    }
    if(option(c,"BALANCE_CORE_JFD")){if(c.rules.crime<0)throw CityYieldUnavailable("Unresolved JFD crime identity");q.crime=value(a.crime,c.rules.crime);q.development=value(a.development,c.rules.crime);}
    for(auto n:{q.empire,q.buildings,q.building_classes,q.policies.total,q.religion,q.unmodded_buildings,q.handicap,q.crime,q.development})q.total=add(q.total,n);return q;
}
VpCityPolicyYieldQuote vp_city_policy_yield(const VpCityHappinessContext &c,CityYieldState &city,std::int64_t y){
    validate(c,city);if(!c.rules.yields.contains(y))throw std::out_of_range("Invalid city policy yield");VpCityPolicyYieldQuote q;q.base=mul(value(vp_city_happiness_city(c.great_works.cities,city.owner,city.id).policies,y),100);
    const auto rate=vp_city_happiness_player(c.great_works.cities,city.owner).to_culture;
    if(option(c,"BALANCE_VP")&&y==c.rules.culture&&rate)q.happiness=std::max<std::int64_t>(0,mul(vp_city_local_happiness(c,city).total,rate));q.total_times100=add(q.base,q.happiness);return q;
}
} // namespace civ5::runtime
