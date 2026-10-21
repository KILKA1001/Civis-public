#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_yield_refresh.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_religion_yield_owner.hpp"
#include "civ5/runtime/bnw_player_happiness_prefix_owner.hpp"
#include "civ5/runtime/bnw_citizen_plot_value_math.hpp"
#include "civ5/runtime/bnw_citizen_specialist_value_math.hpp"
#include "civ5/runtime/bnw_unhappiness.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/bnw_city_current_food_rate.hpp"
#include "civ5/runtime/bnw_city_current_food_consumption.hpp"
#include "civ5/runtime/bnw_happiness_buildings.hpp"
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/bnw_city_current_strategy_eligibility.hpp"
#include "civ5/runtime/bnw_city_current_simple_strategy.hpp"
#include "civ5/runtime/bnw_city_current_building_strategy.hpp"
#include "civ5/runtime/bnw_city_current_geographic_strategy.hpp"
#include "civ5/runtime/bnw_city_current_coastal_strategy.hpp"
#include "civ5/runtime/bnw_city_current_trade_strategy.hpp"
#include "civ5/runtime/bnw_city_current_support_strategy.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_city_current_land_improvement_strategy.hpp"
#include "civ5/runtime/bnw_city_current_capital_strategy.hpp"
#include "civ5/runtime/bnw_city_strategy_obsolete_end.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_core_checkpoint.hpp"
#include "civ5/runtime/bnw_city_production_checkpoint.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/content/rules_database.hpp"
#include <atomic>
#include <climits>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw std::invalid_argument(why);}
std::int32_t native(std::int64_t n){
 need(n>=INT32_MIN&&n<=INT32_MAX,"City entry native int32 operand unavailable");
 return static_cast<std::int32_t>(n);
}
const saves::CityCoreField& core(const saves::SavedCityObservation& c,
 std::string_view name,saves::CityCoreEncoding encoding){
 const saves::CityCoreField* out=nullptr;
 for(const auto& f:c.core.fields)if(f.name&&name==f.name){
  need(!out,"City entry duplicate core field");out=&f;
 }
 need(out&&out->encoding==encoding&&out->values.size()==1&&!out->wire_count&&
  out->row_keys.empty()&&out->row_counts.empty()&&out->hashes.empty()&&out->bytes.empty(),
  "City entry raw core scalar shape unavailable");
 (void)native(out->values.front());return *out;
}
std::int32_t scalar(const saves::SavedCityObservation& c,std::string_view name){
 return native(core(c,name,saves::CityCoreEncoding::i32).values.front());
}
const saves::CityProductionRecord& buildings(const saves::SavedCityObservation& c){
 need(c.production.layout==bnw,"City entry non-BNW production block");
 const saves::CityProductionRecord* out=nullptr;
 for(const auto& b:c.production.blocks)if(b.name&&std::string_view(b.name)=="buildings"){
  need(!out,"City entry duplicate buildings block");out=&b;
 }
 need(out,"City entry buildings block unavailable");return *out;
}
const saves::NamedSaveInteger& sold(const saves::SavedCityObservation& c){
 const saves::NamedSaveInteger* out=nullptr;
 for(const auto& f:buildings(c).fields)
  if(f.name&&std::string_view(f.name)=="m_bSoldBuildingThisTurn"){
   need(!out,"City entry duplicate sold-building flag");out=&f;
  }
 need(out&&(out->value==0||out->value==1),"City entry sold-building native boolean unavailable");
 return *out;
}
bool flag(const saves::SavedCityObservation& c,std::string_view name){
 const auto n=core(c,name,saves::CityCoreEncoding::boolean).values.front();
 need(n==0||n==1,"City entry native boolean unavailable");return n!=0;
}
struct Facts {
 BnwCityEntrySourceKey key{};
 std::uint64_t production_begin=0,production_end=0,buildings_begin=0,buildings_end=0;
 std::array<std::pair<std::uint64_t,std::uint64_t>,3> core_fields{};
 std::array<bool,3> flags{};
 bool operator==(const Facts&)const=default;
};
Facts capture(const LiveSessionState& s,const saves::SavedSession& source,
 std::int32_t owner,std::int32_t handle){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&
  s.world.pregame&&s.world.pregame->layout==bnw,
  "City entry requires native BNW current clock/profile");
 const auto found=s.world.pregame->archive.integers.find("game_turn");
 need(found!=s.world.pregame->archive.integers.end()&&found->second>=0&&
  s.setup.game_turn==found->second,"City entry raw/owned game clock differs");
 const auto turn=native(found->second);
 const auto* raw=resolve_bnw_city_registry(s.world,s.cities,owner,handle);
 need(raw,"City entry current native city absent");
 const auto id=scalar(*raw,"m_iID");
 // Native damage is read before any of the three reset operands.
 detail::require_bnw_city_entry_damage_zero_native(scalar(*raw,"m_iDamage"));
 const auto* original=resolve_bnw_city_registry(source.snapshot,s.cities,owner,id);
 need(original&&original->core.layout==bnw&&raw->core.version==original->core.version&&
  raw->core.start_offset==original->core.start_offset&&raw->core.end_offset==original->core.end_offset&&
  raw->production.start_offset==original->production.start_offset&&
  raw->production.end_offset==original->production.end_offset,
  "City entry original native city/schema spans differ");
 const auto& b=buildings(*raw);const auto& ob=buildings(*original);
 need(b.start_offset==ob.start_offset&&b.end_offset==ob.end_offset,
  "City entry original buildings span differs");
 Facts out;out.key={turn,owner,id,raw->core.start_offset,raw->core.end_offset};
 out.production_begin=raw->production.start_offset;out.production_end=raw->production.end_offset;
 out.buildings_begin=b.start_offset;out.buildings_end=b.end_offset;
 for(std::size_t i=0;i<3;++i){
  const char* name=i==0?"m_iDamage":i==1?"m_bDrafted":"m_bMadeAttack";
  const auto encoding=i==0?saves::CityCoreEncoding::i32:saves::CityCoreEncoding::boolean;
  const auto& f=core(*raw,name,encoding);const auto& of=core(*original,name,encoding);
  need(f.start_offset==of.start_offset&&f.end_offset==of.end_offset,
   "City entry original native field span differs");
  out.core_fields[i]={f.start_offset,f.end_offset};
 }
 out.flags={flag(*raw,"m_bDrafted"),flag(*raw,"m_bMadeAttack"),sold(*raw).value!=0};
 return out;
}
struct FeatureData {
 std::string sha;content::InfoTypeCatalog types;
 std::map<std::int32_t,bool> clearable;
};
struct FeatureSlot {
 std::optional<std::int64_t> index;
 std::uint64_t begin=0,end=0;
 std::int64_t raw_feature=-1;std::int32_t feature=-1;
 bool clearable=false;
 bool operator==(const FeatureSlot&)const=default;
};
struct FeatureFacts {
 std::int32_t x=0,y=0;bool surrounded=false;
 std::uint64_t field_begin=0,field_end=0;
 std::array<FeatureSlot,25> positions;
 bool operator==(const FeatureFacts&)const=default;
};
static_assert(std::is_nothrow_copy_assignable_v<FeatureFacts>);
FeatureFacts capture_feature(const LiveSessionState& s,const saves::SavedSession& source,
 std::int32_t owner,std::int32_t id,const FeatureData& rules){
 const auto* city=resolve_bnw_city_registry(s.world,s.cities,owner,id);
 need(city,"City feature current native city absent");
 const auto* owned=resolve_bnw_city_yield_registry(s.world,s.cities,owner,id);
 const auto* original=resolve_bnw_city_registry(source.snapshot,s.cities,owner,id);
 need(owned&&original,"City feature SAME current/source city absent");
 FeatureFacts out;out.x=scalar(*city,"m_iX");out.y=scalar(*city,"m_iY");
 need(owned->x==out.x&&owned->y==out.y,"City feature raw/owned coordinates differ");
 const auto& flag_field=core(*city,"m_bFeatureSurrounded",saves::CityCoreEncoding::boolean);
 const auto& original_field=core(*original,"m_bFeatureSurrounded",saves::CityCoreEncoding::boolean);
 need(flag_field.start_offset==original_field.start_offset&&flag_field.end_offset==original_field.end_offset,
  "City feature source flag span differs");
 out.surrounded=flag(*city,"m_bFeatureSurrounded");
 out.field_begin=flag_field.start_offset;out.field_end=flag_field.end_offset;
 need(s.world.pregame&&source.snapshot.pregame,"City feature pregame source absent");
 const auto started=s.world.pregame->archive.flags.find("game_started");
 const auto old_started=source.snapshot.pregame->archive.flags.find("game_started");
 need(started!=s.world.pregame->archive.flags.end()&&started->second&&
  old_started!=source.snapshot.pregame->archive.flags.end()&&old_started->second,
  "City feature Clearable cache replay requires actual started BNW import");
 const auto& header=s.world.map_header;const auto& old_header=source.snapshot.map_header;
 need(header.layout==bnw&&old_header.layout==bnw&&s.world.map_plots&&
  s.world.map_projection&&source.snapshot.map_plots&&s.world.map_plots->layout==bnw&&
  source.snapshot.map_plots->layout==bnw&&header.grid.width==old_header.grid.width&&
  header.grid.height==old_header.grid.height&&header.wrap_x==old_header.wrap_x&&
  header.wrap_y==old_header.wrap_y&&header.guid==old_header.guid,
  "City feature SAME native map identity absent");
 const auto& projection=*s.world.map_projection;const auto& plots=*s.world.map_plots;
 need(projection.grid.width==header.grid.width&&projection.grid.height==header.grid.height&&
  projection.wrap_x==header.wrap_x&&projection.wrap_y==header.wrap_y&&
  projection.cells.size()==header.grid.size()&&plots.plots.size()==header.grid.size()&&
  source.snapshot.map_plots->plots.size()==header.grid.size()&&
  plots.start_offset==source.snapshot.map_plots->start_offset&&
  plots.end_offset==source.snapshot.map_plots->end_offset,
  "City feature raw/owned map geometry/span differs");
 MapGeometry geometry{header.grid,header.wrap_x,header.wrap_y,bnw};
 const auto positions=detail::bnw_city_feature_positions(geometry,out.x,out.y);
 for(std::size_t i=0;i<positions.size();++i){
  auto& slot=out.positions[i];slot.index=positions[i];if(!slot.index)continue;
  const auto index=static_cast<std::size_t>(*slot.index);
  const auto& raw=plots.plots.at(index);const auto& projected=projection.cells.at(index);
  const auto& old=source.snapshot.map_plots->plots.at(index);const auto xy=header.grid.coordinates(index);
  need(raw.index==index&&raw.x==xy[0]&&raw.y==xy[1]&&projected.index==index&&
   projected.x==xy[0]&&projected.y==xy[1]&&raw.version&&*raw.version>=1&&*raw.version<=7&&raw.version==old.version&&
   raw.start_offset==old.start_offset&&raw.end_offset==old.end_offset,
   "City feature reached plot native identity/span differs");
  for(const auto* coordinate_name:{"source_x","source_y"}){
   const saves::NamedSaveInteger* coordinate_field=nullptr;
   for(const auto& field:raw.fields)if(field.name&&std::string_view(field.name)==coordinate_name){
    need(!coordinate_field,"City feature duplicate plot source coordinate");coordinate_field=&field;
   }
   const auto expected=std::string_view(coordinate_name)=="source_x"?xy[0]:xy[1];
   need(coordinate_field&&coordinate_field->value>=0&&coordinate_field->value<=INT16_MAX&&
    static_cast<std::uint64_t>(coordinate_field->value)==expected,
    "City feature raw/owned native short plot coordinate differs");
  }
  const bool hashed=*raw.version>=3;const char* name=hashed?"feature_type_hash":"feature_type";
  const saves::NamedSaveInteger* field=nullptr;
  for(const auto& f:raw.fields)if(f.name&&std::string_view(f.name)==name){
   need(!field,"City feature duplicate native plot feature field");field=&f;
  }
  need(field,"City feature native plot feature field absent");
  slot.begin=raw.start_offset;slot.end=raw.end_offset;slot.raw_feature=field->value;
  if(hashed){
   need(field->value>=0&&static_cast<std::uint64_t>(field->value)<=UINT32_MAX&&
    !projected.feature_type&&projected.feature_type_hash==std::optional<std::uint64_t>(static_cast<std::uint64_t>(field->value)),
    "City feature raw/owned native feature hash differs");
   if(field->value!=0){
    const auto* info=rules.types.find_hash(static_cast<std::uint64_t>(field->value));
    need(info&&info->table=="Features","City feature reached hash lacks actual FeatureInfo");
    slot.feature=native(info->id);
   }
  }else{
   slot.feature=native(field->value);
   need(slot.feature>=-1&&slot.feature<=INT8_MAX&&!projected.feature_type_hash&&projected.feature_type==std::optional<std::int64_t>(field->value),
    "City feature raw/owned native Feature enum differs");
  }
  if(slot.feature!=-1){
   const auto found=rules.clearable.find(slot.feature);
   need(found!=rules.clearable.end(),"City feature reached native FeatureInfo hole unavailable");
   slot.clearable=found->second;
  }
 }
 return out;
}
auto& citizen_named(auto& rows,std::string_view name){
 using T=std::remove_reference_t<decltype(rows.front())>;T* out=nullptr;
 for(auto& v:rows)if(v.name&&name==v.name){need(!out,"Citizens duplicate native field");out=&v;}
 need(out,"Citizens missing native field");return *out;
}
// Private source reader. Constructed only inside the already registered Pending
// operation, retaining its actual Host; no public numeric DTO can create it.
struct CitizenBuildingsSource {
 struct Info {std::string type;};std::map<std::int32_t,Info> infos;
 std::uint64_t slots=0;std::vector<std::int32_t> real,free;
 std::optional<BnwBuildingHappinessRules> count_rules;
 CitizenBuildingsSource(const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,
  const LiveSessionState& state,const saves::SavedSessionSnapshot& source,
  std::int32_t owner,std::int32_t city_id,const std::function<void()>& check,bool need_counts=true){
  check();const auto guard=host->database().immutable_preview_fingerprint_currentness();
  need(guard&&guard()&&&host->session().state()==&state,"Citizens Buildings reader SAME actual source absent");
  const auto& db=host->database();const auto* city=resolve_bnw_city_registry(state.world,state.cities,owner,city_id);
  need(city,"Citizens Buildings canonical city absent");
  const auto& types=host->types();
  need(types.report().all_succeeded()&&!types.report().hash_collisions&&!types.report().name_overwrites,
   "Citizens native global Info hash provenance unavailable");
  const auto collection=db.prefetch_info("Buildings",[&](auto slot,auto authored,const auto& row){
   need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"Citizens BuildingInfo native slot unavailable");
   const auto type=row.text("Type");need(type&&!type->empty()&&type->find('\0')==std::string::npos,"Citizens BuildingInfo Type unavailable");
   const auto hash=content::saved_info_type_hash(*type);
   const auto* t=types.find(*type);const auto* h=types.find_hash(hash);
   need(hash&&t&&h&&t->table=="Buildings"&&h->table=="Buildings"&&t->id==authored&&h->id==authored&&
    t->type==*type&&h->type==*type&&t->hash==hash&&h->hash==hash,"Citizens BuildingInfo actual Type/hash identity unavailable");
   need(infos.emplace(static_cast<std::int32_t>(slot),Info{*type}).second,"Citizens duplicate BuildingInfo slot");return true;
  });
  check();need(guard()&&collection.all_succeeded()&&!collection.overlapping_ids&&collection.slots<=INT32_MAX&&
   collection.rows==infos.size()&&collection.rows+collection.holes==collection.slots,"Citizens BuildingInfo span unavailable");
  slots=collection.slots;if(!need_counts)return; // Worst-specialist reaches Info, not GetNumBuilding.
  const auto* original=resolve_bnw_city_registry(source,state.cities,owner,city_id);
  need(original&&city->production.layout==bnw&&city->production.context.bnw_city_version==city->core.version&&
   city->production.start_offset==original->production.start_offset&&city->production.end_offset==original->production.end_offset,
   "Citizens actual Buildings Read source allocation unavailable");
  const auto& block=buildings(*city);const auto& old=buildings(*original);
  const auto version=citizen_named(block.fields,"version").value;
  need(version>=0&&version<=1&&version==citizen_named(old.fields,"version").value&&
   block.start_offset==old.start_offset&&block.end_offset==old.end_offset,"Citizens Buildings Read schema/span unavailable");
  const auto& owned=state.cities.players.at(owner).cities.at(city_id);
  need(owned.building_counts_resolved,"Citizens owned Buildings import absent");
  const auto dense=[&](const char* name,const YieldValues& mirror){
   const auto& rows=citizen_named(block.children,name);
   need(rows.start_offset>=block.start_offset&&rows.end_offset<=block.end_offset,"Citizens Buildings count source span unavailable");
   auto out=detail::bnw_native_building_counts_reset_read_math(rows,collection.slots,types);
   for(const auto& [id,n]:mirror){need(id>=0&&static_cast<std::uint64_t>(id)<collection.slots,"Citizens owned Building count outside native span");(void)native(n);}
   for(std::size_t i=0;i<out.size();++i){const auto m=mirror.find(static_cast<std::int64_t>(i));
    need(out[i]==(m==mirror.end()?0:native(m->second)),"Citizens actual Reset/Read Building count mirror stale");}
   check();need(guard(),"Citizens rules retired after Buildings Read");return out;
  };
  real=dense("m_paiNumRealBuilding",owned.real_buildings);
  free=dense("m_paiNumFreeBuilding",owned.free_buildings);
  count_rules.emplace(BnwBuildingHappinessRules::load(db));
  check();need(guard()&&count_rules->rules_sha256()==host->identity().rules_sha256&&
   count_rules->replacements().building_slots==collection.slots,"Citizens native building-count rules differ");
 }
 std::int32_t count(std::uint64_t id)const{
  need(count_rules&&id<slots,"Citizens reached Building count slot unavailable");
  return count_rules->building_count(real.at(static_cast<std::size_t>(id)),free.at(static_cast<std::size_t>(id)));
 }
};
struct CitizenSpecialistInfo {
 std::string type;
 std::uint32_t hash=0;
 std::int32_t gp=0,experience=0,culture=0;
};
struct CitizenSpecialistSources {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::function<void()> check;
 std::int32_t owner,city;
 std::uint64_t slots=0;
 std::map<std::int32_t,CitizenSpecialistInfo> infos;
 CitizenSpecialistSources(std::shared_ptr<const ui::SelectedBnwPreviewHost> h,
  std::int32_t o,std::int32_t city_id,std::function<void()> c):host(std::move(h)),check(std::move(c)),owner(o),city(city_id){
  check();const auto& types=host->types();
  need(types.report().all_succeeded()&&!types.report().name_overwrites&&!types.report().hash_collisions,
   "Specialist native global remap provenance unavailable");
  const auto report=host->database().prefetch_info("Specialists",[&](auto slot,auto id,const auto& row){
   need(slot<=INT32_MAX&&id==static_cast<std::int64_t>(slot),"Specialist native Info slot unavailable");
   CitizenSpecialistInfo info;const auto type=row.text("Type");
   need(type&&!type->empty()&&type->find('\0')==std::string::npos,"Specialist finite actual Type unavailable");
   info.type=*type;info.hash=content::saved_info_type_hash(*type);
   const auto* by_name=types.find(*type);const auto* by_hash=types.find_hash(info.hash);
   need(info.hash&&by_name&&by_hash&&by_name->table=="Specialists"&&by_hash->table=="Specialists"&&
    by_name->id==id&&by_hash->id==id&&by_name->type==*type&&by_hash->type==*type&&
    by_name->hash==info.hash&&by_hash->hash==info.hash,"Specialist actual Type/hash/slot differs");
   info.gp=native(row.integer("GreatPeopleRateChange"));info.experience=native(row.integer("Experience"));
   info.culture=native(row.integer("CulturePerTurn"));
   need(infos.emplace(native(id),std::move(info)).second,"Specialist duplicate native Info slot");return true;
  });
  check();need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX&&
   report.rows==infos.size()&&report.rows+report.holes==report.slots,"Specialist native Info vector unavailable");slots=report.slots;
 }
 const CitizenSpecialistInfo* info(std::int32_t id)const{
  check();if(id==-1)return nullptr;
  need(id>=0&&static_cast<std::uint64_t>(id)<slots,"Specialist native index outside vector");
  const auto i=infos.find(id);return i==infos.end()?nullptr:&i->second;
 }
 std::int32_t define(std::string_view name)const{
  check();const auto* cached=host->defines().find(name);
  const auto raw=host->database().global_define(name,content::GlobalDefineKind::integer);
  need(cached&&host->defines().profile()==content::GlobalDefineProfile::bnw&&cached->kind==content::GlobalDefineKind::integer&&
   cached->lookup.found&&cached->lookup.error.empty()&&raw.found&&raw.error.empty()&&
   std::holds_alternative<std::int64_t>(cached->lookup.value)&&std::holds_alternative<std::int64_t>(raw.value)&&
   cached->lookup.value==raw.value,"Specialist actual Define/cache unavailable");
  const auto value=native(std::get<std::int64_t>(raw.value));check();return value;
 }
 const saves::SavedPlayerObservation& player()const{
  check();const auto& w=host->session().state().world;
  need(w.all_players&&w.all_players->layout==bnw&&w.all_players->players.size()==64&&owner>=0&&owner<64,
   "Specialist current native player roster unavailable");
  const auto& p=w.all_players->players.at(static_cast<std::size_t>(owner));
  need(p.index==static_cast<std::uint64_t>(owner)&&p.saved_player_id==owner&&p.core.layout==bnw,
   "Specialist current native player identity differs");return p;
 }
 std::int32_t player_scalar(std::string_view name)const{
  const auto& f=citizen_named(player().core.fields,name);
  need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
   f.hashes.empty()&&f.bytes.empty()&&f.row_counts.empty(),"Specialist current native player scalar shape unavailable");
  return native(f.values.front());
 }
 bool half_food()const{return player_scalar("m_iHalfSpecialistFoodCount")>0;}
 bool half_unhappiness()const{return player_scalar("m_iHalfSpecialistUnhappinessCount")>0;}
 std::array<std::int32_t,6> base_yields(std::int32_t id)const{
  const auto* actual=info(id);if(!actual)return {};
  // Exactly CvDatabaseUtility::PopulateArrayByValue: Reset six native slots,
  // inner join, no ORDER BY, repeated rows SET in actual SQLite step order.
  std::array<std::int32_t,6> values{};
  const auto report=host->database().query_rows({
   "select Yields.ID, Yield from SpecialistYields inner join Yields on YieldType = Yields.Type where SpecialistType = ?",
   {actual->type}},[&](const auto& row){
    const auto yi=native(row.integer("ID"));need(yi>=0&&yi<6,"Specialist authored native yield index unavailable");
    constexpr const char* names[]={"YIELD_FOOD","YIELD_PRODUCTION","YIELD_GOLD","YIELD_SCIENCE","YIELD_CULTURE","YIELD_FAITH"};
    const auto* type=host->types().find(names[yi]);need(type&&type->table=="Yields"&&type->id==yi,
     "Specialist source YieldInfo enum differs");values.at(static_cast<std::size_t>(yi))=native(row.integer("Yield"));
   });check();need(report.all_succeeded(),"Specialist native SetYields source SQL failed");return values;
 }
 std::int32_t generic(std::size_t yi)const{
  const auto& f=citizen_named(player().core.fields,"m_aiSpecialistExtraYield");
  need(yi<6&&f.encoding==saves::PlayerCoreEncoding::vector_i32&&f.wire_count==6&&f.values.size()==6&&
   f.hashes.empty()&&f.bytes.empty()&&f.row_counts.empty(),"Specialist current generic native yield shape unavailable");
  return native(f.values[yi]);
 }
 std::int32_t specific(std::int32_t id,std::size_t yi)const{
  const auto& p=player();need(p.administration.layout==bnw,"Specialist actual administration unavailable");
  const auto& pre=citizen_named(p.administration.blocks,"pre_city");
  const auto count=citizen_named(pre.fields,"m_ppaaiSpecialistExtraYield_wire_count").value;
  const auto& f=citizen_named(pre.arrays,"m_ppaaiSpecialistExtraYield");
  need(count>=0&&static_cast<std::uint64_t>(count)==slots&&slots<=SIZE_MAX/6&&f.columns==6&&
   f.values.size()==slots*6&&id>=0&&static_cast<std::uint64_t>(id)<slots&&yi<6,
   "Specialist current per-specialist native matrix unavailable");
  return native(f.values.at(static_cast<std::size_t>(id)*6+yi));
 }
 std::int32_t trait(std::int32_t id,std::size_t yi)const{
  const auto& t=player().traits;
  need(t.layout==bnw&&t.version&&*t.version>=1&&*t.version<=19&&t.dimensions.yields==6,
   "Specialist current native trait schema unavailable");
  const auto& f=citizen_named(t.fields,"m_ppaaiSpecialistYieldChange");
  need(f.encoding==saves::PlayerTraitsEncoding::yield_rows&&f.columns==6&&f.wire_count&&*f.wire_count>=0&&
   static_cast<std::uint64_t>(*f.wire_count)==slots&&slots<=SIZE_MAX/6&&f.values.size()==slots*6&&
   f.rows.empty()&&f.hashes.empty()&&id>=0&&static_cast<std::uint64_t>(id)<slots&&yi<6,
   "Specialist current trait native matrix unavailable");
  return native(f.values.at(static_cast<std::size_t>(id)*6+yi));
 }
 std::int32_t player_yield(std::int32_t id,std::size_t yi)const{
  if(!info(id))return 0;
  const auto base=base_yields(id).at(yi);
  auto value=native(std::int64_t(base)+specific(id,yi));value=native(std::int64_t(value)+trait(id,yi));
  if(id!=define("DEFAULT_SPECIALIST"))value=native(std::int64_t(value)+generic(yi));
  return value;
 }
 std::int32_t culture(std::int32_t id)const{
  const auto* i=info(id);if(!i)return 0;
  return native(std::int64_t(i->culture)+player_scalar("m_iSpecialistCultureChange"));
 }
 bool better(std::int32_t id)const{
  if(!info(id))return false;
  const auto def=define("DEFAULT_SPECIALIST");if(!info(def))return false;
  check();const auto& state=host->session().state();
  const auto* current=resolve_bnw_city_registry(state.world,state.cities,owner,city);
  need(current,"Citizens specialist IsBetter actual city absent");
  const auto focus=native(citizen_named(citizen_named(current->management.blocks,"citizens").fields,"m_eCityAIFocusTypes").value);
  const auto yi=detail::bnw_specialist_focus_yield(focus);if(yi==-1)return true;
  auto value=base_yields(id).at(static_cast<std::size_t>(yi));const auto other=base_yields(def).at(static_cast<std::size_t>(yi));
  if(half_unhappiness()||half_food())value=native(std::int64_t(value)*2);
  return value>=other;
 }
 std::int32_t extra(std::size_t yi,const BnwCitySpecialistCounts& counts,std::int32_t city)const{
  std::int32_t sum=0;for(std::uint64_t n=0;n<slots;++n){
   const auto id=static_cast<std::int32_t>(n);if(id==define("DEFAULT_SPECIALIST"))continue;
   auto multiplier=native(std::int64_t(specific(id,yi))+generic(yi));multiplier=native(std::int64_t(multiplier)+trait(id,yi));
   const auto count=counts.count_for_city(owner,city,id);
   sum=native(std::int64_t(sum)+native(std::int64_t(count)*multiplier));
  }check();return sum;
 }
};
enum class CitizensSpecialistReturn { allocation, remove_worker, remove_building, remove_default };
struct CitizensSpecialistTail {
 std::int32_t specialist=-1,building=-1,change=1;
 std::uint8_t phase=0,yield=0;
 bool awaiting_ui=false,forced=false;
 CitizensSpecialistReturn return_site=CitizensSpecialistReturn::allocation;
};

