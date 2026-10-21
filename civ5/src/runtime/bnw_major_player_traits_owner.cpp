#include "civ5/runtime/bnw_major_player_traits_owner.hpp"
#include "civ5/runtime/bnw_policy_adoption.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include <algorithm>
#include <atomic>
#include <any>
#include <functional>
#include <typeindex>
#include <climits>
#include <map>
#include <set>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw BnwMajorPlayerTraitsUnavailable(why);}
std::string_view name(const char* p){return p?std::string_view(p):std::string_view();}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"BNW trait operand exceeds native int");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)+b);}
template<class C>typename C::value_type& mutable_named(C& c,std::string_view key){
 typename C::value_type* result=nullptr;
 for(auto& v:c)if(name(v.name)==key){need(!result,"Duplicate BNW mutable trait source member");result=&v;}
 need(result,"Missing BNW mutable trait source member");return *result;
}
template<class C>const typename C::value_type& named(const C& c,std::string_view key){
 const typename C::value_type* result=nullptr;
 for(const auto& v:c)if(name(v.name)==key){need(!result,"Duplicate BNW trait source member");result=&v;}
 need(result,"Missing BNW trait source member");return *result;
}
template<class V>bool same_named(const V& a,const V& b){
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i)if(name(a[i].name)!=name(b[i].name)||a[i].value!=b[i].value)return false;
 return true;
}
bool same_arrays(const auto& a,const auto& b){
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i)if(name(a[i].name)!=name(b[i].name)||a[i].columns!=b[i].columns||a[i].values!=b[i].values)return false;
 return true;
}
bool same_hashes(const auto& a,const auto& b){
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i){if(name(a[i].name)!=name(b[i].name)||a[i].wire_count!=b[i].wire_count||a[i].rows.size()!=b[i].rows.size())return false;
  for(std::size_t j=0;j<a[i].rows.size();++j)if(a[i].rows[j].type_hash!=b[i].rows[j].type_hash||a[i].rows[j].values!=b[i].rows[j].values)return false;}
 return true;
}
bool same_record(const saves::PregameRecord& a,const saves::PregameRecord& b){
 return a.integers==b.integers&&a.flags==b.flags&&a.float_bits==b.float_bits&&a.strings==b.strings&&
 a.integer_arrays==b.integer_arrays&&a.flag_arrays==b.flag_arrays&&a.string_arrays==b.string_arrays;
}
bool same_traits(const saves::PlayerTraitsObservation& a,const saves::PlayerTraitsObservation& b){
 if(a.layout!=b.layout||a.version!=b.version||a.dimensions.yields!=b.dimensions.yields||a.dimensions.terrains!=b.dimensions.terrains||a.fields.size()!=b.fields.size())return false;
 for(std::size_t i=0;i<a.fields.size();++i){const auto& x=a.fields[i];const auto& y=b.fields[i];
  if(name(x.name)!=name(y.name)||x.encoding!=y.encoding||x.columns!=y.columns||x.wire_count!=y.wire_count||x.hashes!=y.hashes||x.values!=y.values||x.rows.size()!=y.rows.size())return false;
  for(std::size_t j=0;j<x.rows.size();++j)if(x.rows[j].key!=y.rows[j].key||x.rows[j].wire_count!=y.rows[j].wire_count||x.rows[j].values!=y.rows[j].values)return false;}
 return true;
}
bool same_team(const saves::TeamObservation& a,const saves::TeamObservation& b){
 return a.index==b.index&&same_named(a.fields,b.fields)&&same_arrays(a.arrays,b.arrays)&&same_hashes(a.hashes,b.hashes);
}
bool same_policy(const saves::PlayerStrategyBlock& a,const saves::PlayerStrategyBlock& b){
 return name(a.name)==name(b.name)&&same_named(a.fields,b.fields)&&same_arrays(a.arrays,b.arrays)&&same_hashes(a.hashes,b.hashes);
}

struct TraitMirrors {
 std::int64_t cities=0,population=0,gold_per_partner=0,land=0,naval=0;
 YieldValues combat;
};
// Exact projections already used by restore_bnw_unhappiness, restore_treasury
// and restore_maintenance. No new happiness/maintenance/income formula.
TraitMirrors mirror_values(const saves::PlayerTraitsObservation& traits){
 need(traits.layout==bnw&&traits.version==19,"BNW native trait mirror source unavailable");
 const auto scalar=[&](std::string_view key){const auto& f=named(traits.fields,key);
  need(f.encoding==saves::PlayerTraitsEncoding::i32&&f.values.size()==1,"BNW trait mirror native scalar shape invalid");return native(f.values[0]);};
 TraitMirrors result;result.cities=scalar("m_iCityUnhappinessModifier");
 result.population=scalar("m_iPopulationUnhappinessModifier");
 result.land=scalar("m_iLandUnitMaintenanceModifier");result.naval=scalar("m_iNavalUnitMaintenanceModifier");
 const auto& trade=named(traits.fields,"m_iYieldChangePerTradePartner");
 need(trade.encoding==saves::PlayerTraitsEncoding::yield_array&&trade.values.size()==6,
  "BNW trade-partner trait mirror yield span unavailable");result.gold_per_partner=native(trade.values[2]);
 const auto& combat=named(traits.fields,"m_paiMaintenanceModifierUnitCombat");
 const auto& moves=named(traits.fields,"m_paiMovesChangeUnitCombat");
 need(combat.encoding==saves::PlayerTraitsEncoding::shared_combat_i32&&combat.columns==1&&!combat.wire_count&&combat.values.size()<=INT32_MAX&&
  moves.encoding==saves::PlayerTraitsEncoding::combat_i32&&moves.wire_count&&*moves.wire_count>=0&&
  static_cast<std::uint64_t>(*moves.wire_count)==moves.values.size()&&combat.values.size()==moves.values.size(),
  "BNW maintenance trait mirror native combat span unavailable");
 for(std::size_t i=0;i<combat.values.size();++i){const auto value=native(combat.values[i]);
  if(value)result.combat.emplace(static_cast<std::int64_t>(i),value);}
 return result;
}
bool same_mirrors(const LiveSessionState& state,std::int32_t actor,const TraitMirrors& expected){
 const auto u=state.unhappiness.players.find(actor);const auto m=state.maintenance.players.find(actor);
 const auto gold=state.treasury.players.find(actor);
 return state.maintenance.profile==content::TechnologyProfile::bnw&&state.treasury.profile==content::TechnologyProfile::bnw&&
  u!=state.unhappiness.players.end()&&m!=state.maintenance.players.end()&&gold!=state.treasury.players.end()&&
  u->second.trait_cities_modifier==expected.cities&&u->second.trait_population_modifier==expected.population&&
  gold->second.gold_per_partner==expected.gold_per_partner&&m->second.land_modifier==expected.land&&
  m->second.naval_modifier==expected.naval&&m->second.combat_modifiers==expected.combat;
}

