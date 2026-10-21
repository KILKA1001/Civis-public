#include "civ5/runtime/bnw_city_current_support_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_grand_world_averages.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_city_current_land_improvement_strategy.hpp"
#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include <algorithm>
#include <array>
#include "civ5/runtime/bnw_city_current_science_rate.hpp"
#include "civ5/runtime/bnw_personality_flavors.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
class KnownFrontier final:public std::exception {public:std::string reason;explicit KnownFrontier(const char* r):reason(r){}};
void need(bool b,const char* m){if(!b)throw BnwCitySupportStrategyUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Support city strategy reached signed32 operand unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCitySupportStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCitySupportStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Support city strategy duplicate source field");out=&r;}
 need(out,"Support city strategy required source field absent");return *out;
}
bool same_info(const content::InfoTypeEntry* a,const content::InfoTypeEntry* b){return a&&b&&a->table==b->table&&a->type==b->type&&a->id==b->id&&a->hash==b->hash;}
std::int32_t city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=named(c.core.fields,name);
 need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
  f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Support city strategy city native scalar shape unavailable");
 return native(f.values.front());
}

}
namespace {
std::int32_t required(const std::optional<std::int64_t>& n){need(n.has_value(),"Support detached reached numeric operand absent");return native(*n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(std::int64_t(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return native(std::int64_t(a)*b);}
template<class Reader>bool gp_evaluate(Reader& r){
 need(r.size()<=INT32_MAX,"GoodGP native specialist span unavailable");std::int32_t total=0;
 for(std::size_t i=0;i<r.size();++i){if(!r.present(i))continue;const auto unit=r.unit_class(i);if(unit==-1)continue;
  const auto count=r.count(i);const auto rate=r.rate(i);auto change=mul(mul(rate,count),100);
  change=add(change,mul(r.building_rate(i),100));if(change<=0)continue;
  auto mod=add(0,r.city_mod(i));mod=add(mod,r.player_mod(i));const auto kind=r.class_kind(i,unit);
  if(kind==1){mod=add(mod,r.scientist_trait(i));mod=add(mod,r.class_mod(i,kind));}
  else if(kind>=2&&kind<=6)mod=add(mod,r.class_mod(i,kind));
  change=mul(change,add(100,mod));change=static_cast<std::int32_t>(change/100);total=add(total,change);
 }
 return total>=800;
}
struct GpMathReader {
 std::span<const detail::BnwGpStrategyMathSlot> rows;
 auto size()const{return rows.size();}bool present(std::size_t i)const{return rows[i].info_present;}
 std::int32_t unit_class(std::size_t i)const{return required(rows[i].unit_class);}
 std::int32_t count(std::size_t i)const{return required(rows[i].count);}
 std::int32_t rate(std::size_t i)const{return required(rows[i].rate);}
 std::int32_t building_rate(std::size_t i)const{return required(rows[i].building_rate);}
 std::int32_t city_mod(std::size_t i)const{return required(rows[i].city_modifier);}
 std::int32_t player_mod(std::size_t i)const{return required(rows[i].player_modifier);}
 std::int32_t class_kind(std::size_t i,std::int32_t)const{const auto k=required(rows[i].class_kind);need(k>=0&&k<=6,"GoodGP detached class kind unavailable");return k;}
 std::int32_t scientist_trait(std::size_t i)const{return required(rows[i].trait_scientist_modifier);}
 std::int32_t class_mod(std::size_t i,std::int32_t)const{return required(rows[i].class_modifier);}
};
template<class Reader>bool key_science_evaluate(Reader& r){
 static_assert(std::numeric_limits<float>::is_iec559&&std::numeric_limits<float>::digits==24&&std::numeric_limits<float>::max_exponent==128);
 if(native(r.started())==-1)return false;
 const auto selected=native(r.selected_rate());std::int32_t cities=0,better=0;
 const auto slots=r.size();need(slots<=INT32_MAX,"KeyScience native city traversal span unavailable");
 for(std::size_t i=0;i<slots;++i){if(!r.present(i))continue;cities=add(cities,1);if(r.self(i))continue;if(native(r.rate(i))>selected)better=add(better,1);}
 if(cities<=0)return false;
 r.prepare_flavor();
 const float ratio=static_cast<float>(better)/static_cast<float>(cities);
 const float cutoff=0.05f*static_cast<float>(native(r.flavor()));return ratio<cutoff;
}
struct KeyScienceMathReader {
 std::int64_t start;std::optional<std::int64_t> selected,personality;
 std::span<const detail::BnwKeyScienceMathCity> rows;
 auto started()const{return start;}auto selected_rate()const{return required(selected);}
 auto size()const{return rows.size();}bool present(std::size_t i)const{return rows[i].info_present;}
 bool self(std::size_t i)const{return rows[i].is_self;}
 auto rate(std::size_t i)const{return required(rows[i].science_times100);}
 void prepare_flavor()const{}
 auto flavor()const{return required(personality);}
};
// Private actual-source reader. This is only constructed inside the retained
// SAME Support owner; the internal guard never comes from a caller.
class CultureTourismReader {
 const LiveSessionState& s;const saves::SavedSession& source;
 const content::RulesDatabase& db;const content::InfoTypeCatalog& types;const std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 const saves::SavedCityObservation& city;const CityYieldState& live;
 SessionCheckpointIdentity identity;std::int32_t owner,id;
 std::function<void()> current;
 struct Building {
  std::string type;std::optional<std::string> cls,branch,tech;
  std::optional<std::int64_t> works,cost,espionage;
 };
 std::map<std::int32_t,Building> buildings;
 std::map<std::int32_t,std::optional<std::int64_t>> classes;
 std::uint64_t class_slots=0,building_slots=0;
 std::optional<CityYieldRules> replacements;
 std::optional<std::pair<std::vector<std::int32_t>,std::vector<std::int32_t>>> dense;
 std::optional<std::int32_t> city_max;
 std::optional<BnwNormalPlotYieldRules> normal;
 std::int64_t sparse(const YieldValues& m,std::int64_t key)const{const auto it=m.find(key);return it==m.end()?0:it->second;}
 std::int32_t type(const std::optional<std::string>& name,std::string_view table)const{
  current();if(!name||name->empty())return -1;
  const auto* a=types.find(*name);const auto* b=types.find_hash(content::saved_info_type_hash(*name));
  need(same_info(a,b)&&a->table==table&&a->id>=0&&a->id<=INT32_MAX,"Culture/Tourism actual native Info reference identity unavailable");return native(a->id);
 }
 std::optional<std::int64_t> number(const auto& row,const char* name)const{return row.has_column(name)?std::optional{row.integer(name)}:std::nullopt;}
 void metadata(){
  if(replacements){return;}current();CityYieldRules r;r.profile=content::TechnologyProfile::bnw;
  const auto verify=[&](auto slot,auto authored,const auto& row,std::string_view table){
   need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"Culture/Tourism native Info slot unavailable");
   const auto name=row.text("Type");need(name&&type(name,table)==authored,"Culture/Tourism native Info metadata identity differs");
  };
  const auto cr=db.prefetch_info("BuildingClasses",[&](auto slot,auto authored,const auto& row){verify(slot,authored,row,"BuildingClasses");need(classes.emplace(static_cast<std::int32_t>(slot),number(row,"MaxGlobalInstances")).second,"Culture/Tourism duplicate BuildingClass slot");return true;});current();
  need(cr.all_succeeded()&&!cr.overlapping_ids&&cr.rows+cr.holes==cr.slots&&cr.slots<=INT32_MAX,"Culture/Tourism finite native BuildingClasses unavailable");class_slots=cr.slots;
  const auto br=db.prefetch_info("Buildings",[&](auto slot,auto authored,const auto& row){verify(slot,authored,row,"Buildings");
   Building b{*row.text("Type"),row.text("BuildingClass"),row.text("PolicyBranchType"),row.text("PrereqTech"),number(row,"GreatWorkCount"),number(row,"Cost"),row.has_column("Espionage")?std::optional<std::int64_t>{row.boolean("Espionage")?1:0}:std::nullopt};
   need(buildings.emplace(static_cast<std::int32_t>(slot),std::move(b)).second,"Culture/Tourism duplicate Building slot");return true;});current();
  need(br.all_succeeded()&&!br.overlapping_ids&&br.rows+br.holes==br.slots&&br.slots<=INT32_MAX,"Culture/Tourism finite native Buildings unavailable");building_slots=br.slots;
  load_city_building_replacements(db,r);current();
  need(s.world.pregame&&s.world.pregame->layout==bnw,"Culture/Tourism actual civilization Pregame unavailable");
  const auto& ids=s.world.pregame->slots.integer_arrays.at("civilizations");const auto& keys=s.world.pregame->slots.string_arrays.at("civilization_keys");
  need(ids.size()==64&&keys.size()==64&&ids.at(static_cast<std::size_t>(owner))==s.cities.players.at(owner).civilization,"Culture/Tourism current civilization mirror differs");
  const auto key=keys.at(static_cast<std::size_t>(owner)).substr(0,keys.at(static_cast<std::size_t>(owner)).find('\0'));
  need(type(std::optional<std::string>{key},"Civilizations")==ids.at(static_cast<std::size_t>(owner)),"Culture/Tourism actual civilization native type differs");
  need(r.civilization_buildings.contains(ids.at(static_cast<std::size_t>(owner))),"Culture/Tourism native CivilizationInfo unavailable");
  replacements=std::move(r);
 }
 std::int32_t mapped(std::int32_t cls){metadata();current();const auto& m=replacements->civilization_buildings.at(s.cities.players.at(owner).civilization);const auto it=m.find(cls);const auto value=it==m.end()?-1:native(it->second);need(value==-1||(value>=0&&static_cast<std::uint64_t>(value)<building_slots),"Culture/Tourism mapped Building native slot unavailable");return value;}
 const saves::CityProductionRecord& record(){
  current();const saves::SavedCityObservation* old=nullptr;
  for(const auto& c:source.snapshot.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities)if(city_scalar(c,"m_iID")==id){need(!old,"Culture/Tourism duplicate original city");old=&c;}
  need(old&&city.production.layout==bnw&&city.production.context.bnw_city_version==city.core.version&&city.production.start_offset==old->production.start_offset&&city.production.end_offset==old->production.end_offset,"Culture/Tourism actual Buildings Read source span differs");
  const auto& b=named(city.production.blocks,"buildings");const auto& original=named(old->production.blocks,"buildings");const auto v=native(named(b.fields,"version").value);
  need(v>=0&&v<=1&&v==named(original.fields,"version").value&&b.start_offset==original.start_offset&&b.end_offset==original.end_offset&&b.start_offset<b.end_offset,"Culture/Tourism actual Buildings schema/span unavailable");return b;
 }
 void count_facts(){
  if(dense){return;}metadata();const auto& b=record();need(live.building_counts_resolved,"Culture/Tourism actual Building import unresolved");
  const auto read=[&](const char* name,const YieldValues& mirror){const auto& rows=named(b.children,name);
   need(rows.start_offset>=b.start_offset&&rows.end_offset<=b.end_offset,"Culture/Tourism native Building row span unavailable");
   const auto values=detail::bnw_native_building_counts_reset_read_math(rows,building_slots,types);current();
   for(const auto& [key,n]:mirror){need(key>=0&&static_cast<std::uint64_t>(key)<building_slots,"Culture/Tourism owned Building count outside native span");(void)native(n);}
   for(std::size_t i=0;i<values.size();++i)need(sparse(mirror,static_cast<std::int64_t>(i))==values[i],"Culture/Tourism actual Reset/Read and owned Building count differ");return values;};
  auto real=read("m_paiNumRealBuilding",live.real_buildings);auto free=read("m_paiNumFreeBuilding",live.free_buildings);dense=std::pair{std::move(real),std::move(free)};
 }
 std::int32_t count(std::int32_t building){
  count_facts();if(!city_max){current();const auto defs=db.cache_global_defines(content::GlobalDefineProfile::bnw);const auto* v=defs.find("CITY_MAX_NUM_BUILDINGS");need(v&&v->lookup.found&&v->lookup.sqlite_code==0&&!v->retained_previous&&std::holds_alternative<std::int64_t>(v->lookup.value),"Culture/Tourism reached native CITY_MAX_NUM_BUILDINGS unavailable");city_max=native(std::get<std::int64_t>(v->lookup.value));current();}
  need(building>=0&&static_cast<std::uint64_t>(building)<building_slots,"Culture/Tourism native count index unavailable");const auto i=static_cast<std::size_t>(building);const auto real=dense->first.at(i),free=dense->second.at(i);return *city_max<=1?std::max(real,free):add(real,free);
 }
 bool world(std::int32_t cls){const auto it=classes.find(cls);need(it!=classes.end(),"Culture/Tourism reached native BuildingClassInfo NULL");return required(it->second)!=-1;}
 bool hash_flag(const saves::TeamHashedArray& a,std::string_view table,std::int32_t target){
  current();need(a.wire_count>=0&&static_cast<std::uint64_t>(a.wire_count)==a.rows.size(),"Culture/Tourism actual policy flag wire cardinality differs");std::optional<bool> found;
  for(const auto& row:a.rows){need(row.type_hash<=UINT32_MAX,"Culture/Tourism policy hash native width unavailable");if(row.type_hash==0){need(row.values.empty(),"Culture/Tourism null policy hash payload differs");continue;}
   need(row.values.size()==1&&(row.values[0]==0||row.values[0]==1),"Culture/Tourism actual policy native flag not boolean");const auto* t=types.find_hash(row.type_hash);need(t&&t->table==table&&t->id>=0&&t->id<=INT32_MAX&&same_info(t,types.find(t->type)),"Culture/Tourism actual policy native hash identity unavailable");if(t->id==target)found=row.values[0]!=0;
  }
  if(!found){throw KnownFrontier("Support Culture/Tourism missing reached policy flag requires actual native Reset/Read owner");}return *found;
 }
 const auto& policy_block(){current();const auto& p=s.world.all_players->players.at(static_cast<std::size_t>(owner));const auto& b=named(p.strategy.blocks,"policies");const auto version=native(named(b.fields,"version").value);need(p.strategy.layout==bnw&&(version==1||version==2),"Culture/Tourism actual PlayerPolicies Read schema unavailable");return b;}
 bool has_tech(std::int32_t tech){
  current();if(tech==-1)return true;
  need(tech>=0&&static_cast<std::uint64_t>(tech)<host->technologies().collection.slots&&host->technologies().find(static_cast<std::uint64_t>(tech)),"Culture/Tourism reached native Tech slot unavailable");
  const auto& ids=s.world.pregame->archive.integer_arrays.at("team_types");need(ids.size()==64,"Culture/Tourism actual native team mapping unavailable");const auto team=native(ids.at(static_cast<std::size_t>(owner)));need(team>=0&&team<64&&s.setup.players.at(owner).team==team,"Culture/Tourism raw/owned TEAM differs");
  need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64,"Culture/Tourism actual native Team Read unavailable");const auto& raw=s.world.teams->teams.at(static_cast<std::size_t>(team));need(raw.index==static_cast<std::uint64_t>(team)&&named(raw.fields,"id").value==team,"Culture/Tourism actual TEAM identity differs");
  const auto& hashes=named(raw.arrays,"tech_hashes");need(hashes.columns==1,"Culture/Tourism native HasTech hash columns unavailable");bool covered=false;
  for(auto hash:hashes.values){need(hash>=0&&hash<=UINT32_MAX,"Culture/Tourism native technology hash width unavailable");const auto* t=hash?types.find_hash(static_cast<std::uint64_t>(hash)):nullptr;if(!t)continue;need(t->table=="Technologies","Culture/Tourism native technology hash cross-table unavailable");if(t->id==tech)covered=true;}
  if(!covered)throw KnownFrontier("Support Culture/Tourism missing reached HasTech slot requires actual native TEAM Reset/Read owner");
  const auto& rules=host->technologies();current();const auto decoded=restore_team_technologies(raw,rules,types);const TeamTechnologies* owned=nullptr;for(const auto& t:s.technologies)if(t.team()==team){need(!owned&&t.profile()==content::TechnologyProfile::bnw&&t.slots()==rules.collection.slots,"Culture/Tourism actual owned TEAM tech registry unavailable");owned=&t;}
  need(owned&&decoded.has_tech(tech)==owned->has_tech(tech),"Culture/Tourism actual raw/owned HasTech differs");return owned->has_tech(tech);
 }
 bool no_espionage(){
  // Actual SDK GameOption string branch. Generic ResearchContext exact-name
  // caches do not own native CustomOption prefix semantics or permission.
  current();need(s.world.game_state.layout==bnw&&s.world.pregame&&source.snapshot.pregame,
   "Culture/Tourism actual Pregame string option source unavailable");
  const auto& raw=*s.world.pregame;const auto& old=*source.snapshot.pregame;
  need(raw.layout==bnw&&old.layout==bnw&&raw.slots_version==3&&raw.archive_version==6&&
   raw.slots_version==old.slots_version&&raw.archive_version==old.archive_version&&
   raw.header_tail_offset==old.header_tail_offset&&raw.slots_offset==old.slots_offset&&
   raw.archive_offset==old.archive_offset&&raw.archive_end_offset==old.archive_end_offset&&
   raw.end_offset==old.end_offset&&raw.header_tail_offset<raw.slots_offset&&
   raw.slots_offset<raw.archive_offset&&raw.archive_offset<raw.archive_end_offset&&
   raw.archive_end_offset<=raw.end_offset&&raw.game_options.size()<=UINT32_MAX,
   "Culture/Tourism actual native Pregame Read schema/span unavailable");
  const auto saved=detail::bnw_culture_saved_no_espionage_option(raw.game_options);current();
  if(saved)return *saved==1; // Game::isOption ==1, unlike PitBoss nonzero.
  std::optional<std::int32_t> fallback;
  const auto report=db.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",
   {std::string("GAMEOPTION_NO_ESPIONAGE")}},[&](const auto& row){fallback=native(row.integer_at(0));},1);
  current();need(report.all_succeeded(),"Culture/Tourism actual NO_ESPIONAGE string default query unavailable");
  // Native no Step row leaves Game::isOption's initialized value0. No cache
  // insert, enum synchronization or setup/PitBoss getter runs here.
  return fallback&&*fallback==1;
 }
 bool can_construct_prefix(std::int32_t building,const Building& b){
  current();const auto option=no_espionage();if(option){const auto espionage=required(b.espionage);need(espionage==0||espionage==1,"Culture/Tourism native Building Espionage boolean unavailable");if(espionage)return false;}
  // Actual native currentTeam/BuildingClassInfo is reached before the remap.
  const auto& teams=s.world.pregame->archive.integer_arrays.at("team_types");need(teams.size()==64,"Culture/Tourism native currentTeam mapping unavailable");const auto team=native(teams.at(static_cast<std::size_t>(owner)));need(team>=0&&team<64&&s.setup.players.at(owner).team==team&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&s.world.teams->teams.at(static_cast<std::size_t>(team)).index==static_cast<std::uint64_t>(team)&&named(s.world.teams->teams.at(static_cast<std::size_t>(team)).fields,"id").value==team,"Culture/Tourism actual currentTeam identity differs");
  const auto cls=type(b.cls,"BuildingClasses");need(classes.contains(cls),"Culture/Tourism canConstruct native BuildingClassInfo NULL");
  if(mapped(cls)!=building){return false;}if(required(b.cost)==-1){return false;}
  const auto branch=type(b.branch,"PolicyBranchTypes");if(branch!=-1&&!hash_flag(named(policy_block().hashes,"branch_unlocked"),"PolicyBranchTypes",branch))return false;
  if(!has_tech(type(b.tech,"Technologies")))return false;
  throw KnownFrontier("Support NeedCultureBuilding reached additional AND-tech/obsolete/replacement/CanConstruct admission owner unavailable");
 }
 std::int32_t surface(const saves::PlotObservation& raw,const saves::SavedMapCellProjection& cell,bool feature){
  current();need(raw.version&&*raw.version>=1&&*raw.version<=7,"Culture/Tourism actual native Plot Read version unavailable");
  const auto& numeric=feature?cell.feature_type:cell.improvement_type;const auto& projected_hash=feature?cell.feature_type_hash:cell.improvement_type_hash;
  if(*raw.version<(feature?3u:5u)){const auto n=native(named(raw.fields,feature?"feature_type":"improvement_type").value);need(n>=-1&&n<=INT8_MAX&&!projected_hash&&numeric==std::optional<std::int64_t>{n},"Culture/Tourism raw/projected native surface enum differs");return n;}
  const auto hash=named(raw.fields,feature?"feature_type_hash":"improvement_type_hash").value;
  need(hash>=0&&hash<=UINT32_MAX&&!numeric&&projected_hash==std::optional<std::uint64_t>{static_cast<std::uint64_t>(hash)},"Culture/Tourism raw/projected native surface hash differs");
  if(hash==0){return -1;}const auto* type=types.find_hash(static_cast<std::uint64_t>(hash));need(type&&type->table==(feature?"Features":"Improvements")&&type->id>=0&&type->id<=INT8_MAX&&same_info(type,types.find(type->type)),"Culture/Tourism reached native surface Info unavailable");return native(type->id);
 }
 std::int32_t culture_matrix(std::string_view table,std::string_view filter,std::string_view name){
  current();std::array<std::int32_t,6> values{};const std::string sql="select Yields.ID, Yield from "+std::string(table)+" inner join Yields on YieldType = Yields.Type where "+std::string(filter)+" = ?";
  const auto report=db.query_rows({sql,{std::string(name)}},[&](const auto& row){const auto y=native(row.integer_at(0));need(y>=0&&y<6,"Culture/Tourism native SetYields slot unavailable");values.at(static_cast<std::size_t>(y))=native(row.integer_at(1));});current();need(report.all_succeeded(),"Culture/Tourism native SetYields query unavailable");
  const auto* y=types.find("YIELD_CULTURE");need(y&&y->table=="Yields"&&y->id==4&&s.cities.yield_slots==6,"Culture/Tourism native CULTURE enum differs");return values[4];
 }
 std::int32_t empty_league(){current();const auto& l=s.world.leagues;const auto& old=source.snapshot.leagues;need(l.layout==bnw&&l.version&&*l.version>=1&&*l.version<=5&&l.version==old.version&&l.start_offset==old.start_offset&&l.end_offset==old.end_offset&&l.start_offset<l.end_offset,"Culture/Tourism actual GameLeague Read source unavailable");if(!l.leagues.empty())throw KnownFrontier("Support Culture/Tourism reached actual League member/resolution yield owner unavailable");need(s.cities.leagues.empty(),"Culture/Tourism raw/owned empty League differs");return 0;}
 template<class Visit>void worked(Visit&& visit){
  current();const auto& h=s.world.map_header;const auto& old=source.snapshot.map_header;
  need(h.layout==bnw&&h.grid.width>0&&h.grid.width<=INT16_MAX&&h.grid.height>0&&h.grid.height<=INT16_MAX&&h.grid.size()<=INT32_MAX&&h.grid.width==old.grid.width&&h.grid.height==old.grid.height&&h.wrap_x==old.wrap_x&&h.wrap_y==old.wrap_y&&h.guid==old.guid&&s.world.map_plots&&source.snapshot.map_plots&&s.world.map_projection,"Culture/Tourism actual native map/source unavailable");
  need(s.world.map_plots->layout==bnw&&s.world.map_plots->plots.size()==h.grid.size()&&s.world.map_projection->cells.size()==h.grid.size()&&s.world.map_projection->grid.width==h.grid.width&&s.world.map_projection->grid.height==h.grid.height&&s.world.map_projection->wrap_x==h.wrap_x&&s.world.map_projection->wrap_y==h.wrap_y&&s.world.map_plots->start_offset==source.snapshot.map_plots->start_offset&&s.world.map_plots->end_offset==source.snapshot.map_plots->end_offset,"Culture/Tourism raw/projection map identity differs");
  const auto x=city_scalar(city,"m_iX"),y=city_scalar(city,"m_iY");need(x==live.x&&y==live.y&&x>=0&&y>=0&&x<=INT16_MAX&&y<=INT16_MAX&&static_cast<std::uint64_t>(x)<h.grid.width&&static_cast<std::uint64_t>(y)<h.grid.height,"Culture/Tourism actual current city position differs");
  const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};const auto positions=detail::bnw_citizen_plot_positions(geometry,x,y);
  for(std::size_t slot=1;slot<positions.size();++slot){current();if(!positions[slot])continue;const auto index=native(*positions[slot]);need(index>=0&&static_cast<std::uint64_t>(index)<h.grid.size(),"Culture/Tourism reached native plot index unavailable");const auto i=static_cast<std::size_t>(index);const auto& raw=s.world.map_plots->plots.at(i);const auto& prior=source.snapshot.map_plots->plots.at(i);const auto& cell=s.world.map_projection->cells.at(i);
   need(raw.index==i&&cell.index==i&&raw.x==cell.x&&raw.y==cell.y&&raw.x<h.grid.width&&raw.y<h.grid.height&&raw.version&&*raw.version>=1&&*raw.version<=7&&raw.version==prior.version&&raw.start_offset==prior.start_offset&&raw.end_offset==prior.end_offset&&named(raw.fields,"source_x").value==static_cast<std::int64_t>(raw.x)&&named(raw.fields,"source_y").value==static_cast<std::int64_t>(raw.y),"Culture/Tourism current plot raw/projection/source differs");
   const auto po=native(named(raw.fields,"working_city_owner").value),pi=native(named(raw.fields,"working_city_id").value);need(cell.yield_city&&cell.yield_city->owner==po&&cell.yield_city->id==pi,"Culture/Tourism native workingCity projection differs");const auto* working=resolve_bnw_city_registry(s.world,s.cities,po,pi);if(!working||city_scalar(*working,"m_iID")!=id)continue;
   const auto inverse=detail::bnw_citizen_working_plot_index(geometry,x,y,static_cast<std::int64_t>(raw.x),static_cast<std::int64_t>(raw.y));need(inverse>=0&&inverse<37,"Culture/Tourism native inverse working plot unavailable");
   need(city.management.layout==bnw&&city.management.city_plot_slots==37,"Culture/Tourism native Citizens Read unavailable");const saves::SavedCityObservation* old_city=nullptr;for(const auto& c:source.snapshot.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities)if(city_scalar(c,"m_iID")==id){need(!old_city,"Culture/Tourism duplicate original Citizens city");old_city=&c;}
   need(old_city&&city.management.start_offset==old_city->management.start_offset&&city.management.end_offset==old_city->management.end_offset,"Culture/Tourism actual Citizens Read source span differs");const auto& citizens=named(city.management.blocks,"citizens");const auto& old_citizens=named(old_city->management.blocks,"citizens");const auto version=native(named(citizens.fields,"version").value);need(version>=0&&version<=1&&version==named(old_citizens.fields,"version").value&&citizens.start_offset==old_citizens.start_offset&&citizens.end_offset==old_citizens.end_offset&&citizens.start_offset<citizens.end_offset,"Culture/Tourism actual native Citizens schema/span unavailable");const auto& flags=named(citizens.arrays,"m_pabWorkingPlot");need(flags.columns==1&&flags.values.size()==37,"Culture/Tourism actual native working flags37 unavailable");const auto flag=flags.values.at(static_cast<std::size_t>(inverse));need(flag==0||flag==1,"Culture/Tourism reached working flag not native boolean");if(flag)visit(index,raw,cell);
  }
 }