struct CitizensTerrainSeal {
 std::array<std::int32_t,6> raw{};
 std::map<std::int64_t,CityYieldComponents> owned;
};
struct CitizensUnworkTail {
 std::uint64_t plot=0;
 bool assigning=false;
 bool reallocation_return=false;
 std::array<std::int16_t,6> cached{};
 std::uint8_t next_yield=6;
 std::optional<std::uint8_t> awaiting_yield;
};
enum class CitizensDoTurnStep { focus, validate_forced, remove_working, next_puppet_setting, nonhuman_specialization, forced_demotion, remove_citizen, building_scan, remove_defaults, allocation_pending };
struct CitizensDoTurnTail {
 CitizensDoTurnStep step=CitizensDoTurnStep::focus;
 bool puppet=false;
 std::int32_t forced_demotions=0,working_removals=0;
 std::int32_t demotions_done=0,removals_done=0;
 std::uint64_t building_slot=0;
 std::int32_t building_removals=0,building_done=0,default_removals=0,defaults_done=0;
 bool building_captured=false;
};
struct Ledger {
 std::atomic<bool> attempted{false},feature_attempted{false},quarantined{false};
 // Shared monotone history survives older checkpoints. No world/DB pointer.
 std::atomic<bool> strategy_started{false},citizens_started{false};
 std::atomic<std::uint8_t> citizens_next{0},citizens_slot{0};
 std::atomic<std::uint64_t> citizens_writes{0};
 std::atomic<std::uint64_t> citizens_attempts{0};
 std::atomic<BnwCityCitizensVerifyFrontier> citizens_frontier{BnwCityCitizensVerifyFrontier::not_started};
 std::atomic<std::int32_t> strategy_next{0},strategy_slots{0};
 std::atomic<std::uint32_t> strategy_steps{0},strategy_calls{0};
 std::atomic<std::uint64_t> strategy_revision{0},strategy_effect_attempts{0},strategy_publications{0};
 std::atomic<BnwCityStrategyLoopFrontier> strategy_frontier{BnwCityStrategyLoopFrontier::none};
};
}
std::array<std::optional<std::int64_t>,25> detail::bnw_city_feature_positions(
 const MapGeometry& geometry,std::int64_t x,std::int64_t y){
 need(geometry.layout==bnw&&geometry.grid.width>0&&geometry.grid.width<=INT32_MAX&&
  geometry.grid.height>0&&geometry.grid.height<=INT32_MAX&&geometry.grid.size()<=INT32_MAX,
  "City feature requires native BNW map geometry");
 (void)native(x);(void)native(y);
 need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<geometry.grid.width&&
  static_cast<std::uint64_t>(y)<geometry.grid.height,"City feature native city home outside map");
 const auto half=[](std::int64_t v){return v>=0?v/2:(v-1)/2;};
 const auto hx=native(x-half(y));
 std::array<std::optional<std::int64_t>,25> out{};std::size_t slot=0;
 for(std::int32_t dx=-2;dx<=2;++dx)for(std::int32_t dy=-2;dy<=2;++dy){
  const auto ax=dx<0?-dx:dx,ay=dy<0?-dy:dy;
  const auto distance=(dx>=0)==(dy>=0)?ax+ay:std::max(ax,ay);
  if(distance<=2){
   const auto shifted_x=native(static_cast<std::int64_t>(hx)+dx);
   const auto shifted_y=native(y+dy);
   const auto target_x=native(static_cast<std::int64_t>(shifted_x)+half(shifted_y));
   out[slot]=geometry.plot(target_x,shifted_y);
  }
  ++slot; // SDK counts every25 slot, including hex-range/map NULLs.
 }
 return out;
}
bool detail::bnw_city_feature_majority(const std::array<bool,25>& slots)noexcept{
 std::uint8_t featured=0;for(bool value:slots)if(value)++featured;
 return featured>=25/2;
}
struct BnwCityEntryFeatureRules::Impl : FeatureData {
 const std::thread::id thread=std::this_thread::get_id();std::function<bool()> current;
};
BnwCityEntryFeatureRules::BnwCityEntryFeatureRules(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityEntryFeatureRules::require_current()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"City feature rules foreign thread/moved owner");
 need(p->current&&p->current(),"City feature SAME immutable rules retired");
}
BnwCityEntryFeatureRules BnwCityEntryFeatureRules::from_rules(const content::RulesDatabase& db){
 auto p=std::make_shared<Impl>();p->current=db.immutable_preview_fingerprint_currentness();
 need(p->current&&p->current(),"City feature requires SAME immutable activated rules");
 p->sha=db.gameplay_fingerprint_sha256();p->types=db.cache_info_types();
 need(p->types.report().all_succeeded(),"City feature actual global Info cache unavailable");
 const auto features=db.prefetch_info("Features",[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX,"City feature native Feature collection overflow");
  const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");
  const auto* info=type?p->types.find(*type):nullptr;const auto* hash=info?p->types.find_hash(info->hash):nullptr;
  need(type&&!type->empty()&&authored==id&&info&&info->table=="Features"&&info->id==id&&
   hash&&hash->type==info->type&&hash->table==info->table&&hash->id==id&&
   p->clearable.emplace(id,false).second,"City feature actual Feature slot/Type/hash identity differs");
  return true; // CvFeatureInfo::CacheResults5064 resets Clearable=false.
 });
 need(features.all_succeeded()&&features.slots<=INT32_MAX,"City feature native Feature collection unavailable");
 std::int32_t capacity=0;
 const auto sizes=db.query_rows({"SELECT max(rowid) AS maximum FROM Features",{}},[&](const auto& row){
  const auto maximum=native(row.integer("maximum"));
  need(maximum>=-1&&maximum<INT32_MAX,"City feature native Remove allocation overflow");
  capacity=maximum+1;if(capacity==0)capacity=1;
 });
 need(sizes.all_succeeded()&&sizes.rows==1&&static_cast<std::uint64_t>(capacity)>=features.slots,
  "City feature native FeatureRemove allocation too small");
 // Cache each non-null BuildInfo in native collection order. Holes stay NULL
 // and DoGameStarted skips them. BuildFeatures assigns Remove last-row-wins.
 std::map<std::int32_t,std::map<std::int32_t,bool>> builds;
 const auto report=db.prefetch_info("Builds",[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX,"City feature native Build collection overflow");
  const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");
  const auto* info=type?p->types.find(*type):nullptr;const auto* hash=info?p->types.find_hash(info->hash):nullptr;
  need(type&&!type->empty()&&type->size()<=400&&type->find('\'')==std::string::npos&&
   authored==id&&info&&info->table=="Builds"&&info->id==id&&hash&&hash->type==info->type&&
   hash->table==info->table&&hash->id==id,"City feature actual Build slot/Type/hash identity differs");
  auto [found,inserted]=builds.emplace(id,std::map<std::int32_t,bool>{});
  need(inserted,"City feature overlapping Build slot unavailable");
  // Exact native literal predicate after bounded/no-quote source Type proof;
  // do not alter unspecified relation iteration through a different query.
  const auto relations=db.query_rows({"select * from BuildFeatures where BuildType = '"+*type+"'",{}},[&](const auto& relation){
   const auto feature=relation.text("FeatureType");const auto* target=feature?p->types.find(*feature):nullptr;
   need(target&&target->table=="Features"&&target->id>=0&&target->id<capacity&&
    p->clearable.contains(native(target->id)),"City feature BuildFeatures native remap outside actual FeatureInfo");
   (void)native(relation.integer("Remove")); // GetBool uses native GetInt for integer storage.
   found->second[native(target->id)]=relation.boolean("Remove");
  });
  need(relations.all_succeeded(),"City feature native BuildFeatures query unavailable");return true;
 });
 need(report.all_succeeded()&&report.slots<=INT32_MAX,"City feature native Build collection unavailable");
 // Replay ONLY the feature-cache prefix of DoGameStarted, not UnitCycler
 // Rebuild or any complete GameStarted/turn receipt. Preserve first true Build.
 for(auto& [feature,clearable]:p->clearable)for(const auto& [build,remove]:builds){
  (void)build;const auto found=remove.find(feature);
  if(found!=remove.end()&&found->second){clearable=true;break;}
 }
 need(p->current(),"City feature SAME rules retired during cache replay");
 return BnwCityEntryFeatureRules(std::move(p));
}
bool BnwCityEntryFeatureRules::feature_clearable(std::int64_t id)const{
 const auto p=impl_;require_current();const auto found=p->clearable.find(native(id));
 need(found!=p->clearable.end(),"City feature derived cache native FeatureInfo absent");
 return found->second;
}
void detail::require_bnw_city_entry_damage_zero_native(std::int64_t damage){
 need(native(damage)==0,
  "City entry damage healing/repair requires actual engine event and popup source before flag resets");
}
struct BnwCityEntryPending::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::atomic<bool> retired{false};
 std::shared_ptr<LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;
 std::shared_ptr<const LiveSession::BnwGameDebugReadStorage> game;
 SessionCheckpointIdentity identity;
 std::shared_ptr<Ledger> ledger=std::make_shared<Ledger>();
 Facts expected;BnwCityEntryProgress progress;
 std::shared_ptr<const BnwCityEntryFeatureRules::Impl> feature_rules;
 std::optional<FeatureFacts> feature_expected;
 std::function<bool()> strategy_rules_current;std::string strategy_rules_sha;
 std::shared_ptr<const BnwCityStrategyRuntime> strategy_node;
 std::optional<saves::CivisSaveSection> strategy_beforeimage;
 std::optional<CitizensTerrainSeal> citizens_terrain;
 std::optional<CitizensUnworkTail> citizens_tail;
 std::optional<CitizensSpecialistTail> citizens_specialist;
 std::optional<CitizensDoTurnTail> citizens_do_turn;
 std::optional<saves::CivisSaveSection> specialist_core_beforeimage;
 std::shared_ptr<BnwCityYieldRefreshPending> religion_city_child;
 std::optional<BnwPlotYieldRefreshPending::TerrainPrefixSource> accepted_city_prefix;
 std::optional<std::int32_t> religion_majority;
 std::unique_ptr<BnwCityReligionYieldOwner> religion_owner;
 std::unique_ptr<BnwPlayerHappinessPrefixOwner> player_happiness_owner;
 std::optional<std::uint8_t> religion_ui_yield;
};
struct BnwCityEntryCheckpoint::Impl {
 BnwCityEntrySourceKey key;BnwCityEntryProgress progress;std::shared_ptr<Ledger> ledger;
};
BnwCityEntryPending::BnwCityEntryPending(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityEntryPending::~BnwCityEntryPending()=default;
BnwCityEntryCheckpoint::BnwCityEntryCheckpoint(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
std::shared_ptr<BnwCityEntryPending> BnwCityEntryPending::prepare(
 const LiveSession& session,std::int64_t owner,std::int64_t city){
 need(session.plot_yield_refresh_thread_==std::this_thread::get_id(),
  "City entry belongs to another session thread");
 need(session.source_&&session.state_&&session.bnw_game_debug_owner_current(),
  "City entry requires genuine current BNW import lifecycle owner");
 (void)session.bnw_game_debug_mode(); // Actual raw/cache storage, not a copied flag.
 auto p=std::make_unique<Impl>();p->state=session.state_;p->source=session.source_;
 p->game=session.debug_;p->identity=session.debug_->identity;
 p->expected=capture(*p->state,*p->source,native(owner),native(city));
 // Native codecs validate complete current schema/widths/companions without
 // inventing values or copying/publishing a second world. Prior genuine city
 // reset stores may already differ from original; use this SAME actual baseline.
 (void)make_bnw_city_core_checkpoint(p->state->world,p->state->world,p->identity,true);
 (void)make_bnw_city_production_checkpoint(p->state->world,p->state->world,p->identity,true);
 p->progress.turn=std::get<0>(p->expected.key);p->progress.owner=std::get<1>(p->expected.key);
 p->progress.city=std::get<2>(p->expected.key);p->progress.slot=p->progress.city&8191;
 p->progress.before=p->expected.flags;p->progress.after=p->expected.flags;
 auto out=std::shared_ptr<BnwCityEntryPending>(new BnwCityEntryPending(std::move(p)));
 out->require_current();return out;
}
void BnwCityEntryPending::require_current()const {
 need(impl_->thread==std::this_thread::get_id(),"City entry belongs to another source thread");
 need(!impl_->retired.load()&&!impl_->ledger->quarantined.load()&&impl_->state&&impl_->source&&
  impl_->game&&!impl_->game->retired.load()&&impl_->game->thread==impl_->thread&&
  impl_->game->identity.payload_sha256==impl_->identity.payload_sha256&&
  impl_->game->identity.rules_sha256==impl_->identity.rules_sha256&&
  impl_->game->raw_mode==impl_->game->cached_mode,
  "City entry imported allocation/history retired or quarantined");
 if(impl_->strategy_rules_current)
  need(impl_->strategy_rules_current()&&impl_->strategy_rules_sha==impl_->identity.rules_sha256,
   "City strategy retained SAME immutable rules retired");
 if(impl_->feature_rules){
  BnwCityEntryFeatureRules rules(impl_->feature_rules);rules.require_current();
  need(impl_->feature_rules->sha==impl_->identity.rules_sha256,"City feature retained rules identity differs");
 }
 if(impl_->strategy_node){
  const auto key=std::pair{impl_->progress.owner,impl_->progress.city};
  const auto found=impl_->state->city_strategy_runtime.find(key);
  need(found!=impl_->state->city_strategy_runtime.end()&&found->second==impl_->strategy_node&&impl_->strategy_beforeimage,
   "City strategy actual owned node replaced outside retained transaction");
  impl_->strategy_node->require_binding(*impl_->state,impl_->progress.owner,impl_->progress.city,
   std::get<3>(impl_->expected.key),std::get<4>(impl_->expected.key),impl_->progress.strategy_revision);
  const auto current=make_bnw_city_management_checkpoint(impl_->state->world,impl_->state->world,impl_->identity,true);
  need(current.id==impl_->strategy_beforeimage->id&&current.flags==impl_->strategy_beforeimage->flags&&current.bytes==impl_->strategy_beforeimage->bytes,
   "City raw management changed outside retained strategy/Citizens transaction");
 }
 if(impl_->specialist_core_beforeimage){
  const auto current=make_bnw_city_core_checkpoint(impl_->state->world,impl_->state->world,impl_->identity,true);
  need(current.id==impl_->specialist_core_beforeimage->id&&current.flags==impl_->specialist_core_beforeimage->flags&&
   current.bytes==impl_->specialist_core_beforeimage->bytes,"Citizens specialist current core changed outside source cursor");
  const auto* city=resolve_bnw_city_registry(impl_->state->world,impl_->state->cities,impl_->progress.owner,impl_->progress.city);
  need(city,"Citizens specialist canonical current city absent");
  const auto& raw=citizen_named(city->core.fields,"m_aiBaseYieldRateFromSpecialists");
  const auto& owned=impl_->state->cities.players.at(impl_->progress.owner).cities.at(impl_->progress.city);
  need(raw.values.size()==6,"Citizens specialist six yield seal absent");
  for(std::size_t yi=0;yi<6;++yi){const auto v=owned.yields.find(static_cast<std::int64_t>(yi));
   need((v==owned.yields.end()?0:v->second.specialists)==raw.values[yi],"Citizens specialist canonical raw/owned yield mirror stale");}
 }
 if(impl_->citizens_terrain){
  const auto* raw=resolve_bnw_city_registry(impl_->state->world,impl_->state->cities,impl_->progress.owner,impl_->progress.city);
  need(raw,"Citizens terrain current city absent");
  const auto& base=citizen_named(raw->core.fields,"m_aiBaseYieldRateFromTerrain");
  need(base.values.size()==6,"Citizens terrain seal shape differs");
  const auto& owned=impl_->state->cities.players.at(impl_->progress.owner).cities.at(impl_->progress.city);
  for(std::size_t y=0;y<6;++y){
   need(base.values[y]==impl_->citizens_terrain->raw[y],"Citizens raw terrain changed outside retained prefix");
   const auto now=owned.yields.find(static_cast<std::int64_t>(y));
   const auto old=impl_->citizens_terrain->owned.find(static_cast<std::int64_t>(y));
   need((now==owned.yields.end())==(old==impl_->citizens_terrain->owned.end()),"Citizens owned terrain slot replaced");
   if(now!=owned.yields.end())need(now->second.terrain==old->second.terrain,"Citizens owned terrain changed outside prefix");
  }
 }
 need(capture(*impl_->state,*impl_->source,impl_->progress.owner,impl_->progress.city)==impl_->expected,
  "City entry actual source operands changed");
 if(impl_->feature_expected)
  need(capture_feature(*impl_->state,*impl_->source,impl_->progress.owner,impl_->progress.city,
   *impl_->feature_rules)==*impl_->feature_expected,"City feature captured source operands changed");
}
BnwCityStrategyRuntimeValues BnwCityEntryPending::strategy_runtime_values()const {
 require_current();need(bool(impl_->strategy_node),"City strategy actual Init/Read owner not yet constructed");
 auto values=impl_->strategy_node->values();require_current();return values;
}
BnwCityEntrySourceKey BnwCityEntryPending::source_key()const {
 need(impl_->thread==std::this_thread::get_id(),"City entry key belongs to another source thread");
 return impl_->expected.key;
}
BnwCityEntryProgress BnwCityEntryPending::progress()const {
 need(impl_->thread==std::this_thread::get_id(),"City entry facts belong to another source thread");
 auto out=impl_->progress;
 out.strategy_started=out.strategy_started||impl_->ledger->strategy_started.load();
 out.strategy_next_id=std::max(out.strategy_next_id,impl_->ledger->strategy_next.load());
 out.strategy_slots=std::max(out.strategy_slots,impl_->ledger->strategy_slots.load());
 out.strategy_steps=std::max(out.strategy_steps,impl_->ledger->strategy_steps.load());
 out.strategy_trigger_calls=std::max(out.strategy_trigger_calls,impl_->ledger->strategy_calls.load());
 out.strategy_revision=std::max(out.strategy_revision,impl_->ledger->strategy_revision.load());
 out.strategy_effect_attempts=std::max(out.strategy_effect_attempts,impl_->ledger->strategy_effect_attempts.load());
 out.strategy_publications=std::max(out.strategy_publications,impl_->ledger->strategy_publications.load());
 if(impl_->ledger->strategy_frontier.load()!=BnwCityStrategyLoopFrontier::none)
  out.strategy_frontier=impl_->ledger->strategy_frontier.load();
 out.citizens_started=out.citizens_started||impl_->ledger->citizens_started.load();
 out.citizens_next_plot=std::max(out.citizens_next_plot,impl_->ledger->citizens_next.load());
 out.citizens_working_slot=impl_->ledger->citizens_slot.load();
 out.citizens_native_writes=std::max(out.citizens_native_writes,impl_->ledger->citizens_writes.load());
 out.citizens_attempts=std::max(out.citizens_attempts,impl_->ledger->citizens_attempts.load());
 out.citizens_frontier=impl_->ledger->citizens_frontier.load();
 if(impl_->retired.load()||impl_->ledger->quarantined.load())out.phase=BnwCityEntryPhase::quarantined;
 return out;
}
void BnwCityEntryPending::advance_damage_zero_prefix(){
 require_current();need(!impl_->ledger->attempted.load(),"City entry three native calls already attempted");
 const auto* resolved=resolve_bnw_city_registry(impl_->state->world,impl_->state->cities,
  impl_->progress.owner,impl_->progress.city);
 // Obtain writable fields through the actual privately owned allocation;
 // never const_cast a borrowed source, and allocate/check everything first.
 saves::SavedCityObservation* target=nullptr;
 for(auto& player:impl_->state->world.all_players->players)
  for(auto& candidate:player.cities.cities)if(&candidate==resolved)target=&candidate;
 need(target,"City entry actual writable city disappeared");
 std::int64_t *drafted=nullptr,*attack=nullptr,*sold_flag=nullptr;
 for(auto& f:target->core.fields){
  if(f.name&&std::string_view(f.name)=="m_bDrafted")drafted=&f.values.front();
  if(f.name&&std::string_view(f.name)=="m_bMadeAttack")attack=&f.values.front();
 }
 for(auto& b:target->production.blocks)if(b.name&&std::string_view(b.name)=="buildings")
  for(auto& f:b.fields)if(f.name&&std::string_view(f.name)=="m_bSoldBuildingThisTurn")sold_flag=&f.value;
 need(drafted&&attack&&sold_flag,"City entry prepared native stores unavailable");
 require_current();
 need(impl_->state->ui_source_revision<UINT64_MAX,"City entry UI source revision exhausted");
 impl_->ledger->attempted.store(true); // Retained once marker BEFORE first reset.
 // SDK CvCity.cpp1469/1470/1471. Core setters unconditionally assign. Building
 // setter performs its call but only stores if the old flag differs fromfalse.
 *drafted=0;impl_->progress.native_calls=1;impl_->progress.raw_assignments=1;
 *attack=0;impl_->progress.native_calls=2;impl_->progress.raw_assignments=2;
 if(*sold_flag!=0){*sold_flag=0;impl_->progress.raw_assignments=3;}
 ++impl_->state->ui_source_revision;
 impl_->progress.native_calls=3;impl_->progress.after={false,false,false};
 impl_->expected.flags=impl_->progress.after;
 impl_->progress.phase=BnwCityEntryPhase::feature_surrounded_pending;
 // No throwing callback/allocation/read/hash/validation follows source stores.
}
void BnwCityEntryPending::advance_feature_surrounded(const BnwCityEntryFeatureRules& supplied){
 const auto rules=supplied.impl_;supplied.require_current(); // Pin before any borrow.
 require_current();
 need(impl_->progress.phase==BnwCityEntryPhase::feature_surrounded_pending&&
  impl_->ledger->attempted.load()&&!impl_->ledger->feature_attempted.load(),
  "City feature requires unattempted SAME retained city-entry phase");
 need(rules->sha==impl_->identity.rules_sha256,"City feature actual rules/import identity differs");
 const auto facts=capture_feature(*impl_->state,*impl_->source,impl_->progress.owner,impl_->progress.city,*rules);
 std::array<bool,25> clearable{};std::uint8_t nulls=0,features=0;
 for(std::size_t i=0;i<facts.positions.size();++i){
  clearable[i]=facts.positions[i].clearable;
  if(!facts.positions[i].index)++nulls;
  if(clearable[i])++features;
 }
 const bool after=detail::bnw_city_feature_majority(clearable);
 const auto* resolved=resolve_bnw_city_registry(impl_->state->world,impl_->state->cities,
  impl_->progress.owner,impl_->progress.city);
 std::int64_t* flag_field=nullptr;
 for(auto& player:impl_->state->world.all_players->players)
  for(auto& city:player.cities.cities)if(&city==resolved)
   for(auto& field:city.core.fields)if(field.name&&std::string_view(field.name)=="m_bFeatureSurrounded")
    flag_field=&field.values.front();
 need(flag_field,"City feature actual writable flag disappeared");
 supplied.require_current();require_current();
 need(capture_feature(*impl_->state,*impl_->source,impl_->progress.owner,impl_->progress.city,*rules)==facts,
  "City feature actual source changed before store");
 if(facts.surrounded!=after)need(impl_->state->ui_source_revision<UINT64_MAX,"City feature UI source revision exhausted");
 impl_->feature_rules=rules;impl_->feature_expected=facts; // No-throw owned seals.
 impl_->ledger->feature_attempted.store(true); // BEFORE conditional source setter.
 if(facts.surrounded!=after){*flag_field=after?1:0;++impl_->state->ui_source_revision;}
 impl_->feature_expected->surrounded=after;
 impl_->progress.feature_positions=25;impl_->progress.feature_null_positions=nulls;
 impl_->progress.feature_clearable_positions=features;impl_->progress.feature_before=facts.surrounded;
 impl_->progress.feature_after=after;impl_->progress.feature_assigned=facts.surrounded!=after;
 impl_->progress.phase=BnwCityEntryPhase::citystrategy_pending;
 // No allocation, callback, hash, getter or fallible check follows the store.
}
void BnwCityEntryPending::advance_strategy_one(std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();need(impl_->progress.phase==BnwCityEntryPhase::citystrategy_pending&&
  impl_->ledger->attempted.load()&&impl_->ledger->feature_attempted.load(),
  "City strategy requires exact completed city-entry source predecessor");
 auto& p=*impl_;need(host&&host->session().read_state_lease()==p.state&&
  &host->session().source()==p.source.get()&&host->identity().payload_sha256==p.identity.payload_sha256&&
  host->identity().rules_sha256==p.identity.rules_sha256,"City strategy SAME Host/state/source identity differs");
 (void)host->session().bnw_game_debug_mode();const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard()&&host->database().gameplay_fingerprint_sha256()==p.identity.rules_sha256,
  "City strategy actual immutable rules scope retired");
 // A frontier is owned history, not permission to replay its trigger/helper.
 if(p.progress.strategy_frontier!=BnwCityStrategyLoopFrontier::none)return;
 if(!p.strategy_node){
  const auto* raw=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
  need(raw,"City strategy genuine runtime Read city unavailable");
  const auto previous=p.state->city_strategy_runtime.find(std::pair{p.progress.owner,p.progress.city});
  need(previous==p.state->city_strategy_runtime.end()||bool(previous->second),"City strategy registered runtime node is null");
  const auto revision=previous==p.state->city_strategy_runtime.end()?p.progress.strategy_revision:previous->second->revision();
  if(previous!=p.state->city_strategy_runtime.end())previous->second->require_binding(*p.state,p.progress.owner,p.progress.city,
   std::get<3>(p.expected.key),std::get<4>(p.expected.key),revision);
  auto node=BnwCityStrategyRuntime::read_current(*host,*p.state,raw->management,p.progress.owner,p.progress.city,
   std::get<3>(p.expected.key),std::get<4>(p.expected.key),revision);
  if(previous!=p.state->city_strategy_runtime.end())need(previous->second->values()==node->values(),
   "City strategy existing authoritative vectors diverged from current raw management");
  auto section=make_bnw_city_management_checkpoint(p.state->world,p.state->world,p.identity,true);
  std::optional<saves::CivisSaveSection> seal(std::move(section));
  require_current();need(guard(),"City strategy rules retired before own Init/Read publication");
  // Map allocation is before any effect marker/store. A pre-existing node is
  // not borrowed as a new source proof; actual Read constructs a replacement.
  need(p.state->ui_source_revision<UINT64_MAX,"City strategy UI source revision exhausted");
  auto [position,inserted]=p.state->city_strategy_runtime.try_emplace(std::pair{p.progress.owner,p.progress.city},node);
  if(!inserted)position->second.swap(node);
  p.strategy_node=position->second;p.strategy_beforeimage.swap(seal);
  ++p.state->ui_source_revision;
  p.progress.strategy_revision=revision;p.ledger->strategy_revision.store(revision);
 }
 auto query=BnwCityCurrentStrategyEligibility::prepare_ordered(host,p.progress.owner,p.progress.city);
 const auto slots=query.native_slots();
 if(!p.progress.strategy_started){
  auto retained_guard=guard;auto retained_sha=p.identity.rules_sha256;
  require_current();need(guard(),"City strategy rules retired before cursor binding");
  p.strategy_rules_current.swap(retained_guard);p.strategy_rules_sha.swap(retained_sha);
  p.progress.strategy_slots=slots;p.progress.strategy_started=true;
  p.ledger->strategy_slots.store(slots);p.ledger->strategy_started.store(true);
 }else need(p.progress.strategy_slots==slots,"City strategy native collection span changed");
 const auto frontier=[&](BnwCityStrategyLoopFrontier value)noexcept{
  p.ledger->strategy_frontier.store(value);p.progress.strategy_frontier=value;
 };
 const auto complete_no_effect=[&]()noexcept{
  // Retained monotone marker BEFORE advancing this exact source index.
  p.ledger->strategy_steps.store(p.progress.strategy_steps+1);
  p.ledger->strategy_next.store(p.progress.strategy_next_id+1);
  ++p.progress.strategy_steps;++p.progress.strategy_next_id;
 };
 const auto publish_end=[&](const BnwCityStrategyObsoleteEndStage& stage){
  need(p.progress.strategy_revision<UINT64_MAX&&p.progress.strategy_effect_attempts<UINT64_MAX&&
   p.progress.strategy_publications<UINT64_MAX,"City strategy native publication history exhausted");
  const auto staged=stage.read_world();auto section=stage.management_section();
  const auto computed=stage.computed_flavor_result();
  need(section.id==27,"City strategy effect stage domain differs");
  const auto* raw=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
  need(raw,"City strategy actual target disappeared before effect publication");
  const auto& staged_city=staged->all_players->players.at(static_cast<std::size_t>(p.progress.owner)).cities.cities.at(static_cast<std::size_t>(raw->index));
  need(scalar(staged_city,"m_iID")==p.progress.city&&scalar(staged_city,"m_eOwner")==p.progress.owner&&
   staged_city.core.start_offset==std::get<3>(p.expected.key)&&staged_city.core.end_offset==std::get<4>(p.expected.key),
   "City strategy staged effect native fullID/core span differs");
  auto management=staged_city.management;
  auto replacement=BnwCityStrategyRuntime::read_current(*host,*p.state,management,p.progress.owner,p.progress.city,
   std::get<3>(p.expected.key),std::get<4>(p.expected.key),p.progress.strategy_revision+1);
  const auto readback=replacement->values();
  need(readback.latest==computed.latest&&readback.weights==computed.weights,
   "City strategy raw native Read failed to reproduce actual FlavorUpdate all4 values");
  std::optional<saves::CivisSaveSection> seal(std::move(section));
  saves::CityManagementObservation* target=nullptr;
  for(auto& player:p.state->world.all_players->players)for(auto& city:player.cities.cities)if(&city==raw)target=&city.management;
  auto position=p.state->city_strategy_runtime.find(std::pair{p.progress.owner,p.progress.city});
  need(target&&position!=p.state->city_strategy_runtime.end()&&position->second==p.strategy_node,
   "City strategy actual writable raw/owned target changed");
  require_current();need(guard(),"City strategy rules retired before effect stores");
  // NO allocations, callbacks, native getters or fallible checks after marker.
  static_assert(std::is_nothrow_swappable_v<saves::CityManagementObservation>);
  static_assert(std::is_nothrow_swappable_v<decltype(p.strategy_beforeimage)>);
  need(p.state->ui_source_revision<UINT64_MAX,"City strategy effect UI source revision exhausted");
  p.ledger->strategy_effect_attempts.store(p.progress.strategy_effect_attempts+1);
  ++p.progress.strategy_effect_attempts;
  using std::swap;swap(*target,management);position->second.swap(replacement);
  p.strategy_node=position->second;p.strategy_beforeimage.swap(seal);
  ++p.state->ui_source_revision;
  ++p.progress.strategy_revision;++p.progress.strategy_publications;
  p.ledger->strategy_revision.store(p.progress.strategy_revision);
  p.ledger->strategy_publications.store(p.progress.strategy_publications);
  complete_no_effect(); // Native loop advance follows actual full bounded end effect.
 };
 const auto id=p.progress.strategy_next_id;
 need(id>=0&&id<=slots,"City strategy native loop cursor outside collection");
 if(id==slots){frontier(BnwCityStrategyLoopFrontier::loop_end_before_remaining_city_ai);return;}
 std::optional<BnwCityStrategyEligibilityFact> current;
 try{current=query.record_at(id);}catch(const BnwCityCurrentStrategyEligibilityUnavailable& e){
  frontier(std::string_view(e.what()).find("NULL StrategyInfo")!=std::string_view::npos?
   BnwCityStrategyLoopFrontier::native_null_strategy:BnwCityStrategyLoopFrontier::source_inputs_unavailable);throw;
 }
 const auto& fact=*current;need(fact.native_id==id,"City strategy reached ID differs from ordered cursor");
 if(fact.obligation==BnwCityStrategyTriggerObligation::none||
    fact.obligation==BnwCityStrategyTriggerObligation::skipped_for_minor){
  require_current();need(guard(),"City strategy rules retired before no-effect index advance");
  complete_no_effect();return;
 }
 try{
  if(query.obsolete_for_dispatch(id)){
   need(fact.obligation==BnwCityStrategyTriggerObligation::end,"City strategy obsolete dispatch violates actual eligibility");
   // Genuine full transition computes on PRIVATE staging, not live cache
   // publication. Never retain Stage -> Host -> Session inside Session -> Pending.
   auto owner=bind_bnw_city_strategy_obsolete_end(host,p.progress.owner,p.progress.city);
   auto stage=owner.prepare(id);publish_end(stage);return;
  }
 }catch(...){frontier(BnwCityStrategyLoopFrontier::source_inputs_unavailable);throw;}
 const auto trigger=[&]()->bool{
  if(fact.type=="AICITYSTRATEGY_TINY_CITY")return inspect_bnw_city_current_simple_strategy(host,p.progress.owner,p.progress.city).value(BnwSimpleCityStrategy::tiny);
  if(fact.type=="AICITYSTRATEGY_SMALL_CITY")return inspect_bnw_city_current_simple_strategy(host,p.progress.owner,p.progress.city).value(BnwSimpleCityStrategy::small);
  if(fact.type=="AICITYSTRATEGY_MEDIUM_CITY")return inspect_bnw_city_current_simple_strategy(host,p.progress.owner,p.progress.city).value(BnwSimpleCityStrategy::medium);
  if(fact.type=="AICITYSTRATEGY_LARGE_CITY")return inspect_bnw_city_current_simple_strategy(host,p.progress.owner,p.progress.city).value(BnwSimpleCityStrategy::large);
  if(fact.type=="AICITYSTRATEGY_IS_PUPPET")return inspect_bnw_city_current_simple_strategy(host,p.progress.owner,p.progress.city).value(BnwSimpleCityStrategy::is_puppet);
  if(fact.type=="AICITYSTRATEGY_HAVE_TRAINING_FACILITY")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::training_facility);
  if(fact.type=="AICITYSTRATEGY_FIRST_CULTURE_BUILDING_EMERGENCY")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::first_culture_emergency);
  if(fact.type=="AICITYSTRATEGY_FIRST_CULTURE_BUILDING")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::first_culture);
  if(fact.type=="AICITYSTRATEGY_FIRST_SCIENCE_BUILDING")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::first_science);
  if(fact.type=="AICITYSTRATEGY_FIRST_GOLD_BUILDING")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::first_gold);
  if(fact.type=="AICITYSTRATEGY_FIRST_FAITH_BUILDING")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::first_faith);
  if(fact.type=="AICITYSTRATEGY_FIRST_PRODUCTION_BUILDING")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::first_production);
  if(fact.type=="AICITYSTRATEGY_MEDIUM_CITY_HIGH_DIFFICULTY")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::medium_high_difficulty);
  if(fact.type=="AICITYSTRATEGY_ORIGINAL_CAPITAL")return inspect_bnw_city_current_building_strategy(host,p.progress.owner,p.progress.city).value(BnwBuildingCityStrategy::original_capital);
  if(fact.type=="AICITYSTRATEGY_RIVER_CITY")return inspect_bnw_city_current_geographic_strategy(host,p.progress.owner,p.progress.city).value(BnwGeographicCityStrategy::river);
  if(fact.type=="AICITYSTRATEGY_MOUNTAIN_CITY")return inspect_bnw_city_current_geographic_strategy(host,p.progress.owner,p.progress.city).value(BnwGeographicCityStrategy::mountain);
  if(fact.type=="AICITYSTRATEGY_FOREST_CITY")return inspect_bnw_city_current_geographic_strategy(host,p.progress.owner,p.progress.city).value(BnwGeographicCityStrategy::forest);
  if(fact.type=="AICITYSTRATEGY_HILL_CITY")return inspect_bnw_city_current_geographic_strategy(host,p.progress.owner,p.progress.city).value(BnwGeographicCityStrategy::hill);
  if(fact.type=="AICITYSTRATEGY_JUNGLE_CITY")return inspect_bnw_city_current_geographic_strategy(host,p.progress.owner,p.progress.city).value(BnwGeographicCityStrategy::jungle);
  if(fact.type=="AICITYSTRATEGY_COAST_CITY")return inspect_bnw_city_current_coastal_strategy(host,p.progress.owner,p.progress.city).value(BnwCoastalCityStrategy::coast);
  if(fact.type=="AICITYSTRATEGY_LANDLOCKED")return inspect_bnw_city_current_coastal_strategy(host,p.progress.owner,p.progress.city).value(BnwCoastalCityStrategy::landlocked);
  if(fact.type=="AICITYSTRATEGY_NEED_INTERNATIONAL_LAND_TRADE_ROUTE")return inspect_bnw_city_current_trade_strategy(host,p.progress.owner,p.progress.city).value(BnwTradeCityStrategy::need_land);
  if(fact.type=="AICITYSTRATEGY_NO_NEED_INTERNATIONAL_LAND_TRADE_ROUTE")return inspect_bnw_city_current_trade_strategy(host,p.progress.owner,p.progress.city).value(BnwTradeCityStrategy::no_need_land);
  if(fact.type=="AICITYSTRATEGY_NEED_INTERNATIONAL_SEA_TRADE_ROUTE")return inspect_bnw_city_current_trade_strategy(host,p.progress.owner,p.progress.city).value(BnwTradeCityStrategy::need_sea);
  if(fact.type=="AICITYSTRATEGY_NO_NEED_INTERNATIONAL_SEA_TRADE_ROUTE")return inspect_bnw_city_current_trade_strategy(host,p.progress.owner,p.progress.city).value(BnwTradeCityStrategy::no_need_sea);
  if(fact.type=="AICITYSTRATEGY_INTERNATIONAL_TRADE_DESTINATION")return inspect_bnw_city_current_trade_strategy(host,p.progress.owner,p.progress.city).value(BnwTradeCityStrategy::international_destination);
  if(fact.type=="AICITYSTRATEGY_INTERNATIONAL_TRADE_ORIGIN")return inspect_bnw_city_current_trade_strategy(host,p.progress.owner,p.progress.city).value(BnwTradeCityStrategy::international_origin);
  if(fact.type=="AICITYSTRATEGY_MANY_TECHS_STOLEN")return inspect_bnw_city_current_support_strategy(host,p.progress.owner,p.progress.city).value(BnwSupportCityStrategy::many_techs_stolen);
  if(fact.type=="AICITYSTRATEGY_KEY_SCIENCE_CITY")return inspect_bnw_city_current_support_strategy(host,p.progress.owner,p.progress.city).value(BnwSupportCityStrategy::key_science_city);
  if(fact.type=="AICITYSTRATEGY_GOOD_GP_CITY")return inspect_bnw_city_current_support_strategy(host,p.progress.owner,p.progress.city).value(BnwSupportCityStrategy::good_gp_city);
  if(fact.type=="AICITYSTRATEGY_NEED_CULTURE_BUILDING")return inspect_bnw_city_current_support_strategy(host,p.progress.owner,p.progress.city).value(BnwSupportCityStrategy::need_culture_building);
  if(fact.type=="AICITYSTRATEGY_NEED_TOURISM_BUILDING")return inspect_bnw_city_current_support_strategy(host,p.progress.owner,p.progress.city).value(BnwSupportCityStrategy::need_tourism_building);
  if(fact.type=="AICITYSTRATEGY_GOOD_AIRLIFT_CITY")return inspect_bnw_city_current_support_strategy(host,p.progress.owner,p.progress.city).value(BnwSupportCityStrategy::good_airlift_city);
  if(fact.type=="AICITYSTRATEGY_NEED_NAVAL_GROWTH")return inspect_bnw_city_current_naval_strategy(host,p.progress.owner,p.progress.city).value(BnwNavalCityStrategy::need_naval_growth);
  if(fact.type=="AICITYSTRATEGY_NEED_NAVAL_TILE_IMPROVEMENT")return inspect_bnw_city_current_naval_strategy(host,p.progress.owner,p.progress.city).value(BnwNavalCityStrategy::need_naval_tile_improvement);
  if(fact.type=="AICITYSTRATEGY_ENOUGH_NAVAL_TILE_IMPROVEMENT")return inspect_bnw_city_current_naval_strategy(host,p.progress.owner,p.progress.city).value(BnwNavalCityStrategy::enough_naval_tile_improvement);
  if(fact.type=="AICITYSTRATEGY_UNDER_BLOCKADE")return inspect_bnw_city_current_naval_strategy(host,p.progress.owner,p.progress.city).value(BnwNavalCityStrategy::under_blockade);
  if(fact.type=="AICITYSTRATEGY_NEED_TILE_IMPROVERS")return inspect_bnw_city_current_land_improvement_strategy(host,p.progress.owner,p.progress.city).value(BnwLandImprovementCityStrategy::need_tile_improvers);
  if(fact.type=="AICITYSTRATEGY_WANT_TILE_IMPROVERS")return inspect_bnw_city_current_land_improvement_strategy(host,p.progress.owner,p.progress.city).value(BnwLandImprovementCityStrategy::want_tile_improvers);
  if(fact.type=="AICITYSTRATEGY_ENOUGH_TILE_IMPROVERS")return inspect_bnw_city_current_land_improvement_strategy(host,p.progress.owner,p.progress.city).value(BnwLandImprovementCityStrategy::enough_tile_improvers);
  if(fact.type=="AICITYSTRATEGY_NEED_IMPROVEMENT_FOOD")return inspect_bnw_city_current_land_improvement_strategy(host,p.progress.owner,p.progress.city).value(BnwLandImprovementCityStrategy::need_improvement_food);
  if(fact.type=="AICITYSTRATEGY_NEED_IMPROVEMENT_PRODUCTION")return inspect_bnw_city_current_land_improvement_strategy(host,p.progress.owner,p.progress.city).value(BnwLandImprovementCityStrategy::need_improvement_production);
  if(fact.type=="AICITYSTRATEGY_CAPITAL_NEED_SETTLER")return inspect_bnw_city_current_capital_strategy(host,p.progress.owner,p.progress.city).value(BnwCapitalCityStrategy::need_settler);
  if(fact.type=="AICITYSTRATEGY_CAPITAL_UNDER_THREAT")return inspect_bnw_city_current_capital_strategy(host,p.progress.owner,p.progress.city).value(BnwCapitalCityStrategy::under_threat);
  // SDK has no final else: an actual mod Type outside all 44 names stays false.
  // This derives from current genuine Info Type, never a caller supplied bool.
  return false;
 };
 // History is marked BEFORE the real helper (which may throw while reading
 // unavailable actual operands). No failed/true helper can be replayed.
 const bool recognized=fact.type=="AICITYSTRATEGY_TINY_CITY"||
  fact.type=="AICITYSTRATEGY_SMALL_CITY"||
  fact.type=="AICITYSTRATEGY_MEDIUM_CITY"||
  fact.type=="AICITYSTRATEGY_LARGE_CITY"||
  fact.type=="AICITYSTRATEGY_LANDLOCKED"||
  fact.type=="AICITYSTRATEGY_HAVE_TRAINING_FACILITY"||
  fact.type=="AICITYSTRATEGY_FIRST_CULTURE_BUILDING_EMERGENCY"||
  fact.type=="AICITYSTRATEGY_FIRST_CULTURE_BUILDING"||
  fact.type=="AICITYSTRATEGY_FIRST_SCIENCE_BUILDING"||
  fact.type=="AICITYSTRATEGY_FIRST_GOLD_BUILDING"||
  fact.type=="AICITYSTRATEGY_FIRST_FAITH_BUILDING"||
  fact.type=="AICITYSTRATEGY_FIRST_PRODUCTION_BUILDING"||
  fact.type=="AICITYSTRATEGY_IS_PUPPET"||
  fact.type=="AICITYSTRATEGY_MEDIUM_CITY_HIGH_DIFFICULTY"||
  fact.type=="AICITYSTRATEGY_ORIGINAL_CAPITAL"||
  fact.type=="AICITYSTRATEGY_RIVER_CITY"||
  fact.type=="AICITYSTRATEGY_MOUNTAIN_CITY"||
  fact.type=="AICITYSTRATEGY_FOREST_CITY"||
  fact.type=="AICITYSTRATEGY_HILL_CITY"||
  fact.type=="AICITYSTRATEGY_JUNGLE_CITY"||
  fact.type=="AICITYSTRATEGY_COAST_CITY"||
  fact.type=="AICITYSTRATEGY_NEED_INTERNATIONAL_LAND_TRADE_ROUTE"||
  fact.type=="AICITYSTRATEGY_NO_NEED_INTERNATIONAL_LAND_TRADE_ROUTE"||
  fact.type=="AICITYSTRATEGY_NEED_INTERNATIONAL_SEA_TRADE_ROUTE"||
  fact.type=="AICITYSTRATEGY_NO_NEED_INTERNATIONAL_SEA_TRADE_ROUTE"||
  fact.type=="AICITYSTRATEGY_INTERNATIONAL_TRADE_DESTINATION"||
  fact.type=="AICITYSTRATEGY_INTERNATIONAL_TRADE_ORIGIN"||
  fact.type=="AICITYSTRATEGY_MANY_TECHS_STOLEN"||
  fact.type=="AICITYSTRATEGY_KEY_SCIENCE_CITY"||
  fact.type=="AICITYSTRATEGY_GOOD_GP_CITY"||
  fact.type=="AICITYSTRATEGY_NEED_CULTURE_BUILDING"||
  fact.type=="AICITYSTRATEGY_NEED_TOURISM_BUILDING"||
  fact.type=="AICITYSTRATEGY_GOOD_AIRLIFT_CITY"||
  fact.type=="AICITYSTRATEGY_NEED_NAVAL_GROWTH"||
  fact.type=="AICITYSTRATEGY_NEED_NAVAL_TILE_IMPROVEMENT"||
  fact.type=="AICITYSTRATEGY_ENOUGH_NAVAL_TILE_IMPROVEMENT"||
  fact.type=="AICITYSTRATEGY_UNDER_BLOCKADE"||
  fact.type=="AICITYSTRATEGY_NEED_TILE_IMPROVERS"||
  fact.type=="AICITYSTRATEGY_WANT_TILE_IMPROVERS"||
  fact.type=="AICITYSTRATEGY_ENOUGH_TILE_IMPROVERS"||
  fact.type=="AICITYSTRATEGY_NEED_IMPROVEMENT_FOOD"||
  fact.type=="AICITYSTRATEGY_NEED_IMPROVEMENT_PRODUCTION"||
  fact.type=="AICITYSTRATEGY_CAPITAL_NEED_SETTLER"||
  fact.type=="AICITYSTRATEGY_CAPITAL_UNDER_THREAT";
 if(recognized){p.ledger->strategy_calls.store(p.progress.strategy_trigger_calls+1);++p.progress.strategy_trigger_calls;}
 bool active=false;try{active=trigger();require_current();need(guard(),"City strategy rules retired after helper");}
 catch(...){frontier(BnwCityStrategyLoopFrontier::source_inputs_unavailable);throw;}
 if(active){frontier(BnwCityStrategyLoopFrontier::gameplay_lua);return;}
 if(fact.obligation==BnwCityStrategyTriggerObligation::start){complete_no_effect();return;}
 // Only this actually reached false-END path can call the private preparation
 // method; no supplied boolean/strategy selector grants end publication.
 try{auto owner=bind_bnw_city_strategy_obsolete_end(host,p.progress.owner,p.progress.city);
  auto stage=owner.prepare_false_end(id);publish_end(stage);
 }catch(...){frontier(BnwCityStrategyLoopFrontier::source_inputs_unavailable);throw;}
}
std::vector<BnwCityDirtyKind> detail::bnw_citizens_outer_dirty_kinds(bool selected){
 std::vector<BnwCityDirtyKind> events;
 if(selected){events.push_back(BnwCityDirtyKind::city_info);events.push_back(BnwCityDirtyKind::city_screen);events.push_back(BnwCityDirtyKind::colored_plots);}
 events.push_back(BnwCityDirtyKind::city_info);return events;
}
detail::BnwCitizensUnworkPrefix detail::bnw_citizens_work_prefix(bool home,bool assigning,
 std::int32_t working,std::int32_t unassigned,
 const std::array<std::int16_t,6>& cached,const std::array<std::int32_t,6>& terrain){
 BnwCitizensUnworkPrefix out{working,unassigned,std::nullopt};
 if(!home){
  out.working=native(static_cast<std::int64_t>(working)+(assigning?1:-1));
  out.unassigned=native(static_cast<std::int64_t>(unassigned)+(assigning?-1:1));
  need(out.working>=0&&out.unassigned>=0,"Citizens native count underflow");
 }
 for(std::uint8_t y=0;y<6;++y){
  const auto delta=assigning?static_cast<std::int32_t>(cached[y]):-static_cast<std::int32_t>(cached[y]);
  if(delta!=0){out.first_terrain=std::pair{y,native(static_cast<std::int64_t>(terrain[y])+delta)};break;}
 }
 return out;
}
detail::BnwCitizensUnworkPrefix detail::bnw_citizens_unwork_prefix(bool home,
 std::int32_t working,std::int32_t unassigned,
 const std::array<std::int16_t,6>& cached,const std::array<std::int32_t,6>& terrain){
 return bnw_citizens_work_prefix(home,false,working,unassigned,cached,terrain);
}

