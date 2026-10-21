#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_city_current_coastal_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_personality_flavors.hpp"
#include "civ5/runtime/bnw_plot_domain_binding.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/plot_water.hpp"
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
void need(bool b,const char* m){if(!b)throw BnwCityNavalStrategyUnavailable(m);}
std::int64_t wrap_delta(std::int64_t n,std::uint64_t size,bool wrap){need(size&&size<=INT32_MAX,"Plot refresh native map width unavailable");if(wrap){const auto s=static_cast<std::int64_t>(size);if(n>s/2)return n-s;else if(n<-s/2)return n+s;}return n;}
std::int64_t half_floor(std::int64_t n){return n/2-(n<0&&n%2?1:0);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Naval strategy reached signed32 overflow");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(std::int64_t(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return native(std::int64_t(a)*b);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityNavalStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityNavalStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* found=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!found,"Naval duplicate source field");found=&r;}
 need(found,"Naval source field absent");return *found;
}
std::int32_t city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=named(c.core.fields,name);need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Naval native city scalar unavailable");return native(f.values[0]);
}
std::int32_t unit_scalar(const saves::SavedUnitObservation& u,std::string_view name,saves::UnitCoreEncoding e=saves::UnitCoreEncoding::i32){
 const auto& f=named(u.core.fields,name);need(u.core.layout==bnw&&f.encoding==e&&f.values.size()==1&&!f.wire_count&&f.bytes.empty(),"Naval native unit scalar unavailable");const auto n=native(f.values[0]);if(e==saves::UnitCoreEncoding::boolean)need(n==0||n==1,"Naval native unit bool unavailable");return n;
}
const saves::SavedPlayerObservation& player(const LiveSessionState& s,std::int32_t owner){
 need(owner>=0&&owner<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&s.world.all_players->players.size()==64,"Naval native player registry absent");
 const auto& q=s.world.all_players->players.at(static_cast<std::size_t>(owner));const auto& id=named(q.core.fields,"m_eID");need(q.index==static_cast<std::uint64_t>(owner)&&q.saved_player_id==owner&&q.core.layout==bnw&&id.encoding==saves::PlayerCoreEncoding::i32&&id.values.size()==1&&native(id.values[0])==owner&&!id.wire_count&&id.bytes.empty()&&id.hashes.empty()&&id.row_counts.empty(),"Naval actual player identity differs");return q;
}
std::vector<const saves::SavedUnitObservation*> unit_roster(const saves::SavedSessionSnapshot& world,std::int32_t owner){
 need(world.all_players&&world.all_players->layout==bnw&&world.all_players->context.player_count==64&&world.all_players->players.size()==64,"Naval native unit owner registry absent");const auto& p=world.all_players->players.at(static_cast<std::size_t>(owner));
 need(p.index==static_cast<std::uint64_t>(owner)&&p.saved_player_id==owner,"Naval unit owner identity differs");const auto& units=p.units;const auto& h=units.header;
 need(units.layout==bnw&&h.layout==bnw&&h.fields.size()==5&&h.wire_unit_count>=0&&static_cast<std::uint64_t>(h.wire_unit_count)==units.units.size(),"Naval unit FLTA wire count differs");
 const auto slots=native(named(h.fields,"num_slots").value),last=native(named(h.fields,"last_index").value),head=native(named(h.fields,"free_list_head").value),count=native(named(h.fields,"free_list_count").value),serial=native(named(h.fields,"current_id").value);
 need(slots>=0&&slots<=8192&&h.source_slots==static_cast<std::uint64_t>(slots)&&h.next_free_indices.size()==h.source_slots&&last>=-1&&last<slots&&count>=0&&count<=std::int64_t(last)+1&&serial>=8192&&serial%8192==0&&((count==0&&head==-1)||(count>0&&head>=0&&head<=last)),"Naval native unit FLTA dimensions unavailable");
 std::set<std::int32_t> occupied,freed;std::int32_t previous=-1;std::vector<const saves::SavedUnitObservation*> result;
 for(std::size_t i=0;i<units.units.size();++i){const auto& u=units.units[i];const auto id=unit_scalar(u,"m_iID"),slot=id&8191;
  need(u.index==i&&id>=0&&slot>previous&&slot<=last&&(id&~8191)<serial&&h.next_free_indices.at(static_cast<std::size_t>(slot))==-1&&occupied.insert(slot).second&&unit_scalar(u,"m_eOwner")==owner,"Naval native unit FLTA slot order/owner differs");previous=slot;result.push_back(&u);
 }
 auto next=head;for(std::int32_t i=0;i<count;++i){need(next>=0&&next<=last&&!occupied.contains(next)&&freed.insert(next).second,"Naval native unit free chain invalid");next=native(h.next_free_indices.at(static_cast<std::size_t>(next)));}
 need(next==-1&&occupied.size()+freed.size()==static_cast<std::size_t>(std::int64_t(last)+1),"Naval unit FLTA partition differs");return result;
}
std::optional<std::int64_t> xy(const MapGeometry& g,std::int32_t x,std::int32_t y,std::int32_t dx,std::int32_t dy){
 // Native xToHexspace/yToHexspace/hexspaceToX primitive stages BEFORE
 // MapGeometry's intentionally widened arithmetic. SDK odd negative rows
 // use (y-1)/2 for negative rows, with that exact source subtraction
 // checked independently, including INT32_MIN even though wide math fits.
 const auto half=[&](std::int32_t row){return row>=0?row/2:native(std::int64_t(row)-1)/2;};
 const auto hex=native(std::int64_t(x)-half(y));const auto hx=add(hex,dx),ny=add(y,dy);const auto nx=add(hx,half(ny));return g.plot(nx,ny);
}
constexpr auto city_dx=std::to_array<std::int32_t>({0,0,1,1,0,-1,-1,0,1,2,2,2,1,0,-1,-2,-2,-2,-1,0,1,2,3,3,3,3,2,1,0,-1,-2,-3,-3,-3,-3,-2,-1});
constexpr auto city_dy=std::to_array<std::int32_t>({0,1,0,-1,-1,0,1,2,1,0,-1,-2,-2,-2,-1,0,1,2,2,3,2,1,0,-1,-2,-3,-3,-3,-3,-2,-1,0,1,2,3,3,3});
static_assert(city_dx.size()==37&&city_dy.size()==37,"Native city plot tables must contain all37 explicit entries");
struct Read {
 const LiveSessionState& s;const saves::SavedSession& source;const ui::SelectedBnwPreviewHost& host;
 const saves::SavedCityObservation& city;std::int32_t owner,id;MapGeometry geometry;
 const auto& cell(std::uint64_t index)const{return s.world.map_projection->cells.at(static_cast<std::size_t>(index));}
 const auto& raw(std::uint64_t index)const{
  need(s.world.map_plots&&s.world.map_projection&&source.snapshot.map_plots&&index<geometry.grid.size()&&s.world.map_plots->plots.size()==geometry.grid.size()&&s.world.map_projection->cells.size()==geometry.grid.size()&&source.snapshot.map_plots->plots.size()==geometry.grid.size(),"Naval current map plot shape absent");const auto& p=s.world.map_plots->plots.at(static_cast<std::size_t>(index));const auto& c=cell(index);const auto& old=source.snapshot.map_plots->plots.at(static_cast<std::size_t>(index));const auto pos=geometry.grid.coordinates(index);
  need(p.index==index&&c.index==index&&p.x==pos[0]&&p.y==pos[1]&&c.x==pos[0]&&c.y==pos[1]&&p.version&&*p.version>=1&&*p.version<=7&&p.version==old.version&&p.start_offset==old.start_offset&&p.end_offset==old.end_offset,"Naval plot native identity/span differs");
  for(const auto field:{"source_x","source_y"}){const auto n=named(p.fields,field).value;const auto v=std::string_view(field)=="source_x"?pos[0]:pos[1];need(n>=0&&n<=INT16_MAX&&static_cast<std::uint64_t>(n)==v,"Naval source plot short coordinate differs");}return p;
 }
 bool water(std::uint64_t index)const{const auto n=named(raw(index).fields,"plot_type").value;need(n>=INT8_MIN&&n<=INT8_MAX&&cell(index).plot_type==std::optional<std::int64_t>{n},"Naval raw/projected plot category differs");return n==3;}
 std::int32_t plot_owner(std::uint64_t index)const{const auto n=native(named(raw(index).fields,"owner").value);need(cell(index).owner==std::optional<std::int64_t>{n},"Naval raw/projected plot owner differs");return n;}
 std::int32_t team(std::int32_t who)const{
  (void)player(s,who);need(s.world.pregame&&s.world.pregame->layout==bnw,"Naval native Pregame absent");const auto& a=s.world.pregame->archive.integer_arrays.at("team_types");need(a.size()==64,"Naval native Pregame team dimension differs");const auto t=native(a.at(static_cast<std::size_t>(who)));need(t>=0&&t<64&&s.setup.profile==content::TechnologyProfile::bnw&&s.setup.players.at(who).team==t,"Naval raw/owned player TEAM differs");return t;
 }
 const saves::TeamObservation& team_record(std::int32_t t)const{
  need(t>=0&&t<64&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->dimensions.civ_teams==63&&s.world.teams->teams.size()==64,"Naval native TEAM registry absent");const auto& q=s.world.teams->teams.at(static_cast<std::size_t>(t));need(q.index==static_cast<std::uint64_t>(t)&&native(named(q.fields,"id").value)==t,"Naval native TEAM identity differs");return q;
 }
 bool war(std::int32_t from,std::int32_t to)const{const auto& a=named(team_record(from).arrays,"at_war");need(to>=0&&to<64&&a.columns==1&&a.values.size()==64,"Naval native TEAM war bool span differs");const auto n=a.values.at(static_cast<std::size_t>(to));need(n==0||n==1,"Naval native war bool unavailable");return n!=0;}
 bool city_plot(std::uint64_t index)const{
  const auto& p=raw(index);const auto po=native(named(p.fields,"plot_city_owner").value),pc=native(named(p.fields,"plot_city_id").value);need(cell(index).plot_city&&cell(index).plot_city->owner==po&&cell(index).plot_city->id==pc,"Naval raw/projected plot city differs");if(po==-1)return false;need(po>=0&&po<64,"Naval native plot-city owner invalid");return resolve_bnw_city_registry(s.world,s.cities,po,pc)!=nullptr;
 }
 bool lake(std::uint64_t index)const{
  const auto handle=native(named(raw(index).fields,"landmass").value);need(cell(index).landmass==std::optional<std::int64_t>{handle},"Naval lake raw/projected landmass differs");
  const auto* mass=detail::bnw_coastal_landmass_lookup(s.world,handle);if(!mass)return false;
  const auto ident=native(named(mass->fields,"id").value),flag=native(named(mass->fields,"water").value);need(flag==0||flag==1,"Naval lake native landmass bool invalid");if(!flag)return false;const auto tiles=native(named(mass->fields,"tiles").value);need(tiles>=0,"Naval lake native tile count invalid");
  const auto* d=host.defines().find("LAKE_MAX_AREA_SIZE");need(d&&host.defines().profile()==content::GlobalDefineProfile::bnw&&d->kind==content::GlobalDefineKind::integer&&d->lookup.found&&d->lookup.error.empty()&&d->lookup.sqlite_code==0&&std::holds_alternative<std::int64_t>(d->lookup.value),"Naval actual lake Define unavailable");const auto maximum=native(std::get<std::int64_t>(d->lookup.value));
  const auto db=host.database().global_define("LAKE_MAX_AREA_SIZE",content::GlobalDefineKind::integer);need(db.found&&db.error.empty()&&db.sqlite_code==0&&std::holds_alternative<std::int64_t>(db.value)&&std::get<std::int64_t>(db.value)==maximum,"Naval current/cached lake Define differs");
  auto map=*s.world.map_projection;map.cells.at(static_cast<std::size_t>(index)).landmass=ident;PlotWaterState state;state.layout=bnw;state.landmasses.emplace(ident,PlotWaterLandmass{tiles,flag!=0});const auto rules=load_plot_water_rules(host.database(),content::TechnologyProfile::bnw);const auto& pass=bnw_live_plot_impassability(s);need(s.plot_impassability.rules_sha256==host.identity().rules_sha256,"Naval lake helper actual rules differ");PlotWaterContext context{map,state,rules,host.types(),pass,s.setup,s.cities,host.defines()};return plot_is_lake(context,index);
 }
 const content::InfoTypeEntry* plot_info(std::uint64_t index,const char* stem,const char* table,const std::optional<std::int64_t>& projected)const{
  const auto& p=raw(index);const content::InfoTypeEntry* found=nullptr;std::int32_t expected=-1;
  if(std::string_view(stem)=="resource"||*p.version>=5){const auto n=named(p.fields,std::string(stem)+"_type_hash").value;need(n>=0&&n<=UINT32_MAX,"Naval plot native Info hash width invalid");if(n)found=host.types().find_hash(static_cast<std::uint64_t>(n));}
  else{const auto n=native(named(p.fields,std::string(stem)+"_type").value);if(n!=-1){for(const auto& [key,info]:host.types().entries()){(void)key;if(info.table==table&&info.id==n){need(!found,"Naval native Info ID ambiguous");found=&info;}}}}
  if(found){need(found->table==table&&found->id>=0&&found->id<=INT32_MAX&&detail::bnw_naval_info_same_identity(host.types().find(found->type),found),"Naval plot Info hash table/ID differs");expected=native(found->id);}
  need((expected==-1&&(!projected||*projected==-1))||(projected&&*projected==expected),"Naval raw/owned plot Info resolution differs");return found;
 }
 const saves::SavedUnitObservation* unit(saves::SavedPlotUnitReference ref)const{
  const auto* u=detail::bnw_naval_unit_lookup(s.world,ref.owner,ref.id);if(!u)return nullptr;
  const auto who=unit_scalar(*u,"m_eOwner"),full=unit_scalar(*u,"m_iID");need(source.snapshot.all_players&&source.snapshot.all_players->players.size()==64,"Naval original unit source absent");const auto* old=detail::bnw_naval_unit_lookup(source.snapshot,who,full);need(old&&u->core.version==old->core.version&&u->core.start_offset==old->core.start_offset&&u->core.end_offset==old->core.end_offset,"Naval current unit original identity/span differs");return u;
 }
 std::optional<std::int64_t> unit_plot(const saves::SavedUnitObservation& u)const{return geometry.plot(unit_scalar(u,"m_iX"),unit_scalar(u,"m_iY"));}
 std::int32_t domain(const saves::SavedUnitObservation& u)const{
  const auto type=unit_scalar(u,"m_eUnitType");const auto& hash=named(u.core.fields,"idUnitType");need(hash.encoding==saves::UnitCoreEncoding::hash32&&hash.values.size()==1&&hash.values[0]>0&&hash.values[0]<=UINT32_MAX&&!hash.wire_count&&hash.bytes.empty(),"Naval actual UnitInfo hash unavailable");const auto* info=host.types().find_hash(static_cast<std::uint64_t>(hash.values[0]));need(info&&info->table=="Units"&&info->id==type,"Naval raw UnitInfo native ID/hash differs");
  const auto rules=load_bnw_plot_domain_rules(host.database(),host.types());const auto i=rules.units.find(type);need(i!=rules.units.end()&&i->second.hash==info->hash,"Naval actual Unit Domain rule absent");const auto d=i->second.domain;
  const char* source_type=d==BnwUnitDomain::sea?"DOMAIN_SEA":d==BnwUnitDomain::air?"DOMAIN_AIR":d==BnwUnitDomain::land?"DOMAIN_LAND":d==BnwUnitDomain::immobile?"DOMAIN_IMMOBILE":"DOMAIN_HOVER";const auto expected=d==BnwUnitDomain::sea?0:d==BnwUnitDomain::air?1:d==BnwUnitDomain::land?2:d==BnwUnitDomain::immobile?3:4;const auto* domain_info=host.types().find(source_type);need(domain_info&&domain_info->table=="Domains"&&domain_info->id==expected,"Naval native fixed Domain enum mapping differs");
  if(d==BnwUnitDomain::hover){const auto at=unit_plot(u);if(at)return water(static_cast<std::uint64_t>(*at))?0:2;return 4;}
  if(d==BnwUnitDomain::sea)return 0;if(d==BnwUnitDomain::air)return 1;if(d==BnwUnitDomain::land)return 2;if(d==BnwUnitDomain::immobile)return 3;throw BnwCityNavalStrategyUnavailable("Naval native domain unknown");
 }
 bool valid_action(const saves::SavedUnitObservation& u,std::uint64_t at)const{
  const auto d=domain(u);if(d==0)return water(at)||unit_scalar(u,"m_iCanMoveAllTerrainCount")>0;if(d==1)return false;
  if(d==2||d==3){if(!water(at)||unit_scalar(u,"m_iHoveringUnitCount")>0||unit_scalar(u,"m_iCanMoveAllTerrainCount")>0||unit_scalar(u,"m_bEmbarked",saves::UnitCoreEncoding::boolean)!=0)return true;const auto* i=plot_info(at,"improvement","Improvements",cell(at).improvement_type);if(!i)return false;const auto rules=load_bnw_plot_domain_rules(host.database(),host.types());const auto it=rules.improvements.find(i->id);need(it!=rules.improvements.end()&&it->second.hash==i->hash,"Naval walk-water actual ImprovementInfo absent");return it->second.allows_walk_water;}
  return false;
 }
 std::int32_t base_defense(const saves::SavedUnitObservation& u)const{
  if(!unit_scalar(u,"m_bEmbarked",saves::UnitCoreEncoding::boolean))return unit_scalar(u,"m_iBaseCombat");const auto t=team(unit_scalar(u,"m_eOwner"));const auto era=native(named(team_record(t).fields,"era").value);const auto it=s.maintenance.team_eras.find(t);need(it!=s.maintenance.team_eras.end()&&it->second==era,"Naval current TEAM era raw/owned differs");
  const content::InfoTypeEntry* e=nullptr;for(const auto& [key,i]:host.types().entries()){(void)key;if(i.table=="Eras"&&i.id==era){need(!e,"Naval native era ID ambiguous");e=&i;}}need(e,"Naval embarked defense EraInfo absent");std::optional<std::int32_t> defense;const auto report=host.database().query_rows({"SELECT ID,Type,EmbarkedUnitDefense FROM Eras WHERE ID=?",{era}},[&](const auto& row){need(!defense&&row.integer("ID")==era&&row.text("Type")==e->type,"Naval embarked EraInfo binding differs");defense=native(row.integer("EmbarkedUnitDefense"));},2);need(report.all_succeeded()&&report.rows==1&&defense,"Naval embarked defense actual row absent");
  std::int32_t amount=mul(*defense,100);need(u.tail.layout==bnw&&u.tail.context.bnw_unit_version==u.core.version,"Naval embarked tail schema differs");const auto& tail=named(u.tail.blocks,"state");const auto& modifier=named(tail.children,"m_iEmbarkDefensiveModifier");need(modifier.encoding&&std::string_view(modifier.encoding)=="i32"&&modifier.values.size()==1&&!modifier.wire_count&&modifier.bytes.empty()&&modifier.children.empty(),"Naval native embark modifier absent");const auto m=native(modifier.values[0]);if(m>0)amount=mul(amount,add(100,m))/100;return amount/100;
 }
 bool can_siege(const saves::SavedUnitObservation& u)const{
  const auto at=unit_plot(u);if(base_defense(u)==0)return false;if(unit_scalar(u,"m_bDeathDelay",saves::UnitCoreEncoding::boolean))return false;need(at.has_value(),"Naval IsCanDefend native null unit plot dereference unavailable");if(!valid_action(u,static_cast<std::uint64_t>(*at)))return false;
  const auto enemy_team=team(owner),unit_owner=unit_scalar(u,"m_eOwner"),own_team=team(unit_owner);std::int32_t combat_owner=unit_owner;
  if(enemy_team!=own_team&&enemy_team!=63&&!city_plot(static_cast<std::uint64_t>(*at))&&unit_scalar(u,"m_iAlwaysHostileCount")>0)combat_owner=63;
  if(!war(team(combat_owner),enemy_team))return false;return unit_scalar(u,"m_eInvisibleType")==-1;
 }
 const auto& stack(std::uint64_t index)const{
  const auto& a=named(raw(index).arrays,"units");need(a.columns==2&&a.values.size()%2==0&&a.values.size()/2<=INT32_MAX&&cell(index).units&&cell(index).units->size()==a.values.size()/2,"Naval actual native stack shape differs");
  for(std::size_t i=0;i<cell(index).units->size();++i){const auto& ref=cell(index).units->at(i);need(ref.owner==native(a.values[2*i])&&ref.id==native(a.values[2*i+1]),"Naval raw/projected ordered unit handles differ");}return *cell(index).units;
 }
 bool actual_enemy(std::uint64_t index)const{const auto own_team=team(owner);for(const auto ref:stack(index)){const auto* u=unit(ref);if(!u)continue;if(war(own_team,team(unit_scalar(*u,"m_eOwner")))&&unit_scalar(*u,"m_iBaseCombat")>0)return true;}return false;}
 bool blockaded(std::uint64_t index)const{
  const auto& p=raw(index);const auto* d=host.defines().find("NAVAL_PLOT_BLOCKADE_RANGE");need(d&&host.defines().profile()==content::GlobalDefineProfile::bnw&&d->kind==content::GlobalDefineKind::integer&&d->lookup.found&&d->lookup.error.empty()&&d->lookup.sqlite_code==0&&std::holds_alternative<std::int64_t>(d->lookup.value),"Naval blockade Define absent");const auto range=native(std::get<std::int64_t>(d->lookup.value));const auto db=host.database().global_define("NAVAL_PLOT_BLOCKADE_RANGE",content::GlobalDefineKind::integer);need(db.found&&db.error.empty()&&db.sqlite_code==0&&std::holds_alternative<std::int64_t>(db.value)&&std::get<std::int64_t>(db.value)==range,"Naval cached/current blockade Define differs");
  need(range!=INT32_MIN,"Naval native blockade range negation overflow");if(range<0)return false;need(range<=64,"Naval blockade scan exceeds bounded source work budget");const auto x=native(static_cast<std::int64_t>(p.x)),y=native(static_cast<std::int64_t>(p.y));
  for(std::int32_t a=-range;a<=range;++a){for(std::int32_t b=-range;b<=range;++b){const auto near=xy(geometry,x,y,a,b);if(!near)continue;const auto n=static_cast<std::uint64_t>(*near);if(!water(n))continue;if(native(named(raw(n).fields,"area").value)!=native(named(p.fields,"area").value))continue;const auto& pos=raw(n);const auto distance=geometry.distance(static_cast<std::int64_t>(pos.x),static_cast<std::int64_t>(pos.y),x,y);(void)native(distance);if(distance<=range&&actual_enemy(n))return true;}}
  return false;
 }
 bool can_work(std::uint64_t index)const{
  const auto& p=raw(index);const auto po=native(named(p.fields,"working_city_owner").value),pc=native(named(p.fields,"working_city_id").value);need(cell(index).yield_city&&cell(index).yield_city->owner==po&&cell(index).yield_city->id==pc,"Naval actual working-city raw/projected handle differs");if(po==-1)return false;need(po>=0&&po<64,"Naval native working city owner invalid");if(resolve_bnw_city_registry(s.world,s.cities,po,pc)!=&city)return false;
  for(const auto ref:stack(index)){const auto* u=unit(ref);if(u&&can_siege(*u))return false;}
  if(water(index)&&native(named(team_record(team(owner)).fields,"water_work").value)<=0)return false;
  const auto& a=named(p.arrays,"yields");need(a.columns==1&&a.values.size()==6&&cell(index).cached_yields&&cell(index).cached_yields->size()==6,"Naval actual plot yield short span absent");bool any=false;
  for(std::int32_t yield=0;yield<6;++yield){const auto n=a.values.at(static_cast<std::size_t>(yield));need(n>=INT16_MIN&&n<=INT16_MAX&&cell(index).cached_yields->at(yield)==n,"Naval raw/owned cached plot short differs");if(n>0){any=true;break;}}
  if(!any)return false;if(blockaded(index))return false;return true;
 }
 std::array<std::optional<std::int64_t>,37> plots()const{return detail::bnw_citizen_plot_positions(geometry,city_scalar(city,"m_iX"),city_scalar(city,"m_iY"));}
 const content::InfoTypeEntry* info(const char* type,const char* table)const{const auto* i=host.types().find(type);if(!i)return nullptr;need(i->table==table&&i->id>=0&&i->id<=INT32_MAX&&detail::bnw_naval_info_same_identity(host.types().find_hash(i->hash),i),"Naval activated native Info mapping differs");return i;}
 bool growth()const{
  std::int32_t ocean=0,total=0;for(const auto at:plots()){if(!at)continue;const auto n=static_cast<std::uint64_t>(*at);if(plot_owner(n)!=owner||city_plot(n))continue;total=add(total,1);if(water(n)&&!lake(n))ocean=add(ocean,1);}if(total<=0)return false;
  const auto* strategy=info("AICITYSTRATEGY_NEED_NAVAL_GROWTH","AICityStrategies");need(strategy,"Naval growth native StrategyInfo absent");std::optional<std::int32_t> threshold;const auto report=host.database().query_rows({"SELECT ID,Type,WeightThreshold FROM AICityStrategies WHERE ID=?",{strategy->id}},[&](const auto& row){need(!threshold&&row.integer("ID")==strategy->id&&row.text("Type")==strategy->type,"Naval growth StrategyInfo row differs");threshold=native(row.integer("WeightThreshold"));},2);need(report.all_succeeded()&&report.rows==1&&threshold,"Naval growth actual weight threshold absent");
  const auto& q=player(s,owner);const auto& personality=checked_bnw_personality_flavors(q,q,host.database());std::vector<std::int64_t> modifiers(personality.size(),0);
  const auto rows=host.database().query_rows({"SELECT Flavors.ID AS FlavorID, Flavor FROM AICityStrategy_PersonalityFlavorThresholdMods INNER JOIN Flavors ON Flavors.Type=FlavorType WHERE AICityStrategyType=?",{strategy->type}},[&](const auto& row){const auto slot=native(row.integer("FlavorID"));need(slot>=0&&static_cast<std::size_t>(slot)<modifiers.size(),"Naval growth flavor native slot unavailable");modifiers.at(static_cast<std::size_t>(slot))=native(row.integer("Flavor"));},1024*1024);need(rows.all_succeeded(),"Naval growth actual threshold flavor query failed");return detail::bnw_naval_growth_threshold_math(ocean,total,personality,modifiers,*threshold);
 }
 bool enough()const{
  const auto* strategy=info("AICITYSTRATEGY_NEED_NAVAL_TILE_IMPROVEMENT","AICityStrategies");if(!strategy)return false;need(city.management.layout==bnw,"Naval native strategy management absent");const auto& block=named(city.management.blocks,"strategy");need(named(block.fields,"version").value==1,"Naval native strategy schema differs");const auto& a=named(block.hashes,"m_pabUsingCityStrategy");need(a.wire_count>=0&&a.wire_count<=UINT32_MAX&&static_cast<std::uint64_t>(a.wire_count)==a.rows.size(),"Naval using-strategy hash count differs");std::optional<bool> used;
  for(const auto& row:a.rows){need(row.type_hash<=UINT32_MAX,"Naval native strategy hash width differs");if(!row.type_hash){need(row.values.empty(),"Naval null strategy hash payload invalid");continue;}need(row.values.size()==1&&(row.values[0]==0||row.values[0]==1),"Naval native using-strategy bool invalid");const auto* i=host.types().find_hash(row.type_hash);if(!i)continue;need(i->table=="AICityStrategies","Naval native strategy hash maps wrong table");if(i->id==strategy->id)used=row.values[0]!=0;}
  need(used.has_value(),"Naval missing using slot requires native strategy Reset/Read owner");return !*used;
 }
 std::int32_t units(BnwCityNativeUnitCount selector)const{
  need(selector>=BnwCityNativeUnitCount::workers&&selector<=BnwCityNativeUnitCount::settlers_including_water,"Native unit counter selector unavailable");
  const bool sea=selector==BnwCityNativeUnitCount::sea_workers;const bool water=sea||selector==BnwCityNativeUnitCount::settlers_including_water;const std::int32_t requested=sea?17:(selector==BnwCityNativeUnitCount::workers?2:1);
  const auto* ai=info(sea?"UNITAI_WORKER_SEA":(requested==2?"UNITAI_WORKER":"UNITAI_SETTLE"),"UnitAIInfos");need(ai&&ai->id==requested,"Native UnitAI enum differs");std::int32_t count=0;
  for(const auto* u:unit_roster(s.world,owner)){const auto d=domain(*u);if(d!=0||water)if(unit_scalar(*u,"m_eUnitAIType")==requested)count=add(count,1);}
  const auto& q=player(s,owner);std::int32_t previous=-1;
  for(const auto& c:q.cities.cities){const auto full=city_scalar(c,"m_iID"),slot=full&8191;need(slot>previous&&resolve_bnw_city_registry(s.world,s.cities,owner,full)==&c,"Naval current city native iteration order differs");previous=slot;need(c.production.layout==bnw&&c.production.context.bnw_city_version,"Naval training queue native schema absent");const auto& queue=named(c.production.blocks,"queue");const auto n=named(queue.fields,"wire_count").value;need(n>=0&&n<=UINT32_MAX&&static_cast<std::uint64_t>(n)==queue.children.size(),"Naval training queue wire count differs");const auto& orders=s.cities.players.at(owner).cities.at(full).production_orders;need(orders&&orders->size()==queue.children.size(),"Naval raw/owned training queue differs");if(n==0)continue;
   const auto& head=queue.children.front();const auto& order=orders->front();need(head.index==0&&order.type==native(named(head.fields,"eOrderType").value),"Naval training head native kind differs");if(order.type!=0)continue;
   const content::InfoTypeEntry* entry=nullptr;const auto version=*c.production.context.bnw_city_version;
   if(version>=5){const auto hash=named(head.fields,"iData1_hash").value;need(hash>=0&&hash<=UINT32_MAX&&order.source_hash==std::optional{static_cast<std::uint32_t>(hash)},"Naval training head raw/owned Unit hash differs");if(hash)entry=host.types().find_hash(static_cast<std::uint64_t>(hash));}
   else{const auto item=native(named(head.fields,"iData1").value);need(!order.source_hash&&order.item==item,"Naval training head raw/owned Unit ID differs");for(const auto& [key,i]:host.types().entries()){(void)key;if(i.table=="Units"&&i.id==item){need(!entry,"Naval training UnitInfo ID ambiguous");entry=&i;}}}
   const auto expected=entry?native(entry->id):-1;need(order.item==expected&&(!entry||entry->table=="Units"),"Naval training current UnitInfo resolution differs");if(!entry)continue;
   std::optional<std::string> default_ai;std::int32_t unit_domain=-1;bool seen=false;const auto report=host.database().query_rows({"SELECT ID,Type,Domain,DefaultUnitAI FROM Units WHERE ID=?",{entry->id}},[&](const auto& row){need(!seen&&row.integer("ID")==entry->id&&row.text("Type")==entry->type,"Naval training UnitInfo row differs");seen=true;const auto d=row.text("Domain");need(d&&info(d->c_str(),"Domains"),"Naval training native Domain Info absent");unit_domain=native(info(d->c_str(),"Domains")->id);need(unit_domain>=0&&unit_domain<=4,"Native training Domain enum differs");if(unit_domain!=0||water)default_ai=row.text("DefaultUnitAI");},2);need(report.all_succeeded()&&report.rows==1&&seen,"Naval training actual UnitInfo query failed");const auto* kind=default_ai?host.types().find(*default_ai):nullptr;if(kind){need(kind->table=="UnitAIInfos","Naval training DefaultUnitAI table differs");if(kind->id==requested)count=add(count,1);}
  }return count;
 }
 std::int32_t workers()const{return units(BnwCityNativeUnitCount::sea_workers);}
 bool need_improvement()const{
  std::int32_t resources=0;for(const auto at:plots()){if(!at)continue;const auto n=static_cast<std::uint64_t>(*at);if(plot_owner(n)!=owner||!water(n)||!can_work(n))continue;if(plot_info(n,"resource","Resources",cell(n).resource_type)&&!plot_info(n,"improvement","Improvements",cell(n).improvement_type))resources=add(resources,1);}const auto count=workers();return resources>count;
 }
 bool value(BnwNavalCityStrategy t)const{
  if(t==BnwNavalCityStrategy::under_blockade){const auto p=plots();for(std::size_t i=1;i<p.size();++i)if(p[i]&&blockaded(static_cast<std::uint64_t>(*p[i])))return true;return false;}
  if(t==BnwNavalCityStrategy::need_naval_growth)return growth();if(t==BnwNavalCityStrategy::need_naval_tile_improvement)return need_improvement();if(t==BnwNavalCityStrategy::enough_naval_tile_improvement)return enough();throw BnwCityNavalStrategyUnavailable("Naval strategy outside four-trigger scope");
 }
};
}
bool detail::bnw_naval_info_same_identity(const content::InfoTypeEntry* a,const content::InfoTypeEntry* b)noexcept{
 return a&&b&&a->type==b->type&&a->table==b->table&&a->id==b->id&&a->hash==b->hash;
}
std::int32_t detail::bnw_citizen_plot_slot(const saves::SavedMapProjection& map,std::int32_t cx,std::int32_t cy,std::int32_t px,std::int32_t py){
 const auto wx=wrap_delta(std::int64_t(px)-cx,map.grid.width,map.wrap_x),dy=wrap_delta(std::int64_t(py)-cy,map.grid.height,map.wrap_y);
 const auto dx=wrap_delta((std::int64_t(cx)+wx-half_floor(std::int64_t(cy)+dy))-(std::int64_t(cx)-half_floor(cy)),map.grid.width,map.wrap_x);
 for(int i=0;i<37;++i){if(city_dx[static_cast<std::size_t>(i)]==dx&&city_dy[static_cast<std::size_t>(i)]==dy)return i;}
 return -1;
}
std::array<std::optional<std::int64_t>,37> detail::bnw_citizen_plot_positions(const MapGeometry& g,std::int64_t x,std::int64_t y){
 g.validate();need(g.layout==bnw&&g.grid.width<=INT16_MAX&&g.grid.height<=INT16_MAX,"Naval city geometry native short scope exceeded");const auto nx=native(x),ny=native(y);std::array<std::optional<std::int64_t>,37> result;for(std::size_t i=0;i<result.size();++i)result[i]=xy(g,nx,ny,city_dx[i],city_dy[i]);return result;
}
const saves::SavedUnitObservation* detail::bnw_naval_unit_lookup(const saves::SavedSessionSnapshot& world,std::int64_t owner,std::int64_t handle){
 const auto who=native(owner),id=native(handle);if(who<0||who>=64||id==-1)return nullptr;for(const auto* u:unit_roster(world,who)){const auto full=unit_scalar(*u,"m_iID");if((full&8191)==(id&8191)&&((id&~8191)==0||id==full))return u;}return nullptr;
}
bool detail::bnw_naval_growth_threshold_math(std::int64_t ocean,std::int64_t total,std::span<const std::int64_t> personality,std::span<const std::int64_t> modifiers,std::int64_t threshold){
 const auto n=native(total);if(n<=0)return false;need(personality.size()==modifiers.size()&&personality.size()<=INT32_MAX,"Naval growth flavor span differs");std::int32_t sum=0;for(std::size_t i=0;i<personality.size();++i)sum=add(sum,mul(native(personality[i]),native(modifiers[i])));const auto target=add(native(threshold),sum);return mul(native(ocean),100)/n>=target;
}
struct BnwCityCurrentNavalStrategy::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::shared_ptr<const ui::SelectedBnwPreviewHost> host;std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;std::shared_ptr<const LiveSessionState> state;const saves::SavedSession* source=nullptr;
 std::uint64_t generation=0;SessionCheckpointIdentity identity;std::function<bool()> rules_current;std::int32_t owner=-1,city=-1;
 std::pair<std::uint64_t,std::uint64_t> city_span;mutable std::atomic<bool> retired{false};
};
BnwCityCurrentNavalStrategy::BnwCityCurrentNavalStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentNavalStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Naval strategy foreign constructor thread/moved query");
 try{need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Naval strategy genuine rules/host retired");need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Naval strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode();need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,"Naval strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
namespace {
Read reader(const LiveSessionState& s,const saves::SavedSession& source,const ui::SelectedBnwPreviewHost& host,std::int32_t owner,std::int32_t full,std::pair<std::uint64_t,std::uint64_t> span){
 const auto* c=resolve_bnw_city_registry(s.world,s.cities,owner,full);need(c&&city_scalar(*c,"m_eOwner")==owner&&city_scalar(*c,"m_iID")==full&&std::pair{c->core.start_offset,c->core.end_offset}==span,"Naval canonical city/span differs");const auto x=city_scalar(*c,"m_iX"),y=city_scalar(*c,"m_iY");const auto& owned=s.cities.players.at(owner).cities.at(full);need(owned.x==x&&owned.y==y,"Naval raw/owned city position differs");
 const auto& h=s.world.map_header;const auto& old=source.snapshot.map_header;need(h.layout==bnw&&old.layout==bnw&&h.grid.width==old.grid.width&&h.grid.height==old.grid.height&&h.wrap_x==old.wrap_x&&h.wrap_y==old.wrap_y&&h.guid==old.guid&&h.grid.width>0&&h.grid.width<=INT16_MAX&&h.grid.height>0&&h.grid.height<=INT16_MAX&&x>=0&&y>=0&&static_cast<std::uint64_t>(x)<h.grid.width&&static_cast<std::uint64_t>(y)<h.grid.height&&s.world.map_plots&&s.world.map_plots->layout==bnw&&source.snapshot.map_plots&&source.snapshot.map_plots->layout==bnw&&s.world.map_projection&&s.world.map_projection->grid.width==h.grid.width&&s.world.map_projection->grid.height==h.grid.height&&s.world.map_projection->wrap_x==h.wrap_x&&s.world.map_projection->wrap_y==h.wrap_y,"Naval SAME current native map identity/bounds differs");return {s,source,host,*c,owner,full,{h.grid,h.wrap_x,h.wrap_y,bnw}};
}
}
bool BnwCityCurrentNavalStrategy::value(BnwNavalCityStrategy trigger)const{return typed([&]{
 require_current();need(trigger>=BnwNavalCityStrategy::under_blockade&&trigger<=BnwNavalCityStrategy::enough_naval_tile_improvement,"Naval strategy outside four-trigger scope");const auto& p=*impl_;try{const auto r=reader(*p.state,*p.source,*p.host,p.owner,p.city,p.city_span);const bool result=r.value(trigger);require_current();return result;}catch(...){p.retired.store(true);throw;}
});}
std::int32_t BnwCityCurrentNavalStrategy::unit_count(BnwCityNativeUnitCount selector)const{return typed([&]{
 require_current();need(selector>=BnwCityNativeUnitCount::workers&&selector<=BnwCityNativeUnitCount::settlers_including_water,"Native unit counter selector unavailable");const auto& p=*impl_;try{const auto r=reader(*p.state,*p.source,*p.host,p.owner,p.city,p.city_span);const auto result=r.units(selector);require_current();return result;}catch(...){p.retired.store(true);throw;}
});}
bool BnwCityCurrentNavalStrategy::plot_blockaded(std::uint64_t plot)const{return typed([&]{
 require_current();const auto& p=*impl_;try{const auto r=reader(*p.state,*p.source,*p.host,p.owner,p.city,p.city_span);const bool result=r.blockaded(plot);require_current();return result;}catch(...){p.retired.store(true);throw;}
});}
bool BnwCityCurrentNavalStrategy::can_work(std::uint64_t plot)const{return typed([&]{
 require_current();const auto& p=*impl_;try{const auto r=reader(*p.state,*p.source,*p.host,p.owner,p.city,p.city_span);const bool result=r.can_work(plot);require_current();return result;}catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentNavalStrategy inspect_bnw_city_current_naval_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Naval strategy actual selected host absent");(void)host->session().bnw_game_debug_mode();auto p=std::make_shared<BnwCityCurrentNavalStrategy::Impl>();p->host=std::move(host);p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Naval strategy actual immutable rules scope absent");p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Naval strategy actual host rules identity differs");p->state=p->session->read_state_lease();need(bool(p->state),"Naval strategy actual state absent");p->source=&p->session->source();p->generation=p->session->state_generation();need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Naval strategy actual source payload differs");
 p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Naval strategy native owner outside BNW roster");const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));need(raw,"Naval strategy actual current city absent");p->city=city_scalar(*raw,"m_iID");need(city_scalar(*raw,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Naval strategy canonical owner/span differs");p->city_span={raw->core.start_offset,raw->core.end_offset};BnwCityCurrentNavalStrategy result(std::move(p));result.require_current();return result;
});}
}
