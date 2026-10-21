#include "civ5/runtime/bnw_fresh_espionage_uninit.hpp"
#include <algorithm>
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

} // namespace

void BnwFreshEspionageUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    remaining_services_complete = false;
    initialized_by_gamecore = false;
}

std::optional<BnwFreshEspionageServiceState>
make_bnw_fresh_espionage_constructor_state(
    std::span<const content::GameplayDefine> native_constants) {
    const auto count = selected_max_major_civs(native_constants);
    if (!count) return std::nullopt;
    BnwFreshEspionageServiceState result;
    result.max_tech_cost.assign(static_cast<std::size_t>(*count),-1);
    result.heist_locations.resize(static_cast<std::size_t>(*count));
    return result;
}

BnwFreshEspionageUninitDraft project_bnw_fresh_espionage_uninit(
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
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    BnwFreshEspionageServiceState initial) {
    BnwFreshEspionageUninitDraft result;
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
        initial.spies.size() > 8 ||
        initial.stealable_techs.size() > static_cast<std::size_t>(*count) ||
        initial.techs_to_steal.size() > static_cast<std::size_t>(*count) ||
        std::any_of(initial.stealable_techs.begin(),
                    initial.stealable_techs.end(),
                    [](const auto &list) { return list.size() > 1; }) ||
        initial.max_tech_cost.size() != static_cast<std::size_t>(*count) ||
        initial.heist_locations.size() != static_cast<std::size_t>(*count))
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // CvEspionageClasses.cpp:158-170. Reset does not read or notify the
    // world; all effects apply to the detached owned service state.
    result.state.player_bound = false;
    result.source_order.push_back({"m_pPlayer",-1});
    result.state.spies.clear();
    result.source_order.push_back({"m_aSpyList",-1});
    result.state.spy_name_order.clear();
    result.source_order.push_back({"m_aiSpyListNameOrder",-1});
    result.state.spy_name_order_index = -1;
    result.source_order.push_back({"m_iSpyListNameOrderIndex",-1});
    result.state.techs_to_steal.clear();
    result.source_order.push_back({"m_aiNumTechsToStealList",-1});
    result.state.intrigue_messages.clear();
    result.source_order.push_back({"m_aIntrigueNotificationMessages",-1});
    result.state.stealable_techs.clear();
    result.source_order.push_back({"m_aaPlayerStealableTechList",-1});
    for (std::int32_t major = 0; major < *count; ++major) {
        result.state.max_tech_cost[static_cast<std::size_t>(major)] = -1;
        result.source_order.push_back({"m_aiMaxTechCost",major});
        result.state.heist_locations[static_cast<std::size_t>(major)].clear();
        result.source_order.push_back({"m_aHeistLocations",major});
    }
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
