#include "civ5/saves/game_history.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::saves {
namespace {
class HistoryReader {
  public:
    runtime::BinaryReader &input;
    GameStateBudget remaining;
    bool vp;
    void claim(std::uint64_t count, std::uint64_t minimum_bytes) {
        if (count > remaining.elements || count > input.remaining() / minimum_bytes)
            throw std::runtime_error("Game history collection exceeds input or element budget");
        remaining.elements -= count;
    }
    std::uint64_t count(bool signed_count = false) {
        if (!signed_count)
            return input.u32();
        const auto value = input.i32();
        if (value < 0)
            throw std::runtime_error("Negative game history collection size");
        return static_cast<std::uint64_t>(value);
    }
    bool flag() {
        const auto value = input.u8();
        if (value > 1)
            throw std::runtime_error("Unverified game history boolean");
        return value != 0;
    }
    std::vector<std::uint8_t> bytes() {
        const auto size = count();
        if (size > remaining.string_bytes || size > remaining.total_string_bytes || size > input.remaining())
            throw std::runtime_error("Game history string exceeds input or string budget");
        remaining.total_string_bytes -= size;
        std::vector<std::uint8_t> value(static_cast<std::size_t>(size));
        input.read(value);
        return value;
    }
    ReplayMessageObservation message() {
        ReplayMessageObservation result;
        result.turn = input.i32();
        result.type = input.i32();
        const auto plots = count(!vp);
        claim(plots, 4);
        result.plots.reserve(static_cast<std::size_t>(plots));
        for (std::uint64_t i = 0; i < plots; ++i) {
            SavePlotPosition plot;
            plot.x = input.i16();
            plot.y = input.i16();
            result.plots.push_back(plot);
        }
        result.player = input.i32();
        result.text = bytes();
        return result;
    }
    TradedItemObservation item(bool legacy) {
        TradedItemObservation result;
        if (!vp && !legacy) {
            result.version = input.u32();
            if (*result.version < 1 || *result.version > 2)
                throw std::runtime_error("Unverified BNW traded item version");
        }
        result.type = input.i32();
        result.duration = input.i32();
        result.final_turn = input.i32();
        result.data[0] = input.i32();
        result.data[1] = input.i32();
        if (vp || (!legacy && *result.version == 2)) {
            result.data[2] = input.i32();
            result.flag1 = flag();
        }
        result.from_player = input.i32();
        if (!vp) {
            result.from_renewed = legacy ? false : flag();
            result.to_renewed = legacy ? false : flag();
        }
        return result;
    }
    DealObservation deal() {
        DealObservation result;
        if (vp)
            result.id = input.i32();
        else {
            result.version = input.u32();
            if (*result.version < 1 || *result.version > 3)
                throw std::runtime_error("Unverified BNW deal version");
        }
        result.from_player = input.i32();
        result.to_player = input.i32();
        result.final_turn = input.i32();
        result.duration = input.i32();
        result.start_turn = input.i32();
        if (vp)
            result.is_gift = flag();
        else {
            result.considering_renewal = flag();
            result.checked_renewal = *result.version >= 3 ? flag() : false;
            result.cancelled = flag();
        }
        result.peace_treaty = input.i32();
        result.surrendering_player = input.i32();
        result.demanding_player = input.i32();
        result.requesting_player = input.i32();
        const auto items = count(!vp);
        const bool legacy = !vp && *result.version == 1;
        claim(items, vp ? 29 : legacy ? 24 : 30);
        result.items.reserve(static_cast<std::size_t>(items));
        for (std::uint64_t i = 0; i < items; ++i)
            result.items.push_back(item(legacy));
        return result;
    }
};
} // namespace
GameHistoryObservation observe_game_history(runtime::BinaryReader &input, GameStateLayout layout,
                                            GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (!vp && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unknown game history layout");
    GameHistoryObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    HistoryReader reader{input, budget, vp};
    if (!vp) {
        result.replay_version = input.u32();
        if (*result.replay_version != 2)
            throw std::runtime_error("Unverified BNW replay message version");
    }
    const auto messages = reader.count(!vp);
    reader.claim(messages, 20);
    result.replay.reserve(static_cast<std::size_t>(messages));
    for (std::uint64_t i = 0; i < messages; ++i)
        result.replay.push_back(reader.message());
    result.replay_end_offset = input.position();
    result.sessions = input.i32();
    const auto yields = reader.count();
    reader.claim(yields, 12);
    for (std::uint64_t i = 0; i < yields; ++i) {
        PlotYieldObservation entry;
        entry.plot.x = input.i32();
        entry.plot.y = input.i32();
        const auto values = reader.count();
        reader.claim(values, 4);
        for (std::uint64_t j = 0; j < values; ++j)
            entry.yields.push_back(input.i32());
        result.extra_yields.push_back(std::move(entry));
    }
    const auto costs = reader.count();
    reader.claim(costs, 12);
    for (std::uint64_t i = 0; i < costs; ++i) {
        PlotCostObservation entry;
        entry.plot.x = input.i32();
        entry.plot.y = input.i32();
        entry.cost = input.i32();
        result.extra_costs.push_back(entry);
    }
    result.archaeology_triggered = reader.flag();
    result.earliest_barbarian_release_turn = input.i32();
    result.deals_offset = input.position();
    if (!vp) {
        result.deals.version = input.u32();
        if (*result.deals.version != 1)
            throw std::runtime_error("Unverified BNW game deals version");
    } else
        result.deals.next_deal_id = 0;
    for (std::size_t group = 0; group < result.deals.groups.size(); ++group) {
        const auto count = reader.count(!vp);
        reader.claim(count, vp ? 45 : 46);
        auto &deals = result.deals.groups[group];
        deals.reserve(static_cast<std::size_t>(count));
        for (std::uint64_t i = 0; i < count; ++i) {
            auto deal = reader.deal();
            if (vp && group && *deal.id >= 0)
                result.deals.next_deal_id =
                    std::max(*result.deals.next_deal_id, static_cast<std::uint64_t>(*deal.id) + 1);
            deals.push_back(std::move(deal));
        }
    }
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
