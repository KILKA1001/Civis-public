#include "civ5/saves/player_strategy.hpp"
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace civ5::saves {
namespace {
struct StrategyReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    const PlayerStrategyWireDimensions &d;
    const VpPlayerStrategyWireDimensions *vp_rows;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player strategy exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t number(char type) {
        if (type == 'i')
            return in.i32();
        if (type == 'I')
            return in.u32();
        const auto n = in.u8();
        if (n > 1)
            throw std::runtime_error("Unverified player strategy boolean");
        return n;
    }
    void scalar(PlayerStrategyBlock &b, const char *name, std::int64_t value) {
        claim(1);
        b.fields.push_back({name, value});
    }
    void fields(PlayerStrategyBlock &b, std::initializer_list<const char *> names, char type = 'i') {
        for (auto name : names)
            scalar(b, name, number(type));
    }
    std::uint64_t positive(std::int64_t n) { return n > 0 ? static_cast<std::uint64_t>(n) : 0; }
    std::uint64_t count(PlayerStrategyBlock &b, const char *name, bool signed_count = false,
                        bool sentinel = false) {
        const auto n = number(signed_count ? 'i' : 'I');
        scalar(b, name, n);
        return sentinel && n == UINT32_MAX ? 0 : positive(n);
    }
    void array(PlayerStrategyBlock &b, const char *name, std::uint64_t n, std::string_view types = "i") {
        if (types.empty() || n > budget.elements / types.size())
            throw std::runtime_error("Player strategy row dimensions exceed budget");
        std::uint64_t width = 0;
        for (auto t : types)
            width += t == 'B' ? 1 : 4;
        if (n > in.remaining() / width)
            throw std::runtime_error("Player strategy array exceeds input");
        claim(n * types.size());
        PlotArrayObservation a{name, types.size(), {}};
        a.values.reserve(static_cast<std::size_t>(n * types.size()));
        for (std::uint64_t i = 0; i < n; ++i)
            for (auto t : types)
                a.values.push_back(number(t));
        b.arrays.push_back(std::move(a));
    }
    void counted(PlayerStrategyBlock &b, const char *name, const char *count_name,
                 std::string_view types = "i", bool signed_count = false, bool sentinel = false) {
        array(b, name, count(b, count_name, signed_count, sentinel), types);
    }
    void hashed(PlayerStrategyBlock &b, const char *name, char type = 'i', bool skip_zero = true,
                bool signed_count = false) {
        TeamHashedArray a{};
        a.name = name;
        a.wire_count = number(signed_count ? 'i' : 'I');
        const auto n = positive(a.wire_count);
        claim(n, 4);
        for (std::uint64_t i = 0; i < n; ++i) {
            TeamHashedValues row;
            row.type_hash = in.u32();
            if (!skip_zero || row.type_hash) {
                claim(1);
                row.values.push_back(number(type));
            }
            a.rows.push_back(std::move(row));
        }
        b.hashes.push_back(std::move(a));
    }
    std::uint64_t version(PlayerStrategyBlock &b, std::uint64_t maximum = 1) {
        const auto n = in.u32();
        scalar(b, "version", n);
        if (!n || n > maximum)
            throw std::runtime_error("Unverified player strategy block version");
        return n;
    }
    void policies(PlayerStrategyBlock &b) {
        if (vp) {
            const auto &c = *vp_rows;
            for (auto name : {"free_policy", "has_policy", "one_shot_fired", "one_shot_free_units_fired"})
                array(b, name, c.policies, "B");
            for (auto name : {"branch_unlocked", "branch_blocked", "branch_finished"})
                array(b, name, c.branches, "B");
            fields(b, {"branch_picked_1", "branch_picked_2", "branch_picked_3"});
            array(b, "latest_flavors", c.flavors);
            counted(b, "policy_ai_weights", "policy_ai_weights_wire_count", "ii", true);
            counted(b, "policy_modifiers", "policy_modifiers_wire_count", "i", false, true);
        } else {
            const auto v = version(b, 2);
            for (auto name : {"has_policy", "one_shot_fired", "one_shot_free_units_fired", "branch_unlocked",
                              "branch_blocked", "branch_finished"})
                hashed(b, name, 'B');
            counted(b, "branches_chosen_hashes", "branches_chosen_hashes_wire_count", "I");
            fields(b, {"extra_branches"});
            fields(b, {"branch_picked_1_hash", "branch_picked_2_hash", "branch_picked_3_hash"}, 'I');
            if (v < 2)
                fields(b, {"legacy_max_effective_cities"});
            const auto ai = in.u32();
            scalar(b, "policy_ai_version", ai);
            if (ai != 1)
                throw std::runtime_error("Unverified policy AI version");
            // This loop always saves a weight, including for hash 0.
            hashed(b, "policy_ai_weights", 'i', false);
            counted(b, "latest_flavors", "latest_flavors_wire_count", "i", true);
        }
    }
    void economic(PlayerStrategyBlock &b) {
        if (!vp)
            version(b);
        const auto n = vp ? vp_rows->economic : count(b, "strategy_wire_count", true);
        array(b, "using_strategy", n, "B");
        array(b, "turn_strategy_adopted", n);
        if (vp) {
            counted(b, "exploration_land", "exploration_land_wire_count", "iiii", false, true);
            counted(b, "exploration_coast", "exploration_coast_wire_count", "iiii", false, true);
            counted(b, "exploration_ocean", "exploration_ocean_wire_count", "iiii", false, true);
            fields(b, {"recon_state", "naval_recon_state", "explorers_disbanded", "last_worker_disband_turn",
                       "visible_antiquity_sites", "own_antiquity_sites", "own_hidden_antiquity_sites",
                       "neutral_antiquity_sites", "explorers_needed", "naval_explorers_needed"});
        } else {
            array(b, "yields", d.yields, "I");
            fields(b, {"exploration_plots_dirty"}, 'B');
            counted(b, "exploration_plots", "exploration_plots_wire_count", "ii", true);
            counted(b, "goody_hut_assignments", "goody_hut_assignments_wire_count", "ii", true);
            fields(b, {"recon_state", "naval_recon_state", "explorers_disbanded", "last_worker_disband_turn",
                       "visible_antiquity_sites"});
        }
        counted(b, "requested_savings", "requested_savings_wire_count", "iii", !vp, vp);
    }
    void city_specialization(PlayerStrategyBlock &b) {
        const auto v = vp ? 2 : version(b, 2);
        fields(b, {"specializations_dirty", "interrupt_wonders"}, 'B');
        if (vp)
            fields(b, {"interrupt_buildings", "choose_new_wonder"}, 'B');
        else
            fields(b, {"next_specialization"});
        fields(b, {"next_wonder", "wonder_city_id", "next_wonder_weight"});
        if (v >= 2)
            fields(b, {"last_evaluated_turn"});
    }
    void wonders(PlayerStrategyBlock &b) {
        if (vp)
            counted(b, "wonder_weights", "wonder_weights_wire_count", "ii", true);
        else {
            version(b);
            counted(b, "latest_flavors", "latest_flavors_wire_count", "i", true);
            hashed(b, "wonder_weights", 'i', true, true);
        }
    }
    void military(PlayerStrategyBlock &b) {
        if (vp) {
            fields(b, {"ops_build_skips",
                       "settler_build_skips",
                       "land_units",
                       "ranged_land_units",
                       "mobile_land_units",
                       "melee_land_units",
                       "naval_units",
                       "land_units_in_armies",
                       "naval_units_in_armies",
                       "recommended_naval_units",
                       "air_units",
                       "anti_air_units",
                       "barbarian_camps",
                       "visible_barbarians",
                       "recommended_land_units",
                       "recommended_explorers",
                       "free_carriers",
                       "archer_land_units",
                       "siege_land_units",
                       "skirmisher_land_units",
                       "recon_land_units",
                       "bomber_air_units",
                       "fighter_air_units",
                       "melee_naval_units",
                       "ranged_naval_units",
                       "submarine_naval_units",
                       "carrier_naval_units",
                       "missile_units",
                       "active_unique_units",
                       "land_defense_state",
                       "naval_defense_state"});
            counted(b, "attack_targets", "attack_targets_wire_count", "iiiiiiB", false, true);
            counted(b, "exposed_cities", "exposed_cities_wire_count", "iiiiiiB", false, true);
            array(b, "using_strategy", vp_rows->military, "B");
            array(b, "turn_strategy_adopted", vp_rows->military);
            array(b, "war_focus", d.major);
        } else {
            version(b);
            fields(b, {"total_threat_weight", "army_type_building", "ops_build_skips"});
            const auto n = count(b, "strategy_wire_count", true);
            array(b, "using_strategy", n, "B");
            array(b, "turn_strategy_adopted", n);
            for (auto name : {"last_turn_war_state", "last_turn_military_threat",
                              "last_turn_military_strength", "last_turn_target_value"})
                array(b, name, d.civ_players);
        }
    }
    void grand(PlayerStrategyBlock &b) {
        if (!vp)
            version(b);
        fields(b, {"turns_since_active_set", "active_strategy"});
        array(b, "priorities", vp ? vp_rows->grand : d.grand);
        array(b, "other_player_strategy_guesses", d.major);
        array(b, "guess_confidence", d.major);
    }
};
} // namespace
PlayerStrategyObservation observe_player_strategy(runtime::BinaryReader &input, GameStateLayout layout,
                                                  PlayerStrategyWireDimensions dimensions,
                                                  GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *vp_rows=vp_player_strategy_dimensions(dimensions);
    if (vp != (vp_rows!=nullptr))
        throw std::runtime_error("Supply rules dimensions only for VP player strategy");
    PlayerStrategyObservation result{layout, dimensions, input.position(), 0, {}};
    StrategyReader r{input, budget, vp, dimensions, vp_rows};
    using Method = void (StrategyReader::*)(PlayerStrategyBlock &);
    constexpr std::pair<const char *, Method> steps[] = {
        {"policies", &StrategyReader::policies},
        {"economic_ai", &StrategyReader::economic},
        {"city_specialization_ai", &StrategyReader::city_specialization},
        {"wonder_production_ai", &StrategyReader::wonders},
        {"military_ai", &StrategyReader::military},
        {"grand_strategy_ai", &StrategyReader::grand}};
    for (auto [name, method] : steps) {
        PlayerStrategyBlock b{};
        b.name = name;
        b.start_offset = input.position();
        try {
            (r.*method)(b);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Player ") + name + ": " + e.what());
        }
        b.end_offset = input.position();
        result.blocks.push_back(std::move(b));
    }
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
