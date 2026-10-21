#include "civ5/saves/vp_player_knowledge_profile.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace civ5::saves {
namespace {
struct KnowledgeReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    const VpPlayerKnowledgeWireDimensions *dimensions;
    void claim(std::uint64_t n, std::uint64_t width = 0) {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Player knowledge exceeds input or element budget");
        budget.elements -= n;
    }
    std::int64_t boolean() {
        const auto b = in.u8();
        if (b > 1)
            throw std::runtime_error("Unverified player knowledge boolean");
        return b;
    }
    void value(PlayerKnowledgeBlock &b, const char *name, std::int64_t v) {
        claim(1);
        b.fields.push_back({name, v});
    }
    void integer_fields(PlayerKnowledgeBlock &b, std::initializer_list<const char *> names) {
        for (auto name : names)
            value(b, name, in.i32());
    }
    void version(PlayerKnowledgeBlock &b, const char *name = "version") {
        const auto v = in.u32();
        value(b, name, v);
        if (v != 1)
            throw std::runtime_error("Unverified player knowledge block version");
    }
    std::uint64_t count(PlayerKnowledgeBlock &b, const char *name, bool stl = false) {
        const std::int64_t n = stl ? std::int64_t(in.u32()) : in.i32();
        value(b, name, n);
        return (stl && n == UINT32_MAX) || n <= 0 ? 0 : static_cast<std::uint64_t>(n);
    }
    void array(PlayerKnowledgeBlock &b, const char *name, std::uint64_t n, bool flag = false,
               std::uint64_t columns = 1) {
        if (!columns || n > budget.elements / columns)
            throw std::runtime_error("Player knowledge array dimensions exceed budget");
        claim(n * columns, flag ? 1 : 4);
        PlotArrayObservation a{name, columns, {}};
        a.values.reserve(static_cast<std::size_t>(n * columns));
        for (std::uint64_t i = 0; i < n * columns; ++i)
            a.values.push_back(flag ? boolean() : in.i32());
        b.arrays.push_back(std::move(a));
    }
    void hashed(PlayerKnowledgeBlock &b, const char *name, bool flag = false, bool weights = false) {
        TeamHashedArray a{};
        a.name = name;
        a.wire_count = weights ? std::int64_t(in.i32()) : in.u32();
        const auto n = a.wire_count > 0 ? static_cast<std::uint64_t>(a.wire_count) : 0;
        claim(n, 4);
        for (std::uint64_t i = 0; i < n; ++i) {
            TeamHashedValues row;
            row.type_hash = in.u32();
            // TechAI writes the weight even when WriteHashed returned false.
            if (weights || row.type_hash) {
                claim(1);
                row.values.push_back(flag ? boolean() : in.i32());
            }
            a.rows.push_back(std::move(row));
        }
        b.hashes.push_back(std::move(a));
    }
    void religions(PlayerKnowledgeBlock &b) {
        if (vp) {
            integer_fields(b, {"free_prophets_spawned", "prophets_spawned"});
            value(b, "founding_religion", boolean());
            integer_fields(b, {"majority_religion", "state_religion_override", "state_religion"});
            value(b, "owns_state_religion", boolean());
            integer_fields(b, {"faith_at_last_notify_times100"});
        } else {
            version(b);
            // BNW declares m_bFoundingReligion as int, despite its name.
            integer_fields(b, {"prophets_spawned", "founding_religion"});
        }
    }
    void religion_ai(PlayerKnowledgeBlock &b) {
        if (!vp)
            version(b);
    }
    void corporations(PlayerKnowledgeBlock &b) {
        integer_fields(b, {"founded_corporation", "offices", "franchises", "additional_franchises",
                           "additional_franchises_modifier", "offices_as_franchises",
                           "random_foreign_franchise_modifier", "free_franchise_above_popular",
                           "no_foreign_corporations_in_cities", "no_franchises_in_foreign_cities"});
        array(b, "franchises_per_improvement", count(b, "franchises_per_improvement_wire_count", true));
    }
    void techs(PlayerKnowledgeBlock &b) {
        if (!vp)
            version(b);
        for (auto name : {"researching_tech", "civ_tech_priority", "locale_tech_priority",
                          "grand_strategy_tech_priority", "locale_tech_resource", "civ_tech_unique_unit",
                          "civ_tech_unique_building", "civ_tech_unique_improvement"}) {
            const std::string_view key(name);
            if (!vp && key == "grand_strategy_tech_priority")
                continue;
            const bool flag = key == "researching_tech";
            if (vp)
                array(b, name, dimensions->techs, flag);
            else
                hashed(b, name, flag);
        }
        if (vp)
            array(b, "tech_ai_weights", count(b, "tech_ai_weights_wire_count"), false, 2);
        else {
            version(b, "tech_ai_version");
            hashed(b, "tech_ai_weights", false, true);
        }
        array(b, "latest_flavors", vp ? dimensions->flavors : count(b, "latest_flavors_wire_count"));
        if (vp) {
            value(b, "has_unique_unit_tech", boolean());
            value(b, "will_have_unique_unit_tech_soon", boolean());
        }
    }
    void flavors(PlayerKnowledgeBlock &b) {
        if (!vp)
            version(b);
        const auto n = vp ? dimensions->flavors : count(b, "flavors_wire_count");
        array(b, "personality_flavors", n);
        array(b, "active_flavors", n);
    }
};
} // namespace
PlayerKnowledgeObservation observe_player_knowledge(runtime::BinaryReader &in, GameStateLayout layout,
                                                    PlayerKnowledgeDecodeContext context,
                                                    GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *vp_rows = vp_player_knowledge_dimensions(context);
    if (vp != (vp_rows != nullptr))
        throw std::runtime_error("Supply knowledge rules dimensions only for VP");
    PlayerKnowledgeObservation result{layout, std::move(context.profile_extensions), in.position(), 0, {}};
    KnowledgeReader r{in, budget, vp, vp_player_knowledge_dimensions(result)};
    using Method = void (KnowledgeReader::*)(PlayerKnowledgeBlock &);
    constexpr std::pair<const char *, Method> steps[] = {
        {"player_religions", &KnowledgeReader::religions},
        {"religion_ai", &KnowledgeReader::religion_ai},
        {"player_corporations", &KnowledgeReader::corporations},
        {"player_techs", &KnowledgeReader::techs},
        {"flavor_manager", &KnowledgeReader::flavors}};
    for (auto [name, method] : steps) {
        if (!vp && method == &KnowledgeReader::corporations)
            continue;
        PlayerKnowledgeBlock b{};
        b.name = name;
        b.start_offset = in.position();
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
