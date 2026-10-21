#include "civ5/saves/game_state.hpp"
#include "civ5/saves/vp_roster_profile.hpp"
#include <bit>
#include <stdexcept>

namespace civ5::saves {
namespace {
class StateReader {
  public:
    runtime::BinaryReader &input;
    GameStateBudget remaining;

    void claim(std::uint64_t count, std::uint64_t minimum_wire_bytes) {
        if (count > remaining.elements || count > input.remaining() / minimum_wire_bytes)
            throw std::runtime_error("Game state collection exceeds input or element budget");
        remaining.elements -= count;
    }
    bool flag() {
        const auto byte = input.u8();
        if (byte > 1)
            throw std::runtime_error("Unverified game state boolean at " +
                                     std::to_string(input.position() - 1));
        return byte != 0;
    }
    std::vector<std::uint8_t> bytes(const char *field) {
        const auto offset = input.position();
        const std::uint64_t count = input.u32();
        if (count > remaining.string_bytes || count > remaining.total_string_bytes ||
            count > input.remaining())
            throw std::runtime_error(std::string("Game state ") + field + " length " +
                                     std::to_string(count) + " at byte " +
                                     std::to_string(offset) + " exceeds remaining input " +
                                     std::to_string(input.remaining()) + " or string budget " +
                                     std::to_string(remaining.string_bytes) + "/" +
                                     std::to_string(remaining.total_string_bytes));
        remaining.total_string_bytes -= count;
        std::vector<std::uint8_t> result(static_cast<std::size_t>(count));
        input.read(result);
        return result;
    }
    std::vector<std::int64_t> integers(std::uint64_t count) {
        claim(count, 4);
        std::vector<std::int64_t> result(static_cast<std::size_t>(count));
        for (auto &value : result)
            value = input.i32();
        return result;
    }
};
} // namespace

GameStateObservation observe_game_state(runtime::BinaryReader &input, GameStateLayout layout,
                                        std::optional<RosterWireDimensions> dimensions,
                                        GameStateBudget budget) {
    GameStateObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (vp)
        result.prefix = observe_vp_prefix(input);
    else if (layout == GameStateLayout::expansion2_version1)
        result.prefix = observe_expansion2_prefix(input);
    else
        throw std::runtime_error("Unknown game state layout");
    StateReader reader{input, budget};
    const auto flag = [&](const char *name) { result.flags.push_back({name, reader.flag()}); };
    const auto integer = [&](const char *name) { result.controls.push_back({name, input.i32()}); };
    for (const auto *name : {"score_dirty", "circumnavigated", "final_initialized", "hot_pbem_between_turns",
                             "nukes_valid", "end_game_tech_researched"})
        flag(name);
    if (vp)
        integer("observer_ui_override_player");
    flag("tuner_ever_connected");
    if (vp)
        flag("saved_once");
    for (const auto *name : {"tutorial_ever_attacked", "static_tutorial_active", "ever_right_click_moved"})
        flag(name);
    const std::uint64_t advisors = input.u32();
    reader.claim(advisors, 4);
    for (std::uint64_t i = 0; i < advisors; ++i) {
        if (vp)
            result.advisor_hashes.push_back(input.u32());
        else
            result.advisor_messages.push_back(reader.bytes("advisor_message"));
    }
    for (const auto *name :
         {"handicap", "pause_player", "ai_auto_play_return_player", "best_land_unit", "winner", "victory",
          "game_state", "best_wonders_player", "best_policies_player", "best_great_people_player"})
        integer(name);
    if (!vp)
        integer("religion_tech");
    integer("industrial_route");
    if (vp) {
        integer("game_era");
        // This particular wire field is a signed char, not a four-byte enum.
        // Host IDs are widened; 127 is not a Civis team capacity.
        result.controls.push_back({"team_that_circumnavigated", std::bit_cast<std::int8_t>(input.u8())});
        flag("victory_randomization");
        for (const auto *name :
             {"median_techs_researched", "basic_needs_median", "gold_median", "science_median",
              "culture_median", "spy_threshold", "last_turn_cs_surrendered"})
            integer(name);
    }
    result.controls_end_offset = result.end_offset = input.position();
    if (!dimensions)
        return result;
    const auto *vp_dimensions = vp_roster_dimensions(*dimensions);
    if (!dimensions->player_slots || !dimensions->team_slots ||
        vp != (vp_dimensions != nullptr))
        throw std::runtime_error("Roster wire dimensions do not match the selected game state layout");
    RosterObservation roster;
    roster.dimensions = *dimensions;
    roster.start_offset = input.position();
    if (vp)
        roster.resource_monopoly_players = reader.integers(vp_dimensions->resource_slots);
    try {
        roster.script_data = reader.bytes("script_data");
    } catch (const std::runtime_error &e) {
        throw std::runtime_error(std::string(e.what()) + "; selected Resources slots=" +
                                 std::to_string(vp_dimensions ? vp_dimensions->resource_slots : 0));
    }
    roster.rankings_offset = input.position();
    for (std::size_t i = 0; i < roster.arrays.size(); ++i)
        roster.arrays[i] = reader.integers(i < 4 ? dimensions->player_slots : dimensions->team_slots);
    roster.end_offset = result.end_offset = input.position();
    result.roster = std::move(roster);
    return result;
}
} // namespace civ5::saves
