#include "civ5/runtime/vp_city_trait_yields.hpp"
#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("City trait yield ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate city trait yield field");found=&x;}
    if(!found)throw std::runtime_error("Missing city trait yield field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid city trait yield scalar");return a[0];}
void validate(const VpCityTraitYieldContext &c,const CityYieldState &city){const auto &p=c.cities.players.at(city.owner);
    if(c.cities.profile!=vp||c.players.profile!=vp||c.treasury.profile!=vp||c.defines.profile()!=content::GlobalDefineProfile::vp542||!c.options.all_succeeded()||
       &p.cities.at(city.id)!=&city)throw CityYieldUnavailable("City traits require a bound VP city");
    (void)vp_city_trait_yield_city(c.cities,city.owner,city.id);(void)vp_city_trait_yield_player(c.cities,city.owner);(void)vp_city_trait_era(c.cities,city.owner);
}
void yield(const VpCityTraitYieldContext &c,std::int64_t y){if(!c.rules.yields.contains(y))throw std::out_of_range("Invalid city trait yield");}
std::int64_t era(const VpCityTraitYieldContext &c,std::int64_t owner){return c.team_eras.at(c.players.players.at(owner).team);}
std::int64_t define(const VpCityTraitYieldContext &c,const char *name){const auto *d=c.defines.find(name);if(!d||!d->lookup.error.empty())throw CityYieldUnavailable("Unresolved connection define: "+std::string(name));return std::get<std::int64_t>(d->lookup.value);}
}
VpCityTraitYieldRules load_vp_city_trait_yield_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpCityTraitYieldRules r;const auto y=db.prefetch_info("Yields",[&](auto id,auto,const auto &){r.yields.insert(id64(id));return true;});
    const auto i=db.prefetch_info("Improvements",[](auto,auto,const auto &){return true;});
    if(!y.all_succeeded()||!i.all_succeeded())throw std::runtime_error("City trait yield Info unavailable");r.yield_slots=y.slots;r.improvement_slots=i.slots;
    const auto *culture=types.find("YIELD_CULTURE"),*tourism=types.find("YIELD_TOURISM");if(!culture||!tourism)throw std::runtime_error("Missing city trait yield identities");r.culture=culture->id;r.tourism=tourism->id;return r;
}
void rebuild_vp_trait_improvement_types(CityYieldContext &world,std::int64_t player){auto &a=vp_city_trait_yield_player(world,player);a.improvement_types.clear();
    for(const auto &[id,v]:a.per_improvement)if(std::any_of(v.begin(),v.end(),[](const auto &x){return x.second!=0;}))a.improvement_types.push_back(id);
}
void restore_vp_city_trait_yields(const saves::SavedSessionSnapshot &s,const VpCityTraitYieldRules &r,CityYieldContext &world,const TreasuryContext &treasury){
    if(world.profile!=vp||treasury.profile!=vp||!s.all_players||s.all_players->layout!=layout||world.yield_slots!=r.yield_slots||!r.yield_slots)throw std::runtime_error("City trait yield import requires VP world");
    const auto sparse=[&](const auto &v,std::uint64_t slots){YieldValues a;if(v.size()!=slots)throw std::runtime_error("City trait yield wire span mismatch");for(std::size_t i=0;i<v.size();++i)if(v[i])a[id64(i)]=v[i];return a;};
    const auto vector=[&](const auto &fields,const char *name){return sparse(named(fields,name).values,r.yield_slots);};std::set<std::int64_t> players;auto &extension=ensure_vp_city_trait_yield_profile(world);
    for(const auto &sp:s.all_players->players){const auto id=sp.saved_player_id;auto &p=world.players.at(id);
        if(!players.insert(id).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.tail.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=id||
           treasury.players.at(id).source_core!=std::optional{std::pair{sp.core.start_offset,sp.core.end_offset}})throw std::runtime_error("City trait yield player binding mismatch");
        extension.eras[id]={scalar(sp.traits.fields,"m_iFractionalEraScaler"),scalar(sp.traits.fields,"m_bIsEraScaling")!=0,scalar(sp.traits.fields,"m_bIsOddEraScaler")!=0};
        auto &a=extension.players[id];a.capital_only=scalar(sp.traits.fields,"m_bIsCapitalOnly")!=0;a.historic_events=scalar(sp.core.fields,"m_iNumHistoricEvent");
        a.city_culture=scalar(sp.traits.fields,"m_iCityCultureBonus");a.golden_age_tourism=scalar(sp.traits.fields,"m_iTourismGABonus");
        a.improvements_built=sparse(named(sp.core.fields,"m_paiTotalImprovementsBuilt").values,r.improvement_slots);
        a.historic=vector(sp.traits.fields,"m_iYieldFromHistoricEvent");a.per_partner=vector(sp.traits.fields,"m_iYieldChangePerTradePartner");a.land_connections=vector(sp.core.fields,"m_viYieldFromLandCityConnectionsTimes100");
        a.connection=vector(sp.tail.fields,"m_piYieldChangeTradeRoute");a.trait_connection=vector(sp.traits.fields,"m_iYieldChangeTradeRoute");
        const auto &m=named(sp.traits.fields,"m_ppaaiYieldChangePerImprovementBuilt");if(m.columns!=r.yield_slots||m.values.size()%r.yield_slots||m.values.size()/r.yield_slots!=r.improvement_slots)throw std::runtime_error("City trait improvement matrix span mismatch");
        for(std::size_t i=0;i<m.values.size();++i)if(m.values[i])a.per_improvement[id64(i/r.yield_slots)][id64(i%r.yield_slots)]=m.values[i];rebuild_vp_trait_improvement_types(world,id);
        std::set<std::int64_t> cities;
        for(const auto &sc:sp.cities.cities){const auto cid=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(cid);
            if(!cities.insert(cid).second||sc.core.layout!=layout||city.owner!=id||city.id!=cid||scalar(sc.core.fields,"m_eOwner")!=id||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})throw std::runtime_error("City trait yield city binding mismatch");
            auto &a=extension.cities[{id,cid}];a.connected=scalar(sc.core.fields,"m_bRouteToCapitalConnectedThisTurn")!=0;a.industrial=scalar(sc.core.fields,"m_bIndustrialRouteToCapital")!=0;
            a.connection_modifier=scalar(sc.core.fields,"m_iCityConnectionTradeRouteGoldModifier");a.added=vector(sc.core.fields,"m_iaAddedYieldPerTurnFromTraits");a.unimproved=vector(sc.core.fields,"m_paiYieldFromUnimprovedFeatures");a.permanent_wltkd=vector(sc.core.fields,"m_aiBaseYieldRatePermanentWLTKDTimes100");
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete city trait yield cities");
    }if(players.size()!=world.players.size()||players.size()!=treasury.players.size())throw std::runtime_error("Incomplete city trait yield players");
}
std::int64_t vp_different_trade_partners(const CityYieldContext &world,std::int64_t player){
    if(world.profile!=vp||!world.players.contains(player)||player<0)throw CityYieldUnavailable("Trade partner query requires a VP player");std::set<std::int64_t> others;
    for(const auto &r:world.trade_routes){if((r.origin_x==-1&&r.origin_y==-1&&r.destination_x==-1&&r.destination_y==-1)||r.origin_owner==r.destination_owner)continue;
        if(r.origin_owner!=player&&r.destination_owner!=player)continue;const auto other=r.origin_owner==player?r.destination_owner:r.origin_owner;
        if(other<0)throw CityYieldUnavailable("Active trade route has invalid player");others.insert(other);}
    return id64(others.size());
}
VpCityTraitPerTurnQuote vp_city_trait_per_turn_yield(const VpCityTraitYieldContext &c,const CityYieldState &city,std::int64_t y){
    validate(c,city);yield(c,y);const auto &owner=c.cities.players.at(city.owner);const auto &p=vp_city_trait_yield_player(c.cities,city.owner);const bool capital=owner.capital==city.id;VpCityTraitPerTurnQuote q;
    if(capital||!p.capital_only)for(auto improvement:p.improvement_types){if(improvement==-1)continue;const auto row=p.per_improvement.find(improvement);const auto bonus=row==p.per_improvement.end()?0:value(row->second,y);if(!bonus)continue;
        const auto scale=vp_trait_era_modifier(vp_city_trait_era(c.cities,city.owner),era(c,city.owner));q.improvements=add(q.improvements,YieldFraction(bonus,100).scaled_truncate(value(p.improvements_built,improvement),scale));}
    if(capital){q.historic=mul(value(p.historic,y),p.historic_events);const auto bonus=value(p.per_partner,y);
        if(c.options.enabled("BALANCE_VP")&&bonus>0)q.partners=mul(mul(std::max<std::int64_t>(1,era(c,city.owner)),bonus),vp_different_trade_partners(c.cities,city.owner));}
    q.added=value(vp_city_trait_yield_city(c.cities,city.owner,city.id).added,y);for(auto n:{q.improvements,q.historic,q.partners,q.added})q.total=add(q.total,n);return q;
}
VpCityTraitYieldQuote vp_city_trait_yield(const VpCityTraitYieldContext &c,const CityYieldState &city,std::int64_t y){
    VpCityTraitYieldQuote q;q.per_turn=vp_city_trait_per_turn_yield(c,city,y);const auto &owner=c.cities.players.at(city.owner);const auto &p=vp_city_trait_yield_player(c.cities,city.owner);const auto &a=vp_city_trait_yield_city(c.cities,city.owner,city.id);
    q.traits=mul(q.per_turn.total,100);q.unimproved=mul(value(a.unimproved,y),100);q.permanent_wltkd=value(a.permanent_wltkd,y);if(y==c.rules.culture)q.culture=mul(p.city_culture,100);
    if(owner.capital==city.id){q.land_connections=value(p.land_connections,y);if(y==c.rules.tourism&&owner.golden_age&&p.golden_age_tourism)q.golden_age_tourism=wide::multiply_divide(vp_city_cached_yield(c.cities,city.owner,city.id,c.rules.culture),p.golden_age_tourism,100);}
    for(auto n:{q.traits,q.unimproved,q.permanent_wltkd,q.culture,q.land_connections,q.golden_age_tourism})q.total_times100=add(q.total_times100,n);return q;
}
std::int64_t vp_city_connection_gold_times100(const VpCityTraitYieldContext &c,const CityYieldState *city){
    if(!city)return 0;validate(c,*city);const auto &p=c.cities.players.at(city->owner);const auto capital=p.cities.find(p.capital);if(capital==p.cities.end()||&capital->second==city)return 0;
    const auto &t=c.treasury.players.at(city->owner);auto gold=define(c,"TRADE_ROUTE_BASE_GOLD");gold=add(gold,mul(capital->second.population,define(c,"TRADE_ROUTE_CAPITAL_POP_GOLD_MULTIPLIER")));
    gold=add(gold,mul(city->population,define(c,"TRADE_ROUTE_CITY_POP_GOLD_MULTIPLIER")));gold=add(gold,mul(t.connection_change,100));if(gold<=0)return 0;
    const auto modifier=add(t.connection_modifier,vp_city_trait_yield_city(c.cities,city->owner,city->id).connection_modifier);return modifier?wide::multiply_divide(gold,add(100,modifier),100):gold;
}
VpCityConnectionYieldQuote vp_city_connection_yield(const VpCityTraitYieldContext &c,const CityYieldState &city,std::int64_t y){
    validate(c,city);yield(c,y);const auto &p=vp_city_trait_yield_player(c.cities,city.owner);const auto &a=vp_city_trait_yield_city(c.cities,city.owner,city.id);VpCityConnectionYieldQuote q;
    if(a.connected)q.route=add(mul(value(p.connection,y),100),mul(mul(value(p.trait_connection,y),std::max<std::int64_t>(1,era(c,city.owner))),100));
    if(c.options.enabled("BALANCE_VP")&&a.industrial&&y==c.cities.production)q.industrial=vp_city_connection_gold_times100(c,&city);
    q.total_times100=add(q.route,q.industrial);return q;
}
} // namespace civ5::runtime
