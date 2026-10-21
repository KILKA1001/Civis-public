#include "civ5/saves/supporting_classes.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::saves {
SupportingClassesObservation observe_supporting_classes(runtime::BinaryReader &input, GameStateLayout layout,
                                                        std::uint64_t plot_count, std::uint64_t major_players,
                                                        GameStateBudget budget) {
    if (layout != GameStateLayout::vp542_corpus && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unknown supporting-classes layout");
    const bool bnw = layout == GameStateLayout::expansion2_version1;
    if (plot_count > budget.elements || major_players > budget.elements)
        throw std::runtime_error("Supporting classes exceed element budget");
    const auto plot_width = bnw ? 4ull : 5ull;
    if (plot_count > (std::numeric_limits<std::uint64_t>::max() - (bnw ? 12ull : 0ull)) / plot_width ||
        major_players > std::numeric_limits<std::uint64_t>::max() / 12)
        throw std::runtime_error("Supporting classes dimensions overflow");
    const auto bytes = (bnw ? 12ull : 0ull) + plot_count * plot_width + major_players * 12ull;
    if (bytes > input.remaining())
        throw std::runtime_error("Supporting classes exceed input");

    SupportingClassesObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    result.plot_count = plot_count;
    result.major_players = major_players;
    const auto version = [&]() -> std::uint64_t {
        const auto value = input.u32();
        if (value != 1)
            throw std::runtime_error("Unsupported supporting-class version at " +
                                     std::to_string(input.position() - 4));
        return value;
    };
    if (bnw) {
        result.game_version = version();
        result.barbarians_version = version();
    }
    result.barbarians.resize(static_cast<std::size_t>(plot_count));
    if (!bnw)
        for (auto &plot : result.barbarians) {
            const auto byte = input.u8();
            if (byte > 1)
                throw std::runtime_error("Invalid VP barbarian attacked flag at " +
                                         std::to_string(input.position() - 1));
            plot.attacked = byte != 0;
        }
    for (auto &plot : result.barbarians)
        plot.counter = input.i16();
    for (auto &plot : result.barbarians)
        plot.units_spawned = input.i16();
    if (bnw)
        result.goody_huts_version = version();
    result.goody_hut_results.resize(static_cast<std::size_t>(major_players));
    for (auto &row : result.goody_hut_results)
        for (auto &value : row)
            value = input.i32();
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
