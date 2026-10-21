#include "civ5/runtime/bnw_city_current_food_rate.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include <algorithm>
#include <climits>
#include <optional>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw BnwCityCurrentFoodRateUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Current FOOD rate native operand width unavailable");return static_cast<std::int32_t>(n);}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* found=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!found,"Current FOOD rate duplicate raw field");found=&r;}
 need(found,"Current FOOD rate raw field absent");return *found;
}
std::int32_t scalar(const auto& rows,std::string_view name){const auto& f=named(rows,name);need(f.values.size()==1,"Current FOOD rate raw scalar shape unavailable");return native(f.values[0]);}
void city_vector(const saves::SavedCityObservation& city,std::string_view name){
 const auto& f=named(city.core.fields,name);
 need(f.encoding==saves::CityCoreEncoding::vector_i32&&f.wire_count==6&&f.values.size()==6&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty()&&f.row_keys.empty(),"Current FOOD rate raw city six-vector unavailable");
 for(const auto n:f.values)(void)native(n);
}
void player_vector(const saves::SavedPlayerObservation& player,std::string_view name){
 const auto& f=named(player.core.fields,name);
 need(f.encoding==saves::PlayerCoreEncoding::vector_i32&&f.wire_count==6&&f.values.size()==6&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"Current FOOD rate raw player six-vector unavailable");
 for(const auto n:f.values)(void)native(n);
}
std::int64_t sparse(const YieldValues& v,std::int64_t y){const auto i=v.find(y);return i==v.end()?0:i->second;}
// A length-delimited operand seal, not a saved final result or fresh flag.
using Seal=std::vector<std::int64_t>;
void put(Seal& seal,std::int64_t n){seal.push_back(native(n));}
void count(Seal& seal,std::size_t n){need(n<=INT32_MAX,"Current FOOD rate collection outside native width");put(seal,static_cast<std::int64_t>(n));}
void span(Seal& seal,std::uint64_t begin,std::uint64_t end){
 need(begin<=INT64_MAX&&end<=INT64_MAX&&begin<=end,"Current FOOD rate source span unavailable");
 seal.push_back(static_cast<std::int64_t>(begin));seal.push_back(static_cast<std::int64_t>(end));
}
Seal context_inputs(const CityYieldContext& s,std::int32_t owner,std::int32_t id){
 need(s.profile==content::TechnologyProfile::bnw&&s.yield_slots==6&&s.production==1&&s.gold==2&&s.science==3&&s.pantheon==0,"Current FOOD rate source yield enums unavailable");
 const auto& p=s.players.at(owner);const auto& c=p.cities.at(id);Seal seal;
 for(auto n:{c.owner,c.id,c.x,c.y,c.area,c.population,p.capital})put(seal,n);
 put(seal,c.resistance);put(seal,c.razing);put(seal,c.puppet);put(seal,p.golden_age);put(seal,s.no_happiness);
 need(c.source_core.has_value(),"Current FOOD rate owned source city span missing");span(seal,c.source_core->first,c.source_core->second);
 const auto f=c.yields.find(0);const auto v=f==c.yields.end()?CityYieldComponents{}:f->second;
 for(auto n:{v.terrain,v.buildings,v.specialists,v.misc,v.religion,v.per_population,v.per_religion,v.rate_modifier,v.resource_modifier,v.process_modifier})put(seal,n);
 put(seal,sparse(p.modifiers,0));put(seal,sparse(p.capital_modifiers,0));
 count(seal,c.religions.size());for(const auto& r:c.religions){put(seal,r.religion);put(seal,r.followers);put(seal,r.pressure);}
 // The actual global religion roster/order decides the first matched source.
 count(seal,s.religions.size());for(const auto& r:s.religions){put(seal,r.religion);put(seal,r.founder);count(seal,r.beliefs.size());for(const auto b:r.beliefs)put(seal,b);}
 // Full roster/order, including empty records and all six native yield cells.
 count(seal,s.trade_routes.size());for(const auto& r:s.trade_routes){
  put(seal,r.id.has_value());if(r.id)put(seal,*r.id);
  need(r.profile_extensions.values.empty(),"Current FOOD rate unsupported trade profile fields");
  for(auto n:{r.origin_x,r.origin_y,r.destination_x,r.destination_y,r.origin_owner,r.destination_owner,r.domain,r.connection_type,r.unit_location_index,r.unit_id,r.circuits_completed,r.circuits_to_complete,r.turn_route_complete})put(seal,n);
  put(seal,r.moving_forward);count(seal,r.plots.size());for(const auto& t:r.plots){put(seal,t.x);put(seal,t.y);}
  need(r.origin_yields.size()==6&&r.destination_yields.size()==6,"Current FOOD rate trade native six-vector unavailable");
  for(auto n:r.origin_yields)put(seal,n);for(auto n:r.destination_yields)put(seal,n);
 }
 count(seal,s.area_modifiers.size());for(const auto& [key,values]:s.area_modifiers){put(seal,key.first);put(seal,key.second);for(std::int64_t y=0;y<6;++y)put(seal,sparse(values,y));}
 return seal;
}
struct Captured {Seal seal;std::int32_t value=0;};
Captured capture(const LiveSessionState& s,const content::RulesDatabase& db,
 const content::InfoTypeCatalog& types,const CityYieldRules& rules,
 const content::GlobalDefineCache& defines,const BnwUnhappinessRules& unhappiness_rules,std::int32_t owner,std::int32_t handle,const BnwCitySpecialistCounts& specialist_source){
 specialist_source.require_same_world_database(s.world,db);
 const auto* city=resolve_bnw_city_registry(s.world,s.cities,owner,handle);need(city,"Current FOOD rate actual city absent");
 const auto id=scalar(city->core.fields,"m_iID");
 need(city->tail.layout==bnw,"Current FOOD rate raw city religion layout unavailable");
 for(const auto name:{"m_iID","m_eOwner","m_iX","m_iY","m_iPopulation","m_iResistanceTurns","m_iRazingTurns"}){
  const auto& f=named(city->core.fields,name);
  need(f.encoding==saves::CityCoreEncoding::i32&&!f.wire_count&&f.values.size()==1&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty()&&f.row_keys.empty(),"Current FOOD rate raw city scalar encoding unavailable");
 }
 const auto& puppet=named(city->core.fields,"m_bPuppet");
 need(puppet.encoding==saves::CityCoreEncoding::boolean&&!puppet.wire_count&&puppet.values.size()==1&&(puppet.values[0]==0||puppet.values[0]==1),"Current FOOD rate raw puppet bool unavailable");
 // Source reads this modifier before every other yield-rate operand. Do not
 // turn a reached recursive PRODUCTION branch into a successful zero quote.
 city_vector(*city,"m_aiProductionToYieldModifier");
 const auto process=named(city->core.fields,"m_aiProductionToYieldModifier").values[0];
 const auto& owned=s.cities.players.at(owner).cities.at(id);const auto found=owned.yields.find(0);
 need(process==(found==owned.yields.end()?0:found->second.process_modifier),"Current FOOD rate process raw/owned stale");
 need(process==0,"Current FOOD rate nonzero process requires production recursion owner");
 for(const auto name:{"m_aiBaseYieldRateFromTerrain","m_aiBaseYieldRateFromBuildings","m_aiBaseYieldRateFromSpecialists","m_aiBaseYieldRateFromMisc","m_aiBaseYieldRateFromReligion","m_aiYieldPerPop","m_aiYieldRateModifier","m_aiResourceYieldRateModifier"})city_vector(*city,name);
 if(!city->core.version||*city->core.version>=4)city_vector(*city,"m_aiYieldPerReligion");
 const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(owner));
 need(player.index==static_cast<std::uint64_t>(owner)&&scalar(player.core.fields,"m_eID")==owner,"Current FOOD rate source player slot identity stale");
 for(const auto name:{"m_iCapitalCityID","m_iGoldenAgeTurns"}){
  const auto& f=named(player.core.fields,name);
  need(f.encoding==saves::PlayerCoreEncoding::i32&&!f.wire_count&&f.values.size()==1&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"Current FOOD rate player scalar encoding unavailable");
 }
 player_vector(player,"m_aiYieldRateModifier");player_vector(player,"m_aiCapitalYieldRateModifier");
 need(s.world.trade.layout==bnw&&s.world.trade.dimensions.major_slots==22&&s.world.trade.dimensions.yield_slots==6&&s.world.trade.version&&*s.world.trade.version<=3&&s.world.trade.profile_extensions.values.empty(),"Current FOOD rate actual trade source dimensions unavailable");
 need(s.world.trade.start_offset<s.world.trade.routes_end_offset&&s.world.trade.routes_end_offset==s.world.trade.matrices_offset&&s.world.trade.matrices_offset<s.world.trade.end_offset,"Current FOOD rate full trade roster spans unavailable");
 for(const auto& route:s.world.trade.routes)need(route.id.has_value()==(*s.world.trade.version>=1)&&(*s.world.trade.version>=2||route.turn_route_complete==0),"Current FOOD rate source trade Read version/default unavailable");
 need(s.world.map_regions&&s.world.map_regions->dimensions&&s.world.map_plots&&s.world.map_plots->layout==bnw,"Current FOOD rate raw map/area source unavailable");
 const auto& dims=*s.world.map_regions->dimensions;
 need(dims.fixed_players==80&&dims.fixed_teams==80&&dims.players==64&&dims.yields==6,"Current FOOD rate native area dimensions unavailable");
 need(s.world.religion_culture.layout==bnw,"Current FOOD rate actual global religion layout unavailable");
 for(const auto& religion:s.world.religion_culture.religions)for(const auto hash:religion.beliefs.belief_hashes){
  need(hash<=UINT32_MAX,"Current FOOD rate native belief hash outside width");
  const auto* type=hash?types.find_hash(hash):nullptr;
  need(!type||type->table=="Beliefs","Current FOOD rate belief hash has incompatible Info identity");
 }
 const auto x=native(owned.x),y=native(owned.y);
 need(x==scalar(city->core.fields,"m_iX")&&y==scalar(city->core.fields,"m_iY")&&x>=0&&y>=0,"Current FOOD rate source city coordinates stale");
 const auto index=s.world.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
 const auto& plot=s.world.map_plots->plots.at(static_cast<std::size_t>(index));
 need(plot.index==index&&plot.x==static_cast<std::uint64_t>(x)&&plot.y==static_cast<std::uint64_t>(y),"Current FOOD rate city home plot identity stale");
 const auto area_handle=native(named(plot.fields,"area").value);
 need(area_handle==native(owned.area),"Current FOOD rate source home area handle stale");
 // Validate the actual complete registry BEFORE the existing reconstruction
 // reads its records. INVALID_INDEX has the native NULL lookup semantics.
 const auto* area=resolve_bnw_area_registry(s.world,area_handle);
 validate_bnw_area_registry(s.world); // whole-context reconstruction iterates all records
 // Reuse the existing source reconstruction and formula. This local context is
 // read-only math; it is not published as refreshed gameplay state.
 auto setup=restore_research_context(s.world,db);
 need(setup.game_type==s.setup.game_type&&setup.game_handicap==s.setup.game_handicap&&setup.pitboss==s.setup.pitboss&&setup.players.size()==s.setup.players.size(),"Current FOOD rate raw/owned happiness setup stale");
 for(const auto& [pid,p]:setup.players){const auto& live=s.setup.players.at(pid);
  need(p.team==live.team&&p.handicap==live.handicap&&p.slot_status==live.slot_status&&p.slot_claim==live.slot_claim&&p.alive==live.alive,"Current FOOD rate raw/owned player human/team/handicap stale");
 }
 auto raw=restore_city_yields(s.world,setup,db,rules,types);
 auto expected=context_inputs(raw,owner,id);need(expected==context_inputs(s.cities,owner,id),"Current FOOD rate raw/owned reached operands stale");
 Captured out;out.seal=std::move(expected);span(out.seal,player.core.start_offset,player.core.end_offset);
 span(out.seal,city->core.start_offset,city->core.end_offset);span(out.seal,s.world.trade.start_offset,s.world.trade.end_offset);
 span(out.seal,s.world.trade.routes_end_offset,s.world.trade.matrices_offset);
 put(out.seal,s.world.trade.version.has_value());if(s.world.trade.version)put(out.seal,static_cast<std::int64_t>(*s.world.trade.version));
 const auto& statuses=named(named(city->tail.blocks,"religions").children,"m_ReligionStatus");span(out.seal,statuses.start_offset,statuses.end_offset);
 need(statuses.wire_count&&static_cast<std::uint64_t>(std::max<std::int32_t>(0,native(*statuses.wire_count)))==statuses.children.size(),"Current FOOD rate actual city religion row count unavailable");
 put(out.seal,*statuses.wire_count);
 for(const auto& f:city->core.fields)if(f.name){const auto n=std::string_view(f.name);
  if(n=="m_iID"||n=="m_eOwner"||n=="m_iX"||n=="m_iY"||n=="m_iPopulation"||n=="m_iResistanceTurns"||n=="m_iRazingTurns"||n=="m_bPuppet"||n=="m_aiBaseYieldRateFromTerrain"||n=="m_aiBaseYieldRateFromBuildings"||n=="m_aiBaseYieldRateFromSpecialists"||n=="m_aiBaseYieldRateFromMisc"||n=="m_aiBaseYieldRateFromReligion"||n=="m_aiYieldPerPop"||n=="m_aiYieldPerReligion"||n=="m_aiYieldRateModifier"||n=="m_aiResourceYieldRateModifier"||n=="m_aiProductionToYieldModifier")span(out.seal,f.start_offset,f.end_offset);
 }
 for(const auto& f:player.core.fields)if(f.name){const auto n=std::string_view(f.name);if(n=="m_iCapitalCityID"||n=="m_iGoldenAgeTurns"||n=="m_aiYieldRateModifier"||n=="m_aiCapitalYieldRateModifier")span(out.seal,f.start_offset,f.end_offset);}
 span(out.seal,s.world.religion_culture.start_offset,s.world.religion_culture.culture_offset);
 count(out.seal,s.world.religion_culture.religions.size());
 for(const auto& religion:s.world.religion_culture.religions){count(out.seal,religion.beliefs.belief_hashes.size());for(const auto h:religion.beliefs.belief_hashes)out.seal.push_back(static_cast<std::int64_t>(h));}
 const auto capital_handle=scalar(player.core.fields,"m_iCapitalCityID");
 const auto* capital=resolve_bnw_city_registry(s.world,s.cities,owner,capital_handle);
 raw.players.at(owner).capital=capital?scalar(capital->core.fields,"m_iID"):-1;
 if(!raw.no_happiness){
  detail::require_bnw_current_unhappiness_inputs(s,unhappiness_rules,types,owner);
  const auto quote=detail::bnw_native_unhappiness_math_with_specialists(s.unhappiness,setup,raw,unhappiness_rules,defines,owner,specialist_source);
  need(s.cities.players.at(owner).excess_happiness&&*s.cities.players.at(owner).excess_happiness==quote.excess_happiness,"Current FOOD rate existing excess happiness mirror stale");
  raw.players.at(owner).excess_happiness=quote.excess_happiness;
  const auto& u=s.unhappiness.players.at(owner);
  put(out.seal,s.unhappiness.world_cities_percent);put(out.seal,setup.game_type);put(out.seal,setup.game_handicap);put(out.seal,setup.pitboss.has_value());if(setup.pitboss)put(out.seal,*setup.pitboss);
  count(out.seal,setup.players.size());for(const auto& [pid,p]:setup.players){put(out.seal,pid);for(auto n:{p.team,p.handicap,p.slot_status,p.slot_claim})put(out.seal,n);put(out.seal,p.alive);}
  for(auto n:{u.happiness,u.units,u.units_modifier,u.population_modifier,u.cities_modifier,u.occupied_modifier,u.capital_modifier,u.trait_cities_modifier,u.trait_population_modifier,u.public_opinion})put(out.seal,n);
  put(out.seal,u.half_specialists);count(out.seal,u.cities.size());
  for(const auto& [cid,c]:u.cities){put(out.seal,cid);put(out.seal,c.occupied);put(out.seal,c.no_occupied_unhappiness);put(out.seal,c.ignore);
   const auto& live=s.cities.players.at(owner).cities.at(cid);const auto& from_raw=raw.players.at(owner).cities.at(cid);
   need(live.population==from_raw.population&&live.puppet==from_raw.puppet,"Current FOOD rate full empire raw/owned population stale");
   put(out.seal,live.population);put(out.seal,live.puppet);need(live.source_core.has_value(),"Current FOOD rate empire city core source absent");span(out.seal,live.source_core->first,live.source_core->second);
  }
  for(auto n:{quote.normal_cities_times100,quote.captured_cities_times100,quote.normal_population_times100,quote.occupied_population_times100,quote.units_times100,quote.public_opinion,quote.before_ai,quote.total,quote.excess_happiness})put(out.seal,n);
 }
 span(out.seal,s.world.map_header.start_offset,s.world.map_header.end_offset);span(out.seal,plot.start_offset,plot.end_offset);
 for(const auto b:s.world.map_header.guid)put(out.seal,b);
 need(s.world.map_header.grid.width<=INT32_MAX&&s.world.map_header.grid.height<=INT32_MAX,"Current FOOD rate native grid width unavailable");
 put(out.seal,static_cast<std::int64_t>(s.world.map_header.grid.width));put(out.seal,static_cast<std::int64_t>(s.world.map_header.grid.height));put(out.seal,s.world.map_header.wrap_x);put(out.seal,s.world.map_header.wrap_y);
 raw.players.at(owner).cities.at(id).area=area?native(named(area->fields,"id").value):-1;
 if(area_handle!=-1){
  const auto& areas=named(s.world.map_regions->containers,"areas");span(out.seal,areas.start_offset,areas.end_offset);
  for(auto n:areas.legacy_metadata)put(out.seal,n);count(out.seal,areas.next_free.size());for(auto n:areas.next_free)put(out.seal,n);
  count(out.seal,areas.records.size());for(const auto& a:areas.records){put(out.seal,named(a.fields,"id").value);span(out.seal,a.start_offset,a.end_offset);}
 }
 out.value=detail::bnw_native_food_rate_math(raw,rules,defines,owner,id);
 const auto dense_seal=specialist_source.reached_source_seal();count(out.seal,dense_seal.size());out.seal.insert(out.seal.end(),dense_seal.begin(),dense_seal.end());return out;
}
template<class F>auto typed(F&& f)->decltype(f()){try{return f();}catch(const BnwCityCurrentFoodRateUnavailable&){throw;}catch(const std::exception& e){throw BnwCityCurrentFoodRateUnavailable(e.what());}}
}
struct BnwCityFoodRateRules::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::string sha;
 std::shared_ptr<const content::RulesDatabase> database;std::function<bool()> current;
 content::InfoTypeCatalog types;CityYieldRules rules;content::GlobalDefineCache defines;BnwUnhappinessRules unhappiness;
};
BnwCityFoodRateRules::BnwCityFoodRateRules(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityFoodRateRules::require_current()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Current FOOD rate rules foreign thread/moved owner");
 need(p->current&&p->current(),"Current FOOD rate SAME immutable rules retired");
}
BnwCityFoodRateRules BnwCityFoodRateRules::load(std::shared_ptr<const content::RulesDatabase> db,std::string_view sha){return typed([&]{
 need(bool(db),"Current FOOD rate genuine DB owner absent");auto p=std::make_shared<Impl>();p->database=std::move(db);
 p->current=p->database->immutable_preview_fingerprint_currentness();need(p->current&&p->current(),"Current FOOD rate requires SAME immutable activated rules");
 p->sha=p->database->gameplay_fingerprint_sha256();need(!sha.empty()&&p->sha==sha,"Current FOOD rate rules identity mismatch");
 p->types=p->database->cache_info_types();need(p->types.report().all_succeeded(),"Current FOOD rate actual Info types unavailable");
 p->defines=p->database->cache_global_defines(content::GlobalDefineProfile::bnw);
 p->rules=load_city_yield_rules(*p->database,content::TechnologyProfile::bnw);
 p->unhappiness=load_bnw_unhappiness_rules(*p->database);
 constexpr const char* names[]={"YIELD_FOOD","YIELD_PRODUCTION","YIELD_GOLD","YIELD_SCIENCE","YIELD_CULTURE","YIELD_FAITH"};
 const auto report=p->database->prefetch_info("Yields",[&](auto slot,auto authored,const auto& row){
  const auto type=row.text("Type");
  need(slot<6&&authored==static_cast<std::int64_t>(slot)&&type&&*type==names[slot],"Current FOOD rate actual source YieldInfo identity unavailable");
  const auto* info=p->types.find(names[slot]);need(info&&info->table=="Yields"&&info->id==static_cast<std::int64_t>(slot),"Current FOOD rate YieldInfo catalog identity differs");
  need(sparse(p->rules.golden_age_modifiers,static_cast<std::int64_t>(slot))==native(row.integer("GoldenAgeYieldMod")),"Current FOOD rate SAME actual YieldInfo differs");return true;
 });
 need(report.all_succeeded()&&!report.holes&&!report.overlapping_ids&&report.slots==6,"Current FOOD rate native YieldInfo collection unavailable");
 need(p->current(),"Current FOOD rate rules retired while loading");return BnwCityFoodRateRules(std::move(p));
});}
struct BnwCityCurrentFoodRate::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const LiveSession> session;std::shared_ptr<const BnwCityFoodRateRules> rules;
 std::shared_ptr<const LiveSessionState> state;std::shared_ptr<const saves::SavedSession> source;
 std::uint64_t generation=0;SessionCheckpointIdentity identity;std::int32_t owner=-1,handle=-1;
 std::pair<std::uint64_t,std::uint64_t> core_span;
 Captured inputs;bool numeric_captured=false;
};
BnwCityCurrentFoodRate::BnwCityCurrentFoodRate(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentFoodRate::bind_initial(){
 bind_metadata();capture_initial();
}
void BnwCityCurrentFoodRate::bind_metadata(){
 bind_source_metadata();require_current();
}
void BnwCityCurrentFoodRate::bind_source_metadata(){
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->session&&p->rules,"Current FOOD rate owner/thread absent");
 // Constructor-owned session thread is checked before any private identity,
 // source() or world borrow, including first attachment and moved-from facade.
 need(p->session->plot_yield_refresh_thread_==std::this_thread::get_id(),"Current FOOD rate foreign session thread");
 p->rules->require_current();need(p->session->bnw_game_debug_owner_current(),"Current FOOD rate genuine import owner unavailable");
 p->identity=p->session->debug_->identity;need(p->identity.rules_sha256==p->rules->impl_->sha,"Current FOOD rate SAME session/rules identity mismatch");
 p->state=p->session->state_;p->source=p->session->source_;p->generation=p->session->state_generation_;
 // Metadata only: no FOOD base/rate, consumption, process, modifiers or
 // trade read. The closed difference consumer MUST read its actual head
 // predicate before it calls capture_initial on this source-bound child.
 const auto* raw=resolve_bnw_current_city(*p->state,p->owner,p->handle);
 need(raw,"Current FOOD rate canonical current city absent");
 const auto& id=named(raw->core.fields,"m_iID");
 need(id.encoding==saves::CityCoreEncoding::i32&&id.values.size()==1&&!id.wire_count&&id.bytes.empty()&&id.hashes.empty()&&id.row_counts.empty()&&id.row_keys.empty(),"Current FOOD rate canonical ID shape unavailable");
 p->handle=native(id.values.front());p->core_span={raw->core.start_offset,raw->core.end_offset};
 const auto& owned=p->state->cities.players.at(p->owner).cities.at(p->handle);
 need(owned.owner==p->owner&&owned.id==p->handle&&owned.source_core==std::optional{p->core_span},"Current FOOD rate canonical city mirror stale");
 require_metadata_current();
}
void BnwCityCurrentFoodRate::capture_initial(){
 require_current();const auto p=impl_;need(!p->numeric_captured,"Current FOOD rate initial numeric capture replay");
 auto specialists=BnwCitySpecialistCounts::from_same_rate(*this);
 auto inputs=capture(*p->state,*p->rules->impl_->database,p->rules->impl_->types,p->rules->impl_->rules,p->rules->impl_->defines,p->rules->impl_->unhappiness,p->owner,p->handle,specialists);
 require_current();p->inputs=std::move(inputs);p->numeric_captured=true;
}
void BnwCityCurrentFoodRate::require_current()const{
 // Every real FOOD capture/read must retain the computed happiness guard.
 require_metadata_current();const auto p=impl_;
 need(p->session->bnw_unhappiness_current_,"Current FOOD rate genuine unhappiness owner retired");
}
void BnwCityCurrentFoodRate::require_metadata_current()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->session&&p->rules,"Current FOOD rate foreign thread/moved result");
 need(p->session->plot_yield_refresh_thread_==std::this_thread::get_id(),"Current FOOD rate foreign session thread");
 p->rules->require_current();
 need(p->session->bnw_game_debug_owner_current()&&p->session->state_==p->state&&p->session->source_==p->source&&p->session->state_generation_==p->generation,"Current FOOD rate import allocation/generation retired");
 need(p->session->debug_->identity.payload_sha256==p->identity.payload_sha256&&p->session->debug_->identity.rules_sha256==p->identity.rules_sha256&&p->rules->impl_->sha==p->identity.rules_sha256,"Current FOOD rate source identity retired");
 const auto* city=resolve_bnw_current_city(*p->state,p->owner,p->handle);
 need(city&&std::pair{city->core.start_offset,city->core.end_offset}==p->core_span&&scalar(city->core.fields,"m_iID")==p->handle,"Current FOOD rate canonical city identity retired");
}
std::int32_t BnwCityCurrentFoodRate::value_times100()const{return typed([&]{
 require_current();const auto p=impl_;need(p->numeric_captured,"Current FOOD rate numeric source not yet reached");
 auto specialists=BnwCitySpecialistCounts::from_same_rate(*this);
 const auto now=capture(*p->state,*p->rules->impl_->database,p->rules->impl_->types,p->rules->impl_->rules,p->rules->impl_->defines,p->rules->impl_->unhappiness,p->owner,p->handle,specialists);
 require_current();need(now.seal==p->inputs.seal&&now.value==p->inputs.value,"Current FOOD rate retained source operands stale");return now.value;
});}
BnwCityCurrentFoodRate BnwCityCurrentFoodRate::bind_source_only(std::shared_ptr<const LiveSession> session,std::shared_ptr<const BnwCityFoodRateRules> rules,std::int64_t owner,std::int64_t city){return typed([&]{
 auto p=std::make_shared<BnwCityCurrentFoodRate::Impl>();p->session=std::move(session);p->rules=std::move(rules);p->owner=native(owner);p->handle=native(city);
 BnwCityCurrentFoodRate out(std::move(p));out.bind_metadata();return out;
});}

