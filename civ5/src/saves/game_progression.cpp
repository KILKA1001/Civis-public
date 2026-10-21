#include "civ5/saves/game_progression.hpp"
#include "civ5/saves/vp_game_progression_profile.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::saves {
namespace {
class ProgressionReader {
  public:
    runtime::BinaryReader &input;
    GameStateBudget remaining;
    void claim(std::uint64_t count, std::uint64_t minimum_bytes) {
        if (count > remaining.elements || (minimum_bytes && count > input.remaining() / minimum_bytes))
            throw std::runtime_error("Game progression exceeds input or element budget");
        remaining.elements -= count;
    }
    std::uint64_t count32() {
        const auto count = input.i32();
        if (count < 0)
            throw std::runtime_error("Negative game progression collection size");
        return static_cast<std::uint64_t>(count);
    }
    bool flag() {
        const auto value = input.u8();
        if (value > 1)
            throw std::runtime_error("Unverified game progression boolean");
        return value != 0;
    }
    void version1() {
        if (input.u32() != 1)
            throw std::runtime_error("Unverified game progression structure version");
    }
    std::vector<std::uint8_t> bytes() {
        const std::uint64_t size = input.u32();
        if (size > remaining.string_bytes || size > remaining.total_string_bytes || size > input.remaining())
            throw std::runtime_error("Game progression string exceeds input or string budget");
        remaining.total_string_bytes -= size;
        std::vector<std::uint8_t> value(static_cast<std::size_t>(size));
        input.read(value);
        return value;
    }
    ProgressionArray array(const char *name, bool hashed, std::uint64_t rows, std::uint64_t columns = 1,
                           bool boolean = false) {
        ProgressionArray result{name, input.position(), 0, columns, {}};
        if (hashed)
            rows = input.u32();
        claim(rows, hashed ? 4 : 0);
        // Check an uncounted matrix before allocating even its row descriptors.
        if (!hashed && columns &&
            (rows > remaining.elements / columns || rows > input.remaining() / (boolean ? 1 : 4) / columns))
            throw std::runtime_error("Game progression matrix exceeds input or element budget");
        result.rows.reserve(static_cast<std::size_t>(rows));
        for (std::uint64_t i = 0; i < rows; ++i) {
            ProgressionRow row;
            if (hashed)
                row.type_hash = input.u32();
            if (!hashed || *row.type_hash != 0) {
                claim(columns, boolean ? 1 : 4);
                row.values.reserve(static_cast<std::size_t>(columns));
                for (std::uint64_t j = 0; j < columns; ++j)
                    row.values.push_back(boolean ? flag() : input.i32());
            }
            result.rows.push_back(std::move(row));
        }
        result.end_offset = input.position();
        return result;
    }
    NameHistory history(bool hashed) {
        NameHistory result;
        const std::uint64_t count = input.u32();
        claim(count, 4);
        for (std::uint64_t i = 0; i < count; ++i) {
            if (hashed)
                result.hashes.push_back(input.u32());
            else
                result.strings.push_back(bytes());
        }
        return result;
    }
    VoteFreeListObservation votes(bool triggered) {
        VoteFreeListObservation result;
        const auto slots = count32();
        result.last_index = input.i32();
        result.free_head = input.i32();
        result.free_count = input.i32();
        result.current_id = input.i32();
        claim(slots, 4);
        for (std::uint64_t i = 0; i < slots; ++i)
            result.next_free.push_back(input.i32());
        const auto count = count32();
        claim(count, triggered ? 32 : 16);
        for (std::uint64_t i = 0; i < count; ++i) {
            version1();
            VoteEntryObservation entry;
            entry.id = input.i32();
            entry.source = input.i32();
            const std::uint64_t options = triggered ? 1 : input.u32();
            claim(options, 20);
            for (std::uint64_t j = 0; j < options; ++j) {
                VoteOptionObservation option;
                option.vote = input.i32();
                option.player = input.i32();
                option.city = input.i32();
                option.other_player = input.i32();
                option.text = bytes();
                entry.options.push_back(std::move(option));
            }
            result.entries.push_back(std::move(entry));
        }
        return result;
    }
    RandomStateObservation random(bool vp) {
        if (!vp)
            version1();
        RandomStateObservation result;
        result.seed = input.u32();
        if (vp)
            result.seed |= std::uint64_t(input.u32()) << 32;
        result.calls = input.u32();
        result.resets = input.u32();
        // Retail writes false. A debug build can append variable histories;
        // stopping here prevents mistaking those bytes for the next generator.
        if (!vp && flag())
            throw std::runtime_error("Debug RNG histories are not supported by the retail layout");
        return result;
    }
};
} // namespace
GameProgressionObservation observe_game_progression(runtime::BinaryReader &input, GameStateLayout layout,
                                                    ProgressionWireDimensions dimensions,
                                                    GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *vp_dimensions = vp_game_progression_dimensions(dimensions);
    if ((!vp && layout != GameStateLayout::expansion2_version1) || vp != (vp_dimensions != nullptr) ||
        !dimensions.vote_team_slots || !dimensions.victory_award_slots)
        throw std::runtime_error("Progression dimensions do not match the selected layout");
    GameProgressionObservation result{};
    result.layout = layout;
    result.dimensions = dimensions;
    result.start_offset = input.position();
    ProgressionReader reader{input, budget};
    const auto add = [&](const char *name, bool hashed, std::uint64_t rows, std::uint64_t columns = 1,
                         bool boolean = false) {
        result.arrays.push_back(reader.array(name, hashed, rows, columns, boolean));
    };
    constexpr std::array names{"unit_created_count", "unitclass_created_count", "buildingclass_created_count",
                               "project_created_count"};
    for (std::size_t i = 0; i < names.size(); ++i)
        add(names[i], !vp, vp ? vp_dimensions->rows[i] : 0);
    if (!vp)
        for (const auto *name : {"vote_outcome", "secretary_general_timer", "vote_timer", "diplo_vote"})
            add(name, true, 0);
    for (const auto *name : {"votes_cast", "previous_votes_cast", "votes_for_team"})
        add(name, false, dimensions.vote_team_slots);
    add("special_unit_valid", !vp, vp ? vp_dimensions->rows[4] : 0, 1, true);
    add("team_victory_rank", !vp, vp ? vp_dimensions->rows[5] : 0, dimensions.victory_award_slots);
    if (vp)
        add("contract_units", false, vp_dimensions->rows[6], vp_dimensions->rows[0]);
    result.histories_offset = input.position();
    result.destroyed_cities = reader.history(vp);
    result.great_people = reader.history(vp);
    if (!vp) {
        result.vote_free_lists.emplace();
        (*result.vote_free_lists)[0] = reader.votes(false);
        (*result.vote_free_lists)[1] = reader.votes(true);
    }
    result.random_offset = input.position();
    for (auto &random : result.random)
        random = reader.random(vp);
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
