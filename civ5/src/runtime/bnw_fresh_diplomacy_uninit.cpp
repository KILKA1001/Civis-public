#include "civ5/runtime/bnw_fresh_diplomacy_uninit.hpp"
#include <algorithm>
#include <array>
#include <utility>

namespace civ5::runtime {
namespace {

// Expansion2 CvDiplomacyAI.cpp:610-774. Generated from the ordered
// member = NULL assignments in the installed SDK, excluding the final
// m_pDiploData = NULL after delete. No other aliases are reset by Uninit.
constexpr std::array<std::string_view, 134> kNullWrittenAliases = {{
    "m_paDiploLogStatementTurnCountScratchPad",
    "m_paeMajorCivOpinion",
    "m_paeMajorCivApproach",
    "m_paeApproachScratchPad",
    "m_paeMinorCivApproach",
    "m_paeApproachTowardsUsGuess",
    "m_paeApproachTowardsUsGuessCounter",
    "m_paeWantPeaceCounter",
    "m_paePeaceTreatyWillingToOffer",
    "m_paePeaceTreatyWillingToAccept",
    "m_paiNumWondersBeatenTo",
    "m_pabMusteringForAttack",
    "m_pabWantsResearchAgreementWithPlayer",
    "m_pabWantToRouteToMinor",
    "m_paeWarFace",
    "m_paeWarState",
    "m_paeWarProjection",
    "m_paeLastWarProjection",
    "m_paeWarGoal",
    "m_paiPlayerNumTurnsAtWar",
    "m_paiNumWarsFought",
    "m_paePlayerMilitaryStrengthComparedToUs",
    "m_paePlayerEconomicStrengthComparedToUs",
    "m_paePlayerTargetValue",
    "m_paePlayerLandDisputeLevel",
    "m_paePlayerLastTurnLandDisputeLevel",
    "m_paePlayerVictoryDisputeLevel",
    "m_paePlayerWonderDisputeLevel",
    "m_paePlayerMinorCivDisputeLevel",
    "m_paeMilitaryAggressivePosture",
    "m_paeLastTurnMilitaryAggressivePosture",
    "m_paeExpansionAggressivePosture",
    "m_paePlotBuyingAggressivePosture",
    "m_paeMilitaryThreat",
    "m_paeWarDamageLevel",
    "m_paiWarValueLost",
    "m_paeWarmongerThreat",
    "m_paiPersonalityMajorCivApproachBiases",
    "m_paiPersonalityMinorCivApproachBiases",
    "m_paDeclarationsLog",
    "m_pabPlayerNoSettleRequestAccepted",
    "m_paiPlayerNoSettleRequestCounter",
    "m_pabPlayerStopSpyingRequestAccepted",
    "m_paiPlayerStopSpyingRequestCounter",
    "m_paiDemandCounter",
    "m_paiDemandTooSoonNumTurns",
    "m_pabDoFAccepted",
    "m_paiDoFCounter",
    "m_pabDenouncedPlayer",
    "m_pabFriendDenouncedUs",
    "m_pabFriendDeclaredWarOnUs",
    "m_paiDenouncedPlayerCounter",
    "m_paiNumRequestsRefused",
    "m_paiNumCiviliansReturnedToMe",
    "m_paiNumLandmarksBuiltForMe",
    "m_paiResurrectedOnTurn",
    "m_paiNumTimesCultureBombed",
    "m_paiNegativeReligiousConversionPoints",
    "m_paiNegativeArchaeologyPoints",
    "m_paiNumTimesNuked",
    "m_paiNumTimesRobbedBy",
    "m_paiNumTimesIntrigueSharedBy",
    "m_paiBrokenExpansionPromiseValue",
    "m_paiIgnoredExpansionPromiseValue",
    "m_paiBrokenBorderPromiseValue",
    "m_paiIgnoredBorderPromiseValue",
    "m_paiDeclaredWarOnFriendValue",
    "m_paiNumCitiesLiberated",
    "m_paiTradeValue",
    "m_paiCommonFoeValue",
    "m_paiAssistValue",
    "m_ppaabWorkingAgainstPlayerAccepted",
    "m_ppaaiWorkingAgainstPlayerCounter",
    "m_ppaacCoopWarAcceptedState",
    "m_ppaaiCoopWarCounter",
    "m_pabPlayerMadeMilitaryPromise",
    "m_pabPlayerBrokenMilitaryPromise",
    "m_pabPlayerIgnoredMilitaryPromise",
    "m_paiPlayerMilitaryPromiseCounter",
    "m_paiPlayerMadeExpansionPromiseTurn",
    "m_pabPlayerBrokenExpansionPromise",
    "m_pabPlayerIgnoredExpansionPromise",
    "m_paePlayerExpansionPromiseData",
    "m_paiPlayerMadeBorderPromiseTurn",
    "m_pabPlayerBrokenBorderPromise",
    "m_pabPlayerIgnoredBorderPromise",
    "m_paePlayerBorderPromiseData",
    "m_pabPlayerMadeAttackCityStatePromise",
    "m_pabPlayerBrokenAttackCityStatePromise",
    "m_pabPlayerIgnoredAttackCityStatePromise",
    "m_pabPlayerMadeBullyCityStatePromise",
    "m_pabPlayerBrokenBullyCityStatePromise",
    "m_pabPlayerIgnoredBullyCityStatePromise",
    "m_pabPlayerMadeSpyPromise",
    "m_pabPlayerBrokenSpyPromise",
    "m_pabPlayerIgnoredSpyPromise",
    "m_pabPlayerForgaveForSpying",
    "m_pabPlayerMadeNoConvertPromise",
    "m_pabPlayerBrokenNoConvertPromise",
    "m_pabPlayerIgnoredNoConvertPromise",
    "m_pabPlayerAskedNotToConvert",
    "m_pabPlayerAgreedNotToConvert",
    "m_pabPlayerMadeNoDiggingPromise",
    "m_pabPlayerBrokenNoDiggingPromise",
    "m_pabPlayerIgnoredNoDiggingPromise",
    "m_pabPlayerAskedNotToDig",
    "m_pabPlayerAgreedNotToDig",
    "m_pabPlayerBrokenCoopWarPromise",
    "m_paiOtherPlayerTurnsSinceAttackedProtectedMinor",
    "m_paiOtherPlayerProtectedMinorAttacked",
    "m_paiOtherPlayerNumProtectedMinorsAttacked",
    "m_paiOtherPlayerTurnsSinceKilledProtectedMinor",
    "m_paiOtherPlayerProtectedMinorKilled",
    "m_paiOtherPlayerNumProtectedMinorsKilled",
    "m_paiOtherPlayerNumProtectedMinorsBullied",
    "m_paiOtherPlayerTurnsSinceSidedWithTheirProtectedMinor",
    "m_paiOtherPlayerNumMinorsAttacked",
    "m_paiOtherPlayerNumMinorsConquered",
    "m_paiOtherPlayerNumMajorsAttacked",
    "m_paiOtherPlayerNumMajorsConquered",
    "m_paiOtherPlayerWarmongerAmount",
    "m_paiOtherPlayerTurnsSinceWeLikedTheirProposal",
    "m_paiOtherPlayerTurnsSinceWeDislikedTheirProposal",
    "m_paiOtherPlayerTurnsSinceTheySupportedOurProposal",
    "m_paiOtherPlayerTurnsSinceTheyFoiledOurProposal",
    "m_paiOtherPlayerTurnsSinceTheySupportedOurHosting",
    "m_ppaaeOtherPlayerLandDisputeLevel",
    "m_ppaaeOtherPlayerVictoryDisputeLevel",
    "m_ppaaeOtherPlayerWarDamageLevel",
    "m_ppaaiOtherPlayerWarValueLost",
    "m_ppaaiOtherPlayerLastRoundWarValueLost",
    "m_ppaabSentAttackMessageToMinorCivProtector",
    "m_ppaaeOtherPlayerMilitaryThreat",
    "m_ppaDiploStatementsLog",
}};

} // namespace

std::span<const std::string_view> bnw_fresh_diplomacy_uninit_aliases() {
    return kNullWrittenAliases;
}

void BnwFreshDiplomacyUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshDiplomacyServiceState make_bnw_fresh_diplomacy_constructor_state() {
    BnwFreshDiplomacyServiceState result;
    result.null_written_aliases.assign(
        kNullWrittenAliases.size(), BnwFreshDiplomacyAliasState::null_alias);
    return result;
}

BnwFreshDiplomacyUninitDraft project_bnw_fresh_diplomacy_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    const BnwFreshWonderUninitDraft &wonder,
    const BnwFreshGrandStrategyUninitDraft &grand_strategy,
    std::string_view selected_rules_sha256,
    BnwFreshDiplomacyServiceState initial) {
    BnwFreshDiplomacyUninitDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        policies.selected_rules_sha256 != selected_rules_sha256 ||
        !policies.member_effect_complete || policies.archive_predecessor_verified ||
        policies.initialized_by_gamecore ||
        economic.selected_rules_sha256 != selected_rules_sha256 ||
        !economic.member_effect_complete || economic.archive_predecessor_verified ||
        economic.initialized_by_gamecore ||
        military.selected_rules_sha256 != selected_rules_sha256 ||
        !military.member_effect_complete || military.archive_predecessor_verified ||
        military.initialized_by_gamecore ||
        city_specialization.selected_rules_sha256 != selected_rules_sha256 ||
        !city_specialization.member_effect_complete ||
        city_specialization.archive_predecessor_verified ||
        city_specialization.initialized_by_gamecore ||
        wonder.selected_rules_sha256 != selected_rules_sha256 ||
        !wonder.member_effect_complete || wonder.archive_predecessor_verified ||
        wonder.initialized_by_gamecore ||
        grand_strategy.selected_rules_sha256 != selected_rules_sha256 ||
        !grand_strategy.member_effect_complete ||
        grand_strategy.archive_predecessor_verified ||
        grand_strategy.initialized_by_gamecore ||
        initial.null_written_aliases.size() != kNullWrittenAliases.size() ||
        initial.other_player_major_opinion_alias ==
            BnwFreshDiplomacyAliasState::dangling_after_delete ||
        std::any_of(initial.null_written_aliases.begin(),
                    initial.null_written_aliases.end(),
                    [](BnwFreshDiplomacyAliasState alias) {
                        return alias == BnwFreshDiplomacyAliasState::dangling_after_delete;
                    }))
        return result;
    if (!initial.owned_diplo_data &&
        (initial.other_player_major_opinion_alias ==
             BnwFreshDiplomacyAliasState::bound_to_diplo_data ||
         std::any_of(initial.null_written_aliases.begin(),
                     initial.null_written_aliases.end(),
                     [](BnwFreshDiplomacyAliasState alias) {
                         return alias == BnwFreshDiplomacyAliasState::bound_to_diplo_data;
                     })))
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // CvDiplomacyAI.cpp:610-774 nulls 134 aliases, including aliases
    // into the owned DiplomacyAIData object. There is no value mutation.
    for (std::size_t index = 0; index < kNullWrittenAliases.size(); ++index) {
        result.state.null_written_aliases[index] =
            BnwFreshDiplomacyAliasState::null_alias;
        result.source_order.push_back(kNullWrittenAliases[index]);
    }
    // The SDK omits m_ppaaeOtherPlayerMajorCivOpinion from this reset.
    // Once the owning object is deleted, a formerly bound alias dangles.
    if (result.state.owned_diplo_data &&
        result.state.other_player_major_opinion_alias ==
            BnwFreshDiplomacyAliasState::bound_to_diplo_data)
        result.state.other_player_major_opinion_alias =
            BnwFreshDiplomacyAliasState::dangling_after_delete;
    result.state.owned_diplo_data.reset();
    result.source_order.push_back("delete m_pDiploData");
    result.source_order.push_back("m_pDiploData");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