std::int32_t detail::bnw_citizens_forced_demotions(std::int64_t forced,std::int64_t working){
 const auto f=native(forced),w=native(working);
 return native(static_cast<std::int64_t>(f)-w);
}

std::optional<std::uint64_t> BnwCityEntryPending::citizens_worst_plot(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,bool forced_only){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get(),
  "Citizens worst scan requires SAME actual Host/source");
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard(),"Citizens worst scan actual rules retired");
 const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 need(city&&p.state->world.map_plots&&p.state->world.map_projection,"Citizens worst scan actual city/map absent");
 const auto& map=*p.state->world.map_projection;
 const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};geometry.validate();
 const auto x=scalar(*city,"m_iX"),y=scalar(*city,"m_iY");
 const auto positions=detail::bnw_citizen_plot_positions(geometry,x,y);
 const auto& citizens=citizen_named(city->management.blocks,"citizens");
 std::int32_t best_value=-1;std::optional<std::size_t> best_slot;
 for(std::size_t i=1;i<positions.size();++i){
  if(!positions[i])continue;
  require_current();need(guard(),"Citizens worst scan rules retired before plot");
  const auto plot=static_cast<std::uint64_t>(*positions[i]);
  const auto& raw=p.state->world.map_plots->plots.at(static_cast<std::size_t>(plot));
  const auto& cell=map.cells.at(static_cast<std::size_t>(plot));
  need(raw.index==plot&&cell.index==plot&&raw.x==cell.x&&raw.y==cell.y&&cell.x<=INT32_MAX&&cell.y<=INT32_MAX,
   "Citizens worst scan raw/projected identity differs");
  if(!forced_only){
   const auto owner=native(citizen_named(raw.fields,"working_city_owner").value);
   const auto id=native(citizen_named(raw.fields,"working_city_id").value);
   need(owner>=-1&&owner<64&&cell.yield_city&&cell.yield_city->owner==owner&&cell.yield_city->id==id,
    "Citizens worst scan working-city mirror differs");
   const auto* current=owner==-1?nullptr:resolve_bnw_city_registry(p.state->world,p.state->cities,owner,id);
   if(!current||scalar(*current,"m_iID")!=p.progress.city)continue;
  }
  const auto slot=detail::bnw_citizen_plot_slot(map,x,y,static_cast<std::int32_t>(cell.x),static_cast<std::int32_t>(cell.y));
  need(slot>=0&&slot<37,"Citizens worst scan native inverse unavailable");
  const auto& filter=citizen_named(citizens.arrays,forced_only?"m_pabForcedWorkingPlot":"m_pabWorkingPlot");
  need(filter.columns==1&&filter.values.size()==37,"Citizens worst scan native flags absent");
  const auto active=filter.values.at(static_cast<std::size_t>(slot));
  need(active==0||active==1,"Citizens worst scan flag is not native bool");if(active==0)continue;
  auto value=citizens_plot_value(host,plot,false);
  if(!forced_only){
   const auto& forced=citizen_named(citizens.arrays,"m_pabForcedWorkingPlot");
   need(forced.columns==1&&forced.values.size()==37,"Citizens worst scan forced flags absent");
   const auto flag=forced.values.at(static_cast<std::size_t>(slot));
   need(flag==0||flag==1,"Citizens worst scan forced flag is not native bool");
   if(flag!=0)value=native(std::int64_t(value)+10000);
  }
  require_current();need(guard(),"Citizens worst scan rules retired after score");
  if(detail::bnw_citizen_plot_value_choose(best_value,value,false)){best_value=value;best_slot=i;}
 }
 require_current();need(guard(),"Citizens worst scan rules retired before final native lookup");
 if(!best_slot)return std::nullopt;
 // Native returns GetCityPlotFromIndex(bestID) again, preserving native slots.
 const auto refreshed=detail::bnw_citizen_plot_positions(geometry,x,y);
 need(refreshed==positions&&refreshed[*best_slot].has_value(),"Citizens worst scan selected slot changed");
 return static_cast<std::uint64_t>(*refreshed[*best_slot]);
}