struct Table {
 std::size_t span=0;std::map<std::int64_t,std::string> types;
};
using Tables=std::map<std::string,Table,std::less<>>;
Table load_table(const ui::SelectedBnwPreviewHost& host,const char* table){
 Table t;const auto report=host.database().prefetch_info(table);
 need(report.all_succeeded()&&!report.overlapping_ids&&report.rows&&report.slots<=4'000'000,
      "BNW trait indexed Info table is unavailable or exceeds work budget");
 t.span=static_cast<std::size_t>(report.slots);
 const auto q=host.database().query_rows({"SELECT ID, Type FROM "+std::string(table)+" ORDER BY ID",{}},[&](const auto& row){
  const auto id=row.integer_at(0);const auto type=row.text("Type");
  const auto* canonical=type?host.types().find(*type):nullptr;
  need(id>=0&&static_cast<std::uint64_t>(id)<report.slots&&canonical&&canonical->table==table&&
       canonical->id==id&&canonical->hash==content::saved_info_type_hash(*type)&&
       t.types.emplace(id,*type).second,"BNW trait Info Type/ID/hash remap is not genuine");
 });need(q.all_succeeded()&&t.types.size()==report.rows,"BNW trait indexed table read failed");
 // Native trait array helpers use MaxRows(max(rowid)+1); reject divergent spans.
 std::int64_t maxrow=-1;const auto r=host.database().query_rows({"SELECT MAX(rowid) FROM "+std::string(table),{}},[&](const auto& row){maxrow=row.integer_at(0);});
 need(r.all_succeeded()&&maxrow>=0&&static_cast<std::uint64_t>(maxrow)+1==report.slots,
      "BNW trait native MaxRows and indexed Info span differ");return t;
}
struct Trait {
 std::int64_t id=-1,obsolete=-1,prereq=-1,free_unit=-1,free_tech=-1;
 std::optional<std::int64_t> combat_improvement;
 std::string type;
 std::map<std::string,std::vector<std::int32_t>,std::less<>> values;
 std::vector<BnwTraitFreeResourceXCities> free_resources;
};
using Values=std::map<std::string,std::vector<std::int32_t>,std::less<>>;
struct Scalar {const char* field;const char* column;};
constexpr Scalar scalar_fields[]={
 {"m_iGreatPeopleRateModifier","GreatPeopleRateModifier"},
 {"m_iGreatScientistRateModifier","GreatScientistRateModifier"},
 {"m_iGreatGeneralRateModifier","GreatGeneralRateModifier"},
 {"m_iGreatGeneralExtraBonus","GreatGeneralExtraBonus"},
 {"m_iGreatPersonGiftInfluence","GreatPersonGiftInfluence"},
 {"m_iLevelExperienceModifier","LevelExperienceModifier"},
 {"m_iMaxGlobalBuildingProductionModifier","MaxGlobalBuildingProductionModifier"},
 {"m_iMaxTeamBuildingProductionModifier","MaxTeamBuildingProductionModifier"},
 {"m_iMaxPlayerBuildingProductionModifier","MaxPlayerBuildingProductionModifier"},
 {"m_iCityUnhappinessModifier","CityUnhappinessModifier"},
 {"m_iPopulationUnhappinessModifier","PopulationUnhappinessModifier"},
 {"m_iCityStateBonusModifier","CityStateBonusModifier"},
 {"m_iCityStateFriendshipModifier","CityStateFriendshipModifier"},
 {"m_iCityStateCombatModifier","CityStateCombatModifier"},
 {"m_iLandBarbarianConversionPercent","LandBarbarianConversionPercent"},
 {"m_iLandBarbarianConversionExtraUnits","LandBarbarianConversionExtraUnits"},
 {"m_iSeaBarbarianConversionPercent","SeaBarbarianConversionPercent"},
 {"m_iCapitalBuildingModifier","CapitalBuildingModifier"},
 {"m_iPlotBuyCostModifier","PlotBuyCostModifier"},
 {"m_iPlotCultureCostModifier","PlotCultureCostModifier"},
 {"m_iCultureFromKills","CultureFromKills"},
 {"m_iFaithFromKills","FaithFromKills"},
 {"m_iCityCultureBonus","CityCultureBonus"},
 {"m_iCapitalThemingBonusModifier","CapitalThemingBonusModifier"},
 {"m_iPolicyCostModifier","PolicyCostModifier"},
 {"m_iCityConnectionTradeRouteChange","CityConnectionTradeRouteChange"},
 {"m_iWonderProductionModifier","WonderProductionModifier"},
 {"m_iPlunderModifier","PlunderModifier"},
 {"m_iImprovementMaintenanceModifier","ImprovementMaintenanceModifier"},
 {"m_iGoldenAgeDurationModifier","GoldenAgeDurationModifier"},
 {"m_iGoldenAgeMoveChange","GoldenAgeMoveChange"},
 {"m_iGoldenAgeCombatModifier","GoldenAgeCombatModifier"},
 {"m_iGoldenAgeTourismModifier","GoldenAgeTourismModifier"},
 {"m_iGoldenAgeGreatArtistRateModifier","GoldenAgeGreatArtistRateModifier"},
 {"m_iGoldenAgeGreatMusicianRateModifier","GoldenAgeGreatMusicianRateModifier"},
 {"m_iGoldenAgeGreatWriterRateModifier","GoldenAgeGreatWriterRateModifier"},
 {"m_iExtraEmbarkMoves","ExtraEmbarkMoves"},
 {"m_iNaturalWonderFirstFinderGold","NaturalWonderFirstFinderGold"},
 {"m_iNaturalWonderSubsequentFinderGold","NaturalWonderSubsequentFinderGold"},
 {"m_iNaturalWonderYieldModifier","NaturalWonderYieldModifier"},
 {"m_iNaturalWonderHappinessModifier","NaturalWonderHappinessModifier"},
 {"m_iNearbyImprovementCombatBonus","NearbyImprovementCombatBonus"},
 {"m_iNearbyImprovementBonusRange","NearbyImprovementBonusRange"},
 {"m_iCultureBuildingYieldChange","CultureBuildingYieldChange"},
 {"m_iCombatBonusVsHigherTech","CombatBonusVsHigherTech"},
 {"m_iCombatBonusVsLargerCiv","CombatBonusVsLargerCiv"},
 {"m_iLandUnitMaintenanceModifier","LandUnitMaintenanceModifier"},
 {"m_iNavalUnitMaintenanceModifier","NavalUnitMaintenanceModifier"},
 {"m_iRazeSpeedModifier","RazeSpeedModifier"},
 {"m_iDOFGreatPersonModifier","DOFGreatPersonModifier"},
 {"m_iLuxuryHappinessRetention","LuxuryHappinessRetention"},
 {"m_iExtraSpies","ExtraSpies"},
 {"m_iUnresearchedTechBonusFromKills","UnresearchedTechBonusFromKills"},
 {"m_iExtraFoundedCityTerritoryClaimRange","ExtraFoundedCityTerritoryClaimRange"},
 {"m_iFreeSocialPoliciesPerEra","FreeSocialPoliciesPerEra"},
 {"m_iNumTradeRoutesModifier","NumTradeRoutesModifier"},
 {"m_iTradeRouteResourceModifier","TradeRouteResourceModifier"},
 {"m_iUniqueLuxuryCities","UniqueLuxuryCities"},
 {"m_iUniqueLuxuryQuantity","UniqueLuxuryQuantity"},
 {"m_iWorkerSpeedModifier","WorkerSpeedModifier"},
 {"m_iAfraidMinorPerTurnInfluence","AfraidMinorPerTurnInfluence"},
 {"m_iLandTradeRouteRangeBonus","LandTradeRouteRangeBonus"},
 {"m_iTradeReligionModifier","TradeReligionModifier"},
 {"m_iTradeBuildingModifier","TradeBuildingModifier"},
};
constexpr Scalar boolean_fields[]={
 {"m_bFightWellDamaged","FightWellDamaged"},
 {"m_bMoveFriendlyWoodsAsRoad","MoveFriendlyWoodsAsRoad"},
 {"m_bFasterAlongRiver","FasterAlongRiver"},
 {"m_bFasterInHills","FasterInHills"},
 {"m_bEmbarkedAllWater","EmbarkedAllWater"},
 {"m_bEmbarkedToLandFlatCost","EmbarkedToLandFlatCost"},
 {"m_bNoHillsImprovementMaintenance","NoHillsImprovementMaintenance"},
 {"m_bTechBoostFromCapitalScienceBuildings","TechBoostFromCapitalScienceBuildings"},
 {"m_bStaysAliveZeroCities","StaysAliveZeroCities"},
 {"m_bFaithFromUnimprovedForest","FaithFromUnimprovedForest"},
 {"m_bBonusReligiousBelief","BonusReligiousBelief"},
 {"m_bAbleToAnnexCityStates","AbleToAnnexCityStates"},
 {"m_bCrossesMountainsAfterGreatGeneral","CrossesMountainsAfterGreatGeneral"},
 {"m_bMayaCalendarBonuses","MayaCalendarBonuses"},
 {"m_bNoAnnexing","NoAnnexing"},
 {"m_bTechFromCityConquer","TechFromCityConquer"},
 {"m_bUniqueLuxuryRequiresNewArea","UniqueLuxuryRequiresNewArea"},
 {"m_bRiverTradeRoad","RiverTradeRoad"},
 {"m_bAngerFreeIntrusionOfCityStates","AngerFreeIntrusionOfCityStates"},
};

struct ArraySpec{const char* field;const char* target;const char* relation;const char* ref;const char* value;};
constexpr ArraySpec array_specs[]={
 {"m_iExtraYieldThreshold","Yields","Trait_ExtraYieldThresholds","YieldType","Yield"},
 {"m_iFreeCityYield","Yields","Trait_YieldChanges","YieldType","Yield"},
 {"m_iYieldChangeStrategicResources","Yields","Trait_YieldChangesStrategicResources","YieldType","Yield"},
 {"m_iYieldChangeNaturalWonder","Yields","Trait_YieldChangesNaturalWonder","YieldType","Yield"},
 {"m_iYieldChangePerTradePartner","Yields","Trait_YieldChangesPerTradePartner","YieldType","Yield"},
 {"m_iYieldChangeIncomingTradeRoute","Yields","Trait_YieldChangesIncomingTradeRoute","YieldType","Yield"},
 {"m_iYieldRateModifier","Yields","Trait_YieldModifiers","YieldType","Yield"},
 {"m_iStrategicResourceQuantityModifier","Terrains","Trait_Terrains","TerrainType","StrategicResourceQuantityModifier"},
 {"m_aiResourceQuantityModifier","Resources","Trait_ResourceQuantityModifiers","ResourceType","ResourceQuantityModifier"},
 {"m_paiMovesChangeUnitCombat","UnitCombatInfos","Trait_MovesChangeUnitCombats","UnitCombatType","MovesChange"},
 {"m_paiMaintenanceModifierUnitCombat","UnitCombatInfos","Trait_MaintenanceModifierUnitCombats","UnitCombatType","MaintenanceModifier"}};
constexpr ArraySpec matrix_specs[]={
 {"m_ppaaiUnimprovedFeatureYieldChange","Features","Trait_UnimprovedFeatureYieldChanges","FeatureType","Yield"},
 {"m_ppaaiImprovementYieldChange","Improvements","Trait_ImprovementYieldChanges","ImprovementType","Yield"},
 {"m_ppaaiSpecialistYieldChange","Specialists","Trait_SpecialistYieldChanges","SpecialistType","Yield"}};
