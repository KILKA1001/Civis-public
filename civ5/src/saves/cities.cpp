#include "civ5/saves/cities.hpp"
#include "civ5/saves/vp_cities_profile.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include "civ5/saves/vp_city_management_profile.hpp"
#include "civ5/saves/vp_city_tail_profile.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
// Count retained records and numeric entries separately from opaque string bytes.
// Charge by subtraction, avoiding overflow even for caller budgets above 4 GiB.
struct RetainedBudget {
    std::uint64_t elements, bytes;
    void element(std::uint64_t n = 1) {
        if (n > elements)
            throw std::runtime_error("Cities exceed aggregate retained element budget");
        elements -= n;
    }
    void blob(std::uint64_t n) {
        if (n > bytes)
            throw std::runtime_error("Cities exceed aggregate retained string budget");
        bytes -= n;
    }
    void core(const CityCoreObservation &c) {
        element();
        for (const auto &f : c.fields) {
            element();
            element(f.values.size());
            element(f.row_keys.size());
            element(f.row_counts.size());
            element(f.hashes.size());
            blob(f.bytes.size());
        }
    }
    void production(const CityProductionRecord &r) {
        element();
        element(r.fields.size());
        for (const auto &a : r.arrays) {
            element();
            element(a.values.size());
        }
        for (const auto &b : r.blobs) {
            element();
            blob(b.bytes.size());
        }
        for (const auto &c : r.children)
            production(c);
    }
    void management(const CityManagementBlock &b) {
        element();
        element(b.fields.size());
        for (const auto &a : b.arrays) {
            element();
            element(a.values.size());
        }
        for (const auto &h : b.hashes) {
            element();
            for (const auto &row : h.rows) {
                element();
                element(row.values.size());
            }
        }
    }
    void tail(const CityTailNode &n) {
        element();
        element(n.values.size());
        for (const auto &c : n.children)
            tail(c);
    }
};
} // namespace
CitiesObservation observe_cities(runtime::BinaryReader &in, GameStateLayout layout, CitiesContext context,
                                 CitiesBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *vp_dimensions = context.profile_extensions.find<VpCitiesWireDimensions>();
    if (vp ? bool(context.bnw_projects) : bool(vp_dimensions))
        throw std::runtime_error("Cities context does not match layout");
    CitiesObservation p{};
    p.layout = layout;
    p.context = context;
    p.start_offset = in.position();
    p.header = observe_city_collection_header(in, layout, budget.per_block);
    const auto count =
        p.header.wire_city_count > 0 ? static_cast<std::uint64_t>(p.header.wire_city_count) : 0;
    if (count && (vp ? !vp_dimensions : !context.bnw_projects))
        throw std::runtime_error("Nonempty cities need content dimensions");
    RetainedBudget remaining{budget.retained_elements, budget.retained_string_bytes};
    remaining.element();
    remaining.element(p.header.fields.size());
    remaining.element(p.header.next_free_indices.size());
    // Do not reserve using an untrusted count; each city is admitted only after
    // its fields pass the existing readers and the collection resource budget.
    for (std::uint64_t i = 0; i < count; ++i) {
        const char *stage = "core";
        try {
            remaining.element(); // SavedCityObservation
            SavedCityObservation c{};
            c.index = i;
            c.core = observe_city_core(in, layout, budget.per_block);
            remaining.core(c.core);
            CityProductionContext production;
            CityManagementContext management;
            CityTailContext tail;
            if (vp) {
                const auto &d = *vp_dimensions;
                production.profile_extensions.emplace<VpCityProductionWireDimensions>(
                    VpCityProductionWireDimensions{d.buildings});
                management.profile_extensions.emplace<VpCityManagementWireDimensions>(
                    VpCityManagementWireDimensions{d.flavors, d.strategies, d.promotions, d.buildings});
                tail.profile_extensions.emplace<VpCityTailWireDimensions>(
                    VpCityTailWireDimensions{d.yields, d.emphases});
            } else {
                production.bnw_city_version = c.core.version;
                management.bnw_projects = context.bnw_projects;
                tail.bnw_city_version = c.core.version;
            }
            stage = "production";
            c.production = observe_city_production(in, layout, production, budget.per_block);
            remaining.element();
            for (const auto &b : c.production.blocks)
                remaining.production(b);
            stage = "management";
            c.management = observe_city_management(in, layout, management, budget.per_block);
            remaining.element();
            for (const auto &b : c.management.blocks)
                remaining.management(b);
            stage = "tail";
            c.tail = observe_city_tail(in, layout, tail, budget.per_block);
            remaining.element();
            for (const auto &b : c.tail.blocks)
                remaining.tail(b);
            p.cities.push_back(std::move(c));
        } catch (const std::exception &e) {
            throw std::runtime_error("City " + std::to_string(i) + " " + stage + ": " + e.what());
        }
    }
    p.end_offset = in.position();
    p.retained_elements = budget.retained_elements - remaining.elements;
    p.retained_string_bytes = budget.retained_string_bytes - remaining.bytes;
    return p;
}
} // namespace civ5::saves
