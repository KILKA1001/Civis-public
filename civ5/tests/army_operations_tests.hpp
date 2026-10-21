#pragma once
#include "army_operations_fixture.hpp"
void army_operations_tests() {
    static_assert(sizeof(void *) == 8);
    constexpr auto vp = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    const auto observe = [](const std::string &bytes, auto layout, saves::ArmyOperationsContext context,
                            saves::ArmyOperationsBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_army_operations(in, layout, context, budget);
    };
    const auto patch = [](std::string b, std::uint64_t offset, std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i)
            b.at(offset + i) = char(n >> (8 * i));
        return b;
    };
    const auto child = [](const auto &p, std::string_view name) -> const saves::ArmyOperationsNode & {
        for (const auto &c : p.children)
            if (c.name == name)
                return c;
        throw std::runtime_error("Missing army/operation field");
    };
    const auto header = [](bool is_vp, std::int32_t count) {
        std::string b;
        if (!is_vp)
            for (auto n :
                 {3u, 1u, UINT32_MAX, 0u, 0x7fffffffu, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX})
                u32(b, n);
        u32(b, static_cast<std::uint32_t>(count));
        return b;
    };
    for (auto layout : {bnw, vp}) {
        const bool modded = layout == vp;
        const saves::ArmyOperationsContext ctx{modded ? std::optional<std::uint64_t>{} : 16};
        const auto &bytes = modded ? army_operations_fixture_vp : army_operations_fixture_bnw;
        const auto p = observe(bytes, layout, ctx);
        check(p.start_offset == 0 && p.end_offset == bytes.size() && p.armies.size() == 2 &&
                  p.operations.size() == (modded ? 13 : 20),
              "Complete army and operation collection");
        check(p.header.wire_army_count == 2 && p.wire_operation_count == p.operations.size(),
              "Raw collection counts");
        if (!modded)
            check(p.header.source_slots == 4 && p.header.next_free_indices.size() == 4 &&
                      p.header.fields.back().value == 2147483647,
                  "Army freelist metadata");
        auto pos = p.header.end_offset;
        for (const auto &army : p.armies) {
            check(army.start_offset == pos && army.end_offset > pos, "Army cursor continuity");
            pos = army.end_offset;
            check(child(army, "m_iID").values[0] == 70000, "Army ID was masked or deduplicated");
            const auto &slots = child(army, "m_FormationEntries");
            check(slots.wire_count == 2 && slots.children.size() == 2, "Formation record count");
            if (!modded)
                check(child(slots.children[0], "m_bStartedOnOperation").values[0] == INT32_MIN &&
                          slots.children[0].end_offset - slots.children[0].start_offset == 12,
                      "BNW int named m_bStartedOnOperation was narrowed to bool");
        }
        check(pos == p.armies_end_offset, "Armies did not end at operation count");
        pos += 4;
        for (std::uint64_t i = 0; i < p.operations.size(); ++i) {
            const auto &op = p.operations[i];
            check(op.start_offset == pos && op.end_offset > pos, "Operation cursor continuity");
            pos = op.end_offset;
            check(child(op, "operation_type").values[0] == static_cast<std::int64_t>(i),
                  "Operation dispatch type");
            const auto &base = child(op, "CvAIOperation");
            check(child(base, "m_iID").values[0] == 70000, "Duplicate operation IDs removed");
            check(child(base, "m_viArmyIDs").values == std::vector<std::int64_t>({70000, -1}),
                  "Operation army references");
            check(child(base, "m_viListOfUnitsWeStillNeedToBuild").values ==
                      std::vector<std::int64_t>({70000, 80000, -1, 70000, 80000, 2147483647}),
                  "Operation slot triples/order");
            if (!modded)
                check(child(op, "wire_id").values[0] == -90000 - static_cast<std::int64_t>(i),
                      "Discarded BNW outer operation ID not preserved");
        }
        check(pos == p.operations_end_offset && pos == p.end_offset, "Operation collection endpoint");
        for (std::size_t i = 0; i < bytes.size(); ++i)
            rejects([&] { observe(bytes.substr(0, i), layout, ctx); });
        rejects([&] { observe(patch(bytes, p.armies_end_offset, UINT32_MAX), layout, ctx); });
        rejects([&] {
            observe(patch(bytes, child(p.operations[1], "operation_type").start_offset, 9999), layout, ctx);
        });
        rejects([&] {
            observe(patch(bytes, child(p.operations[1], "operation_type").start_offset, UINT32_MAX), layout,
                    ctx);
        });
        saves::ArmyOperationsBudget exact{p.retained_elements};
        check(observe(bytes, layout, ctx, exact).retained_elements == p.retained_elements,
              "Exact army element budget");
        --exact.elements;
        rejects([&] { observe(bytes, layout, ctx, exact); });
        check(observe(bytes, layout, ctx, {UINT64_MAX}).retained_elements == p.retained_elements,
              "Army budget overflow");
        std::istringstream stream(bytes + "NEXT");
        runtime::BinaryReader in(stream, bytes.size() + 4);
        saves::observe_army_operations(in, layout, ctx);
        check(in.u32() == 0x5458454e, "Army operations consumed player history bytes");
        // Source operation order and repeated types are retained, without map insertion semantics.
        std::string shuffled = bytes.substr(0, p.armies_end_offset);
        u32(shuffled, 3);
        for (auto index : {p.operations.size() - 1, std::size_t(0), p.operations.size() - 1}) {
            const auto &op = p.operations[index];
            shuffled += bytes.substr(op.start_offset, op.end_offset - op.start_offset);
        }
        const auto q = observe(shuffled, layout, ctx);
        check(q.operations.size() == 3 &&
                  child(q.operations[0], "operation_type").values[0] == (modded ? 12 : 19) &&
                  child(q.operations[1], "operation_type").values[0] == 0 &&
                  child(q.operations[2], "operation_type").values[0] == (modded ? 12 : 19),
              "Operation order or duplicates changed");
        for (auto count : {0, -7}) {
            auto empty = header(modded, count);
            u32(empty, 0);
            const auto zero = observe(empty, layout, ctx);
            check(zero.header.wire_army_count == count && zero.armies.empty() && zero.operations.empty() &&
                      zero.end_offset == empty.size(),
                  "Empty/negative army collection");
        }
        std::optional<saves::ArmyOperationsObservation> atomic;
        try {
            atomic = observe(patch(bytes, child(p.operations[1], "operation_type").start_offset, 9999),
                             layout, ctx);
        } catch (const std::exception &e) {
            check(std::string(e.what()).find("Operation 1: Unverified operation type 9999") !=
                      std::string::npos,
                  "Operation error missing index/type");
        }
        check(!atomic, "Partial army/operation result exposed");
    }
    const auto v = observe(army_operations_fixture_vp, vp, {});
    const auto b = observe(army_operations_fixture_bnw, bnw, {16});
    const auto &slots = child(v.armies[0], "m_FormationEntries");
    check(child(slots.children[0], "m_estTurnsToCheckpoint").values == std::vector<std::int64_t>({70000, -1}),
          "VP checkpoint history not retained");
    auto bad = army_operations_fixture_vp;
    bad.at(child(slots.children[0], "m_bIsRequired").start_offset) = char(2);
    rejects([&] { observe(bad, vp, {}); });
    const auto &boolean =
        child(child(b.operations[0], "CvAIOperation"), "m_bShouldReplaceLossesWithReinforcements");
    bad = army_operations_fixture_bnw;
    bad.at(boolean.start_offset) = char(255);
    rejects([&] { observe(bad, bnw, {16}); });
    rejects([&] { observe(patch(army_operations_fixture_bnw, b.armies[0].start_offset, 2), bnw, {16}); });
    for (const auto &op : b.operations)
        for (const auto &layer : op.children) {
            if (layer.children.empty())
                continue;
            const auto &version = child(layer, "version");
            rejects([&] { observe(patch(army_operations_fixture_bnw, version.start_offset, 2), bnw, {16}); });
        }
    for (std::uint64_t version = 1; version <= 16; ++version) {
        const auto &bytes = version <= 10 ? army_operations_fixture_bnw_old : army_operations_fixture_bnw;
        const auto p = observe(bytes, bnw, {version});
        check(p.end_offset == bytes.size() &&
                  p.bnw_unused_popup_count ==
                      (version <= 10 ? std::optional<std::int64_t>(-11) : std::optional<std::int64_t>{}),
              "BNW obsolete popup branch");
    }
    rejects([&] { observe(army_operations_fixture_vp, vp, {16}); });
    rejects([&] { observe(army_operations_fixture_bnw, bnw, {}); });
    rejects([&] { observe(army_operations_fixture_bnw, bnw, {0}); });
    rejects([&] { observe(army_operations_fixture_bnw, bnw, {17}); });
    const auto sentinel = observe(army_operations_fixture_vp_sentinel, vp, {});
    check(child(sentinel.armies[0], "m_FormationEntries").wire_count == UINT32_MAX &&
              child(sentinel.armies[0], "m_FormationEntries").children.empty(),
          "VP formation sentinel");
    check(child(child(sentinel.operations[0], "CvAIOperation"), "m_viArmyIDs").wire_count == UINT32_MAX &&
              child(child(sentinel.operations[0], "CvAIOperation"), "m_viArmyIDs").values.empty(),
          "VP vector sentinel");
    const auto &bnw_entries = child(b.armies[0], "m_FormationEntries");
    std::string negative = army_operations_fixture_bnw.substr(0, bnw_entries.start_offset);
    u32(negative, 0xfffffff7);
    negative += army_operations_fixture_bnw.substr(bnw_entries.end_offset);
    const auto n = observe(negative, bnw, {16});
    check(child(n.armies[0], "m_FormationEntries").wire_count == -9 &&
              child(n.armies[0], "m_FormationEntries").children.empty(),
          "Negative BNW formation count");
    // No 20-slot estimate or original freelist capacity is imposed by Civis.
    std::string many = army_operations_fixture_bnw.substr(0, bnw_entries.start_offset);
    u32(many, 100);
    const auto &one = bnw_entries.children[0];
    for (unsigned i = 0; i < 100; ++i)
        many += army_operations_fixture_bnw.substr(one.start_offset, one.end_offset - one.start_offset);
    many += army_operations_fixture_bnw.substr(bnw_entries.end_offset);
    check(child(observe(many, bnw, {16}).armies[0], "m_FormationEntries").children.size() == 100,
          "Source formation estimate became capacity");
    std::string wide;
    for (auto value : {32768u, 0u, UINT32_MAX, 0u, 0x7fffffffu})
        u32(wide, value);
    for (unsigned i = 0; i < 32768; ++i)
        u32(wide, UINT32_MAX);
    u32(wide, 0);
    u32(wide, 0);
    check(observe(wide, bnw, {16}).header.source_slots == 32768, "Source army freelist cap leaked into x64");
    rejects([&] { observe(patch(wide, 0, UINT32_MAX), bnw, {16}); });
    rejects([&] { observe(patch(wide, 0, 0x7fffffff), bnw, {16}); });
    class ShiftedArmyStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t pos_ = 0;
        pos_type seekpos(pos_type p, std::ios_base::openmode) override {
            pos_ = static_cast<std::uint64_t>(off_type(p));
            return p;
        }
        std::streamsize xsgetn(char *out, std::streamsize n) override {
            const auto local = pos_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(n) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, n, out);
            pos_ += n;
            return n;
        }

      public:
        explicit ShiftedArmyStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(army_operations_fixture_vp);
    std::istream stream(&backing);
    runtime::BinaryReader input(stream, 0x100000123ull + army_operations_fixture_vp.size());
    input.seek(0x100000123ull);
    const auto high = saves::observe_army_operations(input, vp);
    check(high.start_offset == 0x100000123ull &&
              high.end_offset == 0x100000123ull + army_operations_fixture_vp.size() &&
              high.operations.back().start_offset > UINT32_MAX,
          "Army/operation offsets narrowed at 4GB");
}