std::int64_t ref(const ui::SelectedBnwPreviewHost& h,const std::optional<std::string>& key,const char* table){
 if(!key)return -1;const auto* v=h.types().find(*key);
 need(v&&v->id>=0&&v->table==table&&v->hash==content::saved_info_type_hash(*key),"BNW trait symbolic reference unresolved");return v->id;
}
Trait load_trait(const ui::SelectedBnwPreviewHost& host,const Tables& tables,std::int64_t id,const std::string& type){
 Trait t;t.id=id;t.type=type;bool found=false;
 const auto q=host.database().query_rows({"SELECT * FROM Traits WHERE ID=?",{id}},[&](const auto& row){
  need(!found&&row.text("Type")==std::optional<std::string>(type),"BNW trait source row identity changed");found=true;
  for(const auto& f:scalar_fields){need(row.has_column(f.column),"BNW scalar trait column absent");t.values[f.field]={native(row.integer(f.column))};}
  for(const auto& f:boolean_fields){need(row.has_column(f.column),"BNW boolean trait column absent");t.values[f.field]={row.boolean(f.column)?1:0};}
  t.obsolete=ref(host,row.text("ObsoleteTech"),"Technologies");t.prereq=ref(host,row.text("PrereqTech"),"Technologies");
  t.free_unit=ref(host,row.text("FreeUnit"),"UnitClasses");t.free_tech=ref(host,row.text("FreeUnitPrereqTech"),"Technologies");
  // CvTraitEntry ctor does not initialize this member. CacheResults assigns
  // it only for non-NULL GetText. NULL is unknown, NOT NO_IMPROVEMENT or zero.
  if(const auto improvement=row.text("CombatBonusImprovement"))t.combat_improvement=ref(host,improvement,"Improvements");
 });need(q.all_succeeded()&&found,"BNW trait scalar cache source failed");
 for(const auto& spec:array_specs){
  const auto& target=tables.at(spec.target);auto& values=t.values[spec.field];values.assign(target.span,0);
  const auto sql="select "+std::string(spec.target)+".ID, "+spec.value+" from "+spec.relation+" inner join "+spec.target+" on "+spec.ref+" = "+spec.target+".Type where TraitType = ?";
  const auto read=host.database().query_rows({sql,{type}},[&](const auto& row){const auto slot=row.integer_at(0);
   need(target.types.contains(slot),"BNW trait relation target lacks genuine Info");values.at(static_cast<std::size_t>(slot))=native(row.integer_at(1));
  });need(read.all_succeeded(),"BNW trait array cache query failed");
 }
 for(const auto& spec:matrix_specs){
  const auto& target=tables.at(spec.target);auto& values=t.values[spec.field];values.assign(target.span*6,0);
  const auto sql="select "+std::string(spec.target)+".ID, Yields.ID, Yield from "+spec.relation+" inner join "+spec.target+" on "+spec.target+".Type = "+spec.ref+" inner join Yields on Yields.Type = YieldType where TraitType = ?";
  const auto read=host.database().query_rows({sql,{type}},[&](const auto& row){const auto slot=row.integer_at(0),yield=row.integer_at(1);
   need(target.types.contains(slot)&&tables.at("Yields").types.contains(yield)&&yield<6,"BNW trait matrix target remap invalid");
   values.at(static_cast<std::size_t>(slot)*6+static_cast<std::size_t>(yield))=native(row.integer_at(2));
  });need(read.all_succeeded(),"BNW trait matrix cache query failed");
 }
 auto& train=t.values["m_abNoTrain"];train.assign(tables.at("UnitClasses").span,0);
 const auto no_train=host.database().query_rows({"SELECT Traits.ID, UnitClasses.ID FROM Trait_NoTrain inner join Traits on Trait_NoTrain.TraitType = Traits.Type inner join UnitClasses on Trait_NoTrain.UnitClassType = UnitClasses.Type where TraitType = ?",{type}},[&](const auto& row){
  const auto unit=row.integer_at(1);need(row.integer_at(0)==id&&tables.at("UnitClasses").types.contains(unit),"BNW trait no-train relation remap invalid");train.at(static_cast<std::size_t>(unit))=1;
 });need(no_train.all_succeeded(),"BNW trait NoTrain cache query failed");
 t.free_resources.resize(tables.at("Resources").span);
 for(std::size_t i=0;i<t.free_resources.size();++i)t.free_resources[i].resource=static_cast<std::int32_t>(i);
 const auto resources=host.database().query_rows({"select Resources.ID as ResourceID, ResourceQuantity, NumCities from Trait_FreeResourceFirstXCities inner join Resources on Resources.Type = ResourceType where TraitType = ?",{type}},[&](const auto& row){
  const auto resource=row.integer_at(0);need(tables.at("Resources").types.contains(resource),"BNW trait free-resource remap invalid");
  auto& v=t.free_resources.at(static_cast<std::size_t>(resource));v.quantity=native(row.integer_at(1));v.num_cities=native(row.integer_at(2));
 });need(resources.all_succeeded(),"BNW trait FreeResourceFirstXCities cache query failed");return t;
}
// SDK CvUnit::GetNearbyImprovementModifier and installed BNW DLL0x10212720
// test the aggregate modifier BEFORE reading this enum. The sole stock Lua
// exposure returns that same guarded result, never the enum itself. Validate
// ALL selected Traits rows, including inactive/future rows, not just Gandhi.
// This proves cache irrelevance under current rules; no TraitEntry seed proof.
bool unknown_combat_cache_is_unobserved(const ui::SelectedBnwPreviewHost& host,
 const Table& table,const Values& cache){
 bool zero_null_rows=true;std::set<std::int64_t> seen;
 const auto q=host.database().query_rows({
  "SELECT ID, Type, CombatBonusImprovement, NearbyImprovementCombatBonus FROM Traits ORDER BY ID",{}},[&](const auto& row){
  const auto id=row.integer_at(0);const auto type=row.text("Type");
  need(type&&table.types.contains(id)&&table.types.at(id)==*type&&seen.insert(id).second,
   "BNW complete Traits zero-effect source identity changed");
  const auto* info=host.types().find(*type);
  need(info&&info->table=="Traits"&&info->id==id&&info->hash==content::saved_info_type_hash(*type),
   "BNW complete Traits zero-effect Type remap changed");
  if(!row.text("CombatBonusImprovement")&&native(row.integer("NearbyImprovementCombatBonus"))!=0)zero_null_rows=false;
 });
 need(q.all_succeeded()&&seen.size()==table.types.size(),"BNW complete Traits zero-effect proof unavailable");
 return zero_null_rows&&cache.at("m_iNearbyImprovementCombatBonus").at(0)==0;
}
Values reset(const Tables& tables){
 Values cache;
 for(const auto& v:scalar_fields)cache[v.field]={0};for(const auto& v:boolean_fields)cache[v.field]={0};
 cache["m_eCampGuardType"]={-1};cache["m_eCombatBonusImprovement"]={-1};
 for(const auto& v:array_specs)cache[v.field].assign(tables.at(v.target).span,0);
 for(const auto& v:matrix_specs)cache[v.field].assign(tables.at(v.target).span*6,0);
 cache["m_abNoTrain"].assign(tables.at("UnitClasses").span,0);cache["m_aFreeTraitUnits"]={};return cache;
}
void apply_trait(Values& cache,std::vector<BnwTraitFreeResourceXCities>& resources,const Trait& t,
 std::optional<std::int32_t>& combat_improvement,
 std::int32_t* wounded=nullptr,std::int32_t wound_delta=0,std::int32_t wound_floor=0){
 for(const auto& v:scalar_fields)cache.at(v.field)[0]=add(cache.at(v.field)[0],t.values.at(v.field)[0]);
 for(const auto& v:boolean_fields)if(t.values.at(v.field)[0]){
  cache.at(v.field)[0]=1;
  // Import restores the already serialized player value without replaying it.
  // A real Reset/Init effect performs EACH native Change -> Set -> lower clamp.
  if(std::string_view(v.field)=="m_bFightWellDamaged"&&wounded)
   *wounded=std::max(add(*wounded,wound_delta),wound_floor);
 }
 // Exact source ordering and semantics: threshold max, six last-trait
 // assignments, positive-only matrix sums; not generic yield accumulation.
 for(std::size_t y=0;y<6;++y){
  cache.at("m_iExtraYieldThreshold")[y]=std::max(cache.at("m_iExtraYieldThreshold")[y],t.values.at("m_iExtraYieldThreshold")[y]);
  for(const auto& v:array_specs)if(std::string_view(v.target)=="Yields"&&std::string_view(v.field)!="m_iExtraYieldThreshold")cache.at(v.field)[y]=t.values.at(v.field)[y];
  for(const auto& v:matrix_specs)for(std::size_t row=0;row<cache.at(v.field).size()/6;++row){const auto i=row*6+y;const auto change=t.values.at(v.field)[i];if(change>0)cache.at(v.field)[i]=add(cache.at(v.field)[i],change);}
 }
 cache.at("m_iStrategicResourceQuantityModifier")=t.values.at("m_iStrategicResourceQuantityModifier");
 cache.at("m_aiResourceQuantityModifier")=t.values.at("m_aiResourceQuantityModifier");cache.at("m_abNoTrain")=t.values.at("m_abNoTrain");
 if(t.free_unit!=-1){cache.at("m_aFreeTraitUnits").push_back(native(t.free_unit));cache.at("m_aFreeTraitUnits").push_back(native(t.free_tech));}
 // Last active native assignment wins. Keep unknownness explicit through
 // the staging loop; no partially reconstructed cache is ever published.
 if(t.combat_improvement){combat_improvement=native(*t.combat_improvement);cache.at("m_eCombatBonusImprovement")[0]=*combat_improvement;}
 else combat_improvement.reset();
 for(std::size_t j=0;j<cache.at("m_paiMovesChangeUnitCombat").size();++j){
  cache.at("m_paiMovesChangeUnitCombat")[j]=add(cache.at("m_paiMovesChangeUnitCombat")[j],t.values.at("m_paiMovesChangeUnitCombat")[j]);
  cache.at("m_paiMaintenanceModifierUnitCombat")[j]=add(cache.at("m_paiMaintenanceModifierUnitCombat")[j],t.values.at("m_paiMaintenanceModifierUnitCombat")[j]);
 }
 for(std::size_t i=0;i<resources.size();++i)if(t.free_resources[i].quantity>0)resources[i]=t.free_resources[i];
}
bool history(std::string_view key){return key=="m_iBaktunPreviousTurn"||key=="m_aMayaBonusChoices"||key=="m_iUniqueLuxuryCitiesPlaced"||key=="m_aUniqueLuxuryAreas";}
void verify_saved(const saves::PlayerTraitsObservation& saved,const Values& cache,const ui::SelectedBnwPreviewHost& host,const Tables& tables){
 need(saved.layout==bnw&&saved.version==19&&saved.dimensions.yields==6,"Full BNW trait runtime requires current v19 saved cache");
 std::set<std::string_view> visited;
 for(const auto& f:saved.fields){const auto key=name(f.name);need(!key.empty()&&visited.insert(key).second,"BNW saved trait field missing or duplicate");if(history(key))continue;
  const auto it=cache.find(key);need(it!=cache.end(),"BNW saved trait cache has an unsupported field");const auto& expected=it->second;
  using E=saves::PlayerTraitsEncoding;
  if(f.encoding==E::hashed_i32){
   const char* table=key=="m_iStrategicResourceQuantityModifier"?"Terrains":"Resources";
   need((key=="m_iStrategicResourceQuantityModifier"||key=="m_aiResourceQuantityModifier")&&f.hashes.size()==expected.size(),"BNW saved trait hash width invalid");
   std::set<std::int64_t> seen;std::size_t value=0;
   for(const auto hash:f.hashes)if(hash){const auto* type=host.types().find_hash(hash);
    need(type&&type->table==table&&type->id>=0&&type->hash==hash&&content::saved_info_type_hash(type->type)==hash&&tables.at(table).types.contains(type->id)&&seen.insert(type->id).second&&value<f.values.size(),"BNW saved trait hash remap unresolved or duplicate");
    need(f.values[value++]==expected.at(static_cast<std::size_t>(type->id)),"Saved BNW hashed trait cache differs from actual Reset/Init");
   }need(value==f.values.size()&&seen.size()==tables.at(table).types.size(),"BNW saved trait hashed cache is incomplete");
  }else{
   if(f.values.size()!=expected.size())throw BnwMajorPlayerTraitsUnavailable(
    "Saved BNW trait cache differs from actual Reset/Init: field="+std::string(key)+
    " saved_count="+std::to_string(f.values.size())+" native_count="+std::to_string(expected.size())+
    " source_offset="+std::to_string(f.start_offset));
   for(std::size_t i=0;i<expected.size();++i)if(f.values[i]!=expected[i])throw BnwMajorPlayerTraitsUnavailable(
    "Saved BNW trait cache differs from actual Reset/Init: field="+std::string(key)+
    " index="+std::to_string(i)+" saved="+std::to_string(f.values[i])+" native="+std::to_string(expected[i])+
    " source_offset="+std::to_string(f.start_offset));
   if(f.encoding==E::counted_bool||f.encoding==E::combat_i32||f.encoding==E::yield_rows||f.encoding==E::counted_pairs)
    need(f.wire_count&&*f.wire_count>=0&&f.columns>0&&static_cast<std::uint64_t>(*f.wire_count)==expected.size()/f.columns,"BNW saved trait cache count differs from native span");
  }
 }
 for(const auto& [key,value]:cache){(void)value;need(visited.contains(key),"BNW saved trait cache missing current native member");}
}

