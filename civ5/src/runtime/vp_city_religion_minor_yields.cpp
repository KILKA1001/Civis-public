#include "civ5/runtime/vp_city_religion_minor_yields.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("City religion/minor yield ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate city religion/minor yield field");found=&x;}
    if(!found)throw std::runtime_error("Missing city religion/minor yield field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid city religion/minor yield scalar");return a[0];}
std::int64_t row_sum(const YieldModifierRows &rows,std::int64_t y){std::int64_t n=0;
    if(const auto i=rows.find(y);i!=rows.end())for(const auto &[id,v]:i->second)n=add(n,v);return n;}
void validate(const VpCityReligionMinorContext &c,std::int64_t player){const auto &w=c.religion.water;const auto &p=w.cities.players.at(player);const auto &t=c.treasury.players.at(player);
    if(w.cities.profile!=vp||w.players.profile!=vp||w.rules.profile!=vp||w.state.layout!=layout||c.treasury.profile!=vp||!w.options||!w.options->all_succeeded()||
       w.defines.profile()!=content::GlobalDefineProfile::vp542||!vp_player_state_religion(p)||vp_treasury_religion_profile(t).state_religion!=*vp_player_state_religion(p))throw CityYieldUnavailable("Unresolved shared city religion/minor world");
}
void validate(const VpCityReligionMinorContext &c,const CityYieldState &city,std::int64_t yield){validate(c,city.owner);const auto &world=c.religion.water.cities;
    if(&world.players.at(city.owner).cities.at(city.id)!=&city||!vp_city_majority_religion(city))
        throw CityYieldUnavailable("City religion/minor income requires live shared state");
    (void)vp_city_religion_minor_city(world,city.owner,city.id);(void)vp_city_religion_minor_player(world,city.owner);
    if(!c.religion.rules.yields.contains(yield))throw std::out_of_range("Invalid city religion/minor yield");
}
}
void restore_vp_city_religion_minor_yields(const saves::SavedSessionSnapshot &s,CityYieldContext &world,const TreasuryContext &treasury){
    if(world.profile!=vp||treasury.profile!=vp||!s.all_players||s.all_players->layout!=layout)throw std::runtime_error("City religion/minor yield import requires VP world");
    const auto vector=[&](const auto &fields,const char *name){const auto &v=named(fields,name).values;YieldValues result;
        if(v.size()!=world.yield_slots)throw std::runtime_error("City religion/minor yield wire span mismatch");for(std::size_t i=0;i<v.size();++i)if(v[i])result[id64(i)]=v[i];return result;};
    std::set<std::int64_t> players;auto &extension=ensure_vp_city_religion_minor_yield_profile(world);
    for(const auto &sp:s.all_players->players){const auto id=sp.saved_player_id;auto &p=world.players.at(id);
        if(!players.insert(id).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=id||
           treasury.players.at(id).source_core!=std::optional{std::pair{sp.core.start_offset,sp.core.end_offset}})throw std::runtime_error("City religion/minor player binding mismatch");
        auto &pg=extension.players[id];pg.allies=scalar(sp.core.fields,"m_iCSAllies");pg.friends=scalar(sp.core.fields,"m_iCSFriends");pg.own_pantheon=vector(sp.traits.fields,"m_iYieldFromOwnPantheon");
        std::set<std::int64_t> cities;
        for(const auto &sc:sp.cities.cities){const auto cid=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(cid);
            if(!cities.insert(cid).second||sc.core.layout!=layout||sc.tail.layout!=layout||city.owner!=id||city.id!=cid||scalar(sc.core.fields,"m_eOwner")!=id||
               city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})throw std::runtime_error("City religion/minor city binding mismatch");
            auto &a=extension.cities[{id,cid}];
            a.special_religion=vector(sc.core.fields,"m_aiSpecialReligionYieldsTimes100");a.alliance=vector(sc.core.fields,"m_aiBaseYieldRateFromCSAllianceTimes100");a.friendship=vector(sc.core.fields,"m_aiBaseYieldRateFromCSFriendshipTimes100");
            a.per_ally=vector(sc.core.fields,"m_aiYieldPerAllyTimes100");a.per_friend=vector(sc.core.fields,"m_aiYieldPerFriendTimes100");a.from_minors=vector(sc.core.fields,"m_aiYieldFromMinors");
            const auto base=vector(sc.core.fields,"m_aiBaseYieldRateFromReligion");for(std::uint64_t y=0;y<world.yield_slots;++y)city.yields[id64(y)].religion=value(base,id64(y));
            const auto &rows=named(named(sc.tail.blocks,"remainder").children,"m_yieldChanges").children;if(rows.size()!=world.yield_slots)throw std::runtime_error("City religion yield row span mismatch");
            for(std::size_t y=0;y<rows.size();++y){const char *fields[]={"forTerrainFromReligion","forFeatureFromReligion"};YieldModifierRows *outputs[]={&a.terrain_religion,&a.feature_religion};
                for(std::size_t k=0;k<2;++k){const auto &v=named(rows[y].children,fields[k]).values;if(v.size()%2)throw std::runtime_error("Invalid city religion yield pairs");
                    auto &dest=(*outputs[k])[id64(y)];for(std::size_t i=0;i<v.size();i+=2)dest.emplace_back(v[i],v[i+1]);}}
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete city religion/minor cities");
    }if(players.size()!=world.players.size()||players.size()!=treasury.players.size())throw std::runtime_error("Incomplete city religion/minor players");
}
std::int64_t vp_owned_religion(const VpCityReligionMinorContext &c,std::int64_t player){validate(c,player);const auto &w=c.religion.water;
    if(!w.options->enabled("BALANCE_VP"))return vp_created_religion(w,player);
    const auto state=*vp_player_state_religion(w.cities.players.at(player));return vp_treasury_religion_profile(c.treasury.players.at(player)).owns_state_religion&&state!=w.cities.pantheon?state:-1;
}
VpCityReligionYieldQuote vp_city_religion_yield(const VpCityReligionMinorContext &c,const CityYieldState &city,std::int64_t yield){
    validate(c,city,yield);const auto &w=c.religion.water;const auto &a=vp_city_religion_minor_city(w.cities,city.owner,city.id);const auto &p=vp_city_religion_minor_player(w.cities,city.owner);VpCityReligionYieldQuote q;
    const auto base=city.yields.find(yield);q.base=base==city.yields.end()?0:mul(base->second.religion,100);q.terrain=row_sum(a.terrain_religion,yield);q.features=row_sum(a.feature_religion,yield);
    const auto bonus=value(p.own_pantheon,yield);if(bonus>0){auto founded=vp_owned_religion(c,city.owner);if(founded==-1)founded=vp_created_religion(w,city.owner,true);
        if(founded>=w.cities.pantheon){const auto majority=*vp_city_majority_religion(city),pantheon=vp_created_pantheon(w.cities,city.owner);
            if(majority!=-1&&(majority==founded||majority==pantheon))q.own_pantheon=mul(bonus,100);}}
    q.special=value(a.special_religion,yield);for(auto n:{q.base,q.terrain,q.features,q.own_pantheon,q.special})q.total_times100=add(q.total_times100,n);return q;
}
VpCityMinorYieldQuote vp_city_minor_yield(const VpCityReligionMinorContext &c,const CityYieldState &city,std::int64_t yield){
    validate(c,city,yield);const auto &w=c.religion.water;const auto &owner=w.cities.players.at(city.owner);const auto &p=vp_city_religion_minor_player(w.cities,city.owner);const auto &a=vp_city_religion_minor_city(w.cities,city.owner,city.id);const auto &t=c.treasury.players.at(city.owner);VpCityMinorYieldQuote q;
    const auto &minor=vp_treasury_minor_profile(t);q.modifier=add(minor.trait_cs_modifier,minor.cs_modifier);
    if(w.options->enabled("BALANCE_CITY_STATE_SCALE")){const auto *d=w.defines.find("CITY_STATE_SCALE_PER_CITY_MOD");if(!d||!d->lookup.error.empty())throw CityYieldUnavailable("Unresolved city-state scaling define");
        q.city_scaling=mul(std::max<std::int64_t>(0,id64(owner.cities.size())-1),std::get<std::int64_t>(d->lookup.value));}
    q.modifier=add(q.modifier,q.city_scaling);const auto percent=add(100,q.modifier);
    q.alliance_base=wide::multiply_divide(value(a.alliance,yield),percent,100);q.friendship_base=wide::multiply_divide(value(a.friendship,yield),percent,100);
    q.per_ally=mul(value(a.per_ally,yield),p.allies);q.per_friend=mul(value(a.per_friend,yield),p.friends);q.from_minors=mul(value(a.from_minors,yield),100);
    q.total_times100=add(add(add(q.alliance_base,q.per_ally),add(q.friendship_base,q.per_friend)),q.from_minors);return q;
}
} // namespace civ5::runtime
