#include "civ5/runtime/bnw_policy_adoption.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/progression_screens.hpp"
#include <algorithm>
#include <atomic>
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_minor_city_strength.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include <array>
#include <climits>
#include <cmath>
#include "civ5/runtime/bnw_happiness_update_prefix.hpp"
#include "civ5/runtime/bnw_happiness_update_suffix.hpp"
#include "civ5/runtime/bnw_player_do_turn_trade.hpp"
#include "civ5/runtime/bnw_major_player_traits_owner.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include "civ5/ui/bnw_plot_yield_refresh_ui_adapter.hpp"
#include <map>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
namespace civ5::runtime {
namespace {
void need(bool ok,const char* why){if(!ok)throw BnwPolicyAdoptionUnavailable(why);}
std::string_view name(const char* p){return p?std::string_view(p):std::string_view{};}
template<class R> auto& named(R& values,std::string_view key){
 using V=std::remove_reference_t<decltype(*values.begin())>;V* out=nullptr;
 for(auto& v:values)if(name(v.name)==key){need(!out,"Duplicate BNW policy raw member");out=&v;}
 need(out,"Missing BNW policy raw member");return *out;
}
// Physical imported version-2 Notifications owner, bounded to its native
// root plus exactly 100 leaf slots. This comparison invokes no providers and
// authenticates all source fields/blobs; absent/padded slots are not defaults.
bool reformation_notification_owner_same(const saves::PlayerAdministrationRecord& a,const saves::PlayerAdministrationRecord& b)noexcept{
 const auto same_leaf=[](const auto& left,const auto& right){
  if(name(left.name)!=name(right.name)||left.index!=right.index||left.start_offset!=right.start_offset||left.end_offset!=right.end_offset||
   left.fields.size()!=right.fields.size()||!left.arrays.empty()||!right.arrays.empty()||left.blobs.size()!=right.blobs.size())return false;
  for(std::size_t i=0;i<left.fields.size();++i)if(name(left.fields[i].name)!=name(right.fields[i].name)||left.fields[i].value!=right.fields[i].value)return false;
  for(std::size_t i=0;i<left.blobs.size();++i)if(name(left.blobs[i].name)!=name(right.blobs[i].name)||left.blobs[i].bytes!=right.blobs[i].bytes)return false;
  return true;
 };
 if(name(a.name)!="notifications"||a.fields.size()!=6||b.fields.size()!=6||!a.blobs.empty()||!b.blobs.empty()||
  a.children.size()!=100||b.children.size()!=100||!same_leaf(a,b))return false;
 for(std::size_t i=0;i<100;++i){const auto& left=a.children[i];const auto& right=b.children[i];
  if(name(left.name)!="notification"||left.index!=i||left.fields.size()!=11||right.fields.size()!=11||left.blobs.size()!=2||right.blobs.size()!=2||
   !left.children.empty()||!right.children.empty()||!same_leaf(left,right))return false;
 }
 return true;
}
std::int64_t scalar(const saves::PlayerCoreObservation& c,std::string_view key){
 const saves::PlayerCoreField* out=nullptr;
 for(const auto& v:c.fields)if(name(v.name)==key){need(!out,"Duplicate BNW policy scalar");out=&v;}
 need(out&&out->encoding==saves::PlayerCoreEncoding::i32&&out->values.size()==1&&
  out->values[0]>=INT32_MIN&&out->values[0]<=INT32_MAX,"Invalid BNW policy native scalar");return out->values[0];
}
template<class State> auto& raw_player(State& s,std::int64_t player){
 need(player>=0&&player<64&&s.world.all_players&&s.world.all_players->layout==saves::GameStateLayout::expansion2_version1&&
  s.world.all_players->players.size()==64,"BNW policy actual roster unavailable");
 auto& p=s.world.all_players->players.at(static_cast<std::size_t>(player));
 need(p.index==static_cast<std::uint64_t>(player)&&p.saved_player_id==player&&p.core.layout==saves::GameStateLayout::expansion2_version1&&
  p.strategy.layout==saves::GameStateLayout::expansion2_version1,"BNW policy raw player identity/layout changed");return p;
}
struct PolicyRule {std::string type,branch;std::int64_t level=0;std::uint64_t hash=0;};
struct Rules {std::map<std::int64_t,PolicyRule> policies;std::map<std::uint64_t,std::int64_t> policy_hashes,branch_hashes,promotion_hashes;};
Rules read_rules(const ui::SelectedBnwPreviewHost& host){
 Rules out;
 const auto policies=host.database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
  need(slot<=static_cast<std::uint64_t>(INT32_MAX),"BNW policy slot exceeds native index");
  PolicyRule value{row.text("Type").value_or(""),row.text("PolicyBranchType").value_or(""),row.integer("Level"),0};
  need(!value.type.empty()&&value.level>=0&&value.level<=INT32_MAX,"BNW policy actual rule invalid");
  const auto* info=host.types().find(value.type);
  need(info&&info->table=="Policies"&&info->id==static_cast<std::int64_t>(slot),"BNW policy actual remap differs");
  const auto* hashed=host.types().find_hash(info->hash);
  need(hashed&&hashed->table==info->table&&hashed->id==info->id&&hashed->type==info->type,"BNW policy global hash remap collision");
  value.hash=info->hash;need(value.hash&&out.policy_hashes.emplace(value.hash,info->id).second,"BNW policy hash collision");
  out.policies.emplace(info->id,std::move(value));return true;
 });
 const auto branches=host.database().prefetch_info("PolicyBranchTypes",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
  const auto type=row.text("Type").value_or("");const auto* info=host.types().find(type);
  need(slot<=static_cast<std::uint64_t>(INT32_MAX)&&info&&info->table=="PolicyBranchTypes"&&info->id==static_cast<std::int64_t>(slot)&&
   info->hash&&out.branch_hashes.emplace(info->hash,info->id).second,"BNW policy branch actual remap differs");
  const auto* hashed=host.types().find_hash(info->hash);
  need(hashed&&hashed->table==info->table&&hashed->id==info->id&&hashed->type==info->type,"BNW policy branch global hash remap collision");return true;
 });

 const auto promotions=host.database().prefetch_info("UnitPromotions",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
  const auto type=row.text("Type").value_or("");const auto* info=host.types().find(type);const auto* hashed=info?host.types().find_hash(info->hash):nullptr;
  need(slot<=INT32_MAX&&info&&hashed&&info->table=="UnitPromotions"&&info->id==static_cast<std::int64_t>(slot)&&hashed->type==info->type&&hashed->id==info->id&&hashed->table==info->table&&info->hash&&out.promotion_hashes.emplace(info->hash,info->id).second,
   "BNW policy actual promotion catalog/hash identity differs");return true;
 });need(promotions.all_succeeded(),"BNW policy complete promotion native rules unavailable");
 need(policies.all_succeeded()&&branches.all_succeeded()&&!out.policies.empty()&&!out.branch_hashes.empty(),"BNW policy SQL/remap incomplete");return out;
}
std::map<std::int64_t,bool> flags(const saves::TeamHashedArray& raw,const std::map<std::uint64_t,std::int64_t>& remap){
 need(raw.wire_count>=0&&static_cast<std::uint64_t>(raw.wire_count)==raw.rows.size(),"BNW policy hash wire count differs");
 std::map<std::int64_t,bool> out;for(const auto& [hash,id]:remap){(void)hash;out.emplace(id,false);}
 for(const auto& row:raw.rows){if(!row.type_hash){need(row.values.empty(),"BNW policy null hash has payload");continue;}
  const auto it=remap.find(row.type_hash);need(it!=remap.end()&&row.values.size()==1&&(row.values[0]==0||row.values[0]==1),"BNW policy unknown/nonboolean raw flag");
  out.at(it->second)=row.values[0]!=0; // Native sequential read: last assignment wins.
 }return out;
}

std::map<std::int64_t,std::int64_t> policy_free_promotion_counts(const saves::PlayerCoreObservation& raw,const Rules& rules){
 const auto& field=named(raw.fields,"m_paiFreePromotionCount");
 need(field.encoding==saves::PlayerCoreEncoding::hashed_i32&&field.wire_count&&*field.wire_count==field.hashes.size(),"BNW policy free promotion wire shape differs");
 std::map<std::int64_t,std::int64_t> out;for(const auto& [hash,id]:rules.promotion_hashes){(void)hash;out.emplace(id,0);}
 std::size_t cursor=0;for(auto hash:field.hashes){if(!hash)continue;const auto info=rules.promotion_hashes.find(hash);
  need(info!=rules.promotion_hashes.end()&&cursor<field.values.size(),"BNW policy free promotion raw type/count unavailable");
  const auto value=field.values[cursor++];need(value>=0&&value<=INT32_MAX,"BNW policy free promotion count outside native nonnegative int");out.at(info->second)=value;
 }
 need(cursor==field.values.size(),"BNW policy free promotion null-hash payload differs");return out;
}
bool same_rows(const saves::TeamHashedArray& a,const saves::TeamHashedArray& b){
 return a.wire_count==b.wire_count&&a.rows.size()==b.rows.size()&&std::equal(a.rows.begin(),a.rows.end(),b.rows.begin(),[](const auto& x,const auto& y){return x.type_hash==y.type_hash&&x.values==y.values;});
}
void set_flag(saves::TeamHashedArray& raw,std::uint64_t hash,bool value){
 saves::TeamHashedValues* last=nullptr;for(auto& row:raw.rows)if(row.type_hash==hash)last=&row;
 if(last){need(last->values.size()==1,"BNW policy store raw shape changed");last->values[0]=value?1:0;}
 else{need(raw.wire_count<INT32_MAX,"BNW policy wire count overflow");raw.rows.push_back({hash,{value?1:0}});++raw.wire_count;}
}
constexpr std::array<std::pair<const char*,const char*>,27> worker_policy_scalars{{
 {"WorkerSpeedModifier","m_iWorkerSpeedModifier"},
 {"ImprovementCostModifier","m_iImprovementCostModifier"},
 {"ImprovementUpgradeRateModifier","m_iImprovementUpgradeRateModifier"},
 {"SpecialistProductionModifier","m_iSpecialistProductionModifier"},
 {"MilitaryProductionModifier","m_iMilitaryProductionModifier"},
 {"BaseFreeUnits","m_iBaseFreeUnits"},
 {"HappinessPerGarrisonedUnit","m_iHappinessPerGarrisonedUnitCount"},
 {"HappinessPerTradeRoute","m_iHappinessPerTradeRouteCount"},
 {"HappinessPerXPopulation","m_iHappinessPerXPopulation"},
 {"ExtraHappinessPerLuxury","m_iExtraHappinessPerLuxury"},
 {"UnhappinessFromUnitsMod","m_iUnhappinessFromUnitsMod"},
 {"UnhappinessMod","m_iUnhappinessMod"},
 {"CityCountUnhappinessMod","m_iCityCountUnhappinessMod"},
 {"OccupiedPopulationUnhappinessMod","m_iOccupiedPopulationUnhappinessMod"},
 {"CapitalUnhappinessMod","m_iCapitalUnhappinessMod"},
 {"WoundedUnitDamageMod","m_iWoundedUnitDamageMod"},
 {"UnitUpgradeCostMod","m_iUnitUpgradeCostMod"},
 {"BarbarianCombatBonus","m_iBarbarianCombatBonus"},
 {"AlwaysSeeBarbCamps","m_iAlwaysSeeBarbCampsCount"},
 {"NumExtraBuilders","m_iMaxNumBuilders"},
 {"PlotGoldCostMod","m_iPlotGoldCostMod"},
 {"PlotCultureCostModifier","m_iPlotCultureCostModifier"},
 {"PlotCultureExponentModifier","m_iPlotCultureExponentModifier"},
 {"NumCitiesPolicyCostDiscount","m_iNumCitiesPolicyCostDiscount"},
 {"GarrisonFreeMaintenance","m_iGarrisonFreeMaintenanceCount"},
 {"GarrisonedCityRangeStrikeModifier","m_iGarrisonedCityRangeStrikeModifier"},
 {"UnitPurchaseCostModifier","m_iUnitPurchaseCostModifier"},
}};

std::int64_t policy_define(const ui::SelectedBnwPreviewHost& host,std::string_view key){
 const auto* value=host.defines().find(key);
 need(host.defines().profile()==content::GlobalDefineProfile::bnw&&value&&value->kind==content::GlobalDefineKind::integer&&
  value->lookup.found&&value->lookup.error.empty()&&value->lookup.sqlite_code==0&&!value->retained_previous&&
  std::holds_alternative<std::int64_t>(value->lookup.value),"BNW policy actual clamp Define unavailable");
 const auto current=host.database().global_define(key,content::GlobalDefineKind::integer);
 need(current.found&&current.error.empty()&&current.sqlite_code==0&&current.value==value->lookup.value,
  "BNW policy current clamp Define differs from selected cache");
 const auto result=std::get<std::int64_t>(value->lookup.value);
 need(result>=INT32_MIN&&result<=INT32_MAX,"BNW policy clamp Define outside native int");return result;
}

std::int64_t native_sum(std::int64_t a,std::int64_t z){const auto sum=a+z;need(sum>=INT32_MIN&&sum<=INT32_MAX,"BNW policy native int addition overflow");return sum;}
std::int64_t native_product(std::int64_t a,std::int64_t z){const auto value=a*z;need(value>=INT32_MIN&&value<=INT32_MAX,"BNW policy native int product overflow");return value;}
std::int64_t city_scalar(const saves::SavedCityObservation& city,std::string_view member){const auto& f=named(city.core.fields,member);need(f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&f.values[0]>=INT32_MIN&&f.values[0]<=INT32_MAX,"BNW policy connection city scalar unavailable");return f.values[0];}
std::int64_t current_connection_gold(const LiveSessionState& state,std::int64_t actor,const ui::SelectedBnwPreviewHost& host,std::int64_t modifier,std::int64_t extra){
 const auto& raw=raw_player(state,actor);const auto& owned=state.cities.players.at(actor);
 const auto capital_id=scalar(raw.core,"m_iCapitalCityID");need(capital_id==owned.capital,"BNW policy connection current capital cache differs");
 const auto* capital=resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(actor),static_cast<std::int32_t>(capital_id));
 // Exact SDK DoUpdateCityConnectionGold short circuit: no capital OR at most
 // one city never calls the route getter or population/Define formula.
 if(!capital||raw.cities.cities.size()<=1)return 0;
 need(raw.planning.layout==saves::GameStateLayout::expansion2_version1,"BNW policy city connection layout unavailable");
 const auto& connections=named(raw.planning.blocks,"city_connections");
 const auto& ids=named(connections.arrays,"city_plot_ids");const auto& routes=named(connections.arrays,"route_infos");
 const auto dimension=named(connections.fields,"route_infos_dimension").value;
 need(named(connections.fields,"version").value==1&&dimension>=0&&dimension<=UINT32_MAX&&routes.columns==2&&routes.values.size()%2==0&&
  (dimension==0?routes.values.empty():(routes.values.size()/2)%static_cast<std::uint64_t>(dimension)==0&&(routes.values.size()/2)/static_cast<std::uint64_t>(dimension)==static_cast<std::uint64_t>(dimension))&&
  ids.columns==1&&named(connections.fields,"city_ids_wire_count").value==static_cast<std::int64_t>(ids.values.size())&&ids.values.size()<=INT32_MAX,
  "BNW policy genuine cached connection matrix/vector shape differs");
 for(const auto value:routes.values)need(value>=INT8_MIN&&value<=INT8_MAX,"BNW policy cached route info outside source char");
 const auto plot_of=[&](const saves::SavedCityObservation& city){
  const auto id=city_scalar(city,"m_iID");const auto& live=owned.cities.at(id);
  const auto x=city_scalar(city,"m_iX"),y=city_scalar(city,"m_iY");
  need(x==live.x&&y==live.y&&state.world.map_projection&&state.world.map_plots&&x>=0&&y>=0&&
   static_cast<std::uint64_t>(x)<state.world.map_header.grid.width&&static_cast<std::uint64_t>(y)<state.world.map_header.grid.height,
   "BNW policy connection actual city coordinates/plot owner missing");
  const auto index=state.world.map_header.grid.index(x,y);const auto& cell=state.world.map_projection->cells.at(index);const auto& plot=state.world.map_plots->plots.at(index);
  need(index<=INT32_MAX&&cell.index==index&&cell.is_city&&cell.plot_city&&cell.plot_city->owner==actor&&cell.plot_city->id==id&&plot.index==index&&
   named(plot.fields,"plot_city_owner").value==actor&&named(plot.fields,"plot_city_id").value==id,"BNW policy connection city/plot registration differs");
  return static_cast<std::int64_t>(index);
 };
 const auto first_plot=plot_of(*capital);
 std::vector<const saves::SavedCityObservation*> cities;cities.reserve(raw.cities.cities.size());
 for(const auto& city:raw.cities.cities){const auto id=city_scalar(city,"m_iID");need(resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(actor),static_cast<std::int32_t>(id))==&city,"BNW policy connection full native city roster differs");cities.push_back(&city);}
 std::sort(cities.begin(),cities.end(),[](const auto* a,const auto* z){return (city_scalar(*a,"m_iID")&8191)<(city_scalar(*z,"m_iID")&8191);});
 std::int64_t total=0;
 for(const auto* city:cities){
  if(city==capital)continue;const auto second_plot=plot_of(*city);std::int64_t a=-1,z=-1;
  // Native HasCityConnectionRouteBetweenCities: original vector order and
  // break once BOTH indices are found, including its duplicate behavior.
  for(std::size_t i=0;i<ids.values.size();++i){if(ids.values[i]==first_plot)a=static_cast<std::int64_t>(i);if(ids.values[i]==second_plot)z=static_cast<std::int64_t>(i);if(a>=0&&z>=0)break;}
  if(a<0||z<0||a>=dimension||z>=dimension)continue;
  const auto route_index=(static_cast<std::uint64_t>(a)*static_cast<std::uint64_t>(dimension)+static_cast<std::uint64_t>(z))*2+1;
  if((routes.values.at(static_cast<std::size_t>(route_index))&1)==0)continue;
  // Only a genuine connected city reaches the original formula operands.
  const auto capital_pop=city_scalar(*capital,"m_iPopulation"),city_pop=city_scalar(*city,"m_iPopulation");
  need(capital_pop==owned.cities.at(city_scalar(*capital,"m_iID")).population&&city_pop==owned.cities.at(city_scalar(*city,"m_iID")).population&&capital_pop>=0&&city_pop>=0,
   "BNW policy route gold raw/owned city populations differ");
  auto gold=policy_define(host,"TRADE_ROUTE_BASE_GOLD");
  gold=native_sum(gold,native_product(capital_pop,policy_define(host,"TRADE_ROUTE_CAPITAL_POP_GOLD_MULTIPLIER")));
  gold=native_sum(gold,native_product(city_pop,policy_define(host,"TRADE_ROUTE_CITY_POP_GOLD_MULTIPLIER")));
  gold=native_sum(gold,native_product(extra,100));
  if(modifier!=0)gold=native_product(gold,native_sum(100,modifier))/100;
  total=native_sum(total,gold);
 }
 return total;
}
constexpr std::array<std::pair<const char*,const char*>,17> city_policy_scalars{{
 {"CityStrengthMod","m_iCityStrengthMod"},
 {"CityGrowthMod","m_iCityGrowthMod"},
 {"CapitalGrowthMod","m_iCapitalGrowthMod"},
 {"SettlerProductionModifier","m_iSettlerProductionModifier"},
 {"CapitalSettlerProductionModifier","m_iCapitalSettlerProductionModifier"},
 {"RouteGoldMaintenanceMod","m_iRouteGoldMaintenanceMod"},
 {"BuildingGoldMaintenanceMod","m_iBuildingGoldMaintenanceMod"},
 {"UnitGoldMaintenanceMod","m_iUnitGoldMaintenanceMod"},
 {"UnitSupplyMod","m_iUnitSupplyMod"},
 {"HappyPerMilitaryUnit","m_iHappyPerMilitaryUnit"},
 {"HappinessToCulture","m_iHappinessToCulture"},
 {"HappinessToScience","m_iHappinessToScience"},
 {"HalfSpecialistUnhappiness","m_iHalfSpecialistUnhappinessCount"},
 {"HalfSpecialistFood","m_iHalfSpecialistFoodCount"},
 {"MilitaryFoodProduction","m_iMilitaryFoodProductionCount"},
 {"GoldenAgeCultureBonusDisabled","m_iGoldenAgeCultureBonusDisabledCount"},
 {"SecondReligionPantheon","m_iSecondReligionPantheonCount"},
}};

void require_policy_city_shared_pairs(const LiveSessionState& s,std::int64_t actor,const saves::PlayerCoreObservation& raw){
 const auto& m=s.maintenance.players.at(actor);const auto& u=s.unhappiness.players.at(actor);const auto& c=s.cities.players.at(actor);
 need(m.improvement_modifier==scalar(raw,"m_iRouteGoldMaintenanceMod")&&m.building_modifier==scalar(raw,"m_iBuildingGoldMaintenanceMod")&&
  m.unit_modifier==scalar(raw,"m_iUnitGoldMaintenanceMod")&&m.supply_modifier==scalar(raw,"m_iUnitSupplyMod")&&u.half_specialists==(scalar(raw,"m_iHalfSpecialistUnhappinessCount")>0)&&
  (!c.second_pantheon_count||*c.second_pantheon_count==scalar(raw,"m_iSecondReligionPantheonCount")),"BNW policy shared city/maintenance/unhappiness modifier differs");
}
constexpr std::array<std::pair<const char*,const char*>,15> extra_policy_scalars{{
 {"EnablesSSPartHurry","m_iEnablesSSPartHurryCount"},
 {"EnablesSSPartPurchase","m_iEnablesSSPartPurchaseCount"},
 {"MaxConscript","m_iMaxConscript"},
 {"ExpModifier","m_iExpModifier"},
 {"ExpInBorderModifier","m_iExpInBorderModifier"},
 {"MinorQuestFriendshipMod","m_iMinorQuestFriendshipMod"},
 {"MinorGoldFriendshipMod","m_iMinorGoldFriendshipMod"},
 {"MinorFriendshipMinimum","m_iMinorFriendshipMinimum"},
 {"MinorFriendshipDecayMod","m_iMinorFriendshipDecayMod"},
 {"MinorScienceAllies","m_iMinorScienceAlliesCount"},
 {"MinorResourceBonus","m_iMinorResourceBonusCount"},
 {"NewCityExtraPopulation","m_iNewCityExtraPopulation"},
 {"FreeFoodBox","m_iFreeFoodBox"},
 {"StrategicResourceMod","m_iStrategicResourceMod"},
 {"AbleToAnnexCityStates","m_iAbleToAnnexCityStatesCount"},
}};

constexpr std::array<std::pair<const char*,const char*>,8> yield_policy_arrays{{
 {"Policy_YieldModifiers","m_aiYieldRateModifier"},
 {"Policy_CityYieldChanges","m_aiCityYieldChange"},
 {"Policy_CoastalCityYieldChanges","m_aiCoastalCityYieldChange"},
 {"Policy_CapitalYieldChanges","m_aiCapitalYieldChange"},
 {"Policy_CapitalYieldPerPopChanges","m_aiCapitalYieldPerPopChange"},
 {"Policy_CapitalYieldModifiers","m_aiCapitalYieldRateModifier"},
 {"Policy_GreatWorkYieldChanges","m_aiGreatWorkYieldChange"},
 {"Policy_SpecialistExtraYields","m_aiSpecialistExtraYield"},
}};
using PolicyYieldArray=std::array<std::int64_t,6>;
using PolicyRanks=std::pair<std::vector<std::int64_t>,std::vector<std::int64_t>>;
PolicyYieldArray policy_yield_vector(const saves::PlayerCoreObservation& raw,std::string_view key){
 const auto& field=named(raw.fields,key);
 need(field.encoding==saves::PlayerCoreEncoding::vector_i32&&field.wire_count==std::optional<std::uint64_t>{6}&&field.values.size()==6,"BNW policy actual six-yield array missing");
 PolicyYieldArray out{};for(std::size_t i=0;i<out.size();++i){need(field.values[i]>=INT32_MIN&&field.values[i]<=INT32_MAX,"BNW policy native yield array outside int");out[i]=field.values[i];}return out;
}
template<class City> auto& policy_rank_array(City& raw,std::string_view member){
 auto& block=named(raw.production.blocks,"before_strategy");auto& array=named(block.arrays,member);
 need(array.columns==1&&array.values.size()==6,"BNW policy actual city rank validity shape unavailable");
 need(named(block.fields,std::string(member)+"_wire_count").value==6,"BNW policy city rank validity wire count differs");
 for(auto value:array.values)need(value==0||value==1,"BNW policy city rank validity is not native bool");return array;
}
std::array<PolicyYieldArray,8> actual_policy_yield_changes(const ui::SelectedBnwPreviewHost& host,const std::string& type){
 std::set<std::int64_t> slots;
 const auto yields=host.database().prefetch_info("Yields",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
  const auto* info=host.types().find(row.text("Type").value_or(""));
  need(slot<6&&info&&info->table=="Yields"&&info->id==static_cast<std::int64_t>(slot)&&slots.emplace(info->id).second,
   "BNW policy six native Yield Info slots/remap unavailable");return true;
 });
 need(yields.all_succeeded()&&slots.size()==6,"BNW policy complete native Yield catalog unavailable");
 std::optional<std::int64_t> maximum;
 const auto span=host.database().query_rows({"SELECT max(rowid) from Yields",{}},[&](const content::RulesRowView& row){maximum=row.integer_at(0);});
 need(span.all_succeeded()&&span.sqlite_code==0&&span.rows==1&&maximum&&*maximum==5,"BNW policy native MaxRows Yield span differs");
 std::array<PolicyYieldArray,8> out{};
 for(std::size_t i=0;i<out.size();++i){
  // Exact CvDatabaseUtility::SetYields/PopulateArrayByValue SQL, including
  // native sequential last-row assignment and INNER JOIN missing-ref behavior.
  const auto report=host.database().query_rows({std::string("select Yields.ID, Yield from ")+yield_policy_arrays[i].first+
   " inner join Yields on YieldType = Yields.Type where PolicyType = ?",{type}},[&](const content::RulesRowView& row){
    const auto id=row.integer("ID"),value=row.integer("Yield");
    need(id>=0&&id<6&&slots.contains(id)&&value>=INT32_MIN&&value<=INT32_MAX,"BNW policy linked native yield outside source bounds");out[i][static_cast<std::size_t>(id)]=value;
   });
  need(report.all_succeeded()&&report.sqlite_code==0,"BNW policy linked yield SQL incomplete");
 }
 return out;
}
std::int64_t sparse_yield(const YieldValues& values,std::int64_t id){const auto it=values.find(id);return it==values.end()?0:it->second;}
void require_policy_yield_pairs(const LiveSessionState& s,std::int64_t actor,const saves::PlayerCoreObservation& raw){
 need(s.cities.profile==content::TechnologyProfile::bnw&&s.cities.yield_slots==6,"BNW policy shared city Yield context unavailable");
 const auto& player=s.cities.players.at(actor);const auto normal=policy_yield_vector(raw,"m_aiYieldRateModifier"),capital=policy_yield_vector(raw,"m_aiCapitalYieldRateModifier");
 for(std::int64_t id=0;id<6;++id)need(sparse_yield(player.modifiers,id)==normal[static_cast<std::size_t>(id)]&&
  sparse_yield(player.capital_modifiers,id)==capital[static_cast<std::size_t>(id)],"BNW policy shared raw/owned yield modifier cache differs");
}

constexpr std::array<const char*,4> combat_hurry_members{"m_paiUnitCombatProductionModifiers","m_paiUnitCombatFreeExperiences","m_paiHurryCount","m_paiHurryModifier"};
std::vector<std::int64_t> policy_native_vector(const saves::PlayerCoreObservation& raw,std::string_view key){
 const auto& field=named(raw.fields,key);need(field.encoding==saves::PlayerCoreEncoding::vector_i32&&field.wire_count&&*field.wire_count==field.values.size(),
  "BNW policy native combat/hurry vector wire shape differs");
 for(auto value:field.values)need(value>=INT32_MIN&&value<=INT32_MAX,"BNW policy combat/hurry value outside native int");return field.values;
}
std::vector<std::string> actual_policy_catalog(const ui::SelectedBnwPreviewHost& host,const char* table){
 std::vector<std::string> out;const auto report=host.database().prefetch_info(table,[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
  const auto type=row.text("Type").value_or("");const auto* info=host.types().find(type);
  need(slot==out.size()&&slot<=INT32_MAX&&info&&info->table==table&&info->id==static_cast<std::int64_t>(slot),"BNW policy native catalog dense slot/remap differs");
  out.push_back(type);return true;
 });
 need(report.all_succeeded(),"BNW policy complete catalog SQL unavailable");
 std::optional<std::int64_t> maximum;const auto span=host.database().query_rows({std::string("SELECT max(rowid) from ")+table,{}},[&](const content::RulesRowView& row){maximum=row.integer_at(0);});
 need(span.all_succeeded()&&span.sqlite_code==0&&span.rows==1&&maximum&&*maximum>=-1&&static_cast<std::uint64_t>(*maximum+1)==out.size(),"BNW policy native catalog MaxRows span differs");return out;
}
std::vector<std::int64_t> actual_policy_linked_values(const ui::SelectedBnwPreviewHost& host,const char* catalog,const char* table,const char* reference,const char* value,
 const std::string& policy,std::size_t count){
 std::vector<std::int64_t> out(count,0);
 const auto report=host.database().query_rows({std::string("select ")+catalog+".ID, "+value+" from "+table+" inner join "+catalog+" on "+reference+" = "+catalog+".Type where PolicyType = ?",{policy}},[&](const content::RulesRowView& row){
  const auto id=row.integer_at(0),n=row.integer_at(1);need(id>=0&&static_cast<std::uint64_t>(id)<count&&n>=INT32_MIN&&n<=INT32_MAX,"BNW policy linked catalog value outside native index/int");
  out[static_cast<std::size_t>(id)]=n;
 });
 need(report.all_succeeded()&&report.sqlite_code==0,"BNW policy linked catalog SQL incomplete");return out;
}

template<class Raw>std::vector<std::int64_t> policy_improvement_matrix(const Raw& raw){
 const auto& block=named(raw.administration.blocks,"pre_city");const auto& matrix=named(block.arrays,"m_ppaaiImprovementYieldChange");
 const auto count=named(block.fields,"m_ppaaiImprovementYieldChange_wire_count").value;
 need(count>=0&&count<=INT32_MAX&&matrix.columns==6&&static_cast<std::uint64_t>(count)<=SIZE_MAX/6&&matrix.values.size()==static_cast<std::size_t>(count)*6,
  "BNW policy native improvement matrix wire shape differs");
 for(auto value:matrix.values)need(value>=INT32_MIN&&value<=INT32_MAX,"BNW policy improvement matrix outside native int");return matrix.values;
}
struct NativePolicyState {
 std::int64_t culture=0,cost=0,free_policies=0,free_tenets=0,count=0;
 std::map<std::int64_t,bool> has,unlocked,blocked,finished,one_shot,one_shot_free_units;
 std::int64_t extra_branches=0;
 std::array<std::int64_t,3> history{};
 std::map<std::string,std::int64_t,std::less<>> process_scalars;
 std::map<std::string,PolicyYieldArray,std::less<>> process_arrays;
 std::map<std::string,std::vector<std::int64_t>,std::less<>> combat_hurry_arrays;
 std::vector<std::int64_t> improvement_yields;
 std::map<std::int64_t,std::int64_t> free_promotions;
 bool operator==(const NativePolicyState&)const=default;
};
struct PolicyRegistry {std::map<std::int64_t,NativePolicyState> players;std::map<std::int64_t,std::int64_t> pending_policy;std::vector<BnwPolicyUiRequest> ui;std::map<std::pair<std::int64_t,std::int64_t>,std::int64_t> city_policy_culture;std::uint64_t next_occurrence=1;std::map<std::pair<std::int64_t,std::int64_t>,PolicyRanks> city_yield_ranks;};
static_assert(std::is_nothrow_move_assignable_v<PolicyRegistry>);
static_assert(std::is_nothrow_move_assignable_v<NativePolicyState>);