// A finite operand stream, not a world clone. The same traversal records the
// baseline and compares it without allocating after all SQL/math/registry work.
// Cached positive happiness and public opinion are native getter operands;
// neither is represented here as a completed DoUpdateHappiness/Lua effect.
template<class M>const auto& happiness_key(const M& m,std::string_view key){
 const typename M::mapped_type* value=nullptr;
 for(const auto& [k,v]:m)if(std::string_view(k)==key){need(!value,"Duplicate happiness source key");value=&v;}
 need(value,"Missing happiness source key");return *value;
}
template<class Sink>void happiness_inputs(const LiveSessionState& s,Sink&& emit){
 const auto sequence=[&](const auto& values){emit(values.size());for(const auto value:values)emit(value);};
 const auto scalar_field=[&](const auto& fields,std::string_view key){const auto& f=named(fields,key);
  emit(static_cast<std::uint64_t>(f.encoding));emit(f.values.size());for(const auto value:f.values)emit(value);
  emit(f.wire_count.has_value());if(f.wire_count)emit(*f.wire_count);
  sequence(f.row_counts);sequence(f.hashes);sequence(f.bytes);
 };
 need(s.world.pregame&&s.world.all_players,"Happiness complete native source missing");
 emit(static_cast<std::uint64_t>(s.world.game_state.layout));
 const auto& pg=*s.world.pregame;emit(static_cast<std::uint64_t>(pg.layout));
 emit(happiness_key(pg.world.values.integers,"num_cities_unhappiness_percent"));
 emit(happiness_key(pg.archive.integers,"game_type"));emit(named(s.world.game_state.controls,"handicap").value);
 for(const auto key:{"team_types","handicaps","slot_status","slot_claims"})sequence(happiness_key(pg.archive.integer_arrays,key));
 emit(pg.game_options.size());for(const auto& option:pg.game_options){emit(option.name.size());for(const char v:option.name)emit(static_cast<unsigned char>(v));emit(option.value);}
 emit(static_cast<std::uint64_t>(s.setup.profile));emit(s.setup.game_type);emit(s.setup.game_handicap);
 emit(s.setup.pitboss.has_value());if(s.setup.pitboss)emit(*s.setup.pitboss);
 emit(s.setup.players.size());for(const auto& [id,p]:s.setup.players){emit(id);emit(p.team);emit(p.handicap);emit(p.slot_status);emit(p.slot_claim);emit(p.alive);}
 emit(static_cast<std::uint64_t>(s.cities.profile));emit(s.unhappiness.world_cities_percent);
 emit(s.unhappiness.players.size());for(const auto& [id,p]:s.unhappiness.players){
  emit(id);emit(p.happiness);emit(p.units);emit(p.units_modifier);emit(p.population_modifier);emit(p.cities_modifier);
  emit(p.occupied_modifier);emit(p.capital_modifier);emit(p.trait_cities_modifier);emit(p.trait_population_modifier);emit(p.public_opinion);emit(p.half_specialists);
  emit(p.cities.size());for(const auto& [city,a]:p.cities){emit(city);emit(a.occupied);emit(a.no_occupied_unhappiness);emit(a.ignore);
   emit(a.specialists.size());for(const auto& [type,count]:a.specialists){emit(type);emit(count);}}
 }
 emit(s.cities.players.size());for(const auto& [id,p]:s.cities.players){
  emit(id);emit(p.capital);emit(p.excess_happiness.has_value());if(p.excess_happiness)emit(*p.excess_happiness);
  emit(p.cities.size());for(const auto& [city,a]:p.cities){emit(city);emit(a.owner);emit(a.id);emit(a.population);emit(a.puppet);
   emit(a.source_core.has_value());if(a.source_core){emit(a.source_core->first);emit(a.source_core->second);}}
 }
 const auto& all=*s.world.all_players;emit(static_cast<std::uint64_t>(all.layout));emit(all.players.size());
 for(const auto& p:all.players){emit(p.index);emit(p.saved_player_id);emit(static_cast<std::uint64_t>(p.core.layout));
  for(const auto key:{"m_eID","m_bAlive","m_bEverAlive","m_iHappiness","m_iUnhappinessFromUnits","m_iUnhappinessFromUnitsMod",
    "m_iUnhappinessMod","m_iCityCountUnhappinessMod","m_iOccupiedPopulationUnhappinessMod","m_iCapitalUnhappinessMod","m_iHalfSpecialistUnhappinessCount"})scalar_field(p.core.fields,key);
  emit(static_cast<std::uint64_t>(p.traits.layout));for(const auto key:{"m_iCityUnhappinessModifier","m_iPopulationUnhappinessModifier"}){
   const auto& f=named(p.traits.fields,key);emit(static_cast<std::uint64_t>(f.encoding));sequence(f.values);}
  emit(static_cast<std::uint64_t>(p.trade_culture.layout));const auto& culture=named(p.trade_culture.blocks,"player_culture");
  const auto version=named(culture.fields,"version").value;emit(version);if(version>=3)emit(named(culture.fields,"m_iOpinionUnhappiness").value);
  const auto& cities=p.cities;emit(static_cast<std::uint64_t>(cities.layout));emit(static_cast<std::uint64_t>(cities.header.layout));
  emit(cities.header.wire_city_count);emit(cities.header.source_slots);emit(cities.header.fields.size());for(const auto& f:cities.header.fields){
   emit(name(f.name).size());for(const char v:name(f.name))emit(static_cast<unsigned char>(v));emit(f.value);}
  emit(cities.cities.size());for(const auto& a:cities.cities){emit(static_cast<std::uint64_t>(a.core.layout));emit(a.core.start_offset);emit(a.core.end_offset);
   for(const auto key:{"m_iID","m_eOwner","m_iPopulation","m_bPuppet","m_bOccupied","m_iNoOccupiedUnhappinessCount"})scalar_field(a.core.fields,key);
   emit(static_cast<std::uint64_t>(a.management.layout));const auto& citizens=named(a.management.blocks,"citizens");
   const auto& counts=named(citizens.hashes,"m_aiSpecialistCounts");emit(counts.wire_count);emit(counts.rows.size());
   for(const auto& row:counts.rows){emit(row.type_hash);sequence(row.values);}
  }
 }
}
struct HappinessSeal {
 std::vector<std::uint64_t> values;
 explicit HappinessSeal(const LiveSessionState& s){happiness_inputs(s,[&](auto v){values.push_back(static_cast<std::uint64_t>(v));});}
 void require_same(const LiveSessionState& s)const{
  std::size_t at=0;happiness_inputs(s,[&](auto v){need(at<values.size()&&values[at++]==static_cast<std::uint64_t>(v),"Happiness relevant source/owned operands changed during preparation");});
  need(at==values.size(),"Happiness operand stream changed size");
 }
};
void happiness_source_inputs(const LiveSessionState& s,const content::RulesDatabase& db,
 const BnwUnhappinessRules& rules,const content::InfoTypeCatalog& types){
 // One finite raw->owned restoration per operation, never a LiveSessionState
 // rebuild or clone. Runtime ignore-city flags remain the actual live values.
 const auto setup=restore_research_context(s.world,db);
 need(setup.profile==s.setup.profile&&setup.game_type==s.setup.game_type&&setup.game_handicap==s.setup.game_handicap&&
      setup.pitboss==s.setup.pitboss&&setup.players.size()==s.setup.players.size(),"Happiness native setup projection stale");
 for(const auto& [id,p]:setup.players){const auto& live=s.setup.players.at(id);need(p.team==live.team&&p.handicap==live.handicap&&
  p.slot_status==live.slot_status&&p.slot_claim==live.slot_claim&&p.alive==live.alive,"Happiness native player setup projection stale");}
 const auto expected=restore_bnw_unhappiness(s.world,s.setup,s.cities,rules,types);
 need(expected.world_cities_percent==s.unhappiness.world_cities_percent&&expected.players.size()==s.unhappiness.players.size(),"Happiness native world/player projection stale");
 for(const auto& [id,p]:expected.players){const auto& live=s.unhappiness.players.at(id);
  need(p.happiness==live.happiness&&p.units==live.units&&p.units_modifier==live.units_modifier&&p.population_modifier==live.population_modifier&&
   p.cities_modifier==live.cities_modifier&&p.occupied_modifier==live.occupied_modifier&&p.capital_modifier==live.capital_modifier&&
   p.trait_cities_modifier==live.trait_cities_modifier&&p.trait_population_modifier==live.trait_population_modifier&&p.public_opinion==live.public_opinion&&
   p.half_specialists==live.half_specialists&&p.cities.size()==live.cities.size(),"Happiness cached native GetHappiness/GetUnhappiness projection stale");
  for(const auto& [city,c]:p.cities){const auto& a=live.cities.at(city);need(c.occupied==a.occupied&&c.no_occupied_unhappiness==a.no_occupied_unhappiness&&c.specialists==a.specialists,
    "Happiness native city unhappiness projection stale");
   const auto& owned=s.cities.players.at(id).cities.at(city);
   const auto* raw=resolve_bnw_city_registry(s.world,s.cities,native(id),native(city));need(raw,"Happiness native city registry missing");
   const auto& population=named(raw->core.fields,"m_iPopulation").values;
   const auto& puppet=named(raw->core.fields,"m_bPuppet").values;
   need(population.size()==1&&population[0]==owned.population&&puppet.size()==1&&puppet[0]==(owned.puppet?1:0),"Happiness native population/puppet projection stale");
  }
 }
}

} // namespace
struct BnwMajorPlayerTraitsOwner::Impl {
 struct RuntimeCache {
  std::weak_ptr<const LiveSessionState> state;
  std::uint64_t generation=0;std::string fingerprint;
  BnwMajorPlayerTraitsSnapshot snapshot;
  std::optional<BnwMajorPolicyFreeUnitEffect> policy_free_unit;
 };
 struct RuntimeRegistry {std::map<std::int32_t,std::shared_ptr<const RuntimeCache>> players;};
 std::shared_ptr<const RuntimeCache> runtime;
 std::shared_ptr<LiveSessionState> writable;
 std::function<void()> facade_current;
 saves::PlayerCoreField wounded;TraitMirrors mirrors;
 bool applied=false,policy_query_applied=false;LiveSession* facade=nullptr;bool unhappiness_current=false;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> retired{false},busy{false};
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;const saves::SavedSession* source=nullptr;
 std::uint64_t generation=0,revision=0;std::string fingerprint;
 std::int32_t actor=-1;std::int64_t team=-1;PregameSlot slot;bool ever_alive=false;
 saves::PregameRecord slots,archive;std::uint64_t slots_version=0,archive_version=0;
 saves::PlayerTraitsObservation saved;
 saves::TeamObservation raw_team;
 std::unique_ptr<TeamTechnologies> owned_team;
 saves::PlayerStrategyBlock policy;std::set<std::int64_t> owned_policies;
 BnwMajorPlayerTraitsSnapshot cache;Values native_cache;
 const saves::SavedPlayerObservation& raw()const {
  need(state->world.all_players&&actor>=0&&static_cast<std::size_t>(actor)<state->world.all_players->players.size(),"BNW traits player roster disappeared");return state->world.all_players->players[static_cast<std::size_t>(actor)];
 }
 const TeamTechnologies& technologies()const{
  const TeamTechnologies* result=nullptr;for(const auto& t:state->technologies)if(t.team()==team){need(!result,"BNW traits duplicate owned team");result=&t;}
  need(result,"BNW traits actual owned team missing");return *result;
 }
 const saves::TeamObservation& raw_technologies()const{
  need(state->world.teams&&state->world.teams->layout==bnw&&state->world.teams->dimensions.profile_extensions.values.empty(),"BNW traits raw team profile missing");
  const saves::TeamObservation* result=nullptr;
  for(const auto& t:state->world.teams->teams)if(named(t.fields,"id").value==team){need(!result,"BNW traits duplicate raw team");result=&t;}
  need(result,"BNW traits actual raw team missing");return *result;
 }
 struct PolicyCostPostExpectation {std::int64_t before_cost=0,after_cost=0;std::uint64_t before_revision=0,after_revision=0;};
 void check(const saves::TeamObservation* post_raw=nullptr,const TeamTechnologies* post_owned=nullptr,const PolicyCostPostExpectation* policy_cost=nullptr)const{
  need(creator==std::this_thread::get_id(),"BNW trait owner foreign thread before source borrow");
  need(!retired.load()&&host&&state&&owned_team&&facade_current,"BNW trait lifecycle owner is retired");
  facade_current();const auto& session=host->session();
  need(facade==&session&&facade->bnw_unhappiness_current_==unhappiness_current,
   "BNW Traits actual mutable facade/query-cache guard changed");
  need(&session.source()==source&&session.read_state_lease()==state&&session.state_generation()==generation,"BNW trait SAME source allocation retired");
  need(host->database().gameplay_fingerprint_sha256()==fingerprint&&host->identity().rules_sha256==fingerprint,"BNW trait actual rules changed");
  const auto& w=state->world;need(w.pregame&&w.pregame->layout==bnw&&w.pregame->slots_version==slots_version&&w.pregame->archive_version==archive_version&&
   same_record(w.pregame->slots,slots)&&same_record(w.pregame->archive,archive),"BNW trait actual pregame source changed");
  const auto roster=host->pregame_roster().slots.find(actor);need(roster!=host->pregame_roster().slots.end()&&
   roster->second.leader==slot.leader&&roster->second.leader_key==slot.leader_key&&roster->second.civilization==slot.civilization&&
   roster->second.minor==slot.minor&&roster->second.status==slot.status,"BNW trait actual Pregame leader binding changed");
  need(state->setup.profile==content::TechnologyProfile::bnw&&state->player_research.major_players.contains(actor)&&
   state->player_research.players.contains(actor)&&!state->player_research.players.at(actor).pregame_minor,
   "BNW trait major-player/profile binding changed");
  const auto player=state->setup.players.find(actor);need(player!=state->setup.players.end()&&player->second.team==team&&
   player->second.ever_alive==ever_alive&&player->second.policies_resolved&&player->second.policies==owned_policies&&state->ui_source_revision==(policy_cost?policy_cost->after_revision:revision),
   "BNW trait current team/policy/revision binding changed");
  const auto& r=raw();const auto& id=named(r.core.fields,"m_eID");need(id.values.size()==1&&id.values[0]==actor,"BNW trait native player ID differs from facade");
  need(r.index==static_cast<std::uint64_t>(actor)&&r.saved_player_id==actor&&r.core.layout==bnw&&
   r.strategy.layout==bnw&&r.strategy.dimensions.profile_extensions.values.empty()&&same_traits(r.traits,saved)&&
   same_policy(named(r.strategy.blocks,"policies"),policy)&&same_team(raw_technologies(),post_raw?*post_raw:raw_team),"BNW trait raw operands changed in place");
  const auto& ever=named(r.core.fields,"m_bEverAlive");
  need(ever.encoding==saves::PlayerCoreEncoding::boolean&&ever.values.size()==1&&ever.values[0]==(ever_alive?1:0),
   "BNW Traits actual ever-alive actor binding changed");
  const auto& wfield=named(r.core.fields,"m_iWoundedUnitDamageMod");
  need(name(wfield.name)==name(wounded.name)&&wfield.encoding==wounded.encoding&&wfield.wire_count==wounded.wire_count&&
   wfield.values==wounded.values&&wfield.row_counts==wounded.row_counts&&wfield.hashes==wounded.hashes&&wfield.bytes==wounded.bytes,
   "BNW actual wound source operand changed");
  need(same_mirrors(*state,actor,mirrors),"BNW Traits persistent native mirrors changed in place");
  if(runtime){const auto* registry=state->setup.extensions.find<RuntimeRegistry>();
   need(registry&&registry->players.contains(actor)&&registry->players.at(actor)==runtime&&
    runtime->state.lock()==state&&runtime->generation==generation&&runtime->fingerprint==fingerprint,
    "BNW Traits genuine eighth owner disappeared or was replaced");
   if(policy_cost){const auto& cost=named(r.core.fields,"m_iPolicyCostModifier");
    need(revision==policy_cost->before_revision&&policy_cost->after_revision==policy_cost->before_revision+(policy_cost->before_cost!=policy_cost->after_cost?1:0)&&
     cost.encoding==saves::PlayerCoreEncoding::i32&&cost.values.size()==1&&cost.values[0]==policy_cost->after_cost&&
     (!runtime->policy_free_unit||runtime->policy_free_unit->after==policy_cost->before_cost),
     "BNW private policy diagnostic exact earned cost poststate differs");
   }else if(runtime->policy_free_unit){const auto& cost=named(r.core.fields,"m_iPolicyCostModifier");
    need(cost.encoding==saves::PlayerCoreEncoding::i32&&cost.values.size()==1&&cost.values[0]==runtime->policy_free_unit->after,
     "BNW actual paired policy scalar changed after native recompute");}
  }
  const auto& expected=post_owned?*post_owned:*owned_team;
  const auto& current=technologies();need(current.profile()==content::TechnologyProfile::bnw&&current.team()==expected.team()&&
   current.slots()==expected.slots()&&current.last_tech()==expected.last_tech()&&current.nondefault_values()==expected.nondefault_values()&&
   current.profile_extensions().values.empty(),"BNW trait actual owned technology poststate changed");
  try{(void)session.bnw_game_debug_mode();}catch(const std::exception&){
   throw BnwMajorPlayerTraitsUnavailable("BNW Traits actual Game debug owner retired");}
  // No allocating source traversal remains after the fingerprint preparation.
  facade_current();
  need(&session.source()==source&&session.read_state_lease()==state&&session.state_generation()==generation&&!retired.load(),"BNW trait lifecycle changed during final bookend");
 }
 struct Guard{std::shared_ptr<Impl> p;
  explicit Guard(std::shared_ptr<Impl> v):p(std::move(v)){
   need(p&&p->creator==std::this_thread::get_id(),"BNW trait owner foreign admission");
   if(p->busy.exchange(true)){p->retired.store(true);throw BnwMajorPlayerTraitsUnavailable("BNW trait owner reentry");}
  }~Guard(){p->busy.store(false);}
 };
};

