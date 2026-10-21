#pragma once

#include "civ5/runtime/bnw_fresh_military_uninit.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

struct BnwFreshCitySpecializationServiceState {
    bool specializations_dirty = false, interrupt_wonders = false;
    std::int32_t next_specialization = -1, next_wonder = -1;
    std::int32_t wonder_city_id = -1, next_wonder_weight = 0;
    std::int32_t last_turn_evaluated = 0;
    std::vector<std::int32_t> yield_weights, production_subtype_weights;
    bool operator==(const BnwFreshCitySpecializationServiceState &) const = default;
};
struct BnwFreshCitySpecializationUninitDraft {
    BnwFreshCitySpecializationServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// The Expansion2 SDK constructor sets the represented scalars and leaves the
// two weight vectors empty. Other uninitialized members are not invented.
BnwFreshCitySpecializationServiceState
make_bnw_fresh_city_specialization_constructor_state();

// CvCitySpecializationAI.cpp:243-245 has an empty Uninit body. This is a
// source-exact no-op for both fresh and populated state. Its call position
// after MilitaryAI is checked, but the earlier aggregate archive and later
// WonderProductionAI/other service effects remain blocked.
BnwFreshCitySpecializationUninitDraft
project_bnw_fresh_city_specialization_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    std::string_view selected_rules_sha256,
    BnwFreshCitySpecializationServiceState initial);

} // namespace civ5::runtime