void require_policy_shared_pairs(const LiveSessionState& state,std::int64_t actor,const saves::PlayerCoreObservation& raw){
 need(state.maintenance.profile==content::TechnologyProfile::bnw&&state.maintenance.players.contains(actor)&&state.unhappiness.players.contains(actor),
  "BNW policy shared maintenance/unhappiness owner absent");
 const auto& maintenance=state.maintenance.players.at(actor);const auto& unhappy=state.unhappiness.players.at(actor);
 need(maintenance.base_free==scalar(raw,"m_iBaseFreeUnits")&&maintenance.free_garrison==(scalar(raw,"m_iGarrisonFreeMaintenanceCount")>0)&&
  unhappy.units_modifier==scalar(raw,"m_iUnhappinessFromUnitsMod")&&unhappy.population_modifier==scalar(raw,"m_iUnhappinessMod")&&
  unhappy.cities_modifier==scalar(raw,"m_iCityCountUnhappinessMod")&&unhappy.occupied_modifier==scalar(raw,"m_iOccupiedPopulationUnhappinessMod")&&
  unhappy.capital_modifier==scalar(raw,"m_iCapitalUnhappinessMod"),"BNW policy shared raw/owned scalar cache differs");
}
NativePolicyState read_native(const LiveSessionState& s,std::int64_t actor,const Rules& rules){
 auto& raw=raw_player(s,actor);auto& block=named(raw.strategy.blocks,"policies");NativePolicyState out;
 require_policy_shared_pairs(s,actor,raw.core);require_policy_city_shared_pairs(s,actor,raw.core);
 for(const auto* member:{"m_iCulturePerWonder","m_iCultureWonderMultiplier","m_iCulturePerTechResearched","m_iGoldenAgeMeterMod","m_iGoldenAgeModifier"})out.process_scalars.emplace(member,scalar(raw.core,member));
 for(const auto& entry:worker_policy_scalars)out.process_scalars.emplace(entry.second,scalar(raw.core,entry.second));

 for(const auto* member:{"m_iGoldPerUnit","m_iGoldPerMilitaryUnit"})out.process_scalars.emplace(member,scalar(raw.core,member));
 need(raw.administration.layout==saves::GameStateLayout::expansion2_version1&&s.treasury.profile==content::TechnologyProfile::bnw&&s.treasury.players.contains(actor),"BNW policy actual treasury owner/layout unavailable");
 const auto& treasury_block=named(raw.administration.blocks,"treasury");const auto& treasury=s.treasury.players.at(actor);
 need(treasury.source_administration==std::optional{std::pair{raw.administration.start_offset,raw.administration.end_offset}},"BNW policy treasury provenance differs");
 for(const auto* member:{"m_iCityConnectionTradeRouteGoldModifier","m_iCityConnectionGoldTimes100","m_iCityConnectionTradeRouteGoldChange"}){
  const auto value=named(treasury_block.fields,member).value;need(value>=INT32_MIN&&value<=INT32_MAX,"BNW policy treasury scalar outside native int");out.process_scalars.emplace(member,value);}
 need(treasury.connection_modifier==out.process_scalars.at("m_iCityConnectionTradeRouteGoldModifier")&&treasury.connections_times100==out.process_scalars.at("m_iCityConnectionGoldTimes100")&&
  treasury.connection_change==out.process_scalars.at("m_iCityConnectionTradeRouteGoldChange")&&s.maintenance.players.at(actor).gold_per_unit_times100==out.process_scalars.at("m_iGoldPerUnit"),
  "BNW policy treasury/maintenance raw-owned fields differ");
 for(const auto& entry:city_policy_scalars)out.process_scalars.emplace(entry.second,scalar(raw.core,entry.second));
 // FieldSpec third value is MINIMUM VERSION, never a field count.
 need(raw.core.version&&*raw.core.version>=12,"BNW policy missing legacy constructor-only fields requires version-aware native serialization");
 for(const auto& entry:extra_policy_scalars)out.process_scalars.emplace(entry.second,scalar(raw.core,entry.second));
 out.process_scalars.emplace("m_iAllFeatureProduction",scalar(raw.core,"m_iAllFeatureProduction"));
 require_policy_yield_pairs(s,actor,raw.core);for(const auto& entry:yield_policy_arrays)out.process_arrays.emplace(entry.second,policy_yield_vector(raw.core,entry.second));
 for(auto member:combat_hurry_members)out.combat_hurry_arrays.emplace(member,policy_native_vector(raw.core,member));
 for(auto member:{"m_iNumCitiesFreeCultureBuilding","m_iNumCitiesFreeFoodBuilding","m_iAttackBonusTurns","m_iFreeTechCount","m_iGoldenAgeTurns","m_iNumGoldenAges","m_iMedianTechPercentage","m_iNumFreePoliciesEver","m_iNumFreeGreatPeople","m_iHappiness","m_iCityConnectionHappiness","m_iGreatPeopleRateModifier","m_iGreatGeneralRateModifier","m_iGreatAdmiralRateModifier","m_iGreatWriterRateModifier","m_iGreatArtistRateModifier","m_iGreatMusicianRateModifier","m_iGreatMerchantRateModifier","m_iGreatScientistRateModifier","m_iGreatEngineerRateModifier","m_iDomesticGreatGeneralRateModifier","m_iPolicyCostModifier","m_iFreeExperience","m_iMaxEffectiveCities"})out.process_scalars.emplace(member,scalar(raw.core,member));
 need(s.science.profile==content::TechnologyProfile::bnw&&s.science.players.contains(actor)&&s.science.players.at(actor).median_tech_percent==scalar(raw.core,"m_iMedianTechPercentage"),"BNW policy raw/shared median technology cache differs");
 need(out.process_scalars.at("m_iGoldenAgeTurns")>=0&&s.cities.players.at(actor).golden_age==(out.process_scalars.at("m_iGoldenAgeTurns")>0),"BNW policy raw/owned golden-age pair differs");
 need(s.unhappiness.players.at(actor).happiness==out.process_scalars.at("m_iHappiness"),"BNW policy raw/shared happiness cache differs");
 need(s.player_research.profile==content::TechnologyProfile::bnw&&s.player_research.players.contains(actor)&&s.player_research.players.at(actor).max_effective_cities==out.process_scalars.at("m_iMaxEffectiveCities"),"BNW policy raw/shared maximum effective cities cache differs");
 out.free_promotions=policy_free_promotion_counts(raw.core,rules);
 out.improvement_yields=policy_improvement_matrix(raw);
 out.culture=scalar(raw.core,"m_iJONSCulture");out.cost=scalar(raw.core,"m_iCostNextPolicy");
 out.free_policies=scalar(raw.core,"m_iNumFreePolicies");out.free_tenets=scalar(raw.core,"m_iNumFreeTenets");out.count=scalar(raw.core,"m_iNumPolicies");
 need(out.culture>=0&&out.cost>=0&&out.free_policies>=0&&out.free_tenets>=0&&out.count>=0,"BNW policy native payment/count invalid");
 out.has=flags(named(block.hashes,"has_policy"),rules.policy_hashes);
 out.one_shot=flags(named(block.hashes,"one_shot_fired"),rules.policy_hashes);
 out.one_shot_free_units=flags(named(block.hashes,"one_shot_free_units_fired"),rules.policy_hashes);
 out.extra_branches=named(block.fields,"extra_branches").value;need(out.extra_branches>=INT32_MIN&&out.extra_branches<=INT32_MAX,"BNW policy extra branches outside native int");
 out.unlocked=flags(named(block.hashes,"branch_unlocked"),rules.branch_hashes);
 out.blocked=flags(named(block.hashes,"branch_blocked"),rules.branch_hashes);
 out.finished=flags(named(block.hashes,"branch_finished"),rules.branch_hashes);
 for(std::size_t i=0;i<3;++i){const auto v=named(block.fields,"branch_picked_"+std::to_string(i+1)+"_hash").value;
  if(!v)out.history[i]=-1;else{const auto it=rules.branch_hashes.find(static_cast<std::uint64_t>(v));need(it!=rules.branch_hashes.end(),"BNW policy history unknown branch hash");out.history[i]=it->second;}}
 return out;
}
std::set<std::int64_t> has_set(const NativePolicyState& p){std::set<std::int64_t> out;for(const auto& [id,v]:p.has)if(v)out.insert(id);return out;}
}
BnwPolicyNativeObservation observe_bnw_policy_native(const ui::SelectedBnwPreviewHost& host,std::int64_t actor){
 // Owner-thread, serial synchronous read projection. No gameplay admission.
 const auto pin=host.session().read_state_lease();const auto generation=host.session().state_generation();
 need(bool(pin),"BNW policy observed world missing");const auto revision=pin->ui_source_revision;
 const auto current=host.database().immutable_preview_fingerprint_currentness();need(current&&current(),"BNW policy observed rules changed");
 const auto rules=read_rules(host);const auto native=read_native(*pin,actor,rules);
 BnwPolicyNativeObservation out;out.player=actor;out.culture=native.culture;out.cost=native.cost;out.free_policies=native.free_policies;
 out.one_shot=native.one_shot;out.one_shot_free_units=native.one_shot_free_units;out.extra_branches=native.extra_branches;out.free_tenets=native.free_tenets;out.count=native.count;out.has=native.has;out.unlocked=native.unlocked;out.blocked=native.blocked;out.finished=native.finished;out.history=native.history;out.process_scalars=native.process_scalars;out.process_arrays=native.process_arrays;out.combat_hurry_arrays=native.combat_hurry_arrays;out.improvement_yields=native.improvement_yields;out.free_promotions=native.free_promotions;
 if(const auto* registry=pin->setup.extensions.find<PolicyRegistry>();registry&&registry->players.contains(actor)){
  out.owned_pair=registry->players.at(actor)==native&&pin->setup.players.at(actor).policies_resolved&&pin->setup.players.at(actor).policies==has_set(native);
  need(out.owned_pair,"BNW policy observed raw/owned state differs");
  for(const auto& [key,value]:registry->city_yield_ranks)if(key.first==actor)out.city_yield_ranks.emplace(key.second,value);
  for(const auto& request:registry->ui)if(request.player==actor)out.ui.push_back(request);
 }
 for(const auto& city:raw_player(*pin,actor).cities.cities){const auto id=city_scalar(city,"m_iID");need(out.city_strengths.emplace(id,city_scalar(city,"m_iStrengthValue")).second,"BNW policy observed duplicate actual city");}
 for(const auto& city:raw_player(*pin,actor).cities.cities){const auto id=city_scalar(city,"m_iID");const auto& block=named(city.production.blocks,"before_strategy");
  const auto& base=named(block.arrays,"m_aiBaseYieldRank");const auto& rate=named(block.arrays,"m_aiYieldRank");
  need(base.columns==1&&rate.columns==1&&base.values.size()==6&&rate.values.size()==6,"BNW policy actual city rank-number shape unavailable");
  out.city_yield_rank_numbers.emplace(id,PolicyRanks{base.values,rate.values});}
 for(const auto& city:raw_player(*pin,actor).cities.cities)need(out.city_policy_culture.emplace(city_scalar(city,"m_iID"),city_scalar(city,"m_iJONSCulturePerTurnFromPolicies")).second,"BNW policy observed duplicate culture city");
 out.city_dirty_events=pin->city_dirty_events;
 out.generation=generation;out.revision=revision;
 need(current()&&host.session().read_state_lease()==pin&&host.session().state_generation()==generation&&pin->ui_source_revision==revision,"BNW policy observed source changed during projection");return out;
}
struct BnwPolicyAdoptionHooks::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::weak_ptr<const BnwPolicyAdoptionHooks> self;BnwStockLuaAttemptLease lease;const std::uint64_t serial;
 std::weak_ptr<const BnwSelectedStockLuaSourceOwner> source_owner;
 CanAdopt can;Accept accept;bool claimed=false;
 Impl(std::shared_ptr<const ui::SelectedBnwPreviewHost> h,BnwStockLuaAttemptLease l,CanAdopt c,Accept a,std::shared_ptr<const BnwSelectedStockLuaSourceOwner> owner):host(h),lease(std::move(l)),serial(lease.operation_serial()),source_owner(std::move(owner)),can(std::move(c)),accept(std::move(a)){}
};
BnwPolicyAdoptionHooks::BnwPolicyAdoptionHooks(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwPolicyAdoptionHooks::~BnwPolicyAdoptionHooks()=default;
std::shared_ptr<const BnwPolicyAdoptionHooks> BnwPolicyAdoptionHooks::issue(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,BnwStockLuaAttemptLease lease,CanAdopt can,Accept accept,std::shared_ptr<const BnwSelectedStockLuaSourceOwner> source){
 need(host&&source&&can&&accept&&lease.world_lease()==host->session().read_state_lease(),"BNW policy genuine SAME-world hooks unavailable");
 auto out=std::shared_ptr<const BnwPolicyAdoptionHooks>(new BnwPolicyAdoptionHooks(std::make_unique<Impl>(host,std::move(lease),std::move(can),std::move(accept),std::move(source))));
 out->impl_->self=out;out->impl_->lease.require_operation(out->impl_->serial);return out;
}
struct BnwPolicyAdoptionPending::Impl {
 std::weak_ptr<const BnwPolicyAdoptionPending> self;std::atomic_bool retired{false};
 std::int64_t root_target=-1;bool finisher_attempted=false,is_finisher=false,finisher_granted=false,finisher_ever_stored=false,finisher_returned=false;
 bool branch_achievement_backend_disabled=false;std::optional<std::int64_t> finishing_policy;
 std::unique_ptr<Impl> suspended_parent,completed_finisher;
 std::optional<BnwPolicyNativeObservation> finisher_process_before,finisher_process_after;
 bool notification_attempted=false;std::int64_t free_reward_before=0,free_reward_ever_before=0,free_reward_delta=0;
 LiveSession* session=nullptr;
 const std::thread::id thread=std::this_thread::get_id();std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::weak_ptr<const BnwSelectedStockLuaSourceOwner> source_owner;std::weak_ptr<const void> source_bootstrap;
 std::shared_ptr<const BnwPolicyAdoptionHooks> hooks;std::shared_ptr<LiveSessionState> state;
 std::shared_ptr<const saves::SavedSession> source;std::function<bool()> rules_current;Rules rules;
 NativePolicyState expected;std::int64_t actor=-1,target=-1,turn=-1;std::uint64_t generation=0,before=0,previous=0,after=0;
 std::vector<BnwHappinessSuffixStore> happiness_suffix_stores;std::optional<std::int64_t> happiness_expected_excess;bool happiness_dirty_published=false,happiness_achievement=false;
 bool happiness_native_suffix_returned=false,happiness_dirty_attempted=false,happiness_achievement_backend_disabled=false;
 std::vector<BnwHappinessNativeStore> happiness_stores;std::optional<BnwReligionHappinessTerms> happiness_religion;
 bool happiness_inflight=false,happiness_founder_attempt=false,happiness_foreign_attempt=false;std::uint8_t happiness_founder_calls=0;
 std::uint8_t great_people_reset_cursor=0;std::string trade_source;
 std::uint8_t great_people_traits_cursor=0;std::array<std::int32_t,3> great_people_traits_operands{};
 std::uint8_t great_people_modifiers_cursor=0;std::array<std::int32_t,12> great_people_modifiers_operands{};
 bool great_people_recompute_returned=false;std::array<std::int32_t,4> great_people_suffix_operands{};
 bool process_dirty_attempted=false,process_policies_returned=false;
 bool early_process_policies_returned=false;
 bool outer_branch_attempted=false,outer_policy_branch_returned=false;
 bool outer_branch_rewards_attempted=false;std::optional<std::int32_t> outer_branch_free_policy_operand;
 std::optional<BnwPolicyBranchCreditHistory> outer_branch_credit_history;
 bool outer_branch_notifications_attempted=false;std::int32_t outer_branch_notification_cursor=0;
 std::vector<BnwPolicyBranchNotificationHistory> outer_branch_notifications;
 bool outer_dirty_attempted=false,outer_policy_dirty_returned=false;
 bool outer_hook_attempted=false,outer_policy_hook_returned=false;
 bool plot_ui_dispatching=false,outer_yield_resuming=false;BnwPolicyAdoptionPhase plot_parent_call_phase=BnwPolicyAdoptionPhase::prepared;
 bool outer_yield_attempted=false,outer_policy_yield_returned=false;std::uint64_t outer_yield_scanned=0;
 bool next_cost_attempted=false,next_policy_cost_returned=false;std::array<std::int32_t,4> next_policy_cost_operands{};
 std::uint64_t camp_cursor=0;bool camp_visibility_returned=false,camp_store_completed=false;std::optional<std::int64_t> camp_changed_plot;std::string camp_array_name;std::int64_t camp_before_wire=0,camp_after_wire=0,camp_team=-1;
 std::uint8_t post_gp_scalar_cursor=0;std::array<std::int32_t,4> post_gp_scalar_operands{};
 bool cost_observer_inflight=false,cost_observer_stored=false;std::int64_t cost_observer_before=0,cost_observer_after=0;std::uint64_t cost_observer_pre_revision=0,cost_observer_post_revision=0;
 std::uint8_t units_reward_cursor=0;std::optional<std::int64_t> policy_resource_plot;
 std::map<std::int64_t,std::int64_t> resource_policy_reveals;
 bool resource_loop_reached=false,resource_loop_returned=false,resource_reveal_attempted=false;
 std::uint64_t resource_scanned=0;std::vector<std::int64_t> resource_yield_returned_plots;
 bool resource_layout_attempted=false,resource_layout_stored=false;
 std::vector<std::int64_t> resource_layout_returned_plots;
 std::weak_ptr<const ui::BnwClientSelectionControl> resource_layout_selection;
 bool reformation_notification_attempted=false,reformation_native_returned=false,reformation_returned=false,reformation_inserted=false;
 bool post_reformation_tail_attempted=false;std::int64_t post_reformation_spy_change=0,reformation_effect_policy=-1;
 std::vector<saves::ReligionObservation> reformation_raw_religions;
 std::vector<CityYieldReligionBeliefs> reformation_owned_religions;
 std::optional<saves::PlayerAdministrationRecord> reformation_notification_owner;
 std::optional<BnwNotificationLoadTransients> reformation_notification_transients;
 std::uint8_t reward_cursor=0;std::array<std::int64_t,5> reward_effects{};
 bool golden_age_map_attempted=false,golden_age_map_resuming=false,golden_age_map_returned=false;std::uint64_t golden_age_map_scanned=0;
 bool golden_age_gameplay_attempted=false,golden_age_gameplay_stored=false,golden_age_gameplay_returned=false;
 bool golden_age_notification_attempted=false,golden_age_notification_returned=false;
 bool free_tech_text_attempted=false,free_tech_text_returned=false,free_tech_count_stored=false,free_tech_choice_returned=false;
 bool free_tech_dirty_attempted=false,free_tech_dirty_returned=false,free_tech_notification_attempted=false;
 std::array<std::int32_t,4> free_tech_choice_operands{}; // actual delta, old, new, NO_SCIENCE value
 bool golden_age_replay_attempted=false,golden_age_replay_returned=false;
 bool golden_age_history_attempted=false,golden_age_count_stored=false,golden_age_achievement_backend_disabled=false;
 bool golden_age_attempted=false,golden_age_turns_stored=false,golden_age_change_returned=false,golden_age_dirty_attempted=false;std::array<std::int32_t,8> golden_age_operands{};
 std::size_t city_cursor=0;std::vector<std::int64_t> city_order;std::map<std::int64_t,bool> city_garrisons;
 std::size_t promotion_cursor=0;
 std::uint64_t improvement_cursor=0;
 bool combat_attempted=false;std::optional<std::size_t> hurry_frontier;
 bool process_attempted=false,worker_attempted=false,treasury_attempted=false,city_attempted=false,extra_attempted=false,one_shot_attempted=false,yield_attempted=false,map_complete=false;std::uint64_t map_cursor=0;std::optional<BnwNormalPlotYieldRules> normal_plot_rules;std::shared_ptr<BnwPlotYieldRefreshPending> plot_child;std::uint8_t yield_cursor=0;std::map<std::int64_t,PolicyRanks> expected_ranks;std::map<std::pair<std::int64_t,std::int64_t>,std::int64_t> expected_cities;
 std::map<std::int64_t,std::pair<std::int32_t,std::int32_t>> expected_strengths;
 BnwPolicyAdoptionPhase phase=BnwPolicyAdoptionPhase::prepared;
};
BnwPolicyAdoptionPending::BnwPolicyAdoptionPending(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwPolicyAdoptionPending::~BnwPolicyAdoptionPending()=default;
BnwPolicyAdoptionPhase BnwPolicyAdoptionPending::phase()const{need(creator_==std::this_thread::get_id()&&!retired_.load(),"BNW policy receipt foreign thread/retired before context borrow");need(impl_&&!impl_->retired.load(),"BNW policy context retired");return impl_->phase;}
std::uint8_t BnwPolicyAdoptionPending::yield_cursor()const{(void)phase();return impl_->yield_cursor;}
std::optional<std::size_t> BnwPolicyAdoptionPending::hurry_frontier()const{(void)phase();return impl_->hurry_frontier;}
std::uint64_t BnwPolicyAdoptionPending::improvement_yield_cursor()const{(void)phase();return impl_->improvement_cursor;}
std::size_t BnwPolicyAdoptionPending::free_promotion_cursor()const{(void)phase();return impl_->promotion_cursor;}
std::size_t BnwPolicyAdoptionPending::city_effect_cursor()const{(void)phase();return impl_->city_cursor;}
std::map<std::int64_t,bool> BnwPolicyAdoptionPending::observed_city_garrisons()const{(void)phase();return impl_->city_garrisons;}
std::uint8_t BnwPolicyAdoptionPending::reward_cursor()const{(void)phase();return impl_->reward_cursor;}
std::array<std::int32_t,8> BnwPolicyAdoptionPending::golden_age_operands()const{(void)phase();return impl_->golden_age_operands;}
bool BnwPolicyAdoptionPending::golden_age_turns_stored()const{(void)phase();return impl_->golden_age_turns_stored;}
bool BnwPolicyAdoptionPending::golden_age_change_returned()const{(void)phase();return impl_->golden_age_change_returned;}
bool BnwPolicyAdoptionPending::golden_age_map_returned()const{(void)phase();return impl_->golden_age_map_returned;}
bool BnwPolicyAdoptionPending::golden_age_count_stored()const{(void)phase();return impl_->golden_age_count_stored;}
bool BnwPolicyAdoptionPending::golden_age_replay_returned()const{(void)phase();return impl_->golden_age_replay_returned;}
bool BnwPolicyAdoptionPending::golden_age_gameplay_stored()const{(void)phase();return impl_->golden_age_gameplay_stored;}
bool BnwPolicyAdoptionPending::golden_age_gameplay_returned()const{(void)phase();return impl_->golden_age_gameplay_returned;}
bool BnwPolicyAdoptionPending::golden_age_notification_returned()const{(void)phase();return impl_->golden_age_notification_returned;}
bool BnwPolicyAdoptionPending::free_tech_count_stored()const{(void)phase();return impl_->free_tech_count_stored;}
bool BnwPolicyAdoptionPending::free_tech_choice_returned()const{(void)phase();return impl_->free_tech_choice_returned;}
std::array<std::int32_t,4> BnwPolicyAdoptionPending::free_tech_choice_operands()const{(void)phase();return impl_->free_tech_choice_operands;}
bool BnwPolicyAdoptionPending::golden_age_achievement_backend_disabled()const{(void)phase();return impl_->golden_age_achievement_backend_disabled;}
std::uint64_t BnwPolicyAdoptionPending::golden_age_map_scanned()const{(void)phase();return impl_->golden_age_map_scanned;}
std::uint8_t BnwPolicyAdoptionPending::units_reward_cursor()const{(void)phase();return impl_->units_reward_cursor;}
bool BnwPolicyAdoptionPending::reformation_notification_native_returned()const{(void)phase();return impl_->reformation_native_returned;}
bool BnwPolicyAdoptionPending::reformation_notification_returned()const{(void)phase();return impl_->reformation_returned;}
bool BnwPolicyAdoptionPending::reformation_notification_inserted()const{(void)phase();return impl_->reformation_inserted;}
std::shared_ptr<const saves::PlayerAdministrationRecord> BnwPolicyAdoptionPending::reformation_notification_before()const{
 (void)phase();if(!impl_->reformation_notification_owner)return {};return std::make_shared<const saves::PlayerAdministrationRecord>(*impl_->reformation_notification_owner);
}
std::int64_t BnwPolicyAdoptionPending::player()const{(void)phase();return impl_->actor;}
std::int64_t BnwPolicyAdoptionPending::policy()const{(void)phase();return impl_->root_target;}
std::uint64_t BnwPolicyAdoptionPending::before_revision()const{(void)phase();return impl_->before;}
std::uint64_t BnwPolicyAdoptionPending::previous_revision()const{(void)phase();return impl_->previous;}
std::uint64_t BnwPolicyAdoptionPending::after_revision()const{(void)phase();return impl_->after;}

bool BnwPolicyAdoptionPending::observed_earned_native_return_for(const BnwSelectedStockLuaSourceOwner& source)const noexcept{
 // Creator gate precedes weak canonical controls and all mutable fields. No
 // lease/currentness/VM/provider/world getter is called on this error path.
 if(creator_!=std::this_thread::get_id())return false;if(!impl_)return false;
 const auto& p=*impl_;if(p.phase!=BnwPolicyAdoptionPhase::adoption_returned||!p.outer_policy_yield_returned)return false;
 const auto self=p.self.lock();if(!self||self.get()!=this||!p.hooks||!p.hooks->impl_)return false;
 const auto& hooks=*p.hooks->impl_;if(hooks.thread!=p.thread||!hooks.claimed)return false;
 const auto actual_hooks=hooks.self.lock();if(!actual_hooks||actual_hooks.get()!=p.hooks.get()||actual_hooks.owner_before(p.hooks)||p.hooks.owner_before(actual_hooks))return false;
 const auto owner=p.source_owner.lock(),issued=hooks.source_owner.lock();const auto canonical=source.action_self_.lock();
 if(!owner||owner.get()!=&source||!issued||issued.get()!=owner.get()||issued.owner_before(owner)||owner.owner_before(issued)||
    !canonical||canonical.get()!=&source||canonical.owner_before(owner)||owner.owner_before(canonical))return false;
 const auto bootstrap=p.source_bootstrap.lock();
 return bootstrap&&source.bootstrap_&&bootstrap.get()==source.bootstrap_.get()&&!bootstrap.owner_before(source.bootstrap_)&&!source.bootstrap_.owner_before(bootstrap);
 // Only previously earned native return history is observed. A retired VM,
 // changed world, final-source failure or absent renderer ACK stays failed.
}

void BnwPolicyAdoptionPending::require_stored_current(const ui::SelectedBnwPreviewHost& actual)const {
 (void)phase();const auto& p=*impl_;const auto host=p.host.lock();need(host&&host.get()==&actual,"BNW policy actual Host expired/substituted");
 const auto& session=actual.session();need(&session==p.session&&session.read_state_lease()==p.state&&&session.source()==p.source.get()&&session.state_generation()==p.generation&&
  p.state->ui_source_revision==p.after&&p.state->setup.game_turn==p.turn&&p.rules_current&&p.rules_current(),"BNW policy SAME world/rules/revision retired");
 const auto* registry=p.state->setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor)&&registry->players.at(p.actor)==p.expected&&
  (p.phase==BnwPolicyAdoptionPhase::adoption_returned?!registry->pending_policy.contains(p.actor):(registry->pending_policy.contains(p.actor)&&registry->pending_policy.at(p.actor)==p.root_target))&&read_native(*p.state,p.actor,p.rules)==p.expected&&p.state->setup.players.at(p.actor).policies_resolved&&p.state->setup.players.at(p.actor).policies==has_set(p.expected),"BNW policy raw/owned prefix changed");
 for(const auto& [key,value]:p.expected_cities){const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,static_cast<std::int32_t>(key.first),static_cast<std::int32_t>(key.second));need(city&&named(city->core.fields,"m_iJONSCulturePerTurnFromPolicies").values.size()==1&&named(city->core.fields,"m_iJONSCulturePerTurnFromPolicies").values[0]==value&&registry->city_policy_culture.contains(key)&&registry->city_policy_culture.at(key)==value,"BNW policy process city raw/owned cache changed");}

 for(const auto& [id,value]:p.expected_strengths){const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id));
  const auto* stored=p.state->cities.players.at(p.actor).cities.at(id).extensions.find<BnwMinorCityStrengthScratch::LiveStored>();
  need(city&&city_scalar(*city,"m_iStrengthValue")==value.first&&stored&&stored->strength==value.first&&stored->known_technologies==value.second&&stored->generation==p.generation,
   "BNW policy actual paired city strength cache changed");}
 for(const auto& [id,value]:p.expected_ranks){const auto* city=resolve_bnw_city_registry(p.state->world,p.state->cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id));
  need(city&&policy_rank_array(*city,"m_abBaseYieldRankValid").values==value.first&&policy_rank_array(*city,"m_abYieldRankValid").values==value.second&&registry->city_yield_ranks.contains({p.actor,id})&&registry->city_yield_ranks.at({p.actor,id})==value,"BNW policy paired city rank validity changed");}
 if(p.camp_changed_plot){
  need(p.state->world.map_plots&&*p.camp_changed_plot>=0&&static_cast<std::uint64_t>(*p.camp_changed_plot)==p.camp_cursor,"BNW policy camp native retained plot owner changed");
  const auto& plot=p.state->world.map_plots->plots.at(static_cast<std::size_t>(*p.camp_changed_plot));const auto& array=named(plot.arrays,p.camp_array_name);
  need(plot.index==p.camp_cursor&&array.columns==1&&array.values.size()==80&&p.camp_team>=0&&p.camp_team<64&&array.values.at(static_cast<std::size_t>(p.camp_team))==(p.camp_store_completed?p.camp_after_wire:p.camp_before_wire),"BNW policy earned revealed improvement cache changed");
 }
 if(!p.trade_source.empty()){need(bnw_player_trade_registry_revision(*p.state,p.actor)==p.trade_source,"BNW policy true unchanged trade scan source changed");}
 if(!p.happiness_stores.empty()){need(p.state->cities.players.at(p.actor).excess_happiness==p.happiness_expected_excess,"BNW policy derived happiness cache changed");}
 need(session.read_state_lease()==p.state&&session.state_generation()==p.generation&&p.state->ui_source_revision==p.after,"BNW policy world changed during stored proof");
}
std::shared_ptr<const BnwPolicyAdoptionPending> BnwPolicyAdoptionPending::execute(LiveSession& session,std::shared_ptr<const BnwPolicyAdoptionHooks> supplied,const BnwPolicyAdoptionCommand& command){
 std::shared_ptr<const BnwPolicyAdoptionPending> issued_receipt;
 return execute_owned(session,std::move(supplied),command,issued_receipt);
}
std::shared_ptr<const BnwPolicyAdoptionPending> BnwPolicyAdoptionPending::execute_owned(LiveSession& session,std::shared_ptr<const BnwPolicyAdoptionHooks> supplied,const BnwPolicyAdoptionCommand& command,std::shared_ptr<const BnwPolicyAdoptionPending>& issued_receipt){
 need(bool(supplied),"BNW policy genuine SourceOwner hooks absent");auto& binding=*supplied->impl_;
 need(binding.thread==std::this_thread::get_id(),"BNW policy hooks foreign thread before world borrow");
 need(!issued_receipt,"BNW policy issued output sink already owns another attempt");const auto hooks=binding.self.lock();
 need(hooks&&hooks.get()==supplied.get()&&!hooks.owner_before(supplied)&&!supplied.owner_before(hooks),"BNW policy forged hook control");
 const auto host=binding.host.lock();need(host&&&host->session()==&session&&!binding.claimed,"BNW policy SAME Host/once operation unavailable");
 binding.lease.require_operation(binding.serial);need(binding.lease.world_lease()==session.read_state_lease(),"BNW policy operation world differs");
 const auto source_owner=binding.source_owner.lock();need(bool(source_owner),"BNW policy closed SourceOwner issuer expired");
 const auto canonical_source=source_owner->action_self_.lock();
 need(canonical_source&&canonical_source.get()==source_owner.get()&&!canonical_source.owner_before(source_owner)&&!source_owner.owner_before(canonical_source)&&source_owner->bootstrap_,"BNW policy issuer is not its actual SourceOwner factory/bootstrap");
 auto p=std::make_unique<Impl>();p->session=&session;p->host=host;p->hooks=hooks;p->state=session.state_;p->source=session.source_;p->generation=session.state_generation_;
 p->source_owner=source_owner;p->source_bootstrap=source_owner->bootstrap_;
 auto& state=*p->state;p->before=state.ui_source_revision;p->previous=p->before;p->after=p->before;p->turn=state.setup.game_turn;p->actor=command.player;p->target=command.policy;p->root_target=command.policy;
 p->rules_current=host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current()&&host->types().report().all_succeeded()&&state.setup.profile==content::TechnologyProfile::bnw&&
  command.expected_generation==p->generation&&command.expected_turn==p->turn&&command.expected_ui_source_revision==p->before&&p->before<UINT64_MAX&&
  command.player==host->active_player_id()&&command.player==detail::bnw_game_active_player_read(state)&&state.setup.players.at(command.player).alive&&
  detail::bnw_player_human_read(state,static_cast<std::int32_t>(command.player),host->database()),"BNW policy stale/nonactive/nonhuman/invalid rules command");
 p->rules=read_rules(*host);need(p->rules.policies.contains(command.policy),"BNW policy choice absent from rules");
 const auto before=read_native(state,command.player,p->rules);p->expected=before;
 const auto* existing=state.setup.extensions.find<PolicyRegistry>();need(!existing||!existing->pending_policy.contains(p->actor),"BNW policy previous native adoption suffix pending");
 const auto known=ui::preview_bnw_policy_choice(session,host->database(),host->technologies(),host->defines(),command.player,command.policy);
 need(known.passes_known_rules,"BNW policy native prerequisites/cost/branch unavailable");
 const auto require_before=[&]{need(session.state_==p->state&&session.source_==p->source&&session.state_generation_==p->generation&&state.ui_source_revision==p->before&&
  p->rules_current()&&read_native(state,p->actor,p->rules)==before,"BNW policy inputs changed during CanAdoptPolicy");binding.lease.require_operation(binding.serial);};
 require_before();binding.claimed=true;const auto allowed=binding.can(command.player,command.policy);require_before();
 if(allowed&&!*allowed){p->phase=BnwPolicyAdoptionPhase::denied;auto denied=std::shared_ptr<const BnwPolicyAdoptionPending>(new BnwPolicyAdoptionPending(std::move(p)));denied->impl_->self=denied;issued_receipt=denied;return denied;}
 // All allocation and overflow checks precede the first paired native store.
 auto registry=state.setup.extensions.find<PolicyRegistry>()?*state.setup.extensions.find<PolicyRegistry>():PolicyRegistry{};
 if(registry.players.contains(p->actor))need(registry.players.at(p->actor)==before,"BNW policy prior owned/native state differs");
 auto next=before;const auto& rule=p->rules.policies.at(p->target);const auto* branch=host->types().find(rule.branch);
 need(branch&&branch->table=="PolicyBranchTypes"&&!next.has.at(p->target)&&next.count<INT32_MAX,"BNW policy branch/count/ownership invalid");
 const auto branch_id=branch->id;
 bool paid_culture=false;
 if(rule.level>0&&next.free_tenets>0)--next.free_tenets;
 else if(next.free_policies>0)--next.free_policies;
 else{need(next.culture>=next.cost,"BNW policy culture became insufficient");next.culture-=next.cost;paid_culture=next.cost!=0;}
 next.has.at(p->target)=true;++next.count;
 const auto vacant=std::find(next.history.begin(),next.history.end(),-1);
 if(vacant!=next.history.end())*vacant=branch_id;else{next.history[2]=next.history[1];next.history[1]=next.history[0];next.history[0]=branch_id;}
 bool finished=true;for(const auto& [id,value]:p->rules.policies)if(value.branch==rule.branch&&!next.has.at(id)){finished=false;break;}
 next.finished.at(branch_id)=finished;
 auto& raw=raw_player(state,p->actor);auto& block=named(raw.strategy.blocks,"policies");
 auto staged_has=named(block.hashes,"has_policy"),staged_finished=named(block.hashes,"branch_finished");
 set_flag(staged_has,rule.hash,true);set_flag(staged_finished,branch->hash,finished);
 auto desired_has=has_set(next);p->after=p->before+1;
 const auto queue=[&](BnwPolicyUiEffect effect,bool value){need(registry.next_occurrence<UINT64_MAX,"BNW policy UI occurrence exhausted");registry.ui.push_back({registry.next_occurrence++,p->generation,p->after,p->actor,effect,value});};
 if(paid_culture){queue(BnwPolicyUiEffect::game_data,true);}
 queue(BnwPolicyUiEffect::policy_notification_seen,false);
 registry.pending_policy.emplace(p->actor,p->target);
 registry.players.insert_or_assign(p->actor,next);p->expected=next;
 std::array<std::int64_t,3> history_hashes{};for(std::size_t i=0;i<3;++i){if(next.history[i]<0)continue;const auto it=std::find_if(p->rules.branch_hashes.begin(),p->rules.branch_hashes.end(),[&](const auto& v){return v.second==next.history[i];});need(it!=p->rules.branch_hashes.end(),"BNW policy history branch disappeared");history_hashes[i]=static_cast<std::int64_t>(it->first);}
 const auto cell=[&](const char* key)->std::int64_t&{auto& v=named(raw.core.fields,key);need(v.values.size()==1,"BNW policy raw scalar storage changed");return v.values[0];};
 auto* culture=&cell("m_iJONSCulture");auto* free_policies=&cell("m_iNumFreePolicies");auto* free_tenets=&cell("m_iNumFreeTenets");auto* count=&cell("m_iNumPolicies");
 std::array<saves::NamedSaveInteger*,3> history{};for(std::size_t i=0;i<3;++i)history[i]=&named(block.fields,"branch_picked_"+std::to_string(i+1)+"_hash");
 auto& has=named(block.hashes,"has_policy");auto& branch_finished=named(block.hashes,"branch_finished");
 const auto actor=p->actor;const auto after_revision=p->after;
 require_before();
 // Allocate the opaque receipt/control before the stores as well. No failed
 // shared-control allocation can leave an unrecorded native payment behind.
 auto out=std::shared_ptr<const BnwPolicyAdoptionPending>(new BnwPolicyAdoptionPending(std::move(p)));out->impl_->self=out;
 // Shared-control copy cannot throw and occurs before cache installation or
 // the first native effect. Issuance itself is never proof of a store/return.
 issued_receipt=out;
 // Install real owned native cache/transport storage while no raw field has
 // changed. emplace may allocate and throw; all following stores are no-throw.
 state.setup.extensions.emplace<PolicyRegistry>(std::move(registry));
 *culture=next.culture;*free_policies=next.free_policies;*free_tenets=next.free_tenets;
 has.rows.swap(staged_has.rows);has.wire_count=staged_has.wire_count;*count=next.count;
 for(std::size_t i=0;i<3;++i)history[i]->value=history_hashes[i];
 branch_finished.rows.swap(staged_finished.rows);branch_finished.wire_count=staged_finished.wire_count;
 state.setup.players.at(actor).policies.swap(desired_has);state.setup.players.at(actor).policies_resolved=true;
 state.ui_source_revision=after_revision;out->impl_->phase=BnwPolicyAdoptionPhase::uncertain;
 try{out->require_stored_current(*host);binding.accept(*out);binding.lease.require_operation(binding.serial);out->require_stored_current(*host);
  out->impl_->phase=finished?BnwPolicyAdoptionPhase::branch_finisher_pending:BnwPolicyAdoptionPhase::process_policies_pending;
 }catch(...){out->impl_->phase=BnwPolicyAdoptionPhase::uncertain;throw;}
 return out;
}

