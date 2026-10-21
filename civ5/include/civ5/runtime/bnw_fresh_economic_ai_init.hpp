#pragma once

#include "civ5/runtime/bnw_fresh_player_info_reset.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshEconomicInitStep : std::uint8_t {
    selected_strategies_and_player_bound,
    strategy_and_flavor_arrays_allocated,
    yields_and_requests_cleared,
    strategy_and_yield_reset,
    exploration_and_goody_reset,
    purchase_requests_reset,
    counters_and_recon_reset,
    cached_defines_loaded,
    military_ai_pending,
};
struct BnwFreshEconomicInitEvent {
    BnwFreshEconomicInitStep step;
    std::int32_t player_id;
    bool operator==(const BnwFreshEconomicInitEvent &) const = default;
};
enum class BnwFreshEconomicInitBlockerCode {
    player_info_or_selected_rules_unverified,
    economic_strategy_or_define_unverified,
    allocation_budget_exceeded,
    military_and_remaining_ai_pending,
};
struct BnwFreshEconomicInitBlocker {
    BnwFreshEconomicInitBlockerCode code;
    std::string message;
};
struct BnwFreshPurchaseRequest {
    std::int32_t type = -1, amount = 0, priority = 0;
    bool operator==(const BnwFreshPurchaseRequest &) const = default;
};
struct BnwFreshEconomicAIState {
    std::int32_t player_id = -1;
    std::vector<bool> using_strategy;
    std::vector<std::int32_t> turn_strategy_adopted;
    // SDK allocates int[] and does not initialize it in Init/Reset.
    std::vector<std::optional<std::int32_t>> temp_flavors;
    std::array<std::int32_t,6> yields{};
    std::vector<BnwFreshPurchaseRequest> requested_savings;
    bool exploration_plots_dirty = true;
    std::int32_t explorers_disbanded = 0;
    std::int32_t recon_state = -1, naval_recon_state = -1;
    std::int32_t last_turn_worker_disbanded = -1;
    std::int32_t visible_antiquity_sites = 0;
    std::int32_t minimum_settle_fertility = 0;
    std::int32_t early_city_number_target = 0;
};
struct BnwFreshEconomicAIInit {
    std::vector<BnwFreshEconomicAIState> players;
    std::vector<BnwFreshEconomicInitEvent> source_order;
    std::string selected_rules_sha256;
    std::vector<BnwFreshEconomicInitBlocker> blockers;
    bool economic_ai_init_complete = false;
    bool initialized_by_gamecore = false;
    bool has_blocker(BnwFreshEconomicInitBlockerCode code) const;
    void rollback() noexcept;
};

// Fresh CvEconomicAI::Init/Reset for each selected Player, before
// CvMilitaryAI::Init. The temporary flavor array retains uninitialized cells
// as typed absence; no later AI service may read them without writing first.
BnwFreshEconomicAIInit draft_bnw_fresh_economic_ai_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPlayerInfoReset &player_info,
    std::string_view selected_rules_sha256,
    std::uint64_t maximum_total_cells = 16ULL * 1024 * 1024);

} // namespace civ5::runtime
