#include "civ5/saves/army_operations.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace civ5::saves {
namespace {
using Node = ArmyOperationsNode;
enum class E { i32, version, boolean, stl_i32, stl_slots };
struct FieldSpec {
    const char *name;
    E encoding;
};
struct LayerSpec {
    const char *name;
    std::span<const FieldSpec> fields;
};
struct OperationSpec {
    std::int64_t type;
    const char *name;
    std::span<const LayerSpec> layers;
};
#include "army_operations_fields.inc"
struct Reader {
    runtime::BinaryReader &in;
    ArmyOperationsBudget budget;
    bool vp;
    void ensure(std::uint64_t n, std::uint64_t width = 0) const {
        if (n > budget.elements || (width && n > in.remaining() / width))
            throw std::runtime_error("Armies/operations exceed input or retained element budget");
    }
    void claim(std::uint64_t n = 1, std::uint64_t width = 0) {
        ensure(n, width);
        budget.elements -= n;
    }
    Node node(const char *name, const char *encoding = "record") {
        claim();
        return {name, encoding, in.position(), 0, {}, {}, {}};
    }
    void finish(Node &parent, Node child) {
        child.end_offset = in.position();
        parent.children.push_back(std::move(child));
    }
    std::int64_t scalar(Node &parent, const char *name, bool unsigned_value = false) {
        auto n = node(name, unsigned_value ? "u32" : "i32");
        claim(1, 4);
        const std::int64_t value = unsigned_value ? static_cast<std::int64_t>(in.u32()) : in.i32();
        n.values.push_back(value);
        finish(parent, std::move(n));
        return value;
    }
    std::uint64_t count(Node &n, bool signed_count = false, bool stl = false) {
        claim(1, 4);
        n.wire_count = signed_count ? static_cast<std::int64_t>(in.i32()) : in.u32();
        const auto value = *n.wire_count;
        return value <= 0 || (vp && stl && value == UINT32_MAX) ? 0 : static_cast<std::uint64_t>(value);
    }
    void fields(Node &parent, std::span<const FieldSpec> specs) {
        for (const auto &f : specs) {
            try {
                const auto enc = f.encoding;
                auto n = node(f.name, enc == E::i32       ? "i32"
                                      : enc == E::version ? "u32"
                                      : enc == E::boolean ? "boolean"
                                      : enc == E::stl_i32 ? "stl_i32"
                                                          : "stl_slots");
                if (enc == E::i32 || enc == E::version || enc == E::boolean) {
                    claim(1, enc == E::boolean ? 1 : 4);
                    const std::int64_t value = enc == E::i32       ? static_cast<std::int64_t>(in.i32())
                                               : enc == E::version ? static_cast<std::int64_t>(in.u32())
                                                                   : in.u8();
                    if (enc == E::version && value > 1)
                        throw std::runtime_error("Unverified BNW army/operation version");
                    if (enc == E::boolean && value > 1)
                        throw std::runtime_error("Unverified army/operation boolean");
                    n.values.push_back(value);
                } else {
                    const auto rows = count(n, false, true);
                    const std::uint64_t columns = enc == E::stl_slots ? 3 : 1;
                    if (rows > budget.elements / columns)
                        throw std::runtime_error("Operation vector exceeds element budget");
                    const auto total = rows * columns;
                    claim(total, 4);
                    for (std::uint64_t i = 0; i < total; ++i)
                        n.values.push_back(in.i32());
                }
                finish(parent, std::move(n));
            } catch (const std::exception &e) {
                throw std::runtime_error(std::string(parent.name) + "." + f.name + ": " + e.what());
            }
        }
    }
    ArmyCollectionHeader header() {
        claim();
        ArmyCollectionHeader h{};
        h.start_offset = in.position();
        if (!vp) {
            const auto field = [&](const char *name) {
                claim(1, 4);
                const auto n = in.i32();
                h.fields.push_back({name, n});
                return n;
            };
            const auto slots = field("num_slots");
            if (slots < 0)
                throw std::runtime_error("Negative BNW army source slot count");
            h.source_slots = slots ? std::bit_ceil(static_cast<std::uint64_t>(slots)) : 0;
            field("last_index");
            field("free_list_head");
            field("free_list_count");
            field("current_id");
            claim(h.source_slots, 4);
            for (std::uint64_t i = 0; i < h.source_slots; ++i)
                h.next_free_indices.push_back(in.i32());
        }
        claim(1, 4);
        h.wire_army_count = in.i32();
        h.end_offset = in.position();
        return h;
    }
    Node army() {
        auto n = node("army");
        fields(n, vp ? std::span<const FieldSpec>(vp_army) : std::span<const FieldSpec>(bnw_army));
        auto entries = node("m_FormationEntries", vp ? "stl_records" : "signed_records");
        const auto size = count(entries, !vp, vp);
        // BNW's m_bStartedOnOperation is an int despite its name: 12-byte rows.
        // VP rows have ID + deque count/data + bool, with a 9-byte minimum.
        ensure(size, vp ? 9 : 12);
        for (std::uint64_t i = 0; i < size; ++i) {
            auto slot = node("slot");
            fields(slot, vp ? std::span<const FieldSpec>(vp_slot) : std::span<const FieldSpec>(bnw_slot));
            finish(entries, std::move(slot));
        }
        finish(n, std::move(entries));
        n.end_offset = in.position();
        return n;
    }
    Node operation() {
        auto n = node("operation");
        if (!vp)
            scalar(n, "wire_id");
        const auto type = scalar(n, "operation_type");
        const auto table = vp ? std::span<const OperationSpec>(vp_operations)
                              : std::span<const OperationSpec>(bnw_operations);
        const auto found =
            std::find_if(table.begin(), table.end(), [&](const auto &s) { return s.type == type; });
        if (found == table.end())
            throw std::runtime_error("Unverified operation type " + std::to_string(type));
        for (const auto &spec : found->layers) {
            auto layer = node(spec.name);
            fields(layer, spec.fields);
            finish(n, std::move(layer));
        }
        n.end_offset = in.position();
        return n;
    }
};
} // namespace
ArmyOperationsObservation observe_army_operations(runtime::BinaryReader &in, GameStateLayout layout,
                                                  ArmyOperationsContext context,
                                                  ArmyOperationsBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    if (vp ? bool(context.bnw_player_version)
           : !context.bnw_player_version || !*context.bnw_player_version || *context.bnw_player_version > 16)
        throw std::runtime_error("Armies/operations context does not match layout or BNW player version");
    ArmyOperationsObservation p{};
    p.layout = layout;
    p.context = context;
    p.start_offset = in.position();
    Reader r{in, budget, vp};
    r.claim();
    p.header = r.header();
    const auto count =
        p.header.wire_army_count > 0 ? static_cast<std::uint64_t>(p.header.wire_army_count) : 0;
    r.ensure(count, vp ? 36 : 48);
    for (std::uint64_t i = 0; i < count; ++i) {
        try {
            p.armies.push_back(r.army());
        } catch (const std::exception &e) {
            throw std::runtime_error("Army " + std::to_string(i) + ": " + e.what());
        }
    }
    p.armies_end_offset = in.position();
    r.claim(1, 4);
    p.wire_operation_count = in.u32();
    // Explicit player loop, not STL: UINT32_MAX never means an empty operation collection.
    r.ensure(p.wire_operation_count, vp ? 4 : 8);
    for (std::uint64_t i = 0; i < p.wire_operation_count; ++i) {
        try {
            p.operations.push_back(r.operation());
        } catch (const std::exception &e) {
            throw std::runtime_error("Operation " + std::to_string(i) + ": " + e.what());
        }
    }
    p.operations_end_offset = in.position();
    if (!vp && *context.bnw_player_version <= 10) {
        r.claim(1, 4);
        p.bnw_unused_popup_count = in.i32();
    }
    p.end_offset = in.position();
    p.retained_elements = budget.elements - r.budget.elements;
    return p;
}
} // namespace civ5::saves
