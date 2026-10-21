#include "civ5/runtime/bnw_fresh_first_player_state.hpp"
#include "civ5/runtime/bnw_fresh_game_builder.hpp"
#include "civ5/runtime/bnw_fresh_tactical_map.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_native_max_rows.hpp"
#include "civ5/runtime/bnw_policy_branch_mapping.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <map>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
// Expansion2 CvDLLUtilDefines.h:25, not a mutable Defines row or supplied roster limit.
constexpr std::size_t native_max_major_civs=22;
constexpr std::int32_t no_ai_grand_strategy=-1;
constexpr std::int32_t no_guess_confidence_type=-1;
struct ScalarStore { std::string_view name; std::int64_t value; bool boolean; bool registered; };
#include "bnw_fresh_player_scalar_stores.inc"
void require(bool condition,const char* message) {
    if(!condition) throw std::invalid_argument(message);
}
// Selected Rules materialization is caller work, not native Reset stores.
constexpr std::size_t native_max_players=64;
constexpr std::size_t tech_init_budget=1024*1024;
struct OwnedTechPregame {
    // CvPreGame PREGAMEVAR + FAutoVector(size): value-initialized native arrays.
    std::vector<bool> minor=std::vector<bool>(native_max_players,false);
    std::vector<std::int32_t> civilization=std::vector<std::int32_t>(native_max_players,0);
    std::vector<std::int32_t> leader=std::vector<std::int32_t>(native_max_players,0);
};
struct TechSourceEntry {
    std::string type;
    std::optional<std::string> prereq,required_civ,improvement;
    bool specific_civ=false,maya=false;
};
struct TechSourceCollection {
    content::InfoCollectionReport report;
    std::map<std::int32_t,TechSourceEntry> entries;
    const TechSourceEntry* find(std::int32_t id) const {
        const auto it=entries.find(id);return it==entries.end()?nullptr:&it->second;
    }
};
std::int32_t tech_native_i32(std::int64_t value) {
    require(value>=std::numeric_limits<std::int32_t>::min() &&
            value<=std::numeric_limits<std::int32_t>::max(),"PlayerTechs native int32 unavailable");
    return static_cast<std::int32_t>(value);
}
TechSourceCollection tech_source_collection(const content::RulesDatabase& rules,const char* table) {
    TechSourceCollection out;
    out.report=rules.prefetch_info(table,[&](std::uint64_t slot,std::int64_t,
                                             const content::RulesRowView& row) {
        require(slot<tech_init_budget,"PlayerTechs source collection exceeds budget");
        TechSourceEntry entry;
        entry.type=row.text("Type").value_or("");
        entry.prereq=row.text("PrereqTech");entry.required_civ=row.text("CivilizationType");
        entry.improvement=row.text("ImprovementType");
        entry.specific_civ=row.boolean("SpecificCivRequired");entry.maya=row.boolean("MayaCalendarBonuses");
        out.entries.emplace(static_cast<std::int32_t>(slot),std::move(entry));return true;
    },{tech_init_budget});
    require(out.report.all_succeeded() && out.report.slots<=tech_init_budget,
            "PlayerTechs selected source collection unavailable");
    return out;
}
std::int32_t tech_source_enum(const content::InfoTypeCatalog& types,
                             const std::optional<std::string>& name) {
    if(!name || name->empty())return -1;
    const auto* entry=types.find(*name);return entry?tech_native_i32(entry->id):-1;
}
std::int32_t tech_source_define(const content::RulesDatabase& rules,const char* name) {
    const auto value=rules.global_define(name,content::GlobalDefineKind::integer);
    require(value.error.empty(),"PlayerTechs selected Define unavailable");
    const auto* integer=std::get_if<std::int64_t>(&value.value);
    require(integer!=nullptr,"PlayerTechs selected Define is not native integer");
    return tech_native_i32(*integer); // BNW GetDefineINT missing value is zero.
}
struct PolicySourceCollection {
    content::InfoCollectionReport policies,branches;
    std::map<std::int32_t,std::int32_t> policy_branches;
    std::map<std::int32_t,std::int32_t> branch_openers;
};
PolicySourceCollection policy_source_collection(const content::RulesDatabase& rules) {
    // Caller materialization of the already-prefetched game collection. This
    // is distinct from native Init/Reset and never loads happiness operands.
    const auto types=rules.cache_info_types();
    require(types.report().all_succeeded(),"PlayerPolicies native type collection unavailable");
    PolicySourceCollection out;
    out.policies=rules.prefetch_info("Policies",[&](std::uint64_t slot,std::int64_t,
                                                  const content::RulesRowView& row) {
        require(slot<tech_init_budget,"PlayerPolicies policy slot exceeds budget");
        out.policy_branches.emplace(static_cast<std::int32_t>(slot),
                                    tech_source_enum(types,row.text("PolicyBranchType")));
        return true;
    },{tech_init_budget});
    out.branches=rules.prefetch_info("PolicyBranchTypes",[&](std::uint64_t slot,std::int64_t,
                                                          const content::RulesRowView& row) {
        require(slot<tech_init_budget,"PlayerPolicies branch slot exceeds budget");
        out.branch_openers.emplace(static_cast<std::int32_t>(slot),
                                   tech_source_enum(types,row.text("FreePolicy")));
        return true;
    },{tech_init_budget});
    require(out.policies.all_succeeded() && out.branches.all_succeeded() &&
            out.policies.slots<=tech_init_budget && out.branches.slots<=tech_init_budget,
            "PlayerPolicies selected source collections unavailable");
    return out;
}
struct OwnedPlayerPolicies {
    // Nullable native allocations after the actual constructor/Uninit prefix.
    // Their values are established below by real Reset stores, not DTO flags.
    std::optional<std::vector<std::int32_t>> latest_flavors;
    std::optional<std::vector<bool>> has_policy,one_shot,one_shot_free_units;
    std::optional<std::vector<bool>> branch_unlocked,branch_blocked,branch_finished;
    std::optional<std::vector<std::int32_t>> branches_chosen,blocked_branch_check;
    bool is_city=false;
    std::optional<std::int32_t> num_extra_branches,branch_picked1,branch_picked2,branch_picked3;
    // CvPolicyAI constructor owns an empty vector; Reset clears it again.
    std::optional<std::vector<std::pair<std::int32_t,std::int32_t>>> weights;
    std::optional<std::int32_t> propagation_levels,percent_drop_new_branch;
};

struct TechCivMappings {
    std::vector<std::int32_t> building,unit;
    std::vector<bool> building_overridden,unit_overridden;
};
TechCivMappings tech_civ_mappings(const content::RulesDatabase& rules,const std::string& civ,
                                const TechSourceCollection&,const TechSourceCollection&) {
    // Native allocation uses rowid MaxRows. Reset loops still use Info slot
    // counts; each reached index is checked separately, without ID=rowid assumptions.
    TechCivMappings m;
    m.building.assign(static_cast<std::size_t>(detail::bnw_native_max_rows(rules,"BuildingClasses")),-1);
    m.unit.assign(static_cast<std::size_t>(detail::bnw_native_max_rows(rules,"UnitClasses")),-1);
    m.building_overridden.assign(m.building.size(),false);m.unit_overridden.assign(m.unit.size(),false);
    const auto rows=[&](const char* sql,const std::vector<content::DatabaseValue>& args,
                        auto& ids,auto* flags) {
        const auto q=rules.query_rows({sql,args},[&](const content::RulesRowView& row) {
            const auto id=tech_native_i32(row.integer_at(0));
            require(id>=0 && static_cast<std::size_t>(id)<ids.size(),"PlayerTechs class mapping index unavailable");
            ids[static_cast<std::size_t>(id)]=tech_native_i32(row.integer_at(1));
            if(flags)(*flags)[static_cast<std::size_t>(id)]=true;
        },tech_init_budget);
        require(q.all_succeeded(),"PlayerTechs civilization relation unavailable");
    };
    rows("select BuildingClasses.ID, Buildings.ID as BuildingID from BuildingClasses inner join Buildings on Buildings.Type = DefaultBuilding",{},m.building,static_cast<std::vector<bool>*>(nullptr));
    rows("select BuildingClasses.ID, coalesce(Buildings.ID, -1) from Civilization_BuildingClassOverrides inner join BuildingClasses on BuildingClassType = BuildingClasses.Type left outer join Buildings on BuildingType = Buildings.Type where CivilizationType = ?",{civ},m.building,&m.building_overridden);
    rows("select UnitClasses.ID, Units.ID as UnitID from UnitClasses inner join Units on Units.Type = DefaultUnit",{},m.unit,static_cast<std::vector<bool>*>(nullptr));
    rows("select UnitClasses.ID, coalesce(Units.ID, -1) from Civilization_UnitClassOverrides inner join UnitClasses on UnitClassType = UnitClasses.Type left outer join Units on UnitType = Units.Type where CivilizationType = ?",{civ},m.unit,&m.unit_overridden);
    return m;
}

BnwArchiveMemberState& member(std::vector<BnwArchiveMemberState>& values,std::string_view name) {
    const auto found=std::find_if(values.begin(),values.end(),[&](const auto& v){return v.name==name;});
    if(found==values.end()) throw std::invalid_argument("Missing native registered Player member");
    return *found;
}
void store(std::vector<BnwArchiveMemberState>& values,std::string_view name,BnwArchiveValue value,
           std::uint32_t commands=0) {
    auto& target=member(values,name);
    require(target.value.index()==value.index(),"Player store differs from native member type");
    require(target.pending_vector_commands<=std::numeric_limits<std::uint32_t>::max()-commands,
            "Player archive command count overflow");
    target.value=std::move(value);target.pending_vector_commands+=commands;
}

