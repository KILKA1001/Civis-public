#include "civ5/saves/units.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
struct RetainedBudget {
    std::uint64_t elements, bytes;
    void element(std::uint64_t n = 1) {
        if (n > elements)
            throw std::runtime_error("Units exceed aggregate retained element budget");
        elements -= n;
    }
    void blob(std::uint64_t n) {
        if (n > bytes)
            throw std::runtime_error("Units exceed aggregate retained byte budget");
        bytes -= n;
    }
    void tail(const UnitTailNode &n) {
        element();
        element(n.values.size());
        blob(n.bytes.size());
        for (const auto &c : n.children)
            tail(c);
    }
};
} // namespace
UnitsObservation observe_units(runtime::BinaryReader &in, GameStateLayout layout, UnitsBudget budget) {
    UnitsObservation p{};
    p.layout = layout;
    p.start_offset = in.position();
    p.header = observe_unit_collection_header(in, layout, budget.per_block);
    const auto count =
        p.header.wire_unit_count > 0 ? static_cast<std::uint64_t>(p.header.wire_unit_count) : 0;
    RetainedBudget remaining{budget.retained_elements, budget.retained_bytes};
    remaining.element();
    remaining.element(p.header.fields.size());
    remaining.element(p.header.next_free_indices.size());
    // Admit records individually; never reserve from the untrusted collection count.
    for (std::uint64_t i = 0; i < count; ++i) {
        const char *stage = "core";
        try {
            remaining.element();
            SavedUnitObservation u{};
            u.index = i;
            u.core = observe_unit_core(in, layout, budget.per_block);
            remaining.element();
            for (const auto &f : u.core.fields) {
                remaining.element();
                remaining.element(f.values.size());
                remaining.blob(f.bytes.size());
            }
            stage = "tail";
            u.tail = observe_unit_tail(in, layout, UnitTailContext{u.core.version}, budget.per_block);
            remaining.element();
            for (const auto &b : u.tail.blocks)
                remaining.tail(b);
            p.units.push_back(std::move(u));
        } catch (const std::exception &e) {
            throw std::runtime_error("Unit " + std::to_string(i) + " " + stage + ": " + e.what());
        }
    }
    p.end_offset = in.position();
    p.retained_elements = budget.retained_elements - remaining.elements;
    p.retained_bytes = budget.retained_bytes - remaining.bytes;
    return p;
}
} // namespace civ5::saves
