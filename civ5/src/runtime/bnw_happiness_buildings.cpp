#include "civ5/runtime/bnw_happiness_buildings.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_policy_adoption.hpp"
#include "civ5/runtime/bnw_minor_city_strength.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t value) {
    need(value>=INT32_MIN && value<=INT32_MAX,"BNW building happiness exceeds native int");return static_cast<std::int32_t>(value);
}
std::int32_t add(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t multiply(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)*b);}
const auto &named(const auto &rows,std::string_view name) {
    const typename std::remove_reference_t<decltype(rows)>::value_type *found=nullptr;
    for(const auto &row:rows)if(row.name && name==row.name) {need(!found,"Duplicate BNW building happiness field");found=&row;}
    need(found,"Missing BNW building happiness field");return *found;
}
std::int32_t scalar(const auto &rows,std::string_view name) {
    const auto &v=named(rows,name).values;need(v.size()==1,"Malformed BNW building happiness scalar");return integer(v[0]);
}
std::int32_t value(const YieldValues &rows,std::int64_t id) {
    const auto it=rows.find(id);return it==rows.end()?0:integer(it->second);
}
YieldValues counts(const saves::CityProductionRecord &buildings,const char *name,const BnwBuildingHappinessRules &rules) {
    const auto &array=named(buildings.children,name);
    const auto wire=named(array.fields,"wire_count").value;
    need(wire>=0 && static_cast<std::uint64_t>(wire)==array.children.size(),"BNW building count cardinality differs");
    YieldValues result;
    for(const auto &row:array.children) {
        const auto hash=named(row.fields,"hash").value;
        need(hash>=0 && hash<=UINT32_MAX,"BNW building hash exceeds native width");
        if(!hash) {need(row.arrays.empty(),"BNW null building hash consumed payload");continue;}
        const auto &v=named(row.arrays,"values").values;need(v.size()==1,"BNW building count payload malformed");
        const auto count=integer(v[0]);need(count>=0,"BNW building count negative");
        const auto *type=rules.resolve(hash);
        need(type && type->table=="Buildings" && rules.building_modifiers().contains(type->id),"BNW building identity unresolved");
        result[type->id]=count;
    }
    return result;
}
std::int32_t majority(const saves::SavedCityObservation &raw,const CityYieldState &live) {
    const auto &block=named(raw.tail.blocks,"religions");
    need(scalar(block.children,"version")>=0 && scalar(block.children,"version")<=3,"BNW local religion block version unknown");
    const auto &religions=named(block.children,"m_ReligionStatus");
    need(religions.wire_count && *religions.wire_count>=0 && static_cast<std::uint64_t>(*religions.wire_count)==religions.children.size() &&
         religions.children.size()==live.religions.size(),"BNW local religious status cardinality differs");
    std::int32_t total=0,most=-1,pressure=0,religion=-1;
    for(std::size_t i=0;i<religions.children.size();++i) {
        const auto &row=religions.children[i];const auto &current=live.religions[i];
        need(scalar(row.children,"version")>=0 && scalar(row.children,"version")<=2,"BNW local religion row version unknown");
        const auto id=scalar(row.children,"m_eReligion"),followers=scalar(row.children,"m_iFollowers"),p=scalar(row.children,"m_iPressure");
        need(id==current.religion && followers==current.followers && p==current.pressure && followers>=0 && p>=0,
             "BNW local religious status differs from current city owner");
        total=add(total,followers);
        if(followers>most || (followers==most && p>pressure)) {most=followers;pressure=p;religion=id;}
    }
    return multiply(most,2)>=total?religion:-1;
}
template<class M,class C>
BnwLocalCityHappinessTerm local_term(const auto& raw,
 const saves::SavedCityObservation& saved,const CityYieldState& city,
 const BnwBuildingHappinessRules& rules,const std::vector<bool>& owned,
 const std::vector<bool>& blocked,const M& mapped,const C& count_of,bool native_order,const std::function<bool(std::int32_t)>* native_garrison=nullptr) {
    BnwLocalCityHappinessTerm term;term.city=scalar(saved.core.fields,"m_iID");
        const auto &cache=named(saved.production.blocks,"before_strategy");
        term.base=integer(named(cache.fields,"m_iBaseHappinessFromBuildings").value);
        if(!native_order)term.unmodded=integer(named(cache.fields,"m_iUnmoddedHappinessFromBuildings").value);
        std::int32_t garrison_happiness=0;
        if(scalar(raw.core.fields,"m_iHappinessPerGarrisonedUnitCount")>0){
         need(native_garrison,"BNW local happiness requires actual garrison defender owner");
         if((*native_garrison)(integer(term.city)))garrison_happiness=1; // SDK adds ONE before religion/policy/cap.
        }
        need(majority(saved,city)==-1,"BNW local happiness requires secondary pantheon and local religion effects owner");
        for(std::size_t slot=0;slot<owned.size();++slot)if(owned[slot]) {
            const auto &p=rules.policies().policies().at(static_cast<std::int64_t>(slot));
            if(p.blocked_branch>=0 && blocked.at(static_cast<std::size_t>(p.blocked_branch)))continue;
            const auto &mods=rules.policy_modifiers().at(static_cast<std::int64_t>(slot));
            for(auto klass:rules.classes()) {const auto building=mapped(klass);
                if(building!=-1 && count_of(building)>0)term.policy=add(term.policy,value(mods,klass));
            }
        }
        std::optional<std::int32_t> subtotal;if(native_order)subtotal=add(add(term.base,garrison_happiness),term.policy);
        term.cap=integer(city.population);
        if(scalar(raw.traits.fields,"m_iCityUnhappinessModifier")!=0)term.cap=add(multiply(term.cap,20),15)/30;
        term.total=std::min(term.cap,subtotal?*subtotal:add(add(term.base,garrison_happiness),term.policy));return term;
}
template<class M,class C>
void class_terms(BnwBuildingHappinessQuote& q,const BnwBuildingHappinessRules& rules,
 const M& mapped,const C& count_of){
    for(auto klass:rules.classes()) {
        const auto generator=mapped(klass);if(generator==-1 || count_of(generator)<=0)continue;
        const auto &mods=rules.building_modifiers().at(generator);
        // The source inner loop traverses ALL native class slots, including holes.
        for(std::uint64_t slot=0;slot<rules.replacements().building_class_slots;++slot) {
            const auto per=value(mods,static_cast<std::int64_t>(slot));if(per<=0)continue;
            const auto target=mapped(static_cast<std::int64_t>(slot));if(target==-1)continue;
            const auto count=count_of(target),total=multiply(per,count);
            q.class_terms.push_back({generator,target,count,per,total});q.global_classes=add(q.global_classes,total);
        }
    }
}

}
BnwBuildingHappinessRules BnwBuildingHappinessRules::load(const content::RulesDatabase &db) {
    BnwBuildingHappinessRules rules;rules.policies_=BnwPolicyHappinessRules::load(db);rules.sha256_=rules.policies_.rules_sha256();
    rules.types_=db.cache_info_types();need(rules.types_.report().all_succeeded(),"BNW building happiness type cache failed");
    const auto classes=db.prefetch_info("BuildingClasses",[&](auto slot,auto,const auto &) {
        need(slot<=INT32_MAX && rules.classes_.insert(static_cast<std::int64_t>(slot)).second,"BNW native building class slot invalid");return true;
    });
    need(classes.all_succeeded() && classes.slots<=INT32_MAX,"BNW building class rules failed");rules.replacements_.building_class_slots=classes.slots;
    const auto matrix=[&](const char *table,const char *owner_column,const std::string &type,YieldValues &result) {
        const auto report=db.query_rows({std::string("SELECT BuildingClasses.ID, Happiness FROM ")+table+
            " INNER JOIN BuildingClasses ON BuildingClasses.Type=BuildingClassType WHERE "+owner_column+"=?",{type}},[&](const auto &row) {
            const auto id=row.integer_at(0);need(rules.classes_.contains(id),"BNW building happiness class reference invalid");
            result[id]=integer(row.integer_at(1)); // source PopulateArrayByValue assignment: last row wins
        });need(report.all_succeeded(),"BNW building happiness matrix query failed");
    };
    const auto buildings=db.prefetch_info("Buildings",[&](auto slot,auto,const auto &row) {
        need(slot<=INT32_MAX,"BNW native building slot invalid");
        matrix("Building_BuildingClassHappiness","BuildingType",row.text("Type").value_or(""),rules.buildings_[static_cast<std::int64_t>(slot)]);return true;
    });need(buildings.all_succeeded() && buildings.slots<=INT32_MAX,"BNW building happiness Info failed");rules.replacements_.building_slots=buildings.slots;
    const auto policies=db.prefetch_info("Policies",[&](auto slot,auto,const auto &row) {
        matrix("Policy_BuildingClassHappiness","PolicyType",row.text("Type").value_or(""),rules.policy_[static_cast<std::int64_t>(slot)]);return true;
    });need(policies.all_succeeded(),"BNW policy building happiness Info failed");
    load_city_building_replacements(db,rules.replacements_);
    const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);const auto *d=defines.find("CITY_MAX_NUM_BUILDINGS");
    need(d && d->lookup.found && d->lookup.error.empty() && std::holds_alternative<std::int64_t>(d->lookup.value),"BNW CITY_MAX_NUM_BUILDINGS unknown");
    rules.city_max_=integer(std::get<std::int64_t>(d->lookup.value));return rules;
}
std::int32_t BnwBuildingHappinessRules::building_count(std::int32_t real,std::int32_t free) const {
    need(real>=0 && free>=0,"BNW negative live building count");return city_max_<=1?std::max(real,free):add(real,free);
}
BnwBuildingHappinessQuote quote_bnw_happiness_from_buildings(const LiveSessionState &state,const BnwBuildingHappinessRules &rules,
    const SessionCheckpointIdentity &identity,std::int64_t player) {
    need(identity.payload_sha256.size()==64 && identity.rules_sha256==rules.rules_sha256(),"BNW building happiness selected rules differ");
    need(player>=0 && player<64 && state.setup.profile==content::TechnologyProfile::bnw && state.cities.profile==content::TechnologyProfile::bnw &&
         state.setup.players.size()==64 && state.cities.players.size()==64 && state.world.all_players && state.world.all_players->layout==bnw &&
         state.world.all_players->players.size()==64 && state.world.pregame && state.world.pregame->layout==bnw,"BNW building happiness registries incomplete");
    const auto &raw=state.world.all_players->players.at(static_cast<std::size_t>(player));const auto &cities=state.cities.players.at(player);
    need(raw.index==static_cast<std::uint64_t>(player) && raw.saved_player_id==player && raw.core.layout==bnw && raw.cities.layout==bnw &&
         raw.traits.layout==bnw && scalar(raw.core.fields,"m_eID")==player,"BNW building happiness player identity differs");
    const auto civ=state.world.pregame->slots.integer_arrays.at("civilizations").at(static_cast<std::size_t>(player));
    need(civ==cities.civilization && rules.replacements().civilization_buildings.contains(civ),"BNW building happiness civilization binding unknown");
    const auto &mapping=rules.replacements().civilization_buildings.at(civ);
    const auto mapped=[&](std::int64_t klass) {
        const auto it=mapping.find(klass);if(it==mapping.end())return std::int64_t{-1};
        need(it->second==-1 || rules.building_modifiers().contains(it->second),"BNW civilization building replacement unresolved");return it->second;
    };
    // This also validates the SAME shared policy membership and source free-opener blocking.
    const auto policy_quote=quote_bnw_happiness_from_policies(state,rules.policies(),identity,player);
    const auto &block=named(raw.strategy.blocks,"policies");const auto owned=rules.policies().flags(named(block.hashes,"has_policy"),false);
    const auto blocked=rules.policies().flags(named(block.hashes,"branch_blocked"),true);
    need(raw.cities.header.wire_city_count==static_cast<std::int64_t>(cities.cities.size()) && raw.cities.cities.size()==cities.cities.size(),
         "BNW building happiness city collection incomplete");
    BnwBuildingHappinessQuote q;YieldValues empire;std::set<std::int64_t> seen;
    for(const auto &saved:raw.cities.cities) {
        const auto id=scalar(saved.core.fields,"m_iID");need(seen.insert(id).second && cities.cities.contains(id),"BNW building happiness city ID differs");
        const auto &city=cities.cities.at(id);
        need(saved.core.layout==bnw && saved.production.layout==bnw && saved.tail.layout==bnw &&
             saved.core.version && saved.production.context.bnw_city_version==saved.core.version && saved.tail.context.bnw_city_version==saved.core.version &&
             *saved.core.version<=6 && scalar(saved.core.fields,"m_eOwner")==player && city.id==id && city.owner==player &&
             city.source_core==std::optional{std::pair{saved.core.start_offset,saved.core.end_offset}} &&
             scalar(saved.core.fields,"m_iPopulation")==city.population && city.population>=0,"BNW building happiness current city binding differs");
        const auto &buildings=named(saved.production.blocks,"buildings");
        need(named(buildings.fields,"version").value>=0 && named(buildings.fields,"version").value<=1,"BNW saved CityBuildings version unknown");
        const auto real=counts(buildings,"m_paiNumRealBuilding",rules),free=counts(buildings,"m_paiNumFreeBuilding",rules);
        need(city.building_counts_resolved && city.real_buildings==real && city.free_buildings==free,"BNW building happiness live count owner stale");
        YieldValues current;
        for(const auto &[building,mods]:rules.building_modifiers()) {
            const auto count=rules.building_count(value(real,building),value(free,building));current[building]=count;
            if(count>0)empire[building]=add(value(empire,building),count);
        }
        auto term=local_term(raw,saved,city,rules,owned,blocked,mapped,
            [&](std::int64_t building){return value(current,building);},false);
        q.local=add(q.local,term.total);
        q.global_unmodded=add(q.global_unmodded,term.unmodded);q.cities.push_back(term);
    }
    class_terms(q,rules,mapped,[&](std::int64_t building){return value(empire,building);});
    const auto divisor=scalar(raw.core.fields,"m_iHappinessPerXPolicies");if(divisor>0)q.per_policies=policy_quote.owned_policies/divisor;
    q.global=add(add(q.global_classes,q.global_unmodded),q.per_policies);return q;
}

