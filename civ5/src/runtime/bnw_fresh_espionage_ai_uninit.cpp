#include "civ5/runtime/bnw_fresh_espionage_ai_uninit.hpp"
#include <array>
#include <utility>

namespace civ5::runtime {
namespace {

std::optional<std::int32_t> selected_max_major_civs(
    std::span<const content::GameplayDefine> constants) {
    std::optional<std::int32_t> result;
    for (const auto &item : constants) {
        if (item.name != "MAX_MAJOR_CIVS") continue;
        if (result || item.value <= 0 || item.value > 64) return std::nullopt;
        result = static_cast<std::int32_t>(item.value);
    }
    return result;
}

struct VectorReset {
    std::string_view member_name;
    std::vector<std::int32_t> BnwFreshEspionageAiServiceState::* member;
    std::int32_t pushed_value;
};
constexpr std::array<VectorReset,7> kSourceVectors{{
    {"m_aiCivOutOfTechTurn",
     &BnwFreshEspionageAiServiceState::civ_out_of_tech_turn,-1},
    {"m_aiNumSpiesCaught",
     &BnwFreshEspionageAiServiceState::spies_caught,0},
    {"m_aiNumSpiesKilled",
     &BnwFreshEspionageAiServiceState::spies_killed,0},
    {"m_aiNumSpiesDied",
     &BnwFreshEspionageAiServiceState::spies_died,0},
    {"m_aiTurnLastSpyCaught",
     &BnwFreshEspionageAiServiceState::last_spy_caught,-1},
    {"m_aiTurnLastSpyKilled",
     &BnwFreshEspionageAiServiceState::last_spy_killed,-1},
    {"m_aiTurnLastSpyDied",
     &BnwFreshEspionageAiServiceState::last_spy_died,-1},
}};

} // namespace

void BnwFreshEspionageAiUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshEspionageAiServiceState
make_bnw_fresh_espionage_ai_constructor_state() {
    return {};
}

BnwFreshEspionageAiUninitDraft project_bnw_fresh_espionage_ai_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    const BnwFreshWonderUninitDraft &wonder,
    const BnwFreshGrandStrategyUninitDraft &grand_strategy,
    const BnwFreshDiplomacyUninitDraft &diplomacy,
    const BnwFreshReligionsUninitDraft &religions,
    const BnwFreshReligionAiUninitDraft &religion_ai,
    const BnwFreshEspionageUninitDraft &espionage,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    BnwFreshEspionageAiServiceState initial) {
    BnwFreshEspionageAiUninitDraft result;
    const auto count = selected_max_major_civs(native_constants);
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() || !count ||
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
        diplomacy.selected_rules_sha256 != selected_rules_sha256 ||
        !diplomacy.member_effect_complete || diplomacy.archive_predecessor_verified ||
        diplomacy.initialized_by_gamecore ||
        religions.selected_rules_sha256 != selected_rules_sha256 ||
        !religions.member_effect_complete || religions.archive_predecessor_verified ||
        religions.initialized_by_gamecore ||
        religion_ai.selected_rules_sha256 != selected_rules_sha256 ||
        !religion_ai.member_effect_complete ||
        religion_ai.archive_predecessor_verified ||
        religion_ai.initialized_by_gamecore ||
        espionage.selected_rules_sha256 != selected_rules_sha256 ||
        !espionage.member_effect_complete || espionage.archive_predecessor_verified ||
        espionage.initialized_by_gamecore)
        return result;
    for (const auto &item : kSourceVectors) {
        if ((initial.*(item.member)).size() > static_cast<std::size_t>(*count))
            return result;
    }

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    for (const auto &item : kSourceVectors) {
        auto &vector = result.state.*(item.member);
        vector.clear();
        result.source_order.push_back({
            item.member_name,BnwFreshEspionageAiUninitOperation::clear,-1});
        for (std::int32_t major=0; major<*count; ++major) {
            vector.push_back(item.pushed_value);
            result.source_order.push_back({
                item.member_name,BnwFreshEspionageAiUninitOperation::push_back,
                major});
        }
    }
    result.state.un_countdown_started = false;
    result.source_order.push_back({
        "m_bUNCountdownStarted",
        BnwFreshEspionageAiUninitOperation::scalar_write,-1});
    result.state.turn_espionage_started = -1;
    result.source_order.push_back({
        "m_iTurnEspionageStarted",
        BnwFreshEspionageAiUninitOperation::scalar_write,-1});
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
