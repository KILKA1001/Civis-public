#include "civ5/runtime/bnw_fresh_player_uninit_plan.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::string_view,21> kEarlyVectors{
    "m_paiNumResourceUsed", "m_paiNumResourceTotal",
    "m_paiResourceGiftedToMinors", "m_paiResourceExport",
    "m_paiResourceImport", "m_paiResourceFromMinors",
    "m_paiResourcesSiphoned", "m_paiImprovementCount",
    "m_paiFreeBuildingCount", "m_paiFreePromotionCount",
    "m_paiUnitCombatProductionModifiers", "m_paiUnitCombatFreeExperiences",
    "m_paiUnitClassCount", "m_paiUnitClassMaking",
    "m_paiBuildingClassCount", "m_paiBuildingClassMaking",
    "m_paiProjectMaking", "m_paiHurryCount", "m_paiHurryModifier",
    "m_pabLoyalMember", "m_pabGetsScienceFromPlayer"};
constexpr std::array<std::string_view,22> kFirstServices{
    "m_pPlayerPolicies", "m_pEconomicAI", "m_pMilitaryAI",
    "m_pCitySpecializationAI", "m_pWonderProductionAI",
    "m_pGrandStrategyAI", "m_pDiplomacyAI", "m_pReligions",
    "m_pReligionAI", "m_pEspionage", "m_pEspionageAI", "m_pTrade",
    "m_pTradeAI", "m_pLeagueAI", "m_pPlayerTechs",
    "m_pFlavorManager", "m_pTacticalAI", "m_pHomelandAI",
    "m_pMinorCivAI", "m_pDealAI", "m_pBuilderTaskingAI",
    "m_pCityConnections"};
} // namespace

bool BnwFreshPlayerUninitPlan::has_blocker(
    BnwFreshPlayerUninitBlockerCode code) const {
    return std::any_of(blockers.begin(),blockers.end(),
                       [code](const auto &item) { return item.code == code; });
}
void BnwFreshPlayerUninitPlan::rollback() noexcept {
    player_id = -1;
    planned_source_order.clear();
    selected_rules_sha256.clear();
    blockers.clear();
    preflight_complete = false;
    sync_archive_reset_verified = false;
    uninit_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerUninitPlan plan_bnw_fresh_first_player_uninit(
    const BnwFreshGameContentResult &selected,
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256) {
    BnwFreshPlayerUninitPlan plan;
    if (!selected.content_ready() || !selected.rules || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !objects.constructors_complete || objects.initialized_by_gamecore ||
        !objects.has_blocker(BnwFreshFirstPlayerObjectBlockerCode::
                              reset_sync_archive_and_uninit_pending) ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256 !=
            selected_rules_sha256) {
        plan.blockers.push_back({
            BnwFreshPlayerUninitBlockerCode::first_player_or_rules_unverified,
            "Selected first player constructors must precede reset/uninit"});
        return plan;
    }
    plan.player_id = objects.player_id;
    plan.selected_rules_sha256 = std::string(selected_rules_sha256);
    const auto append = [&](BnwFreshPlayerUninitKind kind,
                            std::string_view member) {
        plan.planned_source_order.push_back({kind,member});
    };
    append(BnwFreshPlayerUninitKind::sync_archive_reset,"m_syncArchive");
    for (auto member : kEarlyVectors)
        append(BnwFreshPlayerUninitKind::vector_clear,member);
    for (auto member : kFirstServices)
        append(BnwFreshPlayerUninitKind::service_uninit,member);
    append(BnwFreshPlayerUninitKind::optional_service_uninit,
           "m_pNotifications");
    append(BnwFreshPlayerUninitKind::optional_service_uninit,
           "m_pDiplomacyRequests");
    append(BnwFreshPlayerUninitKind::service_uninit,"m_pTreasury");
    append(BnwFreshPlayerUninitKind::service_uninit,"m_pTraits");
    append(BnwFreshPlayerUninitKind::optional_service_uninit,
           "m_pDangerPlots");
    append(BnwFreshPlayerUninitKind::later_player_state_reset,
           "m_ppaaiSpecialistExtraYield and remaining CvPlayer::uninit");
    plan.preflight_complete = true;
    plan.blockers.push_back({
        BnwFreshPlayerUninitBlockerCode::sync_archive_reset_semantics_unavailable,
        "Installed FirePlace SDK declares FAutoArchive::reset without its implementation"});
    plan.blockers.push_back({
        BnwFreshPlayerUninitBlockerCode::service_uninit_effects_pending,
        "CvPlayerPolicies::Uninit and following service callbacks require owned service state"});
    return plan;
}
} // namespace civ5::runtime