struct BnwMajorPlayerTraitsOwner::PolicyCostObserverRetirement {
 std::shared_ptr<Impl> owner;std::shared_ptr<const BnwPolicyAdoptionPending> pending;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const Impl::RuntimeCache> replacement;
 Impl::PolicyCostPostExpectation post;
 mutable std::atomic<bool> consumed{false};
};
std::shared_ptr<const BnwMajorPlayerTraitsOwner::PolicyCostObserverRetirement>
BnwMajorPlayerTraitsOwner::prepare_policy_cost_observer_retirement(
 const std::shared_ptr<const BnwPolicyAdoptionPending>& supplied,std::int64_t before_cost,
 std::int64_t after_cost,std::uint64_t before_revision,std::uint64_t after_revision)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  p->check();const auto canonical=BnwPolicyAdoptionPending::retain_factory_owner(supplied);
  canonical->require_policy_cost_observer_source(*p->host,*p->state,p->actor,before_cost,after_cost,before_revision,after_revision,false);
  need(p->runtime&&p->revision==before_revision,"BNW policy cost observer lacks actual pre-store Traits runtime");
  auto lease=std::make_shared<PolicyCostObserverRetirement>();lease->owner=p;lease->pending=canonical;lease->host=p->host;
  lease->post={before_cost,after_cost,before_revision,after_revision};
  auto replacement=std::make_shared<Impl::RuntimeCache>(*p->runtime);
  // This is an obsolete diagnostic query receipt, NOT a native trait cache.
  // Preserve every native snapshot/resource/provenance and other observer.
  replacement->policy_free_unit.reset();lease->replacement=std::move(replacement);
  p->check();canonical->require_policy_cost_observer_source(*p->host,*p->state,p->actor,before_cost,after_cost,before_revision,after_revision,false);
  return lease;
 }catch(...){p->retired.store(true);throw;}
}
void BnwMajorPlayerTraitsOwner::accept_policy_cost_store(const std::shared_ptr<const PolicyCostObserverRetirement>& lease){
 need(lease&&lease->owner&&lease->pending&&lease->host&&lease->replacement,"BNW policy cost observer lease absent");
 const auto p=lease->owner;Impl::Guard guard(p);
 try{
  need(!lease->consumed.exchange(true)&&lease->host==p->host,"BNW policy cost observer retirement repeated/substituted");
  const auto canonical=BnwPolicyAdoptionPending::retain_factory_owner(lease->pending);need(canonical==lease->pending,"BNW policy cost observer factory allocation changed");
  const auto& e=lease->post;
  canonical->require_policy_cost_observer_source(*p->host,*p->state,p->actor,e.before_cost,e.after_cost,e.before_revision,e.after_revision,true);
  // Ordinary currentness is unchanged. This private earned post-expectation
  // admits only the exact paired cost/revision store, retaining all other
  // original pregame, raw Traits/policy/technology and native mirror checks.
  p->check(nullptr,nullptr,&e);
  auto* registry=p->writable->setup.extensions.find<Impl::RuntimeRegistry>();
  need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor)==p->runtime,
   "BNW policy cost diagnostic native registry changed");
  need(lease->replacement->state.lock()==p->state&&lease->replacement->generation==p->generation&&
   lease->replacement->fingerprint==p->fingerprint&&!lease->replacement->policy_free_unit,
   "BNW policy cost diagnostic prepared replacement differs");
  canonical->require_policy_cost_observer_source(*p->host,*p->state,p->actor,e.before_cost,e.after_cost,e.before_revision,e.after_revision,true);
  p->facade_current();
  // All allocations and source checks precede this one existing-pointer store.
  // Existing owners/quotes still point at old runtime and honestly become stale.
  registry->players.at(p->actor)=lease->replacement;p->retired.store(true);
 }catch(...){p->retired.store(true);throw;}
}

