#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_pending_created_unit_read.hpp"
#include "civ5/runtime/bnw_unitflag_living_health_read.hpp"
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/runtime/bnw_live_game_context.hpp"
#include "civ5/graphics/bnw_unit_damage_command_owner.hpp"
#include "civ5/runtime/bnw_pathfinder_cache_controls.hpp"
#include "civ5/runtime/bnw_unit_combat_warned_restore_plan.hpp"
#include "civ5/runtime/bnw_two_layer_unit_pathfinder.hpp"
#include "civ5/runtime/bnw_plot_movement_cost.hpp"
#include "civ5/runtime/bnw_unit_movement_query.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/bnw_plot_reveal_read.hpp"
#include "civ5/runtime/bnw_plot_unit_validity_preflight.hpp"
#include <tuple>
#include <bit>
#include "civ5/runtime/bnw_minor_city_strength.hpp"
#include "civ5/runtime/bnw_unit_class_death.hpp"
#include "civ5/runtime/bnw_unit_support_death.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/bnw_major_player_traits_owner.hpp"
#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/runtime/bnw_happiness_update_prefix.hpp"
#include "civ5/runtime/bnw_founder_religion_native.hpp"
#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/runtime/bnw_happiness_update_suffix.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/bnw_unit_sight_plan.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/selected_city_production_services.hpp"
#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/graphics/bnw_pending_unit_visual_owner.hpp"
#include "civ5/graphics/bnw_initial_center_unit_renderer_loan.hpp"
#include "civ5/graphics/bnw_initial_unit_should_dim_renderer_loan.hpp"
#include "civ5/graphics/bnw_initial_unit_setup_graphical_renderer_loan.hpp"
#include "civ5/ui/bnw_client_selection_control.hpp"
#include <algorithm>
#include <any>
#include <string_view>
#include <atomic>
#include <climits>
#include <cmath>
#include <array>
#include <cstddef>
#include <functional>
#include <set>
#include <thread>
#include <typeindex>
#include <type_traits>
namespace civ5::runtime {namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw BnwNativeUnitAllocatorUnavailable(why);}
// Name and hash indexes own separate copies; validate identity by the complete tuple.
bool same_info_binding(const content::InfoTypeEntry* named,const content::InfoTypeEntry* hashed){
 return named&&hashed&&named->type==hashed->type&&named->table==hashed->table&&named->id==hashed->id&&named->hash==hashed->hash;
}
std::string_view name(const char*p){return p?std::string_view(p):std::string_view();}
std::int32_t narrow(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Native unit allocator int overflow");return static_cast<std::int32_t>(v);}
std::int64_t core(const saves::SavedUnitObservation& u,std::string_view key){
 const saves::UnitCoreField* f=nullptr;for(const auto&v:u.core.fields)if(name(v.name)==key){need(!f,"Duplicate native unit core field");f=&v;}
 need(f&&f->values.size()==1,"Missing native unit core field");return f->values[0];
}
enum class ArchiveKind {scalar,text,vector};
struct ArchiveSpec {const char*name;const char*member;ArchiveKind kind;};
constexpr ArchiveSpec archive_specs[]={
 {"CvUnit::m_iID","m_iID",ArchiveKind::scalar},
 {"CvUnit::m_iHotKeyNumber","m_iHotKeyNumber",ArchiveKind::scalar},
 {"CvUnit::m_iX","m_iX",ArchiveKind::scalar},
 {"CvUnit::m_iY","m_iY",ArchiveKind::scalar},
 {"CvUnit::DeployFromOperationTurn","m_iDeployFromOperationTurn",ArchiveKind::scalar},
 {"CvUnit::m_iReconX","m_iReconX",ArchiveKind::scalar},
 {"CvUnit::m_iReconY","m_iReconY",ArchiveKind::scalar},
 {"CvUnit::m_iReconCount","m_iReconCount",ArchiveKind::scalar},
 {"CvUnit::m_iGameTurnCreated","m_iGameTurnCreated",ArchiveKind::scalar},
 {"CvUnit::m_iDamage","m_iDamage",ArchiveKind::scalar},
 {"CvUnit::m_iMoves","m_iMoves",ArchiveKind::scalar},
 {"CvUnit::m_bImmobile","m_bImmobile",ArchiveKind::scalar},
 {"CvUnit::m_iExperience","m_iExperience",ArchiveKind::scalar},
 {"CvUnit::m_iLevel","m_iLevel",ArchiveKind::scalar},
 {"CvUnit::m_iCargo","m_iCargo",ArchiveKind::scalar},
 {"CvUnit::m_iCargoCapacity","m_iCargoCapacity",ArchiveKind::scalar},
 {"CvUnit::m_iAttackPlotX","m_iAttackPlotX",ArchiveKind::scalar},
 {"CvUnit::m_iAttackPlotY","m_iAttackPlotY",ArchiveKind::scalar},
 {"CvUnit::m_iCombatTimer","m_iCombatTimer",ArchiveKind::scalar},
 {"CvUnit::m_iCombatFirstStrikes","m_iCombatFirstStrikes",ArchiveKind::scalar},
 {"CvUnit::m_iCombatDamage","m_iCombatDamage",ArchiveKind::scalar},
 {"CvUnit::m_iFortifyTurns","m_iFortifyTurns",ArchiveKind::scalar},
 {"CvUnit::m_bFortifiedThisTurn","m_bFortifiedThisTurn",ArchiveKind::scalar},
 {"CvUnit::m_iBlitzCount","m_iBlitzCount",ArchiveKind::scalar},
 {"CvUnit::m_iAmphibCount","m_iAmphibCount",ArchiveKind::scalar},
 {"CvUnit::m_iRiverCrossingNoPenaltyCount","m_iRiverCrossingNoPenaltyCount",ArchiveKind::scalar},
 {"CvUnit::m_iEnemyRouteCount","m_iEnemyRouteCount",ArchiveKind::scalar},
 {"CvUnit::m_iRivalTerritoryCount","m_iRivalTerritoryCount",ArchiveKind::scalar},
 {"CvUnit::m_iMustSetUpToRangedAttackCount","m_iMustSetUpToRangedAttackCount",ArchiveKind::scalar},
 {"CvUnit::m_iRangeAttackIgnoreLOSCount","m_iRangeAttackIgnoreLOSCount",ArchiveKind::scalar},
 {"CvUnit::m_iRangedSupportFireCount","m_iRangedSupportFireCount",ArchiveKind::scalar},
 {"CvUnit::m_iAlwaysHealCount","m_iAlwaysHealCount",ArchiveKind::scalar},
 {"CvUnit::m_iHealOutsideFriendlyCount","m_iHealOutsideFriendlyCount",ArchiveKind::scalar},
 {"CvUnit::m_iHillsDoubleMoveCount","m_iHillsDoubleMoveCount",ArchiveKind::scalar},
 {"CvUnit::m_iImmuneToFirstStrikesCount","m_iImmuneToFirstStrikesCount",ArchiveKind::scalar},
 {"CvUnit::m_iExtraVisibilityRange","m_iExtraVisibilityRange",ArchiveKind::scalar},
 {"CvUnit::m_iExtraMoves","m_iExtraMoves",ArchiveKind::scalar},
 {"CvUnit::m_iExtraMoveDiscount","m_iExtraMoveDiscount",ArchiveKind::scalar},
 {"CvUnit::m_iExtraRange","m_iExtraRange",ArchiveKind::scalar},
 {"CvUnit::m_iExtraIntercept","m_iExtraIntercept",ArchiveKind::scalar},
 {"CvUnit::m_iExtraEvasion","m_iExtraEvasion",ArchiveKind::scalar},
 {"CvUnit::m_iExtraFirstStrikes","m_iExtraFirstStrikes",ArchiveKind::scalar},
 {"CvUnit::m_iExtraChanceFirstStrikes","m_iExtraChanceFirstStrikes",ArchiveKind::scalar},
 {"CvUnit::m_iExtraWithdrawal","m_iExtraWithdrawal",ArchiveKind::scalar},
 {"CvUnit::m_iExtraEnemyHeal","m_iExtraEnemyHeal",ArchiveKind::scalar},
 {"CvUnit::m_iExtraNeutralHeal","m_iExtraNeutralHeal",ArchiveKind::scalar},
 {"CvUnit::m_iExtraFriendlyHeal","m_iExtraFriendlyHeal",ArchiveKind::scalar},
 {"CvUnit::m_iSameTileHeal","m_iSameTileHeal",ArchiveKind::scalar},
 {"CvUnit::m_iAdjacentTileHeal","m_iAdjacentTileHeal",ArchiveKind::scalar},
 {"CvUnit::m_iAdjacentModifier","m_iAdjacentModifier",ArchiveKind::scalar},
 {"CvUnit::m_iRangedAttackModifier","m_iRangedAttackModifier",ArchiveKind::scalar},
 {"CvUnit::m_iInterceptionCombatModifier","m_iInterceptionCombatModifier",ArchiveKind::scalar},
 {"CvUnit::m_iInterceptionDefenseDamageModifier","m_iInterceptionDefenseDamageModifier",ArchiveKind::scalar},
 {"CvUnit::m_iAirSweepCombatModifier","m_iAirSweepCombatModifier",ArchiveKind::scalar},
 {"CvUnit::m_iAttackModifier","m_iAttackModifier",ArchiveKind::scalar},
 {"CvUnit::m_iDefenseModifier","m_iDefenseModifier",ArchiveKind::scalar},
 {"CvUnit::m_iExtraCombatPercent","m_iExtraCombatPercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraCityAttackPercent","m_iExtraCityAttackPercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraCityDefensePercent","m_iExtraCityDefensePercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraRangedDefenseModifier","m_iExtraRangedDefenseModifier",ArchiveKind::scalar},
 {"CvUnit::m_iExtraHillsAttackPercent","m_iExtraHillsAttackPercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraHillsDefensePercent","m_iExtraHillsDefensePercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraOpenAttackPercent","m_iExtraOpenAttackPercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraOpenRangedAttackMod","m_iExtraOpenRangedAttackMod",ArchiveKind::scalar},
 {"CvUnit::m_iExtraRoughAttackPercent","m_iExtraRoughAttackPercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraRoughRangedAttackMod","m_iExtraRoughRangedAttackMod",ArchiveKind::scalar},
 {"CvUnit::m_iExtraAttackFortifiedMod","m_iExtraAttackFortifiedMod",ArchiveKind::scalar},
 {"CvUnit::m_iExtraAttackWoundedMod","m_iExtraAttackWoundedMod",ArchiveKind::scalar},
 {"CvUnit::m_iExtraOpenDefensePercent","m_iExtraOpenDefensePercent",ArchiveKind::scalar},
 {"CvUnit::m_iExtraRoughDefensePercent","m_iExtraRoughDefensePercent",ArchiveKind::scalar},
 {"CvUnit::m_iPillageChange","m_iPillageChange",ArchiveKind::scalar},
 {"CvUnit::m_iUpgradeDiscount","m_iUpgradeDiscount",ArchiveKind::scalar},
 {"CvUnit::m_iExperiencePercent","m_iExperiencePercent",ArchiveKind::scalar},
 {"CvUnit::m_iDropRange","m_iDropRange",ArchiveKind::scalar},
 {"CvUnit::m_iAirSweepCapableCount","m_iAirSweepCapableCount",ArchiveKind::scalar},
 {"CvUnit::m_iExtraNavalMoves","m_iExtraNavalMoves",ArchiveKind::scalar},
 {"CvUnit::m_iKamikazePercent","m_iKamikazePercent",ArchiveKind::scalar},
 {"CvUnit::m_iBaseCombat","m_iBaseCombat",ArchiveKind::scalar},
 {"CvUnit::m_eFacingDirection","m_eFacingDirection",ArchiveKind::scalar},
 {"CvUnit::m_iArmyId","m_iArmyId",ArchiveKind::scalar},
 {"CvUnit::m_iIgnoreTerrainCostCount","m_iIgnoreTerrainCostCount",ArchiveKind::scalar},
 {"CvUnit::m_iRoughTerrainEndsTurnCount","m_iRoughTerrainEndsTurnCount",ArchiveKind::scalar},
 {"CvUnit::m_iEmbarkAbilityCount","m_iEmbarkAbilityCount",ArchiveKind::scalar},
 {"CvUnit::m_iHoveringUnitCount","m_iHoveringUnitCount",ArchiveKind::scalar},
 {"CvUnit::m_iFlatMovementCostCount","m_iFlatMovementCostCount",ArchiveKind::scalar},
 {"CvUnit::m_iCanMoveImpassableCount","m_iCanMoveImpassableCount",ArchiveKind::scalar},
 {"CvUnit::m_iOnlyDefensiveCount","m_iOnlyDefensiveCount",ArchiveKind::scalar},
 {"CvUnit::m_iNoDefensiveBonusCount","m_iNoDefensiveBonusCount",ArchiveKind::scalar},
 {"CvUnit::m_iNoCaptureCount","m_iNoCaptureCount",ArchiveKind::scalar},
 {"CvUnit::m_iNukeImmuneCount","m_iNukeImmuneCount",ArchiveKind::scalar},
 {"CvUnit::m_iHiddenNationalityCount","m_iHiddenNationalityCount",ArchiveKind::scalar},
 {"CvUnit::m_iAlwaysHostileCount","m_iAlwaysHostileCount",ArchiveKind::scalar},
 {"CvUnit::m_iNoRevealMapCount","m_iNoRevealMapCount",ArchiveKind::scalar},
 {"CvUnit::m_iCanMoveAllTerrainCount","m_iCanMoveAllTerrainCount",ArchiveKind::scalar},
 {"CvUnit::m_iCanMoveAfterAttackingCount","m_iCanMoveAfterAttackingCount",ArchiveKind::scalar},
 {"CvUnit::m_iFreePillageMoveCount","m_iFreePillageMoveCount",ArchiveKind::scalar},
 {"CvUnit::m_iHPHealedIfDefeatEnemy","m_iHPHealedIfDefeatEnemy",ArchiveKind::scalar},
 {"CvUnit::m_iHealIfDefeatExcludeBarbariansCount","m_iHealIfDefeatExcludeBarbariansCount",ArchiveKind::scalar},
 {"CvUnit::m_iTacticalAIPlotX","m_iTacticalAIPlotX",ArchiveKind::scalar},
 {"CvUnit::m_iTacticalAIPlotY","m_iTacticalAIPlotY",ArchiveKind::scalar},
 {"CvUnit::m_iGarrisonCityID","m_iGarrisonCityID",ArchiveKind::scalar},
 {"CvUnit::m_iFlags","m_iFlags",ArchiveKind::scalar},
 {"CvUnit::m_iNumAttacks","m_iNumAttacks",ArchiveKind::scalar},
 {"CvUnit::m_iAttacksMade","m_iAttacksMade",ArchiveKind::scalar},
 {"CvUnit::m_iGreatGeneralCount","m_iGreatGeneralCount",ArchiveKind::scalar},
 {"CvUnit::m_iGreatGeneralModifier","m_iGreatGeneralModifier",ArchiveKind::scalar},
 {"CvUnit::m_iFriendlyLandsModifier","m_iFriendlyLandsModifier",ArchiveKind::scalar},
 {"CvUnit::m_iFriendlyLandsAttackModifier","m_iFriendlyLandsAttackModifier",ArchiveKind::scalar},
 {"CvUnit::m_iOutsideFriendlyLandsModifier","m_iOutsideFriendlyLandsModifier",ArchiveKind::scalar},
 {"CvUnit::m_iNumInterceptions","m_iNumInterceptions",ArchiveKind::scalar},
 {"CvUnit::m_iMadeInterceptionCount","m_iMadeInterceptionCount",ArchiveKind::scalar},
 {"CvUnit::m_bPromotionReady","m_bPromotionReady",ArchiveKind::scalar},
 {"CvUnit::m_bDeathDelay","m_bDeathDelay",ArchiveKind::scalar},
 {"CvUnit::m_bCombatFocus","m_bCombatFocus",ArchiveKind::scalar},
 {"CvUnit::m_bInfoBarDirty","m_bInfoBarDirty",ArchiveKind::scalar},
 {"CvUnit::m_bNotConverting","m_bNotConverting",ArchiveKind::scalar},
 {"CvUnit::m_bAirCombat","m_bAirCombat",ArchiveKind::scalar},
 {"CvUnit::m_bSetUpForRangedAttack","m_bSetUpForRangedAttack",ArchiveKind::scalar},
 {"CvUnit::m_bEmbarked","m_bEmbarked",ArchiveKind::scalar},
 {"CvUnit::m_bAITurnProcessed","m_bAITurnProcessed",ArchiveKind::scalar},
 {"CvUnit::m_eTacticalMove","m_eTacticalMove",ArchiveKind::scalar},
 {"CvUnit::m_eOwner","m_eOwner",ArchiveKind::scalar},
 {"CvUnit::m_eOriginalOwner","m_eOriginalOwner",ArchiveKind::scalar},
 {"CvUnit::m_eCapturingPlayer","m_eCapturingPlayer",ArchiveKind::scalar},
 {"CvUnit::m_eUnitType","m_eUnitType",ArchiveKind::scalar},
 {"CvUnit::m_eLeaderUnitType","m_eLeaderUnitType",ArchiveKind::scalar},
 {"CvUnit::m_eInvisibleType","m_eInvisibleType",ArchiveKind::scalar},
 {"CvUnit::m_eSeeInvisibleType","m_eSeeInvisibleType",ArchiveKind::scalar},
 {"CvUnit::m_eGreatPeopleDirectiveType","m_eGreatPeopleDirectiveType",ArchiveKind::scalar},
 {"CvUnit::m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility","m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility",ArchiveKind::text},
 {"CvUnit::m_szScriptData","m_strScriptData",ArchiveKind::text},
 {"CvUnit::m_terrainDoubleMoveCount","m_terrainDoubleMoveCount",ArchiveKind::vector},
 {"CvUnit::m_featureDoubleMoveCount","m_featureDoubleMoveCount",ArchiveKind::vector},
 {"CvUnit::m_terrainImpassableCount","m_terrainImpassableCount",ArchiveKind::vector},
 {"CvUnit::m_featureImpassableCount","m_featureImpassableCount",ArchiveKind::vector},
 {"CvUnit::m_extraTerrainAttackPercent","m_extraTerrainAttackPercent",ArchiveKind::vector},
 {"CvUnit::m_extraTerrainDefensePercent","m_extraTerrainDefensePercent",ArchiveKind::vector},
 {"CvUnit::m_extraFeatureAttackPercent","m_extraFeatureAttackPercent",ArchiveKind::vector},
 {"CvUnit::m_extraFeatureDefensePercent","m_extraFeatureDefensePercent",ArchiveKind::vector},
 {"CvUnit::m_extraUnitCombatModifier","m_extraUnitCombatModifier",ArchiveKind::vector},
 {"CvUnit::m_unitClassModifier","m_unitClassModifier",ArchiveKind::vector},
 {"CvUnit::m_iMissionAIX","m_iMissionAIX",ArchiveKind::scalar},
 {"CvUnit::m_iMissionAIY","m_iMissionAIY",ArchiveKind::scalar},
 {"CvUnit::m_eMissionAIType","m_eMissionAIType",ArchiveKind::scalar},
 {"CvUnit::m_eActivityType","m_eActivityType",ArchiveKind::scalar},
 {"CvUnit::m_eAutomateType","m_eAutomateType",ArchiveKind::scalar},
 {"CvUnit::m_eUnitAIType","m_eUnitAIType",ArchiveKind::scalar},
};
struct Bank;struct PendingUnit;
struct ArchiveBinding {ArchiveSpec spec;PendingUnit*target;};
struct NativePromotionVectorSet {std::string member;std::size_t index=0;std::int32_t value=0;bool operator==(const NativePromotionVectorSet&)const=default;};
// Native CvUnit MissionQueue is a constructor-owned ordinary container.
// The actual fresh queue has zero nodes; Reset does NOT clear this member.
struct NativeInitialMission {std::int32_t type=-1,data1=-1,data2=-1,flags=0,push_turn=0;bool operator==(const NativeInitialMission&)const=default;};
struct PendingUnit {
 ~PendingUnit();
 // Each binding borrows this SAME stable node's real storage by member key;
 // values are not duplicated in a second archive DTO.
 std::vector<ArchiveBinding> archive;
 std::set<std::size_t> dirty;
 std::vector<std::size_t> vector_clears;
 std::vector<NativePromotionVectorSet>promotion_vector_sets;
 PendingUnit*self_handle=nullptr;
 std::weak_ptr<Bank> registration_owner;
 std::weak_ptr<PendingUnit> canonical_self;
 std::int32_t registered_slot=-1;
 std::map<std::string,std::int32_t,std::less<>> scalars;
 std::map<std::string,std::vector<std::int32_t>,std::less<>> indexed;
 std::map<std::string,std::string,std::less<>> strings;
 // CvUnitPromotions::Init binds this exact stable unit and genuine source
 // Info universe, then Reset SetSize0; it does NOT preallocate Info-span bits.
 std::weak_ptr<const ui::SelectedBnwPreviewHost> promotion_host;
 PendingUnit* promotion_unit=nullptr;
 std::vector<std::int32_t> promotion_info_ids;
 std::vector<std::uint8_t> promotion_bits;
 std::int32_t religion=-1,religion_strength=0,religion_spreads=0;
 std::uint32_t last_path_destination=UINT32_MAX;
 std::vector<NativeInitialMission>mission_queue;
 bool storage_tail_executed=false,id_assigned=false,init_reset_executed=false,initial_facing_executed=false,initial_coordinates_executed=false;
};
// Native m_iCycleOrder is a plain UNSAVED short, not FAutoVariable.
// These actual Civis nodes own only that proven Reset/load field. They are
// minted by the canonical Bank against exact imported object/full-ID storage.
// No other unknown imported member or archive registration is inferred.
struct ImportedCycleNode {
 std::weak_ptr<Bank>registration_owner;const saves::SavedUnitObservation*source=nullptr;
 std::int32_t owner=-1,id=-1,slot=-1;std::uint64_t generation=0;
 std::int16_t cycle_order=-1;std::uint64_t store_count=0;
};
// Actual mutable imported scalar alias. Its original Load baseline is
// installed without touch (FAutoVariable::load), and dirty lineage belongs to
// this canonical Bank. No unknown full archive state is manufactured.
struct ImportedMovesArchive {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<Bank>owner;const saves::SavedUnitObservation*unit=nullptr;
 saves::UnitCoreField*field=nullptr;std::int64_t*value=nullptr;
 std::int32_t actor=-1,id=-1,loaded=0,expected=0;
 std::uint64_t generation=0,changes=0;std::size_t native_index=0;
 bool dirty=false;
};
// Only the known native m_iDamage Load alias, not a full unit archive.
struct ImportedDamageArchive {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<Bank>owner;const saves::SavedUnitObservation*unit=nullptr;
 saves::UnitCoreField*field=nullptr;std::int64_t*value=nullptr;
 std::int32_t actor=-1,id=-1,loaded=0,expected=0;
 std::uint64_t generation=0,changes=0;std::size_t native_index=0;bool dirty=false;
};
struct ImportedFlagsArchive {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<Bank>owner;const saves::SavedUnitObservation*unit=nullptr;
 saves::UnitCoreField*field=nullptr;std::int64_t*value=nullptr;
 std::int32_t actor=-1,id=-1,loaded=0,expected=0;
 std::uint64_t generation=0,changes=0;std::size_t native_index=0;bool dirty=false;
};
// Actual partial Civis OriginalRead plain members. Native constructor IDInfo
// and reset() create these values; read never serializes/overwrites them.
// Producers must later mutate this SAME node, not remint defaults per query.
struct ImportedMissionRuntime {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<Bank>owner;const saves::SavedUnitObservation*unit=nullptr;
 std::int32_t actor=-1,id=-1,mission_timer=0,combat_city_owner=-1,combat_city_id=-1;
 // Actual ctor/reset(-1), never serialized by Read or reminted by a query.
 std::int32_t last_full_health_turn=-1;std::uint64_t last_full_health_stores=0;
 std::uint64_t generation=0;
 std::vector<std::uint64_t>visual_locations;
};
struct ImportedCityCombatRuntime {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<Bank>owner;const saves::SavedCityObservation*city=nullptr;
 std::int32_t actor=-1,id=-1,combat_owner=-1,combat_id=-1;std::uint64_t generation=0;
};
struct ExistingContinueMissionEntry {
 std::weak_ptr<Bank>owner;const saves::SavedUnitObservation*unit=nullptr;
 std::shared_ptr<ImportedMissionRuntime>runtime;
 std::shared_ptr<ImportedFlagsArchive>flags;
 std::int32_t actor=-1,id=-1;std::uint64_t generation=0;
 std::string before_world_revision,world_revision;BnwUnitPathOwner*paths=nullptr;BnwUnitRetainedPath*path=nullptr;
 // This first-entry API implements the native header default call only.
 // Other callers' steps/ETA require a distinct earned producer/codec version.
 std::int32_t game_turn=-1,steps=0,eta=-1;SimulationRandomState map_rng,game_rng;
 BnwUnitMissionContinuationPhase phase=BnwUnitMissionContinuationPhase::cache_cleared_before_evaluating_flags;
 BnwUnitRetainedPath before_path,current_path;
 // Engaged weak issuer is a real reserved Game attempt, even after expiry.
 // Core-private storage owns it; Bank never strongly owns Game/Host/State.
 std::optional<std::weak_ptr<void>>combat_warned_attempt;
 bool cache_cleared=false;
};
// Actual UnitPathTo body journal, issued only by the closed method after
// genuine GetAt/UpdatePathCache. Public observations cannot reconstitute it.
struct ExistingUnitPathToStage {
 std::weak_ptr<Bank>owner;const saves::SavedUnitObservation*unit=nullptr;
 std::int32_t actor=-1,id=-1,x=0,y=0,flags=0,previous_eta=-1;
 std::uint64_t generation=0;std::string world_revision;
 BnwUnitPathOwner*paths=nullptr;BnwUnitRetainedPath*path=nullptr;
 BnwUnitRetainedPath current_path;
 BnwNativeUnitPathToBoundary boundary=BnwNativeUnitPathToBoundary::unit_move;
 std::optional<std::uint64_t>first_plot;
 bool destination_checked=false,destination_allowed=false,unit_move_checked=false,unit_move_allowed=false;
 std::optional<std::int32_t>move_cost,moves_before,moves_requested;
 std::shared_ptr<ImportedMovesArchive>moves;
};
std::shared_ptr<ImportedFlagsArchive>make_imported_flags_archive(LiveSessionState&s,const saves::SavedSession&source,const std::shared_ptr<Bank>&bank,std::int32_t actor,std::int32_t id,std::uint64_t generation,bool restoring,const SessionCheckpointIdentity&identity);
void require_continue_entry_local(const std::shared_ptr<Bank>&bank,const LiveSessionState&s,const ExistingContinueMissionEntry&e,std::int32_t actor,std::uint64_t generation);
struct TrainBinding {
 std::int32_t owner=-1,city=-1,unit=-1,unit_ai=-1,x=-1,y=-1;
 std::uint64_t before_revision=0;
};
struct NativeIdPrefix {
 BnwNativeUnitIdAssignment assignment;
 std::uint64_t before_revision=0,after_revision=0;
};
struct AbortedSerialBurn {NativeIdPrefix prefix;std::weak_ptr<PendingUnit>destroyed;};
struct Bank {
 std::weak_ptr<const LiveSessionState> source;
 std::uint64_t generation=0;std::string fingerprint;
 std::vector<std::int64_t> baseline;
 std::int32_t slots=0,last=-1,head=-1,free_count=0,serial=8192;
 std::vector<std::int64_t> next;
 std::map<std::int32_t,std::shared_ptr<PendingUnit>> pending_units;
 std::int32_t pending_slot=-1;
 std::map<std::int32_t,std::weak_ptr<PendingUnit>> registered_units;
 std::map<std::int32_t,std::shared_ptr<ImportedCycleNode>>imported_cycle_nodes;
 std::map<std::int32_t,std::shared_ptr<ImportedMovesArchive>>imported_moves_fields;
 std::map<std::int32_t,std::shared_ptr<ImportedDamageArchive>>imported_damage_fields;
 std::map<std::int32_t,std::shared_ptr<ImportedFlagsArchive>>imported_flags_fields;
 std::map<std::int32_t,std::shared_ptr<ImportedMissionRuntime>>imported_mission_runtime;
 std::map<std::int32_t,std::shared_ptr<ImportedCityCombatRuntime>>imported_city_combat_runtime;
 bool imported_city_combat_registered=false;
 std::map<std::int32_t,std::shared_ptr<ExistingContinueMissionEntry>>continue_mission_entries;
 std::map<std::int32_t,std::shared_ptr<ExistingUnitPathToStage>>existing_path_to;
 bool constructor_pending=false;
 bool iteration_started=false,selection_pending=false;
 std::optional<BnwNativeTraitFreeUnitSelection> selected_unit;
 std::optional<TrainBinding> train;
 std::weak_ptr<const ui::SelectedCityTrainCreateSource> train_source;
 std::optional<NativeIdPrefix> last_id_prefix;
 std::int32_t serial_origin=0;std::vector<AbortedSerialBurn>aborted_serial_burns;
 std::size_t iterator=0;std::int64_t iterator_tech=-1;
 std::vector<std::int64_t> trait_units;
 std::atomic<bool> retired=false,busy=false;
};
PendingUnit::~PendingUnit(){
 // Native destructor disables its self handle then erases unitsToCheck before
 // private member/Info cleanup. Weak registry cannot keep units or Host alive.
 self_handle=nullptr;
 if(auto owner=registration_owner.lock()){
  const auto i=owner->registered_units.find(registered_slot);
  if(i!=owner->registered_units.end()&&!i->second.owner_before(canonical_self)&&!canonical_self.owner_before(i->second))owner->registered_units.erase(i);
 }
 promotion_unit=nullptr;promotion_bits.clear();
}
std::optional<std::size_t> archive_index(const PendingUnit&u,std::string_view member){
 for(std::size_t i=0;i<u.archive.size();++i){if(name(u.archive[i].spec.member)==member)return i;}
 return {};
}
void archive_constructor(PendingUnit&u){
 for(const auto&spec:archive_specs){u.archive.push_back({spec,&u});
  switch(spec.kind){case ArchiveKind::scalar:u.scalars.emplace(spec.member,0);break;
   case ArchiveKind::text:u.strings.emplace(spec.member,"");break;
   case ArchiveKind::vector:u.indexed.emplace(spec.member,std::vector<std::int32_t>{});break;}
 }
 u.self_handle=&u;
}
void archive_reset(PendingUnit&u){
 // CvUnit::reset starts clearDelta then reset. FAutoVariable::reset sets
 // ObjectType() without touching; FAutoVector::reset also clears commands.
 u.dirty.clear();u.vector_clears.clear();u.promotion_vector_sets.clear();
 for(const auto&binding:u.archive)switch(binding.spec.kind){
  case ArchiveKind::scalar:u.scalars.at(binding.spec.member)=0;break;
  case ArchiveKind::text:u.strings.at(binding.spec.member).clear();break;
  case ArchiveKind::vector:u.indexed.at(binding.spec.member).clear();break;
 }
}
void assign_scalar(PendingUnit&u,const char*key,std::int32_t value){
 const auto i=u.scalars.find(key);const auto changed=i==u.scalars.end()||i->second!=value;
 if(changed)if(const auto index=archive_index(u,key))u.dirty.insert(*index);
 u.scalars.insert_or_assign(key,value);
}
void clear_archive_vector(PendingUnit&u,const char*key){
 const auto index=archive_index(u,key);need(index&&u.archive[*index].spec.kind==ArchiveKind::vector,"Native archive vector binding missing");
 // FAutoVector::clear always touches and records CLEAR, including empty vectors.
 u.dirty.insert(*index);u.vector_clears.push_back(*index);u.indexed.at(key).clear();
}
void check_registered(const Bank&b,const PendingUnit&u,const std::shared_ptr<PendingUnit>&owner,std::int32_t slot){
 const auto i=b.registered_units.find(slot);
 need(i!=b.registered_units.end()&&i->second.lock().get()==&u&&u.self_handle==&u&&
  !i->second.owner_before(owner)&&!owner.owner_before(i->second)&&u.canonical_self.lock().get()==&u&&
  !u.canonical_self.owner_before(owner)&&!owner.owner_before(u.canonical_self)&&
  u.registration_owner.lock().get()==&b&&u.registered_slot==slot,
  "Native constructor genuine registered handle missing/changed");
 for(const auto&binding:u.archive)need(binding.target==&u,"Native archive binding detached from SAME unit node");
}
// CvPlot::reset initializes its unsaved weak center handle to NULL. This ONE
// SAME-session cache is shared by every Core/actor, never renderer-seeded.
struct InitialCenterHandle {
 const void*node=nullptr;std::weak_ptr<const void> control;
 std::int32_t owner=-1,id=-1;bool ignore_destruction=false;
};
bool same_center_handle(const InitialCenterHandle&a,const InitialCenterHandle&b){
 return a.node==b.node&&a.owner==b.owner&&a.id==b.id&&a.ignore_destruction==b.ignore_destruction&&
  !a.control.owner_before(b.control)&&!b.control.owner_before(a.control);
}
struct InitialCenterCache {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const LiveSessionState> source;std::uint64_t generation=0;
 std::map<std::uint64_t,InitialCenterHandle> plots;
 std::atomic<bool> retired{false},busy{false};
};
struct Registry{std::map<std::int32_t,std::shared_ptr<Bank>> players;std::shared_ptr<InitialCenterCache> centers;};
template<class Emit>void semantic(const LiveSessionState&s,std::int32_t actor,Emit&& emit,
 std::optional<std::int32_t> staged_serial={}){
 need(s.world.all_players&&s.world.all_players->layout==bnw&&actor>=0&&
  static_cast<std::size_t>(actor)<s.world.all_players->players.size(),"Native unit source player unavailable");
 const auto&p=s.world.all_players->players[static_cast<std::size_t>(actor)];
 need(p.saved_player_id==actor&&p.units.layout==bnw&&p.units.header.layout==bnw,
  "Native unit source player/layout identity mismatch");
 const auto&h=p.units.header;emit(h.source_slots);emit(h.wire_unit_count);emit(h.fields.size());
 for(const auto&f:h.fields){for(char x:name(f.name))emit(static_cast<unsigned char>(x));emit(-1);
  emit(staged_serial&&name(f.name)=="current_id"?static_cast<std::int64_t>(*staged_serial):f.value);}
 emit(h.next_free_indices.size());for(auto x:h.next_free_indices)emit(x);emit(p.units.units.size());
 const auto&m=s.maintenance.players.at(actor);emit(m.units.size());
 for(const auto&u:p.units.units){emit(u.index);emit(core(u,"m_iID"));emit(core(u,"m_eOwner"));
  emit(u.core.start_offset);emit(u.core.end_offset);
  const auto it=m.units.find(core(u,"m_iID"));need(it!=m.units.end(),"Native raw/owned unit identity missing");
  const auto&o=it->second;need(o.source_core.has_value(),"Native owned unit source interval missing");
  emit(o.type);emit(o.domain);emit(o.combat);emit(o.source_core->first);emit(o.source_core->second);
  need(o.source_core->first==u.core.start_offset&&o.source_core->second==u.core.end_offset,
   "Native raw/owned unit source intervals differ");
 }
}
std::vector<std::int64_t> capture(const LiveSessionState&s,std::int32_t actor){
 std::vector<std::int64_t> result;semantic(s,actor,[&](auto v){result.push_back(static_cast<std::int64_t>(v));});return result;
}
void compare(const LiveSessionState&s,std::int32_t actor,const std::vector<std::int64_t>& expected){
 std::size_t i=0;semantic(s,actor,[&](auto v){need(i<expected.size()&&expected[i++]==static_cast<std::int64_t>(v),"Native unit source/FLTA changed in place");});
 need(i==expected.size(),"Native unit source/FLTA extent changed");
}
void restore(Bank&b,const saves::UnitsObservation&u,std::int32_t actor){
 const auto&h=u.header;need(h.fields.size()==5&&name(h.fields[0].name)=="num_slots"&&
 name(h.fields[1].name)=="last_index"&&name(h.fields[2].name)=="free_list_head"&&
 name(h.fields[3].name)=="free_list_count"&&name(h.fields[4].name)=="current_id",
 "Native FLTA header layout mismatch");
 b.slots=narrow(h.fields[0].value);b.last=narrow(h.fields[1].value);b.head=narrow(h.fields[2].value);
 b.free_count=narrow(h.fields[3].value);b.serial=narrow(h.fields[4].value);b.next=h.next_free_indices;
 need(b.slots>=0&&b.slots<=8192&&(b.slots&(b.slots-1))==0&&h.source_slots==static_cast<std::uint64_t>(b.slots)&&
 b.next.size()==h.source_slots&&b.last>=-1&&b.last<b.slots&&b.free_count>=0&&b.free_count<=b.last+1&&
 b.serial>=8192&&b.serial%8192==0&&h.wire_unit_count>=0&&static_cast<std::uint64_t>(h.wire_unit_count)==u.units.size()&&
 ((b.free_count==0&&b.head==-1)||(b.free_count>0&&b.head>=0&&b.head<=b.last)),"Native FLTA dimensions invalid");
 std::set<std::int64_t> occupied,freed;std::int64_t previous=-1;
 for(std::size_t i=0;i<u.units.size();++i){const auto&unit=u.units[i];const auto id=core(unit,"m_iID"),slot=id&8191;
 need(unit.index==i&&id>=0&&id<=INT32_MAX&&slot>previous&&slot<=b.last&&(id&~std::int64_t(8191))<b.serial&&
 b.next[static_cast<std::size_t>(slot)]==-1&&core(unit,"m_eOwner")==actor&&occupied.insert(slot).second,
 "Native FLTA occupied slot identity invalid");previous=slot;}
 auto next=b.head;for(std::int32_t i=0;i<b.free_count;++i){need(next>=0&&next<=b.last&&!occupied.contains(next)&&freed.insert(next).second,
 "Native FLTA free chain invalid");next=b.next[static_cast<std::size_t>(next)];}
 need(next==-1&&occupied.size()+freed.size()==static_cast<std::size_t>(b.last+1),"Native FLTA partition incomplete");
}
void register_imported_cycle_nodes(const std::shared_ptr<Bank>&bank,const LiveSessionState&s,std::int32_t actor){
 need(bank&&bank->imported_cycle_nodes.empty(),"Imported CycleOrder registration replay");
 const auto&units=s.world.all_players->players.at(static_cast<std::size_t>(actor)).units;
 for(const auto&u:units.units){const auto id=narrow(core(u,"m_iID")),slot=id&8191;
  auto node=std::make_shared<ImportedCycleNode>();node->registration_owner=bank;node->source=&u;
  node->owner=actor;node->id=id;node->slot=slot;node->generation=bank->generation;
  // Native reset(false) and constructor initialize short=-1; Read never
  // serializes or overwrites it. This is actual partial Civis storage, not an
  // alleged original DLL allocation or fabricated source payload field.
  need(bank->imported_cycle_nodes.emplace(slot,std::move(node)).second,"Duplicate imported CycleOrder slot");
 }
}
void require_imported_mission_payload_lineage(const saves::SavedSession&source,const SessionCheckpointIdentity&identity){
 need(source.source.container==saves::SaveContainerKind::civ5||source.source.container==saves::SaveContainerKind::civis,"Imported mission unknown source container");
 need(identify_bytes(source.source.bytes).sha256==identity.payload_sha256,"Imported mission actual immutable payload differs from canonical Host");
 if(source.source.container==saves::SaveContainerKind::civis){
  need(source.source.source_sha256&&source.source.source_sha256->size()==64&&std::all_of(source.source.source_sha256->begin(),source.source.source_sha256->end(),[](char x){return (x>='0'&&x<='9')||(x>='a'&&x<='f');})&&source.source.source_prefix&&!source.source.source_prefix->empty()&&source.source.inflate.stream_end&&source.source.inflate.available_bytes==source.source.bytes.size(),"Imported mission Civis original source provenance/payload completeness unavailable");
 }
}
void register_imported_mission_runtime(const std::shared_ptr<Bank>&bank,const LiveSessionState&s,const saves::SavedSession&source,std::int32_t actor,const SessionCheckpointIdentity&identity){
 need(bank&&bank->imported_mission_runtime.empty(),"Imported mission actual plain-member registration replay");
 // Both genuine readers retain the SAME original decompressed payload. A
 // Civis scalar overlay is not a mission phase/VM restore. Only native
 // constructor/reset plain defaults are owned here; closed section43 later
 // reconstructs its exact pending history on these actual physical nodes.
 require_imported_mission_payload_lineage(source,identity);
 need(source.snapshot.all_players&&source.snapshot.all_players->layout==bnw,"Imported mission original Read player source unavailable");
 const auto&loaded=source.snapshot.all_players->players.at(actor);const auto&current=s.world.all_players->players.at(actor);
 need(loaded.saved_player_id==actor&&current.saved_player_id==actor&&loaded.units.units.size()==current.units.units.size(),"Imported mission original Read roster extent differs");
 for(const auto&u:current.units.units){const auto id=narrow(core(u,"m_iID"));const saves::SavedUnitObservation*original=nullptr;
  for(const auto&v:loaded.units.units)if(narrow(core(v,"m_iID"))==id){need(!original,"Imported mission duplicate original full ID");original=&v;}
  need(original&&core(*original,"m_eOwner")==actor&&core(u,"m_eOwner")==actor&&original->core.start_offset==u.core.start_offset&&original->core.end_offset==u.core.end_offset&&original->tail.start_offset==u.tail.start_offset&&original->tail.end_offset==u.tail.end_offset,"Imported mission OriginalRead physical object lineage differs");
  auto node=std::make_shared<ImportedMissionRuntime>();node->owner=bank;node->unit=&u;node->actor=actor;node->id=id;node->generation=bank->generation;
  // CvUnit constructor IDInfo m_combatCity(-1,-1), reset timer0 and actual
  // vector.clear. No serialized field is supplied by a fake payload or read.
  node->mission_timer=0;node->visual_locations.clear();
  need(bank->imported_mission_runtime.emplace(id,std::move(node)).second,"Imported mission native plain slot duplicate");
 }
}

std::int32_t busy_city_scalar(const saves::SavedCityObservation& city,std::string_view key){
 const saves::CityCoreField*found=nullptr;for(const auto&f:city.core.fields)if(name(f.name)==key){need(!found,"City busy duplicate scalar");found=&f;}
 need(found&&found->encoding==saves::CityCoreEncoding::i32&&found->values.size()==1,"City busy actual native scalar absent");return narrow(found->values[0]);
}
void register_imported_city_combat_runtime(const std::shared_ptr<Bank>&bank,const LiveSessionState&s,const saves::SavedSession&source,
 std::int32_t actor,const SessionCheckpointIdentity&identity){
 need(bank&&!bank->imported_city_combat_registered&&bank->imported_city_combat_runtime.empty(),"City combat OriginalRead registration replay");
 require_imported_mission_payload_lineage(source,identity);
 need(source.snapshot.all_players&&source.snapshot.all_players->layout==bnw&&source.snapshot.all_players->players.size()==64&&s.world.all_players,
  "City combat genuine original64 Read absent");
 const auto&loaded=source.snapshot.all_players->players.at(actor);const auto&current=s.world.all_players->players.at(actor);
 need(loaded.saved_player_id==actor&&current.saved_player_id==actor&&loaded.cities.layout==bnw&&current.cities.layout==bnw&&
  loaded.cities.cities.size()==current.cities.cities.size(),"City combat original Read roster differs");
 for(const auto&city:current.cities.cities){const auto id=busy_city_scalar(city,"m_iID");const saves::SavedCityObservation*original=nullptr;
  for(const auto&old:loaded.cities.cities)if(busy_city_scalar(old,"m_iID")==id){need(!original,"City combat duplicate original fullID");original=&old;}
  need(original&&busy_city_scalar(*original,"m_eOwner")==actor&&busy_city_scalar(city,"m_eOwner")==actor&&
   original->core.start_offset==city.core.start_offset&&original->core.end_offset==city.core.end_offset&&
   original->tail.start_offset==city.tail.start_offset&&original->tail.end_offset==city.tail.end_offset&&
   resolve_bnw_city_registry(s.world,s.cities,actor,id)==&city,"City combat original new/Read/Load physical lineage differs");
  auto node=std::make_shared<ImportedCityCombatRuntime>();node->owner=bank;node->city=&city;node->actor=actor;node->id=id;node->generation=bank->generation;
  // SDK FLTA operator>> new T, CvCity plain IDInfo ctor NO_PLAYER/INVALID_INDEX;
  // Read/reset omit m_combatUnit. This actual owned node survives every query.
  need(bank->imported_city_combat_runtime.emplace(id,std::move(node)).second,"City combat duplicate native slot");
 }
 bank->imported_city_combat_registered=true;
}
void check_imported_cycle_nodes(const Bank&b,const LiveSessionState&s,std::int32_t actor){
 const auto&units=s.world.all_players->players.at(static_cast<std::size_t>(actor)).units;
 need(b.imported_cycle_nodes.size()==units.units.size(),"Imported CycleOrder registry extent changed");
 for(const auto&u:units.units){const auto id=narrow(core(u,"m_iID")),slot=id&8191;const auto i=b.imported_cycle_nodes.find(slot);
  need(i!=b.imported_cycle_nodes.end()&&i->second,"Imported CycleOrder real registry missing");const auto&n=*i->second;
  auto owner=n.registration_owner.lock();need(owner.get()==&b&&n.source==&u&&n.owner==actor&&n.id==id&&n.slot==slot&&n.generation==b.generation,
   "Imported CycleOrder canonical physical source replaced");
 }
}
// ReadStreamableFFreeListTrashArray -> Load puts each genuine full-ID
// record at its native slot. Callers first validate the COMPLETE manager via
// existing restore/semantic proof; this is not an approximate ordinal lookup.
const saves::SavedUnitObservation* native_imported_get_at(const LiveSessionState&s,std::int32_t actor,std::int32_t id){
 const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(actor));
 const auto&u=player.units;const auto&h=u.header;
 need(player.saved_player_id==actor&&u.layout==bnw&&h.layout==bnw&&h.fields.size()==5,
  "Native imported GetAt genuine manager changed");
 if(id==-1||h.source_slots==0)return nullptr; // INVALID_INDEX or m_pArray NULL.
 const auto slot=id&8191;if(slot>h.fields[1].value)return nullptr;
 const saves::SavedUnitObservation* found=nullptr;
 for(const auto&entry:u.units)if((core(entry,"m_iID")&8191)==slot){need(!found,"Native imported GetAt duplicate occupied slot");found=&entry;}
 if(found&&((id&~8191)==0||core(*found,"m_iID")==id))return found;
 return nullptr; // Real empty slot or obsolete serial, exactly SDK GetAt.
}
std::map<std::string,std::int32_t,std::less<>> constructor_prefix(std::int32_t actual_id=0){
 std::map<std::string,std::int32_t,std::less<>> p;
// Exact CvUnit::reset constructor scalar interval. INVALID_PLOT_COORD=-INT_MAX.
 p.emplace("m_iID",actual_id);
 p.emplace("m_iHotKeyNumber",-1);
 p.emplace("m_iX",-INT32_MAX);
 p.emplace("m_iY",-INT32_MAX);
 p.emplace("m_iLastMoveTurn",0);
 p.emplace("m_iCycleOrder",-1);
 p.emplace("m_iDeployFromOperationTurn",-100);
 p.emplace("m_iReconX",-INT32_MAX);
 p.emplace("m_iReconY",-INT32_MAX);
 p.emplace("m_iReconCount",0);
 p.emplace("m_iGameTurnCreated",0);
 p.emplace("m_iDamage",0);
 p.emplace("m_iMoves",0);
 p.emplace("m_bImmobile",0);
 p.emplace("m_iExperience",0);
 p.emplace("m_iLevel",1);
 p.emplace("m_iCargo",0);
 p.emplace("m_iAttackPlotX",-INT32_MAX);
 p.emplace("m_iAttackPlotY",-INT32_MAX);
 p.emplace("m_iCombatTimer",0);
 p.emplace("m_iCombatFirstStrikes",0);
 p.emplace("m_iCombatDamage",0);
 p.emplace("m_iFortifyTurns",0);
 p.emplace("m_bFortifiedThisTurn",0);
 p.emplace("m_iBlitzCount",0);
 p.emplace("m_iAmphibCount",0);
 p.emplace("m_iRiverCrossingNoPenaltyCount",0);
 p.emplace("m_iEnemyRouteCount",0);
 p.emplace("m_iRivalTerritoryCount",0);
 p.emplace("m_iMustSetUpToRangedAttackCount",0);
 p.emplace("m_iRangeAttackIgnoreLOSCount",0);
 p.emplace("m_iCityAttackOnlyCount",0);
 p.emplace("m_iCaptureDefeatedEnemyCount",0);
 p.emplace("m_iRangedSupportFireCount",0);
 p.emplace("m_iAlwaysHealCount",0);
 p.emplace("m_iHealOutsideFriendlyCount",0);
 p.emplace("m_iHillsDoubleMoveCount",0);
 p.emplace("m_iImmuneToFirstStrikesCount",0);
 p.emplace("m_iExtraVisibilityRange",0);
 p.emplace("m_iExtraMoves",0);
 p.emplace("m_iExtraMoveDiscount",0);
 p.emplace("m_iExtraRange",0);
 p.emplace("m_iExtraIntercept",0);
 p.emplace("m_iExtraEvasion",0);
 p.emplace("m_iExtraFirstStrikes",0);
 p.emplace("m_iExtraChanceFirstStrikes",0);
 p.emplace("m_iExtraWithdrawal",0);
 p.emplace("m_iExtraEnemyHeal",0);
 p.emplace("m_iExtraNeutralHeal",0);
 p.emplace("m_iExtraFriendlyHeal",0);
 p.emplace("m_iSameTileHeal",0);
 p.emplace("m_iAdjacentTileHeal",0);
 p.emplace("m_iEnemyDamageChance",0);
 p.emplace("m_iNeutralDamageChance",0);
 p.emplace("m_iEnemyDamage",0);
 p.emplace("m_iNeutralDamage",0);
 p.emplace("m_iNearbyEnemyCombatMod",0);
 p.emplace("m_iNearbyEnemyCombatRange",0);
 p.emplace("m_iExtraCombatPercent",0);
 p.emplace("m_iAdjacentModifier",0);
 p.emplace("m_iRangedAttackModifier",0);
 p.emplace("m_iInterceptionCombatModifier",0);
 p.emplace("m_iInterceptionDefenseDamageModifier",0);
 p.emplace("m_iAirSweepCombatModifier",0);
 p.emplace("m_iAttackModifier",0);
 p.emplace("m_iDefenseModifier",0);
 p.emplace("m_iExtraCityAttackPercent",0);
 p.emplace("m_iExtraCityDefensePercent",0);
 p.emplace("m_iExtraRangedDefenseModifier",0);
 p.emplace("m_iExtraHillsAttackPercent",0);
 p.emplace("m_iExtraHillsDefensePercent",0);
 p.emplace("m_iExtraOpenAttackPercent",0);
 p.emplace("m_iExtraOpenRangedAttackMod",0);
 p.emplace("m_iExtraRoughAttackPercent",0);
 p.emplace("m_iExtraRoughRangedAttackMod",0);
 p.emplace("m_iExtraAttackFortifiedMod",0);
 p.emplace("m_iExtraAttackWoundedMod",0);
 p.emplace("m_iFlankAttackModifier",0);
 p.emplace("m_iExtraOpenDefensePercent",0);
 p.emplace("m_iExtraRoughDefensePercent",0);
 p.emplace("m_iPillageChange",0);
 p.emplace("m_iUpgradeDiscount",0);
 p.emplace("m_iExperiencePercent",0);
 p.emplace("m_iDropRange",0);
 p.emplace("m_iAirSweepCapableCount",0);
 p.emplace("m_iExtraNavalMoves",0);
 p.emplace("m_iKamikazePercent",0);
 return p;
}
std::shared_ptr<PendingUnit> allocate_pending_unit(const std::shared_ptr<const ui::SelectedBnwPreviewHost>&host){
 auto unit=std::make_shared<PendingUnit>();
 // CvUnit ctor234 native plain member, intentionally not reset() later.
 unit->scalars.emplace("m_iHealOnPillageCount",0);archive_constructor(*unit);
 // Actual religion constructor precedes initPromotions, not religion.Init.
 unit->religion=-1;unit->religion_strength=0;unit->religion_spreads=0;
 const auto info=host->database().prefetch_info("UnitPromotions");
 need(info.all_succeeded()&&!info.overlapping_ids&&info.rows>0&&info.slots<=4'000'000,
  "Actual unit promotion Info universe unavailable");
 // PrefetchCollection retains MaxID+1 slots, with NULL at absent IDs.
 // -1 denotes that absence in this private identity vector; it is never an
 // invented promotion or a compact SQL visit ordinal.
 need(info.slots>=info.rows&&info.holes==info.slots-info.rows,"Actual promotion Info null-slot extent differs");
 unit->promotion_info_ids.assign(static_cast<std::size_t>(info.slots),-1);
 std::uint64_t promotion_span=0;
 const auto q=host->database().query_rows({"SELECT ID, Type FROM UnitPromotions WHERE ID > -1 ORDER BY ID",{}},[&](const auto&row){
  const auto id=row.integer_at(0);const auto type=row.text("Type");const auto*canonical=type?host->types().find(*type):nullptr;
  need(id>=0&&static_cast<std::uint64_t>(id)>=promotion_span&&static_cast<std::uint64_t>(id)<info.slots&&canonical&&canonical->table=="UnitPromotions"&&canonical->id==id&&
   canonical->hash==content::saved_info_type_hash(*type)&&same_info_binding(canonical,host->types().find_hash(canonical->hash))&&
   unit->promotion_info_ids[static_cast<std::size_t>(id)]==-1,"Actual promotion Info identity changed");
  unit->promotion_info_ids[static_cast<std::size_t>(id)]=narrow(id);promotion_span=static_cast<std::uint64_t>(id)+1;
 });need(q.all_succeeded()&&q.rows==info.rows&&promotion_span==info.slots,"Complete actual promotion Info binding failed");
 unit->promotion_host=host;unit->promotion_unit=unit.get();unit->promotion_bits.clear();
 return unit; // registration must precede archive.reset and Reset field writes.
}
void constructor_storage_tail(PendingUnit&u,std::int32_t actor=-1,std::int32_t type=-1,std::int32_t combat=0){
 assign_scalar(u,"m_eFacingDirection",2);
 assign_scalar(u,"m_iIgnoreTerrainCostCount",0);
 assign_scalar(u,"m_iRoughTerrainEndsTurnCount",0);
 assign_scalar(u,"m_iEmbarkAbilityCount",0);
 assign_scalar(u,"m_iHoveringUnitCount",0);
 assign_scalar(u,"m_iFlatMovementCostCount",0);
 assign_scalar(u,"m_iCanMoveImpassableCount",0);
 assign_scalar(u,"m_iOnlyDefensiveCount",0);
 assign_scalar(u,"m_iNoDefensiveBonusCount",0);
 assign_scalar(u,"m_iNoCaptureCount",0);
 assign_scalar(u,"m_iNukeImmuneCount",0);
 assign_scalar(u,"m_iAlwaysHealCount",0);
 assign_scalar(u,"m_iHiddenNationalityCount",0);
 assign_scalar(u,"m_iAlwaysHostileCount",0);
 assign_scalar(u,"m_iNoRevealMapCount",0);
 assign_scalar(u,"m_iCanMoveAllTerrainCount",0);
 assign_scalar(u,"m_iCanMoveAfterAttackingCount",0);
 assign_scalar(u,"m_iFreePillageMoveCount",0);
 assign_scalar(u,"m_iHPHealedIfDefeatEnemy",0);
 assign_scalar(u,"m_iGoldenAgeValueFromKills",0);
 assign_scalar(u,"m_iSapperCount",0);
 assign_scalar(u,"m_iCanHeavyCharge",0);
 assign_scalar(u,"m_iNumExoticGoods",0);
 assign_scalar(u,"m_iTacticalAIPlotX",-INT32_MAX);
 assign_scalar(u,"m_iTacticalAIPlotY",-INT32_MAX);
 assign_scalar(u,"m_iGarrisonCityID",-1);
 assign_scalar(u,"m_iFlags",0);
 assign_scalar(u,"m_iNumAttacks",1);
 assign_scalar(u,"m_iAttacksMade",0);
 assign_scalar(u,"m_iGreatGeneralCount",0);
 assign_scalar(u,"m_iGreatAdmiralCount",0);
 assign_scalar(u,"m_iGreatGeneralModifier",0);
 assign_scalar(u,"m_iGreatGeneralReceivesMovementCount",0);
 assign_scalar(u,"m_iGreatGeneralCombatModifier",0);
 assign_scalar(u,"m_iIgnoreGreatGeneralBenefit",0);
 assign_scalar(u,"m_iIgnoreZOC",0);
 assign_scalar(u,"m_iHealIfDefeatExcludeBarbariansCount",0);
 assign_scalar(u,"m_iNumInterceptions",1);
 assign_scalar(u,"m_iMadeInterceptionCount",0);
 assign_scalar(u,"m_iEverSelectedCount",0);
 assign_scalar(u,"m_bIgnoreDangerWakeup",0);
 assign_scalar(u,"m_iEmbarkedAllWaterCount",0);
 assign_scalar(u,"m_iEmbarkExtraVisibility",0);
 assign_scalar(u,"m_iEmbarkDefensiveModifier",0);
 assign_scalar(u,"m_iCapitalDefenseModifier",0);
 assign_scalar(u,"m_iCapitalDefenseFalloff",0);
 assign_scalar(u,"m_iCityAttackPlunderModifier",0);
 assign_scalar(u,"m_iReligiousStrengthLossRivalTerritory",0);
 assign_scalar(u,"m_iTradeMissionInfluenceModifier",0);
 assign_scalar(u,"m_iTradeMissionGoldModifier",0);
 assign_scalar(u,"m_bPromotionReady",0);
 assign_scalar(u,"m_bDeathDelay",0);
 assign_scalar(u,"m_bCombatFocus",0);
 assign_scalar(u,"m_bInfoBarDirty",0);
 assign_scalar(u,"m_bNotConverting",0);
 assign_scalar(u,"m_bAirCombat",0);
 assign_scalar(u,"m_bSetUpForRangedAttack",0);
 assign_scalar(u,"m_bEmbarked",0);
 assign_scalar(u,"m_bAITurnProcessed",0);
 assign_scalar(u,"m_bWaitingForMove",0);
 assign_scalar(u,"m_eTacticalMove",-1);
 assign_scalar(u,"m_eOwner",actor);
 assign_scalar(u,"m_eOriginalOwner",actor);
 assign_scalar(u,"m_eCapturingPlayer",-1);
 assign_scalar(u,"m_bCapturedAsIs",0);
 assign_scalar(u,"m_eUnitType",type);
 assign_scalar(u,"m_iBaseCombat",combat);
 assign_scalar(u,"m_eLeaderUnitType",-1);
 assign_scalar(u,"m_eInvisibleType",-1);
 assign_scalar(u,"m_eSeeInvisibleType",-1);
 assign_scalar(u,"m_eGreatPeopleDirectiveType",-1);
 assign_scalar(u,"m_iCargoCapacity",0);
 assign_scalar(u,"m_eGreatWork",-1);
 assign_scalar(u,"m_iTourismBlastStrength",0);
 assign_scalar(u,"m_iScenarioData",0);
 assign_scalar(u,"m_iMissionTimer",0);
 assign_scalar(u,"m_eActivityType",0);
 assign_scalar(u,"m_eAutomateType",-1);
 assign_scalar(u,"m_iMapLayer",0);
 assign_scalar(u,"m_iNumGoodyHutsPopped",0);
 assign_scalar(u,"m_iLastGameTurnAtFullHealth",-1);
 // Constructor reset uses NO_UNIT, so its native m_pUnitInfo is null.
 // NO_PLAYER/INVALID_INDEX IDInfo reset is represented by actual paired fields.
 for(const auto*key:{"m_combatUnit.owner","m_combatUnit.id","m_transportUnit.owner","m_transportUnit.id"})
  assign_scalar(u,key,-1);
 u.indexed["m_extraDomainModifiers"].clear();
 for(int i=0;i<5;++i)u.indexed.at("m_extraDomainModifiers").push_back(0); // NUM_DOMAIN_TYPES5
 for(const auto*key:{"m_strName","m_strNameIAmNotSupposedToBeUsedAnyMoreBecauseThisShouldNotBeCheckedAndWeNeedToPreserveSaveGameCompatibility","m_strScriptData"})u.strings[key].clear();
 u.indexed["m_unitMoveLocs"].clear();
 // Genuine newly constructed vectors start empty. uninitInfos clears these
 // nine; terrainDoubleMoveCount is NOT in that helper and remains ctor-empty.
 for(const auto*key:{"m_featureDoubleMoveCount","m_terrainImpassableCount","m_featureImpassableCount","m_extraTerrainAttackPercent",
  "m_extraTerrainDefensePercent","m_extraFeatureAttackPercent","m_extraFeatureDefensePercent","m_extraUnitCombatModifier","m_unitClassModifier"})clear_archive_vector(u,key);
 u.indexed.try_emplace("m_terrainDoubleMoveCount");u.indexed["m_kLastPath"].clear();
 u.last_path_destination=UINT32_MAX;
 // if(!bConstructorCall) is FALSE here: no terrain/feature/combat/class Info
 // resize loop, no MissionAI or UnitAI assignment may be credited at this stage.
 u.storage_tail_executed=true;
}
std::size_t native_init_info_span(const ui::SelectedBnwPreviewHost&host,const char*table){
 const auto info=host.database().prefetch_info(table);
 need(info.all_succeeded()&&!info.overlapping_ids&&info.rows>0&&info.slots>=info.rows&&info.slots<=4'000'000&&info.holes==info.slots-info.rows,
  "Native Init indexed Info universe absent/overlapping/unsupported");
 // Native GC Info extent is MaxID+1. Absent UnitClasses IDs remain NULL
 // Info slots; the indexed modifier vector still resizes over that span.
 // Never manufacture a Type/hash/Info payload for an absent SQL row.
 std::set<std::int64_t>seen;std::uint64_t span=0;
 const auto q=host.database().query_rows({std::string("SELECT ID, Type FROM ")+table+" WHERE ID>-1 ORDER BY ID",{}},[&](const auto&row){
  const auto id=row.integer_at(0);const auto type=row.text("Type");const auto*entry=type?host.types().find(*type):nullptr;
  need(id>=0&&static_cast<std::uint64_t>(id)>=span&&static_cast<std::uint64_t>(id)<info.slots&&seen.insert(id).second&&entry&&entry->table==table&&entry->id==id&&
   entry->hash!=0&&entry->hash==content::saved_info_type_hash(*type)&&same_info_binding(entry,host.types().find_hash(entry->hash)),
   "Native Init indexed actual Type/ID/hash binding differs");span=static_cast<std::uint64_t>(id)+1;
 });need(q.all_succeeded()&&seen.size()==info.rows&&span==info.slots,"Native Init complete indexed Info traversal failed");
 std::size_t catalog_rows=0;for(const auto&[key,entry]:host.types().entries())if(entry.table==table&&entry.id>=0){
  ++catalog_rows;need(seen.contains(entry.id)&&entry.type==key,"Native Init indexed catalog contains no corresponding actual SQL row");
 }need(catalog_rows==seen.size(),"Native Init indexed catalog/SQL universe differs");
 return static_cast<std::size_t>(info.slots);
}
void native_init_indexed_reset(PendingUnit&u,std::size_t terrains,std::size_t features,std::size_t combats,std::size_t classes){
 // CvUnit.cpp926: Reset(false), after real scalar/container reset.
 u.promotion_bits.clear();
 const auto resize=[&](const char*key,std::size_t count){clear_archive_vector(u,key);u.indexed.at(key).resize(count);
  for(std::size_t i=0;i<count;++i)u.indexed.at(key)[i]=0;};
 for(const auto*key:{"m_terrainDoubleMoveCount","m_terrainImpassableCount","m_extraTerrainAttackPercent","m_extraTerrainDefensePercent"})resize(key,terrains);
 for(const auto*key:{"m_featureDoubleMoveCount","m_featureImpassableCount","m_extraFeatureDefensePercent","m_extraFeatureAttackPercent"})resize(key,features);
 resize("m_extraUnitCombatModifier",combats);resize("m_unitClassModifier",classes);
 assign_scalar(u,"m_iMissionAIX",-INT32_MAX);assign_scalar(u,"m_iMissionAIY",-INT32_MAX);
 assign_scalar(u,"m_eMissionAIType",-1);assign_scalar(u,"m_missionAIUnit.owner",-1);assign_scalar(u,"m_missionAIUnit.id",-1);
 assign_scalar(u,"m_eUnitAIType",-1);
}
BnwNativeUnitConstructorStorageSnapshot storage_snapshot(std::int32_t slot,const PendingUnit&u){
 return {slot,u.scalars,u.indexed,u.strings,u.promotion_info_ids,u.promotion_bits,
  u.religion,u.religion_strength,u.religion_spreads,u.last_path_destination};
}
}

template<class Rows>const auto& garrison_named(const Rows& rows,std::string_view key){
 const typename Rows::value_type* result=nullptr;
 for(const auto&row:rows)if(name(row.name)==key){need(!result,"Duplicate garrison operand");result=&row;}
 need(result,"Missing actual garrison operand");return *result;
}
template<class Rows>auto& garrison_named_mutable(Rows&rows,std::string_view key){
 typename Rows::value_type*result=nullptr;
 for(auto&row:rows)if(name(row.name)==key){need(!result,"Duplicate mutable garrison operand");result=&row;}
 need(result,"Missing mutable garrison operand");return *result;
}
template<class Emit>void garrison_policy_semantic(const LiveSessionState&s,std::int32_t actor,Emit&&emit){
 need(s.world.all_players&&s.world.all_players->layout==bnw&&actor>=0&&
  static_cast<std::size_t>(actor)<s.world.all_players->players.size(),"Garrison actual player unavailable");
 const auto&p=s.world.all_players->players[static_cast<std::size_t>(actor)];
 need(p.index==static_cast<std::uint64_t>(actor)&&p.saved_player_id==actor&&p.core.layout==bnw&&p.strategy.layout==bnw,
  "Garrison actual player identity differs");
 const auto&id=garrison_named(p.core.fields,"m_eID");need(id.values.size()==1&&id.values[0]==actor,"Garrison player GetID differs");
 emit(id.values[0]);const auto&policies=garrison_named(p.strategy.blocks,"policies");
 emit(garrison_named(policies.fields,"version").value);
 for(const auto*key:{"has_policy","branch_blocked"}){
  const auto&wire=garrison_named(policies.hashes,key);emit(wire.wire_count);emit(wire.rows.size());
  for(const auto&row:wire.rows){emit(row.type_hash);emit(row.values.size());for(auto value:row.values)emit(value);}
 }
 const auto&shared=s.setup.players.at(actor);emit(shared.policies_resolved);emit(shared.policies.size());
 for(auto policy:shared.policies)emit(policy);
}
void compare_garrison_policies(const LiveSessionState&s,std::int32_t actor,const std::vector<std::int64_t>&seal){
 std::size_t i=0;garrison_policy_semantic(s,actor,[&](auto value){
  need(i<seal.size()&&seal[i++]==static_cast<std::int64_t>(value),"Garrison policies changed in place");});
 need(i==seal.size(),"Garrison policy source extent changed");
}
struct GarrisonPolicyRead {std::vector<std::int64_t> seal;std::int32_t value=0;};
GarrisonPolicyRead prepare_garrison_policies(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,std::int32_t actor){
 GarrisonPolicyRead result;garrison_policy_semantic(s,actor,[&](auto value){result.seal.push_back(static_cast<std::int64_t>(value));});
 const auto rules=BnwPolicyHappinessRules::load(host.database());
 need(rules.rules_sha256()==host.identity().rules_sha256,"Garrison policy activated rules differ");
 const auto&block=garrison_named(s.world.all_players->players.at(static_cast<std::size_t>(actor)).strategy.blocks,"policies");
 const auto version=garrison_named(block.fields,"version").value;need(version==1||version==2,"Garrison policy version unsupported");
 const auto owned=rules.flags(garrison_named(block.hashes,"has_policy"),false);
 const auto blocked=rules.flags(garrison_named(block.hashes,"branch_blocked"),true);
 std::set<std::int64_t> membership;for(std::size_t i=0;i<owned.size();++i)if(owned[i])membership.insert(static_cast<std::int64_t>(i));
 need(s.setup.players.at(actor).policies_resolved&&s.setup.players.at(actor).policies==membership,"Garrison raw/owned policies differ");
 std::map<std::int64_t,std::int32_t> culture;
 const auto report=host.database().prefetch_info("Policies",[&](auto slot,auto,const auto&row){
  const auto type=row.text("Type");const auto*entry=type?host.types().find(*type):nullptr;
  need(slot<=INT32_MAX&&entry&&entry->table=="Policies"&&entry->id==static_cast<std::int64_t>(slot)&&
   entry->hash==content::saved_info_type_hash(*type)&&row.has_column("CulturePerGarrisonedUnit"),"Garrison complete PolicyInfo identity differs");
  need(culture.emplace(static_cast<std::int64_t>(slot),narrow(row.integer("CulturePerGarrisonedUnit"))).second,
   "Duplicate garrison PolicyInfo slot");return true;
 });
 need(report.all_succeeded()&&report.slots==owned.size()&&culture.size()==rules.policies().size(),"Garrison PolicyInfo universe incomplete");
 for(std::size_t slot=0;slot<owned.size();++slot)if(owned[slot]){
  const auto id=static_cast<std::int64_t>(slot);const auto it=rules.policies().find(id);
  need(it!=rules.policies().end()&&culture.contains(id),"Owned garrison PolicyInfo is NULL");
  const auto branch=it->second.blocked_branch;
  if(branch>=0&&blocked.at(static_cast<std::size_t>(branch)))continue;
  result.value=narrow(static_cast<std::int64_t>(result.value)+culture.at(id)); // Native ascending policy slots.
 }
 compare_garrison_policies(s,actor,result.seal);return result;
}
struct InitialGarrisonPrefix {
 const saves::SavedCityObservation* city=nullptr;
 const saves::CityCoreField* field=nullptr;
 std::int32_t before=0,after=0,modifier=0;
 std::uint64_t before_revision=0,after_revision=0;
 std::uint64_t core_start=0,core_end=0;
 std::vector<std::int64_t> policies;
 bool stored=false;
};

namespace {
struct InitialCycleOperand {
 const saves::SavedUnitObservation* raw=nullptr;
 std::int32_t query_owner=-1,query_id=-1,owner=-1,id=-1,type=-1,domain=-1,combat=0,level=0,experience=0,consumed=0;
};
// CvGameCoreUtils::isBeforeUnitCycle182-220, exact getter order and polarity.
bool initial_cycle_before(const InitialCycleOperand&a,const InitialCycleOperand&b)noexcept{
 if(!a.raw)return false; // actual GetPlayerUnit NULL, native utility returns false.
 if(a.owner!=b.owner)return a.owner<b.owner;
 if(a.domain!=b.domain)return a.domain<b.domain;
 if(a.combat!=b.combat)return a.combat>b.combat;
 if(a.type!=b.type)return a.type>b.type;
 if(a.level!=b.level)return a.level>b.level;
 if(a.experience!=b.experience)return a.experience>b.experience;
 return a.id<b.id;
}
void initial_cycle_current(const LiveSessionState&s,const std::vector<InitialCycleOperand>&entries){
 for(const auto&entry:entries){const auto*actual=native_imported_get_at(s,entry.query_owner,entry.query_id);
  need(actual==entry.raw,"Initial addUnit actual compared native slot changed");if(!actual)continue;
  need(core(*actual,"m_eOwner")==entry.owner,"Initial addUnit reached owner getter changed");
  if(entry.consumed<2)continue;
  const auto&owned=s.maintenance.players.at(entry.query_owner).units.at(narrow(core(*actual,"m_iID")));
  need(owned.type==entry.type&&owned.domain==entry.domain,"Initial addUnit reached UnitInfo domain changed");
  if(entry.consumed<3)continue;
  need(core(*actual,"m_bEmbarked")==0&&core(*actual,"m_iBaseCombat")==entry.combat,"Initial addUnit reached native basecombat changed");
  if(entry.consumed<5)continue;
  need(core(*actual,"m_iLevel")==entry.level,"Initial addUnit reached level changed");
  if(entry.consumed<6)continue;
  need(core(*actual,"m_iExperience")==entry.experience,"Initial addUnit reached experience changed");
  if(entry.consumed>=7)need(core(*actual,"m_iID")==entry.id,"Initial addUnit reached fullID getter changed");
 }
}
template<class Plot>auto&initial_plot_roster(Plot&plot){
 using A=std::conditional_t<std::is_const_v<Plot>,const saves::PlotArrayObservation,saves::PlotArrayObservation>;
 A*found=nullptr;for(auto&a:plot.arrays)if(name(a.name)=="units"){need(!found,"Initial addUnit duplicate raw plot list");found=&a;}
 need(found&&found->columns==2&&found->values.size()%2==0,"Initial addUnit raw plot list unavailable");return found->values;
}
template<class Record>auto&initial_area_scalar(Record&record,std::string_view key){
 using F=std::conditional_t<std::is_const_v<Record>,const saves::NamedSaveInteger,saves::NamedSaveInteger>;
 F*found=nullptr;for(auto&f:record.fields)if(name(f.name)==key){need(!found,"Initial area duplicate scalar");found=&f;}
 need(found,"Initial area required scalar unavailable");return found->value;
}
template<class Record>auto&initial_area_players(Record&record){
 using A=std::conditional_t<std::is_const_v<Record>,const saves::PlotArrayObservation,saves::PlotArrayObservation>;
 A*found=nullptr;for(auto&a:record.arrays)if(name(a.name)=="units_per_player"){need(!found,"Initial area duplicate owner counts");found=&a;}
 need(found&&found->columns==1&&found->values.size()==80,"Initial area actual80 owner counts unavailable");return found->values;
}
template<class State>auto&initial_destination_area(State&s,std::uint64_t plot){
 auto&w=s.world;need(w.map_regions&&w.map_regions->layout==bnw&&w.map_regions->dimensions&&
  w.map_regions->dimensions->fixed_players==80&&w.map_plots&&plot<w.map_plots->plots.size(),"Initial area decoded BNW source unavailable");
 const auto&target=w.map_plots->plots.at(static_cast<std::size_t>(plot));const auto id=initial_area_scalar(target,"area");
 need(id>=0&&id<=INT32_MAX,"Initial destination real area ID invalid");
 using C=std::conditional_t<std::is_const_v<State>,const saves::MapRegionContainer,saves::MapRegionContainer>;
 C*container=nullptr;for(auto&a:w.map_regions->containers)if(name(a.name)=="areas"){need(!container,"Initial area duplicate container");container=&a;}
 need(container&&container->wire_count>=0&&static_cast<std::uint64_t>(container->wire_count)==container->records.size(),"Initial area container unavailable");
 using R=std::conditional_t<std::is_const_v<State>,const saves::MapRegionRecord,saves::MapRegionRecord>;
 R*found=nullptr;for(auto&r:container->records)if(initial_area_scalar(r,"id")==id){need(!found,"Initial destination duplicate area ID");found=&r;}
 need(found,"Initial destination actual area absent");return *found;
}
bool initial_same_handles(const std::vector<saves::SavedPlotUnitReference>&a,const std::vector<saves::SavedPlotUnitReference>&b)noexcept{
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i)if(a[i].owner!=b[i].owner||a[i].id!=b[i].id)return false;
 return true;
}
struct InitialPlotAreaPrefix {
 std::uint64_t plot=0,before_revision=0,after_revision=0;std::int32_t owner=-1,id=-1,type=-1,domain=-1,combat=0,level=0,experience=0;
 const saves::PlotObservation*raw_plot=nullptr;const void*projected_plot=nullptr;const saves::MapRegionRecord*area=nullptr;
 std::vector<std::int64_t>*raw=nullptr;std::vector<saves::SavedPlotUnitReference>*projected=nullptr;
 std::vector<std::int64_t>raw_before,raw_after,raw_staging;
 std::vector<saves::SavedPlotUnitReference>projected_before,projected_after,projected_staging;
 std::int64_t*area_units=nullptr;std::vector<std::int64_t>*area_players=nullptr;
 std::vector<std::int64_t>area_players_before,area_players_after;
 std::int64_t area_id=-1,units_before=0,owner_before=0;
 std::vector<InitialCycleOperand>compared;
 std::vector<std::pair<std::int32_t,std::vector<std::int64_t>>>managers;
 bool stored=false;
};
struct InitialPostCityFeature {bool natural=false,promotion_valid=false;std::int32_t promotion=-1;std::uint32_t hash=0;std::string promotion_type;};
struct InitialPostCityPrefix {
 std::vector<std::uint64_t> name_successor_seal,promotion_successor_seal,promotion_loop_seal,traits_promotion_seal,player_promotion_seal,embark_promotion_seal,exotic_xp_seal,immobile_moves_seal,set_moves_seal,cycle_order_seal,army_ai_seal;
 std::int32_t type=-1,domain=-1,unit_class=-1,combat_class=-1,missionary=-1;
 std::array<std::optional<bool>,64> human;
 std::map<std::int32_t,InitialPostCityFeature> features;
 std::vector<std::uint64_t> seal;
 std::uint64_t revision=0,meet_returns=0,adjacent_plots=0;
 bool stored=false;
};
template<class Rows>const auto&post_named(const Rows&rows,std::string_view key){
 const typename Rows::value_type*found=nullptr;
 for(const auto&v:rows)if(name(v.name)==key){need(!found,"Post-city duplicate native operand");found=&v;}
 if(!found)throw BnwNativeUnitAllocatorUnavailable(std::string("Post-city native operand absent: ")+std::string(key));return *found;
}
std::int32_t post_player(const LiveSessionState&s,std::int32_t id,std::string_view key){
 need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&id>=0&&id<64,
  "Post-city actual native player roster absent");
 const auto&p=s.world.all_players->players.at(static_cast<std::size_t>(id));
 const auto&f=post_named(p.core.fields,key);
 need(p.index==static_cast<std::uint64_t>(id)&&p.saved_player_id==id&&p.core.layout==bnw&&f.values.size()==1&&
  f.encoding==(key=="m_bAlive"?saves::PlayerCoreEncoding::boolean:saves::PlayerCoreEncoding::i32)&&
  (key!="m_bAlive"||(f.values[0]==0||f.values[0]==1)),
  "Post-city actual player storage/schema differs");return narrow(f.values[0]);
}
std::int32_t post_player_team(const LiveSessionState&s,std::int32_t id){
 need(id>=0&&id<64&&s.world.pregame&&s.world.pregame->layout==bnw&&post_player(s,id,"m_eID")==id,
  "Post-city actual player/team identity differs");
 const std::vector<std::int64_t>*teams=nullptr;
 for(const auto&[key,value]:s.world.pregame->archive.integer_arrays)if(std::string_view(key)=="team_types")teams=&value;
 need(teams&&teams->size()==64,"Post-city actual Pregame team assignments absent");
 const auto team=narrow(teams->at(static_cast<std::size_t>(id)));const auto mirror=s.setup.players.find(id);
 need(team>=-1&&team<64&&mirror!=s.setup.players.end()&&mirror->second.team==team,"Post-city paired team assignment differs");return team;
}
std::int32_t post_active_team(const LiveSessionState&s){
 need(s.world.pregame.has_value(),"Post-city actual active Game Pregame absent");
 const std::int64_t*value=nullptr;for(const auto&[key,n]:s.world.pregame->archive.integers)if(std::string_view(key)=="active_player")value=&n;
 need(value&&*value>=-1&&*value<64,"Post-city actual active player domain unavailable");
 return *value==-1?-1:post_player_team(s,narrow(*value));
}
const saves::TeamObservation&post_team(const LiveSessionState&s,std::int32_t id){
 need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&id>=0&&id<64,
  "Post-city actual team roster absent");
 const auto&t=s.world.teams->teams.at(static_cast<std::size_t>(id));
 need(t.index==static_cast<std::uint64_t>(id)&&post_named(t.fields,"id").value==id,"Post-city team ID differs");return t;
}
bool post_relation(const LiveSessionState&s,std::int32_t from,std::int32_t to,std::string_view key){
 need(to>=0&&to<64,"Post-city relation native team domain invalid");
 const auto&a=post_named(post_team(s,from).arrays,key);
 need(a.columns==1&&a.values.size()==64,"Post-city actual native relation array absent");
 const auto value=a.values.at(static_cast<std::size_t>(to));
 need(value==0||value==1,"Post-city relation native bool invalid");return value!=0;
}
template<class Emit>void initial_post_city_semantics(const LiveSessionState&s,std::int32_t owner,
 std::int32_t x,std::int32_t y,const std::map<std::string,std::int32_t,std::less<>>&scalars,
 const std::map<std::string,std::vector<std::int32_t>,std::less<>>&indexed,Emit emit){
 const auto text=[&](std::string_view v){emit(v.size());for(char c:v)emit(static_cast<unsigned char>(c));};
 const auto opt=[&](const auto&v){emit(v.has_value());if(v)emit(*v);};
 const auto&w=s.world;need(w.pregame&&w.pregame->layout==bnw&&w.pregame->slots_version==3&&w.pregame->archive_version==6,
  "Post-city native Pregame source absent");
 const auto&preg=*w.pregame;emit(reinterpret_cast<std::uintptr_t>(&preg));
 emit(preg.archive.integers.size());for(const auto&[k,v]:preg.archive.integers){text(k);emit(v);}
 for(const auto key:{"team_types","slot_status","slot_claims"}){
  const std::vector<std::int64_t>*v=nullptr;for(const auto&[k,a]:preg.archive.integer_arrays)if(std::string_view(k)==key)v=&a;
  need(v&&v->size()==64,"Post-city native Pregame64 array absent");for(auto n:*v)emit(n);
 }
 emit(preg.game_options.size());for(const auto&o:preg.game_options){text(o.name);emit(o.value);}
 emit(s.setup.game_type);emit(s.setup.players.size());
 for(const auto&[id,p]:s.setup.players){emit(id);emit(p.team);emit(p.slot_status);emit(p.slot_claim);}
 for(std::int32_t id=0;id<64;++id){
  emit(post_player(s,id,"m_eID"));emit(post_player(s,id,"m_bAlive"));
  const auto&t=post_team(s,id);emit(reinterpret_cast<std::uintptr_t>(&t));emit(post_named(t.fields,"alive_count").value);
  for(const auto key:{"has_met","at_war"}){const auto&a=post_named(t.arrays,key);emit(a.columns);emit(a.values.size());for(auto v:a.values)emit(v);}
 }
 const auto&grid=w.map_header.grid;MapGeometry geometry{grid,w.map_header.wrap_x,w.map_header.wrap_y,bnw};geometry.validate();
 need(w.map_header.layout==bnw&&w.map_plots&&w.map_projection&&w.map_plots->plots.size()==grid.size()&&w.map_projection->cells.size()==grid.size(),
  "Post-city actual map shape absent");
 emit(grid.width);emit(grid.height);emit(w.map_header.wrap_x);emit(w.map_header.wrap_y);emit(owner);emit(x);emit(y);
 const auto plot=geometry.plot(x,y);need(plot.has_value(),"Post-city target outside native map");
 const auto&cache=s.plot_visibility;emit(cache.current);emit(cache.grid.width);emit(cache.grid.height);emit(cache.team_slots);emit(cache.wire_slots);
 need(grid.size()<=SIZE_MAX/80&&cache.current&&cache.team_slots==64&&cache.wire_slots==80&&cache.grid.width==grid.width&&cache.grid.height==grid.height&&
  cache.source_raw.size()==grid.size()*80&&cache.counts.size()==cache.source_raw.size(),"Post-city actual visibility source/cache absent");
 const auto cell=[&](std::uint64_t slot,bool target){
  const auto&raw=w.map_plots->plots.at(static_cast<std::size_t>(slot));const auto&c=w.map_projection->cells.at(static_cast<std::size_t>(slot));
  const auto xy=grid.coordinates(slot);need(raw.index==slot&&c.index==slot&&raw.x==xy[0]&&raw.y==xy[1]&&c.x==xy[0]&&c.y==xy[1],"Post-city actual plot identity changed");
  emit(reinterpret_cast<std::uintptr_t>(&raw));emit(reinterpret_cast<std::uintptr_t>(&c));emit(slot);
  for(const auto key:{"owner","plot_type","terrain_type","feature_type_hash","plot_city_owner","plot_city_id"})emit(post_named(raw.fields,key).value);
  opt(c.owner);opt(c.plot_type);opt(c.terrain_type);opt(c.feature_type);opt(c.feature_type_hash);emit(c.is_city);
  emit(c.plot_city.has_value());if(c.plot_city){emit(c.plot_city->owner);emit(c.plot_city->id);}
  need(c.owner==std::optional<std::int64_t>{post_named(raw.fields,"owner").value}&&
   c.plot_type==std::optional<std::int64_t>{post_named(raw.fields,"plot_type").value}&&
   c.terrain_type==std::optional<std::int64_t>{post_named(raw.fields,"terrain_type").value},"Post-city paired native plot operands differ");
  if(target){const auto&a=post_named(raw.arrays,"visibility");need(a.columns==1&&a.values.size()==80,"Post-city target visibility wire extent absent");
   for(std::size_t i=0;i<80;++i){const auto pos=static_cast<std::size_t>(slot*80)+i;emit(a.values[i]);emit(cache.source_raw[pos]);emit(cache.counts[pos]);
    need(cache.source_raw[pos]==a.values[i]&&cache.counts[pos]>=0,"Post-city actual target visibility cache stale");}
  }
 };
 cell(static_cast<std::uint64_t>(*plot),true);
 for(std::int64_t dir=0;dir<6;++dir){const auto next=geometry.direction(x,y,dir);opt(next);if(next)cell(static_cast<std::uint64_t>(*next),false);}
 emit(scalars.size());for(const auto&[k,v]:scalars){text(k);emit(v);}
 emit(indexed.size());for(const auto&[k,v]:indexed){text(k);emit(v.size());for(auto n:v)emit(n);}
}
void initial_post_city_compare(const LiveSessionState&s,std::int32_t actor,std::int32_t x,std::int32_t y,
 const std::map<std::string,std::int32_t,std::less<>>&scalars,
 const std::map<std::string,std::vector<std::int32_t>,std::less<>>&indexed,const InitialPostCityPrefix&q){
 std::size_t at=0;initial_post_city_semantics(s,actor,x,y,scalars,indexed,[&](auto value){
  need(at<q.seal.size()&&q.seal[at]==static_cast<std::uint64_t>(value),"Post-city relevant source/node operands changed");++at;});
 need(at==q.seal.size(),"Post-city semantic source extent changed");
}
std::int32_t post_info(const ui::SelectedBnwPreviewHost&h,std::string_view type,std::string_view table){
 const auto*e=h.types().find(type);need(e&&e->table==table&&e->id>=0,"Post-city actual selected Info binding absent");return narrow(e->id);
}
InitialPostCityPrefix initial_post_city_rules(const ui::SelectedBnwPreviewHost&h,const LiveSessionState&s,std::int32_t actor,std::int32_t type){
 InitialPostCityPrefix q;q.type=type;
 bool found=false;
 const auto unit=h.database().query_rows({"SELECT Type,Domain,Class,CombatClass FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&r){
  need(!found,"Post-city duplicate actual UnitInfo");found=true;const auto name=r.text("Type"),domain=r.text("Domain"),klass=r.text("Class"),combat=r.text("CombatClass");
  need(name&&domain&&klass&&post_info(h,*name,"Units")==type,"Post-city actual UnitInfo source differs");
  q.domain=post_info(h,*domain,"Domains");q.unit_class=post_info(h,*klass,"UnitClasses");
  if(combat)q.combat_class=post_info(h,*combat,"UnitCombatInfos");
 });need(unit.all_succeeded()&&found,"Post-city actual UnitInfo query failed");
 if(const auto*e=h.types().find("UNITCLASS_MISSIONARY")){need(e->table=="UnitClasses","Post-city missionary binding wrong table");q.missionary=narrow(e->id);}
 // CvTeam::isHuman: alive players in ascending native slots, first human wins.
 const auto human=[&](std::int32_t id){auto&v=q.human.at(static_cast<std::size_t>(id));if(!v)v=detail::bnw_player_human_read(s,id,h.database());return *v;};
 const bool ours=human(actor);
 for(std::int32_t team=0;team<63;++team)if(post_named(post_team(s,team).fields,"alive_count").value>0&&ours){
  for(std::int32_t id=0;id<64;++id)if(post_player(s,id,"m_bAlive")!=0&&post_player_team(s,id)==team&&human(id))break;
 }
 const auto features=h.database().query_rows({"SELECT ID,Type,NaturalWonder,AdjacentUnitFreePromotion FROM Features ORDER BY ID",{}},[&](const auto&r){
  const auto id=narrow(r.integer_at(0));const auto type_name=r.text("Type"),promotion=r.text("AdjacentUnitFreePromotion");const auto wonder=r.integer_at(2);
  need(type_name&&post_info(h,*type_name,"Features")==id&&(wonder==0||wonder==1)&&!q.features.contains(id),"Post-city FeatureInfo table/bool binding invalid");
  InitialPostCityFeature f;f.natural=wonder!=0;f.hash=h.types().find(*type_name)->hash;
  if(promotion){const auto*e=h.types().find(*promotion);if(e){need(e->table=="UnitPromotions","Post-city feature promotion binding wrong table");f.promotion=narrow(e->id);f.promotion_type=*promotion;}}
  q.features.emplace(id,std::move(f));return true;
 });need(features.all_succeeded(),"Post-city actual FeatureInfo query failed");
 for(auto&[id,f]:q.features){(void)id;
  if(f.natural&&f.promotion!=-1&&q.combat_class!=-1){
   const auto relations=h.database().query_rows({"SELECT UnitCombatType FROM UnitPromotions_UnitCombats WHERE PromotionType=?",{f.promotion_type}},[&](const auto&row){
    const auto c=row.text("UnitCombatType");need(c.has_value(),"Post-city native promotion combat relation NULL");
    const auto combat=post_info(h,*c,"UnitCombatInfos");if(combat==q.combat_class)f.promotion_valid=true;
   });need(relations.all_succeeded(),"Post-city native promotion combat relations query failed");
  }
 }
 for(const auto&[key,e]:h.types().entries()){(void)key;if(e.table=="Features")need(q.features.contains(narrow(e.id)),"Post-city complete FeatureInfo entry missing");}
 return q;
}
std::pair<std::uint64_t,std::uint64_t>initial_post_city_prove(const LiveSessionState&s,const ui::SelectedBnwPreviewHost&h,
 std::int32_t actor,std::int32_t city,std::int32_t x,std::int32_t y,
 const std::map<std::string,std::int32_t,std::less<>>&node,const std::map<std::string,std::vector<std::int32_t>,std::less<>>&indexed,
 const InitialPostCityPrefix&q){
 const auto scalar=[&](std::string_view k){const auto it=node.find(k);need(it!=node.end(),"Post-city actual pending scalar absent");return it->second;};
 need(scalar("m_eOwner")==actor&&scalar("m_eUnitType")==q.type&&scalar("m_iX")==x&&scalar("m_iY")==y&&scalar("m_iMapLayer")==0,
  "Post-city actual pending node identity/coordinate/layer differs");
 const auto team=post_player_team(s,actor);need(team>=0,"Post-city actual actor team absent");
 const auto&grid=s.world.map_header.grid;MapGeometry geometry{grid,s.world.map_header.wrap_x,s.world.map_header.wrap_y,bnw};
 const auto slot=geometry.plot(x,y);need(slot.has_value(),"Post-city actual destination absent");
 const auto&target=s.world.map_projection->cells.at(static_cast<std::size_t>(*slot));
 need(target.is_city&&target.plot_city&&target.plot_city->owner==actor&&target.plot_city->id==city,
  "Post-city own actual city binding changed");
 // On this genuine own city isAlwaysHostile is false before its count; actual
 // combatOwner remains owner. isEnemy reaches our native atWar(cityTeam).
 need(!post_relation(s,team,team,"at_war"),"Mandatory actual enemy-city acquire owner required");
 need(scalar("m_transportUnit.owner")==-1&&scalar("m_transportUnit.id")==-1,
  "Post-city actual transport binding is not native Reset NULL");
 need(q.domain==2&&target.plot_type&&*target.plot_type!=3,"Mandatory actual load-on-move domain/water transport owner required");
 need(target.terrain_type&&*target.terrain_type>=0,"Post-city actual destination terrain absent");
 const auto count=indexed.find(std::string_view("m_terrainImpassableCount"));need(count!=indexed.end()&&static_cast<std::uint64_t>(*target.terrain_type)<count->second.size(),
  "Post-city native terrain impassability span absent");
 need(count->second.at(static_cast<std::size_t>(*target.terrain_type))<=0,"Mandatory native terrain-passable promotions/load owner required");
 const auto human=[&](std::int32_t id){const auto&v=q.human.at(static_cast<std::size_t>(id));need(v.has_value(),"Post-city reached human source read absent");return *v;};
 std::uint64_t returns=0,neighbors=0;
 const auto meet=[&](std::int32_t from,std::int32_t to){
  // No special self-team bypass exists in CvTeam::meet. Its actual has_met
  // first predicate must hold, otherwise makeHasMet/AI/TeamMeet are mandatory.
  need(post_relation(s,from,to,"has_met"),"Mandatory first-contact makeHasMet/AI/TeamMeet owner required");++returns;
 };
 for(std::int32_t loop=0;loop<63;++loop){
  if(post_named(post_team(s,loop).fields,"alive_count").value<=0)continue;
  if(human(actor)){
   bool team_human=false;for(std::int32_t id=0;id<64;++id){
    const auto alive=post_player(s,id,"m_bAlive");need(alive==0||alive==1,"Post-city actual player alive bool invalid");
    if(alive&&post_player_team(s,id)==loop&&human(id)){team_human=true;break;}
   }if(!team_human)continue;
  }
  // Exact isInvisible(loop,false): same-team first; then genuine NULL cargo,
  // then NO_INVISIBLE. No invented visibility for unsupported native type.
  if(loop!=team)need(scalar("m_eInvisibleType")==-1,"Mandatory native nonzero invisible-visibility owner required");
  const auto position=static_cast<std::size_t>(static_cast<std::uint64_t>(*slot)*80+static_cast<std::uint64_t>(loop));
  if(s.plot_visibility.counts.at(position)>0)meet(loop,team);
 }
 const auto active=post_active_team(s);
 need(team==active||scalar("m_eInvisibleType")==-1,"Mandatory renderer changed invisible visibility owner required");
 for(std::int64_t direction=0;direction<6;++direction){
  const auto next=geometry.direction(x,y,direction);if(!next)continue;++neighbors;
  const auto&cell=s.world.map_projection->cells.at(static_cast<std::size_t>(*next));
  need(cell.owner.has_value()&&*cell.owner>=-1&&*cell.owner<64,"Post-city actual adjacent owner absent");
  if(*cell.owner!=-1){const auto other=post_player_team(s,narrow(*cell.owner));if(other!=-1)meet(team,other);}
  need(actor!=63,"Mandatory actual adjacent barbarian naval/missionary conversion owner required");
  need(q.missionary==-1||q.unit_class!=q.missionary,"Mandatory actual missionary religion conversion owner required");
  std::int32_t feature=-1;
  if(cell.feature_type){feature=narrow(*cell.feature_type);need(feature>=-1,"Post-city adjacent native feature ID invalid");}
  else {need(cell.feature_type_hash.has_value(),"Post-city adjacent native feature hash unresolved");
   if(*cell.feature_type_hash){const auto*e=h.types().find_hash(*cell.feature_type_hash);need(e&&e->table=="Features","Post-city adjacent native FeatureInfo hash unresolved");feature=narrow(e->id);}
  }
  const auto raw_hash=post_named(s.world.map_plots->plots.at(static_cast<std::size_t>(*next)).fields,"feature_type_hash").value;
  need(raw_hash>=0&&raw_hash<=UINT32_MAX,"Post-city adjacent native feature hash domain invalid");
  if(feature!=-1){const auto f=q.features.find(feature);need(f!=q.features.end()&&f->second.hash==static_cast<std::uint64_t>(raw_hash),"Post-city adjacent actual FeatureInfo NULL/hash differs");
   if(f->second.natural&&f->second.promotion!=-1&&q.combat_class!=-1)
    need(!f->second.promotion_valid,"Mandatory native adjacent natural-wonder setHasPromotion owner required");
  }else need(raw_hash==0,"Post-city adjacent NO_FEATURE raw hash differs");
 }
 // Newborn source has genuine INVALID old plot, so no old city, cargo
 // relocation or naval old-plot conversion. Actual initialized graphics next
 // calls SetPosition(newPlot), not an animation or an empty callback ACK.
 return {returns,neighbors};
}
}

struct InitialHappinessPrefix {
 std::array<BnwHappinessNativeStore,6> stores;
 saves::PlayerCoreField*field=nullptr;std::int32_t before=0,after=0;
 std::uint64_t before_revision=0,after_revision=0;
 std::vector<std::uint64_t> inputs;bool stored=false;
 bool founder_attempted=false,founder_inflight=false,founder_returned=false,religion_stored=false;
 std::int32_t founder_religion=-1;
 std::array<BnwHappinessSuffixStore,5> suffix_stores{};
 saves::PlayerCoreField* connection_field=nullptr;
 std::int32_t connection_before=0,connection_after=0,suffix_after=0;
 std::uint64_t suffix_before_revision=0,suffix_after_revision=0;
 bool suffix_attempted=false,suffix_stored=false;
 bool excess_stored=false,full_returned=false,cache_current_restored=false,achievement_requested=false;
 bool unit_setxy_attempted=false,unit_setxy_inflight=false,unit_setxy_returned=false;
 std::int32_t excess=0;std::uint64_t full_before_revision=0,full_after_revision=0,game_data_occurrence=0;
 std::vector<std::uint64_t> full_inputs;
};
template<class Emit>void initial_happiness_fields(const auto&fields,Emit&emit,const saves::PlayerCoreField*projected=nullptr,const std::vector<std::int64_t>*values=nullptr,const saves::NamedSaveInteger*game_projected=nullptr,std::optional<std::int32_t>game_value={}){
 emit(fields.size());for(const auto&f:fields){
  const auto text=[&](std::string_view key){emit(key.size());for(auto c:key)emit(static_cast<unsigned char>(c));};
  text(name(f.name));
  if constexpr(requires{f.encoding;}){if constexpr(std::is_enum_v<std::remove_cvref_t<decltype(f.encoding)>>)emit(static_cast<std::uint64_t>(f.encoding));else text(name(f.encoding));}
  if constexpr(requires{f.value;}){if constexpr(std::is_same_v<std::remove_cvref_t<decltype(f)>,saves::NamedSaveInteger>)emit(game_projected==&f&&game_value?*game_value:f.value);else emit(f.value);}
  if constexpr(requires{f.values;}){
   if constexpr(std::is_same_v<std::remove_cvref_t<decltype(f)>,saves::PlayerCoreField>){
    const auto&actual=projected==&f&&values?*values:f.values;emit(actual.size());for(auto x:actual)emit(x);
   }else{emit(f.values.size());for(auto x:f.values)emit(x);}
  }
  if constexpr(requires{f.bytes;}){emit(f.bytes.size());for(auto x:f.bytes)emit(x);}
  if constexpr(requires{f.hashes;}){emit(f.hashes.size());for(auto x:f.hashes)emit(x);}
  if constexpr(requires{f.row_counts;}){emit(f.row_counts.size());for(auto x:f.row_counts)emit(x);}
  if constexpr(requires{f.row_keys;}){emit(f.row_keys.size());for(auto x:f.row_keys)emit(x);}
  if constexpr(requires{f.wire_count.has_value();}){emit(f.wire_count.has_value());if(f.wire_count)emit(*f.wire_count);}
  else if constexpr(requires{f.wire_count;})emit(f.wire_count);
  if constexpr(requires{f.columns;})emit(f.columns);
  if constexpr(requires{f.rows;}){emit(f.rows.size());for(const auto&r:f.rows){
   if constexpr(requires{r.key;}){emit(r.key);}
   if constexpr(requires{r.type_hash;}){emit(r.type_hash);}
   if constexpr(requires{r.wire_count;}){emit(r.wire_count);}
   emit(r.values.size());for(auto x:r.values)emit(x);}}
 }
}
template<class Emit>void initial_happiness_records(const auto&records,Emit&emit){
 emit(records.size());for(const auto&r:records){const auto key=name(r.name);emit(key.size());for(auto c:key)emit(static_cast<unsigned char>(c));
  if constexpr(requires{r.fields;})initial_happiness_fields(r.fields,emit);
  if constexpr(requires{r.arrays;}){emit(r.arrays.size());for(const auto&a:r.arrays){const auto n=name(a.name);emit(n.size());for(auto c:n)emit(static_cast<unsigned char>(c));emit(a.columns);emit(a.values.size());for(auto x:a.values)emit(x);}}
  if constexpr(requires{r.hashes;})initial_happiness_fields(r.hashes,emit);
  if constexpr(requires{r.children;})initial_happiness_records(r.children,emit);
 }
}
template<class Emit>void initial_happiness_semantics(const LiveSessionState&s,std::int32_t actor,Emit emit,std::optional<std::int32_t> excess={},const saves::PlayerCoreField*projected=nullptr,const std::vector<std::int64_t>*values=nullptr){
 need(s.world.all_players&&s.world.pregame,"Initial happiness actual world/Pregame missing");
 const auto&p=s.world.all_players->players.at(static_cast<std::size_t>(actor));emit(reinterpret_cast<std::uintptr_t>(&p));emit(p.saved_player_id);
 initial_happiness_fields(p.core.fields,emit,projected,values);initial_happiness_fields(p.traits.fields,emit);initial_happiness_records(p.strategy.blocks,emit);
 emit(static_cast<std::uint64_t>(s.setup.profile));emit(static_cast<std::uint64_t>(s.cities.profile));
 emit(static_cast<std::uint64_t>(p.core.layout));emit(static_cast<std::uint64_t>(p.traits.layout));emit(static_cast<std::uint64_t>(p.strategy.layout));emit(static_cast<std::uint64_t>(p.cities.layout));
 emit(p.cities.header.wire_city_count);emit(p.cities.cities.size());for(const auto&c:p.cities.cities){emit(reinterpret_cast<std::uintptr_t>(&c));emit(c.index);emit(static_cast<std::uint64_t>(c.core.layout));emit(c.core.start_offset);emit(c.core.end_offset);
  emit(c.core.version.has_value());if(c.core.version)emit(*c.core.version);emit(static_cast<std::uint64_t>(c.production.layout));
  emit(c.production.context.bnw_city_version.has_value());if(c.production.context.bnw_city_version)emit(*c.production.context.bnw_city_version);
  initial_happiness_fields(c.core.fields,emit);initial_happiness_records(c.production.blocks,emit);initial_happiness_records(c.tail.blocks,emit);}
 for(const auto&[key,values]:s.world.pregame->archive.integer_arrays){emit(key.size());for(auto c:key)emit(static_cast<unsigned char>(c));emit(values.size());for(auto n:values)emit(n);}
 for(const auto&[key,values]:s.world.pregame->slots.integer_arrays){emit(key.size());for(auto c:key)emit(static_cast<unsigned char>(c));emit(values.size());for(auto n:values)emit(n);}
 const auto&setup=s.setup.players.at(actor);emit(setup.handicap);emit(setup.team);emit(setup.alive);emit(setup.policies_resolved);emit(setup.policies.size());for(auto n:setup.policies)emit(n);
 const auto&cities=s.cities.players.at(actor);emit(cities.civilization);emit(cities.capital);emit(excess.has_value()||cities.excess_happiness.has_value());if(excess)emit(*excess);else if(cities.excess_happiness)emit(*cities.excess_happiness);
 emit(cities.cities.size());for(const auto&[id,c]:cities.cities){emit(id);emit(c.id);emit(c.owner);emit(c.population);emit(c.puppet);emit(c.building_counts_resolved);
  emit(c.source_core.has_value());if(c.source_core){emit(c.source_core->first);emit(c.source_core->second);}
  emit(c.real_buildings.size());for(const auto&[k,n]:c.real_buildings){emit(k);emit(n);}emit(c.free_buildings.size());for(const auto&[k,n]:c.free_buildings){emit(k);emit(n);}}
 emit(s.unhappiness.players.at(actor).happiness);emit(static_cast<std::uint64_t>(s.world.leagues.layout));emit(s.world.leagues.leagues.size());
 for(const auto&l:s.world.leagues.leagues){emit(l.members.size());for(const auto&m:l.members)emit(m.player);
  emit(l.active_resolutions.size());for(const auto&r:l.active_resolutions){emit(r.proposer_decision.vote.choice);emit(r.effects.flags.size());for(const auto&f:r.effects.flags){const auto k=name(f.name);emit(k.size());for(auto c:k)emit(static_cast<unsigned char>(c));emit(f.value);}}}
}
void initial_happiness_match(const LiveSessionState&s,std::int32_t actor,const std::vector<std::uint64_t>&seal){
 std::size_t at=0;initial_happiness_semantics(s,actor,[&](auto n){need(at<seal.size()&&seal[at]==static_cast<std::uint64_t>(n),"Initial happiness actual native input changed in place");++at;});
 need(at==seal.size(),"Initial happiness native input extent changed");
}
template<class Emit>void initial_happiness_suffix_semantics(const LiveSessionState&s,std::int32_t actor,Emit emit,std::optional<std::int32_t> excess={},const saves::PlayerCoreField*projected=nullptr,const std::vector<std::int64_t>*values=nullptr){
 initial_happiness_semantics(s,actor,emit,excess,projected,values);
 need(s.world.all_players&&s.world.teams&&s.world.map_projection&&s.world.pregame,"Initial happiness suffix source domains absent");
 emit(static_cast<std::uint64_t>(s.world.all_players->layout));emit(s.world.all_players->players.size());
 for(const auto&raw:s.world.all_players->players){emit(raw.index);emit(raw.saved_player_id);emit(static_cast<std::uint64_t>(raw.core.layout));
  initial_happiness_fields(raw.core.fields,emit,projected,values);emit(static_cast<std::uint64_t>(raw.planning.layout));emit(raw.planning.dimensions.major);initial_happiness_records(raw.planning.blocks,emit);}
 emit(s.setup.players.size());for(const auto&[id,p]:s.setup.players){emit(id);emit(p.team);emit(p.alive);}
 emit(s.treasury.players.size());for(const auto&[id,p]:s.treasury.players){emit(id);emit(p.ally);}
 emit(s.maintenance.team_eras.size());for(const auto&[id,era]:s.maintenance.team_eras){emit(id);emit(era);}
 emit(s.player_research.teams.size());for(const auto&[id,t]:s.player_research.teams){emit(id);emit(t.at_war.size());for(auto other:t.at_war)emit(other);}
 emit(static_cast<std::uint64_t>(s.world.teams->layout));emit(s.world.teams->teams.size());
 for(const auto&t:s.world.teams->teams){emit(t.index);initial_happiness_fields(t.fields,emit);initial_happiness_fields(t.arrays,emit);}
 const auto&map=*s.world.map_projection;emit(map.grid.width);emit(map.grid.height);emit(s.world.map_header.grid.width);emit(s.world.map_header.grid.height);emit(map.cells.size());
 for(const auto&c:map.cells){emit(c.index);emit(c.x);emit(c.y);emit(c.owner.has_value());if(c.owner)emit(*c.owner);
  emit(c.feature_type.has_value());if(c.feature_type)emit(*c.feature_type);emit(c.feature_type_hash.has_value());if(c.feature_type_hash)emit(*c.feature_type_hash);}
 for(const auto&[key,values]:s.world.pregame->archive.string_arrays){emit(key.size());for(auto c:key)emit(static_cast<unsigned char>(c));emit(values.size());for(const auto&str:values){emit(str.size());for(auto c:str)emit(static_cast<unsigned char>(c));}}
 const auto&cities=s.cities.players.at(actor);emit(cities.capital);emit(cities.cities.size());
 for(const auto&[id,c]:cities.cities){emit(id);emit(c.id);emit(c.owner);emit(c.x);emit(c.y);emit(c.source_core.has_value());if(c.source_core){emit(c.source_core->first);emit(c.source_core->second);}}
 need(s.world.map_plots.has_value(),"Initial happiness suffix actual plot source absent");emit(static_cast<std::uint64_t>(s.world.map_plots->layout));emit(s.world.map_plots->plots.size());
 for(const auto&plot:s.world.map_plots->plots){emit(plot.index);initial_happiness_fields(plot.fields,emit);}
}
void initial_happiness_suffix_match(const LiveSessionState&s,std::int32_t actor,const std::vector<std::uint64_t>&seal){
 std::size_t at=0;initial_happiness_suffix_semantics(s,actor,[&](auto n){need(at<seal.size()&&seal[at]==static_cast<std::uint64_t>(n),"Initial happiness suffix operand changed in place");++at;});
 need(at==seal.size(),"Initial happiness suffix source extent changed");
}
template<class Emit>void initial_happiness_finish_semantics(const LiveSessionState&s,std::int32_t actor,Emit emit,std::optional<std::int32_t> excess={},const saves::PlayerCoreField*projected=nullptr,const std::vector<std::int64_t>*values=nullptr,const saves::NamedSaveInteger*game_projected=nullptr,std::optional<std::int32_t>game_value={}){
 initial_happiness_suffix_semantics(s,actor,emit,excess,projected,values);emit(s.unhappiness.world_cities_percent);emit(s.unhappiness.players.size());
 for(const auto&[id,u]:s.unhappiness.players){emit(id);emit(u.happiness);emit(u.units);emit(u.units_modifier);emit(u.population_modifier);emit(u.cities_modifier);
  emit(u.occupied_modifier);emit(u.capital_modifier);emit(u.trait_cities_modifier);emit(u.trait_population_modifier);emit(u.public_opinion);emit(u.half_specialists);emit(u.cities.size());
  for(const auto&[city,c]:u.cities){emit(city);emit(c.occupied);emit(c.no_occupied_unhappiness);emit(c.ignore);emit(c.specialists.size());for(const auto&[type,count]:c.specialists){emit(type);emit(count);}}}
 initial_happiness_fields(s.world.game_state.controls,emit,nullptr,nullptr,game_projected,game_value);emit(s.world.pregame->archive.integers.size());for(const auto&[key,value]:s.world.pregame->archive.integers){emit(key.size());for(auto c:key)emit(static_cast<unsigned char>(c));emit(value);}
 emit(s.setup.game_handicap);emit(s.setup.game_type);emit(s.setup.pitboss.has_value());if(s.setup.pitboss)emit(*s.setup.pitboss);
 for(const auto&[id,player]:s.setup.players){emit(id);emit(player.handicap);emit(player.slot_status);emit(player.slot_claim);}
 for(const auto&raw:s.world.all_players->players){initial_happiness_fields(raw.traits.fields,emit);emit(static_cast<std::uint64_t>(raw.trade_culture.layout));initial_happiness_records(raw.trade_culture.blocks,emit);emit(raw.cities.cities.size());for(const auto&city:raw.cities.cities){
  initial_happiness_fields(city.core.fields,emit);emit(static_cast<std::uint64_t>(city.management.layout));initial_happiness_records(city.management.blocks,emit);}}
 for(const auto&[id,player]:s.cities.players){emit(id);emit(player.capital);emit(player.cities.size());for(const auto&[city,c]:player.cities){emit(city);emit(c.owner);emit(c.population);emit(c.puppet);}}
}
void initial_happiness_finish_match(const LiveSessionState&s,std::int32_t actor,const std::vector<std::uint64_t>&seal){
 std::size_t at=0;initial_happiness_finish_semantics(s,actor,[&](auto n){need(at<seal.size()&&seal[at]==static_cast<std::uint64_t>(n),"Initial happiness final source changed in place");++at;});
 need(at==seal.size(),"Initial happiness final source extent changed");
}
struct BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch::Storage {
 std::shared_ptr<const BnwNativeUnitAllocatorCore>core;std::shared_ptr<const ui::SelectedCityTrainCreateSource>source;
 BnwStockLuaAttemptLease attempt;std::uint64_t serial=0,generation=0,revision=0;
 std::weak_ptr<const void>bank,node;std::shared_ptr<const LiveSessionState>world;
 std::weak_ptr<const InitialUnitSetXYDispatch>factory_self;std::thread::id thread=std::this_thread::get_id();
 std::int32_t actor=-1,id=-1,x=-1,y=-1;std::atomic<bool>retired{false};
 Storage(std::shared_ptr<const BnwNativeUnitAllocatorCore>c,std::shared_ptr<const ui::SelectedCityTrainCreateSource>s,const BnwStockLuaAttemptLease&a):core(std::move(c)),source(std::move(s)),attempt(a){}
};
struct BnwNativeUnitAllocatorCore::InitialHappinessFounderDispatch::Storage {
 std::shared_ptr<const BnwNativeUnitAllocatorCore> core;
 std::shared_ptr<const ui::SelectedCityTrainCreateSource> source;
 BnwStockLuaAttemptLease attempt;std::uint64_t serial=0,generation=0,revision=0;
 std::weak_ptr<const void> bank,node;
 std::weak_ptr<const InitialHappinessFounderDispatch> factory_self;
 std::shared_ptr<const LiveSessionState> world;std::vector<std::uint64_t> inputs;
 std::int32_t actor=-1;std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};
 Storage(std::shared_ptr<const BnwNativeUnitAllocatorCore> c,std::shared_ptr<const ui::SelectedCityTrainCreateSource> s,
  const BnwStockLuaAttemptLease&lease):core(std::move(c)),source(std::move(s)),attempt(lease){}
};
// Private actual Init descendant. Original city/position/center receipts are
// retained separately; only exact source-derived after-operands are accepted.
struct InitialNameGamePrefix {
 struct CountRow {std::uint64_t hash=0;std::int32_t value=0;};
 saves::ProgressionArray*array=nullptr;saves::ProgressionRow*row=nullptr;
 std::size_t array_index=0,row_index=0;std::uint64_t hash=0;
 std::vector<CountRow> rows;
 std::int32_t before=0,after=0,turn=0;
 std::uint64_t before_revision=0,after_revision=0;
 std::shared_ptr<PendingUnit> node;
 std::map<std::string,std::int32_t,std::less<>> node_before,node_after;
 std::set<std::size_t> dirty_after;
 bool stored=false;
};
struct InitialClassCountRow {std::uint64_t hash=0;std::optional<std::int32_t>value;};
struct InitialGameClassPrefix {
 saves::ProgressionArray*array=nullptr;saves::ProgressionRow*row=nullptr;
 std::size_t array_index=0,row_index=0;std::uint64_t hash=0;
 std::vector<InitialClassCountRow> rows;
 std::int32_t type=-1,unit_class=-1,before=0,after=0;
 std::uint64_t before_revision=0,after_revision=0;
 std::shared_ptr<PendingUnit> node;bool stored=false;
};
struct InitialTeamClassPrefix {
 saves::TeamObservation*team=nullptr;saves::TeamHashedArray*array=nullptr;saves::TeamHashedValues*row=nullptr;
 std::size_t array_index=0,row_index=0;std::uint64_t hash=0;
 std::vector<InitialClassCountRow> rows;
 std::int32_t team_id=-1,type=-1,unit_class=-1,before=0,after=0;
 std::uint64_t before_revision=0,after_revision=0;
 std::shared_ptr<PendingUnit>node;bool stored=false;
};
struct InitialPlayerClassPrefix {
 saves::PlayerCoreField*field=nullptr;saves::SavedPlayerObservation*player=nullptr;
 std::size_t field_index=0;std::int32_t actor=-1,type=-1,unit_class=-1,before=0,after=0;
 std::uint64_t hash=0,before_revision=0,after_revision=0;
 std::vector<std::int64_t>before_values,after_values;std::vector<std::uint64_t>happiness_inputs;
 std::shared_ptr<PendingUnit>node;bool stored=false;
};
struct InitialSupportZeroPrefix {
 BnwUnitSupportRule rule;std::shared_ptr<PendingUnit>node;
 std::int32_t unit_class=-1;std::uint64_t class_hash=0,revision=0;bool stored=false;
};
struct InitialResourceNukeZeroPrefix {
 struct Resource {std::int32_t id=-1,cost=0;std::uint32_t hash=0;std::string type;};
 std::vector<Resource> resources;std::shared_ptr<PendingUnit>node;std::uint64_t revision=0;bool stored=false;
};
struct InitialMilitarySupportPrefix {
 std::shared_ptr<PendingUnit>node;std::shared_ptr<ui::BnwClientSelectionControl>input;
 saves::PlayerCoreField*field=nullptr;saves::SavedPlayerObservation*player=nullptr;std::size_t field_index=0;
 std::int32_t before=0,after=0;std::vector<std::int64_t>after_value;
 std::vector<std::uint64_t>happiness_inputs;
 std::uint64_t before_revision=0,after_revision=0,game_data_before=0,game_data_after=0,input_before_revision=0,input_after_revision=0;
 bool incremented=false,active=false,stored=false,game_data_returned=false;
};

struct InitialUnitInfoPromotionRow {
 std::int32_t id=-1;std::uint64_t hash=0;std::string type;
 bool hover=false;std::optional<std::string> invisible,see_invisible;std::array<std::int32_t,11>flags{};
 bool operator==(const InitialUnitInfoPromotionRow&)const=default;
};
constexpr std::array<const char*,11> initial_promotion_columns={"Blitz","Amphib","River","EnemyRoute","RivalTerritory","MustSetUpToRangedAttack","RangedSupportFire","AlwaysHeal","HealOutsideFriendly","HillsDoubleMove","IgnoreTerrainCost"};
constexpr std::array<const char*,11> initial_promotion_members={"m_iBlitzCount","m_iAmphibCount","m_iRiverCrossingNoPenaltyCount","m_iEnemyRouteCount","m_iRivalTerritoryCount","m_iMustSetUpToRangedAttackCount","m_iRangedSupportFireCount","m_iAlwaysHealCount","m_iHealOutsideFriendlyCount","m_iHillsDoubleMoveCount","m_iIgnoreTerrainCostCount"};
struct InitialUnitInfoPromotionPrefix {
 std::shared_ptr<PendingUnit>node;std::vector<InitialUnitInfoPromotionRow>rows;std::set<std::int32_t>free;
 std::map<std::string,std::int32_t,std::less<>>before,after;
 std::set<std::size_t>dirty_before,dirty_after;std::vector<std::uint8_t>bits_before,bits_after;
 std::uint64_t revision=0;std::int32_t selected=-1;std::size_t first_free=0;bool stored=false;
};
std::pair<std::vector<InitialUnitInfoPromotionRow>,std::set<std::int32_t>> initial_unit_info_promotion_rules(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 // id == -1/hash == 0 represents a genuine NULL GC Info slot. Only actual
 // selected SQL rows populate payloads; all grants require a populated slot.
 std::vector<InitialUnitInfoPromotionRow>rows(node.promotion_info_ids.size());std::set<std::int32_t>free;
 std::size_t present=0;std::uint64_t span=0;
 const auto info=host.database().query_rows({"SELECT ID,Type,HoveringUnit,Invisible,SeeInvisible,Blitz,Amphib,River,EnemyRoute,RivalTerritory,MustSetUpToRangedAttack,RangedSupportFire,AlwaysHeal,HealOutsideFriendly,HillsDoubleMove,IgnoreTerrainCost FROM UnitPromotions WHERE ID > -1 ORDER BY ID",{}},[&](const auto&r){
  InitialUnitInfoPromotionRow row;row.id=narrow(r.integer("ID"));const auto type=r.text("Type");const auto*entry=type?host.types().find(*type):nullptr;
  need(entry&&entry->table=="UnitPromotions"&&entry->id==row.id&&entry->hash>0&&entry->hash==content::saved_info_type_hash(*type)&&
   row.id>=0&&static_cast<std::uint64_t>(row.id)>=span&&static_cast<std::size_t>(row.id)<rows.size()&&
   rows[static_cast<std::size_t>(row.id)].id==-1&&node.promotion_info_ids[static_cast<std::size_t>(row.id)]==row.id&&
   same_info_binding(entry,host.types().find_hash(entry->hash)),
   "Initial free promotion complete actual Type/ID/hash slots differ");row.type=*type;row.hash=entry->hash;
  const auto hover=r.integer("HoveringUnit");need(hover==0||hover==1,"Initial free promotion Hovering flag invalid");row.hover=hover!=0;
  row.invisible=r.text("Invisible");row.see_invisible=r.text("SeeInvisible");
  for(std::size_t i=0;i<row.flags.size();++i){const auto n=r.integer(initial_promotion_columns[i]);need(n==0||n==1,"Initial free promotion native bool encoding unsupported");row.flags[i]=narrow(n);}
  const auto slot=static_cast<std::size_t>(row.id);span=static_cast<std::uint64_t>(row.id)+1;
  rows[slot]=std::move(row);++present;
 });need(info.all_succeeded()&&info.rows==present&&span==rows.size(),"Initial free promotion complete actual Info table absent");
 for(std::size_t slot=0;slot<rows.size();++slot)need(rows[slot].id==node.promotion_info_ids[slot]&&
  (rows[slot].id>=0||(rows[slot].hash==0&&rows[slot].type.empty())),"Initial free promotion NULL Info identity differs");
 std::size_t count=0;const content::InfoTypeEntry*unit=nullptr;
 for(const auto&[key,e]:host.types().entries()){if(e.table=="UnitPromotions"&&e.id>=0){++count;need(e.id>=0&&static_cast<std::size_t>(e.id)<rows.size()&&rows[static_cast<std::size_t>(e.id)].type==key&&rows[static_cast<std::size_t>(e.id)].hash==e.hash,"Initial free promotion actual catalog universe differs");}
  if(e.table=="Units"&&e.id==node.scalars.at("m_eUnitType")){need(!unit,"Initial free promotion duplicate UnitInfo ID");unit=&e;}}
 need(count==present&&unit&&unit->hash==content::saved_info_type_hash(unit->type),"Initial free promotion genuine UnitInfo missing");
 const auto relation=host.database().query_rows({"SELECT PromotionType FROM Unit_FreePromotions WHERE UnitType=?",{unit->type}},[&](const auto&r){
  const auto type=r.text("PromotionType");const auto*e=type?host.types().find(*type):nullptr;
  need(e&&e->table=="UnitPromotions"&&e->id>=0&&static_cast<std::size_t>(e->id)<rows.size()&&rows[static_cast<std::size_t>(e->id)].type==*type,
   "Initial Unit_FreePromotions genuine relation unresolved");free.insert(narrow(e->id)); // PopulateArrayByExistence duplicate rows set true.
 });need(relation.all_succeeded(),"Initial Unit_FreePromotions complete relation unavailable");return {std::move(rows),std::move(free)};
}

struct NativePromotionScalarSpec {const char*column;const char*member;bool boolean,guard,nonnegative,ui,sight;};
constexpr std::array<NativePromotionScalarSpec,103>native_promotion_scalar_specs={{
 {"Blitz", "m_iBlitzCount", true, false, true, false, false},
 {"Amphib", "m_iAmphibCount", true, false, true, false, false},
 {"River", "m_iRiverCrossingNoPenaltyCount", true, false, true, false, false},
 {"EnemyRoute", "m_iEnemyRouteCount", true, false, true, false, false},
 {"RivalTerritory", "m_iRivalTerritoryCount", true, false, true, false, false},
 {"MustSetUpToRangedAttack", "m_iMustSetUpToRangedAttackCount", true, false, true, false, false},
 {"RangedSupportFire", "m_iRangedSupportFireCount", true, false, true, false, false},
 {"AlwaysHeal", "m_iAlwaysHealCount", true, false, true, false, false},
 {"HealOutsideFriendly", "m_iHealOutsideFriendlyCount", true, false, true, false, false},
 {"HillsDoubleMove", "m_iHillsDoubleMoveCount", true, false, true, false, false},
 {"IgnoreTerrainCost", "m_iIgnoreTerrainCostCount", true, true, false, false, false},
 {"RoughTerrainEndsTurn", "m_iRoughTerrainEndsTurnCount", true, true, false, false, false},
 {"HoveringUnit", "m_iHoveringUnitCount", true, true, false, false, false},
 {"FlatMovementCost", "m_iFlatMovementCostCount", true, true, false, false, false},
 {"CanMoveImpassable", "m_iCanMoveImpassableCount", true, true, false, false, false},
 {"OnlyDefensive", "m_iOnlyDefensiveCount", true, true, false, false, false},
 {"NoDefensiveBonus", "m_iNoDefensiveBonusCount", true, true, false, false, false},
 {"NoCapture", "m_iNoCaptureCount", true, true, false, false, false},
 {"NukeImmune", "m_iNukeImmuneCount", true, false, true, false, false},
 {"HiddenNationality", "m_iHiddenNationalityCount", true, false, true, false, false},
 {"AlwaysHostile", "m_iAlwaysHostileCount", true, false, true, false, false},
 {"NoRevealMap", "m_iNoRevealMapCount", true, false, true, false, false},
 {"Recon", "m_iReconCount", true, true, false, false, false},
 {"CanMoveAllTerrain", "m_iCanMoveAllTerrainCount", true, false, true, false, false},
 {"CanMoveAfterAttacking", "m_iCanMoveAfterAttackingCount", true, false, true, false, false},
 {"AirSweepCapable", "m_iAirSweepCapableCount", true, true, false, false, false},
 {"AllowsEmbarkation", "m_iEmbarkAbilityCount", true, true, false, false, false},
 {"RangeAttackIgnoreLOS", "m_iRangeAttackIgnoreLOSCount", true, true, false, false, false},
 {"HealIfDestroyExcludesBarbarians", "m_iHealIfDefeatExcludeBarbariansCount", true, true, false, false, false},
 {"HealOnPillage", "m_iHealOnPillageCount", true, false, true, false, false},
 {"FreePillageMoves", "m_iFreePillageMoveCount", true, false, true, false, false},
 {"EmbarkedAllWater", "m_iEmbarkedAllWaterCount", true, false, false, false, false},
 {"CityAttackOnly", "m_iCityAttackOnlyCount", true, true, false, false, false},
 {"CaptureDefeatedEnemy", "m_iCaptureDefeatedEnemyCount", true, true, false, false, false},
 {"HeavyCharge", "m_iCanHeavyCharge", true, false, false, false, false},
 {"EmbarkExtraVisibility", "m_iEmbarkExtraVisibility", false, false, false, false, false},
 {"EmbarkDefenseModifier", "m_iEmbarkDefensiveModifier", false, false, false, false, false},
 {"CapitalDefenseModifier", "m_iCapitalDefenseModifier", false, false, false, false, false},
 {"CapitalDefenseFalloff", "m_iCapitalDefenseFalloff", false, false, false, false, false},
 {"CityAttackPlunderModifier", "m_iCityAttackPlunderModifier", false, false, false, false, false},
 {"ReligiousStrengthLossRivalTerritory", "m_iReligiousStrengthLossRivalTerritory", false, false, false, false, false},
 {"TradeMissionInfluenceModifier", "m_iTradeMissionInfluenceModifier", false, false, false, false, false},
 {"TradeMissionGoldModifier", "m_iTradeMissionGoldModifier", false, false, false, false, false},
 {"DropRange", "m_iDropRange", false, true, false, false, false},
 {"VisibilityChange", "m_iExtraVisibilityRange", false, true, false, false, true},
 {"MovesChange", "m_iExtraMoves", false, false, false, false, false},
 {"MoveDiscountChange", "m_iExtraMoveDiscount", false, false, true, false, false},
 {"ExtraNavalMovement", "m_iExtraNavalMoves", false, false, true, false, false},
 {"HPHealedIfDestroyEnemy", "m_iHPHealedIfDefeatEnemy", false, false, true, false, false},
 {"GoldenAgeValueFromKills", "m_iGoldenAgeValueFromKills", false, false, true, false, false},
 {"ExtraWithdrawal", "m_iExtraWithdrawal", false, false, true, false, false},
 {"RangeChange", "m_iExtraRange", false, false, false, false, false},
 {"RangedAttackModifier", "m_iRangedAttackModifier", false, true, false, false, false},
 {"InterceptionCombatModifier", "m_iInterceptionCombatModifier", false, true, false, false, false},
 {"InterceptionDefenseDamageModifier", "m_iInterceptionDefenseDamageModifier", false, true, false, false, false},
 {"AirSweepCombatModifier", "m_iAirSweepCombatModifier", false, true, false, false, false},
 {"InterceptChanceChange", "m_iExtraIntercept", false, false, false, false, false},
 {"EvasionChange", "m_iExtraEvasion", false, false, false, false, false},
 {"EnemyHealChange", "m_iExtraEnemyHeal", false, false, false, false, false},
 {"NeutralHealChange", "m_iExtraNeutralHeal", false, false, false, false, false},
 {"FriendlyHealChange", "m_iExtraFriendlyHeal", false, false, true, false, false},
 {"SameTileHealChange", "m_iSameTileHeal", false, false, true, false, false},
 {"AdjacentTileHealChange", "m_iAdjacentTileHeal", false, false, true, false, false},
 {"EnemyDamageChance", "m_iEnemyDamageChance", false, false, true, false, false},
 {"NeutralDamageChance", "m_iNeutralDamageChance", false, false, true, false, false},
 {"EnemyDamage", "m_iEnemyDamage", false, false, true, false, false},
 {"NeutralDamage", "m_iNeutralDamage", false, false, true, false, false},
 {"NearbyEnemyCombatMod", "m_iNearbyEnemyCombatMod", false, false, false, false, false},
 {"NearbyEnemyCombatRange", "m_iNearbyEnemyCombatRange", false, false, true, false, false},
 {"AdjacentMod", "m_iAdjacentModifier", false, true, false, false, false},
 {"AttackMod", "m_iAttackModifier", false, true, false, false, false},
 {"DefenseMod", "m_iDefenseModifier", false, true, false, false, false},
 {"CombatPercent", "m_iExtraCombatPercent", false, true, false, true, false},
 {"CityAttack", "m_iExtraCityAttackPercent", false, true, false, true, false},
 {"CityDefense", "m_iExtraCityDefensePercent", false, true, false, true, false},
 {"RangedDefenseMod", "m_iExtraRangedDefenseModifier", false, true, false, true, false},
 {"HillsAttack", "m_iExtraHillsAttackPercent", false, true, false, true, false},
 {"HillsDefense", "m_iExtraHillsDefensePercent", false, true, false, true, false},
 {"OpenAttack", "m_iExtraOpenAttackPercent", false, true, false, true, false},
 {"OpenRangedAttackMod", "m_iExtraOpenRangedAttackMod", false, true, false, true, false},
 {"RoughAttack", "m_iExtraRoughAttackPercent", false, true, false, true, false},
 {"RoughRangedAttackMod", "m_iExtraRoughRangedAttackMod", false, true, false, true, false},
 {"AttackFortifiedMod", "m_iExtraAttackFortifiedMod", false, true, false, true, false},
 {"AttackWoundedMod", "m_iExtraAttackWoundedMod", false, true, false, true, false},
 {"FlankAttackModifier", "m_iFlankAttackModifier", false, true, false, true, false},
 {"OpenDefense", "m_iExtraOpenDefensePercent", false, true, false, true, false},
 {"RoughDefense", "m_iExtraRoughDefensePercent", false, true, false, true, false},
 {"ExtraAttacks", "m_iNumAttacks", false, true, false, true, false},
 {"NumInterceptionChange", "m_iNumInterceptions", false, true, false, false, false},
 {"GreatGeneral", "m_iGreatGeneralCount", true, false, false, false, false},
 {"GreatAdmiral", "m_iGreatAdmiralCount", true, false, false, false, false},
 {"GreatGeneralModifier", "m_iGreatGeneralModifier", false, true, false, false, false},
 {"GreatGeneralReceivesMovement", "m_iGreatGeneralReceivesMovementCount", true, false, false, false, false},
 {"GreatGeneralCombatModifier", "m_iGreatGeneralCombatModifier", false, false, false, false, false},
 {"IgnoreGreatGeneralBenefit", "m_iIgnoreGreatGeneralBenefit", true, false, false, false, false},
 {"IgnoreZOC", "m_iIgnoreZOC", true, false, false, false, false},
 {"Sapper", "m_iSapperCount", true, false, false, false, false},
 {"FriendlyLandsModifier", "m_iFriendlyLandsModifier", false, true, false, false, false},
 {"FriendlyLandsAttackModifier", "m_iFriendlyLandsAttackModifier", false, true, false, false, false},
 {"OutsideFriendlyLandsModifier", "m_iOutsideFriendlyLandsModifier", false, true, false, false, false},
 {"UpgradeDiscount", "m_iUpgradeDiscount", false, true, false, true, false},
 {"ExperiencePercent", "m_iExperiencePercent", false, true, false, true, false},
 {"CargoChange", "m_iCargoCapacity", false, true, true, true, false}
}};
struct NativePromotionFullInfo {
 InitialUnitInfoPromotionRow prefix;std::array<std::int32_t,103>values{};
 std::map<std::string,std::vector<std::int32_t>,std::less<>>indexed;
 bool operator==(const NativePromotionFullInfo&)const=default;
};
struct InitialUnitInfoPromotionLoop {
 std::shared_ptr<PendingUnit>node;std::shared_ptr<ui::BnwClientSelectionControl>input;
 std::map<std::string,std::int32_t,std::less<>>before,after;
 std::map<std::string,std::vector<std::int32_t>,std::less<>>indexed_before,indexed_after;
 std::set<std::size_t>dirty_before,dirty_after;std::vector<std::uint8_t>bits_before,bits_after;
 std::vector<NativePromotionVectorSet>commands_before,commands_after;
 std::vector<NativePromotionFullInfo>infos;std::uint64_t revision=0,input_revision=0;std::size_t grants=0;bool stored=false;
};

struct NativeTraitPromotionRelation {std::int32_t trait=-1,promotion=-1,combat=-1;bool operator==(const NativeTraitPromotionRelation&)const=default;};
struct InitialTraitsPromotionLoop:InitialUnitInfoPromotionLoop {
 std::shared_ptr<const BnwMajorPlayerTraitsOwner>traits;std::vector<std::int64_t>active;
 std::vector<NativeTraitPromotionRelation>relations;std::set<std::int32_t>free;std::int32_t combat=-1;
};

struct InitialPlayerPromotionValidity {std::vector<std::pair<std::int32_t,std::int32_t>>combats,civilians;bool operator==(const InitialPlayerPromotionValidity&)const=default;};
struct InitialPlayerPromotionLoop:InitialUnitInfoPromotionLoop {
 const saves::PlayerCoreField*field=nullptr;saves::PlayerCoreField raw_before;std::vector<std::int32_t>counts;
 InitialPlayerPromotionValidity validity;std::set<std::int32_t>free;std::int32_t combat=-1;
};

struct InitialEmbarkPromotionLoop:InitialUnitInfoPromotionLoop {
 std::shared_ptr<const BnwMajorPlayerTraitsOwner>traits;std::int32_t team=-1,embark=0,defensive=0,all_water=0;
 const saves::TeamObservation*team_source=nullptr;const saves::PlayerTraitsField*trait_field=nullptr;
 saves::PlayerTraitsField trait_before;std::set<std::int32_t>free,effects;InitialPlayerPromotionValidity validity;
 std::vector<std::pair<std::string,std::int32_t>>defines;std::int32_t combat=-1,domain=-1;
};
struct InitialExoticXpPrefix:InitialUnitInfoPromotionLoop {
 std::int32_t exotic=0,handicap=-1,xp=0,xp_percent=0;std::optional<bool>human;
 decltype(std::declval<LiveSessionState>().world.pregame->archive.integers)pregame_integers;
 decltype(std::declval<LiveSessionState>().world.pregame->archive.integer_arrays)pregame_arrays;
 std::int64_t game_type=-1;decltype(std::declval<LiveSessionState>().setup.pitboss)pitboss;
};
struct InitialImmobileMovesPrefix:InitialUnitInfoPromotionLoop {
 std::shared_ptr<const BnwMajorPlayerTraitsOwner>traits;
 std::int32_t info_moves=0,domain=-1,combat=-1,immobile=0,team=-1,team_extra=0,golden_age=0,naval=0,golden_moves=0,combat_moves=0,denominator=0,max_moves=0,moves_before=0;
 const saves::PlotArrayObservation*team_moves=nullptr;std::vector<std::int64_t>team_moves_before;
 std::vector<const saves::PlayerTraitsField*>trait_fields;std::vector<saves::PlayerTraitsField>trait_before;
};

struct InitialSetMovesPrefix:InitialUnitInfoPromotionLoop {
 std::int32_t before_moves=0,requested_moves=0;std::uint64_t plot=0;
 const std::remove_reference_t<decltype(std::declval<LiveSessionState>().world.map_plots->plots)>::value_type*plot_source=nullptr;
 bool changed=false,returned=false;
 std::shared_ptr<const graphics::BnwInitialUnitShouldDimEnqueueProof>proof;
};
enum class InitialMovesUiPhase {pathfinder,unit_info,selection_plot,plot_list,returned};
struct InitialMovesUiTail {
 std::shared_ptr<ui::BnwClientSelectionControl>input;
 ui::BnwClientUnitDirtySnapshot before,current,next;
 std::optional<std::pair<std::int32_t,std::int64_t>>selected;
 std::optional<std::uint64_t>selection_plot;
 std::uint64_t unit_info_occurrence=0,plot_list_occurrence=0;
 bool skipped=false,unit_selected=false,selection_read=false,common_reset=false,plot_read=false,returned=false;
 std::size_t dirty_count=0;InitialMovesUiPhase phase=InitialMovesUiPhase::selection_plot;
};
struct InitialReligionPrefix {
 std::shared_ptr<PendingUnit>node;std::array<std::int32_t,3>before{},after{};
 bool spread=false,remove=false,applied=false,returned=false;
 std::int32_t info_spreads=0,info_strength=0,majority=-1;
 std::vector<std::uint64_t>city_seal;
};
struct InitialTourismPrefix {
 std::shared_ptr<PendingUnit>node;std::shared_ptr<const BnwPolicyHappinessRules>rules;
 std::int32_t one_shot=0,unit_class=-1,creation=0;std::vector<std::uint64_t>policy_seal;bool returned=false;
};
struct InitialReconPrefix {
 std::shared_ptr<PendingUnit>node;std::int32_t count=0,x=0,y=0;bool returned=false;
};
struct InitialBestLandPrefix {
 std::shared_ptr<PendingUnit>node;saves::NamedSaveInteger*control=nullptr;
 std::int32_t domain=-1,base_combat=0,type=-1,best=-1,best_combat=1;
 bool game_read=false,setter_called=false,changed=false,stored=false,ui_stored=false,returned=false;
 std::uint64_t before_revision=0,after_revision=0,occurrence=0;
 std::shared_ptr<ui::BnwClientSelectionControl>input;ui::BnwClientUnitDirtySnapshot before,current,next;
 std::vector<std::uint64_t>happiness_inputs;
};
struct InitialActiveGameDataPrefix {
 std::shared_ptr<PendingUnit>node;std::shared_ptr<ui::BnwClientSelectionControl>input;
 std::int32_t active_player=-1;bool active=false,stored=false,returned=false;
 std::optional<bool>before_flag;std::uint64_t before_count=0,after_count=0,world_revision=0;
 ui::BnwClientUnitDirtySnapshot before,current;
};
struct InitialCycleEntry {
 std::int32_t id=-1,type=-1,x=-1,y=-1;std::shared_ptr<ImportedCycleNode>imported;
 std::int32_t*newborn_cycle=nullptr;std::int16_t before=-1,after=-1;
 std::uint64_t before_stores=0,after_stores=0;
};
struct InitialCycleStore {std::size_t entry=0;std::int16_t value=0;};
struct InitialUnitCyclerPrefix {
 std::shared_ptr<PendingUnit>node;std::shared_ptr<ui::BnwClientSelectionControl>input;
 std::shared_ptr<const BnwMajorPlayerTraitsOwner>traits;
 std::optional<std::pair<std::int32_t,std::int64_t>>selected;
 ui::BnwClientUnitDirtySnapshot ui_before;
 saves::PlotArrayObservation*array=nullptr;saves::NamedSaveInteger*count=nullptr;
 std::vector<std::int64_t>before,after,empty_reserved;std::vector<InitialCycleEntry>entries;
 std::vector<InitialCycleStore>stores;std::vector<std::pair<std::int32_t,std::int32_t>>rates;
 std::int32_t worker_modifier=0,trait_modifier=0,ai_modifier=0,handicap=-1;
 std::optional<bool>human,ai_teammate;bool active=false,stored=false,returned=false;
 std::int32_t newborn_before=-1,newborn_after=-1;std::vector<std::uint64_t>source_seal;
};
struct InitialWorldUnitMessagePrefix {std::int32_t unit_class=-1,max_global=-1;bool returned=false;};
struct InitialSecondCityStrengthPrefix {
 std::shared_ptr<PendingUnit>node;bool city=false,stored=false,returned=false;
 std::int32_t owner=-1,id=-1,before=0,after=0;const saves::SavedCityObservation*raw=nullptr;
 std::uint64_t before_revision=0,after_revision=0,before_occurrence=0;
 std::vector<BnwCityDirtyEvent>before_events,after_events;
};
struct InitialArmyAiPrefix {
 std::shared_ptr<PendingUnit>node;std::int32_t unit_ai=-1;bool stored=false,returned=false;
 std::map<std::string,std::int32_t,std::less<>>before,after;
 std::set<std::size_t>dirty_before,dirty_after;
 std::int32_t*army=nullptr,*ai=nullptr;
};
struct InitialMaintenancePrefix {
 std::shared_ptr<PendingUnit>node;std::shared_ptr<ui::BnwClientSelectionControl>input;
 saves::PlayerCoreField*field=nullptr;std::int32_t before_cache=0,after=0,active_player=-1;
 bool active=false,stored=false,returned=false;std::uint32_t ui_steps=0;
 std::uint64_t world_revision=0,city_count=0,game_count=0;
 std::optional<bool>city_flag,game_flag;ui::BnwClientUnitDirtySnapshot before,current;
 std::vector<std::int64_t>inputs;
};
struct InitialMinorQuestPrefix {
 std::shared_ptr<PendingUnit>node;std::vector<std::int64_t>inputs;
 std::uint32_t minors=0,quests=0,great_person_checks=0;bool tested=false,returned=false;
};
struct BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::Storage {
 Binding binding;const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool>alive{true};std::atomic<std::size_t>borrowers{0};
 std::weak_ptr<const InitialUnitShouldDimPlan>canonical;
 std::optional<BnwStockLuaAttemptLease>attempt;
 std::optional<BnwStockLuaAttemptLease::ScopeIdentity>scope;
 void retire()noexcept{
  // Poison before release. The shared ScopeIdentity RAII alone owns source
  // pin accounting and releases its source anchor/count on destruction.
  // Core must not mutate source_owner separately or count that anchor twice.
  alive.store(false);scope.reset();attempt.reset();
 }
 ~Storage(){retire();}
};

struct BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::Storage {
 Binding binding;const std::thread::id creator=std::this_thread::get_id();std::atomic<bool>alive{true};std::atomic<std::size_t>borrowers{0};std::weak_ptr<const InitialUnitSetupGraphicalPlan>canonical;
 std::optional<BnwStockLuaAttemptLease>attempt;std::optional<BnwStockLuaAttemptLease::ScopeIdentity>scope;
 std::shared_ptr<PendingUnit>node;
 std::map<std::string,std::int32_t,std::less<>>scalars;std::map<std::string,std::vector<std::int32_t>,std::less<>>indexed;std::map<std::string,std::string,std::less<>>strings;
 std::set<std::size_t>dirty;std::vector<std::uint8_t>bits;std::vector<NativePromotionVectorSet>promotion_sets;
 std::vector<NativeInitialMission>missions;std::int32_t religion=-1,strength=0,spreads=0;std::uint32_t last_path=UINT32_MAX;
 std::weak_ptr<const BnwPendingCreatedUnitRead>created_pending_read;bool created_pending_read_issued=false;
 std::optional<std::int32_t>work_rate;std::optional<BnwMinorCityStrengthScratch::InitialSetupGarrisonQuote>garrison;
 bool readers_prepared=false;
 void retire()noexcept{alive.store(false);scope.reset();attempt.reset();}
 ~Storage(){retire();}
};

struct BnwNativeUnitAllocatorCore::ContinueMissionCombatWarnedAttempt {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeUnitAllocatorCore>core;
 std::weak_ptr<Bank>bank;std::weak_ptr<ExistingContinueMissionEntry>entry;
 std::int32_t actor=-1,id=-1;std::uint64_t generation=0;
 std::shared_ptr<BnwLiveGameContext>context;
 std::unique_ptr<BnwLiveGameContext::GameReference>reference;
 std::shared_ptr<const BnwLiveGameContext::GameReference::CombatWarnedReceipt>receipt;
 // Earned only after SAME actual destroyed reference + final source checks.
 bool native_return_earned=false;
 std::optional<saves::CivisSaveSection>restored_checkpoint;
 std::weak_ptr<BnwTwoLayerUnitPathFinder>restored_tree;
};

struct BnwNativeUnitAllocatorCore::Impl {
 // Weak once registry: no Core->Source->Host cycle; expired attempt still burns
 // this native turn call. Error/reentry never permits a second predicate call.
 std::map<std::int64_t,std::weak_ptr<const BnwNativePlayerBusyUnitsReturn>>player_turn_busy_returns;
 std::map<std::int64_t,std::weak_ptr<const BnwNativePlayerBusyCitiesReturn>>player_turn_busy_city_returns;
 std::map<std::int64_t,std::weak_ptr<const BnwNativeAttritionPrefix>>player_attrition_prefixes;

 const BnwUnitCombatWarnedRestorePlan*combat_warned_restore_plan=nullptr;
 std::map<std::int32_t,std::shared_ptr<ContinueMissionCombatWarnedAttempt>>continue_mission_game_attempts;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;std::shared_ptr<LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;
 std::shared_ptr<const BnwMajorPlayerTraitsOwner> traits;
 std::shared_ptr<const ui::SelectedCityTrainCreateSource> train_source;
 std::shared_ptr<BnwUnitMovePending> initial_set_xy;
 std::optional<InitialVisibilityBinding> initial_hidden_binding;
 std::optional<InitialGarrisonPrefix> initial_garrison;
 std::optional<InitialPlotAreaPrefix> initial_plot_area;
 std::optional<BnwMinorCityStrengthScratch::InitialQuote> initial_city_strength;
 std::optional<InitialPostCityPrefix> initial_post_city;
 std::optional<InitialHappinessPrefix> initial_happiness;
 std::weak_ptr<const InitialHappinessFounderDispatch> initial_founder_dispatch;
 std::weak_ptr<const InitialUnitSetXYDispatch> initial_unit_setxy_dispatch;
 std::shared_ptr<const InitialCenterUnitVisibilityPlan> initial_center_plan;
 std::optional<InitialNameGamePrefix> initial_name_game;
 std::optional<InitialGameClassPrefix> initial_game_class;
 std::optional<InitialTeamClassPrefix> initial_team_class;
 std::optional<InitialPlayerClassPrefix> initial_player_class;
 std::optional<InitialSupportZeroPrefix> initial_support_zero;
 std::optional<InitialResourceNukeZeroPrefix> initial_resource_nuke_zero;
 std::optional<InitialMilitarySupportPrefix> initial_military_support;
 std::optional<InitialUnitInfoPromotionPrefix> initial_unit_info_promotion;
 std::optional<InitialUnitInfoPromotionLoop>initial_unit_info_promotion_loop;
 std::optional<InitialTraitsPromotionLoop>initial_traits_promotions;
 std::optional<InitialPlayerPromotionLoop>initial_player_promotions;
 std::optional<InitialEmbarkPromotionLoop>initial_embark_promotions;
 std::optional<InitialExoticXpPrefix>initial_exotic_xp;
 std::optional<InitialImmobileMovesPrefix>initial_immobile_moves;
 std::optional<InitialSetMovesPrefix>initial_set_moves;
 std::optional<InitialMovesUiTail>initial_moves_ui_tail;
 std::optional<InitialReligionPrefix>initial_religion;
 std::optional<InitialTourismPrefix>initial_tourism;
 std::optional<InitialReconPrefix>initial_recon;
 std::optional<InitialBestLandPrefix>initial_best_land;
 std::optional<InitialActiveGameDataPrefix>initial_active_game_data;
 std::optional<InitialUnitCyclerPrefix>initial_unit_cycler;
 std::optional<InitialWorldUnitMessagePrefix>initial_world_unit_message;
 std::optional<InitialSecondCityStrengthPrefix>initial_second_city_strength;
 std::optional<InitialArmyAiPrefix>initial_army_ai;
 std::optional<InitialMaintenancePrefix>initial_maintenance;
 std::optional<InitialMinorQuestPrefix>initial_minor_quest;
 std::optional<BnwMinorCityStrengthScratch::InitialSuffixQuote>initial_second_city_strength_quote;
 std::shared_ptr<const InitialUnitShouldDimPlan>initial_should_dim_plan;
 std::shared_ptr<const InitialUnitSetupGraphicalPlan>initial_setup_graphical_plan;
 std::shared_ptr<const graphics::BnwInitialUnitSetupGraphicalEnqueueProof>initial_setup_graphical_proof;
 bool initial_setup_graphical_returned=false;
 std::shared_ptr<InitialCenterCache> centers;
 std::shared_ptr<const graphics::BnwPendingUnitPositionProof> initial_position;
 std::weak_ptr<graphics::BnwPendingUnitVisualOwner> initial_position_receiver;
 std::shared_ptr<BnwInitialUnitSightOwner> initial_sight;
 std::uint64_t initial_sight_before=0,initial_sight_after=0;bool initial_sight_stored=false;
 std::shared_ptr<const graphics::BnwPendingUnitHiddenSlotProof> initial_hidden_proof;
 std::weak_ptr<graphics::BnwPendingUnitVisualOwner> initial_hidden_receiver;
 std::int32_t actor=-1;std::thread::id thread=std::this_thread::get_id();
 std::string fingerprint;std::shared_ptr<LiveSession::RetainedFacadeLifetime> facade_token;std::function<void()> facade_current;std::shared_ptr<Bank> bank;bool registry_installed=false;
 struct Guard{std::shared_ptr<Impl>p;explicit Guard(std::shared_ptr<Impl>x):p(std::move(x)){
  need(p&&p->thread==std::this_thread::get_id(),"Foreign native unit allocator call");
  need(p->bank&&!p->bank->retired.load(),"Native unit allocator retired");
  if(p->bank->busy.exchange(true)){p->bank->retired=true;throw BnwNativeUnitAllocatorUnavailable("Native unit allocator reentry");}
 }~Guard(){p->bank->busy=false;}};
 void check()const{
  need(thread==std::this_thread::get_id(),"Foreign native unit allocator borrow");
  if(initial_happiness&&(initial_happiness->founder_inflight||initial_happiness->unit_setxy_inflight)){bank->retired=true;throw BnwNativeUnitAllocatorUnavailable("Reentry during actual newborn founder dispatch");}
  facade_current();
  need(bank&&!bank->retired.load()&&host&&host->session().read_state_lease().get()==state.get()&&
  host->session().state_generation()==generation&&&host->session().source()==source&&
  host->identity().rules_sha256==fingerprint&&bank->source.lock().get()==state.get()&&bank->generation==generation&&
  bank->fingerprint==fingerprint,"Native unit allocator actual source lifetime changed");
  if(registry_installed){const auto*registry=state->setup.extensions.find<Registry>();
   need(registry&&registry->players.contains(actor)&&registry->players.at(actor).get()==bank.get()&&
    !registry->players.at(actor).owner_before(bank)&&!bank.owner_before(registry->players.at(actor)),
    "Native allocator SAMEstate registry authority replaced");
   need(centers&&registry->centers.get()==centers.get()&&!registry->centers.owner_before(centers)&&!centers.owner_before(registry->centers)&&
    centers->creator==thread&&!centers->retired.load()&&centers->source.lock().get()==state.get()&&centers->generation==generation,
    "Native allocator genuine center cache replaced/retired");}
  need(host->database().gameplay_fingerprint_sha256()==fingerprint,"Native allocator rules changed");
  try{(void)host->session().bnw_game_debug_mode();}catch(const std::exception&){
   throw BnwNativeUnitAllocatorUnavailable("Native allocator actual Game debug source retired");}
  compare(*state,actor,bank->baseline);check_imported_cycle_nodes(*bank,*state,actor);facade_current();
  need(!bank->retired.load()&&host->session().read_state_lease().get()==state.get()&&host->session().state_generation()==generation,
   "Native unit allocator source bookend changed");
 }
};
BnwNativeUnitAllocatorCore::BnwNativeUnitAllocatorCore(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwNativeUnitAllocatorCore::~BnwNativeUnitAllocatorCore(){
 if(impl_&&impl_->initial_should_dim_plan)impl_->initial_should_dim_plan->retire_scope_without_callbacks();
}
std::shared_ptr<const BnwNativeUnitAllocatorCore>BnwNativeUnitAllocatorCore::from_selected_preview(const ui::SelectedBnwReadOnlyPreview&preview,std::int32_t actor){
 std::shared_ptr<const BnwNativeUnitAllocatorCore>result;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  need(host&&host->saved_database_thread_==std::this_thread::get_id(),"Foreign selected Core Host before restored lineage");
  if(host->bnw_restored_mission_core_){
   const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(*host->bnw_restored_mission_core_);const auto&owner=issuer->current();const auto p=owner.impl_;
   need(p&&p->actor==actor&&p->host.get()==host.get()&&!p->host.owner_before(host)&&!host.owner_before(p->host)&&p->continue_mission_game_attempts.size()==1,"Selected restored Core actor/canonical Host lineage differs");
   const auto attempt=p->continue_mission_game_attempts.begin()->second;need(attempt&&attempt->restored_checkpoint,"Selected restored Core lost its actual installed section");
   p->check();(void)issuer->current();owner.require_restored_mission_owner_current_without_callbacks(*attempt->restored_checkpoint);
   result=owner.retain_factory_owner();
  }else result=from_actual_host(std::move(host),actor);
 });need(bool(result),"Selected Core genuine owner absent");return result;
}
std::shared_ptr<const BnwNativeUnitAllocatorCore>BnwNativeUnitAllocatorCore::from_unpublished_mission_restore_host(std::shared_ptr<const ui::SelectedBnwPreviewHost>host,const saves::CivisSaveSection&section){
 need(host&&host->saved_database_thread_==std::this_thread::get_id(),"Foreign unpublished mission Host before source read");
 need(!host->bnw_restored_mission_core_,"Unpublished mission Host restore issuer cannot replace lineage");
 const auto record=BnwUnitMissionContinuationCheckpoint::decode(section,host->identity());need(record.phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed,"Unpublished command Host requires genuine completed phase4 join");
 // Private Host factory is the only caller. This is the SAME physical native
 // registrar; no active-player/input/UI callback is needed to issue the Core.
 return from_actual_host(std::move(host),record.actor);
}
std::shared_ptr<const BnwNativeUnitAllocatorCore> BnwNativeUnitAllocatorCore::from_actual_host(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>host,std::int32_t actor){
 std::shared_ptr<Impl> p;
 {
  need(host&&host->saved_database_thread_==std::this_thread::get_id(),"Foreign actual Core Host issuer before mutable source");
  need(host&&actor>=0,"Native unit allocator selected source missing");
  p=std::make_shared<Impl>();p->host=std::move(host);p->actor=actor;
  auto&session=*p->host->session_;const auto token=session.retain_bnw_facade_lifetime();p->facade_token=token;
  p->facade_current=[token]{try{token->require_current();}catch(const std::exception&){throw BnwNativeUnitAllocatorUnavailable("Native unit facade retired");}};
  p->facade_current();p->state=session.state_;p->source=&session.source();p->generation=session.state_generation();p->fingerprint=p->host->identity().rules_sha256;
  need(p->state->setup.profile==content::TechnologyProfile::bnw&&p->state->maintenance.profile==content::TechnologyProfile::bnw&&
    p->state->setup.players.contains(actor)&&p->state->setup.players.at(actor).ever_alive,"Native unit allocator actual BNW player missing");
  const auto key=std::type_index(typeid(Registry));Registry staged;std::optional<Registry> registry_before;
  if(const auto*old=p->state->setup.extensions.find<Registry>()){registry_before=*old;staged=*old;}
  if(!staged.centers){need(!registry_before,"Existing native registry has no genuine center cache");
   staged.centers=std::make_shared<InitialCenterCache>();staged.centers->source=p->state;staged.centers->generation=p->generation;}
  p->centers=staged.centers;
  auto it=staged.players.find(actor);
  if(it==staged.players.end()){
   auto bank=std::make_shared<Bank>();bank->source=p->state;bank->generation=p->generation;bank->fingerprint=p->fingerprint;
   bank->serial_origin=session.bnw_unit_serial_factory_start(actor);
   need(p->state->world.all_players->players.at(static_cast<std::size_t>(actor)).units.header.fields.at(4).value==bank->serial_origin,"Native serial factory refuses unearned raw counter mutation");
   bank->baseline=capture(*p->state,actor);restore(*bank,p->state->world.all_players->players.at(static_cast<std::size_t>(actor)).units,actor);
   register_imported_cycle_nodes(bank,*p->state,actor);register_imported_mission_runtime(bank,*p->state,*p->source,actor,p->host->identity());register_imported_city_combat_runtime(bank,*p->state,*p->source,actor,p->host->identity());p->bank=bank;staged.players.emplace(actor,std::move(bank));
   // Allocate the any/map node before final guards; only a no-throw swap publishes.
   auto extensions=p->state->setup.extensions.values;extensions.insert_or_assign(key,std::any(std::move(staged)));
   p->check();
   const auto*current=p->state->setup.extensions.find<Registry>();
   need(bool(current)==registry_before.has_value(),"Native allocator registry changed during preparation");
   if(current){need(current->centers.get()==registry_before->centers.get()&&!current->centers.owner_before(registry_before->centers)&&!registry_before->centers.owner_before(current->centers),"Native center cache changed while minting actor");need(current->players.size()==registry_before->players.size(),"Native allocator registry extent changed during preparation");
    auto a=current->players.cbegin();auto b=registry_before->players.cbegin();
    for(;a!=current->players.end();++a,++b)need(a->first==b->first&&a->second.get()==b->second.get()&&
     !a->second.owner_before(b->second)&&!b->second.owner_before(a->second),"Native allocator registry authority changed during preparation");}
   p->facade_current();need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation,
    "Native allocator factory final source changed");
   p->state->setup.extensions.values.swap(extensions);p->registry_installed=true;
  }else{p->bank=it->second;need(p->bank&&p->bank->serial_origin==session.bnw_unit_serial_factory_start(actor),"Native serial registry original/restored start replaced");p->registry_installed=true;p->check();}
 }need(bool(p),"Native unit allocator factory did not retain actual source");
 auto result=std::shared_ptr<BnwNativeUnitAllocatorCore>(new BnwNativeUnitAllocatorCore(p));result->factory_self_=result;p->check();return result;
}
saves::CivisSaveDocument BnwNativeUnitAllocatorCore::checkpoint_native_aborted_unit_serial()const {
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 auto&session=*p->host->session_;const auto*registry=p->state->setup.extensions.find<Registry>();
 need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor)==p->bank,
  "Serial checkpoint canonical native registry absent");
 need(!p->source->source.live_bnw_units&&!p->source->source.live_bnw_area_units&&
  !p->source->source.live_bnw_unit_mission_continuation,"Serial checkpoint requires standalone unit domain");
 need(!p->state->player_early_publication&&!p->state->player_gold_publication&&
  !p->state->player_research_publication&&!p->state->game_clock.pending&&!session.has_bnw_operation_checkpoint_history(),
  "Serial checkpoint cannot discard pending/earned world operations");
 // The source-only section persists all actors' changed counters. Every new
 // burn must come from its SAME session Bank; inherited restored45 is separately
 // authenticated by private Session storage, never by caller numeric values.
 for(std::int32_t actor=0;actor<64;++actor){
  const auto start=session.bnw_unit_serial_factory_start(actor);
  const auto raw=p->state->world.all_players->players.at(static_cast<std::size_t>(actor)).units.header.fields.at(4).value;
  const auto found=registry->players.find(actor);
  if(found==registry->players.end()){need(raw==start,"Serial checkpoint lacks genuine actor burn owner");continue;}
  const auto&b=*found->second;
  need(!b.retired.load()&&(!b.busy.load()||found->second==p->bank)&&!b.constructor_pending&&b.pending_slot==-1&&
   b.pending_units.empty()&&b.registered_units.empty()&&b.source.lock()==p->state&&
   !b.source.owner_before(p->state)&&!p->state.owner_before(b.source)&&b.generation==p->generation&&
   b.fingerprint==p->fingerprint&&b.serial_origin==start&&b.serial==raw,
   "Serial checkpoint pending/foreign/retired allocator history");
  need(!b.iteration_started&&!b.selection_pending&&!b.selected_unit&&!b.train&&b.train_source.expired()&&
   b.continue_mission_entries.empty()&&b.existing_path_to.empty()&&
   b.imported_moves_fields.empty()&&b.imported_flags_fields.empty()&&b.imported_damage_fields.empty(),
   "Serial checkpoint cannot discard other native unit action/history");
  compare(*p->state,actor,b.baseline);check_imported_cycle_nodes(b,*p->state,actor);
  auto expected=start;
  for(const auto&receipt:b.aborted_serial_burns){const auto&r=receipt.prefix.assignment;
   need(receipt.destroyed.expired()&&r.player==actor&&r.slot>=0&&r.slot<8192&&r.full_id==static_cast<std::int64_t>(r.serial_before)+r.slot&&
    r.serial_before==expected&&static_cast<std::int64_t>(expected)+8192==r.serial_after&&
    receipt.prefix.before_revision<UINT64_MAX&&receipt.prefix.after_revision==receipt.prefix.before_revision+1,
    "Serial checkpoint incomplete/unearned SetID abort receipt");expected=r.serial_after;}
  need(expected==raw,"Serial checkpoint raw burn history is incomplete");
 }
 const auto identity=p->host->identity();
 auto serial=make_bnw_unit_serial_checkpoint(p->state->world,p->source->snapshot,identity);
 need(p->source->source.source_sha256.has_value()||p->source->source.container==saves::SaveContainerKind::civ5,
  "Serial checkpoint original source identity unavailable");
 auto document=saves::make_civis_import_document(p->source->source,p->host->original_source_sha256_);
 const auto old=std::find_if(document.sections.begin(),document.sections.end(),[](const auto&s){return s.id==45;});
 if(old!=document.sections.end())*old=std::move(serial);else document.sections.push_back(std::move(serial));
 p->check();need(self.get()==this&&p->host->session().state_generation()==p->generation,
  "Serial checkpoint final canonical owner changed");return document;
}
void BnwNativeUnitAllocatorCore::require_current()const{const auto p=impl_;Impl::Guard g(p);p->check();}
std::shared_ptr<const BnwNativeUnitAllocatorCore> BnwNativeUnitAllocatorCore::retain_factory_owner()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign native Core canonical acquisition");
 auto actual=factory_self_.lock();need(actual&&actual.get()==this,"Native Core is not genuine factory canonical owner");return actual;
}
void BnwNativeUnitAllocatorCore::bind_completed_train_source(const ui::SelectedCityTrainCreateSource&supplied)const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();auto&b=*p->bank;
 need(!b.constructor_pending&&!b.iteration_started&&!b.train&&!p->train_source,"Native TRAIN source already bound or allocation begun");
 auto source=supplied.retain_factory_owner();need(source&&source.get()==&supplied,"Native TRAIN source canonical owner differs");
 source->require_for_allocator(*p->host,p->host->session(),p->actor);const auto args=source->allocator_operands();
 need(args.owner==p->actor&&args.city>=0&&args.unit>=0&&args.unit_ai>=0&&args.x>=0&&args.y>=0,
  "Native completed TRAIN source operands unavailable");
 TrainBinding binding{args.owner,args.city,args.unit,args.unit_ai,args.x,args.y,initial_best_land_prior_world_revision_without_callbacks()};
 // Once Source consumption starts, any later failure quarantines this Bank.
 // A half-bound continuation is never reusable or a successful TRAIN grant.
 try{
  source->bind_allocator_once(*self);source->require_for_allocator(*p->host,p->host->session(),p->actor);p->check();
  source->require_for_allocator(*p->host,p->host->session(),p->actor);
  compare(*p->state,p->actor,b.baseline);p->facade_current();
  need(!b.retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation&&!b.constructor_pending&&!b.train&&
   binding.before_revision==initial_best_land_prior_world_revision_without_callbacks(),"Native TRAIN source changed before binding");
  p->train_source=std::move(source);b.train_source=p->train_source;b.train=binding;
 }catch(...){b.retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::require_train_id_prefix(const ui::SelectedCityTrainCreateSource&supplied)const{
 // Called by the genuine Source while the actual Core mutation Guard is held.
 // No nested Guard, provider callback, SQL query or allocation on success.
 const auto self=retain_factory_owner();const auto p=impl_;p->facade_current();
 need(p->bank&&!p->bank->retired.load()&&p->train_source&&p->train_source.get()==&supplied,
  "Native TRAIN ID prefix has no SAME retained source");
 const auto&b=*p->bank;need(b.train&&!b.train_source.owner_before(p->train_source)&&!p->train_source.owner_before(b.train_source)&&
  b.train_source.lock().get()==&supplied&&b.last_id_prefix&&b.constructor_pending,
  "Native TRAIN ID prefix source/control missing");
 need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
  &p->host->session().source()==p->source&&p->host->identity().rules_sha256==p->fingerprint,
  "Native TRAIN ID prefix actual source allocation changed");
 const auto&prefix=*b.last_id_prefix;const auto&i=prefix.assignment;
 need(i.player==p->actor&&i.slot==b.pending_slot&&b.pending_units.contains(i.slot),"Native TRAIN ID prefix actual reserved slot differs");
 const auto unit=b.pending_units.at(i.slot);need(unit&&unit->id_assigned,"Native TRAIN ID prefix assigned node missing");
 check_registered(b,*unit,unit,i.slot);
 const auto&raw=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).units.header;
 need(raw.fields.size()==5&&name(raw.fields[4].name)=="current_id"&&raw.fields[4].value==i.serial_after&&b.serial==i.serial_after&&
  unit->scalars.at("m_iID")==i.full_id&&i.full_id==static_cast<std::int64_t>(i.serial_before)+i.slot&&
  i.serial_after==static_cast<std::int64_t>(i.serial_before)+8192&&prefix.before_revision==b.train->before_revision&&
  prefix.before_revision<UINT64_MAX&&prefix.after_revision==prefix.before_revision+1&&initial_best_land_prior_world_revision_without_callbacks()==prefix.after_revision,
  "Native TRAIN ID prefix raw/node/serial/revision bookends differ");
}
void BnwNativeUnitAllocatorCore::bind_trait_iterator_from_selected_preview(const ui::SelectedBnwReadOnlyPreview&preview)const{
 const auto p=impl_;Impl::Guard guard(p);p->check();
 need(!p->traits&&!p->bank->iteration_started&&!p->bank->train,"Native Traits iterator already bound/started or TRAIN-bound");
 // Authenticate SAME genuine selected Host before invoking its Traits factory.
 bool matched=false;preview.with_owned_host_camera_input([&](auto host,const auto*){
  need(host&&host.get()==p->host.get()&&!host.owner_before(p->host)&&!p->host.owner_before(host),
   "Native Traits iterator requires SAME canonical selected Host");matched=true;
 });need(matched,"Native Traits source handoff missing");p->check();
 auto traits=BnwMajorPlayerTraitsOwner::from_selected_preview(preview,p->actor);
 const auto snapshot=traits->snapshot();std::vector<std::int64_t> units;bool found=false;
 for(const auto&field:snapshot.cache)if(name(field.name)=="m_aFreeTraitUnits"){
  need(!found&&field.encoding==saves::PlayerTraitsEncoding::counted_pairs&&field.columns==2&&field.values.size()%2==0,
   "Actual complete native FreeTraitUnits cache shape invalid");found=true;units=field.values;}
 need(found&&snapshot.player==p->actor,"Actual SAME actor native free-unit cache unavailable");
 traits->require_current();p->check();
 // Only a native cache issued from SAME selected source initializes this
 // unsaved iterator. No full processTech prior-stage or creation receipt.
 if(!p->bank->trait_units.empty())need(p->bank->trait_units==units,"Native shared iterator cache changed");
 p->bank->trait_units.swap(units);p->traits=std::move(traits);
}
BnwNativeUnitSlotReservation BnwNativeUnitAllocatorCore::reserve_constructor_reset_prefix()const{
 const auto p=impl_;Impl::Guard guard(p);try{p->check();auto&b=*p->bank;
  need(!b.constructor_pending,"CvUnit constructor remainder required before another Add");
  auto next=b.next;auto slots=b.slots,last=b.last,head=b.head,count=b.free_count,serial=b.serial;
  if(slots==0){slots=8;last=-1;head=-1;count=0;serial=8192;next.assign(8,-1);}
  if(last==slots-1&&count==0){need(slots<=4096,"Native FLTA maximum buckets reached");slots*=2;next.resize(static_cast<std::size_t>(slots),-1);}
  std::int32_t slot=-1;
  if(count>0){slot=head;head=narrow(next.at(static_cast<std::size_t>(slot)));--count;}else{slot=++last;}
  need(slot>=0&&slot<slots,"Native FLTA selected slot invalid");
  const auto id=narrow(static_cast<std::int64_t>(serial)+slot);
  // Native constructor runs before SetID/currentID+=8192. This real private
  // object executes the first Reset interval, then pauses at its exact tail.
  auto unit=allocate_pending_unit(p->host);auto pending=b.pending_units;auto handles=b.registered_units;
  unit->canonical_self=unit;unit->registration_owner=p->bank;unit->registered_slot=slot;
  need(handles.emplace(slot,unit).second,"Native constructor slot already registered");
  archive_reset(*unit);
  for(const auto&[key,value]:constructor_prefix())assign_scalar(*unit,key.c_str(),value);
  need(pending.emplace(slot,unit).second,"Native FLTA slot already reserved");
  BnwNativeUnitSlotReservation result{p->actor,slot,id,slots,last,head,count,serial,next,unit->scalars};
  p->check();b.next.swap(next);b.pending_units.swap(pending);b.registered_units.swap(handles);b.slots=slots;b.last=last;b.head=head;b.free_count=count;b.serial=serial;b.constructor_pending=true;b.pending_slot=slot;
  // No raw world, owned roster, plots, gameplay RNG or serial stores occurred.
  return result;
 }catch(...){p->bank->retired=true;throw;}
}
BnwNativeUnitConstructorStorageSnapshot BnwNativeUnitAllocatorCore::execute_constructor_storage_tail()const{
 const auto p=impl_;Impl::Guard guard(p);try{p->check();auto&b=*p->bank;
  need(b.constructor_pending&&b.pending_slot>=0&&b.pending_units.contains(b.pending_slot),"Actual reserved pending constructor missing");
  const auto unit=b.pending_units.at(b.pending_slot);
  need(bool(unit),"Native registered pending unit null");check_registered(b,*unit,unit,b.pending_slot);
  need(unit&&!unit->storage_tail_executed&&unit->promotion_unit==unit.get(),"Native constructor storage already consumed or promotion unit mismatched");
  const auto actual_host=unit->promotion_host.lock();
  need(actual_host&&actual_host.get()==p->host.get()&&!actual_host.owner_before(p->host)&&!p->host.owner_before(actual_host),
   "Native constructor actual promotion source owner expired/replaced");
  // Stage every allocation/copy, then perform genuine source gates before
  // no-throw swaps into the SAME stable pending object; no world publication.
  PendingUnit staged=*unit;
  // The staging copy holds prepared storage, never a second registration or
  // self handle. Its destructor cannot erase the genuine live node's entry.
  staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  constructor_storage_tail(staged);
  auto result=storage_snapshot(b.pending_slot,staged);
  p->check();need(!unit->storage_tail_executed&&b.pending_units.at(b.pending_slot)==unit&&
   unit->promotion_unit==unit.get(),"Native pending storage changed before tail stores");
  unit->scalars.swap(staged.scalars);unit->indexed.swap(staged.indexed);unit->strings.swap(staged.strings);
  unit->dirty.swap(staged.dirty);unit->vector_clears.swap(staged.vector_clears);
  unit->last_path_destination=staged.last_path_destination;unit->storage_tail_executed=true;
  return result;
 }catch(...){p->bank->retired=true;throw;}
}
BnwNativeUnitArchiveObservation BnwNativeUnitAllocatorCore::pending_constructor_archive()const{
 const auto p=impl_;Impl::Guard guard(p);p->check();const auto&b=*p->bank;
 need(b.constructor_pending&&b.pending_units.contains(b.pending_slot),"Native pending archive unavailable");
 const auto unit=b.pending_units.at(b.pending_slot);need(bool(unit),"Native pending archive unit null");
 check_registered(b,*unit,unit,b.pending_slot);BnwNativeUnitArchiveObservation result;
 for(const auto&binding:unit->archive){result.binding_names.emplace_back(binding.spec.name);result.binding_members.emplace_back(binding.spec.member);
  // Resolve against genuine node storage; no stale scalar/vector DTO alias.
  if(binding.spec.kind==ArchiveKind::scalar)result.scalar_values.emplace(binding.spec.member,binding.target->scalars.at(binding.spec.member));
 }
 for(const auto i:unit->dirty)result.dirty_members.emplace_back(unit->archive.at(i).spec.member);
 for(const auto i:unit->vector_clears)result.vector_clear_members.emplace_back(unit->archive.at(i).spec.member);
 result.registered_lifetime=unit;p->check();check_registered(b,*unit,unit,b.pending_slot);return result;
}
void BnwNativeUnitAllocatorCore::discard_unpublished_constructor()const{
 const auto p=impl_;Impl::Guard guard(p);p->check();auto&b=*p->bank;
 need(!p->initial_set_xy,"Native unpublished constructor cannot abort an entered SetXY/popup hold; real tail reconciliation required");
 need(b.constructor_pending&&b.pending_units.contains(b.pending_slot),"Native unpublished constructor missing");
 need(!p->initial_plot_area||!p->initial_plot_area->stored,"Mandatory actual plot/area removal before linked newborn can be discarded");
 auto unit=b.pending_units.at(b.pending_slot);need(bool(unit),"Native unpublished unit null");check_registered(b,*unit,unit,b.pending_slot);
 // Abort only this private reservation; imported raw/owned/plot state was never
 // published. Rebuild tiny FLTA storage from the SAME unchanged baseline.
 auto aborted=b.aborted_serial_burns;
 if(unit->id_assigned){
  need(b.last_id_prefix&&b.last_id_prefix->assignment.player==p->actor&&
   b.last_id_prefix->assignment.slot==b.pending_slot&&b.last_id_prefix->assignment.full_id==unit->scalars.at("m_iID")&&
   b.last_id_prefix->assignment.serial_after==b.serial&&b.last_id_prefix->assignment.serial_before>=8192&&
   static_cast<std::int64_t>(b.last_id_prefix->assignment.serial_before)+8192==b.serial,
   "Native abort lost genuine SetID prefix");
  const auto expected=aborted.empty()?b.serial_origin:aborted.back().prefix.assignment.serial_after;
  need(b.last_id_prefix->assignment.serial_before==expected,"Native abort serial history has an unearned gap");
  aborted.push_back({*b.last_id_prefix,unit}); // Allocation BEFORE abort stores.
 }
 Bank restored;restore(restored,p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).units,p->actor);
 p->check();check_registered(b,*unit,unit,b.pending_slot);
 unit->self_handle=nullptr;b.registered_units.erase(b.pending_slot);b.pending_units.erase(b.pending_slot);
 b.next.swap(restored.next);b.slots=restored.slots;b.last=restored.last;b.head=restored.head;b.free_count=restored.free_count;b.serial=restored.serial;
 b.pending_slot=-1;b.constructor_pending=false;unit.reset(); // actual destructor/weak cleanup executes.
 b.aborted_serial_burns.swap(aborted); // Genuine destruction receipt; no callback/allocation.
}
BnwNativeUnitIdAssignment BnwNativeUnitAllocatorCore::assign_pending_unit_native_id()const{
 const auto p=impl_;Impl::Guard guard(p);p->check();auto&b=*p->bank;
 need(b.constructor_pending&&b.pending_slot>=0&&b.pending_units.contains(b.pending_slot),"Native ID assignment has no genuine reserved constructor");
 const auto unit=b.pending_units.at(b.pending_slot);need(bool(unit),"Native ID assignment pending unit null");
 check_registered(b,*unit,unit,b.pending_slot);
 // Repeating this caller operation is a no-effect refusal, not a new burn.
 // It does not retire an otherwise current bank or invalidate abort cleanup.
 need(unit->storage_tail_executed&&!unit->id_assigned,"Native constructor storage required or SetID already executed");
 try{
  const auto train=p->train_source;
  if(b.train){need(train&&b.train_source.lock().get()==train.get()&&!b.train_source.owner_before(train)&&!train.owner_before(b.train_source),"Native SetID TRAIN source lifetime/control changed");train->require_for_allocator(*p->host,p->host->session(),p->actor);}
  auto&raw=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).units.header;
  need(raw.fields.size()==5&&name(raw.fields[4].name)=="current_id","Native primary FLTA serial field unavailable");
  const auto before=narrow(raw.fields[4].value);const auto slot=b.pending_slot;
  need(before>=8192&&before%8192==0&&before==b.serial&&slot>=0&&slot<8192&&slot<b.slots&&
   static_cast<std::size_t>(slot)<b.next.size(),"Native primary/owned serial or reserved slot differs");
  const auto id=narrow(static_cast<std::int64_t>(before)+slot);
  const auto after=narrow(static_cast<std::int64_t>(before)+8192); // refuse signed overflow BEFORE SetID
  need((id&8191)==slot&&(id&~8191)==before,"Native ID generation/index bit partition differs");
  auto id_cell=unit->scalars.find(std::string_view("m_iID"));
  need(id_cell!=unit->scalars.end()&&id_cell->second==0,"Native constructor ID was externally assigned");
  const auto index=archive_index(*unit,"m_iID");need(index.has_value(),"Native SetID archive binding missing");
  auto dirty=unit->dirty;dirty.insert(*index);
  auto next=b.next;next[static_cast<std::size_t>(slot)]=-1; // Add clears slot link after ctor, BEFORE SetID
  std::vector<std::int64_t> baseline;
  semantic(*p->state,p->actor,[&](auto v){baseline.push_back(static_cast<std::int64_t>(v));},after);
  const auto revision=initial_best_land_prior_world_revision_without_callbacks();need(revision<UINT64_MAX,"Native allocation mutation revision overflow");
  BnwNativeUnitIdAssignment result{p->actor,slot,id,before,after};
  if(train)train->require_for_allocator(*p->host,p->host->session(),p->actor);
  p->check();check_registered(b,*unit,unit,slot);
  if(train){train->require_for_allocator(*p->host,p->host->session(),p->actor);compare(*p->state,p->actor,b.baseline);p->facade_current();need(!b.retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation,"Native TRAIN final SetID source changed");}
  need(raw.fields[4].value==before&&b.serial==before&&b.pending_slot==slot&&b.pending_units.at(slot)==unit&&
   unit->storage_tail_executed&&!unit->id_assigned&&id_cell->second==0&&initial_best_land_prior_world_revision_without_callbacks()==revision,
   "Native SetID source/owned operands changed before stores");
  // SDK Add236Р Р†Р вЂљРІР‚Сљ239: clear slot link; CvUnit::SetID12876 stores m_iID;
  // only THEN increment the genuine primary serial. No callback/allocations.
  b.next.swap(next);id_cell->second=id;unit->dirty.swap(dirty);unit->id_assigned=true;
  raw.fields[4].value=after;b.serial=after;b.baseline.swap(baseline);
  p->state->ui_source_revision=revision+1;b.last_id_prefix=NativeIdPrefix{result,revision,revision+1};
  if(train)train->accept_actual_unit_id_prefix(*this);
  // Raw FLTA membership/roster/plot links stay unpublished until full Init.
  // Aborting later restores from CURRENT raw serial and never revives this ID.
  return result;
 }catch(...){b.retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::require_completed_unit_initialization()const{
 const auto p=impl_;Impl::Guard guard(p);p->check();
 if(impl_->bank->constructor_pending&&impl_->bank->pending_units.contains(impl_->bank->pending_slot)&&
    impl_->bank->pending_units.at(impl_->bank->pending_slot)->id_assigned)
  throw BnwNativeUnitAllocatorUnavailable("Mandatory full CvUnit Init(false)/setXY/placement still uncompleted; native assigned ID is not world publication");
 throw BnwNativeUnitAllocatorUnavailable("Mandatory native Add SetID/currentID advance and full Init/setXY/placement not completed; registered constructor-owned archive alone is not world publication");
}
namespace {
BnwNativeTraitFreeUnitSelection select_free_unit(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,
 std::int32_t actor,const std::vector<std::int64_t>&units,std::size_t&index,std::int64_t tech){
 BnwNativeTraitFreeUnitSelection result;result.player=actor;result.technology=tech;
 while(index<units.size()/2&&units[index*2+1]!=tech)++index;
 if(index>=units.size()/2)return result;
 result.unit_class=narrow(units[index*2]);
 const auto roster=restore_pregame_roster(*state.world.pregame,host.database());
 const auto&binding=roster.slots.at(actor);need(binding.civilization>=0&&!binding.minor,"Native major civilization replacement binding missing");
 std::string civilization,class_type;std::int64_t unit=-1;bool found=false;
 const auto civ=host.database().query_rows({"SELECT Type FROM Civilizations WHERE ID=?",{binding.civilization}},[&](const auto&row){
  const auto type=row.text("Type");need(type.has_value()&&!found,"Native civilization Info absent/duplicate");
  const auto*canonical=host.types().find(*type);need(canonical&&canonical->table=="Civilizations"&&canonical->id==binding.civilization&&
  canonical->hash==content::saved_info_type_hash(*type),"Native civilization Info identity mismatch");civilization=*type;found=true;
 });need(civ.all_succeeded()&&found,"Native civilization Info query failed");found=false;
 const auto defaults=host.database().query_rows({"SELECT UnitClasses.Type, Units.ID FROM UnitClasses INNER JOIN Units ON Units.Type=DefaultUnit WHERE UnitClasses.ID=?",{std::int64_t(result.unit_class)}},[&](const auto&row){
  const auto type=row.text("Type");need(type.has_value()&&!found,"Native unit class default absent/duplicate");
  const auto*canonical=host.types().find(*type);need(canonical&&canonical->table=="UnitClasses"&&canonical->id==result.unit_class&&
  canonical->hash==content::saved_info_type_hash(*type),"Native unit class Info identity mismatch");class_type=*type;unit=row.integer_at(1);found=true;
 });need(defaults.all_succeeded()&&found,"Native unit class default query failed");
 // Same defaults-then-overrides source order as CvCivilizationInfo::CacheResults.
 const auto overrides=host.database().query_rows({"SELECT coalesce(Units.ID,-1) FROM Civilization_UnitClassOverrides INNER JOIN UnitClasses ON UnitClassType=UnitClasses.Type LEFT OUTER JOIN Units ON UnitType=Units.Type WHERE CivilizationType=? AND UnitClasses.Type=?",{civilization,class_type}},[&](const auto&row){unit=row.integer_at(0);});
 need(overrides.all_succeeded()&&unit>=0,"Native free unit replacement has no actual unit Info");result.unit=narrow(unit);found=false;
 const auto info=host.database().query_rows({"SELECT Type, DefaultUnitAI FROM Units WHERE ID=?",{unit}},[&](const auto&row){
  const auto type=row.text("Type"),ai=row.text("DefaultUnitAI");need(type&&ai&&!found,"Native free unit/default AI Info absent/duplicate");
  const auto*u=host.types().find(*type);const auto*a=host.types().find(*ai);
  need(u&&u->table=="Units"&&u->id==unit&&u->hash==content::saved_info_type_hash(*type)&&a&&a->table=="UnitAIInfos"&&a->id>=0&&
  a->hash==content::saved_info_type_hash(*ai),"Native unit/default AI Type remap unavailable");result.unit_ai=narrow(a->id);found=true;
 });need(info.all_succeeded()&&found,"Native selected unit/default AI query failed");return result;
}
}
BnwNativeUnitConstructorStorageSnapshot BnwNativeUnitAllocatorCore::execute_selected_trait_unit_init_reset(const ui::SelectedBnwReadOnlyPreview&preview)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->bank&&!p->bank->train,
  "Trait Init entry cannot accept completed TRAIN source");return execute_bound_unit_init_reset(preview);
}
BnwNativeUnitConstructorStorageSnapshot BnwNativeUnitAllocatorCore::execute_bound_unit_init_reset(const ui::SelectedBnwReadOnlyPreview&preview)const{
 const auto p=impl_;Impl::Guard guard(p);p->check();auto&b=*p->bank;
 need(b.train||(p->traits&&b.iteration_started&&b.selection_pending&&b.selected_unit&&b.selected_unit->unit>=0),
  "Mandatory genuine selected Trait unit source before native Init reset");
 need(b.constructor_pending&&b.pending_units.contains(b.pending_slot),"Native Init has no SAME reserved constructor");
 const auto unit=b.pending_units.at(b.pending_slot);need(bool(unit),"Native Init pending node unavailable");
 check_registered(b,*unit,unit,b.pending_slot);
 need(unit->id_assigned&&unit->storage_tail_executed&&!unit->init_reset_executed,"Native Init reset requires assigned ID or already executed");
 try{
  // SetID legitimately changes the UI revision. Reissue current native Traits
  // against the SAME actual Host; do not relax the old owner's revision guard.
  bool same=false;preview.with_owned_host_camera_input([&](auto host,const auto*){
   need(host&&host.get()==p->host.get()&&!host.owner_before(p->host)&&!p->host.owner_before(host),
    "Native Init requires SAME genuine selected Host");same=true;
  });need(same,"Native Init actual selected source handoff missing");p->check();
  std::shared_ptr<const BnwMajorPlayerTraitsOwner> current_traits;
  auto index=b.iterator;BnwNativeTraitFreeUnitSelection selected;
  const auto train=p->train_source;
  if(b.train){
   need(train&&b.train_source.lock().get()==train.get()&&!b.train_source.owner_before(train)&&!train.owner_before(b.train_source),
    "Native TRAIN Init source lifetime/control differs");
   train->require_for_allocator(*p->host,p->host->session(),p->actor);const auto args=train->allocator_operands();
   need(args.owner==b.train->owner&&args.city==b.train->city&&args.unit==b.train->unit&&args.unit_ai==b.train->unit_ai&&
    args.x==b.train->x&&args.y==b.train->y,"Actual completed TRAIN arguments changed before Init");
   selected.player=args.owner;selected.unit=args.unit;selected.unit_ai=args.unit_ai;
  }else{
  current_traits=BnwMajorPlayerTraitsOwner::from_selected_preview(preview,p->actor);
  const auto current_cache=current_traits->snapshot();bool free_units_seen=false;
  for(const auto&field:current_cache.cache)if(name(field.name)=="m_aFreeTraitUnits"){
   need(!free_units_seen&&field.values==b.trait_units,"Native Init current genuine Trait free-unit cache differs");free_units_seen=true;
  }need(free_units_seen,"Native Init actual Trait free-unit cache absent");
  current_traits->require_current();index=b.iterator;
  selected=select_free_unit(*p->host,*p->state,p->actor,b.trait_units,index,b.iterator_tech);
  need(index==b.iterator&&selected.player==b.selected_unit->player&&selected.unit_class==b.selected_unit->unit_class&&
   selected.unit==b.selected_unit->unit&&selected.unit_ai==b.selected_unit->unit_ai&&selected.technology==b.iterator_tech,
   "Actual internal Trait unit source changed before native Init");
  bool has=false;const auto team=p->state->setup.players.at(p->actor).team;
  for(const auto&tech:p->state->technologies)if(tech.team()==team)has=tech.has_tech(b.iterator_tech);
  need(has,"Native technology free unit Init requires actual HasTech poststate");
  }
  const auto id=unit->scalars.at("m_iID");need(id>=8192&&(id&8191)==b.pending_slot&&(id&~8191)<b.serial,
   "Native Init assigned SAME node ID generation differs");
  std::optional<std::int32_t> combat;const auto info=p->host->database().query_rows({"SELECT Type, Combat FROM Units WHERE ID=?",{std::int64_t(selected.unit)}},[&](const auto&row){
   const auto type=row.text("Type");const auto*entry=type?p->host->types().find(*type):nullptr;
   need(!combat&&entry&&entry->table=="Units"&&entry->id==selected.unit&&entry->hash==content::saved_info_type_hash(*type),
    "Native Init selected UnitInfo absent/changed");combat=narrow(row.integer_at(1));
  });need(info.all_succeeded()&&combat.has_value(),"Native Init actual UnitInfo Combat unavailable");
  const auto terrains=native_init_info_span(*p->host,"Terrains"),features=native_init_info_span(*p->host,"Features");
  const auto combats=native_init_info_span(*p->host,"UnitCombatInfos"),classes=native_init_info_span(*p->host,"UnitClasses");
  PendingUnit staged=*unit;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  // Init order: genuine promotions.Init(SAME unit), Religion.Init, then Reset.
  staged.promotion_unit=unit.get();staged.promotion_host=p->host;staged.promotion_bits.clear();
  staged.religion=-1;staged.religion_strength=0;staged.religion_spreads=0;
  archive_reset(staged);for(const auto&[key,value]:constructor_prefix(id))assign_scalar(staged,key.c_str(),value);
  constructor_storage_tail(staged,p->actor,selected.unit,*combat);native_init_indexed_reset(staged,terrains,features,combats,classes);
  auto result=storage_snapshot(b.pending_slot,staged);
  if(train)train->require_for_allocator(*p->host,p->host->session(),p->actor);else current_traits->require_current();
  p->check();check_registered(b,*unit,unit,b.pending_slot);
  if(train){train->require_for_allocator(*p->host,p->host->session(),p->actor);compare(*p->state,p->actor,b.baseline);p->facade_current();need(!b.retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation,"Native TRAIN final Reset source changed");}
  need(unit->id_assigned&&!unit->init_reset_executed&&unit->scalars.at("m_iID")==id&&
   (b.train?b.train->unit==selected.unit&&b.train->unit_ai==selected.unit_ai:
    b.selected_unit&&b.selected_unit->unit==selected.unit&&b.selected_unit->unit_ai==selected.unit_ai&&b.iterator==index&&b.selection_pending),
   "Native Init pending source changed before stores");
  // No allocations/callbacks after gates; preserve genuine binding targets.
  unit->scalars.swap(staged.scalars);unit->indexed.swap(staged.indexed);unit->strings.swap(staged.strings);
  unit->dirty.swap(staged.dirty);unit->vector_clears.swap(staged.vector_clears);unit->promotion_bits.swap(staged.promotion_bits);
  unit->religion=staged.religion;unit->religion_strength=staged.religion_strength;unit->religion_spreads=staged.religion_spreads;
  unit->last_path_destination=staged.last_path_destination;unit->init_reset_executed=true;if(current_traits)p->traits=std::move(current_traits);
  return result; // Mandatory facing/hover promotions/setXY and world publication remain.
 }catch(...){b.retired=true;throw;}
}
BnwNativeUnitConstructorStorageSnapshot BnwNativeUnitAllocatorCore::execute_bound_unit_initial_facing_hover_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();auto&b=*p->bank;
 need(b.constructor_pending&&b.pending_units.contains(b.pending_slot),"Initial facing has no genuine constructor node");
 const auto unit=b.pending_units.at(b.pending_slot);check_registered(b,*unit,unit,b.pending_slot);
 need(unit->id_assigned&&unit->init_reset_executed&&!unit->initial_facing_executed,
  "Initial facing requires actual Reset(false) and is once-only");
 const auto train=p->train_source;
 const auto source_current=[&]{
  if(b.train){need(train&&b.train_source.lock().get()==train.get()&&!b.train_source.owner_before(train)&&!train.owner_before(b.train_source),
    "Initial facing actual TRAIN source/control differs");train->require_for_allocator(*p->host,p->host->session(),p->actor);
   const auto args=train->allocator_operands();need(args.unit==b.train->unit&&args.unit_ai==b.train->unit_ai&&args.owner==p->actor,
    "Initial facing actual TRAIN UnitInfo source differs");
  }else{need(p->traits&&b.selected_unit&&b.selection_pending,"Initial facing has no genuine internal Unit source");p->traits->require_current();}
 };
 source_current();const auto type_id=unit->scalars.at("m_eUnitType");
 need(type_id>=0&&unit->scalars.at("m_eOwner")==p->actor&&unit->scalars.at("m_iX")==-INT32_MAX&&
  unit->scalars.at("m_iY")==-INT32_MAX&&unit->promotion_bits.empty(),"Initial facing node already placed/promoted or owner differs");
 need(b.train?type_id==b.train->unit:type_id==b.selected_unit->unit,"Initial facing actual selected type differs");
 std::string type;std::size_t unit_rows=0;
 const auto info=p->host->database().query_rows({"SELECT Type FROM Units WHERE ID=?",{std::int64_t(type_id)}},[&](const auto&r){
  ++unit_rows;type=r.text("Type").value_or("");const auto*e=p->host->types().find(type);
  need(e&&e->table=="Units"&&e->id==type_id&&e->hash==content::saved_info_type_hash(type),"Initial facing native UnitInfo remap differs");
 });need(info.all_succeeded()&&unit_rows==1,"Initial facing genuine UnitInfo absent");
 const auto&ids=unit->promotion_info_ids;
 need(native_init_info_span(*p->host,"UnitPromotions")==ids.size(),"Initial facing promotion universe differs from SAME Init owner");
 std::map<std::int32_t,bool> hover;
 const auto promotions=p->host->database().query_rows({"SELECT ID,Type,HoveringUnit FROM UnitPromotions WHERE ID > -1 ORDER BY ID",{}},[&](const auto&r){
  const auto id=narrow(r.integer_at(0));const auto promo=r.text("Type").value_or("");const auto*e=p->host->types().find(promo);
  const auto flag=r.integer_at(2);need(e&&e->table=="UnitPromotions"&&e->id==id&&e->hash==content::saved_info_type_hash(promo)&&
   id>=0&&static_cast<std::size_t>(id)<ids.size()&&ids[static_cast<std::size_t>(id)]==id&&
   (flag==0||flag==1)&&hover.emplace(id,flag!=0).second,"Initial facing actual PromotionInfo identity/hover flag differs");
 });need(promotions.all_succeeded()&&promotions.rows==hover.size(),"Initial facing complete native PromotionInfo table absent");
 std::size_t present_promotions=0;for(auto id:ids){if(id<0){continue;}++present_promotions;need(hover.contains(id),"Initial facing PromotionInfo slot absent");}
 need(hover.size()==present_promotions,"Initial facing actual PromotionInfo presence differs");
 std::map<std::int32_t,bool> free;
 const auto relation=p->host->database().query_rows({"SELECT PromotionType FROM Unit_FreePromotions WHERE UnitType=?",{type}},[&](const auto&r){
  const auto text=r.text("PromotionType").value_or("");const auto*e=p->host->types().find(text);
  need(e&&e->table=="UnitPromotions"&&e->id>=0&&hover.contains(narrow(e->id)),"Initial facing free PromotionType unresolved");
  free.insert_or_assign(narrow(e->id),true); // PopulateArrayByExistence: duplicates are true, not additions.
 });need(relation.all_succeeded(),"Initial facing complete Unit_FreePromotions relation unavailable");
 // CvUnit::Init365-386 executes the full Info-ordered loop. No promotion is
 // skipped: a reached hover grant refuses BEFORE any facing/map/goody store
 // until the real setHasPromotion effects + selected-UI branch are supplied.
 for(auto id:ids){if(id<0){continue;}need(!free.contains(id)||!hover.at(id),"Mandatory reached free Hovering promotion native effects unavailable");}
 PendingUnit staged=*unit;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
 // CvPlayer::initUnit defaults for the actual TRAIN/CreateUnit and current
 // isolated Trait Init path: NO_DIRECTION, layer0, goody0. Not caller values.
 assign_scalar(staged,"m_eFacingDirection",2); // DIRECTION_SOUTHEAST
 assign_scalar(staged,"m_iMapLayer",0);assign_scalar(staged,"m_iNumGoodyHutsPopped",0);
 auto result=storage_snapshot(b.pending_slot,staged);
 p->check();source_current();compare(*p->state,p->actor,b.baseline);p->facade_current();check_registered(b,*unit,unit,b.pending_slot);
 need(!b.retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
  unit->init_reset_executed&&!unit->initial_facing_executed&&unit->scalars.at("m_eUnitType")==type_id,
  "Initial facing actual node/source changed before stores");
 unit->scalars.swap(staged.scalars);unit->dirty.swap(staged.dirty);unit->initial_facing_executed=true;
 return result; // Next actual call is setXY; no map, roster or full Init publication.
}
BnwNativeUnitAllocatorCore::InitialPlacementOperands BnwNativeUnitAllocatorCore::initial_placement_operands()const{
 // Only existing SetXY owner, synchronously inside this Core's Guard.
 const auto self=retain_factory_owner();const auto p=impl_;p->check();const auto&b=*p->bank;
 need(b.train&&p->train_source,"Trait addFreeUnit capital/starting-plot/RNG placement predecessors unavailable");
 p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);const auto args=p->train_source->allocator_operands();
 need(b.constructor_pending&&b.pending_units.contains(b.pending_slot),"Initial SetXY requires SAME reserved node");
 const auto u=b.pending_units.at(b.pending_slot);check_registered(b,*u,u,b.pending_slot);
 need(u->initial_facing_executed&&u->init_reset_executed&&u->id_assigned&&args.owner==p->actor&&args.city==b.train->city&&
  args.unit==b.train->unit&&args.x==b.train->x&&args.y==b.train->y,"Initial SetXY genuine source operands changed");
 compare(*p->state,p->actor,b.baseline);p->facade_current();
 return {p->host,p->host->session_.get(),p->state,p->actor,u->scalars.at("m_iID"),args.x,args.y,args.city,u->scalars.at("m_iBaseCombat"),p->generation,initial_best_land_prior_world_revision_without_callbacks()};
}
void BnwNativeUnitAllocatorCore::require_initial_placement_current(const BnwUnitMovePending* pending)const{
 // Existing SetXY owns the phase. This proof never enters a nested Guard or
 // fetches a published roster record for a genuine unpublished newborn.
 const auto self=retain_factory_owner();const auto p=impl_;p->facade_current();const auto&b=*p->bank;
 need(pending&&(!p->initial_set_xy||p->initial_set_xy.get()==pending)&&!b.retired.load()&&b.train&&p->train_source&&
  b.constructor_pending&&b.pending_units.contains(b.pending_slot),"Initial SetXY native Core phase retired");
 const auto u=b.pending_units.at(b.pending_slot);check_registered(b,*u,u,b.pending_slot);
 p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
 compare(*p->state,p->actor,b.baseline);
 need(!b.retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
  &p->host->session().source()==p->source&&p->host->identity().rules_sha256==p->fingerprint&&
  u->initial_facing_executed&&u->init_reset_executed&&u->id_assigned&&u->scalars.at("m_eOwner")==p->actor&&
  u->scalars.at("m_iID")>=8192&&u->scalars.at("m_iX")==
   (u->initial_coordinates_executed?b.train->x:-INT32_MAX)&&u->scalars.at("m_iY")==
   (u->initial_coordinates_executed?b.train->y:-INT32_MAX)&&
  u->scalars.at("m_iGarrisonCityID")==-1&&u->scalars.at("m_eActivityType")==0&&
  u->scalars.at("m_bSetUpForRangedAttack")==0&&u->scalars.at("m_iMapLayer")==0&&
  u->scalars.at("m_transportUnit.owner")==-1&&u->scalars.at("m_transportUnit.id")==-1&&
  u->scalars.at("m_combatUnit.owner")==-1&&u->scalars.at("m_iCombatTimer")==0&&u->scalars.at("m_bDeathDelay")==0,
  "Initial SetXY SAME source/node reset predicates differ");
 p->facade_current();
}
std::shared_ptr<const BnwUnitMovePending> BnwNativeUnitAllocatorCore::begin_initial_set_xy_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(!p->initial_set_xy,"Initial SetXY already attempted; no replay");
 const auto&b=*p->bank;need(b.train&&p->train_source&&b.constructor_pending&&b.pending_units.contains(b.pending_slot)&&
  b.pending_units.at(b.pending_slot)->initial_facing_executed,"Initial SetXY requires actual completed TRAIN and facing/hover predecessor");
 try{
  auto pending=BnwUnitMovePending::create_for_initial_unit(self);
  p->initial_set_xy=pending; // Retain real phase before actual UI setter side effect.
  pending->execute_initial_set_xy_prefix();return pending;
 }catch(...){p->bank->retired=true;throw;} // Popup stores are never rolled back on later failure.
}
BnwNativeUnitAllocatorCore::InitialVisibilityOpeningBinding
BnwNativeUnitAllocatorCore::capture_initial_visibility_opening_before_assignment()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(p->train_source&&p->bank->train,"Scene opening requires genuine completed TRAIN source");
 p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
 const auto&b=*p->bank;
 need(b.constructor_pending&&b.pending_units.contains(b.pending_slot),"Scene opening actual reserved node missing");
 const auto unit=b.pending_units.at(b.pending_slot);check_registered(b,*unit,unit,b.pending_slot);
 const auto&t=*b.train;
 InitialVisibilityOpeningBinding result{this,self,p->host,p->host->session_.get(),p->state,p->source,p->fingerprint,
  p->bank.get(),p->bank,unit.get(),unit,p->train_source,p->actor,b.pending_slot,b.serial,t.city,t.unit,t.unit_ai,t.x,t.y,
  p->generation,t.before_revision};
 // Any allocation/provider activity precedes the final actual pure bookend.
 p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
 require_initial_visibility_opening_current_without_callbacks(result);return result;
}
void BnwNativeUnitAllocatorCore::require_initial_visibility_opening_identity_without_callbacks(
 const InitialVisibilityOpeningBinding&opening)const{
 const auto p=impl_;
 need(p&&p->thread==std::this_thread::get_id(),"Foreign Scene opening before live borrow");
 need(p->facade_token&&p->facade_token->thread==p->thread&&!p->facade_token->retired.load()&&
  !p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load(),"Scene opening actual facade token retired");
 need(opening.core==this&&!factory_self_.expired()&&!opening.core_control.expired()&&
  !opening.core_control.owner_before(factory_self_)&&!factory_self_.owner_before(opening.core_control),
  "Scene opening genuine canonical Core control differs");
 need(opening.host&&p->host.get()==opening.host.get()&&!p->host.owner_before(opening.host)&&!opening.host.owner_before(p->host)&&
  opening.session==p->host->session_.get()&&opening.session==p->facade_token->facade&&opening.state.get()==p->state.get()&&
  !opening.state.owner_before(p->state)&&!p->state.owner_before(opening.state),"Scene opening SAME Host/session/state controls differ");
 const auto&s=*opening.session;
 need(s.state_==p->state&&s.source_.get()==p->source&&opening.source==p->source&&s.state_generation_==p->generation&&
  opening.generation==p->generation&&opening.fingerprint==p->fingerprint&&p->host->identity().rules_sha256==p->fingerprint,
  "Scene opening actual world/source/rules generation differs");
 need(p->bank&&opening.bank==p->bank.get()&&!p->bank->retired.load()&&!opening.bank_control.expired()&&
  !opening.bank_control.owner_before(p->bank)&&!p->bank.owner_before(opening.bank_control),"Scene opening SAME canonical Bank differs");
 const auto&b=*p->bank;const auto*registry=p->state->setup.extensions.find<Registry>();
 need(p->registry_installed&&registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor).get()==p->bank.get()&&
  !registry->players.at(p->actor).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(p->actor))&&
  !b.source.expired()&&!b.source.owner_before(p->state)&&!p->state.owner_before(b.source)&&b.generation==p->generation&&b.fingerprint==p->fingerprint,
  "Scene opening genuine SAME-state Bank registry retired");
 need(b.constructor_pending&&b.pending_slot==opening.slot&&b.pending_units.contains(opening.slot),"Scene opening reserved slot changed");
 const auto&unit=b.pending_units.at(opening.slot);const auto registered=b.registered_units.find(opening.slot);
 need(unit&&unit.get()==opening.node&&!opening.node_control.expired()&&!opening.node_control.owner_before(unit)&&!unit.owner_before(opening.node_control)&&
  registered!=b.registered_units.end()&&!registered->second.expired()&&!registered->second.owner_before(unit)&&!unit.owner_before(registered->second)&&
  unit->registered_slot==opening.slot&&unit->self_handle==unit.get()&&!unit->registration_owner.expired()&&
  !unit->registration_owner.owner_before(p->bank)&&!p->bank.owner_before(unit->registration_owner)&&!unit->canonical_self.expired()&&
  !unit->canonical_self.owner_before(unit)&&!unit.owner_before(unit->canonical_self)&&unit->storage_tail_executed,
  "Scene opening actual registered constructor node/control differs");
 for(const auto&entry:unit->archive)need(entry.target==unit.get(),"Scene opening actual archive detached from node");
 need(p->train_source&&!opening.train_control.expired()&&!opening.train_control.owner_before(p->train_source)&&
  !p->train_source.owner_before(opening.train_control)&&!b.train_source.expired()&&!b.train_source.owner_before(p->train_source)&&
  !p->train_source.owner_before(b.train_source)&&b.train.has_value(),"Scene opening SAME genuine TRAIN issuer retired");
 const auto&t=*b.train;
 need(opening.owner==p->actor&&t.owner==opening.owner&&t.city==opening.city&&t.unit==opening.type&&t.unit_ai==opening.unit_ai&&
  t.x==opening.x&&t.y==opening.y&&t.before_revision==opening.before_revision,
  "Scene opening immutable actual TRAIN arguments/revision changed");
 compare(*p->state,p->actor,b.baseline); // existing allocation-free genuine raw/owned manager seal.
 need(s.state_==p->state&&s.source_.get()==p->source&&s.state_generation_==p->generation&&!b.retired.load(),
  "Scene opening final actual source bookend differs");
}
void BnwNativeUnitAllocatorCore::require_initial_visibility_opening_current_without_callbacks(
 const InitialVisibilityOpeningBinding&opening)const{
 require_initial_visibility_opening_identity_without_callbacks(opening);const auto p=impl_;const auto&b=*p->bank;
 const auto&unit=b.pending_units.at(opening.slot);
 const auto&raw=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).units.header;
 need(!unit->id_assigned&&unit->scalars.at("m_iID")==0&&!b.last_id_prefix&&!p->initial_set_xy&&!p->initial_hidden_binding&&!p->initial_hidden_proof&&
  opening.serial_before>=8192&&opening.serial_before%8192==0&&b.serial==opening.serial_before&&opening.slot>=0&&opening.slot<8192&&
  raw.fields.size()==5&&name(raw.fields[4].name)=="current_id"&&raw.fields[4].value==opening.serial_before&&
  initial_best_land_prior_world_revision_without_callbacks()==opening.before_revision,"Scene opening is not the genuine PRE-SetID interval");
}
void BnwNativeUnitAllocatorCore::require_initial_visibility_hidden_slot_descends_from_opening_without_callbacks(
 const InitialVisibilityOpeningBinding&opening,const graphics::BnwPendingUnitHiddenSlotProof&proof)const{
 require_initial_visibility_opening_identity_without_callbacks(opening);const auto p=impl_;const auto&b=*p->bank;
 need(b.last_id_prefix&&p->initial_hidden_binding&&p->initial_hidden_proof.get()==&proof,
  "Scene hidden slot lacks actual opening/native SetID descendant receipt");
 const auto&i=*b.last_id_prefix;const auto&assigned=i.assignment;const auto&unit=b.pending_units.at(opening.slot);
 const auto&raw=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).units.header;
 need(opening.before_revision<UINT64_MAX&&i.before_revision==opening.before_revision&&i.after_revision==opening.before_revision+1&&
  assigned.player==opening.owner&&assigned.slot==opening.slot&&assigned.serial_before==opening.serial_before&&
  std::int64_t(assigned.serial_after)==std::int64_t(opening.serial_before)+8192&&
  std::int64_t(assigned.full_id)==std::int64_t(opening.serial_before)+opening.slot&&unit->id_assigned&&unit->scalars.at("m_iID")==assigned.full_id&&
  raw.fields.size()==5&&name(raw.fields[4].name)=="current_id"&&raw.fields[4].value==assigned.serial_after&&b.serial==assigned.serial_after&&
  p->initial_hidden_binding->node==opening.node&&p->initial_hidden_binding->id==assigned.full_id&&
  p->initial_hidden_binding->revision==i.after_revision,
  "Scene hidden slot is not SAME actual native SetID receipt descendant");
 require_initial_visibility_hidden_slot_identity_without_callbacks(*p->initial_hidden_binding,proof);
 // This existing pure validator recognizes only earned ID/garrison/sight
 // journals; it never rebases a stale frame or grants arbitrary later phases.
}

BnwNativeUnitAllocatorCore::InitialVisibilityBinding
BnwNativeUnitAllocatorCore::capture_initial_visibility_binding(const BnwUnitMovePending& supplied)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 need(p->initial_set_xy&&p->initial_set_xy.get()==&supplied&&p->bank&&
  p->bank->pending_units.contains(p->bank->pending_slot),"Initial visibility requires actual retained Core/Pending/node");
 // Pin the actual Pending and retain real bank/node through strong Impl BEFORE
 // source/client callbacks. This is never minted from caller operands alone.
 const auto pending=p->initial_set_xy;const auto&b=*p->bank;
 const auto unit=b.pending_units.at(b.pending_slot);
 need(unit&&b.train,"Initial visibility actual node/Train missing");
 InitialVisibilityBinding binding{p->host,p->host->session_.get(),p->state,pending,unit.get(),unit,self,
  p->actor,unit->scalars.at("m_iID"),unit->scalars.at("m_iX"),unit->scalars.at("m_iY"),b.train->city,
  unit->scalars.at("m_iBaseCombat"),p->generation,initial_best_land_prior_world_revision_without_callbacks()};
 require_initial_visibility_binding_current(binding);return binding;
}
void BnwNativeUnitAllocatorCore::require_initial_visibility_binding_controls_without_callbacks(
 const InitialVisibilityBinding&binding)const{
 const auto p=impl_;
 need(p&&p->thread==std::this_thread::get_id(),"Foreign initial visibility before live borrow");
 need(p->facade_token&&p->facade_token->thread==p->thread&&!p->facade_token->retired.load()&&
  !p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load(),"Initial visibility actual facade token retired");
 need(!factory_self_.expired()&&!binding.core_control.owner_before(factory_self_)&&!factory_self_.owner_before(binding.core_control)&&
  binding.host&&p->host.get()==binding.host.get()&&!p->host.owner_before(binding.host)&&!binding.host.owner_before(p->host)&&
  binding.session==p->host->session_.get()&&p->facade_token->facade==binding.session&&binding.state.get()==p->state.get()&&
  !binding.state.owner_before(p->state)&&!p->state.owner_before(binding.state),"Initial visibility canonical Core/Host/session controls differ");
 const auto&session=*binding.session;
 need(session.state_==p->state&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  binding.generation==p->generation&&(binding.revision==initial_happiness_prior_revision_without_callbacks()||(p->initial_garrison&&p->initial_garrison->stored&&
   binding.revision==p->initial_garrison->before_revision&&
   (initial_happiness_prior_revision_without_callbacks()==p->initial_garrison->after_revision||(p->initial_sight_stored&&
    p->initial_sight_before==p->initial_garrison->after_revision&&(initial_happiness_prior_revision_without_callbacks()==p->initial_sight_after||(p->initial_plot_area&&p->initial_plot_area->stored&&
     p->initial_plot_area->before_revision==p->initial_sight_after&&(initial_happiness_prior_revision_without_callbacks()==p->initial_plot_area->after_revision||(p->initial_city_strength&&p->initial_city_strength->stored&&
      p->initial_city_strength->before_revision==p->initial_plot_area->after_revision&&initial_happiness_prior_revision_without_callbacks()==p->initial_city_strength->after_revision))))))))&&
  p->host->identity().rules_sha256==p->fingerprint,"Initial visibility actual source/rules identity changed");
 need(p->bank&&!p->bank->retired.load()&&p->bank->constructor_pending&&p->bank->train&&
  p->initial_set_xy&&binding.pending.get()==p->initial_set_xy.get()&&
  !binding.pending.owner_before(p->initial_set_xy)&&!p->initial_set_xy.owner_before(binding.pending),
  "Initial visibility actual pending control/phase missing");
 const auto&b=*p->bank;
 need(!b.source.expired()&&!b.source.owner_before(p->state)&&!p->state.owner_before(b.source)&&
  b.generation==p->generation&&b.fingerprint==p->fingerprint,"Initial visibility native bank source control changed");
 const auto*registry=p->state->setup.extensions.find<Registry>();
 need(p->registry_installed&&registry&&registry->players.contains(p->actor)&&
  registry->players.at(p->actor).get()==p->bank.get()&&!registry->players.at(p->actor).owner_before(p->bank)&&
  !p->bank.owner_before(registry->players.at(p->actor)),"Initial visibility actual native bank registry replaced");
 const auto i=b.pending_units.find(b.pending_slot);
 need(i!=b.pending_units.end()&&i->second,"Initial visibility actual reserved node missing");
 const auto&unit=i->second;const auto registered=b.registered_units.find(b.pending_slot);
 need(unit.get()==binding.node&&!binding.node_control.owner_before(unit)&&!unit.owner_before(binding.node_control)&&
  registered!=b.registered_units.end()&&!registered->second.expired()&&
  !registered->second.owner_before(unit)&&!unit.owner_before(registered->second)&&unit->self_handle==unit.get()&&
  unit->registered_slot==b.pending_slot&&!unit->registration_owner.expired()&&
  !unit->registration_owner.owner_before(p->bank)&&!p->bank.owner_before(unit->registration_owner)&&
  !unit->canonical_self.expired()&&!unit->canonical_self.owner_before(unit)&&!unit.owner_before(unit->canonical_self),
  "Initial visibility SAME canonical node/handle registration differs");
 for(const auto&entry:unit->archive)need(entry.target==unit.get(),"Initial visibility archive detached from actual node");
 need(unit->id_assigned&&unit->init_reset_executed&&unit->initial_facing_executed&&unit->initial_coordinates_executed&&
  binding.owner==p->actor&&binding.city==b.train->city&&binding.id==unit->scalars.at("m_iID")&&
  binding.x==unit->scalars.at("m_iX")&&binding.y==unit->scalars.at("m_iY")&&binding.x==b.train->x&&binding.y==b.train->y&&
  binding.base_combat==unit->scalars.at("m_iBaseCombat"),"Initial visibility immutable actual node fields changed");
 if(p->initial_garrison&&p->initial_garrison->stored){
  const auto&g=*p->initial_garrison;
  need(g.before_revision==binding.revision&&
   (g.after_revision==initial_happiness_prior_revision_without_callbacks()||(p->initial_sight_stored&&g.after_revision==p->initial_sight_before&&
    (p->initial_sight_after==initial_happiness_prior_revision_without_callbacks()||(p->initial_plot_area&&p->initial_plot_area->stored&&
     p->initial_plot_area->before_revision==p->initial_sight_after&&(initial_happiness_prior_revision_without_callbacks()==p->initial_plot_area->after_revision||(p->initial_city_strength&&p->initial_city_strength->stored&&
      p->initial_city_strength->before_revision==p->initial_plot_area->after_revision&&initial_happiness_prior_revision_without_callbacks()==p->initial_city_strength->after_revision))))&&p->initial_sight_before<UINT64_MAX&&p->initial_sight_after==p->initial_sight_before+1))&&
   g.after_revision==g.before_revision+(g.modifier!=0?1u:0u),"Initial hidden slot garrison lineage differs");
  need(p->state->world.all_players&&p->state->world.all_players->layout==bnw&&p->actor>=0&&
   static_cast<std::size_t>(p->actor)<p->state->world.all_players->players.size(),"Initial hidden slot actual city roster absent");
  const saves::SavedCityObservation*actual=nullptr;
  for(const auto&city:p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).cities.cities)if(&city==g.city)actual=&city;
  need(actual&&actual->core.layout==bnw&&actual->core.start_offset==g.core_start&&actual->core.end_offset==g.core_end&&
   &garrison_named(actual->core.fields,"m_iJONSCulturePerTurnFromPolicies")==g.field,
   "Initial hidden slot garrison storage replaced");
  need(g.field->encoding==saves::CityCoreEncoding::i32&&g.field->values.size()==1&&g.field->values[0]==g.after&&
   unit->scalars.at("m_iFortifyTurns")==0,"Initial hidden slot actual garrison/fortify prefix changed");
  compare_garrison_policies(*p->state,p->actor,g.policies);
 }
 p->initial_set_xy->require_initial_visibility_core_identity_without_callbacks(*this);
}
void BnwNativeUnitAllocatorCore::require_initial_visibility_binding_identity_without_callbacks(const InitialVisibilityBinding&binding)const{
 require_initial_visibility_binding_controls_without_callbacks(binding);
 impl_->initial_set_xy->require_initial_visibility_phase_without_callbacks();
}
void BnwNativeUnitAllocatorCore::require_initial_visibility_hidden_slot_identity_without_callbacks(
 const InitialVisibilityBinding&binding,const graphics::BnwPendingUnitHiddenSlotProof&proof)const{
 require_initial_visibility_binding_controls_without_callbacks(binding);const auto p=impl_;
 need(p->initial_hidden_binding&&p->initial_hidden_proof.get()==&proof&&!p->initial_hidden_receiver.expired()&&
  !proof.owner_.owner_before(p->initial_hidden_receiver)&&!p->initial_hidden_receiver.owner_before(proof.owner_)&&proof.slot_&&
  !proof.node_.owner_before(binding.node_control)&&!binding.node_control.owner_before(proof.node_)&&
  !proof.core_.owner_before(factory_self_)&&!factory_self_.owner_before(proof.core_)&&
  !proof.pending_.owner_before(p->initial_set_xy)&&!p->initial_set_xy.owner_before(proof.pending_),
  "Initial hidden slot is not the actual stored renderer return/control");
 const auto&issued=*p->initial_hidden_binding;
 need(issued.node==binding.node&&issued.pending.get()==binding.pending.get()&&issued.state.get()==binding.state.get()&&
  issued.session==binding.session&&issued.owner==binding.owner&&issued.id==binding.id&&issued.city==binding.city&&
  issued.x==binding.x&&issued.y==binding.y&&issued.generation==binding.generation&&issued.revision==binding.revision,
  "Initial hidden slot binding differs from actual issued successor");
 p->initial_set_xy->require_initial_visibility_hidden_slot_phase_without_callbacks();
 if(p->initial_sight_stored){need(bool(p->initial_sight),"Initial hidden slot actual sight owner missing");
  p->initial_sight->require_after_without_callbacks(*this);}
 if(p->initial_plot_area&&p->initial_plot_area->stored)require_initial_plot_area_prefix(*p->train_source);
}
void BnwNativeUnitAllocatorCore::require_initial_visibility_hidden_slot_current(
 const InitialVisibilityBinding&binding,const graphics::BnwPendingUnitHiddenSlotProof&proof)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 require_initial_visibility_hidden_slot_identity_without_callbacks(binding,proof);
 p->check();require_initial_placement_current(binding.pending.get());binding.pending->require_current();
 require_initial_visibility_hidden_slot_identity_without_callbacks(binding,proof);
}
std::shared_ptr<const BnwUnitMovePending> BnwNativeUnitAllocatorCore::advance_initial_city_visibility_prefix(
 const std::shared_ptr<graphics::BnwPendingUnitVisualOwner>&supplied)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 need(bool(supplied),"Initial city visibility genuine renderer absent");
 const auto receiver=supplied->retain_factory_owner(); // Actual graphics callback-free canonical getter.
 need(receiver&&receiver.get()==supplied.get()&&!receiver.owner_before(supplied)&&!supplied.owner_before(receiver),
  "Initial city visibility rejects alias renderer control");
 Impl::Guard guard(p);p->check();
 need(p->initial_set_xy&&!p->initial_hidden_proof,"Initial city visibility wrong actual source/once phase");
 const auto pending=p->initial_set_xy;const auto binding=capture_initial_visibility_binding(*pending);
 receiver->require_current();require_initial_visibility_binding_current(binding);
 try{
  const auto proof=receiver->apply_initial_visibility_false(*self,*pending); // Real receiver, NEVER a queued ACK.
  require_initial_visibility_binding_current(binding);
  need(proof&&proof->slot_&&!proof->owner_.expired()&&!proof->owner_.owner_before(receiver)&&!receiver.owner_before(proof->owner_)&&
   !proof->node_.owner_before(binding.node_control)&&!binding.node_control.owner_before(proof->node_)&&
   !proof->core_.owner_before(self)&&!self.owner_before(proof->core_)&&
   !proof->pending_.owner_before(pending)&&!pending.owner_before(proof->pending_),
   "Initial visibility did not return SAME actual hidden renderer slot");
  // Shared/weak/optional copies below cannot allocate or run providers. Record
  // ONLY this actual normal return. Policy garrison/fortify/sight remain next.
  p->initial_hidden_binding=binding;p->initial_hidden_proof=proof;p->initial_hidden_receiver=receiver;
  pending->record_initial_visibility_return();
  return pending;
 }catch(...){p->bank->retired=true;throw;} // Renderer mutation is not rolled back/replayed.
}

std::int32_t BnwNativeUnitAllocatorCore::read_native_garrison_policy_culture()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 const auto value=prepare_garrison_policies(*p->host,*p->state,p->actor);
 p->check();compare_garrison_policies(*p->state,p->actor,value.seal);return value.value;
}
void BnwNativeUnitAllocatorCore::require_initial_garrison_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 // Source-only pure acceptance INSIDE the existing Core Guard, after stores.
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_garrison&&p->initial_garrison->stored&&
  p->initial_hidden_binding&&p->initial_hidden_proof&&p->train_source.get()==&source,
  "Garrison source prefix not genuinely stored");
 const auto&g=*p->initial_garrison;
 need(!source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),
  "Garrison canonical TRAIN source differs");
 require_initial_visibility_hidden_slot_identity_without_callbacks(*p->initial_hidden_binding,*p->initial_hidden_proof);
 need(g.modifier!=0&&g.before_revision<UINT64_MAX&&g.after_revision==g.before_revision+1&&
  initial_best_land_prior_world_revision_without_callbacks()==g.after_revision,"Garrison actual mutation revision differs");
 const auto&rows=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).cities.cities;
 const saves::SavedCityObservation*actual=nullptr;for(const auto&city:rows)if(&city==g.city)actual=&city;
 need(actual,"Garrison actual city storage replaced");
 const auto&f=garrison_named(actual->core.fields,"m_iJONSCulturePerTurnFromPolicies");
 need(&f==g.field&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&f.values[0]==g.after&&
  garrison_named(actual->core.fields,"m_iID").values.at(0)==p->initial_hidden_binding->city,
  "Garrison actual stored city operand differs");
 compare_garrison_policies(*p->state,p->actor,g.policies);
}
std::shared_ptr<const BnwUnitMovePending> BnwNativeUnitAllocatorCore::advance_initial_city_garrison_fortify_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(p->initial_set_xy&&p->initial_hidden_binding&&p->initial_hidden_proof&&!p->initial_garrison,
  "Initial garrison requires genuine normal renderer return, once only");
 const auto pending=p->initial_set_xy;const auto binding=*p->initial_hidden_binding;const auto proof=p->initial_hidden_proof;
 require_initial_visibility_hidden_slot_current(binding,*proof);
 need(pending->prefix().boundary==BnwUnitMoveBoundary::initial_city_garrison_policy_owner,
  "Initial garrison exact native predecessor changed");
 auto&b=*p->bank;const auto unit=b.pending_units.at(b.pending_slot);
 const auto policies=prepare_garrison_policies(*p->host,*p->state,p->actor);
 std::optional<std::int32_t> domain;
 const auto report=p->host->database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{std::int64_t(b.train->unit)}},[&](const auto&row){
  const auto type=row.text("Type"),domain_type=row.text("Domain");
  const auto*entry=type?p->host->types().find(*type):nullptr;const auto*d=domain_type?p->host->types().find(*domain_type):nullptr;
  need(!domain&&entry&&entry->table=="Units"&&entry->id==b.train->unit&&d&&d->table=="Domains"&&d->id>=0&&d->id<5,
   "Initial garrison actual UnitInfo domain missing");domain=narrow(d->id);
 });need(report.all_succeeded()&&domain.has_value(),"Initial garrison selected UnitInfo unavailable");
 const auto*land=p->host->types().find("DOMAIN_LAND");need(land&&land->table=="Domains"&&land->id==2,"Initial native DOMAIN_LAND binding differs");
 const auto*max=p->host->defines().find("MAX_FORTIFY_TURNS");
 need(p->host->defines().profile()==content::GlobalDefineProfile::bnw&&max&&max->lookup.found&&
  max->lookup.sqlite_code==0&&max->lookup.error.empty()&&!max->retained_previous&&
  std::holds_alternative<std::int64_t>(max->lookup.value),"Initial fortify clamp define missing");
 const auto maximum=narrow(std::get<std::int64_t>(max->lookup.value));need(maximum>=0,"Initial fortify clamp domain invalid");
 const auto before_fortify=unit->scalars.at("m_iFortifyTurns");
 need(before_fortify==0,"Initial fortify nonzero requires real GameplayUnitFortify receiver");
 // range(0,0,MAX_FORTIFY_TURNS)==current0: SDK does not write or call a receiver.
 const auto*observed=resolve_bnw_city_registry(p->state->world,p->state->cities,p->actor,b.train->city);
 saves::SavedCityObservation*city=nullptr;for(auto&row:p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).cities.cities)
  if(&row==observed)city=&row;
 need(city&&city->core.layout==bnw,"Initial garrison SAME live city unavailable");
 const auto&live=p->state->cities.players.at(p->actor).cities.at(b.train->city);
 need(live.owner==p->actor&&live.id==b.train->city&&live.x==binding.x&&live.y==binding.y&&
  garrison_named(city->core.fields,"m_iID").values.at(0)==binding.city&&
  garrison_named(city->core.fields,"m_iX").values.at(0)==binding.x&&garrison_named(city->core.fields,"m_iY").values.at(0)==binding.y,
  "Initial garrison paired current city identity differs");
 auto*field=&garrison_named_mutable(city->core.fields,"m_iJONSCulturePerTurnFromPolicies");
 need(field->encoding==saves::CityCoreEncoding::i32&&field->values.size()==1,"Initial garrison native city scalar shape differs");
 const auto before=narrow(field->values[0]);
 const auto modifier=unit->scalars.at("m_iBaseCombat")>0&&*domain==land->id?policies.value:0;
 const auto after=narrow(static_cast<std::int64_t>(before)+modifier);const auto revision=initial_best_land_prior_world_revision_without_callbacks();
 need(modifier==0||revision<UINT64_MAX,"Initial garrison mutation revision exhausted");
 InitialGarrisonPrefix staged{city,field,before,after,modifier,revision,revision+(modifier!=0?1u:0u),
  city->core.start_offset,city->core.end_offset,policies.seal,false};
 // Allocate journal before ALL final source callbacks/semantic comparisons.
 p->initial_garrison=std::move(staged);
 try{
  require_initial_visibility_hidden_slot_current(binding,*proof);p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  require_initial_visibility_hidden_slot_identity_without_callbacks(binding,*proof);compare_garrison_policies(*p->state,p->actor,policies.seal);
  // Only integer lookups/traversal below. Verify current city storage BEFORE dereference.
  const saves::SavedCityObservation*current=nullptr;for(const auto&row:p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).cities.cities)
   if(&row==city)current=&row;
  need(current&&current->core.layout==bnw&&current->core.start_offset==p->initial_garrison->core_start&&
   current->core.end_offset==p->initial_garrison->core_end&&&garrison_named(current->core.fields,"m_iJONSCulturePerTurnFromPolicies")==field&&
   field->encoding==saves::CityCoreEncoding::i32&&field->values.size()==1&&
   field->values[0]==before&&unit->scalars.at("m_iFortifyTurns")==before_fortify&&
   initial_best_land_prior_world_revision_without_callbacks()==revision&&unit->scalars.at("m_iBaseCombat")==binding.base_combat,
   "Initial garrison operands changed before stores");
  const auto&current_live=p->state->cities.players.at(p->actor).cities.at(binding.city);
  need(current_live.owner==p->actor&&current_live.id==binding.city&&current_live.x==binding.x&&current_live.y==binding.y&&
   current_live.source_core==std::optional{std::pair{current->core.start_offset,current->core.end_offset}}&&
   garrison_named(current->core.fields,"m_iID").values.at(0)==binding.city&&
   garrison_named(current->core.fields,"m_eOwner").values.at(0)==binding.owner&&
   garrison_named(current->core.fields,"m_iX").values.at(0)==binding.x&&garrison_named(current->core.fields,"m_iY").values.at(0)==binding.y,
   "Initial garrison city changed during preparation");
  require_initial_visibility_hidden_slot_identity_without_callbacks(binding,*proof);
  // Genuine source/node/slot proof already pins all owners. Only no-throw stores.
  if(modifier!=0){field->values[0]=after;p->state->ui_source_revision=revision+1;}
  p->initial_garrison->stored=true;
  pending->record_initial_garrison_fortify_return(modifier,revision,initial_best_land_prior_world_revision_without_callbacks());
  if(modifier!=0)p->train_source->accept_actual_initial_garrison_prefix(*self);
  return pending; // Actual default-layer new sight is mandatory NEXT.
 }catch(...){b.retired=true;throw;}
}


BnwNativeUnitAllocatorCore::InitialSightOperands BnwNativeUnitAllocatorCore::initial_sight_operands()const{
 const auto self=retain_factory_owner();const auto p=impl_;
 need(p->initial_hidden_binding&&p->initial_hidden_proof&&p->initial_garrison&&p->initial_garrison->stored&&!p->initial_sight_stored,
  "Initial sight lacks real hidden/garrison/fortify predecessors");
 require_initial_visibility_hidden_slot_current(*p->initial_hidden_binding,*p->initial_hidden_proof);
 const auto unit=p->bank->pending_units.at(p->bank->pending_slot);
 need(unit->scalars.at("m_iMapLayer")==0&&unit->scalars.at("m_bEmbarked")==0&&unit->scalars.at("m_eSeeInvisibleType")==-1,
  "Initial sight reached unsupported layer/embark/SeeInvisible native state");
 // getFacingDirection(true) ALWAYS NO_DIRECTION (CvUnit.cpp16108), not saved facing.
 const auto team=detail::bnw_player_team_read(*p->state,p->actor);need(team>=0&&team<64,"Initial sight native team invalid");
 InitialSightOperands result{*p->initial_hidden_binding,unit->scalars.at("m_eUnitType"),team,
  unit->scalars.at("m_iExtraVisibilityRange"),unit->scalars.at("m_eSeeInvisibleType")};
 require_initial_sight_operands_without_callbacks(result);return result;
}
void BnwNativeUnitAllocatorCore::require_initial_sight_operands_without_callbacks(const InitialSightOperands&args)const{
 // This is the common source/node subgate, not the enclosing proof getter:
 // the latter additionally verifies this owner's stored map seal (no recursion).
 require_initial_visibility_binding_controls_without_callbacks(args.visual);
 impl_->initial_set_xy->require_initial_visibility_hidden_slot_phase_without_callbacks();
 const auto p=impl_;const auto unit=p->bank->pending_units.at(p->bank->pending_slot);
 need(p->initial_garrison&&p->initial_garrison->stored&&p->bank->train&&
  unit->scalars.at("m_eUnitType")==args.type&&args.type==p->bank->train->unit&&
  unit->scalars.at("m_iMapLayer")==0&&unit->scalars.at("m_bEmbarked")==0&&
  unit->scalars.at("m_eSeeInvisibleType")==args.see_invisible&&args.see_invisible==-1&&
  unit->scalars.at("m_iExtraVisibilityRange")==args.extra_visibility,"Initial sight actual unit operands changed");
 need(p->state->world.pregame&&p->state->world.pregame->layout==bnw,"Initial sight native Pregame absent");
 const std::vector<std::int64_t>*teams=nullptr;
 for(const auto&entry:p->state->world.pregame->archive.integer_arrays)if(std::string_view(entry.first)=="team_types")teams=&entry.second;
 need(teams&&teams->size()==64&&teams->at(static_cast<std::size_t>(p->actor))==args.team&&
  p->state->setup.players.at(p->actor).team==args.team,"Initial sight native raw/owned team changed");
}
void BnwNativeUnitAllocatorCore::require_initial_sight_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_sight&&p->initial_sight_stored&&
  p->train_source.get()==&source&&!source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),
  "Initial sight actual stored Source/control missing");
 require_initial_visibility_hidden_slot_identity_without_callbacks(*p->initial_hidden_binding,*p->initial_hidden_proof);
 need(p->initial_sight_before<UINT64_MAX&&p->initial_sight_after==p->initial_sight_before+1&&
  initial_best_land_prior_world_revision_without_callbacks()==p->initial_sight_after,"Initial sight actual revision differs");
 p->initial_sight->require_after_without_callbacks(*this);
}
std::shared_ptr<const BnwUnitMovePending> BnwNativeUnitAllocatorCore::advance_initial_no_receiver_sight_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(p->initial_hidden_binding&&p->initial_hidden_proof&&p->initial_garrison&&p->initial_garrison->stored&&
  !p->initial_sight&&!p->initial_sight_stored,"Initial sight actual once predecessor missing");
 need(p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_new_sight_owner,
  "Initial sight native phase differs");
 // prepare authenticates SAME Core/node and reuses existing scan/load_rules.
 auto sight=BnwInitialUnitSightOwner::prepare(*self);
 const auto revision=initial_best_land_prior_world_revision_without_callbacks();need(revision<UINT64_MAX,"Initial sight revision exhausted");
 p->initial_sight=std::move(sight);p->initial_sight_before=revision;p->initial_sight_after=revision+1;
 try{
  p->check();require_initial_visibility_hidden_slot_current(*p->initial_hidden_binding,*p->initial_hidden_proof);
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  p->initial_sight->require_before_without_callbacks(*self);
  require_initial_visibility_hidden_slot_identity_without_callbacks(*p->initial_hidden_binding,*p->initial_hidden_proof);
  need(initial_best_land_prior_world_revision_without_callbacks()==revision,"Initial sight revision changed before counter stores");
  p->initial_sight->apply_without_callbacks(); // SDK ring order, real raw/cache+invisible stores.
  p->state->ui_source_revision=revision+1;p->initial_sight_stored=true;
  p->initial_set_xy->record_initial_sight_return(revision+1);
  p->train_source->accept_actual_initial_sight_prefix(*self);
  return p->initial_set_xy; // Actual addUnit list/area/publication are NEXT.
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_post_city_checks_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_post_city&&p->initial_post_city->stored,
  "Initial post-city native checks receipt absent");
 require_initial_city_strength_prefix(*p->train_source);
 const auto&binding=*p->initial_hidden_binding;const auto&node=*p->bank->pending_units.at(p->bank->pending_slot);
 const auto&q=*p->initial_post_city;
 need(q.revision==initial_happiness_prior_revision_without_callbacks(),"Initial post-city actual revision changed");
 if(p->initial_name_game&&p->initial_name_game->stored){
  const auto&seal=p->initial_army_ai&&p->initial_army_ai->stored?q.army_ai_seal:(p->initial_unit_cycler&&p->initial_unit_cycler->stored?q.cycle_order_seal:(p->initial_set_moves&&p->initial_set_moves->stored&&p->initial_set_moves->changed?q.set_moves_seal:(p->initial_immobile_moves&&p->initial_immobile_moves->stored?q.immobile_moves_seal:(p->initial_exotic_xp&&p->initial_exotic_xp->stored?q.exotic_xp_seal:(p->initial_embark_promotions&&p->initial_embark_promotions->stored?q.embark_promotion_seal:(p->initial_player_promotions&&p->initial_player_promotions->stored?q.player_promotion_seal:(p->initial_traits_promotions&&p->initial_traits_promotions->stored?q.traits_promotion_seal:(p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored?q.promotion_loop_seal:(p->initial_unit_info_promotion&&p->initial_unit_info_promotion->stored?q.promotion_successor_seal:q.name_successor_seal)))))))));
  std::size_t at=0;initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,node.scalars,node.indexed,[&](auto n){need(at<seal.size()&&seal[at++]==static_cast<std::uint64_t>(n),"Post-city exact Init name descendant changed");});
  need(at==seal.size(),"Post-city Init descendant extent changed");
 }else initial_post_city_compare(*p->state,p->actor,binding.x,binding.y,node.scalars,node.indexed,q);
 const auto result=initial_post_city_prove(*p->state,*p->host,p->actor,binding.city,binding.x,binding.y,node.scalars,node.indexed,q);
 need(result.first==q.meet_returns&&result.second==q.adjacent_plots,"Initial post-city actual native branch receipts changed");
 p->initial_set_xy->require_initial_set_position_phase_without_callbacks();
 require_initial_visibility_binding_controls_without_callbacks(binding);
 need(!p->bank->retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&
  p->host->session().state_generation()==p->generation,"Initial post-city final actual source bookend changed");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_post_city_checks_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(p->initial_set_xy&&p->initial_city_strength&&p->initial_city_strength->stored&&!p->initial_post_city&&
  p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_post_city_setxy_owner,
  "Initial post-city requires actual city-strength/once predecessor");
 require_initial_visibility_hidden_slot_current(*p->initial_hidden_binding,*p->initial_hidden_proof);
 p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
 need(p->host->session().has_bnw_operation_checkpoint_history(),"Initial post-city requires actual Session quarantine");
 const auto&binding=*p->initial_hidden_binding;
 try{
  auto&node=*p->bank->pending_units.at(p->bank->pending_slot);
  // Prepare semantic storage before any rules/human callback. Source identity
  // cannot be inferred from generation alone around these native DB reads.
  std::size_t count=0;initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,node.scalars,node.indexed,
   [&](auto){need(count<SIZE_MAX,"Initial post-city semantic extent overflow");++count;});
  std::vector<std::uint64_t>seal(count);std::size_t at=0;
  require_initial_city_strength_prefix(*p->train_source);
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,node.scalars,node.indexed,
   [&](auto value){need(at<seal.size(),"Initial post-city source extent changed during preparation");seal[at++]=static_cast<std::uint64_t>(value);});
  need(at==seal.size(),"Initial post-city source extent changed during preparation");
  auto q=initial_post_city_rules(*p->host,*p->state,p->actor,node.scalars.at("m_eUnitType"));
  q.seal.swap(seal);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  // Validate the existing full native visibility source once while allocations
  // are allowed; final target-specific visitor is allocation-free.
  (void)bnw_live_plot_visibility_cache(*p->state);
  p->check();require_initial_visibility_hidden_slot_current(binding,*p->initial_hidden_proof);
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  require_initial_city_strength_prefix(*p->train_source);
  initial_post_city_compare(*p->state,p->actor,binding.x,binding.y,node.scalars,node.indexed,q);
  const auto result=initial_post_city_prove(*p->state,*p->host,p->actor,binding.city,binding.x,binding.y,node.scalars,node.indexed,q);
  q.meet_returns=result.first;q.adjacent_plots=result.second;
  p->initial_post_city.emplace(std::move(q)); // All record storage precedes final bookends.
  require_initial_city_strength_prefix(*p->train_source);
  initial_post_city_compare(*p->state,p->actor,binding.x,binding.y,node.scalars,node.indexed,*p->initial_post_city);
  const auto final=initial_post_city_prove(*p->state,*p->host,p->actor,binding.city,binding.x,binding.y,node.scalars,node.indexed,*p->initial_post_city);
  need(final==result&&!p->bank->retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==p->initial_post_city->revision,
   "Initial post-city final source/branch bookend changed");
  p->initial_post_city->stored=true;
  p->initial_set_xy->record_initial_post_city_checks_return(result.first,result.second);
  return p->initial_set_xy; // Actual initialized-graphics SetPosition receiver NEXT.
 }catch(...){p->bank->retired=true;throw;}
}

BnwNativeUnitAllocatorCore::InitialVisibilityBinding BnwNativeUnitAllocatorCore::capture_initial_set_position_binding(
 const BnwUnitMovePending&pending,const graphics::BnwPendingUnitHiddenSlotProof&hidden)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 need(p->initial_set_xy.get()==&pending&&p->initial_hidden_binding&&p->initial_hidden_proof.get()==&hidden&&!p->initial_position,
  "Initial SetPosition requires genuine SAME pending/hidden predecessor once");
 require_initial_post_city_checks_without_callbacks();pending.require_initial_set_position_phase_without_callbacks();
 const auto binding=*p->initial_hidden_binding;
 require_initial_set_position_binding_current(binding,hidden);return binding;
}
void BnwNativeUnitAllocatorCore::require_initial_set_position_binding_identity_without_callbacks(
 const InitialVisibilityBinding&binding,const graphics::BnwPendingUnitHiddenSlotProof&hidden)const{
 require_initial_visibility_hidden_slot_identity_without_callbacks(binding,hidden);
 require_initial_post_city_checks_without_callbacks();const auto p=impl_;
 need(!p->initial_position,"Initial SetPosition receiver is not the genuine first attempt");
 p->initial_set_xy->require_initial_set_position_phase_without_callbacks();
}
void BnwNativeUnitAllocatorCore::require_initial_set_position_binding_current(
 const InitialVisibilityBinding&binding,const graphics::BnwPendingUnitHiddenSlotProof&hidden)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 require_initial_set_position_binding_identity_without_callbacks(binding,hidden);
 p->check();require_initial_visibility_hidden_slot_current(binding,hidden);
 p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
 require_initial_set_position_binding_identity_without_callbacks(binding,hidden);
}
std::int32_t BnwNativeUnitAllocatorCore::initial_set_position_domain(const InitialVisibilityBinding&binding,const graphics::BnwPendingUnitHiddenSlotProof&hidden)const{
 const auto canonical=retain_factory_owner();require_initial_set_position_binding_identity_without_callbacks(binding,hidden);
 const auto p=impl_;const auto&node=*p->bank->pending_units.at(p->bank->pending_slot);
 const auto&info=*p->initial_post_city;
 need(info.type==node.scalars.at("m_eUnitType")&&info.domain>=0&&info.domain<5,
  "Initial SetPosition actual UnitInfo/domain binding changed");
 // SDK getDomainType() uses current plot water only for DOMAIN_HOVER.
 // That dynamic branch has no admitted ordinary initial-post-city receipt.
 need(info.domain!=4,"Initial SetPosition DOMAIN_HOVER requires genuine current plot-domain getter");
 return info.domain;
}
bool BnwNativeUnitAllocatorCore::initial_set_position_is_hovering(const InitialVisibilityBinding&binding,const graphics::BnwPendingUnitHiddenSlotProof&hidden)const{
 const auto canonical=retain_factory_owner();require_initial_set_position_binding_identity_without_callbacks(binding,hidden);
 const auto&node=*impl_->bank->pending_units.at(impl_->bank->pending_slot);
 return node.scalars.at("m_iHoveringUnitCount")>0; // CvUnit::IsHoveringUnit, actual Reset/promotion storage.
}
bool BnwNativeUnitAllocatorCore::initial_set_position_can_move_impassable(const InitialVisibilityBinding&binding,const graphics::BnwPendingUnitHiddenSlotProof&hidden)const{
 const auto canonical=retain_factory_owner();require_initial_set_position_binding_identity_without_callbacks(binding,hidden);
 const auto&node=*impl_->bank->pending_units.at(impl_->bank->pending_slot);
 return node.scalars.at("m_iCanMoveImpassableCount")>0; // CvUnit::canMoveImpassable, no guessed UnitInfo flag.
}
void BnwNativeUnitAllocatorCore::require_initial_set_position_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_position&&p->initial_hidden_binding&&p->initial_hidden_proof,
  "Initial position lacks actual private renderer receipt");
 require_initial_visibility_hidden_slot_identity_without_callbacks(*p->initial_hidden_binding,*p->initial_hidden_proof);
 require_initial_post_city_checks_without_callbacks();const auto&proof=*p->initial_position;const auto&binding=*p->initial_hidden_binding;
 need(!proof.owner_.expired()&&!p->initial_position_receiver.expired()&&
  !proof.owner_.owner_before(p->initial_position_receiver)&&!p->initial_position_receiver.owner_before(proof.owner_)&&
  !proof.owner_.owner_before(p->initial_hidden_receiver)&&!p->initial_hidden_receiver.owner_before(proof.owner_)&&
  proof.slot_==p->initial_hidden_proof->slot_&&!proof.node_.expired()&&!proof.node_.owner_before(binding.node_control)&&
  !binding.node_control.owner_before(proof.node_)&&!proof.core_.expired()&&!proof.core_.owner_before(factory_self_)&&
  !factory_self_.owner_before(proof.core_)&&!proof.pending_.expired()&&!proof.pending_.owner_before(p->initial_set_xy)&&
  !p->initial_set_xy.owner_before(proof.pending_)&&!proof.hidden_.expired()&&!proof.hidden_.owner_before(p->initial_hidden_proof)&&
  !p->initial_hidden_proof.owner_before(proof.hidden_)&&!proof.world_.expired()&&proof.layout_&&proof.height_&&
  proof.map_x_==binding.x&&proof.map_y_==binding.y&&proof.revision_==initial_happiness_prior_revision_without_callbacks(),
  "Initial position is not SAME canonical hidden slot/world/node/source return");
 for(const auto value:proof.world_position_)need(std::isfinite(value),"Initial renderer world position is not finite");
 p->initial_set_xy->require_initial_position_returned_without_callbacks();
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_set_position_prefix(
 const std::shared_ptr<graphics::BnwPendingUnitVisualOwner>&supplied)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 need(bool(supplied),"Initial SetPosition genuine renderer owner absent");
 const auto receiver=supplied->retain_factory_owner(); // Creator-first, no callback.
 need(receiver&&receiver.get()==supplied.get()&&!receiver.owner_before(supplied)&&!supplied.owner_before(receiver),
  "Initial SetPosition rejects alias/no-op renderer control");
 Impl::Guard guard(p);
 try{
  need(p->initial_set_xy&&p->initial_hidden_binding&&p->initial_hidden_proof&&!p->initial_position,
   "Initial SetPosition requires actual post-city/hidden predecessor once");
  const auto binding=capture_initial_set_position_binding(*p->initial_set_xy,*p->initial_hidden_proof);
  need(!p->initial_hidden_receiver.owner_before(receiver)&&!receiver.owner_before(p->initial_hidden_receiver),
   "Initial SetPosition renderer differs from genuine visibility receiver");
  receiver->require_current();require_initial_set_position_binding_current(binding,*p->initial_hidden_proof);
  const auto proof=receiver->apply_initial_set_position(*self,*p->initial_set_xy,*p->initial_hidden_proof);
  need(bool(proof),"Initial SetPosition actual renderer returned no issuer proof");
  // These are genuine producer-owned validations of the stored current slot,
  // MaterialWorld/layout/height/XYZ and actual paint lifetime. A proof's
  // non-null pointers alone never authorize the continuation.
  receiver->require_initial_set_position_current(binding,*p->initial_hidden_proof,*proof);
  p->check();require_initial_set_position_binding_current(binding,*p->initial_hidden_proof);
  receiver->require_initial_set_position_identity_without_callbacks(binding,*p->initial_hidden_proof,*proof);
  require_initial_set_position_binding_identity_without_callbacks(binding,*p->initial_hidden_proof);
  p->initial_position=proof;p->initial_position_receiver=receiver; // shared/weak copies do not allocate/callback.
  p->initial_set_xy->record_initial_set_position_return();
  return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_unit_info_predecessors_without_callbacks()const{
 require_initial_set_position_without_callbacks();const auto p=impl_;const auto&binding=*p->initial_hidden_binding;
 const auto&raw=p->state->world.map_plots->plots.at(static_cast<std::size_t>(p->state->world.map_header.grid.index(
  static_cast<std::uint64_t>(binding.x),static_cast<std::uint64_t>(binding.y))));
 const auto&cell=p->state->world.map_projection->cells.at(static_cast<std::size_t>(raw.index));
 // Native bNoMove is FALSE. The genuine NULL-improvement branch answers both
 // isGoody and camp equality without a missing ImprovementInfo fallback.
 need(post_named(raw.fields,"improvement_type_hash").value==0&&cell.improvement_type_hash==std::optional<std::uint64_t>{0}&&
  (!cell.improvement_type||*cell.improvement_type==-1),"Mandatory actual goody/camp effects required before UnitInfo");
 need(post_named(raw.fields,"owner").value==p->actor&&cell.owner==std::optional<std::int64_t>{p->actor},
  "Initial SetXY friendly-territory/native enemy notification branch requires actual own plot");
 const auto team=post_player_team(*p->state,p->actor);
 need(team>=0&&team<64&&post_named(post_team(*p->state,team).fields,"can_embark").value<=0,
  "Mandatory actual friendly embark promotion effects required before UnitInfo");
 need(!post_relation(*p->state,team,team,"at_war"),"Mandatory actual enemy territory notification owner required");
 // Original old plot is genuinely NULL, capture list was proved empty by
 // collision and no transport/cargo branch was skipped. Client selection is
 // checked by its genuine queue owner immediately before its UnitInfo store.
 require_initial_set_position_without_callbacks();
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_unit_info_dirty_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 // Pure missing/once admission BEFORE effect quarantine; actual currentness,
 // earned receipt validation, callbacks and stores below remain sticky.
 need(p->train_source&&p->initial_set_xy&&p->initial_hidden_binding&&p->initial_hidden_proof&&p->initial_city_strength&&p->initial_city_strength->stored&&p->initial_post_city&&p->initial_post_city->stored&&p->initial_position,
  "Initial unit stage lacks genuine predecessor or was already attempted");
 try{
  p->check();require_initial_unit_info_predecessors_without_callbacks();
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  p->initial_set_xy->execute_initial_unit_info_dirty_prefix();
  require_initial_set_position_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_enemy_city_range_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 // Pure missing/once admission BEFORE effect quarantine; actual currentness,
 // earned receipt validation, callbacks and stores below remain sticky.
 need(p->train_source&&p->initial_set_xy&&p->initial_hidden_binding&&p->initial_hidden_proof&&p->initial_city_strength&&p->initial_city_strength->stored&&p->initial_post_city&&p->initial_post_city->stored&&p->initial_position,
  "Initial unit stage lacks genuine predecessor or was already attempted");
 try{
  p->check();require_initial_set_position_without_callbacks();
  need(p->initial_set_xy->prefix().initial_info_bar_dirty_stored&&
   p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_enemy_city_range_owner,
   "Initial enemy-city scan requires actual UnitInfo dirty setter once");
  const auto*define=p->host->defines().find("CITY_ATTACK_RANGE");
  need(p->host->defines().profile()==content::GlobalDefineProfile::bnw&&define&&define->lookup.found&&define->lookup.sqlite_code==0&&define->lookup.error.empty()&&!define->retained_previous&&
   std::holds_alternative<std::int64_t>(define->lookup.value),
   "Initial enemy-city actual native CITY_ATTACK_RANGE unavailable");
  const auto radius=narrow(std::get<std::int64_t>(define->lookup.value));const auto&binding=*p->initial_hidden_binding;
  const auto&world=p->state->world;MapGeometry geometry{world.map_header.grid,world.map_header.wrap_x,world.map_header.wrap_y,bnw};
  geometry.validate();need(world.map_header.grid.width<=INT32_MAX&&world.map_header.grid.height<=INT32_MAX&&radius>=0&&radius<=64&&static_cast<std::uint64_t>(radius)*2<world.map_header.grid.width&&
   static_cast<std::uint64_t>(radius)*2<world.map_header.grid.height,"Initial enemy-city scan native range/wrapped extent unsupported");
  const auto team=post_player_team(*p->state,p->actor);need(team>=0&&team<64,"Initial enemy-city actual actor team unavailable");
  std::uint64_t visited=0;
  // SDK plotXYWithRangeCheck/hex distance, DX outer then DY inner. Source
  // city/team pointers are read in the actual current SAME map, not a saved
  // unrelated city list. All reached war predicates finish before credit.
  for(std::int64_t dx=-radius;dx<=radius;++dx)for(std::int64_t dy=-radius;dy<=radius;++dy){
   const auto plot=geometry.plot(std::int64_t(binding.x)+dx,std::int64_t(binding.y)+dy);
   if(!plot){continue;}
   const auto&raw=world.map_plots->plots.at(static_cast<std::size_t>(*plot));
   const auto&cell=world.map_projection->cells.at(static_cast<std::size_t>(*plot));
   if(geometry.distance(binding.x,binding.y,static_cast<std::int64_t>(raw.x),static_cast<std::int64_t>(raw.y))>radius){continue;}
   ++visited;
   need(raw.index==static_cast<std::uint64_t>(*plot)&&cell.index==raw.index&&cell.x==raw.x&&cell.y==raw.y,
    "Initial enemy-city actual plot identity changed");
   const auto owner=narrow(post_named(raw.fields,"plot_city_owner").value),city=narrow(post_named(raw.fields,"plot_city_id").value);
   need(cell.is_city==(owner!=-1)&&cell.plot_city.has_value()==(owner!=-1),"Initial enemy-city raw/projected city presence differs");
   if(owner==-1){need(city==-1,"Initial enemy-city NULL pointer has nonnull city ID");continue;}
   need(owner>=0&&owner<64&&city>=0&&cell.plot_city->owner==owner&&cell.plot_city->id==city&&
    post_named(raw.fields,"owner").value==owner&&cell.owner==std::optional<std::int64_t>{owner},
    "Initial enemy-city actual city/plot owner differs");
   const auto*actual=resolve_bnw_city_registry(world,p->state->cities,owner,city);
   need(actual&&garrison_named(actual->core.fields,"m_iX").values.at(0)==static_cast<std::int64_t>(raw.x)&&
    garrison_named(actual->core.fields,"m_iY").values.at(0)==static_cast<std::int64_t>(raw.y),"Initial enemy-city actual city pointer/coordinates differ");
   const auto other=post_player_team(*p->state,owner);
   if(other!=-1)need(!post_relation(*p->state,team,other,"at_war"),
    "Mandatory genuine enemy-in-range SpecificCityInfo receiver required before happiness");
  }
  // Direct source/phase proof after the no-allocation native traversal. No
  // external callback is skipped when a real enemy exists: that path refuses.
  require_initial_set_position_without_callbacks();
  need(!p->bank->retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation,"Initial enemy-city source retired before return");
  p->initial_set_xy->record_initial_enemy_city_range_return(visited);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_happiness_prior_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign initial happiness lineage before live borrow");
 if(!p->initial_happiness||!p->initial_happiness->stored)return initial_best_land_prior_world_revision_without_callbacks();
 const auto&q=*p->initial_happiness;
 need(p->bank&&!p->bank->retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&
  p->host->session().state_generation()==p->generation&&q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1&&
  initial_setxy_earned_world_revision_without_callbacks()==(q.excess_stored?q.full_after_revision:(q.suffix_stored?q.suffix_after_revision:q.after_revision))&&
  (!q.excess_stored||(q.full_before_revision==q.suffix_after_revision&&q.full_after_revision==q.full_before_revision+1))&&
  (!q.suffix_stored||(q.suffix_before_revision==q.after_revision&&q.suffix_after_revision==q.suffix_before_revision+1))&&p->initial_city_strength&&p->initial_city_strength->stored&&
  p->initial_city_strength->after_revision==q.before_revision&&p->initial_city_strength->happiness_prefix_stored&&
  p->initial_city_strength->happiness_revision==(q.excess_stored?q.full_after_revision:(q.suffix_stored?q.suffix_after_revision:q.after_revision))&&
  p->initial_city_strength->happiness_value==(q.suffix_stored?q.suffix_after:q.after)&&
  p->host->session_->bnw_unhappiness_current()==(q.excess_stored?q.cache_current_restored:false)&&p->state->unhappiness.players.at(p->actor).happiness==(q.suffix_stored?q.suffix_after:q.after)&&
  p->state->cities.players.at(p->actor).excess_happiness==(q.excess_stored?std::optional<std::int64_t>{q.excess}:std::nullopt),"Initial happiness real paired descendant changed");
 const auto&player=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
 const auto&field=garrison_named(player.core.fields,"m_iHappiness");
 need(&field==q.field&&field.values.size()==1&&field.values[0]==(q.suffix_stored?q.suffix_after:q.after),"Initial happiness actual raw scalar storage changed");
 if(q.suffix_stored){const auto&connection=garrison_named(player.core.fields,"m_iCityConnectionHappiness");
  need(&connection==q.connection_field&&connection.values.size()==1&&connection.values[0]==q.connection_after,
   "Initial happiness actual stored connection cache changed");}
 p->initial_set_xy->require_initial_happiness_prefix_without_callbacks();return q.before_revision;
}
void BnwNativeUnitAllocatorCore::require_initial_happiness_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto canonical=retain_factory_owner();const auto p=impl_;
 need(p->initial_happiness&&p->initial_happiness->stored&&p->train_source.get()==&source&&
  !source.factory_self_.expired()&&!source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),
  "Initial happiness missing actual SAME source prefix");
 (void)initial_happiness_prior_revision_without_callbacks();require_initial_set_position_without_callbacks();
 BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_happiness_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 // Pure missing/once admission BEFORE effect quarantine; actual currentness,
 // earned receipt validation, callbacks and stores below remain sticky.
 need(p->train_source&&p->initial_set_xy&&p->initial_hidden_binding&&p->initial_hidden_proof&&p->initial_city_strength&&p->initial_city_strength->stored&&p->initial_post_city&&p->initial_post_city->stored&&p->initial_position&&!p->initial_happiness,
  "Initial unit stage lacks genuine predecessor or was already attempted");
 try{
  p->check();require_initial_set_position_without_callbacks();
  need(!p->initial_happiness&&p->initial_set_xy->prefix().initial_enemy_city_range_returned&&
   p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_happiness_prefix_owner,
   "Initial happiness requires real completed range and is once-only");
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialHappinessPrefix effect;effect.before_revision=initial_best_land_prior_world_revision_without_callbacks();
  need(effect.before_revision<UINT64_MAX,"Initial happiness source revision exhausted");effect.after_revision=effect.before_revision+1;
  std::size_t count=0;initial_happiness_semantics(*p->state,p->actor,[&](auto){need(count<SIZE_MAX,"Initial happiness seal overflow");++count;});
  effect.inputs.resize(count);std::size_t at=0;
  initial_happiness_semantics(*p->state,p->actor,[&](auto n){need(at<effect.inputs.size(),"Initial happiness extent changed while preparing seal");effect.inputs[at++]=static_cast<std::uint64_t>(n);});
  need(at==count,"Initial happiness extent changed while preparing seal");
  const auto rules=BnwHappinessPrefixRules::load(p->host->database());
  need(rules.rules_sha256()==p->fingerprint,"Initial happiness actual selected rule identity differs");
  const std::array stages{BnwHappinessNativeStore::Stage::handicap,BnwHappinessNativeStore::Stage::resources,
   BnwHappinessNativeStore::Stage::cities,BnwHappinessNativeStore::Stage::buildings,BnwHappinessNativeStore::Stage::policies,BnwHappinessNativeStore::Stage::per_city};
  std::int32_t total=0;
  for(std::size_t i=0;i<stages.size();++i){const auto stage=stages[i];std::int32_t operand=0;
   if(stage==BnwHappinessNativeStore::Stage::cities||stage==BnwHappinessNativeStore::Stage::buildings){
    operand=detail::bnw_happiness_buildings_reached_math_for_initial_unit(*p->state,rules.buildings(),p->host->identity(),p->actor,
     stage==BnwHappinessNativeStore::Stage::cities?detail::BnwBuildingHappinessReachedStage::local_cities:detail::BnwBuildingHappinessReachedStage::global_buildings,*canonical);
   }else operand=detail::bnw_happiness_prefix_reached_stage_math(*p->state,rules,p->host->identity(),p->actor,stage).operand;
   total=i==0?operand:narrow(std::int64_t(total)+operand);effect.stores[i]={stage,operand,total};
  }
  effect.after=total;auto&raw=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
  effect.field=&garrison_named_mutable(raw.core.fields,"m_iHappiness");
  need(effect.field->encoding==saves::PlayerCoreEncoding::i32&&effect.field->values.size()==1&&effect.field->bytes.empty()&&
   !effect.field->wire_count&&effect.field->hashes.empty()&&effect.field->row_counts.empty(),
   "Initial happiness native destination schema differs");
  effect.before=narrow(effect.field->values[0]);need(p->state->unhappiness.players.at(p->actor).happiness==effect.before,
   "Initial happiness raw/owned initial scalar differs");
  auto successor=BnwMinorCityStrengthScratch::prepare_initial_happiness_inputs(*canonical,*p->initial_city_strength,total);
  p->initial_happiness.emplace(std::move(effect));auto&q=*p->initial_happiness;auto&city=*p->initial_city_strength;
  city.happiness_inputs.swap(successor);city.happiness_value=total;city.happiness_revision=q.after_revision;
  // All cache operands, overflow, allocation and rule reads are staged BEFORE
  // first native write. Final visitor is noalloc; no callbacks run between six
  // native stores. Original city/position receipt stays an immutable predecessor.
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  require_initial_set_position_without_callbacks();initial_happiness_match(*p->state,p->actor,q.inputs);
  need(!p->bank->retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.before_revision,
   "Initial happiness source retired before native stores");
  for(const auto&item:q.stores){q.field->values[0]=item.happiness;p->state->unhappiness.players.at(p->actor).happiness=item.happiness;
   p->state->cities.players.at(p->actor).excess_happiness.reset();}
  p->host->session_->bnw_unhappiness_current_=false;p->state->ui_source_revision=q.after_revision;
  city.happiness_prefix_stored=true;q.stored=true;p->initial_set_xy->record_initial_happiness_prefix_return(q.after_revision);
  p->train_source->accept_actual_initial_happiness_prefix(*canonical);
  return p->initial_set_xy; // Genuine religion/founder SAMEVM/suffix remains NEXT, not UnitSetXY permission.
 }catch(...){p->bank->retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::require_initial_happiness_suffix_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 require_initial_happiness_prefix(source);const auto p=impl_;const auto&q=*p->initial_happiness;
 need(q.suffix_attempted&&q.suffix_stored&&q.founder_returned&&q.religion_stored&&q.founder_religion==-1&&
  p->initial_set_xy->prefix().initial_happiness_native_suffix_stored&&
  (p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_happiness_excess_owner||
   (p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_unit_setxy_owner&&q.full_returned)||
   ((p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_popup_release_owner||(p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner&&p->initial_set_xy->prefix().initial_popup_released)||(p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_center_unit_owner&&p->initial_set_xy->prefix().initial_popup_released&&p->initial_set_xy->prefix().initial_garrison_dirty_returned))&&q.unit_setxy_returned)),
  "Initial happiness actual native suffix receipt absent");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_happiness_native_suffix_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 // Pure missing/once admission BEFORE effect quarantine; actual currentness,
 // earned receipt validation, callbacks and stores below remain sticky.
 need(p->train_source&&p->initial_set_xy&&p->initial_position&&p->initial_city_strength&&p->initial_happiness&&p->initial_happiness->stored&&p->initial_happiness->founder_returned&&p->initial_happiness->religion_stored&&!p->initial_happiness->founder_inflight&&!p->initial_happiness->suffix_attempted,
  "Initial unit stage lacks genuine predecessor or was already attempted");
 try{
  p->check();require_initial_happiness_prefix(*p->train_source);auto&q=*p->initial_happiness;
  need(q.founder_returned&&q.religion_stored&&!q.founder_inflight&&q.founder_religion==-1&&!q.suffix_attempted,
   "Initial happiness suffix requires actual once SAMEVM religion return");
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  std::size_t count=0;initial_happiness_suffix_semantics(*p->state,p->actor,[&](auto){need(count<SIZE_MAX,"Initial suffix seal overflow");++count;});
  std::vector<std::uint64_t> seal(count);std::size_t at=0;
  initial_happiness_suffix_semantics(*p->state,p->actor,[&](auto n){need(at<seal.size(),"Initial suffix seal extent changed");seal[at++]=static_cast<std::uint64_t>(n);});
  need(at==count,"Initial suffix seal extent changed");
  const auto rules=BnwHappinessSuffixRules::load(p->host->database());need(rules.identity()==p->fingerprint,"Initial happiness suffix actual rules differ");
  using Read=detail::BnwHappinessPostReligionRead;using Stage=BnwHappinessSuffixStage;
  const auto read=[&](Read stage){return detail::bnw_happiness_post_religion_reached_read(*p->state,rules,p->host->identity(),p->actor,stage);};
  std::array<BnwHappinessSuffixStore,5> stores{};std::int32_t total=q.after;std::size_t n=0;
  const auto add=[&](Stage stage,std::int32_t operand){total=narrow(static_cast<std::int64_t>(total)+operand);stores[n++]={stage,operand,total};};
  add(Stage::wonders,read(Read::wonders).operand);add(Stage::minors,read(Read::minors).operand);add(Stage::leagues,read(Read::leagues).operand);
  const auto connections=read(Read::connections_cache);need(connections.connections.has_value(),"Initial suffix actual connection quote absent");
  auto&raw=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
  auto&field=garrison_named_mutable(raw.core.fields,"m_iCityConnectionHappiness");
  need(field.encoding==saves::PlayerCoreEncoding::i32&&!field.wire_count&&field.values.size()==1,
   "Initial suffix connection cache storage unavailable");
  const auto before=narrow(field.values[0]);stores[n++]={Stage::connections_cache,connections.operand,total};
  // Native GetHappinessFromTradeRoutes reads the just-recomputed cache. Read
  // its exact scalar value from the staged native output, not stale raw cache.
  add(Stage::connections_add,connections.operand);need(n==stores.size(),"Initial suffix SDK store sequence changed");
  auto predicted_city=BnwMinorCityStrengthScratch::prepare_initial_happiness_inputs(*canonical,*p->initial_city_strength,total,connections.operand);
  const auto revision=initial_best_land_prior_world_revision_without_callbacks();need(revision<UINT64_MAX,"Initial suffix revision exhausted");
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  require_initial_happiness_prefix(*p->train_source);initial_happiness_suffix_match(*p->state,p->actor,seal);
  need(&garrison_named(raw.core.fields,"m_iCityConnectionHappiness")==&field&&field.values[0]==before&&
   q.field->values.size()==1&&q.field->values[0]==q.after&&!p->bank->retired.load()&&
   p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==revision,
   "Initial suffix final SAME source/storage bookend changed");
  q.suffix_attempted=true;q.suffix_stores=stores;q.connection_field=&field;q.connection_before=before;
  q.connection_after=connections.operand;q.suffix_after=total;q.suffix_before_revision=revision;q.suffix_after_revision=revision+1;
  // All getters, allocations and checked arithmetic precede this SDK-ordered
  // in-place scalar interval. The Session operation journal stays quarantined.
  for(const auto&store:stores){
   if(store.stage==Stage::connections_cache){field.values[0]=store.operand;}
   else{q.field->values[0]=store.happiness;p->state->unhappiness.players.at(p->actor).happiness=store.happiness;}
  }
  p->state->cities.players.at(p->actor).excess_happiness.reset();p->host->session_->bnw_unhappiness_current_=false;
  p->state->ui_source_revision=q.suffix_after_revision;
  auto&city=*p->initial_city_strength;city.happiness_inputs.swap(predicted_city);city.happiness_value=total;city.happiness_revision=q.suffix_after_revision;
  q.suffix_stored=true;p->initial_set_xy->record_initial_happiness_native_suffix_return(q.suffix_after_revision);
  p->train_source->accept_actual_initial_happiness_suffix_prefix(*canonical);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::require_initial_happiness_return(const ui::SelectedCityTrainCreateSource&source)const{
 require_initial_happiness_suffix_prefix(source);const auto p=impl_;const auto&q=*p->initial_happiness;
 need(q.excess_stored&&q.full_returned&&q.game_data_occurrence>0&&p->initial_set_xy->prefix().initial_happiness_returned&&
  p->initial_set_xy->prefix().initial_game_data_occurrence==q.game_data_occurrence,
  "Initial happiness actual GameData/native return absent");initial_happiness_finish_match(*p->state,p->actor,initial_current_happiness_finish_inputs_without_callbacks());
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_happiness_return()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 // Pure missing/once admission BEFORE effect quarantine; actual currentness,
 // earned receipt validation, callbacks and stores below remain sticky.
 need(p->train_source&&p->initial_set_xy&&p->initial_position&&p->initial_city_strength&&p->initial_happiness&&p->initial_happiness->suffix_stored&&!p->initial_happiness->excess_stored&&!p->initial_happiness->full_returned,
  "Initial unit stage lacks genuine predecessor or was already attempted");
 try{
  p->check();require_initial_happiness_suffix_prefix(*p->train_source);auto&q=*p->initial_happiness;
  need(!q.excess_stored&&!q.full_returned,"Initial happiness actual final getter already attempted");
  std::size_t count=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto){need(count<SIZE_MAX,"Initial final happiness extent overflow");++count;});
  std::vector<std::uint64_t>before(count);std::size_t at=0;
  initial_happiness_finish_semantics(*p->state,p->actor,[&](auto n){need(at<before.size(),"Initial final happiness extent changed");before[at++]=static_cast<std::uint64_t>(n);});need(at==count,"Initial final happiness extent changed");
  const auto rules=BnwHappinessSuffixRules::load(p->host->database());need(rules.identity()==p->fingerprint,"Initial final happiness genuine rules differ");
  // Preserve the native isLocalPlayer short circuit. This actual TRAIN path
  // was issued for the active actor; never compute a nonlocal excess for UI.
  need(detail::bnw_game_active_player_read(*p->state)==p->actor,"Initial TRAIN local happiness actor changed");
  const auto expected_setup=restore_research_context(p->state->world,p->host->database());
  need(expected_setup.game_type==p->state->setup.game_type&&expected_setup.game_handicap==p->state->setup.game_handicap&&expected_setup.pitboss==p->state->setup.pitboss&&expected_setup.players.size()==p->state->setup.players.size(),
   "Initial final happiness native setup/human binding stale");
  for(const auto&[actor,expected]:expected_setup.players){const auto&actual=p->state->setup.players.at(actor);
   need(expected.team==actual.team&&expected.handicap==actual.handicap&&expected.slot_status==actual.slot_status&&expected.slot_claim==actual.slot_claim&&expected.alive==actual.alive,
    "Initial final happiness native human/team roster stale");}
  for(const auto&[actor,unused]:p->state->unhappiness.players){(void)unused;detail::require_bnw_current_unhappiness_inputs(*p->state,rules.unhappiness(),rules.types(),actor);}
  const auto&u=p->state->unhappiness.players.at(p->actor);BnwUnhappinessQuote quote;
  std::optional<BnwCitySpecialistCounts>specialists;
  if(u.half_specialists&&!p->state->cities.players.at(p->actor).cities.empty()){
   // One genuine factory source covers all current actor cities and consumes
   // actual native Reset/Read specialist remap, never a supplied count/zero.
   const auto&cities=p->state->cities.players.at(p->actor).cities;need(!cities.empty(),"Initial half-specialist source has no actual city");
   specialists.emplace(BnwCitySpecialistCounts::from_same_host(p->host,p->actor,cities.begin()->first));
   specialists->require_same_world_database(p->state->world,p->host->database());
   quote=detail::bnw_native_unhappiness_math_with_specialists(p->state->unhappiness,p->state->setup,p->state->cities,rules.unhappiness(),rules.defines(),p->actor,*specialists);
  }else quote=detail::bnw_native_unhappiness_math(p->state->unhappiness,p->state->setup,p->state->cities,rules.unhappiness(),rules.defines(),p->actor);
  const auto excess=narrow(quote.excess_happiness);need(excess==narrow(static_cast<std::int64_t>(q.suffix_after)-quote.total),"Initial native excess arithmetic differs");
  auto predicted_city=BnwMinorCityStrengthScratch::prepare_initial_happiness_inputs(*canonical,*p->initial_city_strength,q.suffix_after,q.connection_after,excess);
  std::size_t predicted_count=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto){need(predicted_count<SIZE_MAX,"Initial happiness output extent overflow");++predicted_count;},excess);
  std::vector<std::uint64_t>predicted(predicted_count);at=0;
  initial_happiness_finish_semantics(*p->state,p->actor,[&](auto n){need(at<predicted.size(),"Initial happiness output extent changed");predicted[at++]=static_cast<std::uint64_t>(n);},excess);need(at==predicted.size(),"Initial happiness output extent changed");
  const auto occurrence=p->initial_set_xy->preflight_initial_happiness_game_data();
  const auto revision=initial_best_land_prior_world_revision_without_callbacks();need(revision<UINT64_MAX,"Initial happiness final revision exhausted");
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);if(specialists)specialists->require_current_source();
  require_initial_happiness_suffix_prefix(*p->train_source);initial_happiness_finish_match(*p->state,p->actor,before);
  need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&!p->bank->retired.load()&&initial_best_land_prior_world_revision_without_callbacks()==revision,
   "Initial happiness final source changed before stores");
  q.excess=excess;q.achievement_requested=excess>=100;q.full_before_revision=revision;q.full_after_revision=revision+1;
  q.cache_current_restored=p->initial_city_strength->happiness_current;
  q.full_inputs.swap(predicted);auto&city=*p->initial_city_strength;city.happiness_inputs.swap(predicted_city);
  city.happiness_revision=q.full_after_revision;city.happiness_cache_current=q.cache_current_restored;
  p->state->cities.players.at(p->actor).excess_happiness=excess;p->host->session_->bnw_unhappiness_current_=q.cache_current_restored;
  p->state->ui_source_revision=q.full_after_revision;q.excess_stored=true;
  p->initial_set_xy->record_initial_happiness_excess_store(q.full_after_revision);
  // SDK native setDirty(GameData,true) really runs after the local getter.
  // It is a real Civis flag setter, not a synchronous GPU/event ACK.
  q.game_data_occurrence=p->initial_set_xy->execute_initial_happiness_game_data_dirty(occurrence);
  require_initial_happiness_suffix_prefix(*p->train_source);initial_happiness_finish_match(*p->state,p->actor,initial_current_happiness_finish_inputs_without_callbacks());
  q.full_returned=true;p->initial_set_xy->record_initial_happiness_return(q.game_data_occurrence);
  p->train_source->accept_actual_initial_happiness_return(*canonical);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}
BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch::InitialUnitSetXYDispatch(std::shared_ptr<Storage>p):storage_(std::move(p)){}
void BnwNativeUnitAllocatorCore::require_initial_unit_setxy_dispatch_current(const InitialUnitSetXYDispatch&job)const{
 const auto canonical=retain_factory_owner();const auto p=impl_;const auto d=job.storage_;
 need(d&&d->thread==std::this_thread::get_id()&&!d->retired.load(),"Initial UnitSetXY foreign/retired lease");const auto issued=d->factory_self.lock();
 need(issued&&issued.get()==&job&&!p->initial_unit_setxy_dispatch.owner_before(issued)&&!issued.owner_before(p->initial_unit_setxy_dispatch)&&
  d->core.get()==this&&!d->core.owner_before(canonical)&&!canonical.owner_before(d->core)&&p->initial_happiness&&
  p->initial_happiness->unit_setxy_attempted&&p->initial_happiness->unit_setxy_inflight&&!p->initial_happiness->unit_setxy_returned,
  "Initial UnitSetXY wrong actual issued control/phase");
 need(d->source.get()==p->train_source.get()&&!d->source.owner_before(p->train_source)&&!p->train_source.owner_before(d->source)&&
  d->world.get()==p->state.get()&&!d->world.owner_before(p->state)&&!p->state.owner_before(d->world)&&
  d->generation==p->generation&&d->revision==initial_best_land_prior_world_revision_without_callbacks()&&!d->bank.expired()&&!d->bank.owner_before(p->bank)&&!p->bank.owner_before(d->bank),
  "Initial UnitSetXY actual source world changed");
 d->attempt.require_operation(d->serial);const auto actual=d->attempt.world_lease();need(actual.get()==p->state.get()&&!actual.owner_before(p->state)&&!p->state.owner_before(actual),"Initial UnitSetXY VM world changed");
 require_initial_happiness_return(*d->source);const auto node=p->bank->pending_units.at(p->bank->pending_slot);
 need(!d->node.expired()&&!d->node.owner_before(node)&&!node.owner_before(d->node)&&d->actor==p->actor&&
  d->id==node->scalars.at("m_iID")&&d->x==node->scalars.at("m_iX")&&d->y==node->scalars.at("m_iY")&&!p->bank->retired.load(),
  "Initial UnitSetXY genuine payload changed");
 need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==d->generation&&
  &p->host->session().source()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==d->revision,"Initial UnitSetXY final source bookend changed");
}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch::owner()const{const auto p=storage_;const auto self=p->factory_self.lock();need(self&&self.get()==this,"Initial UnitSetXY nonissued lease");p->core->require_initial_unit_setxy_dispatch_current(*self);return p->actor;}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch::full_id()const{(void)owner();return storage_->id;}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch::x()const{(void)owner();return storage_->x;}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch::y()const{(void)owner();return storage_->y;}
std::shared_ptr<const BnwNativeUnitAllocatorCore::InitialUnitSetXYDispatch>BnwNativeUnitAllocatorCore::begin_initial_unit_setxy_dispatch(
 const ui::SelectedCityTrainCreateSource&source,const BnwStockLuaAttemptLease&attempt)const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 try{
  p->check();require_initial_happiness_return(source);need(!p->initial_happiness->unit_setxy_attempted,"Initial UnitSetXY hook is once only");
  const auto serial=attempt.operation_serial();attempt.require_operation(serial);const auto world=attempt.world_lease();
  need(world.get()==p->state.get()&&!world.owner_before(p->state)&&!p->state.owner_before(world),"Initial UnitSetXY needs actual current SAMEVM");
  const auto actual_source=source.retain_factory_owner();need(actual_source.get()==p->train_source.get()&&!actual_source.owner_before(p->train_source)&&!p->train_source.owner_before(actual_source),"Initial UnitSetXY canonical source differs");
  auto d=std::make_shared<InitialUnitSetXYDispatch::Storage>(canonical,actual_source,attempt);d->serial=serial;d->generation=p->generation;d->revision=initial_best_land_prior_world_revision_without_callbacks();
  d->world=p->state;d->bank=p->bank;const auto node=p->bank->pending_units.at(p->bank->pending_slot);d->node=node;
  d->actor=p->actor;d->id=node->scalars.at("m_iID");d->x=node->scalars.at("m_iX");d->y=node->scalars.at("m_iY");
  auto issued=std::shared_ptr<const InitialUnitSetXYDispatch>(new InitialUnitSetXYDispatch(d));d->factory_self=issued;
  p->check();attempt.require_operation(serial);require_initial_happiness_return(source);
  need(!p->bank->retired.load()&&initial_best_land_prior_world_revision_without_callbacks()==d->revision,"Initial UnitSetXY source changed before once reservation");
  p->initial_unit_setxy_dispatch=issued;p->initial_happiness->unit_setxy_attempted=true;p->initial_happiness->unit_setxy_inflight=true;return issued;
 }catch(...){p->bank->retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::finish_initial_unit_setxy_dispatch(const ui::SelectedCityTrainCreateSource&source,const InitialUnitSetXYDispatch&job)const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 try{require_initial_unit_setxy_dispatch_current(job);need(job.storage_->source.get()==&source,"Initial UnitSetXY returned against wrong Source");
  p->initial_happiness->unit_setxy_inflight=false;p->initial_happiness->unit_setxy_returned=true;job.storage_->retired=true;
  p->initial_set_xy->record_initial_unit_setxy_return();
 }catch(...){p->bank->retired=true;job.storage_->retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::fail_initial_unit_setxy_dispatch(const ui::SelectedCityTrainCreateSource&,const InitialUnitSetXYDispatch&job)const noexcept{
 if(impl_&&impl_->bank){impl_->bank->retired=true;}
 if(job.storage_){job.storage_->retired=true;}
}
BnwNativeUnitAllocatorCore::InitialHappinessFounderDispatch::InitialHappinessFounderDispatch(std::shared_ptr<Storage>p):storage_(std::move(p)){}
std::int32_t BnwNativeUnitAllocatorCore::InitialHappinessFounderDispatch::owner()const{
 const auto p=storage_;const auto self=p->factory_self.lock();
 need(self&&self.get()==this,"Initial founder dispatch is not actual issued canonical job");
 p->core->require_initial_happiness_founder_dispatch_current(*self);return p->actor;
}
void BnwNativeUnitAllocatorCore::require_initial_happiness_founder_dispatch_current(const InitialHappinessFounderDispatch&dispatch)const{
 const auto canonical=retain_factory_owner();const auto p=impl_;const auto d=dispatch.storage_;
 need(d&&d->thread==std::this_thread::get_id()&&!d->retired.load(),"Initial founder foreign/retired job");
 const auto held=d->factory_self.lock();
 need(held&&held.get()==&dispatch&&!p->initial_founder_dispatch.owner_before(held)&&!held.owner_before(p->initial_founder_dispatch)&&
  d->core.get()==this&&!d->core.owner_before(canonical)&&!canonical.owner_before(d->core)&&p->initial_happiness&&
  p->initial_happiness->founder_attempted&&p->initial_happiness->founder_inflight&&!p->initial_happiness->founder_returned,
  "Initial founder wrong source/control/once phase");
 need(d->source.get()==p->train_source.get()&&!d->source.owner_before(p->train_source)&&!p->train_source.owner_before(d->source)&&
  d->world.get()==p->state.get()&&!d->world.owner_before(p->state)&&!p->state.owner_before(d->world)&&
  d->generation==p->generation&&d->revision==initial_best_land_prior_world_revision_without_callbacks()&&d->actor==p->actor&&
  !d->bank.expired()&&!d->bank.owner_before(p->bank)&&!p->bank.owner_before(d->bank),"Initial founder actual world/source authority changed");
 d->attempt.require_operation(d->serial);
 const auto actual=d->attempt.world_lease();need(actual.get()==p->state.get()&&!actual.owner_before(p->state)&&!p->state.owner_before(actual),
  "Initial founder actual SAMEVM world control changed");
 require_initial_happiness_prefix(*d->source);
 need(p->state->world.religion_culture.religions.empty()&&p->state->cities.religions.empty(),
  "Initial founder reached founded-religion owner unavailable");
 const auto node=p->bank->pending_units.at(p->bank->pending_slot);
 need(!d->node.expired()&&!d->node.owner_before(node)&&!node.owner_before(d->node),"Initial founder pending node control changed");
 initial_happiness_match(*p->state,p->actor,d->inputs);
 need(!p->bank->retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==d->generation&&
  initial_best_land_prior_world_revision_without_callbacks()==d->revision,"Initial founder final actual source changed");
}
std::shared_ptr<const BnwNativeUnitAllocatorCore::InitialHappinessFounderDispatch>BnwNativeUnitAllocatorCore::begin_initial_happiness_founder_dispatch(
 const ui::SelectedCityTrainCreateSource&source,const BnwStockLuaAttemptLease&attempt)const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 try{
  p->check();require_initial_happiness_prefix(source);need(!p->initial_happiness->founder_attempted,
   "Initial founder native callback is once-only");
  need(p->state->world.religion_culture.religions.empty()&&p->state->cities.religions.empty(),
   "Initial founder full founded-religion terms remain mandatory");
  const auto actual_source=source.retain_factory_owner();
  need(actual_source.get()==p->train_source.get()&&!actual_source.owner_before(p->train_source)&&!p->train_source.owner_before(actual_source),
   "Initial founder canonical TRAIN issuer differs");
  const auto serial=attempt.operation_serial();attempt.require_operation(serial);const auto world=attempt.world_lease();
  need(world.get()==p->state.get()&&!world.owner_before(p->state)&&!p->state.owner_before(world),
   "Initial founder must run in actual current SAMEVM world");
  auto d=std::make_shared<InitialHappinessFounderDispatch::Storage>(canonical,actual_source,attempt);
  d->serial=serial;d->generation=p->generation;d->revision=initial_best_land_prior_world_revision_without_callbacks();d->actor=p->actor;
  d->world=p->state;d->bank=p->bank;d->node=p->bank->pending_units.at(p->bank->pending_slot);
  std::size_t count=0;initial_happiness_semantics(*p->state,p->actor,[&](auto){need(count<SIZE_MAX,"Initial founder input extent overflow");++count;});
  d->inputs.resize(count);std::size_t at=0;initial_happiness_semantics(*p->state,p->actor,[&](auto n){need(at<d->inputs.size(),"Initial founder input extent changed");d->inputs[at++]=static_cast<std::uint64_t>(n);});
  need(at==count,"Initial founder input extent changed");
  auto issued=std::shared_ptr<const InitialHappinessFounderDispatch>(new InitialHappinessFounderDispatch(d));d->factory_self=issued;
  p->check();attempt.require_operation(serial);require_initial_happiness_prefix(source);initial_happiness_match(*p->state,p->actor,d->inputs);
  need(!p->bank->retired.load()&&initial_best_land_prior_world_revision_without_callbacks()==d->revision,"Initial founder source changed before native once reservation");
  p->initial_founder_dispatch=issued;p->initial_happiness->founder_attempted=true;p->initial_happiness->founder_inflight=true;
  return issued;
 }catch(...){p->bank->retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::finish_initial_happiness_founder_dispatch(const ui::SelectedCityTrainCreateSource&source,
 const InitialHappinessFounderDispatch&dispatch,std::optional<std::int64_t>actual_accumulator)const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 try{
  require_initial_happiness_founder_dispatch_current(dispatch);need(dispatch.storage_->source.get()==&source,
   "Initial founder returned against another TRAIN source");
  const auto model=BnwFounderReligionReadModel::bind(*p->state);std::int32_t religion=-1;
  if(actual_accumulator)religion=narrow(*actual_accumulator);
  else{const auto created=model.religion_created_by_player(p->actor);if(model.eligible_for_founder_benefits(created,p->actor))religion=created;}
  need(religion==-1,"Mandatory actual founded happiness/foreign founder terms required before religion cache store");
  require_initial_happiness_founder_dispatch_current(dispatch);auto&q=*p->initial_happiness;
  // This zero is proved by actual SAMEVM callback/fallback returning NO_RELIGION
  // against complete empty native/owned religion rosters, not an unknown-input
  // substitute. SDK still executes the real paired religion add/store.
  q.field->values[0]=q.after;p->state->unhappiness.players.at(p->actor).happiness=q.after;
  p->state->cities.players.at(p->actor).excess_happiness.reset();
  q.founder_religion=religion;q.religion_stored=true;q.founder_returned=true;q.founder_inflight=false;
  dispatch.storage_->retired=true; // Native suffix/actual GameData setter NEXT; UnitSetXY remains closed.
 }catch(...){p->bank->retired=true;dispatch.storage_->retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::fail_initial_happiness_founder_dispatch(const ui::SelectedCityTrainCreateSource&,
 const InitialHappinessFounderDispatch&dispatch)const noexcept{
 if(impl_&&impl_->bank)impl_->bank->retired=true;
 if(dispatch.storage_)dispatch.storage_->retired=true;
}

void BnwNativeUnitAllocatorCore::require_initial_setxy_tail_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_set_xy&&p->initial_happiness&&p->initial_hidden_binding&&p->initial_city_strength,
  "Initial SetXY tail lacks actual retained source receipts");
 const auto&q=*p->initial_happiness;
 need(q.stored&&q.founder_returned&&q.religion_stored&&q.founder_religion==-1&&q.suffix_stored&&q.excess_stored&&q.full_returned&&
  q.unit_setxy_returned&&!q.unit_setxy_inflight&&p->train_source&&!p->train_source->factory_self_.expired()&&
  !p->train_source->factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(p->train_source->factory_self_),
  "Initial SetXY tail lacks actual returned SAMEVM UnitSetXY");
 require_initial_visibility_binding_controls_without_callbacks(*p->initial_hidden_binding);
 BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*this,*p->initial_city_strength,true);
 initial_happiness_finish_match(*p->state,p->actor,initial_current_happiness_finish_inputs_without_callbacks());
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(prefix.initial_unit_setxy_returned&&prefix.initial_happiness_returned&&prefix.initial_game_data_occurrence==q.game_data_occurrence&&
  ((prefix.boundary==BnwUnitMoveBoundary::initial_popup_release_owner&&!prefix.initial_popup_released)||
  (prefix.boundary==BnwUnitMoveBoundary::initial_garrison_dirty_owner&&prefix.initial_popup_released&&!prefix.initial_garrison_dirty_returned)||
  (prefix.boundary==BnwUnitMoveBoundary::initial_center_unit_owner&&prefix.initial_popup_released&&prefix.initial_garrison_dirty_returned)||
  ((prefix.boundary==BnwUnitMoveBoundary::initial_unit_name_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_unit_class_count_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||(prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned)))&&prefix.initial_unit_info_promotion_prefix_stored))&&prefix.initial_military_support_returned))&&prefix.initial_resource_nuke_zero_returned))&&prefix.initial_support_zero_returned))&&prefix.initial_player_class_counter_returned))&&prefix.initial_team_class_counter_returned))&&prefix.initial_game_class_counter_returned))&&prefix.initial_name_game_counter_returned))&&prefix.initial_popup_released&&prefix.initial_garrison_dirty_returned&&prefix.initial_center_unit_returned&&p->initial_center_plan)),
  "Initial SetXY tail actual ordered popup/GARRISON phase differs");
}
std::optional<std::int32_t>BnwNativeUnitAllocatorCore::initial_actual_garrison_city()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;require_initial_setxy_tail_without_callbacks();
 const auto node=p->bank->pending_units.at(p->bank->pending_slot);
 // CvUnit::IsGarrisoned: default layer -> plot city -> plot owner ->
 // getBestDefender(owner) -> exact full GetID equality, never any-defender test.
 if(node->scalars.at("m_iMapLayer")!=0)return {};
 const auto args=initial_placement_operands();need(args.city>=0&&args.state->world.map_plots.has_value(),"Initial garrison actual destination city missing");
 const auto index=args.state->world.map_header.grid.index(static_cast<std::uint64_t>(args.x),static_cast<std::uint64_t>(args.y));
 const auto&plot=args.state->world.map_plots->plots.at(static_cast<std::size_t>(index));
 const auto owner=narrow(garrison_named(plot.fields,"owner").value);
 if(owner!=p->actor){require_initial_setxy_tail_without_callbacks();return {};}
 const auto selected=BnwMinorCityStrengthScratch::selected_initial_garrison_unit_id(*canonical,args.city);
 require_initial_setxy_tail_without_callbacks();
 return selected&&*selected==node->scalars.at("m_iID")?std::optional{args.city}:std::nullopt;
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_popup_release_and_garrison()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_happiness&&p->initial_happiness->full_returned&&p->initial_happiness->unit_setxy_returned,
  "Initial popup release requires genuine returned SAMEVM predecessor");
 try{
  p->check();require_initial_setxy_tail_without_callbacks();
  need(!p->initial_set_xy->prefix().initial_popup_released,"Actual popup/GARRISON tail already attempted");
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  require_initial_setxy_tail_without_callbacks();p->initial_set_xy->execute_initial_popup_release_and_garrison();
  require_initial_setxy_tail_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}
// Private issuer state survives advance and the stack renderer loan. It owns
// actual source/node controls; Core itself stays weak to avoid a retention cycle.
struct BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::Storage {
 Binding binding;std::shared_ptr<InitialCenterCache> cache;
 std::shared_ptr<Bank> bank;std::shared_ptr<const void> pending_node;
 std::shared_ptr<ui::BnwClientSelectionControl> input;
 std::optional<BnwStockLuaAttemptLease> attempt;
 std::optional<BnwStockLuaAttemptLease::ScopeIdentity> scope;
 std::shared_ptr<LiveSession::BnwGameDebugReadStorage> debug;
 bool raw_debug=false,cached_debug=false,fanout=false,completed=false;
 std::vector<Entry> entries;std::vector<Setter> setters;
 std::vector<std::uint64_t> seal,dirty_occurrences,name_successor_seal,promotion_successor_seal,promotion_loop_seal,traits_promotion_seal,player_promotion_seal,embark_promotion_seal,exotic_xp_seal,immobile_moves_seal,set_moves_seal,cycle_order_seal,army_ai_seal;
 std::uint64_t input_revision=0;std::size_t applied=0;
 bool dirty_inflight=false;std::size_t dirty_setter=0;
 InitialCenterHandle expected;
 std::shared_ptr<const graphics::BnwInitialCenterUnitVisibilityProof> proof;
};
BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::InitialCenterUnitVisibilityPlan(std::shared_ptr<Storage>s):storage_(std::move(s)){}
void BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::require_current_without_callbacks()const{
 need(storage_&&storage_->binding.core,"Center plan was not issued by genuine Core");
 const auto actual=storage_->binding.core_control.lock();
 need(actual&&actual.get()==storage_->binding.core,"Center canonical Core expired");
 actual->require_initial_center_plan_without_callbacks(*this);
}
void BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::require_visibility_dispatch_without_callbacks()const{
 require_current_without_callbacks();const auto&q=*storage_;
 need(q.applied==q.setters.size()&&!q.dirty_inflight,"Center visibility precedes actual ordered center/UnitInfo stores");
 for(std::size_t i=0;i<q.setters.size();++i)need(q.setters[i].unit_info_dirty?(q.dirty_occurrences[i]>0):(q.dirty_occurrences[i]==0),
  "Center visibility lacks exact setter-specific native UnitInfo return");
}
BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::Binding
BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::capture_binding()const{
 require_current_without_callbacks();return storage_->binding;
}
const std::vector<BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::Entry>&
BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::ordered_entries()const{
 require_current_without_callbacks();return storage_->entries;
}
const std::vector<BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::Setter>&
BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::setter_journal()const{
 require_current_without_callbacks();return storage_->setters;
}
bool BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan::requires_visibility_fanout()const{
 require_current_without_callbacks();return storage_->fanout;
}
void BnwNativeUnitAllocatorCore::require_initial_center_plan_without_callbacks(const InitialCenterUnitVisibilityPlan&plan)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign center plan before live borrow");
 need(plan.storage_&&p->initial_center_plan&&p->initial_center_plan->storage_.get()==plan.storage_.get()&&
  !p->initial_center_plan->storage_.owner_before(plan.storage_)&&!plan.storage_.owner_before(p->initial_center_plan->storage_),
  "Center plan is not actual canonical stored issuer state");
 const auto&q=*plan.storage_;const auto&b=q.binding;
 need(b.core==this&&!b.core_control.owner_before(factory_self_)&&!factory_self_.owner_before(b.core_control)&&
  b.host.get()==p->host.get()&&!b.host.owner_before(p->host)&&!p->host.owner_before(b.host)&&
  b.session==p->host->session_.get()&&b.state.get()==p->state.get()&&!b.state.owner_before(p->state)&&!p->state.owner_before(b.state)&&
  b.source==p->source&&b.pending.get()==p->initial_set_xy.get()&&!b.pending.owner_before(p->initial_set_xy)&&!p->initial_set_xy.owner_before(b.pending),
  "Center actual Core/Host/Session/Pending controls differ");
 require_initial_setxy_tail_without_callbacks();
 need(q.attempt&&q.scope,"Center has no actual active SAMEVM scope");
 q.attempt->require_scope_identity_without_callbacks(*q.scope);
 const auto&r=p->state->setup.extensions.find<Registry>();
 need(r&&r->centers.get()==q.cache.get()&&p->centers.get()==q.cache.get()&&
  !r->centers.owner_before(q.cache)&&!q.cache.owner_before(r->centers)&&
  !q.cache->retired.load()&&q.cache->creator==p->thread&&q.cache->generation==p->generation&&
  q.cache->source.lock().get()==p->state.get()&&b.registry==q.cache.get()&&
  !b.registry_control.owner_before(q.cache)&&!q.cache.owner_before(b.registry_control)&&
  b.bank==p->bank.get()&&q.bank.get()==p->bank.get()&&!q.bank.owner_before(p->bank)&&!p->bank.owner_before(q.bank),
  "Center registry/cache/bank genuine lifetime differs");
 const auto&session=*p->host->session_;
 need(session.bnw_game_debug_owner_current()&&session.debug_.get()==q.debug.get()&&
  !session.debug_.owner_before(q.debug)&&!q.debug.owner_before(session.debug_)&&!q.debug->retired.load()&&
  q.debug->raw_mode==q.raw_debug&&q.debug->cached_mode==q.cached_debug,
  "Center actual debug source/cache changed");
 need(initial_setxy_earned_world_revision_without_callbacks()==b.world_revision&&session.state_generation_==b.generation&&session.state_.get()==b.state.get()&&
  session.source_.get()==b.source&&session.city_selection_.lock().get()==q.input.get()&&
  initial_center_earned_input_revision_without_callbacks(q.input)==q.input_revision,
  "Center current world/input revision or source replaced");
 const auto&node=p->bank->pending_units.at(p->bank->pending_slot);
 need(node.get()==q.pending_node.get()&&!node.owner_before(q.pending_node)&&!q.pending_node.owner_before(node),"Center real pending node changed");
 std::size_t at=0;initial_post_city_semantics(*p->state,p->actor,b.x,b.y,node->scalars,node->indexed,[&](auto n){
  const auto&seal=p->initial_army_ai&&p->initial_army_ai->stored?q.army_ai_seal:(p->initial_unit_cycler&&p->initial_unit_cycler->stored?q.cycle_order_seal:(p->initial_set_moves&&p->initial_set_moves->stored&&p->initial_set_moves->changed?q.set_moves_seal:(p->initial_immobile_moves&&p->initial_immobile_moves->stored?q.immobile_moves_seal:(p->initial_exotic_xp&&p->initial_exotic_xp->stored?q.exotic_xp_seal:(p->initial_embark_promotions&&p->initial_embark_promotions->stored?q.embark_promotion_seal:(p->initial_player_promotions&&p->initial_player_promotions->stored?q.player_promotion_seal:(p->initial_traits_promotions&&p->initial_traits_promotions->stored?q.traits_promotion_seal:(p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored?q.promotion_loop_seal:(p->initial_unit_info_promotion&&p->initial_unit_info_promotion->stored?q.promotion_successor_seal:(p->initial_name_game&&p->initial_name_game->stored?q.name_successor_seal:q.seal))))))))));
  need(at<seal.size()&&seal[at]==static_cast<std::uint64_t>(n),"Center native plot/visibility/active-team operands changed");++at;
 });need(at==(p->initial_army_ai&&p->initial_army_ai->stored?q.army_ai_seal.size():(p->initial_unit_cycler&&p->initial_unit_cycler->stored?q.cycle_order_seal.size():(p->initial_set_moves&&p->initial_set_moves->stored&&p->initial_set_moves->changed?q.set_moves_seal.size():(p->initial_immobile_moves&&p->initial_immobile_moves->stored?q.immobile_moves_seal.size():(p->initial_exotic_xp&&p->initial_exotic_xp->stored?q.exotic_xp_seal.size():(p->initial_embark_promotions&&p->initial_embark_promotions->stored?q.embark_promotion_seal.size():(p->initial_player_promotions&&p->initial_player_promotions->stored?q.player_promotion_seal.size():(p->initial_traits_promotions&&p->initial_traits_promotions->stored?q.traits_promotion_seal.size():(p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored?q.promotion_loop_seal.size():(p->initial_unit_info_promotion&&p->initial_unit_info_promotion->stored?q.promotion_successor_seal.size():(p->initial_name_game&&p->initial_name_game->stored?q.name_successor_seal.size():q.seal.size()))))))))))),"Center relevant operand extent changed");
 const auto slot=q.cache->plots.find(b.plot);need(slot!=q.cache->plots.end()&&same_center_handle(slot->second,q.expected),"Center weak handle changed outside native ordered stores");
 for(const auto&e:q.entries){
  if(!e.resolved){need(!e.imported&&!e.native_node&&native_imported_get_at(*p->state,e.owner,e.asked_id)==nullptr,"Center NULL roster entry changed native resolution");continue;}
  const auto control=e.native_control.lock();need(control&&e.native_node,"Center actual roster storage expired");
  if(e.pending){need(e.native_node==node.get()&&control.get()==node.get()&&!e.native_control.owner_before(node)&&!node.owner_before(e.native_control)&&
    e.full_id==node->scalars.at("m_iID")&&e.owner==node->scalars.at("m_eOwner")&&e.unit_type==node->scalars.at("m_eUnitType"),"Center pending identity changed");}
  else{need(control.get()==p->state.get()&&!e.native_control.owner_before(p->state)&&!p->state.owner_before(e.native_control)&&
    native_imported_get_at(*p->state,e.owner,e.full_id)==e.imported&&e.native_node==e.imported&&
    core(*e.imported,"m_eUnitType")==e.unit_type,"Center actual imported current FLTA node changed");}
 }
 // Source/operand comparisons above allocate no storage. Actual scope, facade,
 // pointers and generations are rebookended AFTER the finite semantic visitor.
 q.attempt->require_scope_identity_without_callbacks(*q.scope);
 require_initial_setxy_tail_without_callbacks();
 need(!q.cache->retired.load()&&!p->bank->retired.load()&&session.state_.get()==p->state.get()&&session.state_generation_==p->generation&&
  session.source_.get()==p->source&&initial_setxy_earned_world_revision_without_callbacks()==b.world_revision&&
  initial_center_earned_input_revision_without_callbacks(q.input)==q.input_revision,"Center final source bookend differs");
}
BnwNativeUnitAllocatorCore::InitialCenterDirtyOperands
BnwNativeUnitAllocatorCore::initial_center_dirty_operands(std::size_t setter)const{
 const auto p=impl_;need(p&&p->initial_center_plan,"Center dirty lacks genuine stored plan");
 const auto&plan=*p->initial_center_plan;require_initial_center_plan_without_callbacks(plan);const auto&q=*plan.storage_;
 need(q.dirty_inflight&&q.dirty_setter==setter&&setter<q.setters.size()&&q.applied==setter+1&&
  q.setters[setter].unit_info_dirty&&q.setters[setter].after>=0&&q.dirty_occurrences[setter]==0,"Center UnitInfo is not the actual reached setter");
 const auto&e=q.entries.at(static_cast<std::size_t>(q.setters[setter].after));
 return {e.owner,e.full_id,q.binding.generation,q.binding.world_revision,q.input_revision};
}
void BnwNativeUnitAllocatorCore::record_initial_center_dirty_store(std::size_t setter,std::uint64_t occurrence)const noexcept{
 auto&q=*impl_->initial_center_plan->storage_;q.dirty_occurrences[setter]=occurrence;++q.input_revision;q.dirty_inflight=false;
}
std::optional<std::pair<std::int32_t,std::int32_t>>BnwNativeUnitAllocatorCore::read_native_cached_plot_center(std::int32_t x,std::int32_t y)const{
 const auto actual=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 const auto c=p->centers;need(c&&!c->retired.load()&&c->creator==p->thread&&!c->busy.load()&&c->generation==p->generation&&
  c->source.lock().get()==p->state.get(),"Native center cache unavailable/busy");
 const auto plot=MapGeometry{p->state->world.map_header.grid,p->state->world.map_header.wrap_x,p->state->world.map_header.wrap_y,bnw}.plot(x,y);
 need(plot.has_value(),"Native cached center plot outside actual map");
 const auto found=c->plots.find(static_cast<std::uint64_t>(*plot));
 if(found==c->plots.end()||!found->second.node||found->second.control.expired())return {};
 const auto&h=found->second;
 if(const auto*unit=native_imported_get_at(*p->state,h.owner,h.id)){need(unit==h.node,"Native cached center imported fullID changed");return std::pair{h.owner,h.id};}
 for(const auto&[actor,bank]:p->state->setup.extensions.find<Registry>()->players)if(actor==h.owner)
  for(const auto&[slot,node]:bank->pending_units){(void)slot;if(node.get()==h.node&&node->scalars.at("m_iID")==h.id)return std::pair{h.owner,h.id};}
 throw BnwNativeUnitAllocatorUnavailable("Native center live handle lacks actual current native node");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_center_unit_visibility(
 const graphics::BnwInitialCenterUnitRendererLoan&renderer,const BnwStockLuaAttemptLease&attempt)const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 const auto cache=p->centers;bool cache_entered=false;
 try{
  p->check();require_initial_setxy_tail_without_callbacks();
  need(!p->initial_center_plan&&p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_center_unit_owner,
   "Center prefix requires real SetXY return exactly once");
  need(cache&&cache->creator==p->thread&&!cache->retired.load(),"Center genuine cache unavailable");
  if(cache->busy.exchange(true)){cache->retired=true;throw BnwNativeUnitAllocatorUnavailable("Center actual shared-cache reentry");}cache_entered=true;
  renderer.require_current_for_center(*self,*p->initial_set_xy);
  const auto scope=attempt.pin_active_scope_without_callbacks();need(scope.world==p->state.get(),"Center loan belongs to another actual VM world");
  const auto args=initial_placement_operands();const auto node=p->bank->pending_units.at(p->bank->pending_slot);
  auto q=std::make_shared<InitialCenterUnitVisibilityPlan::Storage>();q->cache=cache;q->bank=p->bank;q->pending_node=node;
  q->attempt=attempt;q->scope=scope;q->input=p->host->session().city_selection_.lock();need(bool(q->input),"Center canonical input missing");
  q->input->require_current();q->input_revision=q->input->initial_center_revision_without_callbacks();
  const auto&session=*p->host->session_;need(session.bnw_game_debug_owner_current(),"Center genuine debug owner absent");
  q->debug=session.debug_;q->raw_debug=q->debug->raw_mode;q->cached_debug=q->debug->cached_mode;
  const auto active=detail::bnw_game_active_player_read(*p->state),team=detail::bnw_game_active_team_read(*p->state);
  need(active==p->actor&&team>=0,"Center active-player/team branch unavailable");
  const auto plot=p->state->world.map_header.grid.index(static_cast<std::uint64_t>(args.x),static_cast<std::uint64_t>(args.y));
  q->binding={self.get(),self,p->host,p->host->session_.get(),p->state,p->initial_set_xy,p->source,cache.get(),cache,p->bank.get(),p->bank,
   active,team,args.x,args.y,p->generation,initial_best_land_prior_world_revision_without_callbacks(),scope.serial,plot};
  const auto&visibility=bnw_live_plot_visibility_cache(*p->state);
  q->fanout=q->cached_debug||visibility.counts.at(static_cast<std::size_t>(plot*80+static_cast<std::uint64_t>(team)))>0;
  // Complete own-team/LAND roster is resolved only on the native active-visible
  // branch. Inactive-visible branch really performs only setCenterUnit(NULL).
  if(q->fanout){
   const auto choice=BnwMinorCityStrengthScratch::prepare_initial_center_selection(*self,q->input->initial_center_selection());
   for(const auto&e:choice.entries){InitialCenterUnitVisibilityPlan::Entry out;
    out.owner=e.owner;out.asked_id=e.asked_id;out.full_id=e.id;out.unit_type=e.type;out.imported=e.raw;out.pending=e.pending;out.resolved=e.resolved;
    if(e.resolved){out.native_node=e.pending?static_cast<const void*>(node.get()):static_cast<const void*>(e.raw);
     out.native_control=e.pending?std::weak_ptr<const void>{node}:std::weak_ptr<const void>{p->state};}
    q->entries.push_back(std::move(out));
   }
   const auto previous=cache->plots.find(plot);if(previous!=cache->plots.end())q->expected=previous->second;
   std::int32_t old=-1;if(q->expected.node&&!q->expected.control.expired()){
    for(std::size_t i=0;i<q->entries.size();++i)if(q->entries[i].resolved&&q->entries[i].native_node==q->expected.node&&q->entries[i].owner==q->expected.owner&&q->entries[i].full_id==q->expected.id){need(old==-1,"Center prior weak handle duplicated in full roster");old=narrow(static_cast<std::int64_t>(i));}
    need(old>=0,"Center prior live weak handle is outside genuine ordered roster");
   }
   const auto append=[&](std::optional<std::size_t> index){const auto next=index?narrow(static_cast<std::int64_t>(*index)):-1;
    q->setters.push_back({old,next,old!=next&&next>=0});old=next;};
   append(choice.selected);if(old==-1&&choice.best)append(choice.best);if(old==-1&&choice.head)append(choice.head);
   for(std::size_t i=0;i<q->entries.size();++i)q->entries[i].visible=q->entries[i].resolved&&old>=0&&i==static_cast<std::size_t>(old);
  }else{const auto previous=cache->plots.find(plot);if(previous!=cache->plots.end())q->expected=previous->second;
   std::int32_t previous_index=-1;
   if(q->expected.node&&!q->expected.control.expired()){
    InitialCenterUnitVisibilityPlan::Entry e;e.owner=q->expected.owner;e.asked_id=q->expected.id;e.full_id=q->expected.id;e.native_node=q->expected.node;e.native_control=q->expected.control;e.resolved=true;
    if(node.get()==e.native_node){e.pending=true;e.unit_type=node->scalars.at("m_eUnitType");}
    else{e.imported=native_imported_get_at(*p->state,e.owner,e.full_id);need(e.imported&&e.imported==e.native_node,"Invisible center cached native node no longer resolves");e.unit_type=narrow(core(*e.imported,"m_eUnitType"));}
    q->entries.push_back(e);previous_index=0;
   }
   q->setters.push_back({previous_index,-1,false}); // Exact old weak handle -> NULL; NO visibility loop.
  }
  q->dirty_occurrences.resize(q->setters.size());
  std::size_t count=0;initial_post_city_semantics(*p->state,p->actor,args.x,args.y,node->scalars,node->indexed,[&](auto){need(count<SIZE_MAX,"Center operand seal overflow");++count;});
  q->seal.resize(count);std::size_t at=0;initial_post_city_semantics(*p->state,p->actor,args.x,args.y,node->scalars,node->indexed,[&](auto n){need(at<q->seal.size(),"Center seal changed while allocating");q->seal[at++]=static_cast<std::uint64_t>(n);});need(at==count,"Center seal extent changed");
  // Stage the actual unsaved NULL handle node before publishing the plan. No
  // unit roster/current cache is mutated by allocating this registry entry.
  auto plots=cache->plots;plots.try_emplace(plot,InitialCenterHandle{});
  p->check();require_initial_setxy_tail_without_callbacks();attempt.require_scope_identity_without_callbacks(scope);
  need(q->input->initial_center_revision_without_callbacks()==q->input_revision,"Center selected input changed during source preparation");
  const auto before=cache->plots.find(plot);need((before==cache->plots.end()&&!q->expected.node)||(before!=cache->plots.end()&&same_center_handle(before->second,q->expected)),"Center handle changed during plan preparation");
  cache->plots.swap(plots);
  p->initial_center_plan=std::shared_ptr<const InitialCenterUnitVisibilityPlan>(new InitialCenterUnitVisibilityPlan(q));
  require_initial_center_plan_without_callbacks(*p->initial_center_plan);
  std::size_t dirty_count=0;for(const auto&setter:q->setters)if(setter.unit_info_dirty)++dirty_count;
  q->input->preflight_initial_center_info_dirty_setters(dirty_count);
  renderer.preflight_plot_unit_visibility(*p->initial_center_plan); // ALL receivers before native/dirty stores.
  p->check();require_initial_center_plan_without_callbacks(*p->initial_center_plan);
  for(std::size_t i=0;i<q->setters.size();++i){
   require_initial_center_plan_without_callbacks(*p->initial_center_plan);const auto&setter=q->setters[i];InitialCenterHandle next;
   if(setter.after>=0){const auto&e=q->entries.at(static_cast<std::size_t>(setter.after));next={e.native_node,e.native_control,e.owner,e.full_id,true};}
   else next.ignore_destruction=true; // Native setCenterUnit(NULL) still invokes ignoreDestruction(true).
   cache->plots.at(plot)=next;q->expected=next;q->applied=i+1;
   if(setter.unit_info_dirty){q->dirty_inflight=true;q->dirty_setter=i;(void)q->input->set_initial_center_info_dirty(*self,i);}
  }
  require_initial_center_plan_without_callbacks(*p->initial_center_plan);
  p->initial_center_plan->require_visibility_dispatch_without_callbacks();
  auto supplied=renderer.apply_plot_unit_visibility(*p->initial_center_plan);
  need(bool(supplied),"Center genuine renderer did not return its actual visibility store proof");
  auto proof=supplied->retain_factory_owner();need(proof&&proof.get()==supplied.get()&&!proof.owner_before(supplied)&&!supplied.owner_before(proof),"Center rejects noncanonical renderer proof control");
  q->proof=proof;proof->require_for_plan(*p->initial_center_plan);
  renderer.require_plot_unit_visibility_current(*p->initial_center_plan,*proof);proof->require_current_for_core(*self);
  p->check();require_initial_center_plan_without_callbacks(*p->initial_center_plan);
  std::uint64_t receivers=0;for(const auto&e:q->entries)if(e.resolved)++receivers;
  p->initial_set_xy->record_initial_center_unit_return(static_cast<std::uint64_t>(q->setters.size()),q->fanout?receivers:0);
  q->completed=true;require_initial_center_plan_without_callbacks(*p->initial_center_plan);cache->busy=false;return p->initial_set_xy;
 }catch(...){p->bank->retired=true;if(cache&&cache_entered){cache->retired=true;cache->busy=false;}throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_name_game_counter_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_name_game&&p->initial_name_game->stored,
  "Init name/Game count actual stored receipt missing");
 const auto&q=*p->initial_name_game;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&
  p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&p->bank&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.state_generation_==p->generation&&session.source_.get()==p->source&&
  initial_game_class_prior_revision_without_callbacks()==q.after_revision&&q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1,
  "Init name/Game count actual source/earned revision changed");
 need(p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot))&&
  initial_name_node_matches_without_callbacks()&&q.node->scalars.at("m_iGameTurnCreated")==q.turn&&q.node->scalars.at("m_eGreatWork")==-1,
  "Init name/creation turn actual SAME node fields changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(p->state->world.pregame&&p->state->world.pregame->layout==bnw&&p->state->world.progression.layout==bnw,
  "Init name/Game actual clock/progression layout missing");
 const std::int64_t*turn=nullptr;for(const auto&[key,value]:p->state->world.pregame->archive.integers)if(std::string_view(key)=="game_turn")turn=&value;
 need(turn&&*turn==q.turn&&p->state->setup.game_turn==q.turn,"Init name/Game actual paired clock changed");
 const auto&arrays=p->state->world.progression.arrays;
 need(q.array_index<arrays.size()&&&arrays[q.array_index]==q.array&&name(q.array->name)=="unit_created_count"&&q.array->columns==1&&
  q.array->rows.size()==q.rows.size()&&q.row_index<q.rows.size()&&&q.array->rows[q.row_index]==q.row,
  "Init Game count actual primary row/storage replaced");
 for(std::size_t i=0;i<q.rows.size();++i){const auto&r=q.array->rows[i];need(r.type_hash==std::optional<std::uint64_t>{q.rows[i].hash}&&r.values.size()==1&&
  r.values[0]==(i==q.row_index?q.after:q.rows[i].value),"Init Game count complete selected wire table changed");}
 need(q.row->type_hash==std::optional<std::uint64_t>{q.hash}&&q.before>=0&&q.before<INT32_MAX&&q.after==q.before+1,
  "Init Game count native checked increment changed");
 const auto*registry=p->state->setup.extensions.find<Registry>();need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor).get()==p->bank.get()&&
  !registry->players.at(p->actor).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(p->actor)),"Init name/Game bank authority replaced");
 need(session.state_.get()==p->state.get()&&session.state_generation_==p->generation&&session.source_.get()==p->source&&
  initial_game_class_prior_revision_without_callbacks()==q.after_revision&&!p->bank->retired.load(),"Init name/Game final source bookend changed");
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_setxy_earned_world_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init descendant revision before live borrow");
 if(p->initial_name_game&&p->initial_name_game->stored){require_initial_name_game_counter_without_callbacks();return p->initial_name_game->before_revision;}
 return initial_best_land_prior_world_revision_without_callbacks();
}
void BnwNativeUnitAllocatorCore::require_initial_name_game_counter_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source.get()==&source&&!source.factory_self_.expired()&&
  !source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),"Init name/Game canonical TRAIN Source changed");
 require_initial_name_game_counter_without_callbacks();
 need(p->initial_set_xy&&p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().initial_name_game_counter_returned,
  "Init name/Game actual pending return missing");
 require_initial_center_plan_without_callbacks(*p->initial_center_plan);
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_name_game_counter_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_center_plan&&!p->initial_name_game&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().initial_center_unit_returned&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_unit_name_owner,
  "Init name/Game requires genuine returned center/once predecessor");
 try{
  p->check();require_initial_center_plan_without_callbacks(*p->initial_center_plan);
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  const auto node=p->bank->pending_units.at(p->bank->pending_slot);check_registered(*p->bank,*node,node,p->bank->pending_slot);
  const auto type=node->scalars.at("m_eUnitType");const content::InfoTypeEntry*info=nullptr;
  for(const auto&[key,e]:p->host->types().entries()){(void)key;if(e.table=="Units"&&e.id==type){need(!info,"Init name duplicate selected UnitInfo ID");info=&e;}}
  need(info&&info->hash==content::saved_info_type_hash(info->type),"Init name actual selected UnitInfo remap missing");
  InitialNameGamePrefix q;q.node=node;q.node_before=node->scalars;q.node_after=q.node_before;
  auto&arrays=p->state->world.progression.arrays;need(p->state->world.progression.layout==bnw,"Init Game count actual BNW progression missing");
  for(std::size_t i=0;i<arrays.size();++i)if(name(arrays[i].name)=="unit_created_count"){need(!q.array,"Duplicate actual Game unit-created array");q.array=&arrays[i];q.array_index=i;}
  need(q.array&&q.array->columns==1,"Init Game actual unit-created array missing");
  std::set<std::int32_t>ids;
  for(std::size_t i=0;i<q.array->rows.size();++i){auto&r=q.array->rows[i];need(r.type_hash&&*r.type_hash!=0&&r.values.size()==1,"Init Game unknown/missing native unit-created slot requires real Reset/Read supplier");
   const auto*e=p->host->types().find_hash(*r.type_hash);need(e&&e->table=="Units"&&e->id>=0&&e->hash==*r.type_hash&&ids.insert(narrow(e->id)).second,"Init Game unit-created Type/ID/hash remap differs");
   const auto value=narrow(r.values[0]);need(value>=0,"Init Game negative native unit-created count");q.rows.push_back({*r.type_hash,value});
   if(e->id==type){need(!q.row,"Init Game duplicate target unit-created slot");q.row=&r;q.row_index=i;q.hash=*r.type_hash;q.before=value;}
  }
  std::size_t expected=0;for(const auto&[key,e]:p->host->types().entries())if(e.table=="Units"){(void)key;need(e.id>=0&&ids.contains(narrow(e.id)),"Init Game incomplete genuine selected UnitInfo counter universe");++expected;}
  need(expected==q.rows.size()&&q.row&&q.before<INT32_MAX,"Init Game actual target count missing/overflow");q.after=q.before+1;
  // GetUnitCreatedCount precedes GetNumUnitNames. Complete zero actual rows
  // make the source name/RNG loop unreachable, rather than skipping it.
  std::size_t name_rows=0;std::int64_t num_names=-1;
  const auto names=p->host->database().query_rows({"SELECT COUNT(*) AS NumNames FROM Unit_UniqueNames WHERE UnitType=?",{info->type}},[&](const auto&r){++name_rows;num_names=r.integer_at(0);});
  need(names.all_succeeded()&&name_rows==1&&num_names==0,"Mandatory native unique-name/RNG/GreatWork history loop required");
  need(p->state->world.pregame&&p->state->world.pregame->layout==bnw,"Init native Game turn source absent");
  const std::int64_t*turn=nullptr;for(const auto&[key,value]:p->state->world.pregame->archive.integers)if(std::string_view(key)=="game_turn")turn=&value;
  need(turn&&*turn>=0&&p->state->setup.game_turn==*turn,"Init actual paired GameTurn differs");q.turn=narrow(*turn);
  need(q.node_after.contains("m_eGreatWork")&&q.node_after.contains("m_iGameTurnCreated"),"Init actual native fields not allocated");
  q.node_after.at("m_eGreatWork")=-1;q.node_after.at("m_iGameTurnCreated")=q.turn;
  auto stored_scalars=q.node_after;auto dirty=node->dirty;for(const auto key:{"m_eGreatWork","m_iGameTurnCreated"})if(q.node_before.at(key)!=q.node_after.at(key))if(auto index=archive_index(*node,key))dirty.insert(*index);
  q.dirty_after=dirty;
  q.before_revision=initial_best_land_prior_world_revision_without_callbacks();need(q.before_revision<UINT64_MAX,"Init Game count source revision exhausted");q.after_revision=q.before_revision+1;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.node_after,node->indexed);
  const auto&binding=*p->initial_hidden_binding;
  std::vector<std::uint64_t> post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.node_after,node->indexed,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  p->initial_name_game.emplace(std::move(q));auto&stage=*p->initial_name_game;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_center_plan_without_callbacks(*p->initial_center_plan);
  need(node->scalars==stage.node_before&&initial_best_land_prior_world_revision_without_callbacks()==stage.before_revision&&
   &arrays[stage.array_index]==stage.array&&stage.array->rows.size()==stage.rows.size()&&&stage.array->rows[stage.row_index]==stage.row,
   "Init name/Game actual source changed during staged preparation");
  for(std::size_t i=0;i<stage.rows.size();++i){const auto&r=stage.array->rows[i];need(r.type_hash==std::optional<std::uint64_t>{stage.rows[i].hash}&&r.values.size()==1&&r.values[0]==stage.rows[i].value,"Init Game count changed during preparation");}
  const std::int64_t*final_turn=nullptr;for(const auto&[key,value]:p->state->world.pregame->archive.integers)if(std::string_view(key)=="game_turn")final_turn=&value;
  need(final_turn&&*final_turn==stage.turn&&p->state->setup.game_turn==stage.turn&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation&&!p->bank->retired.load(),"Init name/Game final clock/lifetime bookend differs");
  // Native order: SetGreatWork -> ordinary name no-effect branch ->
  // GameTurnCreated -> Game unit-created count. All storage exists first.
  node->scalars.swap(stored_scalars);
  node->dirty.swap(dirty);stage.row->values[0]=stage.after;
  p->initial_city_strength->name_inputs.swap(city);p->initial_city_strength->name_prefix_stored=true;
  p->initial_post_city->name_successor_seal.swap(post);p->initial_center_plan->storage_->name_successor_seal.swap(center);
  p->state->ui_source_revision=stage.after_revision;stage.stored=true;
  p->initial_set_xy->record_initial_name_game_counter_return(stage.turn,stage.before,stage.after,stage.after_revision);
  p->train_source->accept_actual_initial_name_game_counter_prefix(*canonical);
  require_initial_name_game_counter_prefix(*p->train_source);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_game_class_counter_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_game_class&&p->initial_game_class->stored,
  "Init Game class actual stored receipt missing");
 const auto&q=*p->initial_game_class;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&
  p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&p->bank&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  initial_team_class_prior_revision_without_callbacks()==q.after_revision&&q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1&&
  p->initial_name_game&&p->initial_name_game->stored&&p->initial_name_game->after_revision==q.before_revision,
  "Init Game class actual source/earned revision differs");
 need(p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot))&&
  q.node->scalars.at("m_eUnitType")==q.type&&q.node->scalars.at("m_eOwner")==p->actor,
  "Init Game class actual SAME node/UnitInfo changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 const auto&arrays=p->state->world.progression.arrays;
 need(p->state->world.progression.layout==bnw&&q.array_index<arrays.size()&&&arrays[q.array_index]==q.array&&
  name(q.array->name)=="unitclass_created_count"&&q.array->columns==1&&q.array->rows.size()==q.rows.size()&&
  q.row_index<q.rows.size()&&&q.array->rows[q.row_index]==q.row,"Init Game class actual native table/storage changed");
 for(std::size_t i=0;i<q.rows.size();++i){const auto&r=q.array->rows[i];need(r.type_hash==std::optional<std::uint64_t>{q.rows[i].hash}&&
  (q.rows[i].hash==0?(!q.rows[i].value&&r.values.empty()):q.rows[i].value&&r.values.size()==1&&r.values[0]==(i==q.row_index?q.after:*q.rows[i].value)),"Init Game class native complete counter universe changed");}
 need(q.row->type_hash==std::optional<std::uint64_t>{q.hash}&&q.before>=0&&q.before<INT32_MAX&&q.after==q.before+1,
  "Init Game class actual checked +1 changed");
 const auto*registry=p->state->setup.extensions.find<Registry>();need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor).get()==p->bank.get()&&
  !registry->players.at(p->actor).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(p->actor)),"Init Game class bank authority changed");
 need(session.state_.get()==p->state.get()&&session.state_generation_==p->generation&&session.source_.get()==p->source&&
  initial_team_class_prior_revision_without_callbacks()==q.after_revision&&!p->bank->retired.load(),"Init Game class final source bookend differs");
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_game_class_prior_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init Game class lineage before borrow");
 if(p->initial_game_class&&p->initial_game_class->stored){require_initial_game_class_counter_without_callbacks();return p->initial_game_class->before_revision;}
 return initial_best_land_prior_world_revision_without_callbacks();
}
void BnwNativeUnitAllocatorCore::require_initial_game_class_counter_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source.get()==&source&&!source.factory_self_.expired()&&
  !source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),"Init Game class canonical TRAIN Source differs");
 require_initial_game_class_counter_without_callbacks();require_initial_name_game_counter_prefix(source);
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(prefix.initial_game_class_counter_returned&&prefix.initial_unit_class==p->initial_game_class->unit_class&&
  prefix.initial_game_class_before==p->initial_game_class->before&&prefix.initial_game_class_after==p->initial_game_class->after&&
  (prefix.boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||(prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned)))&&prefix.initial_unit_info_promotion_prefix_stored))&&prefix.initial_military_support_returned))&&prefix.initial_resource_nuke_zero_returned))&&prefix.initial_support_zero_returned))&&prefix.initial_player_class_counter_returned))&&prefix.initial_team_class_counter_returned)),"Init Game class actual returned prefix differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_game_class_counter_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_name_game&&p->initial_name_game->stored&&!p->initial_game_class&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_unit_class_count_owner,
  "Init Game class requires genuine returned name/count once predecessor");
 try{
  p->check();require_initial_name_game_counter_prefix(*p->train_source);p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialGameClassPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.type=q.node->scalars.at("m_eUnitType");
  // Existing actual complete Unit/Class ID+hash projection; no new formula or
  // caller UnitType/UnitClass identity is introduced for this Init path.
  const auto rules=load_bnw_unit_class_death_rules(p->host->database(),p->host->types());const auto binding=rules.units.find(q.type);
  need(binding!=rules.units.end()&&binding->second.unit_class>=0&&static_cast<std::uint64_t>(binding->second.unit_class)<rules.class_slots&&
   rules.class_slots==rules.class_hashes.size(),"Init Game actual Unit/Class binding absent");
  q.unit_class=narrow(binding->second.unit_class);q.hash=binding->second.class_hash;
  auto&arrays=p->state->world.progression.arrays;
  for(std::size_t i=0;i<arrays.size();++i)if(name(arrays[i].name)=="unitclass_created_count"){need(!q.array,"Duplicate actual Game class counter table");q.array=&arrays[i];q.array_index=i;}
  need(p->state->world.progression.layout==bnw&&q.array&&q.array->columns==1&&q.array->rows.size()==rules.class_slots,"Init Game actual UnitClass counter shape differs");
  std::set<std::uint64_t>hashes;std::size_t null_rows=0;
  for(std::size_t i=0;i<q.array->rows.size();++i){auto&r=q.array->rows[i];need(r.type_hash.has_value(),"Init Game UnitClass native hashed row has no hash");
   if(*r.type_hash==0){need(r.values.empty(),"Init Game NULL UnitClass row has illegal payload");q.rows.push_back({0,std::nullopt});++null_rows;continue;}
   need(*r.type_hash<=UINT32_MAX&&r.values.size()==1&&hashes.insert(*r.type_hash).second&&std::find(rules.class_hashes.begin(),rules.class_hashes.end(),*r.type_hash)!=rules.class_hashes.end(),
   "Init Game UnitClass unknown/duplicate real hash requires actual Reset/Read owner");
   const auto value=narrow(r.values[0]);need(value>=0,"Init Game negative UnitClass created count");q.rows.push_back({*r.type_hash,value});
   if(*r.type_hash==q.hash){need(!q.row,"Init Game duplicate selected UnitClass counter");q.row=&r;q.row_index=i;q.before=value;}
  }
  const auto expected_nulls=static_cast<std::size_t>(std::count(rules.class_hashes.begin(),rules.class_hashes.end(),std::uint32_t{0}));
  need(null_rows==expected_nulls&&hashes.size()==rules.class_slots-expected_nulls,"Init Game UnitClass full native Info slot universe differs");
  need(q.hash!=0&&q.row&&q.before<INT32_MAX,"Init Game actual selected UnitClass counter missing/overflow");q.after=q.before+1;
  q.before_revision=initial_best_land_prior_world_revision_without_callbacks();need(q.before_revision==p->initial_name_game->after_revision&&q.before_revision<UINT64_MAX,"Init Game class actual prior revision differs/exhausted");q.after_revision=q.before_revision+1;
  p->initial_game_class.emplace(std::move(q));auto&stage=*p->initial_game_class;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_name_game_counter_prefix(*p->train_source);
  need(stage.node.get()==p->bank->pending_units.at(p->bank->pending_slot).get()&&stage.node->scalars.at("m_eUnitType")==stage.type&&
   initial_best_land_prior_world_revision_without_callbacks()==stage.before_revision&&&arrays[stage.array_index]==stage.array&&stage.array->rows.size()==stage.rows.size()&&
   &stage.array->rows[stage.row_index]==stage.row,"Init Game class actual operands changed during preparation");
  for(std::size_t i=0;i<stage.rows.size();++i){const auto&r=stage.array->rows[i];need(r.type_hash==std::optional<std::uint64_t>{stage.rows[i].hash}&&
   (stage.rows[i].hash==0?(!stage.rows[i].value&&r.values.empty()):stage.rows[i].value&&r.values.size()==1&&r.values[0]==*stage.rows[i].value),"Init Game class counter changed before write");}
  need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==stage.before_revision&&!p->bank->retired.load(),"Init Game class final source bookend differs");
  stage.row->values[0]=stage.after;p->state->ui_source_revision=stage.after_revision;stage.stored=true;
  p->initial_set_xy->record_initial_game_class_counter_return(stage.unit_class,stage.before,stage.after,stage.after_revision);
  p->train_source->accept_actual_initial_game_class_counter_prefix(*canonical);
  require_initial_game_class_counter_prefix(*p->train_source);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_team_class_counter_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_team_class&&p->initial_team_class->stored,"Init Team class actual stored receipt absent");
 const auto&q=*p->initial_team_class;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&
  p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&p->bank&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  initial_player_class_prior_revision_without_callbacks()==q.after_revision&&q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1&&
  p->initial_game_class&&p->initial_game_class->stored&&p->initial_game_class->after_revision==q.before_revision,
  "Init Team class genuine source/earned revision changed");
 need(q.team_id>=0&&q.team_id<64&&p->state->world.teams&&p->state->world.teams->layout==bnw&&p->state->world.teams->teams.size()==64&&
  &p->state->world.teams->teams[static_cast<std::size_t>(q.team_id)]==q.team&&q.team->index==static_cast<std::uint64_t>(q.team_id)&&
  post_named(q.team->fields,"id").value==q.team_id&&post_player_team(*p->state,p->actor)==q.team_id,
  "Init Team class actual player/team binding changed");
 need(p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot))&&
  q.node->scalars.at("m_eUnitType")==q.type&&p->initial_game_class->unit_class==q.unit_class&&p->initial_game_class->hash==q.hash,
  "Init Team class actual SAME node/selected class changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(q.array_index<q.team->hashes.size()&&&q.team->hashes[q.array_index]==q.array&&name(q.array->name)=="unit_class_count"&&
  q.array->wire_count>=0&&static_cast<std::uint64_t>(q.array->wire_count)==q.rows.size()&&q.array->rows.size()==q.rows.size()&&
  q.row_index<q.rows.size()&&&q.array->rows[q.row_index]==q.row,"Init Team native UnitClass array/storage changed");
 for(std::size_t i=0;i<q.rows.size();++i){const auto&r=q.array->rows[i];need(r.type_hash==q.rows[i].hash&&
  (q.rows[i].hash==0?(!q.rows[i].value&&r.values.empty()):q.rows[i].value&&r.values.size()==1&&r.values[0]==(i==q.row_index?q.after:*q.rows[i].value)),"Init Team actual complete UnitClass count table changed");}
 need(q.row->type_hash==q.hash&&q.before>=0&&q.before<INT32_MAX&&q.after==q.before+1,"Init Team native checked +1 differs");
 const auto*registry=p->state->setup.extensions.find<Registry>();need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor).get()==p->bank.get()&&
  !registry->players.at(p->actor).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(p->actor)),"Init Team class bank authority replaced");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  initial_player_class_prior_revision_without_callbacks()==q.after_revision&&!p->bank->retired.load(),"Init Team class final source bookend differs");
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_team_class_prior_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init Team class lineage before source borrow");
 if(p->initial_team_class&&p->initial_team_class->stored){require_initial_team_class_counter_without_callbacks();return p->initial_team_class->before_revision;}
 return initial_best_land_prior_world_revision_without_callbacks();
}
void BnwNativeUnitAllocatorCore::require_initial_team_class_counter_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source.get()==&source&&!source.factory_self_.expired()&&
  !source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),"Init Team class canonical TRAIN Source differs");
 require_initial_team_class_counter_without_callbacks();require_initial_game_class_counter_prefix(source);
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_team_class_counter_returned&&
  prefix.initial_unit_class_team==p->initial_team_class->team_id&&prefix.initial_team_class_before==p->initial_team_class->before&&
  prefix.initial_team_class_after==p->initial_team_class->after&&(prefix.boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||(prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned)))&&prefix.initial_unit_info_promotion_prefix_stored))&&prefix.initial_military_support_returned))&&prefix.initial_resource_nuke_zero_returned))&&prefix.initial_support_zero_returned))&&prefix.initial_player_class_counter_returned)),
  "Init Team class actual earned return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_team_class_counter_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_game_class&&p->initial_game_class->stored&&!p->initial_team_class&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_team_unit_class_count_owner,
  "Init Team class requires genuine returned Game class once predecessor");
 try{
  p->check();require_initial_game_class_counter_prefix(*p->train_source);p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialTeamClassPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.type=q.node->scalars.at("m_eUnitType");
  const auto rules=load_bnw_unit_class_death_rules(p->host->database(),p->host->types());const auto binding=rules.units.find(q.type);
  need(binding!=rules.units.end()&&binding->second.unit_class==p->initial_game_class->unit_class&&binding->second.class_hash==p->initial_game_class->hash&&
   binding->second.unit_class>=0&&static_cast<std::uint64_t>(binding->second.unit_class)<rules.class_slots,"Init Team actual Unit/Class binding differs");
  q.unit_class=narrow(binding->second.unit_class);q.hash=binding->second.class_hash;q.team_id=post_player_team(*p->state,p->actor);
  need(q.team_id>=0&&q.team_id<64&&p->state->world.teams&&p->state->world.teams->layout==bnw&&p->state->world.teams->teams.size()==64,
   "Init Team actual complete BNW team roster absent");q.team=&p->state->world.teams->teams.at(static_cast<std::size_t>(q.team_id));
  need(q.team->index==static_cast<std::uint64_t>(q.team_id)&&post_named(q.team->fields,"id").value==q.team_id,"Init Team actual ID/storage differs");
  for(std::size_t i=0;i<q.team->hashes.size();++i)if(name(q.team->hashes[i].name)=="unit_class_count"){need(!q.array,"Duplicate actual Team UnitClass count array");q.array=&q.team->hashes[i];q.array_index=i;}
  need(q.array&&q.array->wire_count>=0&&static_cast<std::uint64_t>(q.array->wire_count)==rules.class_slots&&q.array->rows.size()==rules.class_slots,
   "Init Team actual UnitClass count shape differs");std::set<std::uint64_t>hashes;std::size_t null_rows=0;
  for(std::size_t i=0;i<q.array->rows.size();++i){auto&r=q.array->rows[i];
   if(r.type_hash==0){need(r.values.empty(),"Init Team NULL UnitClass hash has illegal payload");q.rows.push_back({0,std::nullopt});++null_rows;continue;}
   need(r.type_hash<=UINT32_MAX&&r.values.size()==1&&hashes.insert(r.type_hash).second&&std::find(rules.class_hashes.begin(),rules.class_hashes.end(),r.type_hash)!=rules.class_hashes.end(),
   "Init Team unknown/duplicate real UnitClass hash requires actual Reset/Read owner");
   const auto value=narrow(r.values[0]);need(value>=0,"Init Team negative native UnitClass count");q.rows.push_back({r.type_hash,value});
   if(r.type_hash==q.hash){need(!q.row,"Duplicate actual selected Team UnitClass counter");q.row=&r;q.row_index=i;q.before=value;}}
  const auto expected_nulls=static_cast<std::size_t>(std::count(rules.class_hashes.begin(),rules.class_hashes.end(),std::uint32_t{0}));
  need(null_rows==expected_nulls&&hashes.size()==rules.class_slots-expected_nulls,"Init Team UnitClass full native Info slot universe differs");
  need(q.hash!=0&&q.row&&q.before<INT32_MAX,"Init Team actual selected counter missing/overflow");q.after=q.before+1;
  q.before_revision=initial_best_land_prior_world_revision_without_callbacks();need(q.before_revision==p->initial_game_class->after_revision&&q.before_revision<UINT64_MAX,"Init Team actual predecessor revision differs/exhausted");q.after_revision=q.before_revision+1;
  p->initial_team_class.emplace(std::move(q));auto&stage=*p->initial_team_class;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_game_class_counter_prefix(*p->train_source);
  need(post_player_team(*p->state,p->actor)==stage.team_id&&stage.team_id>=0&&stage.team_id<64&&p->state->world.teams&&
   p->state->world.teams->layout==bnw&&p->state->world.teams->teams.size()==64&&
   &p->state->world.teams->teams[static_cast<std::size_t>(stage.team_id)]==stage.team&&
   stage.array_index<stage.team->hashes.size()&&&stage.team->hashes[stage.array_index]==stage.array&&
   name(stage.array->name)=="unit_class_count"&&stage.array->wire_count>=0&&static_cast<std::uint64_t>(stage.array->wire_count)==stage.rows.size()&&
   stage.array->rows.size()==stage.rows.size()&&stage.row_index<stage.rows.size()&&&stage.array->rows[stage.row_index]==stage.row,
   "Init Team actual source row/storage changed during preparation");
  for(std::size_t i=0;i<stage.rows.size();++i){const auto&r=stage.array->rows[i];need(r.type_hash==stage.rows[i].hash&&
   (stage.rows[i].hash==0?(!stage.rows[i].value&&r.values.empty()):stage.rows[i].value&&r.values.size()==1&&r.values[0]==*stage.rows[i].value),"Init Team counts changed before write");}
  need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==stage.before_revision&&!p->bank->retired.load(),"Init Team class final source bookend differs");
  stage.row->values[0]=stage.after;p->state->ui_source_revision=stage.after_revision;stage.stored=true;
  p->initial_set_xy->record_initial_team_class_counter_return(stage.team_id,stage.before,stage.after,stage.after_revision);
  p->train_source->accept_actual_initial_team_class_counter_prefix(*canonical);
  require_initial_team_class_counter_prefix(*p->train_source);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_player_class_counter_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_player_class&&p->initial_player_class->stored,"Init Player class actual stored receipt absent");
 const auto&q=*p->initial_player_class;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&
  p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&p->bank&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  initial_military_prior_revision_without_callbacks()==q.after_revision&&q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1&&
  p->initial_team_class&&p->initial_team_class->stored&&p->initial_team_class->after_revision==q.before_revision,
  "Init Player class genuine source/earned revision changed");
 need(q.actor==p->actor&&q.actor>=0&&q.actor<64&&p->state->world.all_players&&p->state->world.all_players->layout==bnw&&
  p->state->world.all_players->players.size()==64&&&p->state->world.all_players->players[static_cast<std::size_t>(q.actor)]==q.player&&
  q.player->index==static_cast<std::uint64_t>(q.actor)&&q.player->saved_player_id==q.actor&&q.player->core.layout==bnw&&
  q.field_index<q.player->core.fields.size()&&&q.player->core.fields[q.field_index]==q.field,
  "Init Player class actual player/native storage changed");
 need(name(q.field->name)=="m_paiUnitClassCount"&&q.field->encoding==saves::PlayerCoreEncoding::vector_i32&&
  q.field->wire_count==q.after_values.size()&&q.field->bytes.empty()&&q.field->hashes.empty()&&q.field->row_counts.empty()&&
  q.field->values==q.after_values&&q.before_values.size()==q.after_values.size()&&q.unit_class>=0&&
  static_cast<std::size_t>(q.unit_class)<q.after_values.size()&&q.before>=0&&q.before<INT32_MAX&&q.after==q.before+1&&
  q.before_values[static_cast<std::size_t>(q.unit_class)]==q.before&&q.after_values[static_cast<std::size_t>(q.unit_class)]==q.after,
  "Init Player exact complete class vector/checked delta differs");
 for(std::size_t i=0;i<q.before_values.size();++i)need(i==static_cast<std::size_t>(q.unit_class)||q.before_values[i]==q.after_values[i],"Init Player class extra count changed");
 need(p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot))&&
  q.node->scalars.at("m_eUnitType")==q.type&&p->initial_team_class->unit_class==q.unit_class&&p->initial_team_class->hash==q.hash,
  "Init Player class SAME node/actual class changed");check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 const auto*registry=p->state->setup.extensions.find<Registry>();need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor).get()==p->bank.get()&&
  !registry->players.at(p->actor).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(p->actor)),"Init Player bank authority replaced");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  initial_military_prior_revision_without_callbacks()==q.after_revision&&!p->bank->retired.load(),"Init Player class final source bookend differs");
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_player_class_prior_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init Player class lineage before live borrow");
 if(p->initial_player_class&&p->initial_player_class->stored){require_initial_player_class_counter_without_callbacks();return p->initial_player_class->before_revision;}
 return initial_best_land_prior_world_revision_without_callbacks();
}
const std::vector<std::uint64_t>&BnwNativeUnitAllocatorCore::initial_current_happiness_finish_inputs_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_happiness,"Init native happiness receipt absent");
 if(p->initial_best_land&&p->initial_best_land->stored){(void)initial_best_land_prior_world_revision_without_callbacks();return p->initial_best_land->happiness_inputs;}
 if(p->initial_military_support&&p->initial_military_support->stored&&p->initial_military_support->incremented){require_initial_military_support_without_callbacks();return p->initial_military_support->happiness_inputs;}
 if(p->initial_player_class&&p->initial_player_class->stored){require_initial_player_class_counter_without_callbacks();return p->initial_player_class->happiness_inputs;}
 return p->initial_happiness->full_inputs;
}
void BnwNativeUnitAllocatorCore::require_initial_player_class_counter_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source.get()==&source&&!source.factory_self_.expired()&&
  !source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),"Init Player class canonical TRAIN Source differs");
 require_initial_player_class_counter_without_callbacks();require_initial_team_class_counter_prefix(source);
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_player_class_counter_returned&&
  prefix.initial_player_class_before==p->initial_player_class->before&&prefix.initial_player_class_after==p->initial_player_class->after&&
  (prefix.boundary==BnwUnitMoveBoundary::initial_num_builders_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||(prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned)))&&prefix.initial_unit_info_promotion_prefix_stored))&&prefix.initial_military_support_returned))&&prefix.initial_resource_nuke_zero_returned))&&prefix.initial_support_zero_returned)),"Init Player class actual return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_player_class_counter_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_team_class&&p->initial_team_class->stored&&!p->initial_player_class&&
  p->initial_happiness&&p->initial_happiness->full_returned&&p->initial_city_strength&&p->initial_city_strength->stored&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_player_unit_class_count_owner,
  "Init Player class requires genuine returned Team class predecessor once");
 try{
  p->check();require_initial_team_class_counter_prefix(*p->train_source);p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialPlayerClassPrefix q;q.actor=p->actor;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.type=q.node->scalars.at("m_eUnitType");
  const auto rules=load_bnw_unit_class_death_rules(p->host->database(),p->host->types());const auto binding=rules.units.find(q.type);
  need(binding!=rules.units.end()&&binding->second.unit_class==p->initial_team_class->unit_class&&binding->second.class_hash==p->initial_team_class->hash&&
   binding->second.unit_class>=0&&static_cast<std::uint64_t>(binding->second.unit_class)<rules.class_slots,"Init Player actual Unit/Class binding differs");
  q.unit_class=narrow(binding->second.unit_class);q.hash=binding->second.class_hash;
  need(p->state->world.all_players&&p->state->world.all_players->layout==bnw&&p->state->world.all_players->players.size()==64,
   "Init Player actual complete player roster absent");q.player=&p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
  need(q.player->index==static_cast<std::uint64_t>(p->actor)&&q.player->saved_player_id==p->actor&&q.player->core.layout==bnw,"Init Player native ID/layout differs");
  for(std::size_t i=0;i<q.player->core.fields.size();++i)if(name(q.player->core.fields[i].name)=="m_paiUnitClassCount"){
   need(!q.field,"Duplicate actual Player UnitClass count vector");q.field=&q.player->core.fields[i];q.field_index=i;}
  need(q.field&&q.field->encoding==saves::PlayerCoreEncoding::vector_i32&&q.field->wire_count==rules.class_slots&&
   q.field->values.size()==rules.class_slots&&q.field->bytes.empty()&&q.field->hashes.empty()&&q.field->row_counts.empty(),
   "Init Player UnitClass indexed count requires exact native Reset/Read binding");
  q.before_values=q.field->values;q.after_values=q.before_values;for(auto n:q.before_values)need(n>=0&&n<=INT32_MAX,"Init Player invalid signed class count");
  q.before=narrow(q.before_values.at(static_cast<std::size_t>(q.unit_class)));need(q.before<INT32_MAX,"Init Player native class count overflow");q.after=q.before+1;
  q.after_values[static_cast<std::size_t>(q.unit_class)]=q.after;
  q.before_revision=initial_best_land_prior_world_revision_without_callbacks();need(q.before_revision==p->initial_team_class->after_revision&&q.before_revision<UINT64_MAX,"Init Player predecessor revision differs/exhausted");q.after_revision=q.before_revision+1;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_player_class_inputs(*canonical,*p->initial_city_strength,*q.field,q.after_values);
  std::size_t count=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto){need(count<SIZE_MAX,"Init Player descendant extent overflow");++count;},{},q.field,&q.after_values);
  q.happiness_inputs.resize(count);std::size_t at=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto n){need(at<q.happiness_inputs.size(),"Init Player descendant extent changed");q.happiness_inputs[at++]=static_cast<std::uint64_t>(n);},{},q.field,&q.after_values);
  need(at==count,"Init Player descendant seal extent differs");p->initial_player_class.emplace(std::move(q));auto&stage=*p->initial_player_class;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_team_class_counter_prefix(*p->train_source);
  need(p->state->world.all_players&&p->state->world.all_players->layout==bnw&&p->state->world.all_players->players.size()==64&&
   &p->state->world.all_players->players[static_cast<std::size_t>(p->actor)]==stage.player&&stage.field_index<stage.player->core.fields.size()&&
   &stage.player->core.fields[stage.field_index]==stage.field&&stage.field->values==stage.before_values&&stage.field->encoding==saves::PlayerCoreEncoding::vector_i32&&
   stage.field->wire_count==stage.before_values.size(),"Init Player complete raw class vector changed before store");
  initial_happiness_finish_match(*p->state,p->actor,p->initial_happiness->full_inputs);
  // Verify the projected whole reached operand seal without allocation, AFTER
  // all source preparation. No callback or allocation separates these gates
  // and the actual native count/revision/cache-receipt stores.
  std::size_t after_at=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto n){need(after_at<stage.happiness_inputs.size()&&stage.happiness_inputs[after_at++]==static_cast<std::uint64_t>(n),"Init Player projected happiness operands changed");},{},stage.field,&stage.after_values);
  need(after_at==stage.happiness_inputs.size()&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation&&&p->host->session().source()==p->source&&
   initial_best_land_prior_world_revision_without_callbacks()==stage.before_revision&&!p->bank->retired.load(),"Init Player final source bookend differs");
  stage.field->values[static_cast<std::size_t>(stage.unit_class)]=stage.after;p->state->ui_source_revision=stage.after_revision;stage.stored=true;
  p->initial_city_strength->player_class_inputs.swap(city);p->initial_city_strength->player_class_stored=true;
  p->initial_set_xy->record_initial_player_class_counter_return(stage.before,stage.after,stage.after_revision);
  p->train_source->accept_actual_initial_player_class_counter_prefix(*canonical);
  require_initial_player_class_counter_prefix(*p->train_source);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_support_zero_prefix_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source&&p->initial_set_xy&&p->initial_support_zero&&p->initial_support_zero->stored,
  "Initial support zero native receipt absent");const auto&q=*p->initial_support_zero;
 require_initial_player_class_counter_prefix(*p->train_source);
 need(q.node&&p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot))&&
  q.node->scalars.at("m_eUnitType")==q.rule.unit_type&&q.unit_class==p->initial_player_class->unit_class&&q.class_hash==p->initial_player_class->hash&&
  q.rule.work_rate>=0&&!(q.rule.work_rate>0&&q.rule.land)&&q.rule.unhappiness==0&&q.rule.extra_maintenance_cost==0&&
  q.rule.unit_hash>0&&initial_military_prior_revision_without_callbacks()==q.revision&&q.revision==p->initial_player_class->after_revision,
  "Initial support actual no-effect operands/source changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(prefix.initial_support_zero_returned&&(prefix.boundary==BnwUnitMoveBoundary::initial_resource_used_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||(prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned)))&&prefix.initial_unit_info_promotion_prefix_stored))&&prefix.initial_military_support_returned))&&prefix.initial_resource_nuke_zero_returned)),
  "Initial support source-order earned boundary differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_support_zero_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_player_class&&p->initial_player_class->stored&&!p->initial_support_zero&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_num_builders_owner,
  "Initial support requires genuine returned Player class predecessor once");
 try{
  p->check();require_initial_player_class_counter_prefix(*p->train_source);p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  const auto classes=load_bnw_unit_class_death_rules(p->host->database(),p->host->types());
  const auto support=load_bnw_unit_support_rules(p->host->database(),p->host->types(),classes);
  InitialSupportZeroPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);const auto type=q.node->scalars.at("m_eUnitType");
  const auto u=classes.units.find(type);const auto r=support.units.find(type);
  need(u!=classes.units.end()&&r!=support.units.end()&&u->second.unit_class==p->initial_player_class->unit_class&&
   u->second.class_hash==p->initial_player_class->hash&&r->second.unit_type==type&&r->second.unit_hash==u->second.unit_hash,
   "Initial support selected actual Unit/Class/source differs");
  q.rule=r->second;q.unit_class=narrow(u->second.unit_class);q.class_hash=u->second.class_hash;q.revision=initial_best_land_prior_world_revision_without_callbacks();
  // SDK order: WorkRate>0 && LAND then ChangeNumBuilders(1); nonzero
  // Unhappiness then ChangeUnhappinessFromUnits; changeExtraUnitCost(0)
  // has no body. These proven zero branches read NO player counter, run
  // NO UI callback and consume NO RNG. A real nonzero operand refuses at
  // its source position BEFORE any native counter write or phase publication.
  need(!(q.rule.work_rate>0&&q.rule.land),"Initial actual builder increment requires native counter and derived city/happiness receipt continuation");
  need(q.rule.unhappiness==0,"Initial actual unit unhappiness update requires paired getter/currentness and happiness continuation");
  need(q.rule.extra_maintenance_cost==0,"Initial actual extra-unit-cost update requires native counter/maintenance and GameData receiver continuation");
  p->initial_support_zero.emplace(std::move(q));auto&stage=*p->initial_support_zero;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_player_class_counter_prefix(*p->train_source);
  require_initial_setxy_tail_without_callbacks();
  need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&!p->bank->retired.load(),
   "Initial support exact no-effect source bookend changed");
  stage.stored=true;p->initial_set_xy->record_initial_support_zero_return();
  require_initial_support_zero_prefix_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_resource_nuke_zero_prefix_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source&&p->initial_set_xy&&p->initial_resource_nuke_zero&&p->initial_resource_nuke_zero->stored,
  "Initial resources/nuke genuine native receipt absent");const auto&q=*p->initial_resource_nuke_zero;
 require_initial_support_zero_prefix_without_callbacks();
 need(q.node&&p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot))&&
  q.node->scalars.at("m_eUnitType")==p->initial_support_zero->rule.unit_type&&p->initial_support_zero->rule.nuke_damage_level==-1&&
  q.revision==p->initial_support_zero->revision&&q.revision==initial_military_prior_revision_without_callbacks(),
  "Initial resource/nuke actual no-effect unit/source changed");
 for(std::size_t i=0;i<q.resources.size();++i)need(q.resources[i].id>=0&&static_cast<std::size_t>(q.resources[i].id)==i&&
  q.resources[i].hash>0&&!q.resources[i].type.empty()&&q.resources[i].cost<=0,"Initial native ResourceInfo array/zero-effect loop changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_resource_nuke_zero_returned&&
  prefix.initial_resource_slots==q.resources.size()&&(prefix.boundary==BnwUnitMoveBoundary::initial_military_support_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||(prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned)))&&prefix.initial_unit_info_promotion_prefix_stored))&&prefix.initial_military_support_returned)),
  "Initial resource/nuke actual returned phase differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_resource_nuke_zero_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_support_zero&&p->initial_support_zero->stored&&!p->initial_resource_nuke_zero&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_resource_used_owner,
  "Initial resource/nuke requires actual support interval once predecessor");
 try{
  p->check();require_initial_support_zero_prefix_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialResourceNukeZeroPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  const auto type=q.node->scalars.at("m_eUnitType");const content::InfoTypeEntry*actual_unit=nullptr;
  for(const auto&[key,entry]:p->host->types().entries())if(entry.table=="Units"&&entry.id==type){need(!actual_unit,"Duplicate actual native UnitInfo ID");actual_unit=&entry;}
  need(actual_unit&&actual_unit->hash==p->initial_support_zero->rule.unit_hash,"Initial resource actual UnitInfo binding changed");
  // Native CvDatabaseUtility::PopulateArrayByValue(Resources,...,Cost)
  // InitializeArray(...,0), then actual selected-unit relation values by ID.
  // This is an owned genuine UnitInfo resource cache, not guessed live counts.
  const auto report=p->host->database().prefetch_info("Resources",[&](std::uint64_t slot,std::int64_t id,const auto&row){
   const auto key=row.text("Type");const auto*info=key?p->host->types().find(*key):nullptr;
   need(id>=0&&static_cast<std::uint64_t>(id)==slot&&info&&info->table=="Resources"&&info->id==id&&
    info->hash>0&&info->hash==content::saved_info_type_hash(*key)&&p->host->types().find_hash(info->hash)&&
    p->host->types().find_hash(info->hash)->type==*key,"Initial complete native ResourceInfo Type/ID/hash binding absent");
   need(slot==q.resources.size(),"Initial ResourceInfo dense slot order changed");q.resources.push_back({narrow(id),0,info->hash,*key});return true;
  });need(report.all_succeeded()&&report.overlapping_ids==0,"Initial complete Resources cache load failed");
  std::size_t catalog_resources=0;for(const auto&[key,entry]:p->host->types().entries())if(entry.table=="Resources"){
   ++catalog_resources;need(entry.id>=0&&static_cast<std::size_t>(entry.id)<q.resources.size()&&q.resources[static_cast<std::size_t>(entry.id)].type==key&&
    q.resources[static_cast<std::size_t>(entry.id)].hash==entry.hash,"Initial Resources catalog universe changed");}
  need(catalog_resources==q.resources.size(),"Initial complete Resources catalog/cache shape differs");std::set<std::int32_t>seen;
  const auto relations=p->host->database().query_rows({"SELECT ResourceType,Cost FROM Unit_ResourceQuantityRequirements WHERE UnitType=?",{actual_unit->type}},[&](const auto&row){
   const auto key=row.text("ResourceType");const auto*info=key?p->host->types().find(*key):nullptr;
   need(info&&info->table=="Resources"&&info->id>=0&&static_cast<std::size_t>(info->id)<q.resources.size()&&
    q.resources[static_cast<std::size_t>(info->id)].hash==info->hash&&seen.insert(narrow(info->id)).second,
    "Initial Resource requirements unknown/duplicate selected Info binding");
   q.resources[static_cast<std::size_t>(info->id)].cost=narrow(row.integer("Cost"));
  });need(relations.all_succeeded(),"Initial Resource requirements native query failed");
  // EXACT native ascending ResourceInfo loop. Only Cost>0 calls real
  // changeNumResourceUsed/notification/GameData; preserve this boundary.
  for(const auto&r:q.resources)need(r.cost<=0,"Initial positive Resource requirement needs real count/notification/GameData owner");
  need(p->initial_support_zero->rule.nuke_damage_level==-1,"Initial actual nuke count increment requires native counter continuation");
  p->initial_resource_nuke_zero.emplace(std::move(q));auto&stage=*p->initial_resource_nuke_zero;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_support_zero_prefix_without_callbacks();
  need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&!p->bank->retired.load(),
   "Initial Resources/nuke exact no-effect final source differs");
  stage.stored=true;p->initial_set_xy->record_initial_resource_nuke_zero_return(stage.resources.size());
  require_initial_resource_nuke_zero_prefix_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_military_support_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source&&p->initial_set_xy&&p->initial_military_support&&p->initial_military_support->stored,
  "Initial military support genuine native receipt absent");const auto&q=*p->initial_military_support;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&
  p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&p->bank&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  initial_best_land_prior_world_revision_without_callbacks()==q.after_revision&&p->initial_resource_nuke_zero&&p->initial_resource_nuke_zero->stored&&
  p->initial_support_zero&&p->initial_support_zero->stored&&q.before_revision==p->initial_resource_nuke_zero->revision&&
  q.incremented==p->initial_support_zero->rule.military_support&&
  ((!q.incremented&&q.after_revision==q.before_revision)||(q.incremented&&q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1)),
  "Initial military support actual source/revision/UnitInfo changed");
 need(q.node&&p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot))&&
  q.node->scalars.at("m_eUnitType")==p->initial_support_zero->rule.unit_type,"Initial military SAME native node changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 if(q.incremented){need(p->state->world.all_players&&p->state->world.all_players->players.size()==64&&p->actor>=0&&p->actor<64&&
   &p->state->world.all_players->players[static_cast<std::size_t>(p->actor)]==q.player&&q.player->saved_player_id==p->actor&&q.player->core.layout==bnw&&
   q.field_index<q.player->core.fields.size()&&&q.player->core.fields[q.field_index]==q.field,
   "Initial military actual player scalar storage changed");
  need(name(q.field->name)=="m_iNumMilitaryUnits"&&q.field->encoding==saves::PlayerCoreEncoding::i32&&!q.field->wire_count&&
   q.field->values==q.after_value&&q.after_value.size()==1&&q.field->bytes.empty()&&q.field->hashes.empty()&&q.field->row_counts.empty()&&
   q.before>=0&&q.before<INT32_MAX&&q.after==q.before+1&&q.after_value[0]==q.after,
   "Initial military native count/checked +1 changed");
  need(q.active==(detail::bnw_game_active_player_read(*p->state)==p->actor)&&q.game_data_returned,
   "Initial military actual active-player/UI return changed");
  if(q.active){need(q.game_data_before<UINT64_MAX&&q.game_data_after==q.game_data_before+1&&q.input_before_revision<UINT64_MAX&&
    q.input_after_revision==q.input_before_revision+1&&q.input&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_after_revision,
    "Initial military GameData once/client revision receipt differs");
   p->initial_set_xy->require_initial_military_game_data_identity_without_callbacks(q.input);}
 }else need(!q.field&&!q.input&&!q.active&&q.game_data_before==0&&q.game_data_after==0,"Initial military no-effect branch has fabricated scalar/UI effect");
 const auto*registry=p->state->setup.extensions.find<Registry>();need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor).get()==p->bank.get()&&
  !registry->players.at(p->actor).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(p->actor)),"Initial military genuine Bank replaced");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&
  initial_best_land_prior_world_revision_without_callbacks()==q.after_revision&&!p->bank->retired.load(),"Initial military final source bookend differs");
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_military_prior_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign initial military lineage before source borrow");
 if(p->initial_military_support&&p->initial_military_support->stored){require_initial_military_support_without_callbacks();return p->initial_military_support->before_revision;}
 return initial_best_land_prior_world_revision_without_callbacks();
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_center_earned_input_revision_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&input)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&input,"Initial center actual retained input absent");
 if(p->initial_military_support&&p->initial_military_support->stored&&p->initial_military_support->incremented&&p->initial_military_support->active){
  require_initial_military_support_without_callbacks();const auto&q=*p->initial_military_support;
  need(input.get()==q.input.get()&&!input.owner_before(q.input)&&!q.input.owner_before(input),"Initial center military setter came from different genuine input");
  return q.input_before_revision;
 }
 return initial_moves_prior_input_revision_without_callbacks(input);
}
void BnwNativeUnitAllocatorCore::require_initial_military_support_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->train_source.get()==&source&&!source.factory_self_.expired()&&
  !source.factory_self_.owner_before(p->train_source)&&!p->train_source.owner_before(source.factory_self_),"Initial military canonical TRAIN Source differs");
 require_initial_military_support_without_callbacks();require_initial_resource_nuke_zero_prefix_without_callbacks();
 need(p->initial_military_support->incremented,"Initial military no-effect branch has no world revision ACK");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_military_support_returned&&prefix.initial_military_support_incremented&&
  prefix.initial_military_count_before==p->initial_military_support->before&&prefix.initial_military_count_after==p->initial_military_support->after&&
  prefix.initial_military_game_data_occurrence==p->initial_military_support->game_data_after&&(prefix.boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner||(prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned)))&&prefix.initial_unit_info_promotion_prefix_stored)),
  "Initial military actual earned return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_military_support_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_resource_nuke_zero&&p->initial_resource_nuke_zero->stored&&!p->initial_military_support&&
  p->initial_city_strength&&p->initial_happiness&&p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_military_support_owner,
  "Initial military requires genuine Resources/nuke predecessor once");
 try{
  p->check();require_initial_resource_nuke_zero_prefix_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialMilitarySupportPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.incremented=p->initial_support_zero->rule.military_support;
  q.before_revision=initial_best_land_prior_world_revision_without_callbacks();q.after_revision=q.before_revision;std::vector<std::int64_t>city;
  if(q.incremented){need(q.before_revision<UINT64_MAX,"Initial military source revision exhausted");q.after_revision=q.before_revision+1;
   need(p->state->world.all_players&&p->state->world.all_players->players.size()==64&&p->actor>=0&&p->actor<64,"Initial military actual player roster absent");
   q.player=&p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
   for(std::size_t i=0;i<q.player->core.fields.size();++i)if(name(q.player->core.fields[i].name)=="m_iNumMilitaryUnits"){
    need(!q.field,"Duplicate actual military count field");q.field=&q.player->core.fields[i];q.field_index=i;}
   need(q.field&&q.field->encoding==saves::PlayerCoreEncoding::i32&&!q.field->wire_count&&q.field->values.size()==1&&
    q.field->bytes.empty()&&q.field->hashes.empty()&&q.field->row_counts.empty(),"Initial military actual native scalar unavailable");
   q.before=narrow(q.field->values[0]);need(q.before>=0&&q.before<INT32_MAX,"Initial military signed count overflow");q.after=q.before+1;q.after_value.push_back(q.after);
   q.active=detail::bnw_game_active_player_read(*p->state)==p->actor;
   if(q.active){q.input=p->initial_set_xy->retain_initial_military_game_data_input();q.input->preflight_game_data_dirty_setter();q.game_data_before=q.input->game_data_dirty_setter_count();
    q.input_before_revision=q.input->initial_center_revision_without_callbacks();
    need(q.game_data_before<UINT64_MAX&&q.input_before_revision<UINT64_MAX,"Initial military GameData occurrence/client revision exhausted");q.input_after_revision=q.input_before_revision+1;}
   city=BnwMinorCityStrengthScratch::prepare_initial_military_inputs(*canonical,*p->initial_city_strength,*q.field,q.after_value);
   std::size_t count=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto){need(count<SIZE_MAX,"Initial military descendant extent overflow");++count;},{},q.field,&q.after_value);
   q.happiness_inputs.resize(count);std::size_t at=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto n){need(at<q.happiness_inputs.size(),"Initial military descendant extent changed");q.happiness_inputs[at++]=static_cast<std::uint64_t>(n);},{},q.field,&q.after_value);
   need(at==count,"Initial military descendant extent differs");
  }
  p->initial_military_support.emplace(std::move(q));auto&stage=*p->initial_military_support;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_resource_nuke_zero_prefix_without_callbacks();require_initial_setxy_tail_without_callbacks();
  if(stage.incremented){
   if(stage.active){p->initial_set_xy->require_initial_military_game_data_identity_without_callbacks(stage.input);
    need(stage.input->game_data_dirty_setter_count()==stage.game_data_before,"Initial military genuine GameData occurrence changed before store");}
   need(p->state->world.all_players&&p->state->world.all_players->players.size()==64&&
    &p->state->world.all_players->players[static_cast<std::size_t>(p->actor)]==stage.player&&stage.field_index<stage.player->core.fields.size()&&
    &stage.player->core.fields[stage.field_index]==stage.field&&stage.field->values.size()==1&&stage.field->values[0]==stage.before&&
    stage.field->encoding==saves::PlayerCoreEncoding::i32&&!stage.field->wire_count,"Initial military actual counter changed during preparation");
   std::size_t at=0;initial_happiness_finish_semantics(*p->state,p->actor,[&](auto n){need(at<stage.happiness_inputs.size()&&stage.happiness_inputs[at++]==static_cast<std::uint64_t>(n),"Initial military projected native operands changed");},{},stage.field,&stage.after_value);
   need(at==stage.happiness_inputs.size()&&stage.active==(detail::bnw_game_active_player_read(*p->state)==p->actor),"Initial military native activePlayer/extent changed");
   if(stage.active)need(stage.input->initial_center_revision_without_callbacks()==stage.input_before_revision,"Initial military actual input revision changed before store");
  }
  need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==stage.before_revision&&!p->bank->retired.load(),"Initial military final source bookend differs");
  if(stage.incremented){stage.field->values[0]=stage.after;p->state->ui_source_revision=stage.after_revision;
   p->initial_city_strength->military_inputs.swap(city);p->initial_city_strength->military_stored=true;}
  stage.stored=true;p->initial_set_xy->record_initial_military_count_store(stage.incremented,stage.before,stage.after,stage.after_revision);
  if(stage.incremented&&stage.active){stage.input->set_game_data_dirty(ui::BnwClientGameDataDirtyKind::game_data);
   need(stage.input->game_data_dirty_setter_count()==stage.game_data_before+1&&stage.input->game_data_dirty()==std::optional<bool>{true},
    "Initial military genuine GameData flag/occurrence did not store");stage.game_data_after=stage.game_data_before+1;}
  stage.game_data_returned=true;p->initial_set_xy->record_initial_military_support_return(stage.game_data_after);
  require_initial_military_support_without_callbacks();require_initial_setxy_tail_without_callbacks();
  if(stage.incremented)p->train_source->accept_actual_initial_military_support_prefix(*canonical);
  return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}


bool BnwNativeUnitAllocatorCore::initial_name_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_name_game&&p->initial_name_game->stored,"Initial promotion name predecessor absent");
 const auto&q=*p->initial_name_game;
 if(p->initial_unit_info_promotion&&p->initial_unit_info_promotion->stored){require_initial_unit_info_promotion_without_callbacks();const auto&r=*p->initial_unit_info_promotion;return r.node.get()==q.node.get()&&r.before==q.node_after&&r.dirty_before==q.dirty_after;}
 return q.node->scalars==q.node_after&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after);
}
void BnwNativeUnitAllocatorCore::require_initial_unit_info_promotion_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_unit_info_promotion&&p->initial_unit_info_promotion->stored&&p->train_source&&p->initial_set_xy,
  "Initial UnitInfo promotion genuine stored interval absent");const auto&q=*p->initial_unit_info_promotion;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&
  p->bank&&!p->bank->retired.load()&&session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&
  !p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot)),"Initial promotion actual source/node/control changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(initial_unit_info_promotion_node_matches_without_callbacks()&&q.node->promotion_unit==q.node.get()&&
  !q.node->promotion_host.expired()&&!q.node->promotion_host.owner_before(p->host)&&!p->host.owner_before(q.node->promotion_host),"Initial promotion actual native stores/host changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(prefix.initial_unit_info_promotion_prefix_stored&&prefix.initial_unit_info_promotion_id==q.selected&&prefix.initial_unit_info_promotion_slots==q.rows.size()&&
  ((q.selected>=0&&prefix.boundary==BnwUnitMoveBoundary::initial_promotion_native_remainder_owner)||((prefix.boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned))&&prefix.initial_traits_promotions_returned))&&(q.selected==-1||(p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored&&prefix.initial_unit_info_promotions_returned)))),
  "Initial promotion native partial/empty-loop boundary changed");
 need(session.state_.get()==p->state.get()&&session.state_generation_==p->generation&&session.source_.get()==p->source&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&!p->bank->retired.load(),"Initial promotion final source bookend changed");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_unit_info_promotion_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_military_support&&p->initial_military_support->stored&&!p->initial_unit_info_promotion&&p->initial_city_strength&&
  p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_free_unit_promotions_owner,"Initial free promotion requires genuine military predecessor once");
 try{
  p->check();require_initial_military_support_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialUnitInfoPromotionPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  need(q.node->promotion_unit==q.node.get()&&!q.node->promotion_host.expired()&&!q.node->promotion_host.owner_before(p->host)&&!p->host.owner_before(q.node->promotion_host)&&q.node->promotion_bits.empty(),
   "Initial free promotion did not retain SAME Reset promotion storage");
  auto rules=initial_unit_info_promotion_rules(*p->host,*q.node);q.rows=std::move(rules.first);q.free=std::move(rules.second);
  for(std::size_t i=0;i<q.rows.size();++i){if(!q.free.contains(q.rows[i].id))continue;need(!q.rows[i].hover,"Initial free Hovering promotion mandatory preplacement effects missing");q.selected=q.rows[i].id;q.first_free=i;break;}
  q.before=q.node->scalars;q.after=q.before;q.dirty_before=q.node->dirty;q.dirty_after=q.dirty_before;q.bits_before=q.node->promotion_bits;q.bits_after=q.bits_before;
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  if(q.selected>=0){const auto&row=q.rows[q.first_free];
   // Native GetInfoTypeForString(NULL/empty,true) == NO_INVISIBLE. A genuine
   // nonempty type must use its sight/invisibility owner, never guessed -1.
   need((!row.invisible||row.invisible->empty())&&(!row.see_invisible||row.see_invisible->empty()),"Mandatory actual promotion invisibility/sight branch unavailable");
   need(staged.scalars.at("m_eInvisibleType")==-1&&staged.scalars.at("m_eSeeInvisibleType")==-1,"Initial free promotion current invisibility differs from Reset source");
   const auto id=static_cast<std::uint32_t>(q.selected);const auto byte=static_cast<std::size_t>(id>>3);q.bits_after.resize(byte+1,0);
   q.bits_after[byte]=static_cast<std::uint8_t>(q.bits_after[byte]|static_cast<std::uint8_t>(1u<<(id&7u)));
   // Exact setHasPromotion order through changeIgnoreTerrainCostCount. All
   // setters here are local checked count += iChange; later numeric/indexed,
   // selected UI and achievement branches are NOT credited by this prefix.
   for(std::size_t i=0;i<initial_promotion_members.size();++i){const auto before=staged.scalars.at(initial_promotion_members[i]);need(before>=0,"Initial promotion count violates native nonnegative invariant");
    const auto after=narrow(static_cast<std::int64_t>(before)+row.flags[i]);if(i!=10||row.flags[i]!=0)assign_scalar(staged,initial_promotion_members[i],after);}
   q.after=staged.scalars;q.dirty_after=staged.dirty;
  }
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.node->indexed);
  const auto&binding=*p->initial_hidden_binding;std::vector<std::uint64_t>post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.node->indexed,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto dirty=q.dirty_after;auto bits=q.bits_after;
  const auto repeated=initial_unit_info_promotion_rules(*p->host,*q.node);need(repeated.first==q.rows&&repeated.second==q.free,"Initial promotion actual rules changed during preparation");
  p->initial_unit_info_promotion.emplace(std::move(q));auto&stage=*p->initial_unit_info_promotion;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_military_support_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  need(stage.node->scalars==stage.before&&stage.node->dirty==stage.dirty_before&&stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_unit==stage.node.get()&&
   initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
   &p->host->session().source()==p->source&&!p->bank->retired.load(),"Initial promotion final noalloc operands/lifetime changed before stores");
  // Genuine unpublished node; no raw/live roster, public HasPromotion result,
  // checkpoint continuation or full grant until the remaining native tail.
  stage.node->promotion_bits.swap(bits);stage.node->scalars.swap(scalars);stage.node->dirty.swap(dirty);
  p->initial_city_strength->promotion_inputs.swap(city);p->initial_city_strength->promotion_stored=true;
  p->initial_post_city->promotion_successor_seal.swap(post);p->initial_center_plan->storage_->promotion_successor_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_unit_info_promotion_prefix(stage.selected,stage.rows.size());
  require_initial_unit_info_promotion_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}


namespace {
std::vector<NativePromotionFullInfo>load_initial_full_promotion_infos(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node,const std::vector<InitialUnitInfoPromotionRow>&prefixes,const std::set<std::int32_t>&free){
 std::string sql="SELECT ID";for(const auto&spec:native_promotion_scalar_specs){sql+=",";sql+=spec.column;}sql+=" FROM UnitPromotions WHERE ID > -1 ORDER BY ID";
 need(prefixes.size()==node.promotion_info_ids.size(),"Initial full promotion native extent differs");
 std::vector<NativePromotionFullInfo>result(prefixes.size());std::size_t present=0;std::uint64_t span=0;
 const auto q=host.database().query_rows({sql,{}},[&](const auto&r){
  const auto id=narrow(r.integer("ID"));need(id>=0&&static_cast<std::uint64_t>(id)>=span&&static_cast<std::size_t>(id)<result.size()&&
   prefixes[static_cast<std::size_t>(id)].id==id&&node.promotion_info_ids[static_cast<std::size_t>(id)]==id&&
   result[static_cast<std::size_t>(id)].prefix.id==-1,"Initial full promotion exact Info binding differs");
  NativePromotionFullInfo info;info.prefix=prefixes[static_cast<std::size_t>(id)];
  for(std::size_t i=0;i<info.values.size();++i){const auto&spec=native_promotion_scalar_specs[i];const auto value=r.integer(spec.column);
   if(spec.boolean){need(value==0||value==1,"Initial full promotion native bool unsupported");}info.values[i]=narrow(value);}
  for(std::size_t i=0;i<11;++i){need(info.values[i]==info.prefix.flags[i],"Initial full promotion first scalar interval differs");}
  result[static_cast<std::size_t>(id)]=std::move(info);span=static_cast<std::uint64_t>(id)+1;++present;
 });need(q.all_succeeded()&&q.rows==present&&span==prefixes.size(),"Initial full promotion complete scalar table absent");
 for(std::size_t slot=0;slot<result.size();++slot)need(result[slot].prefix==prefixes[slot],"Initial full promotion NULL/present Info differs");
 for(auto id:free)need(id>=0&&static_cast<std::size_t>(id)<result.size()&&result[static_cast<std::size_t>(id)].prefix.id==id,
  "Initial full promotion grant targets absent native Info");
 // Real CacheResults array initialization and relation overwrite semantics.
 // Complete Info owners are revalidated, including no-relation defaults.
 const auto terrains=native_init_info_span(host,"Terrains"),features=native_init_info_span(host,"Features"),combats=native_init_info_span(host,"UnitCombatInfos"),classes=native_init_info_span(host,"UnitClasses");
 const auto domains=native_init_info_span(host,"Domains");need(domains<=5,"Initial native domain catalog exceeds NUM_DOMAIN_TYPES");
 for(auto&info:result){
  if(!free.contains(info.prefix.id)){continue;}
  for(const auto key:{"m_extraTerrainAttackPercent","m_extraTerrainDefensePercent","m_terrainDoubleMoveCount","m_terrainImpassableCount"})info.indexed.emplace(key,std::vector<std::int32_t>(terrains,0));
  for(const auto key:{"m_extraFeatureAttackPercent","m_extraFeatureDefensePercent","m_featureDoubleMoveCount","m_featureImpassableCount"})info.indexed.emplace(key,std::vector<std::int32_t>(features,0));
  info.indexed.emplace("m_extraUnitCombatModifier",std::vector<std::int32_t>(combats,0));info.indexed.emplace("m_unitClassModifier",std::vector<std::int32_t>(classes,0));info.indexed.emplace("m_extraDomainModifiers",std::vector<std::int32_t>(5,0));
  const auto terrain_features=[&](const char*table,const char*column,const char*target,const std::array<const char*,4>&members){
   std::set<std::int32_t>seen;const auto rows=host.database().query_rows({std::string("SELECT ")+column+",Attack,Defense,DoubleMove,Impassable FROM "+table+" WHERE PromotionType=?",{info.prefix.type}},[&](const auto&r){
    const auto type=r.text(column);const auto*entry=type?host.types().find(*type):nullptr;
    need(entry&&entry->table==target&&entry->id>=0&&seen.insert(narrow(entry->id)).second,"Initial native promotion indexed relation missing/duplicate Info");
    const auto slot=static_cast<std::size_t>(entry->id);const std::array<const char*,4>columns={"Attack","Defense","DoubleMove","Impassable"};
    for(std::size_t i=0;i<members.size();++i){auto&values=info.indexed.at(members[i]);need(slot<values.size(),"Initial native promotion indexed slot outside genuine span");const auto n=r.integer(columns[i]);if(i>=2){need(n==0||n==1,"Initial native indexed promotion bool unsupported");}values[slot]=narrow(n);}
   });need(rows.all_succeeded(),"Initial native terrain/feature promotion relation unavailable");
  };
  terrain_features("UnitPromotions_Terrains","TerrainType","Terrains",{"m_extraTerrainAttackPercent","m_extraTerrainDefensePercent","m_terrainDoubleMoveCount","m_terrainImpassableCount"});
  terrain_features("UnitPromotions_Features","FeatureType","Features",{"m_extraFeatureAttackPercent","m_extraFeatureDefensePercent","m_featureDoubleMoveCount","m_featureImpassableCount"});
  const auto modifiers=[&](const char*table,const char*column,const char*target,const char*member){
   std::set<std::int32_t>seen;const auto rows=host.database().query_rows({std::string("SELECT ")+column+",Modifier FROM "+table+" WHERE PromotionType=?",{info.prefix.type}},[&](const auto&r){
    const auto type=r.text(column);const auto*entry=type?host.types().find(*type):nullptr;
    need(entry&&entry->table==target&&entry->id>=0&&seen.insert(narrow(entry->id)).second,"Initial native promotion modifier relation missing/duplicate Info");
    auto&values=info.indexed.at(member);need(static_cast<std::size_t>(entry->id)<values.size(),"Initial native promotion modifier slot unsupported");values[static_cast<std::size_t>(entry->id)]=narrow(r.integer("Modifier"));
   });need(rows.all_succeeded(),"Initial native promotion complete modifier relation unavailable");
  };
  modifiers("UnitPromotions_UnitCombatMods","UnitCombatType","UnitCombatInfos","m_extraUnitCombatModifier");modifiers("UnitPromotions_UnitClasses","UnitClassType","UnitClasses","m_unitClassModifier");modifiers("UnitPromotions_Domains","DomainType","Domains","m_extraDomainModifiers");
  for(const auto&[member,values]:info.indexed){need(node.indexed.contains(member)&&node.indexed.at(member).size()==values.size(),"Initial actual Reset indexed storage/catalog span differs");}
 }
 return result;
}
bool native_pending_has_promotion(const PendingUnit&node,std::int32_t id){need(id>=0&&static_cast<std::size_t>(id)<node.promotion_info_ids.size()&&node.promotion_info_ids[static_cast<std::size_t>(id)]==id,"Initial promotion genuine native bit index invalid");const auto n=static_cast<std::uint32_t>(id);const auto byte=static_cast<std::size_t>(n>>3);return byte<node.promotion_bits.size()&&(node.promotion_bits[byte]&(1u<<(n&7u)))!=0;}
void stage_native_promotion_effect(PendingUnit&node,const NativePromotionFullInfo&info,bool resume,bool target=true){
 if(!resume&&native_pending_has_promotion(node,info.prefix.id)==target){return;}
 need(!resume||target,"Native resumed prefix may only complete an actual grant");
 need(!info.prefix.hover,"Mandatory initial hovering promotion predecessor missing");
 if(resume){need(native_pending_has_promotion(node,info.prefix.id),"Actual first promotion bit prefix disappeared");}
 else{
  need((!info.prefix.invisible||info.prefix.invisible->empty())&&(!info.prefix.see_invisible||info.prefix.see_invisible->empty()),"Mandatory initial promotion visibility transition owner unavailable");
  const auto id=static_cast<std::uint32_t>(info.prefix.id);const auto byte=static_cast<std::size_t>(id>>3);if(byte>=node.promotion_bits.size()){node.promotion_bits.resize(byte+1,0);}const auto mask=static_cast<std::uint8_t>(1u<<(id&7u));node.promotion_bits[byte]=target?static_cast<std::uint8_t>(node.promotion_bits[byte]|mask):static_cast<std::uint8_t>(node.promotion_bits[byte]&static_cast<std::uint8_t>(~mask));
 }
 for(std::size_t i=resume?11u:0u;i<native_promotion_scalar_specs.size();++i){const auto&spec=native_promotion_scalar_specs[i];const auto delta=target?info.values[i]:narrow(-std::int64_t(info.values[i]));
  if(spec.guard&&delta==0){continue;}
  need(!spec.sight||delta==0,"Mandatory promotion sight removal/addition owner unavailable");need(!spec.ui||delta==0,"Mandatory genuine promotion DLLUI UnitInfo setter unavailable");
  const auto old=node.scalars.at(spec.member);const auto after=narrow(static_cast<std::int64_t>(old)+delta);if(spec.nonnegative){need(after>=0,"Native promotion scalar nonnegative assertion failed");}assign_scalar(node,spec.member,after);
 }
 // Source order: Terrain attack, defense, double-move, impassable per slot;
 // then Feature, UnitCombat, UnitClass, and fixed NUM_DOMAIN_TYPES=5.
 const auto set=[&](const char*member,std::size_t index,std::int32_t delta,bool guarded,bool nonnegative,bool info_dirty){
  if(guarded&&delta==0){return;}need(!info_dirty||delta==0,"Mandatory indexed promotion DLLUI UnitInfo setter unavailable");if(!target){delta=narrow(-std::int64_t(delta));}auto&values=node.indexed.at(member);need(index<values.size(),"Native promotion indexed slot missing");
  const auto after=narrow(static_cast<std::int64_t>(values[index])+delta);if(nonnegative){need(after>=0,"Native indexed promotion nonnegative assertion failed");}
  if(values[index]!=after){if(std::string_view(member)!="m_extraDomainModifiers"){const auto archive=archive_index(node,member);need(archive.has_value(),"Native promotion archive binding missing");node.dirty.insert(*archive);node.promotion_vector_sets.push_back({member,index,after});}values[index]=after;}
 };
 const auto terrain_features=[&](const std::array<const char*,4>&members){const auto span=info.indexed.at(members[0]).size();for(std::size_t slot=0;slot<span;++slot){for(std::size_t i=0;i<members.size();++i){set(members[i],slot,info.indexed.at(members[i])[slot],i<2,i>=2,i<2);}}};
 terrain_features({"m_extraTerrainAttackPercent","m_extraTerrainDefensePercent","m_terrainDoubleMoveCount","m_terrainImpassableCount"});terrain_features({"m_extraFeatureAttackPercent","m_extraFeatureDefensePercent","m_featureDoubleMoveCount","m_featureImpassableCount"});
 for(const auto member:{"m_extraUnitCombatModifier","m_unitClassModifier","m_extraDomainModifiers"}){const auto&values=info.indexed.at(member);for(std::size_t slot=0;slot<values.size();++slot){set(member,slot,values[slot],false,false,false);}}
}
}
void BnwNativeUnitAllocatorCore::require_initial_unit_info_promotion_loop_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored&&p->train_source&&p->initial_set_xy,"Actual UnitInfo promotion loop receipt missing");const auto&q=*p->initial_unit_info_promotion_loop;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&!p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot)),"Actual promotion loop SAME source/node changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(initial_unit_info_promotion_loop_node_matches_without_callbacks(),
  "Actual native UnitInfo promotion loop stores changed");
 need(q.input&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision&&p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().initial_unit_info_promotions_returned&&
  (p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_traits_promotions_owner||(p->initial_traits_promotions&&p->initial_traits_promotions->stored&&(p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||((p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||(p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||(p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().boundary==BnwUnitMoveBoundary::initial_moves_dim_owner&&p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().initial_immobile_moves_computed))&&p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().initial_exotic_xp_returned)))&&p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks().initial_player_promotions_returned)))),"Actual promotion loop client/original return changed");
 need(session.state_.get()==p->state.get()&&session.state_generation_==p->generation&&session.source_.get()==p->source&&!p->bank->retired.load(),"Actual promotion loop final noalloc lifetime changed");
}
bool BnwNativeUnitAllocatorCore::initial_unit_info_promotion_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_unit_info_promotion&&p->initial_unit_info_promotion->stored,"Actual first promotion receipt absent");const auto&q=*p->initial_unit_info_promotion;
 if(p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored){require_initial_unit_info_promotion_loop_without_callbacks();const auto&r=*p->initial_unit_info_promotion_loop;return r.node.get()==q.node.get()&&r.before==q.after&&r.dirty_before==q.dirty_after&&r.bits_before==q.bits_after;}
 return initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after;
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_unit_info_promotions_return()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_unit_info_promotion&&p->initial_unit_info_promotion->stored&&!p->initial_unit_info_promotion_loop&&p->initial_city_strength,"Actual promotion loop requires first native prefix once");
 try{
  p->check();require_initial_unit_info_promotion_without_callbacks();require_initial_military_support_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialUnitInfoPromotionLoop q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();const auto&first=*p->initial_unit_info_promotion;
  const auto repeated=initial_unit_info_promotion_rules(*p->host,*q.node);need(repeated.first==first.rows&&repeated.second==first.free,"Actual first promotion rules changed before native remainder");
  q.infos=load_initial_full_promotion_infos(*p->host,*q.node,first.rows,first.free);
  q.input=p->initial_center_plan->storage_->input;need(bool(q.input),"Actual promotion canonical client input absent");p->initial_set_xy->require_initial_military_game_data_identity_without_callbacks(q.input);
  q.input_revision=q.input->initial_center_revision_without_callbacks();const auto selected=q.input->initial_center_selection();
  need(!selected||selected->first!=p->actor||selected->second!=q.node->scalars.at("m_iID"),"Mandatory selected newborn promotion SelectionButtons/UnitInfo owner unavailable");
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  q.before=staged.scalars;q.indexed_before=staged.indexed;q.dirty_before=staged.dirty;q.bits_before=staged.promotion_bits;q.commands_before=staged.promotion_vector_sets;
  const auto*chest=p->host->types().find("PROMOTION_BUFFALO_CHEST"),*loins=p->host->types().find("PROMOTION_BUFFALO_LOINS");
  const auto active=detail::bnw_game_active_player_read(*p->state);
  for(std::size_t slot=0;slot<q.infos.size();++slot){const auto&info=q.infos[slot];if(!first.free.contains(info.prefix.id)){continue;}
   need(!info.prefix.hover,"Actual free hover promotion was not applied before SetXY");const auto resume=first.selected>=0&&info.prefix.id==first.selected;
   if(!resume&&native_pending_has_promotion(staged,info.prefix.id)){continue;}
   stage_native_promotion_effect(staged,info,resume);++q.grants;
   // Original achievements are outside first-alpha Steam scope; however this
   // actual reached pair is not silently acknowledged as an engine callback.
   if(p->actor==active&&chest&&loins){need(!((info.prefix.id==chest->id&&native_pending_has_promotion(staged,narrow(loins->id)))||(info.prefix.id==loins->id&&native_pending_has_promotion(staged,narrow(chest->id)))),"Mandatory native promotion achievement pair callback unavailable");}
  }
  q.after=staged.scalars;q.indexed_after=staged.indexed;q.dirty_after=staged.dirty;q.bits_after=staged.promotion_bits;q.commands_after=staged.promotion_vector_sets;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.indexed_after);
  const auto&binding=*p->initial_hidden_binding;std::vector<std::uint64_t>post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.indexed_after,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto indexed=q.indexed_after;auto dirty=q.dirty_after;auto bits=q.bits_after;auto commands=q.commands_after;
  const auto final_infos=load_initial_full_promotion_infos(*p->host,*q.node,first.rows,first.free);need(final_infos==q.infos,"Actual complete promotion rules changed during preparation");
  const auto selected_after=q.input->initial_center_selection();need(selected_after==selected&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision,"Actual native promotion selection changed during preparation");
  p->initial_unit_info_promotion_loop.emplace(std::move(q));auto&stage=*p->initial_unit_info_promotion_loop;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_unit_info_promotion_without_callbacks();require_initial_military_support_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  need(stage.node->scalars==stage.before&&stage.node->indexed==stage.indexed_before&&stage.node->dirty==stage.dirty_before&&stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_vector_sets==stage.commands_before&&
   stage.input->initial_center_revision_without_callbacks()==stage.input_revision&&detail::bnw_game_active_player_read(*p->state)==active&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation&&&p->host->session().source()==p->source&&!p->bank->retired.load(),"Native promotion loop final operands/source changed before stores");
  stage.node->scalars.swap(scalars);stage.node->indexed.swap(indexed);stage.node->dirty.swap(dirty);stage.node->promotion_bits.swap(bits);stage.node->promotion_vector_sets.swap(commands);
  p->initial_city_strength->promotion_loop_inputs.swap(city);p->initial_city_strength->promotion_loop_stored=true;
  p->initial_post_city->promotion_loop_seal.swap(post);p->initial_center_plan->storage_->promotion_loop_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_unit_info_promotions_return(stage.grants);require_initial_unit_info_promotion_loop_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
std::int32_t initial_native_unit_combat(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 const content::InfoTypeEntry*unit=nullptr;for(const auto&[key,entry]:host.types().entries()){(void)key;if(entry.table=="Units"&&entry.id==node.scalars.at("m_eUnitType")){need(!unit,"Traits promotion duplicate actual UnitInfo ID");unit=&entry;}}need(unit&&unit->hash==content::saved_info_type_hash(unit->type),"Traits promotion actual UnitInfo binding absent");
 bool found=false;std::int32_t combat=-1;
 const auto q=host.database().query_rows({"SELECT ID,Type,CombatClass FROM Units WHERE ID=?",{std::int64_t(unit->id)}},[&](const auto&r){
  need(!found&&r.integer("ID")==unit->id&&r.text("Type")==std::optional<std::string>{unit->type},"Traits promotion actual UnitInfo changed");found=true;
  const auto key=r.text("CombatClass");if(!key||key->empty()){return;}const auto*entry=host.types().find(*key);
  need(entry&&entry->table=="UnitCombatInfos"&&entry->id>=0&&entry->hash==content::saved_info_type_hash(*key),"Traits promotion actual CombatClass binding absent");combat=narrow(entry->id);
 });need(q.all_succeeded()&&found,"Traits promotion actual UnitCombat query failed");return combat;
}
std::vector<NativeTraitPromotionRelation>initial_native_trait_promotion_relations(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node,const std::vector<std::int64_t>&active){
 const auto traits=native_init_info_span(host,"Traits"),combats=native_init_info_span(host,"UnitCombatInfos");
 std::set<std::int64_t>unique;for(const auto id:active){need(id>=0&&static_cast<std::uint64_t>(id)<traits&&unique.insert(id).second,"Actual active Traits binding outside complete Info universe");}
 std::vector<NativeTraitPromotionRelation>result;
 // CacheResults multimap keeps duplicate rows. HasFreePromotionUnitCombat is
 // a boolean membership loop, so duplicate rows never grant twice.
 const auto q=host.database().query_rows({"SELECT TraitType,PromotionType,UnitCombatType FROM Trait_FreePromotionUnitCombats ORDER BY TraitType,PromotionType,UnitCombatType",{}},[&](const auto&r){
  const auto t=r.text("TraitType"),p=r.text("PromotionType"),u=r.text("UnitCombatType");
  const auto*ti=t?host.types().find(*t):nullptr;const auto*pi=p?host.types().find(*p):nullptr;const auto*ui=u?host.types().find(*u):nullptr;
  need(ti&&pi&&ui&&ti->table=="Traits"&&pi->table=="UnitPromotions"&&ui->table=="UnitCombatInfos"&&ti->id>=0&&pi->id>=0&&ui->id>=0&&
   static_cast<std::uint64_t>(ti->id)<traits&&static_cast<std::uint64_t>(ui->id)<combats&&static_cast<std::size_t>(pi->id)<node.promotion_info_ids.size()&&node.promotion_info_ids[static_cast<std::size_t>(pi->id)]==pi->id&&
   ti->hash==content::saved_info_type_hash(*t)&&pi->hash==content::saved_info_type_hash(*p)&&ui->hash==content::saved_info_type_hash(*u),"Actual Trait promotion relation has unresolved native Info identity");
  result.push_back({narrow(ti->id),narrow(pi->id),narrow(ui->id)});
 });need(q.all_succeeded(),"Actual complete Trait promotion relation query failed");return result;
}
}
void BnwNativeUnitAllocatorCore::require_initial_traits_promotions_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_traits_promotions&&p->initial_traits_promotions->stored&&p->train_source&&p->initial_set_xy,"Actual Traits promotion return missing");const auto&q=*p->initial_traits_promotions;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&!p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot)),"Actual Traits promotion SAME source/node changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(initial_traits_promotion_node_matches_without_callbacks(),"Actual Traits promotion stores changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(q.input&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision&&prefix.initial_traits_promotions_returned&&prefix.initial_traits_promotion_grants==q.grants&&((prefix.boundary==BnwUnitMoveBoundary::initial_player_free_promotions_owner||(p->initial_player_promotions&&p->initial_player_promotions->stored&&(prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))))||((prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned))&&prefix.initial_player_promotions_returned)),"Actual Traits promotion source/client return changed");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&!p->bank->retired.load(),"Actual Traits promotion final noalloc source changed");
}
bool BnwNativeUnitAllocatorCore::initial_unit_info_promotion_loop_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored,"Actual UnitInfo promotion loop receipt absent");const auto&q=*p->initial_unit_info_promotion_loop;
 if(p->initial_traits_promotions&&p->initial_traits_promotions->stored){require_initial_traits_promotions_without_callbacks();const auto&r=*p->initial_traits_promotions;return r.node.get()==q.node.get()&&r.before==q.after&&r.indexed_before==q.indexed_after&&r.dirty_before==q.dirty_after&&r.bits_before==q.bits_after&&r.commands_before==q.commands_after;}
 return initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&q.node->indexed==q.indexed_after&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after&&q.node->promotion_vector_sets==q.commands_after;
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_traits_promotions_return()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_unit_info_promotion_loop&&p->initial_unit_info_promotion_loop->stored&&!p->initial_traits_promotions&&p->initial_city_strength,"Actual Traits promotions require UnitInfo loop return once");
 try{
  p->check();require_initial_unit_info_promotion_loop_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialTraitsPromotionLoop q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  q.combat=initial_native_unit_combat(*p->host,*q.node);
  if(q.combat!=-1){q.traits=BnwMajorPlayerTraitsOwner::from_source_host(p->host,p->actor);const auto snapshot=q.traits->snapshot();need(snapshot.player==p->actor,"Actual Trait owner actor changed");q.active=snapshot.active_traits;
   q.relations=initial_native_trait_promotion_relations(*p->host,*q.node,q.active);
   // Original ascending promotion loop, each boolean result at most one grant.
   for(const auto id:q.node->promotion_info_ids){if(id<0){continue;}for(const auto trait:q.active){bool granted=false;for(const auto&r:q.relations){if(r.trait==trait&&r.promotion==id&&r.combat==q.combat){q.free.insert(id);granted=true;break;}}if(granted){break;}}}
  }
  q.infos=load_initial_full_promotion_infos(*p->host,*q.node,p->initial_unit_info_promotion->rows,q.free);
  q.input=p->initial_center_plan->storage_->input;need(bool(q.input),"Actual Traits promotion canonical client absent");q.input_revision=q.input->initial_center_revision_without_callbacks();const auto selected=q.input->initial_center_selection();
  need(!selected||selected->first!=p->actor||selected->second!=q.node->scalars.at("m_iID"),"Mandatory selected newborn promotion client owner unavailable");
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  q.before=staged.scalars;q.indexed_before=staged.indexed;q.dirty_before=staged.dirty;q.bits_before=staged.promotion_bits;q.commands_before=staged.promotion_vector_sets;
  const auto*chest=p->host->types().find("PROMOTION_BUFFALO_CHEST"),*loins=p->host->types().find("PROMOTION_BUFFALO_LOINS");const auto active=detail::bnw_game_active_player_read(*p->state);
  for(const auto&info:q.infos){if(!q.free.contains(info.prefix.id)||native_pending_has_promotion(staged,info.prefix.id)){continue;}stage_native_promotion_effect(staged,info,false);++q.grants;
   if(p->actor==active&&chest&&loins){need(!((info.prefix.id==chest->id&&native_pending_has_promotion(staged,narrow(loins->id)))||(info.prefix.id==loins->id&&native_pending_has_promotion(staged,narrow(chest->id)))),"Mandatory native promotion achievement pair callback unavailable");}
  }
  q.after=staged.scalars;q.indexed_after=staged.indexed;q.dirty_after=staged.dirty;q.bits_after=staged.promotion_bits;q.commands_after=staged.promotion_vector_sets;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.indexed_after);const auto&binding=*p->initial_hidden_binding;std::vector<std::uint64_t>post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.indexed_after,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto indexed=q.indexed_after;auto dirty=q.dirty_after;auto bits=q.bits_after;auto commands=q.commands_after;
  need(initial_native_unit_combat(*p->host,*q.node)==q.combat,"Actual UnitCombat changed during Trait promotion preparation");
  if(q.traits){need(q.traits->snapshot().active_traits==q.active&&initial_native_trait_promotion_relations(*p->host,*q.node,q.active)==q.relations,"Actual Trait promotion membership/rules changed during preparation");}
  need(load_initial_full_promotion_infos(*p->host,*q.node,p->initial_unit_info_promotion->rows,q.free)==q.infos,"Actual Trait promotion effects changed during preparation");
  need(q.input->initial_center_selection()==selected&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision,"Actual Trait promotion selection changed during preparation");
  p->initial_traits_promotions.emplace(std::move(q));auto&stage=*p->initial_traits_promotions;
  if(stage.traits){stage.traits->require_current();}p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_unit_info_promotion_loop_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  need(stage.node->scalars==stage.before&&stage.node->indexed==stage.indexed_before&&stage.node->dirty==stage.dirty_before&&stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_vector_sets==stage.commands_before&&stage.input->initial_center_revision_without_callbacks()==stage.input_revision&&detail::bnw_game_active_player_read(*p->state)==active&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&&p->host->session().source()==p->source&&!p->bank->retired.load(),"Actual Traits promotion final operands changed before stores");
  stage.node->scalars.swap(scalars);stage.node->indexed.swap(indexed);stage.node->dirty.swap(dirty);stage.node->promotion_bits.swap(bits);stage.node->promotion_vector_sets.swap(commands);
  p->initial_city_strength->traits_promotion_inputs.swap(city);p->initial_city_strength->traits_promotion_stored=true;p->initial_post_city->traits_promotion_seal.swap(post);p->initial_center_plan->storage_->traits_promotion_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_traits_promotions_return(stage.grants);require_initial_traits_promotions_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
const saves::PlayerCoreField&initial_player_promotion_field(const LiveSessionState&s,std::int32_t actor){
 need(s.world.all_players&&actor>=0&&static_cast<std::size_t>(actor)<s.world.all_players->players.size(),"Initial free promotion actual player absent");const auto&p=s.world.all_players->players[static_cast<std::size_t>(actor)];
 need(p.saved_player_id==actor&&p.core.layout==bnw,"Initial free promotion actual player identity/layout differs");return garrison_named(p.core.fields,"m_paiFreePromotionCount");
}
bool initial_player_promotion_raw_matches(const saves::PlayerCoreField&a,const saves::PlayerCoreField&b){return a.name==b.name&&a.encoding==b.encoding&&a.start_offset==b.start_offset&&a.end_offset==b.end_offset&&a.wire_count==b.wire_count&&a.values==b.values&&a.row_counts==b.row_counts&&a.hashes==b.hashes&&a.bytes==b.bytes;}
std::vector<std::int32_t>initial_player_promotion_counts(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node,const saves::PlayerCoreField&f){
 need(f.encoding==saves::PlayerCoreEncoding::hashed_i32&&f.wire_count&&*f.wire_count==f.hashes.size()&&f.bytes.empty()&&f.row_counts.empty(),"Initial free promotion genuine native hashed count encoding absent");
 // Native Player reset sized the whole Info array with zero BEFORE
 // ReadHashedDataArray. Hash0 has no payload, nonzero hashes one value.
 std::vector<std::int32_t>result(node.promotion_info_ids.size(),0);std::set<std::int64_t>seen;std::size_t value=0;
 for(const auto hash:f.hashes){need(hash<=UINT32_MAX,"Initial free promotion hash width invalid");if(hash==0){continue;}need(value<f.values.size(),"Initial free promotion hashed value missing");const auto*info=host.types().find_hash(hash);
  need(info&&info->table=="UnitPromotions"&&info->id>=0&&static_cast<std::size_t>(info->id)<result.size()&&node.promotion_info_ids[static_cast<std::size_t>(info->id)]==info->id&&info->hash==content::saved_info_type_hash(info->type)&&seen.insert(info->id).second,"Initial free promotion native mapped slot unresolved/duplicate");
  const auto count=narrow(f.values[value++]);need(count>=0,"Initial free promotion negative native counter invalid");result[static_cast<std::size_t>(info->id)]=count;
 }need(value==f.values.size(),"Initial free promotion trailing hashed values invalid");return result;
}
InitialPlayerPromotionValidity initial_player_promotion_validity(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 const auto combats=native_init_info_span(host,"UnitCombatInfos"),units=native_init_info_span(host,"Units");InitialPlayerPromotionValidity result;
 const auto load=[&](const char*table,const char*column,const char*target,std::size_t span,std::vector<std::pair<std::int32_t,std::int32_t>>&values){
  const auto q=host.database().query_rows({std::string("SELECT PromotionType,")+column+" FROM "+table+" ORDER BY PromotionType,"+column,{}},[&](const auto&r){
   const auto p=r.text("PromotionType"),t=r.text(column);const auto*pi=p?host.types().find(*p):nullptr;const auto*ti=t?host.types().find(*t):nullptr;
   need(pi&&ti&&pi->table=="UnitPromotions"&&ti->table==target&&pi->id>=0&&ti->id>=0&&static_cast<std::size_t>(pi->id)<node.promotion_info_ids.size()&&node.promotion_info_ids[static_cast<std::size_t>(pi->id)]==pi->id&&static_cast<std::size_t>(ti->id)<span&&pi->hash==content::saved_info_type_hash(*p)&&ti->hash==content::saved_info_type_hash(*t),"Initial promotion validity genuine relation unresolved");values.emplace_back(narrow(pi->id),narrow(ti->id));
  });need(q.all_succeeded(),"Initial promotion validity complete native relation failed");
 };
 load("UnitPromotions_UnitCombats","UnitCombatType","UnitCombatInfos",combats,result.combats);load("UnitPromotions_CivilianUnitType","UnitType","Units",units,result.civilians);return result;
}
}
void BnwNativeUnitAllocatorCore::require_initial_player_promotions_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_player_promotions&&p->initial_player_promotions->stored&&p->train_source&&p->initial_set_xy,"Actual Player free promotion return missing");const auto&q=*p->initial_player_promotions;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&!p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot)),"Actual Player free promotion SAME source/node changed");
 need(&initial_player_promotion_field(*p->state,p->actor)==q.field&&initial_player_promotion_raw_matches(*q.field,q.raw_before),"Actual player free promotion counters changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(initial_player_promotion_node_matches_without_callbacks(),"Actual Player free promotion stores changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(q.input&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision&&prefix.initial_player_promotions_returned&&prefix.initial_player_promotion_grants==q.grants&&(prefix.boundary==BnwUnitMoveBoundary::initial_embark_promotions_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned))&&prefix.initial_embark_promotions_returned)),"Actual Player free promotion source/client return changed");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&!p->bank->retired.load(),"Actual Player free promotion final noalloc source changed");
}

bool BnwNativeUnitAllocatorCore::initial_traits_promotion_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_traits_promotions&&p->initial_traits_promotions->stored,"Actual Traits promotion loop receipt absent");const auto&q=*p->initial_traits_promotions;
 if(p->initial_player_promotions&&p->initial_player_promotions->stored){require_initial_player_promotions_without_callbacks();const auto&r=*p->initial_player_promotions;return r.node.get()==q.node.get()&&r.before==q.after&&r.indexed_before==q.indexed_after&&r.dirty_before==q.dirty_after&&r.bits_before==q.bits_after&&r.commands_before==q.commands_after;}
 return initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&q.node->indexed==q.indexed_after&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after&&q.node->promotion_vector_sets==q.commands_after;
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_player_promotions_return()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_traits_promotions&&p->initial_traits_promotions->stored&&!p->initial_player_promotions&&p->initial_city_strength,"Actual Player free promotions require UnitInfo loop return once");
 try{
  p->check();require_initial_traits_promotions_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialPlayerPromotionLoop q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  q.combat=initial_native_unit_combat(*p->host,*q.node);
  q.field=&initial_player_promotion_field(*p->state,p->actor);q.raw_before=*q.field;q.counts=initial_player_promotion_counts(*p->host,*q.node,*q.field);q.validity=initial_player_promotion_validity(*p->host,*q.node);
  const auto type=q.node->scalars.at("m_eUnitType");
  for(const auto id:q.node->promotion_info_ids){if(id<0){continue;}if(q.counts[static_cast<std::size_t>(id)]<=0){continue;}
   const bool combat=q.combat!=-1&&std::find(q.validity.combats.begin(),q.validity.combats.end(),std::pair{id,q.combat})!=q.validity.combats.end();
   if(combat||std::find(q.validity.civilians.begin(),q.validity.civilians.end(),std::pair{id,type})!=q.validity.civilians.end()){q.free.insert(id);}
  }
  q.infos=load_initial_full_promotion_infos(*p->host,*q.node,p->initial_unit_info_promotion->rows,q.free);
  q.input=p->initial_center_plan->storage_->input;need(bool(q.input),"Actual Player free promotion canonical client absent");q.input_revision=q.input->initial_center_revision_without_callbacks();const auto selected=q.input->initial_center_selection();
  need(!selected||selected->first!=p->actor||selected->second!=q.node->scalars.at("m_iID"),"Mandatory selected newborn promotion client owner unavailable");
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  q.before=staged.scalars;q.indexed_before=staged.indexed;q.dirty_before=staged.dirty;q.bits_before=staged.promotion_bits;q.commands_before=staged.promotion_vector_sets;
  const auto*chest=p->host->types().find("PROMOTION_BUFFALO_CHEST"),*loins=p->host->types().find("PROMOTION_BUFFALO_LOINS");const auto active=detail::bnw_game_active_player_read(*p->state);
  for(const auto&info:q.infos){if(!q.free.contains(info.prefix.id)||native_pending_has_promotion(staged,info.prefix.id)){continue;}stage_native_promotion_effect(staged,info,false);++q.grants;
   if(p->actor==active&&chest&&loins){need(!((info.prefix.id==chest->id&&native_pending_has_promotion(staged,narrow(loins->id)))||(info.prefix.id==loins->id&&native_pending_has_promotion(staged,narrow(chest->id)))),"Mandatory native promotion achievement pair callback unavailable");}
  }
  q.after=staged.scalars;q.indexed_after=staged.indexed;q.dirty_after=staged.dirty;q.bits_after=staged.promotion_bits;q.commands_after=staged.promotion_vector_sets;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.indexed_after);const auto&binding=*p->initial_hidden_binding;std::vector<std::uint64_t>post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.indexed_after,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto indexed=q.indexed_after;auto dirty=q.dirty_after;auto bits=q.bits_after;auto commands=q.commands_after;
  need(initial_native_unit_combat(*p->host,*q.node)==q.combat,"Actual UnitCombat changed during Trait promotion preparation");
  need(&initial_player_promotion_field(*p->state,p->actor)==q.field&&initial_player_promotion_raw_matches(*q.field,q.raw_before)&&initial_player_promotion_counts(*p->host,*q.node,*q.field)==q.counts&&initial_player_promotion_validity(*p->host,*q.node)==q.validity,"Actual player promotion counters/validity changed during preparation");
  need(load_initial_full_promotion_infos(*p->host,*q.node,p->initial_unit_info_promotion->rows,q.free)==q.infos,"Actual Trait promotion effects changed during preparation");
  need(q.input->initial_center_selection()==selected&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision,"Actual Trait promotion selection changed during preparation");
  p->initial_player_promotions.emplace(std::move(q));auto&stage=*p->initial_player_promotions;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_traits_promotions_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  need(&initial_player_promotion_field(*p->state,p->actor)==stage.field&&initial_player_promotion_raw_matches(*stage.field,stage.raw_before)&&stage.node->scalars==stage.before&&stage.node->indexed==stage.indexed_before&&stage.node->dirty==stage.dirty_before&&stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_vector_sets==stage.commands_before&&stage.input->initial_center_revision_without_callbacks()==stage.input_revision&&detail::bnw_game_active_player_read(*p->state)==active&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&&p->host->session().source()==p->source&&!p->bank->retired.load(),"Actual Player free promotion final operands changed before stores");
  stage.node->scalars.swap(scalars);stage.node->indexed.swap(indexed);stage.node->dirty.swap(dirty);stage.node->promotion_bits.swap(bits);stage.node->promotion_vector_sets.swap(commands);
  p->initial_city_strength->player_promotion_inputs.swap(city);p->initial_city_strength->player_promotion_stored=true;p->initial_post_city->player_promotion_seal.swap(post);p->initial_center_plan->storage_->player_promotion_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_player_promotions_return(stage.grants);require_initial_player_promotions_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}


namespace {
const saves::PlayerTraitsField& initial_embark_trait_field(const LiveSessionState&s,std::int32_t actor){
 need(s.world.all_players&&actor>=0&&static_cast<std::size_t>(actor)<s.world.all_players->players.size(),"Embark actual player absent");
 const auto&p=s.world.all_players->players.at(static_cast<std::size_t>(actor));need(p.saved_player_id==actor&&p.traits.layout==bnw,"Embark actual Traits layout/identity differs");
 return garrison_named(p.traits.fields,"m_bEmbarkedAllWater");
}
bool initial_embark_trait_raw_matches(const saves::PlayerTraitsField&a,const saves::PlayerTraitsField&b){
 return a.name==b.name&&a.encoding==b.encoding&&a.start_offset==b.start_offset&&a.end_offset==b.end_offset&&a.columns==b.columns&&a.wire_count==b.wire_count&&a.values==b.values&&a.rows.empty()&&b.rows.empty()&&a.hashes==b.hashes;
}
std::int32_t initial_embark_define(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node,const char*key){
 const auto*d=host.defines().find(key);need(host.defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),"Embark actual cached native promotion define absent");
 const auto id=narrow(std::get<std::int64_t>(d->lookup.value));need(id>=0&&static_cast<std::size_t>(id)<node.promotion_info_ids.size()&&node.promotion_info_ids[static_cast<std::size_t>(id)]==id,"Embark actual cached promotion define outside Info span");return id;
}
std::int32_t initial_embark_domain(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 std::int32_t result=-1;bool found=false;const auto rows=host.database().query_rows({"SELECT ID,Domain FROM Units WHERE ID=?",{std::int64_t(node.scalars.at("m_eUnitType"))}},[&](const auto&r){
  const auto key=r.text("Domain");const auto*d=key?host.types().find(*key):nullptr;need(!found&&r.integer("ID")==node.scalars.at("m_eUnitType")&&d&&d->table=="Domains"&&d->hash==content::saved_info_type_hash(d->type),"Embark actual UnitInfo Domain differs");found=true;result=narrow(d->id);
 });need(rows.all_succeeded()&&found,"Embark actual Domain query unavailable");const auto*hover=host.types().find("DOMAIN_HOVER");need(!hover||result!=hover->id,"Mandatory native dynamic DOMAIN_HOVER plot owner unavailable");return result;
}
}
bool BnwNativeUnitAllocatorCore::initial_player_promotion_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_player_promotions&&p->initial_player_promotions->stored,"Actual Player promotion receipt absent");const auto&q=*p->initial_player_promotions;
 if(p->initial_embark_promotions&&p->initial_embark_promotions->stored){require_initial_embark_promotions_without_callbacks();const auto&r=*p->initial_embark_promotions;return r.node.get()==q.node.get()&&r.before==q.after&&r.indexed_before==q.indexed_after&&r.dirty_before==q.dirty_after&&r.bits_before==q.bits_after&&r.commands_before==q.commands_after;}
 return initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&q.node->indexed==q.indexed_after&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after&&q.node->promotion_vector_sets==q.commands_after;
}
void BnwNativeUnitAllocatorCore::require_initial_embark_promotions_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_embark_promotions&&p->initial_embark_promotions->stored&&p->train_source&&p->initial_set_xy,"Actual Embark promotion return missing");const auto&q=*p->initial_embark_promotions;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&!p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot)),"Actual Embark promotion SAME source/node changed");
 need(post_player_team(*p->state,p->actor)==q.team&&&post_team(*p->state,q.team)==q.team_source&&post_named(q.team_source->fields,"can_embark").value==q.embark&&post_named(q.team_source->fields,"defensive_embark").value==q.defensive&&&initial_embark_trait_field(*p->state,p->actor)==q.trait_field&&initial_embark_trait_raw_matches(*q.trait_field,q.trait_before),"Actual Embark Team/Trait cache changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(initial_embark_promotion_node_matches_without_callbacks(),"Actual Embark promotion stores changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(q.input&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision&&prefix.initial_embark_promotions_returned&&prefix.initial_embark_promotion_grants==q.grants&&(prefix.boundary==BnwUnitMoveBoundary::initial_exotic_goods_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed))&&prefix.initial_exotic_xp_returned)),"Actual Embark promotion source/client return changed");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&!p->bank->retired.load(),"Actual Embark promotion final noalloc source changed");
}

std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_embark_promotions_return()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_player_promotions&&p->initial_player_promotions->stored&&!p->initial_embark_promotions&&p->initial_city_strength,"Actual Embark promotions require UnitInfo loop return once");
 try{
  p->check();require_initial_player_promotions_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialEmbarkPromotionLoop q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  q.team=post_player_team(*p->state,p->actor);q.team_source=&post_team(*p->state,q.team);q.embark=narrow(post_named(q.team_source->fields,"can_embark").value);q.defensive=narrow(post_named(q.team_source->fields,"defensive_embark").value);
  q.traits=BnwMajorPlayerTraitsOwner::from_source_host(p->host,p->actor);q.all_water=q.traits->cache_value("m_bEmbarkedAllWater");need(q.all_water==0||q.all_water==1,"Embark native Trait bool invalid");
  q.trait_field=&initial_embark_trait_field(*p->state,p->actor);q.trait_before=*q.trait_field;need(q.trait_field->encoding==saves::PlayerTraitsEncoding::boolean&&q.trait_field->values.size()==1&&q.trait_field->values[0]==q.all_water&&q.trait_field->rows.empty()&&q.trait_field->hashes.empty(),"Embark actual paired Trait cache differs");
  const auto define=[&](const char*key){const auto id=initial_embark_define(*p->host,*q.node,key);q.defines.emplace_back(key,id);return id;};
  if(q.embark>0||q.all_water){
   const auto id=define(q.defensive>0?"PROMOTION_DEFENSIVE_EMBARKATION":q.all_water?"PROMOTION_ALLWATER_EMBARKATION":"PROMOTION_EMBARKATION");
   q.domain=initial_embark_domain(*p->host,*q.node);const auto*land=p->host->types().find("DOMAIN_LAND");need(land&&land->table=="Domains","Embark genuine DOMAIN_LAND absent");
   bool grant=q.domain==land->id&&q.node->scalars.at("m_iBaseCombat")<=0;
   if(!grant){q.combat=initial_native_unit_combat(*p->host,*q.node);q.validity=initial_player_promotion_validity(*p->host,*q.node);grant=q.combat!=-1&&std::find(q.validity.combats.begin(),q.validity.combats.end(),std::pair{id,q.combat})!=q.validity.combats.end();}
   if(grant)q.free.insert(id);
  }
  // Native strips only actually owned Ocean bits. Their nonzero inverse setter
  // is a separate mandatory producer; no false ACK or partial node stores.
  if(q.all_water){for(const auto*key:{"PROMOTION_OCEAN_IMPASSABLE","PROMOTION_OCEAN_IMPASSABLE_UNTIL_ASTRONOMY"}){(void)define(key);}}
  q.effects=q.free;if(q.all_water){for(const auto&[key,id]:q.defines){if(key=="PROMOTION_OCEAN_IMPASSABLE"||key=="PROMOTION_OCEAN_IMPASSABLE_UNTIL_ASTRONOMY"){q.effects.insert(id);}}}
  q.infos=load_initial_full_promotion_infos(*p->host,*q.node,p->initial_unit_info_promotion->rows,q.effects);
  q.input=p->initial_center_plan->storage_->input;need(bool(q.input),"Actual Embark promotion canonical client absent");q.input_revision=q.input->initial_center_revision_without_callbacks();const auto selected=q.input->initial_center_selection();
  need(!selected||selected->first!=p->actor||selected->second!=q.node->scalars.at("m_iID"),"Mandatory selected newborn promotion client owner unavailable");
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  q.before=staged.scalars;q.indexed_before=staged.indexed;q.dirty_before=staged.dirty;q.bits_before=staged.promotion_bits;q.commands_before=staged.promotion_vector_sets;
  const auto*chest=p->host->types().find("PROMOTION_BUFFALO_CHEST"),*loins=p->host->types().find("PROMOTION_BUFFALO_LOINS");const auto active=detail::bnw_game_active_player_read(*p->state);
  for(const auto&info:q.infos){if(!q.free.contains(info.prefix.id)||native_pending_has_promotion(staged,info.prefix.id)){continue;}stage_native_promotion_effect(staged,info,false);++q.grants;
   if(p->actor==active&&chest&&loins){need(!((info.prefix.id==chest->id&&native_pending_has_promotion(staged,narrow(loins->id)))||(info.prefix.id==loins->id&&native_pending_has_promotion(staged,narrow(chest->id)))),"Mandatory native promotion achievement pair callback unavailable");}
  }
  if(q.all_water){for(const auto&[key,id]:q.defines){if(key=="PROMOTION_OCEAN_IMPASSABLE"||key=="PROMOTION_OCEAN_IMPASSABLE_UNTIL_ASTRONOMY"){
   if(!native_pending_has_promotion(staged,id)){continue;}
   const auto info=std::find_if(q.infos.begin(),q.infos.end(),[&](const auto&entry){return entry.prefix.id==id;});need(info!=q.infos.end(),"Actual Ocean inverse promotion Info unavailable");stage_native_promotion_effect(staged,*info,false,false);
   // SDK achievement check applies after either true OR false setter.
   if(p->actor==active&&chest&&loins){need(!((id==chest->id&&native_pending_has_promotion(staged,narrow(loins->id)))||(id==loins->id&&native_pending_has_promotion(staged,narrow(chest->id)))),"Mandatory native inverse promotion achievement callback unavailable");}
  }}}
  q.after=staged.scalars;q.indexed_after=staged.indexed;q.dirty_after=staged.dirty;q.bits_after=staged.promotion_bits;q.commands_after=staged.promotion_vector_sets;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.indexed_after);const auto&binding=*p->initial_hidden_binding;std::vector<std::uint64_t>post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.indexed_after,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto indexed=q.indexed_after;auto dirty=q.dirty_after;auto bits=q.bits_after;auto commands=q.commands_after;
  q.traits->require_current();need(q.traits->cache_value("m_bEmbarkedAllWater")==q.all_water&&post_player_team(*p->state,p->actor)==q.team&&&post_team(*p->state,q.team)==q.team_source&&post_named(q.team_source->fields,"can_embark").value==q.embark&&post_named(q.team_source->fields,"defensive_embark").value==q.defensive&&initial_embark_trait_raw_matches(initial_embark_trait_field(*p->state,p->actor),q.trait_before),"Actual Embark cache changed during preparation");
  for(const auto&[key,id]:q.defines){need(initial_embark_define(*p->host,*q.node,key.c_str())==id,"Actual Embark cached define changed");}
  if(q.domain!=-1){need(initial_embark_domain(*p->host,*q.node)==q.domain,"Actual Embark Domain changed");}
  if(q.combat!=-1){need(initial_native_unit_combat(*p->host,*q.node)==q.combat&&initial_player_promotion_validity(*p->host,*q.node)==q.validity,"Actual Embark combat validity changed");}
  need(load_initial_full_promotion_infos(*p->host,*q.node,p->initial_unit_info_promotion->rows,q.effects)==q.infos,"Actual Trait promotion effects changed during preparation");
  need(q.input->initial_center_selection()==selected&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision,"Actual Trait promotion selection changed during preparation");
  p->initial_embark_promotions.emplace(std::move(q));auto&stage=*p->initial_embark_promotions;
  stage.traits->require_current();p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_player_promotions_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  need(post_player_team(*p->state,p->actor)==stage.team&&&post_team(*p->state,stage.team)==stage.team_source&&post_named(stage.team_source->fields,"can_embark").value==stage.embark&&post_named(stage.team_source->fields,"defensive_embark").value==stage.defensive&&&initial_embark_trait_field(*p->state,p->actor)==stage.trait_field&&initial_embark_trait_raw_matches(*stage.trait_field,stage.trait_before)&&stage.node->scalars==stage.before&&stage.node->indexed==stage.indexed_before&&stage.node->dirty==stage.dirty_before&&stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_vector_sets==stage.commands_before&&stage.input->initial_center_revision_without_callbacks()==stage.input_revision&&detail::bnw_game_active_player_read(*p->state)==active&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&&p->host->session().source()==p->source&&!p->bank->retired.load(),"Actual Embark promotion final operands changed before stores");
  stage.node->scalars.swap(scalars);stage.node->indexed.swap(indexed);stage.node->dirty.swap(dirty);stage.node->promotion_bits.swap(bits);stage.node->promotion_vector_sets.swap(commands);
  p->initial_city_strength->embark_promotion_inputs.swap(city);p->initial_city_strength->embark_promotion_stored=true;p->initial_post_city->embark_promotion_seal.swap(post);p->initial_center_plan->storage_->embark_promotion_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_embark_promotions_return(stage.grants);require_initial_embark_promotions_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}



namespace {
std::int32_t initial_exotic_goods_info(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 std::int32_t result=0;bool found=false;const auto rows=host.database().query_rows({"SELECT ID,NumExoticGoods FROM Units WHERE ID=?",{std::int64_t(node.scalars.at("m_eUnitType"))}},[&](const auto&r){need(!found&&r.integer("ID")==node.scalars.at("m_eUnitType"),"Initial exotic actual UnitInfo differs");found=true;result=narrow(r.integer("NumExoticGoods"));});need(rows.all_succeeded()&&found,"Initial exotic actual UnitInfo unavailable");return result;
}
std::int32_t initial_xp_game_handicap(const LiveSessionState&s){need(s.world.game_state.layout==bnw,"Initial XP native Game source absent");const auto id=narrow(post_named(s.world.game_state.controls,"handicap").value);need(id>=0&&s.setup.game_handicap==id,"Initial XP raw/owned Game handicap differs");return id;}
std::pair<std::int32_t,std::int32_t>initial_xp_handicap_info(const ui::SelectedBnwPreviewHost&host,std::int32_t id){
 const content::InfoTypeEntry*info=nullptr;for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="HandicapInfos"&&e.id==id){need(!info,"Initial XP duplicate HandicapInfo ID");info=&e;}}
 need(info&&info->hash==content::saved_info_type_hash(info->type)&&host.types().find_hash(info->hash)==info,"Initial XP genuine HandicapInfo binding missing");bool found=false;std::pair<std::int32_t,std::int32_t>result;
 const auto rows=host.database().query_rows({"SELECT ID,Type,AIFreeXP,AIFreeXPPercent FROM HandicapInfos WHERE ID=?",{std::int64_t(id)}},[&](const auto&r){need(!found&&r.integer("ID")==id&&r.text("Type")==std::optional<std::string>{info->type},"Initial XP native HandicapInfo differs");found=true;result={narrow(r.integer("AIFreeXP")),narrow(r.integer("AIFreeXPPercent"))};});need(rows.all_succeeded()&&found,"Initial XP actual HandicapInfo unavailable");return result;
}
}
bool BnwNativeUnitAllocatorCore::initial_embark_promotion_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_embark_promotions&&p->initial_embark_promotions->stored,"Actual Embark promotion receipt absent");const auto&q=*p->initial_embark_promotions;
 if(p->initial_exotic_xp&&p->initial_exotic_xp->stored){require_initial_exotic_xp_without_callbacks();const auto&r=*p->initial_exotic_xp;return r.node.get()==q.node.get()&&r.before==q.after&&r.indexed_before==q.indexed_after&&r.dirty_before==q.dirty_after&&r.bits_before==q.bits_after&&r.commands_before==q.commands_after;}
 return initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&q.node->indexed==q.indexed_after&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after&&q.node->promotion_vector_sets==q.commands_after;
}
void BnwNativeUnitAllocatorCore::require_initial_exotic_xp_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_exotic_xp&&p->initial_exotic_xp->stored&&p->train_source&&p->initial_set_xy,"Actual Exotic/XP return missing");const auto&q=*p->initial_exotic_xp;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&!p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot)),"Actual Exotic/XP SAME source/node changed");
 need(initial_xp_game_handicap(*p->state)==q.handicap&&p->state->world.pregame&&p->state->world.pregame->layout==bnw&&p->state->world.pregame->archive.integers==q.pregame_integers&&p->state->world.pregame->archive.integer_arrays==q.pregame_arrays&&p->state->setup.game_type==q.game_type&&p->state->setup.pitboss==q.pitboss,"Actual Exotic/XP human/handicap source changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(initial_exotic_xp_node_matches_without_callbacks(),"Actual Exotic/XP stores changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(q.input&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision&&prefix.initial_exotic_xp_returned&&(prefix.boundary==BnwUnitMoveBoundary::initial_immobile_moves_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned))&&prefix.initial_immobile_moves_computed)),"Actual Exotic/XP source/client return changed");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&!p->bank->retired.load(),"Actual Exotic/XP final noalloc source changed");
}

std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_exotic_xp_return()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_embark_promotions&&p->initial_embark_promotions->stored&&!p->initial_exotic_xp&&p->initial_city_strength,"Actual Exotic/XPs require UnitInfo loop return once");
 try{
  p->check();require_initial_embark_promotions_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialExoticXpPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  q.exotic=initial_exotic_goods_info(*p->host,*q.node);q.handicap=initial_xp_game_handicap(*p->state);const auto handicap=initial_xp_handicap_info(*p->host,q.handicap);q.xp=handicap.first;q.xp_percent=handicap.second;
  need(p->state->world.pregame&&p->state->world.pregame->layout==bnw,"Initial XP genuine Pregame source absent");q.pregame_integers=p->state->world.pregame->archive.integers;q.pregame_arrays=p->state->world.pregame->archive.integer_arrays;q.game_type=p->state->setup.game_type;q.pitboss=p->state->setup.pitboss;
  // SDK each XP guard evaluates the nonzero Info FIRST, then human, then
  // canAcquirePromotionAny. Never guess the latter for a reached AI branch.
  if(q.xp!=0){q.human=detail::bnw_player_human_read(*p->state,p->actor,p->host->database());need(*q.human,"Mandatory initial AI canAcquirePromotionAny/experience producer unavailable");}
  if(q.xp_percent!=0){const auto human=detail::bnw_player_human_read(*p->state,p->actor,p->host->database());need(human,"Mandatory initial AI canAcquirePromotionAny/experience-percent producer unavailable");if(q.human)need(*q.human==human,"Initial XP human source changed between guards");q.human=human;}
  q.input=p->initial_center_plan->storage_->input;need(bool(q.input),"Actual Exotic/XP canonical client absent");q.input_revision=q.input->initial_center_revision_without_callbacks();const auto selected=q.input->initial_center_selection();
  need(!selected||selected->first!=p->actor||selected->second!=q.node->scalars.at("m_iID"),"Mandatory selected newborn promotion client owner unavailable");
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  q.before=staged.scalars;q.indexed_before=staged.indexed;q.dirty_before=staged.dirty;q.bits_before=staged.promotion_bits;q.commands_before=staged.promotion_vector_sets;
  const auto active=detail::bnw_game_active_player_read(*p->state);
  if(q.exotic>0){const auto after=narrow(std::int64_t(staged.scalars.at("m_iNumExoticGoods"))+q.exotic);need(after>=0,"Native exotic goods negative count");assign_scalar(staged,"m_iNumExoticGoods",after);}
  q.after=staged.scalars;q.indexed_after=staged.indexed;q.dirty_after=staged.dirty;q.bits_after=staged.promotion_bits;q.commands_after=staged.promotion_vector_sets;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.indexed_after);const auto&binding=*p->initial_hidden_binding;std::vector<std::uint64_t>post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.indexed_after,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto indexed=q.indexed_after;auto dirty=q.dirty_after;auto bits=q.bits_after;auto commands=q.commands_after;
  need(initial_exotic_goods_info(*p->host,*q.node)==q.exotic&&initial_xp_game_handicap(*p->state)==q.handicap&&initial_xp_handicap_info(*p->host,q.handicap)==std::pair{q.xp,q.xp_percent},"Actual exotic/XP Info changed during preparation");
  if(q.human){need(detail::bnw_player_human_read(*p->state,p->actor,p->host->database())==*q.human,"Actual initial XP human predicate changed");}
  need(p->state->world.pregame->archive.integers==q.pregame_integers&&p->state->world.pregame->archive.integer_arrays==q.pregame_arrays&&p->state->setup.game_type==q.game_type&&p->state->setup.pitboss==q.pitboss,"Actual initial XP human source changed during preparation");
  need(q.input->initial_center_selection()==selected&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision,"Actual Trait promotion selection changed during preparation");
  p->initial_exotic_xp.emplace(std::move(q));auto&stage=*p->initial_exotic_xp;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_embark_promotions_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  need(initial_xp_game_handicap(*p->state)==stage.handicap&&p->state->world.pregame&&p->state->world.pregame->archive.integers==stage.pregame_integers&&p->state->world.pregame->archive.integer_arrays==stage.pregame_arrays&&p->state->setup.game_type==stage.game_type&&p->state->setup.pitboss==stage.pitboss&&stage.node->scalars==stage.before&&stage.node->indexed==stage.indexed_before&&stage.node->dirty==stage.dirty_before&&stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_vector_sets==stage.commands_before&&stage.input->initial_center_revision_without_callbacks()==stage.input_revision&&detail::bnw_game_active_player_read(*p->state)==active&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&&p->host->session().source()==p->source&&!p->bank->retired.load(),"Actual Exotic/XP final operands changed before stores");
  stage.node->scalars.swap(scalars);stage.node->indexed.swap(indexed);stage.node->dirty.swap(dirty);stage.node->promotion_bits.swap(bits);stage.node->promotion_vector_sets.swap(commands);
  p->initial_city_strength->exotic_xp_inputs.swap(city);p->initial_city_strength->exotic_xp_stored=true;p->initial_post_city->exotic_xp_seal.swap(post);p->initial_center_plan->storage_->exotic_xp_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_exotic_xp_return();require_initial_exotic_xp_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}




namespace {
struct InitialMovesUnitInfo {std::int32_t moves=0,domain=-1,combat=-1,immobile=0;bool operator==(const InitialMovesUnitInfo&)const=default;};
InitialMovesUnitInfo initial_moves_unit_info(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 InitialMovesUnitInfo result;bool found=false;const content::InfoTypeEntry*unit=nullptr;
 for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="Units"&&e.id==node.scalars.at("m_eUnitType")){need(!unit,"Initial maxMoves duplicate UnitInfo ID");unit=&e;}}
 need(unit&&unit->hash==content::saved_info_type_hash(unit->type),"Initial maxMoves native UnitInfo binding absent");
 const auto rows=host.database().query_rows({"SELECT ID,Type,Moves,Domain,CombatClass,Immobile FROM Units WHERE ID=?",{std::int64_t(unit->id)}},[&](const auto&r){
  need(!found&&r.integer("ID")==unit->id&&r.text("Type")==std::optional<std::string>{unit->type},"Initial maxMoves selected UnitInfo differs");found=true;result.moves=narrow(r.integer("Moves"));result.immobile=narrow(r.integer("Immobile"));need(result.immobile==0||result.immobile==1,"Initial native Immobile bool invalid");
  const auto d=r.text("Domain");const auto*domain=d?host.types().find(*d):nullptr;constexpr std::array<std::string_view,5>names={"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
  need(domain&&domain->table=="Domains"&&domain->id>=0&&domain->id<5&&domain->type==names.at(static_cast<std::size_t>(domain->id))&&domain->hash==content::saved_info_type_hash(domain->type),"Initial maxMoves native Domain enum differs");result.domain=narrow(domain->id);
  const auto key=r.text("CombatClass");if(key&&!key->empty()){const auto*combat=host.types().find(*key);need(combat&&combat->table=="UnitCombatInfos"&&combat->id>=0&&combat->hash==content::saved_info_type_hash(combat->type),"Initial maxMoves native UnitCombat Info absent");result.combat=narrow(combat->id);}
 });need(rows.all_succeeded()&&found,"Initial maxMoves actual UnitInfo query unavailable");need(result.domain!=4,"Mandatory native dynamic DOMAIN_HOVER plot owner unavailable");return result;
}
const saves::PlayerTraitsField&initial_moves_trait_field(const LiveSessionState&s,std::int32_t actor,std::string_view key){
 need(s.world.all_players&&actor>=0&&static_cast<std::size_t>(actor)<s.world.all_players->players.size(),"Initial maxMoves actual player absent");const auto&p=s.world.all_players->players.at(static_cast<std::size_t>(actor));need(p.saved_player_id==actor&&p.traits.layout==bnw,"Initial maxMoves actual Traits source differs");return garrison_named(p.traits.fields,key);
}
std::int32_t initial_moves_denominator(const ui::SelectedBnwPreviewHost&host){const auto*d=host.defines().find("MOVE_DENOMINATOR");need(host.defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),"Initial maxMoves native MOVE_DENOMINATOR absent");return narrow(std::get<std::int64_t>(d->lookup.value));}
}
bool BnwNativeUnitAllocatorCore::initial_exotic_xp_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_exotic_xp&&p->initial_exotic_xp->stored,"Actual Exotic/XP receipt absent");const auto&q=*p->initial_exotic_xp;
 if(p->initial_immobile_moves&&p->initial_immobile_moves->stored){require_initial_immobile_moves_without_callbacks();const auto&r=*p->initial_immobile_moves;return r.node.get()==q.node.get()&&r.before==q.after&&r.indexed_before==q.indexed_after&&r.dirty_before==q.dirty_after&&r.bits_before==q.bits_after&&r.commands_before==q.commands_after;}
 return initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&q.node->indexed==q.indexed_after&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after&&q.node->promotion_vector_sets==q.commands_after;
}
void BnwNativeUnitAllocatorCore::require_initial_immobile_moves_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_immobile_moves&&p->initial_immobile_moves->stored&&p->train_source&&p->initial_set_xy,"Actual Immobile/maxMoves return missing");const auto&q=*p->initial_immobile_moves;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&p->facade_token->facade==&session&&p->facade_token->thread==p->thread&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  p->bank->pending_units.contains(p->bank->pending_slot)&&p->bank->pending_units.at(p->bank->pending_slot).get()==q.node.get()&&!p->bank->pending_units.at(p->bank->pending_slot).owner_before(q.node)&&!q.node.owner_before(p->bank->pending_units.at(p->bank->pending_slot)),"Actual Immobile/maxMoves SAME source/node changed");
 need(post_player_team(*p->state,p->actor)==q.team&&post_player(*p->state,p->actor,"m_iGoldenAgeTurns")==q.golden_age&&p->state->world.teams&&&post_named(post_team(*p->state,q.team).arrays,"extra_moves")==q.team_moves&&q.team_moves->columns==1&&q.team_moves->values==q.team_moves_before,"Actual maxMoves Team/golden-age source changed");
 for(std::size_t i=0;i<q.trait_fields.size();++i){need(&initial_moves_trait_field(*p->state,p->actor,q.trait_before[i].name)==q.trait_fields[i]&&initial_embark_trait_raw_matches(*q.trait_fields[i],q.trait_before[i]),"Actual maxMoves Trait cache changed");}
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(initial_immobile_moves_node_matches_without_callbacks(),"Actual Immobile/maxMoves immutable successor stores changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(q.input&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision&&prefix.initial_immobile_moves_computed&&prefix.initial_moves_before==q.moves_before&&prefix.initial_moves_requested==q.max_moves&&(prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner||((prefix.boundary==BnwUnitMoveBoundary::initial_moves_selection_owner||prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner)&&prefix.initial_moves_setter_returned)),"Actual Immobile/maxMoves source/client return changed");
 need(session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&!p->bank->retired.load(),"Actual Immobile/maxMoves final noalloc source changed");
}


BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::ShouldDimCoreBorrowedOwner(std::shared_ptr<const BnwNativeUnitAllocatorCore>owner)
 :owner_(std::move(owner)){
 need(owner_&&owner_->impl_&&owner_->impl_->thread==std::this_thread::get_id(),"Foreign ShouldDim Core pin before mutable borrow");
 owner_->should_dim_borrowed_core_pins_.fetch_add(1);
 try{(void)current();}catch(...){owner_->should_dim_borrowed_core_pins_.fetch_sub(1);owner_.reset();throw;}
}
std::unique_ptr<BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner>
BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak){
 auto owner=weak.lock();need(bool(owner),"ShouldDim canonical Core expired before counted pin");
 return std::unique_ptr<ShouldDimCoreBorrowedOwner>(new ShouldDimCoreBorrowedOwner(std::move(owner)));
}
BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::~ShouldDimCoreBorrowedOwner(){
 if(owner_){owner_->should_dim_borrowed_core_pins_.fetch_sub(1);owner_.reset();}
}
const BnwNativeUnitAllocatorCore&BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::current()const{
 need(bool(owner_),"ShouldDim counted Core owner absent");owner_->require_should_dim_outside_owner_without_callbacks();return *owner_;
}

void BnwNativeUnitAllocatorCore::require_native_common_pathfinder_session_without_callbacks(const LiveSession&session)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign native path Core before session/source");
 require_should_dim_outside_owner_without_callbacks();
 need(p->host&&p->host->session_.get()==&session&&p->state.get()==session.state_.get()&&p->source==session.source_.get()&&p->generation==session.state_generation_&&
  p->bank&&!p->bank->retired.load()&&p->bank->source.lock().get()==p->state.get()&&p->bank->generation==p->generation&&p->bank->fingerprint==p->fingerprint,
  "Native common path actual canonical Core/session binding differs");
 need(p->registry_installed,"Native common path requires actual installed allocator registry");
 const auto*registry=p->state->setup.extensions.find<Registry>();
 need(registry&&registry->players.contains(p->actor)&&registry->players.at(p->actor).get()==p->bank.get()&&
  !registry->players.at(p->actor).owner_before(p->bank)&&!p->bank.owner_before(registry->players.at(p->actor)),"Native common path SAME actor Bank replaced");
}
void BnwNativeUnitAllocatorCore::require_native_common_pathfinder_binding(const LiveSession&session)const{
 require_native_common_pathfinder_session_without_callbacks(session);impl_->check();require_native_common_pathfinder_session_without_callbacks(session);
}

void BnwNativeUnitAllocatorCore::require_should_dim_outside_owner_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign ShouldDim Core outside-owner guard before source read");
 const auto actual=factory_self_.lock();
 // The one local checker lock is excluded in addition to all registered pins.
 // This guard calls no providers and cannot promote temporary ownership.
 need(actual&&actual.get()==this&&actual.use_count()>static_cast<long>(should_dim_borrowed_core_pins_.load()+1),
  "ShouldDim borrowed Core pins cannot replace independent facade owner");
 const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&
  p->facade_token->thread==p->thread&&p->facade_token->facade==&session&&p->bank&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation,
  "ShouldDim actual independent facade/source retired");
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_moves_prior_input_revision_without_callbacks(
 const std::shared_ptr<ui::BnwClientSelectionControl>&input)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&input,"Foreign actual moves UI lineage before source read");
 if(p->initial_moves_ui_tail&&p->initial_moves_ui_tail->dirty_count){const auto&q=*p->initial_moves_ui_tail;
  need(input.get()==q.input.get()&&!input.owner_before(q.input)&&!q.input.owner_before(input)&&
   p->initial_set_moves&&p->initial_set_moves->returned&&p->initial_set_moves->changed&&
   q.dirty_count<=2&&q.current.client_revision==q.before.client_revision+q.dirty_count&&
   q.current.next_occurrence==q.before.next_occurrence+q.dirty_count&&
   q.current.events.size()==q.before.events.size()+q.dirty_count&&
   initial_best_land_prior_unit_dirty_matches_without_callbacks(q.input,q.current),"Actual moves UI exact earned dirty successor changed");
  const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
  need(prefix.initial_moves_unit_info_occurrence==q.unit_info_occurrence&&prefix.initial_moves_plot_list_occurrence==q.plot_list_occurrence,
   "Actual moves UI private returned occurrences differ from Pending");
  return q.before.client_revision;
 }
 if(p->initial_best_land&&p->initial_best_land->stored){const auto&b=*p->initial_best_land;
  need(initial_best_land_prior_unit_dirty_matches_without_callbacks(input,b.before),"Best land prior input receipt changed");return b.before.client_revision;}
 if(p->initial_active_game_data&&p->initial_active_game_data->stored){const auto&g=*p->initial_active_game_data;need(initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(input,g.before),"Initial GameData prior input receipt changed");return g.before.client_revision;}
 return input->initial_center_revision_without_callbacks();
}
BnwNativeUnitAllocatorCore::InitialMovesDirtyOperands
BnwNativeUnitAllocatorCore::initial_moves_dirty_operands(const ui::BnwClientSelectionControl&receiver,ui::BnwClientUnitDirtyKind kind)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_moves_ui_tail,"Foreign/unissued moves UI setter");
 const auto&input=p->initial_moves_ui_tail->input;
 need(input&&input.get()==&receiver&&p->initial_set_moves&&input.get()==p->initial_set_moves->input.get()&&
  !input.owner_before(p->initial_set_moves->input)&&!p->initial_set_moves->input.owner_before(input)&&
  !input.owner_before(p->host->session_->city_selection_)&&!p->host->session_->city_selection_.owner_before(input),
  "Moves UI setter receiving Input is not canonical SAME session/source owner");
 require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);const auto&q=*p->initial_moves_ui_tail;
 const bool info=kind==ui::BnwClientUnitDirtyKind::unit_info;
 need(!q.skipped&&!q.returned&&q.selection_read&&q.input&&p->initial_set_moves->changed&&
  (info?(q.phase==InitialMovesUiPhase::unit_info&&q.unit_selected&&q.common_reset&&!q.unit_info_occurrence):
   (kind==ui::BnwClientUnitDirtyKind::plot_list_buttons&&q.phase==InitialMovesUiPhase::plot_list&&q.plot_read&&
    q.selection_plot==std::optional{p->initial_set_moves->plot}&&!q.plot_list_occurrence)),
  "Native moves UI setter is not the earned exact selected/plot branch");
 need(q.next.client_revision==q.current.client_revision+1&&q.next.next_occurrence==q.current.next_occurrence+1&&
  q.next.events.size()==q.current.events.size()+1&&initial_best_land_prior_unit_dirty_matches_without_callbacks(q.input,q.current),
  "Actual moves UI staged event/current source differs");
 const auto&event=q.next.events.back();const auto&node=*p->initial_set_moves->node;
 need(event.kind==kind&&event.occurrence==q.current.next_occurrence&&event.client_revision==q.next.client_revision&&
  event.generation==p->generation&&event.world_revision==p->initial_set_moves->revision&&event.owner==p->actor&&
  event.full_id==node.scalars.at("m_iID")&&event.move_occurrence==0,"Moves UI authentic staged operands changed");
 return {p->actor,node.scalars.at("m_iID"),p->generation,p->initial_set_moves->revision,q.current.client_revision,event.occurrence,kind};
}
void BnwNativeUnitAllocatorCore::record_initial_moves_ui_dirty_store(ui::BnwClientUnitDirtyKind kind,std::uint64_t occurrence)const noexcept{
 auto&q=*impl_->initial_moves_ui_tail;
 q.current.unit_info=q.next.unit_info;q.current.plot_list_buttons=q.next.plot_list_buttons;
 q.current.next_occurrence=q.next.next_occurrence;q.current.client_revision=q.next.client_revision;q.current.events.swap(q.next.events);
 ++q.dirty_count;
 const bool info=kind==ui::BnwClientUnitDirtyKind::unit_info;
 if(info){q.unit_info_occurrence=occurrence;q.phase=InitialMovesUiPhase::selection_plot;}
 else{q.plot_list_occurrence=occurrence;q.phase=InitialMovesUiPhase::returned;}
 impl_->initial_set_xy->record_initial_moves_ui_dirty(info,occurrence);
}
void BnwNativeUnitAllocatorCore::require_initial_set_moves_tail_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_moves_ui_tail&&p->initial_moves_ui_tail->returned,
  "Actual full setMoves client tail did not return");
 require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);const auto&q=*p->initial_moves_ui_tail;const auto&r=*p->initial_set_moves;
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(prefix.initial_moves_tail_returned&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner&&q.skipped==!r.changed,
  "Actual complete setMoves tail or native unchanged bypass journal changed");
 if(q.skipped){need(!q.selection_read&&!q.plot_read&&!q.common_reset&&!q.dirty_count&&!q.unit_info_occurrence&&!q.plot_list_occurrence&&
  !prefix.initial_moves_selection_checked&&!prefix.initial_moves_selection_plot_checked,"Unchanged native setter fabricated UI query/effect");return;}
 need(q.selection_read&&q.plot_read&&q.common_reset==q.unit_selected&&prefix.initial_moves_selection_checked&&
  prefix.initial_moves_unit_selected==q.unit_selected&&prefix.initial_moves_common_reset==q.common_reset&&
  prefix.initial_moves_selection_plot_checked&&prefix.initial_moves_selected_plot==q.selection_plot&&
  bool(q.unit_info_occurrence)==q.unit_selected&&bool(q.plot_list_occurrence)==(q.selection_plot==std::optional{r.plot})&&
  q.dirty_count==std::size_t(q.unit_info_occurrence!=0)+std::size_t(q.plot_list_occurrence!=0),
  "Actual full setMoves ordered selection/ForceReset/UI effects differ");
 if(q.dirty_count)(void)initial_moves_prior_input_revision_without_callbacks(q.input);
 else need(initial_best_land_prior_unit_dirty_matches_without_callbacks(q.input,q.before),"Native no-dirty setMoves exact prior input receipt changed");
}

BnwUnitMoveBoundary BnwNativeUnitAllocatorCore::initial_set_moves_earned_frontier_without_callbacks(const BnwUnitMovePending&pending)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign TRAIN moves frontier before source read");
 require_should_dim_outside_owner_without_callbacks();
 need(p->initial_set_xy.get()==&pending&&p->initial_immobile_moves&&p->initial_immobile_moves->stored,
  "TRAIN moves frontier lacks canonical Pending/computed native receipt");
 pending.require_initial_visibility_source_without_callbacks();
 if(!p->initial_set_moves){
  require_initial_immobile_moves_without_callbacks();const auto&prefix=pending.initial_setxy_tail_prefix_without_callbacks();
  need(prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner&&prefix.initial_immobile_moves_computed&&
   !prefix.initial_moves_stored&&!prefix.initial_moves_setter_returned&&!prefix.initial_should_dim_enqueue_returned&&
   !prefix.initial_moves_selection_checked&&!prefix.initial_moves_tail_returned,
   "TRAIN computed moves frontier has unearned future effects");
  return BnwUnitMoveBoundary::initial_moves_dim_owner;
 }
 // A real archive store waiting for Game/queue return is NOT the completed
 // computed frontier. Never admit this in-flight state using its old enum.
 need(p->initial_set_moves->stored&&p->initial_set_moves->returned,
  "TRAIN setMoves store/ShouldDim call still in flight, no earned handoff");
 require_initial_should_dim_return_without_callbacks(pending);
 if(p->initial_moves_ui_tail){
  need(p->initial_moves_ui_tail->returned,"TRAIN setMoves mandatory selection/UI tail still in flight");
  require_initial_set_moves_tail_return_without_callbacks();
  if(p->initial_religion)require_initial_religion_return_without_callbacks();
  if(p->initial_tourism)require_initial_tourism_return_without_callbacks();
  if(p->initial_recon)require_initial_recon_return_without_callbacks();
  if(p->initial_best_land)require_initial_best_land_return_without_callbacks();
  if(p->initial_active_game_data)require_initial_active_game_data_return_without_callbacks();
  if(p->initial_unit_cycler)require_initial_unit_cycler_return_without_callbacks();
  if(p->initial_world_unit_message)require_initial_world_unit_message_return_without_callbacks();
  if(p->initial_second_city_strength)require_initial_second_city_strength_return_without_callbacks();
  if(p->initial_army_ai)require_initial_army_ai_return_without_callbacks();
  if(p->initial_maintenance)require_initial_maintenance_return_without_callbacks();
  if(p->initial_minor_quest)require_initial_minor_quest_return_without_callbacks();
  return BnwUnitMoveBoundary::initial_post_moves_owner;
 }
 const auto&prefix=pending.initial_setxy_tail_prefix_without_callbacks();
 need(!prefix.initial_moves_tail_returned&&!prefix.initial_moves_selection_checked&&!prefix.initial_moves_selection_plot_checked&&
  prefix.boundary==(p->initial_set_moves->changed?BnwUnitMoveBoundary::initial_moves_selection_owner:BnwUnitMoveBoundary::initial_post_moves_owner),
  "TRAIN genuine ShouldDim/no-change frontier journal differs");
 return prefix.boundary;
}

std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_set_moves_selection_and_plot_dirty_tail()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);
 need(p->initial_set_moves&&p->initial_set_moves->returned&&!p->initial_moves_ui_tail,
  "Native setMoves UI tail requires earned ShouldDim/no-change return exactly once");
 try{
  require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);
  InitialMovesUiTail q;q.input=p->initial_set_moves->input;
  if(!p->initial_set_moves->changed){
   // SDK equal-value path already returned without IsSelected/GetSelectionPlot.
   q.skipped=true;q.returned=true;q.phase=InitialMovesUiPhase::returned;p->initial_moves_ui_tail.emplace(std::move(q));
   p->initial_set_xy->record_initial_moves_tail_return();require_initial_set_moves_tail_return_without_callbacks();return p->initial_set_xy;
  }
  p->check();q.before=q.input->unit_dirty_state();q.current=q.before;
  need(q.before.client_revision==p->initial_set_moves->input_revision,"Actual post-ShouldDim client revision changed");
  // DLLUI::IsUnitSelected compares the actual selected identity. The pending
  // newborn is not falsely resolved as an imported/public raw roster member.
  q.selected=q.input->initial_center_selection();const auto&node=*p->initial_set_moves->node;
  q.unit_selected=q.selected&&q.selected->first==node.scalars.at("m_eOwner")&&q.selected->second==node.scalars.at("m_iID");
  require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);
  need(q.input->initial_center_revision_without_callbacks()==q.before.client_revision,"Actual IsSelected changed client source");
  q.selection_read=true;q.phase=q.unit_selected?InitialMovesUiPhase::pathfinder:InitialMovesUiPhase::selection_plot;
  p->initial_moves_ui_tail.emplace(std::move(q));auto&stage=*p->initial_moves_ui_tail;
  p->initial_set_xy->record_initial_moves_selection(stage.unit_selected);
  const auto prepare_dirty=[&](ui::BnwClientUnitDirtyKind kind){
   require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);
   need(stage.input->initial_moves_dirty_matches_without_callbacks(stage.current)&&stage.current.client_revision<UINT64_MAX&&
    stage.current.next_occurrence>0&&stage.current.next_occurrence<UINT64_MAX&&stage.current.events.size()<stage.current.events.max_size(),
    "Actual moves dirty source/ordinal/revision/capacity exhausted");
   stage.next=stage.current;const bool info=kind==ui::BnwClientUnitDirtyKind::unit_info;
   stage.next.events.push_back({stage.current.next_occurrence,0,p->generation,p->initial_set_moves->revision,stage.current.client_revision+1,
    p->actor,node.scalars.at("m_iID"),kind,info?stage.current.unit_info:stage.current.plot_list_buttons});
   if(info)stage.next.unit_info=true;else stage.next.plot_list_buttons=true;
   ++stage.next.next_occurrence;++stage.next.client_revision;
  };
  if(stage.unit_selected){
   // Real retained Session/common pathfinder owner. Missing/retired service
   // throws BnwCommonPathfinderUnavailable; its mandatory effect is not skipped.
   p->host->session_->force_reset_bnw_common_pathfinder();stage.common_reset=true;
   p->initial_set_xy->record_initial_moves_common_reset();stage.phase=InitialMovesUiPhase::unit_info;
   require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);
   prepare_dirty(ui::BnwClientUnitDirtyKind::unit_info);stage.input->set_initial_moves_dirty(*canonical,ui::BnwClientUnitDirtyKind::unit_info);
   require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);
  }
  // Real nullable DLLUI selection plot AFTER UnitInfo, never default null.
  stage.selection_plot=stage.input->selection_plot();
  need(!stage.selection_plot||*stage.selection_plot<=INT32_MAX,"Native selection plot GetPlotIndex exceeds int32");
  require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);
  stage.plot_read=true;p->initial_set_xy->record_initial_moves_selection_plot(stage.selection_plot);
  if(stage.selection_plot==std::optional{p->initial_set_moves->plot}){
   stage.phase=InitialMovesUiPhase::plot_list;prepare_dirty(ui::BnwClientUnitDirtyKind::plot_list_buttons);
   stage.input->set_initial_moves_dirty(*canonical,ui::BnwClientUnitDirtyKind::plot_list_buttons);
   require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);
  }
  stage.phase=InitialMovesUiPhase::returned;stage.returned=true;p->initial_set_xy->record_initial_moves_tail_return();
  require_initial_set_moves_tail_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::InitialUnitShouldDimPlan(std::shared_ptr<Storage>s):storage_(std::move(s)){}
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::~InitialUnitShouldDimPlan(){retire_scope_without_callbacks();}
void BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::retire_scope_without_callbacks()const noexcept{
 if(storage_)storage_->retire();
}
void BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::require_owner_without_callbacks()const{
 need(storage_&&storage_->creator==std::this_thread::get_id(),"Foreign ShouldDim Plan before mutable borrow");
 need(storage_->alive.load(),"ShouldDim issuing Core scope retired");
 const auto actual=storage_->canonical.lock();
 need(actual&&actual.get()==this&&actual->storage_.get()==storage_.get(),"ShouldDim Plan is not canonical heap object");
 need(actual.use_count()>static_cast<long>(storage_->borrowers.load()+1),"ShouldDim counted pins cannot replace issuing Core owner");
 const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 need(&core->current()==storage_->binding.core,"ShouldDim issuing canonical Core expired");
}
std::shared_ptr<const BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan>
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::retain_factory_owner()const{
 require_owner_without_callbacks();return storage_->canonical.lock();
}
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner::BorrowedOwner(const InitialUnitShouldDimPlan&plan)
 :BorrowedOwner(plan.retain_factory_owner()){}
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner::BorrowedOwner(std::shared_ptr<const InitialUnitShouldDimPlan>owner)
 :owner_(std::move(owner)){
 need(owner_&&owner_->storage_&&owner_->storage_->creator==std::this_thread::get_id(),"Foreign ShouldDim counted owner acquisition");
 owner_->storage_->borrowers.fetch_add(1);
 try{(void)current();}catch(...){owner_->storage_->borrowers.fetch_sub(1);owner_.reset();throw;}
}
std::unique_ptr<BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner>
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner::from_weak(const std::weak_ptr<const InitialUnitShouldDimPlan>&weak){
 // Move the one temporary lock into the counted borrower. There is no second
 // outer strong owner whose existence could incorrectly pass outside-owner checks.
 auto owner=weak.lock();need(bool(owner),"ShouldDim weak canonical Plan expired");
 return std::unique_ptr<BorrowedOwner>(new BorrowedOwner(std::move(owner)));
}
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner::~BorrowedOwner(){
 if(owner_){owner_->storage_->borrowers.fetch_sub(1);owner_.reset();}
}
const BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan&
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::BorrowedOwner::current()const{
 need(bool(owner_),"ShouldDim counted pin absent");owner_->require_owner_without_callbacks();return *owner_;
}
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::Binding
BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::capture_binding()const{
 require_owner_without_callbacks();const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 core->current().require_initial_should_dim_plan_without_callbacks(*this,false);return storage_->binding;
}
void BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::require_before_moves_store_without_callbacks()const{
 require_owner_without_callbacks();const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 core->current().require_initial_should_dim_plan_without_callbacks(*this,false);
 need(core->current().impl_->initial_set_moves&&!core->current().impl_->initial_set_moves->stored,"ShouldDim preflight cannot grant a second moves store");
}
void BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::require_should_dim_enqueue_dispatch_without_callbacks()const{
 require_owner_without_callbacks();const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);core->current().require_initial_should_dim_plan_without_callbacks(*this,true);
}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::read_dispatch_owner_without_callbacks()const{
 require_should_dim_enqueue_dispatch_without_callbacks();const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 return core->current().impl_->initial_set_moves->node->scalars.at("m_eOwner");
}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::read_dispatch_full_id_without_callbacks()const{
 require_should_dim_enqueue_dispatch_without_callbacks();const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 return core->current().impl_->initial_set_moves->node->scalars.at("m_iID");
}
bool BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan::read_dispatch_should_dim_without_callbacks()const{
 require_should_dim_enqueue_dispatch_without_callbacks();const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 return core->current().impl_->initial_set_moves->node->scalars.at("m_iMoves")<=0;
}
void BnwNativeUnitAllocatorCore::require_initial_should_dim_plan_without_callbacks(const InitialUnitShouldDimPlan&plan,bool dispatch)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign ShouldDim Plan Core before mutable source read");
 need(plan.storage_&&plan.storage_->creator==p->thread&&plan.storage_->alive.load()&&p->initial_should_dim_plan.get()==&plan&&
  p->initial_should_dim_plan->storage_.get()==plan.storage_.get()&&p->initial_set_moves,"ShouldDim Plan was not issued by current Core scope");
 const auto&q=*plan.storage_;const auto&b=q.binding;const auto&r=*p->initial_set_moves;
 need(b.core==this&&!b.core_control.owner_before(factory_self_)&&!factory_self_.owner_before(b.core_control)&&
  b.host.get()==p->host.get()&&!b.host.owner_before(p->host)&&!p->host.owner_before(b.host)&&
  b.session==p->host->session_.get()&&b.state.get()==p->state.get()&&!b.state.owner_before(p->state)&&!p->state.owner_before(b.state)&&
  b.source==p->source&&b.pending.get()==p->initial_set_xy.get()&&!b.pending.owner_before(p->initial_set_xy)&&!p->initial_set_xy.owner_before(b.pending)&&
  b.bank==p->bank.get()&&!b.bank_control.owner_before(p->bank)&&!p->bank.owner_before(b.bank_control)&&
  b.node==r.node.get()&&!b.node_control.owner_before(r.node)&&!r.node.owner_before(b.node_control)&&
  b.owner==p->actor&&b.full_id==r.node->scalars.at("m_iID")&&b.owner==r.node->scalars.at("m_eOwner")&&b.slot==p->bank->pending_slot&&
  b.generation==p->generation&&b.world_revision==r.revision&&b.before_moves==r.before_moves&&b.requested_moves==r.requested_moves,
  "ShouldDim actual Core/Host/Session/Bank/node/snapshot controls differ");
 need(q.attempt&&q.scope&&q.scope->serial==b.attempt_serial&&q.scope->world==p->state.get(),"ShouldDim SAME source scope absent");
 q.attempt->require_scope_identity_without_callbacks(*q.scope);
 const auto pins=q.scope->control->transport_source_pins.load();
 need(q.scope->source_owner&&pins<=static_cast<std::uint64_t>(LONG_MAX)&&
  q.scope->source_owner.use_count()>static_cast<long>(pins),"ShouldDim counted Source pins cannot replace outside Source owner");
 if(r.stored)require_initial_set_moves_stored_without_callbacks();else require_initial_immobile_moves_without_callbacks();
 need(r.changed&&!r.returned&&(!dispatch||r.stored),"ShouldDim dispatch is not the earned changed setter stage");
}
bool BnwNativeUnitAllocatorCore::initial_immobile_moves_node_matches_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_immobile_moves,"Actual immutable computed moves receipt missing");
 const auto&q=*p->initial_immobile_moves;
 if(p->initial_set_moves&&p->initial_set_moves->stored){require_initial_set_moves_stored_without_callbacks();const auto&r=*p->initial_set_moves;
  return r.node.get()==q.node.get()&&r.before==q.after&&r.indexed_before==q.indexed_after&&r.dirty_before==q.dirty_after&&r.bits_before==q.bits_after&&r.commands_before==q.commands_after;}
 return initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&q.node->indexed==q.indexed_after&&initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after&&q.node->promotion_vector_sets==q.commands_after;
}
void BnwNativeUnitAllocatorCore::require_initial_set_moves_stored_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign actual moves store guard before source read");
 need(p->initial_set_moves&&p->initial_set_moves->stored&&p->initial_set_xy&&p->initial_immobile_moves&&p->initial_immobile_moves->stored,
  "Actual moves setter stage has not returned from private store");
 const auto&q=*p->initial_set_moves;const auto&old=*p->initial_immobile_moves;const auto&session=*p->host->session_;
 need(p->facade_token&&!p->facade_token->retired.load()&&!p->facade_token->destroyed.load()&&!p->facade_token->poisoned.load()&&
  p->facade_token->thread==p->thread&&p->facade_token->facade==&session&&!p->bank->retired.load()&&
  session.state_.get()==p->state.get()&&session.source_.get()==p->source&&session.state_generation_==p->generation&&initial_best_land_prior_world_revision_without_callbacks()==q.revision&&
  q.node.get()==old.node.get()&&q.before==old.after&&q.indexed_before==old.indexed_after&&q.dirty_before==old.dirty_after&&q.bits_before==old.bits_after&&q.commands_before==old.commands_after&&
  q.before_moves==old.moves_before&&q.requested_moves==old.max_moves&&initial_cycle_prior_scalars_matches_without_callbacks(q.node->scalars,q.after)&&q.node->indexed==q.indexed_after&&
  initial_army_prior_dirty_match_without_callbacks(q.node->dirty,q.dirty_after)&&q.node->promotion_bits==q.bits_after&&q.node->promotion_vector_sets==q.commands_after&&
  q.node->scalars.at("m_iMoves")==q.requested_moves&&q.node->scalars.at("m_eOwner")==p->actor,
  "Actual moves setter immutable link/source/node archive changed");
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(q.input&&q.input.get()==old.input.get()&&!q.input.owner_before(old.input)&&!old.input.owner_before(q.input)&&
  initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision,"Actual moves setter canonical client changed");
 if(q.changed){need(p->state->world.map_plots&&q.plot<p->state->world.map_plots->plots.size()&&
  &p->state->world.map_plots->plots.at(static_cast<std::size_t>(q.plot))==q.plot_source&&
  q.plot==p->state->world.map_header.grid.index(q.node->scalars.at("m_iX"),q.node->scalars.at("m_iY")),"Actual pre-store plot() result changed");}
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(prefix.initial_immobile_moves_computed&&prefix.initial_moves_before==q.before_moves&&prefix.initial_moves_requested==q.requested_moves&&
  prefix.initial_moves_value_changed==q.changed&&prefix.initial_moves_stored==q.changed&&
  ((!q.returned&&prefix.boundary==BnwUnitMoveBoundary::initial_moves_dim_owner&&!prefix.initial_moves_setter_returned&&!prefix.initial_should_dim_enqueue_returned)||
   (q.returned&&prefix.initial_moves_setter_returned&&prefix.initial_should_dim_enqueue_returned==q.changed&&
    prefix.boundary==((q.changed&&!(p->initial_moves_ui_tail&&p->initial_moves_ui_tail->returned))?BnwUnitMoveBoundary::initial_moves_selection_owner:BnwUnitMoveBoundary::initial_post_moves_owner))),
  "Actual moves setter Pending tail journal differs");
}
void BnwNativeUnitAllocatorCore::require_initial_should_dim_return_without_callbacks(const BnwUnitMovePending&pending)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign completed ShouldDim return before mutable read");
 require_should_dim_outside_owner_without_callbacks();
 need(p->initial_set_xy.get()==&pending&&p->initial_set_moves&&p->initial_set_moves->stored&&p->initial_set_moves->returned,
  "ShouldDim actual setter return not earned for this Pending");
 require_initial_immobile_moves_without_callbacks();const auto&q=*p->initial_set_moves;
 need(q.changed?bool(q.proof):!q.proof,"ShouldDim genuine proof/no-change bypass differs");
 BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*this,*p->initial_city_strength,true);
 // Deliberately no previous attempt current(), ScopeIdentity renewal, Plan
 // dispatch, cache fanout or Lua acknowledgement after synchronous return.
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_set_moves_and_should_dim_enqueue(
 const graphics::BnwInitialUnitShouldDimRendererLoan&loan,const BnwStockLuaAttemptLease&attempt)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_immobile_moves&&p->initial_immobile_moves->stored&&
  !p->initial_set_moves&&!p->initial_should_dim_plan&&p->initial_city_strength,"Actual setMoves requires computed maxMoves exactly once");
 try{
  p->check();require_initial_immobile_moves_without_callbacks();
  const auto&old=*p->initial_immobile_moves;InitialSetMovesPrefix q;
  q.node=old.node;q.input=old.input;q.revision=old.revision;q.input_revision=old.input_revision;
  q.before=old.after;q.indexed_before=old.indexed_after;q.dirty_before=old.dirty_after;q.bits_before=old.bits_after;q.commands_before=old.commands_after;
  q.before_moves=q.node->scalars.at("m_iMoves");q.requested_moves=old.max_moves;q.changed=q.before_moves!=q.requested_moves;
  // Native unchanged setter returns BEFORE plot(), gDLL, selection, UI and
  // attempt/Game suppliers. No Loan method, Game gate or fabricated proof.
  if(!q.changed){
   q.after=q.before;q.indexed_after=q.indexed_before;q.dirty_after=q.dirty_before;q.bits_after=q.bits_before;q.commands_after=q.commands_before;
   q.stored=true;q.returned=true;p->initial_set_moves.emplace(std::move(q));
   p->initial_set_xy->record_initial_moves_store(false);p->initial_set_xy->record_initial_should_dim_return(false);
   require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);return p->initial_set_xy;
  }
  // Native captures plot() before m_iMoves. Read the genuine map storage at
  // the canonical node coordinates and retain its identity, not a caller DTO.
  need(p->state->world.map_plots.has_value(),"Actual changed setMoves plot source absent");
  q.plot=p->state->world.map_header.grid.index(q.node->scalars.at("m_iX"),q.node->scalars.at("m_iY"));
  q.plot_source=&p->state->world.map_plots->plots.at(static_cast<std::size_t>(q.plot));
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  assign_scalar(staged,"m_iMoves",q.requested_moves);
  q.after=staged.scalars;q.indexed_after=staged.indexed;q.dirty_after=staged.dirty;q.bits_after=staged.promotion_bits;q.commands_after=staged.promotion_vector_sets;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.indexed_after);
  std::vector<std::uint64_t>post,center;const auto&hidden=*p->initial_hidden_binding;
  initial_post_city_semantics(*p->state,p->actor,hidden.x,hidden.y,q.after,q.indexed_after,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto indexed=q.indexed_after;auto dirty=q.dirty_after;auto bits=q.bits_after;auto commands=q.commands_after;
  auto s=std::make_shared<InitialUnitShouldDimPlan::Storage>();s->attempt=attempt;s->scope=attempt.pin_active_scope_without_callbacks();
  need(s->scope->world==p->state.get(),"ShouldDim actual attempt belongs to another world");
  auto&b=s->binding;b.core=this;b.core_control=canonical;b.host=p->host;b.session=p->host->session_.get();b.state=p->state;b.source=p->source;b.pending=p->initial_set_xy;
  b.bank=p->bank.get();b.bank_control=p->bank;b.node=q.node.get();b.node_control=q.node;b.owner=q.node->scalars.at("m_eOwner");b.full_id=q.node->scalars.at("m_iID");b.slot=p->bank->pending_slot;
  b.generation=p->generation;b.world_revision=q.revision;b.attempt_serial=s->scope->serial;b.before_moves=q.before_moves;b.requested_moves=q.requested_moves;
  p->initial_set_moves.emplace(std::move(q));
  p->initial_should_dim_plan=std::shared_ptr<const InitialUnitShouldDimPlan>(new InitialUnitShouldDimPlan(s));s->canonical=p->initial_should_dim_plan;
  // p is the real retained Impl and retains its canonical issued Plan for
  // the entire Guard/issuer call. No setter clears this private Plan. A raw
  // pointer here therefore stays valid through callbacks, while adding no
  // uncounted Plan strong pin. Retirement runs BEFORE those local owners die.
  struct ScopeRetirement{const InitialUnitShouldDimPlan*plan;~ScopeRetirement(){plan->retire_scope_without_callbacks();}}retirement{p->initial_should_dim_plan.get()};
  // No storage has changed yet. Genuine supplier preflight may refuse here.
  loan.preflight_should_dim_enqueue(*p->initial_should_dim_plan);
  p->initial_should_dim_plan->require_before_moves_store_without_callbacks();
  p->check();require_initial_immobile_moves_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  auto&stage=*p->initial_set_moves;
  // Last callback-free gate, then only noexcept swaps/records. Archive/dirty
  // values are the staged native assign_scalar result, not a receipt boolean.
  p->initial_should_dim_plan->require_before_moves_store_without_callbacks();
  need(stage.node->scalars==stage.before&&stage.node->indexed==stage.indexed_before&&stage.node->dirty==stage.dirty_before&&
   stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_vector_sets==stage.commands_before,
   "Actual setMoves preflight altered native pending storage");
  stage.node->scalars.swap(scalars);stage.node->indexed.swap(indexed);stage.node->dirty.swap(dirty);stage.node->promotion_bits.swap(bits);stage.node->promotion_vector_sets.swap(commands);
  p->initial_city_strength->set_moves_inputs.swap(city);p->initial_city_strength->set_moves_stored=true;
  p->initial_post_city->set_moves_seal.swap(post);p->initial_center_plan->storage_->set_moves_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_moves_store(true);
  p->initial_should_dim_plan->require_should_dim_enqueue_dispatch_without_callbacks();
  const auto supplied=loan.enqueue_should_dim(*p->initial_should_dim_plan);need(bool(supplied),"Actual ShouldDim queue/Game return proof absent");
  const auto proof=supplied->retain_factory_owner();need(proof.get()==supplied.get()&&!proof.owner_before(supplied)&&!supplied.owner_before(proof),"ShouldDim proof forged control block");
  proof->require_for_plan(*p->initial_should_dim_plan);loan.require_should_dim_enqueued_current(*p->initial_should_dim_plan,*proof);
  p->initial_should_dim_plan->require_should_dim_enqueue_dispatch_without_callbacks();
  stage.proof=proof;stage.returned=true;p->initial_set_xy->record_initial_should_dim_return(true);
  require_initial_should_dim_return_without_callbacks(*p->initial_set_xy);return p->initial_set_xy;
 }catch(...){if(p->initial_should_dim_plan)p->initial_should_dim_plan->retire_scope_without_callbacks();p->bank->retired=true;throw;}
}

std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_immobile_max_moves_prefix()const{
 const auto canonical=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_exotic_xp&&p->initial_exotic_xp->stored&&!p->initial_immobile_moves&&p->initial_city_strength,"Actual Immobile/maxMovess require UnitInfo loop return once");
 try{
  p->check();require_initial_exotic_xp_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialImmobileMovesPrefix q;q.node=p->bank->pending_units.at(p->bank->pending_slot);q.revision=initial_best_land_prior_world_revision_without_callbacks();
  const auto info=initial_moves_unit_info(*p->host,*q.node);q.info_moves=info.moves;q.domain=info.domain;q.combat=info.combat;q.immobile=info.immobile;
  need(q.node->scalars.at("m_iGreatGeneralCount")<=0,"Mandatory native GreatGeneral stack maxMoves owner unavailable");need(q.node->scalars.at("m_bEmbarked")==0,"Mandatory native embarked baseMoves Policy owner unavailable");
  q.traits=BnwMajorPlayerTraitsOwner::from_source_host(p->host,p->actor);q.team=post_player_team(*p->state,p->actor);const auto&team=post_team(*p->state,q.team);q.team_moves=&post_named(team.arrays,"extra_moves");
  need(p->state->world.teams->dimensions.domains==5&&q.team_moves->columns==1&&q.team_moves->values.size()==5,"Initial maxMoves genuine Team domain array differs");q.team_moves_before=q.team_moves->values;q.team_extra=narrow(q.team_moves->values.at(static_cast<std::size_t>(q.domain)));
  const auto cache=[&](const char*key,std::int32_t index=0){const auto&field=initial_moves_trait_field(*p->state,p->actor,key);const auto value=q.traits->cache_value(key,index);need(field.rows.empty()&&field.hashes.empty()&&index>=0&&static_cast<std::size_t>(index)<field.values.size()&&field.values[static_cast<std::size_t>(index)]==value,"Initial maxMoves actual paired Trait cache differs");q.trait_fields.push_back(&field);q.trait_before.push_back(field);return value;};
  if(q.domain==0){q.naval=q.node->scalars.at("m_iExtraNavalMoves");if(q.node->scalars.at("m_iBaseCombat")==0){q.naval=narrow(std::int64_t(q.naval)+cache("m_iExtraEmbarkMoves"));}}
  q.golden_age=post_player(*p->state,p->actor,"m_iGoldenAgeTurns");if(q.golden_age>0){q.golden_moves=cache("m_iGoldenAgeMoveChange");}
  if(q.combat!=-1){q.combat_moves=cache("m_paiMovesChangeUnitCombat",q.combat);}
  auto moves=narrow(std::int64_t(q.info_moves)+q.node->scalars.at("m_iExtraMoves"));moves=narrow(std::int64_t(moves)+q.team_extra);moves=narrow(std::int64_t(moves)+q.naval);moves=narrow(std::int64_t(moves)+q.golden_moves);moves=narrow(std::int64_t(moves)+q.combat_moves);
  q.denominator=initial_moves_denominator(*p->host);q.max_moves=narrow(std::int64_t(moves)*q.denominator);q.moves_before=q.node->scalars.at("m_iMoves");
  q.input=p->initial_center_plan->storage_->input;need(bool(q.input),"Actual Immobile/maxMoves canonical client absent");q.input_revision=q.input->initial_center_revision_without_callbacks();const auto selected=q.input->initial_center_selection();
  need(!selected||selected->first!=p->actor||selected->second!=q.node->scalars.at("m_iID"),"Mandatory selected newborn promotion client owner unavailable");
  PendingUnit staged=*q.node;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
  q.before=staged.scalars;q.indexed_before=staged.indexed;q.dirty_before=staged.dirty;q.bits_before=staged.promotion_bits;q.commands_before=staged.promotion_vector_sets;
  const auto active=detail::bnw_game_active_player_read(*p->state);
  // Native SetImmobile calls IsImmobile first. DOMAIN_IMMOBILE already true,
  // so the raw m_bImmobile flag is NOT written for that native short circuit.
  if(q.immobile&&q.domain!=3&&staged.scalars.at("m_bImmobile")==0){assign_scalar(staged,"m_bImmobile",1);}
  // m_iMoves is deliberately still the original value. Its authentic changed
  // setter must store first and perform ShouldDim + selection/UI in source order.
  q.after=staged.scalars;q.indexed_after=staged.indexed;q.dirty_after=staged.dirty;q.bits_after=staged.promotion_bits;q.commands_after=staged.promotion_vector_sets;
  auto city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.indexed_after);const auto&binding=*p->initial_hidden_binding;std::vector<std::uint64_t>post,center;
  initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.indexed_after,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  auto scalars=q.after;auto indexed=q.indexed_after;auto dirty=q.dirty_after;auto bits=q.bits_after;auto commands=q.commands_after;
  q.traits->require_current();need(initial_moves_unit_info(*p->host,*q.node)==info&&initial_moves_denominator(*p->host)==q.denominator&&post_player_team(*p->state,p->actor)==q.team&&post_player(*p->state,p->actor,"m_iGoldenAgeTurns")==q.golden_age&&&post_named(post_team(*p->state,q.team).arrays,"extra_moves")==q.team_moves&&q.team_moves->values==q.team_moves_before,"Actual initial maxMoves source changed during preparation");
  for(std::size_t i=0;i<q.trait_fields.size();++i){need(&initial_moves_trait_field(*p->state,p->actor,q.trait_before[i].name)==q.trait_fields[i]&&initial_embark_trait_raw_matches(*q.trait_fields[i],q.trait_before[i]),"Actual initial maxMoves Traits changed during preparation");}
  need(q.input->initial_center_selection()==selected&&initial_moves_prior_input_revision_without_callbacks(q.input)==q.input_revision,"Actual Trait promotion selection changed during preparation");
  p->initial_immobile_moves.emplace(std::move(q));auto&stage=*p->initial_immobile_moves;
  stage.traits->require_current();p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_exotic_xp_without_callbacks();
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*canonical,*p->initial_city_strength,true);
  for(std::size_t i=0;i<stage.trait_fields.size();++i){need(&initial_moves_trait_field(*p->state,p->actor,stage.trait_before[i].name)==stage.trait_fields[i]&&initial_embark_trait_raw_matches(*stage.trait_fields[i],stage.trait_before[i]),"Actual initial maxMoves Trait changed before stores");}
  need(post_player_team(*p->state,p->actor)==stage.team&&post_player(*p->state,p->actor,"m_iGoldenAgeTurns")==stage.golden_age&&&post_named(post_team(*p->state,stage.team).arrays,"extra_moves")==stage.team_moves&&stage.team_moves->values==stage.team_moves_before&&stage.node->scalars==stage.before&&stage.node->indexed==stage.indexed_before&&stage.node->dirty==stage.dirty_before&&stage.node->promotion_bits==stage.bits_before&&stage.node->promotion_vector_sets==stage.commands_before&&stage.input->initial_center_revision_without_callbacks()==stage.input_revision&&detail::bnw_game_active_player_read(*p->state)==active&&initial_best_land_prior_world_revision_without_callbacks()==stage.revision&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&&p->host->session().source()==p->source&&!p->bank->retired.load(),"Actual Immobile/maxMoves final operands changed before stores");
  stage.node->scalars.swap(scalars);stage.node->indexed.swap(indexed);stage.node->dirty.swap(dirty);stage.node->promotion_bits.swap(bits);stage.node->promotion_vector_sets.swap(commands);
  p->initial_city_strength->immobile_moves_inputs.swap(city);p->initial_city_strength->immobile_moves_stored=true;p->initial_post_city->immobile_moves_seal.swap(post);p->initial_center_plan->storage_->immobile_moves_seal.swap(center);stage.stored=true;
  p->initial_set_xy->record_initial_immobile_moves_computed(stage.moves_before,stage.max_moves);require_initial_immobile_moves_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}





namespace {
struct InitialReligionInfo {
 bool spread=false,remove=false;std::int32_t spreads=0,strength=0;
 bool operator==(const InitialReligionInfo&)const=default;
};
InitialReligionInfo initial_religion_info(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 InitialReligionInfo out;bool found=false;const content::InfoTypeEntry*unit=nullptr;
 for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="Units"&&e.id==node.scalars.at("m_eUnitType")){need(!unit,"Init religion duplicate actual UnitInfo ID");unit=&e;}}
 need(unit&&unit->hash==content::saved_info_type_hash(unit->type),"Init religion actual UnitInfo binding absent");
 const auto report=host.database().query_rows({"SELECT ID,Type,SpreadReligion,RemoveHeresy,ReligionSpreads,ReligiousStrength FROM Units WHERE ID=?",{std::int64_t(unit->id)}},[&](const auto&r){
  need(!found&&r.integer("ID")==unit->id&&r.text("Type")==std::optional<std::string>{unit->type},"Init religion actual UnitInfo query differs");
  found=true;out.spread=r.boolean("SpreadReligion");out.remove=r.boolean("RemoveHeresy");out.spreads=narrow(r.integer("ReligionSpreads"));out.strength=narrow(r.integer("ReligiousStrength"));
 });need(report.all_succeeded()&&found,"Init religion required UnitInfo query unavailable");return out;
}
struct InitialReligionCityRead {
 std::int32_t majority=-1,extra=0;bool city=false,extra_read=false;
 std::vector<std::uint64_t>seal;
 bool operator==(const InitialReligionCityRead&)const=default;
};
std::int32_t initial_religion_record_int(const auto&rows,std::string_view key){
 const auto&f=post_named(rows,key);need(name(f.encoding)=="i32"&&f.values.size()==1&&!f.wire_count&&f.children.empty(),"Init religion actual scalar shape unavailable");return narrow(f.values.front());
}
InitialReligionCityRead initial_religion_city_read(const LiveSessionState&s,const PendingUnit&node,bool religious){
 InitialReligionCityRead out;
 // Original nonreligious branch does not call plot/getPlotCity/majority/buildings.
 if(!religious)return out;
 need(s.world.map_plots&&s.world.map_projection,"Init religion actual map owners absent");
 const auto x=node.scalars.at("m_iX"),y=node.scalars.at("m_iY");const auto index=s.world.map_header.grid.index(x,y);
 const auto&raw=s.world.map_plots->plots.at(static_cast<std::size_t>(index));const auto&cell=s.world.map_projection->cells.at(static_cast<std::size_t>(index));
 const auto owner=narrow(post_named(raw.fields,"plot_city_owner").value),id=narrow(post_named(raw.fields,"plot_city_id").value);
 const auto emit=[&](auto v){out.seal.push_back(static_cast<std::uint64_t>(v));};
 emit(index);emit(reinterpret_cast<std::uintptr_t>(&raw));emit(reinterpret_cast<std::uintptr_t>(&cell));emit(owner);emit(id);
 need(raw.index==index&&cell.index==index&&raw.x==static_cast<std::uint64_t>(x)&&raw.y==static_cast<std::uint64_t>(y)&&cell.x==raw.x&&cell.y==raw.y&&
  cell.is_city==(owner!=-1)&&cell.plot_city.has_value()==(owner!=-1),"Init religion paired actual plot identity differs");
 if(owner==-1){need(id==-1,"Init religion null city pointer has nonnull ID");return out;}
 need(owner>=0&&owner<64&&id>=0&&cell.plot_city->owner==owner&&cell.plot_city->id==id,"Init religion actual plot city identity differs");
 const auto*city=resolve_bnw_city_registry(s.world,s.cities,owner,id);
 need(city&&city->tail.layout==bnw&&s.cities.players.contains(owner)&&s.cities.players.at(owner).cities.contains(id),"Init religion current city source absent");
 const auto&owned=s.cities.players.at(owner).cities.at(id);out.city=true;
 need(owned.owner==owner&&owned.id==id&&owned.x==x&&owned.y==y&&garrison_named(city->core.fields,"m_iX").values.at(0)==x&&garrison_named(city->core.fields,"m_iY").values.at(0)==y,"Init religion city source coordinates differ");
 emit(reinterpret_cast<std::uintptr_t>(city));emit(reinterpret_cast<std::uintptr_t>(&owned));
 const auto&statuses=post_named(post_named(city->tail.blocks,"religions").children,"m_ReligionStatus");
 need(name(statuses.encoding)=="signed_records"&&statuses.wire_count&&static_cast<std::uint64_t>(std::max(0,narrow(*statuses.wire_count)))==statuses.children.size()&&statuses.children.size()==owned.religions.size(),"Init religion paired current status roster differs");
 emit(reinterpret_cast<std::uintptr_t>(&statuses));emit(*statuses.wire_count);emit(statuses.children.size());
 std::int32_t total=0,most=-1,pressure=0;
 for(std::size_t i=0;i<statuses.children.size();++i){const auto&row=statuses.children[i].children;
  const auto religion=initial_religion_record_int(row,"m_eReligion"),followers=initial_religion_record_int(row,"m_iFollowers"),p=initial_religion_record_int(row,"m_iPressure");const auto&mirror=owned.religions[i];
  need(mirror.religion==religion&&mirror.followers==followers&&mirror.pressure==p,"Init religion current status mirror stale");
  emit(religion);emit(followers);emit(p);total=narrow(std::int64_t(total)+followers);
  if(followers>most||(followers==most&&p>pressure)){most=followers;pressure=p;out.majority=religion;}
 }
 if(narrow(std::int64_t(most)*2)<total)out.majority=-1;
 // Pantheon/no-majority branches never query GetMissionaryExtraSpreads.
 if(out.majority>0){
  need(city->production.layout==bnw,"Init religion current buildings source unavailable");const auto&buildings=post_named(city->production.blocks,"buildings");
  const auto&version=post_named(buildings.fields,"version");need(version.value>=0&&version.value<=1,"Init religion CityBuildings version unsupported");
  const auto&extra=post_named(buildings.fields,"m_iMissionaryExtraSpreads");out.extra=narrow(extra.value);out.extra_read=true;
  emit(reinterpret_cast<std::uintptr_t>(&buildings));emit(version.value);emit(reinterpret_cast<std::uintptr_t>(&extra));emit(out.extra);
 }
 return out;
}
}
void BnwNativeUnitAllocatorCore::require_initial_religion_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init religion before source read");
 require_should_dim_outside_owner_without_callbacks();
 need(p->initial_religion&&p->initial_religion->returned&&p->initial_moves_ui_tail&&p->initial_moves_ui_tail->returned,"Init religion actual complete predecessor/return absent");
 require_initial_set_moves_tail_return_without_callbacks();const auto&q=*p->initial_religion;
 need(q.node.get()==p->initial_set_moves->node.get()&&q.node->registered_slot==p->bank->pending_slot&&
  std::array{q.node->religion,q.node->religion_strength,q.node->religion_spreads}==q.after&&
  initial_religion_city_read(*p->state,*q.node,q.spread||q.remove).seal==q.city_seal,
  "Init religion actual current node/city source changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();
 need(prefix.initial_religion_returned&&prefix.initial_religion_applied==q.applied&&prefix.initial_religion_majority==q.majority&&
  prefix.initial_religion_value==q.after[0]&&prefix.initial_religion_strength==q.after[1]&&prefix.initial_religion_spreads==q.after[2]&&
  prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init religion exact earned Pending return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_religion_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_set_xy&&p->initial_set_moves&&p->initial_moves_ui_tail&&p->initial_moves_ui_tail->returned&&!p->initial_religion,"Init religion requires once actual complete setMoves tail");
 try{
  p->check();require_initial_set_moves_tail_return_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  InitialReligionPrefix q;q.node=p->initial_set_moves->node;q.before={q.node->religion,q.node->religion_strength,q.node->religion_spreads};q.after=q.before;
  need(q.before==std::array<std::int32_t,3>{-1,0,0},"Init religion constructor-owned storage changed before first setters");
  const auto info=initial_religion_info(*p->host,*q.node);q.spread=info.spread;q.remove=info.remove;q.info_spreads=info.spreads;q.info_strength=info.strength;
  const auto city=initial_religion_city_read(*p->state,*q.node,info.spread||info.remove);q.majority=city.majority;q.city_seal=city.seal;
  q.applied=city.city&&city.majority>0;
  if(q.applied)q.after={city.majority,info.strength,narrow(std::int64_t(info.spreads)+city.extra)};
  need(initial_religion_info(*p->host,*q.node)==info&&initial_religion_city_read(*p->state,*q.node,info.spread||info.remove)==city,"Init religion actual source changed during preparation");
  p->initial_religion.emplace(std::move(q));auto&stage=*p->initial_religion;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_set_moves_tail_return_without_callbacks();
  need(std::array{stage.node->religion,stage.node->religion_strength,stage.node->religion_spreads}==stage.before&&
   initial_religion_city_read(*p->state,*stage.node,stage.spread||stage.remove)==city&&!p->bank->retired.load(),"Init religion final actual operands differ before stores");
  // SDK CvUnitReligion inline setters: no archive dirtying, Lua, UI or RNG.
  // Actual scalar stores preserve SetReligion -> SetSpreadsLeft -> Strength.
  if(stage.applied){stage.node->religion=stage.after[0];stage.node->religion_spreads=stage.after[2];stage.node->religion_strength=stage.after[1];}
  stage.returned=true;p->initial_set_xy->record_initial_religion_return(stage.applied,stage.majority,stage.after[0],stage.after[1],stage.after[2]);
  require_initial_religion_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
struct InitialTourismUnitInfo {std::int32_t one_shot=0,unit_class=-1;bool operator==(const InitialTourismUnitInfo&)const=default;};
InitialTourismUnitInfo initial_tourism_unit_info(const ui::SelectedBnwPreviewHost&host,const PendingUnit&node){
 InitialTourismUnitInfo out;bool found=false;const content::InfoTypeEntry*unit=nullptr;
 for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="Units"&&e.id==node.scalars.at("m_eUnitType")){need(!unit,"Init tourism duplicate UnitInfo");unit=&e;}}
 need(unit&&unit->hash==content::saved_info_type_hash(unit->type),"Init tourism actual UnitInfo absent");
 const auto report=host.database().query_rows({"SELECT ID,Type,OneShotTourism,Class FROM Units WHERE ID=?",{std::int64_t(unit->id)}},[&](const auto&r){
  need(!found&&r.integer("ID")==unit->id&&r.text("Type")==std::optional<std::string>{unit->type},"Init tourism selected UnitInfo differs");found=true;out.one_shot=narrow(r.integer("OneShotTourism"));
  const auto key=r.text("Class");const auto*cls=key?host.types().find(*key):nullptr;need(cls&&cls->table=="UnitClasses"&&cls->id>=0&&cls->hash==content::saved_info_type_hash(cls->type),"Init tourism actual UnitClass unresolved");out.unit_class=narrow(cls->id);
 });need(report.all_succeeded()&&found,"Init tourism UnitInfo source query failed");return out;
}
std::vector<std::uint64_t>initial_tourism_policy_seal(const LiveSessionState&s,std::int32_t actor,const BnwPolicyHappinessRules&rules,
 std::vector<bool>*has_out=nullptr,std::vector<bool>*blocked_out=nullptr){
 need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&actor>=0&&actor<64&&s.setup.players.contains(actor),"Init tourism actual policy player absent");
 const auto&raw=s.world.all_players->players.at(static_cast<std::size_t>(actor));need(raw.index==static_cast<std::uint64_t>(actor)&&raw.saved_player_id==actor&&raw.strategy.layout==bnw&&post_player(s,actor,"m_eID")==actor,"Init tourism actual policy identity differs");
 const auto&policy=post_named(raw.strategy.blocks,"policies");const auto version=post_named(policy.fields,"version").value;need(version==1||version==2,"Init tourism native policy version unsupported");
 const auto&has_wire=post_named(policy.hashes,"has_policy");const auto&blocked_wire=post_named(policy.hashes,"branch_blocked");
 auto has=rules.flags(has_wire,false),blocked=rules.flags(blocked_wire,true);std::set<std::int64_t>membership;
 for(std::size_t i=0;i<has.size();++i)if(has[i]){need(rules.policies().contains(static_cast<std::int64_t>(i)),"Init tourism owned NULL policy Info");membership.insert(static_cast<std::int64_t>(i));}
 const auto&owned=s.setup.players.at(actor);need(owned.policies_resolved&&owned.policies==membership,"Init tourism actual owned policy membership stale");
 std::vector<std::uint64_t>seal;const auto emit=[&](auto v){seal.push_back(static_cast<std::uint64_t>(v));};
 emit(reinterpret_cast<std::uintptr_t>(&raw));emit(reinterpret_cast<std::uintptr_t>(&policy));emit(reinterpret_cast<std::uintptr_t>(&owned));emit(version);
 for(const auto*wire:{&has_wire,&blocked_wire}){emit(reinterpret_cast<std::uintptr_t>(wire));emit(wire->wire_count);emit(wire->rows.size());for(const auto&r:wire->rows){emit(r.type_hash);emit(r.values.size());for(auto v:r.values)emit(v);}}
 emit(has.size());for(auto v:has)emit(bool(v));emit(blocked.size());for(auto v:blocked)emit(bool(v));emit(membership.size());for(auto id:membership)emit(id);
 if(has_out)*has_out=std::move(has);if(blocked_out)*blocked_out=std::move(blocked);return seal;
}
std::int32_t initial_tourism_creation_value(const ui::SelectedBnwPreviewHost&host,const BnwPolicyHappinessRules&rules,
 const std::vector<bool>&has,const std::vector<bool>&blocked,std::int32_t unit_class){
 const auto slots=native_init_info_span(host,"UnitClasses");need(unit_class>=0&&static_cast<std::uint64_t>(unit_class)<slots,"Init tourism actual UnitClass cache index invalid");
 std::optional<std::int32_t>max_rows;const auto max_report=host.database().query_rows({"SELECT max(rowid) FROM UnitClasses",{}},[&](const auto&r){need(!max_rows,"Init tourism duplicate MaxRows result");max_rows=narrow(std::int64_t(narrow(r.integer_at(0)))+1);});
 need(max_report.all_succeeded()&&max_rows&&*max_rows>=0&&static_cast<std::uint64_t>(*max_rows)==slots,"Init tourism native UnitClasses MaxRows span differs");
 std::int32_t total=0;
 // Native Info loop includes holes; false constructor-initialized ownership
 // is proved for them by flags(). Active entries alone execute the getter.
 for(std::size_t i=0;i<has.size();++i){if(!has[i])continue;const auto id=static_cast<std::int64_t>(i);const auto&rule=rules.policies().at(id);
  if(rule.blocked_branch>=0&&blocked.at(static_cast<std::size_t>(rule.blocked_branch)))continue;
  const content::InfoTypeEntry*policy=nullptr;for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="Policies"&&e.id==id){need(!policy,"Init tourism duplicate policy Info");policy=&e;}}
  need(policy&&policy->hash==content::saved_info_type_hash(policy->type),"Init tourism active policy Info unresolved");std::vector<std::int32_t>cache(slots,0);
  // Exact PopulateArrayByValue SQL/order: zero initialize, then SET each row.
  // Duplicate authored rows overwrite; they are not accumulated or deduped.
  const auto report=host.database().query_rows({"select UnitClasses.ID, Tourism from Policy_TourismOnUnitCreation inner join UnitClasses on UnitClassType = UnitClasses.Type where PolicyType = ?",{policy->type}},[&](const auto&r){const auto slot=narrow(r.integer_at(0)),value=narrow(r.integer_at(1));need(slot>=0&&static_cast<std::uint64_t>(slot)<slots,"Init tourism authored native cache index invalid");cache.at(static_cast<std::size_t>(slot))=value;});
  need(report.all_succeeded(),"Init tourism authored policy array query failed");const auto value=cache.at(static_cast<std::size_t>(unit_class));if(value>0)total=narrow(std::int64_t(total)+value);
 }return total;
}
}
void BnwNativeUnitAllocatorCore::require_initial_tourism_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init tourism before source read");require_should_dim_outside_owner_without_callbacks();
 need(p->initial_tourism&&p->initial_tourism->returned&&p->initial_tourism->rules,"Init tourism actual finite return absent");require_initial_religion_return_without_callbacks();const auto&q=*p->initial_tourism;
 need(q.node.get()==p->initial_religion->node.get()&&q.one_shot<=0&&q.creation==0&&
  initial_tourism_policy_seal(*p->state,p->actor,*q.rules)==q.policy_seal,"Init tourism actual source/zero receipt changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_tourism_returned&&prefix.initial_tourism_creation==q.creation&&prefix.initial_tourism_one_shot==q.one_shot&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init tourism earned Pending return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_tourism_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_religion&&p->initial_religion->returned&&!p->initial_tourism,"Init tourism requires once actual religion return");
 try{
  p->check();require_initial_religion_return_without_callbacks();const auto info=initial_tourism_unit_info(*p->host,*p->initial_religion->node);
  // A positive concert tour requires GetTourism's real city/league/traits/
  // policy modifiers before archive SetTourismBlastStrength. Never cached 0.
  need(info.one_shot<=0,"Mandatory actual city/player Culture GetTourism and blast setter owner required");
  InitialTourismPrefix q;q.node=p->initial_religion->node;q.one_shot=info.one_shot;q.unit_class=info.unit_class;
  q.rules=std::make_shared<const BnwPolicyHappinessRules>(BnwPolicyHappinessRules::load(p->host->database()));need(q.rules->rules_sha256()==p->host->identity().rules_sha256,"Init tourism actual policy rules fingerprint differs");
  std::vector<bool>has,blocked;q.policy_seal=initial_tourism_policy_seal(*p->state,p->actor,*q.rules,&has,&blocked);q.creation=initial_tourism_creation_value(*p->host,*q.rules,has,blocked,q.unit_class);
  need(q.creation==0,"Mandatory actual DiplomacyAI IsPlayerValid/PlayerCulture influence stores required for positive creation tourism");
  need(initial_tourism_unit_info(*p->host,*q.node)==info&&initial_tourism_creation_value(*p->host,*q.rules,has,blocked,q.unit_class)==q.creation&&initial_tourism_policy_seal(*p->state,p->actor,*q.rules)==q.policy_seal,"Init tourism actual preparation operands changed");
  p->initial_tourism.emplace(std::move(q));auto&stage=*p->initial_tourism;p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_religion_return_without_callbacks();
  need(initial_tourism_policy_seal(*p->state,p->actor,*stage.rules)==stage.policy_seal&&!p->bank->retired.load(),"Init tourism final actual policy source changed");
  stage.returned=true;p->initial_set_xy->record_initial_tourism_return(stage.one_shot,stage.creation);require_initial_tourism_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}
void BnwNativeUnitAllocatorCore::require_initial_recon_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init recon before source read");require_should_dim_outside_owner_without_callbacks();require_initial_tourism_return_without_callbacks();
 need(p->initial_recon&&p->initial_recon->returned,"Init recon actual finite return absent");const auto&q=*p->initial_recon;
 need(q.node.get()==p->initial_tourism->node.get()&&q.count<=0&&q.node->scalars.at("m_iReconCount")==q.count&&q.node->scalars.at("m_iReconX")==q.x&&q.node->scalars.at("m_iReconY")==q.y,"Init recon actual unchanged node receipt differs");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_recon_returned&&prefix.initial_recon_count==q.count&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init recon exact earned Pending return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_recon_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_tourism&&p->initial_tourism->returned&&!p->initial_recon,"Init recon requires once actual tourism return");
 try{
  p->check();require_initial_tourism_return_without_callbacks();InitialReconPrefix q;q.node=p->initial_tourism->node;q.count=q.node->scalars.at("m_iReconCount");q.x=q.node->scalars.at("m_iReconX");q.y=q.node->scalars.at("m_iReconY");
  need(q.count<=0,"Mandatory actual setReconPlot map recon count and adjacent sight owner required");
  // Native IsRecon false executes no setReconPlot, coordinates or sight calls.
  q.returned=true;p->initial_recon.emplace(std::move(q));p->initial_set_xy->record_initial_recon_return(p->initial_recon->count);require_initial_recon_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
std::int32_t initial_best_land_combat(const ui::SelectedBnwPreviewHost&host,std::int32_t best){
 need(best>=0,"Init best land combat requires nonnull native UnitType");const auto slots=native_init_info_span(host,"Units");need(static_cast<std::uint64_t>(best)<slots,"Init best land saved UnitType exceeds actual native Info span");
 const content::InfoTypeEntry*info=nullptr;for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="Units"&&e.id==best){need(!info,"Init best land duplicate native UnitInfo");info=&e;}}
 // Native sparse Info hole yields NULL, and Game returns combat strength1.
 if(!info)return 1;
 need(info->hash==content::saved_info_type_hash(info->type),"Init best land UnitInfo binding differs");std::optional<std::int32_t>combat;
 const auto report=host.database().query_rows({"SELECT ID,Type,Combat FROM Units WHERE ID=?",{std::int64_t(best)}},[&](const auto&r){need(!combat&&r.integer("ID")==best&&r.text("Type")==std::optional<std::string>{info->type},"Init best land selected UnitInfo differs");combat=narrow(r.integer("Combat"));});
 need(report.all_succeeded()&&combat,"Init best land actual UnitInfo unavailable");return std::max(1,*combat);
}
}
void BnwNativeUnitAllocatorCore::require_initial_best_land_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init best land before source read");require_should_dim_outside_owner_without_callbacks();require_initial_recon_return_without_callbacks();
 need(p->initial_best_land&&p->initial_best_land->returned,"Init best land actual finite return absent");const auto&q=*p->initial_best_land;
 need(q.node.get()==p->initial_recon->node.get()&&p->initial_immobile_moves&&q.domain==p->initial_immobile_moves->domain&&q.node->scalars.at("m_iBaseCombat")==q.base_combat&&q.node->scalars.at("m_eUnitType")==q.type,
  "Init best land actual Unit operands changed");
 if(q.game_read){need(p->state->world.game_state.layout==bnw&&&post_named(p->state->world.game_state.controls,"best_land_unit")==q.control&&q.control->value==(q.changed?q.type:q.best)&&q.node->scalars.at("m_bEmbarked")==0,
  "Init best land genuine unchanged Game source/node differs");}
 need(q.changed?(q.stored&&q.ui_stored&&q.occurrence&&q.input&&initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(q.input,q.current)):
  (!q.stored&&!q.ui_stored&&!q.occurrence),"Init best land real Game/EngineUI effect receipt differs");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_best_land_returned&&prefix.initial_best_land_game_read==q.game_read&&prefix.initial_best_land_setter_called==q.setter_called&&prefix.initial_best_land_type==(q.changed?q.type:q.best)&&prefix.initial_best_land_changed==q.changed&&prefix.initial_best_land_stored==q.stored&&prefix.initial_best_land_unit_info_occurrence==q.occurrence&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init best land earned Pending journal differs");
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_best_land_prior_world_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign best land lineage before mutable read");
 if(!p->initial_best_land||!p->initial_best_land->stored)return initial_second_city_prior_world_revision_without_callbacks();
 const auto&q=*p->initial_best_land;
 need(q.changed&&q.setter_called&&q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1&&initial_second_city_prior_world_revision_without_callbacks()==q.after_revision&&p->bank&&!p->bank->retired.load()&&
  p->state->world.game_state.layout==bnw&&&post_named(p->state->world.game_state.controls,"best_land_unit")==q.control&&q.control->value==q.type,
  "Best land actual Game store/earned world revision changed");return q.before_revision;
}
bool BnwNativeUnitAllocatorCore::initial_best_land_prior_unit_dirty_matches_without_callbacks(
 const std::shared_ptr<ui::BnwClientSelectionControl>&input,const ui::BnwClientUnitDirtySnapshot&expected)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&input,"Foreign best land UI lineage before source read");
 if(!p->initial_best_land||!p->initial_best_land->stored)return initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(input,expected);
 const auto&q=*p->initial_best_land;
 (void)initial_best_land_prior_world_revision_without_callbacks();
 need(q.input&&input.get()==q.input.get()&&!input.owner_before(q.input)&&!q.input.owner_before(input)&&
  q.current.client_revision==q.before.client_revision+(q.ui_stored?1:0)&&q.current.next_occurrence==q.before.next_occurrence+(q.ui_stored?1:0)&&
  q.current.events.size()==q.before.events.size()+(q.ui_stored?1:0)&&initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(q.input,q.current),
  "Best land actual exact UI event descendant changed");
 return expected.unit_info==q.before.unit_info&&expected.plot_list_buttons==q.before.plot_list_buttons&&expected.next_occurrence==q.before.next_occurrence&&expected.client_revision==q.before.client_revision&&expected.events==q.before.events;
}
BnwNativeUnitAllocatorCore::InitialMovesDirtyOperands BnwNativeUnitAllocatorCore::initial_best_land_dirty_operands(const ui::BnwClientSelectionControl&receiver)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_best_land,"Foreign/unissued best land UI setter");const auto&q=*p->initial_best_land;
 need(q.input&&q.input.get()==&receiver&&p->initial_set_moves&&q.input.get()==p->initial_set_moves->input.get()&&
  !q.input.owner_before(p->initial_set_moves->input)&&!p->initial_set_moves->input.owner_before(q.input)&&
  !q.input.owner_before(p->host->session_->city_selection_)&&!p->host->session_->city_selection_.owner_before(q.input),"Best land setter receiving UI is not actual canonical owner");
 require_should_dim_outside_owner_without_callbacks();require_initial_recon_return_without_callbacks();
 need(q.changed&&q.stored&&!q.ui_stored&&!q.returned&&!q.occurrence&&q.current.client_revision<UINT64_MAX&&q.current.next_occurrence>0&&q.current.next_occurrence<UINT64_MAX&&
  q.next.client_revision==q.current.client_revision+1&&q.next.next_occurrence==q.current.next_occurrence+1&&q.next.events.size()==q.current.events.size()+1&&initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(q.input,q.current),"Best land dirty setter not exact actual post-store phase");
 const auto&e=q.next.events.back();need(e.kind==ui::BnwClientUnitDirtyKind::unit_info&&e.occurrence==q.current.next_occurrence&&e.move_occurrence==0&&e.generation==p->generation&&e.world_revision==q.after_revision&&e.client_revision==q.next.client_revision&&e.owner==p->actor&&e.full_id==q.node->scalars.at("m_iID")&&e.previous_value==q.current.unit_info,"Best land actual staged UI operands differ");
 return {p->actor,q.node->scalars.at("m_iID"),p->generation,q.after_revision,q.current.client_revision,e.occurrence,ui::BnwClientUnitDirtyKind::unit_info};
}
void BnwNativeUnitAllocatorCore::record_initial_best_land_dirty_store(std::uint64_t occurrence)const noexcept{
 auto&q=*impl_->initial_best_land;q.current.unit_info=q.next.unit_info;q.current.plot_list_buttons=q.next.plot_list_buttons;q.current.next_occurrence=q.next.next_occurrence;q.current.client_revision=q.next.client_revision;q.current.events.swap(q.next.events);
 q.occurrence=occurrence;q.ui_stored=true;impl_->initial_set_xy->record_initial_best_land_dirty_store(occurrence);
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_best_land_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_recon&&p->initial_recon->returned&&!p->initial_best_land,"Init best land requires once actual recon return");
 try{
  p->check();require_initial_recon_return_without_callbacks();InitialBestLandPrefix q;q.node=p->initial_recon->node;q.domain=p->initial_immobile_moves->domain;q.base_combat=q.node->scalars.at("m_iBaseCombat");q.type=q.node->scalars.at("m_eUnitType");
  // SDK DOMAIN_LAND is2. Nonland and IsCombatUnit=false never call Game.
  if(q.domain==2&&q.base_combat>0){
   need(p->state->world.game_state.layout==bnw,"Init best land actual Game source absent");q.game_read=true;
   for(auto&f:p->state->world.game_state.controls)if(name(f.name)=="best_land_unit"){need(!q.control,"Init best land duplicate mutable Game field");q.control=&f;}
   need(q.control,"Init best land actual mutable Game field unavailable");q.best=narrow(q.control->value);
   if(q.best==-1)q.setter_called=true;
   else{
    need(q.node->scalars.at("m_bEmbarked")==0,"Mandatory actual embarked defense getter before best land comparison required");
    q.best_combat=initial_best_land_combat(*p->host,q.best);q.setter_called=q.base_combat>q.best_combat;
   }
   // Changed CvGame::setBestLandUnit stores m_eBestLandUnit then emits an
   // actual EngineUI UnitInfo19 dirty setter. Never stand in for that return.
   q.changed=q.setter_called&&q.type!=q.best;
   if(q.best!=-1)need(initial_best_land_combat(*p->host,q.best)==q.best_combat,"Init best land actual Info changed during preparation");
  }
  q.before_revision=initial_second_city_prior_world_revision_without_callbacks();q.after_revision=q.before_revision;
  if(q.changed){
   need(q.before_revision<UINT64_MAX,"Best land Game source revision exhausted");q.after_revision=q.before_revision+1;
   q.input=p->initial_set_moves->input;need(bool(q.input),"Best land actual UI owner missing");q.input->preflight_initial_center_info_dirty_setters(1);
   q.before=q.input->unit_dirty_state();q.current=q.before;q.next=q.before;
   need(q.before.client_revision<UINT64_MAX&&q.before.next_occurrence>0&&q.before.next_occurrence<UINT64_MAX&&q.before.events.size()<q.before.events.max_size(),"Best land actual UI capacity exhausted");
   q.next.unit_info=true;++q.next.client_revision;++q.next.next_occurrence;
   q.next.events.push_back({q.before.next_occurrence,0,p->generation,q.after_revision,q.next.client_revision,p->actor,q.node->scalars.at("m_iID"),ui::BnwClientUnitDirtyKind::unit_info,q.before.unit_info});
   initial_happiness_finish_semantics(*p->state,p->actor,[&](auto v){q.happiness_inputs.push_back(static_cast<std::uint64_t>(v));},{},nullptr,nullptr,q.control,q.type);
  }
  p->initial_best_land.emplace(std::move(q));auto&stage=*p->initial_best_land;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_recon_return_without_callbacks();
  need(stage.node->scalars.at("m_iBaseCombat")==stage.base_combat&&stage.node->scalars.at("m_eUnitType")==stage.type&&(!stage.game_read||(stage.control==&post_named(p->state->world.game_state.controls,"best_land_unit")&&stage.control->value==stage.best&&stage.node->scalars.at("m_bEmbarked")==0))&&!p->bank->retired.load(),"Init best land final actual operands changed");
  if(stage.changed){
   need(initial_second_city_prior_world_revision_without_callbacks()==stage.before_revision&&stage.input->initial_moves_dirty_matches_without_callbacks(stage.before),"Best land final actual Game/UI source changed before stores");
   // Native changed Game setter writes BEFORE EngineUI UnitInfo19. No
   // callbacks/allocations intervene in these earned Game state stores.
   stage.control->value=stage.type;p->state->ui_source_revision=stage.after_revision;stage.stored=true;
   p->initial_set_xy->record_initial_best_land_game_store(stage.type,stage.after_revision);
   const auto occurrence=stage.input->set_initial_best_land_dirty(*canonical);
   need(stage.ui_stored&&stage.occurrence==occurrence&&stage.input->initial_moves_dirty_matches_without_callbacks(stage.current),"Best land actual EngineUI normal return missing");
  }
  // All reached native branches have no effect: short circuit, comparison
  // false, or the true equality early return of Game::setBestLandUnit.
  stage.returned=true;p->initial_set_xy->record_initial_best_land_return(stage.game_read,stage.setter_called,stage.changed,stage.changed?stage.type:stage.best);require_initial_best_land_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

bool BnwNativeUnitAllocatorCore::initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(
 const std::shared_ptr<ui::BnwClientSelectionControl>&input,const ui::BnwClientUnitDirtySnapshot&expected)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&input,"Foreign initial GameData prior UI before source read");
 if(!p->initial_active_game_data||!p->initial_active_game_data->stored)return input->initial_moves_dirty_matches_without_callbacks(expected);
 const auto&q=*p->initial_active_game_data;
 need(q.active&&q.input&&input.get()==q.input.get()&&!input.owner_before(q.input)&&!q.input.owner_before(input)&&
  q.before_count<UINT64_MAX&&q.after_count==q.before_count+1&&q.before.client_revision<UINT64_MAX&&q.current.client_revision==q.before.client_revision+1&&
  q.current.unit_info==q.before.unit_info&&q.current.plot_list_buttons==q.before.plot_list_buttons&&q.current.next_occurrence==q.before.next_occurrence&&q.current.events==q.before.events&&
  initial_maintenance_prior_game_data_matches_without_callbacks(q.input,std::optional<bool>{true},q.after_count,q.current.client_revision)&&
  initial_maintenance_prior_unit_dirty_matches_without_callbacks(q.input,q.current),"Initial GameData exact earned physical UI descendant changed");
 return expected==q.before;
}
BnwNativeUnitAllocatorCore::InitialUnitGameDataDirtyOperands BnwNativeUnitAllocatorCore::initial_unit_game_data_dirty_operands(const ui::BnwClientSelectionControl&receiver)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_active_game_data,"Foreign/unissued initial GameData setter");const auto&q=*p->initial_active_game_data;
 need(q.input&&q.input.get()==&receiver&&p->initial_set_moves&&q.input.get()==p->initial_set_moves->input.get()&&
  !q.input.owner_before(p->initial_set_moves->input)&&!p->initial_set_moves->input.owner_before(q.input)&&
  !q.input.owner_before(p->host->session_->city_selection_)&&!p->host->session_->city_selection_.owner_before(q.input),"Initial GameData receiving UI is not actual canonical source");
 require_should_dim_outside_owner_without_callbacks();require_initial_best_land_return_without_callbacks();
 need(q.active&&!q.stored&&!q.returned&&q.node.get()==p->initial_best_land->node.get()&&q.node->scalars.at("m_eOwner")==p->actor&&
  detail::bnw_game_active_player_read(*p->state)==p->actor&&q.before_count<UINT64_MAX&&q.before.client_revision<UINT64_MAX&&
  q.input->initial_unit_game_data_matches_without_callbacks(q.before_flag,q.before_count,q.before.client_revision)&&
  q.input->initial_moves_dirty_matches_without_callbacks(q.before),"Initial GameData not exact earned active-player phase");
 return {p->actor,q.node->scalars.at("m_iID"),p->generation,q.world_revision,q.before.client_revision,q.before_count,q.before_flag};
}
void BnwNativeUnitAllocatorCore::record_initial_unit_game_data_dirty_store()const noexcept{
 auto&q=*impl_->initial_active_game_data;q.current.client_revision=q.before.client_revision+1;q.after_count=q.before_count+1;q.stored=true;
 impl_->initial_set_xy->record_initial_active_game_data_store(q.after_count);
}
void BnwNativeUnitAllocatorCore::require_initial_active_game_data_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_active_game_data&&p->initial_active_game_data->returned,"Initial active GameData actual return absent/foreign");
 require_should_dim_outside_owner_without_callbacks();require_initial_best_land_return_without_callbacks();const auto&q=*p->initial_active_game_data;
 need(q.node.get()==p->initial_best_land->node.get()&&q.node->scalars.at("m_eOwner")==p->actor&&detail::bnw_game_active_player_read(*p->state)==q.active_player&&q.active==(p->actor==q.active_player)&&initial_second_city_prior_world_revision_without_callbacks()==q.world_revision,
  "Initial active GameData actual actor/Game source changed");
 if(q.active)need(q.stored&&q.input&&initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(q.input,q.before),"Initial active GameData actual store/current UI return missing");
 else need(!q.stored&&!q.input&&!q.after_count,"Initial inactive branch invented a UI effect");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_active_game_data_returned&&prefix.initial_active_game_data_applied==q.active&&prefix.initial_active_game_data_occurrence==q.after_count&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Initial active GameData exact earned Pending return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_active_game_data_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_best_land&&p->initial_best_land->returned&&!p->initial_active_game_data,"Initial active GameData requires once actual best land return");
 try{
  p->check();require_initial_best_land_return_without_callbacks();InitialActiveGameDataPrefix q;q.node=p->initial_best_land->node;q.active_player=detail::bnw_game_active_player_read(*p->state);q.active=p->actor==q.active_player;q.world_revision=initial_second_city_prior_world_revision_without_callbacks();
  if(q.active){
   q.input=p->initial_set_moves->input;need(bool(q.input),"Initial active GameData actual retained UI missing");q.input->preflight_game_data_dirty_setter();
   q.before=q.input->unit_dirty_state();q.current=q.before;q.before_flag=q.input->game_data_dirty();q.before_count=q.input->game_data_dirty_setter_count();
   need(q.before_count<UINT64_MAX&&q.before.client_revision<UINT64_MAX&&q.input->initial_unit_game_data_matches_without_callbacks(q.before_flag,q.before_count,q.before.client_revision),"Initial active GameData actual getter preparation changed");
  }
  p->initial_active_game_data.emplace(std::move(q));auto&stage=*p->initial_active_game_data;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_best_land_return_without_callbacks();
  need(detail::bnw_game_active_player_read(*p->state)==stage.active_player&&initial_second_city_prior_world_revision_without_callbacks()==stage.world_revision&&!p->bank->retired.load(),"Initial active GameData final actual Game source changed");
  if(stage.active){stage.input->set_initial_unit_game_data_dirty(*canonical);need(stage.stored&&initial_unit_game_data_prior_unit_dirty_matches_without_callbacks(stage.input,stage.before),"Initial actual GameData normal return missing");}
  stage.returned=true;p->initial_set_xy->record_initial_active_game_data_return(stage.active);require_initial_active_game_data_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
template<class Rows>auto&initial_cycle_named(Rows&rows,std::string_view key){
 decltype(&rows.front())out=nullptr;for(auto&r:rows)if(name(r.name)==key){need(!out,"Init cycle duplicate physical field");out=&r;}
 need(out,"Init cycle required physical field absent");return *out;
}
template<class P>auto&initial_cycle_block(P&player){
 need(player.administration.layout==bnw,"Init cycle actual BNW administration absent");return initial_cycle_named(player.administration.blocks,"pre_city");
}
template<class P>auto&initial_cycle_array(P&player){
 auto&b=initial_cycle_block(player);auto&a=initial_cycle_named(b.arrays,"m_UnitCycle");need(a.columns==1,"Init cycle actual CLinkList columns invalid");return a;
}
template<class P>auto&initial_cycle_count(P&player){return initial_cycle_named(initial_cycle_block(player).fields,"m_UnitCycle_wire_count");}
std::vector<std::uint64_t>initial_cycle_sources(const LiveSessionState&s,std::int32_t actor,const Bank&b,const PendingUnit&node){
 std::vector<std::uint64_t>out;const auto emit=[&](auto v){out.push_back(static_cast<std::uint64_t>(v));};
 emit(s.world.map_header.grid.width);emit(s.world.map_header.grid.height);emit(s.world.map_header.wrap_x);emit(s.world.map_header.wrap_y);
 emit(b.slots);emit(b.last);emit(b.head);emit(b.free_count);emit(b.serial);emit(b.next.size());for(auto v:b.next)emit(v);
 const auto&raw=s.world.all_players->players.at(static_cast<std::size_t>(actor));emit(reinterpret_cast<std::uintptr_t>(&raw));emit(raw.units.units.size());
 for(const auto&u:raw.units.units){emit(reinterpret_cast<std::uintptr_t>(&u));emit(core(u,"m_iID"));emit(core(u,"m_eOwner"));emit(core(u,"m_iX"));emit(core(u,"m_iY"));
  const auto&owned=s.maintenance.players.at(actor).units.at(core(u,"m_iID"));emit(owned.type);emit(reinterpret_cast<std::uintptr_t>(&owned));}
 emit(reinterpret_cast<std::uintptr_t>(&node));emit(node.scalars.at("m_iID"));emit(node.scalars.at("m_eOwner"));emit(node.scalars.at("m_iX"));emit(node.scalars.at("m_iY"));emit(node.scalars.at("m_eUnitType"));
 // Source values used by workRate and native human/team/handicap getters.
 for(const auto&p:s.world.all_players->players){emit(post_player(s,p.saved_player_id,"m_bAlive"));emit(post_player_team(s,p.saved_player_id));}
 emit(post_player(s,actor,"m_iWorkerSpeedModifier"));emit(initial_xp_game_handicap(s));
 need(s.world.pregame.has_value(),"Init cycle actual Pregame absent");
 for(const auto&[key,v]:s.world.pregame->archive.integers){emit(key.size());for(char c:key)emit(static_cast<unsigned char>(c));emit(v);}
 for(const auto&[key,v]:s.world.pregame->archive.integer_arrays){emit(key.size());for(char c:key)emit(static_cast<unsigned char>(c));emit(v.size());for(auto x:v)emit(x);}
 return out;
}
std::int32_t initial_cycle_work_info(const ui::SelectedBnwPreviewHost&host,std::int32_t type){
 const content::InfoTypeEntry*info=nullptr;for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="Units"&&e.id==type){need(!info,"Init cycle duplicate UnitInfo ID");info=&e;}}
 need(info&&info->hash==content::saved_info_type_hash(info->type),"Init cycle workRate actual UnitInfo absent");bool found=false;std::int32_t value=0;
 const auto report=host.database().query_rows({"SELECT ID,Type,WorkRate FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&r){need(!found&&r.integer("ID")==type&&r.text("Type")==std::optional<std::string>{info->type},"Init cycle workRate Info identity changed");found=true;value=narrow(r.integer("WorkRate"));});
 need(report.all_succeeded()&&found,"Init cycle actual workRate Info query unavailable");return value;
}
std::int32_t initial_cycle_ai_work_info(const ui::SelectedBnwPreviewHost&host,std::int32_t id){
 const content::InfoTypeEntry*info=nullptr;for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="HandicapInfos"&&e.id==id){need(!info,"Init cycle duplicate HandicapInfo");info=&e;}}
 need(info&&info->hash==content::saved_info_type_hash(info->type),"Init cycle actual HandicapInfo absent");bool found=false;std::int32_t value=0;
 const auto report=host.database().query_rows({"SELECT ID,Type,AIWorkRateModifier FROM HandicapInfos WHERE ID=?",{std::int64_t(id)}},[&](const auto&r){need(!found&&r.integer("ID")==id&&r.text("Type")==std::optional<std::string>{info->type},"Init cycle actual HandicapInfo changed");found=true;value=narrow(r.integer("AIWorkRateModifier"));});need(report.all_succeeded()&&found,"Init cycle actual AI workRate unavailable");return value;
}
}
bool BnwNativeUnitAllocatorCore::initial_cycle_prior_scalars_matches_without_callbacks(
 const std::map<std::string,std::int32_t,std::less<>>&actual,const std::map<std::string,std::int32_t,std::less<>>&expected)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign initial cycle scalar lineage");
 if(p->initial_army_ai&&p->initial_army_ai->stored)return initial_army_prior_scalars_match_without_callbacks(actual,expected);
 if(!p->initial_unit_cycler||!p->initial_unit_cycler->stored)return actual==expected;
 const auto&q=*p->initial_unit_cycler;
 const auto&player=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
 need(&initial_cycle_array(player)==q.array&&&initial_cycle_count(player)==q.count,"Initial cycle historical physical list identity changed");
 need(q.active&&q.node&&&actual==&q.node->scalars&&q.array&&q.count&&q.array->values==q.after&&q.count->value==static_cast<std::int64_t>(q.after.size())&&
  q.node->scalars.at("m_iCycleOrder")==q.newborn_after&&!p->bank->retired.load(),"Initial cycle historical node receipt lacks actual stores");
 if(actual.size()!=expected.size())return false;auto a=actual.begin(),e=expected.begin();
 for(;a!=actual.end();++a,++e){if(a->first!=e->first)return false;
  if(a->first=="m_iCycleOrder"){if(a->second!=q.newborn_after||e->second!=q.newborn_before)return false;}
  else if(a->second!=e->second)return false;
 }return true;
}
void BnwNativeUnitAllocatorCore::require_initial_unit_cycler_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_unit_cycler&&p->initial_unit_cycler->returned,"Init cycle actual returned prefix absent/foreign");
 require_should_dim_outside_owner_without_callbacks();require_initial_active_game_data_return_without_callbacks();const auto&q=*p->initial_unit_cycler;
 need(q.active==p->initial_active_game_data->active&&q.node.get()==p->initial_active_game_data->node.get(),"Init cycle predecessor/source changed");
 if(q.active){const auto&player=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
  need(q.stored&&&initial_cycle_array(player)==q.array&&&initial_cycle_count(player)==q.count&&q.array->values==q.after&&q.count->value==static_cast<std::int64_t>(q.after.size())&&
   q.input->initial_center_selection()==q.selected&&initial_maintenance_prior_unit_dirty_matches_without_callbacks(q.input,q.ui_before)&&initial_cycle_sources(*p->state,p->actor,*p->bank,*q.node)==q.source_seal,"Init cycle actual returned list/selection/source changed");
  check_imported_cycle_nodes(*p->bank,*p->state,p->actor);
  for(const auto&e:q.entries){if(e.imported)need(e.imported->cycle_order==e.after&&e.imported->store_count==e.after_stores,"Init cycle actual imported plain stores changed");
   else need(e.newborn_cycle&&*e.newborn_cycle==e.after,"Init cycle actual newborn plain stores changed");}
 }
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_unit_cycler_returned&&prefix.initial_unit_cycler_applied==q.active&&prefix.initial_unit_cycler_units==q.entries.size()&&prefix.initial_unit_cycler_plain_stores==q.stores.size()&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init cycle earned Pending receipt changed");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_unit_cycler_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_active_game_data&&p->initial_active_game_data->returned&&!p->initial_unit_cycler,"Init cycle requires once actual GameData predecessor");
 try{
  p->check();require_initial_active_game_data_return_without_callbacks();InitialUnitCyclerPrefix q;q.node=p->initial_active_game_data->node;q.active=p->initial_active_game_data->active;
  if(q.active){auto&player=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));q.array=&initial_cycle_array(player);q.count=&initial_cycle_count(player);
   need(q.count->value>=0&&q.count->value<=INT32_MAX&&static_cast<std::uint64_t>(q.count->value)==q.array->values.size(),"Init cycle actual old signed count malformed");q.before=q.array->values;
   q.input=p->initial_active_game_data->input;need(bool(q.input),"Init cycle genuine head-selection owner absent");q.selected=q.input->initial_center_selection();q.ui_before=q.input->unit_dirty_state();
   check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);check_imported_cycle_nodes(*p->bank,*p->state,p->actor);
   // Native firstUnit/nextUnit scans occupied FLTA slots in ascending order.
   // The actual allocated unpublished node belongs to the SAME manager.
   for(std::int32_t slot=0;slot<=p->bank->last;++slot){
    const auto i=p->bank->imported_cycle_nodes.find(slot);const bool newborn=slot==p->bank->pending_slot;
    need(!(newborn&&i!=p->bank->imported_cycle_nodes.end()),"Init cycle newborn/imported native slot collision");
    if(i!=p->bank->imported_cycle_nodes.end()){const auto&n=i->second;const auto&u=*n->source;const auto&owned=p->state->maintenance.players.at(p->actor).units.at(n->id);
     q.entries.push_back({n->id,narrow(owned.type),narrow(core(u,"m_iX")),narrow(core(u,"m_iY")),n,nullptr,n->cycle_order,0,n->store_count,n->store_count});
    }else if(newborn){auto&cycle=q.node->scalars.at("m_iCycleOrder");need(cycle>=INT16_MIN&&cycle<=INT16_MAX,"Init cycle newborn short exceeds native range");q.newborn_before=cycle;
     q.entries.push_back({q.node->scalars.at("m_iID"),q.node->scalars.at("m_eUnitType"),q.node->scalars.at("m_iX"),q.node->scalars.at("m_iY"),{},&cycle,static_cast<std::int16_t>(cycle),0,0,0});
    }
   }
   need(q.entries.size()==static_cast<std::size_t>(p->bank->last+1-p->bank->free_count)&&q.entries.size()==player.units.units.size()+1&&q.entries.size()<=INT32_MAX,"Init cycle actual getNumUnits/union FLTA mismatch");
   std::optional<std::size_t>start;std::set<std::int32_t>ids;
   for(std::size_t i=0;i<q.entries.size();++i){const auto&e=q.entries[i];need(e.id>=0&&ids.insert(e.id).second,"Init cycle duplicate native full ID");if(q.selected&&q.selected->first==p->actor&&q.selected->second==e.id)start=i;}
   if(q.selected&&q.selected->first==p->actor)need(start.has_value(),"Init cycle same-owner selected head has no actual union node");
   // A foreign head is actual native NULL fallback. No synthetic null input.
   q.source_seal=initial_cycle_sources(*p->state,p->actor,*p->bank,*q.node);
   q.stores.reserve(q.entries.size()*2);q.after.reserve(q.entries.size());q.empty_reserved.reserve(q.entries.size());
   for(std::size_t i=0;i<q.entries.size();++i){q.stores.push_back({i,0});
    if(!start){
     // workRate(true) never executes canMove; even WorkRate=0 still reads
     // actual player/Traits/human operands in the SDK source order.
     const auto base=initial_cycle_work_info(*p->host,q.entries[i].type);
     if(!q.traits){q.worker_modifier=narrow(post_player(*p->state,p->actor,"m_iWorkerSpeedModifier"));q.traits=BnwMajorPlayerTraitsOwner::from_source_host(p->host,p->actor);q.trait_modifier=q.traits->cache_value("m_iWorkerSpeedModifier");
      q.human=detail::bnw_player_human_read(*p->state,p->actor,p->host->database());
      if(!*q.human){bool mate=false;const auto team=post_player_team(*p->state,p->actor);for(std::int32_t id=0;id<64;++id){if(detail::bnw_player_human_read(*p->state,id,p->host->database())&&post_player(*p->state,id,"m_bAlive")!=0&&post_player_team(*p->state,id)==team){mate=true;break;}}q.ai_teammate=mate;
       if(!mate&&p->actor!=63){q.handicap=initial_xp_game_handicap(*p->state);q.ai_modifier=initial_cycle_ai_work_info(*p->host,q.handicap);}
      }
     }
     auto rate=narrow(std::int64_t(base)*std::max(0,narrow(std::int64_t(q.worker_modifier)+q.trait_modifier+100)));rate/=100;
     if(!*q.human&&q.ai_teammate&&!*q.ai_teammate&&p->actor!=63){rate=narrow(std::int64_t(rate)*std::max(0,narrow(std::int64_t(q.ai_modifier)+100)));rate/=100;}
     q.rates.emplace_back(q.entries[i].type,base);if(rate>0)start=i;
    }
   }
   if(!start&&!q.entries.empty())start=0;
   MapGeometry geometry{p->state->world.map_header.grid,p->state->world.map_header.wrap_x,p->state->world.map_header.wrap_y,bnw};geometry.validate();
   for(const auto&e:q.entries)need(geometry.plot(e.x,e.y).has_value(),"Init cycle actual live unit coordinates absent");
   if(start){std::size_t current=*start;q.after.push_back(q.entries[current].id);q.entries[current].after=1;q.stores.push_back({current,1});
    while(q.after.size()<q.entries.size()){std::int32_t best_score=INT32_MAX;std::optional<std::size_t>best;
     for(std::size_t i=0;i<q.entries.size();++i){if(q.entries[i].after==1)continue;const auto score=narrow(geometry.distance(q.entries[current].x,q.entries[current].y,q.entries[i].x,q.entries[i].y));if(score<best_score){best=i;best_score=score;}}
     need(best.has_value(),"Init cycle actual nearest unit absent");current=*best;q.after.push_back(q.entries[current].id);q.entries[current].after=1;q.stores.push_back({current,1});
    }
   }
   for(const auto&store:q.stores){auto&e=q.entries[store.entry];need(e.after_stores<UINT64_MAX,"Init cycle plain store lineage overflow");++e.after_stores;}
   q.newborn_after=1;
   for(const auto&[type,base]:q.rates)need(initial_cycle_work_info(*p->host,type)==base,"Init cycle actual WorkRate Info changed during preparation");
   if(q.traits){q.traits->require_current();need(q.traits->cache_value("m_iWorkerSpeedModifier")==q.trait_modifier,"Init cycle actual Trait WorkerSpeed changed");}
   if(q.handicap!=-1)need(initial_cycle_ai_work_info(*p->host,q.handicap)==q.ai_modifier,"Init cycle actual AI HandicapInfo changed");
  }
  // Separate authentic successor seals; original setMoves/other receipts are
  // immutable. The only predicted node delta is the earned plain CycleOrder.
  std::vector<std::int64_t>city;std::vector<std::uint64_t>post,center;
  if(q.active){
   need(p->initial_city_strength&&p->initial_post_city&&p->initial_center_plan,"Init cycle exact prior observers absent");
   auto projected=q.node->scalars;projected.at("m_iCycleOrder")=q.newborn_after;
   city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,projected,q.node->indexed);
   const auto&binding=*p->initial_hidden_binding;
   initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,projected,q.node->indexed,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  }
  p->initial_unit_cycler.emplace(std::move(q));auto&stage=*p->initial_unit_cycler;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_active_game_data_return_without_callbacks();
  if(stage.active){auto&player=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
   need(&initial_cycle_array(player)==stage.array&&&initial_cycle_count(player)==stage.count&&stage.array->values==stage.before&&stage.count->value==static_cast<std::int64_t>(stage.before.size())&&
    stage.input->initial_center_selection()==stage.selected&&stage.input->unit_dirty_state()==stage.ui_before&&initial_cycle_sources(*p->state,p->actor,*p->bank,*stage.node)==stage.source_seal,"Init cycle final actual list/source/selection changed before store");
   for(const auto&e:stage.entries)if(e.imported)need(e.imported->cycle_order==e.before&&e.imported->store_count==e.before_stores,"Init cycle imported mutable slot changed before store");else need(e.newborn_cycle&&*e.newborn_cycle==e.before,"Init cycle newborn mutable slot changed before store");
   // All allocation/callback/throwing checks precede first native mutation.
   // Rebuild clears list; then resets in native slot order; then appends chosen
   // IDs and writes one. Plain short setters do NOT create archive dirty bits.
   stage.array->values.swap(stage.empty_reserved);stage.count->value=0;
   for(const auto&store:stage.stores){auto&e=stage.entries[store.entry];
    if(store.value==1){stage.array->values.push_back(e.id);++stage.count->value;}
    if(e.imported){e.imported->cycle_order=store.value;++e.imported->store_count;}else *e.newborn_cycle=store.value;
   }
   stage.count->value=static_cast<std::int64_t>(stage.array->values.size());
   p->initial_city_strength->cycle_order_inputs.swap(city);p->initial_city_strength->cycle_order_stored=true;
   p->initial_post_city->cycle_order_seal.swap(post);p->initial_center_plan->storage_->cycle_order_seal.swap(center);stage.stored=true;
  }
  stage.returned=true;p->initial_set_xy->record_initial_unit_cycler_return(stage.active,stage.entries.size(),stage.stores.size());require_initial_unit_cycler_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
std::int32_t initial_world_unit_max_global(const ui::SelectedBnwPreviewHost&host,std::int32_t unit_class){
 const content::InfoTypeEntry*info=nullptr;
 for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table=="UnitClasses"&&e.id==unit_class){need(!info,"Init World Unit duplicate UnitClassInfo");info=&e;}}
 need(info&&info->hash==content::saved_info_type_hash(info->type),"Init World Unit actual UnitClassInfo unavailable");
 bool found=false;std::int32_t value=-1;
 const auto report=host.database().query_rows({"SELECT ID,Type,MaxGlobalInstances FROM UnitClasses WHERE ID=?",{std::int64_t(unit_class)}},[&](const auto&r){
  need(!found&&r.integer("ID")==unit_class&&r.text("Type")==std::optional<std::string>{info->type},"Init World Unit actual ClassInfo changed");found=true;value=narrow(r.integer("MaxGlobalInstances"));
 });need(report.all_succeeded()&&found,"Init World Unit actual ClassInfo query failed");return value;
}
}
void BnwNativeUnitAllocatorCore::require_initial_world_unit_message_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_world_unit_message&&p->initial_world_unit_message->returned,"Init World Unit actual return absent/foreign");
 require_should_dim_outside_owner_without_callbacks();require_initial_unit_cycler_return_without_callbacks();const auto&q=*p->initial_world_unit_message;
 need(q.max_global==-1&&p->initial_tourism&&q.unit_class==p->initial_tourism->unit_class,"Init World Unit source false branch changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_world_unit_message_returned&&prefix.initial_world_unit_class==q.unit_class&&prefix.initial_world_unit_max_global==q.max_global&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init World Unit earned Pending return changed");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_world_unit_message_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_unit_cycler&&p->initial_unit_cycler->returned&&!p->initial_world_unit_message,"Init World Unit requires once actual UnitCycler predecessor");
 try{
  p->check();require_initial_unit_cycler_return_without_callbacks();const auto info=initial_tourism_unit_info(*p->host,*p->initial_unit_cycler->node);
  need(p->initial_tourism&&info.unit_class==p->initial_tourism->unit_class,"Init World Unit actual UnitInfo Class changed");
  InitialWorldUnitMessagePrefix q;q.unit_class=info.unit_class;q.max_global=initial_world_unit_max_global(*p->host,q.unit_class);
  // SDK != -1, NOT >0. A zero authored limit still enters the world body.
  // The active-player conditional affects messages, never unconditional replay.
  need(q.max_global==-1,"Mandatory actual World Unit DLLUI messages/localization and Game replay owner required");
  need(initial_tourism_unit_info(*p->host,*p->initial_unit_cycler->node).unit_class==q.unit_class&&initial_world_unit_max_global(*p->host,q.unit_class)==q.max_global,"Init World Unit actual rules changed during preparation");
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_unit_cycler_return_without_callbacks();
  q.returned=true;p->initial_world_unit_message.emplace(q);p->initial_set_xy->record_initial_world_unit_message_return(q.unit_class,q.max_global);
  require_initial_world_unit_message_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

BnwNativeUnitAllocatorCore::InitialPlacementOperands BnwNativeUnitAllocatorCore::initial_second_city_strength_operands()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign second Init city source");
 require_initial_world_unit_message_return_without_callbacks();p->check();const auto node=p->initial_unit_cycler->node;
 check_registered(*p->bank,*node,node,p->bank->pending_slot);
 const auto x=node->scalars.at("m_iX"),y=node->scalars.at("m_iY");need(p->state->world.map_plots&&p->state->world.map_projection,"Second Init actual plot absent");
 const auto index=p->state->world.map_header.grid.index(x,y);const auto&raw=p->state->world.map_plots->plots.at(index);const auto&cell=p->state->world.map_projection->cells.at(index);
 const auto city_owner=narrow(post_named(raw.fields,"plot_city_owner").value),city_id=narrow(post_named(raw.fields,"plot_city_id").value);
 need(raw.index==index&&cell.index==index&&cell.is_city==(city_owner!=-1)&&cell.plot_city.has_value()==(city_owner!=-1),"Second Init raw/projected actual plot city differs");
 if(city_owner==-1)need(city_id==-1,"Second Init null actual city has nonnull ID");
 else need(city_owner==p->actor&&cell.plot_city->owner==city_owner&&cell.plot_city->id==city_id&&resolve_bnw_city_registry(p->state->world,p->state->cities,city_owner,city_id),"Second Init current own-city producer unavailable");
 return {p->host,p->host->session_.get(),p->state,p->actor,node->scalars.at("m_iID"),x,y,city_id,node->scalars.at("m_iBaseCombat"),p->generation,p->state->ui_source_revision};
}
std::uint64_t BnwNativeUnitAllocatorCore::initial_second_city_prior_world_revision_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign second Init strength prior epoch");
 if(!p->initial_second_city_strength||!p->initial_second_city_strength->stored)return p->state->ui_source_revision;
 const auto&q=*p->initial_second_city_strength;
 const auto*raw=resolve_bnw_city_registry(p->state->world,p->state->cities,q.owner,q.id);
 need(q.city&&q.raw&&raw==q.raw&&q.before==q.after&&garrison_named(raw->core.fields,"m_iStrengthValue").values.at(0)==q.after&&
  q.before_revision<UINT64_MAX&&q.after_revision==q.before_revision+1&&p->state->ui_source_revision==q.after_revision&&
  p->state->city_dirty_events==q.after_events&&p->state->city_dirty_next_occurrence==q.before_occurrence+1&&p->bank&&!p->bank->retired.load(),"Second Init strength actual earned cache/events/epoch changed");
 return q.before_revision;
}
bool BnwNativeUnitAllocatorCore::initial_second_city_prior_events_match_without_callbacks(const std::vector<BnwCityDirtyEvent>&expected,std::uint64_t next)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign second Init prior event queue");
 if(!p->initial_second_city_strength||!p->initial_second_city_strength->stored)return p->state->city_dirty_events==expected&&p->state->city_dirty_next_occurrence==next;
 (void)initial_second_city_prior_world_revision_without_callbacks();const auto&q=*p->initial_second_city_strength;
 return expected==q.before_events&&next==q.before_occurrence;
}
void BnwNativeUnitAllocatorCore::require_initial_second_city_strength_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_second_city_strength&&p->initial_second_city_strength->returned,"Second Init city actual return absent/foreign");
 require_should_dim_outside_owner_without_callbacks();require_initial_world_unit_message_return_without_callbacks();const auto&q=*p->initial_second_city_strength;
 need(q.node.get()==p->initial_unit_cycler->node.get(),"Second Init city actual node replaced");
 if(q.city){need(q.stored&&p->initial_second_city_strength_quote.has_value(),"Second Init actual cache store/quote absent");
  (void)initial_second_city_prior_world_revision_without_callbacks();BnwMinorCityStrengthScratch::require_initial_suffix_strength_without_callbacks(*this,*p->initial_second_city_strength_quote,true);
 }
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_second_city_strength_returned&&prefix.initial_second_city_strength_applied==q.city&&prefix.initial_second_city_strength_before==q.before&&prefix.initial_second_city_strength_after==q.after&&prefix.initial_second_city_strength_occurrence==(q.city?q.before_occurrence:0)&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Second Init exact earned Pending city receipt changed");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_second_city_strength_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);
 need(p->train_source&&p->initial_world_unit_message&&p->initial_world_unit_message->returned&&!p->initial_second_city_strength,"Second Init strength requires once actual World Unit return");
 try{
  p->check();require_initial_world_unit_message_return_without_callbacks();const auto args=initial_second_city_strength_operands();InitialSecondCityStrengthPrefix q;
  q.node=p->initial_unit_cycler->node;q.city=args.city!=-1;q.owner=args.owner;q.id=args.city;q.before_revision=args.revision;q.after_revision=q.before_revision;
  if(q.city){
   p->initial_second_city_strength_quote=BnwMinorCityStrengthScratch::prepare_initial_suffix_strength(*canonical);const auto&fresh=*p->initial_second_city_strength_quote;
   q.raw=fresh.quote.raw;q.before=fresh.quote.before;q.after=fresh.quote.after;
   // Execute the genuine current strength formula. This finite unchanged-
   // cache case still performs the native store and unconditional dirty event.
   need(q.before==q.after,"Mandatory changed second-city strength current-input/old-cache propagation owner required");
   need(q.before_revision<UINT64_MAX&&p->state->city_dirty_next_occurrence>0&&p->state->city_dirty_next_occurrence<UINT64_MAX,"Second Init city revision/dirty occurrence exhausted");
   q.after_revision=q.before_revision+1;q.before_occurrence=p->state->city_dirty_next_occurrence;q.before_events=p->state->city_dirty_events;q.after_events=q.before_events;
   std::uint64_t previous=0;for(const auto&e:q.before_events){need(e.generation==p->generation&&e.occurrence>previous&&e.occurrence<q.before_occurrence,"Second Init current CityInfo events malformed");previous=e.occurrence;}
   q.after_events.push_back({q.before_occurrence,p->generation,q.owner,q.id,-1,-1,BnwCityDirtyKind::city_info,true});
  }
  p->initial_second_city_strength.emplace(std::move(q));auto&stage=*p->initial_second_city_strength;
  auto events=stage.after_events;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_world_unit_message_return_without_callbacks();
  const auto current=initial_second_city_strength_operands();need(current.city==stage.id&&current.revision==stage.before_revision&&!p->bank->retired.load(),"Second Init final city source changed before stores");
  if(stage.city){
   BnwMinorCityStrengthScratch::require_initial_suffix_strength_without_callbacks(*canonical,*p->initial_second_city_strength_quote,false);
   auto&cities=p->state->world.all_players->players.at(stage.owner).cities.cities;saves::CityCoreField*destination=nullptr;
   for(auto&city:cities)if(&city==stage.raw)destination=&garrison_named_mutable(city.core.fields,"m_iStrengthValue");
   need(destination&&destination->encoding==saves::CityCoreEncoding::i32&&destination->values.size()==1&&!destination->wire_count&&destination->bytes.empty()&&destination->hashes.empty()&&destination->row_counts.empty()&&destination->row_keys.empty()&&destination->values[0]==stage.before&&p->state->city_dirty_events==stage.before_events&&p->state->city_dirty_next_occurrence==stage.before_occurrence,"Second Init final native cache/queue destination changed");
   destination->values[0]=stage.after;p->state->city_dirty_events.swap(events);p->state->city_dirty_next_occurrence=stage.before_occurrence+1;
   p->state->ui_source_revision=stage.after_revision;stage.stored=true;
  }
  stage.returned=true;p->initial_set_xy->record_initial_second_city_strength_return(stage.city,stage.before,stage.after,stage.city?stage.before_occurrence:0,stage.after_revision);
  require_initial_second_city_strength_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

bool BnwNativeUnitAllocatorCore::initial_army_prior_scalars_match_without_callbacks(
 const std::map<std::string,std::int32_t,std::less<>>&actual,const std::map<std::string,std::int32_t,std::less<>>&expected)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_army_ai&&p->initial_army_ai->stored,"Init ArmyAI prior scalar return absent/foreign");const auto&q=*p->initial_army_ai;
 need(&actual==&q.node->scalars&&actual==q.after&&q.node.get()==p->initial_unit_cycler->node.get()&&!p->bank->retired.load(),"Init ArmyAI actual scalar owner/current store changed");
 if(actual.size()!=expected.size())return false;auto a=actual.begin(),e=expected.begin();
 for(;a!=actual.end();++a,++e){if(a->first!=e->first)return false;
  if(a->first=="m_iArmyId"||a->first=="m_eUnitAIType"){if(e->second!=q.before.at(a->first))return false;}
  else if(a->first=="m_iCycleOrder"&&p->initial_unit_cycler->stored){const auto&cycle=*p->initial_unit_cycler;
   if(a->second!=cycle.newborn_after||e->second!=cycle.newborn_before)return false;
  }else if(a->second!=e->second)return false;
 }return true;
}
bool BnwNativeUnitAllocatorCore::initial_army_prior_dirty_match_without_callbacks(const std::set<std::size_t>&actual,const std::set<std::size_t>&expected)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init ArmyAI archive dirty lineage");
 if(!p->initial_army_ai||!p->initial_army_ai->stored)return actual==expected;const auto&q=*p->initial_army_ai;
 need(&actual==&q.node->dirty&&actual==q.dirty_after&&q.node->scalars==q.after&&!p->bank->retired.load(),"Init ArmyAI actual archive dirty store changed");return expected==q.dirty_before;
}
void BnwNativeUnitAllocatorCore::require_initial_army_ai_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_army_ai&&p->initial_army_ai->returned,"Init ArmyAI actual return absent/foreign");
 require_should_dim_outside_owner_without_callbacks();require_initial_second_city_strength_return_without_callbacks();const auto&q=*p->initial_army_ai;
 check_registered(*p->bank,*q.node,q.node,p->bank->pending_slot);
 need(q.stored&&q.node.get()==p->initial_second_city_strength->node.get()&&q.node->scalars==q.after&&q.node->dirty==q.dirty_after&&
  &q.node->scalars.at("m_iArmyId")==q.army&&&q.node->scalars.at("m_eUnitAIType")==q.ai&&*q.army==-1&&*q.ai==q.unit_ai&&p->bank->train&&p->bank->train->unit_ai==q.unit_ai,"Init ArmyAI actual canonical member/operand changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_army_ai_returned&&prefix.initial_army_before==q.before.at("m_iArmyId")&&prefix.initial_army_after==-1&&prefix.initial_unit_ai_before==q.before.at("m_eUnitAIType")&&prefix.initial_unit_ai_after==q.unit_ai&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init ArmyAI exact earned Pending return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_army_ai_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_second_city_strength&&p->initial_second_city_strength->returned&&!p->initial_army_ai,"Init ArmyAI requires once actual second-city return");
 try{
  p->check();require_initial_second_city_strength_return_without_callbacks();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  const auto args=p->train_source->allocator_operands();need(p->bank->train&&args.owner==p->actor&&args.unit==p->bank->train->unit&&args.unit_ai==p->bank->train->unit_ai&&args.unit_ai>=0,"Init ArmyAI genuine original TRAIN operand changed");
  InitialArmyAiPrefix q;q.node=p->initial_second_city_strength->node;q.unit_ai=args.unit_ai;q.before=q.node->scalars;q.dirty_before=q.node->dirty;
  q.army=&q.node->scalars.at("m_iArmyId");q.ai=&q.node->scalars.at("m_eUnitAIType");
  const auto army=archive_index(*q.node,"m_iArmyId"),ai=archive_index(*q.node,"m_eUnitAIType");
  need(army&&ai&&q.node->archive[*army].spec.kind==ArchiveKind::scalar&&q.node->archive[*ai].spec.kind==ArchiveKind::scalar&&q.node->archive[*army].target==q.node.get()&&q.node->archive[*ai].target==q.node.get(),"Init ArmyAI real SAME archive bindings absent");
  auto staged=*q.node;assign_scalar(staged,"m_iArmyId",-1);assign_scalar(staged,"m_eUnitAIType",q.unit_ai);q.after=staged.scalars;q.dirty_after=staged.dirty;auto dirty=q.dirty_after;
  std::vector<std::int64_t>city,suffix;std::vector<std::uint64_t>post,center;
  city=BnwMinorCityStrengthScratch::prepare_initial_name_inputs(*canonical,*p->initial_city_strength,q.after,q.node->indexed);
  if(p->initial_second_city_strength_quote)suffix=BnwMinorCityStrengthScratch::prepare_initial_suffix_node_inputs(*canonical,*p->initial_second_city_strength_quote,q.after,q.node->indexed);
  const auto&binding=*p->initial_hidden_binding;initial_post_city_semantics(*p->state,p->actor,binding.x,binding.y,q.after,q.node->indexed,[&](auto n){post.push_back(static_cast<std::uint64_t>(n));});center=post;
  p->initial_army_ai.emplace(std::move(q));auto&stage=*p->initial_army_ai;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_second_city_strength_return_without_callbacks();
  const auto current=p->train_source->allocator_operands();need(current.unit_ai==stage.unit_ai&&current.owner==p->actor&&stage.node->scalars==stage.before&&stage.node->dirty==stage.dirty_before&&
   &stage.node->scalars.at("m_iArmyId")==stage.army&&&stage.node->scalars.at("m_eUnitAIType")==stage.ai&&!p->bank->retired.load(),"Init ArmyAI final genuine source/physical member changed before stores");
  // Preallocated native dirty set then two stable physical FAutoVariable slots.
  // No map swap: existing unsaved Cycler pointers retain the SAME member node.
  stage.node->dirty.swap(dirty);*stage.army=-1;*stage.ai=stage.unit_ai;
  p->initial_city_strength->army_ai_inputs.swap(city);p->initial_city_strength->army_ai_stored=true;
  if(p->initial_second_city_strength_quote){p->initial_second_city_strength_quote->army_ai_inputs.swap(suffix);p->initial_second_city_strength_quote->army_ai_stored=true;}
  p->initial_post_city->army_ai_seal.swap(post);p->initial_center_plan->storage_->army_ai_seal.swap(center);stage.stored=true;stage.returned=true;
  p->initial_set_xy->record_initial_army_ai_return(stage.before.at("m_iArmyId"),stage.before.at("m_eUnitAIType"),stage.unit_ai);require_initial_army_ai_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
struct InitialMaintenanceCalculation {std::vector<std::int64_t>inputs;std::int32_t value=0;};
const content::InfoTypeEntry&initial_maintenance_info(const ui::SelectedBnwPreviewHost&host,const char*table,std::int32_t id){
 const content::InfoTypeEntry*info=nullptr;for(const auto&[key,e]:host.types().entries()){(void)key;if(e.table==table&&e.id==id){need(!info,"Init maintenance duplicate Info ID");info=&e;}}
 need(info&&info->hash==content::saved_info_type_hash(info->type),"Init maintenance actual Info absent");return *info;
}
InitialMaintenanceCalculation initial_maintenance_calculate(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,std::int32_t actor,const Bank&bank,const PendingUnit&node){
 InitialMaintenanceCalculation q;auto emit=[&](auto v){q.inputs.push_back(static_cast<std::int64_t>(v));};
 need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&actor>=0&&actor<64&&s.world.pregame&&s.world.pregame->layout==bnw,"Init maintenance actual BNW player/Pregame unavailable");
 const auto&player=s.world.all_players->players.at(static_cast<std::size_t>(actor));
 need(player.index==static_cast<std::uint64_t>(actor)&&player.saved_player_id==actor&&post_player(s,actor,"m_eID")==actor,"Init maintenance actual player identity differs");
 const auto&handicaps=s.world.pregame->archive.integer_arrays.at("handicaps");need(handicaps.size()==64,"Init maintenance actual Handicap roster unavailable");
 const auto handicap=narrow(handicaps.at(static_cast<std::size_t>(actor)));need(handicap==s.setup.players.at(actor).handicap,"Init maintenance raw/owned Handicap differs");const auto&info=initial_maintenance_info(host,"HandicapInfos",handicap);
 std::int32_t base=0,per_city=0,pop_percent=0;bool found=false;
 auto report=host.database().query_rows({"SELECT ID,Type,ProductionFreeUnits,ProductionFreeUnitsPerCity,ProductionFreeUnitsPopulationPercent FROM HandicapInfos WHERE ID=?",{std::int64_t(handicap)}},[&](const auto&r){
  need(!found&&r.integer("ID")==handicap&&r.text("Type")==std::optional<std::string>{info.type},"Init maintenance actual Handicap Info identity changed");found=true;base=narrow(r.integer("ProductionFreeUnits"));per_city=narrow(r.integer("ProductionFreeUnitsPerCity"));pop_percent=narrow(r.integer("ProductionFreeUnitsPopulationPercent"));});need(report.all_succeeded()&&found,"Init maintenance actual supply Handicap Info unavailable");
 need(player.cities.layout==bnw&&player.cities.header.wire_city_count>=0&&static_cast<std::uint64_t>(player.cities.header.wire_city_count)==player.cities.cities.size()&&player.cities.cities.size()==s.cities.players.at(actor).cities.size(),"Init maintenance native/owned city count differs");
 const auto cities=narrow(player.cities.header.wire_city_count),population=narrow(post_player(s,actor,"m_iTotalPopulation"));need(population==s.cities.players.at(actor).total_population,"Init maintenance native/owned population cache differs");
 auto supplied=narrow(std::int64_t(base)+narrow(std::int64_t(per_city)*cities));supplied=narrow(std::int64_t(supplied)+narrow(std::int64_t(population)*pop_percent)/100);
 emit(handicap);emit(base);emit(per_city);emit(pop_percent);emit(cities);emit(population);
 const auto&minor=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(minor.size()==64&&minor.at(static_cast<std::size_t>(actor))==s.cities.players.at(actor).minor,"Init maintenance actual Minor predicate differs");emit(minor.at(static_cast<std::size_t>(actor)));
 // Native short-circuit: a minor never reaches Human or AI teammate getters.
 if(!minor.at(static_cast<std::size_t>(actor))){const auto human=detail::bnw_player_human_read(s,actor,host.database());emit(human);
  if(!human){bool mate=false;const auto team=post_player_team(s,actor);emit(team);
   for(std::int32_t id=0;id<64;++id){const auto h=detail::bnw_player_human_read(s,id,host.database());emit(id);emit(h);if(h){const auto alive=post_player(s,id,"m_bAlive");emit(alive);if(alive){const auto t=post_player_team(s,id);emit(t);if(t==team){mate=true;break;}}}}
   emit(mate);if(!mate){const auto game_handicap=initial_xp_game_handicap(s);const auto&game_info=initial_maintenance_info(host,"HandicapInfos",game_handicap);std::int32_t percent=0;found=false;
    report=host.database().query_rows({"SELECT ID,Type,AIUnitSupplyPercent FROM HandicapInfos WHERE ID=?",{std::int64_t(game_handicap)}},[&](const auto&r){need(!found&&r.integer("ID")==game_handicap&&r.text("Type")==std::optional<std::string>{game_info.type},"Init maintenance Game Handicap Info changed");found=true;percent=narrow(r.integer("AIUnitSupplyPercent"));});need(report.all_succeeded()&&found,"Init maintenance actual AI supply Info unavailable");
    supplied=narrow(std::int64_t(supplied)*narrow(std::int64_t(100)+percent));supplied/=100;emit(game_handicap);emit(percent);
   }
  }
 }
 emit(supplied);check_imported_cycle_nodes(bank,s,actor);need(bank.pending_units.size()==1&&bank.pending_units.at(bank.pending_slot).get()==&node,"Init maintenance actual unpublished manager node absent");
 need(player.units.layout==bnw&&player.units.header.wire_unit_count>=0&&static_cast<std::uint64_t>(player.units.header.wire_unit_count)==player.units.units.size()&&player.units.units.size()==s.maintenance.players.at(actor).units.size(),"Init maintenance actual imported unit count differs");
 const auto count=narrow(std::int64_t(bank.last)+1-bank.free_count);need(count==narrow(std::int64_t(player.units.units.size())+1),"Init maintenance actual getNumUnits union differs");emit(bank.last);emit(bank.free_count);emit(count);
 std::int32_t trade=0,seen=0;
 for(std::int32_t slot=0;slot<=bank.last;++slot){const auto i=bank.imported_cycle_nodes.find(slot);const bool newborn=slot==bank.pending_slot;need(!(newborn&&i!=bank.imported_cycle_nodes.end()),"Init maintenance native slot collision");if(i==bank.imported_cycle_nodes.end()&&!newborn)continue;
  std::int32_t id=-1,type=-1;if(newborn){id=node.scalars.at("m_iID");type=node.scalars.at("m_eUnitType");need(node.scalars.at("m_eOwner")==actor,"Init maintenance newborn owner differs");}else{const auto&n=*i->second;id=n.id;const auto&owned=s.maintenance.players.at(actor).units.at(id);type=narrow(owned.type);const auto&entry=initial_maintenance_info(host,"Units",type);need(core(*n.source,"m_iID")==id&&core(*n.source,"m_eOwner")==actor&&core(*n.source,"idUnitType")==entry.hash&&owned.source_core==std::optional{std::pair{n.source->core.start_offset,n.source->core.end_offset}},"Init maintenance raw/owned unit identity/type/span differs");}
  const auto&unit_info=initial_maintenance_info(host,"Units",type);bool is_trade=false;found=false;
  report=host.database().query_rows({"SELECT ID,Type,Trade FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&r){need(!found&&r.integer("ID")==type&&r.text("Type")==std::optional<std::string>{unit_info.type},"Init maintenance actual Trade UnitInfo identity changed");found=true;(void)narrow(r.integer("Trade"));is_trade=r.boolean("Trade");});need(report.all_succeeded()&&found,"Init maintenance actual Trade Info unavailable");
  ++seen;if(is_trade)trade=narrow(std::int64_t(trade)+1);emit(slot);emit(id);emit(type);emit(is_trade);
 }
 need(seen==count,"Init maintenance native traversal/getNumUnits differs");const auto paid=std::max(0,narrow(std::int64_t(narrow(std::int64_t(count)-trade))-supplied));
 const auto*d=host.defines().find("MAX_UNIT_SUPPLY_PRODMOD");need(host.defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),"Init maintenance actual MAX_UNIT_SUPPLY_PRODMOD absent");
 const auto cap=narrow(std::get<std::int64_t>(d->lookup.value));q.value=narrow(-std::int64_t(std::min(cap,narrow(std::int64_t(paid)*10))));emit(trade);emit(paid);emit(cap);emit(q.value);return q;
}
}
bool BnwNativeUnitAllocatorCore::initial_maintenance_prior_unit_dirty_matches_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&input,const ui::BnwClientUnitDirtySnapshot&expected)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&input,"Foreign Init maintenance prior UI source");
 if(!p->initial_maintenance||!p->initial_maintenance->ui_steps)return input->initial_moves_dirty_matches_without_callbacks(expected);const auto&q=*p->initial_maintenance;
 need(q.active&&q.stored&&q.input&&input.get()==q.input.get()&&!input.owner_before(q.input)&&!q.input.owner_before(input)&&q.ui_steps<=2&&q.current.client_revision==q.before.client_revision+q.ui_steps&&
  q.current.unit_info==q.before.unit_info&&q.current.plot_list_buttons==q.before.plot_list_buttons&&q.current.events==q.before.events&&q.current.next_occurrence==q.before.next_occurrence&&
  q.field&&q.field->values.size()==1&&q.field->values.front()==q.after&&q.before_cache==q.after&&!p->bank->retired.load()&&
  input->initial_unit_maintenance_matches_without_callbacks(std::optional<bool>{true},q.city_count+1,q.ui_steps==2?std::optional<bool>{true}:q.game_flag,q.game_count+(q.ui_steps==2?1:0),q.current.client_revision)&&input->initial_moves_dirty_matches_without_callbacks(q.current),"Init maintenance actual historical UI descendant changed");
 return expected==q.before;
}
bool BnwNativeUnitAllocatorCore::initial_maintenance_prior_game_data_matches_without_callbacks(const std::shared_ptr<ui::BnwClientSelectionControl>&input,std::optional<bool>flag,std::uint64_t count,std::uint64_t revision)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Init maintenance GameData history");
 if(!p->initial_maintenance||!p->initial_maintenance->ui_steps)return input->initial_unit_game_data_matches_without_callbacks(flag,count,revision);const auto&q=*p->initial_maintenance;
 need(initial_maintenance_prior_unit_dirty_matches_without_callbacks(input,q.before),"Init maintenance physical descendant missing");return flag==q.game_flag&&count==q.game_count&&revision==q.before.client_revision;
}
BnwNativeUnitAllocatorCore::InitialUnitMaintenanceDirtyOperands BnwNativeUnitAllocatorCore::initial_unit_maintenance_dirty_operands(const ui::BnwClientSelectionControl&receiver)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_maintenance,"Foreign/unissued Init maintenance UI phase");const auto&q=*p->initial_maintenance;
 need(q.active&&q.stored&&!q.returned&&q.ui_steps<2&&q.input&&q.input.get()==&receiver&&q.input.get()==p->initial_set_moves->input.get()&&!q.input.owner_before(p->initial_set_moves->input)&&!p->initial_set_moves->input.owner_before(q.input)&&!q.input.owner_before(p->host->session_->city_selection_)&&!p->host->session_->city_selection_.owner_before(q.input),"Init maintenance receiving UI is not SAME actual canonical source");
 require_should_dim_outside_owner_without_callbacks();require_initial_army_ai_return_without_callbacks();
 need(detail::bnw_game_active_player_read(*p->state)==p->actor&&p->state->ui_source_revision==q.world_revision&&q.field&&q.field->values.size()==1&&q.field->values.front()==q.after&&q.before_cache==q.after&&q.city_count<UINT64_MAX&&q.game_count<UINT64_MAX&&q.before.client_revision<=UINT64_MAX-2,"Init maintenance actual cache/actor/counters changed before UI store");
 const auto city_flag=q.ui_steps?std::optional<bool>{true}:q.city_flag;const auto city_count=q.city_count+(q.ui_steps?1:0);
 need(q.input->initial_unit_maintenance_matches_without_callbacks(city_flag,city_count,q.game_flag,q.game_count,q.current.client_revision)&&q.input->initial_moves_dirty_matches_without_callbacks(q.current),"Init maintenance actual UI phase source changed");
 return {p->actor,q.node->scalars.at("m_iID"),p->generation,q.world_revision,q.current.client_revision,q.ui_steps?InitialUnitMaintenanceDirtyKind::game_data:InitialUnitMaintenanceDirtyKind::city_info,city_flag,q.game_flag,city_count,q.game_count};
}
void BnwNativeUnitAllocatorCore::record_initial_unit_maintenance_dirty_store(InitialUnitMaintenanceDirtyKind kind)const noexcept{
 auto&q=*impl_->initial_maintenance;if(kind==InitialUnitMaintenanceDirtyKind::city_info)q.ui_steps=1;else q.ui_steps=2;q.current.client_revision=q.before.client_revision+q.ui_steps;
}
void BnwNativeUnitAllocatorCore::require_initial_maintenance_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_maintenance&&p->initial_maintenance->returned,"Init maintenance genuine return absent/foreign");require_should_dim_outside_owner_without_callbacks();require_initial_army_ai_return_without_callbacks();const auto&q=*p->initial_maintenance;
 need(q.stored&&q.node.get()==p->initial_army_ai->node.get()&&q.active_player==detail::bnw_game_active_player_read(*p->state)&&q.active==(q.active_player==p->actor)&&p->state->ui_source_revision==q.world_revision&&q.field==&initial_cycle_named(p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).core.fields,"m_iUnitProductionMaintenanceMod")&&q.field->values.size()==1&&q.field->values.front()==q.after&&q.before_cache==q.after,"Init maintenance genuine SAME cache/actor source changed");
 const auto actual=initial_maintenance_calculate(*p->host,*p->state,p->actor,*p->bank,*q.node);need(actual.inputs==q.inputs&&actual.value==q.after,"Init maintenance actual supply calculation operands changed");
 if(q.active)need(q.ui_steps==2&&initial_maintenance_prior_unit_dirty_matches_without_callbacks(q.input,q.before),"Init maintenance CityInfo18/GameData11 actual return absent");else need(!q.input&&!q.ui_steps,"Init maintenance inactive branch invented UI effects");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_maintenance_returned&&prefix.initial_maintenance_active==q.active&&prefix.initial_maintenance_value==q.after&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init maintenance earned Pending return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_maintenance_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_army_ai&&p->initial_army_ai->returned&&!p->initial_maintenance,"Init maintenance requires once actual ArmyAI return");
 try{p->check();require_initial_army_ai_return_without_callbacks();InitialMaintenancePrefix q;q.node=p->initial_army_ai->node;q.world_revision=p->state->ui_source_revision;q.active_player=detail::bnw_game_active_player_read(*p->state);q.active=q.active_player==p->actor;
  q.field=&initial_cycle_named(p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).core.fields,"m_iUnitProductionMaintenanceMod");need(q.field->encoding==saves::PlayerCoreEncoding::i32&&q.field->values.size()==1&&!q.field->wire_count&&q.field->bytes.empty(),"Init maintenance actual scalar cache encoding absent");q.before_cache=narrow(q.field->values.front());
  const auto calculation=initial_maintenance_calculate(*p->host,*p->state,p->actor,*p->bank,*q.node);q.after=calculation.value;q.inputs=calculation.inputs;
  need(q.after==q.before_cache,"Init maintenance changed production cache requires mandatory native scalar archive/current observer lineage before store");
  if(q.active){q.input=p->initial_set_moves->input;need(bool(q.input),"Init maintenance actual UI owner unavailable");q.before=q.input->unit_dirty_state();q.current=q.before;q.city_flag=q.input->initial_unit_maintenance_city_info_dirty();q.city_count=q.input->initial_unit_maintenance_city_info_setter_count();q.game_flag=q.input->game_data_dirty();q.game_count=q.input->game_data_dirty_setter_count();
   need(q.city_count<UINT64_MAX&&q.game_count<UINT64_MAX&&q.before.client_revision<=UINT64_MAX-2&&q.input->initial_unit_maintenance_matches_without_callbacks(q.city_flag,q.city_count,q.game_flag,q.game_count,q.before.client_revision),"Init maintenance actual UI preparation changed");}
  p->initial_maintenance.emplace(std::move(q));auto&stage=*p->initial_maintenance;p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_army_ai_return_without_callbacks();
  const auto current=initial_maintenance_calculate(*p->host,*p->state,p->actor,*p->bank,*stage.node);need(current.inputs==stage.inputs&&current.value==stage.after&&p->state->ui_source_revision==stage.world_revision&&detail::bnw_game_active_player_read(*p->state)==stage.active_player&&stage.field->values.front()==stage.before_cache&&!p->bank->retired.load(),"Init maintenance final actual calculation changed before store");
  // Real native equal-value FAutoVariable assignment: no dirty archive bit or
  // world epoch delta is fabricated. SDK nevertheless runs both UI setters.
  stage.field->values.front()=stage.after;stage.stored=true;
  if(stage.active){stage.input->set_initial_unit_maintenance_dirty(*canonical);need(stage.ui_steps==2&&initial_maintenance_prior_unit_dirty_matches_without_callbacks(stage.input,stage.before),"Init maintenance actual two UI normal returns missing");}
  stage.returned=true;p->initial_set_xy->record_initial_maintenance_return(stage.active,stage.after);require_initial_maintenance_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
struct InitialMinorQuestCalculation {std::vector<std::int64_t>inputs;std::uint32_t minors=0,quests=0,great_person_checks=0;bool tested=false;};
InitialMinorQuestCalculation initial_minor_quest_calculate(const LiveSessionState&s,std::int32_t actor,const Bank&bank,const PendingUnit&node){
 InitialMinorQuestCalculation q;auto emit=[&](auto n){q.inputs.push_back(static_cast<std::int64_t>(n));};
 need(s.world.pregame&&s.world.pregame->layout==bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&actor>=0&&actor<64,"Init minor quests actual BNW players/Pregame unavailable");
 const auto&minor=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(minor.size()==64&&minor.at(static_cast<std::size_t>(actor))==s.cities.players.at(actor).minor,"Init minor quests actual Minor predicate differs");emit(minor.at(static_cast<std::size_t>(actor)));
 if(minor.at(static_cast<std::size_t>(actor)))return q;emit(actor==63);if(actor==63)return q;q.tested=true;
 // SDK Init677: only alive slots MAX_MAJOR_CIVS .. MAX_CIV_PLAYERS-1.
 // DoCompletedQuestsForPlayer has an actual early return outside [0,22).
 for(std::int32_t id=22;id<63;++id){const auto alive=post_player(s,id,"m_bAlive");emit(id);emit(alive);if(!alive)continue;++q.minors;if(actor>=22)continue;
  const auto&player=s.world.all_players->players.at(static_cast<std::size_t>(id));need(player.index==static_cast<std::uint64_t>(id)&&player.saved_player_id==id&&post_player(s,id,"m_eID")==id,"Init quest actual minor player identity differs");
  need(player.planning.layout==bnw&&player.planning.dimensions.major==22,"Init quest actual BNW MinorAI dimensions unavailable");
  const auto&block=post_named(player.planning.blocks,"minor_civ_ai");need(post_named(block.fields,"version").value==1&&block.children.size()==22,"Init quest actual loaded MinorAI/quest roster unavailable");
  for(std::size_t assigned=0;assigned<block.children.size();++assigned){const auto&p=block.children[assigned];need(name(p.name)=="player_quests"&&p.index==assigned,"Init quest actual load-handler assigned-player position differs");}
  const auto&list=block.children.at(static_cast<std::size_t>(actor));const auto count=post_named(list.fields,"quests_wire_count").value;
  need(count>=0&&count<=INT32_MAX&&static_cast<std::uint64_t>(count)==list.children.size()&&list.fields.size()==1&&list.arrays.empty()&&list.blobs.empty(),"Init quest actual native signed list count/shape differs");
  emit(list.start_offset);emit(list.end_offset);emit(count);
  for(std::size_t pos=0;pos<list.children.size();++pos){const auto&quest=list.children[pos];need(name(quest.name)=="quest"&&quest.index==pos&&quest.arrays.empty()&&quest.blobs.empty()&&quest.children.empty(),"Init quest actual list entry shape/order differs");
   const auto version=post_named(quest.fields,"version").value;need(version>=1&&version<=2&&quest.fields.size()==static_cast<std::size_t>(version==1?5:6),"Init quest verified native serialized version unavailable");
   const auto type=narrow(post_named(quest.fields,"type").value),start=narrow(post_named(quest.fields,"start_turn").value),data1=narrow(post_named(quest.fields,"data1").value),data2=narrow(post_named(quest.fields,"data2").value);
   // Original loader assigns m_eMinor=id and m_eAssignedPlayer=list index;
   // v1's handled=false comes from the original actual load handler.
   const auto handled=version==1?0:post_named(quest.fields,"handled").value;need(handled==0||handled==1,"Init quest actual handled Boolean malformed");
   emit(pos);emit(quest.start_offset);emit(quest.end_offset);emit(version);emit(type);emit(start);emit(data1);emit(data2);emit(handled);++q.quests;
   constexpr std::int32_t great_person=4; // SDK CvMinorCivAI.h48-56, enum 0..17.
   if(type!=great_person)continue;++q.great_person_checks;
   check_imported_cycle_nodes(bank,s,actor);need(bank.pending_units.size()==1&&bank.pending_units.at(bank.pending_slot).get()==&node,"Init GP quest actual unpublished unit union unavailable");
   std::int32_t visited=0;bool complete=false;
   for(std::int32_t slot=0;slot<=bank.last;++slot){const auto it=bank.imported_cycle_nodes.find(slot);const bool newborn=slot==bank.pending_slot;need(!(newborn&&it!=bank.imported_cycle_nodes.end()),"Init GP quest native full-ID slot collision");if(it==bank.imported_cycle_nodes.end()&&!newborn)continue;
    const auto full_id=newborn?node.scalars.at("m_iID"):it->second->id;const auto unit_type=newborn?node.scalars.at("m_eUnitType"):narrow(s.maintenance.players.at(actor).units.at(full_id).type);
    if(newborn)need(node.scalars.at("m_eOwner")==actor,"Init GP quest actual newborn owner differs");else need(core(*it->second->source,"m_iID")==full_id&&core(*it->second->source,"m_eOwner")==actor,"Init GP quest imported native identity differs");
    ++visited;emit(slot);emit(full_id);emit(unit_type);if(unit_type==data1){complete=true;break;}
   }
   emit(complete);if(!complete)need(visited==narrow(std::int64_t(bank.last)+1-bank.free_count),"Init GP quest actual firstUnit/nextUnit traversal differs");
   // IsComplete=true always calls GetEffectiveFriendship before DoFinishQuest,
   // even for already-handled quests. Never silently bypass these effects.
   need(!complete,"Init completed GreatPerson quest requires actual friendship/reward/notification/DiplomacyAI consumers before effects");
  }
 }
 return q;
}
}
void BnwNativeUnitAllocatorCore::require_initial_minor_quest_return_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_minor_quest&&p->initial_minor_quest->returned,"Init MinorAI actual return absent/foreign");require_should_dim_outside_owner_without_callbacks();require_initial_maintenance_return_without_callbacks();const auto&q=*p->initial_minor_quest;
 need(q.node.get()==p->initial_maintenance->node.get(),"Init MinorAI SAME native node changed");const auto current=initial_minor_quest_calculate(*p->state,p->actor,*p->bank,*q.node);
 need(current.inputs==q.inputs&&current.minors==q.minors&&current.quests==q.quests&&current.great_person_checks==q.great_person_checks&&current.tested==q.tested,"Init MinorAI actual quest/roster/no-completion operands changed");
 const auto&prefix=p->initial_set_xy->initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_minor_quest_returned&&prefix.initial_minor_quest_tested==q.tested&&prefix.initial_minor_quest_minors==q.minors&&prefix.initial_minor_quest_entries==q.quests&&prefix.initial_minor_quest_gp_checks==q.great_person_checks&&prefix.boundary==BnwUnitMoveBoundary::initial_post_moves_owner,"Init MinorAI earned Pending return differs");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_minor_quest_prefix()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_maintenance&&p->initial_maintenance->returned&&!p->initial_minor_quest,"Init MinorAI requires once actual maintenance return");
 try{p->check();require_initial_maintenance_return_without_callbacks();const auto calculation=initial_minor_quest_calculate(*p->state,p->actor,*p->bank,*p->initial_maintenance->node);
  InitialMinorQuestPrefix q;q.node=p->initial_maintenance->node;q.inputs=calculation.inputs;q.minors=calculation.minors;q.quests=calculation.quests;q.great_person_checks=calculation.great_person_checks;q.tested=calculation.tested;
  p->check();p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_maintenance_return_without_callbacks();const auto current=initial_minor_quest_calculate(*p->state,p->actor,*p->bank,*q.node);
  need(current.inputs==q.inputs&&current.minors==q.minors&&current.quests==q.quests&&current.great_person_checks==q.great_person_checks&&current.tested==q.tested&&!p->bank->retired.load(),"Init MinorAI final actual no-completion source changed");
  q.returned=true;p->initial_minor_quest.emplace(std::move(q));const auto&stage=*p->initial_minor_quest;p->initial_set_xy->record_initial_minor_quest_return(stage.tested,stage.minors,stage.quests,stage.great_person_checks);require_initial_minor_quest_return_without_callbacks();return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
struct NativeFriendlyStackInfo {std::int32_t type=-1,domain=-1,combat=0,range=0;bool trade=false;};
struct NativeFriendlyStackCalculation {std::vector<std::int64_t>inputs;std::int32_t count=0;};
std::int32_t native_friendly_tail(const saves::SavedUnitObservation&unit,std::string_view key){
 const saves::UnitTailNode*found=nullptr;const auto walk=[&](auto&&self,const saves::UnitTailNode&n)->void{if(name(n.name)==key){need(!found,"Native friendly stack duplicate tail field");found=&n;}for(const auto&child:n.children)self(self,child);};
 need(unit.tail.layout==bnw,"Native friendly stack actual tail absent");for(const auto&block:unit.tail.blocks)walk(walk,block);
 need(found&&name(found->encoding)=="i32"&&found->values.size()==1&&found->bytes.empty()&&found->children.empty(),"Native friendly stack actual transport member unavailable");return narrow(found->values.front());
}
NativeFriendlyStackInfo native_friendly_info(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const saves::SavedUnitObservation&unit,bool trade_only=false){
 const auto actor=narrow(core(unit,"m_eOwner")),full_id=narrow(core(unit,"m_iID"));const auto hash=core(unit,"idUnitType");need(hash>0&&hash<=UINT32_MAX,"Native friendly stack UnitInfo hash unavailable");
 const auto*entry=host.types().find_hash(static_cast<std::uint32_t>(hash));need(entry&&entry->table=="Units"&&entry->id>=0&&entry->hash==content::saved_info_type_hash(entry->type)&&actor>=0&&actor<64,"Native friendly stack actual UnitInfo binding unavailable");
 const auto&owned=s.maintenance.players.at(actor).units.at(full_id);need(owned.type==entry->id&&owned.source_core==std::optional{std::pair{unit.core.start_offset,unit.core.end_offset}},"Native friendly stack actual raw/owned type/span differs");
 NativeFriendlyStackInfo result;result.type=narrow(entry->id);bool found=false;
 const auto report=host.database().query_rows({"SELECT ID,Type,Domain,Combat,Range,Trade FROM Units WHERE ID=?",{std::int64_t(result.type)}},[&](const auto&r){need(!found&&r.integer("ID")==result.type&&r.text("Type")==std::optional<std::string>{entry->type},"Native friendly stack actual UnitInfo identity changed");found=true;(void)narrow(r.integer("Trade"));result.trade=r.boolean("Trade");if(trade_only)return;
  const auto domain=r.text("Domain");const auto*d=domain?host.types().find(*domain):nullptr;constexpr std::array<std::string_view,5>names={"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
  need(d&&d->table=="Domains"&&d->id>=0&&d->id<5&&d->type==names.at(static_cast<std::size_t>(d->id))&&d->hash==content::saved_info_type_hash(d->type),"Native friendly stack actual Domain enum differs");result.domain=narrow(d->id);result.combat=narrow(r.integer("Combat"));result.range=narrow(r.integer("Range"));});need(report.all_succeeded()&&found,"Native friendly stack actual UnitInfo unavailable");return result;
}
NativeFriendlyStackCalculation native_friendly_count(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const saves::SavedUnitObservation&query,std::uint64_t index,bool break_on_limit){
 NativeFriendlyStackCalculation q;auto emit=[&](auto n){q.inputs.push_back(static_cast<std::int64_t>(n));};
 const auto actor=narrow(core(query,"m_eOwner")),id=narrow(core(query,"m_iID"));emit(actor);emit(id);emit(index);emit(break_on_limit);const auto info=native_friendly_info(host,s,query,true);emit(info.type);emit(info.trade);
 // Original trade exemption returns BEFORE plot/domain/team/transport reads.
 if(info.trade)return q;(void)narrow(core(query,"m_iBaseCombat"));
 need(s.world.map_plots&&s.world.map_projection&&s.world.map_plots->layout==bnw&&s.world.map_plots->plots.size()==s.world.map_header.grid.size()&&s.world.map_projection->cells.size()==s.world.map_header.grid.size()&&index<s.world.map_plots->plots.size(),"Native friendly stack actual target map unavailable");
 const auto&raw=s.world.map_plots->plots.at(index);const auto&cell=s.world.map_projection->cells.at(index);const auto plot_type=narrow(post_named(raw.fields,"plot_type").value);need(raw.index==index&&cell.index==index&&cell.plot_type==std::optional<std::int64_t>{plot_type},"Native friendly stack actual target plot differs");emit(plot_type);
 need(plot_type!=3,"Native friendly stack water target requires actual canEmbarkOnto(origin,target) owner before bPretendEmbarked");
 const auto team=post_player_team(s,actor);need(team>=0,"Native friendly stack actual querying team absent");emit(team);
 const auto&links=post_named(raw.arrays,"units");need(links.columns==2&&links.values.size()%2==0&&cell.units&&cell.units->size()==links.values.size()/2,"Native friendly stack actual raw/projected linked roster shape differs");
 for(std::size_t k=0;k<cell.units->size();++k)need(cell.units->at(k).owner==links.values[k*2]&&cell.units->at(k).id==links.values[k*2+1],"Native friendly stack actual raw/projected linked roster order differs");
 const auto*d=host.defines().find("PLOT_UNIT_LIMIT");need(host.defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),"Native friendly stack actual PLOT_UNIT_LIMIT unavailable");const auto limit=narrow(std::get<std::int64_t>(d->lookup.value));emit(limit);
 for(std::size_t k=0;k<links.values.size();k+=2){const auto owner=narrow(links.values[k]),full_id=narrow(links.values[k+1]);emit(owner);emit(full_id);
  // GetPlayerUnit/getUnit accepts NO_PLAYER and stale serial/empty slot as NULL.
  if(owner==-1){emit(false);continue;}need(owner>=0&&owner<64,"Native friendly stack linked owner outside actual roster");const auto*unit=native_imported_get_at(s,owner,full_id);emit(bool(unit));if(!unit)continue;
  need(unit->core.layout==bnw&&core(*unit,"m_eOwner")==owner,"Native friendly stack resolved object owner differs");const auto their_team=post_player_team(s,owner);need(their_team>=0,"Native friendly stack actual loop team absent");const auto war=post_relation(s,team,their_team,"at_war");emit(their_team);emit(war);if(war)continue;
  bool same=owner!=actor;emit(same);
  if(!same){const auto loop_embarked=narrow(core(*unit,"m_bEmbarked")),query_embarked=narrow(core(query,"m_bEmbarked"));need((loop_embarked==0||loop_embarked==1)&&(query_embarked==0||query_embarked==1),"Native friendly stack actual embark Boolean malformed");emit(loop_embarked);emit(query_embarked);
   if(loop_embarked&&query_embarked)same=true;else{const auto loop_info=native_friendly_info(host,s,*unit),query_info=native_friendly_info(host,s,query);need(!query_info.trade&&query_info.type==info.type,"Native friendly stack original querying Info changed");emit(loop_info.type);emit(loop_info.domain);emit(loop_info.combat);emit(loop_info.range);emit(query_info.domain);emit(query_info.combat);emit(query_info.range);
    // SDK GameQueries uses Info Combat OR Range (not raw BaseCombat), HOVER
    // wildcard matching, and the native air-stacking exclusion.
    auto first=loop_info.domain,second=query_info.domain;if(first==4&&second!=4)first=second;else if(second==4&&first!=4)second=first;
    same=first==second&&first!=1&&((loop_info.combat>0||loop_info.range>0)==(query_info.combat>0||query_info.range>0));
   }
  }
  emit(same);if(same){const auto transport_owner=native_friendly_tail(*unit,"m_transportUnit.eOwner"),transport_id=native_friendly_tail(*unit,"m_transportUnit.iID");emit(transport_owner);emit(transport_id);
   bool cargo=false;if(transport_owner!=-1){need(transport_owner>=0&&transport_owner<64,"Native friendly stack actual transport owner outside roster");cargo=native_imported_get_at(s,transport_owner,transport_id)!=nullptr;}emit(cargo);if(!cargo)q.count=narrow(std::int64_t(q.count)+1);
  }
  // Exact default bBreakOnUnitLimit=true: break even when this loop object
  // did not increment. The branch stays inside the non-enemy native block.
  if(break_on_limit&&q.count>limit)break;
 }
 return q;
}
}
std::optional<std::int32_t>BnwNativeUnitAllocatorCore::read_native_existing_friendly_stack_count(std::int32_t query_id,std::uint64_t plot,bool break_on_limit)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard guard(p);p->check();
 need(p->state->world.map_plots&&p->state->world.map_plots->layout==bnw&&plot<p->state->world.map_plots->plots.size()&&p->state->world.map_plots->plots.at(plot).index==plot,"Native friendly count requires a real target CvPlot object");
 // A quarantined partial newborn must never disappear from GetPlayerUnit.
 // Imported query scope is available only while every Bank has no newborn.
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[owner,bank]:r->players){(void)owner;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Native imported friendly query requires published native unit rosters; a quarantined newborn remains mandatory");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 const auto first=native_friendly_count(*p->host,*p->state,*unit,plot,break_on_limit);p->check();need(native_imported_get_at(*p->state,p->actor,query_id)==unit,"Native friendly query actual manager object changed during reads");
 const auto second=native_friendly_count(*p->host,*p->state,*unit,plot,break_on_limit);need(second.inputs==first.inputs&&second.count==first.count,"Native friendly query actual operands changed across rules/source reads");p->check();(void)issuer->current();return first.count;
}

std::optional<bool>BnwNativeUnitAllocatorCore::read_native_existing_has_no_valid_move(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard guard(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[owner,bank]:r->players){(void)owner;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Native imported IsHasNoValidMove requires published native unit rosters; a quarantined newborn remains mandatory");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 // CvUnit::plot() is the actual current map coordinate. No prospective
 // command target, caller-supplied count or copied pathfinder verdict enters.
 const auto calculate=[&](){
  need(p->state->world.map_plots&&p->state->world.map_projection&&p->state->world.map_plots->layout==bnw,"Native IsHasNoValidMove actual current map absent");const auto&map=*p->state->world.map_projection;
  const auto x=narrow(core(*unit,"m_iX")),y=narrow(core(*unit,"m_iY"));need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<map.grid.width&&static_cast<std::uint64_t>(y)<map.grid.height,"Native IsHasNoValidMove actual current plot is null");const auto index=map.grid.index(x,y);
  need(index<p->state->world.map_plots->plots.size()&&p->state->world.map_plots->plots.at(index).index==index&&index<map.cells.size()&&map.cells.at(index).index==index,"Native IsHasNoValidMove actual current CvPlot object differs");
  auto q=native_friendly_count(*p->host,*p->state,*unit,index,true);q.inputs.push_back(x);q.inputs.push_back(y);
  // Native IsHasNoValidMove reads this define after count even when the
  // actual trade exemption avoided its read inside CvPlot's count.
  const auto*d=p->host->defines().find("PLOT_UNIT_LIMIT");need(p->host->defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),"Native IsHasNoValidMove actual PLOT_UNIT_LIMIT unavailable");const auto limit=narrow(std::get<std::int64_t>(d->lookup.value));q.inputs.push_back(limit);return std::pair{std::move(q),limit};
 };
 const auto first=calculate();p->check();need(native_imported_get_at(*p->state,p->actor,query_id)==unit,"Native IsHasNoValidMove actual native manager object changed during reads");const auto second=calculate();need(second.first.inputs==first.first.inputs&&second.first.count==first.first.count&&second.second==first.second,"Native IsHasNoValidMove actual source/rules operands changed during reads");p->check();(void)issuer->current();
 if(first.first.count<=first.second)return false;
 // SDK switches actual CvTwoLayerPathFinder destination/valid callbacks,
 // generates the unit path, restores all three callbacks and ForceReset.
 // Neither an empty approximate BFS nor an invented successful reset may
 // replace that mandatory mutation for an over-limit current position.
 throw BnwNativeUnitAllocatorUnavailable("Native IsHasNoValidMove over-limit current plot requires actual CvTwoLayerPathFinder FindValidDestination callbacks, GenerateUnitPath, restoration and ForceReset");
}

BnwPlotUnitReadServices BnwNativeUnitAllocatorCore::bind_native_existing_plot_unit_reads(BnwPlotUnitReadServices services)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard guard(p);p->check();
 // Only weak canonical lineage survives binding. Each actual invocation
 // acquires exactly one temporary counted Core borrower, then validates the
 // SAME LiveSessionState before reading any supplied world's members.
 const auto read=[lineage=factory_self_](const LiveSessionState&state,std::uint64_t plot,saves::SavedPlotUnitReference handle,bool cargo)->std::optional<std::int64_t>{
  const auto borrower=ShouldDimCoreBorrowedOwner::from_weak(lineage);const auto&owner=borrower->current();const auto q=owner.impl_;Impl::Guard invocation(q);q->check();need(&state==q->state.get(),"Native bound plot read received a foreign or copied world");
  if(handle.owner!=q->actor){q->check();(void)borrower->current();return std::nullopt;}
  need(handle.id>=INT32_MIN&&handle.id<=INT32_MAX,"Native bound plot read full ID outside native int32");
  need(state.world.map_plots&&state.world.map_plots->layout==bnw&&plot<state.world.map_plots->plots.size()&&state.world.map_plots->plots.at(plot).index==plot,"Native bound plot read requires an actual target CvPlot");
  if(const auto*r=state.setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Native bound imported plot read requires published rosters; quarantined newborn remains mandatory");}
  const auto full_id=static_cast<std::int32_t>(handle.id);const auto*unit=native_imported_get_at(state,q->actor,full_id);if(!unit){q->check();(void)borrower->current();return std::nullopt;}
  if(!cargo){const auto first=native_friendly_count(*q->host,state,*unit,plot,true);q->check();need(native_imported_get_at(state,q->actor,full_id)==unit,"Native bound count manager object changed during reads");const auto second=native_friendly_count(*q->host,state,*unit,plot,true);need(second.inputs==first.inputs&&second.count==first.count,"Native bound count source/rules operands changed during reads");q->check();(void)borrower->current();return first.count;}
  const auto calculate=[&](){const auto transport_owner=native_friendly_tail(*unit,"m_transportUnit.eOwner"),transport_id=native_friendly_tail(*unit,"m_transportUnit.iID");bool found=false;const saves::SavedUnitObservation*transport=nullptr;if(transport_owner!=-1){need(transport_owner>=0&&transport_owner<64,"Native bound cargo transport owner outside native roster");transport=native_imported_get_at(state,transport_owner,transport_id);found=transport!=nullptr;}return std::tuple{transport_owner,transport_id,transport,found};};
  const auto first=calculate();q->check();need(native_imported_get_at(state,q->actor,full_id)==unit,"Native bound cargo manager object changed during reads");const auto second=calculate();need(second==first,"Native bound cargo transport GetAt changed during reads");q->check();(void)borrower->current();return std::get<3>(first)?1:0;
 };
 services.friendly_units_of_type=[read](const LiveSessionState&state,std::uint64_t plot,saves::SavedPlotUnitReference handle){return read(state,plot,handle,false);};
 services.cargo=[read](const LiveSessionState&state,std::uint64_t plot,saves::SavedPlotUnitReference handle)->std::optional<bool>{const auto result=read(state,plot,handle,true);if(!result)return std::nullopt;return *result!=0;};
 // Replacing previous callback storage can run user-owned destructors.
 // Revalidate only after both replacements and before this real return.
 p->check();(void)issuer->current();return services;
}


BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::InitialUnitSetupGraphicalPlan(std::shared_ptr<Storage>s):storage_(std::move(s)){}
BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::~InitialUnitSetupGraphicalPlan(){retire_scope_without_callbacks();}
void BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::retire_scope_without_callbacks()const noexcept{if(storage_)storage_->retire();}
void BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::require_owner_without_callbacks()const{
 need(storage_&&storage_->creator==std::this_thread::get_id(),"Foreign setupGraphical Plan before mutable borrow");need(storage_->alive.load(),"setupGraphical issuing Core scope retired");const auto actual=storage_->canonical.lock();need(actual&&actual.get()==this&&actual->storage_.get()==storage_.get(),"setupGraphical Plan is not canonical heap object");need(actual.use_count()>static_cast<long>(storage_->borrowers.load()+1),"setupGraphical counted Plan pins cannot replace issuing Core owner");const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);need(&core->current()==storage_->binding.core,"setupGraphical issuing Core expired");
}
std::shared_ptr<const BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan>BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::retain_factory_owner()const{require_owner_without_callbacks();return storage_->canonical.lock();}
BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner::BorrowedOwner(const InitialUnitSetupGraphicalPlan&p):BorrowedOwner(p.retain_factory_owner()){}
BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner::BorrowedOwner(std::shared_ptr<const InitialUnitSetupGraphicalPlan>p):owner_(std::move(p)){
 need(owner_&&owner_->storage_&&owner_->storage_->creator==std::this_thread::get_id(),"Foreign setupGraphical counted Plan acquisition");owner_->storage_->borrowers.fetch_add(1);try{(void)current();}catch(...){owner_->storage_->borrowers.fetch_sub(1);owner_.reset();throw;}
}
std::unique_ptr<BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner>BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner::from_weak(const std::weak_ptr<const InitialUnitSetupGraphicalPlan>&weak){auto owner=weak.lock();need(bool(owner),"setupGraphical weak canonical Plan expired");return std::unique_ptr<BorrowedOwner>(new BorrowedOwner(std::move(owner)));}
BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner::~BorrowedOwner(){if(owner_){owner_->storage_->borrowers.fetch_sub(1);owner_.reset();}}
const BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::BorrowedOwner::current()const{need(bool(owner_),"setupGraphical counted Plan pin absent");owner_->require_owner_without_callbacks();return *owner_;}
BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::Binding BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::capture_binding()const{require_setup_graphical_enqueue_dispatch_without_callbacks();return storage_->binding;}
void BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::require_setup_graphical_enqueue_dispatch_without_callbacks()const{require_owner_without_callbacks();const auto core=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);core->current().require_initial_setup_graphical_plan_without_callbacks(*this);}
BnwNativeUnitAllocatorCore::InitialPlacementOperands BnwNativeUnitAllocatorCore::initial_setup_graphical_native_operands()const{
 require_initial_minor_quest_return_without_callbacks();const auto p=impl_;need(p->bank->train&&p->initial_minor_quest&&p->initial_minor_quest->node,"setupGraphical actual TRAIN/node source absent");const auto&u=*p->initial_minor_quest->node;check_registered(*p->bank,u,p->initial_minor_quest->node,p->bank->pending_slot);return {p->host,p->host->session_.get(),p->state,p->actor,u.scalars.at("m_iID"),u.scalars.at("m_iX"),u.scalars.at("m_iY"),p->bank->train->city,u.scalars.at("m_iBaseCombat"),p->generation,p->state->ui_source_revision};
}
void BnwNativeUnitAllocatorCore::require_initial_setup_graphical_plan_without_callbacks(const InitialUnitSetupGraphicalPlan&plan)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign setupGraphical Core before mutable source read");require_should_dim_outside_owner_without_callbacks();need(plan.storage_&&plan.storage_->alive.load()&&plan.storage_->creator==p->thread&&p->initial_setup_graphical_plan.get()==&plan&&p->initial_setup_graphical_plan->storage_.get()==plan.storage_.get(),"setupGraphical Plan was not issued by current Core scope");require_initial_minor_quest_return_without_callbacks();const auto&q=*plan.storage_;const auto&b=q.binding;
 need(b.core==this&&!b.core_control.owner_before(factory_self_)&&!factory_self_.owner_before(b.core_control)&&b.host.get()==p->host.get()&&!b.host.owner_before(p->host)&&!p->host.owner_before(b.host)&&b.session==p->host->session_.get()&&b.state.get()==p->state.get()&&!b.state.owner_before(p->state)&&!p->state.owner_before(b.state)&&b.source==p->source&&b.pending.get()==p->initial_set_xy.get()&&!b.pending.owner_before(p->initial_set_xy)&&!p->initial_set_xy.owner_before(b.pending)&&b.bank==p->bank.get()&&!b.bank_control.owner_before(p->bank)&&!p->bank.owner_before(b.bank_control)&&b.node==q.node.get()&&!b.node_control.owner_before(q.node)&&!q.node.owner_before(b.node_control)&&q.node.get()==p->initial_minor_quest->node.get()&&b.slot==p->bank->pending_slot&&b.generation==p->generation&&b.world_revision==p->state->ui_source_revision&&b.input_revision==p->initial_maintenance->input->initial_center_revision_without_callbacks(),"setupGraphical actual Core/Host/Session/Bank/node/source controls changed");
 need(q.node->scalars==q.scalars&&q.node->indexed==q.indexed&&q.node->strings==q.strings&&q.node->dirty==q.dirty&&q.node->promotion_bits==q.bits&&q.node->promotion_vector_sets==q.promotion_sets&&q.node->mission_queue==q.missions&&q.node->religion==q.religion&&q.node->religion_strength==q.strength&&q.node->religion_spreads==q.spreads&&q.node->last_path_destination==q.last_path&&b.owner==q.node->scalars.at("m_eOwner")&&b.full_id==q.node->scalars.at("m_iID")&&b.x==q.node->scalars.at("m_iX")&&b.y==q.node->scalars.at("m_iY")&&b.plot==p->state->world.map_header.grid.index(b.x,b.y),"setupGraphical actual immutable archive/mission/coordinate snapshot changed");check_registered(*p->bank,*q.node,q.node,b.slot);
 need(q.attempt&&q.scope&&q.scope->serial==b.attempt_serial&&q.scope->world==p->state.get(),"setupGraphical fresh SAME source scope absent");q.attempt->require_scope_identity_without_callbacks(*q.scope);const auto pins=q.scope->control->transport_source_pins.load();need(q.scope->source_owner&&pins<=static_cast<std::uint64_t>(LONG_MAX)&&q.scope->source_owner.use_count()>static_cast<long>(pins),"setupGraphical counted source anchors cannot replace outside source owner");
 if(q.garrison)BnwMinorCityStrengthScratch::require_initial_setup_garrison_without_callbacks(*this,*q.garrison);
}
void BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::prepare_native_readers()const{
 require_setup_graphical_enqueue_dispatch_without_callbacks();const auto borrower=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);const auto&owner=borrower->current();const auto p=owner.impl_;need(!storage_->readers_prepared&&!storage_->work_rate&&!storage_->garrison,"setupGraphical native readers already prepared");
 // Loan calls this only after its authentic GraphicsInitialized true gate.
 // False gate never evaluates UnitInfo WorkRate, missions or garrison.
 if(storage_->node->scalars.at("m_bEmbarked")!=0){storage_->readers_prepared=true;return;}
 const auto type=storage_->node->scalars.at("m_eUnitType");const auto&info=initial_maintenance_info(*p->host,"Units",type);const auto read_work=[&](){std::optional<std::int32_t>value;const auto report=p->host->database().query_rows({"SELECT ID,Type,WorkRate FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&r){need(!value&&r.integer("ID")==type&&r.text("Type")==std::optional<std::string>{info.type},"setupGraphical actual UnitInfo WorkRate identity differs");value=narrow(r.integer("WorkRate"));});need(report.all_succeeded()&&value,"setupGraphical actual WorkRate unavailable");return *value;};
 const auto first=read_work();p->check();require_setup_graphical_enqueue_dispatch_without_callbacks();const auto second=read_work();need(second==first,"setupGraphical WorkRate changed across native source/rules reads");p->check();require_setup_graphical_enqueue_dispatch_without_callbacks();
 need(first<=0||storage_->node->mission_queue.empty(),"setupGraphical actual nonempty mission head requires native GetBuildType Build/Route callbacks");
 std::optional<BnwMinorCityStrengthScratch::InitialSetupGarrisonQuote>garrison;
 if(storage_->node->scalars.at("m_iMapLayer")==0){const auto args=owner.initial_setup_graphical_native_operands();const auto&plot=p->state->world.map_plots->plots.at(storage_->binding.plot);const auto city_owner=narrow(post_named(plot.fields,"plot_city_owner").value),city_id=narrow(post_named(plot.fields,"plot_city_id").value);const auto plot_owner=narrow(post_named(plot.fields,"owner").value);if(city_owner!=-1){need(city_owner>=0&&city_owner<64&&city_id>=0,"setupGraphical native plot-city handle invalid");const auto*actual_city=resolve_bnw_city_registry(p->state->world,p->state->cities,city_owner,city_id);if(actual_city&&plot_owner==args.owner){need(city_owner==args.owner,"setupGraphical actual city/plot owner differs");garrison=BnwMinorCityStrengthScratch::prepare_initial_setup_garrison(owner,city_id);}}}
 p->check();require_setup_graphical_enqueue_dispatch_without_callbacks();(void)borrower->current();storage_->work_rate=first;storage_->garrison=std::move(garrison);storage_->readers_prepared=true;require_setup_graphical_enqueue_dispatch_without_callbacks();
}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_owner_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();return storage_->node->scalars.at("m_eOwner");}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_full_id_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();return storage_->node->scalars.at("m_iID");}
bool BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_embarked_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();const auto n=storage_->node->scalars.at("m_bEmbarked");need(n==0||n==1,"setupGraphical actual embark boolean malformed");return n!=0;}
bool BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_should_dim_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();return storage_->node->scalars.at("m_iMoves")<=0;}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_fortify_turns_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();return storage_->node->scalars.at("m_iFortifyTurns");}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_damage_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();return storage_->node->scalars.at("m_iDamage");}
bool BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_is_work_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();need(storage_->readers_prepared&&storage_->work_rate,"setupGraphical actual WorkRate not prepared after genuine graphics gate");return *storage_->work_rate>0;}
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_build_type_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();need(storage_->readers_prepared&&storage_->work_rate&&*storage_->work_rate>0&&storage_->node->mission_queue.empty(),"setupGraphical actual work head requires genuine GetBuildType owner");return -1;}
bool BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_is_garrisoned_without_callbacks()const{require_setup_graphical_enqueue_dispatch_without_callbacks();need(storage_->readers_prepared,"setupGraphical current best defender not prepared after actual graphics gate");return storage_->garrison&&storage_->garrison->selected==std::optional{storage_->binding.full_id};}
void BnwNativeUnitAllocatorCore::require_initial_setup_graphical_return_without_callbacks(const BnwUnitMovePending&pending)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign completed setupGraphical return before mutable read");require_should_dim_outside_owner_without_callbacks();require_initial_minor_quest_return_without_callbacks();need(p->initial_set_xy.get()==&pending&&p->initial_setup_graphical_returned&&p->initial_setup_graphical_proof&&p->initial_setup_graphical_plan,"setupGraphical original publisher sequence did not earn return");const auto&q=*p->initial_setup_graphical_plan->storage_;need(q.node.get()==p->initial_minor_quest->node.get()&&q.node->scalars==q.scalars&&q.node->indexed==q.indexed&&q.node->dirty==q.dirty&&q.node->promotion_bits==q.bits&&q.node->promotion_vector_sets==q.promotion_sets&&q.node->mission_queue==q.missions&&q.node->strings==q.strings&&q.node->religion==q.religion&&q.node->religion_strength==q.strength&&q.node->religion_spreads==q.spreads&&q.node->last_path_destination==q.last_path&&q.binding.generation==p->generation&&q.binding.world_revision==p->state->ui_source_revision&&q.binding.input_revision==p->initial_maintenance->input->initial_center_revision_without_callbacks(),"setupGraphical completed archived node/source changed");if(q.garrison)BnwMinorCityStrengthScratch::require_initial_setup_garrison_without_callbacks(*this,*q.garrison);const auto&prefix=pending.initial_setxy_tail_prefix_without_callbacks();need(prefix.initial_setup_graphical_returned,"setupGraphical earned Pending return missing");
 // No retired Plan/Attempt/Lua callback or renderer/pump acceptance here.
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_setup_graphical_enqueue(const graphics::BnwInitialUnitSetupGraphicalRendererLoan&loan,const BnwStockLuaAttemptLease&attempt)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto&canonical=issuer->owner_;const auto p=impl_;Impl::Guard guard(p);need(p->train_source&&p->initial_minor_quest&&p->initial_minor_quest->returned&&!p->initial_setup_graphical_plan&&!p->initial_setup_graphical_proof&&!p->initial_setup_graphical_returned,"Final original setupGraphical requires once actual minor quest return");
 try{p->check();require_initial_minor_quest_return_without_callbacks();auto s=std::make_shared<InitialUnitSetupGraphicalPlan::Storage>();s->attempt=attempt;s->scope=attempt.pin_active_scope_without_callbacks();need(s->scope->world==p->state.get(),"setupGraphical fresh attempt belongs to another world");s->node=p->initial_minor_quest->node;const auto&u=*s->node;s->scalars=u.scalars;s->indexed=u.indexed;s->strings=u.strings;s->dirty=u.dirty;s->bits=u.promotion_bits;s->promotion_sets=u.promotion_vector_sets;s->missions=u.mission_queue;s->religion=u.religion;s->strength=u.religion_strength;s->spreads=u.religion_spreads;s->last_path=u.last_path_destination;auto&b=s->binding;b.core=this;b.core_control=canonical;b.host=p->host;b.session=p->host->session_.get();b.state=p->state;b.source=p->source;b.pending=p->initial_set_xy;b.bank=p->bank.get();b.bank_control=p->bank;b.node=s->node.get();b.node_control=s->node;b.owner=u.scalars.at("m_eOwner");b.full_id=u.scalars.at("m_iID");b.slot=p->bank->pending_slot;b.x=u.scalars.at("m_iX");b.y=u.scalars.at("m_iY");b.generation=p->generation;b.world_revision=p->state->ui_source_revision;b.input_revision=p->initial_maintenance->input->initial_center_revision_without_callbacks();b.plot=p->state->world.map_header.grid.index(b.x,b.y);b.attempt_serial=s->scope->serial;
  p->initial_setup_graphical_plan=std::shared_ptr<const InitialUnitSetupGraphicalPlan>(new InitialUnitSetupGraphicalPlan(s));s->canonical=p->initial_setup_graphical_plan;struct ScopeRetirement{const InitialUnitSetupGraphicalPlan*plan;~ScopeRetirement(){plan->retire_scope_without_callbacks();}}retirement{p->initial_setup_graphical_plan.get()};
  loan.preflight_setup_graphical_enqueue(*p->initial_setup_graphical_plan);p->check();require_initial_setup_graphical_plan_without_callbacks(*p->initial_setup_graphical_plan);const auto proof=loan.enqueue_setup_graphical(*p->initial_setup_graphical_plan);need(bool(proof),"setupGraphical actual gate/queue publisher proof absent");{const auto sealed=proof->retain_factory_owner();need(sealed.get()==proof.get()&&!sealed.owner_before(proof)&&!proof.owner_before(sealed),"setupGraphical proof is not canonical actual publisher owner");}loan.require_setup_graphical_enqueued_current(*p->initial_setup_graphical_plan,*proof);proof->require_for_plan(*p->initial_setup_graphical_plan);p->check();require_initial_setup_graphical_plan_without_callbacks(*p->initial_setup_graphical_plan);proof->require_current_for_core(*canonical);
  // This private effect is earned only after the concrete native-order gate
  // or all actual queue members, not a supplied success flag/count/cache ACK.
  require_initial_setup_graphical_plan_without_callbacks(*p->initial_setup_graphical_plan);p->initial_setup_graphical_proof=proof;p->initial_setup_graphical_returned=true;p->initial_set_xy->record_initial_setup_graphical_return();require_initial_setup_graphical_return_without_callbacks(*p->initial_set_xy);return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

namespace {
bool existing_path_bool(const saves::SavedUnitObservation&u,const char*name){
 const auto&f=post_named(u.core.fields,name);need(u.core.layout==bnw&&f.encoding==saves::UnitCoreEncoding::boolean&&f.values.size()==1&&!f.wire_count&&f.bytes.empty(),"UnitPath native Boolean source unavailable");const auto v=narrow(f.values.front());need(v==0||v==1,"UnitPath native Boolean outside domain");return v!=0;
}
std::int32_t existing_path_trait(const LiveSessionState&s,std::int32_t actor,const char*name){
 const auto&t=s.world.all_players->players.at(actor).traits;const auto&f=post_named(t.fields,name);
 need(t.layout==bnw&&t.version&&*t.version>=1&&*t.version<=19&&f.encoding==saves::PlayerTraitsEncoding::i32&&f.values.size()==1&&f.columns==1&&!f.wire_count&&f.rows.empty()&&f.hashes.empty(),"UnitPath actual trait scalar cache unavailable");return narrow(f.values.front());
}
std::int32_t existing_path_combat_trait(const LiveSessionState&s,const ui::SelectedBnwPreviewHost&host,std::int32_t actor,std::int32_t combat){
 if(combat==-1)return 0;
 const auto&t=s.world.all_players->players.at(actor).traits;const auto&f=post_named(t.fields,"m_paiMovesChangeUnitCombat");const auto span=native_init_info_span(host,"UnitCombatInfos");
 need(t.layout==bnw&&t.version&&*t.version>=1&&*t.version<=19&&f.encoding==saves::PlayerTraitsEncoding::combat_i32&&f.wire_count&&*f.wire_count>=0&&static_cast<std::uint64_t>(*f.wire_count)==span&&f.values.size()==span&&f.columns==1&&f.rows.empty()&&f.hashes.empty()&&combat>=0&&static_cast<std::uint64_t>(combat)<span,"UnitPath actual full indexed combat trait cache unavailable");return narrow(f.values.at(combat));
}
}
BnwNativeUnitAllocatorCore::ExistingUnitPathCache BnwNativeUnitAllocatorCore::prepare_existing_unit_path_cache(const saves::SavedUnitObservation&u)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign UnitPath scratch before source read");
 require_native_common_pathfinder_binding(p->host->session());
 need(narrow(core(u,"m_eOwner"))==p->actor&&native_imported_get_at(*p->state,p->actor,narrow(core(u,"m_iID")))==&u,"UnitPath cache requires actual SAME actor/GetAt node");
 const auto hash=core(u,"idUnitType");need(hash>0&&hash<=UINT32_MAX,"UnitPath actual unit type hash unavailable");
 const auto*type=p->host->types().find_hash(hash);need(type&&type->table=="Units"&&type->id>=0&&type->hash==content::saved_info_type_hash(type->type),"UnitPath actual native UnitInfo hash/ID unavailable");
 const auto&owned=p->state->maintenance.players.at(p->actor).units.at(narrow(core(u,"m_iID")));
 need(owned.type==type->id&&owned.source_core==std::optional{std::pair{u.core.start_offset,u.core.end_offset}},"UnitPath raw/owned native UnitInfo/span differs");
 struct Info {std::int32_t moves=0,domain=-1,combat=-1,range=0,ranged_combat=0;bool operator==(const Info&)const=default;};
 const auto read_info=[&]{Info result;bool found=false;const auto rows=p->host->database().query_rows({"SELECT Type,Moves,Domain,CombatClass,Range,RangedCombat FROM Units WHERE ID=?",{type->id}},[&](const auto&r){
  need(!found&&r.text("Type")==std::optional{type->type},"UnitPath actual selected UnitInfo Type differs");found=true;result.moves=narrow(r.integer("Moves"));result.range=narrow(r.integer("Range"));result.ranged_combat=narrow(r.integer("RangedCombat"));
  const auto domain=r.text("Domain");need(domain&& !domain->empty(),"UnitPath native domain absent");const auto*d=p->host->types().find(*domain);
  constexpr std::array<std::string_view,5>names{"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
  need(d&&d->table=="Domains"&&d->id>=0&&d->id<5&&d->type==names.at(d->id),"UnitPath Domain source enum differs");result.domain=narrow(d->id);
  const auto combat=r.text("CombatClass");if(combat&&!combat->empty()){const auto*q=p->host->types().find(*combat);need(q&&q->table=="UnitCombatInfos"&&q->id>=0,"UnitPath CombatClass native identity unavailable");result.combat=narrow(q->id);}
 });need(rows.all_succeeded()&&found,"UnitPath authentic UnitInfo query unavailable");return result;};
 const auto info=read_info();p->check();
 const auto&team=post_team(*p->state,post_player_team(*p->state,p->actor));
 const auto&extra=post_named(team.arrays,"extra_moves");need(extra.columns==1&&extra.values.size()==5,"UnitPath actual Team extraMoves array incomplete");
 const auto define=[&](const char*name){const auto d=p->host->database().global_define(name,content::GlobalDefineKind::integer);p->check();need(d.found&&d.error.empty()&&std::holds_alternative<std::int64_t>(d.value),"UnitPath actual movement Define unavailable");return narrow(std::get<std::int64_t>(d.value));};
 ExistingUnitPathCache result;
 const auto base=[&](std::int32_t into_domain){
  const auto can_embark=info.domain==2&&narrow(core(u,"m_iEmbarkAbilityCount"))>0;
  if((into_domain==0&&can_embark)||(into_domain==-1&&existing_path_bool(u,"m_bEmbarked"))){
   auto value=define("EMBARKED_UNIT_MOVEMENT");value=narrow(std::int64_t(value)+narrow(core(u,"m_iExtraNavalMoves")));
   value=narrow(std::int64_t(value)+narrow(post_named(team.fields,"embarked_extra_moves").value));
   value=narrow(std::int64_t(value)+narrow(extra.values.at(info.domain)));
   value=narrow(std::int64_t(value)+existing_path_trait(*p->state,p->actor,"m_iExtraEmbarkMoves"));
   const auto rules=BnwPolicyHappinessRules::load(p->host->database());p->check();need(rules.rules_sha256()==p->fingerprint,"UnitPath actual policy rules identity changed");
   std::vector<bool>has,blocked;const auto before=initial_tourism_policy_seal(*p->state,p->actor,rules,&has,&blocked);
   std::int32_t modifier=0;
   // Native GetNumericModifier loops ascending PolicyInfo slots and sums ALL
   // signed EmbarkedExtraMoves operands for HasPolicy && !IsPolicyBlocked.
   for(std::size_t i=0;i<has.size();++i){if(!has[i])continue;const auto&policy=rules.policies().at(static_cast<std::int64_t>(i));if(policy.blocked_branch>=0&&blocked.at(policy.blocked_branch))continue;
    const auto&e=initial_maintenance_info(*p->host,"Policies",narrow(i));bool seen=false;std::int32_t operand=0;
    const auto rows=p->host->database().query_rows({"SELECT Type,EmbarkedExtraMoves FROM Policies WHERE ID=?",{e.id}},[&](const auto&r){need(!seen&&r.text("Type")==std::optional{e.type},"UnitPath active PolicyInfo native Type differs");seen=true;operand=narrow(r.integer("EmbarkedExtraMoves"));});p->check();need(rows.all_succeeded()&&seen,"UnitPath active policy numeric getter unavailable");modifier=narrow(std::int64_t(modifier)+operand);
   }
   need(initial_tourism_policy_seal(*p->state,p->actor,rules)==before,"UnitPath actual policy source changed during modifier");return narrow(std::int64_t(value)+modifier);
  }
  auto value=narrow(std::int64_t(info.moves)+narrow(core(u,"m_iExtraMoves")));value=narrow(std::int64_t(value)+narrow(extra.values.at(info.domain)));
  if(info.domain==0){value=narrow(std::int64_t(value)+narrow(core(u,"m_iExtraNavalMoves")));if(narrow(core(u,"m_iBaseCombat"))==0)value=narrow(std::int64_t(value)+existing_path_trait(*p->state,p->actor,"m_iExtraEmbarkMoves"));}
  if(post_player(*p->state,p->actor,"m_iGoldenAgeTurns")>0)value=narrow(std::int64_t(value)+existing_path_trait(*p->state,p->actor,"m_iGoldenAgeMoveChange"));
  return narrow(std::int64_t(value)+existing_path_combat_trait(*p->state,*p->host,p->actor,info.combat));
 };
 // Original UnitPathInitialize unconditionally executes all five baseMoves
 // in Domain enum order BEFORE maxMoves and cached booleans.
 for(std::int32_t i=0;i<5;++i){result.base_moves.at(i)=base(i);p->check();}
 need(narrow(core(u,"m_iGreatGeneralCount"))<=0,"UnitPath maxMoves requires genuine GreatGeneral stack movement owner");
 result.denominator=define("MOVE_DENOMINATOR");result.max_moves=narrow(std::int64_t(base(-1))*result.denominator);
 result.owner=narrow(core(u,"m_eOwner"));result.team=post_player_team(*p->state,result.owner);result.domain=info.domain;
 result.human=detail::bnw_player_human_read(*p->state,result.owner,p->host->database());p->check();
 result.automated=narrow(core(u,"m_eAutomateType"))!=-1;
 result.immobile=result.domain==3||existing_path_bool(u,"m_bImmobile");
 result.no_reveal=narrow(core(u,"m_iNoRevealMapCount"))>0;
 result.can_embark=result.domain==2&&narrow(core(u,"m_iEmbarkAbilityCount"))>0;
 result.embarked=existing_path_bool(u,"m_bEmbarked");
 result.combat=narrow(core(u,"m_iBaseCombat"))>0;
 const auto melee=result.combat&&narrow(core(u,"m_iOnlyDefensiveCount"))<=0;
 result.can_attack=melee||(narrow(std::int64_t(info.range)+narrow(core(u,"m_iExtraRange")))>0&&info.ranged_combat>0);
 need(read_info()==info,"UnitPath immutable UnitInfo changed during scratch initialization");p->check();require_native_common_pathfinder_session_without_callbacks(p->host->session());return result;
}

struct BnwNativeUnitAllocatorCore::ExistingPathMovementCostDispatch::Storage {
 const std::thread::id creator=std::this_thread::get_id();
 const BnwNativeUnitAllocatorCore*issuer=nullptr;std::weak_ptr<const BnwNativeUnitAllocatorCore>lineage;
 std::shared_ptr<const BnwTwoLayerUnitPathFinder>tree;
 const saves::SavedUnitObservation*unit=nullptr;
 const BnwTwoLayerUnitPathFinder::Node*parent=nullptr,*node=nullptr;
 ExistingUnitPathCache cache;
 // Distinct private issuer constructors: live callback-node authority vs
 // exact registered Unit retained-cache first-step authority. No public DTO
 // can choose or populate either source domain.
 bool retained_step=false;const BnwUnitPathOwner*paths=nullptr;
 const BnwUnitRetainedPath*path=nullptr;BnwUnitRetainedPath path_snapshot;
 std::shared_ptr<const ImportedMovesArchive>moves;
 std::uint64_t actual_from=0,actual_to=0;
 std::int32_t parent_moves=0,parent_turns=0,node_moves=0,node_turns=0;
 std::string world_revision;
};
BnwNativeUnitAllocatorCore::ExistingPathMovementCostDispatch::ExistingPathMovementCostDispatch(
 const BnwNativeUnitAllocatorCore&issuer,std::shared_ptr<const BnwTwoLayerUnitPathFinder>tree,
 const saves::SavedUnitObservation&unit,const BnwTwoLayerUnitPathFinder::Node&parent,
 const BnwTwoLayerUnitPathFinder::Node&node,const ExistingUnitPathCache&cache){
 issuer.require_native_common_pathfinder_binding(issuer.impl_->host->session());
 need(tree&&tree->creator_==std::this_thread::get_id()&&tree->busy_&&tree->callbacks_,"Native path cost must be issued inside actual common search callback");
 issuer.impl_->host->session().require_bnw_common_unit_pathfinder_native_current_without_callbacks(*tree,issuer);
 const auto belongs=[&](const auto*entry){for(const auto&n:tree->primary_)if(&n==entry)return true;for(const auto&n:tree->partial_)if(&n==entry)return true;return false;};
 need(belongs(&parent)&&belongs(&node),"Native path cost source nodes are not in SAME common two-layer allocation");
 need(native_imported_get_at(*issuer.impl_->state,issuer.impl_->actor,narrow(core(unit,"m_iID")))==&unit&&issuer.prepare_existing_unit_path_cache(unit)==cache,"Native path cost actual UnitPathInitialize operands changed");
 auto s=std::make_shared<Storage>();s->issuer=&issuer;s->lineage=issuer.factory_self_;s->tree=std::move(tree);s->unit=&unit;s->parent=&parent;s->node=&node;s->cache=cache;
 s->parent_moves=parent.moves;s->parent_turns=parent.turns;s->node_moves=node.moves;s->node_turns=node.turns;
 s->world_revision=bnw_unit_order_world_revision(*issuer.impl_->state,issuer.impl_->host->identity(),issuer.impl_->host->database());storage_=std::move(s);
 (void)issuer.existing_path_movement_cost_operands(*this);
}
BnwNativeUnitAllocatorCore::ExistingPathMovementCostDispatch::ExistingPathMovementCostDispatch(const BnwNativeUnitAllocatorCore&issuer,std::shared_ptr<const BnwTwoLayerUnitPathFinder>tree,const saves::SavedUnitObservation&unit,const BnwUnitPathOwner&paths,const BnwUnitRetainedPath&path,const ExistingUnitPathCache&cache){
 const auto p=issuer.impl_;p->check();need(tree&&!tree->busy_&&!tree->callbacks_,"Retained step cost requires completed SAME common tree");
 p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,issuer);
 const auto id=narrow(core(unit,"m_iID"));const BnwUnitOrderKey key{p->actor,id};
 need(native_imported_get_at(*p->state,p->actor,id)==&unit&&&paths.paths_.at(key)==&path&&path.nodes.size()>=2&&issuer.prepare_existing_unit_path_cache(unit)==cache,"Retained step cost lacks actual GetAt/cache node authority");
 const auto field=p->bank->imported_moves_fields.find(id);need(field!=p->bank->imported_moves_fields.end()&&field->second,"Retained step cost imported scalar registration absent");
 auto s=std::make_shared<Storage>();s->issuer=&issuer;s->lineage=issuer.factory_self_;s->tree=std::move(tree);s->unit=&unit;s->cache=cache;
 s->retained_step=true;s->paths=&paths;s->path=&path;s->path_snapshot=path;s->moves=field->second;
 const auto&h=p->state->world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};const auto origin=geometry.plot(narrow(core(unit,"m_iX")),narrow(core(unit,"m_iY")));
 const auto&node=path.nodes[path.nodes.size()-2];const auto step=geometry.plot(node.x,node.y);
 need(origin&&step&&*origin!=*step,"Retained step cost actual current/first plot invalid");s->actual_from=*origin;s->actual_to=*step;
 const auto&tail=path.nodes.back();need(geometry.plot(tail.x,tail.y)==origin,"Retained step cost cached source no longer actual current plot");
 s->world_revision=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());storage_=std::move(s);
 (void)issuer.existing_path_movement_cost_operands(*this);
}
BnwNativeUnitAllocatorCore::ExistingPathMovementCostOperands BnwNativeUnitAllocatorCore::existing_path_movement_cost_operands(const ExistingPathMovementCostDispatch&d)const{
 const auto s=d.storage_;need(s&&s->creator==std::this_thread::get_id(),"Foreign/missing actual node cost dispatch before source");
 const auto p=impl_;require_native_common_pathfinder_binding(p->host->session());
 need(s->issuer==this&&!s->lineage.expired()&&!s->lineage.owner_before(factory_self_)&&!factory_self_.owner_before(s->lineage)&&s->tree,"Actual node cost Core lineage differs");
 p->host->session().require_bnw_common_unit_pathfinder_native_current_without_callbacks(*s->tree,*this);
 if(s->retained_step){
  need(!s->tree->busy_&&!s->tree->callbacks_&&s->paths&&s->path&&s->moves&&s->unit,"Retained step source domain retired/reentered");
  p->host->session().require_bnw_native_unit_paths_current_without_callbacks(*s->paths,*s->tree,*this);
  const auto id=narrow(core(*s->unit,"m_iID"));const BnwUnitOrderKey key{p->actor,id};
  need(native_imported_get_at(*p->state,p->actor,id)==s->unit&&&s->paths->paths_.at(key)==s->path&&*s->path==s->path_snapshot&&
   bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==s->world_revision,"Retained step actual GetAt/path/world lineage changed");
  const auto registered=p->bank->imported_moves_fields.find(id);const auto&field=*s->moves;
  need(registered!=p->bank->imported_moves_fields.end()&&registered->second.get()==s->moves.get()&&field.creator==std::this_thread::get_id()&&field.owner.lock().get()==p->bank.get()&&field.unit==s->unit&&field.actor==p->actor&&field.id==id&&field.generation==p->generation,"Retained step actual imported scalar control changed");
  const saves::UnitCoreField*actual=nullptr;for(const auto&v:s->unit->core.fields)if(name(v.name)=="m_iMoves"){need(!actual,"Retained step duplicate moves field");actual=&v;}
  need(actual==field.field&&actual&&actual->encoding==saves::UnitCoreEncoding::i32&&actual->values.size()==1&&&actual->values.front()==field.value&&narrow(actual->values.front())==field.expected,"Retained step actual scalar alias/earned moves lineage changed");
  // Re-read real UnitPathInitialize inputs; mutation of team/policy/trait
  // fields not covered by the unit-order hash cannot hide in a cached DTO.
  need(prepare_existing_unit_path_cache(*s->unit)==s->cache,"Retained step actual max/base/team/policy/cache operands changed");
  const auto&h=p->state->world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};
  const auto origin=geometry.plot(narrow(core(*s->unit,"m_iX")),narrow(core(*s->unit,"m_iY")));const auto&node=s->path->nodes[s->path->nodes.size()-2];const auto next=geometry.plot(node.x,node.y);
  need(origin&&next&&*origin==s->actual_from&&*next==s->actual_to,"Retained step source/current/next coordinates changed");bool adjacent=false;
  const auto xy=h.grid.coordinates(*origin);for(std::int32_t i=0;i<6;++i){const auto q=geometry.direction(xy[0],xy[1],i);if(q&&static_cast<std::uint64_t>(*q)==*next)adjacent=true;}need(adjacent,"Retained first step is not actual six-direction edge");
  const auto ground=[&](std::uint64_t i){const auto&raw=p->state->world.map_plots->plots.at(i);const auto&owned=p->state->world.map_projection->cells.at(i);const auto type=narrow(post_named(raw.fields,"plot_type").value);need(raw.index==i&&owned.index==i&&owned.plot_type==std::optional<std::int64_t>{type}&&type>=0&&type<4,"Retained step raw/owned domain source differs");return type!=3;};
  need(s->cache.domain==2&&!s->cache.embarked&&ground(*origin)&&ground(*next),"Retained step water/embark/domain transition owner unavailable");
  ExistingPathMovementCostOperands a;a.host=p->host;a.state=p->state;a.source=p->source;a.unit=s->unit;a.generation=p->generation;a.key=key;a.from=*origin;a.to=*next;
  a.team=s->cache.team;a.base=s->cache.base_moves.at(2);a.max_moves=s->cache.max_moves;a.denominator=s->cache.denominator;
  // CvUnit::move calls targetPlot.movementCost(this,plot()) with the native
  // DEFAULT third operand0. AllMoves/ZOC therefore burn maxMoves, whereas
  // the path callback constructor preserves actual segment remaining moves.
  // Current positive m_iMoves proves canMove; it does not override that
  // authored default or clamp later changeMoves to a path preview's budget.
  a.remaining=0;a.human=s->cache.human;
  need(field.expected>0,"Retained step actual imported unit cannot move this turn");require_native_common_pathfinder_session_without_callbacks(p->host->session());return a;
 }
 need(s->tree->busy_&&s->tree->callbacks_&&s->parent&&s->node&&s->unit&&native_imported_get_at(*p->state,p->actor,narrow(core(*s->unit,"m_iID")))==s->unit,
  "Native node cost source search/GetAt scope retired");
 need(s->parent->moves==s->parent_moves&&s->parent->turns==s->parent_turns&&s->node->moves==s->node_moves&&s->node->turns==s->node_turns&&
  bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==s->world_revision,
  "Native node cost physical parent/scratch/world inputs changed");
 const auto&grid=p->state->world.map_header.grid;
 need(s->tree->valid_coordinates(s->parent->x,s->parent->y)&&s->tree->valid_coordinates(s->node->x,s->node->y),"Native node cost physical coordinates unavailable");
 ExistingPathMovementCostOperands a;a.host=p->host;a.state=p->state;a.source=p->source;a.unit=s->unit;a.generation=p->generation;
 a.key={p->actor,narrow(core(*s->unit,"m_iID"))};a.from=grid.index(s->parent->x,s->parent->y);a.to=grid.index(s->node->x,s->node->y);
 need(a.from!=a.to,"Native MovementCost requires distinct actual adjacent nodes");
 const auto&h=p->state->world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};bool neighbor=false;
 for(std::int32_t i=0;i<6;++i){const auto n=geometry.direction(s->parent->x,s->parent->y,i);if(n&&static_cast<std::uint64_t>(*n)==a.to)neighbor=true;}
 need(neighbor,"Native node cost source is not a genuine six-direction edge");
 const auto kind=[&](std::uint64_t id){const auto&raw=p->state->world.map_plots->plots.at(id);const auto&owned=p->state->world.map_projection->cells.at(id);const auto type=narrow(post_named(raw.fields,"plot_type").value);need(raw.index==id&&owned.index==id&&owned.plot_type==std::optional<std::int64_t>{type}&&type>=0&&type<4,"Native node cost raw/owned PlotType differs");return type;};
 need(s->cache.domain==2&&!s->cache.embarked&&kind(a.from)!=3&&kind(a.to)!=3,"Native path MovementCost water/domain transition requires its complete actual owner");
 a.team=s->cache.team;a.base=s->cache.base_moves.at(2);a.max_moves=s->cache.max_moves;a.denominator=s->cache.denominator;
 a.remaining=s->parent->moves>0?s->parent->moves:narrow(std::int64_t(s->cache.base_moves.at(2))*s->cache.denominator);a.human=s->cache.human;
 require_native_common_pathfinder_session_without_callbacks(p->host->session());return a;
}
namespace {
const saves::PlotObservation&native_path_plot(const LiveSessionState&s,std::uint64_t index){
 need(s.world.map_plots&&s.world.map_projection&&s.world.map_plots->layout==bnw&&index<s.world.map_header.grid.size(),"Native path current plot allocation unavailable");const auto&raw=s.world.map_plots->plots.at(index);const auto&cell=s.world.map_projection->cells.at(index);const auto xy=s.world.map_header.grid.coordinates(index);
 need(raw.index==index&&cell.index==index&&raw.x==xy[0]&&raw.y==xy[1]&&cell.x==xy[0]&&cell.y==xy[1],"Native path current physical plot identity differs");return raw;
}
const std::vector<saves::SavedPlotUnitReference>&native_path_stack(const LiveSessionState&s,std::uint64_t index){
 const auto&raw=native_path_plot(s,index);const auto&cell=s.world.map_projection->cells.at(index);const auto&links=post_named(raw.arrays,"units");need(cell.units&&links.columns==2&&links.values.size()==cell.units->size()*2,"Native path current ordered plot units unavailable");for(std::size_t i=0;i<cell.units->size();++i)need(links.values[i*2]==cell.units->at(i).owner&&links.values[i*2+1]==cell.units->at(i).id,"Native path current ordered plot unit mirrors differ");return *cell.units;
}
bool native_path_city(const LiveSessionState&s,std::uint64_t index,std::int32_t*city_owner=nullptr){
 const auto&raw=native_path_plot(s,index);const auto&cell=s.world.map_projection->cells.at(index);const auto owner=narrow(post_named(raw.fields,"plot_city_owner").value),id=narrow(post_named(raw.fields,"plot_city_id").value);
 need(cell.plot_city&&cell.plot_city->owner==owner&&cell.plot_city->id==id,"Native path plot city raw/owned handle differs");if(owner==-1)return false;need(owner>=0&&owner<64,"Native path city owner outside roster");const auto*city=resolve_bnw_city_registry(s.world,s.cities,owner,id);if(!city)return false;if(city_owner)*city_owner=owner;return true;
}
bool native_path_invisible(const LiveSessionState&s,const saves::SavedUnitObservation&u,std::int32_t observer){
 const auto owner=narrow(core(u,"m_eOwner"));if(post_player_team(s,owner)==observer)return false;
 const auto transport_owner=native_friendly_tail(u,"m_transportUnit.eOwner"),transport_id=native_friendly_tail(u,"m_transportUnit.iID");if(transport_owner>=0&&transport_owner<64&&native_imported_get_at(s,transport_owner,transport_id))return true;
 const auto invisible=narrow(core(u,"m_eInvisibleType"));if(invisible==-1)return false;
 const auto x=narrow(core(u,"m_iX")),y=narrow(core(u,"m_iY"));const auto&grid=s.world.map_header.grid;need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<grid.width&&static_cast<std::uint64_t>(y)<grid.height,"Native invisible unit actual plot NULL");const auto&raw=native_path_plot(s,grid.index(x,y));const auto&dimensions=s.world.map_plots->dimensions;const auto&counts=post_named(raw.arrays,"invisible_visibility");
 need(observer>=0&&observer<64&&dimensions.teams==80&&dimensions.invisible_types>0&&invisible>=0&&static_cast<std::uint64_t>(invisible)<dimensions.invisible_types&&counts.columns==1&&counts.values.size()==dimensions.teams*dimensions.invisible_types,"Native path invisible detection source span unavailable");const auto count=counts.values.at(static_cast<std::size_t>(observer)*dimensions.invisible_types+invisible);need(count>=0&&count<=INT16_MAX,"Native path invisible detection counter outside native short");return count<=0;
}
std::int32_t native_path_combat_team(const LiveSessionState&s,const saves::SavedUnitObservation&u,std::int32_t target,std::uint64_t index){
 auto team=post_player_team(s,narrow(core(u,"m_eOwner")));if(team!=target&&target!=63&&!native_path_city(s,index)&&narrow(core(u,"m_iAlwaysHostileCount"))>0)team=post_player_team(s,63);return team;
}
bool native_path_visible_enemy(const LiveSessionState&s,const saves::SavedUnitObservation&query,std::uint64_t index){
 const auto&stack=native_path_stack(s,index);if(stack.empty())return false;const auto observer=post_player_team(s,narrow(core(query,"m_eOwner")));const bool hostile=!native_path_city(s,index)&&narrow(core(query,"m_iAlwaysHostileCount"))>0;
 for(const auto&handle:stack){if(handle.owner<0||handle.owner>=64)continue;const auto*u=native_imported_get_at(s,narrow(handle.owner),narrow(handle.id));if(!u||native_path_invisible(s,*u,observer))continue;const auto combat_team=native_path_combat_team(s,*u,observer,index);if(hostile?observer!=combat_team:post_relation(s,observer,combat_team,"at_war"))return true;}return false;
}
std::vector<NativeInitialMission>native_imported_mission_rows(const saves::SavedUnitObservation&u){
 need(u.core.layout==bnw&&u.tail.layout==bnw,"Imported mission native profile unavailable");const saves::UnitTailNode*queue=nullptr;
 for(const auto&b:u.tail.blocks)if(name(b.name)=="m_missionQueue"){need(!queue,"Duplicate imported mission queue");queue=&b;}
 need(queue&&name(queue->encoding)=="counted_missions"&&queue->wire_count&&*queue->wire_count<=UINT32_MAX&&queue->bytes.empty()&&queue->children.empty()&&queue->values.size()%5==0&&queue->values.size()/5==*queue->wire_count,"Imported mission native CLinkList Read row shape unavailable");
 std::vector<NativeInitialMission>rows;rows.reserve(queue->values.size()/5);
 // CvStructs.cpp MissionData Read: type, data1, data2, flags, pushTurn.
 // Preserve list order and every signed32 operand; unknown mission type is
 // not silently mapped, removed or granted an executable body.
 for(std::size_t i=0;i<queue->values.size();i+=5)rows.push_back({narrow(queue->values[i]),narrow(queue->values[i+1]),narrow(queue->values[i+2]),narrow(queue->values[i+3]),narrow(queue->values[i+4])});
 return rows;
}
bool native_path_enemy_city(const LiveSessionState&s,const saves::SavedUnitObservation&query,std::uint64_t index){std::int32_t owner=-1;if(!native_path_city(s,index,&owner))return false;const auto city_team=post_player_team(s,owner);return post_relation(s,native_path_combat_team(s,query,city_team,index),city_team,"at_war");}
}
bool BnwNativeUnitAllocatorCore::existing_path_slowed_by_zoc(const ExistingPathMovementCostDispatch&d)const{
 const auto a=existing_path_movement_cost_operands(d);const auto p=impl_;const auto&s=*a.state;const auto&u=*a.unit;
 if(native_friendly_tail(u,"m_iIgnoreZOC")>0)return false;
 // This private dispatch proves ordinary ground domain; CostsOnlyOne's
 // remaining reached operand is the actual unit flat-movement count.
 if(narrow(core(u,"m_iFlatMovementCostCount"))>0)return false;
 const auto*enabled=p->host->defines().find("ZONE_OF_CONTROL_ENABLED");need(enabled&&enabled->lookup.found&&enabled->lookup.error.empty()&&!enabled->retained_previous&&std::holds_alternative<std::int64_t>(enabled->lookup.value),"Native path ZOC actual cached Define unavailable");if(narrow(std::get<std::int64_t>(enabled->lookup.value))<=0)return false;
 const auto&map=s.world.map_header;const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};const auto from=map.grid.coordinates(a.from);
 const auto adjacent_to_target=[&](std::uint64_t index){const auto xy=map.grid.coordinates(index);for(std::int32_t i=0;i<6;++i){const auto q=geometry.direction(xy[0],xy[1],i);if(q&&static_cast<std::uint64_t>(*q)==a.to)return true;}return false;};
 const bool visible_enemy=native_path_visible_enemy(s,u,a.to);
 for(std::int32_t i=0;i<6;++i){const auto plot=geometry.direction(from[0],from[1],i);if(!plot)continue;const auto index=static_cast<std::uint64_t>(*plot);
  if(native_path_enemy_city(s,u,index)&&adjacent_to_target(index))return true;
  for(const auto&handle:native_path_stack(s,index)){if(handle.owner<0||handle.owner>=64)continue;const auto*loop=native_imported_get_at(s,narrow(handle.owner),narrow(handle.id));if(!loop)continue;const auto team=post_player_team(s,narrow(core(*loop,"m_eOwner")));
   if(native_path_invisible(s,*loop,a.team))continue;if(narrow(core(*loop,"m_iBaseCombat"))<=0)continue;
   if(team==63||post_relation(s,a.team,team,"at_war")){
    const auto domain=native_friendly_info(*p->host,s,*loop).domain;p->check();
    // Preserve the original (DOMAIN_SEA && unit_domain_type) condition.
    if(domain!=2&&domain!=0)continue;if(existing_path_bool(*loop,"m_bEmbarked"))continue;
    if(!visible_enemy&&adjacent_to_target(index)){(void)existing_path_movement_cost_operands(d);return true;}
   }
  }
 }
 (void)existing_path_movement_cost_operands(d);return false;
}

std::optional<BnwUnitRetainedPath>BnwNativeUnitAllocatorCore::read_native_existing_retained_path_cache(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Native retained path read requires completed imported rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);
 const auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const BnwUnitOrderKey key{p->actor,narrow(core(*unit,"m_iID"))};
 const auto result=paths.paths_.at(key);p->check();(void)issuer->current();
 p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
 need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&paths.paths_.at(key)==result,"Native retained path changed across actual current bookends");return result;
}

std::optional<BnwNativeUnitPathToObservation>BnwNativeUnitAllocatorCore::advance_native_existing_unit_path_to_prefix(std::int32_t query_id,std::int32_t x,std::int32_t y,std::int32_t flags,std::int32_t previous_eta)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 const saves::SavedUnitObservation*unit=nullptr;std::int32_t id=-1;std::optional<std::uint64_t>target;std::string before;
 {
  Impl::Guard invocation(p);p->check();
  if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"UnitPathTo requires completed imported rosters");}
  unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
  id=narrow(core(*unit,"m_iID"));need(p->bank->continue_mission_entries.empty(),"UnitPathTo cannot bypass required saved Flags/archive phase of ContinueMission");need(p->bank->existing_path_to.empty(),"UnitPathTo prior body has mandatory unconsumed admission/UI/visualization boundary");
  // Source actual at(x,y) returns0 BEFORE destination lookup, domain, path,
  // queue or flags read. It does not manufacture a successful moved unit.
  if(narrow(core(*unit,"m_iX"))==x&&narrow(core(*unit,"m_iY"))==y){p->check();(void)issuer->current();BnwNativeUnitPathToObservation result;result.native_return=0;return result;}
  const auto&h=p->state->world.map_header;target=MapGeometry{h.grid,h.wrap_x,h.wrap_y,bnw}.plot(x,y);
  if(!target){p->check();(void)issuer->current();BnwNativeUnitPathToObservation result;result.native_return=0;return result;}
  const auto info=native_friendly_info(*p->host,*p->state,*unit);p->check();need(info.domain!=1,"UnitPathTo AIR admission/ClearPath owner mandatory");
  need(flags==0,"UnitPathTo actual nonzero movement flags require complete native finder profile");
  before=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());p->check();(void)issuer->current();
 }
 // Closed UpdatePathCache really stores attempted destination/CopyPath when
 // original branch calls GeneratePath. An unsuccessful native result returns
 //0 without UnitMove or visual publication, exactly as the SDK body.
 const auto updated=advance_native_existing_update_path_cache(query_id,*target);
 {
  Impl::Guard invocation(p);p->check();(void)issuer->current();need(updated.has_value()&&native_imported_get_at(*p->state,p->actor,query_id)==unit&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==before,"UnitPathTo actual source changed across UpdatePathCache");
  if(!*updated){BnwNativeUnitPathToObservation result;result.native_return=0;return result;}
  auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const BnwUnitOrderKey key{p->actor,id};auto&path=paths.paths_.at(key);const auto before_path=path;
  auto staged=std::make_shared<ExistingUnitPathToStage>();staged->owner=p->bank;staged->unit=unit;staged->actor=p->actor;staged->id=id;staged->x=x;staged->y=y;staged->flags=flags;staged->previous_eta=previous_eta;staged->generation=p->generation;staged->world_revision=before;staged->paths=&paths;staged->path=&path;staged->current_path=path;
  const auto&h=p->state->world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};
  if(!path.nodes.empty()){const auto&first=path.nodes[path.nodes.size()==1?0:path.nodes.size()-2];staged->first_plot=geometry.plot(first.x,first.y);}
  bool reject=false;
  if(!path.nodes.empty()){
   const auto&node=path.nodes.front();if(previous_eta>=0){need(previous_eta<=INT32_MAX-2,"UnitPathTo previous ETA native signed addition overflow");reject=node.data2>previous_eta+2;}
   // This source condition calls canMoveInto even after ETA made reject=true.
   // Its absent concrete admission must remain the next boundary; no skip
   // to the later clear/visual branch from a guessed predicate result.
   staged->boundary=node.data2==1?BnwNativeUnitPathToBoundary::destination_admission:(reject?BnwNativeUnitPathToBoundary::rejected_path_visualization:BnwNativeUnitPathToBoundary::unit_move);
  }else staged->boundary=BnwNativeUnitPathToBoundary::unit_move;
  const bool clear=staged->boundary==BnwNativeUnitPathToBoundary::rejected_path_visualization;
  if(clear){need(path.path_to_reject_clear_operations<UINT64_MAX,"UnitPathTo actual reject clear receipt exhausted");staged->current_path.nodes.clear();++staged->current_path.path_to_reject_clear_operations;}
  // Allocate the once body journal before any reject-cache store. The private
  // record is the only possible future continuation authority, not the copy.
  p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
  need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==before&&path==before_path&&p->bank->existing_path_to.empty(),"UnitPathTo actual receiving body source changed before publication");
  need(p->bank->existing_path_to.emplace(id,staged).second,"UnitPathTo body journal publication replay");
  if(clear){
   // SDK m_kLastPath.clear ONLY; m_uiLastPathCacheDest remains untouched.
   path.nodes.clear();++path.path_to_reject_clear_operations;
   // PublishQueuedVisualizationMoves is mandatory EVEN for empty original
   // move-locations. No queue-empty or readiness flag substitutes its call.
  }
  p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
  need(path==staged->current_path&&p->bank->existing_path_to.at(id).get()==staged.get()&&native_imported_get_at(*p->state,p->actor,query_id)==unit,"UnitPathTo actual stored prefix/journal changed");
  BnwNativeUnitPathToObservation result;result.boundary=staged->boundary;result.first_plot=staged->first_plot;return result;
 }
}

std::optional<BnwNativeUnitPathToObservation>BnwNativeUnitAllocatorCore::advance_native_existing_unit_move_admission(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 // Original Read registration is real metadata, never a caller-provided
 // current value/store permission. No native scalar is changed by binding.
 const auto moves_control=bind_native_existing_moves_archive(query_id);if(!moves_control)return std::nullopt;
 Impl::Guard invocation(p);p->check();(void)issuer->current();const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);
 need(unit,"UnitMove admission lost actual imported GetAt");const auto id=narrow(core(*unit,"m_iID"));
 const auto found=p->bank->existing_path_to.find(id);need(found!=p->bank->existing_path_to.end()&&found->second,"UnitMove requires actual privately issued UnitPathTo body");const auto stage=found->second;
 need(stage->boundary==BnwNativeUnitPathToBoundary::destination_admission||stage->boundary==BnwNativeUnitPathToBoundary::unit_move,"UnitMove prior required body boundary is not executable admission");
 need(!stage->unit_move_checked&&!stage->moves&&stage->owner.lock().get()==p->bank.get()&&stage->unit==unit&&stage->actor==p->actor&&stage->id==id&&stage->generation==p->generation,"UnitMove receiving actual body/node lifetime changed or replayed");
 const auto registered=p->bank->imported_moves_fields.find(id);need(registered!=p->bank->imported_moves_fields.end()&&registered->second,"UnitMove actual imported moves archive registration absent");const auto field=registered->second;
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const BnwUnitOrderKey key{p->actor,id};auto&path=paths.paths_.at(key);
 const auto&map=p->state->world.map_header;const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};const auto destination=geometry.plot(stage->x,stage->y);
 need(destination&&stage->flags==0&&stage->paths==&paths&&stage->path==&path,"UnitMove actual canonical path/destination differs");
 const auto current=[&]{p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
  need(!tree->busy_&&!tree->callbacks_&&p->bank->existing_path_to.at(id).get()==stage.get()&&native_imported_get_at(*p->state,p->actor,query_id)==unit&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==stage->world_revision&&path==stage->current_path,"UnitMove actual body/common tree/GetAt/world/path changed during native reads");
  need(p->bank->imported_moves_fields.at(id).get()==field.get()&&field->creator==std::this_thread::get_id()&&field->owner.lock().get()==p->bank.get()&&field->unit==unit&&field->actor==p->actor&&field->id==id&&field->generation==p->generation&&field->field&&field->field->encoding==saves::UnitCoreEncoding::i32&&field->field->values.size()==1&&&field->field->values.front()==field->value&&narrow(*field->value)==field->expected&&narrow(core(*unit,"m_iMoves"))==field->expected,"UnitMove actual registered m_iMoves physical alias/current value differs");
 };
 const auto observed=[&]{BnwNativeUnitPathToObservation result;result.boundary=stage->boundary;result.first_plot=stage->first_plot;result.move_cost=stage->move_cost;result.moves_before=stage->moves_before;result.moves_requested=stage->moves_requested;return result;};
 current();const auto cache=prepare_existing_unit_path_cache(*unit);current();
 bool reject=false;if(!path.nodes.empty()&&stage->previous_eta>=0){need(stage->previous_eta<=INT32_MAX-2,"UnitMove prior ETA native addition overflow");reject=path.nodes.front().data2>stage->previous_eta+2;}
 if(stage->boundary==BnwNativeUnitPathToBoundary::destination_admission){
  need(!stage->destination_checked&&!path.nodes.empty()&&path.nodes.front().data2==1,"UnitMove actual destination call phase differs");
  const auto allowed=existing_native_can_move_into(*unit,*destination,cache,std::uint8_t{128});current();
  stage->destination_checked=true;stage->destination_allowed=allowed;
  if(!allowed){
   // Original next calls getBestDefender(NO_PLAYER,owner,this,true), enemyCity,
   // ATTACK admission and only then mission.pushTurn. Unknown capture cannot
   // be skipped from an ETA reject or an empty/copied caller mission list.
   stage->boundary=BnwNativeUnitPathToBoundary::destination_capture_admission;current();return observed();
  }
 }
 if(reject){
  need(path.path_to_reject_clear_operations<UINT64_MAX,"UnitMove reached ETA reject clear counter exhausted");auto after=stage->current_path;after.nodes.clear();++after.path_to_reject_clear_operations;
  current();need(prepare_existing_unit_path_cache(*unit)==cache,"UnitMove ETA rejection actual source cache changed");current();
  // Real SDK reject clear precedes mandatory GameplayUnitMoved publication,
  // including the empty visual queue. No full native0 return is fabricated.
  path.nodes.clear();++path.path_to_reject_clear_operations;stage->current_path=std::move(after);stage->boundary=BnwNativeUnitPathToBoundary::rejected_path_visualization;current();return observed();
 }
 // UnitMove bCombat=false,pCombatUnit=NULL. canMove is actual m_iMoves>0.
 // A first-path pointer NULL/zero moves short-circuits canMoveInto, but the
 // later native noCapture/enemyCity operands are still read in source order.
 bool allowed=false;if(field->expected>0&&stage->first_plot)allowed=existing_native_can_move_into(*unit,*stage->first_plot,cache,std::uint8_t{128});current();
 const bool no_capture=narrow(core(*unit,"m_iNoCaptureCount"))>0;
 const bool enemy_city=stage->first_plot&&native_path_enemy_city(*p->state,*unit,*stage->first_plot);(void)no_capture;(void)enemy_city;current();
 if(!allowed){
  stage->unit_move_checked=true;stage->unit_move_allowed=false;stage->boundary=BnwNativeUnitPathToBoundary::failed_unit_move_visualization;
  // Native sets bExecuteMove=true, then quickMovement can suppress the
  // publisher. Actual unsupplied quick-movement/visual queue is mandatory;
  // no empty-queue success, no cached-path clear/ETA return before this call.
  current();return observed();
 }
 need(stage->first_plot&&path.nodes.size()>=2,"UnitMove genuine cost requires retained physical first edge");
 ExistingPathMovementCostDispatch dispatch(*this,tree,*unit,paths,path,cache);const auto operands=existing_path_movement_cost_operands(dispatch);current();need(operands.to==*stage->first_plot,"UnitMove cost receiving physical first plot differs");
 const auto cost=BnwNativeUnitPathMovementCost::evaluate(*this,dispatch);current();
 // Actual CvUnit::move(default remaining0) has no clamp. Grounded constructor
 // proves no water/disembark transition; changeMoves(-cost) is next native
 // effect, before setXY. checked64 avoids original signed-UB overflow only.
 need(cost!=INT32_MIN,"UnitMove native cost unary minus overflow");const auto requested=narrow(std::int64_t(field->expected)-cost);
 need(existing_path_movement_cost_operands(dispatch)==operands&&prepare_existing_unit_path_cache(*unit)==cache,"UnitMove actual cost/source operands changed before private earned prefix");current();
 stage->unit_move_checked=true;stage->unit_move_allowed=true;stage->move_cost=cost;stage->moves_before=field->expected;stage->moves_requested=requested;stage->moves=field;stage->boundary=BnwNativeUnitPathToBoundary::movement_store_graphical_owner;
 // Do not perform a changed FAutoVariable store until real imported ShouldDim
 // Loan and selected/plot UI consumers preflight. This authentic Core record,
 // not the returned numeric copy, is the possible continuation authority.
 current();return observed();
}

namespace {
std::shared_ptr<ImportedFlagsArchive>make_imported_flags_archive(LiveSessionState&s,const saves::SavedSession&source,const std::shared_ptr<Bank>&bank,std::int32_t actor,std::int32_t id,std::uint64_t generation,bool restoring,const SessionCheckpointIdentity&identity){
 require_imported_mission_payload_lineage(source,identity);need(source.snapshot.all_players&&source.snapshot.all_players->layout==bnw,"Mission Flags requires genuine original payload Read source");
 const auto*unit=native_imported_get_at(s,actor,id);need(unit&&narrow(core(*unit,"m_iID"))==id,"Mission Flags requires complete actual fullID");const auto&original=source.snapshot.all_players->players.at(actor);need(original.saved_player_id==actor,"Mission Flags original player identity differs");
 const saves::SavedUnitObservation*loaded=nullptr;for(const auto&u:original.units.units)if(narrow(core(u,"m_iID"))==id){need(!loaded,"Duplicate original mission fullID");loaded=&u;}
 need(loaded&&core(*loaded,"m_eOwner")==actor&&loaded->core.start_offset==unit->core.start_offset&&loaded->core.end_offset==unit->core.end_offset&&loaded->tail.start_offset==unit->tail.start_offset&&loaded->tail.end_offset==unit->tail.end_offset,"Mission Flags source physical lineage differs");
 auto result=std::make_shared<ImportedFlagsArchive>();result->owner=bank;result->unit=unit;result->actor=actor;result->id=id;result->generation=generation;result->loaded=narrow(core(*loaded,"m_iFlags"));result->expected=narrow(core(*unit,"m_iFlags"));need(restoring||result->loaded==result->expected,"Mission Flags cannot invent original Load after unregistered mutation");
 saves::SavedUnitObservation*mutable_unit=nullptr;for(auto&u:s.world.all_players->players.at(actor).units.units)if(&u==unit)mutable_unit=&u;need(mutable_unit,"Mission Flags mutable SAME object absent");for(auto&f:mutable_unit->core.fields)if(name(f.name)=="m_iFlags"){need(!result->field,"Duplicate actual mission Flags");result->field=&f;}
 need(result->field&&result->field->encoding==saves::UnitCoreEncoding::i32&&result->field->values.size()==1&&result->field->bytes.empty(),"Mission Flags physical i32 alias unavailable");result->value=&result->field->values.front();
 std::optional<std::size_t>slot;for(std::size_t i=0;i<std::size(archive_specs);++i)if(name(archive_specs[i].member)=="m_iFlags"){need(!slot&&name(archive_specs[i].name)=="CvUnit::m_iFlags"&&archive_specs[i].kind==ArchiveKind::scalar,"Mission Flags native archive constructor slot differs");slot=i;}need(slot.has_value(),"Mission Flags native archive slot unavailable");result->native_index=*slot;return result;
}
void require_continue_entry_local(const std::shared_ptr<Bank>&bank,const LiveSessionState&s,const ExistingContinueMissionEntry&e,std::int32_t actor,std::uint64_t generation){
need(e.steps==0&&e.eta==-1,"Mission continuation unsupported original call operands");
 need(e.cache_cleared&&e.owner.lock().get()==bank.get()&&e.actor==actor&&e.generation==generation&&e.unit&&native_imported_get_at(s,actor,e.id)==e.unit&&bank->continue_mission_entries.at(e.id).get()==&e,"Mission continuation actual private entry retired/replaced");
 need(e.runtime&&bank->imported_mission_runtime.at(e.id).get()==e.runtime.get(),"Mission continuation actual OriginalRead runtime replaced");const auto&r=*e.runtime;need(r.creator==std::this_thread::get_id()&&r.owner.lock().get()==bank.get()&&r.unit==e.unit&&r.actor==actor&&r.id==e.id&&r.generation==generation&&r.mission_timer==0&&r.combat_city_owner==-1&&r.combat_city_id==-1&&r.visual_locations.empty(),"Mission continuation reached changed/unknown plain runtime");
 need(e.flags&&bank->imported_flags_fields.at(e.id).get()==e.flags.get(),"Mission continuation actual Flags archive replaced");const auto&f=*e.flags;const saves::UnitCoreField*actual=nullptr;for(const auto&v:e.unit->core.fields)if(name(v.name)=="m_iFlags"){need(!actual,"Mission continuation duplicate current Flags");actual=&v;}need(actual==f.field&&actual&&f.creator==std::this_thread::get_id()&&f.owner.lock().get()==bank.get()&&f.unit==e.unit&&f.actor==actor&&f.id==e.id&&f.generation==generation&&f.field&&f.value&&f.field->encoding==saves::UnitCoreEncoding::i32&&f.field->values.size()==1&&f.field->bytes.empty()&&&f.field->values.front()==f.value&&narrow(*f.value)==f.expected&&narrow(core(*e.unit,"m_iFlags"))==f.expected,"Mission continuation physical Flags alias/dirty lineage changed");
 need(s.world.pregame&&s.world.pregame->archive.integers.at("game_turn")==e.game_turn&&s.setup.game_turn==e.game_turn&&s.random.map.algorithm()==RandomAlgorithm::bnw&&s.random.gameplay.algorithm()==RandomAlgorithm::bnw&&s.random.map.state()==e.map_rng&&s.random.gameplay.state()==e.game_rng,"Mission continuation actual clock/RNG changed");
 if(const auto*registry=s.setup.extensions.find<Registry>())for(const auto&[other,registered]:registry->players){(void)other;need(registered&&!registered->retired.load()&&registered->pending_units.empty()&&registered->existing_path_to.empty()&&(registered.get()==bank.get()||registered->continue_mission_entries.empty()),"Mission continuation overlapping actual imported body");}
 if(e.phase==BnwUnitMissionContinuationPhase::empty_head_awake_exit_flags_stored_before_combat_warned||e.phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed)need(native_imported_mission_rows(*e.unit).empty()&&narrow(core(*e.unit,"m_eActivityType"))==0,"Mission empty-head exit actual NULL/AWAKE proof changed");
 need(e.before_path.clear_operations<UINT64_MAX&&e.current_path.nodes.empty()&&e.current_path.destination==UINT32_MAX&&e.current_path.clear_operations==e.before_path.clear_operations+1&&e.current_path.generation_operations==e.before_path.generation_operations&&e.current_path.path_to_reject_clear_operations==e.before_path.path_to_reject_clear_operations,"Mission continuation once cache journal changed");
}
}

struct BnwUnitCombatWarnedRestorePlan::Storage {
 const std::thread::id creator=std::this_thread::get_id();bool alive=true,original_read=false;
 const BnwNativeUnitAllocatorCore*core=nullptr;const ui::SelectedBnwPreviewHost*host=nullptr;
 std::unique_ptr<BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner>issuer;
 BnwUnitMissionContinuationCheckpoint::Record record;
 std::shared_ptr<ImportedFlagsArchive>flags;std::shared_ptr<ImportedMissionRuntime>runtime;
 const saves::SavedUnitObservation*unit=nullptr;std::uint64_t generation=0;
 std::shared_ptr<BnwTwoLayerUnitPathFinder>tree;BnwUnitPathOwner*paths=nullptr;BnwUnitRetainedPath*path=nullptr;
 std::string actual_before;
 std::shared_ptr<BnwNativeUnitAllocatorCore::ContinueMissionCombatWarnedAttempt>attempt;
};
BnwUnitCombatWarnedRestorePlan::BnwUnitCombatWarnedRestorePlan(const BnwNativeUnitAllocatorCore&core,const saves::CivisSaveSection&section):storage_(std::make_unique<Storage>()){
 storage_->core=&core;storage_->issuer=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);
 try{core.initialize_unit_combat_warned_restore_plan(*this,section);}
 catch(...){if(core.impl_->combat_warned_restore_plan==this)core.impl_->combat_warned_restore_plan=nullptr;storage_->alive=false;throw;}
}
BnwUnitCombatWarnedRestorePlan::~BnwUnitCombatWarnedRestorePlan(){
 if(!storage_)return;
 if(storage_->creator!=std::this_thread::get_id())std::terminate();
 retire_without_callbacks();
 // Retirement precedes the one counted Core pin and physical source anchors.
}
void BnwUnitCombatWarnedRestorePlan::retire_without_callbacks()noexcept{
 if(!storage_)return;if(storage_->creator!=std::this_thread::get_id())std::terminate();
 storage_->alive=false;const auto*core=storage_->core;
 if(core&&core->impl_&&core->impl_->combat_warned_restore_plan==this)core->impl_->combat_warned_restore_plan=nullptr;
}
void BnwUnitCombatWarnedRestorePlan::require_current_without_callbacks()const{
 need(storage_&&storage_->creator==std::this_thread::get_id()&&storage_->alive&&storage_->core,"Foreign/retired Unit completion restore scope");storage_->core->require_unit_combat_warned_restore_plan_without_callbacks(*this);
}
bool BnwUnitCombatWarnedRestorePlan::original_payload_read_qualified()const noexcept{return storage_&&storage_->alive&&storage_->original_read;}
bool BnwUnitCombatWarnedRestorePlan::before()const noexcept{return storage_->record.combat_warned_before;}
std::uint64_t BnwUnitCombatWarnedRestorePlan::before_ordinal()const noexcept{return storage_->record.combat_warned_before_ordinal;}
std::uint64_t BnwUnitCombatWarnedRestorePlan::ordinal()const noexcept{return storage_->record.combat_warned_ordinal;}
const ui::SelectedBnwPreviewHost&BnwUnitCombatWarnedRestorePlan::host_without_callbacks()const{require_current_without_callbacks();return *storage_->host;}
void BnwNativeUnitAllocatorCore::initialize_unit_combat_warned_restore_plan(BnwUnitCombatWarnedRestorePlan&plan,const saves::CivisSaveSection&section)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->bank->busy.load()&&!p->combat_warned_restore_plan,"Completion restore Plan requires ONE actual guarded Core issuer");require_should_dim_outside_owner_without_callbacks();p->check();
 auto&q=*plan.storage_;q.record=BnwUnitMissionContinuationCheckpoint::decode(section,p->host->identity());const auto&r=q.record;
 need(q.core==this&&r.actor==p->actor&&r.phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed,"Completion restore Plan requires bound actual phase4/actor");
 require_imported_mission_payload_lineage(*p->source,p->host->identity());
 need(p->source->snapshot.all_players&&p->source->snapshot.all_players->layout==bnw,"Completion OriginalPayload Read qualifier absent");
 q.unit=native_imported_get_at(*p->state,p->actor,r.full_id);need(q.unit&&narrow(core(*q.unit,"m_iID"))==r.full_id&&q.unit->core.start_offset==r.core_begin&&q.unit->core.end_offset==r.core_end&&q.unit->tail.start_offset==r.tail_begin&&q.unit->tail.end_offset==r.tail_end,"Completion restore physical fullID/source differs");
 need(native_imported_mission_rows(*q.unit).empty()&&narrow(core(*q.unit,"m_eActivityType"))==0,"Completion restore requires actual NULL/equal AWAKE native branch");
 q.runtime=p->bank->imported_mission_runtime.at(r.full_id);need(bool(q.runtime),"Completion OriginalRead runtime absent");q.flags=make_imported_flags_archive(*p->state,*p->source,p->bank,p->actor,r.full_id,p->generation,true,p->host->identity());
 need(q.flags->loaded==r.flags_loaded&&q.flags->native_index==r.native_archive_index&&(q.flags->expected==r.flags_before||q.flags->expected==r.flags_current),"Completion Flags pristine original/overlay lineage differs");
 need(BnwUnitPathOwner::project_unit_flags_revision(*p->state,p->host->identity(),p->host->database(),{p->actor,r.full_id},r.flags_before)==r.before_world_revision&&BnwUnitPathOwner::project_unit_flags_revision(*p->state,p->host->identity(),p->host->database(),{p->actor,r.full_id},r.flags_current)==r.current_world_revision,"Completion historical projected world differs");
 q.actual_before=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());need(q.actual_before==(q.flags->expected==r.flags_before?r.before_world_revision:r.current_world_revision),"Completion actual Flags/world pairing differs");
 q.tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);q.paths=&p->host->session().bnw_native_unit_paths_for_core(*this,*q.tree);q.path=&q.paths->paths_.at({p->actor,r.full_id});q.host=p->host.get();q.generation=p->generation;
 q.original_read=true;p->combat_warned_restore_plan=&plan;require_unit_combat_warned_restore_plan_without_callbacks(plan);
}
void BnwNativeUnitAllocatorCore::require_unit_combat_warned_restore_plan_without_callbacks(const BnwUnitCombatWarnedRestorePlan&plan)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&plan.storage_&&plan.storage_->creator==p->thread&&plan.storage_->alive&&p->combat_warned_restore_plan==&plan,"Completion scope retired/foreign before mutable source");require_should_dim_outside_owner_without_callbacks();const auto&q=*plan.storage_;const auto&r=q.record;
 need(q.core==this&&q.host==p->host.get()&&q.generation==p->generation&&q.original_read&&p->bank->busy.load()&&r.actor==p->actor&&r.phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed,"Completion canonical scope/phase/source differs");
 p->host->session().require_bnw_native_unit_paths_current_without_callbacks(*q.paths,*q.tree,*this);
 need(native_imported_get_at(*p->state,p->actor,r.full_id)==q.unit&&q.unit&&q.unit->core.start_offset==r.core_begin&&q.unit->core.end_offset==r.core_end&&q.unit->tail.start_offset==r.tail_begin&&q.unit->tail.end_offset==r.tail_end,"Completion current physical unit changed");
 const saves::UnitCoreField*actual=nullptr;for(const auto&field:q.unit->core.fields)if(name(field.name)=="m_iFlags"){need(!actual,"Completion duplicate actual Flags");actual=&field;}
 need(actual&&actual==q.flags->field&&actual->encoding==saves::UnitCoreEncoding::i32&&actual->values.size()==1&&actual->bytes.empty()&&&actual->values.front()==q.flags->value&&narrow(actual->values.front())==q.flags->expected&&q.flags->owner.lock().get()==p->bank.get()&&q.flags->creator==p->thread&&!q.flags->dirty&&q.flags->changes==0,"Completion original physical Flags alias/dirty state changed");
 const auto&node=*q.runtime;need(p->bank->imported_mission_runtime.at(r.full_id).get()==q.runtime.get()&&node.creator==p->thread&&node.owner.lock().get()==p->bank.get()&&node.unit==q.unit&&node.actor==p->actor&&node.id==r.full_id&&node.generation==p->generation&&node.mission_timer==0&&node.combat_city_owner==-1&&node.combat_city_id==-1&&node.visual_locations.empty(),"Completion original plain runtime changed");
 const auto&map=p->state->world.map_header;need(map.guid==r.map_guid&&map.grid.width==r.width&&map.grid.height==r.height&&map.wrap_x==r.wrap_x&&map.wrap_y==r.wrap_y&&p->source->snapshot.map_header.guid==r.map_guid,"Completion original map source changed");
 need(p->state->world.pregame&&p->state->world.pregame->archive.integers.at("game_turn")==r.game_turn&&p->state->setup.game_turn==r.game_turn&&p->state->random.map.algorithm()==RandomAlgorithm::bnw&&p->state->random.gameplay.algorithm()==RandomAlgorithm::bnw&&p->state->random.map.state()==r.map_rng&&p->state->random.gameplay.state()==r.game_rng,"Completion actual clock/RNG changed");
 need(native_imported_mission_rows(*q.unit).empty()&&narrow(core(*q.unit,"m_eActivityType"))==0&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==q.actual_before,"Completion NULL/AWAKE/current world changed");
 need(&q.paths->paths_.at({p->actor,r.full_id})==q.path,"Completion canonical path address changed");for(const auto&[key,path]:q.paths->paths_){(void)key;need(path==BnwUnitRetainedPath{},"Completion fresh retained path changed");}
 need(!q.tree->busy_&&!q.tree->callbacks_&&!q.tree->best_&&!q.tree->open_&&!q.tree->tail_&&!q.tree->closed_&&!q.tree->control_->data_identity(),"Completion fresh common tree changed");
 need(p->bank->imported_flags_fields.empty(),"Completion restore prior Flags journal exists");
 if(q.attempt)need(p->continue_mission_game_attempts.size()==1&&p->continue_mission_game_attempts.at(r.full_id).get()==q.attempt.get()&&q.attempt->creator==p->thread&&q.attempt->id==r.full_id&&q.attempt->actor==p->actor&&q.attempt->generation==p->generation&&q.attempt->bank.lock().get()==p->bank.get()&&!q.attempt->core.expired()&&!q.attempt->core.owner_before(factory_self_)&&!factory_self_.owner_before(q.attempt->core)&&!q.attempt->native_return_earned,"Completion restore reserved exact Core issuer changed");
 else need(p->continue_mission_game_attempts.empty(),"Completion restore prior Game attempt exists");if(const auto*registry=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:registry->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty()&&bank->existing_path_to.empty()&&bank->continue_mission_entries.empty(),"Completion restore overlapping actual body");}
}


BnwPathfinderCacheControlOwner BnwNativeUnitAllocatorCore::make_original_payload_mission_pathfinder_controls()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();require_imported_mission_payload_lineage(*p->source,p->host->identity());
 const auto&source=*p->source;need(source.snapshot.map_header.layout==bnw&&source.snapshot.pregame&&source.snapshot.pregame->layout==bnw&&source.snapshot.map_plots&&source.snapshot.map_plots->layout==bnw&&p->bank->pending_units.empty()&&p->bank->continue_mission_entries.empty()&&p->continue_mission_game_attempts.empty(),"OriginalPayload mission path controls require fresh actual complete map/Core");
 const auto identity=p->host->identity();BnwPathfinderCacheControlOwner owner;owner.identity=identity;owner.map_guid=source.snapshot.map_header.guid;owner.grid=source.snapshot.map_header.grid;owner.wrap_x=source.snapshot.map_header.wrap_x;owner.wrap_y=source.snapshot.map_header.wrap_y;
 // Reuse the actual source ctorв†’Initializeв†’setup six-finder implementation.
 // No DTO ready flag, altered container, reset of a retained control or search.
 owner.initialize_source_controls();p->check();(void)issuer->current();require_imported_mission_payload_lineage(*p->source,identity);need(p->source==&source&&owner.map_guid==p->state->world.map_header.guid&&owner.grid.width==p->state->world.map_header.grid.width&&owner.grid.height==p->state->world.map_header.grid.height&&owner.wrap_x==p->state->world.map_header.wrap_x&&owner.wrap_y==p->state->world.map_header.wrap_y,"OriginalPayload actual controls map changed");return owner;
}
BnwUnitPathOwner BnwNativeUnitAllocatorCore::make_original_payload_mission_unit_paths()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();require_imported_mission_payload_lineage(*p->source,p->host->identity());
 need(p->bank->pending_units.empty()&&p->bank->continue_mission_entries.empty()&&p->continue_mission_game_attempts.empty(),"OriginalPayload mission paths require actual pristine native registrar");
 auto owner=bind_bnw_unit_paths_original_import(*p->source,p->host->identity());p->check();(void)issuer->current();require_imported_mission_payload_lineage(*p->source,p->host->identity());return owner;
}

void BnwNativeUnitAllocatorCore::require_restored_mission_owner_current_without_callbacks(const saves::CivisSaveSection&section)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign restored mission owner before mutable source");
 require_should_dim_outside_owner_without_callbacks();require_native_common_pathfinder_session_without_callbacks(*p->host->session_);
 need(!p->bank->busy.load()&&p->continue_mission_game_attempts.size()==1&&p->bank->continue_mission_entries.size()==1,
      "Restored mission ongoing owner journal missing/reentrant");
 compare(*p->state,p->actor,p->bank->baseline);check_imported_cycle_nodes(*p->bank,*p->state,p->actor);
 const auto attempt=p->continue_mission_game_attempts.begin()->second;
 need(attempt&&attempt->creator==p->thread&&attempt->native_return_earned&&attempt->context&&attempt->reference&&
      attempt->receipt&&attempt->receipt->was_stored()&&attempt->receipt->restored_history()&&attempt->restored_checkpoint&&
      attempt->restored_checkpoint->id==section.id&&attempt->restored_checkpoint->flags==section.flags&&
      attempt->restored_checkpoint->bytes==section.bytes,"Restored mission ongoing installed bytes/earned Game history differ");
 const auto found=p->bank->continue_mission_entries.find(attempt->id);
 need(found!=p->bank->continue_mission_entries.end()&&found->second,"Restored mission ongoing physical entry absent");const auto entry=found->second;
 need(entry->phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed&&entry->cache_cleared&&
      entry->steps==0&&entry->eta==-1&&attempt->entry.lock().get()==entry.get()&&attempt->bank.lock().get()==p->bank.get()&&
      attempt->actor==p->actor&&attempt->generation==p->generation&&!attempt->core.expired()&&
      !attempt->core.owner_before(factory_self_)&&!factory_self_.owner_before(attempt->core)&&
      entry->owner.lock().get()==p->bank.get()&&entry->actor==p->actor&&entry->id==attempt->id&&entry->generation==p->generation&&
      entry->combat_warned_attempt&&!entry->combat_warned_attempt->expired()&&
      !entry->combat_warned_attempt->owner_before(attempt)&&!attempt.owner_before(*entry->combat_warned_attempt),
      "Restored mission ongoing canonical Core/entry lineage changed");
 // Old authentic completion is immutable history, not a claim that the
 // current global Game flag/ordinal still equals this historical return.
 attempt->reference->require_destroyed_for(*p->host);
 need(p->host->bnw_live_game_context_.get()==attempt->context.get()&&
      !p->host->bnw_live_game_context_.owner_before(attempt->context)&&
      !attempt->context.owner_before(p->host->bnw_live_game_context_),
      "Restored mission ongoing canonical Game owner changed");
 need(attempt->reference->creator_==p->thread&&attempt->reference->context_.get()==attempt->context.get()&&
      !attempt->reference->context_.owner_before(attempt->context)&&!attempt->context.owner_before(attempt->reference->context_)&&
      attempt->reference->host_.get()==p->host.get()&&!attempt->reference->host_.owner_before(p->host)&&
      !p->host.owner_before(attempt->reference->host_),"Restored mission ongoing actual destroyed Game reference changed");
 const auto issued=attempt->reference->issued_combat_warned_receipt_without_callbacks();
 need(issued&&issued.get()==attempt->receipt.get()&&!issued.owner_before(attempt->receipt)&&
      !attempt->receipt.owner_before(issued)&&issued->was_stored()&&issued->restored_history(),
      "Restored mission ongoing history is not SAME actual once-issued Game receipt");
 const auto tree=attempt->restored_tree.lock();need(tree&&entry->paths,"Restored mission ongoing canonical path owner absent");
 auto&paths=*entry->paths;p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
 const auto current_path=paths.paths_.find({p->actor,attempt->id});
 need(!tree->busy_&&!tree->callbacks_&&current_path!=paths.paths_.end()&&entry->path==&current_path->second,
      "Restored mission ongoing SAME retained path allocation changed");
 const auto*unit=native_imported_get_at(*p->state,p->actor,attempt->id);
 need(unit&&unit==entry->unit&&narrow(core(*unit,"m_iID"))==attempt->id&&narrow(core(*unit,"m_eOwner"))==p->actor,
      "Restored mission ongoing actual imported fullID changed");
 need(p->source->snapshot.all_players&&p->source->snapshot.all_players->layout==bnw,
      "Restored mission ongoing original imported roster absent");
 const saves::SavedUnitObservation*original=nullptr;
 for(const auto&candidate:p->source->snapshot.all_players->players.at(p->actor).units.units)
  if(narrow(core(candidate,"m_iID"))==attempt->id){need(!original,"Restored mission ongoing duplicate original fullID");original=&candidate;}
 need(original&&narrow(core(*original,"m_eOwner"))==p->actor&&original->core.start_offset==unit->core.start_offset&&
      original->core.end_offset==unit->core.end_offset&&original->tail.start_offset==unit->tail.start_offset&&
      original->tail.end_offset==unit->tail.end_offset,"Restored mission ongoing physical original object intervals changed");
 const auto runtime=p->bank->imported_mission_runtime.find(attempt->id);
 need(entry->runtime&&runtime!=p->bank->imported_mission_runtime.end()&&runtime->second.get()==entry->runtime.get(),
      "Restored mission ongoing actual plain runtime owner changed");
 const auto&node=*entry->runtime;
 need(node.creator==p->thread&&node.owner.lock().get()==p->bank.get()&&node.unit==unit&&node.actor==p->actor&&
      node.id==attempt->id&&node.generation==p->generation,"Restored mission ongoing actual runtime source differs");
 const auto registered_flags=p->bank->imported_flags_fields.find(attempt->id);
 need(entry->flags&&registered_flags!=p->bank->imported_flags_fields.end()&&registered_flags->second.get()==entry->flags.get(),
      "Restored mission ongoing actual Flags registrar changed");
 const auto&flags=*entry->flags;const saves::UnitCoreField*actual=nullptr;
 for(const auto&field:unit->core.fields)if(name(field.name)=="m_iFlags"){need(!actual,"Restored mission ongoing duplicate actual Flags");actual=&field;}
 need(actual&&actual==flags.field&&flags.creator==p->thread&&flags.owner.lock().get()==p->bank.get()&&flags.unit==unit&&
      flags.actor==p->actor&&flags.id==attempt->id&&flags.generation==p->generation&&actual->encoding==saves::UnitCoreEncoding::i32&&
      actual->values.size()==1&&actual->bytes.empty()&&&actual->values.front()==flags.value&&narrow(actual->values.front())==flags.expected,
      "Restored mission ongoing current physical Flags alias/producer lineage changed");
 if(const auto*registry=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:registry->players){
  (void)actor;need(bank&&!bank->retired.load()&&!bank->constructor_pending&&bank->pending_units.empty()&&bank->existing_path_to.empty()&&
      (bank.get()==p->bank.get()||bank->continue_mission_entries.empty()),"Restored mission ongoing unknown/incomplete actual unit body");
 }
 require_native_common_pathfinder_session_without_callbacks(*p->host->session_);require_should_dim_outside_owner_without_callbacks();
}
void BnwNativeUnitAllocatorCore::require_restored_mission_weak_owner_continuity_current_without_callbacks(
 const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak,const saves::CivisSaveSection&section){
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(weak);const auto&owner=issuer->current();
 owner.require_restored_mission_owner_current_without_callbacks(section);
}

void BnwNativeUnitAllocatorCore::require_restored_mission_checkpoint_current_without_callbacks(const saves::CivisSaveSection&section)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign restored mission seal before mutable source");require_should_dim_outside_owner_without_callbacks();
 need(!p->bank->busy.load()&&p->continue_mission_game_attempts.size()==1&&p->bank->continue_mission_entries.size()==1,"Restored mission final exact journal missing/reentrant");
 const auto attempt=p->continue_mission_game_attempts.begin()->second;need(attempt&&attempt->creator==p->thread&&attempt->native_return_earned&&attempt->reference&&attempt->receipt&&attempt->receipt->was_stored()&&attempt->restored_checkpoint&&attempt->restored_checkpoint->id==section.id&&attempt->restored_checkpoint->flags==section.flags&&attempt->restored_checkpoint->bytes==section.bytes,"Restored mission final installed bytes/actual earned Game lineage differ");
 const auto found=p->bank->continue_mission_entries.find(attempt->id);need(found!=p->bank->continue_mission_entries.end()&&found->second,"Restored mission current physical entry missing");const auto entry=found->second;
 need(entry->phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed&&attempt->entry.lock().get()==entry.get()&&attempt->bank.lock().get()==p->bank.get()&&attempt->actor==p->actor&&attempt->generation==p->generation&&!attempt->core.expired()&&!attempt->core.owner_before(factory_self_)&&!factory_self_.owner_before(attempt->core)&&entry->combat_warned_attempt&&!entry->combat_warned_attempt->expired()&&!entry->combat_warned_attempt->owner_before(attempt)&&!attempt.owner_before(*entry->combat_warned_attempt),"Restored mission actual canonical Core/entry/source changed");
 // Authenticated pure Game member/current global ordinal/real Destroy/ref0.
 const auto actual=attempt->reference->snapshot_combat_warned_return(attempt->receipt,true);need(actual.restored_history&&!actual.after,"Restored mission lacks genuine Game-owned installed history");
 auto&session=p->host->session();const auto tree=attempt->restored_tree.lock();need(tree&&entry->paths,"Restored mission canonical common finder/path owner absent");
 // The genuine alias pin keeps the owning controls/path allocation alive;
 // Session checks SAME current storage before any actual paths dereference.
 auto&paths=*entry->paths;session.require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);
 need(!tree->busy_&&!tree->callbacks_&&entry->path==&paths.paths_.at({p->actor,attempt->id})&&*entry->path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"Restored mission final physical path/world seal changed");
}
void BnwNativeUnitAllocatorCore::require_restored_mission_weak_owner_current_without_callbacks(const std::weak_ptr<const BnwNativeUnitAllocatorCore>&weak,const saves::CivisSaveSection&section){
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(weak);const auto&owner=issuer->current();owner.require_restored_mission_checkpoint_current_without_callbacks(section);
}

void BnwNativeUnitAllocatorCore::require_live_gold_roster_source_without_callbacks(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,std::int32_t actor)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Foreign Gold roster Core before mutable source read");require_should_dim_outside_owner_without_callbacks();
 need(p->host.get()==&host&&p->state.get()==&state&&p->actor==actor&&actor>=0&&actor<64&&host.session_&&host.session_.get()==&p->host->session()&&
  !host.civis_canonical_self_.expired()&&!host.civis_canonical_self_.owner_before(p->host)&&!p->host.owner_before(host.civis_canonical_self_)&&
  host.identity_.payload_sha256==p->host->identity().payload_sha256&&host.identity_.rules_sha256==p->fingerprint,"Gold roster SAME canonical Host/actor/source control differs");
 require_native_common_pathfinder_session_without_callbacks(*host.session_);
 need(!p->bank->busy.load()&&p->bank->pending_units.empty()&&!p->bank->constructor_pending,"Gold roster cannot observe an incomplete/reentrant allocator");compare(state,actor,p->bank->baseline);check_imported_cycle_nodes(*p->bank,state,actor);
 need(p->state.get()==host.session_->state_.get()&&p->source==host.session_->source_.get()&&p->generation==host.session_->state_generation_&&!p->bank->retired.load(),"Gold roster final pure actual source retired");
}
void BnwNativeUnitAllocatorCore::require_live_gold_roster_source(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,std::int32_t actor)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 {Impl::Guard invocation(p);p->check();(void)issuer->current();need(p->host.get()==&host&&p->state.get()==&state&&p->actor==actor,"Gold roster receiving actual source/actor differs");p->check();(void)issuer->current();}
 require_live_gold_roster_source_without_callbacks(host,state,actor);
}
std::int32_t BnwNativeUnitAllocatorCore::read_native_existing_combat_unit_count()const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();
 need(p->bank->pending_units.empty()&&!p->bank->constructor_pending,"Combat count requires complete actual imported manager");const auto&state=*p->state;const auto revision=bnw_unit_order_world_revision(state,p->host->identity(),p->host->database());const auto map_rng=state.random.map.state(),game_rng=state.random.gameplay.state();
 std::int32_t count=0,seen=0;
 // Actual FLTA::BeginIterate/NextIterate includes occupied slots in ascending
 // native order. GetAt(index-only) resolves the real complete object; its
 // full-ID lookup is checked independently against that SAME address.
 for(std::int32_t slot=0;slot<=p->bank->last;++slot){const auto*unit=native_imported_get_at(state,p->actor,slot);if(!unit)continue;const auto id=narrow(core(*unit,"m_iID"));need((id&8191)==slot&&narrow(core(*unit,"m_eOwner"))==p->actor&&native_imported_get_at(state,p->actor,id)==unit,"Combat count native first/next object/fullID differs");
  const auto&owned=state.maintenance.players.at(p->actor).units.at(id);need(owned.source_core==std::optional{std::pair{unit->core.start_offset,unit->core.end_offset}},"Combat count raw/owned source unit interval differs");const saves::UnitCoreField*base=nullptr;for(const auto&field:unit->core.fields)if(name(field.name)=="m_iBaseCombat"){need(!base,"Combat count duplicate actual BaseCombat");base=&field;}need(base&&base->encoding==saves::UnitCoreEncoding::i32&&base->values.size()==1&&base->bytes.empty(),"Combat count source typed BaseCombat unavailable");
  ++seen;if(narrow(base->values.front())>0)++count;
 }
 need(seen==narrow(static_cast<std::int64_t>(state.world.all_players->players.at(p->actor).units.units.size()))&&seen==p->bank->last+1-p->bank->free_count,"Combat count native traversal/manager extent differs");
 p->check();(void)issuer->current();need(bnw_unit_order_world_revision(state,p->host->identity(),p->host->database())==revision&&state.random.map.state()==map_rng&&state.random.gameplay.state()==game_rng,"Combat count actual source/RNG changed during query");p->check();(void)issuer->current();return count;
}

std::optional<BnwNativeContinueMissionEntryObservation>BnwNativeUnitAllocatorCore::advance_native_existing_continue_mission_cache_prefix(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty()&&bank->existing_path_to.empty()&&bank->continue_mission_entries.empty(),"ContinueMission requires completed rosters and no overlapping native body");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 need(p->bank->existing_path_to.empty()&&p->bank->continue_mission_entries.empty(),"ContinueMission cannot overlap/replay an incomplete actual unit body");const auto id=narrow(core(*unit,"m_iID"));
 const auto runtime=p->bank->imported_mission_runtime.find(id);need(runtime!=p->bank->imported_mission_runtime.end()&&runtime->second,"ContinueMission original Read plain runtime mandatory");
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const BnwUnitOrderKey key{p->actor,id};auto&path=paths.paths_.at(key);
 need(path.clear_operations<UINT64_MAX,"ContinueMission real ClearPathCache counter exhausted");
 need(!p->bank->imported_flags_fields.contains(id),"ContinueMission Flags prior owner must have genuine consumed phase");const auto flags=make_imported_flags_archive(*p->state,*p->source,p->bank,p->actor,id,p->generation,false,p->host->identity());
 auto entry=std::make_shared<ExistingContinueMissionEntry>();entry->owner=p->bank;entry->unit=unit;entry->runtime=runtime->second;entry->flags=flags;entry->actor=p->actor;entry->id=id;entry->generation=p->generation;entry->world_revision=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());entry->paths=&paths;entry->path=&path;entry->before_path=path;entry->current_path=path;
 entry->before_world_revision=entry->world_revision;entry->game_turn=narrow(p->state->setup.game_turn);entry->map_rng=p->state->random.map.state();entry->game_rng=p->state->random.gameplay.state();
 need(p->state->world.pregame&&p->state->world.pregame->archive.integers.at("game_turn")==entry->game_turn&&p->state->random.map.algorithm()==RandomAlgorithm::bnw&&p->state->random.gameplay.algorithm()==RandomAlgorithm::bnw,"ContinueMission clock/RNG profile differs");
 entry->current_path.nodes.clear();entry->current_path.destination=UINT32_MAX;++entry->current_path.clear_operations;
 const auto current=[&](bool stored){p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
  need(!tree->busy_&&!tree->callbacks_&&native_imported_get_at(*p->state,p->actor,query_id)==unit&&&paths.paths_.at(key)==entry->path&&path==(stored?entry->current_path:entry->before_path)&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"ContinueMission actual shared path/GetAt/world changed");
  const auto&node=*entry->runtime;need(p->bank->imported_mission_runtime.at(id).get()==entry->runtime.get()&&node.creator==std::this_thread::get_id()&&node.owner.lock().get()==p->bank.get()&&node.unit==unit&&node.actor==p->actor&&node.id==id&&node.generation==p->generation&&node.mission_timer==0&&node.combat_city_owner==-1&&node.combat_city_id==-1&&node.visual_locations.empty(),"ContinueMission actual typed runtime/defaults changed BEFORE first effect");
  const saves::UnitCoreField*actual=nullptr;for(const auto&f:unit->core.fields)if(name(f.name)=="m_iFlags"){need(!actual,"ContinueMission duplicate physical Flags");actual=&f;}
  need(actual==flags->field&&actual&&actual->encoding==saves::UnitCoreEncoding::i32&&actual->values.size()==1&&&actual->values.front()==flags->value&&narrow(actual->values.front())==flags->expected&&flags->expected==flags->loaded&&!flags->dirty&&flags->changes==0,"ContinueMission actual source Flags alias changed BEFORE first effect");
  need(p->state->world.pregame->archive.integers.at("game_turn")==entry->game_turn&&p->state->setup.game_turn==entry->game_turn&&p->state->random.map.algorithm()==RandomAlgorithm::bnw&&p->state->random.gameplay.algorithm()==RandomAlgorithm::bnw&&p->state->random.map.state()==entry->map_rng&&p->state->random.gameplay.state()==entry->game_rng,"ContinueMission actual clock/RNG changed BEFORE first effect");
  if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty()&&bank->existing_path_to.empty()&&(bank.get()==p->bank.get()||bank->continue_mission_entries.empty()),"ContinueMission other actual body appeared BEFORE first effect");}
 };
 auto staged_entries=p->bank->continue_mission_entries;auto staged_flags=p->bank->imported_flags_fields;need(staged_entries.emplace(id,entry).second&&staged_flags.emplace(id,flags).second,"ContinueMission actual staged first-effect journal replay");
 current(false);p->bank->continue_mission_entries.swap(staged_entries);p->bank->imported_flags_fields.swap(staged_flags);
 // The actual first original effect, before head/steps/advisor/CanMove. Even
 // empty queue and zero moves clear BOTH vector and cached destination.
 paths.clear(key);entry->cache_cleared=true;current(true);
 // Saved archive Flags|1 is the next genuine phase; this entry and its actual
 // clean original Load registration can now mint the finite checkpoint.
 require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);
 return BnwNativeContinueMissionEntryObservation{p->actor,id,path.clear_operations};
}

BnwNativeContinueMissionPhaseObservation BnwNativeUnitAllocatorCore::advance_native_existing_continue_mission_flags_prefix(std::int32_t id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();
 const auto found=p->bank->continue_mission_entries.find(id);need(found!=p->bank->continue_mission_entries.end()&&found->second,"Mission Flags requires genuine earned cache prefix");const auto entry=found->second;need(entry->phase==BnwUnitMissionContinuationPhase::cache_cleared_before_evaluating_flags,"Mission evaluating store cannot replay earned phase");
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);auto&path=paths.paths_.at({p->actor,id});const auto flags=entry->flags;
 const auto current=[&]{p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);need(!tree->busy_&&!tree->callbacks_&&entry->paths==&paths&&entry->path==&path&&path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"Mission Flags actual cache/world changed");};
 current();need(flags->expected==flags->loaded&&flags->changes==0&&!flags->dirty,"Mission Flags original single-field Load lineage changed");const auto requested=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(flags->expected)|1U);const bool changed=requested!=flags->expected;
 auto after_revision=BnwUnitPathOwner::project_unit_flags_revision(*p->state,p->host->identity(),p->host->database(),{p->actor,id},requested);current();
 // Callback-free actual FAutoVariable-equivalent operator= primitive. Equal
 // operands advance the phase but do not touch/dirty/change the archive.
 if(changed){*flags->value=requested;flags->expected=requested;flags->dirty=true;++flags->changes;}
 entry->world_revision.swap(after_revision);entry->phase=BnwUnitMissionContinuationPhase::evaluating_flags_stored_before_head;
 current();return {p->actor,id,flags->expected,flags->loaded,entry->phase,path.clear_operations,flags->changes,flags->dirty,flags};
}

BnwNativeContinueMissionPhaseObservation BnwNativeUnitAllocatorCore::advance_native_existing_continue_mission_empty_head_exit_flags_prefix(std::int32_t id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();
 const auto found=p->bank->continue_mission_entries.find(id);need(found!=p->bank->continue_mission_entries.end()&&found->second,"ContinueMission head requires genuine evaluating phase");const auto entry=found->second;
 need(entry->phase==BnwUnitMissionContinuationPhase::evaluating_flags_stored_before_head,"ContinueMission head/exit cannot replay an earned phase");
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);auto&path=paths.paths_.at({p->actor,id});const auto flags=entry->flags;
 const auto current=[&]{p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);need(!tree->busy_&&!tree->callbacks_&&entry->paths==&paths&&entry->path==&path&&path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"ContinueMission head actual path/world changed");};
 current();
 // CvUnitMission.cpp first real head lookup, not its debug assertion. Unknown
 // nonempty bodies remain a mandatory boundary before any exit mutation.
 need(native_imported_mission_rows(*entry->unit).empty(),"ContinueMission nonempty head requires genuine attack/mission body owner");
 // CvUnit::SetActivityType(AWAKE): the equal branch bypasses plot, fortify,
 // selection getters and both dirty setters. Changed waking is not granted
 // by a copied activity number or by assuming an absent selection plot.
 need(narrow(core(*entry->unit,"m_eActivityType"))==0,"ContinueMission changed AWAKE requires Fortify/InfoBar/graphics/Selection owners before activity store");
 const auto requested=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(flags->expected)&~1U);
 need(requested!=flags->expected&&flags->changes<2,"ContinueMission exit requires actual evaluating bit and finite archive lineage");
 auto after_revision=BnwUnitPathOwner::project_unit_flags_revision(*p->state,p->host->identity(),p->host->database(),{p->actor,id},requested);
 current();need(native_imported_mission_rows(*entry->unit).empty()&&narrow(core(*entry->unit,"m_eActivityType"))==0,"ContinueMission final NULL/equal-AWAKE source changed before exit flag store");
 // Original ContinueMissionExit Flags &= ~EVALUATING precedes the mandatory
 // Game::SetCombatWarned(false). Store earns archive dirty history even when
 // the final scalar equals the original Load value after two assignments.
 *flags->value=requested;flags->expected=requested;flags->dirty=true;++flags->changes;
 entry->world_revision.swap(after_revision);entry->phase=BnwUnitMissionContinuationPhase::empty_head_awake_exit_flags_stored_before_combat_warned;
 current();return {p->actor,id,flags->expected,flags->loaded,entry->phase,path.clear_operations,flags->changes,flags->dirty,flags};
}

BnwNativeContinueMissionEmptyHeadReturnObservation BnwNativeUnitAllocatorCore::advance_native_existing_continue_mission_empty_head_native_return(std::int32_t id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();
 const auto found=p->bank->continue_mission_entries.find(id);need(found!=p->bank->continue_mission_entries.end()&&found->second,"ContinueMission native empty return requires actual exit Flags journal");const auto entry=found->second;
 need(entry->phase==BnwUnitMissionContinuationPhase::empty_head_awake_exit_flags_stored_before_combat_warned&&!entry->combat_warned_attempt&&!p->continue_mission_game_attempts.contains(id),"ContinueMission actual Game return attempt cannot replay/replace an issued owner");
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);auto&path=paths.paths_.at({p->actor,id});const auto flags=entry->flags;
 const auto current=[&]{p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);need(!tree->busy_&&!tree->callbacks_&&entry->paths==&paths&&entry->path==&path&&path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"ContinueMission Game return actual phase/path/world changed");};
 current();
 // Reserve the ONE real attempt before Game acquisition providers. The weak
 // Bank marker survives issuer expiry and cannot authorize a fresh retry.
 auto attempt=std::make_shared<ContinueMissionCombatWarnedAttempt>();attempt->core=factory_self_;attempt->bank=p->bank;attempt->entry=entry;attempt->actor=p->actor;attempt->id=id;attempt->generation=p->generation;
 need(p->continue_mission_game_attempts.emplace(id,attempt).second,"ContinueMission actual Game attempt allocation duplicate");entry->combat_warned_attempt=std::weak_ptr<void>{attempt};
 const auto attempt_current=[&]{current();need(attempt->creator==std::this_thread::get_id()&&attempt->bank.lock().get()==p->bank.get()&&attempt->entry.lock().get()==entry.get()&&attempt->actor==p->actor&&attempt->id==id&&attempt->generation==p->generation&&p->continue_mission_game_attempts.at(id).get()==attempt.get()&&entry->combat_warned_attempt&&!entry->combat_warned_attempt->expired()&&!entry->combat_warned_attempt->owner_before(attempt)&&!attempt.owner_before(*entry->combat_warned_attempt)&&!attempt->core.expired()&&!attempt->core.owner_before(factory_self_)&&!factory_self_.owner_before(attempt->core),"ContinueMission Game attempt SAME Core/private issuer changed");};
 try{
 attempt->context=BnwLiveGameContext::retain_for_actual_host(p->host);attempt_current();
 attempt->reference=attempt->context->get_game();attempt->reference->require_actual_binding(*p->host);attempt_current();
 try{attempt->receipt=attempt->reference->set_combat_warned_false();}
 catch(...){attempt->receipt=attempt->reference->issued_combat_warned_receipt_without_callbacks();throw;}
 // This is the actual unconditional native bool store, including equalfalse.
 // An allocated diagnosis alone never earns this return; the private owner
 // checks its SAME actual reference, member, sequence and stored allocation.
 attempt->reference->require_combat_warned_return(attempt->receipt);attempt_current();
 attempt->reference->destroy();attempt->reference->require_destroyed_for(*p->host);attempt_current();
 attempt->reference->require_combat_warned_return(attempt->receipt,true);
 // All provider checks precede the last pure physical phase/source seal.
 p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);
 need(!tree->busy_&&!tree->callbacks_&&entry->paths==&paths&&entry->path==&path&&path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"ContinueMission actual empty return lost its final physical source");
 attempt->native_return_earned=true;entry->phase=BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed;
 return {p->actor,id,flags->expected,path.clear_operations,flags->changes,attempt->receipt->ordinal(),flags->dirty};
 }catch(...){
  // Release acquired lock/ref1 immediately on the creator by actual wrapper
  // RAII, even if a currentness provider would reject explicit Destroy. The
  // authentic issued/stored receipt survives solely as error history; its
  // saved reference address is never dereferenced or reused as authority.
  if(attempt->reference){if(!attempt->receipt)attempt->receipt=attempt->reference->issued_combat_warned_receipt_without_callbacks();attempt->reference.reset();}
  throw;
 }
}
std::shared_ptr<const BnwUnitMissionContinuationCheckpoint>BnwNativeUnitAllocatorCore::checkpoint_native_existing_continue_mission(std::int32_t id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();const auto found=p->bank->continue_mission_entries.find(id);need(found!=p->bank->continue_mission_entries.end()&&found->second,"Mission checkpoint requires actual finite pending entry");const auto entry=found->second;need(!entry->combat_warned_attempt||entry->phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed,"Mission checkpoint cannot export stale pre-Game phase after actual reserved/stored return");
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const auto&path=paths.paths_.at({p->actor,id});
 const auto current=[&]{p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);need(!tree->busy_&&!tree->callbacks_&&entry->paths==&paths&&entry->path==&path&&path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"Mission checkpoint actual current node/path/world changed");};current();
 BnwUnitMissionContinuationCheckpoint::Record r;r.identity=p->host->identity();const auto&map=p->state->world.map_header;r.map_guid=map.guid;r.width=map.grid.width;r.height=map.grid.height;r.wrap_x=map.wrap_x;r.wrap_y=map.wrap_y;r.actor=p->actor;r.full_id=id;r.slot=id&8191;r.game_turn=entry->game_turn;r.steps=entry->steps;r.eta=entry->eta;r.core_begin=entry->unit->core.start_offset;r.core_end=entry->unit->core.end_offset;r.tail_begin=entry->unit->tail.start_offset;r.tail_end=entry->unit->tail.end_offset;r.before_world_revision=entry->before_world_revision;r.current_world_revision=entry->world_revision;r.map_rng=entry->map_rng;r.game_rng=entry->game_rng;r.phase=entry->phase;r.flags_loaded=entry->flags->loaded;r.flags_before=entry->flags->loaded;r.flags_current=entry->flags->expected;r.native_archive_index=entry->flags->native_index;r.changes=entry->flags->changes;r.dirty=entry->flags->dirty;r.before_path=entry->before_path;r.current_path=entry->current_path;
 if(entry->phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed){
  const auto found_attempt=p->continue_mission_game_attempts.find(id);need(found_attempt!=p->continue_mission_game_attempts.end()&&found_attempt->second,"Completed mission checkpoint actual Game owner absent");const auto attempt=found_attempt->second;
  const auto earned=[&]{need(attempt->creator==p->thread&&attempt->native_return_earned&&attempt->reference&&attempt->receipt&&attempt->receipt->was_stored()&&attempt->bank.lock().get()==p->bank.get()&&attempt->entry.lock().get()==entry.get()&&attempt->actor==p->actor&&attempt->id==id&&attempt->generation==p->generation&&entry->combat_warned_attempt&&!entry->combat_warned_attempt->expired()&&!entry->combat_warned_attempt->owner_before(attempt)&&!attempt.owner_before(*entry->combat_warned_attempt)&&!attempt->core.expired()&&!attempt->core.owner_before(factory_self_)&&!factory_self_.owner_before(attempt->core),"Completed mission checkpoint unearned/retired Game lineage");};
  earned();attempt->reference->require_destroyed_for(*p->host);const auto snapshot=attempt->reference->snapshot_combat_warned_return(attempt->receipt,true);current();earned();
  // Snapshot is authenticated by the Game owner; decoder fields never create
  // a receipt. Global ordinal staleness is rejected by this final SAME owner.
  const auto final=attempt->reference->snapshot_combat_warned_return(attempt->receipt,true);need(final.before==snapshot.before&&final.after==snapshot.after&&final.before_ordinal==snapshot.before_ordinal&&final.ordinal==snapshot.ordinal&&final.restored_history==snapshot.restored_history,"Completed mission Game journal changed during capture");
  p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);earned();need(path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"Completed mission checkpoint lost final physical seal");
  r.combat_warned_before=final.before;r.combat_warned_after=final.after;r.combat_warned_before_ordinal=final.before_ordinal;r.combat_warned_ordinal=final.ordinal;
 }
 auto result=std::shared_ptr<const BnwUnitMissionContinuationCheckpoint>(new BnwUnitMissionContinuationCheckpoint(r));current();need(BnwUnitPathOwner::project_unit_flags_revision(*p->state,p->host->identity(),p->host->database(),{p->actor,id},r.flags_before)==r.before_world_revision,"Mission checkpoint before/world historical seal differs");current();
 if(entry->phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed){const auto attempt=p->continue_mission_game_attempts.at(id);need(attempt&&attempt->native_return_earned&&attempt->reference&&attempt->receipt,"Mission checkpoint final completed issuer retired");const auto final=attempt->reference->snapshot_combat_warned_return(attempt->receipt,true);need(final.before==r.combat_warned_before&&final.after==r.combat_warned_after&&final.before_ordinal==r.combat_warned_before_ordinal&&final.ordinal==r.combat_warned_ordinal,"Mission checkpoint final global Game ordinal moved");p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);need(path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"Mission checkpoint final physical source changed");}
 return result;
}
BnwNativeContinueMissionPhaseObservation BnwNativeUnitAllocatorCore::restore_native_existing_continue_mission(const saves::CivisSaveSection&section)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();(void)issuer->current();auto r=BnwUnitMissionContinuationCheckpoint::decode(section,p->host->identity());need(r.actor==p->actor,"Mission checkpoint cannot restore foreign actor through this Core");
 if(r.phase==BnwUnitMissionContinuationPhase::empty_head_awake_native_return_completed)return restore_native_existing_continue_mission_completed(section);
 if(const auto*registry=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:registry->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty()&&bank->existing_path_to.empty()&&bank->continue_mission_entries.empty(),"Mission checkpoint restore cannot overlap actual Init/path/mission effects");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,r.full_id);need(unit&&narrow(core(*unit,"m_iID"))==r.full_id&&unit->core.start_offset==r.core_begin&&unit->core.end_offset==r.core_end&&unit->tail.start_offset==r.tail_begin&&unit->tail.end_offset==r.tail_end,"Mission checkpoint fullID/physical source interval differs");need(p->bank->imported_flags_fields.empty(),"Mission checkpoint original Flags registrar already used");
 if(r.phase==BnwUnitMissionContinuationPhase::empty_head_awake_exit_flags_stored_before_combat_warned)need(native_imported_mission_rows(*unit).empty()&&narrow(core(*unit,"m_eActivityType"))==0,"Mission exit checkpoint requires actual NULL head/equal AWAKE source");
 const auto runtime=p->bank->imported_mission_runtime.find(r.full_id);need(runtime!=p->bank->imported_mission_runtime.end()&&runtime->second,"Mission checkpoint original Read plain owner absent");const auto node=runtime->second;need(node->creator==std::this_thread::get_id()&&node->owner.lock().get()==p->bank.get()&&node->unit==unit&&node->actor==p->actor&&node->id==r.full_id&&node->generation==p->generation&&node->mission_timer==0&&node->combat_city_owner==-1&&node->combat_city_id==-1&&node->visual_locations.empty(),"Mission checkpoint original runtime changed");
 const auto&map=p->state->world.map_header;need(map.guid==r.map_guid&&map.grid.width==r.width&&map.grid.height==r.height&&map.wrap_x==r.wrap_x&&map.wrap_y==r.wrap_y&&p->source->snapshot.map_header.guid==r.map_guid,"Mission checkpoint map source/domain differs");need(p->state->world.pregame&&p->state->world.pregame->archive.integers.at("game_turn")==r.game_turn&&p->state->setup.game_turn==r.game_turn&&p->state->random.map.algorithm()==RandomAlgorithm::bnw&&p->state->random.gameplay.algorithm()==RandomAlgorithm::bnw&&p->state->random.map.state()==r.map_rng&&p->state->random.gameplay.state()==r.game_rng,"Mission checkpoint clock/RNG must be restored coherently first");
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);auto&path=paths.paths_.at({p->actor,r.full_id});for(const auto&[key,value]:paths.paths_){(void)key;need(value==BnwUnitRetainedPath{},"Mission checkpoint needs pristine fresh OriginalRead retained paths");}need(!tree->busy_&&!tree->callbacks_&&!tree->best_&&!tree->open_&&!tree->tail_&&!tree->closed_&&!tree->control_->data_identity(),"Mission checkpoint common finder must be actual unused original initialization");
 const auto flags=make_imported_flags_archive(*p->state,*p->source,p->bank,p->actor,r.full_id,p->generation,true,p->host->identity());need(flags->loaded==r.flags_loaded&&flags->native_index==r.native_archive_index&&(flags->expected==r.flags_before||flags->expected==r.flags_current),"Mission checkpoint original Flags baseline/slot/raw overlay differs");
 need(BnwUnitPathOwner::project_unit_flags_revision(*p->state,p->host->identity(),p->host->database(),{p->actor,r.full_id},r.flags_before)==r.before_world_revision&&BnwUnitPathOwner::project_unit_flags_revision(*p->state,p->host->identity(),p->host->database(),{p->actor,r.full_id},r.flags_current)==r.current_world_revision,"Mission checkpoint normalized before/current unit world differs");const auto actual_before=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());need(actual_before==(flags->expected==r.flags_before?r.before_world_revision:r.current_world_revision),"Mission checkpoint actual raw flag/world pairing differs");
 auto entry=std::make_shared<ExistingContinueMissionEntry>();entry->owner=p->bank;entry->unit=unit;entry->runtime=node;entry->flags=flags;entry->actor=p->actor;entry->id=r.full_id;entry->generation=p->generation;entry->before_world_revision=r.before_world_revision;entry->world_revision=r.current_world_revision;entry->paths=&paths;entry->path=&path;entry->before_path=std::move(r.before_path);entry->current_path=r.current_path;entry->game_turn=r.game_turn;entry->steps=r.steps;entry->eta=r.eta;entry->map_rng=r.map_rng;entry->game_rng=r.game_rng;entry->phase=r.phase;entry->cache_cleared=true;
 auto staged_entries=p->bank->continue_mission_entries;auto staged_flags=p->bank->imported_flags_fields;need(staged_entries.emplace(r.full_id,entry).second&&staged_flags.emplace(r.full_id,flags).second,"Mission checkpoint staged physical registration replay");
 p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);need(native_imported_get_at(*p->state,p->actor,r.full_id)==unit&&path==BnwUnitRetainedPath{}&&p->bank->continue_mission_entries.empty()&&p->bank->imported_flags_fields.empty()&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==actual_before&&narrow(core(*unit,"m_iFlags"))==flags->expected&&p->state->random.map.state()==r.map_rng&&p->state->random.gameplay.state()==r.game_rng,"Mission checkpoint final precommit source changed");
 const saves::UnitCoreField*actual_flags=nullptr;for(const auto&f:unit->core.fields)if(name(f.name)=="m_iFlags"){need(!actual_flags,"Mission restore final Flags duplicate");actual_flags=&f;}
 need(actual_flags==flags->field&&actual_flags&&actual_flags->encoding==saves::UnitCoreEncoding::i32&&actual_flags->values.size()==1&&&actual_flags->values.front()==flags->value&&narrow(actual_flags->values.front())==flags->expected,"Mission restore final physical alias changed");
 need(p->bank->imported_mission_runtime.at(r.full_id).get()==node.get()&&node->owner.lock().get()==p->bank.get()&&node->unit==unit&&node->actor==p->actor&&node->id==r.full_id&&node->generation==p->generation&&node->mission_timer==0&&node->combat_city_owner==-1&&node->combat_city_id==-1&&node->visual_locations.empty(),"Mission restore final actual plain runtime changed");
 need(map.guid==r.map_guid&&map.grid.width==r.width&&map.grid.height==r.height&&map.wrap_x==r.wrap_x&&map.wrap_y==r.wrap_y&&p->state->world.pregame->archive.integers.at("game_turn")==r.game_turn&&p->state->setup.game_turn==r.game_turn&&p->state->random.map.algorithm()==RandomAlgorithm::bnw&&p->state->random.gameplay.algorithm()==RandomAlgorithm::bnw,"Mission restore final clock/map/RNG algorithm changed");
 for(const auto&[key,value]:paths.paths_){(void)key;need(value==BnwUnitRetainedPath{},"Mission restore final other retained path changed");}need(!tree->busy_&&!tree->callbacks_&&!tree->best_&&!tree->open_&&!tree->tail_&&!tree->closed_&&!tree->control_->data_identity(),"Mission restore final original tree changed");
 if(const auto*registry=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:registry->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty()&&bank->existing_path_to.empty()&&bank->continue_mission_entries.empty(),"Mission restore final other body appeared");}
 // Snapshot installation, NOT re-execution of original ClearPathCache or
 // evaluating assignment. All allocations, decode, source reads precede this
 // no-throw commit. The fresh runtime/generation belong to this actual Core.
 *flags->value=r.flags_current;flags->expected=r.flags_current;flags->changes=r.changes;flags->dirty=r.dirty;
 path.nodes.swap(r.current_path.nodes);path.destination=r.current_path.destination;path.clear_operations=r.current_path.clear_operations;path.generation_operations=r.current_path.generation_operations;path.path_to_reject_clear_operations=r.current_path.path_to_reject_clear_operations;
 p->bank->imported_flags_fields.swap(staged_flags);p->bank->continue_mission_entries.swap(staged_entries);
 return {p->actor,r.full_id,flags->expected,flags->loaded,entry->phase,path.clear_operations,flags->changes,flags->dirty,flags};
}


BnwNativeContinueMissionPhaseObservation BnwNativeUnitAllocatorCore::restore_native_existing_continue_mission_completed(const saves::CivisSaveSection&section)const{
 // Called only inside the existing actual Core Guard. The closed Plan earns
 // the pristine source/defaults before Game is allowed to prepare an installer.
 BnwUnitCombatWarnedRestorePlan plan(*this,section);const auto p=impl_;auto&q=*plan.storage_;const auto&r=q.record;const auto flags=q.flags;
 auto entry=std::make_shared<ExistingContinueMissionEntry>();entry->owner=p->bank;entry->unit=q.unit;entry->runtime=q.runtime;entry->flags=flags;entry->actor=p->actor;entry->id=r.full_id;entry->generation=p->generation;entry->before_world_revision=r.before_world_revision;entry->world_revision=r.current_world_revision;entry->paths=q.paths;entry->path=q.path;entry->before_path=r.before_path;entry->current_path=r.current_path;entry->game_turn=r.game_turn;entry->steps=r.steps;entry->eta=r.eta;entry->map_rng=r.map_rng;entry->game_rng=r.game_rng;entry->phase=r.phase;entry->cache_cleared=true;
 auto staged_entries=p->bank->continue_mission_entries;auto staged_flags=p->bank->imported_flags_fields;need(staged_entries.emplace(r.full_id,entry).second&&staged_flags.emplace(r.full_id,flags).second,"Completed mission staged registration replay");
 auto attempt=std::make_shared<ContinueMissionCombatWarnedAttempt>();attempt->core=factory_self_;attempt->bank=p->bank;attempt->entry=entry;attempt->actor=p->actor;attempt->id=r.full_id;attempt->generation=p->generation;attempt->restored_checkpoint=section;attempt->restored_tree=q.tree;
 need(p->continue_mission_game_attempts.emplace(r.full_id,attempt).second,"Completed mission actual restore attempt already reserved");q.attempt=attempt;entry->combat_warned_attempt=std::weak_ptr<void>{attempt};
 auto installed=r.current_path;
 try{
  plan.require_current_without_callbacks();attempt->context=BnwLiveGameContext::retain_for_actual_host(p->host);p->check();plan.require_current_without_callbacks();
  attempt->reference=attempt->context->get_game();attempt->reference->require_actual_binding(*p->host);p->check();plan.require_current_without_callbacks();
  attempt->receipt=attempt->reference->prepare_restore_completed_combat_warned(plan);p->check();plan.require_current_without_callbacks();
  // Every allocation, callback and source query precedes this last guard.
  // Game installs restored HISTORY, not a rerun of the SDK false setter.
  // All following world/archive/container swaps are actual noexcept stores.
  attempt->reference->install_restore_completed_combat_warned(plan,attempt->receipt);
  *flags->value=r.flags_current;flags->expected=r.flags_current;flags->changes=r.changes;flags->dirty=r.dirty;
  auto&path=*q.path;
  path.nodes.swap(installed.nodes);path.destination=installed.destination;path.clear_operations=installed.clear_operations;path.generation_operations=installed.generation_operations;path.path_to_reject_clear_operations=installed.path_to_reject_clear_operations;
  p->bank->imported_flags_fields.swap(staged_flags);p->bank->continue_mission_entries.swap(staged_entries);plan.retire_without_callbacks();
  const auto current=[&]{p->check();q.issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(*q.paths,*q.tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);need(!q.tree->busy_&&!q.tree->callbacks_&&entry->path==q.path&&path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision&&p->continue_mission_game_attempts.at(r.full_id).get()==attempt.get()&&attempt->entry.lock().get()==entry.get(),"Completed restore actual installed journal/world changed");};
  attempt->reference->require_combat_warned_return(attempt->receipt);current();attempt->reference->destroy();attempt->reference->require_destroyed_for(*p->host);current();const auto actual=attempt->reference->snapshot_combat_warned_return(attempt->receipt,true);
  need(actual.restored_history&&actual.before==r.combat_warned_before&&!actual.after&&actual.before_ordinal==r.combat_warned_before_ordinal&&actual.ordinal==r.combat_warned_ordinal,"Completed restore genuine Game history differs");
  p->host->session().require_bnw_native_unit_paths_current_without_callbacks(*q.paths,*q.tree,*this);require_continue_entry_local(p->bank,*p->state,*entry,p->actor,p->generation);need(path==entry->current_path&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==entry->world_revision,"Completed restore final physical return lost");
  attempt->native_return_earned=true;return {p->actor,r.full_id,flags->expected,flags->loaded,entry->phase,path.clear_operations,flags->changes,flags->dirty,flags};
 }catch(...){
  plan.retire_without_callbacks();if(attempt->reference){if(!attempt->receipt)attempt->receipt=attempt->reference->issued_combat_warned_receipt_without_callbacks();attempt->reference.reset();}
  // A failed/reserved or stored-but-not-earned attempt remains unsupported.
  // It can neither retry the installer nor export a completed phase4 save.
  throw;
 }
}

bool BnwNativeUnitAllocatorCore::existing_native_is_in_combat(const saves::SavedUnitObservation&unit)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->bank&&p->bank->busy.load(),"Native combat read requires actual guarded Core body");
 const auto&s=*p->state;const auto actor=narrow(core(unit,"m_eOwner")),id=narrow(core(unit,"m_iID"));need(actor>=0&&actor<64&&native_imported_get_at(s,actor,id)==&unit,"Native combat read actual imported identity differs");
 // isFighting starts with actual getCombatUnit; valid native full ID short-
 // circuits the unsaved combatCity operand. Stale serialized ID is NULL.
 const auto combat_owner=native_friendly_tail(unit,"m_combatUnit.eOwner"),combat_id=native_friendly_tail(unit,"m_combatUnit.iID");
 if(combat_owner>=0&&combat_owner<64&&native_imported_get_at(s,combat_owner,combat_id))return true;
 const auto*registry=s.setup.extensions.find<Registry>();need(registry,"Native combat city actual runtime registry absent");const auto bank=registry->players.find(actor);need(bank!=registry->players.end()&&bank->second&&!bank->second->retired.load(),"Native combat city actor OriginalRead runtime owner mandatory");
 const auto node=bank->second->imported_mission_runtime.find(id);need(node!=bank->second->imported_mission_runtime.end()&&node->second,"Native combat city OriginalRead typed member unavailable");const auto&runtime=*node->second;
 need(runtime.creator==std::this_thread::get_id()&&runtime.owner.lock().get()==bank->second.get()&&runtime.unit==&unit&&runtime.actor==actor&&runtime.id==id&&runtime.generation==p->generation,"Native combat city actual SAME runtime source changed");
 if(runtime.combat_city_owner>=0&&runtime.combat_city_owner<64&&resolve_bnw_city_registry(s.world,s.cities,runtime.combat_city_owner,runtime.combat_city_id))return true;
 // isAttacking follows isFighting only on false: actual plotCheckInvalid
 // reads serialized attack coordinates before delayedDeath short-circuit.
 const auto x=narrow(core(unit,"m_iAttackPlotX")),y=narrow(core(unit,"m_iAttackPlotY"));
 if(x==-INT32_MAX||y==-INT32_MAX)return false;
 const auto&h=s.world.map_header;const auto attack=MapGeometry{h.grid,h.wrap_x,h.wrap_y,bnw}.plot(x,y);return attack&&!existing_path_bool(unit,"m_bDeathDelay");
}
std::optional<BnwNativeImportedMissionRuntimeObservation>BnwNativeUnitAllocatorCore::read_native_existing_mission_runtime(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Imported mission runtime requires completed imported rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 const auto id=narrow(core(*unit,"m_iID"));const auto i=p->bank->imported_mission_runtime.find(id);need(i!=p->bank->imported_mission_runtime.end()&&i->second,"Imported mission actual OriginalRead runtime unavailable");const auto node=i->second;
 const auto before=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());
 const auto current=[&]{p->check();(void)issuer->current();need(p->bank->imported_mission_runtime.at(id).get()==node.get()&&node->creator==std::this_thread::get_id()&&node->owner.lock().get()==p->bank.get()&&node->unit==unit&&node->actor==p->actor&&node->id==id&&node->generation==p->generation&&native_imported_get_at(*p->state,p->actor,query_id)==unit&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==before,"Imported mission actual plain node/current source changed");};
 current();BnwNativeImportedMissionRuntimeObservation result;result.owner=p->actor;result.full_id=id;result.mission_timer=node->mission_timer;result.combat_city_owner=node->combat_city_owner;result.combat_city_id=node->combat_city_id;result.visual_locations=node->visual_locations.size();result.registered_lifetime=node;
 // Exact IsBusy short-circuit reads the actual unsaved timer FIRST.
 result.busy=node->mission_timer>0||existing_native_is_in_combat(*unit);current();return result;
}

std::optional<BnwNativeImportedMissionObservation>BnwNativeUnitAllocatorCore::read_native_existing_mission_queue(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Imported mission read requires completed rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 const auto rows=native_imported_mission_rows(*unit);BnwNativeImportedMissionObservation result;result.owner=p->actor;result.full_id=narrow(core(*unit,"m_iID"));result.unit_flags=narrow(core(*unit,"m_iFlags"));result.activity=narrow(core(*unit,"m_eActivityType"));result.queue.reserve(rows.size());
 for(const auto&m:rows)result.queue.push_back({m.type,m.data1,m.data2,m.flags,m.push_turn});
 p->check();(void)issuer->current();need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&native_imported_mission_rows(*unit)==rows&&narrow(core(*unit,"m_iFlags"))==result.unit_flags&&narrow(core(*unit,"m_eActivityType"))==result.activity,"Imported mission current queue/physical node changed");return result;
}
std::optional<bool>BnwNativeUnitAllocatorCore::read_native_existing_ready_to_auto(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"ReadyToAuto requires completed imported rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 // Source short circuit: actual canMove() is getMoves()>0. With zero moves
 // no queue/mission Type/body/UI query is executed.
 const auto moves=narrow(core(*unit,"m_iMoves"));bool result=false;std::vector<NativeInitialMission>rows;
 if(moves>0){rows=native_imported_mission_rows(*unit);result=!rows.empty();}
 p->check();(void)issuer->current();need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&narrow(core(*unit,"m_iMoves"))==moves,"ReadyToAuto actual GetAt/moves changed");if(moves>0)need(native_imported_mission_rows(*unit)==rows,"ReadyToAuto native queue changed");return result;
}
std::optional<bool>BnwNativeUnitAllocatorCore::read_native_existing_is_doing_partial_move(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Partial move query requires completed imported rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 const auto rows=native_imported_mission_rows(*unit);if(rows.empty()){p->check();(void)issuer->current();return false;}
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);const auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const BnwUnitOrderKey key{p->actor,narrow(core(*unit,"m_iID"))};const auto&path=paths.paths_.at(key);
 if(path.nodes.empty()){p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);return false;}
 const auto snapshot=path;const auto revision=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());
 const auto&h=p->state->world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};const auto origin=geometry.plot(narrow(core(*unit,"m_iX")),narrow(core(*unit,"m_iY")));
 const auto&last=path.nodes.front();const auto end_path=geometry.plot(last.x,last.y);std::optional<std::uint64_t>end_turn;
 // Source GetPathEndTurnPlot destination-first scan (different from
 // CvPathNodeArray::GetTurnDest reverse walk used by UpdatePathCache).
 if(path.nodes.size()==1||last.data2==1)end_turn=end_path;
 else for(std::size_t i=1;i<path.nodes.size();++i)if(path.nodes[i].data2==1){end_turn=geometry.plot(path.nodes[i].x,path.nodes[i].y);break;}
 bool result=false;
 if(origin==end_turn&&origin!=end_path){
  need(end_path.has_value(),"Partial move reached native end-path NULL dereference");
  const auto xy=h.grid.coordinates(*end_path);const auto&head=rows.front();
  if(head.data1==static_cast<std::int64_t>(xy[0])&&head.data2==static_cast<std::int64_t>(xy[1])){
   const auto moves=narrow(core(*unit,"m_iMoves"));const auto quote=quote_bnw_unit_movement(*p->state,p->host->database(),p->host->types(),p->host->identity(),key);
   need(quote.max_moves.has_value(),quote.blocker.empty()?"Partial move actual maxMoves unavailable":quote.blocker.c_str());result=moves<*quote.max_moves&&moves>0;
  }
 }
 p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
 need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&native_imported_mission_rows(*unit)==rows&&path==snapshot&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==revision,"Partial move actual queue/path/source changed");return result;
}

std::optional<std::int32_t>BnwNativeUnitAllocatorCore::read_native_existing_retained_step_cost(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);
 // Registration runs before this call's own invocation scope, using its
 // genuine original-Read registrar. It supplies no numeric move permission.
 const auto registered=bind_native_existing_moves_archive(query_id);if(!registered)return std::nullopt;
 const auto p=impl_;Impl::Guard invocation(p);p->check();const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);need(unit,"Retained cost actual registered node disappeared");
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);const auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const BnwUnitOrderKey key{p->actor,narrow(core(*unit,"m_iID"))};const auto&path=paths.paths_.at(key);
 if(path.nodes.size()<2){p->check();(void)issuer->current();return std::nullopt;}
 const auto cache=prepare_existing_unit_path_cache(*unit);ExistingPathMovementCostDispatch dispatch(*this,tree,*unit,paths,path,cache);
 const auto result=BnwNativeUnitPathMovementCost::evaluate(*this,dispatch);(void)existing_path_movement_cost_operands(dispatch);p->check();(void)issuer->current();return result;
}

std::optional<BnwNativeImportedMovesArchiveObservation>BnwNativeUnitAllocatorCore::bind_native_existing_moves_archive(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Imported moves registration requires completed imported rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 const auto id=narrow(core(*unit,"m_iID"));auto&fields=p->bank->imported_moves_fields;
 auto existing=fields.find(id);std::shared_ptr<ImportedMovesArchive>binding;
 if(existing==fields.end()){
  need(p->source->source.container==saves::SaveContainerKind::civ5&&p->source->snapshot.all_players&&p->source->snapshot.all_players->layout==bnw,"Imported moves original Read lineage unavailable");
  const auto&original=p->source->snapshot.all_players->players.at(p->actor);need(original.saved_player_id==p->actor,"Imported moves source player identity differs");
  const saves::SavedUnitObservation*loaded=nullptr;
  for(const auto&u:original.units.units)if(narrow(core(u,"m_iID"))==id){need(!loaded,"Duplicate original imported moves full ID");loaded=&u;}
  need(loaded&&core(*loaded,"m_eOwner")==p->actor&&loaded->core.start_offset==unit->core.start_offset&&loaded->core.end_offset==unit->core.end_offset,"Imported moves original physical object lineage differs");
  const auto value=narrow(core(*unit,"m_iMoves"));need(value==narrow(core(*loaded,"m_iMoves")),"Imported moves cannot invent original Load baseline after an unregistered mutation");
  saves::UnitCoreField*field=nullptr;
  // This is the actual mutable SAME-session world object, not a copied source.
  auto&actual=p->state->world.all_players->players.at(p->actor).units.units;
  saves::SavedUnitObservation*mutable_unit=nullptr;for(auto&u:actual)if(&u==unit)mutable_unit=&u;
  need(mutable_unit,"Imported moves mutable canonical object absent");
  for(auto&f:mutable_unit->core.fields)if(name(f.name)=="m_iMoves"){need(!field,"Duplicate imported moves field");field=&f;}
  need(field&&field->encoding==saves::UnitCoreEncoding::i32&&field->values.size()==1&&field->bytes.empty(),"Imported moves actual typed i32 storage unavailable");
  std::optional<std::size_t>native_index;for(std::size_t i=0;i<std::size(archive_specs);++i)if(name(archive_specs[i].member)=="m_iMoves"){need(!native_index&&name(archive_specs[i].name)=="CvUnit::m_iMoves"&&archive_specs[i].kind==ArchiveKind::scalar,"Imported moves native constructor binding differs");native_index=i;}
  need(native_index.has_value(),"Imported moves native archive slot unavailable");
  binding=std::make_shared<ImportedMovesArchive>();binding->owner=p->bank;binding->unit=unit;binding->field=field;binding->value=&field->values.front();
  binding->actor=p->actor;binding->id=id;binding->loaded=value;binding->expected=value;binding->generation=p->generation;binding->native_index=*native_index;
  // Native original Read resets then Load writes this scalar without touch.
  // Only this field's known clean Load baseline is issued. Other fields and
  // future loaded checkpoint/history remain mandatory independent owners.
  p->check();(void)issuer->current();need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&narrow(core(*unit,"m_iMoves"))==value,"Imported moves source changed before real registration");
  need(fields.emplace(id,binding).second,"Imported moves canonical registration replay changed");
 }else binding=existing->second;
 need(binding&&binding->creator==std::this_thread::get_id(),"Imported moves foreign registered scalar owner");
 const auto registered=binding->owner.lock();need(registered.get()==p->bank.get()&&binding->unit==unit&&binding->actor==p->actor&&binding->id==id&&binding->generation==p->generation,"Imported moves registered source/Bank/fullID changed");
 const saves::UnitCoreField*field=nullptr;for(const auto&f:unit->core.fields)if(name(f.name)=="m_iMoves"){need(!field,"Duplicate current imported moves scalar");field=&f;}
 need(field==binding->field&&field&&field->encoding==saves::UnitCoreEncoding::i32&&field->values.size()==1&&&field->values.front()==binding->value&&narrow(field->values.front())==binding->expected,"Imported moves physical field alias/earned scalar lineage changed");
 BnwNativeImportedMovesArchiveObservation result;result.owner=p->actor;result.full_id=id;result.value=binding->expected;result.loaded_value=binding->loaded;result.native_archive_index=binding->native_index;result.binding_name="CvUnit::m_iMoves";result.binding_member="m_iMoves";result.dirty=binding->dirty;result.changes=binding->changes;result.registered_lifetime=binding;
 p->check();(void)issuer->current();need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&field==binding->field&&narrow(core(*unit,"m_iMoves"))==binding->expected,"Imported moves actual registration return changed");return result;
}

#include "bnw_imported_damage_archive.inc"

std::optional<std::uint64_t>BnwNativeUnitAllocatorCore::read_native_existing_path_first_plot(std::int32_t query_id)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Native GetPathFirstPlot requires completed imported rosters");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);
 const auto&paths=p->host->session().bnw_native_unit_paths_for_core(*this,*tree);const BnwUnitOrderKey key{p->actor,narrow(core(*unit,"m_iID"))};
 const auto&path=paths.paths_.at(key);std::optional<std::uint64_t>result;
 if(!path.nodes.empty()){const auto&node=path.nodes.at(path.nodes.size()==1?0:path.nodes.size()-2);const auto&h=p->state->world.map_header;result=MapGeometry{h.grid,h.wrap_x,h.wrap_y,bnw}.plot(node.x,node.y);}
 p->check();(void)issuer->current();p->host->session().require_bnw_native_unit_paths_current_without_callbacks(paths,*tree,*this);
 need(native_imported_get_at(*p->state,p->actor,query_id)==unit,"Native GetPathFirstPlot actual GetAt changed");return result;
}

std::optional<bool>BnwNativeUnitAllocatorCore::advance_native_existing_update_path_cache(std::int32_t query_id,std::uint64_t target)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 // Each actual read/search owns its own invocation guard. No copied caller
 // predicates or re-entrant generate call may fabricate search admission.
 const saves::SavedUnitObservation*unit=nullptr;std::string revision;std::int32_t flags=0,team=-1;
 std::optional<BnwUnitRetainedPath>cached;
 {
  Impl::Guard invocation(p);p->check();
  if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Native UpdatePathCache requires completed imported rosters");}
  unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
  need(target<p->state->world.map_header.grid.size(),"Native UpdatePathCache actual destination NULL");
  flags=narrow(core(*unit,"m_iFlags"));team=post_player_team(*p->state,p->actor);
  revision=bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database());
  p->check();(void)issuer->current();
 }
 const auto current=[&]{Impl::Guard invocation(p);(void)issuer->current();p->check();need(native_imported_get_at(*p->state,p->actor,query_id)==unit&&narrow(core(*unit,"m_iFlags"))==flags&&post_player_team(*p->state,p->actor)==team&&bnw_unit_order_world_revision(*p->state,p->host->identity(),p->host->database())==revision,"Native UpdatePathCache actual source/GetAt/mission flag changed");};
 const auto generated=[&](std::uint64_t destination){current();const auto result=generate_native_existing_unit_path(query_id,destination,false,ExistingPathConsumer::native_retained_cache);current();need(result.has_value(),"Native UpdatePathCache imported node disappeared during generation");return result->found;};
 // SDK ordinary mode always performs GeneratePath, even with a previously
 // successful cache. The 0x1 operand comes from actual serialized m_iFlags.
 if((flags&1)==0)return generated(target);
 cached=read_native_existing_retained_path_cache(query_id);current();need(cached.has_value(),"Native UpdatePathCache actual path owner disappeared");
 const auto path_current=[&]{current();need(read_native_existing_retained_path_cache(query_id)==cached,"Native UpdatePathCache retained path changed before branch read");current();};
 const auto plot=[&](const BnwUnitRetainedPathNode&node){const auto&h=p->state->world.map_header;return MapGeometry{h.grid,h.wrap_x,h.wrap_y,bnw}.plot(node.x,node.y);};
 if(cached->nodes.size()>=2){
  path_current();const auto&dest=cached->nodes.front();
  if((dest.flags&1U)!=0){const auto old_destination=plot(dest);path_current();
   if(old_destination&&bnw_plot_is_visible(*p->state,*old_destination,team)){
    path_current();const auto result=generated(*old_destination);return !result&&target!=*old_destination?generated(target):result;
   }
  }
  path_current();const auto&next=cached->nodes[cached->nodes.size()-2];
  if((next.flags&1U)!=0){
   // CvPathNodeArray::GetTurnDest(1): reverse walk, return the current
   // turn's last node only at an actual turn boundary (or destination).
   const BnwUnitRetainedPathNode*turn=nullptr;
   for(std::size_t i=cached->nodes.size();i--;){const auto&node=cached->nodes[i];if(i==0){if(node.data2==1)turn=&node;break;}if(node.data2==1&&cached->nodes[i-1].data2>1){turn=&node;break;}}
   path_current();if(!turn)return generated(target);
   const auto turn_destination=plot(*turn);path_current();if(!turn_destination)return false;
   const auto result=generated(*turn_destination);return !result&&target!=*turn_destination?generated(target):result;
  }
  path_current();return true;
 }
 path_current();if(cached->destination!=target)return generated(target);
 // Native evaluating-mission already-attempted cache returns false. No
 // synthetic success from target equality or an empty retained cache.
 path_current();return false;
}

std::optional<BnwNativeExistingUnitPathObservation>BnwNativeUnitAllocatorCore::read_native_existing_unit_path(std::int32_t query_id,std::uint64_t target,bool reuse)const{
 return generate_native_existing_unit_path(query_id,target,reuse,ExistingPathConsumer::observation);
}
std::optional<BnwNativeExistingUnitPathObservation>BnwNativeUnitAllocatorCore::advance_native_existing_generate_path_cache(std::int32_t query_id,std::uint64_t target,bool reuse)const{
 return generate_native_existing_unit_path(query_id,target,reuse,ExistingPathConsumer::native_retained_cache);
}
bool BnwNativeUnitAllocatorCore::existing_native_can_move_into(const saves::SavedUnitObservation&source,std::uint64_t target,const ExistingUnitPathCache&cache,std::uint8_t flags)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 need(p&&p->thread==std::this_thread::get_id()&&p->bank&&p->bank->busy.load(),"Native common move admission requires guarded actual Core body");
 const auto&s=*p->state;const auto&h=s.world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};const auto*unit=&source;
 need(flags==0||flags==4||flags==128,"Native common movement flags require genuine attack/declare/embark profile");
 need(target<h.grid.size()&&narrow(core(source,"m_eOwner"))==p->actor,"Native common move actual owner/target outside source");
 const auto id=narrow(core(source,"m_iID")),sx=narrow(core(source,"m_iX")),sy=narrow(core(source,"m_iY"));const auto at=geometry.plot(sx,sy);need(at.has_value(),"Native common move source plot NULL");const auto origin=*at;
 const auto revision=bnw_unit_order_world_revision(s,p->host->identity(),p->host->database());const auto rng=s.random.gameplay.state(),map_rng=s.random.map.state();
 const auto current=[&]{(void)issuer->current();p->check();need(native_imported_get_at(s,p->actor,id)==unit&&bnw_unit_order_world_revision(s,p->host->identity(),p->host->database())==revision&&s.random.gameplay.state()==rng&&s.random.map.state()==map_rng,"Native common move actual GetAt/world/RNG changed during reads");};
 current();need(prepare_existing_unit_path_cache(source)==cache,"Native common move actual UnitPathInitialize operands differ");current();
 const auto require_cache=[&]{current();};
 const auto define=[&](const char*n){const auto*d=p->host->defines().find(n);need(p->host->defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),"Native path actual Define unavailable");return narrow(std::get<std::int64_t>(d->lookup.value));};
 const auto scalar=[&](const char*table,std::int32_t id,const char*column){const auto&e=initial_maintenance_info(*p->host,table,id);bool seen=false;std::int32_t value=0;
  const auto rows=p->host->database().query_rows({std::string("SELECT Type,")+column+" AS Operand FROM "+table+" WHERE ID=?",{id}},[&](const auto&r){need(!seen&&r.text("Type")==std::optional{e.type},"Native path actual Info Type differs");seen=true;value=narrow(r.integer("Operand"));});current();need(rows.all_succeeded()&&seen,"Native path actual Info operand unavailable");return value;};
 const auto kind=[&](std::uint64_t index){const auto&raw=native_path_plot(s,index);const auto v=narrow(post_named(raw.fields,"plot_type").value);need(v>=0&&v<4&&s.world.map_projection->cells.at(index).plot_type==std::optional<std::int64_t>{v},"Native path PlotType mirrors differ");return v;};
 const auto info_id=[&](std::uint64_t index,const char*table,const char*legacy,const char*hashed,std::int32_t version){const auto&raw=native_path_plot(s,index);need(raw.version.has_value(),"Native path plot version unavailable");
  if(*raw.version<static_cast<std::uint64_t>(version))return narrow(post_named(raw.fields,legacy).value);
  const auto hash=post_named(raw.fields,hashed).value;need(hash>=0&&hash<=UINT32_MAX,"Native path actual plot Info hash invalid");if(!hash)return std::int32_t{-1};const auto*t=p->host->types().find_hash(static_cast<std::uint32_t>(hash));need(t&&t->table==table&&t->id>=0&&t->hash==content::saved_info_type_hash(t->type),"Native path actual plot Info hash unresolved");return narrow(t->id);};
 const auto feature=[&](std::uint64_t i){return info_id(i,"Features","feature_type","feature_type_hash",3);};
 const auto terrain=[&](std::uint64_t i){const auto&raw=native_path_plot(s,i);const auto id=narrow(post_named(raw.fields,"terrain_type").value);need(s.world.map_projection->cells.at(i).terrain_type==std::optional<std::int64_t>{id},"Native path Terrain mirrors differ");return id;};
 const auto walk=[&](std::uint64_t i){const auto id=info_id(i,"Improvements","improvement_type","improvement_type_hash",5);if(id==-1)return false;const auto v=scalar("Improvements",id,"AllowsWalkWater");need(v==0||v==1,"Native path WalkWater outside bool");return v!=0;};
 const auto plot_team=[&](std::uint64_t i){const auto owner=narrow(post_named(native_path_plot(s,i).fields,"owner").value);need(owner>=-1&&owner<64&&s.world.map_projection->cells.at(i).owner==std::optional<std::int64_t>{owner},"Native path actual plot owner mirrors differ");return owner==-1?-1:post_player_team(s,owner);};
 const auto is_minor=[&](std::int32_t team){need(s.world.pregame&&s.world.pregame->layout==bnw,"Native path actual pregame unavailable");const auto&flags=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(flags.size()==64,"Native path actual Minor flags incomplete");for(std::int32_t i=22;i<64;++i){const auto&player=s.setup.players.at(i);if(player.alive&&player.team==team)return bool(flags.at(i));}return false;};
 const auto territory=[&](std::int32_t team){require_cache();if(team==-1||team==cache.team)return true;
  if(post_relation(s,native_path_combat_team(s,*unit,team,origin),team,"at_war"))return true;
  const auto type=native_friendly_info(*p->host,s,*unit,true).type;current();const auto rival=scalar("Units",type,"RivalTerritory");need(rival==0||rival==1,"Native path RivalTerritory outside bool");if(rival||narrow(core(*unit,"m_iRivalTerritoryCount"))>0)return true;
  if(is_minor(team)&&!is_minor(cache.team)){if(!post_relation(s,cache.team,team,"has_met"))return true;if(cache.human)return true;throw BnwNativeUnitAllocatorUnavailable("Native path AI Minor territory requires actual AngerFree/OpenBorders/Intruding owner");}
  return post_relation(s,team,cache.team,"open_borders");};
 const auto trait_bool=[&](const char*n){const auto&t=s.world.all_players->players.at(p->actor).traits;const auto&f=post_named(t.fields,n);need(t.layout==bnw&&f.encoding==saves::PlayerTraitsEncoding::boolean&&f.values.size()==1&&f.rows.empty()&&f.hashes.empty()&&!f.wire_count,"Native path actual trait Boolean absent");need(f.values[0]==0||f.values[0]==1,"Native path actual trait Boolean invalid");return f.values[0]!=0;};
 const auto indexed_impassable=[&](const char*n,const char*table,std::int32_t id){const auto&f=post_named(unit->core.fields,n);const auto span=native_init_info_span(*p->host,table);need(f.encoding==saves::UnitCoreEncoding::vector_i32&&f.wire_count&&*f.wire_count>=0&&static_cast<std::uint64_t>(*f.wire_count)==span&&f.values.size()==span&&id>=0&&static_cast<std::uint64_t>(id)<span,"Native path full indexed impassable cache unavailable");return narrow(f.values.at(id))>0;};
 const auto enter_terrain=[&](std::uint64_t i){require_cache();const bool all=narrow(core(*unit,"m_iCanMoveAllTerrainCount"))>0,hover=narrow(core(*unit,"m_iHoveringUnitCount"))>0;
  const auto&pass=bnw_live_plot_impassability(s);need(i<pass.plots.size(),"Native path restored impassability span incomplete");if(pass.plots.at(i).impassable&&narrow(core(*unit,"m_iCanMoveImpassableCount"))<=0&&!all)return false;
  if(kind(i)==0&&!(trait_bool("m_bCrossesMountainsAfterGreatGeneral")&&post_player(s,p->actor,"m_iGreatGeneralsCreated")>0)&&!hover&&!all)return false;
  if(cache.immobile)return false;const bool allows=walk(i);
  if(territory(plot_team(i))){const auto f=feature(i),route=narrow(post_named(native_path_plot(s,i).fields,"route_type").value);
   if(f!=-1&&route==-1){if(indexed_impassable("m_featureImpassableCount","Features",f))throw BnwNativeUnitAllocatorUnavailable("Native path feature passability requires actual promotion-tech/load owner");}
   else if(indexed_impassable("m_terrainImpassableCount","Terrains",terrain(i)))throw BnwNativeUnitAllocatorUnavailable("Native path terrain passability requires actual promotion-tech/load owner");
  }
  need(cache.domain==2,"Native common non-LAND terrain admission requires actual domain owner");if(allows)return true;
  if(kind(i)==3&&!all){
   if(flags&128U){if(cache.can_embark){if(terrain(i)==define("DEEP_WATER_TERRAIN")&&post_named(post_team(s,cache.team).fields,"embarked_all_water").value<=0&&native_friendly_tail(*unit,"m_iEmbarkedAllWaterCount")<=0)return false;}
    else if(hover){if(terrain(i)==define("DEEP_WATER_TERRAIN"))return false;}else return false;
   }else if(!native_path_city(s,i)){if(hover){if(terrain(i)==define("DEEP_WATER_TERRAIN"))return false;}
    else if(!cache.human||kind(origin)==3)return false;
    else throw BnwNativeUnitAllocatorUnavailable("Native common unembarked water requires actual canLoad owner");}
  }
  return true;};
 const auto friendly=[&](std::uint64_t i){const auto a=native_friendly_count(*p->host,s,*unit,i,true);current();const auto b=native_friendly_count(*p->host,s,*unit,i,true);need(a.inputs==b.inputs&&a.count==b.count,"Native path friendly source changed during rules reads");current();return a.count;};
 const auto revealed_team=[&](std::uint64_t i){const auto&a=post_named(native_path_plot(s,i).arrays,"revealed_owners");need(a.columns==1&&a.values.size()==80&&cache.team>=0&&cache.team<64,"Native path actual revealedOwner span incomplete");const auto owner=narrow(a.values.at(cache.team));need(owner>=-1&&owner<64,"Native path revealed owner outside native player domain");return owner==-1?-1:post_player_team(s,owner);};
 const auto move_into=[&](std::uint64_t i){require_cache();if(i==origin)return false;need(!cache.no_reveal,"Native path NoReveal admission requires actual willRevealByMove owner");if(cache.owner==63&&plot_team(i)!=-1)throw BnwNativeUnitAllocatorUnavailable("Native path barbarian owned destination requires actual release-turn owner");
  if(flags&4U){std::int32_t owner=-1;if(native_path_city(s,i,&owner)&&owner!=cache.owner)return false;const auto limit=define("PLOT_UNIT_LIMIT");if(limit>0&&bnw_plot_is_visible(s,i,cache.team)&&friendly(i)>=limit)return false;}
  if(narrow(core(*unit,"m_iNoCaptureCount"))>0&&native_path_enemy_city(s,*unit,i))return false;
  if(native_path_enemy_city(s,*unit,i))throw BnwNativeUnitAllocatorUnavailable("Native path enemy city admission requires actual Damage/GetMaxHitPoints owner");
  need(cache.domain!=1,"Native path AIR move admission unavailable");need(!cache.embarked,"Native path embarked nonattack admission unavailable");
  for(const auto&handle:native_path_stack(s,i)){if(handle.owner<0||handle.owner>=64)continue;const auto*other=native_imported_get_at(s,narrow(handle.owner),narrow(handle.id));if(other&&post_relation(s,cache.team,post_player_team(s,narrow(core(*other,"m_eOwner"))),"at_war")){
   const auto max_hp=define("MAX_HIT_POINTS");if(narrow(core(*other,"m_iDamage"))<max_hp&&existing_native_is_in_combat(*other)){
    const auto owner=native_friendly_tail(*other,"m_combatUnit.eOwner"),id=native_friendly_tail(*other,"m_combatUnit.iID");const auto*combat=owner>=0&&owner<64?native_imported_get_at(s,owner,id):nullptr;if(combat!=unit)return false;
   }break;
  }}
  if(!cache.human||bnw_plot_is_visible(s,i,cache.team)){if(native_path_enemy_city(s,*unit,i)||native_path_visible_enemy(s,*unit,i))return false;}
  const auto team=cache.human?revealed_team(i):plot_team(i);if(!territory(team))throw BnwNativeUnitAllocatorUnavailable("Native path blocked territory requires actual canDeclareWar owner before human refusal");return enter_terrain(i);};
 const auto result=move_into(target);current();need(prepare_existing_unit_path_cache(source)==cache,"Native common move max/base/team/policy/trait operands changed");current();return result;
}

std::optional<BnwNativeExistingUnitPathObservation>BnwNativeUnitAllocatorCore::generate_native_existing_unit_path(std::int32_t query_id,std::uint64_t target,bool reuse,ExistingPathConsumer consumer)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;Impl::Guard invocation(p);p->check();
 if(const auto*r=p->state->setup.extensions.find<Registry>())for(const auto&[actor,bank]:r->players){(void)actor;need(bank&&!bank->retired.load()&&bank->pending_units.empty(),"Native imported path requires completed rosters; quarantined newborn remains mandatory");}
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);if(!unit){p->check();(void)issuer->current();return std::nullopt;}
 const auto&s=*p->state;const auto&h=s.world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,bnw};
 need(target<h.grid.size(),"Native path destination outside actual map");const auto sx=narrow(core(*unit,"m_iX")),sy=narrow(core(*unit,"m_iY"));
 need(geometry.plot(sx,sy).has_value(),"Native path actual starting plot is NULL");const auto origin=h.grid.index(sx,sy);const auto destination=h.grid.coordinates(target);
 const auto revision=bnw_unit_order_world_revision(s,p->host->identity(),p->host->database());
 need(s.setup.game_type==0&&s.world.pregame&&s.world.pregame->archive.integers.at("game_type")==0,"Native path query requires actual sequential SP mode");
 const auto rng=s.random.gameplay.state(),map_rng=s.random.map.state();const auto extras=s.game_plot_extras;
 const auto current=[&]{
  (void)issuer->current();require_native_common_pathfinder_binding(p->host->session());p->check();
  need(native_imported_get_at(s,p->actor,query_id)==unit&&bnw_unit_order_world_revision(s,p->host->identity(),p->host->database())==revision,"Native path world/GetAt changed during callbacks");
  need(s.random.gameplay.state()==rng&&s.random.map.state()==map_rng&&s.game_plot_extras.current==extras.current&&s.game_plot_extras.costs.size()==extras.costs.size()&&s.game_plot_extras.yields.size()==extras.yields.size(),"Native path RNG/Game extras lifecycle changed");
  for(std::size_t i=0;i<extras.costs.size();++i){const auto&a=extras.costs[i];const auto&b=s.game_plot_extras.costs[i];need(a.plot.x==b.plot.x&&a.plot.y==b.plot.y&&a.cost==b.cost,"Native path actual Game extra costs changed");}
  for(std::size_t i=0;i<extras.yields.size();++i){const auto&a=extras.yields[i];const auto&b=s.game_plot_extras.yields[i];need(a.plot.x==b.plot.x&&a.plot.y==b.plot.y&&a.yields==b.yields,"Native path actual Game extra yields changed");}
 };
 auto tree=p->host->session().bnw_common_unit_pathfinder_for_native_unit(*this);current();
 BnwUnitPathOwner*paths=nullptr;BnwUnitRetainedPath*retained=nullptr;std::optional<BnwUnitRetainedPath>before_path;
 const BnwUnitOrderKey path_key{p->actor,narrow(core(*unit,"m_iID"))};
 if(consumer==ExistingPathConsumer::native_retained_cache){
  paths=&p->host->session().bnw_native_unit_paths_for_core(*this,*tree);retained=&paths->paths_.at(path_key);before_path=*retained;
  need(target<=UINT32_MAX&&retained->generation_operations<UINT64_MAX,"Native GeneratePath destination/generation capacity unavailable");current();
 }
 const auto path_current=[&]{if(paths){p->host->session().require_bnw_native_unit_paths_current_without_callbacks(*paths,*tree,*this);need(&paths->paths_.at(path_key)==retained&&*retained==*before_path,"Native retained path changed during actual search");}};
 const auto define=[&](const char*n){const auto*d=p->host->defines().find(n);need(p->host->defines().profile()==content::GlobalDefineProfile::bnw&&d&&d->lookup.found&&d->lookup.sqlite_code==0&&d->lookup.error.empty()&&!d->retained_previous&&std::holds_alternative<std::int64_t>(d->lookup.value),"Native path actual Define unavailable");return narrow(std::get<std::int64_t>(d->lookup.value));};
 const auto scalar=[&](const char*table,std::int32_t id,const char*column){const auto&e=initial_maintenance_info(*p->host,table,id);bool seen=false;std::int32_t value=0;
  const auto rows=p->host->database().query_rows({std::string("SELECT Type,")+column+" AS Operand FROM "+table+" WHERE ID=?",{id}},[&](const auto&r){need(!seen&&r.text("Type")==std::optional{e.type},"Native path actual Info Type differs");seen=true;value=narrow(r.integer("Operand"));});current();need(rows.all_succeeded()&&seen,"Native path actual Info operand unavailable");return value;};
 const auto kind=[&](std::uint64_t index){const auto&raw=native_path_plot(s,index);const auto v=narrow(post_named(raw.fields,"plot_type").value);need(v>=0&&v<4&&s.world.map_projection->cells.at(index).plot_type==std::optional<std::int64_t>{v},"Native path PlotType mirrors differ");return v;};
 const auto info_id=[&](std::uint64_t index,const char*table,const char*legacy,const char*hashed,std::int32_t version){const auto&raw=native_path_plot(s,index);need(raw.version.has_value(),"Native path plot version unavailable");
  if(*raw.version<static_cast<std::uint64_t>(version))return narrow(post_named(raw.fields,legacy).value);
  const auto hash=post_named(raw.fields,hashed).value;need(hash>=0&&hash<=UINT32_MAX,"Native path actual plot Info hash invalid");if(!hash)return std::int32_t{-1};const auto*t=p->host->types().find_hash(static_cast<std::uint32_t>(hash));need(t&&t->table==table&&t->id>=0&&t->hash==content::saved_info_type_hash(t->type),"Native path actual plot Info hash unresolved");return narrow(t->id);};
 const auto feature=[&](std::uint64_t i){return info_id(i,"Features","feature_type","feature_type_hash",3);};
 const auto terrain=[&](std::uint64_t i){const auto&raw=native_path_plot(s,i);const auto id=narrow(post_named(raw.fields,"terrain_type").value);need(s.world.map_projection->cells.at(i).terrain_type==std::optional<std::int64_t>{id},"Native path Terrain mirrors differ");return id;};
 const auto walk=[&](std::uint64_t i){const auto id=info_id(i,"Improvements","improvement_type","improvement_type_hash",5);if(id==-1)return false;const auto v=scalar("Improvements",id,"AllowsWalkWater");need(v==0||v==1,"Native path WalkWater outside bool");return v!=0;};
 const auto plot_team=[&](std::uint64_t i){const auto owner=narrow(post_named(native_path_plot(s,i).fields,"owner").value);need(owner>=-1&&owner<64&&s.world.map_projection->cells.at(i).owner==std::optional<std::int64_t>{owner},"Native path actual plot owner mirrors differ");return owner==-1?-1:post_player_team(s,owner);};
 const auto is_minor=[&](std::int32_t team){need(s.world.pregame&&s.world.pregame->layout==bnw,"Native path actual pregame unavailable");const auto&flags=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(flags.size()==64,"Native path actual Minor flags incomplete");for(std::int32_t i=22;i<64;++i){const auto&player=s.setup.players.at(i);if(player.alive&&player.team==team)return bool(flags.at(i));}return false;};
 ExistingUnitPathCache cache;bool initialized=false;
 const auto require_cache=[&]{need(initialized,"Native Path callback precedes actual UnitPathInitialize");current();};
 const auto territory=[&](std::int32_t team){require_cache();if(team==-1||team==cache.team)return true;
  if(post_relation(s,native_path_combat_team(s,*unit,team,origin),team,"at_war"))return true;
  const auto type=native_friendly_info(*p->host,s,*unit,true).type;current();const auto rival=scalar("Units",type,"RivalTerritory");need(rival==0||rival==1,"Native path RivalTerritory outside bool");if(rival||narrow(core(*unit,"m_iRivalTerritoryCount"))>0)return true;
  if(is_minor(team)&&!is_minor(cache.team)){if(!post_relation(s,cache.team,team,"has_met"))return true;if(cache.human)return true;throw BnwNativeUnitAllocatorUnavailable("Native path AI Minor territory requires actual AngerFree/OpenBorders/Intruding owner");}
  return post_relation(s,team,cache.team,"open_borders");};
 const auto trait_bool=[&](const char*n){const auto&t=s.world.all_players->players.at(p->actor).traits;const auto&f=post_named(t.fields,n);need(t.layout==bnw&&f.encoding==saves::PlayerTraitsEncoding::boolean&&f.values.size()==1&&f.rows.empty()&&f.hashes.empty()&&!f.wire_count,"Native path actual trait Boolean absent");need(f.values[0]==0||f.values[0]==1,"Native path actual trait Boolean invalid");return f.values[0]!=0;};
 const auto indexed_impassable=[&](const char*n,const char*table,std::int32_t id){const auto&f=post_named(unit->core.fields,n);const auto span=native_init_info_span(*p->host,table);need(f.encoding==saves::UnitCoreEncoding::vector_i32&&f.wire_count&&*f.wire_count>=0&&static_cast<std::uint64_t>(*f.wire_count)==span&&f.values.size()==span&&id>=0&&static_cast<std::uint64_t>(id)<span,"Native path full indexed impassable cache unavailable");return narrow(f.values.at(id))>0;};
 const auto enter_terrain=[&](std::uint64_t i){require_cache();const bool all=narrow(core(*unit,"m_iCanMoveAllTerrainCount"))>0,hover=narrow(core(*unit,"m_iHoveringUnitCount"))>0;
  const auto&pass=bnw_live_plot_impassability(s);need(i<pass.plots.size(),"Native path restored impassability span incomplete");if(pass.plots.at(i).impassable&&narrow(core(*unit,"m_iCanMoveImpassableCount"))<=0&&!all)return false;
  if(kind(i)==0&&!(trait_bool("m_bCrossesMountainsAfterGreatGeneral")&&post_player(s,p->actor,"m_iGreatGeneralsCreated")>0)&&!hover&&!all)return false;
  if(cache.immobile)return false;const bool allows=walk(i);
  if(territory(plot_team(i))){const auto f=feature(i),route=narrow(post_named(native_path_plot(s,i).fields,"route_type").value);
   if(f!=-1&&route==-1){if(indexed_impassable("m_featureImpassableCount","Features",f))throw BnwNativeUnitAllocatorUnavailable("Native path feature passability requires actual promotion-tech/load owner");}
   else if(indexed_impassable("m_terrainImpassableCount","Terrains",terrain(i)))throw BnwNativeUnitAllocatorUnavailable("Native path terrain passability requires actual promotion-tech/load owner");
  }
  if(cache.domain==1)return true;
  need(cache.domain==2,"Native path non-LAND terrain admission requires actual domain owner");if(allows)return true;
  if(kind(i)==3&&!all){if(cache.can_embark){if(terrain(i)==define("DEEP_WATER_TERRAIN")&&post_named(post_team(s,cache.team).fields,"embarked_all_water").value<=0&&native_friendly_tail(*unit,"m_iEmbarkedAllWaterCount")<=0)return false;}
   else if(hover){if(terrain(i)==define("DEEP_WATER_TERRAIN"))return false;}else return false;}
  return true;};
 const auto friendly=[&](std::uint64_t i){const auto a=native_friendly_count(*p->host,s,*unit,i,true);current();const auto b=native_friendly_count(*p->host,s,*unit,i,true);need(a.inputs==b.inputs&&a.count==b.count,"Native path friendly source changed during rules reads");current();return a.count;};
 const auto best_defender=[&](std::uint64_t i){const saves::SavedUnitObservation*best=nullptr;
  for(const auto&handle:native_path_stack(s,i)){if(handle.owner<0||handle.owner>=64)continue;const auto*other=native_imported_get_at(s,narrow(handle.owner),narrow(handle.id));if(!other)continue;
   // SDK IsCanDefend executes BEFORE pointer!=attacker. Actual default plot
   // is the candidate's current location, not the queried target coordinate.
   need(!existing_path_bool(*other,"m_bEmbarked"),"Native path candidate GetBaseCombatStrength requires actual embarked defense owner");
   if(narrow(core(*other,"m_iBaseCombat"))==0)continue;if(existing_path_bool(*other,"m_bDeathDelay"))continue;
   const auto x=narrow(core(*other,"m_iX")),y=narrow(core(*other,"m_iY"));const auto location=geometry.plot(x,y);need(location.has_value(),"Native path defender actual plot NULL");
   const auto info=native_friendly_info(*p->host,s,*other);current();bool action=false;
   if(info.domain==0)action=kind(*location)==3||narrow(core(*other,"m_iCanMoveAllTerrainCount"))>0;
   else if(info.domain==1)action=false;
   else if(info.domain==2||info.domain==3)action=kind(*location)!=3||narrow(core(*other,"m_iHoveringUnitCount"))>0||narrow(core(*other,"m_iCanMoveAllTerrainCount"))>0||walk(*location);
   if(!action||other==unit)continue;if(native_path_invisible(s,*other,cache.team))continue;
   need(cache.domain!=1,"Native AIR attacking candidate ranged combat-limit owner unavailable");
   // Native IsBetterDefenderThan(NULL,attacker) returns true immediately.
   // A second eligible candidate really reaches the comparator and stays
   // unavailable; no early existential shortcut hides that mandatory call.
   if(best)throw BnwNativeUnitAllocatorUnavailable("Native path second eligible defender requires actual native comparison owner");best=other;
  }current();return best!=nullptr;};
 const auto revealed_team=[&](std::uint64_t i){const auto&a=post_named(native_path_plot(s,i).arrays,"revealed_owners");need(a.columns==1&&a.values.size()==80&&cache.team>=0&&cache.team<64,"Native path actual revealedOwner span incomplete");const auto owner=narrow(a.values.at(cache.team));need(owner>=-1&&owner<64,"Native path revealed owner outside native player domain");return owner==-1?-1:post_player_team(s,owner);};
 const auto move_into=[&](std::uint64_t i,bool dest){require_cache();const auto result=existing_native_can_move_into(*unit,i,cache,dest?std::uint8_t{4}:std::uint8_t{0});current();return result;};
 const auto move_or_attack=[&](std::uint64_t i){if(move_into(i,true))return true;
  // ATTACK invocation starts with the same real early guards, then actual
  // attack budget; no successful nonattack substitute on a reached attack.
  if(i==origin)return false;need(!cache.no_reveal,"Native path attack NoReveal owner unavailable");
  if(cache.owner==63&&plot_team(i)!=-1)throw BnwNativeUnitAllocatorUnavailable("Native path attack barbarian release owner unavailable");
  const auto limit=define("PLOT_UNIT_LIMIT");if(limit>0&&bnw_plot_is_visible(s,i,cache.team)&&friendly(i)>=limit)return false;
  if(narrow(core(*unit,"m_iBlitzCount"))<=0&&narrow(core(*unit,"m_iAttacksMade"))>=narrow(core(*unit,"m_iNumAttacks")))return false;
  throw BnwNativeUnitAllocatorUnavailable("Native path reached ATTACK admission requires actual city-only/defender/combat checks");};
 const auto has_promotion=[&](std::int32_t id){if(id<0)return false;const auto span=native_init_info_span(*p->host,"UnitPromotions");need(static_cast<std::uint64_t>(id)<span,"Native path promotion Define exceeds native span");const saves::UnitTailNode*promotions=nullptr,*bits=nullptr;
  for(const auto&b:unit->tail.blocks)if(name(b.name)=="promotions"){need(!promotions,"Duplicate path promotions block");promotions=&b;}need(promotions,"Native path promotions block absent");
  for(const auto&b:promotions->children)if(name(b.name)=="m_kHasPromotion"){need(!bits,"Duplicate path promotions cache");bits=&b;}need(bits&&name(bits->encoding)=="signed_hash_bool"&&bits->wire_count&&*bits->wire_count>=0&&bits->values.size()==static_cast<std::uint64_t>(*bits->wire_count)*2,"Native path actual promotion cache unavailable");
  bool value=false;for(std::size_t j=0;j<bits->values.size();j+=2){const auto hash=bits->values[j],flag=bits->values[j+1];need(hash>=0&&hash<=UINT32_MAX&&(flag==0||flag==1),"Native path promotion wire operands invalid");const auto*t=hash?p->host->types().find_hash(static_cast<std::uint32_t>(hash)):nullptr;if(t&&t->table=="UnitPromotions"&&t->id==id)value=flag!=0;}return value;};
 const auto defense=[&](std::uint64_t i){std::int32_t v=0;const auto k=kind(i);if(k==0||k==1)v=define("HILLS_EXTRA_DEFENSE");else{const auto f=feature(i);if(f!=-1)v=scalar("Features",f,"Defense");else{v=scalar("Terrains",terrain(i),"Defense");if(k!=3)v=narrow(std::int64_t(v)+define("FLAT_LAND_EXTRA_DEFENSE"));}}
  const auto improvement=info_id(i,"Improvements","improvement_type","improvement_type_hash",5);const auto pillaged=narrow(post_named(native_path_plot(s,i).fields,"improvement_pillaged").value);need(pillaged==0||pillaged==1,"Native path improvement pillaged invalid");if(improvement!=-1&&!pillaged&&(plot_team(i)==-1||plot_team(i)==cache.team))v=narrow(std::int64_t(v)+scalar("Improvements",improvement,"DefenseModifier"));(void)native_path_city(s,i);return v;};
 const auto index=[&](const BnwTwoLayerUnitPathFinder::Node&n){need(tree->valid_coordinates(n.x,n.y),"Native path callback node outside actual allocation");return h.grid.index(n.x,n.y);};
 BnwTwoLayerUnitPathFinder::Callbacks callbacks;
 callbacks.require_current=[&]{current();path_current();};
 callbacks.initialize=[&]{current();cache=prepare_existing_unit_path_cache(*unit);initialized=true;current();};
 callbacks.uninitialize=[&]{require_cache();need(prepare_existing_unit_path_cache(*unit)==cache,"Native path scratch operands changed across search");current();};
 callbacks.heuristic=[&](std::int32_t x,std::int32_t y,std::int32_t dx,std::int32_t dy){current();return narrow(geometry.distance(x,y,dx,dy)*1000);};
 callbacks.partial_required=[&](const auto&){require_cache();return true;}; // actual info=0 has no IGNORE_STACKING
 callbacks.destination_valid=[&](std::int32_t x,std::int32_t y){require_cache();if(!tree->valid_coordinates(x,y))return false;const auto i=h.grid.index(x,y);if(i==origin)return true;if(kind(i)==0&&(!cache.human||cache.automated))return false;if(cache.immobile)return false;
  if(cache.automated)throw BnwNativeUnitAllocatorUnavailable("Native PathDestValid automation requires actual danger/group-area owner");
  const auto revealed=detail::bnw_plot_revealed_read(s.world,i,cache.team);if(!revealed&&cache.no_reveal)return false;if(revealed){std::int32_t owner=-1;if(native_path_city(s,i,&owner)&&owner!=cache.owner&&!post_relation(s,cache.team,post_player_team(s,owner),"at_war"))return false;}
  if(revealed){need(cache.human,"Native PathDestValid AI IsDeclareWar/attack owner unavailable");if(!move_or_attack(i))return false;}return true;};
 callbacks.valid=[&](const auto*parent,auto&node){require_cache();const auto i=index(node);auto&c=node.cache;
  c.visible=bnw_plot_is_visible(s,i,cache.team);c.friendly_units=friendly(i);c.mountain=kind(i)==0;c.water=kind(i)==3&&!walk(i);c.can_enter_terrain=enter_terrain(i);c.revealed=detail::bnw_plot_revealed_read(s.world,i,cache.team);c.other_friendly_city=false;std::int32_t owner=-1;if(native_path_city(s,i,&owner)&&owner!=cache.owner&&!post_relation(s,cache.team,post_player_team(s,owner),"at_war"))c.other_friendly_city=true;
  c.enemy_city=native_path_enemy_city(s,*unit,i);c.visible_enemy=native_path_visible_enemy(s,*unit,i);c.visible_enemy_defender=best_defender(i);current();if(!parent)return true;
  const auto from=index(*parent);const auto*cursor=&node;bool first=true,hostile=false,visible=c.visible;auto turns=parent->turns;if(parent->moves==0)turns=narrow(std::int64_t(turns)+1);std::int32_t old=-1;std::size_t visited=0;const auto limit=define("PLOT_UNIT_LIMIT");
  while(cursor){need(++visited<=tree->primary_.size()*2,"Native path ancestry cycle");const auto&k=cursor->cache;if(old!=-1||tree->destination(cursor->x,cursor->y)){if(turns!=old||hostile||!visible){if(index(*cursor)!=origin){if(k.visible){if(k.friendly_units>=limit)return false;if(k.mountain&&(!cache.human||cache.automated))return false;if(k.mountain&&!k.can_enter_terrain)return false;}if(k.revealed&&k.other_friendly_city)return false;}}}
   hostile=k.enemy_city||(k.visible_enemy&&k.visible_enemy_defender);visible=k.visible;if(cursor==&node&&first){cursor=parent;first=false;}else cursor=cursor->parent;if(cursor){old=turns;turns=cursor->turns;}}
  if(cache.domain==2&&!parent->cache.water&&c.water&&c.revealed)throw BnwNativeUnitAllocatorUnavailable("Native PathValid water transition requires actual canEmbarkOnto owner");
  if(!cache.combat&&cache.domain!=1)for(const auto&handle:native_path_stack(s,i)){if(handle.owner<0||handle.owner>=64)continue;const auto*other=native_imported_get_at(s,narrow(handle.owner),narrow(handle.id));if(other&&narrow(core(*other,"m_eOwner"))!=cache.owner)return false;}
  if(cache.combat&&cache.domain!=1&&geometry.distance(sx,sy,node.x,node.y)==1&&c.visible_enemy&&c.visible_enemy_defender&&friendly(origin)>limit)return false;
  if(from==origin)return true;if(cache.automated&&(parent->turns>1||parent->moves==0)&&(!cache.combat||narrow(core(*unit,"m_iArmyId"))==-1))throw BnwNativeUnitAllocatorUnavailable("Native PathValid automation requires actual PlotDanger owner");
  if(cache.automated||parent->cache.revealed||cache.embarked||!cache.human){if(!move_into(from,false))return false;}current();return true;};
 callbacks.cost=[&](const auto&parent,const auto&node){require_cache();ExistingPathMovementCostDispatch dispatch(*this,tree,*unit,parent,node,cache);const auto operands=existing_path_movement_cost_operands(dispatch);const auto move=BnwNativeUnitPathMovementCost::evaluate(*this,dispatch);const auto max=operands.remaining;auto left=narrow(std::int64_t(max)-move);auto cost=move;if(left<0){left=0;cost=max;}cost=narrow(std::int64_t(cost)*1000);const auto i=index(node);
  if(left==0){need(define("PATH_DAMAGE_WEIGHT")==0,"Native PathCost feature damage requires actual Game extras/feature/MAX_HP owner");const auto limit=define("PLOT_UNIT_LIMIT");if(limit>0&&friendly(i)>=limit)cost=narrow(std::int64_t(cost)+1000000);if(kind(i)==0&&(!cache.human||cache.automated))cost=narrow(std::int64_t(cost)+1000000);
   if(has_promotion(define("PROMOTION_UNWELCOME_EVANGELIST"))){need(cache.human&&!cache.automated,"Native PathCost missionary foreign penalty owner unavailable");}else if(plot_team(i)!=cache.team)cost=narrow(std::int64_t(cost)+3);
  }
  if(cache.combat){if(left==0){const auto bonus=narrow(core(*unit,"m_iNoDefensiveBonusCount"))>0?0:defense(i);cost=narrow(std::int64_t(cost)+10*std::max<std::int64_t>(0,200-std::int64_t(bonus)));}if(cache.automated&&cache.can_attack&&tree->destination(node.x,node.y)&&best_defender(i))throw BnwNativeUnitAllocatorUnavailable("Native PathCost attack defense/river penalty owner unavailable");}
  need(cost!=INT32_MAX,"Native PathCost MAX_INT sentinel");cost=narrow(std::int64_t(cost)+2);need(cost>0,"Native PathCost nonpositive native cost");current();return cost;};
 callbacks.notify_child=[&](const auto*parent,auto&node,auto notice){require_cache();if(notice==BnwTwoLayerUnitPathFinder::ChildNotice::initial){node.turns=1;node.moves=narrow(core(*unit,"m_iMoves"));need(node.moves>=0,"Native PathAdd negative actual moves");}
  else{need(parent,"Native PathAdd missing physical parent");auto turns=parent->turns;auto start=parent->moves;if(start==0){turns=narrow(std::int64_t(turns)+1);start=narrow(std::int64_t(cache.base_moves.at(2))*cache.denominator);}ExistingPathMovementCostDispatch dispatch(*this,tree,*unit,*parent,node,cache);
   const auto target_index=index(node);const bool consumes=cache.human&&!detail::bnw_plot_revealed_read(s.world,target_index,cache.team);
   // Dispatch observes the old real node data, so derive cost before stores.
   const auto moves=(consumes||existing_path_slowed_by_zoc(dispatch))?0:std::max<std::int32_t>(0,narrow(std::int64_t(start)-BnwNativeUnitPathMovementCost::evaluate(*this,dispatch)));node.turns=turns;node.moves=moves;
  }current();};
 const auto found=tree->generate(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(unit)),sx,sy,narrow(destination[0]),narrow(destination[1]),0,reuse,false,callbacks);current();
 BnwNativeExistingUnitPathObservation result;result.found=found;if(tree->best_){const auto*node=tree->best_;std::size_t count=0;while(node){need(++count<=tree->primary_.size()*2,"Native completed path parent cycle");result.nodes.push_back({index(*node),node->x,node->y,node->moves,node->turns,node->known,node->total});node=node->parent;}std::reverse(result.nodes.begin(),result.nodes.end());}
 current();path_current();p->host->session().require_bnw_common_unit_pathfinder_native_current_without_callbacks(*tree,*this);
 if(paths){
  std::vector<BnwUnitRetainedPathNode>copy;copy.reserve(result.nodes.size());
  // CvAStar::CopyPath is destination-first. The diagnostic view is reversed
  // independently; it never serves as a path admission supplied by caller.
  for(auto i=result.nodes.rbegin();i!=result.nodes.rend();++i)copy.push_back({i->x,i->y,i->moves,i->turns,0});
  current();path_current();const auto team=post_player_team(s,p->actor);
  std::vector<std::uint32_t>flags(copy.size(),0);
  for(std::size_t i=copy.size();i--;){const auto at=geometry.plot(copy[i].x,copy[i].y);if(at&&!bnw_plot_is_visible(s,*at,team)){
    flags[i]|=1U;
    // Preserve native unsigned size-2 semantics without executing its
    // undefined one-node invisible +1 access. A reached malformed boundary
    // is refused before any retained-cache stores.
    need(copy.size()>=2,"Native GeneratePath one-node invisible adjacency underflow");
    if(i<copy.size()-2){flags[i+1]|=2U;const auto dest=geometry.plot(copy.front().x,copy.front().y);if(dest&&!bnw_plot_is_visible(s,*dest,team))flags[0]|=1U;break;}
   }}
  current();path_current();BnwUnitRetainedPath expected=*before_path;expected.nodes=copy;for(std::size_t i=0;i<copy.size();++i)expected.nodes[i].flags=flags[i];expected.destination=static_cast<std::uint32_t>(target);
  const auto stores=retained->generation_operations+1;
  // All capacity and actual visibility reads precede this no-throw native
  // destination -> CopyPath -> flags publication on the SAME registered node.
  expected.generation_operations=stores;
  retained->destination=static_cast<std::uint32_t>(target);
  retained->nodes.swap(copy);
  for(std::size_t i=0;i<retained->nodes.size();++i)retained->nodes[i].flags=flags[i];
  retained->generation_operations=stores;
  before_path=std::move(expected);
  current();path_current();
 }
 return result;
}

bool BnwNativeUnitAllocatorCore::initial_happiness_city_has_garrison(std::int32_t city)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 require_initial_set_position_without_callbacks();p->check();
 const bool result=BnwMinorCityStrengthScratch::has_initial_pending_garrison(*self,city);
 require_initial_set_position_without_callbacks();return result;
}
void BnwNativeUnitAllocatorCore::require_initial_city_strength_node_without_callbacks()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_hidden_binding&&p->initial_plot_area&&p->initial_plot_area->stored,
  "Initial city strength requires genuine stored plot-area predecessor");
 require_initial_visibility_binding_controls_without_callbacks(*p->initial_hidden_binding);
 const auto*registry=p->state->setup.extensions.find<Registry>();
 need(registry&&p->bank->pending_units.size()==1,"Initial city strength canonical single pending node registry differs");
 for(const auto&[actor,bank]:registry->players){(void)actor;need(static_cast<bool>(bank),"Initial city strength native bank roster missing");
  if(bank.get()!=p->bank.get())need(!bank->constructor_pending&&bank->pending_units.empty(),"Initial city defender unrepresented other pending native node requires its genuine resolver");}
 const auto&a=*p->initial_plot_area;
 need(initial_happiness_prior_revision_without_callbacks()==a.after_revision||(p->initial_city_strength&&p->initial_city_strength->stored&&
  p->initial_city_strength->before_revision==a.after_revision&&initial_happiness_prior_revision_without_callbacks()==p->initial_city_strength->after_revision),
  "Initial city strength SAME actual node/source revision changed");
 need(static_cast<bool>(p->initial_set_xy),"Initial city strength actual source-issued phase missing");
 p->initial_set_xy->require_initial_city_strength_phase_without_callbacks();
 require_initial_plot_area_prefix(*p->train_source);
}
const std::map<std::string,std::int32_t,std::less<>>&BnwNativeUnitAllocatorCore::initial_city_strength_scalars()const{
 require_initial_city_strength_node_without_callbacks();return impl_->bank->pending_units.at(impl_->bank->pending_slot)->scalars;
}
const std::map<std::string,std::vector<std::int32_t>,std::less<>>&BnwNativeUnitAllocatorCore::initial_city_strength_indexed()const{
 require_initial_city_strength_node_without_callbacks();return impl_->bank->pending_units.at(impl_->bank->pending_slot)->indexed;
}
void BnwNativeUnitAllocatorCore::require_initial_city_strength_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_city_strength&&p->initial_city_strength->stored,"Initial city strength actual store receipt absent");
 need(p->train_source.get()==&source&&!source.factory_self_.expired()&&!source.factory_self_.owner_before(p->train_source)&&
  !p->train_source.owner_before(source.factory_self_),"Initial city strength canonical TRAIN Source differs");
 need(p->initial_city_strength->generation==p->generation&&p->initial_city_strength->state.get()==p->state.get()&&
  !p->state.owner_before(p->initial_city_strength->state)&&!p->initial_city_strength->state.owner_before(p->state),"Initial city strength SAME state/control differs");
 BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*this,*p->initial_city_strength,true);
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_city_strength_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(p->initial_set_xy&&p->initial_plot_area&&p->initial_plot_area->stored&&!p->initial_city_strength&&
  p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_city_strength_owner,"Initial city strength genuine plot-area/once phase absent");
 require_initial_visibility_hidden_slot_current(*p->initial_hidden_binding,*p->initial_hidden_proof);
 p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
 need(p->host->session().has_bnw_operation_checkpoint_history(),"Initial city strength requires real Session operation quarantine");
 p->initial_city_strength=BnwMinorCityStrengthScratch::prepare_initial_pending(*self);
 try{
  auto&q=*p->initial_city_strength;need(q.before_revision==p->initial_plot_area->after_revision&&q.generation==p->generation&&q.state.get()==p->state.get(),"Initial city strength source lineage changed");
  auto events=q.after_events; // Complete capacity allocation before final bookends.
  p->check();require_initial_visibility_hidden_slot_current(*p->initial_hidden_binding,*p->initial_hidden_proof);
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
  BnwMinorCityStrengthScratch::require_initial_pending_without_callbacks(*self,q,false);
  auto&cities=p->state->world.all_players->players.at(static_cast<std::size_t>(q.quote.owner)).cities.cities;
  saves::CityCoreField*destination=nullptr;for(auto&city:cities)if(&city==q.quote.raw)destination=&garrison_named_mutable(city.core.fields,"m_iStrengthValue");
  need(destination&&destination->encoding==saves::CityCoreEncoding::i32&&!destination->wire_count&&destination->values.size()==1&&destination->bytes.empty()&&
   destination->hashes.empty()&&destination->row_counts.empty()&&destination->row_keys.empty()&&destination->values[0]==q.quote.before,
   "Initial city strength actual scalar destination/storage differs");
  need(!p->bank->retired.load()&&initial_best_land_prior_world_revision_without_callbacks()==q.before_revision&&p->host->session().read_state_lease().get()==p->state.get()&&
   p->host->session().state_generation()==p->generation,"Initial city strength final source/reentry changed");
  // CvCity::updateStrengthValue: native cache first, exactly one CityInfo dirty
  // queue entry next. No renderer callback, public unit or save authority.
  destination->values[0]=q.quote.after;
  p->state->city_dirty_events.swap(events);p->state->city_dirty_next_occurrence=q.before_occurrence+1;
  p->state->ui_source_revision=q.after_revision;q.stored=true;
  p->initial_set_xy->record_initial_city_strength_return(q.after_revision);
  p->train_source->accept_actual_initial_city_strength_prefix(*self);
  return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;}
}

void BnwNativeUnitAllocatorCore::require_initial_plot_area_prefix(const ui::SelectedCityTrainCreateSource&source)const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id()&&p->initial_plot_area&&p->initial_plot_area->stored,
  "Initial plot/area actual stored prefix absent");
 const auto&a=*p->initial_plot_area;
 need(p->train_source.get()==&source&&!source.factory_self_.expired()&&!source.factory_self_.owner_before(p->train_source)&&
  !p->train_source.owner_before(source.factory_self_),"Initial plot/area SAME canonical Source differs");
 // Use the controls-only proof to avoid recursion through the hidden receipt.
 need(p->initial_hidden_binding&&p->initial_hidden_proof&&p->initial_sight_stored,"Initial plot/area real native predecessors missing");
 require_initial_visibility_binding_controls_without_callbacks(*p->initial_hidden_binding);
 p->initial_set_xy->require_initial_visibility_hidden_slot_phase_without_callbacks();p->initial_sight->require_after_without_callbacks(*this);
 need(p->host->session().has_bnw_operation_checkpoint_history()&&a.before_revision==p->initial_sight_after&&a.before_revision<UINT64_MAX&&
  a.after_revision==a.before_revision+1&&(initial_happiness_prior_revision_without_callbacks()==a.after_revision||(p->initial_city_strength&&p->initial_city_strength->stored&&
   p->initial_city_strength->before_revision==a.after_revision&&initial_happiness_prior_revision_without_callbacks()==p->initial_city_strength->after_revision)),"Initial plot/area actual quarantine/revision differs");
 auto&raw=p->state->world.map_plots->plots.at(static_cast<std::size_t>(a.plot));auto&cell=p->state->world.map_projection->cells.at(static_cast<std::size_t>(a.plot));
 need(&raw==a.raw_plot&&&cell==a.projected_plot&&cell.units&&&initial_plot_roster(raw)==a.raw&&&*cell.units==a.projected&&
  *a.raw==a.raw_after&&initial_same_handles(*a.projected,a.projected_after),"Initial plot/area actual stored plot links changed");
 auto&area=initial_destination_area(*p->state,a.plot);
 need(&area==a.area&&initial_area_scalar(area,"id")==a.area_id&&&initial_area_scalar(area,"units")==a.area_units&&
  &initial_area_players(area)==a.area_players&&*a.area_units==a.units_before+1&&*a.area_players==a.area_players_after,
  "Initial plot/area actual stored area counters changed");
 for(const auto&manager:a.managers)compare(*p->state,manager.first,manager.second);
 initial_cycle_current(*p->state,a.compared);
 const auto&u=p->bank->pending_units.at(p->bank->pending_slot);
 need(u->scalars.at("m_iID")==a.id&&u->scalars.at("m_eOwner")==a.owner&&u->scalars.at("m_eUnitType")==a.type&&
  u->scalars.at("m_iBaseCombat")==a.combat&&u->scalars.at("m_iLevel")==a.level&&u->scalars.at("m_iExperience")==a.experience,
  "Initial plot/area SAME pending node sort operands changed");
}
std::shared_ptr<const BnwUnitMovePending>BnwNativeUnitAllocatorCore::advance_initial_plot_area_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(p->initial_set_xy&&p->initial_sight&&p->initial_sight_stored&&!p->initial_plot_area&&
  p->initial_set_xy->prefix().boundary==BnwUnitMoveBoundary::initial_plot_add_unit_owner,
  "Initial plot/area requires genuine returned sight and once pending phase");
 require_initial_visibility_hidden_slot_current(*p->initial_hidden_binding,*p->initial_hidden_proof);
 p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);
 need(p->host->session().has_bnw_operation_checkpoint_history(),"Initial plot/area requires actual Session operation quarantine");
 const auto&u=p->bank->pending_units.at(p->bank->pending_slot);const auto&t=*p->bank->train;
 InitialPlotAreaPrefix a;a.owner=p->actor;a.id=u->scalars.at("m_iID");a.type=u->scalars.at("m_eUnitType");
 a.combat=u->scalars.at("m_iBaseCombat");a.level=u->scalars.at("m_iLevel");a.experience=u->scalars.at("m_iExperience");
 a.plot=p->state->world.map_header.grid.index(static_cast<std::uint64_t>(t.x),static_cast<std::uint64_t>(t.y));
 auto&raw=p->state->world.map_plots->plots.at(static_cast<std::size_t>(a.plot));auto&cell=p->state->world.map_projection->cells.at(static_cast<std::size_t>(a.plot));
 need(raw.index==a.plot&&cell.index==a.plot&&raw.x==t.x&&raw.y==t.y&&cell.x==t.x&&cell.y==t.y&&cell.units,
  "Initial addUnit canonical target plot identity differs");
 a.raw_plot=&raw;a.projected_plot=&cell;a.raw=&initial_plot_roster(raw);a.projected=&*cell.units;
 a.raw_before=*a.raw;a.projected_before=*a.projected;
 need(a.raw_before.size()==a.projected_before.size()*2,"Initial addUnit raw/projected source list differs");
 for(std::size_t i=0;i<a.projected_before.size();++i)need(a.raw_before[2*i]==a.projected_before[i].owner&&a.raw_before[2*i+1]==a.projected_before[i].id&&
  !(a.projected_before[i].owner==a.owner&&a.projected_before[i].id==a.id),"Initial addUnit handle source differs/already inserted");
 // Capture real target area operands before source/SQL callbacks. The native
 // +1 follows insertion, but both effects are staged before any world store.
 auto&area=initial_destination_area(*p->state,a.plot);a.area=&area;a.area_id=initial_area_scalar(area,"id");a.area_units=&initial_area_scalar(area,"units");a.area_players=&initial_area_players(area);
 a.units_before=*a.area_units;a.area_players_before=*a.area_players;a.owner_before=a.area_players_before.at(static_cast<std::size_t>(a.owner));
 need(a.units_before>=0&&a.units_before<INT32_MAX&&a.owner_before>=0&&a.owner_before<INT32_MAX,
  "Initial new-area unit counters exceed checked SDK signed range");
 a.area_players_after=a.area_players_before;a.area_players_after.at(static_cast<std::size_t>(a.owner))=a.owner_before+1;
 const auto managers=prepare_initial_destination_flta_lookup(*p->initial_set_xy);
 for(const auto&manager:managers)a.managers.emplace_back(manager.actor,manager.baseline);
 const auto load_domain=[&](std::int32_t type){std::optional<std::int32_t>domain;
  const auto query=p->host->database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{std::int64_t(type)}},[&](const auto&row){
   const auto typ=row.text("Type"),dom=row.text("Domain");const auto*entry=typ?p->host->types().find(*typ):nullptr;const auto*d=dom?p->host->types().find(*dom):nullptr;
   need(!domain&&entry&&entry->table=="Units"&&entry->id==type&&d&&d->table=="Domains"&&d->id>=0&&d->id<5,"Initial addUnit actual UnitInfo domain unavailable");domain=narrow(d->id);
  });need(query.all_succeeded()&&domain.has_value(),"Initial addUnit selected UnitInfo query failed");
  // Ordinary defaultBNW unit path. HOVER requires its reached real plot getter.
  need(*domain!=4,"Initial addUnit DOMAIN_HOVER dynamic plot-domain supplier required");return *domain;
 };
 a.domain=load_domain(a.type);
 InitialCycleOperand newborn; // actual pending getters, never a SavedUnit surrogate.
 newborn.owner=a.owner;newborn.id=a.id;newborn.type=a.type;newborn.domain=a.domain;newborn.combat=a.combat;newborn.level=a.level;newborn.experience=a.experience;
 std::size_t insertion=0;
 for(;insertion<a.projected_before.size();++insertion){const auto h=a.projected_before[insertion];
  need(h.owner>=0&&h.owner<64&&h.id>=-1&&h.id<=INT32_MAX,"Initial addUnit actual source handle domain invalid");
  InitialCycleOperand operand;operand.query_owner=narrow(h.owner);operand.query_id=narrow(h.id);operand.raw=native_imported_get_at(*p->state,operand.query_owner,operand.query_id);
  if(operand.raw){const auto&unit=*operand.raw;
   do{operand.owner=narrow(core(unit,"m_eOwner"));operand.consumed=1;if(operand.owner!=newborn.owner)break;
    const auto&owned=p->state->maintenance.players.at(operand.query_owner).units.at(narrow(core(unit,"m_iID")));
    operand.type=narrow(owned.type);operand.domain=load_domain(operand.type);operand.consumed=2;
    need(operand.domain==owned.domain,"Initial addUnit actual raw/owned UnitInfo domain differs");if(operand.domain!=newborn.domain)break;
    need(core(unit,"m_bEmbarked")==0,"Initial addUnit reached embarked base-strength supplier required");
    operand.combat=narrow(core(unit,"m_iBaseCombat"));operand.consumed=3;if(operand.combat!=newborn.combat)break;
    operand.consumed=4;if(operand.type!=newborn.type)break;
    operand.level=narrow(core(unit,"m_iLevel"));operand.consumed=5;if(operand.level!=newborn.level)break;
    operand.experience=narrow(core(unit,"m_iExperience"));operand.consumed=6;if(operand.experience!=newborn.experience)break;
    operand.id=narrow(core(unit,"m_iID"));operand.consumed=7;
   }while(false);
  }
  a.compared.push_back(operand);if(!initial_cycle_before(operand,newborn))break;
 }
 a.raw_after=a.raw_before;a.projected_after=a.projected_before;
 a.raw_after.insert(a.raw_after.begin()+static_cast<std::ptrdiff_t>(2*insertion),{a.owner,a.id});
 a.projected_after.insert(a.projected_after.begin()+static_cast<std::ptrdiff_t>(insertion),saves::SavedPlotUnitReference{a.owner,a.id});
 a.raw_staging=a.raw_after;a.projected_staging=a.projected_after;
 a.before_revision=initial_best_land_prior_world_revision_without_callbacks();need(a.before_revision==p->initial_sight_after&&a.before_revision<UINT64_MAX,"Initial plot/area source revision changed");a.after_revision=a.before_revision+1;
 p->initial_plot_area=std::move(a); // retain attempted stage before final callbackful checks.
 try{
  p->check();require_initial_visibility_hidden_slot_current(*p->initial_hidden_binding,*p->initial_hidden_proof);
  p->train_source->require_for_allocator(*p->host,p->host->session(),p->actor);require_initial_flta_current(*p->initial_set_xy,managers);
  auto&stage=*p->initial_plot_area;
  require_initial_visibility_hidden_slot_identity_without_callbacks(*p->initial_hidden_binding,*p->initial_hidden_proof);
  need(p->host->session().has_bnw_operation_checkpoint_history()&&initial_best_land_prior_world_revision_without_callbacks()==stage.before_revision,
   "Initial addUnit actual quarantine/revision changed before stores");
  auto&current_raw=p->state->world.map_plots->plots.at(static_cast<std::size_t>(stage.plot));auto&current_cell=p->state->world.map_projection->cells.at(static_cast<std::size_t>(stage.plot));
  need(&current_raw==stage.raw_plot&&&current_cell==stage.projected_plot&&current_cell.units&&&initial_plot_roster(current_raw)==stage.raw&&
   &*current_cell.units==stage.projected&&*stage.raw==stage.raw_before&&initial_same_handles(*stage.projected,stage.projected_before),
   "Initial addUnit target list/storage changed before stores");
  auto&current_area=initial_destination_area(*p->state,stage.plot);
  need(&current_area==stage.area&&initial_area_scalar(current_area,"id")==stage.area_id&&&initial_area_scalar(current_area,"units")==stage.area_units&&
   &initial_area_players(current_area)==stage.area_players&&*stage.area_units==stage.units_before&&*stage.area_players==stage.area_players_before,
   "Initial new-area actual source counters/storage changed before stores");
  initial_cycle_current(*p->state,stage.compared);
  need(u->scalars.at("m_iID")==stage.id&&u->scalars.at("m_eOwner")==stage.owner&&u->scalars.at("m_eUnitType")==stage.type&&
   u->scalars.at("m_iBaseCombat")==stage.combat&&u->scalars.at("m_iLevel")==stage.level&&u->scalars.at("m_iExperience")==stage.experience&&
   !p->bank->retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation,
   "Initial addUnit SAME pending node/final source changed");
  // SDK addUnit(this,false): real ordered raw/projected plot link; no center
  // callback. Next CvArea::changeUnitsPerPlayer increments total THEN owner.
  stage.raw->swap(stage.raw_staging);stage.projected->swap(stage.projected_staging);
  *stage.area_units=stage.units_before+1;(*stage.area_players)[static_cast<std::size_t>(stage.owner)]=stage.owner_before+1;
  p->state->ui_source_revision=stage.after_revision;stage.stored=true;
  p->initial_set_xy->record_initial_plot_area_return(stage.after_revision);
  p->train_source->accept_actual_initial_plot_area_prefix(*self);
  return p->initial_set_xy;
 }catch(...){p->bank->retired=true;throw;} // actual stores never masquerade as completed Init or roll back burned identity.
}

void BnwNativeUnitAllocatorCore::require_initial_visibility_binding_current(const InitialVisibilityBinding&binding)const{
 const auto self=retain_factory_owner();const auto p=impl_;
 require_initial_visibility_binding_identity_without_callbacks(binding);
 p->check();require_initial_placement_current(binding.pending.get());binding.pending->require_current();
 require_initial_visibility_binding_identity_without_callbacks(binding); // Pure final bookend after ALL providers.
}
std::int32_t BnwNativeUnitAllocatorCore::require_completed_train_unit_id(const ui::SelectedCityTrainCreateSource&supplied)const{
 const auto self=retain_factory_owner();const auto p=impl_;const auto source=supplied.retain_factory_owner();
 need(source&&source.get()==&supplied&&p->train_source&&source.get()==p->train_source.get()&&
  !source.owner_before(p->train_source)&&!p->train_source.owner_before(source),"Completed TRAIN needs actual SAME canonical source");
 p->check();source->require_for_allocator(*p->host,p->host->session(),p->actor);
 // No assigned ID is a full Init receipt. Until real roster/plot/FLTA and
 // remaining mandatory effects publish, the authentic Source cannot advance
 // CityTrained/popOrder. This method never returns an invented unit ID.
 throw BnwNativeUnitAllocatorUnavailable("Completed TRAIN needs full actual CvUnit Init/setXY/publication before CityTrained");
}
std::int32_t BnwNativeUnitAllocatorCore::require_completed_train_create_unit_return(const ui::SelectedCityTrainCreateSource&source)const{
 (void)require_completed_train_unit_id(source);
 throw BnwNativeUnitAllocatorUnavailable("Completed TRAIN needs actual CvCity::CreateUnit validMove/productionXP/rally/operation remainder before CityTrained");
}
std::optional<std::int32_t> BnwNativeUnitAllocatorCore::read_native_existing_unit_id(std::int32_t query_id)const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 const auto*unit=native_imported_get_at(*p->state,p->actor,query_id);
 return unit?std::optional{core(*unit,"m_iID")}:std::nullopt;
}
std::vector<BnwNativeUnitAllocatorCore::InitialFltaOwnerSeal>
BnwNativeUnitAllocatorCore::prepare_initial_destination_flta_lookup(const BnwUnitMovePending& pending)const{
 const auto self=retain_factory_owner();const auto p=impl_;p->check();
 need(p->initial_set_xy.get()==&pending&&p->bank->train,"Native collision manager wrong actual Pending/Train");
 const auto&grid=p->state->world.map_header.grid;const auto&train=*p->bank->train;
 const auto index=grid.index(static_cast<std::uint64_t>(train.x),static_cast<std::uint64_t>(train.y));
 need(p->state->world.map_projection&&p->state->world.map_projection->cells.at(index).units,
  "Native collision actual target roster missing");
 // Copy only owner IDs, finishing allocations before any borrowed unit walk.
 std::array<bool,64> selected{};
 for(const auto&handle:*p->state->world.map_projection->cells.at(index).units){
  need(handle.owner>=0&&handle.owner<64,"Native collision target manager owner invalid");
  selected[static_cast<std::size_t>(handle.owner)]=true;
 }
 std::vector<InitialFltaOwnerSeal> result;result.reserve(64);
 for(std::int32_t actor=0;actor<64;++actor)if(selected[static_cast<std::size_t>(actor)]){
  auto baseline=capture(*p->state,actor);
  const auto&player=p->state->world.all_players->players.at(static_cast<std::size_t>(actor));
  need(player.units.layout==bnw&&player.units.header.layout==bnw&&
   p->state->maintenance.players.at(actor).units.size()==player.units.units.size(),
   "Native collision complete raw/owned manager absent");
  Bank imported;restore(imported,player.units,actor); // Actual existing native partition verifier.
  compare(*p->state,actor,baseline);result.push_back({actor,std::move(baseline)});
 }
 p->check();require_initial_flta_current(pending,result);return result;
}
void BnwNativeUnitAllocatorCore::require_initial_flta_current(const BnwUnitMovePending&pending,
 const std::vector<InitialFltaOwnerSeal>&seals)const{
 const auto p=impl_;p->facade_current();
 need(p->initial_set_xy.get()==&pending&&!p->bank->retired.load(),"Native collision manager Pending retired");
 need(p->bank->train&&p->state->world.map_projection,"Native collision actual target manager source missing");
 const auto&train=*p->bank->train;const auto index=p->state->world.map_header.grid.index(
  static_cast<std::uint64_t>(train.x),static_cast<std::uint64_t>(train.y));
 const auto&target=p->state->world.map_projection->cells.at(index);
 need(target.units.has_value(),"Native collision actual target roster unavailable");
 std::array<bool,64> wanted{},provided{};
 for(const auto&handle:*target.units){need(handle.owner>=0&&handle.owner<64,"Native collision target manager owner invalid");wanted[static_cast<std::size_t>(handle.owner)]=true;}
 for(const auto&seal:seals){need(seal.actor>=0&&seal.actor<64&&!provided[static_cast<std::size_t>(seal.actor)],"Native collision duplicate manager seal");provided[static_cast<std::size_t>(seal.actor)]=true;}
 need(wanted==provided,"Native collision target introduced an unverified manager");
 for(const auto&seal:seals)compare(*p->state,seal.actor,seal.baseline);
 need(p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
  &p->host->session().source()==p->source&&!p->bank->retired.load(),"Native collision manager final SAME source changed");
 p->facade_current(); // Callback-free token; no allocation/query/provider here.
}
const saves::SavedUnitObservation* BnwNativeUnitAllocatorCore::initial_native_unit_at(
 const BnwUnitMovePending&pending,std::int32_t actor,std::int32_t query_id)const{
 const auto p=impl_;need(p->initial_set_xy.get()==&pending&&!p->bank->retired.load(),"Native collision GetAt wrong actual Pending");
 return native_imported_get_at(*p->state,actor,query_id);
}
void BnwNativeUnitAllocatorCore::execute_initial_coordinate_store(const BnwUnitMovePending& pending)const{
 const auto self=retain_factory_owner();const auto p=impl_;auto&b=*p->bank;
 // Invoked by the existing registered Pending while public advance holds
 // Core Guard. No caller-supplied coordinates or successful readiness DTO.
 need(p->initial_set_xy.get()==&pending&&b.constructor_pending&&b.train&&b.pending_units.contains(b.pending_slot),
  "Initial coordinates require actual SAME registered SetXY source");
 const auto u=b.pending_units.at(b.pending_slot);check_registered(b,*u,u,b.pending_slot);
 need(!u->initial_coordinates_executed,"Initial coordinates already executed; no replay");
 PendingUnit staged=*u;staged.registration_owner.reset();staged.canonical_self.reset();staged.registered_slot=-1;staged.self_handle=nullptr;
 assign_scalar(staged,"m_iX",b.train->x);assign_scalar(staged,"m_iY",b.train->y);
 p->check();pending.require_initial_coordinate_store(*self); // Ends with nonalloc collision seal comparison.
 compare(*p->state,p->actor,b.baseline);p->facade_current();check_registered(b,*u,u,b.pending_slot);
 need(!b.retired.load()&&p->host->session().read_state_lease().get()==p->state.get()&&p->host->session().state_generation()==p->generation&&
  !u->initial_coordinates_executed&&u->scalars.at("m_iX")==-INT32_MAX&&u->scalars.at("m_iY")==-INT32_MAX,
  "Initial coordinates source/node changed before stores");
 // SDK13140+: newborn plot() is NULL, so old removal/sight/area/lastMove and
 // direction-from-old-plot branches do not run. Actual coord stores precede
 // the mandatory city GameplayUnitVisibility(false) receiver.
 u->scalars.swap(staged.scalars);u->dirty.swap(staged.dirty);u->initial_coordinates_executed=true;
 pending.record_initial_coordinate_store();
}
std::shared_ptr<const BnwUnitMovePending> BnwNativeUnitAllocatorCore::advance_initial_destination_collision_prefix()const{
 const auto self=retain_factory_owner();const auto p=impl_;Impl::Guard guard(p);p->check();
 need(bool(p->initial_set_xy),"Initial destination collision has no genuine entered SetXY owner");
 p->initial_set_xy->execute_initial_destination_collision_prefix();return p->initial_set_xy;
}
BnwNativeTraitFreeUnitSelection BnwNativeUnitAllocatorCore::first_trait_free_unit(std::int64_t technology)const{
 const auto p=impl_;Impl::Guard guard(p);p->check();need(bool(p->traits),"Mandatory genuine SAME native Traits owner before GetFirstFreeUnit");
 try{p->traits->require_current();auto&b=*p->bank;
  need(!b.iteration_started,"Native free-trait-unit iterator already started");
  need(technology>=0&&p->host->technologies().find(static_cast<std::uint64_t>(technology)),"Actual technology Info missing");
  std::size_t index=0;auto result=select_free_unit(*p->host,*p->state,p->actor,b.trait_units,index,technology);
  p->traits->require_current();p->check();b.iterator=index;b.iterator_tech=technology;b.iteration_started=true;b.selection_pending=result.unit_class!=-1;b.selected_unit=result;
  return result; // selection only; no creation/placement/predecessor completion.
 }catch(...){p->bank->retired=true;throw;}
}
BnwNativeTraitFreeUnitSelection BnwNativeUnitAllocatorCore::next_trait_free_unit()const{
 const auto p=impl_;Impl::Guard guard(p);p->check();need(bool(p->traits),"Mandatory genuine SAME native Traits owner before GetNextFreeUnit");
 try{p->traits->require_current();auto&b=*p->bank;
  need(b.iteration_started,"Native GetFirstFreeUnit must precede GetNextFreeUnit");
  need(!b.selection_pending,"Mandatory genuine free unit Init/placement/notification before GetNextFreeUnit");
  auto index=b.iterator+1;auto result=select_free_unit(*p->host,*p->state,p->actor,b.trait_units,index,b.iterator_tech);
  p->traits->require_current();p->check();b.iterator=index;b.selection_pending=result.unit_class!=-1;b.selected_unit=result;return result;
 }catch(...){p->bank->retired=true;throw;}
}
}

namespace civ5::runtime {
namespace {
// Expected physical operands only, never a cloned world/predicate authority.
struct BusyCoreFieldStamp {
 const void* field=nullptr;const void* values=nullptr;const void* bytes=nullptr;
 std::string member; saves::UnitCoreEncoding encoding{};
 std::vector<std::int64_t> integers;std::vector<std::uint8_t> raw;
};
struct BusyTailStamp {
 const void* node=nullptr;const void* values=nullptr;const void* bytes=nullptr;const void* children=nullptr;
 std::string member,encoding;std::vector<std::int64_t> integers;std::vector<std::uint8_t> raw;std::size_t child_count=0;
};
struct BusyUnitStamp {
 const saves::SavedUnitObservation* unit=nullptr;const void* fields=nullptr;const void* tail=nullptr;
 std::uint64_t start=0,end=0;std::int32_t id=-1,owner=-1;
 std::vector<BusyCoreFieldStamp> core_fields;std::vector<BusyTailStamp> tail_fields;
};
struct BusyManagerStamp {
 const void* player=nullptr;const void* header=nullptr;const void* units=nullptr;
 std::int64_t player_id=-1;std::uint64_t source_slots=0;
 saves::GameStateLayout units_layout{},header_layout{};
 std::vector<std::int64_t> header_values;std::vector<BusyUnitStamp> rows;
};
BusyUnitStamp busy_unit_stamp(const saves::SavedUnitObservation& unit){
 BusyUnitStamp result;result.unit=&unit;result.fields=unit.core.fields.data();result.tail=unit.tail.blocks.data();
 result.start=unit.core.start_offset;result.end=unit.core.end_offset;result.id=narrow(core(unit,"m_iID"));result.owner=narrow(core(unit,"m_eOwner"));
 result.core_fields.reserve(unit.core.fields.size());
 for(const auto& f:unit.core.fields)result.core_fields.push_back({&f,f.values.data(),f.bytes.data(),std::string(name(f.name)),f.encoding,f.values,f.bytes});
 const auto walk=[&](auto&& self,const saves::UnitTailNode& n,std::size_t depth)->void{
  need(depth<64,"Busy unit actual tail recursion unsupported");
  result.tail_fields.push_back({&n,n.values.data(),n.bytes.data(),n.children.data(),std::string(name(n.name)),std::string(name(n.encoding)),n.values,n.bytes,n.children.size()});
  for(const auto& child:n.children)self(self,child,depth+1);
 };for(const auto& n:unit.tail.blocks)walk(walk,n,0);
 return result;
}
void require_busy_unit_stamp(const saves::SavedUnitObservation& unit,const BusyUnitStamp& r){
 need(&unit==r.unit&&unit.core.fields.data()==r.fields&&unit.tail.blocks.data()==r.tail&&
  unit.core.start_offset==r.start&&unit.core.end_offset==r.end&&narrow(core(unit,"m_iID"))==r.id&&narrow(core(unit,"m_eOwner"))==r.owner&&
  unit.core.fields.size()==r.core_fields.size(),"Busy unit physical object/core interval replaced");
 for(std::size_t n=0;n<r.core_fields.size();++n){const auto& f=unit.core.fields[n];const auto& old=r.core_fields[n];
  need(&f==old.field&&f.values.data()==old.values&&f.bytes.data()==old.bytes&&name(f.name)==old.member&&f.encoding==old.encoding&&
   f.values==old.integers&&f.bytes==old.raw,"Busy unit native core operand changed");}
 std::size_t index=0;const auto walk=[&](auto&& self,const saves::UnitTailNode& n,std::size_t depth)->void{
  need(depth<64&&index<r.tail_fields.size(),"Busy unit actual tail shape replaced");const auto& old=r.tail_fields[index++];
  need(&n==old.node&&n.values.data()==old.values&&n.bytes.data()==old.bytes&&n.children.data()==old.children&&
   name(n.name)==old.member&&name(n.encoding)==old.encoding&&n.values==old.integers&&n.bytes==old.raw&&n.children.size()==old.child_count,
   "Busy unit actual combat/tail operand changed");for(const auto& child:n.children)self(self,child,depth+1);
 };for(const auto& n:unit.tail.blocks)walk(walk,n,0);need(index==r.tail_fields.size(),"Busy unit actual tail disappeared");
}
struct BusyRuntimeStamp {
 std::shared_ptr<const ImportedMissionRuntime> node;
 const saves::SavedUnitObservation* unit=nullptr;const saves::SavedCityObservation* combat_city=nullptr;
 std::int32_t id=-1,timer=0,city_owner=-1,city_id=-1;bool city_reached=false,busy=false;
};
}
struct BnwNativePlayerBusyUnitsReturn::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeUnitAllocatorCore> core;std::weak_ptr<const BnwSelectedStockLuaSourceOwner> source;
 std::weak_ptr<const ui::SelectedBnwPreviewHost> host;std::weak_ptr<const LiveSessionState> world;
 std::weak_ptr<const void> control;std::weak_ptr<const BnwNativePlayerBusyUnitsReturn> self;std::weak_ptr<Bank> bank;
 const void* players=nullptr;std::array<BusyManagerStamp,64> managers;
 std::vector<BusyRuntimeStamp> queried;std::uint64_t serial=0,generation=0,revision=0;
 std::int64_t turn=0;std::int32_t actor=-1,required_id=-1;bool completed=false,busy=false,failed=false;
 std::uint64_t grid_width=0,grid_height=0;bool wrap_x=false,wrap_y=false;
 SimulationRandomState map_rng,game_rng;
 void thread()const{need(creator==std::this_thread::get_id(),"Busy predicate return foreign creator");}
 void require_rows(const LiveSessionState& state,std::optional<std::uint64_t> earned_revision={})const{
  need(state.world.all_players&&state.world.all_players->players.size()==64&&state.world.all_players->players.data()==players&&
   state.ui_source_revision==earned_revision.value_or(revision)&&state.setup.game_turn==turn&&state.random.map.algorithm()==RandomAlgorithm::bnw&&
   state.random.gameplay.algorithm()==RandomAlgorithm::bnw&&state.random.map.state()==map_rng&&state.random.gameplay.state()==game_rng&&
   state.world.map_header.grid.width==grid_width&&state.world.map_header.grid.height==grid_height&&
   state.world.map_header.wrap_x==wrap_x&&state.world.map_header.wrap_y==wrap_y,"Busy predicate world/64 roster/clock/RNG changed");
  for(std::size_t n=0;n<64;++n){const auto& p=state.world.all_players->players[n];const auto& r=managers[n];
   need(&p==r.player&&p.saved_player_id==r.player_id&&p.units.layout==r.units_layout&&p.units.header.layout==r.header_layout&&
    p.units.header.fields.data()==r.header&&p.units.units.data()==r.units&&
    p.units.header.source_slots==r.source_slots&&p.units.header.fields.size()==r.header_values.size()&&p.units.units.size()==r.rows.size(),
    "Busy predicate actual native64 manager/storage changed");
   for(std::size_t k=0;k<r.header_values.size();++k)need(p.units.header.fields[k].value==r.header_values[k],"Busy predicate native manager header changed");
   for(std::size_t k=0;k<r.rows.size();++k)require_busy_unit_stamp(p.units.units[k],r.rows[k]);
  }
  const auto b=bank.lock();need(b&&!b->retired.load()&&b->generation==generation&&b->source.lock().get()==&state,"Busy predicate actual Bank retired");
  for(const auto& r:queried){const auto at=b->imported_mission_runtime.find(r.id);
   need(at!=b->imported_mission_runtime.end()&&at->second&&r.node&&at->second.get()==r.node.get()&&
    !at->second.owner_before(r.node)&&!r.node.owner_before(at->second)&&r.node->creator==creator&&r.node->owner.lock().get()==b.get()&&
    r.node->unit==r.unit&&r.node->actor==actor&&r.node->id==r.id&&r.node->generation==generation&&r.node->mission_timer==r.timer&&
    r.node->combat_city_owner==r.city_owner&&r.node->combat_city_id==r.city_id&&native_imported_get_at(state,actor,r.id)==r.unit,
    "Busy predicate canonical plain mission timer/combat node changed");
   if(r.city_reached){const auto* target=(r.city_owner>=0&&r.city_owner<64)?resolve_bnw_city_registry(state.world,state.cities,r.city_owner,r.city_id):nullptr;
    need(target==r.combat_city,"Busy predicate actual combat-city GetAt result changed");}
  }
  if(required_id!=-1)need(b->imported_mission_runtime.find(required_id)==b->imported_mission_runtime.end(),"Busy missing runtime was replaced during source callbacks");
 }
};
BnwNativePlayerBusyUnitsReturn::BnwNativePlayerBusyUnitsReturn(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwNativePlayerBusyUnitsReturn::~BnwNativePlayerBusyUnitsReturn()=default;
void BnwNativePlayerBusyUnitsReturn::require_bound_without_callbacks(const BnwNativeUnitAllocatorCore& core,
 const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,const BnwSelectedStockLuaSourceOwner& source,
 const void* control,std::uint64_t generation,std::uint64_t serial,std::optional<std::uint64_t> earned_revision,const BnwRetainedTurnCursor*,bool)const{
 const auto r=impl_;r->thread();const auto h=r->host.lock();const auto s=r->source.lock();const auto w=r->world.lock();
 const auto scope=r->control.lock();const auto self=r->self.lock();const auto p=core.impl_;
 need(!r->failed&&self&&self.get()==this&&!r->core.expired()&&!r->core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(r->core)&&
  h&&h.get()==&host&&s&&s.get()==&source&&w&&w.get()==&state&&scope&&scope.get()==control&&r->generation==generation&&r->serial==serial&&serial,
  "Busy predicate canonical factory/Source/control/world return changed");
 core.require_live_gold_roster_source_without_callbacks(host,state,r->actor);
 need(p&&p->state.get()==&state&&p->bank.get()==r->bank.lock().get()&&p->generation==generation&&
  p->player_turn_busy_returns.contains(r->turn)&&!p->player_turn_busy_returns.at(r->turn).owner_before(r->self)&&
  !r->self.owner_before(p->player_turn_busy_returns.at(r->turn)),"Busy predicate Core-issued once registry/realm changed");
 r->require_rows(state,earned_revision);
 need(!r->completed||r->required_id==-1,"Busy predicate missing runtime was credited as returned");
 need(!r->busy||(r->completed&&!r->queried.empty()&&r->queried.back().busy),"Busy predicate true result lacks actual queried unit");
}
void BnwNativePlayerBusyUnitsReturn::require_current_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial)const{
 require_bound_without_callbacks(core,host,state,source,control,generation,serial,{},nullptr);
}
void BnwNativePlayerBusyUnitsReturn::require_after_busy_end_turn_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor)const{
 impl_->thread();cursor.require_player_end_after_busy_transition_without_callbacks(host,state,source,impl_->revision,impl_->actor);
 require_bound_without_callbacks(core,host,state,source,control,generation,serial,state.ui_source_revision,&cursor);
}
void BnwNativePlayerBusyUnitsReturn::require_after_processed_auto_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor)const{
 impl_->thread();cursor.require_processed_auto_transition_without_callbacks(host,state,source,impl_->revision,impl_->actor);
 require_bound_without_callbacks(core,host,state,source,control,generation,serial,state.ui_source_revision,&cursor,true);
}
void BnwNativePlayerBusyUnitsReturn::quarantine_without_callbacks()const noexcept{
 const auto r=impl_;if(r->creator!=std::this_thread::get_id())std::terminate();r->failed=true;
}
bool BnwNativePlayerBusyUnitsReturn::completed_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return impl_->completed;}
bool BnwNativePlayerBusyUnitsReturn::busy_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return impl_->busy;}
std::uint64_t BnwNativePlayerBusyUnitsReturn::queried_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return static_cast<std::uint64_t>(impl_->queried.size());}
std::int32_t BnwNativePlayerBusyUnitsReturn::required_id_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return impl_->required_id;}
void BnwNativeUnitAllocatorCore::query_player_turn_busy_units_owned(const ui::SelectedBnwPreviewHost& host,const LiveSessionState& state,
 std::shared_ptr<const BnwSelectedStockLuaSourceOwner> source,std::weak_ptr<const void> control,std::uint64_t serial,std::int32_t actor,
 std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>& issued)const{
 // Counted genuine facade borrow BEFORE copying any currentness providers.
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 require_live_gold_roster_source_without_callbacks(host,state,actor);
 need(source&&!control.expired()&&serial&&actor==p->actor&&!issued&&!p->player_turn_busy_returns.contains(state.setup.game_turn),
  "Busy predicate factory requires genuine Source/realm/once reservation");
 auto r=std::make_shared<BnwNativePlayerBusyUnitsReturn::Impl>();r->core=factory_self_;r->source=source;r->host=p->host;r->world=p->state;
 r->control=control;r->bank=p->bank;r->serial=serial;r->generation=p->generation;r->revision=state.ui_source_revision;r->turn=state.setup.game_turn;r->actor=actor;
 r->grid_width=state.world.map_header.grid.width;r->grid_height=state.world.map_header.grid.height;r->wrap_x=state.world.map_header.wrap_x;r->wrap_y=state.world.map_header.wrap_y;
 r->map_rng=state.random.map.state();r->game_rng=state.random.gameplay.state();r->players=state.world.all_players->players.data();
 issued=std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>(new BnwNativePlayerBusyUnitsReturn(r));r->self=issued;
 try{
  p->player_turn_busy_returns.emplace(r->turn,issued); // sticky issued reservation even if later source callbacks fail
  // Allocate every expected raw operand before the first native predicate read.
  for(std::size_t n=0;n<64;++n){const auto& raw=state.world.all_players->players[n];auto& m=r->managers[n];
   m.player=&raw;m.player_id=raw.saved_player_id;m.units_layout=raw.units.layout;m.header_layout=raw.units.header.layout;m.header=raw.units.header.fields.data();m.units=raw.units.units.data();m.source_slots=raw.units.header.source_slots;
   m.header_values.reserve(raw.units.header.fields.size());for(const auto& f:raw.units.header.fields)m.header_values.push_back(f.value);
   m.rows.reserve(raw.units.units.size());for(const auto& u:raw.units.units)m.rows.push_back(busy_unit_stamp(u));
  }
  r->queried.reserve(state.world.all_players->players[static_cast<std::size_t>(actor)].units.units.size());
  {Impl::Guard invocation(p);p->check();(void)issuer->current();r->require_rows(state);
   for(std::int32_t slot=0;slot<=p->bank->last;++slot){const auto* unit=native_imported_get_at(state,actor,slot);if(!unit)continue;
    const auto id=narrow(core(*unit,"m_iID"));need((id&8191)==slot&&narrow(core(*unit,"m_eOwner"))==actor&&native_imported_get_at(state,actor,id)==unit,
     "Busy predicate first/nextUnit actual fullID/owner differs");
    const auto at=p->bank->imported_mission_runtime.find(id);
    if(at==p->bank->imported_mission_runtime.end()){r->required_id=id;break;} // actual unsupported typed runtime, never saved/default timer
    need(at->second&&at->second->creator==r->creator&&at->second->owner.lock().get()==p->bank.get()&&at->second->unit==unit&&
     at->second->actor==actor&&at->second->id==id&&at->second->generation==p->generation,"Busy predicate OriginalRead runtime source changed");
    BusyRuntimeStamp staged;staged.node=at->second;staged.unit=unit;staged.id=id;staged.timer=staged.node->mission_timer;
    staged.city_owner=staged.node->combat_city_owner;staged.city_id=staged.node->combat_city_id;
    r->queried.push_back(std::move(staged)); // earned timer read survives an isInCombat failure
    auto& read=r->queried.back();
    // Exact original timer FIRST, then real isInCombat on false. No saved
    // timer/default empty node can supply this private query result.
    if(read.timer>0)read.busy=true;
    else{
     const auto owner=native_friendly_tail(*unit,"m_combatUnit.eOwner"),combat=native_friendly_tail(*unit,"m_combatUnit.iID");
     if(!(owner>=0&&owner<64&&native_imported_get_at(state,owner,combat))){
      read.city_reached=true;if(read.city_owner>=0&&read.city_owner<64)read.combat_city=resolve_bnw_city_registry(state.world,state.cities,read.city_owner,read.city_id);
     }
     read.busy=existing_native_is_in_combat(*unit);
    }
    if(read.busy){r->busy=true;r->completed=true;break;}
   }
   if(r->required_id==-1&&!r->completed)r->completed=true;
   r->require_rows(state);p->check();(void)issuer->current();r->require_rows(state);
  }
  const auto held_control=control.lock();need(held_control!=nullptr,"Busy predicate source scope retired after real query");
  issued->require_current_without_callbacks(*this,host,state,*source,held_control.get(),p->generation,serial);
 }catch(...){r->failed=true;p->bank->retired=true;throw;}
}
}

namespace civ5::runtime {
namespace {
struct BusyCityStamp {
 const saves::SavedCityObservation*city=nullptr;const void*fields=nullptr;std::int32_t id=-1,owner=-1;
 std::uint64_t start=0,end=0;const CityYieldState*owned=nullptr;
};
struct BusyCityManagerStamp {
 const void*rows=nullptr;const void*header=nullptr;const void*next=nullptr;
 std::uint64_t slots=0;std::int64_t count=0;std::vector<std::int64_t>values,next_values;std::vector<BusyCityStamp>cities;
};
struct BusyCityRuntimeStamp {
 std::shared_ptr<const ImportedCityCombatRuntime>node;const saves::SavedCityObservation*city=nullptr;
 const saves::SavedUnitObservation*combat=nullptr;std::int32_t id=-1,owner=-1,unit=-1;bool busy=false;
};
}
struct BnwNativePlayerBusyCitiesReturn::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeUnitAllocatorCore>core;std::weak_ptr<const BnwSelectedStockLuaSourceOwner>source;
 std::weak_ptr<const ui::SelectedBnwPreviewHost>host;std::weak_ptr<const LiveSessionState>world;
 std::weak_ptr<const void>control;std::weak_ptr<const BnwNativePlayerBusyCitiesReturn>self;std::weak_ptr<Bank>bank;
 std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>units;std::array<BusyCityManagerStamp,64>managers;
 std::vector<BusyCityRuntimeStamp>queried;std::uint64_t serial=0,generation=0,revision=0;std::int64_t turn=0;
 std::int32_t actor=-1,required_id=-1;bool completed=false,busy=false,failed=false;
 void thread()const{need(creator==std::this_thread::get_id(),"City busy foreign creator");}
 void require_rows(const LiveSessionState&state,std::optional<std::uint64_t> earned_revision={})const{
  need(units&&units->completed_without_callbacks()&&!units->busy_without_callbacks(),"City busy real hasBusyUnit false predecessor absent");
  units->impl_->require_rows(state,earned_revision); // typed exact EndTurn history or ordinary strict current revision
  need(state.world.all_players&&state.world.all_players->players.size()==64&&state.ui_source_revision==earned_revision.value_or(revision)&&state.setup.game_turn==turn,
   "City busy current world/clock changed");
  for(std::size_t n=0;n<64;++n){const auto&raw=state.world.all_players->players[n].cities;const auto&m=managers[n];
   need(raw.layout==bnw&&raw.header.layout==bnw&&raw.cities.data()==m.rows&&raw.header.fields.data()==m.header&&
    raw.header.next_free_indices.data()==m.next&&raw.header.source_slots==m.slots&&raw.header.wire_city_count==m.count&&
    raw.header.fields.size()==m.values.size()&&raw.header.next_free_indices==m.next_values&&raw.cities.size()==m.cities.size(),"City busy actual64 FLTA storage changed");
   for(std::size_t k=0;k<m.values.size();++k)need(raw.header.fields[k].value==m.values[k],"City busy native FLTA header changed");
   for(std::size_t k=0;k<m.cities.size();++k){const auto&c=raw.cities[k];const auto&old=m.cities[k];
    need(&c==old.city&&c.core.fields.data()==old.fields&&c.core.start_offset==old.start&&c.core.end_offset==old.end&&
     busy_city_scalar(c,"m_iID")==old.id&&busy_city_scalar(c,"m_eOwner")==old.owner&&
     resolve_bnw_city_registry(state.world,state.cities,static_cast<std::int32_t>(n),old.id)==&c&&
     resolve_bnw_city_yield_registry(state.world,state.cities,static_cast<std::int32_t>(n),old.id)==old.owned,"City busy actual city fullID/owned registry replaced");}
  }
  const auto b=bank.lock();need(b&&!b->retired.load()&&b->imported_city_combat_registered&&b->generation==generation&&b->source.lock().get()==&state,
   "City busy original combat owner unavailable");
  for(const auto&r:queried){const auto at=b->imported_city_combat_runtime.find(r.id);need(at!=b->imported_city_combat_runtime.end()&&at->second&&r.node&&
   at->second.get()==r.node.get()&&!at->second.owner_before(r.node)&&!r.node.owner_before(at->second)&&r.node->creator==creator&&
   r.node->owner.lock().get()==b.get()&&r.node->city==r.city&&r.node->actor==actor&&r.node->id==r.id&&r.node->generation==generation&&
   r.node->combat_owner==r.owner&&r.node->combat_id==r.unit&&resolve_bnw_city_registry(state.world,state.cities,actor,r.id)==r.city,
   "City busy SAME actual plain combat node changed");
   const auto*combat=(r.owner>=0&&r.owner<64)?native_imported_get_at(state,r.owner,r.unit):nullptr;
   need(combat==r.combat&&r.busy==(combat!=nullptr),"City busy actual getUnit return changed");
  }
  if(required_id!=-1)need(!b->imported_city_combat_runtime.contains(required_id),"City busy missing native runtime changed during callback");
 }
};
BnwNativePlayerBusyCitiesReturn::BnwNativePlayerBusyCitiesReturn(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwNativePlayerBusyCitiesReturn::~BnwNativePlayerBusyCitiesReturn()=default;
void BnwNativePlayerBusyCitiesReturn::require_bound_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,std::optional<std::uint64_t> earned_revision,const BnwRetainedTurnCursor* cursor,bool after_auto)const{
 const auto r=impl_;r->thread();const auto h=r->host.lock();const auto s=r->source.lock();const auto w=r->world.lock();
 const auto scope=r->control.lock();const auto self=r->self.lock();const auto p=core.impl_;
 need(!r->failed&&self&&self.get()==this&&!r->core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(r->core)&&
  h&&h.get()==&host&&s&&s.get()==&source&&w&&w.get()==&state&&scope&&scope.get()==control&&r->generation==generation&&r->serial==serial&&serial&&
  p&&p->bank.get()==r->bank.lock().get()&&p->player_turn_busy_city_returns.contains(r->turn)&&
  !p->player_turn_busy_city_returns.at(r->turn).owner_before(r->self)&&!r->self.owner_before(p->player_turn_busy_city_returns.at(r->turn)),
  "City busy canonical Core/Source/control/once return changed");
 core.require_live_gold_roster_source_without_callbacks(host,state,r->actor);
 need(r->units&&r->units->completed_without_callbacks()&&!r->units->busy_without_callbacks(),"City busy lacks actual hasBusyUnit false");
 if(cursor&&after_auto)r->units->require_after_processed_auto_without_callbacks(core,host,state,source,control,generation,r->units->impl_->serial,*cursor);
 else if(cursor)r->units->require_after_busy_end_turn_without_callbacks(core,host,state,source,control,generation,r->units->impl_->serial,*cursor);
 else r->units->require_current_without_callbacks(core,host,state,source,control,generation,r->units->impl_->serial);
 r->require_rows(state,earned_revision);need(!r->completed||r->required_id==-1,"City busy missing runtime credited false");
 need(!r->busy||(r->completed&&!r->queried.empty()&&r->queried.back().busy),"City busy true lacks actual combat GetAt");
}
void BnwNativePlayerBusyCitiesReturn::require_current_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial)const{
 require_bound_without_callbacks(core,host,state,source,control,generation,serial,{},nullptr);
}
void BnwNativePlayerBusyCitiesReturn::require_after_busy_end_turn_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor)const{
 impl_->thread();cursor.require_player_end_after_busy_transition_without_callbacks(host,state,source,impl_->revision,impl_->actor);
 require_bound_without_callbacks(core,host,state,source,control,generation,serial,state.ui_source_revision,&cursor);
}
void BnwNativePlayerBusyCitiesReturn::require_after_processed_auto_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,const BnwRetainedTurnCursor&cursor)const{
 impl_->thread();cursor.require_processed_auto_transition_without_callbacks(host,state,source,impl_->revision,impl_->actor);
 require_bound_without_callbacks(core,host,state,source,control,generation,serial,state.ui_source_revision,&cursor,true);
}
void BnwNativePlayerBusyCitiesReturn::quarantine_without_callbacks()const noexcept{const auto r=impl_;if(r->creator!=std::this_thread::get_id())std::terminate();r->failed=true;}
bool BnwNativePlayerBusyCitiesReturn::completed_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return impl_->completed;}
bool BnwNativePlayerBusyCitiesReturn::busy_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return impl_->busy;}
std::uint64_t BnwNativePlayerBusyCitiesReturn::queried_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return impl_->queried.size();}
std::int32_t BnwNativePlayerBusyCitiesReturn::required_id_without_callbacks()const noexcept{if(impl_->creator!=std::this_thread::get_id())std::terminate();return impl_->required_id;}
void BnwNativeUnitAllocatorCore::query_player_turn_busy_cities_owned(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&state,
 std::shared_ptr<const BnwSelectedStockLuaSourceOwner>source,std::weak_ptr<const void>control,std::uint64_t serial,std::int32_t actor,
 std::shared_ptr<const BnwNativePlayerBusyUnitsReturn>units,std::shared_ptr<const BnwNativePlayerBusyCitiesReturn>&issued)const{
 const auto issuer=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 require_live_gold_roster_source_without_callbacks(host,state,actor);const auto scope=control.lock();
 need(source&&scope&&serial&&actor==p->actor&&!issued&&!p->player_turn_busy_city_returns.contains(state.setup.game_turn)&&units&&
  units->completed_without_callbacks()&&!units->busy_without_callbacks(),"City busy requires actual once hasBusyUnit false predecessor");
 units->require_current_without_callbacks(*this,host,state,*source,scope.get(),p->generation,units->impl_->serial);
 auto r=std::make_shared<BnwNativePlayerBusyCitiesReturn::Impl>();r->core=factory_self_;r->source=source;r->host=p->host;r->world=p->state;
 r->control=control;r->bank=p->bank;r->units=std::move(units);r->serial=serial;r->generation=p->generation;r->revision=state.ui_source_revision;r->turn=state.setup.game_turn;r->actor=actor;
 issued=std::shared_ptr<const BnwNativePlayerBusyCitiesReturn>(new BnwNativePlayerBusyCitiesReturn(r));r->self=issued;
 try{p->player_turn_busy_city_returns.emplace(r->turn,issued);
  for(std::size_t n=0;n<64;++n){const auto&raw=state.world.all_players->players[n].cities;auto&m=r->managers[n];
   m.rows=raw.cities.data();m.header=raw.header.fields.data();m.next=raw.header.next_free_indices.data();m.slots=raw.header.source_slots;m.count=raw.header.wire_city_count;
   for(const auto&f:raw.header.fields)m.values.push_back(f.value);m.next_values=raw.header.next_free_indices;m.cities.reserve(raw.cities.size());
   for(const auto&c:raw.cities){const auto id=busy_city_scalar(c,"m_iID");m.cities.push_back({&c,c.core.fields.data(),id,busy_city_scalar(c,"m_eOwner"),c.core.start_offset,c.core.end_offset,
    resolve_bnw_city_yield_registry(state.world,state.cities,static_cast<std::int32_t>(n),id)});}
  }
  r->queried.reserve(state.world.all_players->players[actor].cities.cities.size());
  {Impl::Guard invocation(p);p->check();(void)issuer->current();r->require_rows(state);
   const auto&raw=state.world.all_players->players[actor].cities;std::int64_t last=-1;bool found=false;
   for(const auto&f:raw.header.fields)if(name(f.name)=="last_index"){need(!found,"City busy duplicate last_index");last=f.value;found=true;}
   need(found&&last>=-1&&last<=8191,"City busy actual FLTA iteration extent unavailable");
   // firstCity/nextCity ascending occupied slots, never serialized row ordinal.
   for(std::int32_t slot=0;slot<=last;++slot){const auto*city=resolve_bnw_city_registry(state.world,state.cities,actor,slot);if(!city)continue;
    const auto id=busy_city_scalar(*city,"m_iID");const auto at=p->bank->imported_city_combat_runtime.find(id);
    if(at==p->bank->imported_city_combat_runtime.end()){r->required_id=id;break;}
    need(at->second&&at->second->creator==r->creator&&at->second->city==city&&at->second->owner.lock().get()==p->bank.get(),"City busy actual runtime replaced");
    const auto node=at->second;BusyCityRuntimeStamp read;read.node=node;read.city=city;read.id=id;read.owner=node->combat_owner;read.unit=node->combat_id;
    // Exact native ::getUnit owner guard then actual fullID GetAt (stale NULL).
    if(read.owner>=0&&read.owner<64)read.combat=native_imported_get_at(state,read.owner,read.unit);
    read.busy=read.combat!=nullptr;r->queried.push_back(std::move(read));
    if(r->queried.back().busy){r->busy=true;r->completed=true;break;}
   }
   if(r->required_id==-1&&!r->completed)r->completed=true;
   r->require_rows(state);p->check();(void)issuer->current();r->require_rows(state);
  }
  issued->require_current_without_callbacks(*this,host,state,*source,scope.get(),p->generation,serial);
 }catch(...){r->failed=true;p->bank->retired=true;throw;}
}
}

namespace civ5::runtime {
namespace {
struct AttritionTerritoryRead {
 std::vector<std::int64_t>inputs;std::optional<std::int32_t>id,slot;
 bool draw=false,enemy=false,empty=false;std::int32_t chance=0,damage=0;
 BnwAttritionPrefixBoundary boundary=BnwAttritionPrefixBoundary::mountain_domain_owner;
 bool operator==(const AttritionTerritoryRead&)const=default;
};
AttritionTerritoryRead actual_attrition_first_read(const LiveSessionState&s,const Bank&bank,std::int32_t actor,const AttritionTerritoryRead*expected=nullptr){
 AttritionTerritoryRead q;std::size_t read=0;
 const auto emit=[&](auto n){if(expected){need(read<expected->inputs.size()&&expected->inputs[read]==static_cast<std::int64_t>(n),
  "Attrition reached semantic operand changed");++read;}else q.inputs.push_back(static_cast<std::int64_t>(n));};
 const auto finish=[&](){if(expected)need(read==expected->inputs.size()&&q.id==expected->id&&q.slot==expected->slot&&
  q.draw==expected->draw&&q.enemy==expected->enemy&&q.empty==expected->empty&&q.chance==expected->chance&&q.damage==expected->damage&&
  q.boundary==expected->boundary,"Attrition reached branch/operand count changed");return std::move(q);};
 emit(actor);emit(bank.last);const saves::SavedUnitObservation*unit=nullptr;
 // SDK firstUnit/nextUnit default bRev=false: ascending native FLTA slots,
 // including sparse holes; serialized DTO vector order is not native order.
 for(std::int32_t slot=0;slot<=bank.last;++slot){unit=native_imported_get_at(s,actor,slot);if(!unit)continue;
  q.slot=slot;q.id=narrow(core(*unit,"m_iID"));need((*q.id&8191)==slot&&narrow(core(*unit,"m_eOwner"))==actor&&
   native_imported_get_at(s,actor,*q.id)==unit,"Attrition genuine BeginIter/full-ID/owner differs");break;}
 if(!unit){q.empty=true;emit(detail::bnw_game_active_player_read(s));
  q.boundary=detail::bnw_game_active_player_read(s)==actor?BnwAttritionPrefixBoundary::active_deferred_fog_owner:BnwAttritionPrefixBoundary::active_count_owner;return finish();}
 emit(*q.id);emit(*q.slot);emit(reinterpret_cast<std::uintptr_t>(unit));
 const auto x=narrow(core(*unit,"m_iX")),y=narrow(core(*unit,"m_iY"));emit(x);emit(y);
 const auto&map=s.world.map_header;const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};
 const auto at=geometry.plot(x,y);need(at.has_value(),"Attrition actual unit plot() is NULL");emit(*at);
 const auto&plot=native_path_plot(s,*at);const auto&cell=s.world.map_projection->cells.at(*at);
 emit(reinterpret_cast<std::uintptr_t>(&plot));emit(reinterpret_cast<std::uintptr_t>(&cell));
 const auto owner=narrow(post_named(plot.fields,"owner").value);emit(owner);need(owner>=-1&&owner<64&&cell.owner==std::optional<std::int64_t>{owner},
  "Attrition actual plot owner raw/projected mirrors differ");const auto team=post_player_team(s,actor);emit(team);
 const auto plot_team=owner==-1?-1:post_player_team(s,owner);emit(plot_team);
 bool friendly=false;
 const auto minor=[&](std::int32_t player){need(s.world.pregame&&s.world.pregame->layout==bnw,"Attrition actual Pregame absent");
  const auto&flags=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(flags.size()==64&&s.cities.players.contains(player)&&
   flags.at(player)==s.cities.players.at(player).minor,"Attrition actual Minor player mirrors differ");emit(player);emit(flags.at(player));return bool(flags.at(player));};
 const auto minor_team=[&](std::int32_t target){for(std::int32_t n=22;n<64;++n){const auto alive=post_player(s,n,"m_bAlive");emit(n);emit(alive);
   if(!alive)continue;const auto t=post_player_team(s,n);emit(t);if(t==target)return minor(n);}return false;};
 // CvPlot3425 original barbarian/NO_TEAM/same-team/minor-OB/major-OB order.
 emit(actor==63);if(actor!=63&&plot_team!=-1){if(plot_team==team)friendly=true;
  else{if(!minor(actor)&&minor_team(plot_team)){
    q.boundary=BnwAttritionPrefixBoundary::minor_open_borders_owner;return finish(); // mandatory real MinorAI owner, not guessed false
   }friendly=post_relation(s,plot_team,team,"open_borders");emit(friendly);}}
 emit(friendly);if(friendly)return finish();
 // CvUnit::isEnemy uses actual combat-owner/AlwaysHostile city condition;
 // NO_TEAM short-circuits atWar. Do not manufacture barbarian for NO_TEAM.
 if(plot_team!=-1){const auto combat=native_path_combat_team(s,*unit,plot_team,*at);emit(combat);q.enemy=post_relation(s,combat,plot_team,"at_war");}
 emit(q.enemy);
 if(q.enemy){const auto chance=native_friendly_tail(*unit,"m_iEnemyDamageChance");emit(chance);
  if(chance>0){const auto damage=native_friendly_tail(*unit,"m_iEnemyDamage");emit(damage);if(damage>0){q.draw=true;q.chance=chance;q.damage=damage;return finish();}}}
 // This else-if is reached when the entire enemy AND-condition is false,
 // never as a fallback after an enemy JonRand chance miss.
 const auto chance=native_friendly_tail(*unit,"m_iNeutralDamageChance");emit(chance);
 if(chance>0){const auto damage=native_friendly_tail(*unit,"m_iNeutralDamage");emit(damage);if(damage>0){q.draw=true;q.enemy=false;q.chance=chance;q.damage=damage;}}
 return finish();
}
}
namespace {
struct AttritionDomainInfo {
 const content::InfoTypeEntry*unit_type=nullptr;const content::InfoTypeEntry*domain_type=nullptr;
 std::int32_t type=-1,domain=-1;std::uint64_t hash=0;
 bool operator==(const AttritionDomainInfo&)const=default;
};
AttritionDomainInfo actual_attrition_domain_info(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const saves::SavedUnitObservation&u){
 AttritionDomainInfo q;const auto actor=narrow(core(u,"m_eOwner")),id=narrow(core(u,"m_iID"));const auto hash=core(u,"idUnitType");
 need(hash>0&&hash<=UINT32_MAX,"Attrition mountain actual UnitInfo hash absent");q.hash=static_cast<std::uint64_t>(hash);q.unit_type=host.types().find_hash(q.hash);
 need(q.unit_type&&q.unit_type->table=="Units"&&q.unit_type->id>=0&&q.unit_type->hash==content::saved_info_type_hash(q.unit_type->type),
  "Attrition mountain actual UnitInfo source unavailable");q.type=narrow(q.unit_type->id);
 const auto&owned=s.maintenance.players.at(actor).units.at(id);need(owned.type==q.type&&owned.source_core==std::optional{std::pair{u.core.start_offset,u.core.end_offset}},
  "Attrition mountain raw/owned native UnitInfo identity differs");bool found=false;
 const auto report=host.database().query_rows({"SELECT ID,Type,Domain FROM Units WHERE ID=?",{std::int64_t(q.type)}},[&](const auto&r){
  need(!found&&r.integer("ID")==q.type&&r.text("Type")==std::optional<std::string>{q.unit_type->type},"Attrition mountain actual UnitInfo row differs");found=true;
  const auto domain=r.text("Domain");q.domain_type=domain?host.types().find(*domain):nullptr;
  constexpr std::array<std::string_view,5>names{"DOMAIN_SEA","DOMAIN_AIR","DOMAIN_LAND","DOMAIN_IMMOBILE","DOMAIN_HOVER"};
  need(q.domain_type&&q.domain_type->table=="Domains"&&q.domain_type->id>=0&&q.domain_type->id<5&&q.domain_type->type==names.at(q.domain_type->id)&&
   q.domain_type->hash==content::saved_info_type_hash(q.domain_type->type),"Attrition mountain actual domain enum differs");q.domain=narrow(q.domain_type->id);
 });need(report.all_succeeded()&&found,"Attrition mountain genuine UnitInfo read failed");return q;
}
struct AttritionMountainRead {
 AttritionDomainInfo info;std::vector<std::int64_t>inputs;
 std::int32_t actor=-1,id=-1,domain=-1;std::optional<std::int32_t>plot_type,all_terrain_count;bool damage=false;
};
AttritionMountainRead actual_attrition_mountain_read(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const Bank&bank,
 std::int32_t actor,std::int32_t id,const AttritionDomainInfo&info,const AttritionMountainRead*expected=nullptr){
 AttritionMountainRead q;q.actor=actor;q.id=id;q.info=info;std::size_t read=0;
 const auto emit=[&](auto n){if(expected){need(read<expected->inputs.size()&&expected->inputs[read]==static_cast<std::int64_t>(n),
  "Attrition mountain reached native operand differs");++read;}else q.inputs.push_back(static_cast<std::int64_t>(n));};
 const auto finish=[&](){if(expected)need(read==expected->inputs.size()&&q.actor==expected->actor&&q.id==expected->id&&q.info==expected->info&&
  q.domain==expected->domain&&q.plot_type==expected->plot_type&&q.all_terrain_count==expected->all_terrain_count&&q.damage==expected->damage,
  "Attrition mountain reached native branch/count differs");return std::move(q);};
 const auto*u=native_imported_get_at(s,actor,id);need(u&&narrow(core(*u,"m_iID"))==id&&narrow(core(*u,"m_eOwner"))==actor&&
  (id&8191)<=bank.last,"Attrition mountain SAME real first FLTA unit unavailable");emit(actor);emit(id);emit(reinterpret_cast<std::uintptr_t>(u));
 const auto hash=core(*u,"idUnitType");need(hash==static_cast<std::int64_t>(info.hash)&&host.types().find_hash(info.hash)==info.unit_type&&
  info.unit_type&&info.unit_type->table=="Units"&&info.unit_type->id==info.type&&info.unit_type->hash==info.hash&&
  info.domain_type&&host.types().find(info.domain_type->type)==info.domain_type&&info.domain_type->table=="Domains"&&info.domain_type->id==info.domain,
  "Attrition mountain sealed actual immutable UnitInfo source differs");
 const auto&owned=s.maintenance.players.at(actor).units.at(id);need(owned.type==info.type&&owned.source_core==std::optional{std::pair{u->core.start_offset,u->core.end_offset}},
  "Attrition mountain actual raw/owned info mirror differs");emit(hash);emit(info.type);emit(info.domain);
 emit(reinterpret_cast<std::uintptr_t>(info.unit_type));emit(reinterpret_cast<std::uintptr_t>(info.domain_type));
 q.domain=info.domain;
 const auto read_plot_kind=[&](){const auto x=narrow(core(*u,"m_iX")),y=narrow(core(*u,"m_iY"));emit(x);emit(y);
  const MapGeometry geometry{s.world.map_header.grid,s.world.map_header.wrap_x,s.world.map_header.wrap_y,bnw};const auto at=geometry.plot(x,y);
  need(at.has_value(),"Attrition mountain original plot() NULL");const auto&plot=native_path_plot(s,*at);const auto&cell=s.world.map_projection->cells.at(*at);
  const auto kind=narrow(post_named(plot.fields,"plot_type").value);need(kind>=0&&kind<4&&cell.plot_type==std::optional<std::int64_t>{kind},
   "Attrition mountain actual plot kind/mirror differs");emit(*at);emit(reinterpret_cast<std::uintptr_t>(&plot));emit(reinterpret_cast<std::uintptr_t>(&cell));emit(kind);return kind;};
 // SDK9457 getDomainType: HOVER alone queries current plot->isWater.
 if(q.domain==4){const auto kind=read_plot_kind();q.domain=kind==3?0:2;}
 emit(q.domain);if(q.domain!=2)return finish();
 q.plot_type=read_plot_kind();if(*q.plot_type!=0)return finish();
 // SDK11803 native member getter, only reached for LAND + MOUNTAIN.
 q.all_terrain_count=narrow(core(*u,"m_iCanMoveAllTerrainCount"));emit(*q.all_terrain_count);q.damage=*q.all_terrain_count<=0;return finish();
}
#include "bnw_attrition_damage_storage.inc"
#include "bnw_attrition_garrison_storage.inc"
#include "bnw_attrition_city_strength_storage.inc"
struct AttritionMountainChild {
 AttritionMountainRead reached;std::uint64_t serial=0;bool executed=false,failed=false;
};
}
struct BnwNativeAttritionPrefix::Impl {
 const std::thread::id creator=std::this_thread::get_id();
 std::weak_ptr<const BnwNativeUnitAllocatorCore>core;std::weak_ptr<const BnwSelectedStockLuaSourceOwner>source;
 std::weak_ptr<const ui::SelectedBnwPreviewHost>host;std::weak_ptr<const LiveSessionState>world;
 std::weak_ptr<const void>control;std::weak_ptr<const BnwNativeAttritionPrefix>self;std::weak_ptr<Bank>bank;
 std::array<BusyManagerStamp,64>managers;const void*players=nullptr;AttritionTerritoryRead reached;
 std::uint64_t generation=0,serial=0,before=0,after=0;std::int64_t turn=0;std::int32_t actor=-1;
 SimulationRandomState map_rng,before_rng,after_rng;std::optional<std::uint64_t>roll;
 bool executed=false,rng_stored=false,outer_returned=false,failed=false;
 std::unique_ptr<AttritionMountainChild>mountain;
 std::unique_ptr<AttritionDamageChild>damage;
 std::unique_ptr<AttritionGarrisonChild>garrison;
 std::unique_ptr<AttritionCityStrengthChild>city_strength;
 std::shared_ptr<BnwAttritionCityInfoDirtyLoan>city_info;
 std::shared_ptr<BnwAttritionUnitInfoDirtyLoan>unit_info;
 std::shared_ptr<BnwAttritionUnitSelectedRead>unit_selected;
 std::shared_ptr<BnwAttritionSelectedUnitInfoDirtyLoan>selected_unit_info;
 std::shared_ptr<BnwAttritionSelectionPlotRead>selection_plot;
 std::shared_ptr<BnwAttritionPlotListDirtyLoan>plot_list;
 std::shared_ptr<BnwAttritionDamageDllWrapperPrefix>dll_wrapper;
 void thread()const{need(creator==std::this_thread::get_id(),"Attrition prefix foreign immutable creator");}
 void require_rows(const LiveSessionState&s,bool after_execution)const{
  need(s.world.all_players&&s.world.all_players->players.size()==64&&s.world.all_players->players.data()==players&&
   s.ui_source_revision==(after_execution?after:before)&&s.setup.game_turn==turn&&s.random.map.algorithm()==RandomAlgorithm::bnw&&
   s.random.gameplay.algorithm()==RandomAlgorithm::bnw&&s.random.map.state()==map_rng&&
   s.random.gameplay.state()==(after_execution?after_rng:before_rng),"Attrition exact native64/world/clock/RNG prefix changed");
  for(std::size_t n=0;n<64;++n){const auto&p=s.world.all_players->players[n];const auto&r=managers[n];
   need(&p==r.player&&p.saved_player_id==r.player_id&&p.units.layout==r.units_layout&&p.units.header.layout==r.header_layout&&
    p.units.header.fields.data()==r.header&&p.units.units.data()==r.units&&p.units.header.source_slots==r.source_slots&&
    p.units.header.fields.size()==r.header_values.size()&&p.units.units.size()==r.rows.size(),"Attrition actual FLTA/native64 storage differs");
   for(std::size_t k=0;k<r.header_values.size();++k)need(p.units.header.fields[k].value==r.header_values[k],"Attrition native manager header changed");
   for(std::size_t k=0;k<r.rows.size();++k)require_busy_unit_stamp(p.units.units[k],r.rows[k]);}
  const auto b=bank.lock();need(b&&!b->retired.load()&&b->source.lock().get()==&s&&b->generation==generation,
   "Attrition genuine imported Bank retired");(void)actual_attrition_first_read(s,*b,actor,&reached);
 }
};
BnwNativeAttritionPrefix::BnwNativeAttritionPrefix(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwNativeAttritionPrefix::~BnwNativeAttritionPrefix()=default;
void BnwNativeAttritionPrefix::require_current_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,
 const LiveSessionState&s,const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial,bool executed)const{
 const auto r=impl_;r->thread();const auto p=core.impl_;const auto h=r->host.lock();const auto w=r->world.lock();const auto issuer=r->source.lock();
 const auto scope=r->control.lock();const auto self=r->self.lock();
 need(!r->failed&&self&&self.get()==this&&h&&h.get()==&host&&w&&w.get()==&s&&issuer&&issuer.get()==&source&&scope&&scope.get()==control&&
  r->generation==generation&&r->serial==serial&&serial&&!r->core.expired()&&!r->core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(r->core)&&
  p&&p->bank.get()==r->bank.lock().get()&&p->player_attrition_prefixes.contains(r->turn)&&
  !p->player_attrition_prefixes.at(r->turn).owner_before(r->self)&&!r->self.owner_before(p->player_attrition_prefixes.at(r->turn))&&r->executed==executed,
  "Attrition actual Core/Source/control/once entry differs");core.require_live_gold_roster_source_without_callbacks(host,s,r->actor);r->require_rows(s,executed);
 need(!executed||(r->rng_stored==r->reached.draw&&r->outer_returned==r->reached.empty),"Attrition physical native prefix status differs");
}
void BnwNativeAttritionPrefix::execute_native_without_callbacks(LiveSessionState&s)const{
 const auto r=impl_;r->thread();need(!r->executed&&!r->rng_stored&&!r->failed,"Attrition native entry replay");r->require_rows(s,false);
 // LAST pure expected operands above. Prepare/reserve the exact bounded native
 // result before mutation; only ONE reached JonRand100 consumes gameplay RNG.
 if(r->reached.draw){need(r->before<UINT64_MAX&&r->before_rng.calls<UINT64_MAX,"Attrition actual RNG/revision overflow before draw");
  auto target=s.random.gameplay;const auto value=target.draw(100);
  need(r->roll==std::optional{value}&&target.state()==r->after_rng,"Attrition exact prepared JonRand100 differs");
  s.random.gameplay=target;r->rng_stored=true;s.ui_source_revision=r->after;
 }
 r->executed=true;r->outer_returned=r->reached.empty;
}
BnwAttritionPrefixProgress BnwNativeAttritionPrefix::progress_without_callbacks()const{
 const auto r=impl_;r->thread();BnwAttritionPrefixProgress o;o.issued=true;o.executed=r->executed;o.rng_stored=r->rng_stored;o.outer_returned=r->outer_returned;
 o.unit=r->reached.id;o.slot=r->reached.slot;o.roll=r->rng_stored?r->roll:std::nullopt;o.before_revision=r->before;o.after_revision=r->after;o.boundary=r->reached.boundary;
 if(r->rng_stored&&r->reached.draw&&r->roll&&*r->roll<static_cast<std::uint64_t>(r->reached.chance))o.boundary=BnwAttritionPrefixBoundary::localized_damage_owner;return o;
}
void BnwNativeUnitAllocatorCore::prepare_player_attrition_first_owned(const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,
 std::shared_ptr<const BnwSelectedStockLuaSourceOwner>source,std::weak_ptr<const void>control,std::uint64_t serial,std::int32_t actor,
 std::shared_ptr<const BnwNativeAttritionPrefix>&issued)const{
 const auto borrower=ShouldDimCoreBorrowedOwner::from_weak(factory_self_);const auto p=impl_;
 require_live_gold_roster_source_without_callbacks(host,s,actor);const auto scope=control.lock();
 need(source&&scope&&serial&&actor==p->actor&&!issued&&!p->player_attrition_prefixes.contains(s.setup.game_turn),"Attrition requires genuine once Source/Core/actor entry");
 auto r=std::make_shared<BnwNativeAttritionPrefix::Impl>();r->core=factory_self_;r->source=source;r->host=p->host;r->world=p->state;
 r->control=control;r->bank=p->bank;r->generation=p->generation;r->serial=serial;r->before=s.ui_source_revision;r->after=r->before;r->turn=s.setup.game_turn;r->actor=actor;
 r->map_rng=s.random.map.state();r->before_rng=s.random.gameplay.state();r->after_rng=r->before_rng;r->players=s.world.all_players->players.data();
 for(std::size_t n=0;n<64;++n){const auto&raw=s.world.all_players->players[n];auto&m=r->managers[n];m.player=&raw;m.player_id=raw.saved_player_id;
  m.units_layout=raw.units.layout;m.header_layout=raw.units.header.layout;m.header=raw.units.header.fields.data();m.units=raw.units.units.data();m.source_slots=raw.units.header.source_slots;
  m.header_values.reserve(raw.units.header.fields.size());for(const auto&f:raw.units.header.fields)m.header_values.push_back(f.value);
  m.rows.reserve(raw.units.units.size());for(const auto&u:raw.units.units)m.rows.push_back(busy_unit_stamp(u));}
 issued=std::shared_ptr<const BnwNativeAttritionPrefix>(new BnwNativeAttritionPrefix(r));r->self=issued;
 try{p->player_attrition_prefixes.emplace(r->turn,issued);
  {Impl::Guard invocation(p);p->check();(void)borrower->current();r->reached=actual_attrition_first_read(s,*p->bank,actor);
   if(r->reached.draw){need(r->before<UINT64_MAX&&r->before_rng.calls<UINT64_MAX,"Attrition RNG/revision exhausted before native effect");
    auto target=s.random.gameplay;r->roll=target.draw(100);r->after_rng=target.state();r->after=r->before+1;}
   r->require_rows(s,false);p->check();(void)borrower->current();r->require_rows(s,false);}
  issued->require_current_without_callbacks(*this,host,s,*source,scope.get(),p->generation,serial,false);
 }catch(...){r->failed=true;p->bank->retired=true;throw;}
}
}

namespace civ5::runtime {
void BnwNativeAttritionPrefix::prepare_mountain_owned(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,
 const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial)const{
 const auto r=impl_;r->thread();require_current_without_callbacks(core,host,s,source,control,generation,r->serial,true);
 need(!r->mountain&&r->serial<UINT64_MAX&&serial==r->serial+1&&r->reached.id&&!r->outer_returned&&
  progress_without_callbacks().boundary==BnwAttritionPrefixBoundary::mountain_domain_owner,"Attrition mountain genuine once prefix predecessor absent");
 r->mountain=std::make_unique<AttritionMountainChild>();auto&q=*r->mountain;q.serial=serial;const auto p=core.impl_;
 const auto borrower=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);
 try{const auto b=r->bank.lock();need(bool(b),"Attrition mountain actual Bank expired");const auto*u=native_imported_get_at(s,r->actor,*r->reached.id);need(u,"Attrition mountain actual unit absent");
  {BnwNativeUnitAllocatorCore::Impl::Guard invocation(p);p->check();(void)borrower->current();const auto info=actual_attrition_domain_info(host,s,*u);
   p->check();(void)borrower->current();need(actual_attrition_domain_info(host,s,*u)==info,"Attrition mountain UnitInfo changed during providers");
   q.reached=actual_attrition_mountain_read(host,s,*b,r->actor,*r->reached.id,info);p->check();(void)borrower->current();}
  require_mountain_without_callbacks(core,host,s,source,control,generation,serial,false);
 }catch(...){q.failed=true;throw;}
}
void BnwNativeAttritionPrefix::require_mountain_without_callbacks(const BnwNativeUnitAllocatorCore&core,const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,
 const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,std::uint64_t serial,bool executed)const{
 const auto r=impl_;r->thread();require_current_without_callbacks(core,host,s,source,control,generation,r->serial,true);
 need(r->mountain&&!r->mountain->failed&&r->serial<UINT64_MAX&&serial==r->serial+1&&r->mountain->serial==serial&&
  r->mountain->executed==executed&&r->reached.id,"Attrition mountain exact actual once child/source scope differs");
 const auto b=r->bank.lock();need(bool(b),"Attrition mountain canonical Bank expired");const auto&q=r->mountain->reached;
 (void)actual_attrition_mountain_read(host,s,*b,r->actor,*r->reached.id,q.info,&q);
}
void BnwNativeAttritionPrefix::execute_mountain_without_callbacks(const LiveSessionState&s)const{
 const auto r=impl_;r->thread();need(r->mountain&&!r->mountain->failed&&!r->mountain->executed,"Attrition mountain native getter replay");r->require_rows(s,true);
 r->mountain->executed=true; // native condition returned; no localization/damage/HP/religion side effects issued.
}
BnwAttritionMountainProgress BnwNativeAttritionPrefix::mountain_progress_without_callbacks()const{
 const auto r=impl_;r->thread();BnwAttritionMountainProgress o;if(!r->mountain)return o;
 const auto&q=*r->mountain;o.issued=true;o.executed=q.executed;o.damage_required=q.executed&&q.reached.damage;o.unit=q.reached.id;
 o.domain=q.reached.domain;o.plot_type=q.reached.plot_type;o.all_terrain_count=q.reached.all_terrain_count;o.revision=r->after;return o;
}
#include "bnw_attrition_damage_child.inc"
#include "bnw_attrition_garrison_child.inc"
#include "bnw_attrition_city_strength_child.inc"
#include "bnw_attrition_city_info_child.inc"
#include "bnw_attrition_unit_info_child.inc"
#include "bnw_attrition_unit_selected_child.inc"
#include "bnw_attrition_selected_unit_info_child.inc"
#include "bnw_attrition_selection_plot_child.inc"
#include "bnw_attrition_plot_list_child.inc"
#include "bnw_attrition_damage_dll_wrapper_child.inc"
#include "bnw_unitflag_living_health_read.inc"
}

#include "bnw_unit_setup_created_payload.inc"

#include "bnw_pending_created_unit_read.inc"
#include "bnw_pending_created_unit_info.inc"
