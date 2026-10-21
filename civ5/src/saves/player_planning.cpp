#include "civ5/saves/player_planning.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
enum class Wire { I32, U32, I8, Bool };
enum class Extent { S, M, MM, C, L };
struct MinorField {
    bool vp;
    const char *name;
    Wire wire;
    Extent extent;
};
constexpr MinorField minor_fields[] = {
#define VP(name, wire, extent) {true, #name, Wire::wire, Extent::extent},
#define BNW(name, wire, extent) {false, #name, Wire::wire, Extent::extent},
#include "player_planning_minor_fields.inc"
#undef VP
#undef BNW
};
using Record = PlayerPlanningRecord;
struct PlanningReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    PlayerPlanningWireDimensions d;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player planning exceeds input or element budget");
        budget.elements -= n;
    }
    std::uint64_t product(std::uint64_t a, std::uint64_t b) {
        if (b && a > budget.elements / b)
            throw std::runtime_error("Player planning dimensions exceed budget");
        return a * b;
    }
    std::int64_t read(Wire wire) {
        if (wire == Wire::I32)
            return in.i32();
        if (wire == Wire::U32)
            return in.u32();
        const auto n = in.u8();
        if (wire == Wire::I8)
            return n < 128 ? n : std::int64_t(n) - 256;
        if (n > 1)
            throw std::runtime_error("Unverified player planning boolean");
        return n;
    }
    std::int64_t field(Record &b, const char *name, Wire wire = Wire::I32) {
        claim(1);
        const auto n = read(wire);
        b.fields.push_back({name, n});
        return n;
    }
    void fields(Record &b, std::initializer_list<const char *> names, Wire wire = Wire::I32) {
        for (auto name : names)
            field(b, name, wire);
    }
    std::int64_t version(Record &b, std::int64_t maximum = 1) {
        const auto v = field(b, "version", Wire::U32);
        if (v < 1 || v > maximum)
            throw std::runtime_error("Unverified player planning block version");
        return v;
    }
    // Only STL serialization recognizes 0xffffffff as an empty container.
    std::uint64_t count(Record &b, const char *name, bool stl = true, bool signed_count = false) {
        const auto n = field(b, name, signed_count ? Wire::I32 : Wire::U32);
        return (stl && n == UINT32_MAX) || n <= 0 ? 0 : static_cast<std::uint64_t>(n);
    }
    void array(Record &b, const char *name, std::uint64_t n, Wire wire = Wire::I32,
               std::uint64_t columns = 1) {
        if (!columns)
            throw std::runtime_error("Zero player planning array width");
        const auto total = product(n, columns);
        claim(total, wire == Wire::I32 || wire == Wire::U32 ? 4 : 1);
        PlotArrayObservation a{name, columns, {}};
        a.values.reserve(static_cast<std::size_t>(total));
        for (std::uint64_t i = 0; i < total; ++i)
            a.values.push_back(read(wire));
        b.arrays.push_back(std::move(a));
    }
    void vector(Record &b, const char *name, const char *count_name, std::uint64_t columns = 1) {
        array(b, name, count(b, count_name), Wire::I32, columns);
    }
    template <class F> void records(Record &b, const char *name, std::uint64_t n, F decode) {
        claim(n);
        for (std::uint64_t i = 0; i < n; ++i) {
            Record child{name, i, in.position(), 0, {}, {}, {}, {}};
            decode(child);
            child.end_offset = in.position();
            b.children.push_back(std::move(child));
        }
    }
    void blob(Record &b, const char *name, const char *count_name) {
        const auto n = count(b, count_name, false);
        if (n > budget.string_bytes || n > budget.total_string_bytes || n > in.remaining())
            throw std::runtime_error("Player planning blob exceeds input or byte budget");
        budget.total_string_bytes -= n;
        PlayerPlanningBlob value{name, std::vector<std::uint8_t>(static_cast<std::size_t>(n))};
        in.read(value.bytes);
        b.blobs.push_back(std::move(value));
    }
    void tactical(Record &b) {
        if (!vp) {
            version(b);
            field(b, "phony");
            records(b, "temporary_zone", count(b, "temporary_zones_wire_count", false), [&](Record &z) {
                version(z);
                fields(z, {"target_type", "last_turn", "x", "y", "naval_invasion"});
            });
            return;
        }
        records(b, "focus_area", count(b, "focus_areas_wire_count"),
                [&](Record &z) { fields(z, {"x", "y", "radius", "last_turn"}); });
        fields(b, {"player", "last_update"});
        vector(b, "plot_zone_ids", "plot_zone_ids_wire_count");
        records(b, "dominance_zone", count(b, "dominance_zones_wire_count"), [&](Record &z) {
            fields(z, {"zone_id", "territory_type", "overall_dominance", "posture", "owner", "city_id",
                       "area_id"});
            fields(z,
                   {"friendly_melee_strength", "enemy_melee_strength", "friendly_ranged_strength",
                    "enemy_ranged_strength", "friendly_naval_strength", "enemy_naval_strength",
                    "friendly_naval_ranged_strength", "enemy_naval_ranged_strength"},
                   Wire::U32);
            fields(z, {"friendly_unit_count", "enemy_unit_count", "neutral_unit_count"});
            field(z, "neutral_unit_strength", Wire::U32);
            // The source reads the reserved slot as int and discards it. Retain
            // its bits and scaled coordinates without rounding to plot centers.
            fields(z, {"enemy_naval_unit_count", "friendly_naval_unit_count", "zone_value", "reserved",
                       "average_x_times1000", "average_y_times1000", "plot_count"});
            vector(z, "neighboring_zones", "neighboring_zones_wire_count");
        });
        vector(b, "id_lookup", "id_lookup_wire_count", 2);
    }
    void homeland(Record &b) {
        if (!vp) {
            version(b);
            field(b, "move_priority_turn");
        }
    }
    void gift(Record &b) {
        if (field(b, "arrival_countdown") == -1)
            return;
        fields(b,
               {"unit_type", "from_x", "from_y", "original_owner", "gifted_by_player", "game_turn_created"});
        blob(b, "promotions", "promotion_bytes_wire_count");
        vector(b, "promotion_duration", "promotion_duration_wire_count", 2);
        vector(b, "turn_promotion_gained", "turn_promotion_gained_wire_count", 2);
        field(b, "promoted_from_goody", Wire::Bool);
        fields(b, {"experience_times100", "level", "origin_city", "leader_unit_type", "goody_huts_popped"});
        blob(b, "name", "name_bytes_wire_count");
    }
    void quest(Record &b) {
        if (!vp) {
            const auto v = version(b, 2);
            fields(b, {"type", "start_turn", "data1", "data2"});
            if (v >= 2)
                field(b, "handled", Wire::Bool);
            return;
        }
        fields(b, {"minor",
                   "assigned_player",
                   "type",
                   "start_turn",
                   "data1",
                   "data2",
                   "data3",
                   "influence",
                   "disabled_influence",
                   "gold",
                   "science",
                   "culture",
                   "faith",
                   "golden_age_points",
                   "food",
                   "production",
                   "tourism",
                   "happiness",
                   "gp",
                   "gp_global",
                   "general_points",
                   "admiral_points",
                   "experience",
                   "juggernauts"});
        fields(b, {"partial_quest", "handled"}, Wire::Bool);
    }
    void minor(Record &b) {
        if (!vp)
            version(b);
        for (auto f : minor_fields) {
            if (f.vp != vp)
                continue;
            if (f.extent == Extent::S) {
                field(b, f.name, f.wire);
                continue;
            }
            auto n = d.major, columns = std::uint64_t(1);
            if (f.extent == Extent::C)
                n = d.civ_teams;
            if (f.extent == Extent::L)
                n = d.legacy_teams;
            if (f.extent == Extent::MM) {
                // An empty matrix has no entries, but a nonzero reported width.
                columns = d.major ? d.major : 1;
            }
            array(b, f.name, n, f.wire, columns);
        }
        if (vp)
            records(b, "incoming_unit_gift", d.major, [&](Record &g) { gift(g); });
        const auto n = vp ? count(b, "quest_players_wire_count") : d.major;
        records(b, "player_quests", n, [&](Record &p) {
            records(p, "quest", count(p, "quests_wire_count", vp, !vp), [&](Record &q) { quest(q); });
        });
        field(b, "m_bDisableNotifications", Wire::Bool);
    }
    void deal(Record &b) {
        if (!vp)
            version(b);
    }
    void builder(Record &b) {
        if (vp) {
            vector(b, "best_route_type_and_value", "best_routes_wire_count", 3);
            vector(b, "plot_route_purposes", "route_purposes_wire_count", 2);
            vector(b, "any_route_planned", "planned_routes_wire_count", 3);
            vector(b, "canal_wanted_plots", "canal_plots_wire_count");
            vector(b, "unique_improvements", "unique_improvements_wire_count");
        } else {
            const auto v = version(b, 2);
            field(b, "repair_build");
            array(b, "non_territory_plots", count(b, "non_territory_plots_wire_count", false));
            field(b, "keep_marshes", Wire::Bool);
            if (v >= 2)
                field(b, "keep_jungle", Wire::Bool);
        }
    }
    void connections(Record &b) {
        if (vp) {
            vector(b, "plots_with_connection_to_capital", "connected_plots_wire_count");
            vector(b, "plots_with_industrial_connection_to_capital", "industrial_plots_wire_count");
            records(b, "connection_state", count(b, "connection_states_wire_count"), [&](Record &s) {
                field(s, "city_id");
                vector(s, "connections", "connections_wire_count", 3);
            });
            vector(b, "plot_ids_to_connect", "plots_to_connect_wire_count");
        } else {
            version(b);
            const auto n = static_cast<std::uint64_t>(field(b, "route_infos_dimension", Wire::U32));
            array(b, "route_infos", product(n, n), Wire::I8, 2);
            array(b, "city_plot_ids", count(b, "city_ids_wire_count", false, true));
        }
    }
    void danger(Record &b) {
        if (!vp)
            version(b);
        field(b, "player");
        if (vp) {
            vector(b, "known_units", "known_units_wire_count", 2);
            vector(b, "vanished_units", "vanished_units_wire_count", 2);
        } else {
            field(b, "array_allocated", Wire::Bool);
            const auto n = field(b, "grid_size");
            if (n < 0)
                throw std::runtime_error("Negative BNW danger grid allocation");
            array(b, "danger_plots", static_cast<std::uint64_t>(n), Wire::U32);
        }
    }
};
} // namespace
PlayerPlanningObservation observe_player_planning(runtime::BinaryReader &in, GameStateLayout layout,
                                                  PlayerPlanningWireDimensions dimensions,
                                                  GameStateBudget budget) {
    PlayerPlanningObservation result{layout, dimensions, in.position(), 0, {}};
    PlanningReader r{in, budget, layout == GameStateLayout::vp542_corpus, dimensions};
    using Method = void (PlanningReader::*)(Record &);
    constexpr std::pair<const char *, Method> steps[] = {
        {"tactical_ai", &PlanningReader::tactical},       {"homeland_ai", &PlanningReader::homeland},
        {"minor_civ_ai", &PlanningReader::minor},         {"deal_ai", &PlanningReader::deal},
        {"builder_tasking_ai", &PlanningReader::builder}, {"city_connections", &PlanningReader::connections},
        {"danger_plots", &PlanningReader::danger}};
    for (auto [name, method] : steps) {
        Record b{name, 0, in.position(), 0, {}, {}, {}, {}};
        try {
            (r.*method)(b);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Player ") + name + ": " + e.what());
        }
        b.end_offset = in.position();
        result.blocks.push_back(std::move(b));
    }
    result.end_offset = in.position();
    return result;
}
} // namespace civ5::saves
