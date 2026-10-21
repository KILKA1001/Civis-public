#include "civ5/runtime/bnw_unhappiness.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <cmath>
#include <climits>
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {
namespace {
template<class T> const T &named(const std::vector<T> &v,std::string_view name) {
    const T *found=nullptr;
    for(const auto &f:v) if(f.name && std::string_view(f.name)==name) {
        if(found)throw std::runtime_error("Duplicate unhappiness field: "+std::string(name));found=&f;
    }
    if(!found)throw std::runtime_error("Missing unhappiness field: "+std::string(name));return *found;
}
template<class T> std::int64_t scalar(const std::vector<T> &v,std::string_view name) {
    const auto &a=named(v,name).values;
    if(a.size()!=1)throw std::runtime_error("Invalid unhappiness scalar: "+std::string(name));return a.front();
}
const content::GlobalDefineValue &definition(const content::GlobalDefineCache &d,std::string_view name) {
    const auto *entry=d.find(name);
    if(!entry || !entry->lookup.error.empty())throw std::runtime_error("Unresolved unhappiness define: "+std::string(name));
    return entry->lookup.value;
}
std::int64_t integer(const content::GlobalDefineCache &d,std::string_view name) {return std::get<std::int64_t>(definition(d,name));}
std::int64_t truncated_real(double value) {
    if(!std::isfinite(value) || value < -0x1p63 || value >= 0x1p63)
        throw std::overflow_error("Occupied population unhappiness exceeds int64");
    return static_cast<std::int64_t>(value);
}
struct WideUnhappinessArithmetic {
 static constexpr bool native=false;
 static std::int64_t integer(std::int64_t v){return v;}
 static std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
 static std::int64_t subtract(std::int64_t a,std::int64_t b){return wide::subtract(a,b);}
 static std::int64_t multiply_divide(std::int64_t a,std::int64_t b,std::int64_t d){return wide::multiply_divide(a,b,d);}
 static std::int64_t real(double v){return truncated_real(v);}
};
struct NativeUnhappinessArithmetic {
 static constexpr bool native=true;
 static std::int64_t integer(std::int64_t n){if(n<INT32_MIN||n>INT32_MAX)throw std::overflow_error("Native unhappiness int32 intermediate overflow");return n;}
 static std::int64_t add(std::int64_t a,std::int64_t b){return integer(integer(a)+integer(b));}
 static std::int64_t subtract(std::int64_t a,std::int64_t b){return integer(integer(a)-integer(b));}
 static std::int64_t multiply_divide(std::int64_t a,std::int64_t b,std::int64_t d){
  const auto product=integer(integer(a)*integer(b));d=integer(d);
  if(!d||(product==INT32_MIN&&d==-1))throw std::overflow_error("Native unhappiness division unavailable");return product/d;
 }
 static std::int64_t real(double v){
  if(!std::isfinite(v)||v<static_cast<double>(INT32_MIN)||v>=2147483648.0)throw std::overflow_error("Native occupied population conversion unavailable");
  return static_cast<std::int32_t>(v);
 }
};
template<class Arithmetic>std::int64_t percent(std::int64_t value,std::int64_t rate){return Arithmetic::multiply_divide(value,rate,100);}
template<class Arithmetic>std::int64_t plus_percent(std::int64_t value,std::int64_t rate){return percent<Arithmetic>(value,Arithmetic::add(100,rate));}
template<class Arithmetic,class F>void visit_cities(const CityYieldPlayer& cities,F&& visit){
 if constexpr(!Arithmetic::native){for(const auto& [id,c]:cities.cities)visit(id,c);}
 else {
  // firstCity/nextCity visit occupied FLTA slots, not sorted generation IDs.
  std::map<std::int32_t,std::pair<std::int64_t,const CityYieldState*>> slots;
  for(const auto& [id,c]:cities.cities){const auto n=Arithmetic::integer(id);if(n<0||!slots.emplace(static_cast<std::int32_t>(n)&8191,std::pair{id,&c}).second)throw std::runtime_error("Native unhappiness city slots ambiguous");}
  for(const auto& [slot,entry]:slots){(void)slot;visit(entry.first,*entry.second);}
 }
}
void profiles(const ResearchContext &setup,const CityYieldContext &cities) {
    if(setup.profile!=content::TechnologyProfile::bnw || cities.profile!=content::TechnologyProfile::bnw)
        throw std::runtime_error("BNW unhappiness requires the BNW setup/city profile");
}
}
BnwUnhappinessRules load_bnw_unhappiness_rules(const content::RulesDatabase &db) {
    BnwUnhappinessRules rules;
    const auto handicaps=db.prefetch_info("HandicapInfos",[&](auto slot,auto,const auto &row) {
        if(slot>INT64_MAX)throw std::overflow_error("Unhappiness handicap ID exceeds int64");
        rules.handicaps.emplace(static_cast<std::int64_t>(slot),BnwUnhappinessHandicap{
            row.integer("NumCitiesUnhappinessMod"),row.integer("PopulationUnhappinessMod"),row.integer("AIUnhappinessPercent")});
        return true;
    });
    const auto specialists=db.prefetch_info("Specialists");
    if(!handicaps.all_succeeded() || !specialists.all_succeeded())throw std::runtime_error("Unhappiness Info cache is incomplete");
    rules.specialist_slots=specialists.slots;return rules;
}
BnwUnhappinessContext restore_bnw_unhappiness(const saves::SavedSessionSnapshot &snapshot,
    const ResearchContext &setup,const CityYieldContext &cities,const BnwUnhappinessRules &rules,
    const content::InfoTypeCatalog &types) {
    profiles(setup,cities);
    constexpr auto layout=saves::GameStateLayout::expansion2_version1;
    if(!snapshot.pregame || !snapshot.all_players || snapshot.game_state.layout!=layout ||
       snapshot.pregame->layout!=layout || snapshot.all_players->layout!=layout || !types.report().all_succeeded())
        throw std::runtime_error("BNW unhappiness needs matching complete saved state/types");
    BnwUnhappinessContext result;
    result.world_cities_percent=snapshot.pregame->world.values.integers.at("num_cities_unhappiness_percent");
    for(const auto &saved:snapshot.all_players->players) {
        const auto id=saved.saved_player_id;
        if(!setup.players.contains(id) || !cities.players.contains(id) || saved.core.layout!=layout ||
           saved.traits.layout!=layout || saved.trade_culture.layout!=layout || saved.cities.layout!=layout)
            throw std::runtime_error("BNW unhappiness player/profile mismatch");
        BnwPlayerUnhappiness p;
        p.happiness=scalar(saved.core.fields,"m_iHappiness");p.units=scalar(saved.core.fields,"m_iUnhappinessFromUnits");
        p.units_modifier=scalar(saved.core.fields,"m_iUnhappinessFromUnitsMod");p.population_modifier=scalar(saved.core.fields,"m_iUnhappinessMod");
        p.cities_modifier=scalar(saved.core.fields,"m_iCityCountUnhappinessMod");p.occupied_modifier=scalar(saved.core.fields,"m_iOccupiedPopulationUnhappinessMod");
        p.capital_modifier=scalar(saved.core.fields,"m_iCapitalUnhappinessMod");p.half_specialists=scalar(saved.core.fields,"m_iHalfSpecialistUnhappinessCount")>0;
        p.trait_cities_modifier=scalar(saved.traits.fields,"m_iCityUnhappinessModifier");
        p.trait_population_modifier=scalar(saved.traits.fields,"m_iPopulationUnhappinessModifier");
        const auto &culture=named(saved.trade_culture.blocks,"player_culture");
        if(named(culture.fields,"version").value>=3)p.public_opinion=named(culture.fields,"m_iOpinionUnhappiness").value;
        // Earlier PlayerCulture versions retain constructor's zero opinion.
        for(const auto &city:saved.cities.cities) {
            if(city.core.layout!=layout || city.management.layout!=layout)throw std::runtime_error("Unhappiness city profile mismatch");
            const auto city_id=scalar(city.core.fields,"m_iID");
            if(!cities.players.at(id).cities.contains(city_id) || scalar(city.core.fields,"m_eOwner")!=id)
                throw std::runtime_error("Unhappiness city identity mismatch");
            BnwCityUnhappiness c;c.occupied=scalar(city.core.fields,"m_bOccupied")!=0;
            c.no_occupied_unhappiness=scalar(city.core.fields,"m_iNoOccupiedUnhappinessCount")>0;
            // m_bIgnoreCityForHappiness is not serialized in BNW; reset(false).
            const auto &citizens=named(city.management.blocks,"citizens");
            for(const auto &row:named(citizens.hashes,"m_aiSpecialistCounts").rows) {
                const auto *type=row.type_hash?types.find_hash(row.type_hash):nullptr;
                if(!type || type->id<0 || static_cast<std::uint64_t>(type->id)>=rules.specialist_slots)continue;
                if(row.values.size()!=1)throw std::runtime_error("Invalid unhappiness specialist count");
                c.specialists[type->id]=row.values.front();
            }
            if(!p.cities.emplace(city_id,std::move(c)).second)throw std::runtime_error("Duplicate unhappiness city");
        }
        if(p.cities.size()!=cities.players.at(id).cities.size())throw std::runtime_error("Incomplete unhappiness city roster");
        if(!result.players.emplace(id,std::move(p)).second)throw std::runtime_error("Duplicate unhappiness player");
    }
    if(result.players.size()!=setup.players.size() || result.players.size()!=cities.players.size())
        throw std::runtime_error("Incomplete unhappiness player roster");
    return result;
}
template<class Arithmetic>BnwUnhappinessQuote evaluate_unhappiness(const BnwUnhappinessContext &s,const ResearchContext &setup,
    const CityYieldContext &world,const BnwUnhappinessRules &rules,const content::GlobalDefineCache &d,
    std::int64_t player,BnwUnhappinessAssumptions assumptions,const BnwCitySpecialistCounts* specialist_source=nullptr) {
    profiles(setup,world);
    if(d.profile()!=content::GlobalDefineProfile::bnw)throw std::runtime_error("BNW unhappiness define profile mismatch");
    const auto &p=s.players.at(player);const auto &base=setup.players.at(player);const auto &cities=world.players.at(player);
    const auto &handicap=rules.handicaps.at(base.handicap);
    if(p.cities.size()!=cities.cities.size())throw std::runtime_error("Unhappiness city roster changed without its inputs");
    const auto normal_per_city=Arithmetic::multiply_divide(integer(d,"UNHAPPINESS_PER_CITY"),100,1);
    const auto captured_per_city=Arithmetic::multiply_divide(integer(d,"UNHAPPINESS_PER_CAPTURED_CITY"),100,1);
    const auto normal_per_pop=Arithmetic::multiply_divide(integer(d,"UNHAPPINESS_PER_POPULATION"),100,1);
    // The SDK getter is float. Its multiplication by 100 rounds at binary32
    // precision, then promotes to double for the population product. Retaining
    // this rule precision does not impose the original int32 result capacity.
    const float occupied_rate=static_cast<float>(std::get<double>(definition(d,"UNHAPPINESS_PER_OCCUPIED_POPULATION")));
    const double occupied_per_pop=static_cast<double>(occupied_rate*100.0f);
    BnwUnhappinessQuote q;
    visit_cities<Arithmetic>(cities,[&](const auto id,const CityYieldState& city) {
        const auto &c=p.cities.at(id);const UnhappinessCityKey key{player,id};
        const bool annexed=assumptions.annexed==key,puppeted=assumptions.puppeted==key;
        const bool occupied=c.occupied && !city.puppet && !c.no_occupied_unhappiness;
        // Count and population previews have DIFFERENT source precedence when
        // both optional arguments identify the same city.
        const bool normal_count=puppeted || (!annexed && !c.ignore && !occupied);
        const bool captured_count=!puppeted && (annexed || (!c.ignore && occupied));
        const bool normal_pop=!annexed && (puppeted || (!c.ignore && !occupied));
        const bool occupied_pop=annexed || (!puppeted && !c.ignore && occupied);
        if(normal_count)q.normal_cities_times100=Arithmetic::add(q.normal_cities_times100,normal_per_city);
        if(captured_count)q.captured_cities_times100=Arithmetic::add(q.captured_cities_times100,captured_per_city);
        if(!normal_pop && !occupied_pop)return;
        auto population=Arithmetic::integer(city.population);
        if(p.half_specialists) {
            std::int64_t specialists=0;
            if constexpr(Arithmetic::native){
                if(!specialist_source)throw std::runtime_error("Native unhappiness half-specialist native array initialization owner unavailable");
                specialists=Arithmetic::integer(specialist_source->total_for_city(player,id));
            }else{
                const auto default_specialist=integer(d,"DEFAULT_SPECIALIST");
                for(const auto &[type,count]:c.specialists)
                    if(type>=0 && static_cast<std::uint64_t>(type)<rules.specialist_slots && type!=default_specialist)
                        specialists=Arithmetic::add(specialists,count);
            }
            population=Arithmetic::subtract(population,Arithmetic::add(specialists,1)/2);
        }
        if(normal_pop) {
            auto amount=Arithmetic::multiply_divide(population,normal_per_pop,1);
            if(id==cities.capital && p.capital_modifier)amount=plus_percent<Arithmetic>(amount,p.capital_modifier);
            q.normal_population_times100=Arithmetic::add(q.normal_population_times100,amount);
        }
        if(occupied_pop) {
            auto amount=Arithmetic::real(static_cast<double>(population)*occupied_per_pop);
            if(p.occupied_modifier)amount=plus_percent<Arithmetic>(amount,p.occupied_modifier);
            q.occupied_population_times100=Arithmetic::add(q.occupied_population_times100,amount);
        }
    });
    const auto cities_modifier=Arithmetic::add(p.cities_modifier,p.trait_cities_modifier);
    const auto count_modifier=[&](auto n){return percent<Arithmetic>(percent<Arithmetic>(plus_percent<Arithmetic>(n,cities_modifier),handicap.cities),s.world_cities_percent);};
    q.normal_cities_times100=count_modifier(q.normal_cities_times100);
    q.captured_cities_times100=count_modifier(q.captured_cities_times100);
    q.normal_population_times100=percent<Arithmetic>(plus_percent<Arithmetic>(plus_percent<Arithmetic>(q.normal_population_times100,p.population_modifier),p.trait_population_modifier),handicap.population);
    q.occupied_population_times100=percent<Arithmetic>(q.occupied_population_times100,handicap.population);
    q.units_times100=std::max<std::int64_t>(0,Arithmetic::subtract(p.units,integer(d,"FREE_UNIT_HAPPINESS")));
    if(p.units_modifier)q.units_times100=plus_percent<Arithmetic>(q.units_times100,p.units_modifier);
    auto total=Arithmetic::add(q.normal_cities_times100,q.captured_cities_times100);
    total=Arithmetic::add(total,q.normal_population_times100);total=Arithmetic::add(total,q.occupied_population_times100);total=Arithmetic::add(total,q.units_times100);
    q.public_opinion=Arithmetic::integer(p.public_opinion);q.before_ai=Arithmetic::add(total/100,p.public_opinion);q.total=q.before_ai;
    if(!setup.is_human(base)) {
        bool human_teammate=false;
        for(const auto &[id,other]:setup.players) if(setup.is_human(other) && other.alive && other.team==base.team) {human_teammate=true;break;}
        if(!human_teammate) {q.ai_discount_applied=true;q.total=percent<Arithmetic>(q.total,rules.handicaps.at(setup.game_handicap).ai);}
    }
    q.excess_happiness=Arithmetic::subtract(p.happiness,q.total);return q;
}
BnwUnhappinessQuote bnw_unhappiness(const BnwUnhappinessContext& s,const ResearchContext& setup,
 const CityYieldContext& cities,const BnwUnhappinessRules& rules,const content::GlobalDefineCache& defines,
 std::int64_t player,BnwUnhappinessAssumptions assumptions){
 return evaluate_unhappiness<WideUnhappinessArithmetic>(s,setup,cities,rules,defines,player,std::move(assumptions));
}
namespace detail {
BnwUnhappinessQuote bnw_native_unhappiness_math(const BnwUnhappinessContext& s,const ResearchContext& setup,
 const CityYieldContext& cities,const BnwUnhappinessRules& rules,const content::GlobalDefineCache& defines,std::int64_t player){
 return evaluate_unhappiness<NativeUnhappinessArithmetic>(s,setup,cities,rules,defines,player,{});
}
BnwUnhappinessQuote bnw_native_unhappiness_math_with_specialists(const BnwUnhappinessContext& s,const ResearchContext& setup,
 const CityYieldContext& cities,const BnwUnhappinessRules& rules,const content::GlobalDefineCache& defines,std::int64_t player,const BnwCitySpecialistCounts& source){
 source.require_current_source();
 return evaluate_unhappiness<NativeUnhappinessArithmetic>(s,setup,cities,rules,defines,player,{},&source);
}
}

std::map<std::int64_t,BnwUnhappinessQuote> bind_bnw_unhappiness(const BnwUnhappinessContext &s,const ResearchContext &setup,
    CityYieldContext &world,const BnwUnhappinessRules &rules,const content::GlobalDefineCache &defines) {
    if(s.players.size()!=world.players.size() || s.players.size()!=setup.players.size())throw std::runtime_error("Incomplete unhappiness binding roster");
    std::map<std::int64_t,BnwUnhappinessQuote> results;
    for(const auto &[id,p]:world.players)results.emplace(id,bnw_unhappiness(s,setup,world,rules,defines,id));
    for(const auto &[id,q]:results)world.players.at(id).excess_happiness=q.excess_happiness;
    return results;
}

namespace detail {
namespace {void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}}
void require_bnw_current_unhappiness_inputs(const LiveSessionState& s,const BnwUnhappinessRules& rules,
 const content::InfoTypeCatalog& types,std::int64_t p){
    // Reconcile serialized operands only; keep SAME retained ignore flags.
    // This read model is never installed/reset into the live context.
    const auto expected=restore_bnw_unhappiness(s.world,s.setup,s.cities,rules,types);
    require(expected.world_cities_percent==s.unhappiness.world_cities_percent,"BNW happiness current world unhappiness rate stale");const auto &e=expected.players.at(p),&a=s.unhappiness.players.at(p);
    require(e.happiness==a.happiness&&e.units==a.units&&e.units_modifier==a.units_modifier&&e.population_modifier==a.population_modifier&&e.cities_modifier==a.cities_modifier&&
         e.occupied_modifier==a.occupied_modifier&&e.capital_modifier==a.capital_modifier&&e.trait_cities_modifier==a.trait_cities_modifier&&e.trait_population_modifier==a.trait_population_modifier&&
         e.public_opinion==a.public_opinion&&e.half_specialists==a.half_specialists&&e.cities.size()==a.cities.size(),"BNW happiness current serialized unhappiness owner stale");
    for(const auto &[id,c]:e.cities){const auto &live=a.cities.at(id);require(c.occupied==live.occupied&&c.no_occupied_unhappiness==live.no_occupied_unhappiness&&c.specialists==live.specialists,"BNW happiness current city unhappiness operands stale");}
    // Authenticate every population/puppet source read for this player's
    // unhappiness formula against the SAME borrowed city registry.
    for(const auto& [id,c]:s.cities.players.at(p).cities){
        require(p>=0&&p<64&&id>=0&&id<=INT32_MAX,"Current unhappiness canonical city outside native width");
        const auto* raw=resolve_bnw_city_registry(s.world,s.cities,static_cast<std::int32_t>(p),static_cast<std::int32_t>(id));
        if(!raw||scalar(raw->core.fields,"m_iPopulation")!=c.population||(scalar(raw->core.fields,"m_bPuppet")!=0)!=c.puppet)
            throw std::runtime_error("Current unhappiness city population/puppet source stale");
    }
}
}

} // namespace civ5::runtime