std::optional<std::pair<std::int32_t,std::int32_t>> BnwCityEntryPending::citizens_worst_specialist(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&&&host->session().source()==p.source.get(),"Citizens worst specialist SAME source absent");
 const auto guard=host->database().immutable_preview_fingerprint_currentness();need(guard&&guard(),"Citizens worst specialist rules retired");
 CitizenBuildingsSource buildings_source(host,*p.state,p.source->snapshot,p.progress.owner,p.progress.city,[&]{require_current();need(guard(),"Citizens worst specialist rules retired before Info");},false);
 auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
 for(std::uint64_t slot=0;slot<buildings_source.slots;++slot){
  require_current();const auto id=static_cast<std::int32_t>(slot);const auto info=buildings_source.infos.find(id);
  if(info==buildings_source.infos.end())continue; // Native BuildingInfo NULL.
  std::optional<std::int32_t> specialist;
  const auto rows=host->database().query_rows({"SELECT ID,Type,SpecialistType FROM Buildings WHERE ID=?",{std::int64_t(id)}},[&](const auto& row){
   need(!specialist&&row.integer("ID")==id&&row.text("Type")==std::optional{info->second.type},"Citizens worst specialist Info identity differs");
   specialist=-1;const auto type=row.text("SpecialistType");if(type&&!type->empty())if(const auto* native_type=host->types().find(*type)){
    need(native_type->table=="Specialists","Citizens worst building SpecialistType remap foreign");specialist=native(native_type->id);}
  },2);require_current();need(guard()&&rows.all_succeeded()&&rows.rows==1&&specialist,"Citizens worst specialist reached Type unavailable");
  if(*specialist==-1)continue; // Actual eDontChangeSpecialist==NO_SPECIALIST.
  const auto assigned=counts.building_specialists_for_city(p.progress.owner,p.progress.city,id);
  if(assigned>0){(void)counts.reached_source_seal();return std::pair{id,*specialist};}
 }
 (void)counts.reached_source_seal();require_current();return std::nullopt;
}