BnwMajorPlayerTraitsOwner::BnwMajorPlayerTraitsOwner(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorPlayerTraitsOwner::~BnwMajorPlayerTraitsOwner()=default;
std::shared_ptr<const BnwMajorPlayerTraitsOwner> BnwMajorPlayerTraitsOwner::from_selected_preview(
 const ui::SelectedBnwReadOnlyPreview& preview,std::int32_t actor){
 std::shared_ptr<const BnwMajorPlayerTraitsOwner> result;
 preview.with_owned_host_camera_input([&](auto actual,const auto*){result=from_source_host(std::move(actual),actor);});
 need(static_cast<bool>(result),"BNW trait genuine factory source did not run");return result;
}
std::shared_ptr<const BnwMajorPlayerTraitsOwner> BnwMajorPlayerTraitsOwner::from_source_host(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> actual,std::int32_t actor){
 std::shared_ptr<const BnwMajorPlayerTraitsOwner> result;
  need(actual&&actor>=0,"BNW traits genuine selected source unavailable");auto p=std::make_shared<Impl>();p->host=std::move(actual);p->actor=actor;
  const auto& session=p->host->session();
  p->facade=p->host->session_.get();
  need(p->facade==&session,"BNW Traits actual Host facade differs from immutable view");
  p->unhappiness_current=session.bnw_unhappiness_current_;
  const auto token=session.retain_bnw_facade_lifetime();
  p->facade_current=[token]{try{token->require_current();}catch(const std::exception&){
   throw BnwMajorPlayerTraitsUnavailable("BNW Traits genuine facade lifetime retired");}};p->facade_current();
  p->writable=session.state_;p->state=session.read_state_lease();p->source=&session.source();
  p->generation=session.state_generation();p->revision=p->state->ui_source_revision;p->fingerprint=p->host->identity().rules_sha256;
  const auto& w=p->state->world;need(w.pregame&&w.pregame->layout==bnw&&w.all_players&&w.all_players->layout==bnw&&
   p->state->setup.profile==content::TechnologyProfile::bnw&&p->state->player_research.major_players.contains(actor)&&
   !p->state->player_research.players.at(actor).pregame_minor,"BNW traits owner requires actual major player");
  need(p->host->types().report().all_succeeded()&&!p->host->types().report().hash_collisions&&
   !p->host->types().report().name_overwrites,"BNW trait canonical Type catalog ambiguous");
  const auto restored=restore_pregame_roster(*w.pregame,p->host->database());
  const auto binding=restored.slots.find(actor);need(binding!=restored.slots.end()&&!binding->second.minor&&binding->second.leader>=0&&
   !binding->second.leader_key.empty(),"BNW trait actual leader source unavailable");p->slot=binding->second;
  p->slots=w.pregame->slots;p->archive=w.pregame->archive;p->slots_version=w.pregame->slots_version;p->archive_version=w.pregame->archive_version;
  const auto& player=p->state->setup.players.at(actor);need(player.policies_resolved,"BNW trait actual policy membership unavailable");
  p->team=player.team;p->ever_alive=player.ever_alive;p->owned_policies=player.policies;p->policy=named(p->raw().strategy.blocks,"policies");
  const auto policy_rules=BnwPolicyHappinessRules::load(p->host->database());
  const auto membership=policy_rules.flags(named(p->policy.hashes,"has_policy"),false);
  (void)policy_rules.flags(named(p->policy.hashes,"branch_blocked"),true);
  std::set<std::int64_t> policies;for(std::size_t i=0;i<membership.size();++i)if(membership[i])policies.insert(static_cast<std::int64_t>(i));
  need(policies==player.policies&&policy_rules.rules_sha256()==p->fingerprint,"BNW trait raw/owned policy membership differs");
  p->raw_team=p->raw_technologies();p->owned_team=std::make_unique<TeamTechnologies>(p->technologies());
  const auto decoded=restore_team_technologies(p->raw_team,p->host->technologies(),p->host->types());
  need(decoded.profile()==p->owned_team->profile()&&decoded.team()==p->team&&decoded.slots()==p->owned_team->slots()&&
   decoded.last_tech()==p->owned_team->last_tech()&&decoded.nondefault_values()==p->owned_team->nondefault_values()&&
   decoded.profile_extensions().values.empty()&&decoded.remap().all_mapped()&&!decoded.remap().duplicates,"BNW trait actual raw/owned technology remap differs");
  p->saved=p->raw().traits;p->wounded=named(p->raw().core.fields,"m_iWoundedUnitDamageMod");
  p->mirrors=mirror_values(p->saved);
  need(p->wounded.encoding==saves::PlayerCoreEncoding::i32&&p->wounded.values.size()==1,
   "BNW actual serialized wounded modifier unavailable");(void)native(p->wounded.values[0]);p->check();
  Tables tables;for(const auto* t:{"Traits","Leaders","Yields","Terrains","Resources","UnitClasses","UnitCombatInfos","Features","Improvements","Specialists"})tables.emplace(t,load_table(*p->host,t));
  need(tables.at("Yields").span==6&&tables.at("Terrains").span==p->saved.dimensions.terrains&&
   tables.at("Leaders").types.contains(p->slot.leader)&&tables.at("Leaders").types.at(p->slot.leader)==p->slot.leader_key,
   "BNW trait native enum/leader span differs from source");
  std::set<std::int64_t> leader_traits;
  const auto leader=p->host->database().query_rows({"SELECT TraitType FROM Leader_Traits WHERE LeaderType=?",{p->slot.leader_key}},[&](const auto& row){
   const auto key=row.text("TraitType");const auto id=ref(*p->host,key,"Traits");
   need(id>=0&&tables.at("Traits").types.contains(id),"BNW trait actual leader relation unresolved");leader_traits.insert(id);
  });need(leader.all_succeeded(),"BNW trait actual leader relation query failed");
  // Real Reset storage, including the previously unsaved eighth container.
  // These are fresh native defaults belonging to this owner, never guesses
  // about the old serialized world's missing transient allocation.
  auto cache=reset(tables);std::vector<BnwTraitFreeResourceXCities> resources(tables.at("Resources").span);
  for(std::size_t i=0;i<resources.size();++i)resources[i].resource=static_cast<std::int32_t>(i);
  std::vector<std::int64_t> active;std::optional<std::int32_t> combat_improvement=-1;
  for(std::size_t i=0;i<tables.at("Traits").span;++i){const auto id=static_cast<std::int64_t>(i);if(!leader_traits.contains(id))continue;
   // HasTrait short-circuits leader membership before obsolete/prereq checks.
   const auto entry=load_trait(*p->host,tables,id,tables.at("Traits").types.at(id));
   if(entry.obsolete!=-1&&p->owned_team->has_tech(entry.obsolete))continue;
   if(entry.prereq!=-1&&!p->owned_team->has_tech(entry.prereq))continue;
   active.push_back(id);apply_trait(cache,resources,entry,combat_improvement);
  }
  const bool imported_opaque=!combat_improvement.has_value();
  if(imported_opaque){
   const auto&observed=named(p->saved.fields,"m_eCombatBonusImprovement");
   need(observed.encoding==saves::PlayerTraitsEncoding::i32&&observed.values.size()==1,
    "Authentic imported opaque native combat-improvement cache missing");
   // Exact serialized native player cache, not inferred Traits SQL default.
   // No native TraitEntry seed is created. Known domains remain independently
   // checked. Future Init can only canonicalize an observably irrelevant cache.
   cache.at("m_eCombatBonusImprovement")[0]=native(observed.values[0]);
  }
  verify_saved(p->saved,cache,*p->host,tables);
  BnwMajorPlayerTraitsSnapshot staged;staged.player=actor;staged.leader=p->slot.leader;staged.team=p->team;
  staged.active_traits=std::move(active);staged.cache=p->saved.fields;staged.free_resource_x_cities=std::move(resources);
  staged.combat_improvement_provenance=imported_opaque?BnwTraitCombatImprovementProvenance::imported_opaque_cache:BnwTraitCombatImprovementProvenance::native_info_value;
  if(imported_opaque&&cache.at("m_eCombatBonusImprovement")[0]==-1&&
    unknown_combat_cache_is_unobserved(*p->host,tables.at("Traits"),cache))
   staged.combat_improvement_provenance=BnwTraitCombatImprovementProvenance::canonical_unobserved_cache;
  // Only known native domains and authentic opaque cache are published. All
  // allocations precede actual source bookends and this nonthrowing move.
  p->check();static_assert(std::is_nothrow_move_assignable_v<BnwMajorPlayerTraitsSnapshot>);
  static_assert(std::is_nothrow_move_assignable_v<Values>);
  p->native_cache=std::move(cache);p->cache=std::move(staged);
  result=std::shared_ptr<const BnwMajorPlayerTraitsOwner>(new BnwMajorPlayerTraitsOwner(p));p->check();
  // State owns native resource storage; it retains neither Host nor Session.
  // The private typed registry is storage, not an external readiness receipt.
  const auto* prior=p->state->setup.extensions.find<Impl::RuntimeRegistry>();
  const std::optional<Impl::RuntimeRegistry> registry_before=prior?std::optional<Impl::RuntimeRegistry>(*prior):std::nullopt;
  Impl::RuntimeRegistry registry=registry_before?*registry_before:Impl::RuntimeRegistry{};
  auto runtime=std::make_shared<Impl::RuntimeCache>();runtime->state=p->state;
  runtime->generation=p->generation;runtime->fingerprint=p->fingerprint;runtime->snapshot=p->cache;
  const auto old_entry=registry.players.find(actor);
  if(old_entry!=registry.players.end()){
   need(old_entry->second&&old_entry->second->state.lock()==p->state&&old_entry->second->generation==p->generation&&
    old_entry->second->fingerprint==p->fingerprint&&old_entry->second->snapshot.active_traits==p->cache.active_traits&&
    old_entry->second->snapshot.combat_improvement_provenance==p->cache.combat_improvement_provenance&&
    old_entry->second->snapshot.free_resource_x_cities==p->cache.free_resource_x_cities,
    "BNW existing eighth runtime owner is stale");runtime.reset();p->runtime=old_entry->second;
  }else{
   registry.players.emplace(actor,runtime);
   std::map<std::type_index,std::any> staged;
   staged.emplace(std::type_index(typeid(Impl::RuntimeRegistry)),std::any(std::move(registry)));
   auto node=staged.extract(staged.begin());p->check();
   const auto* current_registry=p->state->setup.extensions.find<Impl::RuntimeRegistry>();
   need(bool(current_registry)==bool(registry_before)&&(!current_registry||current_registry->players==registry_before->players),
    "BNW genuine sibling runtime storage changed during factory preparation");
   auto& target=p->writable->setup.extensions.values;const auto existing=target.find(node.key());
   if(existing==target.end())target.insert(std::move(node));else existing->second.swap(node.mapped());
   p->runtime=std::move(runtime);
  }
  p->check();
 need(static_cast<bool>(result),"BNW trait genuine factory source did not run");return result;
}
void BnwMajorPlayerTraitsOwner::require_current()const{const auto p=impl_;Impl::Guard guard(p);try{p->check();}catch(...){p->retired.store(true);throw;}}
std::int32_t BnwMajorPlayerTraitsOwner::cache_value(std::string_view field,std::int64_t index)const{
 const auto p=impl_;Impl::Guard guard(p);try{p->check();const auto it=p->native_cache.find(field);
  need(it!=p->native_cache.end()&&index>=0&&static_cast<std::uint64_t>(index)<it->second.size(),"BNW trait native cache member/index unavailable");
  const auto value=it->second[static_cast<std::size_t>(index)];p->check();return value;
 }catch(...){p->retired.store(true);throw;}
}
BnwMajorPlayerTraitsSnapshot BnwMajorPlayerTraitsOwner::snapshot()const{
 const auto p=impl_;Impl::Guard guard(p);try{p->check();auto result=p->cache;p->check();return result;}catch(...){p->retired.store(true);throw;}
}

namespace {
std::int32_t actual_define(const ui::SelectedBnwPreviewHost& host,std::string_view key){
 const auto* value=host.defines().find(key);need(host.defines().profile()==content::GlobalDefineProfile::bnw&&value&&value->kind==content::GlobalDefineKind::integer&&
  value->lookup.found&&value->lookup.error.empty()&&value->lookup.sqlite_code==0&&!value->retained_previous&&
  std::holds_alternative<std::int64_t>(value->lookup.value),"BNW actual wounded define unavailable");
 const auto current=host.database().global_define(key,content::GlobalDefineKind::integer);
 need(current.found&&current.error.empty()&&current.sqlite_code==0&&current.value==value->lookup.value,
  "BNW current database wounded Define differs from real cached value");
 return native(std::get<std::int64_t>(value->lookup.value));
}
// Preserve actual native wire Type/hash positions and all historical members.
saves::PlayerTraitsObservation stage_serialized(const saves::PlayerTraitsObservation& old,
 const Values& cache,const ui::SelectedBnwPreviewHost& host,const Tables& tables){
 auto next=old;
 for(auto& field:next.fields){const auto key=name(field.name);if(history(key))continue;
  const auto& values=cache.at(std::string(key));
  if(field.encoding==saves::PlayerTraitsEncoding::hashed_i32){std::size_t index=0;
   for(const auto hash:field.hashes)if(hash){const auto* type=host.types().find_hash(hash);
    need(type&&type->id>=0&&index<field.values.size(),"BNW Traits staged hash remap unavailable");
    field.values[index++]=values.at(static_cast<std::size_t>(type->id));}
  }else{
   field.values.assign(values.begin(),values.end());
   if(field.encoding==saves::PlayerTraitsEncoding::counted_pairs){
    need(field.columns==2&&field.values.size()%2==0,"BNW staged free-unit pair shape invalid");
    field.wire_count=static_cast<std::int64_t>(field.values.size()/2);
   }
  }
 }
 verify_saved(next,cache,host,tables);return next;
}
}
BnwMajorPlayerTraitsResetEffect BnwMajorPlayerTraitsOwner::reset_init_after_technology_flag_transition(
 std::int64_t technology,std::int32_t change)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  need(!p->applied&&p->ever_alive&&(change==1||change==-1)&&technology>=0&&p->host->technologies().find(static_cast<std::uint64_t>(technology)),
   "BNW Traits isolated Reset/Init requires a current technology transition");
  p->facade_current();
  auto post_owned=std::make_unique<TeamTechnologies>(*p->owned_team);
  need(post_owned->has_tech(technology)!=(change==1),"BNW Traits transition owner was issued after the flag change");
  // This changes ONLY staged expectation storage, never the live HasTech bits.
  need(post_owned->set_has_tech(technology,change==1),"BNW Traits transition expectation did not change");
  auto post_raw=p->raw_team;
  auto& hashes=mutable_named(post_raw.arrays,"tech_hashes");
  auto& has=mutable_named(post_raw.arrays,"has_tech");
  need(hashes.values.size()==has.values.size(),"BNW technology flag wire span differs");bool found=false;
  for(std::size_t i=0;i<hashes.values.size();++i){const auto* type=p->host->types().find_hash(static_cast<std::uint64_t>(hashes.values[i]));
   if(type&&type->table=="Technologies"&&type->id==technology){need(!found,"BNW transition wire slot duplicate");found=true;has.values[i]=change==1?1:0;}}
  need(found,"BNW actual transition wire slot unavailable");
  if(change==1)mutable_named(post_raw.fields,"last_tech").value=technology;
  p->check(&post_raw,post_owned.get());
  const auto decoded=restore_team_technologies(p->raw_technologies(),p->host->technologies(),p->host->types());
  need(decoded.remap().all_mapped()&&!decoded.remap().duplicates&&decoded.nondefault_values()==post_owned->nondefault_values()&&
   decoded.last_tech()==post_owned->last_tech(),"BNW Traits actual paired HasTech poststate unresolved");
  Tables tables;for(const auto* key:{"Traits","Leaders","Yields","Terrains","Resources","UnitClasses","UnitCombatInfos","Features","Improvements","Specialists"})tables.emplace(key,load_table(*p->host,key));
  need(tables.at("Yields").span==6&&tables.at("Terrains").span==p->saved.dimensions.terrains,
   "BNW Traits poststate enum spans changed");
  std::set<std::int64_t> leader_traits;
  const auto query=p->host->database().query_rows({"SELECT TraitType FROM Leader_Traits WHERE LeaderType=?",{p->slot.leader_key}},[&](const auto& row){
   const auto id=ref(*p->host,row.text("TraitType"),"Traits");need(id>=0&&tables.at("Traits").types.contains(id),"BNW actual poststate leader trait unresolved");leader_traits.insert(id);
  });need(query.all_succeeded(),"BNW poststate leader trait query failed");
  std::optional<std::int32_t> combat_improvement=-1;
  auto cache=reset(tables);std::vector<BnwTraitFreeResourceXCities> resources(tables.at("Resources").span);
  for(std::size_t i=0;i<resources.size();++i)resources[i].resource=static_cast<std::int32_t>(i);
  const auto before=native(p->wounded.values[0]);auto wounded=before;
  std::optional<std::pair<std::int32_t,std::int32_t>> wounded_defines;
  std::vector<std::int64_t> active;
  for(std::size_t i=0;i<tables.at("Traits").span;++i){const auto id=static_cast<std::int64_t>(i);if(!leader_traits.contains(id))continue;
   const auto trait=load_trait(*p->host,tables,id,tables.at("Traits").types.at(id));
   if(trait.obsolete!=-1&&post_owned->has_tech(trait.obsolete))continue;
   if(trait.prereq!=-1&&!post_owned->has_tech(trait.prereq))continue;
   if(trait.values.at("m_bFightWellDamaged")[0]&&!wounded_defines){
    const auto delta=actual_define(*p->host,"TRAIT_WOUNDED_DAMAGE_MOD");
    const auto multiplier=actual_define(*p->host,"WOUNDED_DAMAGE_MULTIPLIER");
    need(multiplier>=0,"BNW wounded lower clamp is outside supported native domain");wounded_defines=std::pair{delta,-multiplier};
   }
   active.push_back(id);apply_trait(cache,resources,trait,combat_improvement,&wounded,
    wounded_defines?wounded_defines->first:0,wounded_defines?wounded_defines->second:0);
  }
  const bool canonical_unobserved=!combat_improvement.has_value();
  if(canonical_unobserved){
   need(unknown_combat_cache_is_unobserved(*p->host,tables.at("Traits"),cache),
    "Mandatory native TraitEntry CombatBonusImprovement seed unavailable for observable nonzero modifier; imported opaque cache is not a rule seed");
   // Only an internal unobserved cache differs. Native combat and the stock
   // Lua API return0 before inspecting this enum under the proved operands.
   // This is not the value of the uninitialized original TraitEntry field.
   cache.at("m_eCombatBonusImprovement")[0]=-1;
  }
  auto saved=stage_serialized(p->saved,cache,*p->host,tables);
  auto mirrors=mirror_values(saved);auto mirrors_baseline=mirrors;
  BnwMajorPlayerTraitsSnapshot snapshot;snapshot.player=p->actor;snapshot.team=p->team;snapshot.leader=p->slot.leader;
  snapshot.active_traits=std::move(active);snapshot.cache=saved.fields;snapshot.free_resource_x_cities=std::move(resources);
  snapshot.combat_improvement_provenance=canonical_unobserved?BnwTraitCombatImprovementProvenance::canonical_unobserved_cache:BnwTraitCombatImprovementProvenance::native_info_value;
  auto runtime=std::make_shared<Impl::RuntimeCache>();runtime->state=p->state;runtime->generation=p->generation;
  runtime->fingerprint=p->fingerprint;runtime->snapshot=snapshot;
  runtime->policy_free_unit=p->runtime?p->runtime->policy_free_unit:std::nullopt;
  const bool derived_changed=cache!=p->native_cache;
  auto saved_baseline=saved;auto wound_baseline=p->wounded;wound_baseline.values[0]=wounded;
  need(p->revision<UINT64_MAX,"BNW Traits UI revision overflow");
  BnwMajorPlayerTraitsResetEffect result{p->actor,before,wounded,technology,change};
  // All allocations/SQL reads precede SAME source, operands and genuine token
  // fences. The following stores have no callbacks and cannot allocate.
  p->check(&post_raw,post_owned.get());
  auto& actual=p->writable->world.all_players->players.at(static_cast<std::size_t>(p->actor));
  auto& actual_wound=mutable_named(actual.core.fields,"m_iWoundedUnitDamageMod");
  auto* registry=p->writable->setup.extensions.find<Impl::RuntimeRegistry>();
  need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor)==p->runtime,
   "BNW actual Traits native storage changed before stores");
  auto& unhappiness=p->writable->unhappiness.players.at(p->actor);
  auto& maintenance=p->writable->maintenance.players.at(p->actor);
  auto& treasury=p->writable->treasury.players.at(p->actor);
  need(same_mirrors(*p->state,p->actor,p->mirrors),"BNW paired mirrors changed immediately before stores");
  p->facade_current();
  static_assert(std::is_nothrow_move_assignable_v<TraitMirrors>);
  static_assert(noexcept(maintenance.combat_modifiers.swap(mirrors.combat)));
  static_assert(std::is_nothrow_move_assignable_v<saves::PlayerTraitsObservation>);
  static_assert(std::is_nothrow_move_assignable_v<saves::PlayerCoreField>);
  static_assert(std::is_nothrow_move_assignable_v<saves::TeamObservation>);
  // No history clears: Reset() does not reset Maya/luxury historical fields.
  actual.traits=std::move(saved);actual_wound.values[0]=wounded;
  unhappiness.trait_cities_modifier=mirrors.cities;unhappiness.trait_population_modifier=mirrors.population;
  treasury.gold_per_partner=mirrors.gold_per_partner;
  maintenance.land_modifier=mirrors.land;maintenance.naval_modifier=mirrors.naval;
  maintenance.combat_modifiers.swap(mirrors.combat);
  registry->players.at(p->actor)=runtime;
  // Positive happiness and plot/city cached yields need their actual later
  // update graph. Do not advertise stale excess-happiness inputs as ready.
  if(derived_changed){p->facade->bnw_unhappiness_current_=false;p->unhappiness_current=false;}
  ++p->writable->ui_source_revision;++p->revision;
  p->saved=std::move(saved_baseline);p->mirrors=std::move(mirrors_baseline);
  p->wounded=std::move(wound_baseline);p->raw_team=std::move(post_raw);p->owned_team=std::move(post_owned);
  p->native_cache=std::move(cache);p->cache=std::move(snapshot);p->runtime=std::move(runtime);p->applied=true;
  return result;
 }catch(...){p->retired.store(true);throw;}
}