// Logical BNW gameplay storage, not the original pointer-sized memory ABI.
// Native source aliases address SAME data owner; uninitialized cells stay empty.
enum class DiploKind {i8,i16,i32,boolean};
struct DiploArray {DiploKind kind;std::vector<std::optional<std::int32_t>> values;};
struct DiploData {
    std::map<std::string_view,DiploArray> fields;
    std::map<std::string_view,std::vector<std::optional<std::size_t>>> pointer_rows;
};
struct DiploAlias {
    const DiploArray* field=nullptr;
    std::vector<std::optional<std::size_t>> row_offsets;
    std::size_t columns=0;
};
struct OwnedDiplomacy {
    const BnwFreshDiplomacyServiceState* uninit=nullptr;
    const void* player=nullptr;
    std::unique_ptr<DiploData> data;
    std::map<std::string_view,DiploAlias> aliases;
    std::map<std::string_view,std::int32_t> scalars;
    std::vector<std::int32_t> greet_players;
    std::uint64_t reset_writes=0;
    void allocate_data() {
        require(!data,"Diplomacy data already allocated");
        data=std::make_unique<DiploData>();
        data->fields.emplace("m_aDiploLogStatementTurnCountScratchPad",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(86,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiMajorCivOpinion",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeMajorCivApproach",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeApproachScratchPad",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeMinorCivApproach",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(58,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeApproachTowardsUsGuess",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeApproachTowardsUsGuessCounter",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeWantPeaceCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePeaceTreatyWillingToOffer",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePeaceTreatyWillingToAccept",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumWondersBeatenTo",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abMusteringForAttack",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abWantsResearchAgreementWithPlayer",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abWantToRouteToMinor",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(58,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeWarFace",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeWarState",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeWarProjection",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeLastWarProjection",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::nullopt)});
        data->fields.emplace("m_aeWarGoal",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPlayerNumTurnsAtWar",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumWarsFought",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerMilitaryStrengthComparedToUs",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerEconomicStrengthComparedToUs",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerTargetValue",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerLandDisputeLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerLastTurnLandDisputeLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerVictoryDisputeLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerWonderDisputeLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerMinorCivDisputeLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeMilitaryAggressivePosture",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeLastTurnMilitaryAggressivePosture",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeExpansionAggressivePosture",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlotBuyingAggressivePosture",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeMilitaryThreat",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeWarDamageLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiWarValueLost",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aeWarmongerThreat",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(80,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPersonalityMajorCivApproachBiases",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(7,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPersonalityMinorCivApproachBiases",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(5,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aDeclarationsLog.m_eDeclaration",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(60,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aDeclarationsLog.m_iData1",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(60,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aDeclarationsLog.m_iData2",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(60,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aDeclarationsLog.m_eMustHaveMetPlayer",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(60,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aDeclarationsLog.m_bActive",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(60,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aDeclarationsLog.m_iTurn",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(60,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerNoSettleRequest",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPlayerNoSettleRequestCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerStopSpyingRequest",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPlayerStopSpyingRequestCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiDemandCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiDemandTooSoonNumTurns",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abDoFAccepted",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiDoFCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abDenouncedPlayer",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abFriendDenouncedUs",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abFriendDeclaredWarOnUs",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiDenouncedPlayerCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumRequestsRefused",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumCiviliansReturnedToMe",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumLandmarksBuiltForMe",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiResurrectedOnTurn",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumTimesCultureBombed",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_paiNegativeReligiousConversionPoints",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_paiNegativeArchaeologyPoints",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumTimesNuked",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumTimesRobbedBy",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiNumTimesIntrigueSharedBy",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiBrokenExpansionPromiseValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiIgnoredExpansionPromiseValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiBrokenBorderPromiseValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiIgnoredBorderPromiseValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiDeclaredWarOnFriendValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiNumCitiesLiberated",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiTradeValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiCommonFoeValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiAssistValue",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_abPlayerMadeMilitaryPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenMilitaryPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredMilitaryPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPlayerMilitaryPromiseCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPlayerMadeExpansionPromiseTurn",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenExpansionPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredExpansionPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerExpansionPromiseData",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiPlayerMadeBorderPromiseTurn",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenBorderPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredBorderPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aePlayerBorderPromiseData",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerMadeAttackCityStatePromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenAttackCityStatePromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredAttackCityStatePromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerMadeBullyCityStatePromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenBullyCityStatePromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredBullyCityStatePromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerMadeSpyPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenSpyPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredSpyPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerForgaveForSpying",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerMadeNoConvertPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenNoConvertPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredNoConvertPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerAskedNotToConvert",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerAgreedNotToConvert",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerMadeNoDiggingPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenNoDiggingPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerIgnoredNoDiggingPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerAskedNotToDig",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerAgreedNotToDig",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_abPlayerBrokenCoopWarPromise",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceAttackedProtectedMinor",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerProtectedMinorAttacked",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerNumProtectedMinorsAttacked",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceKilledProtectedMinor",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerProtectedMinorKilled",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerNumProtectedMinorsKilled",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerNumProtectedMinorsBullied",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceSidedWithTheirProtectedMinor",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerNumMinorsAttacked",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerNumMinorsConquered",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerNumMajorsAttacked",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerNumMajorsConquered",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(22,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aiOtherPlayerWarmongerAmount",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceWeLikedTheirProposal",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceWeDislikedTheirProposal",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceTheySupportedOurProposal",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceTheyFoiledOurProposal",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aiOtherPlayerTurnsSinceTheySupportedOurHosting",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(22,std::nullopt)});
        data->fields.emplace("m_aaeOtherPlayerMajorCivOpinion",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(484,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaeOtherPlayerLandDisputeLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(6400,std::optional<std::int32_t>{0})});
        // Only the declared80 source constructor cells have known zero values.
        // Expanded safe logical backing remains unknown until actual Reset.
        data->fields.emplace("m_aaeOtherPlayerVictoryDisputeLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(6400,std::nullopt)});
        std::fill_n(data->fields.at("m_aaeOtherPlayerVictoryDisputeLevel").values.begin(),80,std::optional<std::int32_t>{0});
        data->fields.emplace("m_aaeOtherPlayerWarDamageLevel",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(6400,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaiOtherPlayerWarValueLost",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(6400,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaiOtherPlayerLastRoundWarValueLost",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(6400,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aabSentAttackMessageToMinorCivProtector",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(6400,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaeOtherPlayerMilitaryThreat",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(6400,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaDiploStatementsLog.m_eDiploLogStatement",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(1320,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaDiploStatementsLog.m_iTurn",DiploArray{DiploKind::i32,std::vector<std::optional<std::int32_t>>(1320,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aabWorkingAgainstPlayerAccepted",DiploArray{DiploKind::boolean,std::vector<std::optional<std::int32_t>>(484,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaiWorkingAgainstPlayerCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(484,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aacCoopWarAcceptedState",DiploArray{DiploKind::i8,std::vector<std::optional<std::int32_t>>(484,std::optional<std::int32_t>{0})});
        data->fields.emplace("m_aaiCoopWarCounter",DiploArray{DiploKind::i16,std::vector<std::optional<std::int32_t>>(484,std::optional<std::int32_t>{0})});
        data->pointer_rows.emplace("m_apaeOtherPlayerMajorCivOpinion",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaeOtherPlayerLandDisputeLevel",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaeOtherPlayerVictoryDisputeLevel",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaeOtherPlayerWonderDisputeLevel",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaeOtherPlayerMinorCivDisputeLevel",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaeOtherPlayerWarDamageLevel",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaiOtherPlayerWarValueLost",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaiOtherPlayerLastRoundWarValueLost",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apabSentAttackMessageToMinorCivProtector",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaeOtherPlayerMilitaryThreat",std::vector<std::optional<std::size_t>>(80));
        data->pointer_rows.emplace("m_apaDiploStatementsLog",std::vector<std::optional<std::size_t>>(22));
        data->pointer_rows.emplace("m_apabWorkingAgainstPlayerAccepted",std::vector<std::optional<std::size_t>>(22));
        data->pointer_rows.emplace("m_apaiWorkingAgainstPlayerCounter",std::vector<std::optional<std::size_t>>(22));
        data->pointer_rows.emplace("m_apacCoopWarAcceptedState",std::vector<std::optional<std::size_t>>(22));
        data->pointer_rows.emplace("m_apaiCoopWarCounter",std::vector<std::optional<std::size_t>>(22));
    }
    void bind_one(std::string_view alias,std::string_view field) {
        const auto& a=data->fields.at(field);
        require(aliases.emplace(alias,DiploAlias{&a,{std::size_t{0}},a.values.size()}).second,
                "Diplomacy duplicate native alias");
    }
    void bind_rows(std::string_view alias,std::string_view table,std::string_view field,
                   std::size_t rows,std::size_t stride) {
        const auto& a=data->fields.at(field);auto& offsets=data->pointer_rows.at(table);
        require(rows<=offsets.size() && rows*stride<=a.values.size(),"Diplomacy logical matrix bounds");
        for(std::size_t i=0;i<rows;++i) offsets[i]=i*stride;
        require(aliases.emplace(alias,DiploAlias{&a,offsets,stride}).second,"Diplomacy duplicate matrix alias");
    }
    void store(std::string_view alias,std::size_t row,std::size_t col,std::int32_t value) {
        const auto& a=aliases.at(alias);
        require(row<a.row_offsets.size() && a.row_offsets[row] && col<a.columns,
                "Diplomacy native store outside bound alias");
        auto& f=data->fields.at(field_name(alias));
        require(&f==a.field,"Diplomacy store lost SAME data owner");
        const bool fits=f.kind==DiploKind::i32 ||
            (f.kind==DiploKind::i8 && value>=-128 && value<=127) ||
            (f.kind==DiploKind::i16 && value>=-32768 && value<=32767) ||
            (f.kind==DiploKind::boolean && (value==0 || value==1));
        require(fits,"Diplomacy native store width unavailable");
        f.values.at(*a.row_offsets[row]+col)=value;
        ++reset_writes;
    }
    std::string_view field_name(std::string_view alias) const {
        const auto* actual=aliases.at(alias).field;
        for(const auto& [name,value]:data->fields) if(&value==actual) return name;
        throw std::invalid_argument("Diplomacy alias has no owned field");
    }
    void scalar_store(std::string_view name,std::int32_t value) {scalars.insert_or_assign(name,value);++reset_writes;}
    std::optional<std::int32_t> value(std::string_view alias,std::size_t row,std::size_t col) const {
        const auto& a=aliases.at(alias);
        require(row<a.row_offsets.size() && col<a.columns,"Diplomacy read index outside native extent");
        if(!a.row_offsets[row]) return std::nullopt;
        return a.field->values.at(*a.row_offsets[row]+col);
    }
};
}
struct BnwFreshFirstPlayerState::Impl {
    // All constructed service/container owners survive their genuine ordered Uninit.
    // archive.defaults is canonical; other projections are retained predecessor evidence.
    BnwFreshPlayerUninitPrefixDraft uninit;
    std::vector<std::pair<std::string_view,BnwArchiveValue>> unregistered;
    BnwFreshPlayerEarlyReset early;
    BnwFreshEconomicAIState economic;
    BnwFreshMilitaryAIState military;
    BnwFreshCitySpecializationState specialization;
    OwnedDiplomacy diplomacy;
    const OwnedDiplomacy* military_diplomacy=nullptr;
    // SAME first-player service after its real constructor and empty Uninit.
    BnwFreshReligionsServiceState religions;
    const Impl* religions_player=nullptr;
    BnwFreshReligionAiServiceState religion_ai;
    // Existing selected Rules source collection provenance, owned independently
    // of borrowed DB facade lifetime. Future Entries remain outside this scope.
    content::InfoCollectionReport belief_info;
    const content::InfoCollectionReport* religion_ai_beliefs=nullptr;
    const Impl* religion_ai_player=nullptr;
    OwnedTechPregame pregame;
    BnwFreshPlayerTechsServiceState techs;
    TechSourceCollection tech_info;
    const TechSourceCollection* techs_info=nullptr;
    const Impl* techs_player=nullptr;
    const BnwFreshPlayerTechsServiceState* tech_ai_current=nullptr;
    // Reuse the real constructor/empty-Uninit state already executed by this
    // same player's service chain. Opaque predecessor bytes remain opaque.
    BnwFreshTacticalAiServiceState tactical;
    const Impl* tactical_player=nullptr;
    std::optional<std::int32_t> tactical_move_priority_turn;
    std::optional<std::nullptr_t> tactical_map_reset;
    bool tactical_init_attempted=false;
    const BnwFreshTacticalMap* tactical_game_map=nullptr;
    std::map<std::string_view,std::int32_t> tactical_init_values;
    std::optional<float> tactical_flavor_dampening;
    std::vector<std::pair<std::string_view,std::int32_t>> tactical_cached_types;
    OwnedPlayerPolicies policies;
    PolicySourceCollection policy_info;
    const PolicySourceCollection* policies_info=nullptr;
    const Impl* policies_player=nullptr;
    const OwnedPlayerPolicies* policy_ai_current=nullptr;
    // Fresh constructor vectors are distinct: Reset clears only WonderAIWeights.
    std::vector<std::int32_t> wonder_flavors;
    std::vector<std::pair<std::int32_t,std::int32_t>> wonder_weights;
    std::vector<std::pair<std::int32_t,std::int32_t>> wonder_buildables;
    const Impl* wonder_player=nullptr;
    bool wonder_is_city=false;
    // The actual constructor/Uninit arrays are copied before Init. Historical
    // uninit remains provenance; this is the authoritative initialized child.
    BnwFreshGrandStrategyServiceState grand_strategy;
    const Impl* grand_player=nullptr;
    std::vector<std::string_view> trace;
    std::string sha;
};
BnwFreshFirstPlayerState::BnwFreshFirstPlayerState(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwFreshFirstPlayerState::~BnwFreshFirstPlayerState()=default;
std::shared_ptr<const BnwFreshFirstPlayerState> BnwFreshFirstPlayerState::bind(
    const BnwFreshGameBuilderResult& d,const BnwFreshGameContentResult& selected,
    const BnwFreshGameBuilderInputs& inputs) {
    // This private call is made only after the builder's actual selected-content factories.
    // Supplied diagnostics and caller booleans cannot issue this owner.
    require(d.first_player_diagnostic.diagnostic_complete &&
            d.first_player_diagnostic.detached_uninit.detached_effects_complete &&
            d.first_player_diagnostic.detached_uninit.blocked_at=="archive.clearDelta" &&
            d.player_info.selected_arrays_complete && d.economic.economic_ai_init_complete &&
            d.military.arrays_and_reset_complete && d.city_specialization.city_specialization_init_complete,
            "Incomplete native first Player predecessor");
    auto p=std::make_unique<Impl>();
    p->uninit=d.first_player_diagnostic.detached_uninit;
    p->sha=d.selected_rules_sha256;
    p->tactical=p->uninit.tactical_ai.state;
    p->religions=p->uninit.religions.state;
    p->religion_ai=p->uninit.religion_ai.state;
    p->diplomacy.uninit=&p->uninit.diplomacy.state;
    auto& constructed_diplo=p->diplomacy;
    constructed_diplo.scalars.emplace("m_eStateAllWars",0);
    constructed_diplo.scalars.emplace("m_eTargetPlayer",-1);
    constructed_diplo.scalars.emplace("m_eTestToPlayer",-1);
    constructed_diplo.scalars.emplace("m_eTestStatement",-1);
    constructed_diplo.scalars.emplace("m_iTestStatementArg1",-1);
    for(const auto* sha:std::array<const std::string*,6>{&p->uninit.selected_rules_sha256,&d.player_early.selected_rules_sha256,
                        &d.player_info.selected_rules_sha256,&d.economic.selected_rules_sha256,
                        &d.military.selected_rules_sha256,&d.city_specialization.selected_rules_sha256})
        require(!p->sha.empty() && *sha==p->sha,"First Player source identity mismatch");
    require(!p->uninit.archive.aggregate_traversal_verified &&
            p->uninit.archive.all_registered_member_defaults &&
            p->uninit.diplomacy.member_effect_complete,"Invalid owned reset/Uninit predecessor");
    p->trace.push_back("m_syncArchive.reset gameplay values");
    for(const auto& e:p->uninit.source_order)
        if(e.kind!=BnwFreshPlayerUninitPrefixEventKind::archive_reset_projection)
            p->trace.push_back(e.sdk_member);
    // Scalar clearDelta is empty; vector clearDelta removes only commands.
    // Our backend has no OOS/network delta set. No gameplay value is erased here.
    for(auto& m:p->uninit.archive.defaults) m.pending_vector_commands=0;
    p->trace.push_back("archive.clearDelta owned command tracking");
    for(const auto& s:scalar_stores) {
        require(s.value>=std::numeric_limits<std::int32_t>::min() &&
                s.value<=std::numeric_limits<std::int32_t>::max(),"Native Player scalar exceeds int32");
        BnwArchiveValue value=s.boolean?BnwArchiveValue{bool(s.value)}:
            BnwArchiveValue{static_cast<std::int32_t>(s.value)};
        if(s.registered) {
            auto& m=member(p->uninit.archive.defaults,s.name);
            if(m.kind==BnwArchiveMemberKind::unsigned_scalar) {
                require(s.value>=0,"Negative native uint store");
                value=static_cast<std::uint32_t>(s.value);
            }
            store(p->uninit.archive.defaults,s.name,std::move(value));
        } else p->unregistered.emplace_back(s.name,std::move(value));
        p->trace.push_back(s.name);
    }
    require(d.player_info.players.size()==d.economic.players.size() &&
            d.economic.players.size()==d.military.players.size() &&
            d.military.players.size()==d.city_specialization.players.size() &&
            !d.player_info.players.empty(),"First Player component shape mismatch");
    const auto& info=d.player_info.players.front();
    require(info.early.id==0 && d.economic.players.front().player_id==0 &&
            d.military.players.front().player_id==0 && d.city_specialization.players.front().player_id==0,
            "First Player component actor mismatch");
    p->early=info.early;
    // Use the existing selected early-reset table; its unregistered vectors/strings
    // remain owned in early and registered writes go to canonical archive storage.
    require(stage_bnw_fresh_archive_reset_prefix(p->uninit.archive,d.player_early),
            "Native early-reset source table is unavailable");
    for(const auto& w:p->uninit.archive.reset_prefix_writes) {
        const auto kind=member(p->uninit.archive.defaults,w.member).kind;
        const bool vector=kind==BnwArchiveMemberKind::int_vector ||
                          kind==BnwArchiveMemberKind::bool_vector ||
                          kind==BnwArchiveMemberKind::yield_array_vector;
        store(p->uninit.archive.defaults,w.member,w.value,vector?2U:0U);
    }
    // m_aOptions.clear is one command, rather than clear/resize.
    member(p->uninit.archive.defaults,"m_aOptions").pending_vector_commands=1;
    p->trace.push_back("CvPlayer::reset early values/vectors/strings");
    const auto set_vector=[&](std::string_view name,const auto& value){
        store(p->uninit.archive.defaults,name,value,2);
        p->trace.push_back(name);
    };
    set_vector("m_paiNumResourceUsed",info.resource_used);
    set_vector("m_paiNumResourceTotal",info.resource_total);
    set_vector("m_paiResourceGiftedToMinors",info.resource_gifted_to_minors);
    set_vector("m_paiResourceExport",info.resource_export);
    set_vector("m_paiResourceImport",info.resource_import);
    set_vector("m_paiResourceFromMinors",info.resource_from_minors);
    set_vector("m_paiResourcesSiphoned",info.resource_siphoned);
    set_vector("m_paiImprovementCount",info.improvement_count);
    set_vector("m_paiUnitCombatProductionModifiers",info.unit_combat_production);
    set_vector("m_paiUnitCombatFreeExperiences",info.unit_combat_experience);
    set_vector("m_paiFreeBuildingCount",info.free_building);
    set_vector("m_paiFreePromotionCount",info.free_promotion);
    set_vector("m_paiUnitClassCount",info.unit_class_count);
    set_vector("m_paiUnitClassMaking",info.unit_class_making);
    set_vector("m_paiBuildingClassCount",info.building_class_count);
    set_vector("m_paiBuildingClassMaking",info.building_class_making);
    set_vector("m_paiProjectMaking",info.project_making);
    set_vector("m_paiHurryCount",info.hurry_count);
    set_vector("m_paiHurryModifier",info.hurry_modifier);
    set_vector("m_pabLoyalMember",info.loyal_member);
    set_vector("m_pabGetsScienceFromPlayer",info.gets_science_from_player);
    p->economic=d.economic.players.front();
    p->trace.push_back("m_pEconomicAI->Init");
    // Native Military Init stores this argument and Reset does not dereference it.
    // Bind to the same real constructor/Uninit owner retained in this allocation.
    p->military_diplomacy=&p->diplomacy;
    p->military=d.military.players.front();
    p->trace.push_back("m_pMilitaryAI->Init");
    p->specialization=d.city_specialization.players.front();
    p->trace.push_back("m_pCitySpecializationAI->Init");
    // The private builder supplies the actual selected parent; a caller SHA or
    // detached Info count cannot initialize the next native service.
    require(selected.content_ready() && selected.rules && selected.activation &&
            selected.activation->profile_kind==selected.profile.kind &&
            selected.profile.kind!=ContentProfileKind::vanilla,
            "Wonder Init requires actual selected content");
    const auto& rules=*selected.rules;
    const auto current_sha=[&]{return identify_bytes(rules.gameplay_fingerprint_data()).sha256;};
    require(current_sha()==p->sha,"Wonder Init selected rules changed");
    // GC collection spans include sparse holes. Init stores no building flavor
    // weights and calls no IsWonder/city/buildability getter at this point.
    const auto flavor_count=detail::bnw_native_max_rows(rules,"Flavors");
    const auto buildings=rules.prefetch_info("Buildings",{},{1024*1024});
    require(buildings.all_succeeded() && buildings.slots<=1024*1024 &&
            current_sha()==p->sha,"Wonder Init selected Info unavailable");
    p->wonder_flavors.assign(static_cast<std::size_t>(flavor_count),0);
    p->trace.push_back("CvWonderProductionAI::CvFlavorRecipient::Init");
    // SAME player allocation, not a detached Player DTO. The pointer is never
    // dereferenced by this Init/Reset; no later AI operation is exposed.
    p->wonder_player=p.get();
    p->wonder_is_city=false;
    p->trace.push_back("CvWonderProductionAI::selected_buildings_player_isCity_bound");
    p->wonder_weights.clear();
    for(std::int32_t i=0;i<static_cast<std::int32_t>(buildings.slots);++i)
        p->wonder_weights.emplace_back(i,0);
    p->trace.push_back("CvWonderProductionAI::Reset weights");
    p->trace.push_back("m_pWonderProductionAI->Init");
    // No strategy entry, priority evaluation, flavor update or RNG is reached.
    // Init only binds the collection span/player, allocates three arrays, Reset.
    const auto strategies=rules.prefetch_info("AIGrandStrategies",{},{1024*1024});
    require(strategies.all_succeeded() && strategies.slots<=1024*1024 &&
            current_sha()==p->sha,"GrandStrategy Init selected Info unavailable");
    p->grand_strategy=p->uninit.grand_strategy.state;
    auto& grand=p->grand_strategy;
    grand.strategies_info_bound=true;
    p->grand_player=p.get();
    grand.player_bound=true;
    p->trace.push_back("CvGrandStrategyAI::selected_strategies_player_bound");
    require(!grand.strategy_priorities,"GrandStrategy priority array already allocated");
    grand.strategy_priorities.emplace(static_cast<std::size_t>(strategies.slots));
    p->trace.push_back("CvGrandStrategyAI::allocate priorities");
    require(!grand.other_player_strategy_guesses,"GrandStrategy guesses already allocated");
    grand.other_player_strategy_guesses.emplace(native_max_major_civs);
    p->trace.push_back("CvGrandStrategyAI::allocate guesses");
    require(!grand.other_player_guess_confidence,"GrandStrategy confidence already allocated");
    grand.other_player_guess_confidence.emplace(native_max_major_civs);
    p->trace.push_back("CvGrandStrategyAI::allocate confidence");
    grand.turns_since_active_set=0;
    p->trace.push_back("CvGrandStrategyAI::m_iNumTurnsSinceActiveSet");
    grand.active_strategy=no_ai_grand_strategy;
    p->trace.push_back("CvGrandStrategyAI::m_eActiveGrandStrategy");
    for(std::size_t i=0;i<grand.strategy_priorities->size();++i)
        (*grand.strategy_priorities)[i]=-1;
    p->trace.push_back("CvGrandStrategyAI::Reset priorities");
    for(std::size_t i=0;i<native_max_major_civs;++i) {
        (*grand.other_player_strategy_guesses)[i]=no_ai_grand_strategy;
        (*grand.other_player_guess_confidence)[i]=no_guess_confidence_type;
    }
    p->trace.push_back("CvGrandStrategyAI::Reset guesses and confidence");
    p->trace.push_back("m_pGrandStrategyAI->Init");
    // Actual SAME constructed diplomacy object already held by Military Init.
    auto& diplo=p->diplomacy;
    require(diplo.uninit==&p->uninit.diplomacy.state && !diplo.uninit->owned_diplo_data &&
            diplo.uninit->other_player_major_opinion_alias==BnwFreshDiplomacyAliasState::null_alias &&
            std::all_of(diplo.uninit->null_written_aliases.begin(),diplo.uninit->null_written_aliases.end(),
                [](auto a){return a==BnwFreshDiplomacyAliasState::null_alias;}),
            "Diplomacy constructor/Uninit ownership unavailable");
    diplo.player=p.get();
    p->trace.push_back("CvDiplomacyAI::m_pPlayer SAME allocation");
    diplo.allocate_data();
    p->trace.push_back("CvDiplomacyAI::DiplomacyAIData constructor allocation");
    diplo.bind_one("m_paDiploLogStatementTurnCountScratchPad","m_aDiploLogStatementTurnCountScratchPad");
    diplo.bind_one("m_paeMajorCivOpinion","m_aiMajorCivOpinion");
    diplo.bind_one("m_paeMajorCivApproach","m_aeMajorCivApproach");
    diplo.bind_one("m_paeApproachScratchPad","m_aeApproachScratchPad");
    diplo.bind_one("m_paeMinorCivApproach","m_aeMinorCivApproach");
    diplo.bind_one("m_paeApproachTowardsUsGuess","m_aeApproachTowardsUsGuess");
    diplo.bind_one("m_paeApproachTowardsUsGuessCounter","m_aeApproachTowardsUsGuessCounter");
    diplo.bind_one("m_paeWantPeaceCounter","m_aeWantPeaceCounter");
    diplo.bind_one("m_paePeaceTreatyWillingToOffer","m_aePeaceTreatyWillingToOffer");
    diplo.bind_one("m_paePeaceTreatyWillingToAccept","m_aePeaceTreatyWillingToAccept");
    diplo.bind_one("m_paiNumWondersBeatenTo","m_aiNumWondersBeatenTo");
    diplo.bind_one("m_pabMusteringForAttack","m_abMusteringForAttack");
    diplo.bind_one("m_pabWantsResearchAgreementWithPlayer","m_abWantsResearchAgreementWithPlayer");
    diplo.bind_one("m_pabWantToRouteToMinor","m_abWantToRouteToMinor");
    diplo.bind_one("m_paeWarFace","m_aeWarFace");
    diplo.bind_one("m_paeWarState","m_aeWarState");
    diplo.bind_one("m_paeWarProjection","m_aeWarProjection");
    diplo.bind_one("m_paeLastWarProjection","m_aeLastWarProjection");
    diplo.bind_one("m_paeWarGoal","m_aeWarGoal");
    diplo.bind_one("m_paiPlayerNumTurnsAtWar","m_aiPlayerNumTurnsAtWar");
    diplo.bind_one("m_paiNumWarsFought","m_aiNumWarsFought");
    diplo.bind_one("m_paePlayerMilitaryStrengthComparedToUs","m_aePlayerMilitaryStrengthComparedToUs");
    diplo.bind_one("m_paePlayerEconomicStrengthComparedToUs","m_aePlayerEconomicStrengthComparedToUs");
    diplo.bind_one("m_paePlayerTargetValue","m_aePlayerTargetValue");
    diplo.bind_one("m_paePlayerLandDisputeLevel","m_aePlayerLandDisputeLevel");
    diplo.bind_one("m_paePlayerLastTurnLandDisputeLevel","m_aePlayerLastTurnLandDisputeLevel");
    diplo.bind_one("m_paePlayerVictoryDisputeLevel","m_aePlayerVictoryDisputeLevel");
    diplo.bind_one("m_paePlayerWonderDisputeLevel","m_aePlayerWonderDisputeLevel");
    diplo.bind_one("m_paePlayerMinorCivDisputeLevel","m_aePlayerMinorCivDisputeLevel");
    diplo.bind_one("m_paeMilitaryAggressivePosture","m_aeMilitaryAggressivePosture");
    diplo.bind_one("m_paeLastTurnMilitaryAggressivePosture","m_aeLastTurnMilitaryAggressivePosture");
    diplo.bind_one("m_paeExpansionAggressivePosture","m_aeExpansionAggressivePosture");
    diplo.bind_one("m_paePlotBuyingAggressivePosture","m_aePlotBuyingAggressivePosture");
    diplo.bind_one("m_paeMilitaryThreat","m_aeMilitaryThreat");
    diplo.bind_one("m_paeWarDamageLevel","m_aeWarDamageLevel");
    diplo.bind_one("m_paiWarValueLost","m_aiWarValueLost");
    diplo.bind_one("m_paeWarmongerThreat","m_aeWarmongerThreat");
    diplo.bind_one("m_paiPersonalityMajorCivApproachBiases","m_aiPersonalityMajorCivApproachBiases");
    diplo.bind_one("m_paiPersonalityMinorCivApproachBiases","m_aiPersonalityMinorCivApproachBiases");
    diplo.bind_one("m_paDeclarationsLog.m_eDeclaration","m_aDeclarationsLog.m_eDeclaration");
    diplo.bind_one("m_paDeclarationsLog.m_iData1","m_aDeclarationsLog.m_iData1");
    diplo.bind_one("m_paDeclarationsLog.m_iData2","m_aDeclarationsLog.m_iData2");
    diplo.bind_one("m_paDeclarationsLog.m_eMustHaveMetPlayer","m_aDeclarationsLog.m_eMustHaveMetPlayer");
    diplo.bind_one("m_paDeclarationsLog.m_bActive","m_aDeclarationsLog.m_bActive");
    diplo.bind_one("m_paDeclarationsLog.m_iTurn","m_aDeclarationsLog.m_iTurn");
    diplo.bind_one("m_pabPlayerNoSettleRequestAccepted","m_abPlayerNoSettleRequest");
    diplo.bind_one("m_paiPlayerNoSettleRequestCounter","m_aiPlayerNoSettleRequestCounter");
    diplo.bind_one("m_pabPlayerStopSpyingRequestAccepted","m_abPlayerStopSpyingRequest");
    diplo.bind_one("m_paiPlayerStopSpyingRequestCounter","m_aiPlayerStopSpyingRequestCounter");
    diplo.bind_one("m_paiDemandCounter","m_aiDemandCounter");
    diplo.bind_one("m_paiDemandTooSoonNumTurns","m_aiDemandTooSoonNumTurns");
    diplo.bind_one("m_pabDoFAccepted","m_abDoFAccepted");
    diplo.bind_one("m_paiDoFCounter","m_aiDoFCounter");
    diplo.bind_one("m_pabDenouncedPlayer","m_abDenouncedPlayer");
    diplo.bind_one("m_pabFriendDenouncedUs","m_abFriendDenouncedUs");
    diplo.bind_one("m_pabFriendDeclaredWarOnUs","m_abFriendDeclaredWarOnUs");
    diplo.bind_one("m_paiDenouncedPlayerCounter","m_aiDenouncedPlayerCounter");
    diplo.bind_one("m_paiNumRequestsRefused","m_aiNumRequestsRefused");
    diplo.bind_one("m_paiNumCiviliansReturnedToMe","m_aiNumCiviliansReturnedToMe");
    diplo.bind_one("m_paiNumLandmarksBuiltForMe","m_aiNumLandmarksBuiltForMe");
    diplo.bind_one("m_paiResurrectedOnTurn","m_aiResurrectedOnTurn");
    diplo.bind_one("m_paiNumTimesCultureBombed","m_aiNumTimesCultureBombed");
    diplo.bind_one("m_paiNegativeReligiousConversionPoints","m_paiNegativeReligiousConversionPoints");
    diplo.bind_one("m_paiNegativeArchaeologyPoints","m_paiNegativeArchaeologyPoints");
    diplo.bind_one("m_paiNumTimesNuked","m_aiNumTimesNuked");
    diplo.bind_one("m_paiNumTimesRobbedBy","m_aiNumTimesRobbedBy");
    diplo.bind_one("m_paiNumTimesIntrigueSharedBy","m_aiNumTimesIntrigueSharedBy");
    diplo.bind_one("m_paiBrokenExpansionPromiseValue","m_aiBrokenExpansionPromiseValue");
    diplo.bind_one("m_paiIgnoredExpansionPromiseValue","m_aiIgnoredExpansionPromiseValue");
    diplo.bind_one("m_paiBrokenBorderPromiseValue","m_aiBrokenBorderPromiseValue");
    diplo.bind_one("m_paiIgnoredBorderPromiseValue","m_aiIgnoredBorderPromiseValue");
    diplo.bind_one("m_paiDeclaredWarOnFriendValue","m_aiDeclaredWarOnFriendValue");
    diplo.bind_one("m_paiNumCitiesLiberated","m_aiNumCitiesLiberated");
    diplo.bind_one("m_paiTradeValue","m_aiTradeValue");
    diplo.bind_one("m_paiCommonFoeValue","m_aiCommonFoeValue");
    diplo.bind_one("m_paiAssistValue","m_aiAssistValue");
    diplo.bind_one("m_pabPlayerMadeMilitaryPromise","m_abPlayerMadeMilitaryPromise");
    diplo.bind_one("m_pabPlayerBrokenMilitaryPromise","m_abPlayerBrokenMilitaryPromise");
    diplo.bind_one("m_pabPlayerIgnoredMilitaryPromise","m_abPlayerIgnoredMilitaryPromise");
    diplo.bind_one("m_paiPlayerMilitaryPromiseCounter","m_aiPlayerMilitaryPromiseCounter");
    diplo.bind_one("m_paiPlayerMadeExpansionPromiseTurn","m_aiPlayerMadeExpansionPromiseTurn");
    diplo.bind_one("m_pabPlayerBrokenExpansionPromise","m_abPlayerBrokenExpansionPromise");
    diplo.bind_one("m_pabPlayerIgnoredExpansionPromise","m_abPlayerIgnoredExpansionPromise");
    diplo.bind_one("m_paePlayerExpansionPromiseData","m_aePlayerExpansionPromiseData");
    diplo.bind_one("m_paiPlayerMadeBorderPromiseTurn","m_aiPlayerMadeBorderPromiseTurn");
    diplo.bind_one("m_pabPlayerBrokenBorderPromise","m_abPlayerBrokenBorderPromise");
    diplo.bind_one("m_pabPlayerIgnoredBorderPromise","m_abPlayerIgnoredBorderPromise");
    diplo.bind_one("m_paePlayerBorderPromiseData","m_aePlayerBorderPromiseData");
    diplo.bind_one("m_pabPlayerMadeAttackCityStatePromise","m_abPlayerMadeAttackCityStatePromise");
    diplo.bind_one("m_pabPlayerBrokenAttackCityStatePromise","m_abPlayerBrokenAttackCityStatePromise");
    diplo.bind_one("m_pabPlayerIgnoredAttackCityStatePromise","m_abPlayerIgnoredAttackCityStatePromise");
    diplo.bind_one("m_pabPlayerMadeBullyCityStatePromise","m_abPlayerMadeBullyCityStatePromise");
    diplo.bind_one("m_pabPlayerBrokenBullyCityStatePromise","m_abPlayerBrokenBullyCityStatePromise");
    diplo.bind_one("m_pabPlayerIgnoredBullyCityStatePromise","m_abPlayerIgnoredBullyCityStatePromise");
    diplo.bind_one("m_pabPlayerMadeSpyPromise","m_abPlayerMadeSpyPromise");
    diplo.bind_one("m_pabPlayerBrokenSpyPromise","m_abPlayerBrokenSpyPromise");
    diplo.bind_one("m_pabPlayerIgnoredSpyPromise","m_abPlayerIgnoredSpyPromise");
    diplo.bind_one("m_pabPlayerForgaveForSpying","m_abPlayerForgaveForSpying");
    diplo.bind_one("m_pabPlayerMadeNoConvertPromise","m_abPlayerMadeNoConvertPromise");
    diplo.bind_one("m_pabPlayerBrokenNoConvertPromise","m_abPlayerBrokenNoConvertPromise");
    diplo.bind_one("m_pabPlayerIgnoredNoConvertPromise","m_abPlayerIgnoredNoConvertPromise");
    diplo.bind_one("m_pabPlayerAskedNotToConvert","m_abPlayerAskedNotToConvert");
    diplo.bind_one("m_pabPlayerAgreedNotToConvert","m_abPlayerAgreedNotToConvert");
    diplo.bind_one("m_pabPlayerMadeNoDiggingPromise","m_abPlayerMadeNoDiggingPromise");
    diplo.bind_one("m_pabPlayerBrokenNoDiggingPromise","m_abPlayerBrokenNoDiggingPromise");
    diplo.bind_one("m_pabPlayerIgnoredNoDiggingPromise","m_abPlayerIgnoredNoDiggingPromise");
    diplo.bind_one("m_pabPlayerAskedNotToDig","m_abPlayerAskedNotToDig");
    diplo.bind_one("m_pabPlayerAgreedNotToDig","m_abPlayerAgreedNotToDig");
    diplo.bind_one("m_pabPlayerBrokenCoopWarPromise","m_abPlayerBrokenCoopWarPromise");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceAttackedProtectedMinor","m_aiOtherPlayerTurnsSinceAttackedProtectedMinor");
    diplo.bind_one("m_paiOtherPlayerProtectedMinorAttacked","m_aiOtherPlayerProtectedMinorAttacked");
    diplo.bind_one("m_paiOtherPlayerNumProtectedMinorsAttacked","m_aiOtherPlayerNumProtectedMinorsAttacked");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceKilledProtectedMinor","m_aiOtherPlayerTurnsSinceKilledProtectedMinor");
    diplo.bind_one("m_paiOtherPlayerProtectedMinorKilled","m_aiOtherPlayerProtectedMinorKilled");
    diplo.bind_one("m_paiOtherPlayerNumProtectedMinorsKilled","m_aiOtherPlayerNumProtectedMinorsKilled");
    diplo.bind_one("m_paiOtherPlayerNumProtectedMinorsBullied","m_aiOtherPlayerNumProtectedMinorsBullied");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceSidedWithTheirProtectedMinor","m_aiOtherPlayerTurnsSinceSidedWithTheirProtectedMinor");
    diplo.bind_one("m_paiOtherPlayerNumMinorsAttacked","m_aiOtherPlayerNumMinorsAttacked");
    diplo.bind_one("m_paiOtherPlayerNumMinorsConquered","m_aiOtherPlayerNumMinorsConquered");
    diplo.bind_one("m_paiOtherPlayerNumMajorsAttacked","m_aiOtherPlayerNumMajorsAttacked");
    diplo.bind_one("m_paiOtherPlayerNumMajorsConquered","m_aiOtherPlayerNumMajorsConquered");
    diplo.bind_one("m_paiOtherPlayerWarmongerAmount","m_aiOtherPlayerWarmongerAmount");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceWeLikedTheirProposal","m_aiOtherPlayerTurnsSinceWeLikedTheirProposal");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceWeDislikedTheirProposal","m_aiOtherPlayerTurnsSinceWeDislikedTheirProposal");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceTheySupportedOurProposal","m_aiOtherPlayerTurnsSinceTheySupportedOurProposal");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceTheyFoiledOurProposal","m_aiOtherPlayerTurnsSinceTheyFoiledOurProposal");
    diplo.bind_one("m_paiOtherPlayerTurnsSinceTheySupportedOurHosting","m_aiOtherPlayerTurnsSinceTheySupportedOurHosting");
    diplo.bind_rows("m_ppaaeOtherPlayerMajorCivOpinion","m_apaeOtherPlayerMajorCivOpinion","m_aaeOtherPlayerMajorCivOpinion",22,22);
    diplo.bind_rows("m_ppaaeOtherPlayerLandDisputeLevel","m_apaeOtherPlayerLandDisputeLevel","m_aaeOtherPlayerLandDisputeLevel",63,80);
    diplo.bind_rows("m_ppaaeOtherPlayerVictoryDisputeLevel","m_apaeOtherPlayerVictoryDisputeLevel","m_aaeOtherPlayerVictoryDisputeLevel",63,80);
    diplo.bind_rows("m_ppaaeOtherPlayerWarDamageLevel","m_apaeOtherPlayerWarDamageLevel","m_aaeOtherPlayerWarDamageLevel",63,80);
    diplo.bind_rows("m_ppaaiOtherPlayerWarValueLost","m_apaiOtherPlayerWarValueLost","m_aaiOtherPlayerWarValueLost",63,80);
    diplo.bind_rows("m_ppaaiOtherPlayerLastRoundWarValueLost","m_apaiOtherPlayerLastRoundWarValueLost","m_aaiOtherPlayerLastRoundWarValueLost",63,80);
    diplo.bind_rows("m_ppaabSentAttackMessageToMinorCivProtector","m_apabSentAttackMessageToMinorCivProtector","m_aabSentAttackMessageToMinorCivProtector",63,80);
    diplo.bind_rows("m_ppaaeOtherPlayerMilitaryThreat","m_apaeOtherPlayerMilitaryThreat","m_aaeOtherPlayerMilitaryThreat",63,80);
    diplo.bind_rows("m_ppaDiploStatementsLog.m_eDiploLogStatement","m_apaDiploStatementsLog","m_aaDiploStatementsLog.m_eDiploLogStatement",22,60);
    diplo.bind_rows("m_ppaDiploStatementsLog.m_iTurn","m_apaDiploStatementsLog","m_aaDiploStatementsLog.m_iTurn",22,60);
    diplo.bind_rows("m_ppaabWorkingAgainstPlayerAccepted","m_apabWorkingAgainstPlayerAccepted","m_aabWorkingAgainstPlayerAccepted",22,22);
    diplo.bind_rows("m_ppaaiWorkingAgainstPlayerCounter","m_apaiWorkingAgainstPlayerCounter","m_aaiWorkingAgainstPlayerCounter",22,22);
    diplo.bind_rows("m_ppaacCoopWarAcceptedState","m_apacCoopWarAcceptedState","m_aacCoopWarAcceptedState",22,22);
    diplo.bind_rows("m_ppaaiCoopWarCounter","m_apaiCoopWarCounter","m_aaiCoopWarCounter",22,22);
    p->trace.push_back("CvDiplomacyAI::Init native array aliases");

	std::size_t iI=0, iJ=0;

	for(iI = 0; iI < 86; iI++)
	{
		diplo.store("m_paDiploLogStatementTurnCountScratchPad",0,iI,-1);
	}

	for(iI = 0; iI < 22; iI++)
	{
		diplo.store("m_paeMajorCivOpinion",0,iI,-1);

		for(iJ = 0; iJ < 22; iJ++)
		{
			diplo.store("m_ppaaeOtherPlayerMajorCivOpinion",iI,iJ,-1);
			diplo.store("m_ppaabWorkingAgainstPlayerAccepted",iI,iJ,0);
			diplo.store("m_ppaaiWorkingAgainstPlayerCounter",iI,iJ,-1);
			diplo.store("m_ppaacCoopWarAcceptedState",iI,iJ,-1);
			diplo.store("m_ppaaiCoopWarCounter",iI,iJ,-1);
		}

		diplo.store("m_paeMajorCivApproach",0,iI,-1);
		diplo.store("m_paeApproachTowardsUsGuess",0,iI,-1);
		diplo.store("m_paeApproachTowardsUsGuessCounter",0,iI,0);

		diplo.store("m_paePeaceTreatyWillingToOffer",0,iI,-1);
		diplo.store("m_paePeaceTreatyWillingToAccept",0,iI,-1);

		diplo.store("m_paiOtherPlayerTurnsSinceAttackedProtectedMinor",0,iI,-1);
		diplo.store("m_paiOtherPlayerProtectedMinorAttacked",0,iI,-1);
		diplo.store("m_paiOtherPlayerNumProtectedMinorsAttacked",0,iI,0);

		diplo.store("m_paiOtherPlayerTurnsSinceKilledProtectedMinor",0,iI,-1);
		diplo.store("m_paiOtherPlayerProtectedMinorKilled",0,iI,-1);
		diplo.store("m_paiOtherPlayerNumProtectedMinorsKilled",0,iI,0);

		diplo.store("m_paiOtherPlayerNumProtectedMinorsBullied",0,iI,0);

		diplo.store("m_paiOtherPlayerTurnsSinceSidedWithTheirProtectedMinor",0,iI,-1);

		diplo.store("m_paiOtherPlayerNumMinorsAttacked",0,iI,0);
		diplo.store("m_paiOtherPlayerNumMinorsConquered",0,iI,0);
		diplo.store("m_paiOtherPlayerNumMajorsAttacked",0,iI,0);
		diplo.store("m_paiOtherPlayerNumMajorsConquered",0,iI,0);
		
		diplo.store("m_paiOtherPlayerWarmongerAmount",0,iI,0);

		diplo.store("m_paiOtherPlayerTurnsSinceWeLikedTheirProposal",0,iI,-1);
		diplo.store("m_paiOtherPlayerTurnsSinceWeDislikedTheirProposal",0,iI,-1);
		diplo.store("m_paiOtherPlayerTurnsSinceTheySupportedOurProposal",0,iI,-1);
		diplo.store("m_paiOtherPlayerTurnsSinceTheyFoiledOurProposal",0,iI,-1);
		diplo.store("m_paiOtherPlayerTurnsSinceTheySupportedOurHosting",0,iI,-1);

		diplo.store("m_pabWantsResearchAgreementWithPlayer",0,iI,0);

		

		diplo.store("m_pabPlayerNoSettleRequestAccepted",0,iI,0);
		diplo.store("m_paiPlayerNoSettleRequestCounter",0,iI,-1);

		diplo.store("m_pabPlayerStopSpyingRequestAccepted",0,iI,0);
		diplo.store("m_paiPlayerStopSpyingRequestCounter",0,iI,-1);

		diplo.store("m_paiDemandCounter",0,iI,-1);
		diplo.store("m_paiDemandTooSoonNumTurns",0,iI,-1);

		diplo.store("m_pabDoFAccepted",0,iI,0);
		diplo.store("m_paiDoFCounter",0,iI,-1);

		diplo.store("m_pabDenouncedPlayer",0,iI,0);
		diplo.store("m_pabFriendDenouncedUs",0,iI,0);
		diplo.store("m_pabFriendDeclaredWarOnUs",0,iI,0);
		diplo.store("m_paiDenouncedPlayerCounter",0,iI,-1);

		diplo.store("m_paiNumRequestsRefused",0,iI,0);

		diplo.store("m_paiNumCiviliansReturnedToMe",0,iI,0);
		diplo.store("m_paiNumLandmarksBuiltForMe",0,iI,0);
		diplo.store("m_paiResurrectedOnTurn",0,iI,-1);
		diplo.store("m_paiNumTimesCultureBombed",0,iI,0);

		diplo.store("m_paiNegativeReligiousConversionPoints",0,iI,0);

		diplo.store("m_paiNegativeArchaeologyPoints",0,iI,0);

		diplo.store("m_paiNumTimesNuked",0,iI,0);
		diplo.store("m_paiNumTimesRobbedBy",0,iI,0);
		diplo.store("m_paiNumTimesIntrigueSharedBy",0,iI,0);

		diplo.store("m_paiBrokenExpansionPromiseValue",0,iI,0);
		diplo.store("m_paiIgnoredExpansionPromiseValue",0,iI,0);
		diplo.store("m_paiBrokenBorderPromiseValue",0,iI,0);
		diplo.store("m_paiIgnoredBorderPromiseValue",0,iI,0);
		diplo.store("m_paiDeclaredWarOnFriendValue",0,iI,0);
		diplo.store("m_paiNumCitiesLiberated",0,iI,0);
		diplo.store("m_paiTradeValue",0,iI,0);
		diplo.store("m_paiCommonFoeValue",0,iI,0);
		diplo.store("m_paiAssistValue",0,iI,0);

		

		diplo.store("m_pabPlayerMadeMilitaryPromise",0,iI,0);
		diplo.store("m_pabPlayerBrokenMilitaryPromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredMilitaryPromise",0,iI,0);
		diplo.store("m_paiPlayerMilitaryPromiseCounter",0,iI,-1);

		diplo.store("m_paiPlayerMadeExpansionPromiseTurn",0,iI,-1);
		diplo.store("m_pabPlayerBrokenExpansionPromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredExpansionPromise",0,iI,0);
		diplo.store("m_paePlayerExpansionPromiseData",0,iI,-1);

		diplo.store("m_paiPlayerMadeBorderPromiseTurn",0,iI,-1);
		diplo.store("m_pabPlayerBrokenBorderPromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredBorderPromise",0,iI,0);
		diplo.store("m_paePlayerBorderPromiseData",0,iI,-1);

		diplo.store("m_pabPlayerMadeAttackCityStatePromise",0,iI,0);
		diplo.store("m_pabPlayerBrokenAttackCityStatePromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredAttackCityStatePromise",0,iI,0);

		diplo.store("m_pabPlayerMadeBullyCityStatePromise",0,iI,0);
		diplo.store("m_pabPlayerBrokenBullyCityStatePromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredBullyCityStatePromise",0,iI,0);

		diplo.store("m_pabPlayerMadeSpyPromise",0,iI,0);
		diplo.store("m_pabPlayerBrokenSpyPromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredSpyPromise",0,iI,0);

		diplo.store("m_pabPlayerForgaveForSpying",0,iI,0);

		diplo.store("m_pabPlayerMadeNoConvertPromise",0,iI,0);
		diplo.store("m_pabPlayerBrokenNoConvertPromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredNoConvertPromise",0,iI,0);
		diplo.store("m_pabPlayerAskedNotToConvert",0,iI,0);
		diplo.store("m_pabPlayerAgreedNotToConvert",0,iI,0);

		diplo.store("m_pabPlayerMadeNoDiggingPromise",0,iI,0);
		diplo.store("m_pabPlayerBrokenNoDiggingPromise",0,iI,0);
		diplo.store("m_pabPlayerIgnoredNoDiggingPromise",0,iI,0);
		diplo.store("m_pabPlayerAskedNotToDig",0,iI,0);
		diplo.store("m_pabPlayerAgreedNotToDig",0,iI,0);

		diplo.store("m_pabPlayerBrokenCoopWarPromise",0,iI,0);
	}

	for(iI = 0; iI < 41; iI++)
	{
		diplo.store("m_paeMinorCivApproach",0,iI,-1);
		diplo.store("m_pabWantToRouteToMinor",0,iI,1);
	}

	for(iI = 0; iI < 63; iI++)
	{
		diplo.store("m_paeApproachScratchPad",0,iI,-1);

		diplo.store("m_paiNumWondersBeatenTo",0,iI,0);
		diplo.store("m_pabMusteringForAttack",0,iI,0);

		diplo.store("m_paeWantPeaceCounter",0,iI,0);

		diplo.store("m_paeWarFace",0,iI,-1);
		diplo.store("m_paeWarState",0,iI,-1);
		diplo.store("m_paeWarProjection",0,iI,-1);
		diplo.store("m_paeLastWarProjection",0,iI,-1);
		diplo.store("m_paeWarGoal",0,iI,-1);
		diplo.store("m_paiPlayerNumTurnsAtWar",0,iI,0);
		diplo.store("m_paiNumWarsFought",0,iI,0);

		diplo.store("m_paePlayerMilitaryStrengthComparedToUs",0,iI,-1);
		diplo.store("m_paePlayerEconomicStrengthComparedToUs",0,iI,-1);
		diplo.store("m_paePlayerTargetValue",0,iI,-1);

		diplo.store("m_paePlayerLandDisputeLevel",0,iI,-1);
		diplo.store("m_paePlayerLastTurnLandDisputeLevel",0,iI,-1);
		diplo.store("m_paePlayerVictoryDisputeLevel",0,iI,-1);
		diplo.store("m_paePlayerWonderDisputeLevel",0,iI,-1);
		diplo.store("m_paePlayerMinorCivDisputeLevel",0,iI,-1);

		for(iJ = 0; iJ < 63; iJ++)
		{
			diplo.store("m_ppaaeOtherPlayerLandDisputeLevel",iI,iJ,-1);
			diplo.store("m_ppaaeOtherPlayerVictoryDisputeLevel",iI,iJ,-1);

			diplo.store("m_ppaaeOtherPlayerWarDamageLevel",iI,iJ,-1);
			diplo.store("m_ppaaiOtherPlayerWarValueLost",iI,iJ,0);
			diplo.store("m_ppaaiOtherPlayerLastRoundWarValueLost",iI,iJ,0);
			diplo.store("m_ppaabSentAttackMessageToMinorCivProtector",iI,iJ,0);

			diplo.store("m_ppaaeOtherPlayerMilitaryThreat",iI,iJ,-1);
		}

		diplo.store("m_paeWarDamageLevel",0,iI,-1);
		diplo.store("m_paiWarValueLost",0,iI,0);

		diplo.store("m_paeMilitaryAggressivePosture",0,iI,-1);
		diplo.store("m_paeLastTurnMilitaryAggressivePosture",0,iI,-1);
		diplo.store("m_paeExpansionAggressivePosture",0,iI,-1);
		diplo.store("m_paePlotBuyingAggressivePosture",0,iI,-1);

		diplo.store("m_paeMilitaryThreat",0,iI,-1);

		diplo.store("m_paeWarmongerThreat",0,iI,-1);
	}

	diplo.scalar_store("m_eDemandTargetPlayer",-1);
	diplo.scalar_store("m_bDemandReady",0);

	diplo.scalar_store("m_iVictoryCompetitiveness",-1);
	diplo.scalar_store("m_iWonderCompetitiveness",-1);
	diplo.scalar_store("m_iMinorCivCompetitiveness",-1);
	diplo.scalar_store("m_iBoldness",-1);
	diplo.scalar_store("m_iDiploBalance",-1);
	diplo.scalar_store("m_iWarmongerHate",-1);
	diplo.scalar_store("m_iDenounceWillingness",-1);
	diplo.scalar_store("m_iDoFWillingness",-1);
	diplo.scalar_store("m_iLoyalty",-1);
	diplo.scalar_store("m_iNeediness",-1);
	diplo.scalar_store("m_iForgiveness",-1);
	diplo.scalar_store("m_iChattiness",-1);
	diplo.scalar_store("m_iMeanness",-1);

	for(iI = 0; iI < 7; iI++)
	{
		diplo.store("m_paiPersonalityMajorCivApproachBiases",0,iI,0);
	}

	for(iI = 0; iI < 5; iI++)
	{
		diplo.store("m_paiPersonalityMinorCivApproachBiases",0,iI,0);
	}

	for(iI = 0; iI < 22; iI++)
	{
		for(iJ = 0; iJ < 60; iJ++)
		{
			diplo.store("m_ppaDiploStatementsLog.m_eDiploLogStatement",iI,iJ,-1);
			diplo.store("m_ppaDiploStatementsLog.m_iTurn",iI,iJ,-1);
		}
	}

	for(iI = 0; iI < 60; iI++)
	{
		diplo.store("m_paDeclarationsLog.m_eDeclaration",0,iI,-1);
		diplo.store("m_paDeclarationsLog.m_iTurn",0,iI,-1);
	}

	diplo.scalar_store("m_eStateAllWars",0);

	diplo.greet_players.clear();

    p->trace.push_back("CvDiplomacyAI::Reset gameplay stores");
    p->trace.push_back("m_pDiplomacyAI->Init");
    // CvPlayerReligions::Init binds the real owner before Reset; Reset has
    // only these two scalar stores. It calls no Info/Lua/RNG/world getter.
    p->religions_player=p.get();
    p->religions.player_bound=true;
    p->trace.push_back("CvPlayerReligions::m_pPlayer SAME allocation");
    p->religions.founding_religion=0; // native int assigned false
    p->trace.push_back("CvPlayerReligions::m_bFoundingReligion");
    p->religions.prophets_spawned=0;
    p->trace.push_back("CvPlayerReligions::m_iNumProphetsSpawned");
    p->trace.push_back("m_pReligions->Init");
    // Caller-side materialization of the existing Rules parent's collection.
    // These DB reads are NOT CvReligionAI::Reset or native Init gameplay stores.
    require(current_sha()==p->sha,"ReligionAI selected rules changed");
    const auto beliefs=rules.prefetch_info("Beliefs",{},{1024*1024});
    require(beliefs.all_succeeded() && beliefs.slots<=1024*1024 &&
            current_sha()==p->sha,"ReligionAI Init selected Beliefs unavailable");
    p->belief_info=beliefs;
    p->trace.push_back("Rules parent::Beliefs source collection materialization");
    // Exact native Init order: m_pBeliefs, m_pPlayer, then empty Reset.
    p->religion_ai_beliefs=&p->belief_info;
    p->religion_ai.beliefs_bound=true;
    p->trace.push_back("CvReligionAI::m_pBeliefs SAME source collection");
    p->religion_ai_player=p.get();
    p->religion_ai.player_bound=true;
    p->trace.push_back("CvReligionAI::m_pPlayer SAME allocation");
    p->trace.push_back("CvReligionAI::Reset empty body");
    p->trace.push_back("m_pReligionAI->Init");
    // Real constructor-owned Pregame primitive state, followed by selected setup
    // values and the already ordered InitPlayers setMinorCiv writes. This is not
    // complete setCivilization/bindKeys or full Pregame/UI readiness.
    require(!inputs.major_color_slots.empty() && !inputs.pregame_leader_ids.empty() &&
            inputs.major_color_slots.size()<=native_max_players &&
            inputs.pregame_leader_ids.size()<=native_max_players,
            "PlayerTechs selected Pregame inputs unavailable");
    for(std::size_t i=0;i<inputs.major_color_slots.size();++i)
        p->pregame.civilization[i]=inputs.major_color_slots[i].civilization_id;
    for(std::size_t i=0;i<inputs.pregame_leader_ids.size();++i)
        p->pregame.leader[i]=inputs.pregame_leader_ids[i];
    for(const auto& e:d.slot_info.source_order) {
        require(e.player_id>=0 && static_cast<std::size_t>(e.player_id)<native_max_players,
                "PlayerTechs selected Pregame slot out of bounds");
        const auto i=static_cast<std::size_t>(e.player_id);
        if(e.field==BnwFreshSlotWriteField::minor_civ) {
            require(e.value==0 || e.value==1,"PlayerTechs selected Pregame minor flag malformed");
            p->pregame.minor[i]=e.value==1;
        } else if(e.field==BnwFreshSlotWriteField::civilization)p->pregame.civilization[i]=e.value;
        else if(e.field==BnwFreshSlotWriteField::leader)p->pregame.leader[i]=e.value;
    }
    require(p->pregame.leader[0]==p->early.personality,"PlayerTechs SAME Pregame leader mismatch");
    p->trace.push_back("Pregame primitive constructor/setup/InitPlayers minor writes");
    // Caller materializes the existing GameTechs collection. Fingerprints do
    // not authorize future Tech Entries or a full game initialization.
    require(current_sha()==p->sha,"PlayerTechs selected rules changed");
    p->tech_info=tech_source_collection(rules,"Technologies");
    const auto tech_types=rules.cache_info_types();
    require(tech_types.report().all_succeeded() && current_sha()==p->sha,
            "PlayerTechs selected source type cache unavailable");
    p->techs=p->uninit.player_techs.state;
    const auto n=static_cast<std::size_t>(p->tech_info.report.slots);
    // Native Init: base flavor allocation first, then false/tech/player binding.
    require(!p->techs.latest_flavor_values,"PlayerTechs flavor allocation already exists");
    p->techs.latest_flavor_values.emplace(static_cast<std::size_t>(flavor_count),0);
    p->trace.push_back("CvPlayerTechs::CvFlavorRecipient::Init zeros");
    p->techs.is_city=false;p->trace.push_back("CvPlayerTechs::m_bIsCity false");
    p->techs_info=&p->tech_info;p->techs.tech_entries_bound=true;
    p->trace.push_back("CvPlayerTechs::m_pTechs SAME source collection");
    p->techs_player=p.get();p->techs.player_bound=true;
    p->trace.push_back("CvPlayerTechs::m_pPlayer SAME allocation");
    require(!p->techs.researching_tech && !p->techs.civ_tech_priority && !p->techs.locale_tech_priority &&
            !p->techs.locale_tech_resources && !p->techs.civ_tech_unique_units &&
            !p->techs.civ_tech_unique_buildings && !p->techs.civ_tech_unique_improvements &&
            !p->techs.tech_ai_weights,"PlayerTechs constructor pointers are not null");
    p->techs.researching_tech.emplace(n);p->trace.push_back("CvPlayerTechs::allocate researching");
    p->techs.civ_tech_priority.emplace(n);p->trace.push_back("CvPlayerTechs::allocate civ priority");
    p->techs.locale_tech_priority.emplace(n);p->trace.push_back("CvPlayerTechs::allocate locale priority");
    p->techs.locale_tech_resources.emplace(n);p->trace.push_back("CvPlayerTechs::allocate resources");
    p->techs.civ_tech_unique_units.emplace(n);p->trace.push_back("CvPlayerTechs::allocate unique units");
    p->techs.civ_tech_unique_buildings.emplace(n);p->trace.push_back("CvPlayerTechs::allocate unique buildings");
    p->techs.civ_tech_unique_improvements.emplace(n);p->trace.push_back("CvPlayerTechs::allocate unique improvements");
    p->tech_ai_current=&p->techs;p->techs.tech_ai_weights.emplace();
    p->trace.push_back("CvTechAI::constructor SAME PlayerTechs");
    const auto game_buildings=tech_source_collection(rules,"Buildings");
    p->trace.push_back("CvPlayerTechs::Reset GameBuildings pointer");
    for(std::size_t i=0;i<n;++i) {
        (*p->techs.researching_tech)[i]=false;
        (*p->techs.civ_tech_priority)[i]=1;
        (*p->techs.locale_tech_priority)[i]=1;
        (*p->techs.locale_tech_resources)[i]=-1;
        (*p->techs.civ_tech_unique_units)[i]=-1;
        (*p->techs.civ_tech_unique_buildings)[i]=-1;
        (*p->techs.civ_tech_unique_improvements)[i]=-1;
        p->trace.push_back("CvPlayerTechs::Reset tech slot false/1/1/-1/-1/-1/-1");
    }
    const auto multiply=[&](std::int32_t tech,const char* define) {
        require(tech>=0 && static_cast<std::size_t>(tech)<n,"PlayerTechs priority tech index unavailable");
        const auto i=static_cast<std::size_t>(tech);
        const auto factor=tech_source_define(rules,define);
        (*p->techs.civ_tech_priority)[i]=tech_native_i32(
            std::int64_t{(*p->techs.civ_tech_priority)[i]}*factor);
        p->trace.push_back("CvPlayerTechs::Reset reached native priority multiply");
    };
    // Actual first-player ID=0, native BARBARIAN_PLAYER=63. Predicates read the
    // SAME constructor-owned Pregame fields, never caller supplied bool grants.
    if(!p->pregame.minor[0] && p->early.id!=63 && p->pregame.civilization[0]!=-1) {
        const auto civilizations=tech_source_collection(rules,"Civilizations");
        if(const auto* civ=civilizations.find(p->pregame.civilization[0])) {
            require(!civ->type.empty(),"PlayerTechs civilization Type unavailable");
            const auto bc=tech_source_collection(rules,"BuildingClasses");
            const auto uc=tech_source_collection(rules,"UnitClasses");
            const auto mappings=tech_civ_mappings(rules,civ->type,bc,uc);
            for(std::size_t i=0;i<static_cast<std::size_t>(bc.report.slots);++i) {
                require(i<mappings.building.size(),"PlayerTechs reached building class read unavailable");
                const auto building=mappings.building[i];
                const auto* entry=building!=-1?game_buildings.find(building):nullptr;
                if(entry && mappings.building_overridden[i]) {
                    const auto tech=tech_source_enum(tech_types,entry->prereq);
                    if(tech!=-1) {multiply(tech,"TECH_PRIORITY_UNIQUE_ITEM");
                        (*p->techs.civ_tech_unique_buildings)[static_cast<std::size_t>(tech)]=building;
                        p->trace.push_back("CvPlayerTechs::Reset unique building");}
                }
            }
            const auto units=tech_source_collection(rules,"Units");
            for(std::size_t i=0;i<static_cast<std::size_t>(uc.report.slots);++i) {
                require(i<mappings.unit_overridden.size(),"PlayerTechs reached unit class read unavailable");
                if(mappings.unit_overridden[i]) {
                const auto unit=mappings.unit[i];
                const auto* entry=unit!=-1?units.find(unit):nullptr;
                if(entry) {const auto tech=tech_source_enum(tech_types,entry->prereq);
                    if(tech!=-1) {multiply(tech,"TECH_PRIORITY_UNIQUE_ITEM");
                        (*p->techs.civ_tech_unique_units)[static_cast<std::size_t>(tech)]=unit;
                        p->trace.push_back("CvPlayerTechs::Reset unique unit");}}
                }
            }
            const auto improvements=tech_source_collection(rules,"Improvements");
            const auto builds=tech_source_collection(rules,"Builds");
            for(const auto& [i,entry]:improvements.entries)
                if(entry.specific_civ && tech_source_enum(tech_types,entry.required_civ)==p->pregame.civilization[0])
                    for(const auto& [j,build]:builds.entries) {
                        static_cast<void>(j);
                        if(tech_source_enum(tech_types,build.improvement)==i) {
                            const auto tech=tech_source_enum(tech_types,build.prereq);
                            if(tech!=-1) {multiply(tech,"TECH_PRIORITY_UNIQUE_ITEM");
                                (*p->techs.civ_tech_unique_improvements)[static_cast<std::size_t>(tech)]=i;
                                p->trace.push_back("CvPlayerTechs::Reset unique improvement");}
                        }
                    }
        }
        // Traits are outside the civilization Info-null conditional.
        const auto traits=tech_source_collection(rules,"Traits");
        if(traits.report.slots!=0) {
            const auto leaders=tech_source_collection(rules,"Leaders");
            const auto* leader=leaders.find(p->pregame.leader[0]);
            require(leader && !leader->type.empty(),"PlayerTechs native Leader Info unavailable");
            std::vector<bool> has_trait(static_cast<std::size_t>(detail::bnw_native_max_rows(rules,"Traits")),false);
            const auto relation=rules.query_rows({"select Traits.ID from Traits inner join Leader_Traits on Traits.Type = TraitType where LeaderType = ?",{leader->type}},
                [&](const content::RulesRowView& row) {
                    const auto id=tech_native_i32(row.integer_at(0));
                    require(id>=0 && static_cast<std::size_t>(id)<has_trait.size(),"PlayerTechs leader trait index unavailable");
                    has_trait[static_cast<std::size_t>(id)]=true;
                },tech_init_budget);
            require(relation.all_succeeded(),"PlayerTechs native Leader Traits relation unavailable");
            for(std::size_t i=0;i<static_cast<std::size_t>(traits.report.slots);++i) {
                require(i<has_trait.size(),"PlayerTechs reached Leader hasTrait read unavailable");
                if(has_trait[i]) {
                const auto* trait=traits.find(static_cast<std::int32_t>(i));
                if(trait && trait->maya) {
                    const auto tech=tech_source_enum(tech_types,trait->prereq);
                    if(tech!=-1)multiply(tech,"TECH_PRIORITY_MAYA_CALENDAR_BONUS");
                }
                }
            }
        }
    }
    require(p->tech_ai_current==&p->techs && p->techs_info==&p->tech_info && p->techs_player==p.get(),
            "PlayerTechs owned pointer binding changed");
    p->techs.tech_ai_weights->clear();p->trace.push_back("CvTechAI::Reset clear weights");
    for(std::size_t i=0;i<n;++i) {
        p->techs.tech_ai_weights->emplace_back(static_cast<std::int32_t>(i),0);
        p->trace.push_back("CvTechAI::Reset push native slot zero");
    }
    require(current_sha()==p->sha,"PlayerTechs selected rules changed after Reset");
    p->trace.push_back("m_pPlayerTechs->Init");



    require(current_sha()==p->sha,"PlayerPolicies selected rules changed before Init");
    p->policy_info=policy_source_collection(rules);
    require(current_sha()==p->sha,"PlayerPolicies selected rules changed during materialization");
    const auto policy_count=static_cast<std::size_t>(p->policy_info.policies.slots);
    const auto branch_count=static_cast<std::size_t>(p->policy_info.branches.slots);
    auto& policy=p->policies;
    // CvFlavorRecipient::Init uses the same GC native MaxRows flavor count.
    require(!policy.latest_flavors,"PlayerPolicies flavor allocation already exists");
    policy.latest_flavors.emplace(static_cast<std::size_t>(flavor_count),0);
    p->trace.push_back("CvPlayerPolicies::CvFlavorRecipient::Init zeros");
    policy.is_city=false;p->trace.push_back("CvPlayerPolicies::m_bIsCity false");
    p->policies_info=&p->policy_info;p->trace.push_back("CvPlayerPolicies::m_pPolicies SAME collection");
    p->policies_player=p.get();p->trace.push_back("CvPlayerPolicies::m_pPlayer SAME allocation");
    policy.has_policy.emplace(policy_count);p->trace.push_back("CvPlayerPolicies::allocate HasPolicy");
    policy.one_shot.emplace(policy_count);p->trace.push_back("CvPlayerPolicies::allocate OneShotPolicyFired");
    policy.one_shot_free_units.emplace(policy_count);p->trace.push_back("CvPlayerPolicies::allocate OneShotFreeUnitsFired");
    policy.branch_unlocked.emplace(branch_count);p->trace.push_back("CvPlayerPolicies::allocate BranchUnlocked");
    policy.branch_blocked.emplace(branch_count);p->trace.push_back("CvPlayerPolicies::allocate BranchBlocked");
    policy.branch_finished.emplace(branch_count);p->trace.push_back("CvPlayerPolicies::allocate BranchFinished");
    policy.branches_chosen.emplace(branch_count);p->trace.push_back("CvPlayerPolicies::allocate BranchesChosen");
    policy.blocked_branch_check.emplace(policy_count);p->trace.push_back("CvPlayerPolicies::allocate BlockedBranchCheck");
    policy.weights.emplace();p->policy_ai_current=&policy;
    p->trace.push_back("CvPolicyAI::constructor SAME PlayerPolicies");
    for(std::size_t i=0;i<policy_count;++i) {
        (*policy.has_policy)[i]=false;(*policy.one_shot)[i]=false;
        (*policy.one_shot_free_units)[i]=false;(*policy.blocked_branch_check)[i]=-2;
        p->trace.push_back("CvPlayerPolicies::Reset policy slot false/false/false/-2");
    }
    for(std::size_t i=0;i<branch_count;++i) {
        (*policy.branch_unlocked)[i]=false;(*policy.branch_blocked)[i]=false;
        (*policy.branch_finished)[i]=false;(*policy.branches_chosen)[i]=-1;
        p->trace.push_back("CvPlayerPolicies::Reset branch slot false/false/false/-1");
    }
    policy.num_extra_branches=0;p->trace.push_back("CvPlayerPolicies::Reset NumExtraBranches0");
    policy.branch_picked1=-1;p->trace.push_back("CvPlayerPolicies::Reset BranchPicked1-1");
    policy.branch_picked2=-1;p->trace.push_back("CvPlayerPolicies::Reset BranchPicked2-1");
    policy.branch_picked3=-1;p->trace.push_back("CvPlayerPolicies::Reset BranchPicked3-1");
    require(p->policy_ai_current==&policy && p->policies_info==&p->policy_info && p->policies_player==p.get(),
            "PlayerPolicies owned pointer binding changed");
    policy.weights->clear();p->trace.push_back("CvPolicyAI::Reset clear weights");
    policy.propagation_levels=tech_source_define(rules,"POLICY_WEIGHT_PROPAGATION_LEVELS");
    p->trace.push_back("CvPolicyAI::Reset cached PropagationLevels");
    policy.percent_drop_new_branch=tech_source_define(rules,"POLICY_WEIGHT_PERCENT_DROP_NEW_BRANCH");
    p->trace.push_back("CvPolicyAI::Reset cached PercentDropNewBranch");
    for(std::size_t i=0;i<policy_count;++i) {
        policy.weights->emplace_back(static_cast<std::int32_t>(i),0);
        p->trace.push_back("CvPolicyAI::Reset push native slot zero");
    }
    for(std::size_t i=0;i<policy_count;++i) {
        const auto slot=static_cast<std::int32_t>(i);
        const auto found=p->policy_info.policy_branches.find(slot);
        (*policy.blocked_branch_check)[i]=found==p->policy_info.policy_branches.end()?-1:
            tech_native_i32(detail::bnw_policy_blocked_branch(slot,found->second,p->policy_info.branch_openers));
        p->trace.push_back("CvPlayerPolicies::Reset final native blocked branch");
    }
    require(current_sha()==p->sha,"PlayerPolicies selected rules changed after Reset");
    p->trace.push_back("m_pPlayerPolicies->Init");

    // CvTacticalAI::Init240: bind player, then the actual Reset323. Do not
    // replace the following Game pointer getter with a constructed dummy map.
    p->tactical_player=p.get();
    p->trace.push_back("CvTacticalAI::Init SAME player");
    p->tactical_move_priority_turn=-1;
    p->trace.push_back("CvTacticalAI::Reset MovePriorityTurn-1");
    p->tactical_map_reset=nullptr;
    p->trace.push_back("CvTacticalAI::Reset MapNULL");
    require(p->tactical_player==p.get(),"TacticalAI actual player binding changed");
    require(current_sha()==p->sha,"TacticalAI selected rules changed after Reset");
    // No Game owner has been issued by this builder. Preserve the reached
    // prefix; the next pointer read and ALL subsequent operands remain pending.

    return std::shared_ptr<const BnwFreshFirstPlayerState>(new BnwFreshFirstPlayerState(std::move(p)));
}
void BnwFreshFirstPlayerState::complete_tactical_init_from_game(const BnwFreshTacticalMap& actual_map,
    const content::RulesDatabase& rules,std::int32_t actual_game_handicap)const {
    auto& p=*impl_;require(p.tactical_player==impl_.get()&&p.tactical_map_reset.has_value()&&!p.tactical_init_attempted,
        "Tactical Init requires SAME actual first Player binding/Reset once");
    p.tactical_init_attempted=true;
    require(identify_bytes(rules.gameplay_fingerprint_data()).sha256==p.sha,"Actual first Tactical Init selected Rules changed");
    (void)actual_map.observe();p.tactical_game_map=&actual_map;
    p.trace.push_back("CvTacticalAI::Init actual SAME Game GetTacticalAnalysisMap");
    const auto integer=[&](const char* name){const auto source=rules.global_define(name,content::GlobalDefineKind::integer);
        require(source.error.empty()&&std::holds_alternative<std::int64_t>(source.value),"Actual Tactical cached integer Define unavailable");
        return tech_native_i32(std::get<std::int64_t>(source.value));};
    p.tactical_init_values.emplace("recruit_range",integer("AI_TACTICAL_RECRUIT_RANGE"));p.trace.push_back("CvTacticalAI::Init RecruitRange");
    std::int32_t land=0,sea=0;const auto handicap=rules.query_rows({"SELECT * FROM HandicapInfos WHERE ID = ? LIMIT 1",{static_cast<std::int64_t>(actual_game_handicap)}},
        [&](const content::RulesRowView& row){require(row.has_column("BarbarianLandTargetRange")&&row.has_column("BarbarianSeaTargetRange"),"Actual Game Handicap tactical fields missing");
            land=tech_native_i32(row.integer("BarbarianLandTargetRange"));sea=tech_native_i32(row.integer("BarbarianSeaTargetRange"));},1);
    require(handicap.all_succeeded()&&handicap.rows==1,"Actual owning Game Handicap source unavailable");
    p.tactical_init_values.emplace("land_barbarian_range",land);p.trace.push_back("CvTacticalAI::Init Game Handicap LandBarbarianRange");
    p.tactical_init_values.emplace("sea_barbarian_range",sea);p.trace.push_back("CvTacticalAI::Init Game Handicap SeaBarbarianRange");
    p.tactical_init_values.emplace("reposition_range",integer("AI_TACTICAL_REPOSITION_RANGE"));p.trace.push_back("CvTacticalAI::Init RepositionRange");
    p.tactical_init_values.emplace("deploy_radius",integer("AI_OPERATIONAL_CITY_ATTACK_DEPLOY_RANGE"));p.trace.push_back("CvTacticalAI::Init DeployRadius");
    p.tactical_init_values.emplace("random_range",integer("AI_TACTICAL_MOVE_PRIORITY_RANDOMNESS"));p.trace.push_back("CvTacticalAI::Init RandomRange");
    const auto flavor=rules.global_define("AI_TACTICAL_FLAVOR_DAMPENING_FOR_MOVE_PRIORITIZATION",content::GlobalDefineKind::source_float);
    require(flavor.error.empty()&&std::holds_alternative<double>(flavor.value),"Actual Tactical source float Define unavailable");
    p.tactical_flavor_dampening=static_cast<float>(std::get<double>(flavor.value));p.trace.push_back("CvTacticalAI::Init FlavorDampening source float");
    const auto types=rules.cache_info_types();require(types.report().all_succeeded(),"Actual Tactical native Info type source failed");
    struct Cached {std::string_view key,type;};const Cached cache[]{
#include "bnw_new_game_tactical_cached_types.inc"
    };
    for(const auto& entry:cache){const auto* found=types.find(entry.type);const auto id=found?tech_native_i32(found->id):-1;
        p.tactical_cached_types.emplace_back(entry.key,id);p.trace.push_back(entry.key);
    }
    require(p.tactical_game_map==&actual_map,"Tactical Init lost SAME actual Game pointer");(void)actual_map.observe();
    require(identify_bytes(rules.gameplay_fingerprint_data()).sha256==p.sha,"Tactical selected Rules changed before return");
    p.trace.push_back("CvTacticalAI::Init actual normal return; HomelandAI required");
}
bool BnwFreshFirstPlayerState::tactical_init_map_is_bound()const{return impl_->tactical_game_map!=nullptr;}
std::int32_t BnwFreshFirstPlayerState::tactical_init_integer(std::string_view name)const{return impl_->tactical_init_values.at(name);}
float BnwFreshFirstPlayerState::tactical_init_flavor_dampening()const{return impl_->tactical_flavor_dampening.value();}
std::span<const std::pair<std::string_view,std::int32_t>> BnwFreshFirstPlayerState::tactical_init_cached_types()const{return impl_->tactical_cached_types;}

const BnwArchiveMemberState& BnwFreshFirstPlayerState::registered_member(std::string_view name) const {
    return member(impl_->uninit.archive.defaults,name);
}
const BnwArchiveValue& BnwFreshFirstPlayerState::unregistered_scalar(std::string_view name) const {
    const auto found=std::find_if(impl_->unregistered.begin(),impl_->unregistered.end(),
                                [&](const auto& s){return s.first==name;});
    if(found==impl_->unregistered.end()) throw std::out_of_range("Unknown native unregistered Player scalar");
    return found->second;
}
const BnwFreshEconomicAIState& BnwFreshFirstPlayerState::economic() const {return impl_->economic;}
const BnwFreshMilitaryAIState& BnwFreshFirstPlayerState::military() const {return impl_->military;}
const BnwFreshCitySpecializationState& BnwFreshFirstPlayerState::city_specialization() const {return impl_->specialization;}
std::span<const std::int32_t> BnwFreshFirstPlayerState::wonder_latest_flavors() const {return impl_->wonder_flavors;}
std::span<const std::pair<std::int32_t,std::int32_t>> BnwFreshFirstPlayerState::wonder_weights() const {return impl_->wonder_weights;}
std::span<const std::pair<std::int32_t,std::int32_t>> BnwFreshFirstPlayerState::wonder_buildables() const {return impl_->wonder_buildables;}
bool BnwFreshFirstPlayerState::wonder_is_city() const {return impl_->wonder_is_city;}
std::span<const std::int32_t> BnwFreshFirstPlayerState::grand_strategy_priorities() const {return impl_->grand_strategy.strategy_priorities.value();}
std::span<const std::int32_t> BnwFreshFirstPlayerState::grand_strategy_guesses() const {return impl_->grand_strategy.other_player_strategy_guesses.value();}
std::span<const std::int32_t> BnwFreshFirstPlayerState::grand_strategy_guess_confidence() const {return impl_->grand_strategy.other_player_guess_confidence.value();}
std::int32_t BnwFreshFirstPlayerState::grand_strategy_turns_since_active_set() const {return impl_->grand_strategy.turns_since_active_set.value();}
std::int32_t BnwFreshFirstPlayerState::grand_strategy_active() const {return impl_->grand_strategy.active_strategy.value();}
std::optional<std::int32_t> BnwFreshFirstPlayerState::diplomacy_array_value(std::string_view alias,std::size_t row,std::size_t col) const {return impl_->diplomacy.value(alias,row,col);}
std::int32_t BnwFreshFirstPlayerState::diplomacy_scalar(std::string_view name) const {return impl_->diplomacy.scalars.at(name);}
std::uint64_t BnwFreshFirstPlayerState::diplomacy_reset_write_count() const {return impl_->diplomacy.reset_writes;}
std::int32_t BnwFreshFirstPlayerState::religions_prophets_spawned() const {return impl_->religions.prophets_spawned;}
std::int32_t BnwFreshFirstPlayerState::religions_founding_religion() const {return impl_->religions.founding_religion;}
std::int32_t BnwFreshFirstPlayerState::religion_ai_belief_slots() const {return static_cast<std::int32_t>(impl_->religion_ai_beliefs->slots);}
std::span<const std::int32_t> BnwFreshFirstPlayerState::tech_latest_flavors() const {return *impl_->techs.latest_flavor_values;}
std::span<const std::pair<std::int32_t,std::int32_t>> BnwFreshFirstPlayerState::tech_weights() const {return *impl_->techs.tech_ai_weights;}
bool BnwFreshFirstPlayerState::tech_researching(std::size_t i) const {return impl_->techs.researching_tech->at(i);}
std::int32_t BnwFreshFirstPlayerState::tech_reset_value(std::string_view name,std::size_t i) const {
    if(name=="civ_priority")return impl_->techs.civ_tech_priority->at(i);
    if(name=="locale_priority")return impl_->techs.locale_tech_priority->at(i);
    if(name=="resource")return impl_->techs.locale_tech_resources->at(i);
    if(name=="unit")return impl_->techs.civ_tech_unique_units->at(i);
    if(name=="building")return impl_->techs.civ_tech_unique_buildings->at(i);
    if(name=="improvement")return impl_->techs.civ_tech_unique_improvements->at(i);
    throw std::invalid_argument("Unknown owned PlayerTechs array");
}
bool BnwFreshFirstPlayerState::tech_pregame_minor() const {return impl_->pregame.minor[0];}
std::int32_t BnwFreshFirstPlayerState::tech_pregame_civilization() const {return impl_->pregame.civilization[0];}
std::int32_t BnwFreshFirstPlayerState::tech_pregame_leader() const {return impl_->pregame.leader[0];}
std::int32_t BnwFreshFirstPlayerState::tactical_reset_value(std::string_view name) const {
    if(name=="current_zone_index") return impl_->tactical.current_zone_index;
    if(name=="move_priority_turn") return impl_->tactical_move_priority_turn.value();
    throw std::out_of_range("TacticalAI field is not reached by this prefix");
}
bool BnwFreshFirstPlayerState::tactical_reset_map_is_null() const {
    return impl_->tactical_map_reset.has_value(); // Actual Reset store, not Game getter.
}
std::span<const std::int32_t> BnwFreshFirstPlayerState::policy_latest_flavors() const {return *impl_->policies.latest_flavors;}
std::span<const std::pair<std::int32_t,std::int32_t>> BnwFreshFirstPlayerState::policy_weights() const {return *impl_->policies.weights;}
bool BnwFreshFirstPlayerState::policy_flag(std::string_view name,std::size_t i) const {
    if(name=="has_policy")return impl_->policies.has_policy->at(i);
    if(name=="one_shot")return impl_->policies.one_shot->at(i);
    if(name=="one_shot_free_units")return impl_->policies.one_shot_free_units->at(i);
    if(name=="branch_unlocked")return impl_->policies.branch_unlocked->at(i);
    if(name=="branch_blocked")return impl_->policies.branch_blocked->at(i);
    if(name=="branch_finished")return impl_->policies.branch_finished->at(i);
    throw std::invalid_argument("Unknown owned PlayerPolicies flag array");
}
std::int32_t BnwFreshFirstPlayerState::policy_reset_value(std::string_view name,std::size_t i) const {
    if(name=="blocked_branch")return impl_->policies.blocked_branch_check->at(i);
    if(name=="branch_chosen")return impl_->policies.branches_chosen->at(i);
    throw std::invalid_argument("Unknown owned PlayerPolicies enum array");
}
std::int32_t BnwFreshFirstPlayerState::policy_scalar(std::string_view name) const {
    const auto& p=impl_->policies;
    if(name=="is_city")return p.is_city?1:0;
    if(name=="extra_branches")return p.num_extra_branches.value();
    if(name=="branch_picked1")return p.branch_picked1.value();
    if(name=="branch_picked2")return p.branch_picked2.value();
    if(name=="branch_picked3")return p.branch_picked3.value();
    if(name=="propagation_levels")return p.propagation_levels.value();
    if(name=="percent_drop_new_branch")return p.percent_drop_new_branch.value();
    throw std::invalid_argument("Unknown owned PlayerPolicies scalar");
}
std::span<const std::int32_t> BnwFreshFirstPlayerState::great_work_yield_change() const {return impl_->early.yield_vectors[7];}
std::span<const std::int32_t> BnwFreshFirstPlayerState::siphon_luxury_count() const {return impl_->early.siphon_luxury_count;}
std::string_view BnwFreshFirstPlayerState::embarked_graphic_override() const {return impl_->early.embarked_graphic_override;}
std::span<const std::string_view> BnwFreshFirstPlayerState::source_order() const {return impl_->trace;}
std::string_view BnwFreshFirstPlayerState::selected_rules_sha256() const {return impl_->sha;}
} // namespace civ5::runtime
