#include "civ5/runtime/bnw_minor_city_strength.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <functional>
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include <algorithm>
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <array>
#include <cmath>
#include <string_view>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* s){if(!b)throw BnwMinorCityStrengthUnavailable(s);}
std::int32_t integer(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"City strength native int operand/operation overflow");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return integer(std::int64_t(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return integer(std::int64_t(a)*b);}
auto& named(auto& rows,std::string_view name){
 using T=typename std::remove_reference_t<decltype(rows)>::value_type;
 using P=std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(rows)>>,const T,T>;
 P* found=nullptr;for(auto& row:rows)if(row.name&&std::string_view(row.name)==name){need(!found,"City strength duplicate source field");found=&row;}
 need(found,"City strength required source field absent");return *found;
}
std::int32_t city_scalar(const saves::SavedCityObservation& city,std::string_view name){
 const auto& f=named(city.core.fields,name);
 need(f.encoding==saves::CityCoreEncoding::i32&&!f.wire_count&&f.values.size()==1&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"City strength city scalar encoding unavailable");return integer(f.values[0]);
}
std::int32_t player_scalar(const saves::SavedPlayerObservation& player,std::string_view name,saves::PlayerCoreEncoding encoding=saves::PlayerCoreEncoding::i32){
 const auto& f=named(player.core.fields,name);
 need(f.encoding==encoding&&!f.wire_count&&f.values.size()==1&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"City strength player scalar encoding unavailable");return integer(f.values[0]);
}
double define(const ui::SelectedBnwPreviewHost& host,const char* name,content::GlobalDefineKind kind){
 const auto* d=host.defines().find(name);
 need(host.defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->kind==kind&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty(),"City strength actual cached Define unavailable");
 const auto current=host.database().global_define(name,kind);
 need(current.found&&current.sqlite_code==0&&current.error.empty()&&current.value==d->lookup.value,"City strength current/cached Define diverged");
 if(kind==content::GlobalDefineKind::integer){need(std::holds_alternative<std::int64_t>(current.value),"City strength integer Define type differs");return integer(std::get<std::int64_t>(current.value));}
 need(kind==content::GlobalDefineKind::source_float&&std::holds_alternative<double>(current.value),"City strength source-float Define type differs");
 const auto n=std::get<double>(current.value);need(std::isfinite(n),"City strength source-float Define nonfinite");return n;
}
std::int32_t unit_scalar(const saves::SavedUnitObservation& u,std::string_view name,saves::UnitCoreEncoding encoding=saves::UnitCoreEncoding::i32){
 const auto& f=named(u.core.fields,name);
 need(u.core.layout==bnw&&f.encoding==encoding&&!f.wire_count&&f.values.size()==1&&f.bytes.empty(),"City getBestDefender actual unit scalar unavailable");
 return integer(f.values[0]);
}
const saves::SavedUnitObservation* plot_unit(const LiveSessionState& state,std::int32_t owner,std::int32_t handle){
 if(owner<0||owner>=64||handle==-1)return nullptr; // Genuine GetPlayerUnit/GetAt null branches.
 need(owner>=0&&owner<64&&state.maintenance.profile==content::TechnologyProfile::bnw&&state.maintenance.extensions.values.empty(),"City getBestDefender actual unit owner/profile absent");
 const auto& player=state.world.all_players->players.at(static_cast<std::size_t>(owner));const auto& units=player.units;
 const auto owned=state.maintenance.players.find(owner);need(owned!=state.maintenance.players.end()&&owned->second.profile_extensions.values.empty(),"City getBestDefender paired owned player unavailable");
 need(player.index==static_cast<std::uint64_t>(owner)&&player.saved_player_id==owner&&units.layout==bnw&&units.header.layout==bnw&&units.header.wire_unit_count==static_cast<std::int64_t>(units.units.size())&&units.units.size()==owned->second.units.size(),"City getBestDefender raw/owned unit roster differs");
 const auto last=integer(named(units.header.fields,"last_index").value),free=integer(named(units.header.fields,"free_list_count").value);
 need(last>=-1&&free>=0&&std::int64_t(last)-free+1==static_cast<std::int64_t>(units.units.size())&&named(units.header.fields,"num_slots").value==static_cast<std::int64_t>(units.header.source_slots),"City getBestDefender native FLTA roster unavailable");
 const saves::SavedUnitObservation* found=nullptr;
 for(std::size_t i=0;i<units.units.size();++i){const auto& u=units.units[i];const auto id=unit_scalar(u,"m_iID"),slot=id&8191;
  const auto live=owned->second.units.find(id);
  need(id>=0&&slot<=last&&static_cast<std::uint64_t>(slot)<units.header.source_slots&&unit_scalar(u,"m_eOwner")==owner&&live!=owned->second.units.end()&&live->second.source_core==std::optional{std::pair{u.core.start_offset,u.core.end_offset}}&&live->second.profile_extensions.values.empty(),"City getBestDefender genuine unit storage binding differs");
  for(std::size_t j=0;j<i;++j)need((unit_scalar(units.units[j],"m_iID")&8191)!=slot,"City getBestDefender duplicate native FLTA slot");
  if(handle!=-1&&slot==(handle&8191)&&((handle&~8191)==0||id==handle))found=&u;
 }
 return found; // Original GetAt accepts index-only handles; stale full IDs miss.
}
std::int32_t unit_domain(const ui::SelectedBnwPreviewHost& host,const saves::SavedUnitObservation& unit){
 const auto& hash_field=named(unit.core.fields,"idUnitType");
 need(hash_field.encoding==saves::UnitCoreEncoding::hash32&&!hash_field.wire_count&&hash_field.values.size()==1&&hash_field.bytes.empty()&&hash_field.values[0]>0&&hash_field.values[0]<=UINT32_MAX,"City getBestDefender saved Unit Info hash unavailable");
 const auto* info=host.types().find_hash(static_cast<std::uint64_t>(hash_field.values[0]));
 need(host.types().report().all_succeeded()&&!host.types().report().hash_collisions&&!host.types().report().name_overwrites&&info&&info->table=="Units"&&info->id==unit_scalar(unit,"m_eUnitType")&&info->hash==content::saved_info_type_hash(info->type),"City getBestDefender actual Unit Info ID/hash differs");
 std::int32_t domain=-1;bool found=false;
 const auto result=host.database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{info->id}},[&](const content::RulesRowView& row){
  need(!found&&row.text("Type")==info->type,"City getBestDefender active Unit SQL differs");found=true;
  const auto name=row.text("Domain");const auto* d=name?host.types().find(*name):nullptr;
  constexpr std::array<std::string_view,5> domains{"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
  need(d&&d->table=="Domains"&&d->id>=0&&d->id<5&&d->type==domains[static_cast<std::size_t>(d->id)]&&d->hash==content::saved_info_type_hash(d->type),"City getBestDefender native Domain ID unavailable");domain=integer(d->id);
 },2);
 need(result.all_succeeded()&&found,"City getBestDefender actual active Unit SQL failed");return domain;
}
std::int32_t selected_defender_strength(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t city_owner,const saves::PlotObservation& plot){
 const auto& stack=named(plot.arrays,"units");const saves::SavedUnitObservation* best=nullptr;
 // CvCity passes getOwner(); only the remaining CvPlot arguments default:
 // NO_PLAYER/null/false/false/false/false. No war, visibility or cargo test.
 for(std::size_t i=0;i<stack.values.size();i+=2){
  const auto* u=plot_unit(state,integer(stack.values[i]),integer(stack.values[i+1]));if(!u)continue;
  need(unit_scalar(*u,"m_iX")==static_cast<std::int64_t>(plot.x)&&unit_scalar(*u,"m_iY")==static_cast<std::int64_t>(plot.y),"City getBestDefender actual unit/plot coordinates differ");
  const auto id=unit_scalar(*u,"m_iID"),owner=unit_scalar(*u,"m_eOwner");const auto& live=state.maintenance.players.at(owner).units.at(id);
  const auto base=unit_scalar(*u,"m_iBaseCombat");const auto embarked=unit_scalar(*u,"m_bEmbarked",saves::UnitCoreEncoding::boolean);
  need(base>=0&&(embarked==0||embarked==1)&&live.military==(base>0)&&live.civilian==(base<=0),"City getBestDefender actual base/owned unit class differs");
  need(!embarked,"City getBestDefender reached GetEmbarkedUnitDefense supplier unavailable");
  if(base==0)continue; // IsCanDefend checks strength before delayed death/domain.
  const auto dead=unit_scalar(*u,"m_bDeathDelay",saves::UnitCoreEncoding::boolean);need(dead==0||dead==1,"City getBestDefender delayed-death boolean invalid");if(dead)continue;
  const auto domain=unit_domain(host,*u);const auto dynamic_domain=domain==4?(named(plot.fields,"plot_type").value==3?0:2):domain;
  need(live.type==unit_scalar(*u,"m_eUnitType")&&live.domain==dynamic_domain,"City getBestDefender raw/owned active Unit domain differs");
  bool valid=false;
  if(dynamic_domain==1)valid=false; // DOMAIN_AIR never defends this action.
  else if(dynamic_domain==0)valid=named(plot.fields,"plot_type").value==3||unit_scalar(*u,"m_iCanMoveAllTerrainCount")>0;
  else if(dynamic_domain==2||dynamic_domain==3){
   need(named(plot.fields,"plot_type").value!=3,"City getBestDefender reached water-land hovering/walk-water supplier unavailable");valid=true;
  }else need(false,"City getBestDefender unsupported native Domain");
  if(!valid||owner!=city_owner)continue;
  // isBetterDefenderThan(nullptr,nullptr) returns true immediately. A second
  // eligible candidate reaches GetMaxDefenseStrength/interception/cycle tie.
  need(!best,"City getBestDefender competing eligible units require actual defense comparison supplier");best=u;
 }
 if(!best)return 0;
 const auto max_hits=integer(static_cast<std::int64_t>(define(host,"MAX_HIT_POINTS",content::GlobalDefineKind::integer)));
 const auto damage=unit_scalar(*best,"m_iDamage");need(max_hits>0&&damage>=0&&damage<=max_hits,"City getBestDefender actual health/division unsupported");
 const auto remaining=integer(std::int64_t(max_hits)-damage);
 return mul(mul(unit_scalar(*best,"m_iBaseCombat"),100),remaining)/max_hits;
}

