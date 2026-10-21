#pragma once
#include "civ5/saves/game_state.hpp"

namespace civ5::saves {
struct SavePlotPosition {
    std::int64_t x = 0, y = 0;
};
struct ReplayMessageObservation {
    std::int64_t turn = 0, type = 0, player = 0;
    std::vector<SavePlotPosition> plots;
    std::vector<std::uint8_t> text;
};
struct PlotYieldObservation {
    SavePlotPosition plot;
    std::vector<std::int64_t> yields;
};
struct PlotCostObservation {
    SavePlotPosition plot;
    std::int64_t cost = 0;
};
struct TradedItemObservation {
    // Absent in VP and the legacy item layout inside BNW deal version 1.
    std::optional<std::uint64_t> version;
    std::int64_t type = 0, duration = 0, final_turn = 0, from_player = 0;
    std::array<std::int64_t, 3> data{};
    bool flag1 = false;
    std::optional<bool> from_renewed, to_renewed;
};
struct DealObservation {
    std::optional<std::uint64_t> version;
    std::optional<std::int64_t> id;
    std::int64_t from_player = 0, to_player = 0, final_turn = 0, duration = 0, start_turn = 0;
    std::optional<bool> is_gift, considering_renewal, checked_renewal, cancelled;
    std::int64_t peace_treaty = 0, surrendering_player = 0, demanding_player = 0, requesting_player = 0;
    std::vector<TradedItemObservation> items;
};
struct DealsObservation {
    std::optional<std::uint64_t> version;
    std::array<std::vector<DealObservation>, 3> groups;
    static constexpr std::array<const char *, 3> group_names{"proposed", "current", "historical"};
    // Derived from current/historical VP IDs in host width; not a wire field.
    // This does not allocate an ID or mutate an engine session.
    std::optional<std::uint64_t> next_deal_id;
};
struct GameHistoryObservation {
    GameStateLayout layout;
    std::uint64_t start_offset = 0, replay_end_offset = 0, deals_offset = 0, end_offset = 0;
    std::optional<std::uint64_t> replay_version;
    std::vector<ReplayMessageObservation> replay;
    std::int64_t sessions = 0, earliest_barbarian_release_turn = 0;
    std::vector<PlotYieldObservation> extra_yields;
    std::vector<PlotCostObservation> extra_costs;
    bool archaeology_triggered = false;
    DealsObservation deals;
};
// Starts after the second RNG, stops before CvGameReligions. Preserves opaque
// text, serialized IDs/order and expired/cancelled trades. Does not execute
// diplomacy, deduplicate messages, increment sessions or add a reload message.
GameHistoryObservation observe_game_history(runtime::BinaryReader &input, GameStateLayout layout,
                                            GameStateBudget budget = {});
} // namespace civ5::saves
