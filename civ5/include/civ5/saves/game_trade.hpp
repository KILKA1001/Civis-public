#pragma once
#include "civ5/saves/game_state.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::saves {
// Fixed array widths in the selected source GameCore. These are not the
// number of living players, SQL row counts, or capacities of a Civis game.
struct TradeWireDimensions {
    std::uint64_t major_slots = 0, yield_slots = 0;
};
struct TradePlotObservation {
    std::int64_t x = 0, y = 0;
};
struct TradeRouteObservation {
    // BNW version 0 has no ID and its reader supplies MAX_INT. Preserve the
    // absence instead of importing an x86 sentinel as a generated identity.
    std::optional<std::int64_t> id;
    std::int64_t origin_x = 0, origin_y = 0, destination_x = 0, destination_y = 0;
    std::int64_t origin_owner = 0, destination_owner = 0, domain = 0, connection_type = 0;
    std::int64_t unit_location_index = 0, unit_id = 0;
    bool moving_forward = false;
    std::int64_t circuits_completed = 0, circuits_to_complete = 0, turn_route_complete = 0;
    // Whole path, shared by logic and future rendering; no visual point cap.
    std::vector<TradePlotObservation> plots;
    std::vector<std::int64_t> origin_yields, destination_yields;
    runtime::ProfileExtensions profile_extensions;
};
struct GameTradeObservation {
    GameStateLayout layout;
    TradeWireDimensions dimensions;
    std::optional<std::uint64_t> version;
    std::uint64_t start_offset = 0, routes_end_offset = 0, matrices_offset = 0, end_offset = 0;
    std::vector<TradeRouteObservation> routes;
    // Flat, row-major major_slots x major_slots arrays. Missing technology
    // matrix in BNW versions 0..2 implies the original zero initialization;
    // no synthetic matrix is allocated during observation.
    std::optional<std::vector<std::int64_t>> technology_difference;
    std::int64_t next_id = 0;
    runtime::ProfileExtensions profile_extensions;
};
// Starts after CvGameLeagues. Stops before the embedded database in BNW or
// CvGameCorporations in VP. Does not move units, add yields, rebuild paths,
// validate city identities or issue calls to the original graphics engine.
GameTradeObservation observe_game_trade(runtime::BinaryReader &input, GameStateLayout layout,
                                        TradeWireDimensions dimensions, GameStateBudget budget = {});
} // namespace civ5::saves