std::optional<std::int64_t> BnwPolicyAdoptionPending::finishing_policy()const{(void)phase();return impl_->finishing_policy;}
bool BnwPolicyAdoptionPending::finisher_granted()const{(void)phase();return impl_->finisher_granted;}
bool BnwPolicyAdoptionPending::finisher_ever_stored()const{(void)phase();return impl_->finisher_ever_stored;}
bool BnwPolicyAdoptionPending::finisher_returned()const{(void)phase();return impl_->finisher_returned;}
std::int64_t BnwPolicyAdoptionPending::effect_policy()const{(void)phase();return impl_->target;}
std::int64_t BnwPolicyAdoptionPending::observed_effect_policy()const{return effect_policy();}
std::optional<BnwPolicyNativeObservation> BnwPolicyAdoptionPending::finisher_process_before()const{
 (void)phase();const auto& p=*impl_;return p.completed_finisher?p.completed_finisher->finisher_process_before:p.finisher_process_before;
}
std::optional<BnwPolicyNativeObservation> BnwPolicyAdoptionPending::finisher_process_after()const{
 (void)phase();const auto& p=*impl_;return p.completed_finisher?p.completed_finisher->finisher_process_after:p.finisher_process_after;
}
void BnwPolicyAdoptionPending::advance_branch_finisher()const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::branch_finisher_pending&&!p.finisher_attempted&&!p.is_finisher&&!p.suspended_parent&&!p.completed_finisher,
  "BNW actual branch finishing policy unreached/repeated");
 p.finisher_attempted=true;p.phase=BnwPolicyAdoptionPhase::branch_finisher_executing;
 try{
  const auto host=p.host.lock();need(bool(host),"BNW branch finisher canonical Host expired");require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
  const auto selected=p.rules.policies.at(p.target);const auto* branch=host->types().find(selected.branch);
  need(branch&&branch->table=="PolicyBranchTypes"&&p.expected.finished.at(branch->id),"BNW finisher requires genuine already-finished current branch");
  for(const auto& [id,rule]:p.rules.policies)if(rule.branch==selected.branch)need(p.expected.has.at(id),"BNW branch finisher preceded an unowned branch policy");
  std::optional<std::int64_t> finisher;std::size_t found=0;
  const auto report=host->database().prefetch_info("PolicyBranchTypes",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(branch->id)){
    ++found;need(row.text("Type").value_or("")==selected.branch,"BNW finisher actual BranchInfo remap differs");const auto type=row.text("FreeFinishingPolicy").value_or("");
    if(!type.empty()){const auto* mapped=host->types().find(type);need(mapped&&mapped->table=="Policies"&&p.rules.policies.contains(mapped->id)&&p.rules.policies.at(mapped->id).type==type,
     "BNW finisher FreeFinishingPolicy actual native Info unresolved");finisher=mapped->id;}
   }return true;
  });need(report.all_succeeded()&&found==1,"BNW finisher complete selected BranchInfo absent");
  auto& state=*p.state;const auto services=state.standalone_services;saves::require_supported_standalone_services(services);
  need(services.steam_achievements==saves::StandaloneServiceBackend::backend_disabled,"BNW finished branch achievement backend unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);need(state.standalone_services==services,"BNW finished branch explicit achievement backend changed");
  p.branch_achievement_backend_disabled=true;p.finishing_policy=finisher;
  if(!finisher){p.finisher_returned=true;p.phase=BnwPolicyAdoptionPhase::process_policies_pending;return;}
  need(*finisher!=p.target&&p.rules.policies.at(*finisher).branch.empty(),"BNW branch-recursive/cyclic finishing policy requires its genuine nested branch owner");
  if(p.expected.has.at(*finisher)){
   // Real setHasPolicy(true) no-op still returns to SetPolicy's unconditional
   // ChangeNumFreePoliciesEver(1). Do not invent another processPolicies call.
   p.phase=BnwPolicyAdoptionPhase::finisher_process_returned;resume_after_finisher_process();return;
  }
  auto child=std::make_unique<Impl>();child->self=p.self;child->session=p.session;child->host=p.host;child->hooks=p.hooks;child->state=p.state;child->source=p.source;
  child->source_owner=p.source_owner;child->source_bootstrap=p.source_bootstrap;child->generation=p.generation;child->rules_current=p.rules_current;child->rules=p.rules;
  child->actor=p.actor;child->target=*finisher;child->root_target=p.root_target;child->turn=p.turn;child->before=p.before;child->previous=p.after;
  child->is_finisher=true;child->finisher_attempted=true;child->finishing_policy=finisher;child->branch_achievement_backend_disabled=true;
  child->expected_cities=p.expected_cities;child->expected_strengths=p.expected_strengths;child->expected_ranks=p.expected_ranks;
  auto desired=p.expected;desired.has.at(*finisher)=true;desired.count=native_sum(desired.count,1);
  // SDK DoNewPolicyPickedForHistory maps only free branch OPENERS. A genuine
  // branchless finishing policy normally records NO_BRANCH, including shifting
  // a full history. Resolve the actual first opener match rather than guessing.
  std::int64_t history_branch=-1;
  const auto history_info=host->database().prefetch_info("PolicyBranchTypes",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(history_branch<0&&row.text("FreePolicy").value_or("")==p.rules.policies.at(*finisher).type){
    const auto* mapped=host->types().find(row.text("Type").value_or(""));need(slot<=INT32_MAX&&mapped&&mapped->table=="PolicyBranchTypes"&&mapped->id==static_cast<std::int64_t>(slot),"BNW finisher history native branch/opener remap differs");history_branch=mapped->id;
   }return true;
  });need(history_info.all_succeeded(),"BNW finisher original opener history lookup failed");
  const auto empty=std::find(desired.history.begin(),desired.history.end(),-1);
  if(empty!=desired.history.end())*empty=history_branch;else{desired.history[2]=desired.history[1];desired.history[1]=desired.history[0];desired.history[0]=history_branch;}
  auto& raw=raw_player(state,p.actor);auto& block=named(raw.strategy.blocks,"policies");auto staged=named(block.hashes,"has_policy");set_flag(staged,p.rules.policies.at(*finisher).hash,true);
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.at(p.actor)==p.expected,"BNW finisher true prior owned/native policy pair absent");
  auto prepared=*registry;prepared.players.at(p.actor)=desired;need(p.after<UINT64_MAX&&prepared.next_occurrence<UINT64_MAX,"BNW finisher native/UI occurrence exhausted");
  child->after=p.after+1;prepared.ui.push_back({prepared.next_occurrence++,p.generation,child->after,p.actor,BnwPolicyUiEffect::policy_notification_seen,false});
  child->expected=desired;auto desired_has=has_set(desired);std::array<std::int64_t,3> history_hashes{};
  for(std::size_t k=0;k<history_hashes.size();++k)if(desired.history[k]>=0){
   const auto mapped=std::find_if(p.rules.branch_hashes.begin(),p.rules.branch_hashes.end(),[&](const auto& v){return v.second==desired.history[k];});
   need(mapped!=p.rules.branch_hashes.end(),"BNW finisher history hash absent");history_hashes[k]=static_cast<std::int64_t>(mapped->first);
  }
  require_stored_current(*host);binding.lease.require_operation(binding.serial);need(state.standalone_services==services,"BNW finisher explicit local backend changed before grant");
  // Reacquire destinations after ALL providers. The frame switch and native
  // writes below contain no callback, allocation or destructor of old state.
  auto& final_raw=raw_player(state,p.actor);auto& final_block=named(final_raw.strategy.blocks,"policies");auto& has=named(final_block.hashes,"has_policy");auto& count=named(final_raw.core.fields,"m_iNumPolicies");
  auto* final_registry=state.setup.extensions.find<PolicyRegistry>();need(final_registry&&final_registry->players.at(p.actor)==p.expected&&count.values.size()==1&&count.values[0]==p.expected.count&&flags(has,p.rules.policy_hashes)==p.expected.has,
   "BNW finisher final HasPolicy/count source changed");
  std::array<saves::NamedSaveInteger*,3> history{};for(std::size_t k=0;k<3;++k)history[k]=&named(final_block.fields,"branch_picked_"+std::to_string(k+1)+"_hash");
  child->suspended_parent=std::move(impl_);impl_=std::move(child);auto& current=*impl_;
  *final_registry=std::move(prepared);has.rows.swap(staged.rows);has.wire_count=staged.wire_count;count.values[0]=desired.count;
  for(std::size_t k=0;k<3;++k)history[k]->value=history_hashes[k];state.setup.players.at(current.actor).policies.swap(desired_has);state.setup.players.at(current.actor).policies_resolved=true;
  state.ui_source_revision=current.after;current.finisher_granted=true;current.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  current.finisher_process_before=observe_bnw_policy_native(*host,current.actor);require_stored_current(*host);binding.lease.require_operation(binding.serial);
  current.phase=BnwPolicyAdoptionPhase::process_policies_pending;
 }catch(...){impl_->phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::resume_after_finisher_process()const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::finisher_process_returned&&p.finisher_attempted&&p.finishing_policy&&!p.finisher_ever_stored&&!p.finisher_returned&&
  (p.is_finisher?(p.suspended_parent&&(p.process_policies_returned||p.early_process_policies_returned)):(!p.suspended_parent&&p.expected.has.at(*p.finishing_policy))),
  "BNW finisher continuation requires genuine setHasPolicy/process return");
 try{
  const auto host=p.host.lock();need(bool(host),"BNW finisher returned Host expired");require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);auto& state=*p.state;
  need(p.branch_achievement_backend_disabled&&state.standalone_services.steam_achievements==saves::StandaloneServiceBackend::backend_disabled,"BNW finisher optional achievement backend changed");
  const auto old=p.expected.process_scalars.at("m_iNumFreePoliciesEver"),value=native_sum(old,1);auto desired=p.expected;desired.process_scalars.at("m_iNumFreePoliciesEver")=value;
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.at(p.actor)==p.expected,"BNW finisher Ever true prior owned/native pair absent");auto prepared=*registry;prepared.players.at(p.actor)=desired;auto expected=desired;
  const auto before=p.after;need(before<UINT64_MAX,"BNW finisher Ever revision exhausted");const auto after=before+1;
  // Prepare all parent-cache copies before the first scalar write. The actual
  // child frame remains active through Source/Selection receiver bookends.
  if(p.is_finisher){auto& parent=*p.suspended_parent;parent.expected=desired;parent.expected_cities=p.expected_cities;parent.expected_strengths=p.expected_strengths;parent.expected_ranks=p.expected_ranks;}
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& final_raw=raw_player(state,p.actor);auto& field=named(final_raw.core.fields,"m_iNumFreePoliciesEver");auto* final_registry=state.setup.extensions.find<PolicyRegistry>();
  need(final_registry&&final_registry->players.at(p.actor)==p.expected&&field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&field.values[0]==old&&state.ui_source_revision==before,
   "BNW finisher Ever final native destination changed");
  *final_registry=std::move(prepared);field.values[0]=value;p.expected=std::move(expected);p.previous=before;p.after=after;state.ui_source_revision=after;p.finisher_ever_stored=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  if(p.is_finisher){
   p.finisher_process_after=observe_bnw_policy_native(*host,p.actor);require_stored_current(*host);binding.lease.require_operation(binding.serial);
   auto child=std::move(impl_);auto parent=std::move(child->suspended_parent);
   parent->previous=child->previous;parent->after=child->after;parent->finisher_granted=child->finisher_granted;parent->finisher_ever_stored=true;parent->finisher_returned=true;
   parent->completed_finisher=std::move(child);parent->phase=BnwPolicyAdoptionPhase::process_policies_pending;impl_=std::move(parent);
  }else{
   // The actual already-owned setHasPolicy no-op still earns Ever+1. Preserve
   // that measured native baseline separately from the original paid choice.
   p.finisher_process_after=observe_bnw_policy_native(*host,p.actor);require_stored_current(*host);binding.lease.require_operation(binding.serial);
   p.finisher_returned=true;p.phase=BnwPolicyAdoptionPhase::process_policies_pending;
  }
 }catch(...){impl_->phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_initial_scalars()const {
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::process_policies_pending&&!p.process_attempted,"BNW policy process prefix replay/unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;
 binding.lease.require_operation(binding.serial);p.process_attempted=true;
 try {
  std::array<std::int64_t,5> changes{};std::size_t matches=0;
  const auto rule=p.rules.policies.at(p.target);
  const auto report=host->database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(p.target)){
    need(row.text("Type").value_or("")==rule.type,"BNW policy scalar current Info changed");++matches;
    constexpr std::array<const char*,5> columns{"CulturePerWonder","CultureWonderMultiplier","CulturePerTechResearched","GoldenAgeMeterMod","GoldenAgeDurationMod"};
    for(std::size_t i=0;i<changes.size();++i){changes[i]=row.integer(columns[i]);need(changes[i]>=INT32_MIN&&changes[i]<=INT32_MAX,"BNW policy native modifier outside int32");}
   }return true;
  });
  need(report.all_succeeded()&&matches==1,"BNW policy scalar SQL/current entry incomplete");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy owned process cache absent");
  auto desired=p.expected;
  constexpr std::array<const char*,5> members{"m_iCulturePerWonder","m_iCultureWonderMultiplier","m_iCulturePerTechResearched","m_iGoldenAgeMeterMod","m_iGoldenAgeModifier"};
  std::array<std::int64_t*,5> fields{};std::array<std::int64_t,5> computed_sums{};
  for(std::size_t i=0;i<fields.size();++i){
   auto& field=named(raw.core.fields,members[i]);need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1,"BNW policy process raw scalar shape differs");
   fields[i]=&field.values[0];need(*fields[i]==desired.process_scalars.at(members[i]),"BNW policy process native/owned scalar differs");
   const auto sum=*fields[i]+changes[i];need(sum>=INT32_MIN&&sum<=INT32_MAX,"BNW policy modifier addition overflows native int");desired.process_scalars.at(members[i])=sum;computed_sums[i]=sum;
  }
  struct CityWrite {saves::SavedCityObservation* raw=nullptr;std::int64_t id=-1,slot=-1,before=0,after=0,wonders=0;std::int64_t* field=nullptr;};
  std::vector<CityWrite> cities;
  auto next_city_cache=registry->city_policy_culture;
  if(changes[0]!=0){
   // Original firstCity/nextCity ascends real FLTA slots, not owned map keys or
   // incidental serialization vector order. Existing native resolver validates
   // the complete raw/owned FLTA roster for EVERY reached city.
   const auto& owned=state.cities.players.at(p.actor).cities;
   need(raw.cities.cities.size()==owned.size(),"BNW policy city native/owned roster differs");
   const auto& header=raw.cities.header;
   const auto last=named(header.fields,"last_index").value,free=named(header.fields,"free_list_count").value;
   need(raw.cities.layout==saves::GameStateLayout::expansion2_version1&&header.layout==raw.cities.layout&&
    header.wire_city_count==static_cast<std::int64_t>(raw.cities.cities.size())&&last>=-1&&last<=INT32_MAX&&free>=0&&free<=INT32_MAX&&
    last-free+1==static_cast<std::int64_t>(owned.size())&&named(header.fields,"num_slots").value==static_cast<std::int64_t>(header.source_slots),
    "BNW policy native firstCity/nextCity roster unavailable");
   for(auto& city:raw.cities.cities){
    auto& id_field=named(city.core.fields,"m_iID");need(id_field.encoding==saves::CityCoreEncoding::i32&&id_field.values.size()==1&&id_field.values[0]>=0&&id_field.values[0]<=INT32_MAX,"BNW policy city real ID unavailable");
    const auto id=id_field.values[0];const auto* actual=resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id));
    need(actual==&city,"BNW policy city genuine native slot differs");
    auto& f=named(city.core.fields,"m_iJONSCulturePerTurnFromPolicies");const auto& w=named(city.core.fields,"m_iNumWorldWonders");
    need(f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&w.encoding==saves::CityCoreEncoding::i32&&w.values.size()==1&&
     f.values[0]>=INT32_MIN&&f.values[0]<=INT32_MAX&&w.values[0]>=0&&w.values[0]<=INT32_MAX,"BNW policy city culture/wonder native shape differs");
    const auto delta=w.values[0]*changes[0];need(delta>=INT32_MIN&&delta<=INT32_MAX,"BNW policy wonder culture native product overflow");
    const auto sum=f.values[0]+delta;need(sum>=INT32_MIN&&sum<=INT32_MAX,"BNW policy city culture native addition overflow");
    const auto key=std::pair{p.actor,id};if(next_city_cache.contains(key))need(next_city_cache.at(key)==f.values[0],"BNW policy prior city culture cache differs");
    next_city_cache.insert_or_assign(key,sum);cities.push_back({&city,id,id&8191,f.values[0],sum,w.values[0],&f.values[0]});
   }
   std::sort(cities.begin(),cities.end(),[](const CityWrite& a,const CityWrite& z){return a.slot<z.slot;});
   for(std::size_t i=1;i<cities.size();++i)need(cities[i-1].slot!=cities[i].slot,"BNW policy city FLTA duplicate slot");
  }
  const bool changed=std::any_of(changes.begin(),changes.end(),[](std::int64_t value){return value!=0;});
  const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy process revision exhausted");const auto after=before+(changed?1u:0u);
  // Stage ALL owned cache allocations before writes. Core loops below contain
  // no callbacks, allocations or unknown setters. The native culture city
  // setter itself is a guarded scalar addition (CvCity7582), not updateYield.
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;next_registry.city_policy_culture=std::move(next_city_cache);
  auto expected_copy=desired;auto expected_cities=next_registry.city_policy_culture;
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  for(const auto& city:cities)need(resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(city.id))==city.raw&&
   named(city.raw->core.fields,"m_iJONSCulturePerTurnFromPolicies").values.data()==city.field&&*city.field==city.before&&
   named(city.raw->core.fields,"m_iNumWorldWonders").values[0]==city.wonders,"BNW policy city source changed before stores");
  for(std::size_t i=0;i<fields.size();++i)need(named(raw.core.fields,members[i]).values.data()==fields[i]&&*fields[i]==p.expected.process_scalars.at(members[i]),"BNW policy source scalar changed before stores");
  *registry=std::move(next_registry);
  // CvPlayer20923: CulturePerWonder store precedes ALL actual city setters.
  *fields[0]=computed_sums[0];for(const auto& city:cities)if(city.after!=city.before)*city.field=city.after;
  *fields[1]=computed_sums[1];*fields[2]=computed_sums[2];
  *fields[3]=computed_sums[3];*fields[4]=computed_sums[4];
  p.expected=std::move(expected_copy);p.expected_cities=std::move(expected_cities);p.previous=before;p.after=after;state.ui_source_revision=after;
  p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=BnwPolicyAdoptionPhase::worker_speed_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_worker_scalars()const {
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::worker_speed_pending&&!p.worker_attempted,"BNW policy worker effects replay/unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;
 binding.lease.require_operation(binding.serial);p.worker_attempted=true;
 try {
  std::array<std::int64_t,worker_policy_scalars.size()> changes{},sums{};
  std::size_t matches=0;const auto rule=p.rules.policies.at(p.target);
  const auto report=host->database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(p.target)){
    need(row.text("Type").value_or("")==rule.type,"BNW worker policy current Info changed");++matches;
    for(std::size_t i=0;i<changes.size();++i){
     changes[i]=row.integer(worker_policy_scalars[i].first);
     need(changes[i]>=INT32_MIN&&changes[i]<=INT32_MAX,"BNW worker policy modifier outside native int");
     if(i==18||i==24)need(changes[i]==0||changes[i]==1,"BNW policy native boolean SQL value unsupported");
    }
   }return true;
  });
  need(report.all_succeeded()&&matches==1,"BNW worker policy complete SQL/entry unavailable");
  // The SDK SetWoundedUnitDamageMod and SetUnitUpgradeCostMod clamps execute
  // even on zero change. A cached scalar is never a substitute for these calls.
  const auto wounded=policy_define(*host,"WOUNDED_DAMAGE_MULTIPLIER");
  need(wounded!=INT32_MIN,"BNW wounded damage native negation overflows");
  const auto upgrade=policy_define(*host,"UNIT_UPGRADE_COST_DISCOUNT_MAX");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy worker owned cache missing");
  auto desired=p.expected;std::array<std::int64_t*,worker_policy_scalars.size()> fields{};
  auto& maintenance=state.maintenance.players.at(p.actor);auto& unhappy=state.unhappiness.players.at(p.actor);
  const std::array<std::pair<std::size_t,std::int64_t*>,6> shared_fields{{
   {5,&maintenance.base_free},{10,&unhappy.units_modifier},{11,&unhappy.population_modifier},
   {12,&unhappy.cities_modifier},{13,&unhappy.occupied_modifier},{14,&unhappy.capital_modifier}}};

  for(std::size_t i=0;i<fields.size();++i){
   const auto member=worker_policy_scalars[i].second;auto& field=named(raw.core.fields,member);
   need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&field.values[0]==p.expected.process_scalars.at(member),
    "BNW policy worker raw/owned scalar differs");fields[i]=&field.values[0];
   const auto sum=*fields[i]+changes[i];need(sum>=INT32_MIN&&sum<=INT32_MAX,"BNW policy worker native addition overflow");
   sums[i]=sum;if(i==15)sums[i]=std::max(sum,-wounded);if(i==16)sums[i]=std::max(sum,upgrade);
   desired.process_scalars.at(member)=sums[i];
  }
  bool changed=false;for(std::size_t i=0;i<fields.size();++i)changed=changed||sums[i]!=*fields[i];
  const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy worker revision exhausted");const auto after=before+(changed?1u:0u);
  // changeBaseFreeUnits emits GameData(true) immediately after its actual
  // nonzero addition for the active player. The transport storage is allocated
  // first and published at that exact point, before the happiness counters.
  auto next_ui=registry->ui;auto next_occurrence=registry->next_occurrence;
  const bool base_dirty=changes[5]!=0&&detail::bnw_game_active_player_read(state)==p.actor;
  if(base_dirty){need(next_occurrence<UINT64_MAX,"BNW policy worker UI occurrence exhausted");
   next_ui.push_back({next_occurrence++,p.generation,after,p.actor,BnwPolicyUiEffect::game_data,true});}
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  for(std::size_t i=0;i<fields.size();++i)need(named(raw.core.fields,worker_policy_scalars[i].second).values.data()==fields[i]&&
   *fields[i]==p.expected.process_scalars.at(worker_policy_scalars[i].second),"BNW worker policy source changed before stores");
  // All potentially throwing lookups/allocations and all native-width checks
  // have finished. The following SAME-state section has no callback or clone.
  *registry=std::move(next_registry);
  for(std::size_t i=0;i<fields.size();++i){
   *fields[i]=sums[i];
   for(const auto& shared:shared_fields)if(shared.first==i)*shared.second=sums[i];
   if(i==24)maintenance.free_garrison=sums[i]>0;
   if(i==5&&base_dirty){registry->ui.swap(next_ui);registry->next_occurrence=next_occurrence;}
  }
  p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;
  p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=BnwPolicyAdoptionPhase::treasury_route_gold_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_treasury_scalars()const {
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::treasury_route_gold_pending&&!p.treasury_attempted,"BNW policy treasury effects replay/unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;
 binding.lease.require_operation(binding.serial);p.treasury_attempted=true;
 try {
  std::array<std::int64_t,3> modifiers{};std::size_t matches=0;const auto rule=p.rules.policies.at(p.target);
  const auto report=host->database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(p.target)){need(row.text("Type").value_or("")==rule.type,"BNW treasury policy Info changed");++matches;
    constexpr std::array<const char*,3> columns{"CityConnectionTradeRouteGoldModifier","GoldPerUnit","GoldPerMilitaryUnit"};
    for(std::size_t i=0;i<3;++i){modifiers[i]=row.integer(columns[i]);need(modifiers[i]>=INT32_MIN&&modifiers[i]<=INT32_MAX,"BNW treasury policy modifier not native int");}}
   return true;
  });
  need(report.all_succeeded()&&matches==1,"BNW treasury policy complete actual SQL unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto& treasury=state.treasury.players.at(p.actor);auto& maintenance=state.maintenance.players.at(p.actor);
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy treasury cache unavailable");
  auto& block=named(raw.administration.blocks,"treasury");
  auto* modifier_field=&named(block.fields,"m_iCityConnectionTradeRouteGoldModifier").value;
  auto* gold_field=&named(block.fields,"m_iCityConnectionGoldTimes100").value;
  auto* per_unit=&named(raw.core.fields,"m_iGoldPerUnit").values.at(0);
  auto* military=&named(raw.core.fields,"m_iGoldPerMilitaryUnit").values.at(0);
  const auto new_modifier=native_sum(*modifier_field,modifiers[0]);
  const auto new_gold=modifiers[0]!=0?current_connection_gold(state,p.actor,*host,new_modifier,treasury.connection_change):*gold_field;
  const auto unit_change=native_product(modifiers[1],100);
  const auto new_unit=native_sum(*per_unit,unit_change),new_military=native_sum(*military,modifiers[2]);
  auto desired=p.expected;desired.process_scalars.at("m_iCityConnectionTradeRouteGoldModifier")=new_modifier;
  desired.process_scalars.at("m_iCityConnectionGoldTimes100")=new_gold;
  desired.process_scalars.at("m_iGoldPerUnit")=new_unit;desired.process_scalars.at("m_iGoldPerMilitaryUnit")=new_military;
  const bool changed=new_modifier!=*modifier_field||new_gold!=*gold_field||new_unit!=*per_unit||new_military!=*military;
  const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy treasury revision exhausted");const auto after=before+(changed?1u:0u);
  auto unit_ui=registry->ui;auto military_ui=registry->ui;auto occurrence=registry->next_occurrence;
  const bool active=detail::bnw_game_active_player_read(state)==p.actor;
  if(unit_change!=0&&active){need(occurrence<UINT64_MAX,"BNW policy gold/unit UI occurrence exhausted");unit_ui.push_back({occurrence++,p.generation,after,p.actor,BnwPolicyUiEffect::game_data,true});}
  military_ui=unit_ui;
  if(modifiers[2]!=0&&active){need(occurrence<UINT64_MAX,"BNW policy gold/military UI occurrence exhausted");military_ui.push_back({occurrence++,p.generation,after,p.actor,BnwPolicyUiEffect::game_data,true});}
  const auto unit_occurrence=registry->next_occurrence+((unit_change!=0&&active)?1u:0u);
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  need(named(block.fields,"m_iCityConnectionTradeRouteGoldModifier").value==p.expected.process_scalars.at("m_iCityConnectionTradeRouteGoldModifier")&&
   named(block.fields,"m_iCityConnectionGoldTimes100").value==p.expected.process_scalars.at("m_iCityConnectionGoldTimes100")&&
   *per_unit==p.expected.process_scalars.at("m_iGoldPerUnit")&&*military==p.expected.process_scalars.at("m_iGoldPerMilitaryUnit"),"BNW policy treasury source changed before writes");
  *registry=std::move(next_registry);
  // Native treasury setter BEFORE subsequent player gold cost setters. The
  // recomputation reads the current route matrix; it never regenerates paths.
  *modifier_field=new_modifier;treasury.connection_modifier=new_modifier;
  if(modifiers[0]!=0){*gold_field=new_gold;treasury.connections_times100=new_gold;}
  *per_unit=new_unit;maintenance.gold_per_unit_times100=new_unit;
  if(unit_change!=0&&active){registry->ui.swap(unit_ui);registry->next_occurrence=unit_occurrence;}
  *military=new_military;
  if(modifiers[2]!=0&&active){registry->ui.swap(military_ui);registry->next_occurrence=occurrence;}
  p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=BnwPolicyAdoptionPhase::city_strength_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_city_scalars()const {
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::city_strength_pending&&!p.city_attempted,"BNW policy city effects replay/unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;
 binding.lease.require_operation(binding.serial);p.city_attempted=true;
 try {
  std::array<std::int64_t,17> changes{},sums{};std::size_t matches=0;const auto rule=p.rules.policies.at(p.target);
  const auto report=host->database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(p.target)){need(row.text("Type").value_or("")==rule.type,"BNW city policy Info changed");++matches;
    for(std::size_t i=0;i<changes.size();++i){changes[i]=row.integer(city_policy_scalars[i].first);need(changes[i]>=INT32_MIN&&changes[i]<=INT32_MAX,"BNW city policy modifier outside native int");
     if(i>=12)need(changes[i]==0||changes[i]==1,"BNW city policy boolean SQL unsupported");}}
   return true;
  });
  need(report.all_succeeded()&&matches==1,"BNW city policy complete SQL unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto& maintenance=state.maintenance.players.at(p.actor);auto& unhappy=state.unhappiness.players.at(p.actor);
  auto& player_cities=state.cities.players.at(p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy city cache missing");
  auto desired=p.expected;std::array<std::int64_t*,17> fields{};
  for(std::size_t i=0;i<fields.size();++i){auto& field=named(raw.core.fields,city_policy_scalars[i].second);
   need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&field.values[0]==p.expected.process_scalars.at(city_policy_scalars[i].second),"BNW city policy scalar shape/owned pair differs");
   fields[i]=&field.values[0];sums[i]=native_sum(*fields[i],changes[i]);desired.process_scalars.at(city_policy_scalars[i].second)=sums[i];}
  using Strength=BnwMinorCityStrengthScratch;
  struct CityWrite {
   Strength::LiveQuote quote;std::int64_t* field=nullptr;ProfileExtensions* extensions=nullptr;const Strength::LiveStored* old=nullptr;
   std::map<std::type_index,std::any>::node_type node;
  };
  std::vector<CityWrite> cities;auto expected_strengths=p.expected_strengths;
  auto events=state.city_dirty_events;const auto first_occurrence=state.city_dirty_next_occurrence;auto occurrence=first_occurrence;
  std::uint64_t previous=0;for(const auto& event:events){need(event.generation==p.generation&&event.occurrence>previous&&event.occurrence<occurrence,"BNW policy existing city dirty order differs");previous=event.occurrence;}
  const auto push_dirty=[&](std::int32_t city){need(occurrence>0&&occurrence<UINT64_MAX,"BNW policy city dirty occurrence exhausted");events.push_back({occurrence++,p.generation,static_cast<std::int32_t>(p.actor),city,-1,-1,BnwCityDirtyKind::city_info,true});};
  if(changes[0]!=0){
   // SDK SetCityStrengthMod calls every real city in ascending FLTA slot order.
   // ALL new strengths are prepared against the old SAME world with the sealed
   // new rule-derived modifier; no temporary mutation or whole-state clone.
   cities.reserve(raw.cities.cities.size());
   for(auto& city:raw.cities.cities){const auto id=city_scalar(city,"m_iID");
    need(resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id))==&city,"BNW policy city strength real FLTA roster differs");
    const auto q=Strength::quote_same_live_policy(state,*host,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id),static_cast<std::int32_t>(sums[0]));
    auto& owned=player_cities.cities.at(id);const auto* old=owned.extensions.find<Strength::LiveStored>();
    need(q.raw==&city&&q.owner==p.actor&&q.city==id&&(!old||(old->strength==q.before&&old->generation==p.generation)),"BNW policy existing city strength owned cache differs");
    auto& field=named(city.core.fields,"m_iStrengthValue");need(field.encoding==saves::CityCoreEncoding::i32&&field.values.size()==1&&field.values[0]==q.before,"BNW policy actual strength destination differs");
    std::map<std::type_index,std::any> staging;staging.emplace(std::type_index(typeid(Strength::LiveStored)),std::any(Strength::LiveStored{q.after,q.known,p.generation}));
    cities.push_back({q,&field.values[0],&owned.extensions,old,staging.extract(staging.begin())});
    expected_strengths.insert_or_assign(id,std::pair{q.after,q.known});
   }
   // Even an empty collection must prove the actual native GetCount/FLTA shape.
   const auto last=named(raw.cities.header.fields,"last_index").value,free=named(raw.cities.header.fields,"free_list_count").value;
   need(last>=-1&&last<=INT32_MAX&&free>=0&&free<=INT32_MAX&&named(raw.cities.header.fields,"num_slots").value==static_cast<std::int64_t>(raw.cities.header.source_slots),"BNW policy empty/native city strength FLTA header unavailable");
   need(raw.cities.cities.size()==player_cities.cities.size()&&raw.cities.header.wire_city_count==static_cast<std::int64_t>(cities.size())&&
    raw.cities.layout==saves::GameStateLayout::expansion2_version1&&raw.cities.header.layout==raw.cities.layout&&
    named(raw.cities.header.fields,"last_index").value-named(raw.cities.header.fields,"free_list_count").value+1==static_cast<std::int64_t>(cities.size()),
    "BNW policy native city strength complete count differs");
   std::sort(cities.begin(),cities.end(),[](const CityWrite& a,const CityWrite& z){return (a.quote.city&8191)<(z.quote.city&8191);});
   for(const auto& city:cities)push_dirty(city.quote.city);
  }
  auto strength_events=events;const auto strength_occurrence=occurrence;
  const bool military_dirty=changes[14]!=0&&state.setup.players.at(p.actor).team==detail::bnw_game_active_team_read(state);
  if(military_dirty)push_dirty(-1);
  bool changed=occurrence!=first_occurrence;for(std::size_t i=0;i<fields.size();++i)changed=changed||sums[i]!=*fields[i];
  const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy city revision exhausted");const auto after=before+(changed?1u:0u);
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  const std::array<std::pair<std::size_t,std::int64_t*>,4> shared_fields{{{5,&maintenance.improvement_modifier},{6,&maintenance.building_modifier},{7,&maintenance.unit_modifier},{8,&maintenance.supply_modifier}}};
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  need(state.city_dirty_next_occurrence==first_occurrence,"BNW policy city dirty source changed before writes");
  for(const auto& city:cities){const auto final=Strength::quote_same_live_policy(state,*host,static_cast<std::int32_t>(p.actor),city.quote.city,static_cast<std::int32_t>(sums[0]));
   need(final.raw==city.quote.raw&&final.owner==city.quote.owner&&final.city==city.quote.city&&final.before==city.quote.before&&final.after==city.quote.after&&final.known==city.quote.known&&final.plot==city.quote.plot&&
    player_cities.cities.at(city.quote.city).extensions.find<Strength::LiveStored>()==city.old&&*city.field==city.quote.before,"BNW policy reached strength formula/destination changed before writes");}
  for(std::size_t i=0;i<fields.size();++i)need(named(raw.core.fields,city_policy_scalars[i].second).values.data()==fields[i]&&*fields[i]==p.expected.process_scalars.at(city_policy_scalars[i].second),"BNW city policy source scalar changed before stores");
  // Actual native modifier, all city caches/dirty publication, THEN all later
  // scalar effects. Preallocated map node adoption/swaps cannot allocate here.
  *registry=std::move(next_registry);*fields[0]=sums[0];
  for(auto& city:cities){*city.field=city.quote.after;auto& values=city.extensions->values;const auto found=values.find(city.node.key());
   if(found==values.end())values.insert(std::move(city.node));else found->second.swap(city.node.mapped());}
  if(changes[0]!=0){state.city_dirty_events.swap(strength_events);state.city_dirty_next_occurrence=strength_occurrence;}
  for(std::size_t i=1;i<fields.size();++i){*fields[i]=sums[i];for(const auto& shared:shared_fields)if(shared.first==i)*shared.second=sums[i];
   if(i==12)unhappy.half_specialists=sums[i]>0;
   if(i==14&&military_dirty){state.city_dirty_events.swap(events);state.city_dirty_next_occurrence=occurrence;}
   if(i==16)player_cities.second_pantheon_count=sums[i];}
  p.expected=std::move(expected_copy);p.expected_strengths=std::move(expected_strengths);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=BnwPolicyAdoptionPhase::spaceship_hurry_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::advance_process_extra_scalars()const {
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::spaceship_hurry_pending&&!p.extra_attempted,"BNW policy worker effects replay/unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;
 binding.lease.require_operation(binding.serial);p.extra_attempted=true;
 try {
  std::array<std::int64_t,extra_policy_scalars.size()> changes{},sums{};
  std::size_t matches=0;const auto rule=p.rules.policies.at(p.target);
  const auto report=host->database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(p.target)){
    need(row.text("Type").value_or("")==rule.type,"BNW worker policy current Info changed");++matches;
    for(std::size_t i=0;i<changes.size();++i){
     changes[i]=row.integer(extra_policy_scalars[i].first);
     need(changes[i]>=INT32_MIN&&changes[i]<=INT32_MAX,"BNW worker policy modifier outside native int");
     if(i==0||i==1||i==9||i==10||i==14)need(changes[i]==0||changes[i]==1,"BNW policy native boolean SQL value unsupported");
    }
   }return true;
  });
  need(report.all_succeeded()&&matches==1,"BNW worker policy complete SQL/entry unavailable");

  // CvMap::getWorldInfo delegates to CvPreGame::worldInfo. This is the actual
  // retained native WorldInfo object read from BNW Pregame version2, not an
  // invented map-size coefficient or a default database world choice.
  need(p.state->world.pregame&&p.state->world.pregame->layout==saves::GameStateLayout::expansion2_version1&&
   p.state->world.pregame->world.wire_version==std::optional<std::uint64_t>{2},"BNW policy actual WorldInfo unavailable");
  const auto world_modifier=p.state->world.pregame->world.values.integers.at("max_conscript_modifier");
  need(world_modifier>=INT32_MIN&&world_modifier<=INT32_MAX,"BNW policy world conscript modifier outside native int");
  changes[2]=native_product(changes[2],std::max<std::int64_t>(0,native_sum(world_modifier,100)))/100;
  const auto anchor=policy_define(*host,"MINOR_FRIENDSHIP_AT_WAR");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy worker owned cache missing");
  auto desired=p.expected;std::array<std::int64_t*,extra_policy_scalars.size()> fields{};
  for(std::size_t i=0;i<fields.size();++i){
   const auto member=extra_policy_scalars[i].second;auto& field=named(raw.core.fields,member);
   need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&field.values[0]==p.expected.process_scalars.at(member),
    "BNW policy worker raw/owned scalar differs");fields[i]=&field.values[0];
   const auto sum=*fields[i]+changes[i];need(sum>=INT32_MIN&&sum<=INT32_MAX,"BNW policy worker native addition overflow");
   sums[i]=i==7?std::max(sum,anchor):sum;
   desired.process_scalars.at(member)=sums[i];
  }
  bool changed=false;for(std::size_t i=0;i<fields.size();++i)changed=changed||sums[i]!=*fields[i];
  const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy worker revision exhausted");const auto after=before+(changed?1u:0u);
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  need(p.state->world.pregame->world.values.integers.at("max_conscript_modifier")==world_modifier,"BNW policy WorldInfo changed before stores");
  for(std::size_t i=0;i<fields.size();++i)need(named(raw.core.fields,extra_policy_scalars[i].second).values.data()==fields[i]&&
   *fields[i]==p.expected.process_scalars.at(extra_policy_scalars[i].second),"BNW worker policy source changed before stores");
  // All potentially throwing lookups/allocations and all native-width checks
  // have finished. The following SAME-state section has no callback or clone.
  *registry=std::move(next_registry);
  for(std::size_t i=0;i<fields.size();++i){
   *fields[i]=sums[i];
  }
  p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;
  p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=BnwPolicyAdoptionPhase::one_shot_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_one_shot()const {
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::one_shot_pending&&!p.one_shot_attempted,"BNW policy one-shot replay/unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;
 binding.lease.require_operation(binding.serial);p.one_shot_attempted=true;
 try {
  std::array<std::int64_t,3> changes{};std::size_t matches=0;const auto rule=p.rules.policies.at(p.target);
  const auto report=host->database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(p.target)){
    need(row.text("Type").value_or("")==rule.type,"BNW one-shot policy current Info changed");++matches;
    changes={row.integer("OneShot"),row.integer("NumExtraBranches"),row.integer("AllFeatureProduction")};
    for(auto value:changes)need(value>=INT32_MIN&&value<=INT32_MAX,"BNW one-shot modifier outside native int");
    need(changes[0]==0||changes[0]==1,"BNW one-shot native boolean SQL value unsupported");
   }return true;
  });
  need(report.all_succeeded()&&matches==1,"BNW one-shot complete SQL/entry unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  // Exact CvPlayer::processPolicies early return occurs AFTER all preceding
  // scalar effects. It bypasses the remainder of processPolicies only: the
  // outer doAdoptPolicy still must recompute cost, unlock branch and run hooks.
  if(changes[0]!=0&&p.expected.one_shot.at(p.target)){
   // This flag is earned only at the actual original early return, after
   // earlier scalar calls. It cannot stand for GP/camp/inner-UI completion.
   p.early_process_policies_returned=true;p.phase=p.is_finisher?BnwPolicyAdoptionPhase::finisher_process_returned:BnwPolicyAdoptionPhase::policy_cost_pending;return;
  }
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto& block=named(raw.strategy.blocks,"policies");
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy one-shot owned cache missing");
  auto desired=p.expected;
  if(changes[0]!=0)desired.one_shot.at(p.target)=true;
  desired.extra_branches=native_sum(desired.extra_branches,changes[1]);
  const auto feature=native_sum(desired.process_scalars.at("m_iAllFeatureProduction"),changes[2]);
  desired.process_scalars.at("m_iAllFeatureProduction")=feature;
  auto staged=named(block.hashes,"one_shot_fired");if(changes[0]!=0)set_flag(staged,rule.hash,true);
  auto& native_flag=named(block.hashes,"one_shot_fired");
  auto* extra=&named(block.fields,"extra_branches").value;
  auto& field=named(raw.core.fields,"m_iAllFeatureProduction");
  need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1,"BNW feature production raw shape changed");auto* feature_field=&field.values[0];
  const bool changed=desired!=p.expected;const auto before=p.after;
  need(!changed||before<UINT64_MAX,"BNW one-shot revision exhausted");const auto after=before+(changed?1u:0u);
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  need(&named(block.hashes,"one_shot_fired")==&native_flag&&flags(native_flag,p.rules.policy_hashes)==p.expected.one_shot&&
   &named(block.fields,"extra_branches").value==extra&&*extra==p.expected.extra_branches&&
   named(raw.core.fields,"m_iAllFeatureProduction").values.data()==feature_field&&*feature_field==p.expected.process_scalars.at("m_iAllFeatureProduction"),
   "BNW one-shot source changed before paired stores");
  // No callbacks/allocations after this point until all actual native stores
  // complete. Native order: SetOneShotPolicyFired, ChangeNumExtraBranches,
  // ChangeAllFeatureProduction (the SDK deliberately omits *iChange here).
  *registry=std::move(next_registry);
  if(changes[0]!=0){native_flag.rows.swap(staged.rows);native_flag.wire_count=staged.wire_count;}
  *extra=desired.extra_branches;*feature_field=feature;
  p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;
  p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=BnwPolicyAdoptionPhase::yield_arrays_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_yield_arrays()const {
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::yield_arrays_pending&&(!p.yield_attempted||p.map_complete),"BNW policy yield loop replay/unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);p.yield_attempted=true;p.map_complete=false;
 try {
  const auto changes=actual_policy_yield_changes(*host,p.rules.policies.at(p.target).type);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  // Source outer loop is YieldTypes, inner calls are the eight explicit
  // setters. A zero modifier proves the original conditional never invokes
  // updateYield/processSpecialist; it is not acceptance of a cached result.
  while(p.yield_cursor<48){
   const auto yield=static_cast<std::size_t>(p.yield_cursor/8),call=static_cast<std::size_t>(p.yield_cursor%8);const auto delta=changes[call][yield];
   if(delta==0){++p.yield_cursor;continue;}
   if(call==7){p.phase=BnwPolicyAdoptionPhase::specialist_yield_pending;return;}
   auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();
   need(registry&&registry->players.contains(p.actor),"BNW policy actual yield registry missing");
   const auto member=yield_policy_arrays[call].second;auto desired=p.expected;auto& field=named(raw.core.fields,member);
   need(field.encoding==saves::PlayerCoreEncoding::vector_i32&&field.values.size()==6&&policy_yield_vector(raw.core,member)==p.expected.process_arrays.at(member),
    "BNW policy raw/owned yield array differs");auto* destination=&field.values[yield];
   const auto value=native_sum(*destination,(call==1||call==3)?native_product(delta,100):delta);desired.process_arrays.at(member)[yield]=value;
   auto next_ranks=p.expected_ranks;std::vector<saves::SavedCityObservation*> cities;
   auto actual_modifier=state.cities.players.at(p.actor).modifiers,actual_capital=state.cities.players.at(p.actor).capital_modifiers;
   if(call==0||call==5){
    for(auto& city:raw.cities.cities){const auto id=city_scalar(city,"m_iID");
     need(resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id))==&city,
      "BNW policy rank invalidation actual city FLTA identity differs");
     (void)policy_rank_array(city,"m_abBaseYieldRankValid");(void)policy_rank_array(city,"m_abYieldRankValid");
     need(next_ranks.emplace(id,PolicyRanks{std::vector<std::int64_t>(6,0),std::vector<std::int64_t>(6,0)}).second||next_ranks.at(id)==PolicyRanks{std::vector<std::int64_t>(6,0),std::vector<std::int64_t>(6,0)},
      "BNW policy prior rank invalidation projection changed");cities.push_back(&city);
    }
    std::sort(cities.begin(),cities.end(),[](const auto* a,const auto* z){return (city_scalar(*a,"m_iID")&8191)<(city_scalar(*z,"m_iID")&8191);});
    auto& cache=call==0?actual_modifier:actual_capital;
    if(value==0)cache.erase(static_cast<std::int64_t>(yield));else cache.insert_or_assign(static_cast<std::int64_t>(yield),value);
    if(call==5){const auto capital=scalar(raw.core,"m_iCapitalCityID");need(capital==state.cities.players.at(p.actor).capital,"BNW policy capital-yield actual capital cache changed");
     (void)resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(capital));}
   }
   const auto before=p.after;need(before<UINT64_MAX,"BNW policy yield revision exhausted");const auto after=before+1;
   auto dirty=state.city_dirty_events;auto occurrence=state.city_dirty_next_occurrence;
   if(call==0&&state.setup.players.at(p.actor).team==detail::bnw_game_active_team_read(state)){
    need(occurrence>0&&occurrence<UINT64_MAX,"BNW policy yield dirty occurrence exhausted");dirty.push_back({occurrence++,p.generation,static_cast<std::int32_t>(p.actor),-1,-1,-1,BnwCityDirtyKind::city_info,true});
   }
   auto next_registry=*registry;next_registry.players.at(p.actor)=desired;for(const auto& [id,ranks]:next_ranks)next_registry.city_yield_ranks.insert_or_assign({p.actor,id},ranks);auto expected_copy=desired;
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   need(named(raw.core.fields,member).values.data()+yield==destination&&*destination==p.expected.process_arrays.at(member)[yield],"BNW policy yield source changed before stores");
   // Prevalidate every actual city array again before the native modifier
   // store. Actual invalidation clears ALL six validity flags (CvPlayer
   // discards its eYield argument), never the stored rank numbers themselves.
   for(auto* city:cities){(void)policy_rank_array(*city,"m_abBaseYieldRankValid");(void)policy_rank_array(*city,"m_abYieldRankValid");}
   *registry=std::move(next_registry);*destination=value;
   if(call==0||call==5){
    state.cities.players.at(p.actor).modifiers.swap(actual_modifier);state.cities.players.at(p.actor).capital_modifiers.swap(actual_capital);
    for(auto* city:cities){auto& block=named(city->production.blocks,"before_strategy");
     auto& a=named(block.arrays,"m_abBaseYieldRankValid");auto& z=named(block.arrays,"m_abYieldRankValid");
     std::fill(a.values.begin(),a.values.end(),0);std::fill(z.values.begin(),z.values.end(),0);}
    state.city_dirty_events.swap(dirty);state.city_dirty_next_occurrence=occurrence;
   }
   p.expected=std::move(expected_copy);p.expected_ranks.swap(next_ranks);p.previous=before;p.after=after;state.ui_source_revision=after;
   p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
   if(call==1||call==2||call==3||call==4){p.map_cursor=0;p.phase=BnwPolicyAdoptionPhase::plot_yield_pending;return;}
   ++p.yield_cursor;p.phase=BnwPolicyAdoptionPhase::yield_arrays_pending;
  }
  p.phase=BnwPolicyAdoptionPhase::unit_combat_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