BnwCityCurrentFoodRate BnwCityCurrentFoodRate::bind_specialist_source_only(std::shared_ptr<const LiveSession> session,std::shared_ptr<const BnwCityFoodRateRules> rules,std::int64_t owner,std::int64_t city){return typed([&]{
 auto p=std::make_shared<BnwCityCurrentFoodRate::Impl>();p->session=std::move(session);p->rules=std::move(rules);p->owner=native(owner);p->handle=native(city);
 BnwCityCurrentFoodRate out(std::move(p));out.bind_source_metadata();return out;
});}
const LiveSessionState& BnwCityCurrentFoodRate::current_source_state()const{require_metadata_current();return *impl_->state;}
std::shared_ptr<const content::RulesDatabase> BnwCityCurrentFoodRate::current_source_database_owner()const{require_metadata_current();return impl_->rules->impl_->database;}
std::string BnwCityCurrentFoodRate::current_rules_sha()const{require_metadata_current();return impl_->identity.rules_sha256;}
const content::InfoTypeCatalog& BnwCityCurrentFoodRate::current_source_info_types()const{require_metadata_current();return impl_->rules->impl_->types;}
const content::GlobalDefineCache& BnwCityCurrentFoodRate::current_source_defines()const{require_metadata_current();return impl_->rules->impl_->defines;}
std::int32_t BnwCityCurrentFoodRate::canonical_owner()const{require_metadata_current();return impl_->owner;}
std::int32_t BnwCityCurrentFoodRate::canonical_city()const{require_metadata_current();return impl_->handle;}
std::int32_t BnwCityCurrentFoodRate::canonical_core_scalar(std::string_view name)const{
 require_metadata_current();const auto* city=resolve_bnw_current_city(*impl_->state,impl_->owner,impl_->handle);need(city,"Current FOOD rate canonical city scalar source absent");const auto& f=named(city->core.fields,name);
 need(f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty()&&f.row_keys.empty(),"Current FOOD rate canonical scalar shape unavailable");return native(f.values.front());
}
BnwCityCurrentFoodRate inspect_bnw_city_current_food_rate(std::shared_ptr<const LiveSession> session,std::shared_ptr<const BnwCityFoodRateRules> rules,std::int64_t owner,std::int64_t city){return typed([&]{
 auto p=std::make_shared<BnwCityCurrentFoodRate::Impl>();p->session=std::move(session);p->rules=std::move(rules);p->owner=native(owner);p->handle=native(city);
 BnwCityCurrentFoodRate out(std::move(p));out.bind_initial();return out;
});}
}
