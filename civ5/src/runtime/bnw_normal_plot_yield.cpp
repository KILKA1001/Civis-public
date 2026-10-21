#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include "civ5/runtime/bnw_displayed_plot_yield.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_unowned_plot_yield.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/plot_religion.hpp"
#include "civ5/runtime/improvement_yield_calculation.hpp"
#include "civ5/runtime/bnw_happiness_policies.hpp"
#include <algorithm>
#include <climits>
#include <set>
#include <thread>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw BnwNormalPlotYieldUnavailable(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Normal plot yield native intermediate overflow");return static_cast<std::int32_t>(n);}
std::int32_t coordinate(std::uint64_t n){need(n<=INT32_MAX,"Normal plot yield native coordinate overflow");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(std::int64_t(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return native(std::int64_t(a)*b);}
template<class T>const T& named(const std::vector<T>& v,std::string_view name){
 const T* found=nullptr;for(const auto& x:v)if(x.name&&std::string_view(x.name)==name){need(!found,"Normal plot yield duplicate raw field");found=&x;}need(found,"Normal plot yield raw field absent");return *found;
}
template<class T>std::int32_t scalar(const std::vector<T>& v,std::string_view name){const auto& a=named(v,name).values;need(a.size()==1,"Normal plot yield scalar shape");return native(a[0]);}
template<class T>std::int32_t vector_value(const std::vector<T>& v,std::string_view name,std::int32_t y){const auto& a=named(v,name).values;need(a.size()==6,"Normal plot yield native six-vector absent");for(const auto n:a)(void)native(n);return native(a.at(static_cast<std::size_t>(y)));}

std::int64_t sparse(const YieldValues& v,std::int64_t y){const auto it=v.find(y);return it==v.end()?0:it->second;}
const saves::SavedPlayerObservation& player(const LiveSessionState& s,std::int32_t owner){
 need(owner>=0&&owner<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64,"Normal plot yield actual player registry absent");
 const auto& p=s.world.all_players->players.at(static_cast<std::size_t>(owner));need(p.saved_player_id==owner&&p.core.layout==bnw,"Normal plot yield raw player identity differs");return p;
}
void bind_map(const LiveSessionState& s){
 need(s.world.map_plots&&s.world.map_projection&&s.world.map_header.layout==bnw&&s.world.map_plots->layout==bnw,"Normal plot yield map source absent");
 need(coordinate(s.world.map_header.grid.width)>0&&coordinate(s.world.map_header.grid.height)>0,"Normal plot yield native map dimensions invalid");
 const auto raw=saves::project_saved_map(s.world.map_header,*s.world.map_plots);const auto& live=*s.world.map_projection;
 need(raw.grid.width==live.grid.width&&raw.grid.height==live.grid.height&&raw.wrap_x==live.wrap_x&&raw.wrap_y==live.wrap_y&&raw.cells.size()==live.cells.size(),"Normal plot yield raw/owned geometry stale");
 for(std::size_t i=0;i<raw.cells.size();++i){const auto& a=raw.cells[i];const auto& b=live.cells[i];
  need(a.index==b.index&&a.x==b.x&&a.y==b.y&&a.owner==b.owner&&a.plot_type==b.plot_type&&a.terrain_type==b.terrain_type&&a.feature_type==b.feature_type&&a.feature_type_hash==b.feature_type_hash&&a.resource_type==b.resource_type&&a.resource_type_hash==b.resource_type_hash&&a.force_resource_reveal==b.force_resource_reveal&&a.improvement_type==b.improvement_type&&a.improvement_type_hash==b.improvement_type_hash&&a.improvement_pillaged==b.improvement_pillaged&&a.artifact_era==b.artifact_era&&a.river_crossing_bits==b.river_crossing_bits&&a.route==b.route&&a.landmass==b.landmass&&a.rivers==b.rivers&&a.plot_city==b.plot_city&&a.yield_city==b.yield_city&&a.potential_city_work==b.potential_city_work,"Normal plot yield raw/owned map operands stale");
 }
}
const saves::SavedCityObservation* city(const LiveSessionState& s,const std::optional<saves::SavedPlotCityReference>& ref){
 need(ref.has_value(),"Normal plot yield city reference undecoded");const auto* raw=resolve_bnw_current_city(s,native(ref->owner),native(ref->id));if(!raw)return nullptr;
 const auto id=scalar(raw->core.fields,"m_iID"); // Registry resolves native index-only handles to the current full ID.
 const auto& live=s.cities.players.at(ref->owner).cities.at(id);
 need(scalar(raw->core.fields,"m_eOwner")==ref->owner&&live.owner==ref->owner&&live.id==id&&live.x==scalar(raw->core.fields,"m_iX")&&live.y==scalar(raw->core.fields,"m_iY")&&live.population==scalar(raw->core.fields,"m_iPopulation"),"Normal plot yield raw/owned city operands stale");
 const auto& statuses=named(named(raw->tail.blocks,"religions").children,"m_ReligionStatus").children;need(statuses.size()==live.religions.size(),"Normal plot yield religion statuses stale");
 std::int32_t followers=0;
 for(std::size_t i=0;i<statuses.size();++i){const auto& a=statuses[i].children;const auto& b=live.religions[i];const auto n=scalar(a,"m_iFollowers");need(n>=0&&n<=INT32_MAX/2&&b.religion==scalar(a,"m_eReligion")&&b.followers==n&&b.pressure==scalar(a,"m_iPressure"),"Normal plot yield raw/owned religion operands stale or native majority overflow");followers=add(followers,n);}
 (void)followers;return raw;
}
std::int32_t extra_surface(const saves::SavedCityObservation& c,const content::InfoTypeCatalog& types,std::string_view field,std::string_view table,std::int32_t surface,std::int32_t y){
 const auto& matrix=named(named(c.production.blocks,"before_strategy").children,field);
 need(matrix.children.size()<=UINT32_MAX&&named(matrix.fields,"wire_count").value==static_cast<std::int64_t>(matrix.children.size()),"Normal plot yield matrix raw count stale or outside native width");const auto& rows=matrix.children;std::int32_t result=0;
 for(const auto& row:rows){const auto hash=named(row.fields,"hash").value;need(hash>=0&&hash<=UINT32_MAX,"Normal plot yield matrix hash width");if(!hash){need(row.arrays.empty(),"Normal plot yield null hash payload");continue;}
  const auto& a=named(row.arrays,"values");need(a.columns==6&&a.values.size()==6,"Normal plot yield matrix six-vector absent");const auto* t=types.find_hash(static_cast<std::uint64_t>(hash));need(!t||t->table==table,"Normal plot yield surface matrix hash has incompatible Info identity");for(const auto n:a.values)(void)native(n);
  if(t&&t->table==table&&t->id==surface)result=native(a.values.at(static_cast<std::size_t>(y))); // native remapping: last assignment wins
 }return result;
}
std::int32_t feature_type(const saves::SavedMapCellProjection& cell,const content::InfoTypeCatalog& types){
 need(!(cell.feature_type&&cell.feature_type_hash),"Normal plot yield ambiguous feature identity");
 if(cell.feature_type){const auto id=native(*cell.feature_type);need(id>=-1,"Normal plot yield negative feature outside native enum");return id;}
 need(cell.feature_type_hash&&*cell.feature_type_hash<=UINT32_MAX,"Normal plot yield feature hash absent or outside native width");
 if(!*cell.feature_type_hash)return -1;
 const auto* info=types.find_hash(*cell.feature_type_hash);
 need(info&&info->table=="Features","Normal plot yield actual feature hash Info unavailable or incompatible");return native(info->id);
}
std::int32_t resource_type(const saves::SavedMapCellProjection& cell,const content::InfoTypeCatalog& types){
 need(!(cell.resource_type&&cell.resource_type_hash),"Normal plot yield ambiguous resource identity");
 if(cell.resource_type){const auto id=native(*cell.resource_type);need(id>=-1,"Normal plot yield negative resource outside native enum");return id;}
 need(cell.resource_type_hash&&*cell.resource_type_hash<=UINT32_MAX,"Normal plot yield resource hash absent or outside native width");
 if(!*cell.resource_type_hash)return -1;
 const auto* info=types.find_hash(*cell.resource_type_hash);need(info&&info->table=="Resources","Normal plot yield actual resource hash Info unavailable or incompatible");return native(info->id);
}
std::int32_t improvement_type(const saves::SavedMapCellProjection& cell,const content::InfoTypeCatalog& types){
 need(!(cell.improvement_type&&cell.improvement_type_hash),"Normal plot yield ambiguous improvement identity");
 if(cell.improvement_type){const auto id=native(*cell.improvement_type);need(id>=-1,"Normal plot yield negative improvement enum");return id;}
 need(cell.improvement_type_hash&&*cell.improvement_type_hash<=UINT32_MAX,"Normal plot yield improvement hash native width");
 if(!*cell.improvement_type_hash)return -1;
 const auto* info=types.find_hash(*cell.improvement_type_hash);need(info&&info->table=="Improvements","Normal plot yield improvement hash Info identity unavailable or incompatible");return native(info->id);
}
// Authenticate inputs BEFORE the existing remapper reads them. This is not
// another improvement formula or a reconstructed technology/policy cache.
void improvement_matrix_inputs(const LiveSessionState& s,const ImprovementYieldRules& rules,const content::InfoTypeCatalog& types){
 need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64&&s.cities.yield_slots==6,"Normal improvement native registry absent");
 std::set<std::int32_t> seen;
 for(const auto& raw:s.world.teams->teams){const auto id=native(named(raw.fields,"id").value);need(id>=0&&id<64&&raw.index==static_cast<std::uint64_t>(id)&&seen.insert(id).second,"Normal improvement raw team identity differs");
  for(const auto name:{"improvement_yields","improvement_dry_yields","improvement_fresh_yields"}){const auto& rows=named(raw.hashes,name);
   need(rows.rows.size()<=INT32_MAX&&rows.wire_count>=0&&rows.wire_count<=INT32_MAX&&rows.wire_count==static_cast<std::int64_t>(rows.rows.size()),"Normal improvement raw team count differs");
   for(const auto& row:rows.rows){need(row.type_hash<=UINT32_MAX,"Normal improvement team hash native width");if(!row.type_hash){need(row.values.empty(),"Normal improvement null hash payload");continue;}
    const auto* type=types.find_hash(row.type_hash);need(type&&type->table=="Improvements"&&rules.improvements.contains(type->id)&&row.values.size()==6,"Normal improvement team hash/row Info differs");for(const auto n:row.values)(void)native(n);
   }
  }
 }
 const auto matrix=[&](const auto& raw){need(raw.columns==6&&rules.improvement_slots<=SIZE_MAX/6&&raw.values.size()==static_cast<std::size_t>(rules.improvement_slots)*6,"Normal improvement native matrix dimensions differ");for(const auto n:raw.values)(void)native(n);};
 for(std::int32_t id=0;id<64;++id){const auto& raw=player(s,id);need(scalar(raw.core.fields,"m_eID")==id&&raw.traits.layout==bnw&&raw.administration.layout==bnw,"Normal improvement raw player identity differs");
  matrix(named(raw.traits.fields,"m_ppaaiImprovementYieldChange"));matrix(named(named(raw.administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange"));
  (void)scalar(raw.core.fields,"m_iSecondReligionPantheonCount");
  for(const auto& c:raw.cities.cities){const auto full=scalar(c.core.fields,"m_iID");(void)city(s,saves::SavedPlotCityReference{id,full});(void)scalar(c.core.fields,"m_iWeLoveTheKingDayCounter");}
 }
}
std::int64_t matrix_value(const ImprovementYieldMatrix& m,std::int32_t row,std::int32_t y){const auto it=m.find(row);return it==m.end()?0:sparse(it->second,y);}
struct NormalCulturePolicy {std::int32_t numeric=0;std::map<std::int32_t,std::int32_t> improvements;};
struct NormalCultureMembership {std::vector<bool> owned,blocked;};
NormalCultureMembership culture_membership(const LiveSessionState& s,std::int32_t owner,const BnwPolicyHappinessRules& rules,const content::InfoTypeCatalog& types,const std::set<std::int32_t>& branches){
 const auto& raw=player(s,owner);need(raw.index==static_cast<std::uint64_t>(owner)&&raw.strategy.layout==bnw,"Normal improvement culture actual policy owner differs");
 const auto& block=named(raw.strategy.blocks,"policies");const auto version=named(block.fields,"version").value;need(version==1||version==2,"Normal improvement culture policy version unavailable");
 const auto check=[&](const saves::TeamHashedArray& wire,std::string_view table){
  need(wire.rows.size()<=UINT32_MAX&&wire.wire_count>=0&&wire.wire_count<=UINT32_MAX&&wire.wire_count==static_cast<std::int64_t>(wire.rows.size()),"Normal improvement culture policy wire count invalid");
  for(const auto& row:wire.rows){need(row.type_hash<=UINT32_MAX,"Normal improvement culture policy hash native width");if(!row.type_hash){need(row.values.empty(),"Normal improvement culture null policy payload");continue;}
   need(row.values.size()==1&&(row.values[0]==0||row.values[0]==1),"Normal improvement culture native policy boolean invalid");
   const auto* info=types.find_hash(row.type_hash);need(info&&info->table==table&&info->id>=0&&info->id<=INT32_MAX,"Normal improvement culture policy Info identity differs");
   need(table=="Policies"?rules.policies().contains(info->id):branches.contains(static_cast<std::int32_t>(info->id)),"Normal improvement culture policy native slot unavailable");
  }
 };
 const auto& has=named(block.hashes,"has_policy");const auto& blocked=named(block.hashes,"branch_blocked");check(has,"Policies");check(blocked,"PolicyBranchTypes");
 NormalCultureMembership result{rules.flags(has,false),rules.flags(blocked,true)};
 std::set<std::int64_t> membership;for(std::size_t slot=0;slot<result.owned.size();++slot)if(result.owned[slot]){need(slot<=INT32_MAX,"Normal improvement culture owned policy slot overflow");membership.insert(static_cast<std::int64_t>(slot));}
 const auto& current=s.setup.players.at(owner);need(current.policies_resolved&&current.policies==membership,"Normal improvement culture current policy mirror unavailable or stale");return result;
}
bool culture_policy_active(const NormalCultureMembership& membership,const BnwPolicyHappinessRules::Policy& policy,std::int32_t id){
 if(!membership.owned.at(static_cast<std::size_t>(id)))return false;
 const auto branch=native(policy.blocked_branch);need(branch>=-1,"Normal improvement culture blocked branch outside native enum");
 return branch==-1||!membership.blocked.at(static_cast<std::size_t>(branch));
}
std::int32_t culture_policy_numeric(const NormalCultureMembership& membership,const BnwPolicyHappinessRules& binding,const std::map<std::int32_t,NormalCulturePolicy>& metadata){
 std::int32_t result=0;for(const auto& [id,policy]:binding.policies())if(culture_policy_active(membership,policy,native(id)))result=add(result,metadata.at(native(id)).numeric);return result;
}
std::int32_t culture_policy_improvement(const NormalCultureMembership& membership,const BnwPolicyHappinessRules& binding,const std::map<std::int32_t,NormalCulturePolicy>& metadata,std::int32_t improvement){
 std::int32_t result=0;for(const auto& [id,policy]:binding.policies())if(culture_policy_active(membership,policy,native(id))){const auto& rows=metadata.at(native(id)).improvements;const auto row=rows.find(improvement);if(row!=rows.end())result=add(result,row->second);}return result;
}
std::int32_t culture_same_improvement(const LiveSessionState& s,const content::InfoTypeCatalog& types,const ImprovementYieldRules& rules,std::int32_t plot,std::int32_t improvement,std::int32_t adjacency){
 const auto& map=*s.world.map_projection;const auto& cell=map.cells.at(static_cast<std::size_t>(plot));const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};
 std::int32_t result=0;
 for(std::int32_t d=0;d<6;++d){
  if(const auto n=geometry.direction(coordinate(cell.x),coordinate(cell.y),d)){
   const auto id=improvement_type(map.cells.at(static_cast<std::size_t>(*n)),types);need(id==-1||rules.improvements.contains(id),"Normal improvement culture neighbor Improvement Info unavailable");
   if(id==improvement)result=add(result,adjacency); // SOURCE six slots; neither owner nor neighbor pillage filters this getter.
  }
 }
 return result;
}
void improvement_native_proof(const LiveSessionState& s,const ImprovementYieldContext& c,std::int32_t plot,std::int32_t improvement,std::int32_t y,std::int32_t owner,std::int32_t team,std::int32_t visible_resource){
 const auto& map=c.water.map;const auto& cell=map.cells.at(static_cast<std::size_t>(plot));const auto& info=c.rules.improvements.at(improvement);
 // SDK directionXY subtracts native hex coordinates before its wrap. This
 // conservative geometry bound covers every reached subtraction/addition.
 need(map.grid.width+map.grid.height<=INT32_MAX/2,"Normal improvement native hex geometry bounds unavailable");
 need(cell.plot_type&&*cell.plot_type>=0&&*cell.plot_type<=3&&cell.landmass,"Normal improvement native target operands invalid");(void)native(*cell.landmass);
 std::int64_t absolute=0;const auto bound=[&](std::int64_t n){(void)native(n);absolute+=n<0?-n:n;need(absolute<=INT32_MAX,"Normal improvement native additive bounds unavailable");};
 const auto term=[&](ImprovementYieldKind kind){return sparse(info.yields.at(static_cast<std::size_t>(kind)),y);};
 bound(term(ImprovementYieldKind::base));
 const auto per=native(term(ImprovementYieldKind::per_era));
 if(owner!=-1&&per>0){need(cell.artifact_era.has_value(),"Normal improvement artifact era absent");const auto ancient=native(*cell.artifact_era);need(ancient>=-1,"Normal improvement artifact era outside native enum");
  const saves::TeamObservation* raw=nullptr;for(const auto& t:s.world.teams->teams)if(named(t.fields,"id").value==team)raw=&t;need(raw,"Normal improvement current era team absent");
  const auto era=native(named(raw->fields,"era").value);bool actual_era=false,actual_ancient=ancient==-1;for(const auto& [name,t]:c.water.types.entries()){(void)name;if(t.table=="Eras"){if(t.id==era)actual_era=true;if(t.id==ancient)actual_ancient=true;}}
  need(actual_era&&actual_ancient&&s.maintenance.team_eras.at(team)==era,"Normal improvement raw/owned current team era stale");
  // Native subtraction executes even for NO_ERA, before its sentinel branch.
  const auto difference=native(std::int64_t(era)-ancient);if(ancient!=-1&&difference>0)bound(mul(difference,per));
 }
 need(cell.river_crossing_bits&&*cell.river_crossing_bits>=INT8_MIN&&*cell.river_crossing_bits<=INT8_MAX,"Normal improvement native river byte absent or invalid");
 bound(term(ImprovementYieldKind::river));bound(term(ImprovementYieldKind::coastal));bound(term(ImprovementYieldKind::hills));
 const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};
 const auto adjacent_city=native(term(ImprovementYieldKind::adjacent_city)),mountain=native(term(ImprovementYieldKind::adjacent_mountain));
 for(std::int32_t d=0;d<6;++d)if(const auto n=geometry.direction(coordinate(cell.x),coordinate(cell.y),d)){
  const auto& other=map.cells.at(static_cast<std::size_t>(*n));need(other.plot_type&&other.owner&&other.landmass,"Normal improvement neighbor operands absent");
  const auto kind=native(*other.plot_type),other_owner=native(*other.owner);need(kind>=0&&kind<=3&&other_owner>=-1&&other_owner<64,"Normal improvement neighbor native enum invalid");(void)native(*other.landmass);
  const auto feature=feature_type(other,c.water.types);need(feature==-1||c.water.rules.features.contains(feature),"Normal improvement neighbor Feature Info absent");
  if(adjacent_city>0||mountain>0){(void)city(s,other.plot_city);bound(adjacent_city);bound(mountain);} // Six SOURCE direction slots, including wrapped duplicates.
 }
 for(const auto& [id,landmass]:c.water.state.landmasses){(void)native(id);(void)native(landmass.tiles);}
 for(const auto name:{"MIN_WATER_SIZE_FOR_OCEAN","LAKE_MAX_AREA_SIZE"}){const auto* define=c.water.defines.find(name);need(define&&define->lookup.found&&define->lookup.error.empty()&&std::holds_alternative<std::int64_t>(define->lookup.value),"Normal improvement water define unavailable");(void)native(std::get<std::int64_t>(define->lookup.value));}
 const auto route=native(cell.route->type);need(route>=-1&&(route==-1||c.rules.routes.contains(route)),"Normal improvement reached Route Info unavailable");
 if(route!=-1)bound(matrix_value(info.routes,route,y)); // Improvement route term deliberately ignores route pillage.
 if(owner==-1){
  // SDK7194..7211: EVERY native Tech slot, base THEN selected wet/dry,
  // followed by EVERY non-null Policy Info. Sparse omitted entries were
  // authenticated as zero-filled native array slots during rules capture.
  // Absolute prefix bounds conservatively reject cancellation BEFORE the
  // wide helper; owned caches and per-era values never enter neutral math.
  const bool fresh=plot_is_fresh_water(c.water,plot);
  const auto& water_tech=fresh?info.fresh_technologies:info.dry_technologies;
  for(std::int32_t id=0;std::uint64_t(id)<c.rules.technology_slots;++id){bound(matrix_value(info.technologies,id,y));bound(matrix_value(water_tech,id,y));}
  for(const auto& [id,policy]:c.rules.policies){need(id>=0&&std::uint64_t(id)<c.rules.policy_slots,"Normal neutral improvement Policy slot unavailable");bound(matrix_value(policy,improvement,y));}
 }else{
  const auto& bonuses=c.state.players.at(owner);bound(matrix_value(bonuses.own,improvement,y));bound(matrix_value(bonuses.traits,improvement,y));
  const auto& team_values=c.state.teams.at(team);bound(matrix_value(team_values[0],improvement,y));
  // Both candidate source caches conservatively bound the selected freshwater
  // branch; the existing water/helper decides which one contributes.
  bound(matrix_value(team_values[1],improvement,y));bound(matrix_value(team_values[2],improvement,y));
  if(visible_resource!=-1)bound(matrix_value(info.resources,visible_resource,y));
 }
 if(const auto* working=resolve_bnw_city_yield_registry(s.world,c.water.cities,native(cell.yield_city->owner),native(cell.yield_city->id)))
  for(const auto* belief:bnw_plot_beliefs(c.water.cities,*working,c.religion->beliefs))bound(matrix_value(belief->improvements,improvement,y));
}
const TeamTechnologies& team_techs(const LiveSessionState& s,std::int32_t team,std::uint64_t slots){
 const TeamTechnologies* found=nullptr;for(const auto& t:s.technologies)if(t.team()==team){need(!found&&t.profile()==content::TechnologyProfile::bnw&&t.slots()==slots,"Normal plot yield current technology team/slots ambiguous");found=&t;}
 need(found,"Normal plot yield current technology team absent");return *found;
}
ResourceVisibilityState bind_resource_visibility(const LiveSessionState& s,const ResourceVisibilityRules& rules,const content::TechnologyRules& tech_rules,const content::InfoTypeCatalog& types,ResearchContext& setup,std::int32_t team,std::int32_t reveal){
 need(team>=0&&team<64&&s.world.pregame&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64&&setup.players.size()==64,"Normal plot yield current resource roster absent");
 const auto& teams=s.world.pregame->archive.integer_arrays.at("team_types");need(teams.size()==64,"Normal plot yield resource team assignments width differs");
 for(std::int32_t id=0;id<64;++id){const auto& raw=player(s,id);const auto alive=scalar(raw.core.fields,"m_bAlive");const auto raw_team=native(teams.at(static_cast<std::size_t>(id)));const auto& owned=setup.players.at(id);
  need(scalar(raw.core.fields,"m_eID")==id&&(alive==0||alive==1)&&raw_team>=-1&&raw_team<64&&(alive==0||raw_team>=0)&&owned.alive==(alive==1)&&owned.team==raw_team,"Normal plot yield resource raw/owned alive/team stale");
  const auto& policies=named(named(raw.strategy.blocks,"policies").hashes,"has_policy");
  need(policies.wire_count>=0&&policies.wire_count<=UINT32_MAX&&policies.wire_count==static_cast<std::int64_t>(policies.rows.size()),"Normal plot yield policy raw count differs");
  for(const auto& row:policies.rows){need(row.type_hash<=UINT32_MAX,"Normal plot yield policy hash native width differs");if(!row.type_hash){need(row.values.empty(),"Normal plot yield null policy hash payload");continue;}const auto* type=types.find_hash(row.type_hash);need(type&&type->table=="Policies"&&type->id>=0&&static_cast<std::uint64_t>(type->id)<rules.policy_slots,"Normal plot yield policy hash Info identity unavailable or incompatible");
   need(row.values.size()==1&&(row.values[0]==0||row.values[0]==1),"Normal plot yield native policy boolean invalid"); // CvPolicyClasses m_pabHasPolicy is bool[], not a byte-valued grant.
  }
 }
 const saves::TeamObservation* actual_team=nullptr;
 std::set<std::int32_t> seen;
 for(const auto& raw:s.world.teams->teams){const auto id=native(named(raw.fields,"id").value);need(id>=0&&id<64&&raw.index==static_cast<std::uint64_t>(id)&&seen.insert(id).second&&setup.team_members.contains(id),"Normal plot yield resource raw team registry differs");
  const auto& forced=named(raw.arrays,"revealed_resources");need(named(raw.fields,"revealed_resources_wire_count").value>=0&&named(raw.fields,"revealed_resources_wire_count").value<=UINT32_MAX&&named(raw.fields,"revealed_resources_wire_count").value==static_cast<std::int64_t>(forced.values.size()),"Normal plot yield forced-resource raw count differs");
  for(const auto hash:forced.values){need(hash>=0&&hash<=UINT32_MAX,"Normal plot yield forced resource native hash differs");if(!hash)continue;const auto* type=types.find_hash(static_cast<std::uint64_t>(hash));need(type&&type->table=="Resources"&&rules.resources.contains(type->id),"Normal plot yield forced resource Info identity differs");}
  if(id==team)actual_team=&raw;
 }
 need(actual_team,"Normal plot yield reached resource team absent");
 auto state=restore_resource_visibility(s.world,setup,rules,types);
 for(const auto& [id,current]:s.setup.players)if(current.policies_resolved)need(setup.players.at(id).policies==current.policies,"Normal plot yield current raw/owned policy mirror stale");
 // Reuse actual source remapping; only the reached HasTech flag is relevant.
 // Unknown source tech hashes retain the SDK's ignored-row semantics. Known
 // hashes from another Info table cannot be interpreted as Technology slots.
 for(const auto hash:named(actual_team->arrays,"tech_hashes").values){need(hash>=0&&hash<=UINT32_MAX,"Normal plot yield team technology hash native width differs");const auto* type=hash?types.find_hash(static_cast<std::uint64_t>(hash)):nullptr;need(!type||type->table=="Technologies","Normal plot yield team technology hash Info table differs");}
 const auto decoded=restore_team_technologies(*actual_team,tech_rules,types);const auto& owned=team_techs(s,team,tech_rules.collection.slots);
 need(decoded.has_tech(reveal)==owned.has_tech(reveal),"Normal plot yield current raw/owned resource HasTech stale");return state;
}
template<class F>auto typed(F&& f)->decltype(f()){try{return f();}catch(const BnwNormalPlotYieldUnavailable&){throw;}catch(const std::exception& e){throw BnwNormalPlotYieldUnavailable(e.what());}}
}
struct BnwNormalPlotYieldRules::Impl {
 struct Yield {std::int32_t city=0,offset=0,divisor=0,min=0,golden_threshold=0,golden=0;};
 std::thread::id thread=std::this_thread::get_id();std::string sha;std::function<bool()> current;
 content::InfoTypeCatalog types;content::GlobalDefineCache defines;
 BnwNatureYieldRules nature;PlotWaterRules water;std::array<Yield,6> yields;
 std::map<std::int32_t,std::array<std::int32_t,6>> routes;
 struct Resource {std::int32_t technology=-1,policy=-1,usage=0;};
 ResourceVisibilityRules resource_visibility;content::TechnologyRules technologies;
 std::map<std::int32_t,Resource> resources;
 ImprovementYieldRules improvements;ImprovementReligionRules improvement_religion;
 BnwPolicyHappinessRules policy_binding;std::set<std::int32_t> policy_branches;
 std::map<std::int32_t,NormalCulturePolicy> culture_policies;
 std::map<std::int32_t,std::int32_t> culture_adjacency;
};
BnwNormalPlotYieldRules::BnwNormalPlotYieldRules(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwNormalPlotYieldRules::require_current()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->current&&p->current(),"Normal plot yield SAME rules/thread unavailable");
}
void BnwNormalPlotYieldRules::require_current(std::string_view sha)const{
 require_current();need(!sha.empty()&&impl_->sha==sha,"Normal plot yield rules identity mismatch");
}
BnwNormalPlotYieldRules BnwNormalPlotYieldRules::load(const content::RulesDatabase& db,std::string_view sha){return typed([&]{
 auto p=std::make_shared<Impl>();p->current=db.immutable_preview_fingerprint_currentness();need(p->current&&p->current(),"Normal plot yield requires SAME immutable activated rules");p->sha=db.gameplay_fingerprint_sha256();need(p->sha==sha,"Normal plot yield rules identity mismatch");
 p->types=db.cache_info_types();need(p->types.report().all_succeeded(),"Normal plot yield Info catalog unavailable");p->defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);
 p->nature=load_bnw_nature_yield_rules(db);p->water=load_plot_water_rules(db,content::TechnologyProfile::bnw);
 p->improvements=load_improvement_yield_rules(db,content::TechnologyProfile::bnw);
 p->improvement_religion=load_improvement_religion_rules(db);
 p->policy_binding=BnwPolicyHappinessRules::load(db);need(p->policy_binding.rules_sha256()==p->sha,"Normal improvement culture policy rules differ");
 need(p->improvements.improvement_slots<=INT32_MAX&&p->improvements.technology_slots<=INT32_MAX&&p->improvements.route_slots<=INT32_MAX&&p->improvements.policy_slots<=INT32_MAX&&p->improvements.yields.size()==6,"Normal improvement native rule collections unavailable");
 const auto canonical=[&](std::string_view table,std::int64_t id){(void)native(id);for(const auto& [name,t]:p->types.entries())if(t.table==table&&t.id==id){const auto* wire=p->types.find_hash(t.hash);if(wire&&wire->type==name&&wire->table==table&&wire->id==id)return true;}return false;};
 const auto values=[&](const YieldValues& row){for(const auto& [y,n]:row){need(y>=0&&y<6,"Normal improvement rule yield outside native six slots");(void)native(n);}};
 const auto matrix=[&](const ImprovementYieldMatrix& rows,std::string_view table){for(const auto& [id,row]:rows){need(canonical(table,id),"Normal improvement rule relation Info identity differs");values(row);}};
 const auto improvements_report=db.prefetch_info("Improvements",[&](auto slot,auto authored,const auto& row){need(slot<=INT32_MAX,"Normal improvement native Info slot overflow");const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");const auto* info=type?p->types.find(*type):nullptr;const auto* wire=info?p->types.find_hash(info->hash):nullptr;
  need(type&&info&&info->table=="Improvements"&&info->id==id&&authored==id&&wire&&wire->type==info->type&&wire->table=="Improvements"&&wire->id==id&&p->improvements.improvements.at(id).authored_id==id,"Normal improvement actual slot/hash identity differs");
  need(p->culture_adjacency.emplace(id,native(row.integer("CultureAdjacentSameType"))).second,"Normal improvement duplicate culture adjacency slot");return true;
 });need(improvements_report.all_succeeded()&&improvements_report.slots==p->improvements.improvement_slots,"Normal improvement actual Info collection unavailable");
 for(const auto& [id,info]:p->improvements.improvements){(void)id;for(const auto& row:info.yields)values(row);matrix(info.routes,"Routes");matrix(info.resources,"Resources");matrix(info.technologies,"Technologies");matrix(info.fresh_technologies,"Technologies");matrix(info.dry_technologies,"Technologies");}
 for(const auto& [id,rows]:p->improvements.policies){need(canonical("Policies",id),"Normal improvement Policy Info identity differs");matrix(rows,"Improvements");}
 for(const auto& [id,belief]:p->improvement_religion.beliefs){need(canonical("Beliefs",id),"Normal improvement Belief Info identity differs");matrix(belief.improvements,"Improvements");}
 // Reuse the exact BNW blocked-branch/free-opener binder. Only the two small
 // missing CvPolicyEntry culture metadata getters are loaded here.
 const auto branch_report=db.prefetch_info("PolicyBranchTypes",[&](auto slot,auto authored,const auto& row){need(slot<=INT32_MAX,"Normal improvement culture branch slot overflow");const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");const auto* info=type?p->types.find(*type):nullptr;const auto* wire=info?p->types.find_hash(info->hash):nullptr;
  need(type&&info&&info->table=="PolicyBranchTypes"&&info->id==id&&authored==id&&wire&&wire->type==info->type&&wire->table==info->table&&wire->id==id&&p->policy_branches.insert(id).second,"Normal improvement culture branch slot/hash identity differs");
  const auto opener=row.text("FreePolicy");if(opener&&!opener->empty()){const auto* policy=p->types.find(*opener);need(policy&&policy->table=="Policies"&&canonical("Policies",policy->id)&&p->policy_binding.policies().contains(policy->id),"Normal improvement culture free-opener native identity unavailable");}return true;
 });need(branch_report.all_succeeded()&&branch_report.slots<=INT32_MAX&&branch_report.slots==p->policy_branches.size(),"Normal improvement culture branch rules unavailable");
 const auto policy_report=db.prefetch_info("Policies",[&](auto slot,auto authored,const auto& row){need(slot<=INT32_MAX,"Normal improvement culture policy slot overflow");const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");const auto* info=type?p->types.find(*type):nullptr;const auto* wire=info?p->types.find_hash(info->hash):nullptr;
  need(type&&info&&info->table=="Policies"&&info->id==id&&authored==id&&wire&&wire->type==info->type&&wire->table==info->table&&wire->id==id&&p->policy_binding.policies().contains(id),"Normal improvement culture policy slot/hash identity differs");
  const auto branch=native(p->policy_binding.policies().at(id).blocked_branch);need(branch==-1||p->policy_branches.contains(branch),"Normal improvement culture blocked-branch source binding invalid");
  NormalCulturePolicy policy;policy.numeric=native(row.integer("CultureImprovementChange"));
  const auto rows=db.query_rows({"select Improvements.ID, CultureChange from Policy_ImprovementCultureChanges inner join Improvements on ImprovementType = Improvements.Type where PolicyType = ?",{std::string(*type)}},[&](const auto& linked){const auto improvement=native(linked.integer_at(0));need(p->improvements.improvements.contains(improvement),"Normal improvement culture policy relation Info unavailable");policy.improvements[improvement]=native(linked.integer_at(1));}); // SOURCE last assignment, including zero.
  need(rows.all_succeeded()&&p->culture_policies.emplace(id,std::move(policy)).second,"Normal improvement culture actual policy metadata unavailable");return true;
 });need(policy_report.all_succeeded()&&policy_report.slots<=INT32_MAX&&policy_report.slots==p->policy_binding.policies().size()&&policy_report.slots==p->culture_policies.size(),"Normal improvement culture policy collections differ");
 // Reuse the existing nature loader; authenticate its actual Feature slots
 // and flags against the SAME activated DB and cross-table wire identities.
 const auto feature_report=db.prefetch_info("Features",[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX,"Normal plot yield Feature slot outside native width");const auto id=static_cast<std::int32_t>(slot);
  const auto type=row.text("Type");const auto* info=type?p->types.find(*type):nullptr;
  const auto* wire=info?p->types.find_hash(info->hash):nullptr;
  need(type&&!type->empty()&&authored==id&&info&&info->table=="Features"&&info->id==id&&wire&&wire->type==info->type&&wire->table=="Features"&&wire->id==id,"Normal plot yield actual Feature slot/hash identity differs");
  const auto found=p->nature.features.find(id);need(found!=p->nature.features.end()&&found->second.nonadditive==row.boolean("YieldNotAdditive")&&found->second.natural_wonder==row.boolean("NaturalWonder"),"Normal plot yield SAME nature Feature flags differ");return true;
 });
 need(feature_report.all_succeeded()&&feature_report.slots==p->nature.feature_slots&&feature_report.slots<=INT32_MAX,"Normal plot yield Feature collection unavailable");
 p->resource_visibility=load_resource_visibility_rules(db,p->types,content::TechnologyProfile::bnw);
 need(p->resource_visibility.policy_slots<=INT32_MAX,"Normal plot yield policy collection outside native width");
 // DomainTypes has five native entries in this BNW SDK (including HOVER);
 // reuse the genuine technology loader/remapper, not a sparse rule DTO.
 p->technologies=content::load_technology_rules(db,p->types,content::TechnologyProfile::bnw,5);
 need(p->technologies.all_succeeded()&&p->technologies.collection.slots<=INT32_MAX,"Normal plot yield actual technology rules unavailable");
 // Authenticate finite native Tech array span, including holes. SDK loops
 // allocated slots even for null Tech Info; absent relation entries at
 // those holes stay native zero through the existing sparse loader.
 std::set<std::int32_t> actual_tech;
 const auto neutral_tech_report=db.prefetch_info("Technologies",[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX,"Normal neutral improvement Tech slot overflow");const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");const auto* info=type?p->types.find(*type):nullptr;const auto* wire=info?p->types.find_hash(info->hash):nullptr;
  need(type&&info&&info->table=="Technologies"&&info->id==id&&authored==id&&wire&&wire->type==info->type&&wire->table==info->table&&wire->id==id&&actual_tech.insert(id).second,"Normal neutral improvement Tech slot/hash identity differs");return true;
 });
 need(neutral_tech_report.all_succeeded()&&neutral_tech_report.slots==p->improvements.technology_slots&&neutral_tech_report.slots==p->technologies.collection.slots,"Normal neutral improvement Tech native collection differs");
 for(const auto& [id,info]:p->improvements.improvements){(void)id;
  for(const auto* rows:{&info.technologies,&info.fresh_technologies,&info.dry_technologies})for(const auto& [tech,values]:*rows){(void)values;need(tech>=0&&std::uint64_t(tech)<neutral_tech_report.slots&&actual_tech.contains(native(tech)),"Normal neutral improvement Tech relation outside actual native collection");}
 }
 const auto resource_report=db.prefetch_info("Resources",[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX,"Normal plot yield Resource slot outside native width");const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");const auto* info=type?p->types.find(*type):nullptr;const auto* wire=info?p->types.find_hash(info->hash):nullptr;
  need(type&&!type->empty()&&authored==id&&info&&info->table=="Resources"&&info->id==id&&wire&&wire->type==info->type&&wire->table=="Resources"&&wire->id==id&&p->nature.resources.contains(id),"Normal plot yield actual Resource slot/hash identity differs");
  const auto ref=[&](const char* field,std::string_view table){const auto value=row.text(field);if(!value||value->empty())return std::int32_t{-1};const auto* target=p->types.find(*value);need(target&&target->table==table&&target->id>=0,"Normal plot yield Resource reveal Info reference unavailable or incompatible");return native(target->id);};
  const auto technology=ref("TechReveal","Technologies"),policy=ref("PolicyReveal","Policies");const auto usage=native(row.integer("ResourceUsage"));
  need(usage>=0&&usage<=2&&(technology==-1||p->technologies.find(static_cast<std::uint64_t>(technology)))&&(policy==-1||static_cast<std::uint64_t>(policy)<p->resource_visibility.policy_slots),"Normal plot yield Resource native reveal/usage bounds differ");
  const auto& visibility=p->resource_visibility.resources.at(id);need(visibility.technology==technology&&visibility.policy==policy,"Normal plot yield SAME Resource visibility rules differ");
  need(p->resources.emplace(id,Impl::Resource{technology,policy,usage}).second,"Normal plot yield duplicate Resource slot");return true;
 });
 need(resource_report.all_succeeded()&&resource_report.slots<=INT32_MAX&&resource_report.slots==p->resource_visibility.resources.size(),"Normal plot yield Resource collection unavailable");
 // CvRouteInfo::CacheResults calls SetYields/PopulateArrayByValue: allocate
 // a zero-filled Yields vector first, then assign each joined row in order.
 // The BNW improvement loader deliberately does not populate Route_Yields;
 // that unrelated helper cannot stand in for this separate CvPlot branch.
 const auto route_report=db.prefetch_info("Routes",[&](auto slot,auto authored,const auto& row){
  const auto type=row.text("Type");need(type&&!type->empty(),"Normal plot yield actual Route Type absent");
  need(slot<=INT32_MAX,"Normal plot yield Route slot outside native width");const auto index=static_cast<std::int32_t>(slot);
  const auto* info=p->types.find(*type);
  need(authored==index&&info&&info->table=="Routes"&&info->id==index,"Normal plot yield actual Route slot identity differs");
  std::array<std::int32_t,6> values{};
  const auto rows=db.query_rows({"select Yields.ID, Yield from Route_Yields inner join Yields on YieldType = Yields.Type where RouteType = ?",{std::string(*type)}},[&](const auto& linked){
   const auto yield=native(linked.integer_at(0));need(yield>=0&&yield<6,"Normal plot yield Route_Yields native index differs");
   values.at(static_cast<std::size_t>(yield))=native(linked.integer_at(1)); // source last assignment wins
  });
  need(rows.all_succeeded(),"Normal plot yield actual Route_Yields unavailable");
  need(p->routes.emplace(index,values).second,"Normal plot yield duplicate Route slot");return true;
 });
 need(route_report.all_succeeded()&&route_report.slots<=INT32_MAX,"Normal plot yield Route collection unavailable");
 const auto report=db.prefetch_info("Yields",[&](auto slot,auto,const auto& row){need(slot<6,"Normal plot yield unsupported Info slots");auto& v=p->yields.at(static_cast<std::size_t>(slot));v={native(row.integer("CityChange")),native(row.integer("PopulationChangeOffset")),native(row.integer("PopulationChangeDivisor")),native(row.integer("MinCity")),native(row.integer("GoldenAgeYieldThreshold")),native(row.integer("GoldenAgeYield"))};return true;});
 const char* names[]={"YIELD_FOOD","YIELD_PRODUCTION","YIELD_GOLD","YIELD_SCIENCE","YIELD_CULTURE","YIELD_FAITH"};for(std::int32_t y=0;y<6;++y){const auto* t=p->types.find(names[y]);need(t&&t->table=="Yields"&&t->id==y,"Normal plot yield fixed source YieldTypes differs");}
 need(report.all_succeeded()&&report.slots==6&&p->current()&&db.gameplay_fingerprint_sha256()==p->sha,"Normal plot yield rules retired during load");return BnwNormalPlotYieldRules(std::move(p));
});}
namespace {
// One shared formula. A display selector can only be constructed below from
// the genuine private session; it is never a public readiness/debug flag.
struct DisplaySelection {std::int32_t player=-1,team=-1,improvement=-1,route=-1;};

// Display selectors are source reads, not projection rewrites or cached art.
const saves::PlotObservation& display_plot(const LiveSessionState& s,std::int32_t plot){
 need(plot>=0&&s.world.map_plots&&s.world.map_projection&&s.world.map_header.layout==bnw&&s.world.map_plots->layout==bnw,"Displayed yield current map absent");
 const auto index=static_cast<std::size_t>(plot);const auto& raw=s.world.map_plots->plots.at(index);const auto& owned=s.world.map_projection->cells.at(index);const auto xy=s.world.map_header.grid.coordinates(static_cast<std::uint64_t>(plot));
 need(raw.index==static_cast<std::uint64_t>(plot)&&owned.index==raw.index&&raw.x==xy[0]&&raw.y==xy[1]&&owned.x==raw.x&&owned.y==raw.y,"Displayed yield raw/projected plot identity stale");return raw;
}
std::int32_t debug_plot_yield(const LiveSessionState& s,std::int32_t plot,std::int32_t y){
 const auto& raw=display_plot(s,plot);const auto& cache=named(raw.arrays,"yields");const auto& owned=s.world.map_projection->cells.at(static_cast<std::size_t>(plot));
 need(s.world.map_plots->dimensions.yields==6&&cache.columns==1&&cache.values.size()==6&&owned.cached_yields&&owned.cached_yields->size()==6,"Displayed debug raw/owned short cache unavailable");
 const auto n=cache.values.at(static_cast<std::size_t>(y));need(n>=INT16_MIN&&n<=INT16_MAX&&owned.cached_yields->contains(y)&&owned.cached_yields->at(y)==n,"Displayed debug current raw/owned selected short cache stale");
 return static_cast<std::int32_t>(n); // NO final clamp in native getYield.
}
std::pair<std::int32_t,std::int32_t> display_active_team(const LiveSessionState& s){
 need(s.world.pregame&&s.world.pregame->layout==bnw&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64,"Displayed yield active team registry absent");
 const auto active=native(s.world.pregame->archive.integers.at("active_player"));
 need(active>=0&&active<64,"Displayed yield NO_TEAM cannot index revealed source arrays");const auto& p=player(s,active);need(p.index==static_cast<std::uint64_t>(active)&&scalar(p.core.fields,"m_eID")==active,"Displayed yield active player identity stale");
 const auto& assignments=s.world.pregame->archive.integer_arrays.at("team_types");need(assignments.size()==64,"Displayed yield team assignment width differs");
 const auto team=native(assignments.at(static_cast<std::size_t>(active)));need(team>=0&&team<64&&s.setup.players.at(active).team==team,"Displayed yield current active player/team stale or NO_TEAM");
 const auto& raw=s.world.teams->teams.at(static_cast<std::size_t>(team));need(raw.index==static_cast<std::uint64_t>(team)&&named(raw.fields,"id").value==team,"Displayed yield actual team slot stale");return {active,team};
}
bool city_revealed(const saves::SavedCityObservation& city,std::int32_t team){
 need(team>=0&&team<64,"Displayed city reveal team outside native domain");const auto& reveal=named(city.core.fields,"m_abRevealed");
 need(reveal.encoding==saves::CityCoreEncoding::vector_bool&&reveal.wire_count==80&&reveal.values.size()==80&&reveal.bytes.empty()&&reveal.hashes.empty()&&reveal.row_counts.empty()&&reveal.row_keys.empty(),"Displayed city native reveal array unavailable");
 for(const auto n:reveal.values)need(n==0||n==1,"Displayed city native reveal boolean invalid");return reveal.values.at(static_cast<std::size_t>(team))==1;
}

DisplaySelection display_selection(const LiveSessionState& s,const content::InfoTypeCatalog& types,const ImprovementYieldRules& improvements,std::int32_t plot,std::int32_t active,std::int32_t team){
 const auto& raw=display_plot(s,plot);need(s.world.map_plots->dimensions.teams==80&&team>=0&&team<64&&active>=0&&active<64,"Displayed yield native reveal storage/domain differs");
 const auto element=[&](std::string_view name){const auto& a=named(raw.arrays,name);need(a.columns==1&&a.values.size()==80,"Displayed yield native reveal array shape unavailable");return a.values.at(static_cast<std::size_t>(team));};
 const auto revealed_owner=native(element("revealed_owners"));need(revealed_owner>=-1&&revealed_owner<64,"Displayed yield revealed owner outside native registry");
 DisplaySelection result{revealed_owner==-1?active:revealed_owner,team,-1,-1};
 need(raw.version&&*raw.version>=1&&*raw.version<=7,"Displayed yield source plot version unavailable");
 if(*raw.version>=6){const auto hash=element("revealed_improvement_hashes");need(hash>=0&&hash<=UINT32_MAX,"Displayed yield revealed improvement hash outside native width");if(hash){const auto* info=types.find_hash(static_cast<std::uint64_t>(hash));need(info&&info->table=="Improvements"&&improvements.improvements.contains(info->id),"Displayed yield revealed improvement hashed Info frontier unavailable");result.improvement=native(info->id);}}
 else {const auto id=native(element("revealed_improvements"));need(id>=-1&&id<=INT16_MAX,"Displayed yield revealed improvement outside native short");if(id!=-1&&improvements.improvements.contains(id))result.improvement=id;} // SDK Read: missing old numeric Info resets NO_IMPROVEMENT.
 result.route=native(element("revealed_routes"));need(result.route>=-1&&result.route<=INT16_MAX,"Displayed yield revealed route outside native short");
 const auto& actor=player(s,result.player);need(actor.index==static_cast<std::uint64_t>(result.player)&&scalar(actor.core.fields,"m_eID")==result.player,"Displayed yield selected actor identity stale");return result;
}

std::int32_t calculate_plot_source(const LiveSessionState& s,const saves::SavedSession& source,const SessionCheckpointIdentity& identity,const auto& p,std::int32_t plot,std::int32_t y,bool display,std::int32_t active_player=-1,std::int32_t active_team=-1,std::optional<bool> current_game_debug={}){return typed([&]{need(p&&p->thread==std::this_thread::get_id()&&p->current&&p->current()&&p->sha==identity.rules_sha256,"Normal plot yield SAME rules/thread unavailable"); // BEFORE any world/source borrow
 need(plot>=0&&y>=0&&y<6&&s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw,"Normal plot yield profile/index differs");
 // Preserve the public normal query's established payload/history binding,
 // including SDK early-zero paths. Genuine display queries authenticate their
 // private import owner and keep SDK extras reads at the actual suffix.
 std::optional<BnwGamePlotExtras> extras;
 if(!display&&!current_game_debug)extras.emplace(restore_bnw_game_plot_extras(s,source,identity));
 bind_map(s);auto map=*s.world.map_projection;const auto& cell=map.cells.at(static_cast<std::size_t>(plot));
 if(const auto early=bnw_plot_early_zero_yield(s,plot,y)){need(p->current(),"Normal plot yield rules retired");return native(early->total);}
 need(cell.route.has_value(),"Normal plot yield decoded route absent");
 const auto actual_improvement=improvement_type(cell,p->types);
 std::optional<DisplaySelection> selected;
 if(display)selected=display_selection(s,p->types,p->improvements,plot,active_player,active_team);
 const auto improvement=selected?selected->improvement:actual_improvement;
 bool improvement_pillaged=false;
 if(improvement!=-1){need(p->improvements.improvements.contains(improvement)&&cell.improvement_pillaged.has_value(),"Normal plot yield reached Improvement Info or pillage unavailable");
  const auto flag=named(s.world.map_plots->plots.at(static_cast<std::size_t>(plot)).fields,"improvement_pillaged").value;
  need((flag==0||flag==1)&&*cell.improvement_pillaged==(flag==1),"Normal plot yield native raw improvement pillage flag invalid");improvement_pillaged=flag==1;
 }
 const auto feature=feature_type(cell,p->types);
 const BnwNatureYieldRules::Surface* actual_feature=nullptr;
 if(feature!=-1){const auto found=p->nature.features.find(feature);need(found!=p->nature.features.end(),"Normal plot yield reached Feature Info unavailable");actual_feature=&found->second;}
 const auto resource=resource_type(cell,p->types);if(resource!=-1)need(p->resources.contains(resource),"Normal plot yield reached Resource Info unavailable");
 const auto route_type=selected?selected->route:native(cell.route->type);
 if(route_type!=-1){const auto flag=named(s.world.map_plots->plots.at(static_cast<std::size_t>(plot)).fields,"route_pillaged").value;need((flag==0||flag==1)&&(flag==1)==cell.route->pillaged,"Normal plot yield native raw route pillage flag invalid");}
 need(cell.owner&&cell.plot_type&&cell.terrain_type&&cell.rivers,"Normal plot yield actual plot operands missing");const auto actual_owner=native(*cell.owner),terrain=native(*cell.terrain_type);need(actual_owner>=-1&&actual_owner<64,"Normal plot yield owner outside native registry");
 const auto owner=selected?selected->player:actual_owner;
 const auto* home=owner==-1?nullptr:city(s,cell.plot_city);const auto* working=city(s,cell.yield_city);
 if(home)need(scalar(home->core.fields,"m_eOwner")==actual_owner&&scalar(home->core.fields,"m_iX")==coordinate(cell.x)&&scalar(home->core.fields,"m_iY")==coordinate(cell.y),"Normal plot yield plot-city owner/coordinate binding stale");
 auto cities=s.cities;const auto nature_state=restore_bnw_nature_yields(s.world,p->nature,cities);
 // The optional derived mirror may not yet have been installed by import.
 // This calculation reads the actual current raw counter and authenticates
 // an existing mirror if present; it never invents a missing raw value.
 for(const auto& [id,live]:s.cities.players)if(live.second_pantheon_count)need(live.second_pantheon_count==cities.players.at(id).second_pantheon_count,"Normal plot yield secondary pantheon counter stale");
 need(cities.religions.size()==s.world.religion_culture.religions.size(),"Normal plot yield global religion cache stale");
 for(std::size_t i=0;i<cities.religions.size();++i){const auto& raw=s.world.religion_culture.religions[i];const auto& live=cities.religions[i];std::vector<std::int64_t> beliefs;for(const auto hash:raw.beliefs.belief_hashes){const auto* t=hash?p->types.find_hash(hash):nullptr;need(!t||t->table=="Beliefs","Normal plot yield belief hash has incompatible Info identity");if(t&&t->id>=0&&p->nature.beliefs.contains(t->id))beliefs.push_back(t->id);}need(live.religion==raw.religion&&live.founder==raw.founder&&live.beliefs==beliefs,"Normal plot yield global religion identity stale");}
 // SDK gates city reveal only when the corresponding suffix is reached.
 // Feature/terrain/resource extras use the actual working city without it.
 const auto working_visible=[&]{return working&&(!selected||city_revealed(*working,selected->team));};
 std::int32_t team=-1;
 if(owner!=-1){const auto& raw=player(s,owner);(void)raw;need(s.world.pregame.has_value(),"Normal plot yield current team setup missing");team=native(s.world.pregame->archive.integer_arrays.at("team_types").at(static_cast<std::size_t>(owner)));need(team==s.setup.players.at(owner).team,"Normal plot yield raw/owned team stale");}
 auto setup=s.setup;ResourceVisibilityState resource_state;
 if(resource!=-1&&owner!=-1)need(team>=0&&team<64,"Normal plot yield owned resource has no native team");
 if(resource!=-1&&team!=-1){const auto& force=named(s.world.map_plots->plots.at(static_cast<std::size_t>(plot)).arrays,"resource_force_reveal");need(s.world.map_plots->layout==bnw&&s.world.map_plots->dimensions.teams==80&&force.columns==1&&force.values.size()==80&&cell.force_resource_reveal,"Normal plot yield current native resource force flags absent or wrong width");resource_state=bind_resource_visibility(s,p->resource_visibility,p->technologies,p->types,setup,team,p->resources.at(resource).technology);}
 const auto water_state=restore_plot_water(s.world);need(s.plot_impassability.rules_sha256==p->sha,"Normal plot yield impassability rules differ");const auto& pass=bnw_live_plot_impassability(s);PlotWaterContext water{map,water_state,p->water,p->types,pass,setup,cities,p->defines};
 ResourceVisibilityContext resources{map,resource_state,p->resource_visibility,setup,s.player_research,s.technologies,p->types,p->defines,current_game_debug.value_or(false)};
 std::int32_t visible_resource=-1;
 if(resource!=-1&&team!=-1){visible_resource=native(plot_resource_type(resources,plot,team));if(!selected&&!current_game_debug)need(visible_resource==resource,"Normal plot yield hidden resource requires genuine debug owner join");}
 // Public detached normal math admits only debug-invariant resources.
 // Private normal mode reads the actual Game owner instead: hidden resources
 // legitimately remain NO_RESOURCE under its current false cached field.
 BnwNatureYieldContext nature{p->nature,nature_state,water,resources};nature.current_city_registry=&s.world;
 // SDK6873..7050: native32 proof is issued BEFORE the shared wide helper.
 // Natural wonders also reach this prefix on impassable/mountain plots;
 // terrain still executes before a nonadditive feature replaces its value.
 // Absolute sums conservatively refuse extreme cancellation cases. They
 // bound each nested majority+secondary sum, including duplicate beliefs.
 if((!pass.plots.at(static_cast<std::size_t>(plot)).impassable&&*cell.plot_type!=0)||(actual_feature&&actual_feature->natural_wonder)){
  std::int64_t absolute=0;const auto bound=[&](std::int64_t n){(void)native(n);absolute+=n<0?-n:n;need(absolute<=INT32_MAX,"Normal plot yield native nature additive bounds unavailable");};
  std::vector<const BnwNatureYieldRules::Belief*> beliefs;
  if(working){const auto* c=resolve_bnw_city_yield_registry(s.world,cities,native(cell.yield_city->owner),native(cell.yield_city->id));need(c,"Normal plot yield current working-city registry disappeared");beliefs=bnw_plot_beliefs(cities,*c,p->nature.beliefs);}
  const auto& surface=p->nature.terrains.at(terrain);const auto& yi=p->nature.yields.at(y);
  bound(sparse(surface.base,y));
  for(const auto* belief:beliefs){const auto it=belief->terrains.find(terrain);if(it!=belief->terrains.end())bound(sparse(it->second,y));}
  if(*cell.plot_type==1)bound(yi.hills);
  if(*cell.plot_type==0)bound(yi.mountain);
  if(plot_is_lake(water,plot))bound(yi.lake);
  if(actual_feature){
   const auto prefix=absolute;absolute=0;std::int32_t native_feature_change=0;
   const auto feature_term=[&](std::int64_t n){bound(n);if(actual_feature->natural_wonder)native_feature_change=add(native_feature_change,native(n));};
   feature_term(sparse(actual_feature->base,y));
   if(actual_owner!=-1&&actual_improvement==-1){const auto& rows=nature_state.players.at(actual_owner).unimproved_features;const auto row=rows.find(feature);if(row!=rows.end())feature_term(sparse(row->second,y));}
   // CvVotingClasses7087: IsMember executes for EVERY reached yield before
   // the per-league getter. Only that getter's resolution reads are CULTURE
   // gated (3896); both accumulators remain native and ordered.
   if(actual_feature->natural_wonder&&working){
    const auto city_owner=scalar(working->core.fields,"m_eOwner");std::int32_t league_change=0;
    for(const auto& league:nature_state.leagues)if(std::any_of(league.members.begin(),league.members.end(),[&](std::int64_t member){return native(member)==city_owner;})){
     std::int32_t resolution_change=0;
     if(y==nature.culture)for(const auto n:league.culture_per_natural_wonder){bound(n);resolution_change=add(resolution_change,native(n));}
     league_change=add(league_change,resolution_change);
    }
    native_feature_change=add(native_feature_change,league_change);
   }
   for(const auto* belief:beliefs){const auto row=belief->features.find(feature);if(row!=belief->features.end())feature_term(sparse(row->second,y));}
   if(actual_feature->natural_wonder&&actual_owner!=-1){
    for(const auto* belief:beliefs)feature_term(sparse(belief->natural_wonder,y));
    std::int32_t modifier=0;std::int64_t modifier_absolute=0;
    const auto modifier_term=[&](std::int64_t n){const auto value=native(n);modifier_absolute+=value<0?-std::int64_t(value):value;need(modifier_absolute<=INT32_MAX,"Normal plot yield native natural-wonder modifier bounds unavailable");modifier=add(modifier,value);};
    for(const auto* belief:beliefs)modifier_term(sparse(belief->natural_wonder_modifier,y));
    const auto& traits=nature_state.players.at(actual_owner);feature_term(sparse(traits.natural_wonder,y));modifier_term(traits.natural_wonder_modifier);
    // SDK6999..7000: BOTH 100+iMod and the multiplication must fit int32
    // BEFORE division. The wide fused divide cannot rescue their overflow.
    if(modifier>0)native_feature_change=mul(native_feature_change,add(100,modifier))/100;
    absolute=native_feature_change<0?-std::int64_t(native_feature_change):native_feature_change;
   }
   if(!actual_feature->nonadditive){absolute+=prefix;need(absolute<=INT32_MAX,"Normal plot yield native terrain/feature addition bounds unavailable");}
  }
  if(visible_resource!=-1){bound(sparse(p->nature.resources.at(visible_resource),y));for(const auto* belief:beliefs){const auto row=belief->resources.find(visible_resource);if(row!=belief->resources.end())bound(sparse(row->second,y));}}
  if(cell.rivers->is_river)bound(sparse(actual_feature?actual_feature->river:surface.river,y));
  if(*cell.plot_type==1)bound(sparse(actual_feature?actual_feature->hills:surface.hills,y));
 }
 auto value=native(bnw_nature_yield(nature,plot,y,team,false).total);const auto& info=p->yields.at(static_cast<std::size_t>(y));
 // SDK7303..7325: actual improvement presence suppresses unimproved nature
 // traits even when pillaged. Pillage skips BOTH helper and culture suffix.
 if(improvement!=-1&&!improvement_pillaged){
  if(owner!=-1)need(team>=0&&team<64,"Normal improvement actual owning team unavailable");
  std::optional<NormalCultureMembership> culture;
  if(y==4&&actual_owner!=-1)culture=culture_membership(s,actual_owner,p->policy_binding,p->types,p->policy_branches); // Actual source flags/mirror BEFORE wide helper.
  improvement_matrix_inputs(s,p->improvements,p->types);
  const auto improvement_state=restore_improvement_yield_state(s.world,p->improvements,p->types,cities);
  for(const auto& [id,current]:s.cities.players){if(current.second_pantheon_count)need(current.second_pantheon_count==cities.players.at(id).second_pantheon_count,"Normal improvement current secondary pantheon mirror stale");
   for(const auto& [key,c]:current.cities)if(c.we_love_king_turns)need(c.we_love_king_turns==cities.players.at(id).cities.at(key).we_love_king_turns,"Normal improvement current WLTKD mirror stale");
  }
  ImprovementYieldContext improvement_context{p->improvements,improvement_state,water,resources,s.maintenance.team_eras,&p->improvement_religion};improvement_context.current_city_registry=&s.world;
  improvement_native_proof(s,improvement_context,plot,improvement,y,owner,team,visible_resource);
  value=add(value,native(bnw_improvement_yield(improvement_context,plot,improvement,y,owner,false).total()));
  if(y==4){const auto adjacent=p->culture_adjacency.at(improvement);
   if(sparse(p->improvements.improvements.at(improvement).yields[0],4)>0&&adjacent>0)value=add(value,culture_same_improvement(s,p->types,p->improvements,plot,improvement,adjacent));
   // Two separate SDK getters and outer adds. Never merge their native
   // accumulators: later per-improvement cancellation cannot excuse overflow.
   if(culture){value=add(value,culture_policy_numeric(*culture,p->policy_binding,p->culture_policies));
    value=add(value,culture_policy_improvement(*culture,p->policy_binding,p->culture_policies,improvement));}
  }
 }
 // SDK 7327..7333: pillage is tested BEFORE GC.getRouteInfo/getYieldChange;
 // this addition precedes home-city, city minimum, capital, extra and clamping.
 if(route_type!=-1&&!cell.route->pillaged){
  const auto route=p->routes.find(route_type);
  need(route!=p->routes.end(),"Normal plot yield reached Route Info unavailable");
  value=add(value,route->second.at(static_cast<std::size_t>(y)));
 }
 const auto* owner_raw=owner==-1?nullptr:&player(s,owner);
 const bool home_visible=home&&(!selected||city_revealed(*home,selected->team));
 if(home_visible){value=add(value,info.city);if(info.divisor){const auto pop=scalar(home->core.fields,"m_iPopulation");const auto numerator=add(pop,info.offset);need(!(numerator==INT32_MIN&&info.divisor==-1),"Normal plot yield native division overflow");value=add(value,numerator/info.divisor);}}
 if(owner_raw){
  const bool open=!pass.plots.at(static_cast<std::size_t>(plot)).impassable&&*cell.plot_type!=0;
  if(*cell.plot_type==3&&open){value=add(value,vector_value(owner_raw->core.fields,"m_aiSeaPlotYield",y));if(working_visible()){auto city_value=vector_value(working->core.fields,"m_aiSeaPlotYield",y);if(plot_is_lake(water,plot)){const auto lake=vector_value(working->core.fields,"m_aiLakePlotYield",y);city_value=lake>0?lake:0;}value=add(value,city_value);}
   if(visible_resource!=-1&&working_visible())value=add(value,vector_value(working->core.fields,"m_aiSeaResourceYield",y)); // SDK7382..7389, visibility alone
  }
  if(cell.rivers->is_river&&open&&working_visible())value=add(value,vector_value(working->core.fields,"m_aiRiverPlotYield",y));
  // SDK7410..7417: city feature extra applies even on impassable/mountain
  // plots; it precedes the separately guarded terrain-extra addition.
  if(feature!=-1&&working)value=add(value,extra_surface(*working,p->types,"m_ppaiFeatureYieldChange","Features",feature,y));
  if(open&&working)value=add(value,extra_surface(*working,p->types,"m_ppaiTerrainYieldChange","Terrains",terrain,y));
  if(visible_resource!=-1){const auto& info=p->resources.at(visible_resource);const bool has_tech=team_techs(s,team,p->technologies.collection.slots).has_tech(info.technology);
   // SDK7433..7447 imposes owning-player policy and actual reveal technology
   // AGAIN. Team-policy visibility/forced reveal cannot replace these gates.
   if(has_tech&&(info.policy==-1||setup.players.at(owner).policies.contains(info.policy))){
    if(working)value=add(value,extra_surface(*working,p->types,"m_ppaiResourceYieldChange","Resources",visible_resource,y));
    if(info.usage==1)value=add(value,vector_value(owner_raw->traits.fields,"m_iYieldChangeStrategicResources",y));
   }
  }
 }
 if(home_visible){need(actual_owner!=-1,"Displayed city actual plot owner unavailable");const auto* city_owner_raw=&player(s,actual_owner);value=std::max(value,info.min);auto temp=vector_value(city_owner_raw->core.fields,"m_aiCityYieldChange",y);
  if(plot_is_coastal_land(water,plot))value=add(value,vector_value(city_owner_raw->core.fields,"m_aiCoastalCityYieldChange",y));
  const auto capital=scalar(city_owner_raw->core.fields,"m_iCapitalCityID");need(s.cities.players.at(actual_owner).capital==capital,"Normal plot yield raw/owned capital stale");const auto* capital_raw=resolve_bnw_current_city(s,actual_owner,capital);if(capital_raw==home){temp=add(temp,vector_value(city_owner_raw->core.fields,"m_aiCapitalYieldChange",y));value=add(value,mul(scalar(home->core.fields,"m_iPopulation"),vector_value(city_owner_raw->core.fields,"m_aiCapitalYieldPerPopChange",y))/100);}
  value=add(value,temp/100);
 }
 if(!extras)extras.emplace(restore_bnw_game_plot_extras(s,source,identity));
 value=add(value,native(extras->extra_yield(coordinate(cell.x),coordinate(cell.y),y)));
 if(owner_raw){const auto threshold=vector_value(owner_raw->core.fields,"m_aiExtraYieldThreshold",y);if(threshold>0&&value>=threshold){const auto* d=p->defines.find("EXTRA_YIELD");need(d&&d->lookup.found&&d->lookup.error.empty()&&std::holds_alternative<std::int64_t>(d->lookup.value),"Normal plot yield EXTRA_YIELD actual define missing");value=add(value,native(std::get<std::int64_t>(d->lookup.value)));}
  const bool golden=scalar(owner_raw->core.fields,"m_iGoldenAgeTurns")>0;need(s.cities.players.at(owner).golden_age==golden,"Normal plot yield raw/owned golden-age stale");if(golden&&value>=info.golden_threshold)value=add(value,info.golden);
 }
 need(p->current(),"Normal plot yield rules retired during calculation");return std::max<std::int32_t>(0,value);
});}
} // private shared arithmetic body

