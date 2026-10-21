#include "civ5/saves/game_trade.hpp"
#include "civ5/saves/vp_game_trade_profile.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::saves {
namespace {
class TradeReader {
  public:
    runtime::BinaryReader &input;
    GameStateBudget remaining;
    TradeWireDimensions dimensions;
    bool vp;
    void claim(std::uint64_t size, std::uint64_t minimum_bytes) {
        if (size > remaining.elements || size > input.remaining() / minimum_bytes)
            throw std::runtime_error("Trade collection exceeds input or element budget");
        remaining.elements -= size;
    }
    std::uint64_t count() {
        if (vp)
            return input.u32();
        const auto value = input.i32();
        if (value < 0)
            throw std::runtime_error("Negative trade collection size");
        return static_cast<std::uint64_t>(value);
    }
    bool flag() {
        const auto value = input.u8();
        if (value > 1)
            throw std::runtime_error("Unverified trade boolean");
        return value != 0;
    }
    std::vector<std::int64_t> integers(std::uint64_t size) {
        claim(size, 4);
        std::vector<std::int64_t> result(static_cast<std::size_t>(size));
        for (auto &value : result)
            value = input.i32();
        return result;
    }
    std::vector<std::int64_t> matrix() {
        if (dimensions.major_slots > std::numeric_limits<std::uint64_t>::max() / dimensions.major_slots)
            throw std::runtime_error("Trade matrix dimension product overflow");
        return integers(dimensions.major_slots * dimensions.major_slots);
    }
    TradeRouteObservation route(std::uint64_t version) {
        TradeRouteObservation result;
        auto *vp_route = vp ? &ensure_vp_trade_route(result) : nullptr;
        if (vp || version >= 1)
            result.id = input.i32();
        if (vp)
            vp_route->origin_id = input.i32();
        result.origin_x = input.i32();
        result.origin_y = input.i32();
        if (vp)
            vp_route->destination_id = input.i32();
        result.destination_x = input.i32();
        result.destination_y = input.i32();
        result.origin_owner = input.i32();
        result.destination_owner = input.i32();
        result.domain = input.i32();
        result.connection_type = input.i32();
        result.unit_location_index = input.i32();
        result.moving_forward = flag();
        result.unit_id = input.i32();
        if (vp)
            vp_route->speed_factor = input.i32();
        result.circuits_completed = input.i32();
        result.circuits_to_complete = input.i32();
        if (vp || version >= 2)
            result.turn_route_complete = input.i32();
        if (vp)
            vp_route->recalled = flag();
        const auto plots = count();
        claim(plots, 8);
        result.plots.reserve(static_cast<std::size_t>(plots));
        for (std::uint64_t i = 0; i < plots; ++i) {
            TradePlotObservation plot;
            plot.x = input.i32();
            plot.y = input.i32();
            result.plots.push_back(plot);
        }
        if (vp) {
            result.origin_yields = integers(dimensions.yield_slots);
            result.destination_yields = integers(dimensions.yield_slots);
        } else {
            // BNW stores origin/destination pairs, VP stores whole arrays.
            claim(dimensions.yield_slots, 8);
            if (dimensions.yield_slots > remaining.elements)
                throw std::runtime_error("Trade yields exceed element budget");
            remaining.elements -= dimensions.yield_slots;
            result.origin_yields.resize(static_cast<std::size_t>(dimensions.yield_slots));
            result.destination_yields.resize(static_cast<std::size_t>(dimensions.yield_slots));
            for (std::uint64_t i = 0; i < dimensions.yield_slots; ++i) {
                result.origin_yields[static_cast<std::size_t>(i)] = input.i32();
                result.destination_yields[static_cast<std::size_t>(i)] = input.i32();
            }
        }
        return result;
    }
};
} // namespace
GameTradeObservation observe_game_trade(runtime::BinaryReader &input, GameStateLayout layout,
                                        TradeWireDimensions dimensions, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (!vp && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unknown game trade layout");
    if (!dimensions.major_slots || !dimensions.yield_slots)
        throw std::runtime_error("Missing source GameCore trade dimensions");
    GameTradeObservation result{};
    result.layout = layout;
    result.dimensions = dimensions;
    result.start_offset = input.position();
    if (!vp) {
        result.version = input.u32();
        if (*result.version > 3)
            throw std::runtime_error("Unverified game trade version");
    }
    TradeReader reader{input, budget, dimensions, vp};
    const auto routes = reader.count();
    reader.claim(routes, vp ? 74 : 53);
    result.routes.reserve(static_cast<std::size_t>(routes));
    for (std::uint64_t i = 0; i < routes; ++i)
        result.routes.push_back(reader.route(result.version.value_or(0)));
    result.routes_end_offset = input.position();
    if (vp) {
        const auto players = reader.count();
        reader.claim(players, 4);
        auto &profile = ensure_vp_game_trade(result);
        profile.routes_per_player.reserve(static_cast<std::size_t>(players));
        for (std::uint64_t i = 0; i < players; ++i)
            profile.routes_per_player.push_back(reader.integers(reader.count()));
    }
    result.matrices_offset = input.position();
    if (vp || *result.version >= 3)
        result.technology_difference = reader.matrix();
    if (vp)
        result.profile_extensions.find<VpGameTradeState>()->policy_difference = reader.matrix();
    result.next_id = input.i32();
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
