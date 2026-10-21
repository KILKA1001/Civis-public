#pragma once

#include "civ5/runtime/bnw_fresh_city_specialization_uninit.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace civ5::runtime {

struct BnwFreshWonderServiceState {
    // Inherited CvFlavorRecipient array is the sole owner released by Uninit.
    std::optional<std::vector<std::int32_t>> latest_flavors;
    // CvWonderProductionAI owns both weight vectors, but inherited Uninit
    // does not touch them. Constructor binds both borrowed pointers.
    std::vector<std::pair<std::int32_t,std::int32_t>> wonder_weights;
    std::vector<std::pair<std::int32_t,std::int32_t>> buildables;
    bool player_bound = true, buildings_info_bound = true;
    bool operator==(const BnwFreshWonderServiceState &) const = default;
};
struct BnwFreshWonderUninitDraft {
    BnwFreshWonderServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// CvPlayer.cpp:411 constructs the Wonder AI with player and selected building
// Info. Its CvFlavorRecipient base starts with a null flavor array; the two
// weighted vectors start empty.
BnwFreshWonderServiceState make_bnw_fresh_wonder_constructor_state();

// CvWonderProductionAI declares no Uninit override: CvPlayer::uninit calls
// inherited CvFlavorRecipient::Uninit, which only SAFE_DELETE_ARRAYs the
// latest-flavor pointer. The CitySpecialization predecessor is checked; the
// aggregate archive and GrandStrategyAI/remaining services stay blocked.
BnwFreshWonderUninitDraft project_bnw_fresh_wonder_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    std::string_view selected_rules_sha256,
    BnwFreshWonderServiceState initial);

} // namespace civ5::runtime