// Actual native unit getter adapter. A pending view is minted only inside the
// private canonical Core handoff; imported views keep their real SavedUnit.
struct InitialDefenderView {
 const saves::SavedUnitObservation* raw=nullptr;
 const std::map<std::string,std::int32_t,std::less<>>* scalars=nullptr;
 const std::map<std::string,std::vector<std::int32_t>,std::less<>>* indexed=nullptr;
 const LiveSessionState*state=nullptr;std::int32_t owner=-1,id=-1;
 std::function<void()>pending_current;
 const saves::SavedUnitObservation&current_raw()const{
  need(state,"City defender raw source lease absent");const auto*current=plot_unit(*state,owner,id);
  need(current&&current==raw,"City defender imported unit source/storage changed during query");return *current;
 }
 std::int32_t scalar(std::string_view key,bool boolean=false)const{
  if(scalars){need(static_cast<bool>(pending_current),"City defender pending canonical validator absent");pending_current();const auto i=scalars->find(key);need(i!=scalars->end(),"City defender pending native member absent");
   need(!boolean||i->second==0||i->second==1,"City defender pending boolean noncanonical");return i->second;}
  need(raw,"City defender genuine unit absent");const auto&r=current_raw();
  for(const auto&field:r.core.fields)if(field.name&&std::string_view(field.name)==key)
   return unit_scalar(r,key,boolean?saves::UnitCoreEncoding::boolean:saves::UnitCoreEncoding::i32);
  const auto&block=named(r.tail.blocks,"state");const auto&f=named(block.children,key);
  need(f.encoding&&std::string_view(f.encoding)=="i32"&&!f.wire_count&&f.values.size()==1&&f.bytes.empty()&&f.children.empty(),"City defender actual tail scalar absent/encoding differs");
  return integer(f.values[0]);
 }
 std::int32_t array(std::string_view key,std::int32_t slot,std::int32_t slots)const{
  need(slot>=0&&slot<slots,"City defender actual indexed slot out of range");
  if(indexed){need(static_cast<bool>(pending_current),"City defender pending canonical validator absent");pending_current();const auto i=indexed->find(key);need(i!=indexed->end()&&i->second.size()==static_cast<std::size_t>(slots),"City defender pending indexed native span differs");return i->second[static_cast<std::size_t>(slot)];}
  const auto&f=named(current_raw().core.fields,key);need(f.encoding==saves::UnitCoreEncoding::vector_i32&&f.values.size()==static_cast<std::size_t>(slots)&&f.bytes.empty(),"City defender imported indexed native span differs");
  return integer(f.values[static_cast<std::size_t>(slot)]);
 }
};
std::int32_t initial_info_integer(const ui::SelectedBnwPreviewHost&host,std::string_view table,std::string_view column,std::int32_t id){
 need(table=="Units"||table=="UnitClasses"||table=="Terrains"||table=="Features"||table=="Improvements","City defender private Info query table unsupported");
 std::optional<std::int32_t> result;
 const auto query=host.database().query_rows({"SELECT Type,"+std::string(column)+" FROM "+std::string(table)+" WHERE ID=?",{std::int64_t(id)}},[&](const auto&row){
  const auto type=row.text("Type");const auto*entry=type?host.types().find(*type):nullptr;
  need(!result&&entry&&entry->table==table&&entry->id==id,"City defender actual Info Type/ID unavailable");result=integer(row.integer(column));
 });need(query.all_succeeded()&&result.has_value(),"City defender actual Info query failed");return *result;
}
std::int32_t initial_info_span(const ui::SelectedBnwPreviewHost&host,std::string_view table){
 std::int32_t count=0;
 const auto query=host.database().query_rows({"SELECT ID,Type FROM "+std::string(table)+" ORDER BY ID",{}},[&](const auto&row){
  const auto type=row.text("Type");const auto*entry=type?host.types().find(*type):nullptr;
  need(row.integer("ID")==count&&entry&&entry->table==table&&entry->id==count,"City defender complete native Info span/remap unavailable");count=add(count,1);
 });need(query.all_succeeded()&&count>0,"City defender complete Info table absent");return count;
}
std::int32_t initial_trait_scalar(const saves::SavedPlayerObservation&player,std::string_view key){
 const auto&f=named(player.traits.fields,key);need(player.traits.layout==bnw&&f.encoding==saves::PlayerTraitsEncoding::i32&&!f.wire_count&&f.values.size()==1&&f.rows.empty()&&f.hashes.empty(),"City defender actual trait native cache unavailable");return integer(f.values[0]);
}
// CvUnit::GetMaxDefenseStrength(plot,nullptr,false), with the conservative
// callback-free ordinary own-city domain. Nonzero unimplemented effects are
// refused, never replaced by zero. All ordinary native contributions below
// use authentic serialized runtime members and active selected Info tables.
std::int32_t initial_defense_strength(const LiveSessionState&state,const ui::SelectedBnwPreviewHost&host,
 const saves::PlotObservation&plot,const InitialDefenderView&u,std::int32_t owner,
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)>&resolve){
 const auto base=u.scalar("m_iBaseCombat");need(u.scalar("m_bEmbarked",true)==0,"City defender reached embarked defense requires actual Era owner");
 if(base==0)return 0;
 auto modifier=u.scalar("m_iExtraCombatPercent");
 const auto&player=state.world.all_players->players.at(static_cast<std::size_t>(owner));
 // Native founder lookup precedes happiness. Complete empty actual religion
 // roster proves NO_RELIGION; a founded religion requires its native owner.
 need(state.world.religion_culture.layout==bnw&&state.world.religion_culture.religions.empty(),"City defender founded-religion combat owner required");
 const auto&yp=state.cities.players.at(owner);need(host.session().bnw_unhappiness_current()&&yp.excess_happiness.has_value(),"City defender actual current GetExcessHappiness absent");
 if(*yp.excess_happiness<0){auto penalty=mul(mul(-1,integer(*yp.excess_happiness)),integer(static_cast<std::int64_t>(define(host,"VERY_UNHAPPY_COMBAT_PENALTY_PER_UNHAPPY",content::GlobalDefineKind::integer))));
  penalty=std::max(penalty,integer(static_cast<std::int64_t>(define(host,"VERY_UNHAPPY_MAX_COMBAT_PENALTY",content::GlobalDefineKind::integer))));modifier=add(modifier,penalty);}
 const auto type=u.scalar("m_eUnitType");
 const content::InfoTypeEntry*unit_info=nullptr;
 const auto uq=host.database().query_rows({"SELECT Type FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&row){const auto name=row.text("Type");unit_info=name?host.types().find(*name):nullptr;need(unit_info&&unit_info->table=="Units"&&unit_info->id==type,"City defender actual UnitInfo unavailable");});
 need(uq.all_succeeded()&&unit_info,"City defender selected UnitInfo query failed");
 // With complete genuine no-positive requirements, the resource loop's
 // contribution is zero even if another unit overuses a resource. Preserve
 // the final native max(0, exhaustedPenalty), including custom positive define.
 const auto rq=host.database().query_rows({"SELECT ResourceType,Cost FROM Unit_ResourceQuantityRequirements WHERE UnitType=?",{unit_info->type}},[&](const auto&row){
  const auto resource=row.text("ResourceType");const auto*entry=resource?host.types().find(*resource):nullptr;
  need(entry&&entry->table=="Resources"&&row.integer("Cost")<=0,"City defender reached strategic resource penalty owner required");
 });need(rq.all_succeeded(),"City defender actual resource requirements query failed");
 modifier=add(modifier,std::max(0,integer(static_cast<std::int64_t>(define(host,"STRATEGIC_RESOURCE_EXHAUSTED_PENALTY",content::GlobalDefineKind::integer)))));
 // An exact full actual live roster proof excludes every possible general/
 // admiral/reverse-GG bonus in the radius. Includes the actual pending node;
 // unlike m_hGarrison, this is genuine map/FLTA unit resolution.
 for(std::int32_t actor=0;actor<64;++actor){const auto&raw=state.world.all_players->players.at(static_cast<std::size_t>(actor));
  for(const auto&unit:raw.units.units){const auto view=resolve(actor,unit_scalar(unit,"m_iID"));
   need(view.raw,"City defender genuine roster handle unresolved");
   need(view.scalar("m_iGreatGeneralCount")<=0&&view.scalar("m_iGreatAdmiralCount")<=0&&view.scalar("m_iNearbyEnemyCombatMod")==0,
    "City defender nonzero nearby general/admiral/reverse aura requires native radius owner");
  }
 }
 need(u.scalar("m_iGreatGeneralCount")<=0&&u.scalar("m_iGreatAdmiralCount")<=0&&u.scalar("m_iNearbyEnemyCombatMod")==0,"City defender pending nearby aura requires native radius owner");
 need(initial_trait_scalar(player,"m_iNearbyImprovementCombatBonus")==0,"City defender nonzero nearby improvement combat requires native range owner");
 // Exact zero unit contribution, without assuming no adjacent unit exists.
 need(u.scalar("m_iAdjacentModifier")==0,"City defender nonzero adjacent modifier requires actual neighboring native unit scan");
 const auto golden=player_scalar(player,"m_iGoldenAgeTurns");need(golden>=0&&yp.golden_age==(golden>0),"City defender raw-owned golden age differs");
 if(golden>0)modifier=add(modifier,initial_trait_scalar(player,"m_iGoldenAgeCombatModifier"));
 const auto&cell=state.world.map_projection->cells.at(static_cast<std::size_t>(plot.index));
 need(cell.owner==std::optional<std::int64_t>{owner}&&named(plot.fields,"owner").value==owner,"City defender ordinary friendly-city territory required");
 modifier=add(modifier,u.scalar("m_iFriendlyLandsModifier"));
 const auto capital=u.scalar("m_iCapitalDefenseModifier");
 if(capital>0){need(yp.capital==named(plot.fields,"plot_city_id").value,"City defender noncapital distance supplier required");modifier=add(modifier,capital);} // actual distance zero
 // With null attacker, higher-tech/larger-civ bonuses cannot contribute.
 modifier=add(modifier,u.scalar("m_iDefenseModifier"));
 const auto plot_type=integer(named(plot.fields,"plot_type").value);need(plot_type>=0&&plot_type<=3,"City defender plot category invalid");
 need(cell.terrain_type.has_value(),"City defender actual terrain absent");const auto terrain=integer(*cell.terrain_type);
 std::int32_t feature=-1,improvement=-1;
 const auto resolve_cell_info=[&](auto numeric,auto hash,std::string_view table){
  if(numeric){if(*numeric==-1)return std::int32_t(-1);need(*numeric>=0,"City defender invalid native Info slot");return integer(*numeric);}
  need(hash.has_value(),"City defender actual cell Info unresolved");if(*hash==0)return std::int32_t(-1);
  const auto*entry=host.types().find_hash(*hash);need(entry&&entry->table==table,"City defender actual cell Info hash unresolved");return integer(entry->id);
 };
 feature=resolve_cell_info(cell.feature_type,cell.feature_type_hash,"Features");improvement=resolve_cell_info(cell.improvement_type,cell.improvement_type_hash,"Improvements");
 auto plotdef=(plot_type==1||plot_type==0)?integer(static_cast<std::int64_t>(define(host,"HILLS_EXTRA_DEFENSE",content::GlobalDefineKind::integer))):
  feature>=0?initial_info_integer(host,"Features","Defense",feature):initial_info_integer(host,"Terrains","Defense",terrain);
 if(plot_type!=1&&plot_type!=0&&feature<0&&plot_type!=3)plotdef=add(plotdef,integer(static_cast<std::int64_t>(define(host,"FLAT_LAND_EXTRA_DEFENSE",content::GlobalDefineKind::integer))));
 const auto pillaged=named(plot.fields,"improvement_pillaged").value;need(pillaged==0||pillaged==1,"City defender actual pillaged boolean invalid");
 if(improvement>=0&&!pillaged)plotdef=add(plotdef,initial_info_integer(host,"Improvements","DefenseModifier",improvement)); // same real team; friendly territory
 if(u.scalar("m_iNoDefensiveBonusCount")<=0||plotdef<0)modifier=add(modifier,plotdef);
 auto fortify=0;const auto turns=u.scalar("m_iFortifyTurns");need(turns>=0,"City defender negative native fortify count");
 if(u.scalar("m_iRangedSupportFireCount")<=0&&turns>0){fortify=mul(turns,integer(static_cast<std::int64_t>(define(host,"FORTIFY_MODIFIER_PER_TURN",content::GlobalDefineKind::integer))));
  const auto mod=player_scalar(player,"m_iUnitFortificationModifier");if(mod)fortify=mul(add(100,mod)/100,fortify);fortify=std::max(0,fortify);}
 modifier=add(modifier,fortify);modifier=add(modifier,u.scalar("m_iExtraCityDefensePercent"));
 if(plot_type==1)modifier=add(modifier,u.scalar("m_iExtraHillsDefensePercent"));
 const auto rough=named(plot.fields,"rough_feature").value;need(rough==0||rough==1,"City defender actual rough feature flag invalid");
 if(plot_type!=1&&plot_type!=0&&!rough)modifier=add(modifier,u.scalar("m_iExtraOpenDefensePercent"));
 if(plot_type==1||plot_type==0||rough)modifier=add(modifier,u.scalar("m_iExtraRoughDefensePercent"));
 const auto terrains=initial_info_span(host,"Terrains"),features=initial_info_span(host,"Features");
 if(feature>=0)modifier=add(modifier,u.array("m_extraFeatureDefensePercent",feature,features));
 else{modifier=add(modifier,u.array("m_extraTerrainDefensePercent",terrain,terrains));if(plot_type==1){
  const auto*hill=host.types().find("TERRAIN_HILL");need(hill&&hill->table=="Terrains"&&hill->id==7,"City defender native TERRAIN_HILL enum identity differs");
  modifier=add(modifier,u.array("m_extraTerrainDefensePercent",7,terrains));}}
 modifier=std::max(-90,modifier);return std::max(1,mul(base,add(modifier,100)));
}
std::int32_t initial_world_class(const ui::SelectedBnwPreviewHost&host,const InitialDefenderView&u){
 std::optional<std::int32_t>max;
 const auto q=host.database().query_rows({"SELECT c.ID,c.Type,c.MaxGlobalInstances FROM Units u JOIN UnitClasses c ON c.Type=u.Class WHERE u.ID=?",{std::int64_t(u.scalar("m_eUnitType"))}},[&](const auto&row){
  const auto type=row.text("Type");const auto*entry=type?host.types().find(*type):nullptr;need(!max&&entry&&entry->table=="UnitClasses"&&entry->id==row.integer("ID"),"City defender actual unit class unresolved");max=integer(row.integer("MaxGlobalInstances"));
 });need(q.all_succeeded()&&max.has_value(),"City defender actual world-unit class query failed");return *max!=-1;
}
bool initial_better_defender(const LiveSessionState&state,const ui::SelectedBnwPreviewHost&host,const saves::PlotObservation&plot,
 const InitialDefenderView&u,const InitialDefenderView&best,std::int32_t owner,
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)>&resolve){
 // pDefender=null ->true precedes every remaining native getter. Attacker is
 // null, so canCoexistWithEnemyUnit(NO_TEAM) is false before any visibility.
 if(!best.raw&&!best.scalars)return true;
 auto ours=initial_defense_strength(state,host,plot,u,owner,resolve);
 if(initial_world_class(host,u))ours/=2;
 const auto their_intercept=std::max(0,best.scalar("m_iExtraIntercept"));if(their_intercept>0)ours=mul(ours,add(100,their_intercept))/100;
 const auto cargo=u.scalar("m_iCargo");need(cargo>=0&&cargo<INT32_MAX,"City defender native cargo divisor invalid");ours/=cargo+1;
 auto theirs=initial_defense_strength(state,host,plot,best,owner,resolve);
 if(initial_world_class(host,best))theirs/=2;
 const auto our_intercept=std::max(0,u.scalar("m_iExtraIntercept"));if(our_intercept>0)theirs=mul(theirs,add(100,our_intercept))/100;
 const auto othercargo=best.scalar("m_iCargo");need(othercargo>=0&&othercargo<INT32_MAX,"City defender native competing cargo divisor invalid");theirs/=othercargo+1;
 if(ours==theirs){const auto leader=u.scalar("m_eLeaderUnitType"),otherleader=best.scalar("m_eLeaderUnitType");
  if(leader==-1&&otherleader!=-1)ours=add(ours,1);else if(leader!=-1&&otherleader==-1)theirs=add(theirs,1);
  else{bool before=false;
   // Same actual owner/domain were validated by the reached CanDefend loop.
   const auto combat=u.scalar("m_iBaseCombat"),othercombat=best.scalar("m_iBaseCombat");
   if(combat!=othercombat)before=combat>othercombat;
   else{const auto type=u.scalar("m_eUnitType"),other=best.scalar("m_eUnitType");if(type!=other)before=type>other;
    else{const auto level=u.scalar("m_iLevel"),otherlevel=best.scalar("m_iLevel");if(level!=otherlevel)before=level>otherlevel;
     else{const auto xp=u.scalar("m_iExperience"),otherxp=best.scalar("m_iExperience");before=xp!=otherxp?xp>otherxp:u.scalar("m_iID")<best.scalar("m_iID");}}}
   if(before)ours=add(ours,1);
  }
 }return ours>theirs;
}
InitialDefenderView initial_selected_defender(const LiveSessionState&state,const ui::SelectedBnwPreviewHost&host,
 std::int32_t owner,const saves::PlotObservation&plot,const std::function<InitialDefenderView(std::int32_t,std::int32_t)>&resolve){
 InitialDefenderView best;const auto&stack=named(plot.arrays,"units");
 for(std::size_t row=0;row<stack.values.size();row+=2){const auto actor=integer(stack.values[row]),id=integer(stack.values[row+1]);const auto u=resolve(actor,id);
  if(!u.raw&&!u.scalars)continue;
  need(u.scalar("m_iX")==integer(plot.x)&&u.scalar("m_iY")==integer(plot.y),"City defender native unit/plot coordinates differ");
  need(u.scalar("m_bEmbarked",true)==0,"City defender reached embarked CanDefend requires Era owner");
  const auto base=u.scalar("m_iBaseCombat");need(base>=0,"City defender base strength negative");if(!base)continue;
  if(u.scalar("m_bDeathDelay",true))continue;
  // Actual ordinary land path; other domains remain strict before mutations.
  auto domain=std::int32_t(-1);
  if(u.raw)domain=unit_domain(host,u.current_raw());
  else{const auto q=host.database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{std::int64_t(u.scalar("m_eUnitType"))}},[&](const auto&row){
   const auto type=row.text("Type"),name=row.text("Domain");const auto*entry=type?host.types().find(*type):nullptr;const auto*d=name?host.types().find(*name):nullptr;
   need(domain==-1&&entry&&entry->table=="Units"&&entry->id==u.scalar("m_eUnitType")&&d&&d->table=="Domains","City defender pending actual domain identity differs");domain=integer(d->id);
  });need(q.all_succeeded()&&domain>=0,"City defender pending native domain unavailable");}
  need(domain==2,"City defender non-land native CanDefend branch requires its domain owner");
  need(named(plot.fields,"plot_type").value!=3,"City defender land unit water/WalkWater supplier required");
  if(u.scalar("m_eOwner")!=owner)continue;
  if(initial_better_defender(state,host,plot,u,best,owner,resolve))best=u;
 }
 return best;
}
std::int32_t initial_selected_defender_strength(const LiveSessionState&state,const ui::SelectedBnwPreviewHost&host,
 std::int32_t owner,const saves::PlotObservation&plot,const std::function<InitialDefenderView(std::int32_t,std::int32_t)>&resolve){
 const auto best=initial_selected_defender(state,host,owner,plot,resolve);
 if(!best.raw&&!best.scalars)return 0;
 const auto hp=integer(static_cast<std::int64_t>(define(host,"MAX_HIT_POINTS",content::GlobalDefineKind::integer))),damage=best.scalar("m_iDamage");
 need(hp>0&&damage>=0&&damage<=hp,"City defender native HP divisor/domain invalid");
 return mul(mul(best.scalar("m_iBaseCombat"),100),hp-damage)/hp;
}