std::int32_t calculate_bnw_normal_plot_yield(const LiveSessionState& s,const saves::SavedSession& source,const SessionCheckpointIdentity& identity,const BnwNormalPlotYieldRules& rules,std::int32_t plot,std::int32_t y){
 return calculate_plot_source(s,source,identity,rules.impl_,plot,y,false);
}
std::int32_t BnwNormalPlotYieldRules::calculate_current_normal(const BnwPlotYieldRefreshIssuer& issuer,
 const LiveSessionState& state,const saves::SavedSession& source,const SessionCheckpointIdentity& identity,
 std::int32_t plot,std::int32_t y)const{return typed([&]{
 require_current(identity.rules_sha256); // BEFORE any Game/source/world binding read.
 const auto debug=issuer.require_game_debug(state,source,identity);
 const auto result=calculate_plot_source(state,source,identity,impl_,plot,y,false,-1,-1,debug);
 require_current(identity.rules_sha256);(void)issuer.require_game_debug(state,source,identity);
 return result;
});}
std::int32_t BnwDisplayedPlotYield::calculate_source(const LiveSession& session,const BnwNormalPlotYieldRules& rules,std::int32_t plot,std::int32_t y){
 need(session.plot_yield_refresh_thread_==std::this_thread::get_id(),"Displayed yield foreign session thread");
 need(session.bnw_game_debug_owner_current(),"Displayed yield actual import owner unavailable");
 rules.require_current(session.debug_->identity.rules_sha256);
 need(plot>=0&&y>=0&&y<6,"Displayed yield native index unavailable");
 const auto& state=*session.state_;const auto& identity=session.debug_->identity;
 // SDK7254: Game cache branch BEFORE terrain/workability/active-team lookup.
 if(session.bnw_game_debug_mode())return debug_plot_yield(state,plot,y);
 // SDK early zeros precede the native revealed-array team access.
 const auto early=bnw_plot_early_zero_yield(state,plot,y);
 if(early)return calculate_plot_source(state,*session.source_,identity,rules.impl_,plot,y,true);
 const auto active=display_active_team(state);
 return calculate_plot_source(state,*session.source_,identity,rules.impl_,plot,y,true,active.first,active.second);
}
}
