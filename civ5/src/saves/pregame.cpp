#include "civ5/saves/pregame.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace civ5::saves {
namespace {
class Reader {
  public:
    Reader(runtime::BinaryReader &input, PregameBudget budget) : in(input), left(budget) {}
    runtime::BinaryReader &in;
    PregameBudget left;
    std::string string() {
        const std::uint64_t n = in.u32();
        std::string result;
        if (n > left.string_bytes || n > left.total_string_bytes || n > in.remaining() || n > result.max_size())
            throw std::runtime_error("Pregame string exceeds input or host budget");
        left.total_string_bytes -= n;
        result.resize(static_cast<std::size_t>(n));
        in.read({reinterpret_cast<std::uint8_t *>(result.data()), result.size()});
        return result;
    }
    bool flag() {
        const auto value = in.u8();
        if (value > 1) throw std::runtime_error("Noncanonical Pregame boolean");
        return value != 0;
    }
    std::uint64_t count(std::uint64_t wire_size) {
        const std::uint64_t n = in.u32();
        if (n > left.elements || n > in.remaining() / wire_size)
            throw std::runtime_error("Pregame collection exceeds input or host budget");
        left.elements -= n;
        return n;
    }
    template<class T, class F> std::vector<T> array(std::uint64_t wire_size, F read) {
        const auto n = count(wire_size);
        std::vector<T> result;
        if (n > result.max_size()) throw std::runtime_error("Pregame collection exceeds host address space");
        result.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) result.push_back(read());
        return result;
    }
    void integer(PregameRecord &r, const char *name) { r.integers.emplace(name, in.i32()); }
    void unsigned_integer(PregameRecord &r, const char *name) { r.integers.emplace(name, in.u32()); }
    void boolean(PregameRecord &r, const char *name) { r.flags.emplace(name, flag()); }
    void floating(PregameRecord &r, const char *name) { r.float_bits.emplace(name, in.u32()); }
    void text(PregameRecord &r, const char *name) { r.strings.emplace(name, string()); }
    void integers(PregameRecord &r, const char *name) {
        r.integer_arrays.emplace(name, array<std::int64_t>(4, [&] { return in.i32(); }));
    }
    void flags(PregameRecord &r, const char *name) {
        r.flag_arrays.emplace(name, array<bool>(1, [&] { return flag(); }));
    }
    void strings(PregameRecord &r, const char *name) {
        r.string_arrays.emplace(name, array<std::string>(4, [&] { return string(); }));
    }
    PregameInfo base() {
        PregameInfo result;
        result.id = in.i32();
        for (auto &s : result.text) s = string();
        return result;
    }
    std::vector<PregameOption> options() {
        return array<PregameOption>(8, [&] { auto name = string(); return PregameOption{std::move(name), in.i32()}; });
    }
};
}