struct Computed {const saves::SavedCityObservation* raw=nullptr;std::int32_t id=0,before=0,after=0,known=0;std::int64_t plot=-1;std::int32_t base=0;};
Computed compute_any_living(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t owner,std::int32_t handle,const std::function<std::int32_t(const saves::PlotObservation&)>* pending_defender=nullptr,std::optional<std::int32_t> prepared_strength_modifier={}){
 need(owner>=0&&owner<64&&state.world.game_state.layout==bnw&&state.setup.profile==content::TechnologyProfile::bnw&&state.world.all_players&&state.world.all_players->layout==bnw&&state.world.all_players->players.size()==64&&state.world.all_players->context.player_count==64&&state.world.pregame&&state.world.pregame->layout==bnw,"City strength actual BNW player/Pregame roster absent");
 const auto& player=state.world.all_players->players.at(static_cast<std::size_t>(owner));
 need(player.index==static_cast<std::uint64_t>(owner)&&player.saved_player_id==owner&&player.core.layout==bnw&&player_scalar(player,"m_eID")==owner&&player_scalar(player,"m_bAlive",saves::PlayerCoreEncoding::boolean)==1,"City strength requires actual living member");
 const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");
 const auto& minors=state.world.pregame->archive.flag_arrays.at("minor_nation_civs");
 need(teams.size()==64&&minors.size()==64,"City strength actual Pregame dimensions differ");
 const auto team=integer(teams.at(static_cast<std::size_t>(owner)));need(team>=0&&team<63,"City strength actual member team unavailable");
 need(state.setup.players.at(owner).team==team&&state.setup.players.at(owner).alive&&
  minors.at(static_cast<std::size_t>(owner))==state.player_research.players.at(owner).pregame_minor,
  "City strength actual living member/team raw-owned identity differs");
 const auto* city=resolve_bnw_city_registry(state.world,state.cities,owner,handle);need(city,"City strength actual city handle absent");
 const auto id=city_scalar(*city,"m_iID"),x=city_scalar(*city,"m_iX"),y=city_scalar(*city,"m_iY"),population=city_scalar(*city,"m_iPopulation");
 const auto& owned=state.cities.players.at(owner).cities.at(id);
 need(city_scalar(*city,"m_eOwner")==owner&&x>=0&&y>=0&&owned.x==x&&owned.y==y&&owned.population==population&&population>=0,"City strength actual city population/coordinate raw-owned differs");
 const auto index=state.world.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
 need(state.world.map_header.layout==bnw&&state.world.map_plots&&state.world.map_plots->layout==bnw&&state.world.map_projection&&state.world.map_projection->grid.width==state.world.map_header.grid.width&&state.world.map_projection->grid.height==state.world.map_header.grid.height&&state.world.map_plots->plots.size()==state.world.map_header.grid.size()&&state.world.map_projection->cells.size()==state.world.map_header.grid.size(),"City strength actual complete map absent");
 const auto& raw=state.world.map_plots->plots.at(static_cast<std::size_t>(index));const auto& cell=state.world.map_projection->cells.at(static_cast<std::size_t>(index));
 need(raw.index==index&&raw.x==static_cast<std::uint64_t>(x)&&raw.y==static_cast<std::uint64_t>(y)&&cell.index==index&&cell.x==raw.x&&cell.y==raw.y&&cell.plot_city&&cell.plot_city->owner==owner&&cell.plot_city->id==id&&named(raw.fields,"plot_city_owner").value==owner&&named(raw.fields,"plot_city_id").value==id,"City strength actual city home plot binding differs");
 const auto& stack=named(raw.arrays,"units");
 need(stack.columns==2&&stack.values.size()%2==0&&cell.units&&cell.units->size()==stack.values.size()/2,"City strength actual ordered plot stack unavailable");
 for(std::size_t row=0;row<cell.units->size();++row)need(cell.units->at(row).owner==integer(stack.values[row*2])&&cell.units->at(row).id==integer(stack.values[row*2+1]),"City strength raw/projected unit stack differs");
 const auto plot_type=integer(named(raw.fields,"plot_type").value);
 need(plot_type>=0&&plot_type<=3&&cell.plot_type==std::optional<std::int64_t>{plot_type},"City strength actual hill plot category differs");
 need(city->production.layout==bnw,"City strength actual CityBuildings layout absent");
 const auto& buildings=named(city->production.blocks,"buildings");
 auto defense=integer(named(buildings.fields,"m_iBuildingDefense").value);
 const auto defense_mod=integer(named(buildings.fields,"m_iBuildingDefenseMod").value);
 const auto& rules=host.technologies();need(rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded()&&rules.collection.slots>0&&rules.collection.slots<=INT32_MAX&&state.world.teams&&state.world.teams->layout==bnw&&state.world.teams->teams.size()==64&&state.technologies.size()==64,"City strength actual technology owner absent");
 const auto& t=state.technologies.at(static_cast<std::size_t>(team));const auto& raw_team=state.world.teams->teams.at(static_cast<std::size_t>(team));
 need(raw_team.index==static_cast<std::uint64_t>(team)&&named(raw_team.fields,"id").value==team&&t.team()==team&&t.profile()==rules.profile&&t.slots()==rules.collection.slots&&t.profile_extensions().values.empty()&&t.remap().all_mapped()&&!t.remap().duplicates&&!t.remap().holes&&host.types().report().all_succeeded()&&!host.types().report().hash_collisions&&!host.types().report().name_overwrites,"City strength technology identity/remap differs");
 const auto decoded=restore_team_technologies(raw_team,rules,host.types());
 need(decoded.last_tech()==t.last_tech()&&decoded.slots()==t.slots()&&decoded.remap().all_mapped()&&!decoded.remap().duplicates&&!decoded.remap().holes,"City strength raw-owned technology identity differs");
 for(std::uint64_t slot=0;slot<t.slots();++slot)need(rules.find(slot)&&decoded.at(static_cast<std::int64_t>(slot))==t.at(static_cast<std::int64_t>(slot)),"City strength raw-owned technology value differs");
 const auto known=integer(t.known_count());need(known>=0&&known<=static_cast<std::int64_t>(rules.collection.slots),"City strength known technology count invalid");
 auto strength=integer(static_cast<std::int64_t>(define(host,"CITY_STRENGTH_DEFAULT",content::GlobalDefineKind::integer)));
 const auto pop_change=integer(static_cast<std::int64_t>(define(host,"CITY_STRENGTH_POPULATION_CHANGE",content::GlobalDefineKind::integer)));
 strength=add(strength,mul(population,pop_change));defense=mul(defense,add(100,defense_mod))/100;strength=add(strength,defense);
 const auto garrison_strength=pending_defender?(*pending_defender)(raw):selected_defender_strength(state,host,owner,raw);
 const auto divisor=integer(static_cast<std::int64_t>(define(host,"CITY_STRENGTH_UNIT_DIVISOR",content::GlobalDefineKind::integer)));
 need(divisor!=0,"City strength reached native garrison division by zero");
 const auto garrison_term=mul(garrison_strength,100);need(!(garrison_term==INT32_MIN&&divisor==-1),"City strength native garrison division overflow");strength=add(strength,garrison_term/divisor);
 const auto tech_progress=mul(known,100)/integer(static_cast<std::int64_t>(rules.collection.slots));
 const double progress=tech_progress/100.0*define(host,"CITY_STRENGTH_TECH_BASE",content::GlobalDefineKind::source_float);
 const double exponent=define(host,"CITY_STRENGTH_TECH_EXPONENT",content::GlobalDefineKind::source_float);
 const auto multiplier=integer(static_cast<std::int64_t>(define(host,"CITY_STRENGTH_TECH_MULTIPLIER",content::GlobalDefineKind::integer)));
 need(std::isfinite(progress)&&progress>=0&&exponent>=0,"City strength unsupported source pow domain");
 double tech_mod=std::pow(progress,exponent);tech_mod*=multiplier;tech_mod*=100;
 const double rounded=tech_mod+0.005;
 need(std::isfinite(rounded)&&rounded>double(INT32_MIN)-1.0&&rounded<double(INT32_MAX)+1.0,"City strength reached pow/native conversion undefined boundary");
 strength=add(strength,static_cast<std::int32_t>(rounded));
 // Default path reads the exact same native operand. The private genuine
 // policy owner may stage only this SDK modifier, after sealing the real old
 // raw field; all actual city/defender/buildings/technology guards stay reached.
 const auto raw_strength_modifier=player_scalar(player,"m_iCityStrengthMod");
 strength=mul(strength,add(100,prepared_strength_modifier.value_or(raw_strength_modifier)))/100;
 const auto base_strength=strength;
 if(plot_type==1)strength=add(strength,integer(static_cast<std::int64_t>(define(host,"CITY_STRENGTH_HILL_CHANGE",content::GlobalDefineKind::integer))));
 need(index<=INT64_MAX,"City strength staged UI plot index width unavailable");
 return {city,id,city_scalar(*city,"m_iStrengthValue"),strength,known,static_cast<std::int64_t>(index),base_strength};
}
Computed compute(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t owner,std::int32_t handle){
 need(owner>=22&&owner<63,"City strength existing scratch requires actual minor member");
 need(state.world.pregame&&state.world.pregame->layout==bnw&&state.world.all_players&&state.world.all_players->players.size()==64,
  "City strength existing minor roster missing");
 const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");
 const auto& minors=state.world.pregame->archive.flag_arrays.at("minor_nation_civs");
 need(teams.size()==64&&minors.size()==64,"City strength existing minor Pregame dimensions differ");
 const auto team=integer(teams.at(static_cast<std::size_t>(owner)));
 // isMinorCiv is the first living player22..63 whose actual Pregame team matches.
 bool minor=false;for(std::size_t slot=22;slot<64;++slot){const auto& p=state.world.all_players->players[slot];
  need(p.index==slot&&p.saved_player_id==static_cast<std::int64_t>(slot)&&p.core.layout==bnw&&player_scalar(p,"m_eID")==static_cast<std::int64_t>(slot),"City strength minor roster identity differs");
  const auto alive=player_scalar(p,"m_bAlive",saves::PlayerCoreEncoding::boolean);need(alive==0||alive==1,"City strength actual alive boolean invalid");
  if(alive){need(teams[slot]>=0&&teams[slot]<64,"City strength living member Pregame team invalid");if(teams[slot]==team){minor=minors[slot];break;}}
 }
 need(minor&&state.setup.players.at(owner).team==team&&state.setup.players.at(owner).alive,"City strength actual minor/team raw-owned identity differs");
 return compute_any_living(state,host,owner,handle);
}

}

