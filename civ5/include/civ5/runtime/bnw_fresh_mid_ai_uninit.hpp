#pragma once

#include "civ5/runtime/bnw_fresh_tactical_ai_uninit.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// SDK HomelandAI, MinorCivAI and DealAI Uninit bodies are empty. These
// detached witnesses are preserved verbatim; they are not native objects.
struct BnwFreshEmptyAiServiceState {
    std::vector<std::byte> member_snapshot;
    bool operator==(const BnwFreshEmptyAiServiceState &) const = default;
};

struct BnwFreshBuilderTaskingUninitState {
    std::int32_t repair_build = -1; // NO_BUILD
    bool player_bound = false;
    bool logging = false;
    std::int32_t num_cities = -1;
    bool target_plot_bound = false;
    // Other member/static values are untouched by Uninit.
    std::vector<std::byte> untouched_member_snapshot;
    bool operator==(const BnwFreshBuilderTaskingUninitState &) const = default;
};

struct BnwFreshCityConnectionsUninitState {
    bool player_bound = false;
    std::optional<std::vector<std::byte>> route_infos;
    std::optional<std::uint32_t> route_infos_dimension;
    std::vector<std::int32_t> buildings_allow_water_routes;
    std::vector<std::byte> plot_route_infos;
    bool operator==(const BnwFreshCityConnectionsUninitState &) const = default;
};

struct BnwFreshMidAiUninitInput {
    BnwFreshEmptyAiServiceState homeland_ai, minor_civ_ai, deal_ai;
    BnwFreshBuilderTaskingUninitState builder_tasking_ai;
    BnwFreshCityConnectionsUninitState city_connections;
    bool operator==(const BnwFreshMidAiUninitInput &) const = default;
};

struct BnwFreshMidAiUninitDraft {
    BnwFreshMidAiUninitInput state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

BnwFreshMidAiUninitInput make_bnw_fresh_mid_ai_constructor_state();

// Expansion2 CvPlayer.cpp:684-688. Source order: three empty Uninit calls,
// BuilderTaskingAI five assignments, CityConnections player detach/array
// delete. The conditional Notifications call is the next blocker.
BnwFreshMidAiUninitDraft project_bnw_fresh_mid_ai_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshTacticalAiUninitDraft &tactical_ai,
    std::string_view selected_rules_sha256,
    BnwFreshMidAiUninitInput initial);

} // namespace civ5::runtime