std::pair<std::uint64_t,std::uint64_t> BnwPolicyAdoptionPending::private_policy_plot_identity()const{
 (void)phase();const auto& p=*impl_;const auto native_phase=(p.plot_child&&(p.plot_ui_dispatching||p.phase==BnwPolicyAdoptionPhase::plot_ui_pending||p.phase==BnwPolicyAdoptionPhase::policy_outer_plot_ui_pending||p.phase==BnwPolicyAdoptionPhase::golden_age_plot_ui_pending))?p.plot_parent_call_phase:p.phase;const bool improvement=native_phase==BnwPolicyAdoptionPhase::improvement_plot_yield_pending;
 need(!improvement||p.improvement_cursor<=UINT64_MAX-48,"BNW policy genuine matrix plot ordinal overflow");
 const bool outer=native_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending;need(!outer||(p.outer_policy_hook_returned&&p.outer_yield_attempted&&p.expected.improvement_yields.size()<=UINT64_MAX-48),"BNW actual outer updateYield ordinal/predecessor unavailable");
 const bool golden=native_phase==BnwPolicyAdoptionPhase::golden_age_map_pending;
 need(!golden||(p.golden_age_map_attempted&&p.golden_age_turns_stored&&p.expected.improvement_yields.size()<=UINT64_MAX-49),"BNW GoldenAge whole-map ordinal/predecessor unavailable");
  const bool resource=native_phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending;
  need(!resource||(p.resource_loop_reached&&!p.resource_loop_returned&&p.expected.improvement_yields.size()<=UINT64_MAX-50),"BNW resource PolicyReveal call ordinal unavailable");
 return {p.hooks->impl_->serial,resource?50+static_cast<std::uint64_t>(p.expected.improvement_yields.size()):(golden?49+static_cast<std::uint64_t>(p.expected.improvement_yields.size()):(outer?48+static_cast<std::uint64_t>(p.expected.improvement_yields.size()):(improvement?48+p.improvement_cursor:p.yield_cursor)))};
}
void BnwPolicyAdoptionPending::require_unissued_policy_plot_source(const BnwPlotYieldRefreshIssuer& issuer,
 const LiveSessionState& state,const saves::SavedSession& source,const SessionCheckpointIdentity& identity,std::int64_t plot)const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(bool(host),"BNW policy plot parent Host expired");
 require_stored_current(*host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 const auto& session=host->session();
 need(((p.phase==BnwPolicyAdoptionPhase::plot_yield_pending||p.phase==BnwPolicyAdoptionPhase::improvement_plot_yield_pending)||(p.phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending||p.phase==BnwPolicyAdoptionPhase::golden_age_map_pending||p.phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending))&&!p.plot_child&&p.map_cursor==static_cast<std::uint64_t>(plot)&&
  (p.phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending?(p.resource_loop_reached&&!p.resource_loop_returned&&p.policy_resource_plot==plot&&!p.resource_reveal_attempted):(p.phase==BnwPolicyAdoptionPhase::golden_age_map_pending?(p.golden_age_map_attempted&&p.golden_age_turns_stored&&!p.golden_age_map_returned&&(p.golden_age_operands[6]>0)!=(p.golden_age_operands[7]>0)):(p.phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending?(p.outer_policy_hook_returned&&p.outer_yield_attempted):(p.phase==BnwPolicyAdoptionPhase::plot_yield_pending?p.yield_cursor<48:p.improvement_cursor<p.expected.improvement_yields.size()))))&&&state==p.state.get()&&&source==p.source.get()&&session.plot_yield_refresh_issuer_.get()==&issuer&&
  session.bnw_game_debug_owner_current()&&session.debug_->identity.payload_sha256==identity.payload_sha256&&
  session.debug_->identity.rules_sha256==identity.rules_sha256,
  "BNW policy unissued plot call requires genuine current parent/ordinal/source");
 const auto& raw=state.world.map_plots->plots.at(static_cast<std::size_t>(plot));const auto& cell=state.world.map_projection->cells.at(static_cast<std::size_t>(plot));
 need(raw.index==static_cast<std::uint64_t>(plot)&&cell.index==raw.index&&named(raw.fields,"owner").value==cell.owner&&(p.phase==BnwPolicyAdoptionPhase::golden_age_map_pending||p.phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending||cell.owner==p.actor),
  "BNW actual policy map child owner/projection differs or owned-only call reached foreign plot");
}
void BnwPolicyAdoptionPending::advance_process_plot_yields()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&((p.phase==BnwPolicyAdoptionPhase::plot_yield_pending||p.phase==BnwPolicyAdoptionPhase::improvement_plot_yield_pending)||(p.phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending||p.phase==BnwPolicyAdoptionPhase::golden_age_map_pending||p.phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending))&&!p.plot_child,"BNW policy map call replay or required child UI pending");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 const auto caller_phase=p.phase;
 if(caller_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending){need(p.outer_policy_hook_returned&&!p.outer_policy_yield_returned&&(!p.outer_yield_attempted||p.outer_yield_resuming),"BNW final native updateYield unreached/repeated");p.outer_yield_resuming=false;p.outer_yield_attempted=true;}
 if(caller_phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending)need(p.resource_loop_reached&&!p.resource_loop_returned&&!p.resource_reveal_attempted,"BNW PolicyReveal map call unreached/repeated");
 if(caller_phase==BnwPolicyAdoptionPhase::golden_age_map_pending){need(p.golden_age_attempted&&p.golden_age_turns_stored&&!p.golden_age_map_returned&&(!p.golden_age_map_attempted||p.golden_age_map_resuming),"BNW actual GoldenAge whole-map call unreached/repeated");p.golden_age_map_resuming=false;p.golden_age_map_attempted=true;}
 try{
  // CvPlayer::updateYield reaches owned plots; GoldenAge invokes the distinct
  // CvMap::updateYield and reaches EVERY index, including foreign/unowned.
  // Both use genuine normal native child calculate/store owners in source order.
  need(p.session&&p.session==&host->session(),"BNW policy actual mutable session facade changed");auto& session=*p.session;
  need(session.bnw_game_debug_owner_current(),"BNW policy map call requires actual imported game debug/source control");
  const auto identity=session.debug_->identity;

  auto& state=*p.state;need(state.world.map_plots&&state.world.map_projection&&
   state.world.map_plots->plots.size()==state.world.map_header.grid.size()&&state.world.map_projection->cells.size()==state.world.map_plots->plots.size(),
   "BNW policy actual map/projection counts differ");
  if(!session.plot_yield_refresh_issuer_)session.plot_yield_refresh_issuer_=std::shared_ptr<BnwPlotYieldRefreshIssuer>(new BnwPlotYieldRefreshIssuer(p.state,p.generation));
  session.plot_yield_refresh_issuer_->bind_game_debug(session.debug_,p.source);
  while(p.map_cursor<state.world.map_plots->plots.size()){
   const auto index=p.map_cursor;need(index<=INT32_MAX,"BNW policy source plot index outside native int");
   const auto& raw=state.world.map_plots->plots[static_cast<std::size_t>(index)];const auto& cell=state.world.map_projection->cells[static_cast<std::size_t>(index)];
   need(raw.index==index&&cell.index==index&&cell.owner==named(raw.fields,"owner").value,"BNW policy actual map owner/projection differs");
    if(caller_phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending){
     const auto hash=named(raw.fields,"resource_type_hash").value;
     need(hash>=0&&hash<=UINT32_MAX&&cell.resource_type_hash&&*cell.resource_type_hash==static_cast<std::uint64_t>(hash),"BNW resource PolicyReveal actual raw/projected type differs");
     const auto* info=hash?host->types().find_hash(static_cast<std::uint64_t>(hash)):nullptr;
     need(!hash||(info&&info->table=="Resources"&&p.resource_policy_reveals.contains(info->id)&&(!cell.resource_type||*cell.resource_type==info->id)),"BNW resource PolicyReveal selected Resource Info missing");
     if(!hash||p.resource_policy_reveals.at(info->id)!=p.target){++p.map_cursor;p.resource_scanned=p.map_cursor;continue;}
     p.policy_resource_plot=static_cast<std::int64_t>(index);p.resource_reveal_attempted=false;
    }
    if(caller_phase!=BnwPolicyAdoptionPhase::golden_age_map_pending&&caller_phase!=BnwPolicyAdoptionPhase::resource_policy_yield_pending&&cell.owner!=p.actor){++p.map_cursor;if(caller_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending)p.outer_yield_scanned=p.map_cursor;continue;}
   // Rules load only after a reached plot: owned for CvPlayer, every actual
   // index for CvMap GoldenAge. No foreign/neutral cache is normalized away.
   if(!p.normal_plot_rules)p.normal_plot_rules=BnwNormalPlotYieldRules::load(host->database(),identity.rules_sha256);
   // Authenticate any actual earlier native child, then reserve storage before
   // new child construction and every cache store. There is no public replay
   // of a standalone plot claim; only this parent-issued ordinal can supersede.
   const auto key=static_cast<std::int64_t>(index);auto found=session.plot_yield_refresh_pending_.find(key);
   if(found!=session.plot_yield_refresh_pending_.end()){
    need(bool(found->second),"BNW policy existing unissued plot slot");const auto old=found->second->progress().phase;
    need(old==BnwPlotYieldRefreshPhase::complete||old==BnwPlotYieldRefreshPhase::skipped_no_area,"BNW policy earlier plot effect must actually finish");
   }
   else found=session.plot_yield_refresh_pending_.emplace(key,std::shared_ptr<BnwPlotYieldRefreshPending>{}).first;
   p.plot_parent_call_phase=caller_phase;
   auto child=BnwPlotYieldRefreshPending::prepare_normal_policy(session.plot_yield_refresh_issuer_,p.state,p.source,identity,*p.normal_plot_rules,key,*this);
   p.plot_child=child;found->second=child;const auto before=p.after;
   child->advance_prefix();const auto progress=child->progress();
   p.previous=before;p.after=state.ui_source_revision;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);if(p.after!=before)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
   p.phase=caller_phase;
   if(progress.phase!=BnwPlotYieldRefreshPhase::complete&&progress.phase!=BnwPlotYieldRefreshPhase::skipped_no_area){
    // Actual changed plot/terrain prefix committed. Current original mandatory
    // CityScreen predicate/dirty/symbol phases retain their genuine owner;
    // nothing here acknowledges display or silently finishes them.
    p.phase=caller_phase==BnwPolicyAdoptionPhase::golden_age_map_pending?BnwPolicyAdoptionPhase::golden_age_plot_ui_pending:(caller_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending?BnwPolicyAdoptionPhase::policy_outer_plot_ui_pending:BnwPolicyAdoptionPhase::plot_ui_pending);return;
   }
   p.plot_child.reset();
   if(caller_phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending){if(!finish_resource_policy_plot())return;}
   else ++p.map_cursor;
   if(caller_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending)p.outer_yield_scanned=p.map_cursor;if(caller_phase==BnwPolicyAdoptionPhase::golden_age_map_pending)p.golden_age_map_scanned=p.map_cursor;
  }
  if(caller_phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending){
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   need(p.resource_scanned==state.world.map_plots->plots.size()&&!p.plot_child&&!p.policy_resource_plot,"BNW resource PolicyReveal returned before its whole map scan");
   p.resource_loop_returned=true;p.map_cursor=0;p.phase=BnwPolicyAdoptionPhase::policy_happiness_pending;return;
  }
  p.map_complete=true;p.map_cursor=0;
  if(caller_phase==BnwPolicyAdoptionPhase::golden_age_map_pending){
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   need(p.golden_age_map_attempted&&p.golden_age_turns_stored&&p.golden_age_map_scanned==state.world.map_plots->plots.size()&&!p.plot_child,"BNW GoldenAge returned before every original CvMap plot call");
   p.golden_age_map_returned=true;
   // Actual whole-map call returned. A beginning must next SetNumGoldenAges,
   // optional standalone achievement handling and localized replay BEFORE DLL
   // started/notification. An ending skips history/replay but still needs DLL.
   p.phase=p.golden_age_operands[7]>0?BnwPolicyAdoptionPhase::golden_age_history_pending:BnwPolicyAdoptionPhase::golden_age_gameplay_pending;return;
  }
  if(caller_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending){
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   need(p.outer_yield_attempted&&p.outer_policy_hook_returned&&p.outer_yield_scanned==state.world.map_plots->plots.size()&&!p.plot_child,"BNW final updateYield returned without the entire reached real map scan");
   auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->pending_policy.contains(p.actor)&&registry->pending_policy.at(p.actor)==p.root_target,"BNW final policy call ownership changed");
   // Native doAdoptPolicy has actually returned. Release only the internal
   // actor busy slot; no gameplay store, world revision or UI acknowledgment
   // is invented at this call boundary. Old receipts remain current only
   // until the next real mutation/new call; there is no checkpoint permission.
   registry->pending_policy.erase(p.actor);p.outer_policy_yield_returned=true;p.phase=BnwPolicyAdoptionPhase::adoption_returned;
   require_stored_current(*host);binding.lease.require_operation(binding.serial);return;
  }
  if(caller_phase==BnwPolicyAdoptionPhase::improvement_plot_yield_pending){++p.improvement_cursor;p.phase=BnwPolicyAdoptionPhase::improvement_yield_pending;}
  else{++p.yield_cursor;p.phase=BnwPolicyAdoptionPhase::yield_arrays_pending;}
 }catch(...){
  // A late source/lease failure cannot erase the native return already earned
  // after the whole final map scan and genuine private busy-slot release.
  if(p.phase!=BnwPolicyAdoptionPhase::adoption_returned||!p.outer_policy_yield_returned)p.phase=BnwPolicyAdoptionPhase::uncertain;
  throw;
 }
}

void BnwPolicyAdoptionPending::advance_process_combat_hurry()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::unit_combat_pending&&!p.combat_attempted,"BNW policy combat/hurry replay or unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);p.combat_attempted=true;
 try{
  const auto combat=actual_policy_catalog(*host,"UnitCombatInfos"),hurry=actual_policy_catalog(*host,"HurryInfos");
  const auto policy=p.rules.policies.at(p.target).type;
  const auto production=actual_policy_linked_values(*host,"UnitCombatInfos","Policy_UnitCombatProductionModifiers","UnitCombatType","ProductionModifier",policy,combat.size());
  const auto experience=actual_policy_linked_values(*host,"UnitCombatInfos","Policy_UnitCombatFreeExperiences","UnitCombatType","FreeExperience",policy,combat.size());
  const auto modifiers=actual_policy_linked_values(*host,"HurryInfos","Policy_HurryModifiers","HurryType","HurryCostModifier",policy,hurry.size());
  std::vector<std::pair<bool,std::int64_t>> hurry_rules;hurry_rules.reserve(hurry.size());
  const auto report=host->database().prefetch_info("HurryInfos",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   need(slot<hurry.size()&&row.text("Type").value_or("")==hurry[static_cast<std::size_t>(slot)],"BNW policy Hurry Info current catalog differs");
   const auto ref=row.text("PolicyPrereq").value_or("");const auto* info=ref.empty()?nullptr:host->types().find(ref);
   need(ref.empty()||(info&&info->table=="Policies"),"BNW policy Hurry policy prerequisite unresolved");
   const auto population=row.integer("ProductionPerPopulation");need(population>=INT32_MIN&&population<=INT32_MAX,"BNW policy hurry population operand outside native int");
   hurry_rules.emplace_back(info&&info->id==p.target,population);return true;
  });
  need(report.all_succeeded()&&hurry_rules.size()==hurry.size(),"BNW policy complete Hurry rules unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();
  need(registry&&registry->players.contains(p.actor),"BNW policy combat/hurry cache absent");
  auto desired=p.expected;std::array<std::vector<std::int64_t>*,4> fields{};
  for(std::size_t i=0;i<fields.size();++i){auto& f=named(raw.core.fields,combat_hurry_members[i]);
   need(policy_native_vector(raw.core,combat_hurry_members[i])==p.expected.combat_hurry_arrays.at(combat_hurry_members[i])&&f.values.size()==(i<2?combat.size():hurry.size()),
    "BNW policy raw combat/hurry native span differs from complete current catalog");fields[i]=&f.values;}
  for(std::size_t i=0;i<combat.size();++i){
   desired.combat_hurry_arrays.at(combat_hurry_members[0])[i]=native_sum((*fields[0])[i],production[i]);
   desired.combat_hurry_arrays.at(combat_hurry_members[1])[i]=native_sum((*fields[1])[i],experience[i]);
  }
  // The native count store can activate m_iPopRushHurryCount only on a >0
  // threshold transition with ProductionPerPopulation>0. This retained decoder
  // currently exposes no such mutable native scalar. Stop BEFORE the coupled
  // count store, never fabricate zero or silently lose the population effect.
  std::optional<std::size_t> population_frontier;
  for(std::size_t i=0;i<hurry.size();++i){
   auto count=(*fields[2])[i];if(hurry_rules[i].first)count=native_sum(count,1);
   if(hurry_rules[i].first&&(((*fields[2])[i]>0)!=(count>0))&&hurry_rules[i].second>0){population_frontier=i;break;}
   need(count>=0,"BNW policy native HurryCount became negative");desired.combat_hurry_arrays.at(combat_hurry_members[2])[i]=count;
   desired.combat_hurry_arrays.at(combat_hurry_members[3])[i]=native_sum((*fields[3])[i],modifiers[i]);
  }
  const bool changed=desired!=p.expected;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy combat/hurry revision exhausted");const auto after=before+(changed?1u:0u);
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  std::array<std::vector<std::int64_t>,4> prepared_values;
  for(std::size_t i=0;i<prepared_values.size();++i)prepared_values[i]=desired.combat_hurry_arrays.at(combat_hurry_members[i]);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  for(std::size_t i=0;i<fields.size();++i)need(&named(raw.core.fields,combat_hurry_members[i]).values==fields[i]&&*fields[i]==p.expected.combat_hurry_arrays.at(combat_hurry_members[i]),
   "BNW policy combat/hurry source changed before native stores");
  // Allocate everything first, then preserve exact native call order: each
  // combat production followed by experience, each hurry count followed by cost.
  *registry=std::move(next_registry);
  for(std::size_t i=0;i<combat.size();++i){(*fields[0])[i]=prepared_values[0][i];(*fields[1])[i]=prepared_values[1][i];}
  const auto hurry_end=population_frontier.value_or(hurry.size());
  for(std::size_t i=0;i<hurry_end;++i){(*fields[2])[i]=prepared_values[2][i];(*fields[3])[i]=prepared_values[3][i];}
  p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;
  p.phase=BnwPolicyAdoptionPhase::uncertain;require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.hurry_frontier=population_frontier;p.phase=population_frontier?BnwPolicyAdoptionPhase::hurry_population_pending:BnwPolicyAdoptionPhase::improvement_yield_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_improvement_yields()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::improvement_yield_pending&&!p.plot_child,"BNW policy improvement call unreached or prior mandatory child pending");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  const auto improvements=actual_policy_catalog(*host,"Improvements");const auto yields=actual_policy_catalog(*host,"Yields");
  need(yields.size()==6&&improvements.size()<=SIZE_MAX/6,"BNW policy actual improvement/yield native catalog dimensions differ");
  const auto count=improvements.size()*6;std::vector<std::int64_t> changes(count,0);
  const auto report=host->database().query_rows({
   "select Improvements.ID as ImprovementID, Yields.ID as YieldID, Yield from Policy_ImprovementYieldChanges inner join Improvements on Improvements.Type = ImprovementType inner join Yields on Yields.Type = YieldType where PolicyType = ?",
   {p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
    const auto improvement=row.integer_at(0),yield=row.integer_at(1),value=row.integer_at(2);
    need(improvement>=0&&static_cast<std::uint64_t>(improvement)<improvements.size()&&yield>=0&&yield<6&&value>=INT32_MIN&&value<=INT32_MAX,
     "BNW policy actual improvement relation native index/value differs");
    changes[static_cast<std::size_t>(improvement)*6+static_cast<std::size_t>(yield)]=value; // SDK sequential last assignment.
   });
  need(report.all_succeeded()&&report.sqlite_code==0,"BNW policy complete ImprovementYield SQL unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();
  auto& field=named(named(raw.administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange");
  need(registry&&registry->players.contains(p.actor)&&policy_improvement_matrix(raw)==p.expected.improvement_yields&&field.values.size()==count&&p.improvement_cursor<=count,
   "BNW policy improvement raw/owned actual catalog source differs");
  while(p.improvement_cursor<count){
   const auto index=static_cast<std::size_t>(p.improvement_cursor);const auto delta=changes[index];
   if(delta==0){++p.improvement_cursor;continue;} // CvPlayer native setter itself returns without updateYield.
   auto desired=p.expected;desired.improvement_yields[index]=native_sum(field.values[index],delta);
   const auto before=p.after;need(before<UINT64_MAX,"BNW policy improvement revision exhausted");const auto after=before+1;
   auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;const auto prepared_value=desired.improvement_yields[index];
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   need(&named(named(raw.administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange")==&field&&policy_improvement_matrix(raw)==p.expected.improvement_yields,
    "BNW policy improvement source changed before stores");
   // Actual native setter stores one matrix element, then calls updateYield.
   // Normal plot calculation reads this same retained matrix through existing
   // restore_improvement_yield_state, not an obsolete policy-derived cache.
   *registry=std::move(next_registry);field.values[index]=prepared_value;p.expected=std::move(expected_copy);
   p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
   p.map_cursor=0;p.phase=BnwPolicyAdoptionPhase::improvement_plot_yield_pending;return;
  }
  p.phase=BnwPolicyAdoptionPhase::free_promotions_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_free_promotions()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::free_promotions_pending,"BNW policy promotion interval unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  const auto promotions=actual_policy_catalog(*host,"UnitPromotions");std::vector<bool> selected(promotions.size(),false);
  const auto report=host->database().query_rows({"select UnitPromotions.ID from Policy_FreePromotions inner join UnitPromotions on PromotionType = UnitPromotions.Type where PolicyType = ?",{p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
   const auto id=row.integer_at(0);need(id>=0&&static_cast<std::uint64_t>(id)<selected.size(),"BNW policy free promotion linked Info differs");selected[static_cast<std::size_t>(id)]=true;
  });
  need(report.all_succeeded()&&report.sqlite_code==0&&p.promotion_cursor<=promotions.size(),"BNW policy complete free promotion SQL unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();
  need(registry&&registry->players.contains(p.actor)&&policy_free_promotion_counts(raw.core,p.rules)==p.expected.free_promotions,"BNW policy paired free promotion source differs");
  while(p.promotion_cursor<promotions.size()){
   const auto index=p.promotion_cursor;if(!selected[index]){++p.promotion_cursor;continue;}
   const auto id=static_cast<std::int64_t>(index);const auto* info=host->types().find(promotions[index]);
   need(info&&info->table=="UnitPromotions"&&info->id==id&&p.rules.promotion_hashes.contains(info->hash)&&p.rules.promotion_hashes.at(info->hash)==id,
    "BNW policy current free promotion hash identity differs");
   auto& field=named(raw.core.fields,"m_paiFreePromotionCount");auto hashes=field.hashes;auto values=field.values;
   const auto old=p.expected.free_promotions.at(id),value=native_sum(old,1);std::optional<std::size_t> destination;std::size_t cursor=0;
   for(auto hash:hashes)if(hash){if(hash==info->hash)destination=cursor;++cursor;}
   need(cursor==values.size(),"BNW policy promotion raw source shape changed");
   if(destination)values.at(*destination)=value;else{need(hashes.size()<UINT32_MAX,"BNW policy promotion wire count exhausted");hashes.push_back(info->hash);values.push_back(value);}
   auto desired=p.expected;desired.free_promotions.at(id)=value;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const auto before=p.after;need(before<UINT64_MAX,"BNW policy free promotion revision exhausted");const auto after=before+1;
   const bool newly_free=old==0;bool has_units=false;
   if(newly_free){
    const auto& units=raw.units;const auto& owned=state.maintenance.players.at(p.actor).units;
    need(units.layout==saves::GameStateLayout::expansion2_version1&&units.header.layout==units.layout&&units.header.wire_unit_count==static_cast<std::int64_t>(units.units.size())&&units.units.size()==owned.size(),
     "BNW policy newly free promotion actual unit roster unavailable");has_units=!units.units.empty();
   }
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   need(policy_free_promotion_counts(raw.core,p.rules)==p.expected.free_promotions,"BNW policy promotion source changed before store");
   // ChangeFreePromotionCount stores BEFORE the optional firstUnit effects.
   *registry=std::move(next_registry);field.hashes=std::move(hashes);field.values=std::move(values);field.wire_count=field.hashes.size();
   p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
   if(has_units){p.phase=BnwPolicyAdoptionPhase::unit_promotions_pending;return;}
   ++p.promotion_cursor;p.phase=BnwPolicyAdoptionPhase::free_promotions_pending;
  }
  // Read the real policy object fields, including native defaults, rather than
  // accept caller declarations that these subsequent branches have no effects.
  std::optional<std::int64_t> reveal,decay;const auto branches=host->database().query_rows({"SELECT RevealAllCapitals, OtherPlayersMinorFriendshipDecayMod FROM Policies WHERE Type = ?",{p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
   need(!reveal&&!decay,"BNW policy duplicate native policy object");reveal=row.integer_at(0);decay=row.integer_at(1);
  });
  need(branches.all_succeeded()&&branches.sqlite_code==0&&branches.rows==1&&reveal&&decay&&(*reveal==0||*reveal==1)&&*decay>=INT32_MIN&&*decay<=INT32_MAX,
   "BNW policy subsequent native branch operands unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  if(*reveal){p.phase=BnwPolicyAdoptionPhase::capital_reveal_pending;return;}
  if(*decay){p.phase=BnwPolicyAdoptionPhase::other_minor_friendship_pending;return;}
  p.phase=BnwPolicyAdoptionPhase::city_effects_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_city_effects()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::city_effects_pending,"BNW policy city effects unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  std::array<std::int64_t,4> rules{};std::size_t rows=0;
  const auto report=host->database().query_rows({"SELECT NumCitiesFreeCultureBuilding, NumCitiesFreeFoodBuilding, CulturePerCity, CulturePerGarrisonedUnit FROM Policies WHERE Type = ?",{p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
   ++rows;for(std::size_t i=0;i<rules.size();++i){rules[i]=row.integer_at(static_cast<int>(i));need(rules[i]>=INT32_MIN&&rules[i]<=INT32_MAX,"BNW policy native city effect operand outside int");}});
  need(report.all_succeeded()&&report.sqlite_code==0&&rows==1,"BNW policy complete city effects rules unavailable");
  const auto classes=actual_policy_catalog(*host,"BuildingClasses");const auto yields=actual_policy_catalog(*host,"Yields");need(yields.size()==6&&classes.size()<=SIZE_MAX/6,"BNW policy native building class/yield dimensions differ");
  const auto culture=actual_policy_linked_values(*host,"BuildingClasses","Policy_BuildingClassCultureChanges","BuildingClassType","CultureChange",p.rules.policies.at(p.target).type,classes.size());
  std::array<std::vector<std::int64_t>,2> arrays{std::vector<std::int64_t>(classes.size()*6,0),std::vector<std::int64_t>(classes.size()*6,0)};
  const std::array<const char*,2> tables{"Policy_BuildingClassYieldModifiers","Policy_BuildingClassYieldChanges"},values{"YieldMod","YieldChange"};
  for(std::size_t call=0;call<arrays.size();++call){
   const auto complete=host->database().query_rows({std::string("select BuildingClasses.ID, Yields.ID, ")+values[call]+" from "+tables[call]+" inner join BuildingClasses on BuildingClasses.Type = BuildingClassType inner join Yields on Yields.Type = YieldType where PolicyType = ?",{p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
    const auto c=row.integer_at(0),y=row.integer_at(1),n=row.integer_at(2);need(c>=0&&static_cast<std::uint64_t>(c)<classes.size()&&y>=0&&y<6&&n>=INT32_MIN&&n<=INT32_MAX,"BNW policy native city building relation index/value differs");arrays[call][static_cast<std::size_t>(c)*6+static_cast<std::size_t>(y)]=n;
   });need(complete.all_succeeded()&&complete.sqlite_code==0,"BNW policy complete building yield SQL unavailable");
  }
  // A derived proof over the whole source-native selected arrays. Skipped
  // class/building getters are pure; every possible count multiplies zero.
  // Culture yield is handled by CultureChange; native ignores this matrix's
  // culture column. Faith modifier is ignored, and other modifiers require >0.
  bool building_no_effect=std::all_of(culture.begin(),culture.end(),[](auto n){return n==0;});
  for(std::size_t c=0;c<classes.size();++c)for(std::size_t y=0;y<6;++y){
   if(y==4)continue;if(arrays[1][c*6+y]!=0)building_no_effect=false;
   if(y!=5&&arrays[0][c*6+y]>0)building_no_effect=false;
  }
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy city owned cache absent");
  std::vector<std::pair<std::int64_t,saves::SavedCityObservation*>> cities;
  need(raw.cities.cities.size()==state.cities.players.at(p.actor).cities.size(),"BNW policy actual city roster count differs");
  const auto& header=raw.cities.header;const auto last=named(header.fields,"last_index").value,free=named(header.fields,"free_list_count").value;
  need(raw.cities.layout==saves::GameStateLayout::expansion2_version1&&header.layout==raw.cities.layout&&header.wire_city_count==static_cast<std::int64_t>(raw.cities.cities.size())&&last>=-1&&last<=INT32_MAX&&free>=0&&free<=INT32_MAX&&last-free+1==static_cast<std::int64_t>(raw.cities.cities.size())&&named(header.fields,"num_slots").value==static_cast<std::int64_t>(header.source_slots),"BNW policy native city iteration header differs");
  for(auto& city:raw.cities.cities){const auto id=city_scalar(city,"m_iID");need(id>=0&&resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id))==&city,"BNW policy actual city FLTA resolver differs");cities.emplace_back(id,&city);}
  std::sort(cities.begin(),cities.end(),[](const auto& a,const auto& z){return (a.first&8191)<(z.first&8191);});std::vector<std::int64_t> order;
  for(const auto& [id,city]:cities){(void)city;order.push_back(id);}if(p.city_order.empty()&&p.city_cursor==0)p.city_order=order;
  need(p.city_order==order&&p.city_cursor<=cities.size(),"BNW policy actual city ordered source changed");
  while(p.city_cursor<cities.size()){
   auto& city=*cities[p.city_cursor].second;const auto id=cities[p.city_cursor].first;
   if(rules[0]>0||rules[1]>0){p.phase=BnwPolicyAdoptionPhase::city_free_building_pending;return;}
   // The real existing native GetBestDefender implementation resolves actual
   // plot links/units and all combat guards. No saved m_hGarrison substitute.
   const bool garrison=rules[3]!=0&&BnwMinorCityStrengthScratch::has_same_live_policy_garrison(state,*host,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id));
   const auto delta=native_sum(rules[2],garrison?rules[3]:0);auto observed_garrisons=p.city_garrisons;
   if(rules[3]!=0)observed_garrisons.insert_or_assign(id,garrison);
   auto& field=named(city.core.fields,"m_iJONSCulturePerTurnFromPolicies");const auto old=city_scalar(city,"m_iJONSCulturePerTurnFromPolicies"),value=native_sum(old,delta);
   auto next_registry=*registry;const auto key=std::pair{p.actor,id};if(next_registry.city_policy_culture.contains(key))need(next_registry.city_policy_culture.at(key)==old,"BNW policy city culture actual paired cache differs");
   next_registry.city_policy_culture.insert_or_assign(key,value);auto expected_cities=next_registry.city_policy_culture;
   const auto before=p.after;const bool changed=value!=old;need(!changed||before<UINT64_MAX,"BNW policy city culture revision exhausted");const auto after=before+(changed?1u:0u);
   require_stored_current(*host);binding.lease.require_operation(binding.serial);need(resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id))==&city&&field.values.size()==1&&field.values[0]==old,"BNW policy actual city culture changed before stores");
   if(rules[3]!=0)need(BnwMinorCityStrengthScratch::has_same_live_policy_garrison(state,*host,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id))==garrison,"BNW policy actual garrison changed before culture store");
   *registry=std::move(next_registry);field.values[0]=value;p.city_garrisons=std::move(observed_garrisons);p.expected_cities=std::move(expected_cities);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
   if(!building_no_effect){p.phase=BnwPolicyAdoptionPhase::city_building_effects_pending;return;}
   ++p.city_cursor;p.phase=BnwPolicyAdoptionPhase::city_effects_pending;
  }
  // The no-city branch reaches these native remaining-count setters even when
  // there were no city selectors. Negative input also reaches the plain store.
  constexpr std::array<const char*,2> members{"m_iNumCitiesFreeCultureBuilding","m_iNumCitiesFreeFoodBuilding"};
  auto desired=p.expected;std::array<std::int64_t*,2> fields{};std::array<std::int64_t,2> targets{};
  for(std::size_t i=0;i<fields.size();++i){auto& f=named(raw.core.fields,members[i]);need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&f.values[0]==p.expected.process_scalars.at(members[i]),"BNW policy remaining building count source differs");fields[i]=&f.values[0];targets[i]=native_sum(*fields[i],rules[i]);desired.process_scalars.at(members[i])=targets[i];}
  auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;const bool changed=desired!=p.expected;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy remaining building count revision exhausted");const auto after=before+(changed?1u:0u);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  *registry=std::move(next_registry);for(std::size_t i=0;i<fields.size();++i)*fields[i]=targets[i];p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);p.phase=BnwPolicyAdoptionPhase::policy_disables_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_rewards()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_disables_pending&&p.reward_cursor==0,"BNW policy reward suffix unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  const auto catalog=actual_policy_catalog(*host,"Policies");std::vector<std::int64_t> disabled(catalog.size(),-1);std::size_t assignments=0;
  const auto disables=host->database().query_rows({"select Policies.ID from Policy_Disables inner join Policies on Policies.Type = PolicyDisable where PolicyType = ?",{p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
   const auto id=row.integer_at(0);need(assignments<disabled.size()&&id>=0&&p.rules.policies.contains(id),"BNW policy native disable array exceeds span or unresolved Info");disabled[assignments++]=id;
  });
  const auto* slots=host->defines().find("NUM_AND_TECH_PREREQS");need(disables.all_succeeded()&&disables.sqlite_code==0&&host->defines().profile()==content::GlobalDefineProfile::bnw&&slots&&slots->kind==content::GlobalDefineKind::integer&&slots->lookup.found&&slots->lookup.sqlite_code==0&&slots->lookup.error.empty()&&!slots->retained_previous,"BNW policy native disable SQL/AND slots unavailable");
  const auto* count=std::get_if<std::int64_t>(&slots->lookup.value);need(count&&*count>=0&&static_cast<std::uint64_t>(*count)<=disabled.size(),"BNW policy native disable iteration exceeds allocated Policies array");
  std::array<std::int64_t,5> effects{};std::size_t rows=0;
  const auto report=host->database().query_rows({"SELECT AttackBonusTurns, GoldenAgeTurns, NumFreeTechs, MedianTechPercentChange, NumFreePolicies FROM Policies WHERE Type = ?",{p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
   ++rows;for(std::size_t i=0;i<effects.size();++i){effects[i]=row.integer_at(static_cast<int>(i));need(effects[i]>=INT32_MIN&&effects[i]<=INT32_MAX,"BNW policy native reward operand outside int");}});
  need(report.all_succeeded()&&report.sqlite_code==0&&rows==1,"BNW policy complete reward selected object unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy reward owned pair absent");
  // setHasPolicy(false) invokes recursive processPolicies(-1). Do not remove a
  // real policy flag and lose its obligatory effects, payment/history or Lua.
  for(std::size_t i=0;i<static_cast<std::size_t>(*count);++i)if(disabled[i]!=-1&&p.expected.has.at(disabled[i])){p.phase=BnwPolicyAdoptionPhase::recursive_policy_disable_pending;return;}
  p.reward_effects=effects;p.reward_cursor=1;
  auto store_scalar=[&](const char* member,std::int64_t delta,std::int64_t* shared){
   auto& field=named(raw.core.fields,member);const auto old=scalar(raw.core,member);need(old==p.expected.process_scalars.at(member)&&(!shared||*shared==old),"BNW policy reward raw/native/shared pair differs");
   const auto value=native_sum(old,delta);auto desired=p.expected;desired.process_scalars.at(member)=value;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const auto before=p.after;const bool changed=value!=old;need(!changed||before<UINT64_MAX,"BNW policy reward revision exhausted");const auto after=before+(changed?1u:0u);
   require_stored_current(*host);binding.lease.require_operation(binding.serial);need(field.values.size()==1&&field.values[0]==old&&(!shared||*shared==old),"BNW policy reward source changed before store");
   *registry=std::move(next_registry);field.values[0]=value;if(shared)*shared=value;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  };
  if(effects[0]>0)store_scalar("m_iAttackBonusTurns",effects[0],nullptr);p.reward_cursor=2;
  if(effects[1]>0){p.phase=BnwPolicyAdoptionPhase::golden_age_pending;return;}p.reward_cursor=3;
  advance_process_reward_tail();
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_reward_tail()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_disables_pending&&p.reward_cursor==3,"BNW actual policy reward tail unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  // These are the SAME complete PolicyInfo operands reached before attack/GA.
  // Resume after the actual change returned; do not replay attack or disables.
  const auto effects=p.reward_effects;auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy reward tail owned pair absent");
  auto store_scalar=[&](const char* member,std::int64_t delta,std::int64_t* shared){
   auto& field=named(raw.core.fields,member);const auto old=scalar(raw.core,member);need(old==p.expected.process_scalars.at(member)&&(!shared||*shared==old),"BNW policy reward raw/native/shared pair differs");
   const auto value=native_sum(old,delta);auto desired=p.expected;desired.process_scalars.at(member)=value;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const auto before=p.after;const bool changed=value!=old;need(!changed||before<UINT64_MAX,"BNW policy reward revision exhausted");const auto after=before+(changed?1u:0u);
   require_stored_current(*host);binding.lease.require_operation(binding.serial);need(field.values.size()==1&&field.values[0]==old&&(!shared||*shared==old),"BNW policy reward source changed before store");
   *registry=std::move(next_registry);field.values[0]=value;if(shared)*shared=value;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  };
  if(effects[2]>0&&!p.free_tech_choice_returned){p.phase=BnwPolicyAdoptionPhase::free_techs_pending;return;}p.reward_cursor=4;
  // Existing research-agreement getter actually reads this owned Science cache.
  need(state.science.profile==content::TechnologyProfile::bnw&&state.science.players.contains(p.actor),"BNW policy shared Science cache missing");
  store_scalar("m_iMedianTechPercentage",effects[3],&state.science.players.at(p.actor).median_tech_percent);p.reward_cursor=5;
  if(effects[4]>0){
   auto desired=p.expected;const auto free_policies=native_sum(desired.free_policies,effects[4]),ever=native_sum(desired.process_scalars.at("m_iNumFreePoliciesEver"),effects[4]);
   desired.free_policies=free_policies;desired.process_scalars.at("m_iNumFreePoliciesEver")=ever;
   auto& free=named(raw.core.fields,"m_iNumFreePolicies");auto& all=named(raw.core.fields,"m_iNumFreePoliciesEver");
   need(free.values.size()==1&&all.values.size()==1&&free.values[0]==p.expected.free_policies&&all.values[0]==p.expected.process_scalars.at("m_iNumFreePoliciesEver"),"BNW policy genuine free-policy source differs");
   auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;const auto before=p.after;need(before<UINT64_MAX,"BNW policy free reward revision exhausted");const auto after=before+1;
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   p.free_reward_before=p.expected.free_policies;p.free_reward_ever_before=p.expected.process_scalars.at("m_iNumFreePoliciesEver");p.free_reward_delta=effects[4];
   // Native SetNumFreePolicies increments ever FIRST, then current balance.
   *registry=std::move(next_registry);all.values[0]=ever;free.values[0]=free_policies;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);p.reward_cursor=6;
   if(!raw.cities.cities.empty()){p.phase=BnwPolicyAdoptionPhase::free_policy_notification_pending;return;}
  }
  p.reward_cursor=6;p.phase=BnwPolicyAdoptionPhase::one_shot_free_units_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::require_free_tech_text_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&p.reward_cursor==3&&p.reward_effects[2]>0&&p.reward_effects[2]<=INT32_MAX&&
  !p.free_tech_text_returned&&!p.free_tech_count_stored&&!p.free_tech_choice_returned&&
  ((p.phase==BnwPolicyAdoptionPhase::free_techs_pending&&!p.free_tech_text_attempted)||
   (p.phase==BnwPolicyAdoptionPhase::free_techs_executing&&p.free_tech_text_attempted)),
  "BNW policy free tech text requires reached original positive reward");
 need(detail::bnw_player_human_read(state,static_cast<std::int32_t>(p.actor),host.database()),"BNW free tech human chooseTech requires genuine human player");
 require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_free_tech_text_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::free_techs_pending&&!p.free_tech_text_attempted,"BNW free tech text dispatch unreached/repeated");
 p.free_tech_text_attempted=true;p.phase=BnwPolicyAdoptionPhase::free_techs_executing;
 try{require_free_tech_text_source(host,state);return p.hooks->impl_->lease;}catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_free_tech_text_and_count(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::free_techs_executing&&p.free_tech_text_attempted&&!p.free_tech_text_returned&&!p.free_tech_count_stored,"BNW free tech native count unissued/repeated");
 try{
  // Original processPolicies resolves PolicyInfo text BEFORE chooseTech reads
  // NO_SCIENCE. Only the actual SAME-VM selected-locale owner seals that work.
  engine.require_free_tech_text_return(*this);const auto host=p.host.lock();need(bool(host),"BNW free tech actual Host expired");
  require_free_tech_text_source(*host,*p.state);auto& binding=*p.hooks->impl_;auto& state=*p.state;
  auto setup=state.setup;const auto no_science=setup.game_option_value(host->database(),"GAMEOPTION_NO_SCIENCE");
  need(no_science>=INT32_MIN&&no_science<=INT32_MAX,"BNW free tech actual native option outside int");
  require_free_tech_text_source(*host,state);engine.require_free_tech_text_return(*this);
  const auto old=p.expected.process_scalars.at("m_iFreeTechCount");
  p.free_tech_choice_operands={static_cast<std::int32_t>(p.reward_effects[2]),static_cast<std::int32_t>(old),static_cast<std::int32_t>(old),static_cast<std::int32_t>(no_science)};
  if(no_science==1){
   // Genuine CvGame::isOption()==1 early return: no count assignment, dirty
   // setter, notification or technology grant occurs. Source resumes the
   // remaining native reward tail only AFTER this actual receiver returns.
   p.free_tech_text_returned=true;p.free_tech_choice_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_disables_pending;return;
  }
  auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,"m_iFreeTechCount");const auto value=native_sum(old,p.reward_effects[2]);
  need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&field.values[0]==old,"BNW free tech raw destination changed");
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW free tech native owned pair absent");
  auto desired=p.expected;desired.process_scalars.at("m_iFreeTechCount")=value;auto prepared=*registry;prepared.players.at(p.actor)=desired;auto expected=desired;
  const auto before=p.after;need(before<UINT64_MAX,"BNW free tech count revision exhausted");const auto after=before+1;
  const auto active=detail::bnw_game_active_player_read(state);require_free_tech_text_source(*host,state);engine.require_free_tech_text_return(*this);
  auto final_setup=state.setup;need(final_setup.game_option_value(host->database(),"GAMEOPTION_NO_SCIENCE")==no_science,"BNW free tech native option changed before store");
  require_free_tech_text_source(*host,state);engine.require_free_tech_text_return(*this);binding.lease.require_operation(binding.serial);
  // Re-resolve after ALL callbackful providers. No stale raw/registry pointer
  // is used if a source callback replaced native storage during validation.
  auto& final_raw=raw_player(state,p.actor);auto& final_field=named(final_raw.core.fields,"m_iFreeTechCount");auto* final_registry=state.setup.extensions.find<PolicyRegistry>();
  need(final_registry&&final_registry->players.at(p.actor)==p.expected&&final_field.encoding==saves::PlayerCoreEncoding::i32&&final_field.values.size()==1&&final_field.values[0]==old&&
   state.ui_source_revision==before&&detail::bnw_game_active_player_read(state)==active,"BNW free tech final paired source changed");
  *final_registry=std::move(prepared);final_field.values[0]=value;p.expected=std::move(expected);p.previous=before;p.after=after;state.ui_source_revision=after;
  p.free_tech_choice_operands[2]=static_cast<std::int32_t>(value);p.free_tech_text_returned=true;p.free_tech_count_stored=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=active==p.actor?BnwPolicyAdoptionPhase::free_tech_dirty_pending:BnwPolicyAdoptionPhase::free_tech_notification_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::require_free_tech_dirty_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&p.reward_cursor==3&&p.free_tech_text_returned&&p.free_tech_count_stored&&!p.free_tech_choice_returned&&!p.free_tech_dirty_returned&&
  p.free_tech_choice_operands[3]!=1&&p.expected.process_scalars.at("m_iFreeTechCount")==p.free_tech_choice_operands[2]&&
  ((p.phase==BnwPolicyAdoptionPhase::free_tech_dirty_pending&&!p.free_tech_dirty_attempted)||(p.phase==BnwPolicyAdoptionPhase::free_tech_dirty_executing&&p.free_tech_dirty_attempted)),
  "BNW free tech ResearchButtons requires actual paired count store");
 need(detail::bnw_game_active_player_read(state)==p.actor,"BNW free tech ResearchButtons active predicate changed");
 require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_free_tech_dirty_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::free_tech_dirty_pending&&!p.free_tech_dirty_attempted,"BNW free tech ResearchButtons unreached/repeated");
 p.free_tech_dirty_attempted=true;p.phase=BnwPolicyAdoptionPhase::free_tech_dirty_executing;
 try{require_free_tech_dirty_source(host,state);return p.hooks->impl_->lease;}catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_free_tech_dirty_return(const ui::BnwClientSelectionControl& selection)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::free_tech_dirty_executing&&p.free_tech_dirty_attempted&&!p.free_tech_dirty_returned,"BNW free tech ResearchButtons return unissued/repeated");
 try{selection.require_free_tech_dirty_return(*this);const auto host=p.host.lock();need(bool(host),"BNW free tech dirty Host expired");require_free_tech_dirty_source(*host,*p.state);selection.require_free_tech_dirty_return(*this);
  p.free_tech_dirty_returned=true;p.phase=BnwPolicyAdoptionPhase::free_tech_notification_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::require_free_tech_notification_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&p.reward_cursor==3&&p.free_tech_text_returned&&p.free_tech_count_stored&&!p.free_tech_choice_returned&&p.free_tech_choice_operands[3]!=1&&
  p.expected.process_scalars.at("m_iFreeTechCount")==p.free_tech_choice_operands[2]&&
  ((p.phase==BnwPolicyAdoptionPhase::free_tech_notification_pending&&!p.free_tech_notification_attempted)||(p.phase==BnwPolicyAdoptionPhase::free_tech_notification_executing&&p.free_tech_notification_attempted)),
  "BNW free tech notification requires actual chooseTech count predecessor");
 const auto active=detail::bnw_game_active_player_read(state);need(active!=p.actor||p.free_tech_dirty_returned,"BNW active free tech notification skipped ResearchButtons");
 require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_free_tech_notification_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::free_tech_notification_pending&&!p.free_tech_notification_attempted,"BNW free tech notification unreached/repeated");
 p.free_tech_notification_attempted=true;p.phase=BnwPolicyAdoptionPhase::free_tech_notification_executing;
 try{require_free_tech_notification_source(host,state);return p.hooks->impl_->lease;}catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_free_tech_notification_return(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::free_tech_notification_executing&&p.free_tech_notification_attempted&&!p.free_tech_choice_returned,"BNW free tech notification return unissued/repeated");
 try{engine.require_free_tech_notification_return(*this);const auto host=p.host.lock();need(bool(host),"BNW free tech notification Host expired");
  const auto before=p.after,after=p.state->ui_source_revision;need(after>=before&&after-before<=1,"BNW free tech concrete native Add/noop revision differs");
  p.previous=before;p.after=after;p.phase=BnwPolicyAdoptionPhase::uncertain;require_stored_current(*host);auto& binding=*p.hooks->impl_;
  if(after!=before)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);engine.require_free_tech_notification_return(*this);
  p.free_tech_choice_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_disables_pending;
  // Actual SourceOwner resumes reward tail AFTER Engine sealed return bookends.
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_free_tech_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