namespace {
using Seal=std::vector<std::int64_t>;
void put(auto& s,std::uint64_t n){need(n<=INT64_MAX,"City scratch semantic operand exceeds supported width");s.push_back(static_cast<std::int64_t>(n));}
void span(auto& s,const auto& values){put(s,values.size());for(const auto n:values)s.push_back(static_cast<std::int64_t>(n));}
void optional(auto& s,const auto& value){s.push_back(value.has_value());if(value)s.push_back(static_cast<std::int64_t>(*value));}
void core_fields(auto& s,const auto& fields,std::initializer_list<std::string_view> names){
 for(const auto name:names){
  std::uint64_t count=0;
  for(const auto& f:fields){
   if(f.name&&std::string_view(f.name)==name){
    ++count;s.push_back(static_cast<std::int64_t>(f.encoding));optional(s,f.wire_count);span(s,f.values);span(s,f.hashes);span(s,f.bytes);span(s,f.row_counts);
    if constexpr(requires{f.row_keys;})span(s,f.row_keys);
   }
  }
  put(s,count);
 }
}
void integers(auto& s,const auto& fields,std::initializer_list<std::string_view> names){
 for(const auto name:names){std::uint64_t count=0;for(const auto& f:fields)if(f.name&&std::string_view(f.name)==name){++count;s.push_back(f.value);}put(s,count);}
}
void arrays(auto& s,const auto& fields,std::initializer_list<std::string_view> names){
 for(const auto name:names){std::uint64_t count=0;for(const auto& f:fields)if(f.name&&std::string_view(f.name)==name){++count;put(s,f.columns);span(s,f.values);}put(s,count);}
}
constexpr std::array<std::string_view,6> tech_player_columns{"FeatureProductionModifier","UnitFortificationModifier","UnitBaseHealModifier","WorkerSpeedModifier","InfluenceSpreadModifier","ExtraVotesPerDiplomat"};
constexpr std::array<std::string_view,6> tech_player_members{"m_iFeatureProductionModifier","m_iUnitFortificationModifier","m_iUnitBaseHealModifier","m_iWorkerSpeedModifier","m_iInfluenceSpreadModifier","m_iExtraVotesPerDiplomat"};
struct ModifierLocator {std::int32_t player=-1;std::array<std::size_t,6> positions{};bool found=false;};
void player_modifier_inputs(const saves::SavedPlayerObservation& player,auto& s,ModifierLocator* locator){
 s.push_back(static_cast<std::int64_t>(player.core.layout));core_fields(s,player.core.fields,{"m_eID","m_bEverAlive"});
 for(std::size_t i=0;i<tech_player_members.size();++i){
  if(locator&&player.saved_player_id==locator->player){
   const auto& f=named(player.core.fields,tech_player_members[i]);
   need(f.encoding==saves::PlayerCoreEncoding::i32&&!f.wire_count&&f.values.size()==1,"Technology first-player scalar locator encoding differs");
   locator->positions[i]=s.size()+3;locator->found=true;
  }
  core_fields(s,player.core.fields,{tech_player_members[i]});
 }
}

void unit_core_fields(auto& s,const auto& fields,std::initializer_list<std::string_view> names){
 for(const auto name:names){std::uint64_t count=0;
  for(const auto& f:fields){if(f.name&&std::string_view(f.name)==name){
   ++count;s.push_back(static_cast<std::int64_t>(f.encoding));optional(s,f.wire_count);span(s,f.values);span(s,f.bytes);
  }}
  put(s,count);
 }
}
void unit_inputs(const LiveSessionState& state,auto& s,ModifierLocator* locator=nullptr){
 // Relevant GetPlayerUnit/FLTA + IsCanDefend/base/damage operands. This is a
 // finite unit roster projection, not a unit/world serializer or ready input.
 s.push_back(static_cast<std::int64_t>(state.maintenance.profile));s.push_back(state.maintenance.extensions.values.empty());put(s,state.maintenance.players.size());
 for(const auto& [id,p]:state.maintenance.players){s.push_back(id);s.push_back(p.profile_extensions.values.empty());put(s,p.units.size());
  for(const auto& [uid,u]:p.units){s.push_back(uid);s.push_back(u.type);s.push_back(u.domain);s.push_back(u.military);s.push_back(u.civilian);s.push_back(u.profile_extensions.values.empty());
   s.push_back(u.source_core.has_value());if(u.source_core){put(s,u.source_core->first);put(s,u.source_core->second);}
  }
 }
 for(const auto& p:state.world.all_players->players){put(s,p.index);s.push_back(p.saved_player_id);player_modifier_inputs(p,s,locator);const auto& units=p.units;
  s.push_back(static_cast<std::int64_t>(units.layout));s.push_back(static_cast<std::int64_t>(units.header.layout));put(s,units.header.source_slots);s.push_back(units.header.wire_unit_count);
  integers(s,units.header.fields,{"last_index","free_list_count","num_slots"});span(s,units.header.next_free_indices);put(s,units.units.size());
  for(const auto& u:units.units){put(s,u.index);s.push_back(static_cast<std::int64_t>(u.core.layout));put(s,u.core.start_offset);put(s,u.core.end_offset);optional(s,u.core.version);
   unit_core_fields(s,u.core.fields,{"m_iID","m_eOwner","m_iX","m_iY","m_eUnitType","idUnitType","m_iBaseCombat","m_iDamage","m_bEmbarked","m_bDeathDelay","m_iCanMoveAllTerrainCount"});
  }
 }
}

void technology_inputs(const LiveSessionState& state,auto& s){
 put(s,state.technologies.size());for(const auto& t:state.technologies){s.push_back(static_cast<std::int64_t>(t.profile()));s.push_back(t.team());put(s,t.slots());s.push_back(t.last_tech());
  s.push_back(t.profile_extensions().values.empty());const auto& r=t.remap();for(const auto n:{r.saved,r.mapped,r.sentinels,r.unknown,r.outside,r.duplicates,r.holes})put(s,n);s.push_back(r.last_tech_outside);
  for(std::uint64_t slot=0;slot<t.slots();++slot){const auto v=t.at(static_cast<std::int64_t>(slot));s.push_back(v.has_tech);s.push_back(v.no_trade);s.push_back(v.progress_times100);s.push_back(v.count);}
 }
}
std::vector<std::int64_t> technology_values(const LiveSessionState& state){Seal s;technology_inputs(state,s);return s;}
struct InputMatch {
 const Seal& expected;std::size_t position=0;bool equal=true;
 void push_back(std::int64_t n)noexcept{if(position>=expected.size()||expected[position]!=n)equal=false;++position;}
 std::size_t size()const noexcept{return position;}
};
struct InputCount {
 std::size_t position=0;
 void push_back(std::int64_t)noexcept{++position;}
 std::size_t size()const noexcept{return position;}
};
struct Semantic {
 Seal inputs;std::vector<BnwCityDirtyEvent> events;std::uint64_t next=0,revision=0;
 SimulationRandomState rng,map_rng;
 bool operator==(const Semantic&)const=default;
};
void semantic_inputs(const LiveSessionState& state,auto& s,std::optional<std::pair<std::int32_t,std::int32_t>> target=std::nullopt,std::size_t* strength_offset=nullptr,ModifierLocator* modifier_locator=nullptr){
 // Finite relevant inputs only: member/registry, home plots, Buildings defense,
 // player modifier and team technologies. No whole-world serialized snapshot.
 s.push_back(static_cast<std::int64_t>(state.world.game_state.layout));s.push_back(static_cast<std::int64_t>(state.setup.profile));
 need(state.world.pregame&&state.world.all_players&&state.world.teams&&state.world.map_plots&&state.world.map_projection,"City scratch actual relevant BNW source missing");
 s.push_back(static_cast<std::int64_t>(state.world.pregame->layout));
 span(s,state.world.pregame->archive.integer_arrays.at("team_types"));span(s,state.world.pregame->archive.flag_arrays.at("minor_nation_civs"));
 s.push_back(static_cast<std::int64_t>(state.world.all_players->layout));put(s,state.world.all_players->context.player_count);put(s,state.world.all_players->players.size());
 s.push_back(static_cast<std::int64_t>(state.world.map_header.layout));put(s,state.world.map_header.grid.width);put(s,state.world.map_header.grid.height);
 s.push_back(static_cast<std::int64_t>(state.world.map_plots->layout));put(s,state.world.map_plots->plots.size());put(s,state.world.map_projection->cells.size());put(s,state.world.map_projection->grid.width);put(s,state.world.map_projection->grid.height);
 put(s,state.setup.players.size());for(const auto& [id,p]:state.setup.players){s.push_back(id);s.push_back(p.team);s.push_back(p.alive);s.push_back(p.ever_alive);}
 s.push_back(static_cast<std::int64_t>(state.cities.profile));put(s,state.cities.players.size());
 for(std::size_t slot=22;slot<state.world.all_players->players.size();++slot){
  const auto& p=state.world.all_players->players[slot];put(s,p.index);s.push_back(p.saved_player_id);s.push_back(static_cast<std::int64_t>(p.core.layout));
  core_fields(s,p.core.fields,{"m_eID","m_bAlive","m_iCityStrengthMod"});
  s.push_back(static_cast<std::int64_t>(p.cities.layout));s.push_back(static_cast<std::int64_t>(p.cities.header.layout));put(s,p.cities.header.source_slots);s.push_back(p.cities.header.wire_city_count);
  integers(s,p.cities.header.fields,{"last_index","free_list_count","num_slots"});span(s,p.cities.header.next_free_indices);put(s,p.cities.cities.size());
  const auto owned=state.cities.players.find(static_cast<std::int64_t>(slot));s.push_back(owned!=state.cities.players.end());
  if(owned!=state.cities.players.end()){put(s,owned->second.cities.size());for(const auto& [id,city]:owned->second.cities){s.push_back(id);s.push_back(city.id);s.push_back(city.owner);s.push_back(city.x);s.push_back(city.y);s.push_back(city.population);s.push_back(city.source_core.has_value());if(city.source_core){put(s,city.source_core->first);put(s,city.source_core->second);}}}
  for(const auto& city:p.cities.cities){put(s,city.index);s.push_back(static_cast<std::int64_t>(city.core.layout));put(s,city.core.start_offset);put(s,city.core.end_offset);optional(s,city.core.version);
   core_fields(s,city.core.fields,{"m_iID","m_eOwner","m_iX","m_iY","m_iPopulation"});
   if(target&&target->first==static_cast<std::int32_t>(slot)&&target->second==city_scalar(city,"m_iID")){
    need(strength_offset,"City scratch semantic locator missing");
    // i32 encoding, optional wire-count presence, then vector size precede value.
    const auto& strength=named(city.core.fields,"m_iStrengthValue");
    need(strength.encoding==saves::CityCoreEncoding::i32&&!strength.wire_count&&strength.values.size()==1,"City scratch strength locator encoding differs");
    *strength_offset=s.size()+3;
   }
   core_fields(s,city.core.fields,{"m_iStrengthValue"});
   s.push_back(static_cast<std::int64_t>(city.production.layout));std::uint64_t count=0;
   for(const auto& b:city.production.blocks)if(b.name&&std::string_view(b.name)=="buildings"){++count;integers(s,b.fields,{"m_iBuildingDefense","m_iBuildingDefenseMod"});}put(s,count);
   const auto x=city_scalar(city,"m_iX"),y=city_scalar(city,"m_iY");need(x>=0&&y>=0,"City scratch source home coordinate unavailable");
   const auto index=state.world.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));put(s,index);
   const auto& plot=state.world.map_plots->plots.at(static_cast<std::size_t>(index));put(s,plot.index);put(s,plot.x);put(s,plot.y);
   integers(s,plot.fields,{"plot_city_owner","plot_city_id","plot_type"});arrays(s,plot.arrays,{"units"});
   const auto& cell=state.world.map_projection->cells.at(static_cast<std::size_t>(index));put(s,cell.index);put(s,cell.x);put(s,cell.y);optional(s,cell.plot_type);s.push_back(cell.plot_city.has_value());if(cell.plot_city){s.push_back(cell.plot_city->owner);s.push_back(cell.plot_city->id);}
   s.push_back(cell.units.has_value());if(cell.units){put(s,cell.units->size());for(const auto& ref:*cell.units){s.push_back(ref.owner);s.push_back(ref.id);}}
  }
 }
 unit_inputs(state,s,modifier_locator);
 s.push_back(static_cast<std::int64_t>(state.world.teams->layout));put(s,state.world.teams->teams.size());
 for(const auto& team:state.world.teams->teams){put(s,team.index);integers(s,team.fields,{"id","tech_version","saved_techs","last_tech"});arrays(s,team.arrays,{"tech_hashes","has_tech","no_trade_tech","research_progress","tech_count"});}
 // Preserve the original length-delimited tech segment using a nonallocating
 // counter visitor, then the SAME visitor emitting actual fields. Both counts
 // are private traversal output, never an external attestation/witness.
 InputCount count;technology_inputs(state,count);put(s,count.size());technology_inputs(state,s);
}
Semantic capture(const LiveSessionState& state){
 Semantic result;semantic_inputs(state,result.inputs);
 result.events=state.city_dirty_events;result.next=state.city_dirty_next_occurrence;result.revision=state.ui_source_revision;result.rng=state.random.gameplay.state();result.map_rng=state.random.map.state();return result;
}
bool same_semantics(const LiveSessionState& state,const Semantic& expected){
 // On a supported source all successful operations below are nonallocating:
 // scalars, native container traversal/lookup, POD event/RNG comparisons.
 // Malformed input can throw and retire; it cannot reach an effect store.
 InputMatch match{expected.inputs};semantic_inputs(state,match);
 return match.equal&&match.position==expected.inputs.size()&&
  state.city_dirty_events==expected.events&&state.city_dirty_next_occurrence==expected.next&&state.ui_source_revision==expected.revision&&
  state.random.gameplay.state()==expected.rng&&state.random.map.state()==expected.map_rng;
}
void initial_name(auto&s,const char*name){need(name,"City defender unnamed actual source field");const auto n=std::string_view(name);put(s,n.size());for(const auto c:n)s.push_back(static_cast<unsigned char>(c));}
void initial_core_fields(auto&s,const auto&fields,bool omit_strength=false,std::optional<std::int64_t> happiness={},std::optional<std::int64_t> connection={},const saves::PlayerCoreField*projected=nullptr,const std::vector<std::int64_t>*values=nullptr){
 put(s,fields.size());for(const auto&f:fields){initial_name(s,f.name);s.push_back(static_cast<std::int64_t>(f.encoding));
  optional(s,f.wire_count);if(omit_strength&&std::string_view(f.name)=="m_iStrengthValue"){put(s,f.values.size());continue;}
  if(happiness&&std::string_view(f.name)=="m_iHappiness"){need(f.values.size()==1,"Initial happiness scalar shape changed");put(s,f.values.size());s.push_back(*happiness);}
  else if(connection&&std::string_view(f.name)=="m_iCityConnectionHappiness"){need(f.values.size()==1,"Initial connection scalar shape changed");put(s,f.values.size());s.push_back(*connection);}
  else {
   if constexpr(std::is_same_v<std::remove_cvref_t<decltype(f)>,saves::PlayerCoreField>){if(projected==&f&&values)span(s,*values);else span(s,f.values);}
   else span(s,f.values);
  }
  span(s,f.bytes);
  if constexpr(requires{f.hashes;})span(s,f.hashes);
  if constexpr(requires{f.row_counts;})span(s,f.row_counts);
  if constexpr(requires{f.row_keys;})span(s,f.row_keys);
 }
}
void initial_tail_inputs(auto&s,const std::vector<saves::UnitTailNode>&fields){
 put(s,fields.size());for(const auto&f:fields){initial_name(s,f.name);initial_name(s,f.encoding);optional(s,f.wire_count);span(s,f.values);span(s,f.bytes);initial_tail_inputs(s,f.children);}
}
void initial_city_inputs(const LiveSessionState&state,std::int32_t owner,std::int32_t city,auto&s,std::optional<std::int64_t> happiness={},bool clear_excess=false,std::optional<std::int64_t> connection={},std::optional<std::int64_t> excess={},const saves::PlayerCoreField*projected=nullptr,const std::vector<std::int64_t>*values=nullptr){
 // Finite comparator/strength closure: genuine roster (all radius candidates),
 // actor native cache+trait operands, target plot/city, actual technologies.
 // No world clone or arbitrary caller source/freshness witness exists.
 need(state.world.all_players&&state.world.pregame&&state.world.teams&&state.world.map_plots&&state.world.map_projection,"Initial city actual source domains absent");
 put(s,state.world.all_players->players.size());unit_inputs(state,s);
 for(const auto&p:state.world.all_players->players){
  put(s,p.units.units.size());for(const auto&u:p.units.units){put(s,u.index);put(s,u.core.start_offset);put(s,u.core.end_offset);initial_core_fields(s,u.core.fields);initial_tail_inputs(s,u.tail.blocks);}
 }
 const auto&p=state.world.all_players->players.at(static_cast<std::size_t>(owner));initial_core_fields(s,p.core.fields,false,happiness,connection,projected,values);
 put(s,p.traits.fields.size());for(const auto&f:p.traits.fields){initial_name(s,f.name);s.push_back(static_cast<std::int64_t>(f.encoding));optional(s,f.wire_count);put(s,f.columns);span(s,f.values);span(s,f.hashes);
  put(s,f.rows.size());for(const auto&r:f.rows){s.push_back(r.key);s.push_back(r.wire_count);span(s,r.values);}}
 const auto*raw=resolve_bnw_city_registry(state.world,state.cities,owner,city);need(raw,"Initial city source handle retired");
 put(s,raw->index);put(s,raw->core.start_offset);put(s,raw->core.end_offset);initial_core_fields(s,raw->core.fields,true);
 for(const auto&block:raw->production.blocks)if(block.name&&std::string_view(block.name)=="buildings")integers(s,block.fields,{"m_iBuildingDefense","m_iBuildingDefenseMod"});
 const auto&owned=state.cities.players.at(owner);if(excess){s.push_back(true);s.push_back(*excess);}else if(clear_excess){s.push_back(false);}else optional(s,owned.excess_happiness);s.push_back(owned.golden_age);s.push_back(owned.capital);
 const auto&yc=owned.cities.at(city);s.push_back(yc.id);s.push_back(yc.owner);s.push_back(yc.population);s.push_back(yc.x);s.push_back(yc.y);
 const auto index=state.world.map_header.grid.index(static_cast<std::uint64_t>(yc.x),static_cast<std::uint64_t>(yc.y));put(s,index);
 const auto&plot=state.world.map_plots->plots.at(static_cast<std::size_t>(index));put(s,plot.index);put(s,plot.x);put(s,plot.y);
 put(s,plot.fields.size());for(const auto&f:plot.fields){initial_name(s,f.name);s.push_back(f.value);}
 put(s,plot.arrays.size());for(const auto&a:plot.arrays){initial_name(s,a.name);put(s,a.columns);span(s,a.values);}
 const auto&cell=state.world.map_projection->cells.at(static_cast<std::size_t>(index));put(s,cell.index);put(s,cell.x);put(s,cell.y);
 optional(s,cell.owner);optional(s,cell.plot_type);optional(s,cell.terrain_type);optional(s,cell.feature_type);optional(s,cell.feature_type_hash);optional(s,cell.improvement_type);optional(s,cell.improvement_type_hash);
 s.push_back(cell.units.has_value());if(cell.units){put(s,cell.units->size());for(const auto&u:*cell.units){s.push_back(u.owner);s.push_back(u.id);}}
 s.push_back(cell.plot_city.has_value());if(cell.plot_city){s.push_back(cell.plot_city->owner);s.push_back(cell.plot_city->id);}
 // Native founded-religion path is refused if the real roster is nonempty.
 s.push_back(static_cast<std::int64_t>(state.world.religion_culture.layout));put(s,state.world.religion_culture.religions.size());
 put(s,state.setup.players.size());for(const auto&[id,p]:state.setup.players){s.push_back(id);s.push_back(p.team);s.push_back(p.alive);s.push_back(p.ever_alive);}
 for(const auto&[key,values]:state.world.pregame->archive.integer_arrays)if(key=="team_types")span(s,values);
 for(const auto&[key,values]:state.world.pregame->archive.flag_arrays)if(key=="minor_nation_civs")span(s,values);
 for(const auto&t:state.world.teams->teams){put(s,t.index);integers(s,t.fields,{"id","tech_version","saved_techs","last_tech"});arrays(s,t.arrays,{"tech_hashes","has_tech","no_trade_tech","research_progress","tech_count"});}
 technology_inputs(state,s);
}
void initial_node_inputs(const auto&scalars,const auto&indexed,auto&s){
 put(s,scalars.size());for(const auto&[key,value]:scalars){initial_name(s,key.c_str());s.push_back(value);}
 put(s,indexed.size());for(const auto&[key,values]:indexed){initial_name(s,key.c_str());span(s,values);}
}