void BnwCityEntryPending::advance_citizens_do_turn_one(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get()&&
  host->identity().payload_sha256==p.identity.payload_sha256&&host->identity().rules_sha256==p.identity.rules_sha256,
  "Citizens DoTurn continuation requires SAME actual Host/session/source");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard()&&host->database().gameplay_fingerprint_sha256()==p.identity.rules_sha256,
  "Citizens DoTurn SAME immutable rules retired");
 need(p.progress.citizens_started&&p.progress.citizens_next_plot==37&&p.ledger->citizens_next.load()==37&&
  !p.citizens_tail&&!p.citizens_specialist&&!p.religion_city_child&&!p.religion_owner,
  "Citizens DoTurn needs actual VerifyWorkingPlots return; child/Lua/UI prefix is not completion");
 const auto check=[&]{require_current();need(guard(),"Citizens DoTurn rules retired before source borrow");};
 check();
 const auto* resolved=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 saves::SavedCityObservation* city=nullptr;
 for(auto& player:p.state->world.all_players->players)for(auto& candidate:player.cities.cities)
  if(&candidate==resolved)city=&candidate;
 need(city,"Citizens DoTurn canonical mutable city absent");
 auto& citizens=citizen_named(city->management.blocks,"citizens");
 const auto marker=[&]{
  need(p.progress.citizens_attempts<UINT64_MAX,"Citizens DoTurn once history exhausted");
  ++p.progress.citizens_attempts;p.ledger->citizens_attempts.store(p.progress.citizens_attempts);
 };
 const auto frontier=[&](BnwCityCitizensVerifyFrontier f)noexcept{
  p.progress.citizens_frontier=f;p.ledger->citizens_frontier.store(f);
 };
 if(!p.citizens_do_turn){
  check();const bool puppet=flag(*city,"m_bPuppet");
  const auto* owned=resolve_bnw_city_yield_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
  need(owned&&owned->puppet==puppet,"Citizens actual puppet raw/owned mirror differs");
  check();marker();p.citizens_do_turn=CitizensDoTurnTail{};p.citizens_do_turn->puppet=puppet;
 }
 auto& tail=*p.citizens_do_turn;
 if(tail.step==CitizensDoTurnStep::focus){
  if(!tail.puppet){
   check();const bool human=detail::bnw_player_human_read(*p.state,p.progress.owner,host->database());check();
   marker();tail.step=human?CitizensDoTurnStep::validate_forced:CitizensDoTurnStep::nonhuman_specialization;
   if(!human)frontier(BnwCityCitizensVerifyFrontier::nonhuman_focus_specialization);
  }else{
   check();auto* focus=&citizen_named(citizens.fields,"m_eCityAIFocusTypes").value;
   const auto current=native(*focus);constexpr std::int32_t gold=2; // CvEnums CityAIFocusTypes
   if(current==gold){
    check();marker();tail.step=CitizensDoTurnStep::next_puppet_setting;
    frontier(BnwCityCitizensVerifyFrontier::puppet_next_setting);return;
   }
   auto world=p.state->world;
   const auto* staged=resolve_bnw_city_registry(world,p.state->cities,p.progress.owner,p.progress.city);
   saves::SavedCityObservation* target=nullptr;
   for(auto& player:world.all_players->players)for(auto& candidate:player.cities.cities)
    if(&candidate==staged)target=&candidate;
   need(target,"Citizens focus staging city absent");
   citizen_named(citizen_named(target->management.blocks,"citizens").fields,"m_eCityAIFocusTypes").value=gold;
   std::optional<saves::CivisSaveSection> seal(make_bnw_city_management_checkpoint(world,world,p.identity,true));
   need(p.strategy_beforeimage&&p.progress.citizens_native_writes<UINT64_MAX&&p.state->ui_source_revision<UINT64_MAX,
    "Citizens focus native seal/write/revision unavailable");
   static_assert(std::is_nothrow_swappable_v<decltype(seal)>);
   check();marker();
   *focus=gold;p.strategy_beforeimage.swap(seal);++p.state->ui_source_revision;
   ++p.progress.citizens_native_writes;p.ledger->citizens_writes.store(p.progress.citizens_native_writes);
   // SetFocusType has not returned: its nested reallocation precedes setters239/240.
   tail.step=CitizensDoTurnStep::validate_forced;
  }
 }
 if(tail.step==CitizensDoTurnStep::validate_forced){
  check();const auto forced=citizen_named(citizens.fields,"m_iNumForcedWorkingPlots").value;
  const auto working=citizen_named(citizens.fields,"m_iNumCitizensWorkingPlots").value;
  const auto surplus=detail::bnw_citizens_forced_demotions(forced,working);
  check();marker();tail.forced_demotions=surplus;
  tail.step=surplus>0?CitizensDoTurnStep::forced_demotion:CitizensDoTurnStep::remove_working;
  if(surplus>0)frontier(BnwCityCitizensVerifyFrontier::reallocation_forced_plot);
 }
 if(tail.step==CitizensDoTurnStep::forced_demotion){
  need(tail.forced_demotions>0&&tail.demotions_done>=0&&tail.demotions_done<=tail.forced_demotions,
   "Citizens forced demotion native loop metadata differs");
  if(tail.demotions_done<tail.forced_demotions){
   const auto chosen=citizens_worst_plot(host,true);check();
   if(chosen){
    const auto& map=*p.state->world.map_projection;const auto& cell=map.cells.at(static_cast<std::size_t>(*chosen));
    need(cell.index==*chosen&&cell.x<=INT32_MAX&&cell.y<=INT32_MAX,"Citizens forced setter plot identity differs");
    const auto slot=detail::bnw_citizen_plot_slot(map,scalar(*city,"m_iX"),scalar(*city,"m_iY"),
     static_cast<std::int32_t>(cell.x),static_cast<std::int32_t>(cell.y));
    need(slot>=0&&slot<37,"Citizens forced setter native inverse unavailable");
    auto& flags=citizen_named(citizens.arrays,"m_pabForcedWorkingPlot");
    need(flags.columns==1&&flags.values.size()==37,"Citizens forced setter native flags absent");
    const auto previous=flags.values.at(static_cast<std::size_t>(slot));
    need(previous==0||previous==1,"Citizens forced setter native bool unavailable");
    if(previous!=0){
     auto* count=&citizen_named(citizens.fields,"m_iNumForcedWorkingPlots").value;
     const auto after=native(std::int64_t(native(*count))-1);
     auto world=p.state->world;
     const auto* resolved_stage=resolve_bnw_city_registry(world,p.state->cities,p.progress.owner,p.progress.city);
     saves::SavedCityObservation* staged=nullptr;
     for(auto& player:world.all_players->players)for(auto& candidate:player.cities.cities)
      if(&candidate==resolved_stage)staged=&candidate;
     need(staged,"Citizens forced setter staged canonical city absent");
     auto& fields=citizen_named(staged->management.blocks,"citizens");
     citizen_named(fields.arrays,"m_pabForcedWorkingPlot").values.at(static_cast<std::size_t>(slot))=0;
     citizen_named(fields.fields,"m_iNumForcedWorkingPlots").value=after;
     std::optional<saves::CivisSaveSection> seal(make_bnw_city_management_checkpoint(world,world,p.identity,true));
     need(p.strategy_beforeimage&&p.progress.citizens_native_writes<=UINT64_MAX-2&&p.state->ui_source_revision<UINT64_MAX,
      "Citizens forced setter native seal/history/revision exhausted");
     static_assert(std::is_nothrow_swappable_v<decltype(seal)>);
     check();marker(); // Exactly BEFORE the first conditional native store.
     flags.values[static_cast<std::size_t>(slot)]=0;*count=after;p.strategy_beforeimage.swap(seal);
     ++p.state->ui_source_revision;p.progress.citizens_native_writes+=2;
     p.ledger->citizens_writes.store(p.progress.citizens_native_writes);
    }else{check();marker();} // Native setter called, no counter read/store.
   }else{check();marker();} // Void native demotion with no winning plot returns.
   ++tail.demotions_done; // No throwing work follows the effect in this call.
   frontier(BnwCityCitizensVerifyFrontier::reallocation_forced_plot);return;
  }
  check();marker();tail.step=CitizensDoTurnStep::remove_working;
 }
 if(tail.step==CitizensDoTurnStep::remove_working){
  check();const auto count=native(citizen_named(citizens.fields,"m_iNumCitizensWorkingPlots").value);
  check();marker();tail.working_removals=count;
  tail.step=count>0?CitizensDoTurnStep::remove_citizen:CitizensDoTurnStep::building_scan;
  frontier(count>0?BnwCityCitizensVerifyFrontier::reallocation_remove_working:BnwCityCitizensVerifyFrontier::reallocation_building_scan);
 }
 if(tail.step==CitizensDoTurnStep::remove_citizen){
  need(tail.working_removals>0&&tail.removals_done>=0&&tail.removals_done<=tail.working_removals,
   "Citizens working removal native loop metadata differs");
  if(tail.removals_done==tail.working_removals){
   check();marker();tail.step=CitizensDoTurnStep::building_scan;
   frontier(BnwCityCitizensVerifyFrontier::reallocation_building_scan);
  }else{
   // The native default iCurrentCityPopulation=-1 resolves this getter FIRST.
   check();const auto population=scalar(*city,"m_iPopulation");
   const auto* owned=resolve_bnw_city_yield_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
   need(owned&&owned->population==population,"Citizens removal raw/owned population differs");
   const auto unassigned=native(citizen_named(citizens.fields,"m_iNumUnassignedCitizens").value);
   check();const auto population_again=scalar(*city,"m_iPopulation");
   if(unassigned==population_again){
    check();marker();++tail.removals_done;
    frontier(BnwCityCitizensVerifyFrontier::reallocation_remove_working);return;
   }
   const auto defaults=native(citizen_named(citizens.fields,"m_iNumDefaultSpecialists").value);
   if(defaults>0){
    const auto forced=native(citizen_named(citizens.fields,"m_iNumForcedDefaultSpecialists").value);
    if(defaults>forced){begin_citizens_specialist_removal(std::move(host),-1,-1,false,false);return;}
    if(defaults>population){begin_citizens_specialist_removal(std::move(host),-1,-1,false,true);return;}
   }
   const auto chosen=citizens_worst_plot(host,false);check();
   if(!chosen){
    const auto worst=citizens_worst_specialist(host);check();
    if(worst){begin_citizens_specialist_removal(std::move(host),worst->first,worst->second,true,false);return;}
    check();marker();++tail.removals_done;frontier(BnwCityCitizensVerifyFrontier::reallocation_remove_working);return;
   }
   // Native DoReallocate uses bRemoveForcedStatus=false. This writer owns the
   // full flag/count/terrain/UI prefix, then returns to this exact loop call.
   set_citizens_working_plot(std::move(host),*chosen,false);return;
  }
 }
 if(tail.step==CitizensDoTurnStep::building_scan){
  CitizenBuildingsSource buildings_source(host,*p.state,p.source->snapshot,p.progress.owner,p.progress.city,check);
  need(tail.building_slot<=buildings_source.slots,"Citizens building removal cursor outside actual Info span");
  if(tail.building_slot==buildings_source.slots){
   check();const auto defaults=native(citizen_named(citizens.fields,"m_iNumDefaultSpecialists").value);
   const auto forced=native(citizen_named(citizens.fields,"m_iNumForcedDefaultSpecialists").value);
   const auto bound=native(std::int64_t(defaults)-forced);check();marker();tail.default_removals=bound;
   tail.step=bound>0?CitizensDoTurnStep::remove_defaults:CitizensDoTurnStep::allocation_pending;
   frontier(BnwCityCitizensVerifyFrontier::reallocation_building_scan);
  }else{
   const auto id=static_cast<std::int32_t>(tail.building_slot);
   if(!tail.building_captured){
    // Source GetNumBuilding FIRST, including NULL native Info slots.
    check();const auto count=buildings_source.count(tail.building_slot);
    if(count<=0){check();marker();++tail.building_slot;frontier(BnwCityCitizensVerifyFrontier::reallocation_building_scan);return;}
    auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
    const auto assigned=counts.building_specialists_for_city(p.progress.owner,p.progress.city,id);
    const auto forced=counts.forced_building_specialists_for_city(p.progress.owner,p.progress.city,id);
    const auto bound=native(std::int64_t(assigned)-forced);(void)counts.reached_source_seal();
    check();marker();tail.building_captured=true;tail.building_removals=bound;tail.building_done=0;
   }
   if(tail.building_done>=tail.building_removals){check();marker();++tail.building_slot;tail.building_captured=false;
    frontier(BnwCityCitizensVerifyFrontier::reallocation_building_scan);return;}
   const auto info=buildings_source.infos.find(id);
   if(info==buildings_source.infos.end()){check();marker();++tail.building_done;return;} // Native DoRemove NULL return.
   std::optional<std::int32_t> specialist;
   const auto rows=host->database().query_rows({"SELECT ID,Type,SpecialistType FROM Buildings WHERE ID=?",{std::int64_t(id)}},[&](const auto& row){
    need(!specialist&&row.integer("ID")==id&&row.text("Type")==std::optional{info->second.type},"Citizens reallocation BuildingInfo identity differs");
    specialist=-1;const auto type=row.text("SpecialistType");if(type&&!type->empty())if(const auto* native_type=host->types().find(*type)){
     need(native_type->table=="Specialists","Citizens reallocation Building SpecialistType remap foreign");specialist=native(native_type->id);}
   },2);check();need(rows.all_succeeded()&&rows.rows==1&&specialist,"Citizens reallocation Building SpecialistType unavailable");
   auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
   const auto assigned=counts.building_specialists_for_city(p.progress.owner,p.progress.city,id);(void)counts.reached_source_seal();
   if(assigned<=0){check();marker();++tail.building_done;return;}
   begin_citizens_specialist_removal(std::move(host),id,*specialist,false,false);return;
  }
 }
 if(tail.step==CitizensDoTurnStep::remove_defaults){
  need(tail.default_removals>0&&tail.defaults_done>=0&&tail.defaults_done<=tail.default_removals,"Citizens default removal captured loop differs");
  if(tail.defaults_done<tail.default_removals){begin_citizens_specialist_removal(std::move(host),-1,-1,false,false);return;}
  check();marker();tail.step=CitizensDoTurnStep::allocation_pending;
 }
 need(tail.step!=CitizensDoTurnStep::allocation_pending,
  "Citizens reallocation requires actual captured DoAddBestCitizen/GP continuation after signed removals");
 // Named reached effects remain mandatory. No original getter/callback is replaced by success.
 need(tail.step!=CitizensDoTurnStep::next_puppet_setting,
  "Citizens puppet SetNoAutoAssignSpecialists/SetForcedAvoidGrowth requires their ordered nested continuations");
 need(tail.step!=CitizensDoTurnStep::nonhuman_specialization,
  "Citizens nonhuman focus requires actual CitySpecializationAI/economic strategy owners");
 need(tail.step!=CitizensDoTurnStep::forced_demotion,
  "Citizens reallocation requires actual ordered GetPlotValue(false)/DoDemoteWorstForcedWorkingPlot");
 need(tail.step!=CitizensDoTurnStep::remove_citizen,
  "Citizens reallocation requires actual DoRemoveWorstCitizen before BuildingInfo scan");

}

void BnwCityEntryPending::advance_citizens_verify_one(LiveSession& session,std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&&host->session()==&session&&session.state_.get()==p.state.get()&&session.source_==p.source&&session.debug_==p.game,
  "Citizens continuation needs actual owning Session/Host before mutation");
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get()&&
  host->identity().payload_sha256==p.identity.payload_sha256&&host->identity().rules_sha256==p.identity.rules_sha256,
  "Citizens verification requires SAME actual Host/session/source");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard()&&host->database().gameplay_fingerprint_sha256()==p.identity.rules_sha256,
  "Citizens SAME immutable rules retired");
 need(p.progress.strategy_started&&p.progress.strategy_next_id==p.progress.strategy_slots&&
  p.progress.strategy_frontier==BnwCityStrategyLoopFrontier::loop_end_before_remaining_city_ai,
  "Citizens requires actual native strategy-loop end; Lua/source/trigger frontier is not completion");
 if(p.progress.citizens_started&&p.progress.citizens_frontier!=BnwCityCitizensVerifyFrontier::none){
  if(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::inner_city_screen_predicate||
     p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::remaining_terrain||
     p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::outer_city_selected)
   advance_citizens_ui_one(std::move(host));
  else if(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_process)
   advance_citizens_specialist_one(std::move(host));
  else if(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::reassignment)
   advance_citizens_reassignment_one(std::move(host));
  else if(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_update_religion)
   advance_citizens_update_religion(session,std::move(host));
  else if(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::focus_and_specialists||
          p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::puppet_next_setting||
          p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::nonhuman_focus_specialization||
          p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::reallocation_forced_plot||
          p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::reallocation_remove_working||
          p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::reallocation_building_scan)
   advance_citizens_do_turn_one(std::move(host));
  return;
 }
 const auto resolved=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 need(resolved,"Citizens canonical current city unavailable");
 saves::SavedCityObservation* city=nullptr;
 for(auto& player:p.state->world.all_players->players)
  for(auto& candidate:player.cities.cities)if(&candidate==resolved)city=&candidate;
 need(city,"Citizens actual mutable city allocation absent");
 const auto& map=*p.state->world.map_projection;
 const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};geometry.validate();
 const auto x=scalar(*city,"m_iX"),y=scalar(*city,"m_iY");
 const auto slots=detail::bnw_citizen_plot_positions(geometry,x,y);
 auto& citizens=citizen_named(city->management.blocks,"citizens");
 auto& flags=citizen_named(citizens.arrays,"m_pabWorkingPlot");
 need(flags.columns==1&&flags.values.size()==37,"Citizens actual native37 working flags absent");
 for(const auto f:flags.values)need(f==0||f==1,"Citizens working flag is not native boolean");
 const auto index=p.progress.citizens_next_plot;
 need(index<=37,"Citizens verify native cursor outside37 slots");
 if(!p.progress.citizens_started){
  p.ledger->citizens_started.store(true);p.progress.citizens_started=true;
  p.ledger->citizens_frontier.store(BnwCityCitizensVerifyFrontier::none);
  p.progress.citizens_frontier=BnwCityCitizensVerifyFrontier::none;
 }
 const auto advance=[&]()noexcept{
  p.ledger->citizens_next.store(static_cast<std::uint8_t>(index+1));
  p.progress.citizens_next_plot=static_cast<std::uint8_t>(index+1);
 };
 if(index==37){
  p.ledger->citizens_frontier.store(BnwCityCitizensVerifyFrontier::focus_and_specialists);
  p.progress.citizens_frontier=BnwCityCitizensVerifyFrontier::focus_and_specialists;return;
 }
 if(!slots[index]){require_current();need(guard(),"Citizens rules retired before NULL slot");advance();return;}
 const auto plot=static_cast<std::uint64_t>(*slots[index]);
 const auto& cell=map.cells.at(static_cast<std::size_t>(plot));
 need(cell.index==plot&&cell.x<=INT32_MAX&&cell.y<=INT32_MAX,"Citizens actual plot identity differs");
 const auto slot=detail::bnw_citizen_plot_slot(map,x,y,static_cast<std::int32_t>(cell.x),static_cast<std::int32_t>(cell.y));
 need(slot>=0&&slot<37,"Citizens native inverse working slot outside37");
 if(flags.values.at(static_cast<std::size_t>(slot))==0){
  require_current();need(guard(),"Citizens rules retired before unworked slot");advance();return;
 }
 const auto can_work=inspect_bnw_city_current_naval_strategy(host,p.progress.owner,p.progress.city).can_work(plot);
 require_current();need(guard(),"Citizens rules retired after actual can_work");
 if(can_work){advance();return;}
 set_citizens_working_plot(std::move(host),plot,false);
}

void BnwCityEntryPending::set_citizens_working_plot(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::uint64_t plot,bool assigning){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get(),
  "Citizens assignment requires SAME actual Host/source");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard()&&host->identity().rules_sha256==p.identity.rules_sha256,
  "Citizens assignment actual rules owner differs");
 const auto* resolved=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 saves::SavedCityObservation* city=nullptr;
 for(auto& player:p.state->world.all_players->players)
  for(auto& candidate:player.cities.cities)if(&candidate==resolved)city=&candidate;
 need(city,"Citizens assignment canonical allocation absent");
 const auto& map=*p.state->world.map_projection;
 const auto& cell=map.cells.at(static_cast<std::size_t>(plot));
 need(cell.index==plot&&cell.x<=INT32_MAX&&cell.y<=INT32_MAX,"Citizens assignment plot identity differs");
 const auto slot=detail::bnw_citizen_plot_slot(map,scalar(*city,"m_iX"),scalar(*city,"m_iY"),
  static_cast<std::int32_t>(cell.x),static_cast<std::int32_t>(cell.y));
 need(slot>=0&&slot<37,"Citizens assignment inverse slot unavailable");
 auto& citizens=citizen_named(city->management.blocks,"citizens");
 auto& flags=citizen_named(citizens.arrays,"m_pabWorkingPlot");
 need(flags.columns==1&&flags.values.size()==37,"Citizens assignment working flags shape differs");
 for(const auto f:flags.values)need(f==0||f==1,"Citizens assignment flag is not native bool");
 need((flags.values.at(static_cast<std::size_t>(slot))!=0)!=assigning,
  "Citizens selected assignment no longer changes actual working flag");
 // The private caller has already reached actual native assignment/unassignment.
 // All staging and native arithmetic checks precede its first scalar store.
 const auto* source_city=resolve_bnw_city_registry(p.source->snapshot,p.state->cities,p.progress.owner,p.progress.city);
 need(source_city,"Citizens immutable original city absent");
 const auto& original=*source_city;
 need(city->management.start_offset==original.management.start_offset&&city->management.end_offset==original.management.end_offset,
  "Citizens original management source span differs");
 auto staged_world=p.state->world;
 const auto* staged=resolve_bnw_city_registry(staged_world,p.state->cities,p.progress.owner,p.progress.city);
 // Do not write through a borrowed const result; locate mutable staged owner.
 saves::SavedCityObservation* target=nullptr;
 for(auto& player:staged_world.all_players->players)
  for(auto& candidate:player.cities.cities)if(&candidate==staged)target=&candidate;
 need(target,"Citizens private detached staging city absent");
 auto& staged_citizens=citizen_named(target->management.blocks,"citizens");
 auto& staged_flags=citizen_named(staged_citizens.arrays,"m_pabWorkingPlot");
 staged_flags.values.at(static_cast<std::size_t>(slot))=assigning?1:0;
 std::int32_t working=0,unassigned=0;
 if(slot!=0){working=native(citizen_named(citizens.fields,"m_iNumCitizensWorkingPlots").value);
  unassigned=native(citizen_named(citizens.fields,"m_iNumUnassignedCitizens").value);}
 need(p.state->world.map_plots&&p.source->snapshot.map_plots&&plot<p.state->world.map_plots->plots.size(),
  "Citizens actual current map cache source absent");
 const auto& raw_plot=p.state->world.map_plots->plots.at(static_cast<std::size_t>(plot));
 const auto& old_plot=p.source->snapshot.map_plots->plots.at(static_cast<std::size_t>(plot));
 need(raw_plot.start_offset==old_plot.start_offset&&raw_plot.end_offset==old_plot.end_offset&&raw_plot.version==old_plot.version,
  "Citizens actual cached plot source span differs");
 const auto& cache=citizen_named(raw_plot.arrays,"yields");
 need(cache.columns==1&&cache.values.size()==6&&cell.cached_yields&&cell.cached_yields->size()==6,
  "Citizens current native six short cache shape differs");
 std::array<std::int16_t,6> cached{};
 for(std::size_t yi=0;yi<6;++yi){
  const auto n=cache.values[yi];const auto found=cell.cached_yields->find(static_cast<std::int64_t>(yi));
  need(n>=INT16_MIN&&n<=INT16_MAX&&found!=cell.cached_yields->end()&&found->second==n,
   "Citizens raw/owned signed short cache differs");cached[yi]=static_cast<std::int16_t>(n);
 }
 auto& owned=p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city);
 std::optional<CitizensTerrainSeal> terrain;
 auto staged_yields=owned.yields;
 std::array<std::int32_t,6> current_terrain{};
 if(std::any_of(cached.begin(),cached.end(),[](auto n){return n!=0;})){
  const auto& base=citizen_named(city->core.fields,"m_aiBaseYieldRateFromTerrain");
  const auto& source_base=citizen_named(original.core.fields,"m_aiBaseYieldRateFromTerrain");
  need(base.encoding==saves::CityCoreEncoding::vector_i32&&base.wire_count==6&&base.values.size()==6&&
   base.hashes.empty()&&base.bytes.empty()&&base.row_keys.empty()&&base.row_counts.empty()&&
   base.start_offset==source_base.start_offset&&base.end_offset==source_base.end_offset,
   "Citizens current terrain vector native source shape differs");
  terrain.emplace();
  for(std::size_t yi=0;yi<6;++yi){
   current_terrain[yi]=native(base.values[yi]);const auto found=owned.yields.find(static_cast<std::int64_t>(yi));
   need((found==owned.yields.end()?0:found->second.terrain)==base.values[yi],"Citizens raw/owned terrain beforeimage differs");
  }
 }
 const auto math=detail::bnw_citizens_work_prefix(slot==0,assigning,working,unassigned,cached,current_terrain);
 if(slot!=0){
  citizen_named(staged_citizens.fields,"m_iNumCitizensWorkingPlots").value=math.working;
  citizen_named(staged_citizens.fields,"m_iNumUnassignedCitizens").value=math.unassigned;
 }
 if(math.first_terrain){
  const auto [yi,value]=*math.first_terrain;
  citizen_named(target->core.fields,"m_aiBaseYieldRateFromTerrain").values[yi]=value;
  staged_yields[yi].terrain=value;current_terrain[yi]=value;
  terrain->raw=current_terrain;terrain->owned=staged_yields;
 }
 std::optional<saves::CivisSaveSection> seal(make_bnw_city_management_checkpoint(staged_world,staged_world,p.identity,true));
 std::optional<saves::CivisSaveSection> specialist_seal;
 if(p.specialist_core_beforeimage)specialist_seal=make_bnw_city_core_checkpoint(staged_world,staged_world,p.identity,true);
 // Core codec intentionally rejects changed management companions. Validate
 // detached current schema against itself; this is not an exported section
 // or source-baseline persistence receipt. Original field spans checked above.
 (void)make_bnw_city_core_checkpoint(staged_world,staged_world,p.identity,true);
 need(p.progress.citizens_attempts<UINT64_MAX&&p.progress.citizens_native_writes<=UINT64_MAX-4,
  "Citizens native prefix once history exhausted");
 static_assert(std::is_nothrow_swappable_v<decltype(seal)>);
 static_assert(std::is_nothrow_swappable_v<decltype(terrain)>);
 // Bind every mutable target before the attempt marker. No allocation follows.
 auto* working_field=slot==0?nullptr:&citizen_named(citizens.fields,"m_iNumCitizensWorkingPlots").value;
 auto* unassigned_field=slot==0?nullptr:&citizen_named(citizens.fields,"m_iNumUnassignedCitizens").value;
 auto* terrain_field=math.first_terrain?&citizen_named(city->core.fields,"m_aiBaseYieldRateFromTerrain"):nullptr;
 CitizensUnworkTail tail;tail.plot=plot;tail.cached=cached;tail.assigning=assigning;
 tail.reallocation_return=!assigning&&p.citizens_do_turn&&p.citizens_do_turn->step==CitizensDoTurnStep::remove_citizen;
 if(math.first_terrain){tail.next_yield=static_cast<std::uint8_t>(math.first_terrain->first+1);tail.awaiting_yield=math.first_terrain->first;}
 require_current();need(guard(),"Citizens rules retired before actual source stores");
 need(p.state->ui_source_revision<UINT64_MAX,"Citizens UI source revision exhausted");
 p.ledger->citizens_attempts.store(p.progress.citizens_attempts+1);++p.progress.citizens_attempts;
 p.ledger->citizens_slot.store(static_cast<std::uint8_t>(slot));p.progress.citizens_working_slot=static_cast<std::uint8_t>(slot);
 flags.values[static_cast<std::size_t>(slot)]=assigning?1:0;
 std::uint8_t writes=1;
 if(working_field){*working_field=math.working;*unassigned_field=math.unassigned;writes=3;}
 if(math.first_terrain){terrain_field->values[math.first_terrain->first]=math.first_terrain->second;owned.yields.swap(staged_yields);++writes;}
 p.strategy_beforeimage.swap(seal);p.citizens_terrain.swap(terrain);p.citizens_tail=tail;
 if(p.specialist_core_beforeimage)p.specialist_core_beforeimage.swap(specialist_seal);
 ++p.state->ui_source_revision;
 p.progress.citizens_native_writes+=writes;
 p.ledger->citizens_writes.store(p.progress.citizens_native_writes);
 p.progress.citizens_frontier=math.first_terrain?BnwCityCitizensVerifyFrontier::inner_city_screen_predicate:BnwCityCitizensVerifyFrontier::outer_city_selected;
 p.ledger->citizens_frontier.store(p.progress.citizens_frontier);
 // Native UI read-dependent predicate precedes later yields/dirty events and
 // DoAddBestCitizenFromUnassigned. There is no public receipt/skip API.
}

