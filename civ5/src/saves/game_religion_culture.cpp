#include "civ5/saves/game_religion_culture.hpp"
#include <stdexcept>

namespace civ5::saves {
namespace {
class ReligionCultureReader {
  public:
    runtime::BinaryReader &input;
    GameStateBudget remaining;
    bool vp;
    void claim(std::uint64_t count, std::uint64_t minimum_bytes) {
        if (count > remaining.elements || count > input.remaining() / minimum_bytes)
            throw std::runtime_error("Religion/culture collection exceeds input or element budget");
        remaining.elements -= count;
    }
    std::uint64_t count(bool signed_count = false) {
        if (!signed_count)
            return input.u32();
        const auto value = input.i32();
        if (value < 0)
            throw std::runtime_error("Negative religion/culture collection size");
        return static_cast<std::uint64_t>(value);
    }
    std::uint64_t version(std::uint64_t maximum) {
        const auto value = input.u32();
        if (!value || value > maximum)
            throw std::runtime_error("Unverified religion/culture structure version");
        return value;
    }
    bool flag() {
        const auto value = input.u8();
        if (value > 1)
            throw std::runtime_error("Unverified religion/culture boolean");
        return value != 0;
    }
    std::vector<std::uint8_t> bytes(std::uint64_t size) {
        if (size > remaining.string_bytes || size > remaining.total_string_bytes || size > input.remaining())
            throw std::runtime_error("Religion/culture string exceeds input or string budget");
        remaining.total_string_bytes -= size;
        std::vector<std::uint8_t> result(static_cast<std::size_t>(size));
        input.read(result);
        return result;
    }
    std::vector<std::int64_t> integers() {
        const auto size = count();
        claim(size, 4);
        std::vector<std::int64_t> result(static_cast<std::size_t>(size));
        for (auto &value : result)
            value = input.i32();
        return result;
    }
    ReligionBeliefsObservation beliefs() {
        ReligionBeliefsObservation result;
        if (vp) {
            result.religion = input.i32();
            result.belief_ids = integers();
            // Serialized vector<int>, not a bool array or fixed-size bitset.
            result.belief_lookup = integers();
            return result;
        }
        result.version = version(2);
        constexpr std::array names{"faith_from_dying_units",
                                   "river_happiness",
                                   "plot_culture_cost_modifier",
                                   "city_range_strike_modifier",
                                   "combat_modifier_enemy_cities",
                                   "combat_modifier_friendly_cities",
                                   "friendly_heal_change",
                                   "city_state_friendship_modifier",
                                   "land_barbarian_conversion_percent",
                                   "spread_strength_modifier",
                                   "spread_distance_modifier",
                                   "prophet_strength_modifier",
                                   "prophet_cost_modifier",
                                   "missionary_strength_modifier",
                                   "missionary_cost_modifier",
                                   "friendly_city_state_spread_modifier",
                                   "great_person_expended_faith",
                                   "city_state_minimum_influence",
                                   "city_state_influence_modifier",
                                   "other_religion_pressure_erosion",
                                   "spy_pressure",
                                   "inquisitor_pressure_retention",
                                   "faith_building_tourism",
                                   "obsolete_era",
                                   "resource_revealed",
                                   "spread_modifier_doubling_tech"};
        for (std::size_t i = 0; i < names.size(); ++i)
            result.cached_values.push_back({names[i], i == 22 && *result.version == 1 ? 0 : input.i32()});
        const auto hashes = count();
        claim(hashes, 4);
        for (std::uint64_t i = 0; i < hashes; ++i)
            result.belief_hashes.push_back(input.u32());
        const auto buildings = count();
        claim(buildings, 4);
        for (std::uint64_t i = 0; i < buildings; ++i) {
            BeliefBuildingCount entry;
            entry.type_hash = input.u32();
            if (entry.type_hash)
                entry.value = input.i32();
            result.building_class_enabled.push_back(entry);
        }
        return result;
    }
    ReligionObservation religion() {
        ReligionObservation result;
        if (!vp)
            result.version = version(4);
        result.religion = input.i32();
        result.founder = input.i32();
        result.holy_city_x = input.i32();
        result.holy_city_y = input.i32();
        result.turn_founded = input.i32();
        if (vp || *result.version >= 2)
            result.pantheon = flag();
        if (vp || *result.version >= 4)
            result.enhanced = flag();
        if (vp || *result.version >= 3)
            result.custom_name = bytes(128);
        if (vp)
            result.reformed = flag();
        result.beliefs = beliefs();
        return result;
    }
    GreatWorkObservation great_work() {
        GreatWorkObservation result;
        if (!vp) {
            result.version = version(3);
            if (*result.version == 1)
                result.legacy_name = bytes(count());
        }
        result.creator = bytes(count());
        result.type = input.i32();
        if (vp || *result.version >= 3)
            result.class_type = input.i32();
        result.turn_founded = input.i32();
        result.era = input.i32();
        result.player = input.i32();
        return result;
    }
};
} // namespace
GameReligionCultureObservation observe_game_religion_culture(runtime::BinaryReader &input,
                                                             GameStateLayout layout, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (!vp && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unknown religion/culture layout");
    GameReligionCultureObservation result{};
    result.layout = layout;
    result.start_offset = input.position();
    ReligionCultureReader reader{input, budget, vp};
    if (!vp)
        result.religions_version = reader.version(4);
    if (vp || *result.religions_version >= 3)
        result.minimum_faith_next_pantheon = input.i32();
    if (!vp && *result.religions_version < 4)
        result.obsolete_minimum_faith_next_prophet = input.i32();
    if (vp || *result.religions_version >= 2) {
        const auto count = reader.count(!vp);
        reader.claim(count, vp ? 163 : 136);
        result.religions.reserve(static_cast<std::size_t>(count));
        for (std::uint64_t i = 0; i < count; ++i)
            result.religions.push_back(reader.religion());
    }
    result.culture_offset = input.position();
    if (!vp)
        result.culture_version = reader.version(2);
    const auto works = reader.count(!vp);
    reader.claim(works, 24);
    result.great_works.reserve(static_cast<std::size_t>(works));
    for (std::uint64_t i = 0; i < works; ++i)
        result.great_works.push_back(reader.great_work());
    if (vp || *result.culture_version >= 2)
        result.reported_someone_influential = reader.flag();
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