std::size_t minor_city_strength_seal_offset(const LiveSessionState& state,std::int32_t owner,std::int32_t city){
 std::size_t offset=SIZE_MAX;InputCount count;semantic_inputs(state,count,std::pair{owner,city},&offset);need(offset!=SIZE_MAX,"City scratch exact strength operand not sealed");return offset;
}
}
struct BnwMinorCityStrengthScratch::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};bool busy=false;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> source;std::unique_ptr<LiveSessionState> scratch;
 std::uint64_t generation=0;std::string fingerprint;Semantic source_baseline,scratch_expected;
 explicit Impl(std::shared_ptr<const ui::SelectedBnwPreviewHost> h):host(std::move(h)){
  need(static_cast<bool>(host),"City scratch genuine selected parent missing");source=host->session().read_state_lease();need(static_cast<bool>(source),"City scratch actual source lease missing");
  generation=host->session().state_generation();fingerprint=host->database().gameplay_fingerprint_sha256();need(fingerprint==host->identity().rules_sha256,"City scratch actual rules changed");
  source_baseline=capture(*source);scratch=std::make_unique<LiveSessionState>(*source);scratch_expected=capture(*scratch);
  need(scratch_expected==source_baseline,"City scratch cloning changed relevant source operands");check();
 }
 void creator(){if(std::this_thread::get_id()!=thread){retired.store(true);throw BnwMinorCityStrengthUnavailable("City scratch foreign creator thread");}}
 void check(){creator();need(!retired.load(),"City scratch retired");
  need(host->session().read_state_lease()==source&&host->session().state_generation()==generation,"City scratch actual source/parent retired");
  // Actual rules bookend follows every preparation allocation. Its storage
  // and destruction finish BEFORE the final nonallocating semantic visitor.
  {const auto current=host->database().gameplay_fingerprint_sha256();need(current==fingerprint,"City scratch actual rules retired");}
  need(same_semantics(*source,source_baseline),"City scratch actual in-place source operands/events changed");
  need(same_semantics(*scratch,scratch_expected),"City scratch private operands/events changed outside owned stores");
  need(host->session().read_state_lease()==source&&host->session().state_generation()==generation&&!retired.load(),"City scratch final parent/generation/reentry retired");
 }
};
BnwMinorCityStrengthScratch::LiveQuote BnwMinorCityStrengthScratch::quote_same_live(
 const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t owner,std::int32_t city){
 const auto q=compute_any_living(state,host,owner,city);
 return {q.raw,owner,q.id,q.before,q.after,q.known,q.plot};
}
BnwMinorCityStrengthScratch::LiveQuote BnwMinorCityStrengthScratch::quote_same_live_policy(
 const LiveSessionState&state,const ui::SelectedBnwPreviewHost&host,std::int32_t owner,std::int32_t city,std::int32_t prepared_modifier){
 // Arithmetic quote is private to the genuine Policy owner: it supplies an
 // actual rule-derived staged modifier, never a public readiness witness.
 const auto source=host.session().read_state_lease();const auto generation=host.session().state_generation();
 const auto revision=state.ui_source_revision;
 need(source&&source.get()==&state,"Policy city strength requires genuine SAME Host state");
 const auto q=compute_any_living(state,host,owner,city,nullptr,prepared_modifier);
 const auto current=host.session().read_state_lease();
 need(current.get()==source.get()&&!current.owner_before(source)&&!source.owner_before(current)&&
  host.session().state_generation()==generation&&state.ui_source_revision==revision,
  "Policy city strength actual source changed during native quote");
 return {q.raw,owner,q.id,q.before,q.after,q.known,q.plot};
}