public:
 CultureTourismReader(const LiveSessionState& state,const saves::SavedSession& saved,const content::RulesDatabase& rules,const content::InfoTypeCatalog& catalog,std::shared_ptr<const ui::SelectedBnwPreviewHost> actual_host,const saves::SavedCityObservation& raw,const CityYieldState& owned,SessionCheckpointIdentity key,std::int32_t player,std::int32_t full,std::function<void()> guard):s(state),source(saved),db(rules),types(catalog),host(actual_host),city(raw),live(owned),identity(std::move(key)),owner(player),id(full),current(std::move(guard)){}
 bool need_culture(){
  metadata();for(std::uint64_t slot=0;slot<class_slots;++slot){const auto cls=static_cast<std::int32_t>(slot),building=mapped(cls);if(building==-1)continue;const auto b=buildings.find(building);if(b==buildings.end()||!classes.contains(cls))continue;if(world(cls))continue;if(required(b->second.works)>0&&count(building)==0){(void)can_construct_prefix(building,b->second);}}
  // Every potential candidate was natively skipped or genuinely rejected.
  // No supplied selected slot/open-slots flag can reach the suffix.
  current();return false;
 }
 std::int32_t wonders(){
  metadata();std::int32_t value=0;for(std::uint64_t slot=0;slot<class_slots;++slot){const auto b=mapped(static_cast<std::int32_t>(slot));if(b==-1||count(b)<=0)continue;const auto it=buildings.find(b);if(it==buildings.end())continue;const auto cls=type(it->second.cls,"BuildingClasses");if(world(cls)){value=add(value,culture_matrix("Building_YieldChanges","BuildingType",it->second.type));value=add(value,empty_league());}}
  current();return value;
 }
 std::int32_t natural(){
  std::int32_t value=0;worked([&](std::int32_t,const auto& raw,const auto& cell){const auto feature=surface(raw,cell,true);if(feature==-1)return;
   bool found=false,wonder=false;const auto report=db.prefetch_info("Features",[&](auto slot,auto authored,const auto& row){if(authored==feature){need(slot==static_cast<std::uint64_t>(feature)&&type(row.text("Type"),"Features")==feature,"Culture/Tourism native FeatureInfo identity differs");need(row.has_column("NaturalWonder"),"Culture/Tourism native NaturalWonder metadata unavailable");found=true;wonder=row.boolean("NaturalWonder");}return true;});current();need(report.all_succeeded()&&!report.overlapping_ids&&found,"Culture/Tourism reached actual FeatureInfo unavailable");if(!wonder)return;
   const auto& cache=named(raw.arrays,"yields");need(cache.columns==1&&cache.values.size()==6&&cell.cached_yields&&cell.cached_yields->size()==6,"Culture/Tourism actual native six plot cache unavailable");const auto n=cache.values[4];need(n>=INT16_MIN&&n<=INT16_MAX&&n==cell.cached_yields->at(4),"Culture/Tourism actual raw/projected culture short cache differs");value=add(value,static_cast<std::int32_t>(n));});current();return value;
 }
 std::int32_t improvements(){
  std::int32_t value=0;worked([&](std::int32_t plot,const auto& raw,const auto& cell){const auto improvement=surface(raw,cell,false);if(improvement==-1)return;const content::InfoTypeEntry* actual_improvement=nullptr;
   if(!normal){normal=BnwNormalPlotYieldRules::load(db,identity.rules_sha256);current();}
   try{value=add(value,calculate_bnw_normal_plot_yield(s,source,identity,*normal,plot,4));}catch(const BnwNormalPlotYieldUnavailable& e){if(std::string_view(e.what())=="Normal plot yield hidden resource requires genuine debug owner join")throw KnownFrontier("Support NeedTourismBuilding reached hidden-resource actual normal/debug owner join unavailable");throw;}current();
   // The extra adjacency is a SECOND native getter, never silently folded
   // into the normal calculation or omitted after the first add.
   bool found=false;std::optional<std::int64_t> adjacent;const auto report=db.prefetch_info("Improvements",[&](auto slot,auto authored,const auto& row){if(authored==improvement){need(slot==static_cast<std::uint64_t>(improvement)&&type(row.text("Type"),"Improvements")==improvement,"Culture/Tourism actual ImprovementInfo identity differs");found=true;actual_improvement=types.find(*row.text("Type"));adjacent=number(row,"CultureAdjacentSameType");}return true;});current();need(report.all_succeeded()&&!report.overlapping_ids,"Culture/Tourism native ImprovementInfo lookup unavailable");if(found&&culture_matrix("Improvement_Yields","ImprovementType",actual_improvement->type)>0&&required(adjacent)>0)throw KnownFrontier("Support NeedTourismBuilding reached SECOND ComputeCultureFromAdjacentImprovement source owner unavailable");});current();return value;
 }
 std::int32_t base(){
  current();auto child=inspect_bnw_city_current_base_tourism(host,owner,id);
  try{const auto value=child.value();current();return value;}
  catch(const BnwCityCurrentBaseTourismUnavailable& e){const std::string_view why=e.what();if(why.find("requires")!=std::string_view::npos||why.find("returned actual beliefs")!=std::string_view::npos)throw KnownFrontier(e.what());throw;}
 }
 bool need_tourism(){auto value=add(0,wonders());value=add(value,natural());value=add(value,improvements());value=add(value,base());current();return value>10;}
};

}
std::optional<std::int32_t> detail::bnw_culture_saved_no_espionage_option(std::span<const saves::PregameOption> options){return typed([&]()->std::optional<std::int32_t>{
 constexpr std::string_view requested="GAMEOPTION_NO_ESPIONAGE";
 // Native CustomOption::Read completes all rows before GetGameOption: every
 // first-NUL C string must fit strcpy_s's64-byte target; every value is i32.
 need(options.size()<=UINT32_MAX,"Culture/Tourism native CustomOption vector count unavailable");
 for(const auto& option:options){const auto saved=std::string_view(option.name).substr(0,option.name.find('\0'));
  need(saved.size()<64,"Culture/Tourism native CustomOption Read name exceeds63 bytes");(void)native(option.value);}
 for(const auto& option:options){const auto saved=std::string_view(option.name).substr(0,option.name.find('\0'));
  // strncmp(saved,requested,strlen(saved)): empty/prefix first match wins;
  // a longer saved key differs at requested's terminating NUL.
  if(saved.size()<=requested.size()&&saved==requested.substr(0,saved.size()))return native(option.value);
 }
 return std::nullopt;
});}
bool detail::bnw_need_tourism_four_terms_math(const std::array<std::optional<std::int64_t>,4>& terms){return typed([&]{std::int32_t value=0;for(const auto& n:terms)value=add(value,required(n));return value>10;});}
bool detail::bnw_good_gp_strategy_math(std::span<const BnwGpStrategyMathSlot> rows){return typed([&]{GpMathReader r{rows};return gp_evaluate(r);});}
bool detail::bnw_key_science_strategy_math(std::int64_t started,std::optional<std::int64_t> selected,std::span<const BnwKeyScienceMathCity> rows,std::optional<std::int64_t> personality){return typed([&]{KeyScienceMathReader r{started,selected,personality,rows};return key_science_evaluate(r);});}
bool detail::bnw_many_techs_stolen_math(std::int64_t game_turn,std::int64_t started,std::optional<std::int64_t> robbed){
 static_assert(std::numeric_limits<float>::is_iec559&&std::numeric_limits<float>::digits==24&&std::numeric_limits<float>::max_exponent==128);
 const auto start=native(started),turn=native(game_turn);const auto elapsed=native(std::int64_t(turn)-start);if(start==0)return false;
 const float ratio=static_cast<float>(required(robbed))/static_cast<float>(elapsed);return ratio>0.0f;
}
bool detail::bnw_good_airlift_strategy_math(const MapGeometry& g,bool capital,std::optional<std::int64_t> area,std::optional<std::int64_t> cap_area,std::optional<std::int64_t> x,std::optional<std::int64_t> y,std::optional<std::int64_t> cx,std::optional<std::int64_t> cy){return typed([&]{
 if(capital){return true;}if(required(area)!=required(cap_area)){return true;}
 need(g.layout==bnw&&g.grid.width>0&&g.grid.width<=INT16_MAX&&g.grid.height>0&&g.grid.height<=INT16_MAX&&g.grid.size()<=INT32_MAX,"GoodAirlift native distance prefix bounded geometry unavailable");
 const auto nx=required(x),ny=required(y),ncx=required(cx),ncy=required(cy);for(const auto n:{nx,ny,ncx,ncy})need(n>=0&&n<=INT16_MAX,"GoodAirlift native short coordinate unavailable");
 need(static_cast<std::uint64_t>(nx)<g.grid.width&&static_cast<std::uint64_t>(ncx)<g.grid.width&&static_cast<std::uint64_t>(ny)<g.grid.height&&static_cast<std::uint64_t>(ncy)<g.grid.height,"GoodAirlift actual coordinate outside map");
 return native(g.distance(nx,ny,ncx,ncy))>20;
});}
struct BnwCityCurrentSupportStrategy::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;
 std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;
 mutable std::optional<BnwCitySpecialistCounts> specialists;
 std::int32_t owner=-1,city=-1;
 std::pair<std::uint64_t,std::uint64_t> city_span;
 mutable std::atomic<bool> retired{false};
};
BnwCityCurrentSupportStrategy::BnwCityCurrentSupportStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentSupportStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Support city strategy foreign constructor thread/moved query");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Support city strategy genuine rules/host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&
   p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,
   "Support city strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode(); // actual owning thread/lifecycle BEFORE facade state/source
  need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&
   p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,
   "Support city strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwCityCurrentSupportStrategy::value(BnwSupportCityStrategy trigger)const{return typed([&]{
 require_current();need(trigger>=BnwSupportCityStrategy::need_culture_building&&trigger<=BnwSupportCityStrategy::many_techs_stolen,"Support city strategy outside six-trigger scope");const auto& p=*impl_;
 try{
 const auto& s=*p.state;const auto* city=resolve_bnw_city_registry(s.world,s.cities,p.owner,p.city);need(city&&city_scalar(*city,"m_iID")==p.city&&city_scalar(*city,"m_eOwner")==p.owner&&std::pair{city->core.start_offset,city->core.end_offset}==p.city_span,"Support city strategy canonical city/span differs");
 const auto& owner=s.cities.players.at(p.owner);
 const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(p.owner));
 const auto player_scalar=[&](const char* name){require_current();const auto& q=named(player.core.fields,name);need(player.index==static_cast<std::uint64_t>(p.owner)&&player.saved_player_id==p.owner&&player.core.layout==bnw&&q.encoding==saves::PlayerCoreEncoding::i32&&q.values.size()==1&&!q.wire_count&&q.bytes.empty(),"Support city strategy player native scalar unavailable");return native(q.values.front());};
 const auto native_type=[&](std::string_view name,std::string_view table){require_current();const auto* t=p.host->types().find(name);if(!t)return std::int32_t{-1};need(t->table==table&&t->id>=0&&t->id<=INT32_MAX&&t->hash==content::saved_info_type_hash(name)&&same_info(t,p.host->types().find_hash(t->hash)),"Support city strategy reached actual native Info identity unavailable");return native(t->id);};
 if(trigger==BnwSupportCityStrategy::need_culture_building||trigger==BnwSupportCityStrategy::need_tourism_building){
  const auto& live=owner.cities.at(p.city);need(live.owner==p.owner&&live.id==p.city&&live.source_core==std::optional{p.city_span},"Culture/Tourism actual city ownership/span differs");
  CultureTourismReader r{s,*p.source,*p.database,p.host->types(),p.host,*city,live,p.identity,p.owner,p.city,[&]{require_current();}};
  const auto result=trigger==BnwSupportCityStrategy::need_culture_building?r.need_culture():r.need_tourism();require_current();return result;
 }
 if(trigger==BnwSupportCityStrategy::key_science_city||trigger==BnwSupportCityStrategy::many_techs_stolen){
  require_current();const auto& e=player.espionage;const auto& old=p.source->snapshot.all_players->players.at(static_cast<std::size_t>(p.owner)).espionage;
  need(e.layout==bnw&&e.start_offset==old.start_offset&&e.end_offset==old.end_offset&&e.start_offset<e.end_offset,"Support city strategy actual EspionageAI source span unavailable");const auto& ai=named(e.blocks,"espionage_ai");need(named(ai.fields,"version").value==0&&e.start_offset<=ai.start_offset&&ai.end_offset<=e.end_offset,"Support city strategy native EspionageAI Read version unavailable");const auto started=native(named(ai.fields,"m_iTurnEspionageStarted").value);
  if(trigger==BnwSupportCityStrategy::key_science_city){
   struct ActualKeyReader {
    std::int32_t start;std::function<std::int32_t()> selected,personality;std::function<void()> prepare;
    std::function<std::size_t()> slots;
    std::function<const saves::SavedCityObservation*(std::size_t)> at;
    std::function<std::int32_t(const saves::SavedCityObservation&)> science;
    const saves::SavedCityObservation* target;
    auto started()const{return start;}auto selected_rate()const{return selected();}
    auto size()const{return slots();}bool present(std::size_t i)const{return at(i)!=nullptr;}
    bool self(std::size_t i)const{return at(i)==target;}
    auto rate(std::size_t i)const{const auto* c=at(i);need(c,"KeyScience reached city disappeared");return science(*c);}
    void prepare_flavor()const{prepare();}
    auto flavor()const{return personality();}
   };
   const auto science=[&](const saves::SavedCityObservation& c){
    require_current();try{auto rate=inspect_bnw_city_current_science_rate(p.host,p.owner,city_scalar(c,"m_iID"));const auto value=rate.value_times100();require_current();return value;}
    catch(const BnwCityCurrentScienceRateUnavailable& error){const std::string_view reason=error.what();
     if(reason=="Current SCIENCE nonzero process requires actual PRODUCTION recursion owner"||reason=="Current PRODUCTION recursive self-process unavailable before base"||reason=="Current SCIENCE older city schema requires native YieldPerReligion Reset owner" ){throw KnownFrontier(error.what());}throw;}
   };
   const auto slots=[&](){require_current();const auto& h=player.cities.header;const auto last=native(named(h.fields,"last_index").value);
    need(h.layout==bnw&&h.source_slots<=8192&&named(h.fields,"num_slots").value==static_cast<std::int64_t>(h.source_slots)&&last>=-1&&(last==-1||static_cast<std::uint64_t>(last)<h.source_slots),"KeyScience native FLTA traversal capacity unavailable");return static_cast<std::size_t>(std::int64_t(last)+1);};
   const auto at=[&](std::size_t slot){require_current();need(slot<8192,"KeyScience native FLTA index unavailable");return resolve_bnw_city_registry(s.world,s.cities,p.owner,static_cast<std::int32_t>(slot));};
   std::int32_t espionage=-1;const auto prepare_flavor=[&](){require_current();
    const auto flavors=p.database->prefetch_info("Flavors",[&](auto slot,auto authored,const auto& row){const auto name=row.text("Type");need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot)&&name&&!name->empty(),"KeyScience native flavor string unavailable");need(native_type(*name,"Flavors")==authored,"KeyScience actual flavor native ID differs");if(*name=="FLAVOR_ESPIONAGE")espionage=static_cast<std::int32_t>(slot);return true;});require_current();
    need(flavors.all_succeeded()&&!flavors.holes&&!flavors.overlapping_ids&&flavors.slots<=INT32_MAX&&espionage>=0,"KeyScience native ESPIONAGE flavor absent");};
   const auto personality=[&](){require_current();const auto& original=p.source->snapshot.all_players->players.at(static_cast<std::size_t>(p.owner));
    const auto& values=checked_bnw_personality_flavors(original,player,*p.database);require_current();need(static_cast<std::size_t>(espionage)<values.size(),"KeyScience native personality index unavailable");return native(values.at(static_cast<std::size_t>(espionage)));};
   ActualKeyReader reader{started,[&]{return science(*city);},personality,prepare_flavor,slots,at,science,city};
   const bool result=key_science_evaluate(reader);require_current();return result;
  }
  need(s.world.pregame&&s.world.pregame->layout==bnw&&s.setup.profile==content::TechnologyProfile::bnw,"Support ManyTechsStolen actual Game turn owner unavailable");const auto turn=native(s.world.pregame->archive.integers.at("game_turn"));need(turn==s.setup.game_turn,"Support ManyTechsStolen raw/owned Game turn differs");(void)native(std::int64_t(turn)-started);if(started==0){require_current();return false;}
  // Native flavor type scan is reached, but its result is UNUSED: no
  // personality numeric getter and no invented no-espionage return.
  require_current();const auto flavors=p.database->prefetch_info("Flavors",[&](auto slot,auto authored,const auto& row){const auto name=row.text("Type");need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot)&&name&&!name->empty(),"Support ManyTechsStolen native flavor string unavailable");need(native_type(*name,"Flavors")==authored,"Support ManyTechsStolen actual flavor native ID differs");return true;});require_current();need(flavors.all_succeeded()&&!flavors.holes&&!flavors.overlapping_ids&&flavors.slots<=INT32_MAX,"Support ManyTechsStolen native flavor scan unavailable");
  need(p.owner<22,"Support ManyTechsStolen native robbed array owner index outside major22");need(city->tail.layout==bnw&&city->tail.context.bnw_city_version==city->core.version,"Support ManyTechsStolen native major-index/city espionage schema unavailable");const auto& ce=named(city->tail.blocks,"espionage");const auto& version=named(ce.children,"version");need(version.values.size()==1&&version.values.front()==0,"Support ManyTechsStolen CityEspionage version unavailable");const auto& robbed=named(ce.children,"m_aiNumTimesCityRobbed");need(robbed.encoding&&std::string_view(robbed.encoding)=="counted_i32"&&robbed.wire_count&&*robbed.wire_count==22&&robbed.values.size()==22,"Support ManyTechsStolen actual robbed22 Read array unavailable; partial Reset owner required");
  const auto n=native(robbed.values.at(static_cast<std::size_t>(p.owner)));const bool result=detail::bnw_many_techs_stolen_math(turn,started,n);require_current();return result;
 }
 if(trigger==BnwSupportCityStrategy::good_airlift_city){
  const auto handle=player_scalar("m_iCapitalCityID");need(owner.capital==handle,"Support GoodAirlift raw/owned capital handle differs");const auto* capital=resolve_bnw_city_registry(s.world,s.cities,p.owner,handle);if(capital==city){require_current();return true;}
  const auto& h=s.world.map_header;const auto& original=p.source->snapshot.map_header;need(h.layout==bnw&&h.grid.width==original.grid.width&&h.grid.height==original.grid.height&&h.wrap_x==original.wrap_x&&h.wrap_y==original.wrap_y&&h.guid==original.guid&&s.world.map_plots&&p.source->snapshot.map_plots&&s.world.map_projection&&s.world.map_plots->layout==bnw&&s.world.map_plots->plots.size()==h.grid.size()&&s.world.map_projection->cells.size()==h.grid.size()&&s.world.map_projection->grid.width==h.grid.width&&s.world.map_projection->grid.height==h.grid.height&&s.world.map_projection->wrap_x==h.wrap_x&&s.world.map_projection->wrap_y==h.wrap_y&&s.world.map_plots->start_offset==p.source->snapshot.map_plots->start_offset&&s.world.map_plots->end_offset==p.source->snapshot.map_plots->end_offset&&p.source->snapshot.map_plots->plots.size()==h.grid.size(),"Support GoodAirlift actual native map identity unavailable");
  const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};
  const auto home_area=[&](const saves::SavedCityObservation& c){require_current();const auto x=city_scalar(c,"m_iX"),y=city_scalar(c,"m_iY"),id=city_scalar(c,"m_iID");need(x>=0&&y>=0&&x<=INT16_MAX&&y<=INT16_MAX&&h.grid.width>0&&h.grid.width<=INT16_MAX&&h.grid.height>0&&h.grid.height<=INT16_MAX&&h.grid.size()<=INT32_MAX&&static_cast<std::uint64_t>(x)<h.grid.width&&static_cast<std::uint64_t>(y)<h.grid.height,"Support GoodAirlift actual home coordinates/native geometry unavailable");const auto& live=owner.cities.at(id);need(live.x==x&&live.y==y,"Support GoodAirlift raw/owned home coordinates differ");const auto index=h.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));const auto& raw=s.world.map_plots->plots.at(static_cast<std::size_t>(index));const auto& cell=s.world.map_projection->cells.at(static_cast<std::size_t>(index));const auto& old=p.source->snapshot.map_plots->plots.at(static_cast<std::size_t>(index));need(raw.index==index&&cell.index==index&&raw.x==static_cast<std::uint64_t>(x)&&raw.y==static_cast<std::uint64_t>(y)&&cell.x==raw.x&&cell.y==raw.y&&raw.version&&*raw.version>=1&&*raw.version<=7&&raw.version==old.version&&raw.start_offset==old.start_offset&&raw.end_offset==old.end_offset,"Support GoodAirlift current home source span differs");
   need(named(raw.fields,"source_x").value==x&&named(raw.fields,"source_y").value==y,"Support GoodAirlift native saved home coordinates differ");const auto ro=native(named(raw.fields,"plot_city_owner").value),ri=native(named(raw.fields,"plot_city_id").value);need(cell.plot_city&&cell.plot_city->owner==ro&&cell.plot_city->id==ri&&resolve_bnw_city_registry(s.world,s.cities,ro,ri)==&c,"Support GoodAirlift native home-city reference differs");const auto area=native(named(raw.fields,"area").value);need(area>=INT16_MIN&&area<=INT16_MAX&&live.area==area,"Support GoodAirlift actual raw/owned short area differs");return area;};
  const auto area=home_area(*city);need(capital,"Support GoodAirlift native missing capital cannot be dereferenced");const auto cap_area=home_area(*capital);if(area!=cap_area){require_current();return true;}
  const bool result=detail::bnw_good_airlift_strategy_math(geometry,false,area,cap_area,city_scalar(*city,"m_iX"),city_scalar(*city,"m_iY"),city_scalar(*capital,"m_iX"),city_scalar(*capital,"m_iY"));require_current();return result;
 }
 // GoodGP: current shared dense Citizens Reset/Read leaf; no FOOD numeric call.
 struct Entry {std::string type;std::optional<std::string> unit_class;std::optional<std::int64_t> rate;};std::map<std::int32_t,Entry> entries;
 require_current();const auto report=p.database->prefetch_info("Specialists",[&](auto slot,auto authored,const auto& row){need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"Support GoodGP native SpecialistInfo slot unavailable");const auto name=row.text("Type");need(name&&native_type(*name,"Specialists")==authored&&row.has_column("GreatPeopleUnitClass"),"Support GoodGP actual SpecialistInfo identity unavailable");Entry e{*name,row.text("GreatPeopleUnitClass"),{}};if(row.has_column("GreatPeopleRateChange"))e.rate=row.integer("GreatPeopleRateChange");need(entries.emplace(static_cast<std::int32_t>(slot),std::move(e)).second,"Support GoodGP duplicate native Info slot");return true;});require_current();need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX&&report.rows==entries.size()&&report.rows+report.holes==report.slots,"Support GoodGP finite native SpecialistInfo collection unavailable");
 struct ActualGpReader {
  std::size_t span;const std::map<std::int32_t,Entry>& entries;
  std::function<std::int32_t(std::string_view,std::string_view)> type;
  std::function<BnwCitySpecialistCounts&()> counts;
  std::function<std::int32_t(const char*)> city_get,player_get,trait_get;std::int32_t owner,city;
  auto size()const{return span;}bool present(std::size_t i)const{return entries.contains(static_cast<std::int32_t>(i));}
  const auto& entry(std::size_t i)const{return entries.at(static_cast<std::int32_t>(i));}
  std::int32_t unit_class(std::size_t i)const{const auto& name=entry(i).unit_class;return name?type(*name,"UnitClasses"):-1;}
  std::int32_t count(std::size_t i)const{return counts().count_for_city(owner,city,static_cast<std::int32_t>(i));}
  std::int32_t rate(std::size_t i)const{return required(entry(i).rate);}
  std::int32_t building_rate(std::size_t i)const{return counts().building_rate_for_city(owner,city,static_cast<std::int32_t>(i));}
  std::int32_t city_mod(std::size_t)const{return city_get("m_iGreatPeopleRateModifier");}
  std::int32_t player_mod(std::size_t)const{return player_get("m_iGreatPeopleRateModifier");}
  std::int32_t class_kind(std::size_t,std::int32_t unit)const{constexpr const char* names[]={"UNITCLASS_SCIENTIST","UNITCLASS_WRITER","UNITCLASS_ARTIST","UNITCLASS_MUSICIAN","UNITCLASS_MERCHANT","UNITCLASS_ENGINEER"};for(std::int32_t i=0;i<6;++i)if(unit==type(names[i],"UnitClasses"))return i+1;return 0;}
  std::int32_t scientist_trait(std::size_t)const{return trait_get("m_iGreatScientistRateModifier");}
  std::int32_t class_mod(std::size_t,std::int32_t kind)const{constexpr const char* fields[]={"m_iGreatScientistRateModifier","m_iGreatWriterRateModifier","m_iGreatArtistRateModifier","m_iGreatMusicianRateModifier","m_iGreatMerchantRateModifier","m_iGreatEngineerRateModifier"};return player_get(fields[kind-1]);}
 };
 const auto count_owner=[&]()->BnwCitySpecialistCounts&{require_current();if(!p.specialists)p.specialists=BnwCitySpecialistCounts::from_same_host(p.host,p.owner,p.city);require_current();p.specialists->require_same_world_database(s.world,*p.database);return *p.specialists;};
 const auto trait_scalar=[&](const char* name){require_current();const auto& traits=player.traits;const auto& q=named(traits.fields,name);need(traits.layout==bnw&&traits.version&&*traits.version>=1&&*traits.version<=19&&q.encoding==saves::PlayerTraitsEncoding::i32&&q.values.size()==1,"Support GoodGP native reached trait scalar unavailable");return native(q.values.front());};
 ActualGpReader r{static_cast<std::size_t>(report.slots),entries,native_type,count_owner,[&](const char* name){require_current();return city_scalar(*city,name);},player_scalar,trait_scalar,p.owner,p.city};
 const bool result=gp_evaluate(r);require_current();return result;
 }catch(const KnownFrontier& e){throw BnwCitySupportStrategyUnavailable(e.reason);}catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentSupportStrategy inspect_bnw_city_current_support_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Support city strategy actual selected host absent");
 (void)host->session().bnw_game_debug_mode(); // actual thread/owner BEFORE DB or world
 auto p=std::make_shared<BnwCityCurrentSupportStrategy::Impl>();p->host=std::move(host);
 p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());
 p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Support city strategy actual immutable rules scope absent");
 p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&
  p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Support city strategy actual host rules identity differs");
 p->state=p->session->read_state_lease();need(bool(p->state),"Support city strategy actual state absent");
 p->source=&p->session->source();p->generation=p->session->state_generation();
 need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Support city strategy actual source payload differs");
 p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Support city strategy native owner outside BNW roster");
 const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));
 need(raw,"Support city strategy actual current city absent");p->city=city_scalar(*raw,"m_iID");
 need(city_scalar(*raw,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Support city strategy canonical owner/source span differs");
 p->city_span={raw->core.start_offset,raw->core.end_offset};
 BnwCityCurrentSupportStrategy result(std::move(p));result.require_current();return result;
});}
}
