#include "civ5/runtime/bnw_plot_movement_cost.hpp"
#include "civ5/runtime/bnw_unit_movement_query.hpp"
#include "civ5/runtime/bnw_plot_reveal_read.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <atomic>
#include <climits>
#include <functional>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>
#include <variant>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char* why){if(!ok)throw BnwPlotMovementCostUnavailable(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Movement cost reached signed32 overflow/operand");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)*b);}
bool boolean(std::int64_t n){need(n==0||n==1,"Movement cost reached native boolean invalid");return n!=0;}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwPlotMovementCostUnavailable&){throw;}
 catch(const std::exception& e){throw BnwPlotMovementCostUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Movement cost duplicate source field");out=&r;}
 need(out,"Movement cost reached source field absent");return *out;
}
std::int32_t core(const saves::UnitCoreObservation& c,std::string_view name){
 const auto& f=named(c.fields,name);need(c.layout==bnw&&f.encoding==saves::UnitCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty(),"Movement cost native Unit scalar schema unavailable");return native(f.values[0]);
}
bool unit_bool(const saves::UnitCoreObservation& c,std::string_view name){
 const auto& f=named(c.fields,name);need(f.encoding==saves::UnitCoreEncoding::boolean&&f.values.size()==1&&!f.wire_count&&f.bytes.empty(),"Movement cost native Unit bool schema unavailable");return boolean(f.values[0]);
}
// One shared reached arithmetic body. ActualReader supplies genuine getters;
// MathReader supplies explicitly detached operands and cannot bind any owner.
template<class R>std::int32_t costs(R& r){
 const bool faster_river=r.b("faster_river"),faster_hills=r.b("faster_hills"),ignore=r.b("ignore");
 const auto denominator=r.i("denominator");const bool crossing=r.b("crossing");
 const auto feature=r.i("feature");r.bind_info("feature",feature);
 const auto terrain=r.i("terrain");r.bind_info("terrain",terrain);
 std::int32_t regular=0;
 if(ignore||(faster_river&&r.b("river"))||(faster_hills&&r.b("hills")))regular=1;
 else{
  regular=feature==-1?r.i("terrain_move"):r.i("feature_move");
  if(r.b("hills")&&!r.b("city"))regular=add(regular,r.i("hill_extra"));
  if(regular>0)regular=std::max<std::int32_t>(1,native(static_cast<std::int64_t>(regular)-r.i("discount")));
 }
 if(r.b("rough")&&r.b("rough_ends"))regular=INT32_MAX;
 else{
  if(!(ignore||faster_river)&&crossing)regular=add(regular,r.i("river_extra"));
  regular=mul(regular,denominator);
  if(r.b("hills")&&r.b("hill_double"))regular/=2;
  else if(feature==-1?r.b("terrain_double"):r.b("feature_double"))regular/=2;
 }
 regular=std::min(regular,mul(r.i("base"),denominator));
 std::int32_t route=INT32_MAX,flat=INT32_MAX;
 if(r.b("from_route")&&r.b("to_route")&&(r.b("bridge")||!r.b("crossing"))){
  // Info metadata is obtained in native from/to order, before route arithmetic.
  r.bind_route(false);const auto from_move=r.i("from_route_move"),from_flat_move=r.i("from_route_flat");
  r.bind_route(true);const auto to_move=r.i("to_route_move"),to_flat_move=r.i("to_route_flat");
  const auto from_change=r.i("from_route_change");const auto from=add(from_move,from_change);
  const auto to_change=r.i("to_route_change");const auto to=add(to_move,to_change);route=std::max(from,to);
  const auto from_flat=mul(from_flat_move,r.i("base"));
  const auto to_flat=mul(to_flat_move,r.i("base"));flat=std::max(from_flat,to_flat);
 }else if(r.b("own_target")&&(feature==5||feature==1)&&r.b("woods_road")){
  r.bind_road();route=r.i("road_move");flat=mul(r.i("road_flat"),r.i("base"));
 }
 if(r.b("has_team")&&(r.b("team_border")||r.b("player_border"))){
  // Supported actual branch is ground DOMAIN_LAND. Pure operands represent
  // precisely this same scope, not water/air movement.
  if(!r.b("same_team")&&!r.b("open_borders"))regular=add(regular,denominator);
 }
 return std::max<std::int32_t>(1,std::min(regular,std::min(route,flat)));
}
struct MathReader{
 const std::map<std::string,std::int64_t,std::less<>>& fields;
 std::int32_t i(std::string_view key)const{const auto it=fields.find(key);need(it!=fields.end(),"Movement detached reached operand absent");return native(it->second);}
 bool b(std::string_view key)const{return boolean(i(key));}
 void bind_info(const char*,std::int32_t)const{}
 void bind_route(bool)const{}
 void bind_road()const{}
};
struct Info {content::InfoTypeEntry identity;std::map<std::string,std::int64_t,std::less<>> values;};
struct Collection {std::uint64_t span=0;std::map<std::int32_t,Info> rows;};
struct ActualReader{
 const LiveSessionState& s;const saves::SavedSession& source;const ui::SelectedBnwPreviewHost& host;
 const saves::SavedUnitObservation& u;BnwUnitOrderKey key;std::uint64_t from,to;
 std::int32_t team,base,denominator;MapGeometry geometry;std::function<void()> guard;
 std::map<std::string,Collection,std::less<>> collections;
 const saves::PlotObservation& raw(std::uint64_t id)const{
  need(s.world.map_plots&&s.world.map_projection&&source.snapshot.map_plots,"Movement actual native map unavailable");
  const auto& p=s.world.map_plots->plots.at(id);const auto& c=s.world.map_projection->cells.at(id);const auto& old=source.snapshot.map_plots->plots.at(id);const auto xy=geometry.grid.coordinates(id);
  need(p.index==id&&c.index==id&&p.x==xy[0]&&p.y==xy[1]&&c.x==xy[0]&&c.y==xy[1]&&p.version&&*p.version>=1&&*p.version<=7&&p.version==old.version&&p.start_offset==old.start_offset&&p.end_offset==old.end_offset,"Movement canonical raw/owned plot source span differs");
  need(named(p.fields,"source_x").value==static_cast<std::int64_t>(xy[0])&&named(p.fields,"source_y").value==static_cast<std::int64_t>(xy[1])&&xy[0]<=INT16_MAX&&xy[1]<=INT16_MAX,"Movement native short plot coordinate differs");return p;
 }
 const auto& cell(std::uint64_t id)const{return s.world.map_projection->cells.at(id);}
 std::int32_t plot(std::uint64_t id,const char* field)const{return native(named(raw(id).fields,field).value);}
 std::int32_t kind(std::uint64_t id)const{const auto n=plot(id,"plot_type");need(n>=0&&n<4&&cell(id).plot_type==n,"Movement actual plot enum differs");return n;}
 std::int32_t owner(std::uint64_t id)const{const auto n=plot(id,"owner");need(n>=-1&&n<64&&cell(id).owner==n,"Movement actual plot owner differs");return n;}
 bool city(std::uint64_t id)const{
  const auto o=plot(id,"plot_city_owner"),handle=plot(id,"plot_city_id");need(cell(id).plot_city&&cell(id).plot_city->owner==o&&cell(id).plot_city->id==handle,"Movement plot-city raw/owned handle differs");
  if(o==-1&&handle==-1)return false;
  need(o>=0&&o<64&&handle>=0,"Movement plot-city native handle invalid");return resolve_bnw_city_registry(s.world,s.cities,o,handle)!=nullptr;
 }
 bool trait(const char* name)const{
  const auto& t=s.world.all_players->players.at(static_cast<std::size_t>(key.first)).traits;
  const auto& f=named(t.fields,name);need(t.layout==bnw&&t.version&&*t.version>=1&&*t.version<=19&&f.encoding==saves::PlayerTraitsEncoding::boolean&&f.values.size()==1&&f.columns==1&&!f.wire_count&&f.rows.empty()&&f.hashes.empty(),"Movement trait native bool source unavailable");return boolean(f.values[0]);
 }
 Collection& collection(const char* table){
  const auto existing=collections.find(table);if(existing!=collections.end())return existing->second;
  guard();Collection result;
  const auto report=host.database().prefetch_info(table,[&](auto slot,auto id,const auto& row){
   need(id>=0&&id<=INT32_MAX&&slot==static_cast<std::uint64_t>(id)&&row.has_column("Type"),"Movement native Info slot schema differs");
   const auto type=row.text("Type");need(type&&!type->empty(),"Movement native Info Type absent");
   const auto* e=host.types().find(*type);const auto* h=e?host.types().find_hash(e->hash):nullptr;
   need(e&&h&&e->table==table&&e->id==id&&e->type==*type&&e->hash==content::saved_info_type_hash(*type)&&h->type==e->type&&h->table==e->table&&h->id==e->id&&h->hash==e->hash,"Movement native Info table/ID/hash/collision identity differs");
   Info info{*e,{}};
   for(const char* column:{"Movement","FlatMovement"})if(row.has_column(column))info.values.emplace(column,row.integer(column));
   if(row.has_column("AllowsWalkWater"))info.values.emplace("AllowsWalkWater",row.boolean("AllowsWalkWater")?1:0);
   need(result.rows.emplace(native(id),std::move(info)).second,"Movement duplicate Info slot");return true;
  });guard();need(report.all_succeeded()&&report.overlapping_ids==0&&report.slots<=INT32_MAX,"Movement native Info collection incomplete");result.span=report.slots;
  return collections.emplace(table,std::move(result)).first->second;
 }
 const Info* info(const char* table,std::int32_t id){
  if(id==-1)return nullptr;auto& c=collection(table);need(id>=0&&static_cast<std::uint64_t>(id)<c.span,"Movement reached native Info index outside collection");const auto it=c.rows.find(id);return it==c.rows.end()?nullptr:&it->second;
 }
 std::int32_t info_value(const char* table,std::int32_t id,const char* column){const auto* row=info(table,id);if(!row)return 0;const auto it=row->values.find(column);need(it!=row->values.end(),"Movement reached Info numeric column absent");return native(it->second);}
 std::int32_t hash_id(std::uint64_t slot,bool feature){
  const auto& p=raw(slot);const auto& c=cell(slot);const char* table=feature?"Features":"Improvements";const auto threshold=feature?3U:5U;
  if(*p.version<threshold){const auto id=plot(slot,feature?"feature_type":"improvement_type");need(id>=-1&&id<=INT8_MAX&&(feature?c.feature_type:c.improvement_type)==id,"Movement legacy raw/owned Info ID differs");return id;}
  const auto h=named(p.fields,feature?"feature_type_hash":"improvement_type_hash").value;need(h>=0&&h<=UINT32_MAX,"Movement reached Info hash outside uint32");
  if(feature)need(c.feature_type_hash==static_cast<std::uint64_t>(h),"Movement raw/owned feature hash differs");
  else if(h==0)need(!c.improvement_type||*c.improvement_type==-1,"Movement raw/owned no-improvement differs");
  if(h==0)return -1;
  const auto* e=host.types().find_hash(static_cast<std::uint64_t>(h));need(e&&e->table==table,"Movement native unknown/cross-table Info remap requires producer");
  if(!feature)need(c.improvement_type==e->id,"Movement raw/owned improvement mapping differs");return native(e->id);
 }
 bool walk(std::uint64_t slot){const auto id=hash_id(slot,false);if(id==-1)return false;return boolean(info_value("Improvements",id,"AllowsWalkWater"));}
 std::int32_t terrain(){const auto n=plot(to,"terrain_type");need(n>=-1&&n<=INT8_MAX&&cell(to).terrain_type==n,"Movement raw/owned terrain enum differs");return n;}
 std::int32_t route_id(std::uint64_t slot)const{const auto n=plot(slot,"route_type");need(n>=-1&&n<=INT8_MAX&&cell(slot).route&&cell(slot).route->type==n,"Movement raw/owned route enum differs");return n;}
 const auto& team_raw(std::int32_t id)const{need(id>=0&&id<64&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64,"Movement actual team roster unavailable");const auto& t=s.world.teams->teams.at(static_cast<std::size_t>(id));need(t.index==static_cast<std::uint64_t>(id),"Movement actual team ID differs");return t;}
 std::int32_t plot_team(std::uint64_t slot)const{const auto id=owner(slot);return id==-1?-1:detail::bnw_player_team_read(s,id);}
 bool team_bit(std::int32_t id,const char* name,std::int32_t other)const{const auto& a=named(team_raw(id).arrays,name);need(other>=0&&other<64&&a.columns==1&&a.values.size()==64,"Movement actual native team bool span unavailable");return boolean(a.values.at(static_cast<std::size_t>(other)));}
 bool enemy(std::uint64_t slot)const{
  const auto target_team=plot_team(slot);if(target_team==-1)return false;
  auto combat_team=team;
  if(team!=target_team&&target_team!=63&&!city(slot)&&core(u.core,"m_iAlwaysHostileCount")>0)combat_team=detail::bnw_player_team_read(s,63);
  if(combat_team==-1)return false;return team_bit(combat_team,"at_war",target_team);
 }
 bool valid_route(std::uint64_t slot)const{
  if(route_id(slot)==-1)return false;const auto pillaged=boolean(plot(slot,"route_pillaged"));need(cell(slot).route->pillaged==pillaged,"Movement raw/owned route pillage differs");
  if(pillaged)return false;return !enemy(slot)||core(u.core,"m_iEnemyRouteCount")>0;
 }
 std::int32_t define(const char* name){guard();const auto d=host.database().global_define(name,content::GlobalDefineKind::integer);guard();need(d.found&&d.error.empty()&&std::holds_alternative<std::int64_t>(d.value),"Movement reached cached Define unavailable");return native(std::get<std::int64_t>(d.value));}
 bool crossing()const{
  const auto a=geometry.grid.coordinates(from),b=geometry.grid.coordinates(to);
  const auto wrap=[](std::int32_t n,std::int32_t width,bool yes){if(yes){if(n>width/2)return add(n,-width);if(n<-width/2)return add(n,width);}return n;};
  const auto hx1=native(static_cast<std::int64_t>(a[0])-static_cast<std::int64_t>(a[1]/2));
  const auto hx2=native(static_cast<std::int64_t>(b[0])-static_cast<std::int64_t>(b[1]/2));
  const auto dx=wrap(native(static_cast<std::int64_t>(hx2)-hx1),native(static_cast<std::int64_t>(geometry.grid.width)),geometry.wrap_x);
  const auto dy=wrap(native(static_cast<std::int64_t>(b[1])-static_cast<std::int64_t>(a[1])),native(static_cast<std::int64_t>(geometry.grid.height)),geometry.wrap_y);
  const auto direction=dy>0?(dx>=0?0:5):(dy==0?(dx>0?1:(dx==0?-1:4)):(dx>0?2:3));
  if(direction==-1)return false;const auto bits=plot(from,"river_crossing_bits");need(bits>=INT8_MIN&&bits<=INT8_MAX&&cell(from).river_crossing_bits==bits,"Movement raw/owned native crossing byte differs");return (bits&(1<<direction))!=0;
 }
 std::int32_t route_change(std::int32_t id){
  auto& routes=collection("Routes");need(id>=0&&static_cast<std::uint64_t>(id)<routes.span,"Movement reached route-change native index unavailable");
  const auto& a=named(team_raw(team).hashes,"route_change");need(a.wire_count>=0&&static_cast<std::uint64_t>(a.wire_count)==a.rows.size(),"Movement route-change Read span unavailable");
  std::optional<std::int32_t> result;
  for(const auto& row:a.rows){if(row.type_hash==0){need(row.values.empty(),"Movement route-change null hash carried value");continue;}need(row.type_hash<=UINT32_MAX&&row.values.size()==1,"Movement route-change Read row shape unavailable");const auto* e=host.types().find_hash(row.type_hash);if(!e)continue;need(e->table=="Routes"&&e->id>=0&&static_cast<std::uint64_t>(e->id)<routes.span,"Movement route-change cross-table remap frontier");if(e->id==id)result=native(row.values[0]);}
  need(result.has_value(),"Movement route-change omitted reached slot needs genuine TEAM Reset/Read owner");return *result;
 }
 bool double_move(bool feature){const auto id=feature?hash_id(to,true):terrain();auto& c=collection(feature?"Features":"Terrains");const auto& f=named(u.core.fields,feature?"m_featureDoubleMoveCount":"m_terrainDoubleMoveCount");need(f.encoding==saves::UnitCoreEncoding::vector_i32&&f.wire_count&&*f.wire_count==f.values.size()&&f.values.size()==c.span&&f.bytes.empty()&&id>=0&&static_cast<std::uint64_t>(id)<c.span,"Movement actual double-move native vector span unavailable");return native(f.values.at(static_cast<std::size_t>(id)))>0;}
 bool zoc(){
  const auto& state=named(u.tail.blocks,"state");const auto& f=named(state.children,"m_iIgnoreZOC");need(u.tail.layout==bnw&&f.encoding&&std::string_view(f.encoding)=="i32"&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.children.empty(),"Movement actual IgnoreZOC Read leaf unavailable");
  if(native(f.values[0])>0)return false;
  // CostsOnlyOne was already false on these SAME immutable operands.
  if(define("ZONE_OF_CONTROL_ENABLED")<=0)return false;
  const auto stack=[&](std::uint64_t slot)->const auto&{
   const auto& a=named(raw(slot).arrays,"units");const auto& owned=cell(slot).units;
   need(a.columns==2&&a.values.size()%2==0&&owned&&owned->size()==a.values.size()/2,"Movement actual ordered ZOC stack shape unavailable");
   for(std::size_t j=0;j<owned->size();++j)need((*owned)[j].owner==a.values[j*2]&&(*owned)[j].id==a.values[j*2+1],"Movement actual ordered ZOC stack mirror differs");
   return *owned;
  };
  need(stack(to).empty(),"Movement ZOC reached isVisibleEnemyUnit/actual invisibility owner required");
  const auto xy=geometry.grid.coordinates(from);
  for(std::int32_t d=0;d<6;++d)if(const auto p=geometry.direction(static_cast<std::int64_t>(xy[0]),static_cast<std::int64_t>(xy[1]),d)){
   const auto slot=static_cast<std::uint64_t>(*p);
   need(!city(slot),"Movement ZOC reached enemy-city combat-owner/destination adjacency frontier");
   need(stack(slot).empty(),"Movement ZOC reached unit FLTA/invisibility/combat/war/domain owner required");
  }return false;
 }
 void bind_info(const char* name,std::int32_t id){(void)info(std::string_view(name)=="feature"?"Features":"Terrains",id);}
 void bind_route(bool target){(void)info("Routes",route_id(target?to:from));}
 void bind_road(){const auto* r=info("Routes",0);need(r&&r->identity.type=="ROUTE_ROAD","Movement native ROAD0 mandatory Info unavailable");}
 std::int32_t i(std::string_view n){
  if(n=="denominator")return denominator;if(n=="base")return base;
  if(n=="feature")return hash_id(to,true);if(n=="terrain")return terrain();
  if(n=="terrain_move")return info_value("Terrains",terrain(),"Movement");if(n=="feature_move")return info_value("Features",hash_id(to,true),"Movement");
  if(n=="hill_extra")return define("HILLS_EXTRA_MOVEMENT");if(n=="river_extra")return define("RIVER_EXTRA_MOVEMENT");if(n=="discount")return core(u.core,"m_iExtraMoveDiscount");
  const bool target=n.starts_with("to_");
  if(n=="from_route_move"||n=="to_route_move")return info_value("Routes",route_id(target?to:from),"Movement");
  if(n=="from_route_flat"||n=="to_route_flat")return info_value("Routes",route_id(target?to:from),"FlatMovement");
  if(n=="from_route_change"||n=="to_route_change")return route_change(route_id(target?to:from));
  if(n=="road_move")return info_value("Routes",0,"Movement");if(n=="road_flat")return info_value("Routes",0,"FlatMovement");
  throw BnwPlotMovementCostUnavailable("Movement unknown internal numeric getter");
 }
 bool b(std::string_view n){
  if(n=="faster_river")return trait("m_bFasterAlongRiver");if(n=="faster_hills")return trait("m_bFasterInHills");if(n=="ignore")return core(u.core,"m_iIgnoreTerrainCostCount")>0;
  if(n=="crossing")return crossing();if(n=="river"){const auto count=plot(to,"river_crossing_count");need(count>=INT8_MIN&&count<=INT8_MAX&&cell(to).rivers&&cell(to).rivers->is_river==(count>0),"Movement actual river count mirror differs");return count>0;}
  if(n=="hills")return kind(to)==1;if(n=="city")return city(to);
  if(n=="rough")return kind(to)==1||kind(to)==0||boolean(plot(to,"rough_feature"));if(n=="rough_ends")return core(u.core,"m_iRoughTerrainEndsTurnCount")>0;
  if(n=="hill_double")return core(u.core,"m_iHillsDoubleMoveCount")>0;if(n=="terrain_double")return double_move(false);if(n=="feature_double")return double_move(true);
  if(n=="from_route")return valid_route(from);if(n=="to_route")return valid_route(to);if(n=="bridge")return native(named(team_raw(team).fields,"bridge_building").value)>0;
  if(n=="own_target")return owner(to)==key.first;if(n=="woods_road")return trait("m_bMoveFriendlyWoodsAsRoad");if(n=="has_team")return plot_team(to)!=-1;
  if(n=="same_team")return plot_team(to)==team;if(n=="open_borders")return team_bit(plot_team(to),"open_borders",team);
  if(n=="team_border")return native(named(team_raw(plot_team(to)).fields,"border_obstacle").value)>0;
  if(n=="player_border"){
   const auto& f=named(s.world.all_players->players.at(static_cast<std::size_t>(owner(to))).core.fields,"m_iBorderObstacleCount");need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1,"Movement actual player border count unavailable");return native(f.values[0])>0;
  }
  throw BnwPlotMovementCostUnavailable("Movement unknown internal boolean getter");
 }
};
}
std::int32_t bnw_plot_movement_cost_math(const std::map<std::string,std::int64_t,std::less<>>& fields){return typed([&]{MathReader r{fields};return costs(r);});}
struct BnwPlotMovementCostQuery::Impl{
 const std::thread::id thread=std::this_thread::get_id();std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;const saves::SavedSession* source=nullptr;
 std::function<bool()> rules_current;SessionCheckpointIdentity identity;std::uint64_t generation=0,ui_revision=0,target=0;
 BnwUnitOrderKey key;std::string unit_revision;mutable std::atomic<bool> retired{false};
};
BnwPlotMovementCostQuery::BnwPlotMovementCostQuery(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwPlotMovementCostQuery::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Movement query foreign thread/moved owner");
 try{
  need(!p->retired.load()&&p->host&&p->rules_current&&p->rules_current(),"Movement query immutable rules/Host retired");
  const auto& session=p->host->session();(void)session.bnw_game_debug_mode();
  need(session.read_state_lease()==p->state&&&session.source()==p->source&&session.state_generation()==p->generation&&p->state->ui_source_revision==p->ui_revision,"Movement query SAME state/source/generation/UI revision retired");
  need(p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256&&p->host->database().gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Movement query actual DB/source identity differs");
 }catch(...){p->retired.store(true);throw;}
}
std::int32_t BnwPlotMovementCostQuery::value()const{return typed([&]{
 require_current();const auto& p=*impl_;const auto& s=*p.state;
 need(bnw_unit_order_world_revision(s,p.identity,p.host->database())==p.unit_revision,"Movement query current unit/source operands changed");
 const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(p.key.first));const saves::SavedUnitObservation* u=nullptr;
 for(const auto& candidate:player.units.units)if(core(candidate.core,"m_iID")==p.key.second){need(!u,"Movement duplicate full unit ID");u=&candidate;}
 need(u&&player.saved_player_id==p.key.first&&core(u->core,"m_eOwner")==p.key.first&&u->core.start_offset<u->core.end_offset,"Movement actual full-ID unit unavailable");
 const auto& owned=s.maintenance.players.at(p.key.first).units.at(p.key.second);need(owned.source_core==std::optional{std::pair{u->core.start_offset,u->core.end_offset}},"Movement current unit raw/owned source span differs");
 const auto from=bnw_unit_order_plot(s,p.key);const auto& h=s.world.map_header;const auto& old=p.source->snapshot.map_header;
 need(h.layout==bnw&&h.guid==old.guid&&h.grid.width==old.grid.width&&h.grid.height==old.grid.height&&h.wrap_x==old.wrap_x&&h.wrap_y==old.wrap_y&&h.grid.width<=INT32_MAX&&h.grid.height<=INT32_MAX&&h.grid.size()<=INT32_MAX&&s.world.map_plots&&s.world.map_projection&&s.world.map_plots->layout==bnw&&s.world.map_plots->plots.size()==h.grid.size()&&s.world.map_projection->cells.size()==h.grid.size()&&s.world.map_projection->grid.width==h.grid.width&&s.world.map_projection->grid.height==h.grid.height&&s.world.map_projection->wrap_x==h.wrap_x&&s.world.map_projection->wrap_y==h.wrap_y,"Movement SAME source map/geometry unavailable");
 const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};need(p.target<h.grid.size()&&p.target!=from,"Movement target outside actual distinct map");
 const auto xy=h.grid.coordinates(from);bool neighbor=false;for(std::int32_t d=0;d<6;++d){const auto n=geometry.direction(static_cast<std::int64_t>(xy[0]),static_cast<std::int64_t>(xy[1]),d);if(n&&static_cast<std::uint64_t>(*n)==p.target)neighbor=true;}
 need(neighbor,"Movement target is not native six-direction neighbor");
 const auto quote=quote_bnw_unit_movement(s,p.host->database(),p.host->types(),p.identity,p.key);
 need(quote.base_moves&&quote.max_moves,quote.blocker.empty()?"Movement genuine base/max getters unavailable":quote.blocker.c_str());
 need(quote.terms.domain==2&&!unit_bool(u->core,"m_bEmbarked"),"Movement ordinary unembarked LAND owner required");
 const auto team=detail::bnw_player_team_read(s,native(p.key.first));need(team>=0&&team<64&&quote.terms.team==team,"Movement actual unit Pregame team differs");
 ActualReader r{s,*p.source,*p.host,*u,p.key,from,p.target,team,*quote.base_moves,native(quote.terms.denominator),geometry,[this]{require_current();},{}};
 need(r.kind(p.target)!=3&&r.kind(from)!=3,"Movement destination-water baseMoves/domain-transition owner required");
 // This owner is the ordinary revealed branch. Fog-human maxMoves belongs
 // to the complementary genuine MovePending prefix; fog AI remains explicit.
 need(detail::bnw_plot_revealed_read(s.world,p.target,team),"Movement unrevealed target requires genuine fog-human/AI branch");
 // Native !embarked && (toWalk || fromWalk) precedes domain validity. Both
 // locations are genuine ground LAND, so remaining valid-domain tests return
 // true before hovering/allterrain/embark reads and water transition is false.
 const bool walk=r.walk(p.target)||r.walk(from);(void)walk;
 // Native CostsOnlyOne: valid ground domain, debug immobile assertion only.
 if(core(u->core,"m_iFlatMovementCostCount")>0){require_current();return native(quote.terms.denominator);}
 if(r.zoc()){require_current();return *quote.max_moves;}
 const auto result=costs(r);require_current();need(bnw_unit_order_world_revision(s,p.identity,p.host->database())==p.unit_revision,"Movement actual source changed during read");return result;
});}

