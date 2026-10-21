#include "civ5/runtime/vp_city_growth.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/runtime/vp_unit_supply.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <cmath>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Growth ID exceeds int64");return static_cast<std::int64_t>(n);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate growth field");found=&x;}
    if(!found)throw std::runtime_error("Missing growth field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid growth scalar");return a[0];}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto it=v.find(id);return it==v.end()?0:it->second;}
const auto &definition(const content::GlobalDefineCache &d,const char *name){const auto *v=d.find(name);
    if(!v||!v->lookup.error.empty())throw std::runtime_error("Unresolved growth define: "+std::string(name));return v->lookup.value;}
std::int64_t integer(const content::GlobalDefineCache &d,const char *name){return std::get<std::int64_t>(definition(d,name));}
std::int64_t global_penalty(const content::GlobalDefineCache &d,std::int64_t happiness){
    const float rate=static_cast<float>(std::get<double>(definition(d,"GLOBAL_GROWTH_PENALTY_PER_UNHAPPY")));
    if(!std::isfinite(rate))throw std::runtime_error("Nonfinite growth rule");
    // Native cvtsi2ss/mulss retain binary32 rule precision. Clamp before the
    // integer conversion so a large owned count cannot inherit int32 overflow.
    const float product=rate*static_cast<float>(wide::subtract(happiness,integer(d,"UNHAPPY_THRESHOLD")));
    return product<=-100.0f?-100:product>=0.0f?0:static_cast<std::int64_t>(product);
}
std::int64_t supply_penalty(const content::GlobalDefineCache &d,std::int64_t over){
    if(over<=0)return 0;
    const auto cap=std::max<std::int64_t>(0,integer(d,"MAX_UNIT_SUPPLY_GROWTH_MOD"));
    const auto rate=std::max<std::int64_t>(0,integer(d,"GROWTH_PENALTY_PER_UNIT_OVER_SUPPLY"));
    if(!rate)return 0;
    return -(over>cap/rate?cap:wide::multiply_divide(over,rate,1));
}
}
VpCityGrowthRules load_vp_city_growth_rules(const content::RulesDatabase &db){
    VpCityGrowthRules r;const auto q=db.prefetch_info("Beliefs",[&](auto id,auto,const auto &row){
        r.beliefs[id64(id)]={row.integer("CityGrowthModifier"),row.boolean("RequiresPeace")};return true;});
    if(!q.all_succeeded())throw std::runtime_error("Growth belief Info: "+q.error);return r;
}
void restore_vp_city_growth(const saves::SavedSessionSnapshot &s,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout)throw std::runtime_error("Growth import requires VP world");
    const auto vector=[&](const auto &fields){const auto &v=named(fields,"m_aiGoldenAgeYieldMod").values;YieldValues result;
        if(v.size()!=world.yield_slots)throw std::runtime_error("Growth yield wire span mismatch");
        for(std::size_t i=0;i<v.size();++i)if(v[i])result[id64(i)]=v[i];return result;};
    std::set<std::int64_t> players;
    for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;auto &p=world.players.at(owner);
        if(!players.insert(owner).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=owner)
            throw std::runtime_error("Growth player binding mismatch");
        auto &profile=ensure_vp_city_growth_profile(world);
        VpCityGrowthProfileState::Player g;g.city=scalar(sp.core.fields,"m_iCityGrowthMod");g.capital=scalar(sp.core.fields,"m_iCapitalGrowthMod");
        g.boon=scalar(sp.traits.fields,"m_iGrowthBoon");g.population_boost_religion=scalar(sp.traits.fields,"m_bPopulationBoostReligion")!=0;g.golden_age=vector(sp.core.fields);
        const auto &map=named(sp.traits.fields,"m_aiGoldenAgeYieldModifier").values;
        if(map.size()%2)throw std::runtime_error("Invalid growth trait map");
        for(std::size_t i=0;i<map.size();i+=2)g.trait_golden_age.emplace(map[i],map[i+1]); // native map insert: first duplicate wins
        profile.players[owner]=std::move(g);std::set<std::int64_t> cities;
        for(const auto &sc:sp.cities.cities){const auto id=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(id);
            if(!cities.insert(id).second||sc.core.layout!=layout||city.owner!=owner||city.id!=id||scalar(sc.core.fields,"m_eOwner")!=owner||
               city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})throw std::runtime_error("Growth city binding mismatch");
            VpCityGrowthProfileState::City cg;cg.happiness_delta=scalar(sc.core.fields,"m_iHappinessDelta");cg.event=scalar(sc.core.fields,"m_iGrowthEvent");
            cg.tourism=scalar(sc.core.fields,"m_iGrowthFromTourism");cg.golden_age=vector(sc.core.fields);profile.cities[{owner,id}]=std::move(cg);
        }
        if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete growth cities");
    }
    if(players.size()!=world.players.size())throw std::runtime_error("Incomplete growth players");
}
VpCityGrowthQuote vp_city_growth_modifiers(const VpCityGrowthContext &c,const CityYieldState &city,std::int64_t over,std::int64_t assumed){
    const auto &w=c.religion.water;const auto &world=w.cities;const auto &d=w.defines;
    if(world.profile!=vp||w.players.profile!=vp||c.diplomacy.profile!=vp||c.science.profile!=vp||
       d.profile()!=content::GlobalDefineProfile::vp542||!w.options||!w.options->all_succeeded())throw std::runtime_error("Growth requires shared VP world");
    const auto &p=world.players.at(city.owner);
    if(&p.cities.at(city.id)!=&city||!city.we_love_king_turns||over<0)throw std::runtime_error("Growth requires live resolved inputs");
    const auto &pg=vp_city_growth_player(world,city.owner);const auto &cg=vp_city_growth_city(world,city.owner,city.id);const bool balance=w.options->enabled("BALANCE_VP");VpCityGrowthQuote q;
    q.capital=city.id==p.capital?pg.capital:0;q.player=pg.city;
    if(p.golden_age){q.golden_city=value(cg.golden_age,c.rules.food);q.golden_player=value(pg.golden_age,c.rules.food);}
    q.golden_trait=value(pg.trait_golden_age,c.rules.food); // intentionally unconditional in VP
    if(balance)q.supply=supply_penalty(d,over);
    q.event=cg.event;q.tourism=cg.tourism;if(city.puppet)q.puppet=std::min<std::int64_t>(0,integer(d,"PUPPET_GROWTH_MODIFIER"));
    const auto team=w.players.players.at(city.owner).team;
    // getAtWarCount(false): living civilization teams INCLUDING city-states;
    // the explicit civ-team domain excludes barbarians, not a host ID bound.
    const auto &wars=c.diplomacy.teams.at(team).at_war;
    for(auto other:c.diplomacy.civ_teams)if(c.diplomacy.teams.at(other).alive_count>0&&wars.contains(other)){q.at_peace=false;break;}
    const auto selection=vp_plot_religion_selection(world,c.religion.rules,city);
    if(const auto *religion=selection.majority){
        const auto &religion_profile=vp_religion_beliefs(*religion);
        for(auto id:religion->beliefs){const auto &b=c.rules.beliefs.at(id);
            if(b.modifier&&(q.at_peace||!b.requires_peace)&&vp_plot_belief_valid(c.religion,id,religion_profile.belief_religion,city.owner,&city))q.beliefs=wide::add(q.beliefs,b.modifier);}
        if(pg.population_boost_religion){if(!vp_player_state_religion(p))throw std::runtime_error("Unresolved growth state religion");
            if(vp_city_majority_religion(city)==vp_player_state_religion(p)){std::int64_t followers=0;for(const auto &r:city.religions)if(r.religion==*vp_city_majority_religion(city)){followers=r.followers;break;}
                q.followers=wide::multiply_divide(followers,integer(d,"BALANCE_FOLLOWER_GROWTH_BONUS"),1);}}
        if(selection.secondary!=-1)q.secondary=c.rules.beliefs.at(selection.secondary).modifier;
    }
    const auto pantheon=vp_permanent_pantheon(w,city,selection);
    if(pantheon.religion)q.pantheon=c.rules.beliefs.at(pantheon.belief).modifier;
    const bool neutral=p.minor||c.science.barbarian_players.contains(city.owner)||p.cities.empty()||world.no_happiness;
    q.excess_happiness=neutral?(balance?integer(d,"UNHAPPY_THRESHOLD"):0):
        vp_science_profile(c.science.players.at(city.owner)).cached_happiness_total;
    q.unhappy=q.excess_happiness<(balance?integer(d,"UNHAPPY_THRESHOLD"):0);
    if(balance){const auto local=wide::add(cg.happiness_delta,assumed);
        auto mod=wide::multiply_divide(local,integer(d,local>0?"LOCAL_HAPPINESS_FOOD_MODIFIER":"LOCAL_UNHAPPINESS_FOOD_MODIFIER"),1);
        if(q.unhappy)mod=wide::add(std::min<std::int64_t>(0,mod),global_penalty(d,q.excess_happiness));
        q.happiness=std::clamp<std::int64_t>(mod,-100,100);
    }else if(q.excess_happiness<=integer(d,"VERY_UNHAPPY_THRESHOLD"))q.happiness=integer(d,"VERY_UNHAPPY_GROWTH_PENALTY");
    else if(q.unhappy)q.happiness=integer(d,"UNHAPPY_GROWTH_PENALTY");
    if(*city.we_love_king_turns>0)q.celebration=wide::add(integer(d,"WLTKD_GROWTH_MULTIPLIER"),pg.boon);
    for(auto n:{q.capital,q.player,q.golden_city,q.golden_player,q.golden_trait,q.supply,q.event,q.tourism,q.puppet,q.beliefs,q.followers,q.secondary,q.pantheon,q.happiness,q.celebration})q.total=wide::add(q.total,n);
    q.total=std::max<std::int64_t>(-100,q.total);return q;
}
VpCityGrowthQuote vp_city_growth(const VpCityGrowthContext &growth,VpUnitSupplyContext &supply,const CityYieldState &city,std::int64_t assumed){
    const auto &w=growth.religion.water;
    if(&w.cities!=&supply.cities||&w.players!=&supply.players||&growth.diplomacy!=&supply.diplomacy||&w.defines!=&supply.defines||w.options!=&supply.options)
        throw std::runtime_error("Growth and supply require the same shared world");
    return vp_city_growth_modifiers(growth,city,vp_unit_supply(supply,city.owner).over,assumed);
}
} // namespace civ5::runtime