std::map<std::int64_t,BnwUnhappinessQuote> BnwMajorPlayerTraitsOwner::refresh_cached_happiness_getter_projection()const{
 auto p=impl_;Impl::Guard guard(p);
 try{
  p->check();HappinessSeal seal(*p->state);
  const auto rules=load_bnw_unhappiness_rules(p->host->database());
  happiness_source_inputs(*p->state,p->host->database(),rules,p->host->types());
  std::map<std::int64_t,BnwUnhappinessQuote> results;
  std::vector<BnwCitySpecialistCounts> specialists;
  need(p->state->cities.players.size()==p->state->setup.players.size()&&p->state->cities.players.size()==p->state->unhappiness.players.size(),"Happiness native complete player roster required");
  for(const auto& [id,player]:p->state->cities.players){
   const auto& u=p->state->unhappiness.players.at(id);
   if(u.half_specialists&&!player.cities.empty()){
    specialists.push_back(BnwCitySpecialistCounts::from_same_host(p->host,id,player.cities.begin()->first));
    specialists.back().require_same_world_database(p->state->world,p->host->database());
    results.emplace(id,detail::bnw_native_unhappiness_math_with_specialists(p->state->unhappiness,p->state->setup,p->state->cities,rules,p->host->defines(),id,specialists.back()));
   }else results.emplace(id,detail::bnw_native_unhappiness_math(p->state->unhappiness,p->state->setup,p->state->cities,rules,p->host->defines(),id));
  }
  // Allocate/check every target and return value before any write. Public
  // snapshots/callers supply no mutable world and no completed-Lua receipt.
  std::vector<std::pair<std::optional<std::int64_t>*,std::int64_t>> targets;bool changed=!p->unhappiness_current;
  for(const auto& [id,q]:results){auto& output=p->writable->cities.players.at(id).excess_happiness;
   changed=changed||output!=std::optional<std::int64_t>{q.excess_happiness};targets.emplace_back(&output,q.excess_happiness);}
  if(changed)need(p->revision<UINT64_MAX,"Happiness projection revision overflow");
  for(const auto& source:specialists)source.require_current_source();
  p->check();seal.require_same(*p->state);p->facade_current();
  need(p->facade==&p->host->session()&&p->host->session().read_state_lease()==p->state&&
   p->host->session().state_generation()==p->generation&&!p->retired.load()&&p->state->ui_source_revision==p->revision&&
   p->facade->bnw_unhappiness_current_==p->unhappiness_current,
   "Happiness final SAMEstate allocation/revision bookend changed");
  // Callback-free, allocation-free SAMEstate commit. Existing current guard
  // means this getter cache, not DoUpdateHappiness/religion/technology readiness.
  static_assert(std::is_nothrow_assignable_v<std::optional<std::int64_t>&,std::int64_t>);
  static_assert(std::is_nothrow_move_constructible_v<decltype(results)>);
  for(const auto& [output,value]:targets)*output=value;
  p->facade->bnw_unhappiness_current_=true;p->unhappiness_current=true;
  if(changed){++p->writable->ui_source_revision;++p->revision;}
  return results;
 }catch(const BnwMajorPlayerTraitsUnavailable&){p->retired.store(true);throw;}
 catch(const std::exception& e){p->retired.store(true);throw BnwMajorPlayerTraitsUnavailable(e.what());}
 catch(...){p->retired.store(true);throw;}
}

} // namespace civ5::runtime