std::int32_t BnwNativeUnitPathMovementCost::evaluate(const BnwNativeUnitAllocatorCore&core_owner,const BnwNativeUnitAllocatorCore::ExistingPathMovementCostDispatch&dispatch){return typed([&]{
 const auto a=core_owner.existing_path_movement_cost_operands(dispatch);
 const auto guard=[&]{need(core_owner.existing_path_movement_cost_operands(dispatch)==a,"Native path MovementCost actual node/source/base/max/remaining changed");};
 const auto&h=a.state->world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};
 ActualReader r{*a.state,*a.source,*a.host,*a.unit,a.key,a.from,a.to,a.team,a.base,a.denominator,geometry,guard,{}};
 const auto burn=[&]{guard();return a.remaining>0?a.remaining:a.max_moves;};
 // Original ConsumesAllMoves: the genuine human-unrevealed branch returns
 // before ground validity/walk/route/ZOC reads, with this segment's budget.
 if(!detail::bnw_plot_revealed_read(a.state->world,a.to,a.team)&&a.human)return burn();
 need(r.kind(a.from)!=3&&r.kind(a.to)!=3,"Native path MovementCost actual domain-transition owner unavailable");
 const bool walk=r.walk(a.to)||r.walk(a.from);(void)walk;
 if(core(a.unit->core,"m_iFlatMovementCostCount")>0){guard();return a.denominator;}
 if(core_owner.existing_path_slowed_by_zoc(dispatch))return burn();
 const auto result=costs(r);guard();return result;
});}