void BnwPolicyAdoptionPending::advance_process_golden_age()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::golden_age_pending&&p.reward_cursor==2&&!p.golden_age_attempted&&p.reward_effects[1]>0,"BNW actual policy GoldenAge change unreached/repeated");
 // Reserve BEFORE callbackful source, TraitInfo and GameSpeed providers. A
 // failed issuer cannot retry the already-reached change or duplicate a store.
 p.golden_age_attempted=true;
 try{
  require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);auto& state=*p.state;
  const auto player_modifier=p.expected.process_scalars.at("m_iGoldenAgeModifier");std::int64_t trait_modifier=0;
  {
   const auto traits=BnwMajorPlayerTraitsOwner::from_source_host(host,static_cast<std::int32_t>(p.actor));trait_modifier=traits->cache_value("m_iGoldenAgeDurationModifier");traits->require_current();
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
  }
  const auto modifier=native_sum(player_modifier,trait_modifier);auto turns=p.reward_effects[1];if(modifier>0)turns=native_product(turns,native_sum(100,modifier))/100;
  need(state.world.pregame&&state.world.pregame->layout==saves::GameStateLayout::expansion2_version1,"BNW GoldenAge actual Pregame binding absent");const auto speed=state.world.pregame->archive.integers.at("game_speed");need(speed==state.setup.speed&&speed>=0&&speed<=INT32_MAX,"BNW GoldenAge actual GameSpeed differs");
  std::optional<std::int64_t> percent;
  const auto report=host->database().prefetch_info("GameSpeeds",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(speed)){need(!percent,"BNW GoldenAge duplicate native GameSpeed");const auto type=row.text("Type").value_or("");const auto* mapped=host->types().find(type);need(mapped&&mapped->table=="GameSpeeds"&&mapped->id==speed,"BNW GoldenAge selected GameSpeed/remap differs");percent=row.integer("GoldenAgePercent");need(*percent>=INT32_MIN&&*percent<=INT32_MAX,"BNW GoldenAge native speed percent outside int");}return true;
  });need(report.all_succeeded()&&percent,"BNW GoldenAge complete current speed Info absent");
  // Exactly the SDK multiply/divide boundaries, including nonpositive modifier
  // short circuit and speed rounding to a zero change. Undefined native int
  // overflow domains fail before the actual destination store.
  turns=native_product(turns,*percent)/100;require_stored_current(*host);binding.lease.require_operation(binding.serial);
  const auto old=p.expected.process_scalars.at("m_iGoldenAgeTurns"),value=native_sum(old,turns);need(old>=0&&value>=0,"BNW GoldenAge native turns nonnegative invariant failed");
  p.golden_age_operands={static_cast<std::int32_t>(p.reward_effects[1]),static_cast<std::int32_t>(player_modifier),static_cast<std::int32_t>(trait_modifier),static_cast<std::int32_t>(modifier),static_cast<std::int32_t>(*percent),static_cast<std::int32_t>(turns),static_cast<std::int32_t>(old),static_cast<std::int32_t>(value)};
  if(turns==0){
   // changeGoldenAgeTurns(0) reaches no store, map, replay, notification or UI.
   p.golden_age_change_returned=true;p.reward_cursor=3;p.phase=BnwPolicyAdoptionPhase::policy_disables_pending;advance_process_reward_tail();return;
  }
  auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,"m_iGoldenAgeTurns");need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&field.values[0]==old&&state.cities.players.at(p.actor).golden_age==(old>0),"BNW GoldenAge raw/owned source changed");
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW GoldenAge paired PolicyRegistry absent");auto desired=p.expected;desired.process_scalars.at("m_iGoldenAgeTurns")=value;auto prepared=*registry;prepared.players.at(p.actor)=desired;auto expected_copy=desired;
  const auto before=p.after;need(before<UINT64_MAX,"BNW GoldenAge revision exhausted");const auto after=before+1;require_stored_current(*host);binding.lease.require_operation(binding.serial);
  *registry=std::move(prepared);field.values[0]=value;state.cities.players.at(p.actor).golden_age=value>0;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.golden_age_turns_stored=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  if((old>0)!=(value>0)){
   // The actual native store is earned, but source next calls the WHOLE map
   // updateYield before NumGoldenAges, replay, DLL event and notification.
   // This frontier cannot reuse owned-only CvPlayer::updateYield or skip them.
   p.map_cursor=0;p.map_complete=false;p.phase=BnwPolicyAdoptionPhase::golden_age_map_pending;return;
  }
  const auto active=detail::bnw_game_active_player_read(state);require_stored_current(*host);binding.lease.require_operation(binding.serial);
  if(active==p.actor){p.phase=BnwPolicyAdoptionPhase::golden_age_dirty_pending;return;}
  // Actual unchanged GoldenAge status + native inactive player reaches none of
  // the transition effects or GameData setter, so this change really returned.
  p.golden_age_change_returned=true;p.reward_cursor=3;p.phase=BnwPolicyAdoptionPhase::policy_disables_pending;advance_process_reward_tail();
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_golden_age_history()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();
 need(host&&p.phase==BnwPolicyAdoptionPhase::golden_age_history_pending&&p.golden_age_attempted&&p.golden_age_turns_stored&&p.golden_age_map_returned&&
  !p.golden_age_history_attempted&&!p.golden_age_count_stored&&!p.golden_age_change_returned&&p.reward_cursor==2&&p.golden_age_operands[6]==0&&p.golden_age_operands[7]>0,
  "BNW GoldenAge history requires actual once-only begun transition after whole-map return");
 p.golden_age_history_attempted=true;
 try{
  require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);auto& state=*p.state;
  // This persisted standalone service is an explicit local backend choice,
  // not an original BNW flag or assertion that Steam unlocked an achievement.
  // Its disabled backend has no gameplay side effect. Replay/DLL/notification
  // remain mandatory and cannot be satisfied by this configuration check.
  const auto services=state.standalone_services;saves::require_supported_standalone_services(services);
  need(services.steam_achievements==saves::StandaloneServiceBackend::backend_disabled,"BNW GoldenAge achievement backend has no genuine implementation");
  auto& field=named(raw_player(state,p.actor).core.fields,"m_iNumGoldenAges");const auto old=p.expected.process_scalars.at("m_iNumGoldenAges");
  const auto value=native_sum(old,1);need(old>=0&&field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&field.values[0]==old,"BNW GoldenAge native history counter source changed");
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW GoldenAge history paired registry absent");
  auto desired=p.expected;desired.process_scalars.at("m_iNumGoldenAges")=value;auto prepared=*registry;prepared.players.at(p.actor)=desired;auto expected_copy=desired;
  const auto before=p.after;need(before<UINT64_MAX,"BNW GoldenAge history revision exhausted");const auto after=before+1;
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  need(state.standalone_services==services&&field.values[0]==old,"BNW GoldenAge history configuration/counter changed before actual store");
  // CvPlayer::ChangeNumGoldenAges -> SetNumGoldenAges writes this scalar
  // before optional achievement work and before addReplayMessage. Keep the
  // exact paired PolicyRegistry store; never manufacture a replay completion.
  *registry=std::move(prepared);field.values[0]=value;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;
  p.golden_age_count_stored=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  saves::require_supported_standalone_services(state.standalone_services);need(state.standalone_services==services,"BNW GoldenAge explicit achievement backend changed after counter store");
  p.golden_age_achievement_backend_disabled=true;p.phase=BnwPolicyAdoptionPhase::golden_age_replay_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::require_golden_age_replay_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&p.golden_age_attempted&&p.golden_age_turns_stored&&p.golden_age_map_returned&&p.golden_age_history_attempted&&p.golden_age_count_stored&&
  p.golden_age_achievement_backend_disabled&&!p.golden_age_replay_returned&&!p.golden_age_change_returned&&p.reward_cursor==2&&p.golden_age_operands[6]==0&&p.golden_age_operands[7]>0&&
  state.world.history.layout==saves::GameStateLayout::expansion2_version1&&
  ((p.phase==BnwPolicyAdoptionPhase::golden_age_replay_pending&&!p.golden_age_replay_attempted)||(p.phase==BnwPolicyAdoptionPhase::golden_age_replay_executing&&p.golden_age_replay_attempted)),
  "BNW GoldenAge replay requires actual begun map/history return and same native phase");
 saves::require_supported_standalone_services(state.standalone_services);
 need(state.standalone_services.steam_achievements==saves::StandaloneServiceBackend::backend_disabled,"BNW GoldenAge explicit achievement backend changed before replay");
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_golden_age_replay_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_replay_pending&&!p.golden_age_replay_attempted,"BNW GoldenAge replay dispatch unreached/repeated");
 // Reserve before any Engine/localization/civilization/currentness provider.
 p.golden_age_replay_attempted=true;p.phase=BnwPolicyAdoptionPhase::golden_age_replay_executing;
 try{require_golden_age_replay_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_golden_age_replay_return(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_replay_executing&&p.golden_age_replay_attempted&&!p.golden_age_replay_returned,"BNW GoldenAge replay return unissued/repeated");
 try{
  // Private actual receiver seal includes exact appended original replay,
  // full immutable prior prefix, SAME canonical Pending/world and source.
  engine.require_golden_age_replay_return(*this);const auto host=p.host.lock();need(bool(host),"BNW GoldenAge replay Host expired");
  const auto before=p.after,after=p.state->ui_source_revision;need(before<UINT64_MAX&&after==before+1,"BNW GoldenAge actual replay append revision differs");
  p.previous=before;p.after=after;p.phase=BnwPolicyAdoptionPhase::uncertain;require_stored_current(*host);
  auto& binding=*p.hooks->impl_;binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  engine.require_golden_age_replay_return(*this);p.golden_age_replay_returned=true;p.phase=BnwPolicyAdoptionPhase::golden_age_gameplay_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_golden_age_replay_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

void BnwPolicyAdoptionPending::require_golden_age_gameplay_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 const bool begun=p.golden_age_operands[7]>0;
 need(&state==p.state.get()&&p.golden_age_attempted&&p.golden_age_turns_stored&&p.golden_age_map_returned&&!p.golden_age_gameplay_returned&&!p.golden_age_gameplay_stored&&
  !p.golden_age_change_returned&&p.reward_cursor==2&&(p.golden_age_operands[6]>0)!=begun&&
  (begun?(p.golden_age_history_attempted&&p.golden_age_count_stored&&p.golden_age_achievement_backend_disabled&&p.golden_age_replay_returned):
   (!p.golden_age_history_attempted&&!p.golden_age_count_stored&&!p.golden_age_replay_attempted&&!p.golden_age_replay_returned))&&
  ((p.phase==BnwPolicyAdoptionPhase::golden_age_gameplay_pending&&!p.golden_age_gameplay_attempted)||(p.phase==BnwPolicyAdoptionPhase::golden_age_gameplay_executing&&p.golden_age_gameplay_attempted)),
  "BNW GoldenAge facade requires real changed-state map/history/replay predecessor");
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_golden_age_gameplay_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_gameplay_pending&&!p.golden_age_gameplay_attempted,"BNW GoldenAge facade dispatch unreached/repeated");
 p.golden_age_gameplay_attempted=true;p.phase=BnwPolicyAdoptionPhase::golden_age_gameplay_executing;
 try{require_golden_age_gameplay_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_golden_age_gameplay_return(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_gameplay_executing&&p.golden_age_gameplay_attempted&&!p.golden_age_gameplay_stored&&!p.golden_age_gameplay_returned,"BNW GoldenAge facade return unissued/repeated");
 try{
  // A real Civis-owned FIFO event store/normal native enqueue return earns
  // this revision. It is not an original DLL callback, renderer or audio ACK.
  engine.require_golden_age_gameplay_return(*this);const auto host=p.host.lock();need(bool(host),"BNW GoldenAge facade Host expired");
  const auto before=p.after,after=p.state->ui_source_revision;need(before<UINT64_MAX&&after==before+1,"BNW GoldenAge concrete facade event revision differs");
  p.golden_age_gameplay_stored=true;p.previous=before;p.after=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  engine.require_golden_age_gameplay_return(*this);p.golden_age_gameplay_returned=true;p.phase=BnwPolicyAdoptionPhase::golden_age_notification_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_golden_age_gameplay_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}
void BnwPolicyAdoptionPending::require_golden_age_notification_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&p.golden_age_attempted&&p.golden_age_turns_stored&&p.golden_age_map_returned&&p.golden_age_gameplay_stored&&p.golden_age_gameplay_returned&&
  !p.golden_age_notification_returned&&!p.golden_age_change_returned&&p.reward_cursor==2&&(p.golden_age_operands[6]>0)!=(p.golden_age_operands[7]>0)&&
  ((p.phase==BnwPolicyAdoptionPhase::golden_age_notification_pending&&!p.golden_age_notification_attempted)||(p.phase==BnwPolicyAdoptionPhase::golden_age_notification_executing&&p.golden_age_notification_attempted)),
  "BNW GoldenAge notification requires actual same native facade return");
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_golden_age_notification_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_notification_pending&&!p.golden_age_notification_attempted,"BNW GoldenAge notification dispatch unreached/repeated");
 p.golden_age_notification_attempted=true;p.phase=BnwPolicyAdoptionPhase::golden_age_notification_executing;
 try{require_golden_age_notification_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_golden_age_notification_return(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_notification_executing&&p.golden_age_notification_attempted&&!p.golden_age_notification_returned,"BNW GoldenAge notification return unissued/repeated");
 try{
  // Exact native nullable/debug/human/redundant/Add return is sealed by the
  // actual owner of the notification ring, transients and publication queue.
  engine.require_golden_age_notification_return(*this);const auto host=p.host.lock();need(bool(host),"BNW GoldenAge notification Host expired");
  const auto before=p.after,after=p.state->ui_source_revision;need(after>=before&&after-before<=1,"BNW GoldenAge notification native revision differs");
  p.previous=before;p.after=after;p.phase=BnwPolicyAdoptionPhase::uncertain;require_stored_current(*host);
  auto& binding=*p.hooks->impl_;if(after!=before)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  engine.require_golden_age_notification_return(*this);p.golden_age_notification_returned=true;
  const auto active=detail::bnw_game_active_player_read(*p.state);require_stored_current(*host);binding.lease.require_operation(binding.serial);engine.require_golden_age_notification_return(*this);
  if(active==p.actor){p.phase=BnwPolicyAdoptionPhase::golden_age_dirty_pending;return;}
  // Keep the sealed receiver's revision intact through its actual return.
  // SourceOwner alone invokes the native reward tail after receiver bookends.
  p.golden_age_change_returned=true;p.reward_cursor=3;p.phase=BnwPolicyAdoptionPhase::policy_disables_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_golden_age_notification_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

void BnwPolicyAdoptionPending::require_golden_age_dirty_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&p.golden_age_attempted&&p.golden_age_turns_stored&&!p.golden_age_change_returned&&p.reward_cursor==2&&p.golden_age_operands[5]!=0&&
  ((p.golden_age_operands[6]>0)==(p.golden_age_operands[7]>0)||(p.golden_age_map_returned&&p.golden_age_gameplay_stored&&p.golden_age_gameplay_returned&&p.golden_age_notification_returned))&&p.expected.process_scalars.at("m_iGoldenAgeTurns")==p.golden_age_operands[7]&&
  ((p.phase==BnwPolicyAdoptionPhase::golden_age_dirty_pending&&!p.golden_age_dirty_attempted)||(p.phase==BnwPolicyAdoptionPhase::golden_age_dirty_executing&&p.golden_age_dirty_attempted)),"BNW GoldenAge GameData requires actual paired unchanged-state change");
 need(detail::bnw_game_active_player_read(state)==p.actor,"BNW GoldenAge GameData is not the actual active player branch");require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_golden_age_dirty_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_dirty_pending&&!p.golden_age_dirty_attempted,"BNW GoldenAge GameData dispatch unreached/repeated");
 // Actual active-player predicate was reached after its native turn store.
 // Reserve before any rules/currentness/retained Engine/Selection provider.
 p.golden_age_dirty_attempted=true;p.phase=BnwPolicyAdoptionPhase::golden_age_dirty_executing;
 try{require_golden_age_dirty_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_golden_age_dirty_return(const ui::BnwClientSelectionControl& selection)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::golden_age_dirty_executing&&p.golden_age_dirty_attempted&&!p.golden_age_change_returned,"BNW GoldenAge GameData return unissued/repeated");
 try{
  // Sealed concrete setter return proves SAME canonical Pending/world/source,
  // unchanged world revision and exactly one actual GameData11 occurrence.
  // There is no caller ACK, display flag or generic currentness promotion.
  selection.require_golden_age_dirty_return(*this);const auto host=p.host.lock();need(bool(host),"BNW GoldenAge GameData Host expired");require_golden_age_dirty_source(*host,*p.state);selection.require_golden_age_dirty_return(*this);
  // The concrete receiver must finish its sealed SAME-world bookends first.
  // SourceOwner resumes the reward tail only after that genuine return; its
  // science/free-policy stores must not stale the receiver mid-return.
  p.golden_age_change_returned=true;p.reward_cursor=3;p.phase=BnwPolicyAdoptionPhase::policy_disables_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_golden_age_dirty_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

std::shared_ptr<const BnwPolicyAdoptionPending> BnwPolicyAdoptionPending::retain_factory_owner(const std::shared_ptr<const BnwPolicyAdoptionPending>& supplied){
 need(supplied&&supplied->creator_==std::this_thread::get_id()&&!supplied->retired_.load(),"BNW policy notification foreign/retired facade before context borrow");need(bool(supplied->impl_),"BNW policy notification genuine Pending absent");const auto& p=*supplied->impl_;
 need(p.thread==std::this_thread::get_id()&&!p.retired.load(),"BNW policy notification creator retired/foreign before source borrow");
 const auto canonical=p.self.lock();need(canonical&&canonical.get()==supplied.get()&&!canonical.owner_before(supplied)&&!supplied.owner_before(canonical),"BNW policy notification forged Pending control");return canonical;
}
void BnwPolicyAdoptionPending::require_free_policy_notification_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&(p.phase==BnwPolicyAdoptionPhase::free_policy_notification_pending||p.phase==BnwPolicyAdoptionPhase::free_policy_notification_executing)&&
  (p.phase==BnwPolicyAdoptionPhase::free_policy_notification_pending?!p.notification_attempted:p.notification_attempted)&&p.reward_cursor==6&&p.free_reward_delta>0&&
  p.expected.free_policies==native_sum(p.free_reward_before,p.free_reward_delta)&&p.expected.process_scalars.at("m_iNumFreePoliciesEver")==native_sum(p.free_reward_ever_before,p.free_reward_delta)&&
  !raw_player(state,p.actor).cities.cities.empty()&&state.cities.players.at(p.actor).cities.size()==raw_player(state,p.actor).cities.cities.size(),
  "BNW policy notification requires actual earned-count/city/native phase");
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_free_policy_notification_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;
 need(!p.notification_attempted&&p.phase==BnwPolicyAdoptionPhase::free_policy_notification_pending,"BNW policy notification native Add already attempted");
 // Pure creator/phase checks precede reservation. No callbackful rules/source
 // currentness, locale, allocation or target invocation can reissue Add.
 p.notification_attempted=true;p.phase=BnwPolicyAdoptionPhase::free_policy_notification_executing;
 try{require_free_policy_notification_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_free_policy_notification_return(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.notification_attempted&&p.phase==BnwPolicyAdoptionPhase::free_policy_notification_executing,"BNW policy unissued notification native return");
 try{
  // Engine's sealed return proves its actual nullable/redundant/debug/human
  // native branch, ring/DLL effects, SAME canonical Pending/world/generation,
  // and exact stored before/after revision. It is pure, never a caller ACK.
  engine.require_free_policy_notification_return(*this);const auto host=p.host.lock();need(bool(host),"BNW policy notification Host expired");
  const auto before=p.after,after=p.state->ui_source_revision;need(after>=before,"BNW policy notification revision regressed");
  p.previous=before;p.after=after;p.phase=BnwPolicyAdoptionPhase::uncertain;require_stored_current(*host);
  auto& binding=*p.hooks->impl_;if(after!=before)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  engine.require_free_policy_notification_return(*this);p.phase=BnwPolicyAdoptionPhase::one_shot_free_units_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_free_policy_notification_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

void BnwPolicyAdoptionPending::advance_process_one_shot_units()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::one_shot_free_units_pending,"BNW policy one-shot units unreached");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  std::array<std::int64_t,5> effects{};std::size_t rows=0;
  const auto report=host->database().query_rows({"SELECT IncludesOneShotFreeUnits, NumFreeGreatPeople, MinorGreatPeopleAllies, AddReformationBelief, StealTechFasterModifier FROM Policies WHERE Type = ?",{p.rules.policies.at(p.target).type}},[&](const content::RulesRowView& row){
   ++rows;for(std::size_t i=0;i<effects.size();++i){effects[i]=row.integer_at(static_cast<int>(i));need(effects[i]>=INT32_MIN&&effects[i]<=INT32_MAX,"BNW policy unit/post-unit operand outside int");}});
  need(report.all_succeeded()&&report.sqlite_code==0&&rows==1&&(effects[0]==0||effects[0]==1)&&(effects[2]==0||effects[2]==1)&&(effects[3]==0||effects[3]==1),"BNW policy complete one-shot native object unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy native one-shot cache absent");
  if(effects[0]&&!p.expected.one_shot_free_units.at(p.target)){
   auto& flags=named(named(raw.strategy.blocks,"policies").hashes,"one_shot_free_units_fired");auto prepared_flags=flags;set_flag(prepared_flags,p.rules.policies.at(p.target).hash,true);
   auto desired=p.expected;desired.one_shot_free_units.at(p.target)=true;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const auto before=p.after;need(before<UINT64_MAX,"BNW policy one-shot flag revision exhausted");const auto after=before+1;
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   *registry=std::move(next_registry);flags.rows.swap(prepared_flags.rows);flags.wire_count=prepared_flags.wire_count;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);p.units_reward_cursor=1;
   if(effects[1]>0){
    auto& field=named(raw.core.fields,"m_iNumFreeGreatPeople");const auto old=scalar(raw.core,"m_iNumFreeGreatPeople"),value=native_sum(old,effects[1]);need(old==p.expected.process_scalars.at("m_iNumFreeGreatPeople"),"BNW policy free great person source differs");
    auto next=p.expected;next.process_scalars.at("m_iNumFreeGreatPeople")=value;auto next_registry=*registry;next_registry.players.at(p.actor)=next;auto expected_copy=next;
    const auto before=p.after;need(before<UINT64_MAX,"BNW policy free great person revision exhausted");const auto after=before+1;
    require_stored_current(*host);binding.lease.require_operation(binding.serial);
    *registry=std::move(next_registry);field.values[0]=value;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
    require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);p.units_reward_cursor=2;p.phase=BnwPolicyAdoptionPhase::free_great_people_pending;return;
   }
   const auto classes=actual_policy_catalog(*host,"UnitClasses");const auto free=actual_policy_linked_values(*host,"UnitClasses","Policy_FreeUnitClasses","UnitClassType","Count",p.rules.policies.at(p.target).type,classes.size());
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   const auto capital_id=scalar(raw.core,"m_iCapitalCityID");need(state.cities.players.at(p.actor).capital==capital_id,"BNW policy actual capital pair changed");
   const auto* capital=resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(capital_id));
   if(capital&&std::any_of(free.begin(),free.end(),[](auto n){return n>0;})){p.phase=BnwPolicyAdoptionPhase::free_units_create_pending;return;}
   // Real native no-capital or whole selected count<=0 branch: no initUnit.
  }
  p.units_reward_cursor=3;p.post_reformation_spy_change=effects[4];
  if(effects[2]){p.phase=BnwPolicyAdoptionPhase::minor_great_people_allies_pending;return;}
  // CvPlayer.cpp21471: isHuman precedes the PolicyInfo predicate. A true
  // policy flag alone does not call Add: the native religion and nullable
  // Notifications getters must also reach their real positive branches.
  const bool reformation_human=detail::bnw_player_human_read(state,static_cast<std::int32_t>(p.actor),host->database());
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  if(reformation_human&&effects[3]){
   const auto raw_religions=state.world.religion_culture.religions;
   const auto owned_religions=state.cities.religions;
   const auto religion_current=[&]{
    require_stored_current(*host);binding.lease.require_operation(binding.serial);
    need(state.world.religion_culture.layout==saves::GameStateLayout::expansion2_version1&&
     state.cities.profile==content::TechnologyProfile::bnw&&state.cities.pantheon==0&&
     state.world.religion_culture.religions.size()==raw_religions.size()&&state.cities.religions.size()==owned_religions.size()&&
     raw_religions.size()==owned_religions.size(),"BNW reformation actual religion owner/order differs");
    for(std::size_t i=0;i<raw_religions.size();++i){
     const auto& old=raw_religions[i];const auto& live=state.world.religion_culture.religions[i];
     const auto& owned=state.cities.religions[i];const auto& prior=owned_religions[i];
     need(live.religion==old.religion&&live.founder==old.founder&&live.pantheon==old.pantheon&&
      live.beliefs.belief_hashes==old.beliefs.belief_hashes&&owned.religion==prior.religion&&owned.founder==prior.founder&&
      owned.beliefs==prior.beliefs&&owned.religion==live.religion&&owned.founder==live.founder,
      "BNW reformation reached raw/owned religious source changed");
    }
   };
   religion_current();
   // GetReligionCreatedByPlayer scans the whole source list and retains the
   // LAST founded non-pantheon religion; it does not use current city majority.
   std::int64_t created=-1;
   for(const auto& religion:raw_religions){
    need(religion.founder>=-1&&religion.founder<64&&religion.religion>=-1&&religion.religion<=INT32_MAX,
     "BNW reformation native religion identity outside source domain");
    if(religion.founder==p.actor&&!religion.pantheon)created=religion.religion;
   }
   if(created>0){
    // GetReligion(created, actor) returns the FIRST matching real religion.
    // BNW has no serialized reformed flag: HasAddedReformationBelief scans
    // the selected Belief Info entries, preserving belief order and duplicates.
    bool added=false;
    const auto religion=std::find_if(raw_religions.begin(),raw_religions.end(),[&](const auto& value){return value.religion==created;});
    if(religion!=raw_religions.end()){
     const auto index=static_cast<std::size_t>(religion-raw_religions.begin());
     std::vector<std::int64_t> mapped;
     for(const auto hash:religion->beliefs.belief_hashes){
      const auto* info=hash?host->types().find_hash(hash):nullptr;
      if(!info||info->id<0)continue; // Native unknown hash maps to NO_BELIEF/null Info.
      need(info->table=="Beliefs"&&info->id<=INT32_MAX,"BNW reformation actual belief Info remap differs");mapped.push_back(info->id);
     }
     need(mapped==owned_religions[index].beliefs,"BNW reformation native/owned belief vector differs");
     for(const auto belief:mapped){
      std::optional<bool> actual;std::size_t matches=0;
      const auto report=host->database().prefetch_info("Beliefs",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
       if(slot==static_cast<std::uint64_t>(belief)){++matches;const auto* info=host->types().find(row.text("Type").value_or(""));
        need(info&&info->table=="Beliefs"&&info->id==belief,"BNW reformation selected Belief Info identity differs");
        const auto value=row.integer("Reformation");need(value==0||value==1,"BNW reformation native Belief flag outside bool");actual=value!=0;
       }return true;
      });
      need(report.all_succeeded()&&matches==1&&actual.has_value(),"BNW reformation selected Belief Info unavailable");religion_current();
      if(*actual){added=true;break;}
     }
    }
    religion_current();
    if(!added){
     const auto& notifications=raw_player(state,p.actor).administration;
     need(notifications.layout==saves::GameStateLayout::expansion2_version1&&!notifications.blocks.empty()&&name(notifications.blocks.front().name)=="notifications",
      "BNW reformation actual Notifications owner absent");
     const auto present=named(notifications.blocks.front().fields,"has_notifications").value;
     need(present==0||present==1,"BNW reformation native Notifications pointer flag outside bool");
     religion_current();
     if(present){
      need(!p.reformation_notification_attempted&&!p.reformation_notification_owner&&!p.reformation_notification_transients,
       "BNW reformation native positive guard already captured");
      // Capture the actually reached owner after final religion/source
      // bookends; no raw row retained across that callbackful boundary.
      const auto& actual_owner=named(raw_player(state,p.actor).administration.blocks,"notifications");
      need(named(actual_owner.fields,"has_notifications").value==1&&named(actual_owner.fields,"version").value==2&&
       named(actual_owner.fields,"m_ePlayer").value==p.actor&&reformation_notification_owner_same(actual_owner,actual_owner)&&
       state.notification_load_transients.current&&state.notification_load_transients.players.contains(p.actor),
       "BNW reformation actual imported Init/Read Notifications lifecycle unavailable");
      auto owner=actual_owner;auto transients=state.notification_load_transients;
      p.reformation_raw_religions=raw_religions;p.reformation_owned_religions=owned_religions;
      p.reformation_effect_policy=p.target;p.reformation_notification_owner=std::move(owner);
      p.reformation_notification_transients=std::move(transients);p.phase=BnwPolicyAdoptionPhase::reformation_pending;return;
     }
    }
   }
  }
  p.phase=BnwPolicyAdoptionPhase::post_reformation_pending;
  advance_process_post_reformation_tail();
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::advance_process_post_reformation_tail()const{
 (void)phase();auto& p=*impl_;need(!p.post_reformation_tail_attempted&&p.units_reward_cursor==3&&
  ((p.phase==BnwPolicyAdoptionPhase::post_reformation_pending&&!p.reformation_notification_attempted)||
   (p.phase==BnwPolicyAdoptionPhase::reformation_returned&&p.reformation_notification_attempted&&p.reformation_native_returned&&p.reformation_returned)),
  "BNW post-Reformation suffix unearned/repeated");
 // Reserve before every callbackful currentness/DB/VM supplier. A throwing
 // native prefix is historical and may not re-enter the unit reward stage.
 p.post_reformation_tail_attempted=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
 try{const auto host=p.host.lock();need(bool(host),"BNW post-Reformation Host expired");
  require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);auto& state=*p.state;
  if(p.post_reformation_spy_change!=0){
   // UpdateSpies calls GetCityWithSpy in native vector order. The exact
   // (-1,-1) coordinate pair returns NULL before map/city/rate queries;
   // an empty list likewise completes without a city-progress mutation.
   // Assigned or partially specified coordinates still require the genuine
   // map/city/espionage UpdateCity owner, never a fabricated zero rate.
   const auto& espionage=raw_player(state,p.actor).espionage;
   need(espionage.layout==saves::GameStateLayout::expansion2_version1,
    "BNW policy UpdateSpies actual player espionage owner absent");
   const auto& spies=named(espionage.blocks,"player_espionage");
   const auto count=named(spies.fields,"m_aSpyList_wire_count").value;
   need(count>=0&&static_cast<std::uint64_t>(count)<=UINT32_MAX,
    "BNW policy UpdateSpies native vector count outside uint loop");
   std::size_t ordinal=0;
   for(const auto& spy:spies.children)if(name(spy.name)=="spy"){
    need(spy.index==ordinal&&static_cast<std::uint64_t>(ordinal)<static_cast<std::uint64_t>(count),
     "BNW policy UpdateSpies native vector order/count differs");
    ++ordinal;
   }
   need(static_cast<std::uint64_t>(ordinal)==static_cast<std::uint64_t>(count),
    "BNW policy UpdateSpies native vector omits spy records");
   for(const auto& spy:spies.children)if(name(spy.name)=="spy"){
    const auto x=named(spy.fields,"m_iCityX").value,y=named(spy.fields,"m_iCityY").value;
    need(x>=INT32_MIN&&x<=INT32_MAX&&y>=INT32_MIN&&y<=INT32_MAX,
     "BNW policy UpdateSpies native city coordinates outside int");
    if(x!=-1||y!=-1){require_stored_current(*host);binding.lease.require_operation(binding.serial);p.phase=BnwPolicyAdoptionPhase::spy_progress_pending;return;}
   }
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
  }
  // Original final map loop calls updateYield for ANY matching resource plot.
  // Validate the complete source catalogue/map, never assume absence from old
  // cached yields or only inspect our player's owned plots.
  const auto catalog=actual_policy_catalog(*host,"Resources");std::map<std::int64_t,std::int64_t> reveals;
  const auto resources=host->database().prefetch_info("Resources",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   need(slot<catalog.size()&&row.text("Type").value_or("")==catalog[static_cast<std::size_t>(slot)],"BNW policy current Resource Info collection differs");
   const auto ref=row.text("PolicyReveal").value_or("");const auto* info=ref.empty()?nullptr:host->types().find(ref);need(ref.empty()||(info&&info->table=="Policies"),"BNW policy Resource.PolicyReveal unresolved");reveals.emplace(static_cast<std::int64_t>(slot),info?info->id:-1);return true;
  });need(resources.all_succeeded()&&reveals.size()==catalog.size(),"BNW policy complete Resource.PolicyReveal catalogue unavailable");
  need(state.world.map_plots&&state.world.map_projection&&state.world.map_plots->layout==saves::GameStateLayout::expansion2_version1,"BNW policy resource map actual owners missing");
  const auto& raw_map=*state.world.map_plots;const auto& map=*state.world.map_projection;const auto count=state.world.map_header.grid.size();
  need(count>0&&count<=INT32_MAX&&raw_map.plots.size()==count&&map.cells.size()==count&&map.grid.width==state.world.map_header.grid.width&&map.grid.height==state.world.map_header.grid.height,"BNW policy full resource map source dimensions differ");
  p.resource_policy_reveals=reveals;p.resource_yield_returned_plots.clear();
  p.resource_yield_returned_plots.reserve(raw_map.plots.size());
  p.resource_layout_returned_plots.clear();p.resource_layout_returned_plots.reserve(raw_map.plots.size());
  p.resource_layout_attempted=false;p.resource_layout_stored=false;
  p.resource_loop_reached=true;p.resource_loop_returned=false;p.resource_reveal_attempted=false;p.resource_scanned=0;
  for(std::size_t i=0;i<raw_map.plots.size();++i){const auto& plot=raw_map.plots[i];const auto& cell=map.cells[i];const auto xy=map.grid.coordinates(i);
   const auto hash=named(plot.fields,"resource_type_hash").value,owner=named(plot.fields,"owner").value;
   need(plot.index==i&&cell.index==i&&plot.x==xy[0]&&plot.y==xy[1]&&cell.x==plot.x&&cell.y==plot.y&&hash>=0&&hash<=UINT32_MAX&&cell.resource_type_hash&&*cell.resource_type_hash==static_cast<std::uint64_t>(hash)&&cell.owner&&*cell.owner==owner&&owner>=-1&&owner<64,"BNW policy resource paired plot identity differs");
   if(!hash){need(!cell.resource_type||*cell.resource_type==-1,"BNW policy actual NO_RESOURCE substituted");continue;}
   const auto* info=host->types().find_hash(static_cast<std::uint64_t>(hash));need(info&&info->table=="Resources"&&reveals.contains(info->id)&&(!cell.resource_type||*cell.resource_type==info->id),"BNW policy actual plot resource remap differs");
   if(reveals.at(info->id)==p.target){p.map_cursor=i;p.resource_scanned=i;p.policy_resource_plot=static_cast<std::int64_t>(i);p.phase=BnwPolicyAdoptionPhase::resource_policy_yield_pending;return;}
  }
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  p.resource_scanned=raw_map.plots.size();p.resource_loop_returned=true;
  p.phase=BnwPolicyAdoptionPhase::policy_happiness_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