void BnwCityEntryPending::advance_citizens_ui_one(std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get(),
  "Citizens UI continuation requires SAME actual Host/source allocation");
 need(p.citizens_tail.has_value(),"Citizens actual unwork continuation absent");
 auto& tail=*p.citizens_tail;
 const auto set_frontier=[&](BnwCityCitizensVerifyFrontier f)noexcept{
  p.progress.citizens_frontier=f;p.ledger->citizens_frontier.store(f);
 };
 const auto attempt=[&]{
  need(p.progress.citizens_attempts<UINT64_MAX,"Citizens source cursor history exhausted");
  ++p.progress.citizens_attempts;p.ledger->citizens_attempts.store(p.progress.citizens_attempts);
 };
 const auto selection=[&]{
  auto control=host->session().city_selection_.lock();
  need(bool(control),"Citizens actual initialized client selection owner missing");
  control->require_current();return control;
 };
 const auto append=[&](const std::vector<BnwCityDirtyKind>& kinds){
  need(kinds.size()<=UINT64_MAX-p.state->city_dirty_next_occurrence,
   "Citizens ordered dirty occurrence exhausted");
  auto events=p.state->city_dirty_events;
  need(kinds.size()<=events.max_size()-events.size(),"Citizens dirty event storage exhausted");
  events.reserve(events.size()+kinds.size());
  auto ordinal=p.state->city_dirty_next_occurrence;
  const auto generation=host->session().state_generation();
  for(const auto kind:kinds)events.push_back({ordinal++,generation,p.progress.owner,p.progress.city,
   static_cast<std::int64_t>(tail.plot),tail.awaiting_yield?static_cast<std::int8_t>(*tail.awaiting_yield):std::int8_t{-1},kind,true});
  require_current();
  if(!kinds.empty())need(p.state->ui_source_revision<UINT64_MAX,"Citizens notification UI source revision exhausted");
  attempt();
  // All throwing work precedes the actual ordered notification publication.
  p.state->city_dirty_events.swap(events);p.state->city_dirty_next_occurrence=ordinal;
  if(!kinds.empty())++p.state->ui_source_revision;
 };
 if(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::inner_city_screen_predicate){
  need(tail.awaiting_yield&&*tail.awaiting_yield<6,"Citizens reached terrain UI operand absent");
  // Native getTeam first, then Game getActiveTeam. Selection is NOT read on
  // differing teams (including the genuine NO_PLAYER/NO_TEAM Game branch).
  const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
  need(city,"Citizens actual current team city unavailable");
  const auto city_team=detail::bnw_player_team_read(*p.state,p.progress.owner);
  const auto active_team=detail::bnw_game_active_team_read(*p.state);
  std::vector<BnwCityDirtyKind> events;
  if(city_team==active_team){
   const auto control=selection();
   if(control->is_selected(p.progress.owner,p.progress.city))events.push_back(BnwCityDirtyKind::city_screen);
   control->require_current();
  }
  append(events);tail.awaiting_yield.reset();set_frontier(BnwCityCitizensVerifyFrontier::remaining_terrain);return;
 }
 if(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::remaining_terrain){
  need(!tail.awaiting_yield&&tail.next_yield<=6,"Citizens remaining native terrain cursor differs");
  if(tail.next_yield==6){require_current();attempt();set_frontier(BnwCityCitizensVerifyFrontier::outer_city_selected);return;}
  const auto yi=tail.next_yield;
  // Cached getYield is a signed short; unary negation promotes to native int.
  const auto& current_plot=p.state->world.map_plots->plots.at(static_cast<std::size_t>(tail.plot));
  const auto& cache=citizen_named(current_plot.arrays,"yields");
  need(cache.columns==1&&cache.values.size()==6&&cache.values[yi]==tail.cached[yi],
   "Citizens reached cached yield changed outside retained unwork prefix");
  const auto delta=tail.assigning?static_cast<std::int32_t>(tail.cached[yi]):-static_cast<std::int32_t>(tail.cached[yi]);
  if(delta==0){require_current();attempt();++tail.next_yield;return;}
  const auto* resolved=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
  need(resolved&&p.citizens_terrain,"Citizens actual terrain storage/seal absent");
  saves::SavedCityObservation* city=nullptr;
  for(auto& player:p.state->world.all_players->players)
   for(auto& candidate:player.cities.cities)if(&candidate==resolved)city=&candidate;
  need(city,"Citizens mutable canonical city allocation absent");
  auto& raw=citizen_named(city->core.fields,"m_aiBaseYieldRateFromTerrain");
  auto& owned=p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city);
  need(raw.values.size()==6,"Citizens remaining terrain vector shape differs");
  const auto value=native(static_cast<std::int64_t>(native(raw.values[yi]))+delta);
  auto yields=owned.yields;yields[yi].terrain=value;
  auto seal=p.citizens_terrain;seal->raw[yi]=value;seal->owned=yields;
  std::optional<saves::CivisSaveSection> specialist_seal;
  if(p.specialist_core_beforeimage){
   auto world=p.state->world;const auto* staged=resolve_bnw_city_registry(world,p.state->cities,p.progress.owner,p.progress.city);
   saves::SavedCityObservation* target=nullptr;
   for(auto& player:world.all_players->players)for(auto& candidate:player.cities.cities)if(&candidate==staged)target=&candidate;
   need(target,"Citizens remaining terrain canonical staging city absent");
   citizen_named(target->core.fields,"m_aiBaseYieldRateFromTerrain").values[yi]=value;
   specialist_seal=make_bnw_city_core_checkpoint(world,world,p.identity,true);
  }
  need(p.progress.citizens_native_writes<UINT64_MAX,"Citizens native write history exhausted");
  require_current();need(p.state->ui_source_revision<UINT64_MAX,"Citizens terrain UI source revision exhausted");attempt();
  raw.values[yi]=value;owned.yields.swap(yields);p.citizens_terrain.swap(seal);
  if(p.specialist_core_beforeimage)p.specialist_core_beforeimage.swap(specialist_seal);
  ++p.state->ui_source_revision;
  ++p.progress.citizens_native_writes;p.ledger->citizens_writes.store(p.progress.citizens_native_writes);
  tail.awaiting_yield=yi;++tail.next_yield;set_frontier(BnwCityCitizensVerifyFrontier::inner_city_screen_predicate);return;
 }
 need(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::outer_city_selected&&
  tail.next_yield==6&&!tail.awaiting_yield,"Citizens outer selection predecessor differs");
 if(tail.assigning)need(p.progress.citizens_next_plot<37,"Citizens assignment original verify cursor exhausted");
 const auto control=selection();
 const bool selected=control->is_selected(p.progress.owner,p.progress.city);
 if(tail.reallocation_return){
  need(p.citizens_do_turn&&p.citizens_do_turn->step==CitizensDoTurnStep::remove_citizen&&
   p.citizens_do_turn->removals_done>=0&&p.citizens_do_turn->removals_done<p.citizens_do_turn->working_removals,
   "Citizens removal tail lost actual native return site");
 }
 const auto events=detail::bnw_citizens_outer_dirty_kinds(selected);
 control->require_current();append(events);
 if(tail.assigning){
  // Actual DoAddBest returns true only after its SetWorkingPlot completed.
  ++p.progress.citizens_next_plot;p.ledger->citizens_next.store(p.progress.citizens_next_plot);
  p.citizens_tail.reset();set_frontier(BnwCityCitizensVerifyFrontier::none);
 }else if(tail.reallocation_return){
  ++p.citizens_do_turn->removals_done;p.citizens_tail.reset();
  set_frontier(BnwCityCitizensVerifyFrontier::reallocation_remove_working);
 }else set_frontier(BnwCityCitizensVerifyFrontier::reassignment);
}

std::int32_t BnwCityEntryPending::citizens_plot_value(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::uint64_t plot,bool allow_growth){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get()&&
  host->identity().rules_sha256==p.identity.rules_sha256,"Citizens score requires SAME actual Host/source");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard(),"Citizens score actual rules retired");const auto& db=host->database();
 const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 need(city&&p.state->world.map_plots&&p.state->world.map_projection,"Citizens score actual city/map absent");
 const auto& citizens=citizen_named(city->management.blocks,"citizens");
 const auto cbool=[&](std::string_view name){const auto n=citizen_named(citizens.fields,name).value;
  need(n==0||n==1,"Citizens score native bool unavailable");return n!=0;};
 need(plot<p.state->world.map_plots->plots.size()&&plot<p.state->world.map_projection->cells.size(),"Citizens score map index outside actual allocation");
 const auto& raw=p.state->world.map_plots->plots.at(static_cast<std::size_t>(plot));
 const auto& cell=p.state->world.map_projection->cells.at(static_cast<std::size_t>(plot));
 need(raw.index==plot&&cell.index==plot&&raw.x==cell.x&&raw.y==cell.y,
  "Citizens score raw/projected plot identity differs");
 const auto define=[&](std::string_view name){
  require_current();need(guard(),"Citizens rules retired before Define read");
  const auto* cached=host->defines().find(name);
  const auto current=db.global_define(name,content::GlobalDefineKind::integer);
  need(cached&&host->defines().profile()==content::GlobalDefineProfile::bnw&&cached->kind==content::GlobalDefineKind::integer&&
   cached->lookup.error.empty()&&cached->lookup.found&&current.error.empty()&&current.found&&
   std::holds_alternative<std::int64_t>(cached->lookup.value)&&std::holds_alternative<std::int64_t>(current.value)&&
   cached->lookup.value==current.value,"Citizens reached native Define cache/source unavailable");
  const auto out=native(std::get<std::int64_t>(cached->lookup.value));
  require_current();need(guard(),"Citizens rules retired after Define read");return out;
 };
  const auto& cache=citizen_named(raw.arrays,"yields");
  need(cache.columns==1&&cache.values.size()==6&&cell.cached_yields&&cell.cached_yields->size()==6,
   "Citizens best-plot native six short cache absent");
  constexpr const char* names[]={"AI_CITIZEN_VALUE_FOOD","AI_CITIZEN_VALUE_PRODUCTION","AI_CITIZEN_VALUE_GOLD",
   "AI_CITIZEN_VALUE_SCIENCE","AI_CITIZEN_VALUE_CULTURE","AI_CITIZEN_VALUE_FAITH"};
  std::array<std::int32_t,6> weighted{};
  for(std::size_t yi=0;yi<6;++yi){
   const auto d=define(names[yi]);const auto v=cache.values[yi];const auto mirror=cell.cached_yields->find(static_cast<std::int64_t>(yi));
   need(v>=INT16_MIN&&v<=INT16_MAX&&mirror!=cell.cached_yields->end()&&mirror->second==v,
    "Citizens reached raw/owned signed short yield differs");
   weighted[yi]=detail::bnw_citizen_plot_value_term(d,static_cast<std::int16_t>(v));
  }
  // Exactly AFTER six native products. Closed SAME rate -> consumption owner;
  // there is no supplied food/cache/freshness DTO or second database.
  auto session=std::shared_ptr<const LiveSession>(host,&host->session());
  auto database=std::shared_ptr<const content::RulesDatabase>(host,&db);
  auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(database,p.identity.rules_sha256));
  const auto rate=inspect_bnw_city_current_food_rate(session,rules,p.progress.owner,p.progress.city);
  const auto food=rate.value_times100();
  const auto consumption=BnwCityCurrentFoodConsumption::from_same_rate(rate,p.progress.owner,p.progress.city,0);
  const auto excess=native(std::int64_t(food)-native(std::int64_t(consumption.value())*100));
  bool avoid=false;
  // Current Rate replays raw Game NO_HAPPINESS and native excess-happiness
  // calculation and reconciles both mirrors. Reuse those SAME reached facts.
  if(!p.state->cities.no_happiness){
   const auto happiness=p.state->cities.players.at(p.progress.owner).excess_happiness;
   need(happiness.has_value(),"Citizens actual current excess-happiness unavailable");
   avoid=native(*happiness)<0;if(!avoid)avoid=cbool("m_bForceAvoidGrowth");
  }
  const auto focus=native(citizen_named(citizens.fields,"m_eCityAIFocusTypes").value);
  // Focus products/food branches precede the native small-city population read.
  weighted=detail::bnw_citizen_plot_value_focus_food(weighted,excess,avoid,focus,allow_growth);
  std::int32_t population=0;
  if((focus==-1||focus==6||focus==7)&&!avoid)population=scalar(*city,"m_iPopulation");
  auto value=detail::bnw_citizen_plot_value_total(weighted,avoid,focus,population);
  consumption.require_same_inputs(*p.state,p.identity.rules_sha256);(void)rate.value_times100();require_current();
  need(guard(),"Citizens score rules retired after operands");return value;
}

void BnwCityEntryPending::advance_citizens_reassignment_one(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get()&&
  host->identity().rules_sha256==p.identity.rules_sha256,"Citizens reassignment SAME actual Host absent");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard(),"Citizens reassignment actual immutable rules retired");
 need(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::reassignment&&p.citizens_tail&&!p.citizens_tail->assigning,
  "Citizens reassignment has no completed actual unwork predecessor");
 const auto& db=host->database();
 const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 need(city,"Citizens reassignment canonical current city absent");
 const auto& citizens=citizen_named(city->management.blocks,"citizens");
 const auto cbool=[&](std::string_view name){const auto v=citizen_named(citizens.fields,name).value;
  need(v==0||v==1,"Citizens reassignment native bool unavailable");return v!=0;};
 const auto frontier=[&](BnwCityCitizensVerifyFrontier f){
  require_current();need(guard(),"Citizens rules retired before reached frontier");
  need(p.progress.citizens_attempts<UINT64_MAX,"Citizens reassignment history exhausted");
  ++p.progress.citizens_attempts;p.ledger->citizens_attempts.store(p.progress.citizens_attempts);
  p.progress.citizens_frontier=f;p.ledger->citizens_frontier.store(f);
 };
 // SDK's zero test is FIRST; no NoAuto/Building/plot/rate operand is read here.
 const auto unassigned=native(citizen_named(citizens.fields,"m_iNumUnassignedCitizens").value);
 if(unassigned==0){
  need(p.progress.citizens_next_plot<37,"Citizens original verify cursor exhausted");
  frontier(BnwCityCitizensVerifyFrontier::none);
  ++p.progress.citizens_next_plot;p.ledger->citizens_next.store(p.progress.citizens_next_plot);
  p.citizens_tail.reset();return;
 }
 const bool no_auto=cbool("m_bNoAutoAssignSpecialists");
 const auto define=[&](std::string_view name){
  require_current();need(guard(),"Citizens rules retired before Define read");
  const auto* cached=host->defines().find(name);
  const auto current=db.global_define(name,content::GlobalDefineKind::integer);
  need(cached&&host->defines().profile()==content::GlobalDefineProfile::bnw&&cached->kind==content::GlobalDefineKind::integer&&
   cached->lookup.error.empty()&&cached->lookup.found&&current.error.empty()&&current.found&&
   std::holds_alternative<std::int64_t>(cached->lookup.value)&&std::holds_alternative<std::int64_t>(current.value)&&
   cached->lookup.value==current.value,"Citizens reached native Define cache/source unavailable");
  const auto out=native(std::get<std::int64_t>(cached->lookup.value));
  require_current();need(guard(),"Citizens rules retired after Define read");return out;
 };
 CitizenSpecialistSources spec(host,p.progress.owner,p.progress.city,[&]{require_current();need(guard(),"Citizens specialist actual rules retired");});
 const auto specialist_value=[&](std::int32_t id){
  const auto* info=spec.info(id);if(!info)return std::int32_t{0}; // source NULL early return
  const auto bonus=spec.half_food()?1:0;
  constexpr const char* names[]={"AI_CITIZEN_VALUE_FOOD","AI_CITIZEN_VALUE_PRODUCTION","AI_CITIZEN_VALUE_GOLD",
   "AI_CITIZEN_VALUE_SCIENCE","AI_CITIZEN_VALUE_CULTURE","AI_CITIZEN_VALUE_FAITH"};
  std::array<std::int32_t,6> weighted{};
  for(std::size_t yi=0;yi<6;++yi){const auto weight=define(names[yi]);
   auto value=yi==4?spec.culture(id):spec.player_yield(id,yi);
   if(yi==0)value=native(std::int64_t(value)+bonus);
   weighted[yi]=detail::bnw_specialist_weighted_yield(weight,value);
  }
  auto gp=native(std::int64_t(info->gp)*3);
  auto happiness=spec.half_unhappiness()?5:0;
  // Source IsEmpireUnhappy executes even with a zero happiness yield. Reuse
  // the existing native formula with a genuine metadata-only dense count child.
  const auto yield_rules=load_city_yield_rules(db,content::TechnologyProfile::bnw);
  auto setup=restore_research_context(p.state->world,db);
  const auto raw_yields=restore_city_yields(p.state->world,setup,db,yield_rules,host->types());
  need(raw_yields.no_happiness==p.state->cities.no_happiness,"Citizens specialist Game NO_HAPPINESS mirror stale");
  if(!raw_yields.no_happiness){
   const auto uh=load_bnw_unhappiness_rules(db);spec.check();
   detail::require_bnw_current_unhappiness_inputs(*p.state,uh,host->types(),p.progress.owner);
   auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
   const auto quote=detail::bnw_native_unhappiness_math_with_specialists(p.state->unhappiness,p.state->setup,p.state->cities,
    uh,host->defines(),p.progress.owner,counts);
   need(p.state->cities.players.at(p.progress.owner).excess_happiness&&
    *p.state->cities.players.at(p.progress.owner).excess_happiness==quote.excess_happiness,
    "Citizens specialist current excess-happiness mirror stale");
   if(quote.excess_happiness<0)happiness=native(std::int64_t(happiness)*2);
   (void)counts.reached_source_seal();
  }
  auto session=std::shared_ptr<const LiveSession>(host,&host->session());
  auto database=std::shared_ptr<const content::RulesDatabase>(host,&db);
  auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(database,p.identity.rules_sha256));
  const auto rate=inspect_bnw_city_current_food_rate(session,rules,p.progress.owner,p.progress.city);
  const auto food=rate.value_times100();const auto consumption=BnwCityCurrentFoodConsumption::from_same_rate(rate,p.progress.owner,p.progress.city,0);
  const auto excess=native(std::int64_t(food)-native(std::int64_t(consumption.value())*100));
  bool avoid=false;if(!p.state->cities.no_happiness){
   const auto h=p.state->cities.players.at(p.progress.owner).excess_happiness;need(h.has_value(),"Citizens specialist current happiness absent");
   avoid=native(*h)<0;if(!avoid)avoid=cbool("m_bForceAvoidGrowth");
  }
  const auto focus=native(citizen_named(citizens.fields,"m_eCityAIFocusTypes").value);
  if(focus==3)gp=native(std::int64_t(gp)*3);
  weighted=detail::bnw_citizen_plot_value_focus_food(weighted,excess,avoid,focus);
  std::int32_t population=0;if((focus==-1||focus==6||focus==7)&&!avoid)population=scalar(*city,"m_iPopulation");
  const auto value=detail::bnw_specialist_total(weighted,avoid,focus,population,gp,happiness);
  consumption.require_same_inputs(*p.state,p.identity.rules_sha256);(void)rate.value_times100();spec.check();return value;
 };
 struct BestSpecialist {std::int32_t building=-1,specialist=-1,value=-1;};
 const auto best_specialist_building=[&](){
  BestSpecialist best;std::int32_t best_adjusted=-1;
  // GetAIBestSpecialistBuilding: actual BuildingInfo vector, including NULL
  // slots, and true Buildings Init/Reset -> ordered Read count reconstruction.
  // It does not infer an empty city from sparse imported counts.
  CitizenBuildingsSource building_sources(host,*p.state,p.source->snapshot,p.progress.owner,p.progress.city,
   [&]{require_current();need(guard(),"Citizens building source rules retired");});
  const auto& infos=building_sources.infos;const auto& real=building_sources.real;const auto& free=building_sources.free;
  const auto& count_rules=*building_sources.count_rules;const auto slots=building_sources.slots;
  auto specialist_counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
  for(std::uint64_t i=0;i<slots;++i){
   const auto id=static_cast<std::int32_t>(i);const auto info=infos.find(id);
   if(info==infos.end())continue; // actual NULL BuildingInfo; no GetNumBuilding
   const auto n=count_rules.building_count(real.at(static_cast<std::size_t>(i)),free.at(static_cast<std::size_t>(i)));
   if(n<=0)continue;
   // Native IsCanAdd reads assigned count BEFORE population, then Info cap,
   // then the global cap. Dense per-building Citizens Read has a private owner.
   const auto assigned=specialist_counts.building_specialists_for_city(p.progress.owner,p.progress.city,id);
   if(assigned>=scalar(*city,"m_iPopulation"))continue;
   std::optional<std::int32_t> allowed;std::optional<std::string> specialist_type;
   const auto result=db.query_rows({"SELECT ID,Type,SpecialistCount,SpecialistType FROM Buildings WHERE ID=?",{std::int64_t(id)}},[&](const auto& row){
    need(!allowed&&row.has_column("ID")&&row.has_column("Type")&&row.has_column("SpecialistCount")&&
     row.integer("ID")==id&&row.text("Type")==std::optional{info->second.type},"Citizens reached BuildingInfo specialist cap differs");
    allowed=native(row.integer("SpecialistCount"));specialist_type=row.text("SpecialistType");
   },2);
   require_current();need(guard()&&result.all_succeeded()&&result.rows==1&&allowed,"Citizens reached specialist cap unavailable");
   if(assigned>=*allowed)continue;
   if(assigned>=define("MAX_SPECIALISTS_FROM_BUILDING"))continue;
   (void)specialist_counts.reached_source_seal();
   std::int32_t specialist=-1;
   if(specialist_type&&!specialist_type->empty()){
    const auto* t=host->types().find(*specialist_type);
    if(t){need(t->table=="Specialists","Citizens reached Building SpecialistType is a foreign native remap");specialist=native(t->id);}
   }
   const auto value=specialist_value(specialist);
   const auto adjusted=detail::bnw_specialist_building_value(*allowed,value);
   if(adjusted.adjusted>best_adjusted){best_adjusted=adjusted.adjusted;best={id,specialist,adjusted.unmodified};}
  }
  (void)specialist_counts.reached_source_seal();return best;
 };
 BestSpecialist chosen;if(!no_auto)chosen=best_specialist_building();
 const bool better_slacker=chosen.building!=-1&&spec.better(chosen.specialist);
 // Actual best specialist (or native NoAuto/NO_BUILDING) is retained while
 // calculating the real plot competitor; no eager focus getter on NO_BUILDING.
 // GetBestCityPlotWithValue(...,true,false) preserves all37/native wrap slots.
 const auto& map=*p.state->world.map_projection;
 const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};geometry.validate();
 const auto x=scalar(*city,"m_iX"),y=scalar(*city,"m_iY");
 const auto positions=detail::bnw_citizen_plot_positions(geometry,x,y);
 const auto& working=citizen_named(citizens.arrays,"m_pabWorkingPlot");
 need(working.columns==1&&working.values.size()==37,"Citizens best-plot actual working flags absent");
 const auto naval=inspect_bnw_city_current_naval_strategy(host,p.progress.owner,p.progress.city);
 std::optional<std::uint64_t> best;std::int32_t best_value=-1;
 for(std::size_t i=1;i<positions.size();++i){
  if(!positions[i])continue;
  require_current();need(guard(),"Citizens rules retired before actual plot read");
  const auto plot=static_cast<std::uint64_t>(*positions[i]);
  const auto& raw=p.state->world.map_plots->plots.at(static_cast<std::size_t>(plot));
  const auto& cell=map.cells.at(static_cast<std::size_t>(plot));
  need(raw.index==plot&&cell.index==plot&&raw.x==cell.x&&raw.y==cell.y&&cell.x<=INT32_MAX&&cell.y<=INT32_MAX,
   "Citizens best-plot raw/owned plot identity differs");
  const auto owner=native(citizen_named(raw.fields,"working_city_owner").value);
  const auto handle=native(citizen_named(raw.fields,"working_city_id").value);
  need(owner>=-1&&owner<64&&cell.yield_city&&cell.yield_city->owner==owner&&cell.yield_city->id==handle,"Citizens best-plot current working-city mirror differs");
  const auto* current=owner==-1?nullptr:resolve_bnw_city_registry(p.state->world,p.state->cities,owner,handle);
  if(!current||scalar(*current,"m_iID")!=p.progress.city)continue; // SDK compares ID here, not owner pointer
  const auto slot=detail::bnw_citizen_plot_slot(map,x,y,static_cast<std::int32_t>(cell.x),static_cast<std::int32_t>(cell.y));
  need(slot>=0&&slot<37,"Citizens best-plot inverse slot unavailable");
  const auto w=working.values.at(static_cast<std::size_t>(slot));need(w==0||w==1,"Citizens reached working bool unavailable");
  if(w!=0)continue;
  if(!naval.can_work(plot))continue;
  require_current();need(guard(),"Citizens rules retired after actual IsCanWork");
  auto value=citizens_plot_value(host,plot,true);
  const auto& forced=citizen_named(citizens.arrays,"m_pabForcedWorkingPlot");
  need(forced.columns==1&&forced.values.size()==37,"Citizens reached forced flags absent");
  const auto f=forced.values.at(static_cast<std::size_t>(slot));need(f==0||f==1,"Citizens reached forced flag native bool unavailable");
  if(f!=0)value=native(std::int64_t(value)+10000);
  if(best_value==-1||value>best_value){best_value=value;best=plot;}
 }
 require_current();need(guard(),"Citizens rules retired after native best plot scan");
 const bool specialist_better=chosen.building!=-1&&chosen.value>=best_value;
 if(specialist_better&&better_slacker){begin_citizens_specialist(std::move(host),chosen.building,chosen.specialist);return;}
 if(!specialist_better&&best){set_citizens_working_plot(std::move(host),*best,true);return;}
 // Native fallback: owner IsHuman FIRST, then NoAuto only for a human.
 // The same genuine Game predicate as growth is reused, including real lazy
 // PitBoss source-string/default lookup for OPEN slots (no preview actor).
 const auto human=detail::bnw_player_human_read(*p.state,p.progress.owner,db);
 require_current();need(guard(),"Citizens rules retired after actual owner human predicate");
 if(!human||!cbool("m_bNoAutoAssignSpecialists")){
  const auto fallback=best_specialist_building();
  if(fallback.building!=-1&&spec.better(fallback.specialist)){
   begin_citizens_specialist(std::move(host),fallback.building,fallback.specialist);return;
  }
 }
 // Genuine native default path: its counter is written before the DEFAULT
 // getter/count and processSpecialist. The retained cursor never repeats it.
 begin_citizens_specialist(std::move(host),-1,-1);
}

