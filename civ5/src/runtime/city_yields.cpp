#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <climits>
#include <optional>
#include <stdexcept>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;

struct WideYieldArithmetic {
 static std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
 static std::int64_t subtract(std::int64_t a,std::int64_t b){return wide::subtract(a,b);}
 static std::int64_t multiply_divide(std::int64_t a,std::int64_t b,std::int64_t d){return wide::multiply_divide(a,b,d);}
 static void process(std::int64_t){}
 static void happiness(const CityYieldContext&,const CityYieldPlayer&,std::int64_t){}
};
struct NativeFoodYieldArithmetic {
 static std::int32_t integer(std::int64_t n){
  if(n<INT32_MIN||n>INT32_MAX)throw CityYieldUnavailable("Native FOOD rate int32 intermediate overflow");
  return static_cast<std::int32_t>(n);
 }
 static std::int64_t add(std::int64_t a,std::int64_t b){return integer(std::int64_t(integer(a))+integer(b));}
 static std::int64_t subtract(std::int64_t a,std::int64_t b){return integer(std::int64_t(integer(a))-integer(b));}
 static std::int64_t multiply_divide(std::int64_t a,std::int64_t b,std::int64_t d){
  // The source product is an int BEFORE division. A wide quotient that fits
  // cannot rescue an overflowing source product.
  const auto product=integer(std::int64_t(integer(a))*integer(b));const auto divisor=integer(d);
  if(!divisor||(product==INT32_MIN&&divisor==-1))throw CityYieldUnavailable("Native FOOD rate division unavailable");
  return product/divisor;
 }
 static void process(std::int64_t modifier){
  if(integer(modifier)!=0)throw CityYieldUnavailable("Native FOOD rate nonzero process requires production recursion owner");
 }
 static void happiness(const CityYieldContext& s,const CityYieldPlayer& p,std::int64_t yield){
  if(yield==0&&!s.no_happiness){
   if(!p.excess_happiness)throw CityYieldUnavailable("Native FOOD rate actual excess happiness owner unavailable");
   const auto excess=integer(*p.excess_happiness);
   if(excess<0)(void)subtract(0,excess); // evaluated BEFORE source yield switch
  }
 }
};

struct NativeScienceYieldArithmetic:NativeFoodYieldArithmetic {
 static void process(std::int64_t modifier){
  (void)integer(modifier); // actual current owner binds reached Production child; pure evaluator keeps recursion
 }
 static void happiness(const CityYieldContext& s,const CityYieldPlayer& p,std::int64_t){
  if(!s.no_happiness){
   if(!p.excess_happiness)throw CityYieldUnavailable("Native SCIENCE rate actual excess happiness owner unavailable");
   const auto excess=integer(*p.excess_happiness);
   if(excess<0)(void)subtract(0,excess); // source negation occurs before yield switch even SCIENCE
  }
 }
};