bool BnwMinorCityStrengthScratch::has_same_live_policy_garrison(const LiveSessionState&state,const ui::SelectedBnwPreviewHost&host,std::int32_t owner,std::int32_t city){
 // CvCity::GetGarrisonedUnit6991 always computes the actual plot owner
 // getBestDefender(getOwner()); saved m_hGarrison is not that authority.
 const auto source=host.session().read_state_lease();const auto generation=host.session().state_generation();const auto revision=state.ui_source_revision;
 need(source&&source.get()==&state,"Policy garrison requires genuine SAME Host state");
 const auto*raw=resolve_bnw_city_registry(state.world,state.cities,owner,city);
 need(raw&&state.world.map_plots&&state.cities.players.contains(owner)&&state.cities.players.at(owner).cities.contains(city),"Policy garrison actual city/plot owner absent");
 const auto&actual=state.cities.players.at(owner).cities.at(city);const auto&grid=state.world.map_header.grid;
 need(actual.owner==owner&&actual.id==city&&actual.x>=0&&actual.y>=0&&static_cast<std::uint64_t>(actual.x)<grid.width&&static_cast<std::uint64_t>(actual.y)<grid.height,
  "Policy garrison actual paired city coordinates unavailable");
 const auto index=grid.index(static_cast<std::uint64_t>(actual.x),static_cast<std::uint64_t>(actual.y));
 need(index<state.world.map_plots->plots.size(),"Policy garrison city plot outside native map");const auto&plot=state.world.map_plots->plots[static_cast<std::size_t>(index)];
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)>resolver=[&](std::int32_t actor,std::int32_t handle){
  const auto*unit=plot_unit(state,actor,handle);return InitialDefenderView{unit,nullptr,nullptr,&state,actor,unit?unit_scalar(*unit,"m_iID"):-1,{}};
 };
 const auto selected=initial_selected_defender(state,host,owner,plot,resolver);const bool result=selected.raw!=nullptr;
 const auto current=host.session().read_state_lease();
 need(current.get()==source.get()&&!current.owner_before(source)&&!source.owner_before(current)&&host.session().state_generation()==generation&&state.ui_source_revision==revision&&
  resolve_bnw_city_registry(state.world,state.cities,owner,city)==raw,"Policy garrison actual source/city changed during native scan");
 return result;
}

BnwMinorCityStrengthScratch::InitialQuote BnwMinorCityStrengthScratch::prepare_initial_pending(const BnwNativeUnitAllocatorCore&core){
 const auto canonical=core.retain_factory_owner();core.require_initial_city_strength_node_without_callbacks();
 const auto args=core.initial_placement_operands();
 InitialQuote result;result.state=args.state;result.core=canonical;result.generation=args.generation;
 result.session=args.session;result.happiness_current=args.session->bnw_unhappiness_current();
 result.before_revision=args.revision;need(args.revision<UINT64_MAX,"Initial city strength revision exhausted");result.after_revision=args.revision+1;
 const auto*original_city=resolve_bnw_city_registry(args.state->world,args.state->cities,args.owner,args.city);
 need(original_city,"Initial city strength actual city absent before preparation");
 initial_city_inputs(*args.state,args.owner,args.city,result.inputs);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),result.inputs);
 result.before_events=args.state->city_dirty_events;result.before_occurrence=args.state->city_dirty_next_occurrence;
 result.rng=args.state->random.gameplay.state();result.map_rng=args.state->random.map.state();
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)> resolver=[&](std::int32_t actor,std::int32_t id){
  if(actor==args.owner&&id!=-1&&(id&8191)==(args.id&8191)&&((id&~8191)==0||id==args.id)){
   core.require_initial_city_strength_node_without_callbacks();
   return InitialDefenderView{nullptr,&core.initial_city_strength_scalars(),&core.initial_city_strength_indexed(),nullptr,-1,-1,[&core]{core.require_initial_city_strength_node_without_callbacks();}};
  }
  const auto*raw=plot_unit(*args.state,actor,id);return InitialDefenderView{raw,nullptr,nullptr,args.state.get(),actor,raw?unit_scalar(*raw,"m_iID"):-1,{}};
 };
 const std::function<std::int32_t(const saves::PlotObservation&)> defender=[&](const auto&plot){
  return initial_selected_defender_strength(*args.state,*args.host,args.owner,plot,resolver);
 };
 const auto q=compute_any_living(*args.state,*args.host,args.owner,args.city,&defender);
 need(q.raw==original_city,"Initial city strength city storage replaced during preparation");
 result.quote={q.raw,args.owner,q.id,q.before,q.after,q.known,q.plot};
 const auto occurrence=result.before_occurrence;need(occurrence>0&&occurrence<UINT64_MAX,"Initial city strength dirty occurrence exhausted");
 std::uint64_t previous=0;for(const auto&e:result.before_events){need(e.generation==result.generation&&e.occurrence>previous&&e.occurrence<occurrence,"Initial city strength actual event order/generation differs");previous=e.occurrence;}
 result.after_events=result.before_events;need(result.after_events.size()<result.after_events.max_size(),"Initial city strength dirty capacity exhausted");
 result.after_events.push_back({occurrence,result.generation,args.owner,q.id,-1,-1,BnwCityDirtyKind::city_info,true});
 require_initial_pending_without_callbacks(core,result,false);return result;
}

BnwMinorCityStrengthScratch::InitialSuffixQuote BnwMinorCityStrengthScratch::prepare_initial_suffix_strength(const BnwNativeUnitAllocatorCore&core){
 const auto issuer=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);const auto&canonical=issuer->owner_;const auto args=core.initial_second_city_strength_operands();
 need(args.city>=0&&args.state&&args.host,"Second Init strength actual city absent");InitialSuffixQuote out;
 out.core=canonical;out.state=args.state;out.generation=args.generation;out.before_revision=args.revision;
 initial_city_inputs(*args.state,args.owner,args.city,out.inputs);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),out.inputs);
 out.rng=args.state->random.gameplay.state();out.map_rng=args.state->random.map.state();
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)>resolver=[&](std::int32_t owner,std::int32_t id){
  if(owner==args.owner&&id!=-1&&(id&8191)==(args.id&8191)&&((id&~8191)==0||id==args.id)){
   core.require_initial_city_strength_node_without_callbacks();
   return InitialDefenderView{nullptr,&core.initial_city_strength_scalars(),&core.initial_city_strength_indexed(),nullptr,-1,-1,[&core]{core.require_initial_city_strength_node_without_callbacks();}};
  }
  const auto*raw=plot_unit(*args.state,owner,id);return InitialDefenderView{raw,nullptr,nullptr,args.state.get(),owner,raw?unit_scalar(*raw,"m_iID"):-1,{}};
 };
 const std::function<std::int32_t(const saves::PlotObservation&)>defender=[&](const auto&plot){return initial_selected_defender_strength(*args.state,*args.host,args.owner,plot,resolver);};
 const auto q=compute_any_living(*args.state,*args.host,args.owner,args.city,&defender);
 out.quote={q.raw,args.owner,q.id,q.before,q.after,q.known,q.plot};require_initial_suffix_strength_without_callbacks(core,out,false);return out;
}
void BnwMinorCityStrengthScratch::require_initial_suffix_strength_without_callbacks(const BnwNativeUnitAllocatorCore&core,const InitialSuffixQuote&q,bool after){
 const auto args=core.initial_second_city_strength_operands();
 need(!q.core.expired()&&!q.core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(q.core)&&args.state.get()==q.state.get()&&
  args.generation==q.generation&&args.owner==q.quote.owner&&args.city==q.quote.city&&q.before_revision<UINT64_MAX&&args.revision==q.before_revision+(after?1u:0u),"Second Init strength exact current source/revision changed");
 const auto*raw=resolve_bnw_city_registry(q.state->world,q.state->cities,q.quote.owner,q.quote.city);
 need(raw&&raw==q.quote.raw&&city_scalar(*raw,"m_iStrengthValue")== (after?q.quote.after:q.quote.before),"Second Init strength physical cache changed");
 const auto&expected=q.army_ai_stored?q.army_ai_inputs:q.inputs;InputMatch match{expected};initial_city_inputs(*q.state,q.quote.owner,q.quote.city,match);initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),match);
 need(match.equal&&match.position==expected.size()&&q.state->random.gameplay.state()==q.rng&&q.state->random.map.state()==q.map_rng,"Second Init strength genuine current inputs/RNG changed");
 core.require_initial_city_strength_node_without_callbacks();
}

std::vector<std::int64_t>BnwMinorCityStrengthScratch::prepare_initial_suffix_node_inputs(const BnwNativeUnitAllocatorCore&core,const InitialSuffixQuote&q,
 const std::map<std::string,std::int32_t,std::less<>>&scalars,const std::map<std::string,std::vector<std::int32_t>,std::less<>>&indexed){
 const auto issuer=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);const auto&canonical=issuer->owner_;require_initial_suffix_strength_without_callbacks(core,q,true);
 InputCount count;initial_city_inputs(*q.state,q.quote.owner,q.quote.city,count);initial_node_inputs(scalars,indexed,count);
 std::vector<std::int64_t>result;result.reserve(count.size());initial_city_inputs(*q.state,q.quote.owner,q.quote.city,result);initial_node_inputs(scalars,indexed,result);
 need(result.size()==count.size(),"Init ArmyAI suffix observer shape changed");require_initial_suffix_strength_without_callbacks(core,q,true);return result;
}

