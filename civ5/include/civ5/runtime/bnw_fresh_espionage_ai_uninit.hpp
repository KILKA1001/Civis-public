#pragma once

#include "civ5/runtime/bnw_fresh_espionage_uninit.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshEspionageAiServiceState {
    // CvEspionageAI::Uninit never clears this borrowed player pointer.
    bool player_bound = false;
    std::vector<std::int32_t> civ_out_of_tech_turn;
    std::vector<std::int32_t> spies_caught, spies_killed, spies_died;
    std::vector<std::int32_t> last_spy_caught, last_spy_killed, last_spy_died;
    bool un_countdown_started = false;
    std::int32_t turn_espionage_started = -1;
    bool operator==(const BnwFreshEspionageAiServiceState &) const = default;
};
enum class BnwFreshEspionageAiUninitOperation : std::uint8_t {
    clear,
    push_back,
    scalar_write,
};
struct BnwFreshEspionageAiUninitEvent {
    std::string_view sdk_member;
    BnwFreshEspionageAiUninitOperation operation;
    std::int32_t major_index = -1;
    bool operator==(const BnwFreshEspionageAiUninitEvent &) const = default;
};
struct BnwFreshEspionageAiUninitDraft {
    BnwFreshEspionageAiServiceState state;
    std::vector<BnwFreshEspionageAiUninitEvent> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Constructor sets player=NULL, countdown=false, start turn=-1; seven
// FStaticVectors remain empty. No Reset is called by the constructor.
BnwFreshEspionageAiServiceState make_bnw_fresh_espionage_ai_constructor_state();

// CvPlayer.cpp:677 calls after CvPlayerEspionage. Expansion2
// CvEspionageClasses.cpp:4081-4143: Uninit invokes Reset, which clears and
// refills seven vectors using selected MAX_MAJOR_CIVS, then sets two scalars.
// It retains m_pPlayer. Archive traversal and Trade/remaining services stay
// blocked.
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
    BnwFreshEspionageAiServiceState initial);

} // namespace civ5::runtime