std::int64_t value(const YieldValues &a, std::int64_t id) {
    const auto it=a.find(id); return it==a.end() ? 0 : it->second;
}
std::int64_t id64(std::uint64_t id) {
    if (id>INT64_MAX) throw std::overflow_error("City yield ID exceeds int64");
    return static_cast<std::int64_t>(id);
}
template<class T> const T &named(const std::vector<T> &v, std::string_view name) {
    const T *found=nullptr;
    for (const auto &x:v) if (x.name && std::string_view(x.name)==name) {
        if (found) throw std::runtime_error("Duplicate city yield field: "+std::string(name));
        found=&x;
    }
    if (!found) throw std::runtime_error("Missing city yield field: "+std::string(name));
    return *found;
}
template<class T> std::int64_t scalar(const std::vector<T> &v, std::string_view name) {
    const auto &a=named(v,name).values;
    if (a.size()!=1) throw std::runtime_error("Invalid city yield scalar: "+std::string(name));
    return a.front();
}
YieldValues sparse(const std::vector<std::int64_t> &v) {
    YieldValues a; for (std::size_t i=0;i<v.size();++i) if(v[i]) a.emplace(id64(i),v[i]); return a;
}
std::int64_t define(const content::GlobalDefineCache &d, std::string_view name) {
    const auto *entry=d.find(name);
    if(!entry || !entry->lookup.error.empty() || !std::holds_alternative<std::int64_t>(entry->lookup.value))
        throw std::runtime_error("Unresolved city yield define: "+std::string(name));
    return std::get<std::int64_t>(entry->lookup.value);
}
bool empty(const saves::TradeRouteObservation &r) {
    return r.origin_x==-1 && r.origin_y==-1 && r.destination_x==-1 && r.destination_y==-1;
}
template<class Arithmetic> std::int64_t trade_yield(const CityYieldContext &s, const CityYieldState &city, std::int64_t yield) {
    std::int64_t total=0;
    for (const auto &r:s.trade_routes) if (!empty(r)) {
        if (r.origin_owner==city.owner && r.origin_x==city.x && r.origin_y==city.y)
            total=Arithmetic::add(total,r.origin_yields.at(static_cast<std::size_t>(yield)));
        if (r.destination_owner==city.owner && r.destination_x==city.x && r.destination_y==city.y)
            total=Arithmetic::add(total,r.destination_yields.at(static_cast<std::size_t>(yield)));
    }
    return total;
}
template<class Arithmetic> std::int64_t religious_modifier(const CityYieldContext &s, const CityYieldRules &rules,
    const CityYieldState &city, std::int64_t yield) {
    std::int64_t total=0, most=-1, pressure=0, religion=-1;
    for (const auto &r:city.religions) {
        total=Arithmetic::add(total,r.followers);
        if (r.followers>most || (r.followers==most && r.pressure>pressure)) {
            most=r.followers; pressure=r.pressure; religion=r.religion;
        }
    }
    if (Arithmetic::multiply_divide(most,2,1)<total) religion=-1;
    for (const auto &r:s.religions) if(r.religion==religion && (religion!=s.pantheon || r.founder==city.owner)) {
        std::int64_t maximum=0;
        // This getter scans Info IDs once, unlike secondary pantheon selection.
        for (auto belief:std::set<std::int64_t>(r.beliefs.begin(),r.beliefs.end())) {
            const auto entry=rules.belief_follower_maximum.find(belief);
            if (entry==rules.belief_follower_maximum.end())
                throw CityYieldUnavailable("Religion refers to an unresolved belief Info");
            maximum=Arithmetic::add(maximum,value(entry->second,yield));
        }
        if (maximum>0) {
            // GetNumFollowers returns the FIRST matching row, not the row
            // selected by majority's pressure tie-break or a sum of duplicates.
            for (const auto &status:city.religions) if(status.religion==religion) return std::min(status.followers,maximum);
        }
        return 0;
    }
    return 0;
}
template<class Arithmetic> std::int64_t city_state_production_modifier(const CityYieldContext &s,const CityYieldRules &rules,
    const content::GlobalDefineCache &defines,const CityYieldState &city) {
    const auto civ=rules.civilization_buildings.find(s.players.at(city.owner).civilization);
    if(civ==rules.civilization_buildings.end()) return 0;
    std::int64_t result=0;
    for (const auto &[klass,building]:civ->second) {
        if (klass<0 || static_cast<std::uint64_t>(klass)>=rules.building_class_slots || building==-1) continue;
        const auto maximum=Arithmetic::add(0,define(defines,"CITY_MAX_NUM_BUILDINGS")); // native Define BEFORE count reads
        const auto real=Arithmetic::add(0,value(city.real_buildings,building)), free=Arithmetic::add(0,value(city.free_buildings,building));
        const auto count=maximum<=1 ? std::max(real,free) : Arithmetic::add(real,free);
        if(count<=0 || !rules.building_trade_modifiers.contains(building)) continue;
        const auto mod=Arithmetic::add(0,value(rules.building_trade_modifiers,building)); // native getter BEFORE route count
        std::int64_t routes=0;
        // The source does NOT exclude empty trade entries in this getter.
        for(const auto &r:s.trade_routes) if(r.origin_owner==city.owner && s.players.at(r.destination_owner).minor)
            routes=Arithmetic::add(routes,1);
        if(mod>0 && routes>0) result=Arithmetic::multiply_divide(mod,routes,1); // last matching class wins
    }
    return result;
}
} // namespace
CityYieldRules load_city_yield_rules(const content::RulesDatabase &db, TechnologyProfile profile) {
    CityYieldRules r; r.profile=profile;
    if(profile==TechnologyProfile::vp542) return r; // cached default query has no Info dependency
    if(profile!=TechnologyProfile::bnw) throw std::runtime_error("Unknown city yield profile");
    const auto read=[&](content::DatabaseStatement statement,const auto &visit) {
        const auto report=db.query_rows(statement,visit);
        if(!report.all_succeeded()) throw std::runtime_error("City yield rules: "+report.error);
    };
    const auto info=[&](const char *table,const auto &visit) {
        const auto report=db.prefetch_info(table,visit);
        if(!report.all_succeeded()) throw std::runtime_error("City yield Info: "+report.error);
        return report.slots;
    };
    info("Yields",[&](auto slot,auto,const auto &row){r.golden_age_modifiers[id64(slot)]=row.integer("GoldenAgeYieldMod");return true;});
    r.belief_slots=info("Beliefs",[&](auto slot,auto,const auto &row) {
        auto &values=r.belief_follower_maximum[id64(slot)];
        read({"select Yields.ID, Max from Belief_MaxYieldModifierPerFollower inner join Yields on Yields.Type = YieldType where BeliefType = ?",
              {row.text("Type").value_or("")}},[&](const auto &v){
            if(v.integer_at(0)<0) throw std::runtime_error("Negative belief yield index");
            values[v.integer_at(0)]=v.integer_at(1);
        }); return true;
    });
    r.building_slots=info("Buildings",[&](auto slot,auto,const auto &row){r.building_trade_modifiers[id64(slot)]=row.integer("CityStateTradeRouteProductionModifier");return true;});
    r.building_class_slots=info("BuildingClasses",[](auto,auto,const auto &){return true;});
    load_city_building_replacements(db,r);
    return r;
}
void load_city_building_replacements(const content::RulesDatabase &db,CityYieldRules &r) {
    const auto read=[&](content::DatabaseStatement statement,const auto &visit) {
        const auto result=db.query_rows(statement,visit);if(!result.all_succeeded())throw std::runtime_error("City building replacements: "+result.error);
    };
    YieldValues defaults;
    read({"select BuildingClasses.ID, Buildings.ID from BuildingClasses inner join Buildings on Buildings.Type = DefaultBuilding",{}},
        [&](const auto &v){defaults[v.integer_at(0)]=v.integer_at(1);});
    std::map<std::int64_t,YieldValues> replacements;
    const auto result=db.prefetch_info("Civilizations",[&](auto slot,auto,const auto &row){
        auto &buildings=replacements[id64(slot)]; buildings=defaults;
        read({"select BuildingClasses.ID, coalesce(Buildings.ID, -1) from Civilization_BuildingClassOverrides inner join BuildingClasses on BuildingClassType = BuildingClasses.Type left outer join Buildings on BuildingType = Buildings.Type where CivilizationType = ?",
              {row.text("Type").value_or("")}},[&](const auto &v){buildings[v.integer_at(0)]=v.integer_at(1);}); return true;
    });
    if(!result.all_succeeded())throw std::runtime_error("City civilization Info: "+result.error);
    r.civilization_buildings=std::move(replacements);
}
CityYieldContext restore_city_yields(const saves::SavedSessionSnapshot &snapshot, ResearchContext &setup,
    const content::RulesDatabase &db,const CityYieldRules &rules,const content::InfoTypeCatalog &types) {
    const bool vp=setup.profile==TechnologyProfile::vp542;
    const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!snapshot.all_players || !snapshot.pregame || snapshot.game_state.layout!=layout ||
       snapshot.all_players->layout!=layout || snapshot.pregame->layout!=layout || rules.profile!=setup.profile ||
       snapshot.trade.layout!=layout || !types.report().all_succeeded() ||
       (!vp && (!snapshot.map_regions || !snapshot.map_regions->dimensions || !snapshot.map_plots ||
                snapshot.map_regions->layout!=layout || snapshot.map_plots->layout!=layout)))
        throw std::runtime_error("City yields need matching complete saved state and rules");
    CityYieldContext s; s.profile=setup.profile; s.yield_slots=snapshot.trade.dimensions.yield_slots;
    s.trade_routes=snapshot.trade.routes;
    s.no_happiness=setup.game_option_value(db,"GAMEOPTION_NO_HAPPINESS")==1;
    const auto &civilizations=snapshot.pregame->slots.integer_arrays.at("civilizations");
    const auto &minors=snapshot.pregame->archive.flag_arrays.at("minor_nation_civs");
    if(!vp) {
        const auto &areas=named(snapshot.map_regions->containers,"areas");
        const auto &dimensions=*snapshot.map_regions->dimensions;
        if(dimensions.yields!=s.yield_slots) throw std::runtime_error("Area/city yield wire span mismatch");
        for(const auto &area:areas.records) {
            const auto id=named(area.fields,"id").value;
            const auto &v=named(area.arrays,"yield_modifiers").values;
            for(const auto &[player,unused]:setup.players) {
                if(player<0 || static_cast<std::uint64_t>(player)>=dimensions.players) throw std::runtime_error("Player outside area wire rows");
                YieldValues values;
                for(std::uint64_t y=0;y<dimensions.yields;++y) {
                    const auto n=v.at(static_cast<std::size_t>(player*dimensions.yields+y));
                    if(n) values.emplace(id64(y),n);
                }
                if(!s.area_modifiers.emplace(std::pair{id,player},std::move(values)).second) throw std::runtime_error("Duplicate city yield area");
            }
        }
        for(const auto &religion:snapshot.religion_culture.religions) {
            CityYieldReligionBeliefs b; b.religion=religion.religion; b.founder=religion.founder;
            for(auto hash:religion.beliefs.belief_hashes) {
                const auto *entry=hash?types.find_hash(hash):nullptr;
                if(!entry || entry->id==-1) continue; // source unknown hash maps to NO_BELIEF
                if(entry->id<0 || static_cast<std::uint64_t>(entry->id)>=rules.belief_slots) continue;
                b.beliefs.push_back(entry->id);
            }
            s.religions.push_back(std::move(b));
        }
    }
    for(const auto &saved:snapshot.all_players->players) {
        const auto player=saved.saved_player_id;
        if(player<0 || !setup.players.contains(player) || saved.core.layout!=layout || saved.cities.layout!=layout)
            throw std::runtime_error("Invalid city yield player/profile");
        CityYieldPlayer p; p.civilization=civilizations.at(static_cast<std::size_t>(player));
        p.minor=minors.at(static_cast<std::size_t>(player));
        p.capital=scalar(saved.core.fields,"m_iCapitalCityID");
        p.golden_age=scalar(saved.core.fields,"m_iGoldenAgeTurns")>0;
        if(!vp) {
            // CvPlayer::Read restores this existing empire counter. Import it
            // before any live city effect; neither zero nor a sum of city rows
            // supplies the saved native total. VP retains its profile loader.
            const auto &total=named(saved.core.fields,"m_iTotalPopulation");
            if(total.encoding!=saves::PlayerCoreEncoding::i32 || total.values.size()!=1 ||
               total.wire_count || !total.row_counts.empty() || !total.hashes.empty() || !total.bytes.empty() ||
               total.values.front()<0 || total.values.front()>INT32_MAX)
                throw std::runtime_error("BNW city yield player total population native scalar unavailable");
            p.total_population=total.values.front();
            p.modifiers=sparse(named(saved.core.fields,"m_aiYieldRateModifier").values);
            p.capital_modifiers=sparse(named(saved.core.fields,"m_aiCapitalYieldRateModifier").values);
        }
        for(const auto &saved_city:saved.cities.cities) {
            if(saved_city.core.layout!=layout || (!vp && (saved_city.production.layout!=layout || saved_city.tail.layout!=layout)))
                throw std::runtime_error("City yield record profile mismatch");
            CityYieldState c; c.owner=scalar(saved_city.core.fields,"m_eOwner");
            c.source_core=std::pair{saved_city.core.start_offset,saved_city.core.end_offset};
            c.id=scalar(saved_city.core.fields,"m_iID"); c.x=scalar(saved_city.core.fields,"m_iX"); c.y=scalar(saved_city.core.fields,"m_iY");
            if(c.owner!=player) throw std::runtime_error("City yield owner disagrees with player collection");
            c.population=scalar(saved_city.core.fields,"m_iPopulation");
            c.resistance=scalar(saved_city.core.fields,"m_iResistanceTurns")>0;
            c.razing=scalar(saved_city.core.fields,"m_iRazingTurns")>0; c.puppet=scalar(saved_city.core.fields,"m_bPuppet")!=0;
            if(vp) {
                restore_vp_city_yield_cache(s,c.owner,c.id,saved_city.core);
            } else {
                const auto index=snapshot.map_header.grid.index(static_cast<std::uint64_t>(c.x),static_cast<std::uint64_t>(c.y));
                const auto &plot=snapshot.map_plots->plots.at(static_cast<std::size_t>(index));
                if(plot.index!=index) throw std::runtime_error("City plot collection is not ordered by grid index");
                c.area=named(plot.fields,"area").value;
                const auto fill=[&](const char *name,auto member) {
                    const auto &v=named(saved_city.core.fields,name).values;
                    if(v.size()!=s.yield_slots) throw std::runtime_error("BNW city yield wire span mismatch");
                    for(std::size_t y=0;y<v.size();++y) if(v[y]) c.yields[id64(y)].*member=v[y];
                };
                fill("m_aiBaseYieldRateFromTerrain",&CityYieldComponents::terrain);
                fill("m_aiBaseYieldRateFromBuildings",&CityYieldComponents::buildings);
                fill("m_aiBaseYieldRateFromSpecialists",&CityYieldComponents::specialists);
                fill("m_aiBaseYieldRateFromMisc",&CityYieldComponents::misc);
                fill("m_aiBaseYieldRateFromReligion",&CityYieldComponents::religion);
                fill("m_aiYieldPerPop",&CityYieldComponents::per_population);
                if(!saved_city.core.version || *saved_city.core.version>=4) fill("m_aiYieldPerReligion",&CityYieldComponents::per_religion);
                fill("m_aiYieldRateModifier",&CityYieldComponents::rate_modifier);
                fill("m_aiResourceYieldRateModifier",&CityYieldComponents::resource_modifier);
                fill("m_aiProductionToYieldModifier",&CityYieldComponents::process_modifier);
                const auto &religions=named(named(saved_city.tail.blocks,"religions").children,"m_ReligionStatus");
                for(const auto &r:religions.children) c.religions.push_back({scalar(r.children,"m_eReligion"),scalar(r.children,"m_iFollowers"),scalar(r.children,"m_iPressure")});
                const auto &buildings=named(saved_city.production.blocks,"buildings");
                const auto load_buildings=[&](const char *name,YieldValues &values) {
                    for(const auto &row:named(buildings.children,name).children) {
                        const auto hash=named(row.fields,"hash").value;
                        const auto *entry=hash?types.find_hash(static_cast<std::uint64_t>(hash)):nullptr;
                        if(!entry || entry->id<0 || static_cast<std::uint64_t>(entry->id)>=rules.building_slots) continue;
                        const auto &v=named(row.arrays,"values").values;
                        if(v.size()!=1) throw std::runtime_error("Invalid saved building count");
                        values[entry->id]=v.front(); // last remapped duplicate wins, including zero
                    }
                };
                load_buildings("m_paiNumRealBuilding",c.real_buildings); load_buildings("m_paiNumFreeBuilding",c.free_buildings);
                c.building_counts_resolved=true;
            }
            if(!p.cities.emplace(c.id,std::move(c)).second) throw std::runtime_error("Duplicate city yield city");
        }
        if(!s.players.emplace(player,std::move(p)).second) throw std::runtime_error("Duplicate city yield player");
    }
    if(s.players.size()!=setup.players.size()) throw std::runtime_error("Incomplete city yield roster");
    return s;
}
namespace {
template<class Arithmetic>std::int64_t base_production_whole(const CityYieldComponents& components){
    auto base=Arithmetic::add(components.terrain,components.buildings);
    base=Arithmetic::add(base,components.specialists); base=Arithmetic::add(base,components.misc); base=Arithmetic::add(base,components.religion);
    return base;
}
template<class Arithmetic>std::int64_t production_difference_base(const CityYieldState& city,const CityYieldComponents& components){
    auto base=Arithmetic::multiply_divide(base_production_whole<Arithmetic>(components),100,1);
    return Arithmetic::add(base,Arithmetic::multiply_divide(components.per_population,city.population,1));
}
template<class Arithmetic>std::int64_t source_rate_modifier(const CityYieldContext& s,const CityYieldRules& rules,const content::GlobalDefineCache& defines,std::int64_t player,const CityYieldState& city,const CityYieldComponents& components,std::int64_t yield,std::optional<std::int32_t> extra){
    const auto& p=s.players.at(player);
    auto mod=Arithmetic::add(components.rate_modifier,components.resource_modifier);
    Arithmetic::happiness(s,p,yield);
    if(!s.no_happiness && (yield==s.production || yield==s.gold)) {
        if(!p.excess_happiness) throw CityYieldUnavailable("BNW production/gold needs calculated empire unhappiness");
        if(*p.excess_happiness<0) {
            const auto unhappy=Arithmetic::subtract(0,*p.excess_happiness);
            const auto penalty=yield==s.production?"VERY_UNHAPPY_PRODUCTION_PENALTY_PER_UNHAPPY":"VERY_UNHAPPY_GOLD_PENALTY_PER_UNHAPPY";
            const auto cap=yield==s.production?"VERY_UNHAPPY_MAX_PRODUCTION_PENALTY":"VERY_UNHAPPY_MAX_GOLD_PENALTY";
            const auto product=Arithmetic::multiply_divide(unhappy,define(defines,penalty),1);
            const auto native_cap=Arithmetic::add(0,define(defines,cap));
            mod=Arithmetic::add(mod,std::max(product,native_cap));
        }
    }
    const auto area=s.area_modifiers.find({city.area,player});
    if(area!=s.area_modifiers.end()) mod=Arithmetic::add(mod,value(area->second,yield)); // absent CvArea pointer is skipped
    mod=Arithmetic::add(mod,value(p.modifiers,yield));
    if(city.id==p.capital) mod=Arithmetic::add(mod,value(p.capital_modifiers,yield));
    if(p.golden_age) mod=Arithmetic::add(mod,value(rules.golden_age_modifiers,yield));
    mod=Arithmetic::add(mod,religious_modifier<Arithmetic>(s,rules,city,yield));
    if(yield==s.production) {
        const auto bonus=city_state_production_modifier<Arithmetic>(s,rules,defines,city);
        if(bonus>0) mod=Arithmetic::add(mod,bonus);
    }
    if(city.puppet) {
        if(yield==s.science) mod=Arithmetic::add(mod,define(defines,"PUPPET_SCIENCE_MODIFIER"));
        // The original BNW switch intentionally has no break after SCIENCE.
        if(yield==s.science || yield==s.gold) mod=Arithmetic::add(mod,define(defines,"PUPPET_GOLD_MODIFIER"));
    }
    if(extra)mod=Arithmetic::add(mod,*extra);
    return std::max<std::int64_t>(0,Arithmetic::add(mod,100));
}
template<class Arithmetic>std::int64_t base_yield_times100(const CityYieldContext& s,const CityYieldState& city,const CityYieldComponents& components){
    std::int64_t result=0;
    auto base=base_production_whole<Arithmetic>(components);
    result=Arithmetic::multiply_divide(base,100,1);
    result=Arithmetic::add(result,Arithmetic::multiply_divide(components.per_population,city.population,1));
    std::int64_t religions=0;
    for(const auto &r:city.religions) if(r.followers>0 && r.religion>s.pantheon) religions=Arithmetic::add(religions,1);
    result=Arithmetic::add(result,Arithmetic::multiply_divide(components.per_religion,religions,1));
    return result;
}
template<class Arithmetic> CityYieldQuote evaluate(const CityYieldContext &s,const CityYieldRules &rules,const content::GlobalDefineCache &defines,
    std::int64_t player,std::int64_t city_id,std::int64_t yield,bool ignore_trade,std::set<std::int64_t> &visiting) {
    if(yield<0 || static_cast<std::uint64_t>(yield)>=s.yield_slots) throw std::out_of_range("City yield index");
    const auto &p=s.players.at(player); const auto &city=p.cities.at(city_id);
    CityYieldQuote q;
    if(s.profile==TechnologyProfile::vp542) {
        return vp_city_cached_yield_quote(s,player,city_id,yield,ignore_trade);
    }
    if((city.resistance || city.razing) && (yield==s.gold || yield==s.science)) {q.suppressed=true;return q;}
    if(!visiting.insert(yield).second) throw std::runtime_error("Cyclic city production-to-yield rule");
    const auto found=city.yields.find(yield);
    const auto components=found==city.yields.end()?CityYieldComponents{}:found->second;
    Arithmetic::process(components.process_modifier);
    if(components.process_modifier)
        q.process_times100=Arithmetic::multiply_divide(evaluate<Arithmetic>(s,rules,defines,player,city_id,s.production,false,visiting).total_times100,components.process_modifier,100);
    q.base_times100=base_yield_times100<Arithmetic>(s,city,components);
    q.modifier=source_rate_modifier<Arithmetic>(s,rules,defines,player,city,components,yield,std::nullopt);
    q.total_times100=Arithmetic::add(Arithmetic::multiply_divide(q.base_times100,q.modifier,100),q.process_times100);
    if(!ignore_trade) {q.trade_times100=trade_yield<Arithmetic>(s,city,yield);q.total_times100=Arithmetic::add(q.total_times100,q.trade_times100);}
    visiting.erase(yield);
    return q;
}
}
CityYieldQuote city_yield_times100(const CityYieldContext &s,const CityYieldRules &r,const content::GlobalDefineCache &d,
    std::int64_t player,std::int64_t city,std::int64_t yield,bool ignore_trade) {
    if(s.profile!=r.profile || d.profile()!=(s.profile==TechnologyProfile::vp542?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw))
        throw std::runtime_error("City yield profile mismatch");
    std::set<std::int64_t> visiting; return evaluate<WideYieldArithmetic>(s,r,d,player,city,yield,ignore_trade,visiting);
}