bool BnwPolicyAdoptionPending::resource_policy_loop_reached()const{(void)phase();return impl_->resource_loop_reached;}
bool BnwPolicyAdoptionPending::resource_policy_loop_returned()const{(void)phase();return impl_->resource_loop_returned;}
std::uint64_t BnwPolicyAdoptionPending::resource_policy_scanned()const{(void)phase();return impl_->resource_scanned;}
std::vector<std::int64_t> BnwPolicyAdoptionPending::resource_policy_yield_returned_plots()const{(void)phase();return impl_->resource_yield_returned_plots;}
std::optional<std::int64_t> BnwPolicyAdoptionPending::resource_policy_layout_plot()const{
 (void)phase();const auto& p=*impl_;return (p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_pending||p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_executing||p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_returned||(p.phase==BnwPolicyAdoptionPhase::uncertain&&p.resource_layout_attempted&&p.resource_layout_stored))?p.policy_resource_plot:std::nullopt;
}
std::vector<std::int64_t> BnwPolicyAdoptionPending::resource_policy_layout_returned_plots()const{(void)phase();return impl_->resource_layout_returned_plots;}
bool BnwPolicyAdoptionPending::resource_policy_layout_effects_current()const{
 (void)phase();const auto& p=*impl_;if(p.resource_layout_returned_plots.empty())return true;
 const auto selection=p.resource_layout_selection.lock();if(!selection)return false;
 for(const auto plot:p.resource_layout_returned_plots)if(!selection->resource_policy_plot_layout_matches_without_callbacks(*p.state,p.generation,plot))return false;
 return true;
}
void BnwPolicyAdoptionPending::require_resource_policy_layout_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 need(&state==p.state.get()&&p.resource_loop_reached&&!p.resource_loop_returned&&p.resource_reveal_attempted&&!p.plot_child&&
  p.policy_resource_plot==static_cast<std::int64_t>(p.map_cursor)&&!p.resource_yield_returned_plots.empty()&&p.resource_yield_returned_plots.back()==*p.policy_resource_plot&&
  !p.resource_layout_stored&&((p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_pending&&!p.resource_layout_attempted)||
   (p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_executing&&p.resource_layout_attempted)),"BNW resource layout requires reached visible plot after actual yield return");
 const auto team=detail::bnw_player_team_read(state,static_cast<std::int32_t>(p.actor));
 require_stored_current(host);binding.lease.require_operation(binding.serial);
 const auto& raw=state.world.map_plots->plots.at(p.map_cursor);const auto& cell=state.world.map_projection->cells.at(p.map_cursor);
 const auto art=saves::observe_saved_plot_team_art(raw,saves::GameStateLayout::expansion2_version1,static_cast<std::uint64_t>(team));
 need(raw.index==p.map_cursor&&cell.index==p.map_cursor&&raw.x==cell.x&&raw.y==cell.y&&art.revealed==true&&cell.revealed_teams&&cell.revealed_teams->contains(team),"BNW resource layout lost actual paired isRevealed predicate");
 require_stored_current(host);binding.lease.require_operation(binding.serial);
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_resource_policy_layout_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_pending&&!p.resource_layout_attempted&&!p.resource_layout_stored&&
  p.resource_layout_returned_plots.size()<p.resource_layout_returned_plots.capacity(),"BNW resource layout dispatch repeated/unreached or history capacity exhausted");
 // Reserve once before any callbackful provider or retained Engine/UI borrow.
 p.resource_layout_attempted=true;p.phase=BnwPolicyAdoptionPhase::resource_policy_layout_executing;
 try{require_resource_policy_layout_source(host,state);return p.hooks->impl_->lease;}catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
std::int64_t BnwPolicyAdoptionPending::private_resource_policy_layout_plot()const{
 (void)phase();const auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_executing&&p.resource_layout_attempted&&!p.resource_layout_stored&&p.policy_resource_plot&&
  *p.policy_resource_plot==static_cast<std::int64_t>(p.map_cursor),"BNW resource layout receiving plot unissued");return *p.policy_resource_plot;
}
std::uint64_t BnwPolicyAdoptionPending::private_resource_policy_layout_operation_serial()const{
 (void)phase();const auto& p=*impl_;need(p.resource_layout_attempted&&p.policy_resource_plot&&p.hooks&&p.hooks->impl_,"BNW resource layout receipt lacks genuine reservation");return p.hooks->impl_->serial;
}
void BnwPolicyAdoptionPending::accept_actual_resource_policy_layout_return(const ui::BnwClientSelectionControl& selection)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_executing&&p.resource_layout_attempted&&!p.resource_layout_stored&&
  p.policy_resource_plot&&p.resource_layout_returned_plots.size()<p.resource_layout_returned_plots.capacity(),"BNW resource layout return unissued/repeated");
 try{
  // Pure physical receipt FIRST: retain earned history before late providers.
  // UI33 happened before the true transient assignment in this receiver.
  selection.require_resource_policy_layout_return(*this);
  const auto canonical=p.session->city_selection_.lock();need(canonical&&canonical.get()==&selection,"BNW resource layout foreign canonical UI return");
  p.resource_layout_returned_plots.push_back(*p.policy_resource_plot);p.resource_layout_selection=canonical;p.resource_layout_stored=true;
  const auto host=p.host.lock();need(bool(host),"BNW resource layout Host expired after native effects");require_stored_current(*host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
  selection.require_resource_policy_layout_return(*this);p.phase=BnwPolicyAdoptionPhase::resource_policy_layout_returned;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_resource_policy_layout_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(impl_)impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}
void BnwPolicyAdoptionPending::resume_after_resource_policy_layout()const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::resource_policy_layout_returned&&p.resource_layout_attempted&&p.resource_layout_stored&&p.policy_resource_plot&&
  !p.resource_layout_returned_plots.empty()&&p.resource_layout_returned_plots.back()==*p.policy_resource_plot,"BNW resource layout continuation repeated/unearned");
 p.phase=BnwPolicyAdoptionPhase::uncertain;
 const auto host=p.host.lock();need(bool(host),"BNW resource layout continuation Host expired");require_stored_current(*host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(resource_policy_layout_effects_current(),"BNW resource layout transient owner/effects changed before continuation");
 p.policy_resource_plot.reset();++p.map_cursor;p.resource_scanned=p.map_cursor;p.resource_reveal_attempted=false;
 p.resource_layout_attempted=false;p.resource_layout_stored=false;p.phase=BnwPolicyAdoptionPhase::resource_policy_yield_pending;
}

bool BnwPolicyAdoptionPending::finish_resource_policy_plot()const{
 (void)phase();auto& p=*impl_;
 need(p.phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending&&p.resource_loop_reached&&!p.resource_loop_returned&&!p.plot_child&&
  p.policy_resource_plot==static_cast<std::int64_t>(p.map_cursor)&&!p.resource_reveal_attempted&&
  p.resource_yield_returned_plots.size()<p.resource_yield_returned_plots.capacity(),"BNW resource PolicyReveal child return unreached/repeated");
 // Reserve before actor-team/currentness providers. The genuine updateYield
 // has returned; this records history, never a layout or render ACK.
 p.resource_reveal_attempted=true;p.resource_yield_returned_plots.push_back(static_cast<std::int64_t>(p.map_cursor));
 p.phase=BnwPolicyAdoptionPhase::uncertain;
 const auto host=p.host.lock();need(bool(host),"BNW resource PolicyReveal Host retired");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 auto& state=*p.state;const auto team=detail::bnw_player_team_read(state,static_cast<std::int32_t>(p.actor));
 require_stored_current(*host);binding.lease.require_operation(binding.serial);
 const auto& raw=state.world.map_plots->plots.at(static_cast<std::size_t>(p.map_cursor));
 const auto& cell=state.world.map_projection->cells.at(static_cast<std::size_t>(p.map_cursor));
 need(raw.index==p.map_cursor&&cell.index==p.map_cursor&&raw.x==cell.x&&raw.y==cell.y&&raw.version&&cell.revealed_teams,"BNW resource PolicyReveal actual visibility owners missing");
 const auto art=saves::observe_saved_plot_team_art(raw,saves::GameStateLayout::expansion2_version1,static_cast<std::uint64_t>(team));
 need(art.revealed.has_value()&&cell.revealed_teams->contains(team)==*art.revealed,"BNW resource PolicyReveal actual isRevealed pair differs");
 const bool revealed=*art.revealed;
 require_stored_current(*host);binding.lease.require_operation(binding.serial);
 if(revealed){p.resource_layout_attempted=false;p.resource_layout_stored=false;p.phase=BnwPolicyAdoptionPhase::resource_policy_layout_pending;return false;}
 // SDK false isRevealed really bypasses setLayoutDirty and continues the
 // map iteration, regardless of plot ownership or the recalculated cache.
 p.policy_resource_plot.reset();++p.map_cursor;p.resource_scanned=p.map_cursor;
 p.resource_reveal_attempted=false;p.phase=BnwPolicyAdoptionPhase::resource_policy_yield_pending;return true;
}

void BnwPolicyAdoptionPending::require_reformation_notification_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 need(&state==p.state.get()&&p.units_reward_cursor==3&&p.target==p.reformation_effect_policy&&p.reformation_notification_owner&&
  p.reformation_notification_transients&&
  ((p.phase==BnwPolicyAdoptionPhase::reformation_pending&&!p.reformation_notification_attempted)||
   (p.phase==BnwPolicyAdoptionPhase::reformation_executing&&p.reformation_notification_attempted)),
  "BNW reformation notification requires actual reached positive native guard");
 // The native positive isHuman read is reauthenticated from actual Pregame
 // source. The following callbackful bookends precede fresh raw borrowing.
 need(detail::bnw_player_human_read(state,static_cast<std::int32_t>(p.actor),host.database()),
  "BNW reformation actual human source changed before native Add");
 require_stored_current(host);binding.lease.require_operation(binding.serial);
 const auto physical=[&]{
  need(state.world.religion_culture.layout==saves::GameStateLayout::expansion2_version1&&state.cities.profile==content::TechnologyProfile::bnw&&state.cities.pantheon==0&&
   state.world.religion_culture.religions.size()==p.reformation_raw_religions.size()&&state.cities.religions.size()==p.reformation_owned_religions.size(),
   "BNW reformation actual guard religion owner retired");
  for(std::size_t i=0;i<p.reformation_raw_religions.size();++i){const auto& live=state.world.religion_culture.religions[i];const auto& old=p.reformation_raw_religions[i];
   need(live.religion==old.religion&&live.founder==old.founder&&live.pantheon==old.pantheon&&live.beliefs.belief_hashes==old.beliefs.belief_hashes,
    "BNW reformation actual founded/reformed belief source changed");
  }
  for(std::size_t i=0;i<p.reformation_owned_religions.size();++i){const auto& live=state.cities.religions[i];const auto& old=p.reformation_owned_religions[i];
   need(live.religion==old.religion&&live.founder==old.founder&&live.beliefs==old.beliefs,
    "BNW reformation owned religious source changed");
  }
  const auto& player=raw_player(state,p.actor);const auto& admin=player.administration;
  need(admin.layout==saves::GameStateLayout::expansion2_version1&&admin.blocks.size()==3&&name(admin.blocks.front().name)=="notifications"&&
   player.index==static_cast<std::uint64_t>(p.actor)&&player.saved_player_id==p.actor,
   "BNW reformation actual Notifications player/reset owner changed");
  need(reformation_notification_owner_same(admin.blocks.front(),*p.reformation_notification_owner)&&
   state.notification_load_transients.current==p.reformation_notification_transients->current&&
   state.notification_load_transients.external_update_pending==p.reformation_notification_transients->external_update_pending&&
   state.notification_load_transients.players==p.reformation_notification_transients->players,
   "BNW reformation pre-Add native ring/Read transients changed");
 };
 physical();require_stored_current(host);binding.lease.require_operation(binding.serial);physical();
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_reformation_notification_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::reformation_pending&&!p.reformation_notification_attempted,
  "BNW reformation notification unreached/repeated dispatch");
 // Reservation precedes all source/DB/VM/Engine providers. An attempted Add
 // cannot be retried after a provider failure or historical native mutation.
 p.reformation_notification_attempted=true;p.phase=BnwPolicyAdoptionPhase::reformation_executing;
 try{require_reformation_notification_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

std::uint64_t BnwPolicyAdoptionPending::private_reformation_operation_serial()const{
 (void)phase();const auto& p=*impl_;need(p.reformation_notification_attempted&&p.reformation_effect_policy==p.target&&p.hooks&&p.hooks->impl_,
  "BNW Reformation actual issued operation absent");
 need(p.hooks->impl_->thread==std::this_thread::get_id()&&p.hooks->impl_->serial!=0,"BNW Reformation issued operation foreign");return p.hooks->impl_->serial;
}
void BnwPolicyAdoptionPending::accept_actual_reformation_notification_return(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::reformation_executing&&p.reformation_notification_attempted&&!p.reformation_native_returned,
  "BNW Reformation native Add return unissued/repeated");
 try{
  // This is a PURE concrete native ring/transient/FIFO seal, with no current
  // provider at the old revision. Record earned history before late callbacks.
  engine.require_reformation_notification_return(*this);
  const auto before=p.after,after=p.state->ui_source_revision;need(after>=before&&after-before<=1,"BNW Reformation native Add revision differs");
  p.previous=before;p.after=after;p.reformation_inserted=after!=before;p.reformation_native_returned=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
  const auto host=p.host.lock();need(bool(host),"BNW Reformation native return Host expired");
  require_stored_current(*host);auto& binding=*p.hooks->impl_;if(after!=before)binding.accept(*this);
  binding.lease.require_operation(binding.serial);require_stored_current(*host);engine.require_reformation_notification_return(*this);
  p.reformation_returned=true;p.phase=BnwPolicyAdoptionPhase::reformation_returned;
  // SourceOwner resumes the once-only tail only after Engine/source bookends.
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::fail_reformation_notification_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

std::vector<BnwHappinessNativeStore> BnwPolicyAdoptionPending::happiness_stores()const{(void)phase();return impl_->happiness_stores;}
std::shared_ptr<const ui::SelectedBnwPreviewHost> BnwPolicyAdoptionPending::require_happiness_math_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,std::int64_t actor)const{
 (void)phase();const auto& p=*impl_;const auto actual=p.host.lock();need(actual&&actual.get()==&host,"BNW policy happiness actual Host retired/substituted");
 require_stored_current(*actual);
 need(&state==p.state.get()&&actor==p.actor&&p.phase==BnwPolicyAdoptionPhase::uncertain&&p.happiness_inflight&&p.happiness_stores.size()==2,
  "BNW policy local happiness math requires genuine reached third stage");
 p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);return actual;
}
void BnwPolicyAdoptionPending::advance_process_happiness_prefix()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_happiness_pending&&!p.happiness_inflight&&p.happiness_stores.empty(),"BNW policy happiness unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  const auto rules=BnwHappinessPrefixRules::load(host->database());
  need(rules.rules_sha256()==host->identity().rules_sha256,"BNW policy actual happiness rules differ");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,"m_iHappiness");
  need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1&&state.unhappiness.players.at(p.actor).happiness==field.values[0],"BNW policy initial happiness pair differs");
  p.happiness_stores.reserve(7);p.happiness_inflight=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
  // SDK CvPlayer::DoUpdateHappiness 10208: no NO_HAPPINESS shortcut here.
  // Each getter reads the genuine intermediate cache; no detached world.
  for(const auto stage:{BnwHappinessNativeStore::Stage::handicap,BnwHappinessNativeStore::Stage::resources,
   BnwHappinessNativeStore::Stage::cities,BnwHappinessNativeStore::Stage::buildings,
   BnwHappinessNativeStore::Stage::policies,BnwHappinessNativeStore::Stage::per_city}){
   BnwHappinessNativeStore item;
   if(stage==BnwHappinessNativeStore::Stage::cities){
    const auto canonical=p.self.lock();need(canonical&&canonical.get()==this,"BNW policy third happiness stage lost canonical factory owner");
    const auto operand=detail::bnw_happiness_buildings_local_math_for_policy(state,rules.buildings(),host->identity(),p.actor,*host,canonical);
    need(p.happiness_stores.size()==2&&state.unhappiness.players.at(p.actor).happiness==scalar(raw.core,"m_iHappiness"),"BNW policy reached local happiness source pair differs");
    item={stage,operand,static_cast<std::int32_t>(native_sum(scalar(raw.core,"m_iHappiness"),operand))};
   }else{item=detail::bnw_happiness_prefix_reached_stage_math(state,rules,host->identity(),p.actor,stage);}
   auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy happiness cache absent");
   auto desired=p.expected;desired.process_scalars.at("m_iHappiness")=item.happiness;
   auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const bool changed=field.values[0]!=item.happiness||state.cities.players.at(p.actor).excess_happiness.has_value();
   const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy happiness revision exhausted");const auto after=before+(changed?1:0);
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   *registry=std::move(next_registry);field.values[0]=item.happiness;state.unhappiness.players.at(p.actor).happiness=item.happiness;
   state.cities.players.at(p.actor).excess_happiness.reset();p.expected=std::move(expected_copy);p.happiness_stores.push_back(item);
   p.previous=before;p.after=after;state.ui_source_revision=after;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  }
  p.happiness_inflight=false;p.phase=BnwPolicyAdoptionPhase::policy_happiness_founder_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
std::int64_t BnwPolicyAdoptionPending::begin_happiness_founder_dispatch()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(bool(host),"BNW policy founder Host retired");
 require_stored_current(*host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(p.happiness_stores.size()==6&&!p.happiness_founder_attempt&&
  ((p.phase==BnwPolicyAdoptionPhase::policy_happiness_founder_pending&&p.happiness_founder_calls==0)||
   (p.phase==BnwPolicyAdoptionPhase::policy_happiness_foreign_founder_pending&&p.happiness_founder_calls==1&&p.happiness_religion)),
  "BNW policy founder actual native interval unreached/repeated");
 p.happiness_foreign_attempt=p.phase==BnwPolicyAdoptionPhase::policy_happiness_foreign_founder_pending;
 p.happiness_founder_attempt=true;p.phase=BnwPolicyAdoptionPhase::uncertain;return p.actor;
}
void BnwPolicyAdoptionPending::finish_happiness_founder_dispatch(std::optional<std::int64_t> actual_accumulator)const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(bool(host),"BNW policy founder Host retired");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  need(p.phase==BnwPolicyAdoptionPhase::uncertain&&p.happiness_founder_attempt&&p.happiness_stores.size()==6,"BNW policy founder lacks actual SAME-VM reserved attempt");
  auto& state=*p.state;const auto model=BnwFounderReligionReadModel::bind(state);std::int32_t religion=-1;
  if(actual_accumulator){need(*actual_accumulator>=INT32_MIN&&*actual_accumulator<=INT32_MAX,"BNW policy founder accumulator outside native int");religion=static_cast<std::int32_t>(*actual_accumulator);}
  else{const auto created=model.religion_created_by_player(p.actor);if(model.eligible_for_founder_benefits(created,p.actor))religion=created;}
  ++p.happiness_founder_calls;p.happiness_founder_attempt=false;std::int32_t amount=0;
  if(p.happiness_foreign_attempt){
   auto& terms=*p.happiness_religion;terms.foreign_founder_religion=religion;terms.founder_callback_calls=2;
   if(religion>0){terms.peaceful_foreign_followers=BnwReligionFollowersReadModel::bind(state).peaceful_foreign_followers(p.actor,religion);}
   terms.foreign_happiness=static_cast<std::int32_t>(static_cast<std::int64_t>(terms.peaceful_foreign_followers)/terms.peaceful_foreign_divisor);
   terms.total=static_cast<std::int32_t>(native_sum(terms.total,terms.foreign_happiness));amount=terms.total;
  }else{
   const auto* entry=religion==-1?nullptr:model.religion(religion,-1);
   if(entry){
    const auto rules=BnwReligionHappinessRules::load(host->database());
    need(rules.rules_sha256()==host->identity().rules_sha256&&state.cities.religions.size()==state.world.religion_culture.religions.size(),"BNW policy actual religion rules/registry differ");
    for(std::size_t i=0;i<state.cities.religions.size();++i){
     const auto& raw=state.world.religion_culture.religions[i];const auto& owned=state.cities.religions[i];std::vector<std::int64_t> expected;
     for(const auto id:rules.resolved_beliefs(raw)){if(id>=0&&static_cast<std::uint64_t>(id)<rules.belief_slots())expected.push_back(id);}
     need(raw.religion==owned.religion&&raw.founder==owned.founder&&owned.beliefs==expected,"BNW policy founded belief mirror differs");
    }
    const auto cities=BnwReligionFollowersReadModel::bind(state);BnwReligionHappinessTerms terms;
    terms.at_peace=cities.at_peace(p.actor);const auto beliefs=rules.terms(*entry,terms.at_peace);
    terms.player_happiness=beliefs.player_happiness;terms.per_following_city=beliefs.per_following_city;
    terms.peaceful_foreign_divisor=beliefs.peaceful_foreign_divisor;terms.following_cities=cities.cities_following(religion);
    const float following=static_cast<float>(static_cast<float>(terms.following_cities)*terms.per_following_city);
    need(std::isfinite(following)&&static_cast<double>(following)>=INT32_MIN&&static_cast<double>(following)<=INT32_MAX,"BNW policy religion native float conversion overflow");
    terms.following_happiness=static_cast<std::int32_t>(following);
    terms.total=static_cast<std::int32_t>(native_sum(terms.player_happiness,terms.following_happiness));p.happiness_religion=terms;
    if(terms.peaceful_foreign_divisor>0){require_stored_current(*host);binding.lease.require_operation(binding.serial);p.phase=BnwPolicyAdoptionPhase::policy_happiness_foreign_founder_pending;return;}
    amount=terms.total;
   }
  }
  auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,"m_iHappiness");
  const auto total=native_sum(field.values.at(0),amount);auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy religion happiness cache absent");
  auto desired=p.expected;desired.process_scalars.at("m_iHappiness")=total;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  const bool changed=field.values[0]!=total||state.cities.players.at(p.actor).excess_happiness.has_value();const auto before=p.after;
  need(!changed||before<UINT64_MAX,"BNW policy religion happiness revision exhausted");const auto after=before+(changed?1:0);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  *registry=std::move(next_registry);field.values[0]=total;state.unhappiness.players.at(p.actor).happiness=total;
  state.cities.players.at(p.actor).excess_happiness.reset();p.expected=std::move(expected_copy);
  p.happiness_stores.push_back({BnwHappinessNativeStore::Stage::religion,amount,static_cast<std::int32_t>(total)});
  p.previous=before;p.after=after;state.ui_source_revision=after;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.phase=BnwPolicyAdoptionPhase::policy_happiness_suffix_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

std::vector<BnwHappinessSuffixStore> BnwPolicyAdoptionPending::happiness_suffix_stores()const{(void)phase();return impl_->happiness_suffix_stores;}
bool BnwPolicyAdoptionPending::happiness_achievement_requested()const{(void)phase();return impl_->happiness_achievement;}
bool BnwPolicyAdoptionPending::happiness_native_suffix_returned()const{(void)phase();return impl_->happiness_native_suffix_returned;}
bool BnwPolicyAdoptionPending::happiness_dirty_returned()const{(void)phase();return impl_->happiness_dirty_published;}
bool BnwPolicyAdoptionPending::happiness_achievement_backend_disabled()const{(void)phase();return impl_->happiness_achievement_backend_disabled;}
void BnwPolicyAdoptionPending::advance_process_happiness_suffix()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_happiness_suffix_pending&&p.happiness_stores.size()==7&&p.happiness_suffix_stores.empty(),"BNW policy happiness suffix unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  const auto rules=BnwHappinessSuffixRules::load(host->database());need(rules.identity()==host->identity().rules_sha256,"BNW policy actual happiness suffix rules differ");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);p.happiness_suffix_stores.reserve(6);p.phase=BnwPolicyAdoptionPhase::uncertain;
  using Read=detail::BnwHappinessPostReligionRead;
  const auto read=[&](Read stage){return detail::bnw_happiness_post_religion_reached_read(state,rules,host->identity(),p.actor,stage);};
  const auto store=[&](BnwHappinessSuffixStage stage,std::int32_t operand,bool connection){
   const auto* name=connection?"m_iCityConnectionHappiness":"m_iHappiness";auto& field=named(raw.core.fields,name);
   const auto old=scalar(raw.core,name),value=connection?static_cast<std::int64_t>(operand):native_sum(old,operand);
   need(old==p.expected.process_scalars.at(name),"BNW policy happiness suffix raw source differs");
   auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy happiness suffix cache absent");
   auto desired=p.expected;desired.process_scalars.at(name)=value;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const bool changed=old!=value||(!connection&&state.cities.players.at(p.actor).excess_happiness.has_value());const auto before=p.after;
   need(!changed||before<UINT64_MAX,"BNW policy happiness suffix revision exhausted");const auto after=before+(changed?1:0);
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   *registry=std::move(next_registry);field.values[0]=value;p.expected=std::move(expected_copy);
   if(!connection){state.unhappiness.players.at(p.actor).happiness=value;state.cities.players.at(p.actor).excess_happiness.reset();p.happiness_expected_excess.reset();}
   p.happiness_suffix_stores.push_back({stage,operand,static_cast<std::int32_t>(p.expected.process_scalars.at("m_iHappiness"))});
   p.previous=before;p.after=after;state.ui_source_revision=after;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  };
  store(BnwHappinessSuffixStage::wonders,read(Read::wonders).operand,false);
  store(BnwHappinessSuffixStage::minors,read(Read::minors).operand,false);
  // SDK GetHappinessFromLeagues is THIS native cache getter, not recomputation.
  store(BnwHappinessSuffixStage::leagues,read(Read::leagues).operand,false);
  const auto connections=read(Read::connections_cache);need(connections.connections.has_value(),"BNW policy actual connection quote absent");
  store(BnwHappinessSuffixStage::connections_cache,connections.operand,true);
  store(BnwHappinessSuffixStage::connections_add,read(Read::connections_add).operand,false);
  // isLocalPlayer short-circuits before GetExcessHappiness for nonlocal actors.
  const auto achievement=read(Read::achievement_condition);require_stored_current(*host);binding.lease.require_operation(binding.serial);
  const auto excess=achievement.unhappiness?std::optional<std::int64_t>(achievement.unhappiness->excess_happiness):std::nullopt;
  // SDK local achievement is reached BEFORE unconditional GameData11.
  // Standalone Civis explicitly disables the optional Steam backend. This
  // records the actual condition, without a fictitious unlock or Lua request.
  p.happiness_achievement=achievement.achievement_requested;
  const auto services=state.standalone_services;
  if(p.happiness_achievement){
   saves::require_supported_standalone_services(services);
   need(services.steam_achievements==saves::StandaloneServiceBackend::backend_disabled,"BNW happiness achievement backend has no genuine implementation");
  }
  const auto before=p.after;const auto prior_excess=state.cities.players.at(p.actor).excess_happiness;
  const bool changed=prior_excess!=excess;need(!changed||before<UINT64_MAX,"BNW happiness derived cache revision exhausted");const auto after=before+(changed?1:0);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  need(state.ui_source_revision==before&&state.cities.players.at(p.actor).excess_happiness==prior_excess&&
   (!p.happiness_achievement||state.standalone_services==services),"BNW happiness actual cache/backend changed before native suffix return");
  state.cities.players.at(p.actor).excess_happiness=excess;p.happiness_expected_excess=excess;
  if(excess){p.happiness_suffix_stores.push_back({BnwHappinessSuffixStage::excess,static_cast<std::int32_t>(*excess),static_cast<std::int32_t>(p.expected.process_scalars.at("m_iHappiness"))});}
  p.happiness_achievement_backend_disabled=p.happiness_achievement;
  p.happiness_native_suffix_returned=true;p.previous=before;p.after=after;state.ui_source_revision=after;
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  // No generic transport queue can satisfy native setDirty(GameData,true).
  // Remain here until the SAME canonical concrete UI receiver really stores.
  p.phase=BnwPolicyAdoptionPhase::policy_happiness_dirty_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::require_happiness_dirty_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 need(&state==p.state.get()&&p.happiness_stores.size()==7&&p.happiness_native_suffix_returned&&!p.happiness_dirty_published&&
  (p.happiness_suffix_stores.size()==5||p.happiness_suffix_stores.size()==6)&&(!p.happiness_achievement||p.happiness_achievement_backend_disabled)&&
  ((p.phase==BnwPolicyAdoptionPhase::policy_happiness_dirty_pending&&!p.happiness_dirty_attempted)||
   (p.phase==BnwPolicyAdoptionPhase::policy_happiness_dirty_executing&&p.happiness_dirty_attempted)),"BNW happiness GameData requires genuine completed native suffix");
 need(state.unhappiness.players.at(p.actor).happiness==p.expected.process_scalars.at("m_iHappiness")&&state.cities.players.at(p.actor).excess_happiness==p.happiness_expected_excess,
  "BNW happiness GameData native paired/cache operands changed");
 if(p.happiness_achievement){saves::require_supported_standalone_services(state.standalone_services);need(state.standalone_services.steam_achievements==saves::StandaloneServiceBackend::backend_disabled,"BNW happiness optional achievement backend changed");}
 require_stored_current(host);binding.lease.require_operation(binding.serial);
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_happiness_dirty_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_happiness_dirty_pending&&!p.happiness_dirty_attempted&&!p.happiness_dirty_published,"BNW happiness GameData dispatch unreached/repeated");
 p.happiness_dirty_attempted=true;p.phase=BnwPolicyAdoptionPhase::policy_happiness_dirty_executing;
 try{require_happiness_dirty_source(host,state);return p.hooks->impl_->lease;}catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