BnwMinorCityStrengthScratch::InitialCenterSelection BnwMinorCityStrengthScratch::prepare_initial_center_selection(
 const BnwNativeUnitAllocatorCore&core,std::optional<std::pair<std::int32_t,std::int64_t>> selection){
 const auto canonical=core.retain_factory_owner();core.require_initial_setxy_tail_without_callbacks();
 const auto args=core.initial_placement_operands();
 need(args.host&&args.state&&args.owner==detail::bnw_game_active_player_read(*args.state),"Initial center actual active actor differs");
 const auto team=detail::bnw_game_active_team_read(*args.state);
 need(team==detail::bnw_player_team_read(*args.state,args.owner),"Initial center actual active team differs");
 const auto index=args.state->world.map_header.grid.index(static_cast<std::uint64_t>(args.x),static_cast<std::uint64_t>(args.y));
 const auto&plot=args.state->world.map_plots->plots.at(static_cast<std::size_t>(index));
 const auto&stack=named(plot.arrays,"units");need(stack.columns==2&&stack.values.size()%2==0,"Initial center native linked plot handles malformed");
 InitialCenterSelection result;result.entries.reserve(stack.values.size()/2);
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)> resolver=[&](std::int32_t owner,std::int32_t id){
  if(owner==args.owner&&id!=-1&&(id&8191)==(args.id&8191)&&((id&~8191)==0||id==args.id)){
   core.require_initial_city_strength_node_without_callbacks();
   return InitialDefenderView{nullptr,&core.initial_city_strength_scalars(),&core.initial_city_strength_indexed(),nullptr,-1,-1,
    [&core]{core.require_initial_city_strength_node_without_callbacks();}};
  }
  const auto*raw=plot_unit(*args.state,owner,id);return InitialDefenderView{raw,nullptr,nullptr,args.state.get(),owner,raw?unit_scalar(*raw,"m_iID"):-1,{}};
 };
 for(std::size_t row=0;row<stack.values.size();row+=2){
  const auto owner=integer(stack.values[row]),asked=integer(stack.values[row+1]);const auto unit=resolver(owner,asked);
  InitialCenterEntry entry;entry.owner=owner;entry.asked_id=asked;entry.raw=unit.raw;entry.pending=unit.scalars!=nullptr;
  entry.resolved=unit.raw||unit.scalars;
  if(entry.resolved){
   entry.id=unit.scalar("m_iID");entry.type=unit.scalar("m_eUnitType");
   need(unit.scalar("m_eOwner")==owner&&unit.scalar("m_iX")==args.x&&unit.scalar("m_iY")==args.y,"Initial center actual resolved unit identity/plot differs");
   // Scope is genuine full own roster, never an owner-filtered subset. For
   // EVERY resolved entry the actual native isInvisible(activeTeam,false)
   // returns false at owner-team equality BEFORE cargo/invisible fields.
   need(owner==args.owner&&detail::bnw_player_team_read(*args.state,owner)==team,
    "Initial center reached foreign unit invisibility/defender requires its native owner");
   const auto domain=unit.raw?unit_domain(*args.host,unit.current_raw()):[&]{
    std::optional<std::int32_t> value;const auto q=args.host->database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{std::int64_t(entry.type)}},[&](const auto&r){
     const auto type=r.text("Type"),domain=r.text("Domain");const auto*t=type?args.host->types().find(*type):nullptr;const auto*d=domain?args.host->types().find(*domain):nullptr;
     need(!value&&t&&t->table=="Units"&&t->id==entry.type&&d&&d->table=="Domains","Initial center actual UnitInfo domain remap differs");value=integer(d->id);
    });need(q.all_succeeded()&&value.has_value(),"Initial center actual pending domain missing");return *value;
   }();
   need(domain==2,"Initial center non-LAND receiver/domain branch not closed");
   if(selection&&selection->first==entry.owner&&selection->second==entry.id&&!result.selected)result.selected=result.entries.size();
  }
  result.entries.push_back(entry);
 }
 if(!result.selected){
  // Native call is getBestDefender(NO_PLAYER,activePlayer). All resolved
  // owners were exhaustively proved same active actor; invisibility is the
  // source-backed own-team false branch. Thus reuse exactly the existing
  // full ordered native comparator, WITHOUT dropping foreign entries.
  const auto best=initial_selected_defender(*args.state,*args.host,args.owner,plot,resolver);
  if(best.raw||best.scalars){const auto id=best.scalar("m_iID");
   for(std::size_t i=0;i<result.entries.size();++i)if(result.entries[i].resolved&&result.entries[i].owner==args.owner&&result.entries[i].id==id){need(!result.best,"Initial center selected fullID duplicated");result.best=i;}
   need(result.best.has_value(),"Initial center genuine best defender missing from full roster");
  }else if(!result.entries.empty()&&result.entries.front().resolved){
   // SDK fallback checks ONLY head, not later eligible entries. A head with
   // zero CanDefend has no garrison: the exact complete best-defender call
   // above returned NULL. LAND +own-team invisibility were really proved.
   result.head=0;
  }
 }
 core.require_initial_setxy_tail_without_callbacks();return result;
}
std::optional<std::int32_t> BnwMinorCityStrengthScratch::selected_initial_garrison_unit_id(const BnwNativeUnitAllocatorCore&core,std::int32_t city){
 const auto canonical=core.retain_factory_owner();core.require_initial_set_position_without_callbacks();
 const auto args=core.initial_placement_operands();
 need(args.state&&args.host&&city>=0,"Initial happiness actual city/source missing");
 const auto*raw=resolve_bnw_city_registry(args.state->world,args.state->cities,args.owner,city);
 need(raw,"Initial happiness actual city registry missing");
 const auto&live=args.state->cities.players.at(args.owner).cities.at(city);
 need(live.owner==args.owner&&live.id==city&&live.x>=0&&live.y>=0&&
  static_cast<std::uint64_t>(live.x)<args.state->world.map_header.grid.width&&
  static_cast<std::uint64_t>(live.y)<args.state->world.map_header.grid.height,"Initial happiness actual city/plot identity differs");
 const auto index=args.state->world.map_header.grid.index(static_cast<std::uint64_t>(live.x),static_cast<std::uint64_t>(live.y));
 const auto&plot=args.state->world.map_plots->plots.at(static_cast<std::size_t>(index));
 need(named(plot.fields,"plot_city_owner").value==args.owner&&named(plot.fields,"plot_city_id").value==city,
  "Initial happiness actual city pointer differs from plot");
 InputCount count;initial_city_inputs(*args.state,args.owner,city,count);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),count);
 Seal before;before.reserve(count.size()); // No callback/capture allocation after this reservation.
 initial_city_inputs(*args.state,args.owner,city,before);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),before);
 need(before.size()==count.size(),"Initial happiness input extent changed during staging");
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)> resolver=[&](std::int32_t actor,std::int32_t id){
  if(actor==args.owner&&id!=-1&&(id&8191)==(args.id&8191)&&((id&~8191)==0||id==args.id)){
   core.require_initial_city_strength_node_without_callbacks();
   return InitialDefenderView{nullptr,&core.initial_city_strength_scalars(),&core.initial_city_strength_indexed(),nullptr,-1,-1,
    [&core]{core.require_initial_city_strength_node_without_callbacks();}};
  }
  const auto*u=plot_unit(*args.state,actor,id);return InitialDefenderView{u,nullptr,nullptr,args.state.get(),actor,u?unit_scalar(*u,"m_iID"):-1,{}};
 };
 const auto selected=initial_selected_defender(*args.state,*args.host,args.owner,plot,resolver);
 std::optional<std::int32_t> result;
 if(selected.raw)result=unit_scalar(*selected.raw,"m_iID");
 else if(selected.scalars)result=selected.scalars->at("m_iID"); // GetID of the genuine selected defender, never lowbits.
 core.require_initial_set_position_without_callbacks();
 InputMatch current{before};initial_city_inputs(*args.state,args.owner,city,current);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),current);
 need(current.equal&&current.position==before.size()&&args.host->session().read_state_lease().get()==args.state.get()&&
  args.host->session().state_generation()==args.generation,"Initial happiness defender/source operands changed during native query");
 core.require_initial_set_position_without_callbacks();return result;
}
BnwMinorCityStrengthScratch::InitialSetupGarrisonQuote BnwMinorCityStrengthScratch::prepare_initial_setup_garrison(const BnwNativeUnitAllocatorCore&core,std::int32_t city){
 const auto issuer=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);(void)issuer->current();core.require_initial_minor_quest_return_without_callbacks();
 const auto args=core.initial_setup_graphical_native_operands();
 const auto rng_before=args.state->random.gameplay.state(),map_rng_before=args.state->random.map.state();
 need(args.state&&args.host&&city>=0,"Initial happiness actual city/source missing");
 const auto*raw=resolve_bnw_city_registry(args.state->world,args.state->cities,args.owner,city);
 need(raw,"Initial happiness actual city registry missing");
 const auto&live=args.state->cities.players.at(args.owner).cities.at(city);
 need(live.owner==args.owner&&live.id==city&&live.x>=0&&live.y>=0&&
  static_cast<std::uint64_t>(live.x)<args.state->world.map_header.grid.width&&
  static_cast<std::uint64_t>(live.y)<args.state->world.map_header.grid.height,"Initial happiness actual city/plot identity differs");
 const auto index=args.state->world.map_header.grid.index(static_cast<std::uint64_t>(live.x),static_cast<std::uint64_t>(live.y));
 const auto&plot=args.state->world.map_plots->plots.at(static_cast<std::size_t>(index));
 need(named(plot.fields,"plot_city_owner").value==args.owner&&named(plot.fields,"plot_city_id").value==city,
  "Initial happiness actual city pointer differs from plot");
 InputCount count;initial_city_inputs(*args.state,args.owner,city,count);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),count);
 Seal before;before.reserve(count.size()); // No callback/capture allocation after this reservation.
 initial_city_inputs(*args.state,args.owner,city,before);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),before);
 need(before.size()==count.size(),"Initial happiness input extent changed during staging");
 const std::function<InitialDefenderView(std::int32_t,std::int32_t)> resolver=[&](std::int32_t actor,std::int32_t id){
  if(actor==args.owner&&id!=-1&&(id&8191)==(args.id&8191)&&((id&~8191)==0||id==args.id)){
   core.require_initial_city_strength_node_without_callbacks();
   return InitialDefenderView{nullptr,&core.initial_city_strength_scalars(),&core.initial_city_strength_indexed(),nullptr,-1,-1,
    [&core]{core.require_initial_city_strength_node_without_callbacks();}};
  }
  const auto*u=plot_unit(*args.state,actor,id);return InitialDefenderView{u,nullptr,nullptr,args.state.get(),actor,u?unit_scalar(*u,"m_iID"):-1,{}};
 };
 const auto selected=initial_selected_defender(*args.state,*args.host,args.owner,plot,resolver);
 std::optional<std::int32_t> result;
 if(selected.raw)result=unit_scalar(*selected.raw,"m_iID");
 else if(selected.scalars)result=selected.scalars->at("m_iID"); // GetID of the genuine selected defender, never lowbits.
 core.require_initial_minor_quest_return_without_callbacks();
 InputMatch current{before};initial_city_inputs(*args.state,args.owner,city,current);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),current);
 need(current.equal&&current.position==before.size()&&args.host->session().read_state_lease().get()==args.state.get()&&
  args.host->session().state_generation()==args.generation,"Initial happiness defender/source operands changed during native query");
 core.require_initial_minor_quest_return_without_callbacks();(void)issuer->current();InitialSetupGarrisonQuote out;out.state=args.state;out.owner=args.owner;out.city=city;out.generation=args.generation;out.revision=args.revision;out.selected=result;out.inputs=std::move(before);out.rng=rng_before;out.map_rng=map_rng_before;require_initial_setup_garrison_without_callbacks(core,out);return out;
}
void BnwMinorCityStrengthScratch::require_initial_setup_garrison_without_callbacks(const BnwNativeUnitAllocatorCore&core,const InitialSetupGarrisonQuote&q){
 const auto args=core.initial_setup_graphical_native_operands();need(args.state.get()==q.state.get()&&args.owner==q.owner&&args.city==q.city&&args.generation==q.generation&&args.revision==q.revision,"setupGraphical current garrison source/city/revision changed");InputMatch current{q.inputs};initial_city_inputs(*q.state,q.owner,q.city,current);initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),current);need(current.equal&&current.position==q.inputs.size()&&q.state->random.gameplay.state()==q.rng&&q.state->random.map.state()==q.map_rng,"setupGraphical current native defender inputs/RNG changed");core.require_initial_minor_quest_return_without_callbacks();
}

bool BnwMinorCityStrengthScratch::has_initial_pending_garrison(const BnwNativeUnitAllocatorCore&core,std::int32_t city){
 return selected_initial_garrison_unit_id(core,city).has_value(); // Existing native pointer != NULL numeric semantics.
}
std::vector<std::int64_t>BnwMinorCityStrengthScratch::prepare_initial_happiness_inputs(const BnwNativeUnitAllocatorCore&core,const InitialQuote&q,std::int32_t happiness,std::optional<std::int32_t> connection,std::optional<std::int32_t> excess){
 const auto canonical=core.retain_factory_owner();core.require_initial_set_position_without_callbacks();
 require_initial_pending_without_callbacks(core,q,true);InputCount count;
 initial_city_inputs(*q.state,q.quote.owner,q.quote.city,count,happiness,true,connection,excess);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),count);
 std::vector<std::int64_t> result;result.reserve(count.size());
 initial_city_inputs(*q.state,q.quote.owner,q.quote.city,result,happiness,true,connection,excess);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),result);
 need(result.size()==count.size(),"Initial happiness descendant extent changed during preparation");
 require_initial_pending_without_callbacks(core,q,true);return result;
}
std::vector<std::int64_t>BnwMinorCityStrengthScratch::prepare_initial_name_inputs(const BnwNativeUnitAllocatorCore&core,const InitialQuote&q,
 const std::map<std::string,std::int32_t,std::less<>>&scalars,const std::map<std::string,std::vector<std::int32_t>,std::less<>>&indexed){
 const auto issuer=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);const auto&canonical=issuer->owner_;require_initial_pending_without_callbacks(core,q,true);
 InputCount count;initial_city_inputs(*q.state,q.quote.owner,q.quote.city,count);initial_node_inputs(scalars,indexed,count);
 std::vector<std::int64_t>result;result.reserve(count.size());initial_city_inputs(*q.state,q.quote.owner,q.quote.city,result);initial_node_inputs(scalars,indexed,result);
 need(result.size()==count.size(),"Initial name descendant seal changed during preparation");require_initial_pending_without_callbacks(core,q,true);return result;
}

std::vector<std::int64_t>BnwMinorCityStrengthScratch::prepare_initial_player_class_inputs(const BnwNativeUnitAllocatorCore&core,const InitialQuote&q,const saves::PlayerCoreField&field,const std::vector<std::int64_t>&values){
 const auto canonical=core.retain_factory_owner();require_initial_pending_without_callbacks(core,q,true);
 need(q.state->world.all_players&&q.quote.owner>=0&&static_cast<std::size_t>(q.quote.owner)<q.state->world.all_players->players.size(),"Init Player city actual actor absent");
 const auto&actual=q.state->world.all_players->players[static_cast<std::size_t>(q.quote.owner)];bool found=false;
 for(const auto&f:actual.core.fields)if(&f==&field){need(!found&&(f.name?std::string_view(f.name):std::string_view{})=="m_paiUnitClassCount"&&f.encoding==saves::PlayerCoreEncoding::vector_i32&&
  f.values.size()==values.size()&&f.wire_count==values.size(),"Init Player city exact native class field differs");found=true;}
 need(found,"Init Player city actual field outside actor");InputCount count;
 initial_city_inputs(*q.state,q.quote.owner,q.quote.city,count,{},false,{},{},&field,&values);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),count);
 std::vector<std::int64_t>result;result.reserve(count.size());initial_city_inputs(*q.state,q.quote.owner,q.quote.city,result,{},false,{},{},&field,&values);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),result);
 need(result.size()==count.size(),"Init Player city descendant extent changed");require_initial_pending_without_callbacks(core,q,true);return result;
}
std::vector<std::int64_t>BnwMinorCityStrengthScratch::prepare_initial_military_inputs(const BnwNativeUnitAllocatorCore&core,const InitialQuote&q,const saves::PlayerCoreField&field,const std::vector<std::int64_t>&values){
 const auto canonical=core.retain_factory_owner();require_initial_pending_without_callbacks(core,q,true);
 need(q.state->world.all_players&&q.quote.owner>=0&&static_cast<std::size_t>(q.quote.owner)<q.state->world.all_players->players.size(),"Init military city actual actor absent");
 const auto&actual=q.state->world.all_players->players[static_cast<std::size_t>(q.quote.owner)];bool found=false;
 for(const auto&f:actual.core.fields)if(&f==&field){need(!found&&(f.name?std::string_view(f.name):std::string_view{})=="m_iNumMilitaryUnits"&&f.encoding==saves::PlayerCoreEncoding::i32&&
  f.values.size()==1&&values.size()==1&&!f.wire_count,"Init military city exact native class field differs");found=true;}
 need(found,"Init military city actual field outside actor");InputCount count;
 initial_city_inputs(*q.state,q.quote.owner,q.quote.city,count,{},false,{},{},&field,&values);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),count);
 std::vector<std::int64_t>result;result.reserve(count.size());initial_city_inputs(*q.state,q.quote.owner,q.quote.city,result,{},false,{},{},&field,&values);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),result);
 need(result.size()==count.size(),"Init military city descendant extent changed");require_initial_pending_without_callbacks(core,q,true);return result;
}

void BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(const BnwNativeUnitAllocatorCore&core,const InitialQuote&q,bool after){
 core.require_initial_city_strength_node_without_callbacks();
 need(!q.core.expired()&&!q.core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(q.core),"Initial city strength canonical Core receipt differs");
 const auto&state=*q.state;
 need(q.session&&q.session->bnw_unhappiness_current()==(q.happiness_prefix_stored?q.happiness_cache_current:q.happiness_current),"Initial city strength actual happiness current guard changed");
 need(q.quote.owner>=0&&q.quote.city>=0&&core.initial_setxy_earned_world_revision_without_callbacks()==(after?(q.happiness_prefix_stored?q.happiness_revision:q.after_revision):q.before_revision),"Initial city strength actual source revision differs");
 const auto*raw=resolve_bnw_city_registry(state.world,state.cities,q.quote.owner,q.quote.city);
 need(raw&&raw==q.quote.raw&&city_scalar(*raw,"m_iStrengthValue")== (after?q.quote.after:q.quote.before),"Initial city strength real city/cache storage changed");
 const auto&expected=q.army_ai_stored?q.army_ai_inputs:(q.cycle_order_stored?q.cycle_order_inputs:(q.set_moves_stored?q.set_moves_inputs:(q.immobile_moves_stored?q.immobile_moves_inputs:(q.exotic_xp_stored?q.exotic_xp_inputs:(q.embark_promotion_stored?q.embark_promotion_inputs:(q.player_promotion_stored?q.player_promotion_inputs:(q.traits_promotion_stored?q.traits_promotion_inputs:(q.promotion_loop_stored?q.promotion_loop_inputs:(q.promotion_stored?q.promotion_inputs:(q.military_stored?q.military_inputs:(q.player_class_stored?q.player_class_inputs:(q.name_prefix_stored?q.name_inputs:(q.happiness_prefix_stored?q.happiness_inputs:q.inputs)))))))))))));
 InputMatch match{expected};initial_city_inputs(state,q.quote.owner,q.quote.city,match);
 initial_node_inputs(core.initial_city_strength_scalars(),core.initial_city_strength_indexed(),match);
 need(match.equal&&match.position==expected.size(),"Initial city strength relevant native operands changed");
 need(core.initial_second_city_prior_events_match_without_callbacks(after?q.after_events:q.before_events,q.before_occurrence+(after?1u:0u))&&
  state.random.gameplay.state()==q.rng&&state.random.map.state()==q.map_rng,"Initial city strength actual events/RNG changed");
 // Final strong state/control checks belong to the actual Core, and occur
 // AFTER all nonallocating current semantic comparisons, right before stores.
 core.require_initial_city_strength_node_without_callbacks();
}

BnwMinorCityStrengthScratch::BnwMinorCityStrengthScratch(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwMinorCityStrengthScratch::~BnwMinorCityStrengthScratch()=default;
std::shared_ptr<BnwMinorCityStrengthScratch> BnwMinorCityStrengthScratch::from_selected_preview(const ui::SelectedBnwReadOnlyPreview& preview){
 std::shared_ptr<BnwMinorCityStrengthScratch> result;
 preview.with_owned_host_camera_input([&](std::shared_ptr<const ui::SelectedBnwPreviewHost> actual,const ui::LiveBnwPreviewInput*){
  need(!result,"City scratch genuine producer invoked twice");
  auto p=std::make_shared<Impl>(std::move(actual));auto owner=std::shared_ptr<BnwMinorCityStrengthScratch>(new BnwMinorCityStrengthScratch(p));p->check();result=std::move(owner);
 });need(static_cast<bool>(result),"City scratch genuine producer returned no parent");result->impl_->check();return result;
}
BnwMinorCityStrengthUpdate BnwMinorCityStrengthScratch::refresh(std::int32_t owner,std::int32_t handle){
 const auto p=impl_;p->creator();if(p->busy){p->retired.store(true);throw BnwMinorCityStrengthUnavailable("City scratch reentered");}
 p->busy=true;struct Exit{Impl& p;~Exit(){p.busy=false;}}exit{*p};
 try{
  p->check();const auto q=compute(*p->scratch,*p->host,owner,handle);
  auto events=p->scratch->city_dirty_events;const auto occurrence=p->scratch->city_dirty_next_occurrence;
  need(events.size()<events.max_size()&&occurrence>0&&occurrence<UINT64_MAX&&p->scratch->ui_source_revision<UINT64_MAX,"City strength dirty occurrence/storage/revision exhausted");
  std::uint64_t previous=0;for(const auto& event:events){need(event.generation==p->generation&&event.occurrence>previous&&event.occurrence<occurrence,"City strength dirty event order/generation differs");previous=event.occurrence;}
  events.push_back({occurrence,p->generation,owner,q.id,-1,-1,BnwCityDirtyKind::city_info,true});
  saves::CityCoreField* destination=nullptr;for(auto& city:p->scratch->world.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities)if(&city==q.raw)destination=&named(city.core.fields,"m_iStrengthValue");
  need(destination&&destination->values.size()==1&&destination->values[0]==q.before,"City strength actual destination changed");
  // All allocations, including successor semantic baseline, precede stores.
  // Prepare its exact delta in private metadata, without mutating world to
  // manufacture a capture or caller-provided freshness witness.
  auto expected=p->scratch_expected;
  const auto old_inputs=expected.inputs;
  const auto old_strength=destination->values[0];
  // capture() records city strength at a known private offset, obtained below
  // by a source-bound semantic locator, never by matching an arbitrary value.
  const auto strength_offset=minor_city_strength_seal_offset(*p->scratch,owner,q.id);
  need(strength_offset<expected.inputs.size()&&expected.inputs[strength_offset]==old_strength,"City scratch strength semantic position changed");
  expected.inputs[strength_offset]=q.after;expected.events=events;expected.next=occurrence+1;++expected.revision;
  p->check();need(old_inputs==p->scratch_expected.inputs&&destination->values[0]==q.before,"City scratch semantic inputs changed during preparation");
  const BnwMinorCityStrengthUpdate result{owner,q.id,q.before,q.after,q.known,q.plot,occurrence};
  destination->values[0]=q.after;
  p->scratch->city_dirty_events.swap(events);p->scratch->city_dirty_next_occurrence=occurrence+1;++p->scratch->ui_source_revision;
  static_assert(std::is_nothrow_move_assignable_v<Semantic>);
  p->scratch_expected=std::move(expected);return result;
 }catch(...){p->retired.store(true);throw;}
}
std::optional<BnwTechFirstPlayerScalarPrefix> BnwMinorCityStrengthScratch::first_player_scalar_prefix(std::int32_t team,std::int32_t technology,std::int32_t change){
 const auto p=impl_;p->creator();if(p->busy){p->retired.store(true);throw BnwMinorCityStrengthUnavailable("Technology first-player prefix reentered");}
 p->busy=true;struct Exit{Impl& p;~Exit(){p.busy=false;}}exit{*p};
 try{
  p->check();need(team>=0&&team<64&&(change==1||change==-1)&&technology>=0,"Technology first-player prefix native arguments unsupported");
  const auto& rules=p->host->technologies();const auto* info=rules.find(static_cast<std::uint64_t>(technology));
  need(rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded()&&info&&!info->base.type.empty(),"Technology first-player prefix actual rule slot unavailable");
  const auto* type=p->host->types().find(info->base.type);
  need(p->host->types().report().all_succeeded()&&!p->host->types().report().hash_collisions&&!p->host->types().report().name_overwrites&&type&&type->table=="Technologies"&&type->id==technology&&type->hash==content::saved_info_type_hash(info->base.type),"Technology first-player prefix active Type/ID differs");
  std::array<std::int32_t,6> modifiers{};
  for(std::size_t i=0;i<modifiers.size();++i){const auto value=info->integers.find(tech_player_columns[i]);need(value!=info->integers.end(),"Technology first-player prefix cached modifier absent");modifiers[i]=integer(value->second);}
  bool found=false;
  const auto query=p->host->database().query_rows({"SELECT Type,FeatureProductionModifier,UnitFortificationModifier,UnitBaseHealModifier,WorkerSpeedModifier,InfluenceSpreadModifier,ExtraVotesPerDiplomat FROM Technologies WHERE ID=?",{std::int64_t(technology)}},[&](const content::RulesRowView& row){
   need(!found&&row.text("Type")==info->base.type,"Technology first-player prefix active SQL Type differs");found=true;
   for(std::size_t i=0;i<modifiers.size();++i)need(row.has_column(tech_player_columns[i])&&integer(row.integer(tech_player_columns[i]))==modifiers[i],"Technology first-player prefix active/cached modifier differs");
  },2);
  need(query.all_succeeded()&&found,"Technology first-player prefix actual SQL query failed");
  auto& state=*p->scratch;const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");
  need(state.world.game_state.layout==bnw&&state.setup.profile==content::TechnologyProfile::bnw&&state.world.pregame->layout==bnw&&state.world.all_players->layout==bnw&&teams.size()==64&&state.setup.players.size()==64&&state.world.all_players->players.size()==64,"Technology first-player prefix complete roster absent");
  std::optional<std::int32_t> actor;
  // Source tests getTeam first, then isEverAlive. Living status is irrelevant.
  for(std::int32_t slot=0;slot<64;++slot){const auto& raw=state.world.all_players->players[static_cast<std::size_t>(slot)];const auto& owned=state.setup.players.at(slot);
   need(raw.index==static_cast<std::uint64_t>(slot)&&raw.saved_player_id==slot&&raw.core.layout==bnw&&player_scalar(raw,"m_eID")==slot&&teams[static_cast<std::size_t>(slot)]==owned.team,"Technology first-player prefix actual Pregame/member identity differs");
   if(owned.team!=team)continue;
   const auto ever=player_scalar(raw,"m_bEverAlive",saves::PlayerCoreEncoding::boolean);need((ever==0||ever==1)&&owned.ever_alive==(ever!=0),"Technology first-player prefix actual ever-alive differs");
   if(ever){actor=slot;break;}
  }
  if(!actor){p->check();return std::nullopt;} // No prefix; no cursor/processTech advancement.
  BnwTechFirstPlayerScalarPrefix result;result.team=team;result.technology=technology;result.player=*actor;result.change=change;
  auto& player=state.world.all_players->players[static_cast<std::size_t>(*actor)];std::array<saves::PlayerCoreField*,6> destinations{};
  for(std::size_t i=0;i<destinations.size();++i){
   result.before[i]=player_scalar(player,tech_player_members[i]);result.delta[i]=mul(modifiers[i],change);result.after[i]=add(result.before[i],result.delta[i]);
   destinations[i]=&named(player.core.fields,tech_player_members[i]);need(destinations[i]->hashes.empty(),"Technology first-player scalar carries unsupported hashes");
  }
  need(state.ui_source_revision<UINT64_MAX,"Technology first-player prefix private revision exhausted");
  auto expected=p->scratch_expected;ModifierLocator locator;locator.player=*actor;InputCount count;semantic_inputs(state,count,std::nullopt,nullptr,&locator);
  need(locator.found,"Technology first-player prefix semantic destinations absent");
  for(std::size_t i=0;i<destinations.size();++i){const auto position=locator.positions[i];need(position<expected.inputs.size()&&expected.inputs[position]==result.before[i],"Technology first-player prefix semantic destination differs");expected.inputs[position]=result.after[i];}
  ++expected.revision;
  // Every query/allocation and all six checked adds finish before this final
  // SAME source/scratch/rules bookend. After it: no allocation or callbacks.
  p->check();
  for(std::size_t i=0;i<destinations.size();++i)need(destinations[i]->values[0]==result.before[i],"Technology first-player prefix destination changed during preparation");
  for(std::size_t i=0;i<destinations.size();++i)destinations[i]->values[0]=result.after[i];
  ++state.ui_source_revision;static_assert(std::is_nothrow_move_assignable_v<Semantic>);p->scratch_expected=std::move(expected);
  // Source next executes this player's free-promotion loop and traits. This
  // isolated scalar interval neither supplies those nor visits another actor.
  return result;
 }catch(...){p->retired.store(true);throw;}
}
std::array<std::int32_t,6> BnwMinorCityStrengthScratch::inspect_player_scalar_prefix(std::int32_t player)const{
 const auto p=impl_;p->creator();need(!p->busy&&player>=0&&player<64,"Technology first-player diagnostic actor/reentry unsupported");
 need(same_semantics(*p->scratch,p->scratch_expected),"Technology first-player forensic semantic state differs");std::array<std::int32_t,6> values{};
 const auto& raw=p->scratch->world.all_players->players.at(static_cast<std::size_t>(player));
 for(std::size_t i=0;i<values.size();++i){values[i]=player_scalar(raw,tech_player_members[i]);}
 return values;
}

BnwMinorCityStrengthScratchRead BnwMinorCityStrengthScratch::inspect(std::int32_t owner,std::int32_t handle)const{
 const auto p=impl_;p->creator();need(!p->busy,"City scratch diagnostic called during active preparation");
 // Private forensic data, not a live/source admission. This remains readable
 // after retirement to verify that refused effects left no partial writes.
 need(same_semantics(*p->scratch,p->scratch_expected),"City scratch forensic semantic state differs");
 const auto* city=resolve_bnw_city_registry(p->scratch->world,p->scratch->cities,owner,handle);need(city,"City scratch diagnostic city absent");
 return {city_scalar(*city,"m_iStrengthValue"),p->scratch->city_dirty_events,p->scratch->city_dirty_next_occurrence,p->scratch->ui_source_revision,technology_values(*p->scratch),p->scratch->random.gameplay.state(),p->scratch->random.map.state()};
}
}

#include "bnw_attrition_garrison_supplier.inc"

#include "bnw_attrition_city_strength_supplier.inc"
