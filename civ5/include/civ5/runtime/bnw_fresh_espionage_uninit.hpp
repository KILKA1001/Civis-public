#pragma once

#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_fresh_religion_ai_uninit.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace civ5::runtime {

struct BnwFreshEspionageSpy {
    std::int32_t name = -1, city_x = -1, city_y = -1;
    std::int32_t rank = 0, state = 0, revive_counter = 0;
    bool diplomat = false, evaluate_reassignment = false;
    bool operator==(const BnwFreshEspionageSpy &) const = default;
};
struct BnwFreshSpyNotification {
    std::int32_t city_x = -1, city_y = -1, attacking_player = -1;
    std::int32_t spy_result = 0, stolen_tech = -1;
    bool operator==(const BnwFreshSpyNotification &) const = default;
};
struct BnwFreshIntrigueNotification {
    std::string spy_name;
    std::int32_t discovering_player = -1, source_player = -1;
    std::int32_t target_player = -1, city_x = -1, city_y = -1;
    bool shared = false;
    bool operator==(const BnwFreshIntrigueNotification &) const = default;
};

struct BnwFreshEspionageServiceState {
    bool player_bound = false;
    std::vector<BnwFreshEspionageSpy> spies;
    std::vector<std::int32_t> spy_name_order;
    std::int32_t spy_name_order_index = -1;
    std::vector<std::int32_t> techs_to_steal;
    std::vector<BnwFreshIntrigueNotification> intrigue_messages;
    std::vector<std::vector<std::int32_t>> stealable_techs;
    std::vector<std::int32_t> max_tech_cost;
    std::vector<std::vector<std::pair<std::int32_t,std::int32_t>>> heist_locations;
    // CvPlayerEspionage::Reset does not clear this separate message queue.
    std::vector<BnwFreshSpyNotification> spy_notifications;
    bool operator==(const BnwFreshEspionageServiceState &) const = default;
};
struct BnwFreshEspionageUninitEvent {
    std::string_view sdk_member;
    std::int32_t major_index = -1; // -1 for non-loop writes.
    bool operator==(const BnwFreshEspionageUninitEvent &) const = default;
};
struct BnwFreshEspionageUninitDraft {
    BnwFreshEspionageServiceState state;
    std::vector<BnwFreshEspionageUninitEvent> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// The installed BNW SDK CvPlayerEspionage constructor calls Reset. Its loop
// count must come from exactly one selected MAX_MAJOR_CIVS native constant.
std::optional<BnwFreshEspionageServiceState>
make_bnw_fresh_espionage_constructor_state(
    std::span<const content::GameplayDefine> native_constants);

// CvPlayer.cpp:676 calls after ReligionAI. Expansion2
// CvEspionageClasses.cpp:152-170 is entirely Reset: seven leading writes,
// then two writes per major civ. Spy notifications are retained. Aggregate
// archive traversal and EspionageAI/remaining services stay blocked.
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
    BnwFreshEspionageServiceState initial);

} // namespace civ5::runtime