BnwPlotMovementCostQuery bind_bnw_plot_movement_cost(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,BnwUnitOrderKey key,std::uint64_t target){return typed([&]{
 need(bool(host),"Movement query genuine selected Host absent");auto p=std::make_shared<BnwPlotMovementCostQuery::Impl>();p->host=std::move(host);
 p->rules_current=p->host->database().immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Movement query actual immutable DB guard absent");p->identity=p->host->identity();
 (void)p->host->session().bnw_game_debug_mode();p->state=p->host->session().read_state_lease();p->source=&p->host->session().source();p->generation=p->host->session().state_generation();
 need(p->state&&key.first>=0&&key.first<64&&key.second>=0&&key.second<=INT32_MAX&&p->state->setup.profile==content::TechnologyProfile::bnw&&identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Movement actual native owner/source/profile unavailable");
 p->ui_revision=p->state->ui_source_revision;p->target=target;p->key=key;p->unit_revision=bnw_unit_order_world_revision(*p->state,p->identity,p->host->database());
 (void)bnw_unit_order_plot(*p->state,key);
 BnwPlotMovementCostQuery result(std::move(p));result.require_current();return result;
});}
std::int32_t inspect_bnw_plot_movement_cost(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,BnwUnitOrderKey key,std::uint64_t target){return bind_bnw_plot_movement_cost(std::move(host),key,target).value();}
}