std::uint64_t BnwPolicyAdoptionPending::private_happiness_dirty_operation_serial()const{
 (void)phase();const auto& p=*impl_;need(p.happiness_dirty_attempted&&p.happiness_native_suffix_returned&&p.hooks&&p.hooks->impl_,"BNW happiness GameData receipt unissued");return p.hooks->impl_->serial;
}
void BnwPolicyAdoptionPending::accept_actual_happiness_dirty_return(const ui::BnwClientSelectionControl& selection)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_happiness_dirty_executing&&p.happiness_dirty_attempted&&!p.happiness_dirty_published,"BNW happiness GameData return repeated/unissued");
 try{
  // Pure SAME-Pending physical setter seal earns history before late callbacks.
  selection.require_happiness_dirty_return(*this);p.happiness_dirty_published=true;
  const auto host=p.host.lock();need(bool(host),"BNW happiness Host expired after GameData store");require_stored_current(*host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
  selection.require_happiness_dirty_return(*this);p.phase=BnwPolicyAdoptionPhase::policy_happiness_dirty_returned;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_happiness_dirty_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(impl_)impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}
void BnwPolicyAdoptionPending::resume_after_happiness_dirty()const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_happiness_dirty_returned&&p.happiness_dirty_attempted&&p.happiness_dirty_published&&p.happiness_native_suffix_returned,"BNW happiness trade continuation repeated/unearned");
 p.phase=BnwPolicyAdoptionPhase::uncertain;
 const auto host=p.host.lock();need(bool(host),"BNW happiness continuation Host expired");require_stored_current(*host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 p.phase=BnwPolicyAdoptionPhase::policy_trade_pending;
}

std::uint8_t BnwPolicyAdoptionPending::great_people_reset_cursor()const{(void)phase();return impl_->great_people_reset_cursor;}
void BnwPolicyAdoptionPending::advance_process_trade_and_great_people_prefix()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_trade_pending&&p.happiness_dirty_published&&p.great_people_reset_cursor==0,"BNW policy trade/GP prefix unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  auto& state=*p.state;const auto& trade=state.world.trade;
  need(trade.layout==saves::GameStateLayout::expansion2_version1&&trade.version&&*trade.version<=3&&trade.dimensions.major_slots==22&&trade.dimensions.yield_slots==6,"BNW policy actual complete trade registry unavailable");
  const auto source=bnw_player_trade_registry_revision(state,p.actor);
  // Original UpdateTradeConnectionValues: empty routes skip BOTH owner
  // branches. A nonempty relevant route must really recompute six yields.
  for(const auto& route:trade.routes){
   const bool empty=route.origin_x==-1&&route.origin_y==-1&&route.destination_x==-1&&route.destination_y==-1;
   if(empty){continue;}
   need(route.origin_owner>=0&&route.origin_owner<22&&route.destination_owner>=0&&route.destination_owner<22&&route.origin_yields.size()==6&&route.destination_yields.size()==6,"BNW policy actual nonempty trade endpoint/array invalid");
   if(route.origin_owner==p.actor||route.destination_owner==p.actor){require_stored_current(*host);binding.lease.require_operation(binding.serial);return;}
  }
  require_stored_current(*host);binding.lease.require_operation(binding.serial);need(source==bnw_player_trade_registry_revision(state,p.actor),"BNW policy trade source changed during full scan");
  p.trade_source=source;p.phase=BnwPolicyAdoptionPhase::uncertain;
  // SDK recomputeGreatPeopleModifiers first performs ALL ten zero stores,
  // before Traits/policies/buildings/friendships/leagues/define additions.
  for(const auto* member:{"m_iGreatPeopleRateModifier","m_iGreatGeneralRateModifier","m_iGreatAdmiralRateModifier","m_iGreatWriterRateModifier","m_iGreatArtistRateModifier","m_iGreatMusicianRateModifier","m_iGreatMerchantRateModifier","m_iGreatScientistRateModifier","m_iGreatEngineerRateModifier","m_iDomesticGreatGeneralRateModifier"}){
   auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,member);const auto old=scalar(raw.core,member);
   need(old==p.expected.process_scalars.at(member),"BNW policy GP initial raw source differs");auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy GP native cache absent");
   auto desired=p.expected;desired.process_scalars.at(member)=0;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const bool changed=old!=0;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy GP reset revision exhausted");const auto after=before+(changed?1:0);
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   *registry=std::move(next_registry);field.values[0]=0;p.expected=std::move(expected_copy);++p.great_people_reset_cursor;
   p.previous=before;p.after=after;state.ui_source_revision=after;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  }
  p.phase=BnwPolicyAdoptionPhase::policy_great_people_traits_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

std::uint8_t BnwPolicyAdoptionPending::great_people_traits_cursor()const{(void)phase();return impl_->great_people_traits_cursor;}
std::array<std::int32_t,3> BnwPolicyAdoptionPending::great_people_traits_operands()const{(void)phase();return impl_->great_people_traits_operands;}
void BnwPolicyAdoptionPending::advance_process_great_people_traits()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_great_people_traits_pending&&p.great_people_reset_cursor==10&&p.great_people_traits_cursor==0,"BNW policy GP traits unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  auto& state=*p.state;
  for(const auto* member:{"m_iGreatPeopleRateModifier","m_iGreatGeneralRateModifier","m_iGreatScientistRateModifier"}){
   // Issue the genuine current native cache owner separately for each SDK
   // getter: a sibling store may advance the source revision in between.
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   const auto traits=BnwMajorPlayerTraitsOwner::from_source_host(host,static_cast<std::int32_t>(p.actor));
   const auto operand=traits->cache_value(member);traits->require_current();
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,member);const auto old=scalar(raw.core,member),value=native_sum(old,operand);
   need(old==p.expected.process_scalars.at(member),"BNW policy GP trait source/raw pair differs");
   auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy GP trait native storage absent");
   auto desired=p.expected;desired.process_scalars.at(member)=value;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const bool changed=old!=value;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy GP trait revision exhausted");const auto after=before+(changed?1:0);
   traits->require_current();require_stored_current(*host);binding.lease.require_operation(binding.serial);
   // Actual sequential m_iRate += Traits getter, before policy modifiers.
   p.phase=BnwPolicyAdoptionPhase::uncertain;*registry=std::move(next_registry);field.values[0]=value;p.expected=std::move(expected_copy);
   p.great_people_traits_operands.at(p.great_people_traits_cursor)=operand;++p.great_people_traits_cursor;
   p.previous=before;p.after=after;state.ui_source_revision=after;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  }
  p.phase=BnwPolicyAdoptionPhase::policy_great_people_policies_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

std::uint8_t BnwPolicyAdoptionPending::great_people_modifiers_cursor()const{(void)phase();return impl_->great_people_modifiers_cursor;}
std::array<std::int32_t,12> BnwPolicyAdoptionPending::great_people_modifiers_operands()const{(void)phase();return impl_->great_people_modifiers_operands;}
void BnwPolicyAdoptionPending::advance_process_great_people_policy_buildings()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_great_people_policies_pending&&p.great_people_traits_cursor==3&&p.great_people_modifiers_cursor==0,"BNW policy GP policy/building modifiers unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  auto& state=*p.state;
  const auto policies=BnwPolicyHappinessRules::load(host->database());need(policies.rules_sha256()==host->identity().rules_sha256&&policies.policies().size()==p.rules.policies.size(),"BNW GP actual policy blocked-branch mapping differs");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  const auto store=[&](const char* member,std::int64_t operand){
   auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,member);const auto old=scalar(raw.core,member),value=native_sum(old,operand);
   need(old==p.expected.process_scalars.at(member)&&operand>=INT32_MIN&&operand<=INT32_MAX,"BNW GP current modifier/raw native pair differs");
   auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW GP current modifier native storage absent");
   auto desired=p.expected;desired.process_scalars.at(member)=value;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const bool changed=old!=value;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW GP current modifier revision exhausted");const auto after=before+(changed?1:0);
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   p.phase=BnwPolicyAdoptionPhase::uncertain;*registry=std::move(next_registry);field.values[0]=value;p.expected=std::move(expected_copy);
   p.great_people_modifiers_operands.at(p.great_people_modifiers_cursor)=static_cast<std::int32_t>(operand);++p.great_people_modifiers_cursor;
   p.previous=before;p.after=after;state.ui_source_revision=after;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  };
  // Nine SDK GetNumericModifier calls, each folds current HasPolicy and
  // native Reset's blocked-branch mapping in ascending native Info order.
  // No policy cost, finished-branch or historical-membership substitute.
  const std::array<std::pair<const char*,const char*>,9> numeric={{{"m_iGreatPeopleRateModifier","GreatPeopleRateModifier"},{"m_iGreatGeneralRateModifier","GreatGeneralRateModifier"},{"m_iGreatAdmiralRateModifier","GreatAdmiralRateModifier"},{"m_iGreatWriterRateModifier","GreatWriterRateModifier"},{"m_iGreatArtistRateModifier","GreatArtistRateModifier"},{"m_iGreatMusicianRateModifier","GreatMusicianRateModifier"},{"m_iGreatMerchantRateModifier","GreatMerchantRateModifier"},{"m_iGreatScientistRateModifier","GreatScientistRateModifier"},{"m_iDomesticGreatGeneralRateModifier","DomesticGreatGeneralRateModifier"}}};
  for(const auto& [member,column]:numeric){
   std::int64_t total=0;
   for(const auto& [id,rule]:p.rules.policies){
    if(!p.expected.has.at(id))continue;
    const auto branch=policies.policies().at(id).blocked_branch;if(branch>=0&&p.expected.blocked.at(branch))continue;
    std::optional<std::int64_t> value;
    const auto report=host->database().query_rows({std::string("SELECT ")+column+" FROM Policies WHERE Type = ?",{rule.type}},[&](const content::RulesRowView& row){need(!value,"BNW GP duplicate actual current policy Info");value=row.integer_at(0);});
    need(report.all_succeeded()&&report.rows==1&&value&&*value>=INT32_MIN&&*value<=INT32_MAX,"BNW GP current policy numeric Info unavailable");
    require_stored_current(*host);binding.lease.require_operation(binding.serial);total=native_sum(total,*value);
   }
   store(member,total);
  }
  // Three actual player native building-cache getters; these caches were
  // already maintained by processBuilding. Do not reconstruct them from a
  // guessed city inventory or imported current GP total.
  for(const auto& [member,cache]:std::array<std::pair<const char*,const char*>,3>{{{"m_iGreatPeopleRateModifier","m_iGreatPeopleRateModFromBldgs"},{"m_iGreatGeneralRateModifier","m_iGreatGeneralRateModFromBldgs"},{"m_iDomesticGreatGeneralRateModifier","m_iDomesticGreatGeneralRateModFromBldgs"}}}){
   require_stored_current(*host);binding.lease.require_operation(binding.serial);const auto value=scalar(raw_player(state,p.actor).core,cache);store(member,value);
  }
  p.phase=BnwPolicyAdoptionPhase::policy_great_people_friendships_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

bool BnwPolicyAdoptionPending::great_people_recompute_returned()const{(void)phase();return impl_->great_people_recompute_returned;}
std::array<std::int32_t,4> BnwPolicyAdoptionPending::great_people_suffix_operands()const{(void)phase();return impl_->great_people_suffix_operands;}
void BnwPolicyAdoptionPending::advance_process_great_people_suffix()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_great_people_friendships_pending&&p.great_people_modifiers_cursor==12&&!p.great_people_recompute_returned,"BNW policy GP suffix unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  auto& state=*p.state;
  const auto trait=[&](std::int32_t actor){require_stored_current(*host);binding.lease.require_operation(binding.serial);const auto native=BnwMajorPlayerTraitsOwner::from_source_host(host,actor);const auto result=native->cache_value("m_iDOFGreatPersonModifier");native->require_current();require_stored_current(*host);binding.lease.require_operation(binding.serial);return result;};
  const auto valid=[&](std::int32_t other){
   // Native IsPlayerValid short-circuits alive, then city count, then met,
   // then same-team. In particular cities==0 never reads a foreign Trait.
   const auto& raw=raw_player(state,other);const auto& alive=named(raw.core.fields,"m_bAlive");
   need(alive.encoding==saves::PlayerCoreEncoding::boolean&&alive.values.size()==1&&(alive.values[0]==0||alive.values[0]==1)&&state.setup.players.at(other).alive==(alive.values[0]!=0),"BNW GP friendship native alive pair unavailable");if(!alive.values[0])return false;
   need(raw.cities.header.wire_city_count>=0&&static_cast<std::uint64_t>(raw.cities.header.wire_city_count)==raw.cities.cities.size()&&raw.cities.cities.size()==state.cities.players.at(other).cities.size(),"BNW GP friendship actual live city count differs");if(raw.cities.cities.empty())return false;
   const auto mine=detail::bnw_player_team_read(state,static_cast<std::int32_t>(p.actor)),theirs=detail::bnw_player_team_read(state,other);
   need(state.world.teams&&state.world.teams->layout==saves::GameStateLayout::expansion2_version1&&state.world.teams->teams.size()==64,"BNW GP friendship actual teams unavailable");
   const auto& team=state.world.teams->teams.at(static_cast<std::size_t>(mine));need(team.index==static_cast<std::uint64_t>(mine)&&named(team.fields,"id").value==mine,"BNW GP friendship native team ID differs");
   const auto& met=named(team.arrays,"has_met");need(met.columns==1&&met.values.size()==64,"BNW GP friendship native met array differs");const auto flag=met.values.at(static_cast<std::size_t>(theirs));need((flag==0||flag==1)&&state.player_research.teams.at(mine).met.contains(theirs)==(flag!=0),"BNW GP friendship native/owned met pair differs");if(!flag)return false;
   return mine!=theirs;
  };
  const auto accepted=[&](std::int32_t other){const auto& diplo=raw_player(state,p.actor).diplomacy;need(diplo.layout==saves::GameStateLayout::expansion2_version1&&diplo.dimensions.major==22,"BNW GP friendship actual diplomacy owner unavailable");const auto& field=named(diplo.fields,"m_pabDoFAccepted");need(field.encoding==saves::DiplomacyEncoding::boolean&&field.rows==1&&field.columns==22&&field.values.size()==22,"BNW GP friendship actual DoF source differs");const auto value=field.values.at(static_cast<std::size_t>(other));need(value==0||value==1,"BNW GP friendship native DoF bool invalid");return value!=0;};
  const auto store=[&](const char* member,std::int64_t operand){
   auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,member);const auto old=scalar(raw.core,member),value=native_sum(old,operand);need(old==p.expected.process_scalars.at(member),"BNW GP suffix native/raw pair differs");
   auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW GP suffix native storage absent");auto desired=p.expected;desired.process_scalars.at(member)=value;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const bool changed=old!=value;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW GP suffix revision exhausted");const auto after=before+(changed?1:0);require_stored_current(*host);binding.lease.require_operation(binding.serial);
   p.phase=BnwPolicyAdoptionPhase::uncertain;*registry=std::move(next_registry);field.values[0]=value;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;
   require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  };
  const auto own_trait=trait(static_cast<std::int32_t>(p.actor));std::int64_t friendships=0;
  if(own_trait>0){std::int64_t count=0;for(std::int32_t other=0;other<22;++other)if(valid(other)&&accepted(other))count=native_sum(count,1);friendships=native_product(count,own_trait);}
  else{for(std::int32_t other=0;other<22;++other)if(valid(other)&&accepted(other)){const auto foreign=trait(other);if(foreign>0)friendships=native_sum(friendships,foreign);}}
  require_stored_current(*host);binding.lease.require_operation(binding.serial);store("m_iGreatPeopleRateModifier",friendships);p.great_people_suffix_operands[0]=static_cast<std::int32_t>(friendships);
  const auto league_rate=[&](std::string_view member){
   const auto& leagues=state.world.leagues;need(leagues.layout==saves::GameStateLayout::expansion2_version1&&leagues.version.has_value(),"BNW GP actual active leagues source absent");std::int64_t sum=0;
   for(const auto& league:leagues.leagues){const bool member_found=std::any_of(league.members.begin(),league.members.end(),[&](const auto& value){return value.player==p.actor;});if(!member_found)continue;
    std::int64_t modifier=0;for(const auto& resolution:league.active_resolutions){const auto value=named(resolution.effects.integers,member).value;need(value>=INT32_MIN&&value<=INT32_MAX,"BNW GP actual league effect native width invalid");if(value)modifier=native_sum(modifier,value);}if(modifier)sum=native_sum(sum,modifier);
   }
   require_stored_current(*host);binding.lease.require_operation(binding.serial);return sum;
  };
  // SDK reads both league modifiers before applying either set of additions.
  const auto artsy=league_rate("artsy_great_person_rate_mod"),sciencey=league_rate("sciencey_great_person_rate_mod");
  p.great_people_suffix_operands[1]=static_cast<std::int32_t>(artsy);p.great_people_suffix_operands[2]=static_cast<std::int32_t>(sciencey);
  if(artsy)for(const auto* member:{"m_iGreatWriterRateModifier","m_iGreatArtistRateModifier","m_iGreatMusicianRateModifier"})store(member,artsy);
  if(sciencey)for(const auto* member:{"m_iGreatScientistRateModifier","m_iGreatEngineerRateModifier","m_iGreatMerchantRateModifier"})store(member,sciencey);
  const auto border=policy_define(*host,"COMBAT_EXPERIENCE_IN_BORDERS_PERCENT");require_stored_current(*host);binding.lease.require_operation(binding.serial);store("m_iDomesticGreatGeneralRateModifier",border);p.great_people_suffix_operands[3]=static_cast<std::int32_t>(border);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);p.great_people_recompute_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_post_gp_cost_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

void BnwPolicyAdoptionPending::require_policy_cost_observer_source(const ui::SelectedBnwPreviewHost& host,
 const LiveSessionState& state,std::int64_t actor,std::int64_t before_cost,std::int64_t after_cost,
 std::uint64_t before_revision,std::uint64_t after_revision,bool stored)const{
 (void)phase();const auto& p=*impl_;const auto actual=p.host.lock();need(actual&&actual.get()==&host,"BNW policy cost observer actual Host retired/substituted");
 require_stored_current(*actual);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&actor==p.actor&&p.phase==BnwPolicyAdoptionPhase::uncertain&&p.cost_observer_inflight&&p.cost_observer_stored==stored&&
  p.cost_observer_before==before_cost&&p.cost_observer_after==after_cost&&p.cost_observer_pre_revision==before_revision&&p.cost_observer_post_revision==after_revision&&
  before_cost>=INT32_MIN&&before_cost<=INT32_MAX&&after_cost>=INT32_MIN&&after_cost<=INT32_MAX&&
  (before_cost==after_cost||before_revision<UINT64_MAX)&&after_revision==before_revision+(before_cost!=after_cost?1:0)&&p.after==(stored?after_revision:before_revision)&&
  scalar(raw_player(state,actor).core,"m_iPolicyCostModifier")==p.expected.process_scalars.at("m_iPolicyCostModifier")&&
  p.expected.process_scalars.at("m_iPolicyCostModifier")== (stored?after_cost:before_cost),
  "BNW policy cost observer requires exact earned paired store interval");
}

std::uint8_t BnwPolicyAdoptionPending::post_great_people_scalar_cursor()const{(void)phase();return impl_->post_gp_scalar_cursor;}
std::array<std::int32_t,4> BnwPolicyAdoptionPending::post_great_people_scalar_operands()const{(void)phase();return impl_->post_gp_scalar_operands;}
void BnwPolicyAdoptionPending::advance_process_policy_cost_and_experience()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_post_gp_cost_pending&&p.great_people_recompute_returned&&p.post_gp_scalar_cursor==0&&!p.cost_observer_inflight,"BNW post-GP cost/experience unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  auto& state=*p.state;const auto policies=BnwPolicyHappinessRules::load(host->database());need(policies.rules_sha256()==host->identity().rules_sha256&&policies.policies().size()==p.rules.policies.size(),"BNW post-GP actual policy mapping differs");
  const auto numeric=[&](const char* column){std::int64_t total=0;
   for(const auto& [id,rule]:p.rules.policies){if(!p.expected.has.at(id))continue;const auto branch=policies.policies().at(id).blocked_branch;if(branch>=0&&p.expected.blocked.at(branch))continue;
    std::optional<std::int64_t> value;const auto report=host->database().query_rows({std::string("SELECT ")+column+" FROM Policies WHERE Type = ?",{rule.type}},[&](const content::RulesRowView& row){need(!value,"BNW post-GP duplicate active policy Info");value=row.integer_at(0);});need(report.all_succeeded()&&report.rows==1&&value&&*value>=INT32_MIN&&*value<=INT32_MAX,"BNW post-GP actual policy numeric modifier unavailable");
    require_stored_current(*host);binding.lease.require_operation(binding.serial);total=native_sum(total,*value);
   }return total;
  };
  auto cost=numeric("PolicyCostModifier");cost=native_sum(cost,scalar(raw_player(state,p.actor).core,"m_iPolicyCostBuildingModifier"));cost=native_sum(cost,scalar(raw_player(state,p.actor).core,"m_iPolicyCostMinorCivModifier"));
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  const auto traits=BnwMajorPlayerTraitsOwner::from_source_host(host,static_cast<std::int32_t>(p.actor));const auto trait_cost=traits->cache_value("m_iPolicyCostModifier");traits->require_current();cost=native_sum(cost,trait_cost);
  const auto floor=policy_define(*host,"POLICY_COST_DISCOUNT_MAX");if(cost<floor)cost=floor;require_stored_current(*host);binding.lease.require_operation(binding.serial);
  auto& raw=raw_player(state,p.actor);auto& field=named(raw.core.fields,"m_iPolicyCostModifier");const auto old=scalar(raw.core,"m_iPolicyCostModifier");need(old==p.expected.process_scalars.at("m_iPolicyCostModifier"),"BNW policy cost raw/owned source differs");
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy cost native registry missing");auto desired=p.expected;desired.process_scalars.at("m_iPolicyCostModifier")=cost;auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
  const bool changed=old!=cost;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW policy cost revision exhausted");const auto after=before+(changed?1:0);
  const auto canonical=p.self.lock();need(canonical&&canonical.get()==this,"BNW policy cost lost actual factory owner");
  p.cost_observer_before=old;p.cost_observer_after=cost;p.cost_observer_pre_revision=before;p.cost_observer_post_revision=after;p.cost_observer_stored=false;p.cost_observer_inflight=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
  const auto observer=traits->prepare_policy_cost_observer_retirement(canonical,old,cost,before,after);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  *registry=std::move(next_registry);field.values[0]=cost;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.cost_observer_stored=true;
  // Earned exact paired store BEFORE obsolete diagnostic retirement. No
  // fabricated new Trait query result, public reset or HasTech mutation.
  BnwMajorPlayerTraitsOwner::accept_policy_cost_store(observer);
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.cost_observer_inflight=false;p.post_gp_scalar_operands[0]=static_cast<std::int32_t>(cost);++p.post_gp_scalar_cursor;
  const auto experience_store=[&](std::int64_t operand,bool add){
   auto& current=raw_player(state,p.actor);auto& target=named(current.core.fields,"m_iFreeExperience");const auto previous=scalar(current.core,"m_iFreeExperience"),value=add?native_sum(previous,operand):operand;
   need(operand>=INT32_MIN&&operand<=INT32_MAX&&previous==p.expected.process_scalars.at("m_iFreeExperience"),"BNW policy experience source/raw pair differs");
   auto* actual=state.setup.extensions.find<PolicyRegistry>();need(actual&&actual->players.contains(p.actor),"BNW policy experience native registry missing");auto prepared=p.expected;prepared.process_scalars.at("m_iFreeExperience")=value;auto prepared_registry=*actual;prepared_registry.players.at(p.actor)=prepared;auto copied=prepared;
   const bool differs=previous!=value;const auto pre=p.after;need(!differs||pre<UINT64_MAX,"BNW policy experience revision exhausted");const auto post=pre+(differs?1:0);require_stored_current(*host);binding.lease.require_operation(binding.serial);
   *actual=std::move(prepared_registry);target.values[0]=value;p.expected=std::move(copied);p.post_gp_scalar_operands.at(p.post_gp_scalar_cursor)=static_cast<std::int32_t>(operand);++p.post_gp_scalar_cursor;p.previous=pre;p.after=post;state.ui_source_revision=post;
   require_stored_current(*host);if(differs)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  };
  // SDK13417/13418 has TWO ASSIGNMENTS, not +=. Minors overwrites buildings;
  // preserve its real intermediate store even when final value is unchanged.
  experience_store(scalar(raw_player(state,p.actor).core,"m_iFreeExperienceFromBldgs"),false);
  experience_store(scalar(raw_player(state,p.actor).core,"m_iFreeExperienceFromMinors"),false);
  experience_store(numeric("FreeExperience"),true);
  p.phase=BnwPolicyAdoptionPhase::policy_camp_visibility_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

std::uint64_t BnwPolicyAdoptionPending::camp_visibility_cursor()const{(void)phase();return impl_->camp_cursor;}
bool BnwPolicyAdoptionPending::camp_visibility_returned()const{(void)phase();return impl_->camp_visibility_returned;}
std::optional<std::int64_t> BnwPolicyAdoptionPending::camp_visibility_changed_plot()const{(void)phase();return impl_->camp_changed_plot;}
void BnwPolicyAdoptionPending::advance_process_camp_visibility()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_camp_visibility_pending&&p.post_gp_scalar_cursor==4&&!p.camp_visibility_returned&&!p.camp_changed_plot,"BNW policy camp visibility unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
 try{
  auto& state=*p.state;const auto count=scalar(raw_player(state,p.actor).core,"m_iAlwaysSeeBarbCampsCount");
  // IsAlwaysSeeBarbCamps is the actual native counter >0 predicate. Its
  // false branch does not read map, team or camp rules at all.
  if(count<=0){p.camp_visibility_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_process_dirty_pending;return;}
  need(state.world.map_plots&&state.world.map_projection&&state.world.map_plots->layout==saves::GameStateLayout::expansion2_version1,"BNW policy camp actual map owner unavailable");
  auto& plots=state.world.map_plots->plots;const auto& cells=state.world.map_projection->cells;need(plots.size()==cells.size(),"BNW policy camp full map cardinality differs");
  const auto team=detail::bnw_player_team_read(state,static_cast<std::int32_t>(p.actor));
  const auto* define=host->defines().find("BARBARIAN_CAMP_IMPROVEMENT");need(host->defines().profile()==content::GlobalDefineProfile::bnw&&define&&define->lookup.found&&define->lookup.error.empty()&&!define->retained_previous&&std::holds_alternative<std::int64_t>(define->lookup.value),"BNW policy camp actual resolved global define absent");
  const auto camp=std::get<std::int64_t>(define->lookup.value);need(camp>=-1&&camp<=INT32_MAX,"BNW policy camp resolved native define width invalid");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  for(;p.camp_cursor<plots.size();++p.camp_cursor){
   auto& raw=plots.at(static_cast<std::size_t>(p.camp_cursor));const auto& cell=cells.at(static_cast<std::size_t>(p.camp_cursor));
   need(raw.index==p.camp_cursor&&cell.index==p.camp_cursor&&raw.x==cell.x&&raw.y==cell.y&&raw.version&&cell.revealed_teams,"BNW policy camp actual plot identity/reveal cache unavailable");
   const auto art=saves::observe_saved_plot_team_art(raw,saves::GameStateLayout::expansion2_version1,static_cast<std::uint64_t>(team));
   need(art.revealed.has_value()&&cell.revealed_teams->contains(team)==*art.revealed,"BNW policy camp native/current isRevealed pair differs");if(!*art.revealed)continue;
   std::int64_t improvement=-1,revealed=-1,target=-1;const char* array_name=nullptr;
   if(*raw.version>=6){
    const auto hash=named(raw.fields,"improvement_type_hash").value;need(hash>=0&&hash<=UINT32_MAX&&cell.improvement_type_hash&&*cell.improvement_type_hash==static_cast<std::uint64_t>(hash),"BNW policy camp native/projected improvement hash differs");
    if(hash){const auto* info=host->types().find_hash(static_cast<std::uint64_t>(hash));need(info&&info->table=="Improvements","BNW policy camp native improvement Info unavailable");improvement=info->id;}
    if(improvement!=camp)continue;
    need(art.revealed_improvement_hash.has_value(),"BNW policy camp native revealed hash absent");if(*art.revealed_improvement_hash){const auto* info=host->types().find_hash(*art.revealed_improvement_hash);need(info&&info->table=="Improvements","BNW policy camp actual old revealed improvement Info unavailable");revealed=info->id;}
    array_name="revealed_improvement_hashes";target=hash;
   }else{
    improvement=named(raw.fields,"improvement_type").value;need(cell.improvement_type&&*cell.improvement_type==improvement&&improvement>=-1&&improvement<=INT16_MAX,"BNW policy camp actual native improvement ID differs");if(improvement!=camp)continue;
    need(art.revealed_improvement.has_value(),"BNW policy camp actual old revealed improvement absent");revealed=*art.revealed_improvement;array_name="revealed_improvements";target=improvement;
   }
   if(revealed==improvement)continue;
   // Original setter stores m_aeRevealedImprovementType FIRST. Its active
   // team symbols/layout and all-team native notification then remain real
   // obligatory receiver work, never acknowledged through a supplied bool.
   auto& values=named(raw.arrays,array_name);need(values.columns==1&&values.values.size()==80&&team>=0&&team<64,"BNW policy camp actual revealed array capacity unavailable");
   const auto old=values.values.at(static_cast<std::size_t>(team));const auto before=p.after;need(before<UINT64_MAX,"BNW policy camp revision exhausted");const auto after=before+1;
   const bool active=team==detail::bnw_game_active_team_read(state);
   p.camp_array_name=array_name;p.camp_before_wire=old;p.camp_after_wire=target;p.camp_team=team;p.camp_changed_plot=static_cast<std::int64_t>(p.camp_cursor);p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   values.values.at(static_cast<std::size_t>(team))=target;p.camp_store_completed=true;p.previous=before;p.after=after;state.ui_source_revision=after;
   require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
   p.phase=active?BnwPolicyAdoptionPhase::policy_camp_plot_ui_pending:BnwPolicyAdoptionPhase::policy_camp_notification_pending;return;
  }
  require_stored_current(*host);binding.lease.require_operation(binding.serial);p.camp_visibility_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_process_dirty_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

bool BnwPolicyAdoptionPending::process_policies_returned()const{(void)phase();return impl_->process_policies_returned;}
void BnwPolicyAdoptionPending::require_policy_process_dirty_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&p.camp_visibility_returned&&p.post_gp_scalar_cursor==4&&p.great_people_recompute_returned&&!p.process_policies_returned&&
  ((p.phase==BnwPolicyAdoptionPhase::policy_process_dirty_pending&&!p.process_dirty_attempted)||
   (p.phase==BnwPolicyAdoptionPhase::policy_process_dirty_executing&&p.process_dirty_attempted)),
  "BNW processPolicies dirty requires genuine returned camp/GP/cost/experience predecessor");
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_policy_process_dirty_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_process_dirty_pending&&!p.process_dirty_attempted,"BNW processPolicies dirty already reserved/unreached");
 // Reserve before callbackful source/rules/UI providers. There is no public
 // setter counter, imported dirty flag or caller-supplied success grant.
 p.process_dirty_attempted=true;p.phase=BnwPolicyAdoptionPhase::policy_process_dirty_executing;
 try{require_policy_process_dirty_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_policy_process_dirty_return(const ui::BnwClientSelectionControl& selection)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_process_dirty_executing&&p.process_dirty_attempted&&!p.process_policies_returned,"BNW processPolicies dirty return unissued/repeated");
 try{
  selection.require_policy_process_dirty_return(*this);const auto host=p.host.lock();need(bool(host),"BNW processPolicies dirty Host retired");
  require_policy_process_dirty_source(*host,*p.state);selection.require_policy_process_dirty_return(*this);
  // Both native DLLUI setters returned in order (CityInfo18, GameData11).
  // Painting, outer next-cost/branch/hook/updateYield and full adoption are
  // separate requirements and cannot be inferred from this actual return.
  p.process_policies_returned=true;p.phase=p.is_finisher?BnwPolicyAdoptionPhase::finisher_process_returned:BnwPolicyAdoptionPhase::policy_outer_next_cost_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_policy_process_dirty_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

bool BnwPolicyAdoptionPending::next_policy_cost_returned()const{(void)phase();return impl_->next_policy_cost_returned;}
std::array<std::int32_t,4> BnwPolicyAdoptionPending::next_policy_cost_operands()const{(void)phase();return impl_->next_policy_cost_operands;}
void BnwPolicyAdoptionPending::advance_outer_next_policy_cost()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&(p.phase==BnwPolicyAdoptionPhase::policy_outer_next_cost_pending||(p.phase==BnwPolicyAdoptionPhase::policy_cost_pending&&p.early_process_policies_returned))&&(p.process_policies_returned||p.early_process_policies_returned)&&!p.next_cost_attempted,"BNW outer next policy cost unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);p.next_cost_attempted=true;
 try{
  auto& state=*p.state;auto& raw=raw_player(state,p.actor);need(state.world.pregame&&state.world.pregame->layout==saves::GameStateLayout::expansion2_version1,"BNW next policy cost native Pregame absent");
  const auto& pregame=*state.world.pregame;
  const auto info=[&](const char* table,const char* member,std::int64_t id){
   need(id>=0&&id<=INT32_MAX,"BNW next policy cost Info native index invalid");std::optional<std::int64_t> result;
   const auto report=host->database().prefetch_info(table,[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
    if(slot==static_cast<std::uint64_t>(id)){need(!result,"BNW next policy cost duplicate native Info");const auto type=row.text("Type").value_or("");const auto* mapped=host->types().find(type);
     need(mapped&&mapped->table==table&&mapped->id==id,"BNW next policy cost native Info/remap differs");result=row.integer(member);need(*result>=INT32_MIN&&*result<=INT32_MAX,"BNW next policy cost Info exceeds native int");}return true;
   });need(report.all_succeeded()&&result,"BNW next policy cost complete native Info unavailable");require_stored_current(*host);binding.lease.require_operation(binding.serial);return *result;
  };
  // SDK GetNumPoliciesOwned counts all actual HasPolicy entries, including
  // blocked branches. It does not read m_iNumPolicies or active modifiers.
  std::int64_t policies=0;for(const auto& [id,has]:p.expected.has){(void)id;if(has)policies=native_sum(policies,1);}
  const auto earned=native_sum(native_sum(p.expected.process_scalars.at("m_iNumFreePoliciesEver"),-p.expected.free_policies),-p.expected.free_tenets);
  policies=native_sum(policies,-earned);
  auto cost=native_product(policies,policy_define(*host,"POLICY_COST_INCREASE_TO_BE_EXPONENTED"));
  const auto* exponent=host->defines().find("POLICY_COST_EXPONENT");
  need(exponent&&host->defines().profile()==content::GlobalDefineProfile::bnw&&exponent->kind==content::GlobalDefineKind::source_float&&exponent->lookup.found&&exponent->lookup.error.empty()&&exponent->lookup.sqlite_code==0&&!exponent->retained_previous&&std::holds_alternative<double>(exponent->lookup.value),"BNW policy cost native float Define unavailable");
  const auto current_exponent=host->database().global_define("POLICY_COST_EXPONENT",content::GlobalDefineKind::source_float);
  need(current_exponent.found&&current_exponent.error.empty()&&current_exponent.sqlite_code==0&&current_exponent.value==exponent->lookup.value,"BNW policy cost selected native float Define differs");
  const auto power=std::pow(static_cast<double>(cost),std::get<double>(exponent->lookup.value));
  // Native out-of-domain conversion has no portable result to invent. Fail
  // readiness before the reached GetMaxEffectiveCities assignment instead.
  const auto truncated=std::trunc(power);need(std::isfinite(power)&&truncated>=INT32_MIN&&truncated<=INT32_MAX,"BNW policy cost native pow/int conversion undefined");cost=static_cast<std::int32_t>(truncated);
  cost=native_sum(cost,policy_define(*host,"BASE_POLICY_COST"));
  const auto world_id=pregame.archive.integers.at("world_size");need(world_id==pregame.world.id,"BNW policy cost actual WorldInfo binding differs");
  auto city_modifier=info("Worlds","NumCitiesPolicyCostMod",world_id);need(city_modifier==pregame.world.values.integers.at("num_cities_policy_cost_mod"),"BNW policy cost actual serialized WorldInfo differs from current selected Info");
  const auto discount=p.expected.process_scalars.at("m_iNumCitiesPolicyCostDiscount");if(discount!=0)city_modifier=native_product(city_modifier,native_sum(100,discount))/100;
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  // Reached native getter has a store. Preserve the two source city scans:
  // a puppet may also be in limbo and is then subtracted twice, exactly SDK.
  const auto effective=[&](){
   const auto& source=raw_player(state,p.actor);const auto& research=state.player_research.players.at(p.actor);need(source.cities.cities.size()<=INT32_MAX&&research.cities.size()==source.cities.cities.size()&&state.cities.players.at(p.actor).cities.size()==source.cities.cities.size(),"BNW policy cost full current city roster differs");
   std::int64_t cities=static_cast<std::int64_t>(source.cities.cities.size()),puppets=0,limbo=0;std::set<std::int64_t> visited;
   for(const auto& city:source.cities.cities){const auto id=city_scalar(city,"m_iID");need(visited.insert(id).second&&research.cities.contains(id)&&resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(p.actor),static_cast<std::int32_t>(id))==&city,"BNW policy cost genuine city ID/FLTA registry differs");
    const auto& puppet=named(city.core.fields,"m_bPuppet");need(puppet.encoding==saves::CityCoreEncoding::boolean&&puppet.values.size()==1&&(puppet.values[0]==0||puppet.values[0]==1)&&research.cities.at(id).puppet==(puppet.values[0]!=0),"BNW policy cost actual puppet getter pair differs");if(research.cities.at(id).puppet)puppets=native_sum(puppets,1);
   }
   cities=native_sum(cities,-puppets);
   for(const auto& city:source.cities.cities){const auto id=city_scalar(city,"m_iID");const auto& owned=research.cities.at(id);need(owned.razing==(city_scalar(city,"m_iRazingTurns")>0),"BNW policy cost actual razing getter pair differs");
    // BNW IsIgnoreCityForHappiness is constructor-owned and unsaved. Use its
    // existing restored/live owned getter; never demand a VP raw field.
    if(owned.ignore_happiness||owned.razing)limbo=native_sum(limbo,1);
   }
   cities=native_sum(cities,-limbo);if(cities==0)cities=1;return cities;
  };
  const auto cities=effective(),old_max=p.expected.process_scalars.at("m_iMaxEffectiveCities"),maximum=std::max(old_max,cities);
  auto& max_field=named(raw.core.fields,"m_iMaxEffectiveCities");need(max_field.encoding==saves::PlayerCoreEncoding::i32&&max_field.values.size()==1&&max_field.values[0]==old_max&&state.player_research.players.at(p.actor).max_effective_cities==old_max,"BNW policy cost effective city maximum pair differs");
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW policy next cost native registry missing");auto desired=p.expected;desired.process_scalars.at("m_iMaxEffectiveCities")=maximum;auto prepared=*registry;prepared.players.at(p.actor)=desired;auto expected_copy=desired;
  const bool max_changed=maximum!=old_max;const auto max_before=p.after;need(!max_changed||max_before<UINT64_MAX,"BNW policy maximum city revision exhausted");const auto max_after=max_before+(max_changed?1:0);
  require_stored_current(*host);binding.lease.require_operation(binding.serial);need(effective()==cities&&scalar(raw.core,"m_iMaxEffectiveCities")==old_max,"BNW next policy city source changed before reached native store");p.phase=BnwPolicyAdoptionPhase::uncertain;
  *registry=std::move(prepared);max_field.values[0]=maximum;state.player_research.players.at(p.actor).max_effective_cities=maximum;p.expected=std::move(expected_copy);p.previous=max_before;p.after=max_after;state.ui_source_revision=max_after;
  p.next_policy_cost_operands[0]=static_cast<std::int32_t>(cities);p.next_policy_cost_operands[1]=static_cast<std::int32_t>(old_max);p.next_policy_cost_operands[2]=static_cast<std::int32_t>(maximum);
  require_stored_current(*host);if(max_changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  // Continue only after the actual getter returned its stored maximum. Every
  // native multiply/divide keeps SDK source order and truncation boundaries.
  city_modifier=native_product(native_product(cost,native_sum(maximum,-1)),city_modifier)/100;cost=native_sum(cost,city_modifier);
  cost=native_product(cost,native_sum(100,p.expected.process_scalars.at("m_iPolicyCostModifier")))/100;
  const auto speed=pregame.archive.integers.at("game_speed");need(speed==state.setup.speed,"BNW policy cost actual GameSpeed binding differs");cost=native_product(cost,info("GameSpeeds","CulturePercent",speed))/100;
  const auto& handicaps=pregame.archive.integer_arrays.at("handicaps");need(handicaps.size()==64&&handicaps.at(static_cast<std::size_t>(p.actor))==state.setup.players.at(p.actor).handicap,"BNW policy cost actual Handicap binding differs");cost=native_product(cost,info("HandicapInfos","PolicyPercent",state.setup.players.at(p.actor).handicap))/100;
  const auto divisor=policy_define(*host,"POLICY_COST_VISIBLE_DIVISOR");need(divisor!=0&&!(cost==INT32_MIN&&divisor==-1),"BNW policy visible cost native division undefined");cost=native_product(cost/divisor,divisor);need(cost>=0,"BNW next policy cost native nonnegative invariant failed");
  auto& next_field=named(raw.core.fields,"m_iCostNextPolicy");need(next_field.encoding==saves::PlayerCoreEncoding::i32&&next_field.values.size()==1&&next_field.values[0]==p.expected.cost,"BNW next policy cost existing paired destination differs");
  registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW next policy cost registry retired");desired=p.expected;desired.cost=cost;prepared=*registry;prepared.players.at(p.actor)=desired;expected_copy=desired;
  const bool changed=cost!=p.expected.cost;const auto before=p.after;need(!changed||before<UINT64_MAX,"BNW next policy cost revision exhausted");const auto after=before+(changed?1:0);require_stored_current(*host);binding.lease.require_operation(binding.serial);
  *registry=std::move(prepared);next_field.values[0]=cost;p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.next_policy_cost_operands[3]=static_cast<std::int32_t>(cost);
  require_stored_current(*host);if(changed)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);p.next_policy_cost_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_branch_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

bool BnwPolicyAdoptionPending::outer_policy_branch_returned()const{(void)phase();return impl_->outer_policy_branch_returned;}
void BnwPolicyAdoptionPending::advance_outer_policy_branch()const{
 (void)phase();auto& p=*impl_;const auto host=p.host.lock();need(host&&p.phase==BnwPolicyAdoptionPhase::policy_outer_branch_pending&&p.next_policy_cost_returned&&!p.outer_branch_attempted,"BNW outer branch setter unreached/repeated");
 require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);p.outer_branch_attempted=true;
 try{
  const auto& selected=p.rules.policies.at(p.target);const auto* branch=host->types().find(selected.branch);need(branch&&branch->table=="PolicyBranchTypes"&&p.expected.unlocked.contains(branch->id),"BNW outer branch setter actual policy branch unavailable");
  std::size_t matches=0;const auto report=host->database().prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){if(slot==static_cast<std::uint64_t>(p.target)){need(row.text("Type").value_or("")==selected.type&&row.text("PolicyBranchType").value_or("")==selected.branch,"BNW outer current GetPolicyBranchType differs");++matches;}return true;});
  need(report.all_succeeded()&&matches==1,"BNW outer branch actual policy Info missing");require_stored_current(*host);binding.lease.require_operation(binding.serial);
  // SDK SetPolicyBranchUnlocked first compares the live flag. Already true
  // calls no free-policy helper, world-player scan, localization or setters.
  if(p.expected.unlocked.at(branch->id)){
   p.outer_policy_branch_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_dirty_pending;return;
  }
  // The changed branch must first grant native first/second-adopter policies
  // or tenets and actual ideology notifications, then store the unlocked
  // flag. No successful no-op, premature flag, UI ACK or public grant.
  p.phase=BnwPolicyAdoptionPhase::policy_outer_branch_rewards_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}