namespace civ5::runtime {
BnwMajorPlayerTraitsResetEffect BnwMajorPlayerTraitsOwner::reset_init_for_pending(
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,
 std::uint64_t generation,std::uint64_t before_revision,std::uint64_t after_revision,
 std::int64_t technology)const{
 const auto p=impl_;
 try{
  {
   Impl::Guard guard(p);
   need(host&&host.get()==p->host.get()&&!host.owner_before(p->host)&&!p->host.owner_before(host),
    "BNW Traits actual core canonical Host differs");
   p->facade_current();
   need(!p->retired.load()&&!p->applied&&p->generation==generation&&p->revision==before_revision&&
    after_revision>before_revision&&p->state->ui_source_revision==after_revision&&
    host->session().state_generation()==generation&&host->session().read_state_lease()==p->state,
    "BNW Traits genuine pre-flag owner/core revision transition differs");
   // Only the friend core calls this, using its actually earned revision.
   // Full raw/owned single-flag and untouched Traits/policy/mirror operands
   // are checked by the existing Reset/Init before any world write.
   p->revision=after_revision;
  }
  return reset_init_after_technology_flag_transition(technology,1);
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::optional<BnwMajorPolicyFreeUnitEffect> BnwMajorPlayerTraitsOwner::policy_free_unit_effect()const{
 const auto p=impl_;Impl::Guard guard(p);p->check();return p->runtime->policy_free_unit;
}
BnwMajorPolicyFreeUnitEffect BnwMajorPlayerTraitsOwner::recompute_policy_and_first_free_unit(
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,std::int64_t technology)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  need(host&&host.get()==p->host.get()&&!host.owner_before(p->host)&&!p->host.owner_before(host)&&
   p->applied&&p->runtime&&!p->policy_query_applied,"BNW policy/free-unit requires actual Reset/Init once");
  p->check();
  const auto rules=BnwPolicyHappinessRules::load(host->database());
  need(rules.rules_sha256()==p->fingerprint,"BNW current numeric policy catalogue differs");
  const auto owned=rules.flags(named(p->policy.hashes,"has_policy"),false);
  const auto blocked=rules.flags(named(p->policy.hashes,"branch_blocked"),true);
  std::map<std::int64_t,std::int32_t> costs;
  const auto catalogue=host->database().prefetch_info("Policies",[&](auto slot,auto,const auto& row){
   const auto key=row.text("Type");const auto* type=key?host->types().find(*key):nullptr;
   need(slot<=INT32_MAX&&type&&type->table=="Policies"&&type->id==static_cast<std::int64_t>(slot)&&
    type->hash==content::saved_info_type_hash(*key)&&
    row.has_column("PolicyCostModifier")&&rules.policies().contains(type->id),"BNW full Policy numeric entry unavailable");
   need(costs.emplace(type->id,native(row.integer("PolicyCostModifier"))).second,"BNW duplicate Policy numeric slot");return true;
  });
  need(catalogue.all_succeeded()&&!catalogue.overlapping_ids&&catalogue.slots==owned.size()&&costs.size()==rules.policies().size(),
   "BNW complete native Policy numeric roster differs");
  BnwMajorPolicyFreeUnitEffect effect;effect.player=p->actor;effect.technology=technology;
  std::set<std::int64_t> membership;
  for(std::size_t id=0;id<owned.size();++id){if(!owned[id])continue;
   const auto native_id=static_cast<std::int64_t>(id);membership.insert(native_id);
   need(costs.contains(native_id),"BNW owned policy refers to null Info entry");
   const auto branch=rules.policies().at(native_id).blocked_branch;
   if(branch>=0&&blocked.at(static_cast<std::size_t>(branch)))continue;
   effect.policy=add(effect.policy,costs.at(native_id));
  }
  need(membership==p->owned_policies,"BNW current raw/owned policy membership differs");
  const auto& raw=p->raw();
  const auto scalar=[&](const char* key){const auto& field=named(raw.core.fields,key);
   need(field.encoding==saves::PlayerCoreEncoding::i32&&field.values.size()==1,"BNW policy scalar native shape differs");return native(field.values[0]);};
  effect.before=scalar("m_iPolicyCostModifier");effect.building=scalar("m_iPolicyCostBuildingModifier");
  effect.minor=scalar("m_iPolicyCostMinorCivModifier");
  const auto& trait=p->native_cache.at("m_iPolicyCostModifier");need(trait.size()==1,"BNW freshly Reset Trait policy modifier malformed");
  effect.traits=trait[0];effect.floor=actual_define(*host,"POLICY_COST_DISCOUNT_MAX");
  // Exact native sequential int addition and clamp, never previous cost.
  effect.after=std::max(add(add(add(effect.policy,effect.building),effect.minor),effect.traits),effect.floor);
  const auto& units=p->native_cache.at("m_aFreeTraitUnits");
  need(units.size()%2==0&&units.size()/2<=UINT32_MAX,"BNW fresh native trait-unit vector malformed");
  effect.free_unit_index=0;effect.first_free_unit_class=-1;
  while(effect.free_unit_index<units.size()/2){
   const auto index=static_cast<std::size_t>(effect.free_unit_index)*2;
   if(units[index+1]==technology){effect.first_free_unit_class=units[index];break;}
   ++effect.free_unit_index;
  }
  auto runtime=std::make_shared<Impl::RuntimeCache>(*p->runtime);runtime->policy_free_unit=effect;
  need(p->revision<UINT64_MAX,"BNW policy/free-unit revision overflow");p->check();
  need(scalar("m_iPolicyCostModifier")==effect.before&&scalar("m_iPolicyCostBuildingModifier")==effect.building&&
   scalar("m_iPolicyCostMinorCivModifier")==effect.minor,"BNW policy scalar operands changed before stores");
  auto& target=mutable_named(p->writable->world.all_players->players.at(static_cast<std::size_t>(p->actor)).core.fields,"m_iPolicyCostModifier");
  auto* registry=p->writable->setup.extensions.find<Impl::RuntimeRegistry>();
  need(registry&&registry->players.at(p->actor)==p->runtime,"BNW native Traits storage changed before policy/query stores");
  p->facade_current();
  // Policy scalar assignment first. Then GetFirstFreeUnit resets/searches
  // the genuine owned unsaved iterator, even when it returns NO_UNITCLASS.
  target.values[0]=effect.after;registry->players.at(p->actor)=runtime;
  p->runtime=std::move(runtime);p->policy_query_applied=true;++p->writable->ui_source_revision;++p->revision;
  return effect;
 }catch(...){p->retired.store(true);throw;}
}
}
