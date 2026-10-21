#include "civ5/runtime/bnw_city_current_trade_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_happiness_buildings.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/team_technologies.hpp"
#include "civ5/runtime/city_yields.hpp"
#include <bit>
#include <set>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <string_view>
#include <thread>
#include <type_traits>
#include <variant>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
class PositiveRemainingFrontier final:public std::exception {};
void need(bool b,const char* m){if(!b)throw BnwCityTradeStrategyUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Trade city strategy reached signed32 operand unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityTradeStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityTradeStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Trade city strategy duplicate source field");out=&r;}
 need(out,"Trade city strategy required source field absent");return *out;
}
bool same_info(const content::InfoTypeEntry* a,const content::InfoTypeEntry* b){return a&&b&&a->table==b->table&&a->type==b->type&&a->id==b->id&&a->hash==b->hash;}
std::int32_t city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=named(c.core.fields,name);
 need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
  f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Trade city strategy city native scalar shape unavailable");
 return native(f.values.front());
}
}
std::int32_t detail::bnw_trade_remaining_math(std::int64_t capacity,std::int64_t used){need(used>=0,"Trade city strategy native used count negative");const auto a=static_cast<std::uint32_t>(native(capacity)),b=static_cast<std::uint32_t>(native(used));return std::bit_cast<std::int32_t>(a-b);}
std::int32_t detail::bnw_trade_international_count_math(std::span<const saves::TradeRouteObservation> raw,
 std::span<const saves::TradeRouteObservation> mirrors,std::int64_t x,std::int64_t y,BnwTradeCityStrategy trigger){
 need(trigger==BnwTradeCityStrategy::international_origin||trigger==BnwTradeCityStrategy::international_destination,"Trade city strategy count outside origin/destination scope");
 const auto nx=native(x),ny=native(y);need(raw.size()==mirrors.size()&&raw.size()<=UINT32_MAX,"Trade city strategy route count mirror absent");std::int32_t count=0;
 for(std::size_t index=0;index<raw.size();++index){const auto& r=raw[index];const auto& mirror=mirrors[index];const auto read=[&](auto member){const auto n=native(r.*member);need(mirror.*member==n,"Trade city strategy reached raw/owned route scalar differs");return n;};
  if(read(&saves::TradeRouteObservation::origin_x)==-1&&read(&saves::TradeRouteObservation::origin_y)==-1&&read(&saves::TradeRouteObservation::destination_x)==-1&&read(&saves::TradeRouteObservation::destination_y)==-1)continue;
  const bool origin=trigger==BnwTradeCityStrategy::international_origin;
  if(read(origin?&saves::TradeRouteObservation::origin_x:&saves::TradeRouteObservation::destination_x)==nx&&read(origin?&saves::TradeRouteObservation::origin_y:&saves::TradeRouteObservation::destination_y)==ny&&read(&saves::TradeRouteObservation::connection_type)==0)count=native(std::int64_t(count)+1);
 }
 return count;
}
struct BnwCityCurrentTradeStrategy::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;
 std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;
 std::int32_t owner=-1,city=-1;
 std::pair<std::uint64_t,std::uint64_t> city_span;
 mutable std::atomic<bool> retired{false};
};
BnwCityCurrentTradeStrategy::BnwCityCurrentTradeStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentTradeStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Trade city strategy foreign constructor thread/moved query");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Trade city strategy genuine rules/host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&
   p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,
   "Trade city strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode(); // actual owning thread/lifecycle BEFORE facade state/source
  need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&
   p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,
   "Trade city strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwCityCurrentTradeStrategy::value(BnwTradeCityStrategy trigger)const{return typed([&]{
 require_current();need(trigger>=BnwTradeCityStrategy::need_land&&trigger<=BnwTradeCityStrategy::international_destination,"Trade city strategy outside six-source scope");const auto& p=*impl_;
 try{
 const auto& s=*p.state;const auto* city=resolve_bnw_city_registry(s.world,s.cities,p.owner,p.city);need(city&&city_scalar(*city,"m_iID")==p.city&&city_scalar(*city,"m_eOwner")==p.owner&&std::pair{city->core.start_offset,city->core.end_offset}==p.city_span,"Trade city strategy canonical city/span differs");
 const auto& owned=s.cities.players.at(p.owner).cities.at(p.city);
 const auto player=[&]()->const saves::SavedPlayerObservation&{require_current();need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&s.world.all_players->players.size()==64,"Trade city strategy actual native roster absent");const auto& q=s.world.all_players->players.at(static_cast<std::size_t>(p.owner));const auto& f=named(q.core.fields,"m_eID");need(q.index==static_cast<std::uint64_t>(p.owner)&&q.saved_player_id==p.owner&&q.core.layout==bnw&&f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&native(f.values.front())==p.owner,"Trade city strategy actual player differs");return q;};
 if(trigger==BnwTradeCityStrategy::international_origin||trigger==BnwTradeCityStrategy::international_destination){
  const auto x=city_scalar(*city,"m_iX"),y=city_scalar(*city,"m_iY");need(owned.x==x&&owned.y==y,"Trade city strategy raw/owned coordinates differ");const auto& g=s.world.trade;const auto& original=p.source->snapshot.trade;
  need(g.layout==bnw&&g.version&&*g.version<=3&&g.version==original.version&&g.dimensions.major_slots==22&&g.dimensions.yield_slots==6&&g.start_offset<g.routes_end_offset&&g.routes_end_offset<=g.end_offset&&g.start_offset==original.start_offset&&g.end_offset==original.end_offset&&g.routes_end_offset==original.routes_end_offset&&g.routes.size()<=UINT32_MAX&&s.cities.profile==content::TechnologyProfile::bnw&&s.cities.trade_routes.size()==g.routes.size(),"Trade city strategy actual global route collection absent/stale");
  const auto count=detail::bnw_trade_international_count_math(g.routes,s.cities.trade_routes,x,y,trigger);
  require_current();return count>=2; // SDK counts the full vector; no stop at2.
 }
 const auto& actor=player();
 const auto cities=[&](){std::vector<const saves::SavedCityObservation*> result;std::int32_t previous=-1;for(const auto& raw:actor.cities.cities){const auto id=city_scalar(raw,"m_iID"),slot=id&8191;need(slot>previous&&resolve_bnw_city_registry(s.world,s.cities,p.owner,id)==&raw,"Trade city strategy native first/next city slot order differs");previous=slot;result.push_back(&raw);}return result;};
 // The actual SDK returns uint capacity. Native signed intermediates are
 // checked before conversion; Remaining preserves the uint subtraction bits.
 const auto capacity=[&](){
  require_current();need(s.world.pregame&&s.world.pregame->layout==bnw&&s.cities.profile==content::TechnologyProfile::bnw,"Trade city strategy actual civilization owner absent");const auto& civil=s.world.pregame->slots.integer_arrays.at("civilizations");need(civil.size()==64,"Trade city strategy civilization native roster differs");const auto civ=native(civil.at(static_cast<std::size_t>(p.owner)));need(civ==s.cities.players.at(p.owner).civilization,"Trade city strategy raw/owned civilization differs");if(civ==-1)return std::int32_t{0};
  need(civ>=0,"Trade city strategy native CivilizationInfo unavailable");const auto& rules=p.host->technologies();need(rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded()&&rules.collection.slots<=INT32_MAX,"Trade city strategy actual native TechInfo collection absent");
  need(s.world.pregame->archive.integer_arrays.at("team_types").size()==64&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64,"Trade city strategy actual team roster absent");const auto team=native(s.world.pregame->archive.integer_arrays.at("team_types").at(static_cast<std::size_t>(p.owner)));need(team>=0&&team<64&&s.setup.players.at(p.owner).team==team,"Trade city strategy raw/owned team differs");const auto& rawteam=s.world.teams->teams.at(static_cast<std::size_t>(team));need(rawteam.index==static_cast<std::uint64_t>(team),"Trade city strategy raw native team identity differs");
  std::set<std::int32_t> source_has_tech_slots;for(const auto hash:named(rawteam.arrays,"tech_hashes").values){need(hash>=0&&hash<=UINT32_MAX,"Trade city strategy raw native Tech hash invalid");const auto* t=hash?p.host->types().find_hash(static_cast<std::uint64_t>(hash)):nullptr;need(!t||(t->table=="Technologies"&&t->id>=0&&static_cast<std::uint64_t>(t->id)<rules.collection.slots),"Trade city strategy cross-table/outside Tech hash unsupported");if(t)source_has_tech_slots.insert(native(t->id));}
  const auto decoded=restore_team_technologies(rawteam,rules,p.host->types());need(native(named(rawteam.fields,"id").value)==team&&decoded.team()==team,"Trade city strategy actual TeamTech identity differs");const TeamTechnologies* live=nullptr;for(const auto& t:s.technologies)if(t.team()==team){need(!live,"Trade city strategy duplicate owned team");live=&t;}need(live&&live->profile()==content::TechnologyProfile::bnw&&live->slots()==rules.collection.slots,"Trade city strategy actual owned TechInfo span differs");std::int32_t total=0;
  for(std::uint64_t slot=0;slot<rules.collection.slots;++slot){const auto id=static_cast<std::int32_t>(slot);need(source_has_tech_slots.contains(id),"Trade city strategy reached HasTech slot lacks actual raw Read coverage; native Team Reset owner required");const bool known=decoded.has_tech(id);need(live->has_tech(id)==known,"Trade city strategy reached raw/owned HasTech differs");if(!known)continue;const auto* t=rules.find(slot);if(!t)continue;const auto* type=p.host->types().find(t->base.type);need(type&&type->table=="Technologies"&&type->id==id&&same_info(type,p.host->types().find_hash(type->hash))&&t->base.authored_id==id,"Trade city strategy reached native TechInfo identity differs");const auto term=native(t->integers.at("InternationalTradeRoutesChange"));total=native(std::int64_t(total)+term);}
  // Repository source loaders/restorers reused: they decode hashed dense
  // counts and native civilization replacements. Nothing is published.
  const auto actualcities=cities();if(!actualcities.empty()){
  require_current();const auto yrules=load_city_yield_rules(*p.database,content::TechnologyProfile::bnw);need(yrules.building_slots<=INT32_MAX,"Trade city strategy native building collection span unsupported");require_current();
  // Guard hash family/count wire before the existing generic restorer remaps.
  // It traverses all cities, so these provenance checks cover that same closure.
  for(const auto& actor_source:s.world.all_players->players)for(const auto& c:actor_source.cities.cities){need(c.production.layout==bnw,"Trade city strategy native building count schema absent");const auto& block=named(c.production.blocks,"buildings");const auto version=named(block.fields,"version").value;need(version>=0&&version<=1,"Trade city strategy building source version unsupported");
   for(const auto* name:{"m_paiNumRealBuilding","m_paiNumFreeBuilding"}){const auto& array=named(block.children,name);const auto wire=named(array.fields,"wire_count").value;need(wire>=0&&wire<=INT32_MAX&&static_cast<std::uint64_t>(wire)==array.children.size(),"Trade city strategy building count wire cardinality differs");
    for(const auto& row:array.children){const auto hash=named(row.fields,"hash").value;need(hash>=0&&hash<=UINT32_MAX,"Trade city strategy building hash native width unavailable");if(!hash){need(row.arrays.empty(),"Trade city strategy null building hash has payload");continue;}const auto& values=named(row.arrays,"values").values;need(values.size()==1,"Trade city strategy source building count invalid");const auto* type=p.host->types().find_hash(static_cast<std::uint64_t>(hash));need(!type||(type->table=="Buildings"&&type->id>=0&&static_cast<std::uint64_t>(type->id)<yrules.building_slots),"Trade city strategy cross-table/native Building hash unsupported");if(type)need(native(values.front())>=0,"Trade city strategy reached building count negative");}
   }
  }
  require_current();const auto building=BnwBuildingHappinessRules::load(*p.database);auto setup=s.setup;const auto sourcecounts=restore_city_yields(s.world,setup,*p.database,yrules,p.host->types());require_current();need(building.rules_sha256()==p.identity.rules_sha256&&building.replacements().building_class_slots<=INT32_MAX&&building.replacements().building_slots<=INT32_MAX,"Trade city strategy actual building rules differ");const auto& mapping=building.replacements().civilization_buildings.at(civ);
  for(const auto* raw:actualcities){const auto id=city_scalar(*raw,"m_iID");const auto& current=s.cities.players.at(p.owner).cities.at(id);const auto& reconstructed=sourcecounts.players.at(p.owner).cities.at(id);need(current.building_counts_resolved&&reconstructed.building_counts_resolved&&current.real_buildings==reconstructed.real_buildings&&current.free_buildings==reconstructed.free_buildings,"Trade city strategy actual raw/owned building counts differ");
   for(std::uint64_t klass=0;klass<building.replacements().building_class_slots;++klass){const auto k=static_cast<std::int32_t>(klass);const auto it=mapping.find(k);const auto bid=it==mapping.end()?std::int64_t{-1}:it->second;if(bid==-1)continue;const auto nativeid=native(bid);need(nativeid>=0&&static_cast<std::uint64_t>(nativeid)<building.replacements().building_slots,"Trade city strategy native mapped Building index unsupported");
    std::optional<std::string> metadata;const auto info=p.database->query_rows({"SELECT ID,Type FROM Buildings WHERE ID=?",{bid}},[&](const auto& row){need(!metadata&&row.has_column("ID")&&row.has_column("Type")&&row.integer("ID")==bid,"Trade city strategy native BuildingInfo metadata differs");const auto name=row.text("Type");const auto* type=name?p.host->types().find(*name):nullptr;need(type&&type->table=="Buildings"&&type->id==bid&&same_info(type,p.host->types().find_hash(type->hash)),"Trade city strategy mapped Building Type/hash identity differs");metadata=*name;},2);require_current();need(info.all_succeeded(),"Trade city strategy BuildingInfo query unavailable");if(!metadata){need(info.rows==0,"Trade city strategy mapped BuildingInfo duplicate/hole unsupported");continue;}
    const auto get=[&](const YieldValues& counts){const auto v=counts.find(bid);return v==counts.end()?std::int32_t{0}:native(v->second);};const auto n=building.building_count(get(current.real_buildings),get(current.free_buildings));if(n==0)continue;
    // Native bonus getter is reached ONLY after nonzero GetNumBuilding.
    std::optional<std::int32_t> bonus;const auto report=p.database->query_rows({"SELECT ID,Type,NumTradeRouteBonus FROM Buildings WHERE ID=?",{bid}},[&](const auto& row){need(!bonus&&row.has_column("ID")&&row.has_column("Type")&&row.has_column("NumTradeRouteBonus")&&row.integer("ID")==bid&&row.text("Type")==metadata,"Trade city strategy reached BuildingInfo bonus identity differs");bonus=native(row.integer("NumTradeRouteBonus"));},2);require_current();need(report.all_succeeded()&&report.rows==1&&bonus,"Trade city strategy reached BuildingInfo bonus unavailable");if(*bonus!=0)total=native(std::int64_t(total)+*bonus);
   }
  }
  }
  const auto& trait=named(actor.traits.fields,"m_iNumTradeRoutesModifier");need(actor.traits.layout==bnw&&actor.traits.version&&*actor.traits.version>=6&&*actor.traits.version<=19&&trait.encoding==saves::PlayerTraitsEncoding::i32&&trait.values.size()==1,"Trade city strategy actual trait route modifier absent (legacy Reset producer required)");const auto modifier=native(std::int64_t{100}+native(trait.values.front()));total=native(std::int64_t(total)*modifier);return static_cast<std::int32_t>(total/100);
 };
 const auto used=[&](){
  require_current();const auto& units=actor.units;need(units.layout==bnw&&units.header.layout==bnw&&units.header.source_slots<=8192&&units.units.size()<=8192&&units.header.wire_unit_count==static_cast<std::int64_t>(units.units.size())&&units.header.next_free_indices.size()==units.header.source_slots&&s.maintenance.players.at(p.owner).units.size()==units.units.size(),"Trade city strategy native unit FLTA shape unavailable");const auto num_slots=native(named(units.header.fields,"num_slots").value);need(num_slots>=0&&units.header.source_slots==(num_slots?std::bit_ceil(static_cast<std::uint64_t>(num_slots)):0),"Trade city strategy native unit FLTA capacity differs");const auto last=native(named(units.header.fields,"last_index").value),free=native(named(units.header.fields,"free_list_count").value);need(last>=-1&&free>=0&&std::int64_t(last)+1-free==units.header.wire_unit_count&&last<static_cast<std::int64_t>(units.header.source_slots),"Trade city strategy native unit FLTA count stale");std::int32_t count=0,previous=-1;
  for(const auto& unit:units.units){const auto scalar=[&](const char* name){const auto& f=named(unit.core.fields,name);need(unit.core.layout==bnw&&f.encoding==saves::UnitCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty(),"Trade city strategy native unit scalar unavailable");return native(f.values.front());};const auto id=scalar("m_iID"),slot=id&8191;need(id>=0&&slot>previous&&slot<=last&&scalar("m_eOwner")==p.owner,"Trade city strategy native unit FLTA order/owner differs");previous=slot;const auto& hashfield=named(unit.core.fields,"idUnitType");need(hashfield.encoding==saves::UnitCoreEncoding::hash32&&hashfield.values.size()==1&&hashfield.values.front()>=0&&hashfield.values.front()<=UINT32_MAX,"Trade city strategy native UnitInfo hash unavailable");const auto* type=p.host->types().find_hash(static_cast<std::uint64_t>(hashfield.values.front()));need(type&&type->table=="Units"&&type->id>=0&&type->id<=INT32_MAX&&same_info(type,p.host->types().find(type->type)),"Trade city strategy actual UnitInfo absent");const auto& mirror=s.maintenance.players.at(p.owner).units.at(id);need(s.maintenance.profile==content::TechnologyProfile::bnw&&mirror.type==type->id&&mirror.source_core==std::optional{std::pair{unit.core.start_offset,unit.core.end_offset}},"Trade city strategy raw/owned UnitInfo/source differs");(void)bnw_unit_order_plot(s,{p.owner,id});
   std::optional<bool> trade;const auto report=p.database->query_rows({"SELECT ID,Type,Trade FROM Units WHERE ID=?",{type->id}},[&](const auto& row){need(!trade&&row.has_column("ID")&&row.has_column("Type")&&row.has_column("Trade")&&row.integer("ID")==type->id&&row.text("Type")==type->type,"Trade city strategy actual UnitInfo metadata differs");trade=row.boolean("Trade");},2);require_current();need(report.all_succeeded()&&report.rows==1&&trade.has_value(),"Trade city strategy actual UnitInfo.IsTrade unavailable");if(*trade)count=native(std::int64_t(count)+1);
  }
  for(const auto* raw:cities()){const auto id=city_scalar(*raw,"m_iID");need(raw->production.layout==bnw&&raw->production.context.bnw_city_version,"Trade city strategy actual BNW queue schema absent");const auto& q=named(raw->production.blocks,"queue");const auto n=named(q.fields,"wire_count").value;const auto& orders=s.cities.players.at(p.owner).cities.at(id).production_orders;need(n>=0&&n<=UINT32_MAX&&static_cast<std::uint64_t>(n)==q.children.size()&&orders&&orders->size()==q.children.size(),"Trade city strategy raw/owned full queue count differs");
   std::int32_t queued=0;for(std::size_t index=0;index<q.children.size();++index){const auto& row=q.children.at(index);const auto kind=native(named(row.fields,"eOrderType").value);need(row.index==index&&orders->at(index).type==kind,"Trade city strategy reached raw/owned queue kind differs");if(kind!=0)continue;const auto ai=native(named(row.fields,"iData2").value);need(orders->at(index).secondary==ai,"Trade city strategy reached raw/owned queue AI differs");const auto* type=p.host->types().find("UNITAI_TRADE_UNIT");need(type&&type->table=="UnitAIInfos"&&type->id==38&&same_info(type,p.host->types().find_hash(type->hash)),"Trade city strategy native trade UnitAI enum/Info differs");if(ai==38)queued=native(std::int64_t(queued)+1);}
   count=native(std::int64_t(count)+queued); // SDK per-city sum THEN outer used sum.
  }
  return count;
 };
 const auto possible=capacity();require_current();const auto occupied=used();require_current();const auto remaining=detail::bnw_trade_remaining_math(possible,occupied);
 const bool no_need=trigger==BnwTradeCityStrategy::no_need_land||trigger==BnwTradeCityStrategy::no_need_sea;if(remaining<=0){require_current();return no_need;}
 throw PositiveRemainingFrontier{};
 }catch(const PositiveRemainingFrontier&){throw BnwCityTradeStrategyUnavailable("Trade city strategy positive remaining reaches CvPlayerTrade::GetNumPotentialConnections: actual CanCreateTradeRoute/IsValidTradeRoutePath owner unavailable");}
 catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentTradeStrategy inspect_bnw_city_current_trade_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Trade city strategy actual selected host absent");
 (void)host->session().bnw_game_debug_mode(); // actual thread/owner BEFORE DB or world
 auto p=std::make_shared<BnwCityCurrentTradeStrategy::Impl>();p->host=std::move(host);
 p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());
 p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Trade city strategy actual immutable rules scope absent");
 p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&
  p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Trade city strategy actual host rules identity differs");
 p->state=p->session->read_state_lease();need(bool(p->state),"Trade city strategy actual state absent");
 p->source=&p->session->source();p->generation=p->session->state_generation();
 need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Trade city strategy actual source payload differs");
 p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Trade city strategy native owner outside BNW roster");
 const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));
 need(raw,"Trade city strategy actual current city absent");p->city=city_scalar(*raw,"m_iID");
 need(city_scalar(*raw,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Trade city strategy canonical owner/source span differs");
 p->city_span={raw->core.start_offset,raw->core.end_offset};
 BnwCityCurrentTradeStrategy result(std::move(p));result.require_current();return result;
});}
}