void BnwCityEntryPending::begin_citizens_specialist(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t building,std::int32_t specialist){
 require_current();auto& p=*impl_;
 need(p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::reassignment&&!p.citizens_specialist,
  "Citizens specialist assignment lacks genuine reassignment predecessor");
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get()&&
  host->identity().rules_sha256==p.identity.rules_sha256,"Citizens specialist SAME Host/source absent");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();need(guard&&guard(),"Citizens specialist actual rules retired");
 const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 need(city,"Citizens specialist canonical current city absent");
 const auto& citizens=citizen_named(city->management.blocks,"citizens");
 if(building!=-1){
  // Source DoAddSpecialistToBuilding rechecks IsCanAdd. No effects occurred
  // between this scan's owned choice and this private invocation.
  auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
  const auto assigned=counts.building_specialists_for_city(p.progress.owner,p.progress.city,building);
  need(assigned<scalar(*city,"m_iPopulation"),"Citizens chosen building no longer passes native population cap");
  std::optional<std::int32_t> allowed;std::optional<std::string> type;
  const auto rows=host->database().query_rows({"SELECT ID,Type,SpecialistCount,SpecialistType FROM Buildings WHERE ID=?",
   {std::int64_t(building)}},[&](const auto& row){
    need(!allowed&&row.integer("ID")==building,"Citizens chosen BuildingInfo occurrence differs");
    allowed=native(row.integer("SpecialistCount"));type=row.text("SpecialistType");
   },2);
  need(rows.all_succeeded()&&rows.rows==1&&allowed&&assigned<*allowed,"Citizens chosen actual building cap changed");
  CitizenSpecialistSources spec(host,p.progress.owner,p.progress.city,[&]{require_current();need(guard(),"Citizens specialist rules retired");});
  need(assigned<spec.define("MAX_SPECIALISTS_FROM_BUILDING"),"Citizens chosen global specialist cap changed");
  std::int32_t current=-1;if(type&&!type->empty()){const auto* t=host->types().find(*type);
   if(t){need(t->table=="Specialists","Citizens chosen building specialist remap foreign");current=native(t->id);}}
  need(current==specialist,"Citizens chosen actual Building SpecialistType changed");
  (void)counts.reached_source_seal();
 }
 need(native(citizen_named(citizens.fields,"m_iNumUnassignedCitizens").value)!=0,
  "Citizens specialist reached mandatory DoRemoveWorstCitizen/DoRemoveWorstSpecialist without its owner");
 require_current();need(guard(),"Citizens specialist rules retired before native continuation");
 CitizensSpecialistTail issued;issued.specialist=specialist;issued.building=building;
 p.citizens_specialist=issued;
 p.progress.citizens_frontier=BnwCityCitizensVerifyFrontier::specialist_process;
 p.ledger->citizens_frontier.store(p.progress.citizens_frontier);
 advance_citizens_specialist_one(std::move(host));
}

void BnwCityEntryPending::begin_citizens_specialist_removal(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int32_t building,
 std::int32_t specialist,bool forced,bool forced_default){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&&&host->session().source()==p.source.get()&&
  p.citizens_do_turn&&!p.citizens_specialist&&!p.citizens_tail,"Signed specialist needs genuine retained reallocation parent");
 const auto step=p.citizens_do_turn->step;
 need(step==CitizensDoTurnStep::remove_citizen||step==CitizensDoTurnStep::building_scan||step==CitizensDoTurnStep::remove_defaults,
  "Signed specialist private return site unavailable");
 const auto guard=host->database().immutable_preview_fingerprint_currentness();need(guard&&guard(),"Signed specialist actual rules retired");
 if(building!=-1){
  need(!forced_default,"Signed building removal has no forced-default prefix");
  std::optional<std::int32_t> current;
  const auto rows=host->database().query_rows({"SELECT ID,Type,SpecialistType FROM Buildings WHERE ID=?",{std::int64_t(building)}},[&](const auto& row){
   need(!current&&row.integer("ID")==building,"Signed specialist BuildingInfo row unavailable");
   const auto type=row.text("Type");const auto* by_name=type?host->types().find(*type):nullptr;
   const auto* by_hash=type?host->types().find_hash(content::saved_info_type_hash(*type)):nullptr;
   need(type&&by_name&&by_hash&&by_name->table=="Buildings"&&by_hash->table==by_name->table&&by_name->id==building&&
    by_hash->id==building&&by_name->type==*type&&by_hash->type==*type&&by_name->hash==by_hash->hash,"Signed specialist actual BuildingInfo identity unavailable");
   current=-1;const auto st=row.text("SpecialistType");if(st&&!st->empty())if(const auto* t=host->types().find(*st)){
    need(t->table=="Specialists","Signed specialist BuildingInfo remap foreign");current=native(t->id);}
  },2);require_current();need(guard()&&rows.all_succeeded()&&rows.rows==1&&current&&*current==specialist,"Signed chosen BuildingInfo/specialist changed");
  auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
  need(counts.building_specialists_for_city(p.progress.owner,p.progress.city,building)>0,"Signed specialist assigned count no longer positive");
  (void)counts.reached_source_seal();
 }else need(!forced&&specialist==-1,"Signed default branch cannot supply a specialist remap");
 CitizensSpecialistTail issued;issued.specialist=specialist;issued.building=building;issued.change=-1;issued.forced=forced;
 issued.phase=forced_default?9:0;
 issued.return_site=step==CitizensDoTurnStep::remove_citizen?CitizensSpecialistReturn::remove_worker:
  step==CitizensDoTurnStep::building_scan?CitizensSpecialistReturn::remove_building:CitizensSpecialistReturn::remove_default;
 require_current();need(guard(),"Signed specialist rules retired before actual child");p.citizens_specialist=issued;
 p.progress.citizens_frontier=BnwCityCitizensVerifyFrontier::specialist_process;p.ledger->citizens_frontier.store(p.progress.citizens_frontier);
 advance_citizens_specialist_one(std::move(host));
}

void BnwCityEntryPending::advance_citizens_specialist_one(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&host->session().read_state_lease()==p.state&& &host->session().source()==p.source.get()&&
  host->identity().rules_sha256==p.identity.rules_sha256,"Citizens specialist continuation SAME Host/source absent");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();need(guard&&guard(),"Citizens specialist current rules retired");
 need(p.citizens_specialist&&p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_process,
  "Citizens specialist genuine partial cursor absent");
 auto& tail=*p.citizens_specialist;
 CitizenSpecialistSources spec(host,p.progress.owner,p.progress.city,[&]{require_current();need(guard(),"Citizens specialist rules retired before borrow");});
 const auto* resolved=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
 saves::SavedCityObservation* city=nullptr;
 for(auto& player:p.state->world.all_players->players)for(auto& candidate:player.cities.cities)
  if(&candidate==resolved)city=&candidate;
 need(city,"Citizens specialist mutable canonical city allocation absent");
 auto& owned=p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city);
 const auto attempt=[&]{need(p.progress.citizens_attempts<UINT64_MAX,"Citizens specialist history exhausted");
  ++p.progress.citizens_attempts;p.ledger->citizens_attempts.store(p.progress.citizens_attempts);};
 const auto advance=[&](std::uint8_t phase){spec.check();attempt();tail.phase=phase;tail.yield=0;};
 // All allocation/checkpoint/native checks occur before each actual store.
 // Replacing only core+management leaves the canonical city's owning address,
 // roster, source spans and source identity intact. Future getters replay this
 // SAME current projection and compare the paired owned yield/count mirrors.
 const auto publish=[&](auto&& mutate,std::uint64_t writes){
  auto world=p.state->world;
  const auto* detached=resolve_bnw_city_registry(world,p.state->cities,p.progress.owner,p.progress.city);
  saves::SavedCityObservation* target=nullptr;
  for(auto& player:world.all_players->players)for(auto& candidate:player.cities.cities)if(&candidate==detached)target=&candidate;
  need(target,"Citizens specialist detached staging city absent");
  auto yields=owned.yields;
  const auto& current_base=citizen_named(city->core.fields,"m_aiBaseYieldRateFromSpecialists");
  need(current_base.encoding==saves::CityCoreEncoding::vector_i32&&current_base.wire_count==6&&current_base.values.size()==6,
   "Citizens specialist current canonical yield shape unavailable");
  for(std::size_t yi=0;yi<6;++yi){const auto old=yields.find(static_cast<std::int64_t>(yi));
   need((old==yields.end()?0:old->second.specialists)==current_base.values[yi],"Citizens first specialist canonical yield mirror stale");}
  auto counts=p.state->unhappiness.players.at(p.progress.owner).cities.at(p.progress.city).specialists;
  auto* excess=&p.state->cities.players.at(p.progress.owner).excess_happiness;
  auto* count_target=&p.state->unhappiness.players.at(p.progress.owner).cities.at(p.progress.city).specialists;
  mutate(*target,yields,counts);
  std::optional<saves::CivisSaveSection> management(make_bnw_city_management_checkpoint(world,world,p.identity,true));
  std::optional<saves::CivisSaveSection> core_seal(make_bnw_city_core_checkpoint(world,world,p.identity,true));
  need(p.progress.citizens_native_writes<=UINT64_MAX-writes&&p.state->ui_source_revision<UINT64_MAX,
   "Citizens specialist native write/revision history exhausted");
  static_assert(std::is_nothrow_swappable_v<decltype(city->core)>);
  static_assert(std::is_nothrow_swappable_v<decltype(city->management)>);
  static_assert(std::is_nothrow_swappable_v<decltype(management)>);
  spec.check();attempt();
  // Native store order is represented by this operation's tiny staged body;
  // there is no callback between these stores. The no-throw swap publishes the
  // paired source+mirrors before the NEXT native getter/notification boundary.
  std::swap(city->core,target->core);std::swap(city->management,target->management);
  owned.yields.swap(yields);count_target->swap(counts);
  p.strategy_beforeimage.swap(management);p.specialist_core_beforeimage.swap(core_seal);
  // Building assignment must next run UpdateReligion/DoUpdateHappiness. An
  // earlier quoted excess value is not a current truth after its count store.
  // Default assignment is excluded from native total specialist unhappiness.
  if(tail.building!=-1&&tail.phase==1)excess->reset();
  ++p.state->ui_source_revision;p.progress.citizens_native_writes+=writes;
  p.ledger->citizens_writes.store(p.progress.citizens_native_writes);
 };
 const auto vector=[&](std::string_view name)->const saves::CityCoreField&{
  const auto& v=citizen_named(city->core.fields,name);
  need(v.encoding==saves::CityCoreEncoding::vector_i32&&v.wire_count==6&&v.values.size()==6&&
   v.hashes.empty()&&v.bytes.empty()&&v.row_counts.empty()&&v.row_keys.empty(),"Citizens specialist native vector shape unavailable");
  return v;
 };
 const auto count_store=[&](saves::SavedCityObservation& target,std::string_view name,std::int32_t id,
  std::uint32_t hash,std::int32_t next){
  auto& c=citizen_named(target.management.blocks,"citizens");auto& rows=citizen_named(c.hashes,name);
  rows=detail::bnw_specialist_known_hash_projection_math(rows,hash,next);
  (void)id; // already proved actual native collection slot, not a caller remap
 };
 if(tail.awaiting_ui){
  std::vector<BnwCityDirtyEvent> events=p.state->city_dirty_events;
  auto next=p.state->city_dirty_next_occurrence;
  if(detail::bnw_player_team_read(*p.state,p.progress.owner)==detail::bnw_game_active_team_read(*p.state)){
   auto control=host->session().city_selection_.lock();need(bool(control),"Citizens specialist actual selection owner absent");
   control->require_current();if(control->is_selected(p.progress.owner,p.progress.city)){
    need(next<UINT64_MAX&&events.size()<events.max_size(),"Citizens specialist dirty queue exhausted");
    events.push_back({next++,host->session().state_generation(),p.progress.owner,p.progress.city,-1,
     static_cast<std::int8_t>(tail.yield),BnwCityDirtyKind::city_screen,true});
   }control->require_current();
  }
  spec.check();need(next==p.state->city_dirty_next_occurrence||p.state->ui_source_revision<UINT64_MAX,
   "Citizens specialist notification revision exhausted");attempt();
  p.state->city_dirty_events.swap(events);
  if(next!=p.state->city_dirty_next_occurrence)++p.state->ui_source_revision;
  p.state->city_dirty_next_occurrence=next;tail.awaiting_ui=false;++tail.yield;return;
 }
 need(tail.change==1||tail.change==-1,"Citizens specialist private sign unavailable");
 if(tail.phase==9){
  need(tail.change==-1&&tail.building==-1,"Citizens forced-default branch metadata differs");
  const auto old=native(citizen_named(citizen_named(city->management.blocks,"citizens").fields,"m_iNumForcedDefaultSpecialists").value);
  const auto next=native(std::int64_t(old)-1);
  publish([&](auto& target,auto&,auto&){citizen_named(citizen_named(target.management.blocks,"citizens").fields,"m_iNumForcedDefaultSpecialists").value=next;},1);
  tail.phase=0;return;
 }
 if(tail.phase==10){
  need(tail.change==-1&&tail.building>=0&&tail.forced,"Citizens forced-building branch metadata differs");
  auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
  const auto old=counts.forced_building_specialists_for_city(p.progress.owner,p.progress.city,tail.building);
  const auto next=native(std::int64_t(old)-1);(void)counts.reached_source_seal();
  std::optional<std::uint32_t> hash;const auto rows=host->database().query_rows({"SELECT ID,Type FROM Buildings WHERE ID=?",{std::int64_t(tail.building)}},[&](const auto& row){
   const auto type=row.text("Type");need(!hash&&row.integer("ID")==tail.building&&type,"Citizens forced-building actual Info row unavailable");
   const auto* t=host->types().find(*type);const auto* h=host->types().find_hash(content::saved_info_type_hash(*type));
   need(t&&h&&t->table=="Buildings"&&h->table==t->table&&t->id==tail.building&&h->id==t->id&&t->type==*type&&h->type==t->type&&t->hash==h->hash,"Citizens forced-building Type/hash identity differs");hash=t->hash;
  },2);need(rows.all_succeeded()&&rows.rows==1&&hash,"Citizens forced-building source query unavailable");
  publish([&](auto& target,auto&,auto&){count_store(target,"m_aiNumForcedSpecialistsInBuilding",tail.building,*hash,next);},1);
  tail.phase=3;return;
 }
 if(tail.phase==0){
  if(tail.building==-1){
   const auto old=native(citizen_named(citizen_named(city->management.blocks,"citizens").fields,"m_iNumDefaultSpecialists").value);
   const auto next=native(std::int64_t(old)+tail.change);
   publish([&](auto& target,auto&,auto&){citizen_named(citizen_named(target.management.blocks,"citizens").fields,"m_iNumDefaultSpecialists").value=next;},1);
   tail.phase=1;return;
  }
  tail.phase=1; // building path has no ChangeNumDefaultSpecialists prefix
 }
 if(tail.phase==1){
  // Default getter is reached AFTER its counter write, exactly as SDK.
  if(tail.building==-1)tail.specialist=spec.define("DEFAULT_SPECIALIST");
  const auto* info=spec.info(tail.specialist);
  need(info,"Citizens reached count for NULL specialist Info needs genuine serializer identity");
  auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
  const auto current=counts.count_for_city(p.progress.owner,p.progress.city,tail.specialist);
  const auto next=native(std::int64_t(current)+tail.change);(void)counts.reached_source_seal();
  publish([&](auto& target,auto&,auto& unhappiness){count_store(target,"m_aiSpecialistCounts",tail.specialist,info->hash,next);unhappiness[tail.specialist]=next;},1);
  tail.phase=2;return;
 }
 if(tail.phase==2){
  if(tail.building!=-1){
   auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
   const auto before=counts.building_specialists_for_city(p.progress.owner,p.progress.city,tail.building);
   const auto next=native(std::int64_t(before)+tail.change);(void)counts.reached_source_seal();
   std::optional<std::uint32_t> building_hash;
   const auto report=host->database().query_rows({"SELECT Type FROM Buildings WHERE ID=?",{std::int64_t(tail.building)}},
    [&](const auto& row){const auto type=row.text("Type");need(type.has_value(),"Citizens specialist actual Building Type absent");
     const auto* t=host->types().find(*type);need(t&&t->table=="Buildings"&&t->id==tail.building&&
      t->hash==content::saved_info_type_hash(*type),"Citizens specialist BuildingInfo hash identity differs");
     need(!building_hash,"Citizens specialist duplicate BuildingInfo hash source");building_hash=t->hash;
    },2);
   need(report.all_succeeded()&&report.rows==1&&building_hash,"Citizens specialist building hash source failed");
   publish([&](auto& target,auto&,auto&){count_store(target,"m_aiNumSpecialistsInBuilding",tail.building,*building_hash,next);},1);
  }else advance(3);
  tail.phase=tail.forced?10:3;return;
 }
 // processSpecialist NULL branch is represented explicitly (count identity
 // currently requires nonnull Info before arriving here).
 const auto* info=spec.info(tail.specialist);if(!info){advance(8);return;}
 if(tail.phase==3){
  const auto delta=detail::bnw_specialist_weighted_yield(tail.change,info->gp);
  const auto value=native(std::int64_t(scalar(*city,"m_iBaseGreatPeopleRate"))+delta);
  publish([&](auto& target,auto&,auto&){citizen_named(target.core.fields,"m_iBaseGreatPeopleRate").values.front()=value;},1);
  tail.phase=4;tail.yield=0;return;
 }
 if(tail.phase==4){
  if(tail.yield==6){advance(5);return;}
  const auto yi=tail.yield;const auto delta=detail::bnw_specialist_weighted_yield(tail.change,spec.base_yields(tail.specialist)[yi]);
  if(delta==0){spec.check();attempt();++tail.yield;return;}
  const auto& raw=vector("m_aiBaseYieldRateFromSpecialists");const auto value=native(std::int64_t(native(raw.values[yi]))+delta);
  const auto old=owned.yields.find(yi);need((old==owned.yields.end()?0:old->second.specialists)==raw.values[yi],"Citizens specialist yield mirror stale");
  publish([&](auto& target,auto& yields,auto&){citizen_named(target.core.fields,"m_aiBaseYieldRateFromSpecialists").values[yi]=value;yields[yi].specialists=value;},1);
  tail.awaiting_ui=true;return;
 }
 if(tail.phase==5){
  if(tail.yield==6){advance(6);return;}
  const auto yi=tail.yield;const auto& extra=vector("m_aiExtraSpecialistYield");
  const auto old=native(extra.values[yi]); // source old getter BEFORE native loop
  auto counts=BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city);
  const auto next=spec.extra(yi,counts,p.progress.city);(void)counts.reached_source_seal();
  if(old==next){spec.check();attempt();++tail.yield;return;}
  const auto delta=native(std::int64_t(next)-old);
  const auto& base=vector("m_aiBaseYieldRateFromSpecialists");const auto value=native(std::int64_t(native(base.values[yi]))+delta);
  publish([&](auto& target,auto& yields,auto&){
   citizen_named(target.core.fields,"m_aiExtraSpecialistYield").values[yi]=next;
   citizen_named(target.core.fields,"m_aiBaseYieldRateFromSpecialists").values[yi]=value;yields[yi].specialists=value;
  },2);tail.awaiting_ui=true;return;
 }
 if(tail.phase==6){
  const auto delta=detail::bnw_specialist_weighted_yield(tail.change,info->experience);
  const auto value=native(std::int64_t(scalar(*city,"m_iSpecialistFreeExperience"))+delta);
  publish([&](auto& target,auto&,auto&){citizen_named(target.core.fields,"m_iSpecialistFreeExperience").values.front()=value;},1);
  tail.phase=7;return;
 }
 if(tail.phase==7){
  const auto delta=detail::bnw_specialist_weighted_yield(tail.change,spec.culture(tail.specialist));
  if(delta!=0){const auto value=native(std::int64_t(scalar(*city,"m_iJONSCulturePerTurnFromSpecialists"))+delta);
   publish([&](auto& target,auto&,auto&){citizen_named(target.core.fields,"m_iJONSCulturePerTurnFromSpecialists").values.front()=value;},1);
  }else{spec.check();attempt();}
  tail.phase=8;return;
 }
 need(tail.phase==8,"Citizens specialist source phase unavailable");
 if(tail.building!=-1){
  // Exact DoAdd/DoRemoveSpecialistToBuilding boundary: signed process completed,
  // next UpdateReligion starts with city.updateYield. Neither unassigned nor
  // outer dirty calls are executed before that missing gameplay owner.
  spec.check();attempt();p.progress.citizens_frontier=BnwCityCitizensVerifyFrontier::specialist_update_religion;
  p.ledger->citizens_frontier.store(p.progress.citizens_frontier);return;
 }
 const auto unassigned=native(citizen_named(citizen_named(city->management.blocks,"citizens").fields,"m_iNumUnassignedCitizens").value);
 const auto next=native(std::int64_t(unassigned)-tail.change);
 if(tail.change==1)need(p.progress.citizens_next_plot<37,"Citizens completed default original cursor exhausted");
 else need(p.citizens_do_turn&&((tail.return_site==CitizensSpecialistReturn::remove_worker&&p.citizens_do_turn->step==CitizensDoTurnStep::remove_citizen&&p.citizens_do_turn->removals_done<p.citizens_do_turn->working_removals)||
  (tail.return_site==CitizensSpecialistReturn::remove_default&&p.citizens_do_turn->step==CitizensDoTurnStep::remove_defaults&&p.citizens_do_turn->defaults_done<p.citizens_do_turn->default_removals)),"Signed default lost genuine native return site");
 publish([&](auto& target,auto&,auto&){citizen_named(citizen_named(target.management.blocks,"citizens").fields,"m_iNumUnassignedCitizens").value=next;},1);
 if(tail.change==1){++p.progress.citizens_next_plot;p.ledger->citizens_next.store(p.progress.citizens_next_plot);}
 else if(tail.return_site==CitizensSpecialistReturn::remove_worker)++p.citizens_do_turn->removals_done;
 else ++p.citizens_do_turn->defaults_done;
 const auto destination=tail.change==1?BnwCityCitizensVerifyFrontier::none:BnwCityCitizensVerifyFrontier::reallocation_remove_working;
 p.citizens_tail.reset();p.citizens_specialist.reset();p.progress.citizens_frontier=destination;
 p.ledger->citizens_frontier.store(p.progress.citizens_frontier);
}

