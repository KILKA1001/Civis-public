#include "civ5/runtime/bnw_city_current_coastal_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/plot_water.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <set>
#include <string_view>
#include <thread>
#include <type_traits>
#include <variant>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw BnwCityCoastalStrategyUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Coastal strategy reached signed32 operand unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityCoastalStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityCoastalStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Coastal strategy duplicate source field");out=&r;}need(out,"Coastal strategy source field absent");return *out;}
std::int32_t scalar(const saves::SavedCityObservation& c,std::string_view name){const auto& f=named(c.core.fields,name);need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Coastal strategy city native scalar unavailable");return native(f.values.front());}
// CvMap::getLandmass uses its own FFreeListTrashArray, not m_areas. This
// source-bound borrowed lookup never renames an area or constructs a world.
const saves::MapRegionRecord* landmass(const saves::SavedSessionSnapshot& world,std::int32_t handle){
 if(handle==-1)return nullptr;
 need(world.map_regions&&world.map_regions->layout==bnw,"Coastal strategy actual landmass registry absent");
 const auto& c=named(world.map_regions->containers,"landmasses");need(c.legacy_metadata.size()==5,"Coastal strategy landmass native FLTA metadata absent");
 const auto slots=native(c.legacy_metadata[0]),last=native(c.legacy_metadata[1]),free=native(c.legacy_metadata[3]);
 need(c.records.size()<=8192&&slots>=0&&slots<=8192&&last>=-1&&last<slots&&free>=0&&std::int64_t(last)+1-free==c.wire_count&&c.wire_count==static_cast<std::int64_t>(c.records.size())&&c.next_free.size()==static_cast<std::size_t>(slots),"Coastal strategy native landmass FLTA shape stale");
 std::set<std::int32_t> occupied;const saves::MapRegionRecord* found=nullptr;
 for(const auto& record:c.records){const auto id=native(named(record.fields,"id").value),slot=id&8191;need(id>=0&&slot<=last&&slot<slots&&occupied.insert(slot).second,"Coastal strategy native landmass slot ambiguous");if(slot==(handle&8191)&&((handle&~8191)==0||id==handle))found=&record;}
 return found;
}
struct MapRead {
 const LiveSessionState& s;const saves::SavedSession& source;
 const auto& raw(std::int64_t id)const{
  need(id>=0&&static_cast<std::uint64_t>(id)<s.world.map_plots->plots.size(),"Coastal strategy reached plot index unavailable");const auto index=static_cast<std::size_t>(id);const auto& p=s.world.map_plots->plots.at(index);const auto& c=s.world.map_projection->cells.at(index);const auto& old=source.snapshot.map_plots->plots.at(index);const auto xy=s.world.map_header.grid.coordinates(index);
  need(p.index==index&&c.index==index&&p.x==xy[0]&&p.y==xy[1]&&c.x==xy[0]&&c.y==xy[1]&&p.version&&*p.version>=1&&*p.version<=7&&p.version==old.version&&p.start_offset==old.start_offset&&p.end_offset==old.end_offset,"Coastal strategy reached plot identity/span differs");
  for(const auto name:{"source_x","source_y"}){const auto n=named(p.fields,name).value;const auto v=std::string_view(name)=="source_x"?xy[0]:xy[1];need(n>=0&&n<=INT16_MAX&&static_cast<std::uint64_t>(n)==v,"Coastal strategy native short coordinate differs");}return p;
 }
 const auto& cell(std::int64_t id)const{return s.world.map_projection->cells.at(static_cast<std::size_t>(id));}
 bool water(std::int64_t id)const{const auto n=named(raw(id).fields,"plot_type").value;need(n>=INT8_MIN&&n<=INT8_MAX&&cell(id).plot_type==std::optional<std::int64_t>{n},"Coastal strategy raw/owned native plot category differs");return n==3;}
 std::int32_t handle(std::int64_t id)const{const auto n=native(named(raw(id).fields,"landmass").value);need(cell(id).landmass==std::optional<std::int64_t>{n},"Coastal strategy raw/owned landmass handle differs");return n;}
};
void bind_record(const saves::SavedSessionSnapshot& world,const saves::SavedSession& source,const saves::MapRegionRecord& r){
 need(world.map_regions&&source.snapshot.map_regions&&world.map_regions->layout==bnw&&source.snapshot.map_regions->layout==bnw&&world.map_regions->start_offset==source.snapshot.map_regions->start_offset&&world.map_regions->end_offset==source.snapshot.map_regions->end_offset,"Coastal strategy current landmass source region span differs");
 const auto& c=named(world.map_regions->containers,"landmasses");const auto& old=named(source.snapshot.map_regions->containers,"landmasses");need(c.start_offset==old.start_offset&&c.end_offset==old.end_offset&&r.version==std::optional<std::uint64_t>{1}&&r.start_offset<r.end_offset,"Coastal strategy landmass source container/record absent");
 const auto id=native(named(r.fields,"id").value);const saves::MapRegionRecord* baseline=nullptr;for(const auto& x:old.records)if(native(named(x.fields,"id").value)==id){need(!baseline,"Coastal strategy original landmass ID ambiguous");baseline=&x;}
 need(baseline&&baseline->version==r.version&&baseline->start_offset==r.start_offset&&baseline->end_offset==r.end_offset,"Coastal strategy reached landmass lacks original native object span");
}
}
const saves::MapRegionRecord* detail::bnw_coastal_landmass_lookup(const saves::SavedSessionSnapshot& world,std::int64_t handle){return landmass(world,native(handle));}
struct BnwCityCurrentCoastalStrategy::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::shared_ptr<const ui::SelectedBnwPreviewHost> host;std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;std::shared_ptr<const LiveSessionState> state;const saves::SavedSession* source=nullptr;
 std::uint64_t generation=0;SessionCheckpointIdentity identity;std::function<bool()> rules_current;std::int32_t owner=-1,city=-1;
 std::pair<std::uint64_t,std::uint64_t> city_span;mutable std::atomic<bool> retired{false};
};
BnwCityCurrentCoastalStrategy::BnwCityCurrentCoastalStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentCoastalStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Coastal strategy foreign constructor thread/moved query");
 try{need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Coastal strategy genuine rules/host retired");need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Coastal strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode();need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,"Coastal strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwCityCurrentCoastalStrategy::value(BnwCoastalCityStrategy trigger)const{return typed([&]{
 require_current();need(trigger==BnwCoastalCityStrategy::coast||trigger==BnwCoastalCityStrategy::landlocked,"Coastal strategy trigger outside two-source scope");const auto& p=*impl_;
 try{
  const auto& s=*p.state;const auto* city=resolve_bnw_city_registry(s.world,s.cities,p.owner,p.city);need(city&&scalar(*city,"m_iID")==p.city&&scalar(*city,"m_eOwner")==p.owner&&std::pair{city->core.start_offset,city->core.end_offset}==p.city_span,"Coastal strategy canonical city/span differs");const auto x=scalar(*city,"m_iX"),y=scalar(*city,"m_iY");const auto& owned=s.cities.players.at(p.owner).cities.at(p.city);need(owned.x==x&&owned.y==y,"Coastal strategy raw/owned city coordinate differs");
  const auto& h=s.world.map_header;const auto& old=p.source->snapshot.map_header;need(h.layout==bnw&&old.layout==bnw&&h.grid.width==old.grid.width&&h.grid.height==old.grid.height&&h.wrap_x==old.wrap_x&&h.wrap_y==old.wrap_y&&h.guid==old.guid&&s.world.map_plots&&s.world.map_projection&&p.source->snapshot.map_plots&&s.world.map_plots->layout==bnw&&p.source->snapshot.map_plots->layout==bnw,"Coastal strategy SAME native map identity absent");
  need(h.grid.width>0&&h.grid.width<=INT32_MAX&&h.grid.height>0&&h.grid.height<=INT32_MAX&&h.grid.size()<=INT32_MAX&&x>=0&&y>=0&&static_cast<std::uint64_t>(x)<h.grid.width&&static_cast<std::uint64_t>(y)<h.grid.height,"Coastal strategy native map bounds unavailable");const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};const auto home=geometry.plot(x,y);need(home.has_value(),"Coastal strategy actual home absent");
  const auto& map=*s.world.map_projection;const auto& rawmap=*s.world.map_plots;need(map.grid.width==h.grid.width&&map.grid.height==h.grid.height&&map.wrap_x==h.wrap_x&&map.wrap_y==h.wrap_y&&map.cells.size()==h.grid.size()&&rawmap.plots.size()==h.grid.size()&&p.source->snapshot.map_plots->plots.size()==h.grid.size()&&rawmap.start_offset==p.source->snapshot.map_plots->start_offset&&rawmap.end_offset==p.source->snapshot.map_plots->end_offset,"Coastal strategy raw/owned map geometry/span differs");
  MapRead read{s,*p.source};const auto& raw=read.raw(*home);const auto& cell=read.cell(*home);const auto po=native(named(raw.fields,"plot_city_owner").value),pc=native(named(raw.fields,"plot_city_id").value);need(cell.plot_city&&cell.plot_city->owner==po&&cell.plot_city->id==pc&&resolve_bnw_city_registry(s.world,s.cities,po,pc)==city,"Coastal strategy actual city-home reference differs");
  const bool home_water=read.water(*home);
  if(home_water){require_current();return trigger==BnwCoastalCityStrategy::landlocked;} // SDK BEFORE Define, passability or landmass joins
  require_current();const auto* d=p.host->defines().find("MIN_WATER_SIZE_FOR_OCEAN");need(p.host->defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->kind==content::GlobalDefineKind::integer&&d->lookup.found&&d->lookup.error.empty()&&d->lookup.sqlite_code==0&&std::holds_alternative<std::int64_t>(d->lookup.value),"Coastal strategy reached actual ocean Define absent");const auto minimum=native(std::get<std::int64_t>(d->lookup.value));
  const auto db_define=p.database->global_define("MIN_WATER_SIZE_FOR_OCEAN",content::GlobalDefineKind::integer);require_current();need(db_define.found&&db_define.error.empty()&&db_define.sqlite_code==0&&std::holds_alternative<std::int64_t>(db_define.value)&&std::get<std::int64_t>(db_define.value)==minimum,"Coastal strategy cached/current ocean Define differs");
  auto helper_map=map;PlotWaterState water_state;water_state.layout=h.layout;
  // Native reached-prefix validation. This controls proofs/alias bindings,
  // not the result: the existing water helper returns the coastal value.
  for(std::int32_t direction=0;direction<6;++direction){const auto n=geometry.direction(x,y,direction);if(!n||!read.water(*n))continue;if(minimum<=0)break;
   const auto handle=read.handle(*n);const auto* record=detail::bnw_coastal_landmass_lookup(s.world,handle);if(!record)continue;bind_record(s.world,*p.source,*record);
   const auto id=native(named(record->fields,"id").value),tiles=native(named(record->fields,"tiles").value);const auto flag=named(record->fields,"water").value;need(tiles>=0&&(flag==0||flag==1),"Coastal strategy reached native landmass count/bool unavailable");
   helper_map.cells.at(static_cast<std::size_t>(*n)).landmass=id;const auto [it,inserted]=water_state.landmasses.emplace(id,PlotWaterLandmass{tiles,flag!=0});need(inserted||(it->second.tiles==tiles&&it->second.water==(flag!=0)),"Coastal strategy duplicate resolved landmass facts differ");if(tiles>=minimum)break;
  }
  require_current();need(s.plot_impassability.rules_sha256==p.identity.rules_sha256,"Coastal strategy actual passability rules differ");const auto& pass=bnw_live_plot_impassability(s);
  const auto rules=load_plot_water_rules(*p.database,content::TechnologyProfile::bnw);require_current();
  PlotWaterContext context{helper_map,water_state,rules,p.host->types(),pass,s.setup,s.cities,p.host->defines()};
  const bool coastal=plot_is_coastal_land(context,*home,minimum,true,false);require_current();return trigger==BnwCoastalCityStrategy::coast?coastal:!coastal;
 }catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentCoastalStrategy inspect_bnw_city_current_coastal_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Coastal strategy actual selected host absent");(void)host->session().bnw_game_debug_mode();auto p=std::make_shared<BnwCityCurrentCoastalStrategy::Impl>();p->host=std::move(host);p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Coastal strategy actual immutable rules scope absent");p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Coastal strategy actual host rules identity differs");p->state=p->session->read_state_lease();need(bool(p->state),"Coastal strategy actual state absent");p->source=&p->session->source();p->generation=p->session->state_generation();need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Coastal strategy actual source payload differs");
 p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Coastal strategy native owner outside BNW roster");const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));need(raw,"Coastal strategy actual current city absent");p->city=scalar(*raw,"m_iID");need(scalar(*raw,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Coastal strategy canonical owner/span differs");p->city_span={raw->core.start_offset,raw->core.end_offset};BnwCityCurrentCoastalStrategy result(std::move(p));result.require_current();return result;
});}
}