namespace detail {
static std::int32_t reached_math(const LiveSessionState& state,
 const BnwBuildingHappinessRules& rules,const SessionCheckpointIdentity& identity,
 std::int64_t player,BnwBuildingHappinessReachedStage stage,const std::function<bool(std::int32_t)>* garrison){
 need(stage==BnwBuildingHappinessReachedStage::local_cities||stage==BnwBuildingHappinessReachedStage::global_buildings,
  "BNW building happiness reached stage unavailable");
 need(identity.payload_sha256.size()==64&&identity.rules_sha256==rules.rules_sha256(),"BNW building happiness selected rules differ");
 need(player>=0&&player<64&&state.setup.profile==content::TechnologyProfile::bnw&&state.cities.profile==content::TechnologyProfile::bnw&&
  state.setup.players.size()==64&&state.cities.players.size()==64&&state.world.all_players&&state.world.all_players->layout==bnw&&
  state.world.all_players->players.size()==64&&state.world.pregame&&state.world.pregame->layout==bnw,"BNW building happiness registries incomplete");
 const auto& raw=state.world.all_players->players.at(static_cast<std::size_t>(player));const auto& cities=state.cities.players.at(player);
 need(raw.index==static_cast<std::uint64_t>(player)&&raw.saved_player_id==player&&raw.core.layout==bnw&&raw.cities.layout==bnw&&
  scalar(raw.core.fields,"m_eID")==player,"BNW building happiness player identity differs");
 need(raw.cities.cities.size()<=INT32_MAX&&raw.cities.cities.size()==cities.cities.size()&&
  raw.cities.header.wire_city_count==static_cast<std::int64_t>(cities.cities.size()),"BNW building happiness city collection incomplete");
 BnwBuildingHappinessQuote q;
 // Native firstCity null reaches no city getter, policy or civilization query.
 if(stage==BnwBuildingHappinessReachedStage::local_cities&&raw.cities.cities.empty())return 0;
 const auto civ=state.world.pregame->slots.integer_arrays.at("civilizations").at(static_cast<std::size_t>(player));
 need(civ==cities.civilization&&rules.replacements().civilization_buildings.contains(civ),"BNW building happiness civilization binding unknown");
 const auto& mapping=rules.replacements().civilization_buildings.at(civ);
 const auto mapped=[&](std::int64_t klass){const auto it=mapping.find(klass);if(it==mapping.end())return std::int64_t{-1};
  need(it->second==-1||rules.building_modifiers().contains(it->second),"BNW civilization building replacement unresolved");return it->second;};
 // Actual membership and Reset/Read flags; no ExtraHappiness/per-city arithmetic.
 const auto policy_owned=[&](){
  need(raw.strategy.layout==bnw,"BNW building happiness policy source layout unavailable");
  const auto& block=named(raw.strategy.blocks,"policies");const auto version=named(block.fields,"version").value;
  need(version==1||version==2,"BNW building happiness policy source version unavailable");
  auto owned=rules.policies().flags(named(block.hashes,"has_policy"),false);std::set<std::int64_t> membership;
  for(std::size_t slot=0;slot<owned.size();++slot)if(owned[slot])membership.insert(static_cast<std::int64_t>(slot));
  const auto& shared=state.setup.players.at(player);need(shared.policies_resolved&&shared.policies==membership,
   "BNW building happiness current policy membership stale");return owned;
 };
 // Structural/raw-owned validation is metadata. GetNumBuilding arithmetic is
 // evaluated only for the exact reached building requested by native loops.
 const auto bind_identity=[&](const saves::SavedCityObservation& saved)->const CityYieldState&{
  const auto id=scalar(saved.core.fields,"m_iID");need(cities.cities.contains(id),"BNW building happiness city ID differs");
  const auto& city=cities.cities.at(id);
  need(saved.core.layout==bnw&&saved.production.layout==bnw&&saved.core.version&&*saved.core.version<=6&&
   saved.production.context.bnw_city_version==saved.core.version&&scalar(saved.core.fields,"m_eOwner")==player&&
   city.id==id&&city.owner==player&&city.source_core==std::optional{std::pair{saved.core.start_offset,saved.core.end_offset}},
   "BNW building happiness current city binding differs");
  return city;
 };
 const auto bind_city=[&](const saves::SavedCityObservation& saved){
  const auto& city=bind_identity(saved);
  const auto& buildings=named(saved.production.blocks,"buildings");
  need(named(buildings.fields,"version").value>=0&&named(buildings.fields,"version").value<=1,"BNW saved CityBuildings version unknown");
  auto real=counts(buildings,"m_paiNumRealBuilding",rules),free=counts(buildings,"m_paiNumFreeBuilding",rules);
  need(city.building_counts_resolved&&city.real_buildings==real&&city.free_buildings==free,"BNW building happiness live count owner stale");
  return std::pair{std::move(real),std::move(free)};
 };
 if(stage==BnwBuildingHappinessReachedStage::local_cities){
  const auto owned=policy_owned();const auto& block=named(raw.strategy.blocks,"policies");
  const auto blocked=rules.policies().flags(named(block.hashes,"branch_blocked"),true);std::set<std::int64_t> seen;
  for(const auto& saved:raw.cities.cities){const auto id=scalar(saved.core.fields,"m_iID");
   need(seen.insert(id).second,"BNW building happiness city ID differs");const auto& city=cities.cities.at(id);
   need(saved.tail.layout==bnw&&saved.tail.context.bnw_city_version==saved.core.version&&raw.traits.layout==bnw&&
    scalar(saved.core.fields,"m_iPopulation")==city.population&&city.population>=0,"BNW local happiness city/traits source differs");
   const auto [real,free]=bind_city(saved);
   auto term=local_term(raw,saved,city,rules,owned,blocked,mapped,[&](std::int64_t building){
    return rules.building_count(value(real,building),value(free,building));},true,garrison);
   q.local=add(q.local,term.total);q.cities.push_back(term);
  }
  return q.local;
 }
 const auto empire_count=[&](std::int64_t building){std::int32_t total=0;std::set<std::int64_t> seen;
  for(const auto& saved:raw.cities.cities){need(seen.insert(scalar(saved.core.fields,"m_iID")).second,"BNW building happiness city ID differs");
   const auto [real,free]=bind_city(saved);total=add(total,rules.building_count(value(real,building),value(free,building)));}
  return total;};
 // Native classes first, then ordered city unmodded cache, then owned policy count.
 class_terms(q,rules,mapped,empire_count);q.global=q.global_classes;
 std::set<std::int64_t> unmodded_seen;
 for(const auto& saved:raw.cities.cities){(void)bind_identity(saved);
  need(unmodded_seen.insert(scalar(saved.core.fields,"m_iID")).second,"BNW global happiness city ID differs");
  const auto amount=integer(named(named(saved.production.blocks,"before_strategy").fields,"m_iUnmoddedHappinessFromBuildings").value);
  q.global=add(q.global,amount);
 }
 const auto divisor=scalar(raw.core.fields,"m_iHappinessPerXPolicies");if(divisor>0){
  const auto owned=policy_owned();std::int32_t count=0;for(bool has:owned)if(has)count=add(count,1);
  q.per_policies=count/divisor;q.global=add(q.global,q.per_policies);
 }
 return q.global;
}
std::int32_t bnw_happiness_buildings_reached_math(const LiveSessionState&s,const BnwBuildingHappinessRules&r,
 const SessionCheckpointIdentity&id,std::int64_t player,BnwBuildingHappinessReachedStage stage){
 return reached_math(s,r,id,player,stage,nullptr); // Ordinary imported path remains strict.
}
std::int32_t bnw_happiness_buildings_reached_math_for_initial_unit(const LiveSessionState&s,const BnwBuildingHappinessRules&r,
 const SessionCheckpointIdentity&id,std::int64_t player,BnwBuildingHappinessReachedStage stage,const BnwNativeUnitAllocatorCore&core){
 const auto canonical=core.retain_factory_owner();core.require_initial_set_position_without_callbacks();
 const auto args=core.initial_placement_operands();
 need(args.state.get()==&s&&args.owner==player&&args.host->identity().payload_sha256==id.payload_sha256&&
  args.host->identity().rules_sha256==id.rules_sha256,"Initial happiness math must borrow SAME canonical Core world/actor/rules");
 const std::function<bool(std::int32_t)> resolver=[&](std::int32_t city){return core.initial_happiness_city_has_garrison(city);};
 const auto value=reached_math(s,r,id,player,stage,&resolver);
 core.require_initial_set_position_without_callbacks();return value;
}
}