bool BnwPolicyAdoptionPending::outer_policy_dirty_returned()const{(void)phase();return impl_->outer_policy_dirty_returned;}
void BnwPolicyAdoptionPending::require_outer_policy_dirty_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&(p.process_policies_returned||p.early_process_policies_returned)&&p.next_policy_cost_returned&&p.outer_policy_branch_returned&&!p.outer_policy_dirty_returned&&
  ((p.phase==BnwPolicyAdoptionPhase::policy_outer_dirty_pending&&!p.outer_dirty_attempted)||(p.phase==BnwPolicyAdoptionPhase::policy_outer_dirty_executing&&p.outer_dirty_attempted)),"BNW outer policy dirty requires exact returned native predecessors");
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_outer_policy_dirty_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_dirty_pending&&!p.outer_dirty_attempted,"BNW outer policy dirty already reserved/unreached");p.outer_dirty_attempted=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_dirty_executing;
 try{require_outer_policy_dirty_source(host,state);return p.hooks->impl_->lease;}catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
bool BnwPolicyAdoptionPending::outer_policy_actor_is_active(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 // Selection invokes this actual native GC active-player getter AFTER its
 // GameData11 setter returned, matching the original outer source ordering.
 require_outer_policy_dirty_source(host,state);const auto active=detail::bnw_game_active_player_read(state);require_outer_policy_dirty_source(host,state);return active==impl_->actor;
}
void BnwPolicyAdoptionPending::accept_actual_outer_policy_dirty_return(const ui::BnwClientSelectionControl& selection)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_dirty_executing&&p.outer_dirty_attempted&&!p.outer_policy_dirty_returned,"BNW outer policy dirty return unissued/repeated");
 try{selection.require_outer_policy_dirty_return(*this);const auto host=p.host.lock();need(bool(host),"BNW outer dirty Host retired");require_outer_policy_dirty_source(*host,*p.state);selection.require_outer_policy_dirty_return(*this);
  p.outer_policy_dirty_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_hook_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_outer_policy_dirty_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}

bool BnwPolicyAdoptionPending::outer_policy_hook_returned()const{(void)phase();return impl_->outer_policy_hook_returned;}
void BnwPolicyAdoptionPending::require_outer_policy_hook_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 need(&state==p.state.get()&&(p.process_policies_returned||p.early_process_policies_returned)&&p.next_policy_cost_returned&&p.outer_policy_branch_returned&&p.outer_policy_dirty_returned&&!p.outer_policy_hook_returned&&
  ((p.phase==BnwPolicyAdoptionPhase::policy_outer_hook_pending&&!p.outer_hook_attempted)||(p.phase==BnwPolicyAdoptionPhase::policy_outer_hook_executing&&p.outer_hook_attempted)),"BNW PlayerAdoptPolicy hook requires exact returned native/UI predecessors");
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_outer_policy_hook_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_hook_pending&&!p.outer_hook_attempted,"BNW PlayerAdoptPolicy hook already reserved/unreached");p.outer_hook_attempted=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_hook_executing;
 try{require_outer_policy_hook_source(host,state);return p.hooks->impl_->lease;}catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_outer_policy_hook_return(const BnwSelectedStockLuaSourceOwner& source)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_hook_executing&&p.outer_hook_attempted&&!p.outer_policy_hook_returned,"BNW PlayerAdoptPolicy return unissued/repeated");
 try{source.require_policy_adopt_hook_return(*this);const auto host=p.host.lock();need(bool(host),"BNW policy hook Host retired");require_outer_policy_hook_source(*host,*p.state);source.require_policy_adopt_hook_return(*this);
  // A genuine CallHook false/no-listeners return is normal only when earned
  // by this exact retained SAME VM. Script/API errors cannot become success.
  p.outer_policy_hook_returned=true;p.map_cursor=0;p.map_complete=false;p.phase=BnwPolicyAdoptionPhase::policy_outer_yield_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_outer_policy_hook_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(!impl_)return;impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}
bool BnwPolicyAdoptionPending::outer_policy_yield_returned()const{(void)phase();return impl_->outer_policy_yield_returned;}
std::uint64_t BnwPolicyAdoptionPending::outer_policy_yield_scanned()const{(void)phase();return impl_->outer_yield_scanned;}
bool BnwPolicyAdoptionPending::early_process_policies_returned()const{(void)phase();return impl_->early_process_policies_returned;}

void BnwPolicyAdoptionPending::consume_actual_policy_plot_ui()const{
 (void)phase();auto& p=*impl_;need((p.phase==BnwPolicyAdoptionPhase::plot_ui_pending||p.phase==BnwPolicyAdoptionPhase::policy_outer_plot_ui_pending||p.phase==BnwPolicyAdoptionPhase::golden_age_plot_ui_pending)&&p.plot_child&&!p.plot_ui_dispatching,"BNW actual policy plot UI unreached/repeated/recursive");
 // Once reserve before any rules/currentness/native selection provider.
 p.plot_ui_dispatching=true;const auto held_child=p.plot_child;const auto held_phase=p.plot_parent_call_phase;
 try{
  const auto host=p.host.lock();need(bool(host),"BNW actual policy plot UI Host retired");require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
  need(held_phase==BnwPolicyAdoptionPhase::plot_yield_pending||held_phase==BnwPolicyAdoptionPhase::improvement_plot_yield_pending||held_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending||held_phase==BnwPolicyAdoptionPhase::golden_age_map_pending||held_phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending,"BNW actual policy plot UI lost original native call ordinal");
  auto& state=*p.state;auto& session=*p.session;const auto plot=static_cast<std::int64_t>(p.map_cursor);
  need(p.map_cursor<=INT32_MAX&&session.plot_yield_refresh_pending_.contains(plot)&&session.plot_yield_refresh_pending_.at(plot)==held_child,"BNW actual policy plot UI registered child replaced");held_child->require_policy_parent_source(*this);
  // This existing genuine adapter runs the current native selection predicate,
  // publishes original DLLUI CityScreen21 and typed native symbols event.
  // It does not fabricate painting or consume renderer completion flags.
  const auto adapter=ui::BnwPlotYieldRefreshUiAdapter::from_selected_host(host);adapter->require_current();require_stored_current(*host);binding.lease.require_operation(binding.serial);
  for(std::uint8_t calls=0;calls<13;++calls){
   held_child->require_policy_parent_source(*this);const auto before_progress=held_child->progress();
   if(before_progress.phase==BnwPlotYieldRefreshPhase::complete||before_progress.phase==BnwPlotYieldRefreshPhase::skipped_no_area)break;
   need(before_progress.phase==BnwPlotYieldRefreshPhase::city_screen_predicate||before_progress.phase==BnwPlotYieldRefreshPhase::city_screen_dirty||before_progress.phase==BnwPlotYieldRefreshPhase::symbols,"BNW actual policy plot UI child is not at a genuine native UI call");
   const auto facts=held_child->ui_tail_source();need(facts.state==p.state&&facts.generation==p.generation&&facts.plot==plot&&facts.operation==before_progress.operation,"BNW actual policy plot UI canonical native facts differ");
   const auto cities_before=state.city_dirty_events;const auto plots_before=state.plot_gameplay_events;const auto city_occurrence=state.city_dirty_next_occurrence,plot_occurrence=state.plot_gameplay_next_occurrence,revision=p.after;
   require_stored_current(*host);binding.lease.require_operation(binding.serial);held_child->require_policy_parent_source(*this);
   const auto after_progress=adapter->continue_plot(plot);held_child->require_policy_parent_source(*this);
   need(session.read_state_lease()==p.state&&session.state_generation()==p.generation&&session.plot_yield_refresh_pending_.at(plot)==held_child&&after_progress.operation==before_progress.operation&&after_progress.plot==plot&&after_progress.plot_writes>=before_progress.plot_writes&&after_progress.terrain_writes>=before_progress.terrain_writes,"BNW actual policy plot UI replaced native child/source/counters");
   need(state.city_dirty_events.size()>=cities_before.size()&&state.plot_gameplay_events.size()>=plots_before.size()&&std::equal(cities_before.begin(),cities_before.end(),state.city_dirty_events.begin()),"BNW actual policy plot UI rewrote prior native notifications");
   const auto same_event=[](const BnwPlotGameplayEvent& a,const BnwPlotGameplayEvent& z){return a.occurrence==z.occurrence&&a.generation==z.generation&&a.operation==z.operation&&a.target_revision==z.target_revision&&a.map_guid==z.map_guid&&a.plot==z.plot&&a.x==z.x&&a.y==z.y&&a.yields==z.yields&&a.kind==z.kind;};
   need(std::equal(plots_before.begin(),plots_before.end(),state.plot_gameplay_events.begin(),same_event),"BNW actual policy plot UI changed prior gameplay event bytes");
   const auto city_delta=state.city_dirty_events.size()-cities_before.size(),plot_delta=state.plot_gameplay_events.size()-plots_before.size();need(city_delta<=1&&plot_delta<=1&&!(city_delta&&plot_delta),"BNW actual policy plot UI emitted unrelated native notifications");
   need(city_occurrence<=UINT64_MAX-city_delta&&plot_occurrence<=UINT64_MAX-plot_delta&&state.city_dirty_next_occurrence==city_occurrence+city_delta&&state.plot_gameplay_next_occurrence==plot_occurrence+plot_delta,"BNW actual policy plot UI native occurrence counters differ");
   if(city_delta){const auto& event=state.city_dirty_events.back();need(facts.city&&event.occurrence==city_occurrence&&event.generation==p.generation&&event.owner==facts.city->first&&event.city==facts.city->second&&event.plot==plot&&event.yield==facts.next_yield&&event.kind==BnwCityDirtyKind::city_screen&&event.value&&before_progress.phase!=BnwPlotYieldRefreshPhase::symbols,"BNW actual policy CityScreen setter event is not this child/source phase");}
   if(plot_delta){const auto& event=state.plot_gameplay_events.back();need(before_progress.phase==BnwPlotYieldRefreshPhase::symbols&&event.occurrence==plot_occurrence&&event.generation==p.generation&&event.operation==facts.operation&&event.map_guid==facts.map_guid&&event.plot==plot&&event.x==facts.x&&event.y==facts.y&&event.yields==facts.yields&&event.kind==BnwPlotGameplayEventKind::yield_might_have_changed&&event.target_revision==state.ui_source_revision,"BNW actual policy native symbols publication is not this issued child");}
   const auto writes=std::uint64_t(after_progress.plot_writes-before_progress.plot_writes)+std::uint64_t(after_progress.terrain_writes-before_progress.terrain_writes)+city_delta+plot_delta;
   need(revision<=UINT64_MAX-writes&&state.ui_source_revision==revision+writes,"BNW actual policy plot UI revision is not exactly its earned native stores/publications");
   p.previous=revision;p.after=state.ui_source_revision;p.phase=BnwPolicyAdoptionPhase::uncertain;require_stored_current(*host);if(writes)binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
   p.phase=held_phase==BnwPolicyAdoptionPhase::golden_age_map_pending?BnwPolicyAdoptionPhase::golden_age_plot_ui_pending:(held_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending?BnwPolicyAdoptionPhase::policy_outer_plot_ui_pending:BnwPolicyAdoptionPhase::plot_ui_pending);
  }
  held_child->require_policy_parent_source(*this);const auto returned=held_child->progress();need(returned.phase==BnwPlotYieldRefreshPhase::complete||returned.phase==BnwPlotYieldRefreshPhase::skipped_no_area,"BNW actual policy plot UI exceeded finite six-yield native tail (no replay)");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);need(p.plot_child==held_child&&p.map_cursor<state.world.map_plots->plots.size(),"BNW actual policy plot return changed original map cursor");
  p.plot_child.reset();p.phase=held_phase;p.plot_ui_dispatching=false;
  if(held_phase==BnwPolicyAdoptionPhase::resource_policy_yield_pending){if(!finish_resource_policy_plot())return;}
  else ++p.map_cursor;
  if(held_phase==BnwPolicyAdoptionPhase::policy_outer_yield_pending){p.outer_yield_scanned=p.map_cursor;p.outer_yield_resuming=true;}
  if(held_phase==BnwPolicyAdoptionPhase::golden_age_map_pending){p.golden_age_map_scanned=p.map_cursor;p.golden_age_map_resuming=true;}
  // Continue the original native map loop only after this precise real child
  // returned. A later changed child will reserve a fresh adapter call again.
  advance_process_plot_yields();
 }catch(...){
  p.plot_ui_dispatching=false;
  // The resumed native final map call may have already returned before its
  // final provider throws. Preserve that earned history, never swallow failure.
  if(p.phase!=BnwPolicyAdoptionPhase::adoption_returned||!p.outer_policy_yield_returned)p.phase=BnwPolicyAdoptionPhase::uncertain;
  throw;
 }
}

std::optional<std::int32_t> BnwPolicyAdoptionPending::outer_branch_free_policy_operand()const{(void)phase();return impl_->outer_branch_free_policy_operand;}
std::optional<BnwPolicyBranchCreditHistory> BnwPolicyAdoptionPending::outer_branch_credit_history()const noexcept{
 // Creator identity is immutable and read BEFORE private mutable history.
 // A late failure preserves earned facts, never source currentness or retry.
 if(creator_!=std::this_thread::get_id()||!impl_)return std::nullopt;
 return impl_->outer_branch_credit_history;
}
void BnwPolicyAdoptionPending::advance_outer_policy_branch_zero_reward()const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_branch_rewards_pending&&p.outer_branch_attempted&&!p.outer_branch_rewards_attempted&&!p.outer_policy_branch_returned,"BNW actual outer branch reward helper unreached/repeated");p.outer_branch_rewards_attempted=true;
 try{
  const auto host=p.host.lock();need(bool(host),"BNW outer branch reward Host retired");require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);auto& state=*p.state;
  const auto* branch=host->types().find(p.rules.policies.at(p.target).branch);need(branch&&branch->table=="PolicyBranchTypes"&&p.expected.unlocked.contains(branch->id)&&!p.expected.unlocked.at(branch->id),"BNW actual outer branch is not current locked native branch");
  std::optional<std::int64_t> era,first,second,purchase;const auto report=host->database().prefetch_info("PolicyBranchTypes",[&](std::uint64_t slot,std::int64_t,const content::RulesRowView& row){
   if(slot==static_cast<std::uint64_t>(branch->id)){need(!era&&row.text("Type").value_or("")==branch->type,"BNW outer branch reward current Info changed");const auto reference=row.text("EraPrereq").value_or("");const auto* info=reference.empty()?nullptr:host->types().find(reference);need(reference.empty()||(info&&info->table=="Eras"),"BNW outer branch native era prerequisite unresolved");era=info?info->id:-1;first=row.integer("FirstAdopterFreePolicies");second=row.integer("SecondAdopterFreePolicies");purchase=row.integer("PurchaseByLevel");}return true;
  });need(report.all_succeeded()&&era&&first&&second&&purchase&&(*purchase==0||*purchase==1)&&*first>=INT32_MIN&&*first<=INT32_MAX&&*second>=INT32_MIN&&*second<=INT32_MAX,"BNW outer branch actual native reward Info unavailable");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);need(state.world.pregame&&state.world.pregame->layout==saves::GameStateLayout::expansion2_version1&&state.world.pregame->archive.integers.at("era")==state.setup.start_era,"BNW branch free-policy helper actual start era pair differs");
  const auto count_unlockers=[&](){
   const auto& minor_flags=state.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(minor_flags.size()==64,"BNW branch free-policy helper actual minor roster missing");std::int64_t count=0;
   // MAX_CIV_PLAYERS excludes barbarian ID63. Alive short circuit precedes
   // native Pregame isMinorCiv and then actual branch-unlocked flag getter.
   for(std::int32_t actor=0;actor<63;++actor){const auto& raw=raw_player(state,actor);need(raw.strategy.dimensions.civ_players==63,"BNW branch free-policy helper selected native scan domain differs");const auto& alive=named(raw.core.fields,"m_bAlive");need(alive.encoding==saves::PlayerCoreEncoding::boolean&&alive.values.size()==1&&(alive.values[0]==0||alive.values[0]==1)&&state.setup.players.at(actor).alive==(alive.values[0]!=0),"BNW branch free-policy helper native alive pair changed");if(!alive.values[0])continue;
    need(state.player_research.players.contains(actor)&&state.player_research.players.at(actor).pregame_minor==minor_flags.at(static_cast<std::size_t>(actor)),"BNW branch free-policy helper current minor pair changed");if(minor_flags.at(static_cast<std::size_t>(actor)))continue;
    const auto& block=named(raw.strategy.blocks,"policies");const auto unlocked=flags(named(block.hashes,"branch_unlocked"),p.rules.branch_hashes);if(unlocked.at(branch->id))count=native_sum(count,1);
   }return count;
  };
  std::int64_t free=0;std::optional<std::int64_t> unlockers;if(*era>=state.setup.start_era){unlockers=count_unlockers();if(*unlockers==0)free=*first;else if(*unlockers==1)free=*second;}
  require_stored_current(*host);binding.lease.require_operation(binding.serial);p.outer_branch_free_policy_operand=static_cast<std::int32_t>(free);
  bool branch_credit_stored=false;
  if(*purchase!=0||free!=0){
   // SDK SetPolicyBranchUnlocked(bRevolution=false) calls the real credit
   // setter BEFORE any free-policy/ideology notification and unlocked flag.
   const bool tenets=*purchase!=0;
   auto& raw=raw_player(state,p.actor);
   auto& credits=named(raw.core.fields,tenets?"m_iNumFreeTenets":"m_iNumFreePolicies");
   auto& ever=named(raw.core.fields,"m_iNumFreePoliciesEver");
   const auto old=tenets?p.expected.free_tenets:p.expected.free_policies;
   const auto old_ever=p.expected.process_scalars.at("m_iNumFreePoliciesEver");
   need(credits.encoding==saves::PlayerCoreEncoding::i32&&credits.values.size()==1&&credits.values[0]==old&&
    ever.encoding==saves::PlayerCoreEncoding::i32&&ever.values.size()==1&&ever.values[0]==old_ever,
    "BNW branch reward raw/owned credit pair differs");
   const auto value=native_sum(old,free),ever_value=native_sum(old_ever,free>0?free:0);
   need(value>=0,"BNW branch reward negative balance requires invalid native policy state");
   // Plain current operands are re-read after the final callbackful guards.
   // A NULL Notifications pointer is a reached native short circuit, never
   // inferred from a missing owner, failed decode, or a default flag.
   const auto current_free_notification=[&](){
    const auto& current_raw=raw_player(state,p.actor);
    if(free<=0||current_raw.cities.cities.empty())return false;
    need(state.cities.players.contains(p.actor)&&state.cities.players.at(p.actor).cities.size()==current_raw.cities.cities.size(),
     "BNW branch reward actual city count pair differs");
    need(current_raw.administration.layout==saves::GameStateLayout::expansion2_version1,
     "BNW branch reward actual Notifications layout absent");
    const auto& notifications=named(current_raw.administration.blocks,"notifications");
    const auto present=named(notifications.fields,"has_notifications").value;
    need(present==0||present==1,"BNW branch reward actual Notifications pointer outside bool");
    return present!=0;
   };
   const bool free_notification=current_free_notification();
   auto* registry=state.setup.extensions.find<PolicyRegistry>();
   need(registry&&registry->players.contains(p.actor),"BNW branch reward native registry missing");
   auto desired=p.expected;
   if(tenets)desired.free_tenets=value;else desired.free_policies=value;
   desired.process_scalars.at("m_iNumFreePoliciesEver")=ever_value;
   auto next_registry=*registry;next_registry.players.at(p.actor)=desired;auto expected_copy=desired;
   const auto before=p.after;const bool changed=value!=old||ever_value!=old_ever;
   need(!changed||before<UINT64_MAX,"BNW branch credit revision exhausted");
   const auto after=before+(changed?1u:0u);
   const BnwPolicyBranchCreditHistory earned{static_cast<std::int32_t>(branch->id),static_cast<std::int32_t>(free),tenets,free_notification,old,value,old_ever,ever_value,before,after};
   require_stored_current(*host);binding.lease.require_operation(binding.serial);
   need(state.world.pregame->archive.integers.at("era")==state.setup.start_era&&
    (!unlockers||count_unlockers()==*unlockers)&&credits.values[0]==old&&ever.values[0]==old_ever&&
    !p.expected.unlocked.at(branch->id)&&!p.outer_branch_credit_history&&current_free_notification()==free_notification,
    "BNW branch reward operands changed before real setters");
   // All allocations precede this no-callback interval. Native setter order:
   // ChangeNumFreePoliciesEver(positive difference), THEN balance assignment.
   if(free>0)ever.values[0]=ever_value;
   credits.values[0]=value;*registry=std::move(next_registry);p.expected=std::move(expected_copy);
   p.previous=before;p.after=after;state.ui_source_revision=after;
   p.outer_branch_credit_history=earned;branch_credit_stored=true;p.phase=BnwPolicyAdoptionPhase::uncertain;
   require_stored_current(*host);if(changed)binding.accept(*this);
   binding.lease.require_operation(binding.serial);require_stored_current(*host);
   // ChangeNumFreeTenets' free-policy Add precedes the SDK 22-player
   // ideology loop. Even zero tenets still reaches that ideology loop.
   // Neither actual notification supplier is replaced by an unlocked flag.
   if(tenets||free_notification){p.phase=BnwPolicyAdoptionPhase::policy_outer_branch_notifications_pending;return;}
  }
  auto& raw=raw_player(state,p.actor);auto& block=named(raw.strategy.blocks,"policies");auto& credits=named(raw.core.fields,"m_iNumFreePolicies");need(credits.encoding==saves::PlayerCoreEncoding::i32&&credits.values.size()==1&&credits.values[0]==p.expected.free_policies,"BNW branch zero-credit native destination differs");
  auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW branch zero-credit native registry missing");auto desired=p.expected;desired.unlocked.at(branch->id)=true;auto prepared=*registry;prepared.players.at(p.actor)=desired;auto expected_copy=desired;auto staged=named(block.hashes,"branch_unlocked");set_flag(staged,branch->hash,true);
  const auto before=p.after;need(before<UINT64_MAX,"BNW native branch unlocked revision exhausted");const auto after=before+1;require_stored_current(*host);binding.lease.require_operation(binding.serial);
  need(state.world.pregame->archive.integers.at("era")==state.setup.start_era&&(!unlockers||count_unlockers()==*unlockers)&&credits.values[0]==p.expected.free_policies,"BNW native branch reward operands changed before setters");
  // Real ChangeNumFreePolicies(0) calls its plain SetNumFreePolicies with the
  // unchanged actual value; no ever counter increment or notification. Native
  // branch flag assignment follows this setter and cannot precede rewards.
  if(!branch_credit_stored)credits.values[0]=p.expected.free_policies;auto& native=named(block.hashes,"branch_unlocked");native.rows.swap(staged.rows);native.wire_count=staged.wire_count;*registry=std::move(prepared);p.expected=std::move(expected_copy);p.previous=before;p.after=after;state.ui_source_revision=after;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);p.outer_policy_branch_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_dirty_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
}

namespace civ5::runtime {
std::vector<BnwPolicyBranchNotificationHistory> BnwPolicyAdoptionPending::outer_branch_notification_history()const{
 need(creator_==std::this_thread::get_id()&&bool(impl_),"BNW branch notification history foreign/absent");return impl_->outer_branch_notifications;
}
std::uint64_t BnwPolicyAdoptionPending::private_outer_branch_notification_serial()const{
 need(creator_==std::this_thread::get_id()&&bool(impl_),"BNW branch notification serial foreign/absent");return impl_->hooks->impl_->serial;
}
BnwPolicyBranchNotificationOperands BnwPolicyAdoptionPending::outer_branch_notification_operands_without_callbacks()const{
 need(creator_==std::this_thread::get_id()&&!retired_.load()&&bool(impl_),"BNW branch notification operands foreign/retired");const auto& p=*impl_;
 need(p.thread==std::this_thread::get_id()&&p.outer_branch_credit_history&&p.outer_branch_notifications_attempted&&
  p.phase==BnwPolicyAdoptionPhase::policy_outer_branch_notifications_executing&&!p.outer_policy_branch_returned,
  "BNW branch notification operands not genuinely reserved");
 const auto& credit=*p.outer_branch_credit_history;const auto& state=*p.state;
 const auto limit=credit.tenets?23:1;
 need(p.outer_branch_notification_cursor>=0&&p.outer_branch_notification_cursor<limit&&state.ui_source_revision==p.after,
  "BNW branch notification ordinal/current world differs");
 BnwPolicyBranchNotificationOperands q;q.ordinal=p.outer_branch_notification_cursor;q.actor=static_cast<std::int32_t>(p.actor);
 q.branch=credit.branch;q.generation=p.generation;q.revision=p.after;q.free_policy=q.ordinal==0;
 q.recipient=q.free_policy?q.actor:q.ordinal-1;
 if(q.free_policy&&!credit.free_policy_notification_required){q.skip=true;return q;}
 if(!q.free_policy&&q.recipient==q.actor){q.skip=true;return q;}
 if(!q.free_policy){
  need(state.world.pregame&&state.world.teams&&state.world.teams->teams.size()==64,"BNW ideology notification actual teams absent");
  const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");need(teams.size()==64,"BNW ideology notification team slots absent");
  const auto mine=teams.at(static_cast<std::size_t>(q.recipient)),theirs=teams.at(static_cast<std::size_t>(q.actor));
  need(mine>=0&&mine<64&&theirs>=0&&theirs<64&&state.setup.players.at(q.recipient).team==mine&&state.setup.players.at(q.actor).team==theirs,
   "BNW ideology notification actual team pair differs");
  const auto& team=state.world.teams->teams.at(static_cast<std::size_t>(mine));need(team.index==static_cast<std::uint64_t>(mine)&&named(team.fields,"id").value==mine,"BNW ideology notification Team identity differs");
  const auto& met=named(team.arrays,"has_met");need(met.columns==1&&met.values.size()==64,"BNW ideology notification native met domain differs");
  const auto value=met.values.at(static_cast<std::size_t>(theirs));need((value==0||value==1)&&state.player_research.teams.at(mine).met.contains(theirs)==(value!=0),"BNW ideology notification raw/owned met differs");q.has_met=value!=0;
 }
 // Original GetNotifications occurs AFTER isHasMet for each other recipient.
 const auto& raw=raw_player(state,q.recipient);need(raw.administration.layout==saves::GameStateLayout::expansion2_version1,"BNW branch notification actual owner layout absent");
 const auto present=named(named(raw.administration.blocks,"notifications").fields,"has_notifications").value;
 need(present==0||present==1,"BNW branch notification pointer outside native bool");q.present=present!=0;return q;
}
void BnwPolicyAdoptionPending::require_outer_branch_notification_source(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();const auto& p=*impl_;need(&state==p.state.get()&&p.outer_branch_credit_history&&p.outer_branch_notifications_attempted&&
  p.phase==BnwPolicyAdoptionPhase::policy_outer_branch_notifications_executing&&!p.outer_policy_branch_returned,
  "BNW branch notification source not actual outer credit prefix");require_stored_current(host);p.hooks->impl_->lease.require_operation(p.hooks->impl_->serial);
 const auto& h=*p.outer_branch_credit_history;need(!p.expected.unlocked.at(h.branch)&&
  (h.tenets?p.expected.free_tenets:p.expected.free_policies)==h.credits_after&&p.expected.process_scalars.at("m_iNumFreePoliciesEver")==h.ever_after,
  "BNW branch notification earned credit/locked branch pair differs");
 (void)outer_branch_notification_operands_without_callbacks();
}
BnwStockLuaAttemptLease BnwPolicyAdoptionPending::begin_outer_branch_notification_dispatch(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_branch_notifications_pending&&p.outer_branch_credit_history&&!p.outer_branch_notifications_attempted,
  "BNW branch notification batch unreached/repeated");
 // Reserve BEFORE allocation, locale, source currentness or receiver retention.
 p.outer_branch_notifications_attempted=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_branch_notifications_executing;
 try{p.outer_branch_notifications.reserve(23);require_outer_branch_notification_source(host,state);return p.hooks->impl_->lease;}
 catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::accept_actual_outer_branch_notification_return(const ui::BnwEngineUiTurnOwner& engine)const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_branch_notifications_executing,"BNW branch notification acceptance unreserved");
 try{
  // PURE physical return first; no stale-revision provider in this interval.
  const auto h=engine.require_outer_branch_notification_return(*this);
  need(h.operands.ordinal==p.outer_branch_notification_cursor&&h.operands.actor==p.actor&&h.operands.generation==p.generation&&
   h.operands.revision==p.after&&h.revision_after==p.state->ui_source_revision&&
   p.outer_branch_notifications.size()==static_cast<std::size_t>(h.operands.ordinal)&&p.outer_branch_notifications.capacity()>=23,
   "BNW branch notification native receipt/earned ordinal differs");
  p.outer_branch_notifications.push_back(h);p.previous=p.after;p.after=h.revision_after;
  // A real world FIFO is an emitted effect, not an acknowledgement from the
  // notification renderer/FX/minimap. Stop HERE, before original Add return,
  // later recipients and branch unlock, until a closed physical consumer exists.
  const bool publication=h.client_effects_emitted!=0;
  if(!publication)++p.outer_branch_notification_cursor;
  p.phase=BnwPolicyAdoptionPhase::uncertain;const auto host=p.host.lock();need(bool(host),"BNW branch notification Host retired");
  require_stored_current(*host);auto& binding=*p.hooks->impl_;if(p.after!=p.previous)binding.accept(*this);
  binding.lease.require_operation(binding.serial);require_stored_current(*host);(void)engine.require_outer_branch_notification_return(*this);
  p.phase=publication?BnwPolicyAdoptionPhase::policy_outer_branch_publication_pending:
   (p.outer_branch_notification_cursor==(p.outer_branch_credit_history->tenets?23:1)?BnwPolicyAdoptionPhase::policy_outer_branch_notifications_returned:BnwPolicyAdoptionPhase::policy_outer_branch_notifications_executing);
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
void BnwPolicyAdoptionPending::fail_outer_branch_notification_dispatch()const noexcept{
 if(creator_!=std::this_thread::get_id()){retired_.store(true);return;}if(impl_)impl_->phase=BnwPolicyAdoptionPhase::uncertain;
}
void BnwPolicyAdoptionPending::finish_outer_branch_notifications_without_publication()const{
 (void)phase();auto& p=*impl_;need(p.phase==BnwPolicyAdoptionPhase::policy_outer_branch_notifications_returned&&p.outer_branch_credit_history&&
  !p.outer_policy_branch_returned&&p.outer_branch_notification_cursor==(p.outer_branch_credit_history->tenets?23:1),"BNW original branch notification tail unfinished/publication pending");
 try{const auto host=p.host.lock();need(bool(host),"BNW branch unlock Host retired");require_stored_current(*host);auto& binding=*p.hooks->impl_;binding.lease.require_operation(binding.serial);
  for(const auto& h:p.outer_branch_notifications)need(h.client_effects_emitted==0,"BNW branch unlock mandatory notification publication not returned");
  auto& state=*p.state;auto* registry=state.setup.extensions.find<PolicyRegistry>();need(registry&&registry->players.contains(p.actor),"BNW branch unlock registry missing");
  const auto branch=p.outer_branch_credit_history->branch;need(!p.expected.unlocked.at(branch),"BNW branch unlock already stored");
  const auto* info=host->types().find(p.rules.policies.at(p.target).branch);need(info&&info->id==branch&&info->table=="PolicyBranchTypes","BNW branch unlock Info identity differs");
  auto desired=p.expected;desired.unlocked.at(branch)=true;auto next=*registry;next.players.at(p.actor)=desired;auto expected=desired;
  auto staged=named(named(raw_player(state,p.actor).strategy.blocks,"policies").hashes,"branch_unlocked");set_flag(staged,info->hash,true);
  const auto before=p.after;need(before<UINT64_MAX,"BNW branch unlock revision exhausted");
  require_stored_current(*host);binding.lease.require_operation(binding.serial);
  // Re-resolve raw objects after callbackful bookends. No saved row pointer
  // is dereferenced after a provider could replace underlying storage.
  auto& raw=named(named(raw_player(state,p.actor).strategy.blocks,"policies").hashes,"branch_unlocked");
  need(!flags(raw,p.rules.branch_hashes).at(branch)&&registry==state.setup.extensions.find<PolicyRegistry>()&&state.ui_source_revision==before,"BNW branch unlock final raw/owned pair differs");
  raw.rows.swap(staged.rows);raw.wire_count=staged.wire_count;*registry=std::move(next);p.expected=std::move(expected);p.previous=before;p.after=before+1;state.ui_source_revision=p.after;p.phase=BnwPolicyAdoptionPhase::uncertain;
  require_stored_current(*host);binding.accept(*this);binding.lease.require_operation(binding.serial);require_stored_current(*host);
  p.outer_policy_branch_returned=true;p.phase=BnwPolicyAdoptionPhase::policy_outer_dirty_pending;
 }catch(...){p.phase=BnwPolicyAdoptionPhase::uncertain;throw;}
}
}