void BnwCityEntryPending::accept_city_plot_child_prefix(const BnwCityYieldRefreshPending& parent,
 const BnwPlotYieldRefreshPending& child){
 auto& p=*impl_;
 need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&!p.ledger->quarantined.load()&&
  p.game&&!p.game->retired.load()&&p.game->thread==p.thread,
  "City religion prefix Entry lifecycle retired before borrow");
 need(p.strategy_rules_current&&p.strategy_rules_current()&&p.strategy_rules_sha==p.identity.rules_sha256,
  "City religion prefix immutable rules retired before borrow");
 need(p.religion_city_child.get()==&parent&&p.specialist_core_beforeimage,
  "City religion prefix is not the retained actual Entry child");
 const auto receipt=child.terrain_prefix_source(parent,*this);
 need(receipt.state.get()==p.state.get()&&receipt.source==p.source&&
  receipt.identity.payload_sha256==p.identity.payload_sha256&&receipt.identity.rules_sha256==p.identity.rules_sha256,
  "City religion prefix child allocation/source identity differs");
 const auto same_operation=p.accepted_city_prefix&&p.accepted_city_prefix->operation==receipt.operation;
 if(same_operation){
  const auto& previous=*p.accepted_city_prefix;
  need(previous.city==receipt.city&&previous.generation==receipt.generation&&
   previous.terrain_write_count<=receipt.terrain_write_count&&previous.ledger_revision<=receipt.ledger_revision&&
   previous.next_yield<=receipt.next_yield,
   "City religion prefix native child history regressed/rebound");
 }
 // The private child accessor already proves raw/owned current values, exact
 // written native slots, and unchanged nonterrain components/known zero nodes.
 // Restore ONLY that actual working city's previous terrain prefix in a
 // detached comparison world. This may be a different owner/city from Entry.
 auto normalized=p.state->world;
 if(receipt.city){
  const auto [owner,city]=*receipt.city;
  const auto* resolved=resolve_bnw_city_registry(normalized,p.state->cities,owner,city);
  need(resolved,"City religion prefix actual working city absent");
  saves::SavedCityObservation* target=nullptr;
  for(auto& player:normalized.all_players->players)
   for(auto& candidate:player.cities.cities)if(&candidate==resolved)target=&candidate;
  need(target,"City religion prefix working city allocation not canonical");
  auto& terrain=citizen_named(target->core.fields,"m_aiBaseYieldRateFromTerrain");
  need(terrain.values.size()==6,"City religion prefix working terrain shape differs");
  const auto& previous=same_operation?p.accepted_city_prefix->current_terrain:receipt.initial_terrain;
  for(std::size_t yi=0;yi<6;++yi)terrain.values[yi]=previous[yi];
 }
 const auto before=make_bnw_city_core_checkpoint(normalized,normalized,p.identity,true);
 need(before.id==p.specialist_core_beforeimage->id&&before.flags==p.specialist_core_beforeimage->flags&&
  before.bytes==p.specialist_core_beforeimage->bytes,
  "City religion child changed a core field outside exact terrain prefix");
 std::optional<saves::CivisSaveSection> after(make_bnw_city_core_checkpoint(p.state->world,p.state->world,p.identity,true));
 auto own_terrain=p.citizens_terrain;
 if(receipt.city==std::optional{std::pair{p.progress.owner,p.progress.city}}&&own_terrain){
  for(std::size_t yi=0;yi<6;++yi)own_terrain->raw[yi]=native(receipt.current_terrain[yi]);
  own_terrain->owned=receipt.current_owned;
 }
 std::optional<BnwPlotYieldRefreshPending::TerrainPrefixSource> accepted(receipt);
 // Allocate every replacement before acknowledging the actual owned prefix.
 // These are source seals, not a second publication of raw/gameplay values.
 (void)child.terrain_prefix_source(parent,*this);
 need(p.strategy_rules_current(),"City religion prefix rules retired before seal adoption");
 p.specialist_core_beforeimage.swap(after);p.citizens_terrain.swap(own_terrain);
 p.accepted_city_prefix.swap(accepted);
 require_current();
}

void BnwCityEntryPending::advance_citizens_update_religion(LiveSession& session,
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&&host->session()==&session&&session.state_.get()==p.state.get()&&session.source_==p.source&&
  session.debug_==p.game&&session.state_generation_==host->session().state_generation(),
  "City religion continuation needs actual owning Session/Host");
 need(p.citizens_specialist&&p.citizens_specialist->building!=-1&&p.citizens_specialist->phase==8&&
  p.progress.citizens_frontier==BnwCityCitizensVerifyFrontier::specialist_update_religion,
  "City religion continuation lacks genuine building specialist predecessor");
 const auto found=session.city_entry_pending_.find(source_key());
 need(found!=session.city_entry_pending_.end()&&found->second.get()==this,
  "City religion continuation Entry is not exact registered owner");
 const auto attempt=[&]{need(p.progress.citizens_attempts<UINT64_MAX,"City religion native history exhausted");
  ++p.progress.citizens_attempts;p.ledger->citizens_attempts.store(p.progress.citizens_attempts);};
 try{
  if(!p.religion_city_child){
   // The source argument is evaluated BEFORE CvCity::updateYield's first call.
   p.religion_majority=BnwCityReligionYieldOwner::capture_majority_for_entry(*this,host,p.state,p.source);
   attempt();
   p.religion_city_child=session.begin_bnw_city_yield_refresh(host,p.progress.owner,p.progress.city,found->second);
   return;
  }
  if(p.religion_city_child->progress().phase!=BnwCityYieldRefreshPhase::source_loop_returned){
   attempt();p.religion_city_child->advance(session,host);
   // Source child begin/continue synchronously adopted exact prefix seals
   // BEFORE returning here. Public progress never grants Religion effects.
   require_current();return;
  }
  p.religion_city_child->require_completed_for_entry(*this);
  if(!p.religion_owner){
   need(p.religion_majority.has_value(),"City religion pre-child majority argument absent");
   p.religion_owner=BnwCityReligionYieldOwner::after_city_yield_child(*this,host,p.state,p.source,*p.religion_majority);
  }
  auto& owner=*p.religion_owner;owner.require_current(*this,host);
  if(p.religion_ui_yield){
   const auto yi=*p.religion_ui_yield;
   const auto city_team=detail::bnw_player_team_read(*p.state,p.progress.owner);
   const auto active_team=detail::bnw_game_active_team_read(*p.state);
   auto events=p.state->city_dirty_events;auto next=p.state->city_dirty_next_occurrence;bool emitted=false;
   if(city_team==active_team){
    const auto control=session.city_selection_.lock();need(bool(control),"City religion actual selection owner absent");
    control->require_current();
    if(control->is_selected(p.progress.owner,p.progress.city)){
     need(next<UINT64_MAX&&events.size()<events.max_size(),"City religion notification queue exhausted");
     events.push_back({next++,session.state_generation_,p.progress.owner,p.progress.city,-1,
      static_cast<std::int8_t>(yi),BnwCityDirtyKind::city_screen,true});emitted=true;
    }control->require_current();
   }
   p.religion_city_child->require_completed_for_entry(*this);owner.require_current(*this,host);
   if(emitted)need(p.state->ui_source_revision<UINT64_MAX,"City religion notification revision exhausted");
   attempt();p.state->city_dirty_events.swap(events);p.state->city_dirty_next_occurrence=next;
   if(emitted)++p.state->ui_source_revision;
   p.religion_ui_yield.reset();return;
  }
  std::optional<BnwCitySpecialistCounts> counts;
  if(owner.needs_specialist_total())counts.emplace(BnwCitySpecialistCounts::from_same_host(host,p.progress.owner,p.progress.city));
  if(owner.player_happiness_tail_reached()){advance_citizens_player_happiness(session,host);return;}
  const auto step=owner.prepare_next(*this,host,counts?&*counts:nullptr);
  if(!step.writes){
   p.religion_city_child->require_completed_for_entry(*this);owner.validate_before_publication(step,*this,host);
   attempt();owner.commit_published(step);return;
  }
  auto world=p.state->world;const auto* resolved=resolve_bnw_city_registry(world,p.state->cities,p.progress.owner,p.progress.city);
  need(resolved,"City religion staging current city absent");saves::SavedCityObservation* staged=nullptr;
  for(auto& player:world.all_players->players)for(auto& city:player.cities.cities)if(&city==resolved)staged=&city;
  need(staged,"City religion staging allocation absent");
  const auto index=step.vector_yield<0?std::size_t{0}:static_cast<std::size_t>(step.vector_yield);
  citizen_named(staged->core.fields,step.field).values.at(index)=step.after_value;
  auto yields=p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).yields;
  if(step.vector_yield>=0)yields[step.vector_yield].religion=step.after_value;
  auto terrain_seal=p.citizens_terrain;
  if(terrain_seal)terrain_seal->owned=yields;
  std::optional<saves::CivisSaveSection> seal(make_bnw_city_core_checkpoint(world,world,p.identity,true));
  const auto* current=resolve_bnw_city_registry(p.state->world,p.state->cities,p.progress.owner,p.progress.city);
  saves::SavedCityObservation* raw=nullptr;
  for(auto& player:p.state->world.all_players->players)for(auto& city:player.cities.cities)if(&city==current)raw=&city;
  need(raw&&p.specialist_core_beforeimage,"City religion canonical mutable allocation/core seal absent");
  auto* target=&citizen_named(raw->core.fields,step.field).values.at(index);
  auto* mirror=&p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).yields;
  p.religion_city_child->require_completed_for_entry(*this);owner.validate_before_publication(step,*this,host);
  need(p.progress.citizens_native_writes<UINT64_MAX&&p.state->ui_source_revision<UINT64_MAX,
   "City religion native write/revision exhausted");attempt();
  // No throwing operation in the source field + mirror + seal publication.
  *target=step.after_value;mirror->swap(yields);p.specialist_core_beforeimage.swap(seal);
  p.citizens_terrain.swap(terrain_seal);
  ++p.progress.citizens_native_writes;p.ledger->citizens_writes.store(p.progress.citizens_native_writes);
  ++p.state->ui_source_revision;owner.commit_published(step);
  if(step.base_city_screen)p.religion_ui_yield=static_cast<std::uint8_t>(step.vector_yield);
 }catch(...){
  // Partial native stores/events are retained facts. Their actual once-history
  // is quarantined; copying an old CP cannot restore a live call stack.
  invalidate();throw;
 }
}

void BnwCityEntryPending::advance_citizens_player_happiness(LiveSession& session,
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 require_current();auto& p=*impl_;
 need(host&&&host->session()==&session&&session.state_.get()==p.state.get()&&session.source_==p.source&&session.debug_==p.game,
  "Player happiness continuation needs actual owning Session/Host");
 const auto registered=session.city_entry_pending_.find(source_key());
 need(registered!=session.city_entry_pending_.end()&&registered->second.get()==this&&p.religion_owner&&p.religion_city_child&&!p.religion_ui_yield,
  "Player happiness continuation lacks exact actual Entry/religion predecessor");
 p.religion_city_child->require_completed_for_entry(*this);p.religion_owner->require_player_happiness_tail(*this,host);
 try{
  if(!p.player_happiness_owner)p.player_happiness_owner=BnwPlayerHappinessPrefixOwner::from_religion_tail(
   *this,*p.religion_owner,host,p.state,p.source);
  auto& owner=*p.player_happiness_owner;auto step=owner.prepare_next(*this,host);
  auto& raw=p.state->world.all_players->players.at(static_cast<std::size_t>(p.progress.owner));
  auto* target=&citizen_named(raw.core.fields,"m_iHappiness").values.front();
  auto* mirror=&p.state->unhappiness.players.at(p.progress.owner).happiness;
  auto* excess=&p.state->cities.players.at(p.progress.owner).excess_happiness;
  owner.validate_before_publication(step,*this,host);p.religion_city_child->require_completed_for_entry(*this);
  need(p.progress.citizens_attempts<UINT64_MAX&&p.progress.citizens_native_writes<UINT64_MAX&&p.state->ui_source_revision<UINT64_MAX,
   "Player happiness native store/history/revision exhausted");
  // The actual once marker precedes the no-throw raw/echo/excess/cursor stores.
  ++p.progress.citizens_attempts;p.ledger->citizens_attempts.store(p.progress.citizens_attempts);
  *target=step.after;*mirror=step.after;excess->reset();
  ++p.progress.citizens_native_writes;p.ledger->citizens_writes.store(p.progress.citizens_native_writes);
  ++p.state->ui_source_revision;owner.commit_published(std::move(step),p.state->ui_source_revision);
 }catch(...){invalidate();throw;}
}

void BnwCityEntryPending::invalidate()noexcept {
 impl_->retired.store(true);impl_->ledger->quarantined.store(true);
 if(impl_->thread==std::this_thread::get_id()){
  if(impl_->religion_city_child)impl_->religion_city_child->invalidate();
  if(impl_->religion_owner)impl_->religion_owner->retire();
  if(impl_->player_happiness_owner)impl_->player_happiness_owner->retire();
 }
}
std::shared_ptr<const BnwCityEntryCheckpoint> BnwCityEntryPending::checkpoint()const {
 need(impl_->thread==std::this_thread::get_id(),"City entry checkpoint belongs to another thread");
 if(!impl_->retired.load()&&!impl_->ledger->quarantined.load())require_current();
 return checkpoint_history();
}
std::shared_ptr<const BnwCityEntryCheckpoint> BnwCityEntryPending::checkpoint_history()const {
 need(impl_->thread==std::this_thread::get_id(),"City entry history belongs to another thread");
 auto cp=std::make_shared<BnwCityEntryCheckpoint::Impl>();
 cp->key=impl_->expected.key;cp->progress=progress();cp->ledger=impl_->ledger;
 return std::shared_ptr<const BnwCityEntryCheckpoint>(new BnwCityEntryCheckpoint(std::move(cp)));
}
std::shared_ptr<BnwCityEntryPending> BnwCityEntryPending::restore_quarantined(
 const BnwCityEntryCheckpoint& checkpoint){
 need(checkpoint.impl_&&checkpoint.impl_->ledger,"City entry checkpoint history missing");
 auto p=std::make_unique<Impl>();p->expected.key=checkpoint.impl_->key;
 p->progress=checkpoint.impl_->progress;p->progress.phase=BnwCityEntryPhase::quarantined;
 // Rollback may restore older raw data, but never earlier native loop/history.
 // Only owned atomic metadata is read; old DB/Game/VM/world is NOT queried.
 p->progress.strategy_started=p->progress.strategy_started||checkpoint.impl_->ledger->strategy_started.load();
 p->progress.strategy_next_id=std::max(p->progress.strategy_next_id,checkpoint.impl_->ledger->strategy_next.load());
 p->progress.strategy_slots=std::max(p->progress.strategy_slots,checkpoint.impl_->ledger->strategy_slots.load());
 p->progress.strategy_steps=std::max(p->progress.strategy_steps,checkpoint.impl_->ledger->strategy_steps.load());
 p->progress.strategy_trigger_calls=std::max(p->progress.strategy_trigger_calls,checkpoint.impl_->ledger->strategy_calls.load());
 p->progress.strategy_revision=std::max(p->progress.strategy_revision,checkpoint.impl_->ledger->strategy_revision.load());
 p->progress.strategy_effect_attempts=std::max(p->progress.strategy_effect_attempts,checkpoint.impl_->ledger->strategy_effect_attempts.load());
 p->progress.strategy_publications=std::max(p->progress.strategy_publications,checkpoint.impl_->ledger->strategy_publications.load());
 if(checkpoint.impl_->ledger->strategy_frontier.load()!=BnwCityStrategyLoopFrontier::none)
  p->progress.strategy_frontier=checkpoint.impl_->ledger->strategy_frontier.load();
 p->ledger=checkpoint.impl_->ledger;p->retired.store(true);
 // Metadata-only preparation. No old rules/debug/world query and no shared
 // history mutation until Root has prepared every replacement allocation.
 return std::shared_ptr<BnwCityEntryPending>(new BnwCityEntryPending(std::move(p)));
}
void BnwCityEntryPending::commit_quarantine()noexcept {
 impl_->ledger->quarantined.store(true);impl_->retired.store(true);
}
}
