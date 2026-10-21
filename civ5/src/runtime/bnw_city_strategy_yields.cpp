#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <algorithm>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"City strategy operand exceeds native int");return static_cast<std::int32_t>(n);}
std::int32_t unsigned_integer(std::uint64_t n){need(n<=INT32_MAX,"City strategy operand exceeds native int");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t sub(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)-b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)*b);}
auto &named(auto &rows,std::string_view name){using Row=std::remove_reference_t<decltype(rows.front())>;Row *found=nullptr;for(auto &r:rows)if(r.name&&name==r.name){need(!found,"Duplicate city strategy source field");found=&r;}need(found,"Missing city strategy current source field");return *found;}
std::int32_t scalar(const auto &rows,std::string_view name){const auto &f=named(rows,name);need(f.values.size()==1,"City strategy scalar shape differs");return integer(f.values[0]);}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto it=v.find(id);return it==v.end()?0:it->second;}
const saves::SavedCityObservation *resolve(const LiveSessionState &s,std::int32_t owner,std::int32_t id){
 return resolve_bnw_city_registry(s.world,s.cities,owner,id);
}
std::array<std::int16_t,6> yields(const LiveSessionState &s,std::uint64_t index){
 const auto &raw=s.world.map_plots->plots.at(index);const auto &cell=s.world.map_projection->cells.at(index);const auto &a=named(raw.arrays,"yields");
 need(raw.index==index&&cell.index==index&&a.columns==1&&a.values.size()==6&&cell.cached_yields&&cell.cached_yields->size()==6,"City strategy current native yield cache unknown");std::array<std::int16_t,6> out{};
 for(std::size_t y=0;y<6;++y){need(a.values[y]>=INT16_MIN&&a.values[y]<=INT16_MAX&&cell.cached_yields->at(static_cast<std::int64_t>(y))==a.values[y],"City strategy native/projected short yield cache stale");out[y]=static_cast<std::int16_t>(a.values[y]);}return out;
}
YieldValues counts(const saves::SavedCityObservation &c,const char *name,const BnwCityStrategyYieldRules &r){
 const auto &b=named(c.production.blocks,"buildings");need(named(b.fields,"version").value==1,"City strategy native buildings cache version unknown");const auto &a=named(b.children,name);need(named(a.fields,"wire_count").value==static_cast<std::int64_t>(a.children.size()),"City strategy building hash count differs");YieldValues out;
 for(const auto &row:a.children){const auto hash=named(row.fields,"hash").value;need(hash>=0&&hash<=UINT32_MAX,"City strategy building hash width differs");if(!hash){need(row.arrays.empty(),"Null native building hash consumed payload");continue;}const auto *type=r.types().find_hash(static_cast<std::uint64_t>(hash));need(type&&type->table=="Buildings"&&r.buildings().contains(integer(type->id)),"City strategy building identity unknown");const auto &values=named(row.arrays,"values");need(values.columns==1&&values.values.size()==1&&values.values[0]>=0,"City strategy building count unknown");out[type->id]=integer(values.values[0]);}return out;
}
std::int32_t food_buildings(const saves::SavedCityObservation &raw,const CityYieldState &live,const BnwCityStrategyYieldRules &r){
 need(raw.production.layout==bnw&&live.building_counts_resolved,"City strategy current building counts unresolved");const auto real=counts(raw,"m_paiNumRealBuilding",r),free=counts(raw,"m_paiNumFreeBuilding",r);
 for(const auto *v:{&live.real_buildings,&live.free_buildings})for(const auto &[id,n]:*v)need(r.buildings().contains(integer(id))&&n>=0&&n<=INT32_MAX,"City strategy current owned building count malformed");std::int32_t total=0;
 for(const auto &[id,food]:r.buildings()){need(value(real,id)==value(live.real_buildings,id)&&value(free,id)==value(live.free_buildings,id),"City strategy native/owned building counts stale");if(r.building_count(integer(value(real,id)),integer(value(free,id)))>0)total=add(total,food);}return total;
}
std::int16_t narrow_short(std::int32_t n){const auto bits=static_cast<std::uint16_t>(n);return static_cast<std::int16_t>(bits<=INT16_MAX?bits:static_cast<std::int32_t>(bits)-65536);}
}
const saves::SavedCityObservation *resolve_bnw_current_city(
 const LiveSessionState &s,std::int32_t owner,std::int32_t id){
 if(owner>=0&&owner<64&&id!=-1)
  need(s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&
       s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64,
       "Current city lookup requires SAME owned BNW player/city registry");
 return resolve(s,owner,id);
}
BnwCityStrategyYieldRules BnwCityStrategyYieldRules::load(const content::RulesDatabase &db){
 BnwCityStrategyYieldRules r;r.sha256_=identify_bytes(db.gameplay_fingerprint_data()).sha256;r.types_=db.cache_info_types();need(r.types_.report().all_succeeded(),"City strategy source Info types incomplete");
 const char *names[]={"YIELD_FOOD","YIELD_PRODUCTION","YIELD_GOLD","YIELD_SCIENCE","YIELD_CULTURE","YIELD_FAITH"};for(std::size_t y=0;y<6;++y){const auto *type=r.types_.find(names[y]);need(type&&type->table=="Yields"&&type->id==static_cast<std::int64_t>(y),"City strategy fixed native yield slots differ");}
 const auto yields_report=db.prefetch_info("Yields",[&](auto slot,auto,const auto &row){need(slot<6,"City strategy unsupported native yield slot");if(slot==0)r.min_food_=integer(row.integer("MinCity"));return true;});need(yields_report.all_succeeded()&&yields_report.slots==6,"City strategy six-yield Info vector incomplete");
 const auto matrix=[&](const char *table,const char *column,const std::string &type,std::array<std::int32_t,6> &values){const auto report=db.query_rows({std::string("SELECT Yields.ID,Yield FROM ")+table+" INNER JOIN Yields ON Yields.Type=YieldType WHERE "+column+"=?",{type}},[&](const auto &row){const auto id=integer(row.integer_at(0));need(id>=0&&id<6,"City strategy rule yield reference invalid");values[static_cast<std::size_t>(id)]=integer(row.integer_at(1));});need(report.all_succeeded(),"City strategy source SetYields matrix failed");};
 const auto buildings=db.prefetch_info("Buildings",[&](auto slot,auto,const auto &row){std::array<std::int32_t,6> values{};matrix("Building_YieldChanges","BuildingType",row.text("Type").value_or(""),values);r.building_food_[unsigned_integer(slot)]=values[0];return true;});need(buildings.all_succeeded(),"City strategy Buildings Info failed");
 const auto specs=db.prefetch_info("CitySpecializations",[&](auto slot,auto,const auto &row){Specialization v;v.type=row.text("Type").value_or("");need(!v.type.empty(),"City strategy specialization type unknown");v.is_default=row.integer("IsDefault")!=0;const auto yield=row.text("YieldType");if(yield&&!yield->empty()){const auto *type=r.types_.find(*yield);need(type&&type->table=="Yields"&&type->id>=0&&type->id<6,"City strategy specialization yield invalid");v.yield=integer(type->id);}matrix("CitySpecialization_TargetYields","CitySpecializationType",v.type,v.targets);r.specializations_.emplace(unsigned_integer(slot),std::move(v));return true;});need(specs.all_succeeded(),"City strategy Specializations Info failed");
 const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);const auto *d=defines.find("CITY_MAX_NUM_BUILDINGS");need(d&&d->lookup.found&&d->lookup.error.empty()&&std::holds_alternative<std::int64_t>(d->lookup.value),"City strategy native building-count define missing");r.max_buildings_=integer(std::get<std::int64_t>(d->lookup.value));return r;
}
std::int32_t BnwCityStrategyYieldRules::building_count(std::int32_t real,std::int32_t free)const{return max_buildings_<=1?std::max(real,free):add(real,free);}
BnwCityStrategyYieldQuote quote_bnw_city_strategy_best_yields(const LiveSessionState &s,const BnwCityStrategyYieldRules &r,const SessionCheckpointIdentity &id,std::int64_t owner,std::int64_t city){
 need(id.payload_sha256.size()==64&&id.rules_sha256==r.identity()&&owner>=0&&owner<64&&city>=0&&city<=INT32_MAX&&s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.world.map_projection&&s.world.map_plots&&s.world.map_plots->layout==bnw,"City strategy current source/rules/world incomplete");
 const auto *raw=resolve(s,static_cast<std::int32_t>(owner),static_cast<std::int32_t>(city));need(raw&&scalar(raw->core.fields,"m_iID")==city,"City strategy target city stale/absent");const auto &live=s.cities.players.at(owner).cities.at(city);const auto x=scalar(raw->core.fields,"m_iX"),y=scalar(raw->core.fields,"m_iY");need(x==live.x&&y==live.y&&scalar(raw->core.fields,"m_iPopulation")==live.population,"City strategy current city operands stale");
 const auto &map=*s.world.map_projection;need(s.world.map_header.layout==bnw&&map.grid.width==s.world.map_header.grid.width&&map.grid.height==s.world.map_header.grid.height&&map.wrap_x==s.world.map_header.wrap_x&&map.wrap_y==s.world.map_header.wrap_y&&map.cells.size()==map.grid.size()&&s.world.map_plots->plots.size()==map.grid.size(),"City strategy current map geometry differs");MapGeometry geo{map.grid,map.wrap_x,map.wrap_y,bnw};geo.validate();const auto home=geo.plot(x,y);need(home.has_value(),"City strategy home plot absent");
 BnwCityStrategyYieldQuote q;q.owner=owner;q.city=city;q.population=integer(live.population);need(q.population>=0,"City strategy native population malformed");const auto population_slots=std::min(add(q.population,2),37);q.average.fill(INT16_MAX);
 constexpr std::int32_t dx[]={0,0,1,1,0,-1,-1,0,1,2,2,2,1,0,-1,-2,-2,-2,-1,0,1,2,3,3,3,3,2,1,0,-1,-2,-3,-3,-3,-3,-2,-1};
 constexpr std::int32_t dy[]={0,1,0,-1,-1,0,1,2,1,0,-1,-2,-2,-2,-1,0,1,2,2,3,2,1,0,-1,-2,-3,-3,-3,-3,-2,-1,0,1,2,3,3,3};
 const auto half=[](std::int64_t n){return n/2-(n<0&&n%2?1:0);};std::array<std::vector<std::uint8_t>,6> sorted;
 for(std::uint32_t slot=0;slot<37;++slot){const auto py=static_cast<std::int64_t>(y)+dy[slot];const auto px=static_cast<std::int64_t>(x)-half(y)+dx[slot]+half(py);const auto index=geo.plot(px,py);if(!index)continue;const auto &plot=s.world.map_plots->plots.at(static_cast<std::size_t>(*index));const auto &cell=map.cells.at(static_cast<std::size_t>(*index));need(plot.index==static_cast<std::uint64_t>(*index)&&cell.index==static_cast<std::uint64_t>(*index)&&cell.yield_city,"City strategy working-city current handle unknown");const auto working_owner=integer(named(plot.fields,"working_city_owner").value),working_id=integer(named(plot.fields,"working_city_id").value);need(cell.yield_city->owner==working_owner&&cell.yield_city->id==working_id,"City strategy projected working-city handle stale");const auto *working=resolve(s,working_owner,working_id);if(!working||scalar(working->core.fields,"m_iID")!=city)continue;
  BnwCityStrategyPlotSample sample;sample.city_slot=slot;sample.plot=static_cast<std::uint64_t>(*index);sample.native_yields=yields(s,static_cast<std::uint64_t>(*index));for(std::size_t yield=0;yield<6;++yield){sample.narrowed_yields[yield]=static_cast<std::uint8_t>(sample.native_yields[yield]);sorted[yield].push_back(sample.narrowed_yields[yield]);}q.samples.push_back(sample);
 }
 q.evaluated=unsigned_integer(q.samples.size());q.slots=std::min(q.evaluated,population_slots);if(q.slots<=0)return q;
 const auto home_yields=yields(s,static_cast<std::uint64_t>(*home));q.food_extra=add(sub(home_yields[0],r.min_city_food()),food_buildings(*raw,live,r));
 for(std::size_t yield=0;yield<6;++yield){auto &values=sorted[yield];std::stable_sort(values.begin(),values.end(),std::greater<>{});std::int32_t sum=0;for(std::int32_t slot=0;slot<q.slots;++slot)sum=add(sum,mul(values[static_cast<std::size_t>(slot)],100));if(yield==0)sum=add(sum,mul(q.food_extra,100));q.average[yield]=static_cast<std::uint16_t>(sum/q.slots);}
 need(raw->management.layout==bnw,"City strategy current management unknown");const auto &strategy=named(raw->management.blocks,"strategy");q.specialization=integer(named(strategy.fields,"m_eSpecialization").value);
 if(q.specialization==-1){if(s.setup.is_human(s.setup.players.at(owner))){const auto focus=integer(named(named(raw->management.blocks,"citizens").fields,"m_eCityAIFocusTypes").value);const char *wanted=focus==1?"CITYSPECIALIZATION_SETTLER_PUMP":focus==3?"CITYSPECIALIZATION_COMMERCE":nullptr;if(wanted)for(const auto &[slot,spec]:r.specializations())if(spec.type==wanted){q.specialization=slot;break;}}
  if(q.specialization==-1)for(const auto &[slot,spec]:r.specializations())if(spec.is_default){q.specialization=slot;break;}
 }
 const auto spec=r.specializations().find(q.specialization);if(spec==r.specializations().end())return q;q.delta.emplace();for(std::size_t y=0;y<6;++y)(*q.delta)[y]=narrow_short(sub(q.average[y],spec->second.targets[y]));q.focus_yield=spec->second.yield;return q;
}
BnwCityStrategyYieldOwner bind_bnw_city_strategy_yields_original_import(const saves::SavedSession &source,const SessionCheckpointIdentity &id){need(source.source.container==saves::SaveContainerKind::civ5&&identify_bytes(source.source.bytes).sha256==id.payload_sha256&&id.rules_sha256.size()==64&&source.snapshot.all_players&&source.snapshot.all_players->layout==bnw&&source.snapshot.all_players->players.size()==64,"City strategy Read owner requires original-import identity");BnwCityStrategyYieldOwner o;o.identity_=id;o.guid_=source.snapshot.map_header.guid;for(const auto &p:source.snapshot.all_players->players)for(const auto &c:p.cities.cities){need(c.management.layout==bnw,"City strategy native Read management unknown");const auto id=scalar(c.core.fields,"m_iID");need(o.cities_.emplace(std::pair{p.saved_player_id,static_cast<std::int64_t>(id)},BnwCityStrategyRetainedYield{}).second,"City strategy duplicate Read city");}return o;}
void update_bnw_city_strategy_best_yields(const LiveSessionState &s,BnwCityStrategyYieldOwner &o,const BnwCityStrategyYieldRules &r,std::int64_t owner,std::int64_t city){need(o.guid_==s.world.map_header.guid&&o.identity_.rules_sha256==r.identity(),"City strategy retained owner source/rules differs");const auto q=quote_bnw_city_strategy_best_yields(s,r,o.identity_,owner,city);const auto entry=o.cities_.find({owner,city});need(entry!=o.cities_.end(),"City strategy construction/lifecycle retained owner unknown");const auto &before=entry->second;need(before.updates<UINT64_MAX&&(q.delta||before.delta),"City strategy source early return leaves mandatory delta unknown; no zero inference");auto staged=o;auto &current=staged.cities_.at({owner,city});current.average=q.average;current.focus_yield=q.focus_yield;if(q.delta)current.delta=q.delta;++current.updates;o=std::move(staged);}
}