// Only this private friend may connect the earned policy stage to the existing
// native GetGarrisonedUnit/GetBestDefender resolver. All native city arithmetic
// remains the same reached_math implementation above.
struct BnwPolicyHappinessMathAccess {
 static std::int32_t local(const LiveSessionState&s,const BnwBuildingHappinessRules&r,
  const SessionCheckpointIdentity&id,std::int64_t player,const ui::SelectedBnwPreviewHost&host,
  const std::shared_ptr<const BnwPolicyAdoptionPending>&supplied){
  const auto canonical=BnwPolicyAdoptionPending::retain_factory_owner(supplied);
  const auto held_host=canonical->require_happiness_math_source(host,s,player);
  need(held_host&&held_host.get()==&host&&id.payload_sha256==held_host->identity().payload_sha256&&
   id.rules_sha256==held_host->identity().rules_sha256&&r.rules_sha256()==id.rules_sha256,
   "Policy local happiness must borrow SAME issued Host/rules/save");
  const auto actor=integer(player);
  const std::function<bool(std::int32_t)> resolver=[&](std::int32_t city){
   (void)canonical->require_happiness_math_source(*held_host,s,player);
   const bool selected=BnwMinorCityStrengthScratch::has_same_live_policy_garrison(s,*held_host,actor,city);
   (void)canonical->require_happiness_math_source(*held_host,s,player);return selected;
  };
  const auto result=detail::reached_math(s,r,id,player,
   detail::BnwBuildingHappinessReachedStage::local_cities,&resolver);
  (void)canonical->require_happiness_math_source(*held_host,s,player);
  return result;
 }
};
namespace detail {
std::int32_t bnw_happiness_buildings_local_math_for_policy(const LiveSessionState&s,
 const BnwBuildingHappinessRules&r,const SessionCheckpointIdentity&id,std::int64_t player,
 const ui::SelectedBnwPreviewHost&host,const std::shared_ptr<const BnwPolicyAdoptionPending>&pending){
 return BnwPolicyHappinessMathAccess::local(s,r,id,player,host,pending);
}
}
}