namespace detail {
std::int32_t bnw_native_food_rate_math(const CityYieldContext& s,const CityYieldRules& r,
 const content::GlobalDefineCache& d,std::int64_t player,std::int64_t city){
 if(s.profile!=TechnologyProfile::bnw||r.profile!=TechnologyProfile::bnw||
    d.profile()!=content::GlobalDefineProfile::bnw||s.yield_slots!=6||
    s.production!=1||s.gold!=2||s.science!=3||s.pantheon!=0)
  throw CityYieldUnavailable("Native FOOD rate source enum/profile mismatch");
 std::set<std::int64_t> visiting;
 return NativeFoodYieldArithmetic::integer(evaluate<NativeFoodYieldArithmetic>(s,r,d,player,city,0,false,visiting).total_times100);
}
std::int32_t bnw_native_gold_rate_prefix_math(const CityYieldContext& s,std::int64_t player,std::int64_t city){
 if(s.profile!=TechnologyProfile::bnw||s.yield_slots!=6||s.production!=1||s.gold!=2||s.science!=3||s.pantheon!=0)
  throw CityYieldUnavailable("Native GOLD prefix source enum/profile mismatch");
 const auto& c=s.players.at(player).cities.at(city);if(c.resistance||c.razing)return 0;
 const auto f=c.yields.find(2);const auto v=f==c.yields.end()?CityYieldComponents{}:f->second;
 NativeScienceYieldArithmetic::process(v.process_modifier);
 (void)base_yield_times100<NativeScienceYieldArithmetic>(s,c,v);
 return NativeScienceYieldArithmetic::integer(NativeScienceYieldArithmetic::add(v.rate_modifier,v.resource_modifier));
}
std::int32_t bnw_native_gold_rate_math(const CityYieldContext& s,const CityYieldRules& r,
 const content::GlobalDefineCache& d,std::int64_t player,std::int64_t city){
 if(s.profile!=TechnologyProfile::bnw||r.profile!=TechnologyProfile::bnw||d.profile()!=content::GlobalDefineProfile::bnw||s.yield_slots!=6||s.production!=1||s.gold!=2||s.science!=3||s.pantheon!=0)
  throw CityYieldUnavailable("Native GOLD rate source enum/profile mismatch");
 std::set<std::int64_t> visiting;
 return NativeScienceYieldArithmetic::integer(evaluate<NativeScienceYieldArithmetic>(s,r,d,player,city,2,false,visiting).total_times100);
}

std::int32_t bnw_native_science_rate_prefix_math(const CityYieldContext& s,std::int64_t player,std::int64_t city){
 if(s.profile!=TechnologyProfile::bnw||s.yield_slots!=6||s.production!=1||s.gold!=2||s.science!=3||s.pantheon!=0)
  throw CityYieldUnavailable("Native SCIENCE prefix source enum/profile mismatch");
 const auto& c=s.players.at(player).cities.at(city);if(c.resistance||c.razing)return 0;
 const auto f=c.yields.find(3);const auto v=f==c.yields.end()?CityYieldComponents{}:f->second;
 NativeScienceYieldArithmetic::process(v.process_modifier);
 (void)base_yield_times100<NativeScienceYieldArithmetic>(s,c,v);
 return NativeScienceYieldArithmetic::integer(NativeScienceYieldArithmetic::add(v.rate_modifier,v.resource_modifier));
}
std::vector<std::int32_t> bnw_native_building_counts_reset_read_math(
 const saves::CityProductionRecord& rows,std::uint64_t slots,const content::InfoTypeCatalog& types){
 const auto require=[](bool b,const char* text){if(!b)throw CityYieldUnavailable(text);};
 require(slots<=INT32_MAX&&types.report().all_succeeded(),"Native Buildings Reset source span/catalog unavailable");
 std::vector<std::int32_t> dense(static_cast<std::size_t>(slots),0); // actual Init/Reset includes holes
 const auto wire=NativeScienceYieldArithmetic::integer(named(rows.fields,"wire_count").value);
 require(rows.start_offset<=rows.end_offset&&rows.children.size()==static_cast<std::size_t>(std::max<std::int32_t>(0,wire)),"Native Buildings Read wire count unavailable");
 for(std::size_t i=0;i<rows.children.size();++i){const auto& row=rows.children[i];const auto hash=named(row.fields,"hash").value;
  require(row.index==i&&row.start_offset>=rows.start_offset&&row.end_offset<=rows.end_offset&&row.start_offset<=row.end_offset&&hash>=0&&hash<=UINT32_MAX,"Native Buildings Read row unavailable");
  if(hash==0){require(row.arrays.empty(),"Native Buildings null hash carried value");continue;}
  const auto& values=named(row.arrays,"values");require(values.columns==1&&values.values.size()==1,"Native Buildings nonnull hash value unavailable");
  const auto n=NativeScienceYieldArithmetic::integer(values.values.front()); // native i32 is consumed even for unknown hash
  const auto* info=types.find_hash(static_cast<std::uint64_t>(hash));if(!info){continue;}
  const auto* actual=types.find(info->type);require(info->table=="Buildings"&&info->id>=0&&static_cast<std::uint64_t>(info->id)<slots&&actual&&actual->table==info->table&&actual->id==info->id&&actual->hash==hash&&content::saved_info_type_hash(info->type)==hash,"Native Buildings global hash remap/collision unavailable");
  dense.at(static_cast<std::size_t>(info->id))=n; // wire order; duplicate last assignment
 }
 return dense;
}
std::int32_t bnw_native_production_rate_prefix_math(const CityYieldContext& s,std::int64_t player,std::int64_t city){
 if(s.profile!=TechnologyProfile::bnw||s.yield_slots!=6||s.production!=1||s.gold!=2||s.science!=3||s.pantheon!=0)throw CityYieldUnavailable("Native PRODUCTION prefix enum/profile mismatch");
 const auto& c=s.players.at(player).cities.at(city);const auto f=c.yields.find(1);const auto v=f==c.yields.end()?CityYieldComponents{}:f->second;
 if(NativeScienceYieldArithmetic::integer(v.process_modifier)!=0)throw CityYieldUnavailable("Native PRODUCTION recursive self-process unavailable before base");
 (void)base_yield_times100<NativeScienceYieldArithmetic>(s,c,v);return NativeScienceYieldArithmetic::integer(NativeScienceYieldArithmetic::add(v.rate_modifier,v.resource_modifier));
}
std::int32_t bnw_native_production_rate_math(const CityYieldContext& s,const CityYieldRules& r,const content::GlobalDefineCache& d,std::int64_t player,std::int64_t city){
 if(s.profile!=TechnologyProfile::bnw||r.profile!=TechnologyProfile::bnw||d.profile()!=content::GlobalDefineProfile::bnw||s.yield_slots!=6||s.production!=1||s.gold!=2||s.science!=3||s.pantheon!=0)throw CityYieldUnavailable("Native PRODUCTION rate enum/profile mismatch");
 std::set<std::int64_t> visiting;return NativeScienceYieldArithmetic::integer(evaluate<NativeScienceYieldArithmetic>(s,r,d,player,city,1,false,visiting).total_times100);
}
std::int32_t bnw_native_current_production_difference_prefix_math(const CityYieldContext& s,std::int64_t owner,std::int64_t id){
 if(s.profile!=TechnologyProfile::bnw||s.yield_slots!=6||s.production!=1)throw CityYieldUnavailable("Native current production difference enum/profile mismatch");
 const auto& c=s.players.at(owner).cities.at(id);const auto found=c.yields.find(1);const auto v=found==c.yields.end()?CityYieldComponents{}:found->second;
 (void)production_difference_base<NativeScienceYieldArithmetic>(c,v);
 return NativeScienceYieldArithmetic::integer(NativeScienceYieldArithmetic::add(v.rate_modifier,v.resource_modifier));
}
std::int32_t bnw_native_current_production_difference_math(const CityYieldContext& s,const CityYieldRules& r,const content::GlobalDefineCache& d,std::int64_t owner,std::int64_t id,std::int32_t order_modifier,std::int32_t overflow){
 if(s.profile!=TechnologyProfile::bnw||r.profile!=TechnologyProfile::bnw||d.profile()!=content::GlobalDefineProfile::bnw||s.yield_slots!=6||s.production!=1)throw CityYieldUnavailable("Native current production difference enum/profile mismatch");
 const auto& c=s.players.at(owner).cities.at(id);const auto found=c.yields.find(1);const auto v=found==c.yields.end()?CityYieldComponents{}:found->second;
 const auto base=production_difference_base<NativeScienceYieldArithmetic>(c,v);
 const auto mod=source_rate_modifier<NativeScienceYieldArithmetic>(s,r,d,owner,c,v,1,order_modifier);
 auto total=NativeScienceYieldArithmetic::multiply_divide(base,mod,100);
 total=NativeScienceYieldArithmetic::add(total,overflow);
 total=NativeScienceYieldArithmetic::add(total,0); // PROJECT food=false, native *100 of zero
 total=NativeScienceYieldArithmetic::add(total,trade_yield<NativeScienceYieldArithmetic>(s,c,1));
 return NativeScienceYieldArithmetic::integer(total);
}
std::int32_t bnw_native_science_rate_math(const CityYieldContext& s,const CityYieldRules& r,
 const content::GlobalDefineCache& d,std::int64_t player,std::int64_t city){
 if(s.profile!=TechnologyProfile::bnw||r.profile!=TechnologyProfile::bnw||d.profile()!=content::GlobalDefineProfile::bnw||s.yield_slots!=6||s.production!=1||s.gold!=2||s.science!=3||s.pantheon!=0)
  throw CityYieldUnavailable("Native SCIENCE rate source enum/profile mismatch");
 std::set<std::int64_t> visiting;
 return NativeScienceYieldArithmetic::integer(evaluate<NativeScienceYieldArithmetic>(s,r,d,player,city,3,false,visiting).total_times100);
}

}

std::int64_t science_from_cities_times100(const CityYieldContext &s,const CityYieldRules &r,const content::GlobalDefineCache &d,std::int64_t player) {
    std::int64_t total=0;
    for(const auto &[id,city]:s.players.at(player).cities) total=wide::add(total,city_yield_times100(s,r,d,player,id,s.science).total_times100);
    return total;
}
} // namespace civ5::runtime