PregameObservation observe_pregame(std::span<const std::uint8_t> prefix, const SaveHeader &header,
                                  GameStateLayout layout, PregameBudget budget) {
    if (layout != GameStateLayout::expansion2_version1 && layout != GameStateLayout::vp542_corpus)
        throw std::runtime_error("Unverified Pregame layout");
    if (prefix.size() > budget.total_bytes) throw std::runtime_error("Pregame prefix exceeds host byte budget");
    validate_source_prefix(prefix, header);
    std::string storage(reinterpret_cast<const char *>(prefix.data()), prefix.size());
    std::istringstream stream(storage, std::ios::binary);
    runtime::BinaryReader input(stream, prefix.size());
    input.seek(header.end_offset);
    Reader r(input, budget);
    PregameObservation out;
    out.layout = layout;
    const bool vp = layout == GameStateLayout::vp542_corpus;
    out.header_tail_offset = input.position();
    for (auto &s : out.header_tail_strings) s = r.string();
    input.read(out.gamecore_guid);
    out.gamecore_version = r.string();
    input.read(out.engine_guid);
    out.engine_value = input.i32();
    if (windows_guid(out.gamecore_guid) != (vp ? "bf9bf7f0-e078-4d4e-8a3e-84712f85aa2b" :
                                               "83f34ff0-3d60-43e7-aedb-82c79867ee3f"))
        throw std::runtime_error("Pregame GameCore identity disagrees with selected wire layout");
    out.slots_offset = input.position();
    out.slots_version = input.u32();
    if (out.slots_version != 3) throw std::runtime_error("Unverified Pregame slot-hints version");
    auto &s = out.slots;
    r.integer(s, "game_speed"); r.integer(s, "world_size"); r.text(s, "map_script");
    r.integers(s, "civilizations"); r.strings(s, "nicknames");
    for (auto name : {"slot_status", "slot_claims", "team_types", "handicaps"}) r.integers(s, name);
    r.strings(s, "civilization_keys"); r.strings(s, "leader_keys");
    if (vp) {
        // The pinned 43-major-player VP DLL serializes uint64 masks. This is a
        // source field width, not a MAX_MAJOR_CIVS restriction on live Civis.
        out.wire_known_player_masks = r.array<std::uint64_t>(8, [&] {
            const std::uint64_t lo = input.u32(), hi = input.u32();
            return lo | (hi << 32);
        });
    }
    out.archive_offset = input.position();
    out.archive_version = input.u32();
    if (out.archive_version != 6) throw std::runtime_error("Unverified Pregame archive version");
    auto &a = out.archive;
    r.integer(a, "active_player"); r.text(a, "admin_password");
    if (!vp) r.integer(a, "advanced_start_points");
    r.text(a, "alias"); r.integers(a, "art_styles"); r.boolean(a, "autorun");
    r.floating(a, "autorun_turn_delay"); r.integer(a, "autorun_turn_limit");
    r.integer(a, "bandwidth"); r.integer(a, "calendar"); out.calendar = r.base();
    for (auto name : {"civ_adjectives", "civ_descriptions", "civ_passwords", "civ_short_descriptions"}) r.strings(a, name);
    r.integer(a, "climate"); out.climate = r.base();
    for (auto name : {"desert_percent_change", "jungle_latitude", "hill_range", "mountain_percent"})
        r.integer(out.climate.values, name);
    for (auto name : {"snow_latitude_change", "tundra_latitude_change", "grass_latitude_change",
                     "desert_bottom_latitude_change", "desert_top_latitude_change", "ice_latitude", "rand_ice_latitude"})
        r.floating(out.climate.values, name);
    r.integer(a, "era"); r.strings(a, "email_addresses"); r.floating(a, "end_turn_timer_length");
    r.strings(a, "flag_decals"); r.flags(a, "deprecated_force_controls");
    r.integer(a, "game_mode"); r.text(a, "game_name"); r.integer(a, "game_speed");
    r.boolean(a, "game_started"); r.integer(a, "game_turn");
    // Native GameTypes is a signed byte; GameMapTypes is an int32 enum.
    const std::int64_t game_type = input.u8();
    a.integers.emplace("game_type", game_type < 128 ? game_type : game_type - 256);
    r.integer(a, "game_map_type");
    r.integer(a, "game_update_time"); r.integers(a, "handicaps"); r.integers(a, "last_human_handicaps");
    r.boolean(a, "is_earth_map"); r.boolean(a, "is_internet_game");
    r.strings(a, "leader_names"); r.text(a, "load_file_name"); r.text(a, "local_player_email");
    r.boolean(a, "map_no_players"); r.unsigned_integer(a, "map_random_seed");
    r.boolean(a, "load_wb_scenario"); r.boolean(a, "override_scenario_handicap"); r.text(a, "map_script");
    for (auto name : {"max_city_elimination", "max_turns", "num_minor_civs"}) r.integer(a, name);
    // Type names are preserved; DB lookup belongs to binding the active rules.
    r.strings(a, "minor_civ_types"); r.flags(a, "minor_nation_civs"); r.boolean(a, "dummy_value");
    r.flags(a, "multiplayer_options"); r.integers(a, "net_ids"); r.strings(a, "nicknames");
    r.integer(a, "num_victory_infos"); r.integer(a, "pit_boss_turn_time");
    r.flags(a, "playable_civs"); r.strings(a, "player_colors");
    r.boolean(a, "private_game"); r.boolean(a, "quick_combat"); r.boolean(a, "quick_combat_default");
    r.integer(a, "quick_handicap"); r.boolean(a, "quickstart");
    r.boolean(a, "random_world_size"); r.boolean(a, "random_map_script"); r.flags(a, "ready_players");
    r.integer(a, "sea_level"); out.sea_level = r.base(); r.integer(out.sea_level.values, "sea_level_change");
    r.boolean(a, "dummy_value2"); r.integers(a, "slot_claims"); r.integers(a, "slot_status");
    r.text(a, "smtp_host"); r.unsigned_integer(a, "sync_random_seed"); r.integer(a, "target_score");
    r.integers(a, "team_types"); r.boolean(a, "transferred_map"); out.turn_timer = r.base();
    for (auto name : {"base_time", "city_resource", "unit_resource", "first_turn_multiplier"})
        r.integer(out.turn_timer.values, name);
    r.integer(a, "turn_timer_type"); r.boolean(a, "city_screen_blocked");
    r.flags(a, "victories"); r.flags(a, "white_flags");
    if (!vp) {
        const auto version = input.u32();
        if (version != 2) throw std::runtime_error("Unverified BNW Pregame WorldInfo version");
        out.world = r.base(); out.world.wire_version = version;
    } else out.world = r.base();
    for (auto name : {"default_players", "default_minor_civs", "fog_tiles_per_barbarian_camp", "num_natural_wonders",
                     "unit_name_modifier", "target_num_cities", "num_free_building_resources", "building_class_prereq_modifier"})
        r.integer(out.world.values, name);
    if (!vp) r.integer(out.world.values, "max_conscript_modifier");
    for (auto name : {"grid_width", "grid_height", "max_active_religions", "terrain_grain_change", "feature_grain_change", "research_percent"})
        r.integer(out.world.values, name);
    if (!vp) r.integer(out.world.values, "advanced_start_points_mod");
    for (auto name : {"num_cities_unhappiness_percent", "num_cities_policy_cost_mod", "num_cities_tech_cost_mod"})
        r.integer(out.world.values, name);
    if (vp) for (auto name : {"trade_route_distance_mod", "num_cities_tourism_cost_mod", "num_cities_unit_supply_mod",
                             "min_distance_cities", "min_distance_city_states", "reformation_percent"})
        r.integer(out.world.values, name);
    r.integer(a, "world_size"); out.game_options = r.options(); out.map_options = r.options();
    r.text(a, "version_string"); r.flags(a, "turn_notify_steam_invite"); r.flags(a, "turn_notify_email");
    r.strings(a, "turn_notify_email_address");
    out.archive_end_offset = input.position();
    // The two verified engine envelopes have one separate 4-byte trailer.
    // Keep it opaque; do not attribute an unverified meaning to its value.
    input.read(out.engine_trailer);
    if (out.engine_trailer != std::array<std::uint8_t, 4>{2, 0, 0, 0} || input.remaining() != 0)
        throw std::runtime_error("Unverified engine bytes after Pregame archive");
    out.end_offset = input.position();
    return out;
}
} // namespace civ5::saves
