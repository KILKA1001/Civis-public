#include "civ5/runtime/bnw_city_current_geographic_strategy.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::int32_t mountain_type=0,hill_type=1,jungle_type=1,forest_type=5;
void need(bool b,const char* m){if(!b)throw BnwCityGeographicStrategyUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Geographic strategy reached signed32 operand unavailable");return static_cast<std::int32_t>(n);}
std::int32_t byte(std::int64_t n){need(n>=INT8_MIN&&n<=INT8_MAX,"Geographic strategy reached native signed-byte operand unavailable");return native(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityGeographicStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityGeographicStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Geographic strategy duplicate raw field");out=&r;}need(out,"Geographic strategy source field absent");return *out;}
std::int32_t scalar(const saves::SavedCityObservation& c,std::string_view name){const auto& f=named(c.core.fields,name);need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Geographic strategy city native scalar shape unavailable");return native(f.values.front());}
void supported(BnwGeographicCityStrategy t){
 if(t==BnwGeographicCityStrategy::coast||t==BnwGeographicCityStrategy::landlocked)throw BnwCityGeographicStrategyUnavailable("Geographic strategy Coast/Landlocked requires current native landmass/water owner join");
 need(t==BnwGeographicCityStrategy::river||t==BnwGeographicCityStrategy::mountain||t==BnwGeographicCityStrategy::hill||t==BnwGeographicCityStrategy::forest||t==BnwGeographicCityStrategy::jungle,"Geographic strategy trigger outside supported five");
}
void geometry_bound(const MapGeometry& g,std::int64_t x,std::int64_t y){
 need(g.layout==bnw&&g.grid.width>0&&g.grid.width<=INT32_MAX&&g.grid.height>0&&g.grid.height<=INT32_MAX&&g.grid.size()<=INT32_MAX,"Geographic strategy native BNW geometry unavailable");
 (void)native(x);(void)native(y);need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<g.grid.width&&static_cast<std::uint64_t>(y)<g.grid.height,"Geographic strategy native home outside map");
}
template<class Reader>bool evaluate(BnwGeographicCityStrategy t,const MapGeometry& g,std::int64_t x,std::int64_t y,Reader& r){
 supported(t);geometry_bound(g,x,y);const auto home=g.plot(x,y);need(home.has_value(),"Geographic strategy actual home plot absent");
 if(t==BnwGeographicCityStrategy::river)return r.river(*home)>0;
 if(t==BnwGeographicCityStrategy::mountain){
  for(std::int32_t d=0;d<6;++d)if(const auto n=g.direction(x,y,d))if(r.kind(*n)==mountain_type)return true;
  return false;
 }
 const auto positions=detail::bnw_city_feature_positions(g,x,y);std::int32_t matches=0;
 for(const auto n:positions){if(!n)continue;
  const bool type_matches=t==BnwGeographicCityStrategy::hill?r.kind(*n)==hill_type:r.feature(*n)==(t==BnwGeographicCityStrategy::forest?forest_type:jungle_type);
  // SDK does not read either owner unless this slot's type matched. The
  // home owner is read again at each matched slot; wrapped slots count again.
  if(type_matches&&r.owner(*n)==r.owner(*home)){++matches;if(matches>1)return true;}
 }return false;
}
struct MathReader {
 std::span<const BnwGeographicPlotMath> plots;
 const auto& at(std::int64_t id)const{need(id>=0&&static_cast<std::uint64_t>(id)<plots.size(),"Geographic pure plot index unavailable");return plots[static_cast<std::size_t>(id)];}
 std::int32_t get(const std::optional<std::int64_t>& n)const{need(n.has_value(),"Geographic pure reached operand absent");return byte(*n);}
 std::int32_t kind(std::int64_t id)const{return get(at(id).plot_type);}
 std::int32_t owner(std::int64_t id)const{return get(at(id).owner);}
 std::int32_t feature(std::int64_t id)const{return get(at(id).feature);}
 std::int32_t river(std::int64_t id)const{return get(at(id).river_crossing_count);}
};
struct ActualReader {
 const LiveSessionState& s;const saves::SavedSession& source;const content::InfoTypeCatalog& types;
 const saves::PlotObservation& raw(std::int64_t id)const{
  need(id>=0&&static_cast<std::uint64_t>(id)<s.world.map_plots->plots.size(),"Geographic strategy reached plot index unavailable");
  const auto index=static_cast<std::size_t>(id);const auto& p=s.world.map_plots->plots.at(index);const auto& c=s.world.map_projection->cells.at(index);const auto& old=source.snapshot.map_plots->plots.at(index);const auto xy=s.world.map_header.grid.coordinates(index);
  need(p.index==index&&c.index==index&&p.x==xy[0]&&p.y==xy[1]&&c.x==xy[0]&&c.y==xy[1]&&p.version&&*p.version>=1&&*p.version<=7&&p.version==old.version&&p.start_offset==old.start_offset&&p.end_offset==old.end_offset,"Geographic strategy reached plot identity/span differs");
  for(const auto name:{"source_x","source_y"}){const auto v=named(p.fields,name).value;const auto expected=std::string_view(name)=="source_x"?xy[0]:xy[1];need(v>=0&&v<=INT16_MAX&&static_cast<std::uint64_t>(v)==expected,"Geographic strategy reached native short coordinate differs");}
  return p;
 }
 const auto& cell(std::int64_t id)const{return s.world.map_projection->cells.at(static_cast<std::size_t>(id));}
 std::int32_t kind(std::int64_t id)const{const auto n=byte(named(raw(id).fields,"plot_type").value);need(cell(id).plot_type==std::optional<std::int64_t>{n},"Geographic strategy raw/owned plot category differs");return n;}
 std::int32_t owner(std::int64_t id)const{const auto n=byte(named(raw(id).fields,"owner").value);need(n>=-1&&n<64&&cell(id).owner==std::optional<std::int64_t>{n},"Geographic strategy raw/owned plot owner differs");return n;}
 std::int32_t river(std::int64_t id)const{const auto n=byte(named(raw(id).fields,"river_crossing_count").value);need(cell(id).rivers&&cell(id).rivers->is_river==(n>0),"Geographic strategy raw/owned crossing count differs");return n;}
 std::int32_t feature(std::int64_t id)const{
  const auto& p=raw(id);const auto& c=cell(id);
  if(*p.version<3){const auto n=byte(named(p.fields,"feature_type").value);need(!c.feature_type_hash&&c.feature_type==std::optional<std::int64_t>{n},"Geographic strategy raw/owned feature enum differs");return n;}
  const auto hash=named(p.fields,"feature_type_hash").value;need(hash>=0&&static_cast<std::uint64_t>(hash)<=UINT32_MAX&&!c.feature_type&&c.feature_type_hash==std::optional<std::uint64_t>{static_cast<std::uint64_t>(hash)},"Geographic strategy raw/owned feature hash differs");
  if(hash==0)return -1;
  const auto* info=types.find_hash(static_cast<std::uint64_t>(hash));need(info&&info->table=="Features","Geographic strategy reached FeatureInfo hash unavailable");return byte(info->id);
 }
};
}
bool bnw_city_geographic_strategy_math(BnwGeographicCityStrategy t,const MapGeometry& g,std::int64_t x,std::int64_t y,std::span<const BnwGeographicPlotMath> plots){return typed([&]{supported(t);geometry_bound(g,x,y);need(plots.size()==g.grid.size(),"Geographic pure map size differs");MathReader r{plots};return evaluate(t,g,x,y,r);});}
struct BnwCityCurrentGeographicStrategy::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;SessionCheckpointIdentity identity;
 std::function<bool()> rules_current;std::int32_t owner=-1,city=-1;
 std::pair<std::uint64_t,std::uint64_t> city_span;mutable std::atomic<bool> retired{false};
};
BnwCityCurrentGeographicStrategy::BnwCityCurrentGeographicStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentGeographicStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Geographic strategy foreign constructor thread/moved query");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Geographic strategy genuine rules/host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Geographic strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode();
  need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,"Geographic strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwCityCurrentGeographicStrategy::value(BnwGeographicCityStrategy t)const{return typed([&]{
 require_current();supported(t);const auto& p=*impl_;
 try{
  const auto& s=*p.state;const auto* city=resolve_bnw_city_registry(s.world,s.cities,p.owner,p.city);need(city&&scalar(*city,"m_iID")==p.city&&scalar(*city,"m_eOwner")==p.owner&&std::pair{city->core.start_offset,city->core.end_offset}==p.city_span,"Geographic strategy canonical city/span differs");
  const auto x=scalar(*city,"m_iX"),y=scalar(*city,"m_iY");const auto& owned=s.cities.players.at(p.owner).cities.at(p.city);need(owned.x==x&&owned.y==y,"Geographic strategy raw/owned city coordinate differs");
  const auto& h=s.world.map_header;const auto& old=p.source->snapshot.map_header;
  need(h.layout==bnw&&old.layout==bnw&&h.grid.width==old.grid.width&&h.grid.height==old.grid.height&&h.wrap_x==old.wrap_x&&h.wrap_y==old.wrap_y&&h.guid==old.guid&&s.world.map_plots&&s.world.map_projection&&p.source->snapshot.map_plots&&s.world.map_plots->layout==bnw&&p.source->snapshot.map_plots->layout==bnw,"Geographic strategy SAME native map identity unavailable");
  const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};geometry_bound(geometry,x,y);
  const auto& map=*s.world.map_projection;const auto& rawmap=*s.world.map_plots;
  need(map.grid.width==h.grid.width&&map.grid.height==h.grid.height&&map.wrap_x==h.wrap_x&&map.wrap_y==h.wrap_y&&map.cells.size()==h.grid.size()&&rawmap.plots.size()==h.grid.size()&&p.source->snapshot.map_plots->plots.size()==h.grid.size()&&rawmap.start_offset==p.source->snapshot.map_plots->start_offset&&rawmap.end_offset==p.source->snapshot.map_plots->end_offset,"Geographic strategy raw/owned map geometry/span differs");
  ActualReader r{s,*p.source,p.host->types()};const auto home=geometry.plot(x,y);need(home.has_value(),"Geographic strategy city home absent");
  const auto& raw=r.raw(*home);const auto& cell=r.cell(*home);const auto plot_owner=native(named(raw.fields,"plot_city_owner").value),plot_city=native(named(raw.fields,"plot_city_id").value);
  need(cell.plot_city&&cell.plot_city->owner==plot_owner&&cell.plot_city->id==plot_city&&resolve_bnw_city_registry(s.world,s.cities,plot_owner,plot_city)==city,"Geographic strategy actual city-home reference differs");
  if(t==BnwGeographicCityStrategy::forest||t==BnwGeographicCityStrategy::jungle){
   const char* name=t==BnwGeographicCityStrategy::forest?"FEATURE_FOREST":"FEATURE_JUNGLE";const auto id=t==BnwGeographicCityStrategy::forest?forest_type:jungle_type;
   const auto& types=p.host->types();const auto* info=types.find(name);need(types.report().all_succeeded()&&info&&info->table=="Features"&&info->id==id&&info->hash==content::saved_info_type_hash(name),"Geographic strategy actual FeatureInfo native constant mapping differs");
   const auto* roundtrip=types.find_hash(info->hash);need(roundtrip&&roundtrip->type==name&&roundtrip->table=="Features"&&roundtrip->id==id,"Geographic strategy native FeatureInfo hash roundtrip differs");
  }
  const auto result=evaluate(t,geometry,x,y,r);require_current();return result;
 }catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentGeographicStrategy inspect_bnw_city_current_geographic_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Geographic strategy actual selected host absent");(void)host->session().bnw_game_debug_mode();
 auto p=std::make_shared<BnwCityCurrentGeographicStrategy::Impl>();p->host=std::move(host);p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Geographic strategy actual immutable rules scope absent");
 p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Geographic strategy actual host rules identity differs");
 p->state=p->session->read_state_lease();need(bool(p->state),"Geographic strategy actual state absent");p->source=&p->session->source();p->generation=p->session->state_generation();need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Geographic strategy actual source payload differs");
 p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Geographic strategy native owner outside BNW roster");const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));need(raw,"Geographic strategy actual current city absent");p->city=scalar(*raw,"m_iID");need(scalar(*raw,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Geographic strategy canonical owner/span differs");p->city_span={raw->core.start_offset,raw->core.end_offset};
 BnwCityCurrentGeographicStrategy result(std::move(p));result.require_current();return result;
});}
}
