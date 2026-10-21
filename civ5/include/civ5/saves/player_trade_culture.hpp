#pragma once
#include "civ5/saves/map_state.hpp"

namespace civ5::saves {
struct PlayerTradeCultureWireDimensions {
    std::uint64_t major = 22, yields = 6;
};
struct PlayerTradeCultureBlob {
    const char *name;
    std::vector<std::uint8_t> bytes;
};
struct PlayerTradeCultureRecord {
    const char *name;
    std::uint64_t index = 0, start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    std::vector<PlotArrayObservation> arrays;
    std::vector<PlayerTradeCultureBlob> blobs;
    std::vector<PlayerTradeCultureRecord> children;
};
struct PlayerTradeCultureObservation {
    GameStateLayout layout;
    PlayerTradeCultureWireDimensions dimensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<PlayerTradeCultureRecord> blocks;
};
// Full PlayerTrade, TradeAI, LeagueAI and PlayerCulture. Next is the player's
// has-notifications flag. Preserves expired/plundered routes, vote promises,
// source culture units and raw tooltips. No world pointers, yield application,
// influence recomputation, votes, trades, notifications or synthesized defaults.
// VP fixed source spans must be supplied explicitly; BNW carries its lengths.
PlayerTradeCultureObservation
observe_player_trade_culture(runtime::BinaryReader &input, GameStateLayout layout,
                             std::optional<PlayerTradeCultureWireDimensions> vp_dimensions = {},
                             GameStateBudget budget = {});
} // namespace civ5::saves
