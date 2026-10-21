#pragma once
#include "units_fixture.hpp"
void units_tests() {
    static_assert(sizeof(void *) == 8);
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    constexpr auto vp = saves::GameStateLayout::vp542_corpus;
    const auto patch = [](std::string b, std::uint64_t offset, std::uint32_t n) {
        for (unsigned j = 0; j < 4; ++j)
            b.at(offset + j) = char(n >> (8 * j));
        return b;
    };
    const auto tail = [&](const std::string &bytes, auto layout, saves::UnitTailContext context,
                          saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader input(stream, bytes.size());
        return saves::observe_unit_tail(input, layout, context, budget);
    };
    const auto units = [&](const std::string &bytes, auto layout, saves::UnitsBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader input(stream, bytes.size());
        return saves::observe_units(input, layout, budget);
    };
    const auto child = [](const auto &parent, std::string_view name) -> const saves::UnitTailNode & {
        for (const auto &n : parent.children)
            if (n.name == name)
                return n;
        throw std::runtime_error("Missing unit tail child");
    };
    const std::string *bnw_tails[] = {
        &unit_tail_fixture_bnw0, &unit_tail_fixture_bnw1, &unit_tail_fixture_bnw2, &unit_tail_fixture_bnw3,
        &unit_tail_fixture_bnw4, &unit_tail_fixture_bnw5, &unit_tail_fixture_bnw6, &unit_tail_fixture_bnw7,
        &unit_tail_fixture_bnw8, &unit_tail_fixture_bnw9};
    const auto check_tail = [&](const std::string &bytes, auto layout, saves::UnitTailContext context) {
        const auto p = tail(bytes, layout, context);
        check(p.start_offset == 0 && p.end_offset == bytes.size(), "Complete unit tail boundary");
        check(p.blocks.size() == (layout == vp ? 6 : 5), "Unit tail block count");
        std::uint64_t end = 0;
        for (const auto &b : p.blocks) {
            check(b.start_offset == end && b.end_offset >= end, "Unit tail block continuity");
            end = b.end_offset;
            auto pos = b.start_offset;
            for (const auto &n : b.children) {
                check(n.start_offset == pos && n.end_offset >= pos, "Unit tail field continuity");
                pos = n.end_offset;
            }
            if (!b.children.empty())
                check(pos == end, "Unit tail child end");
        }
        check(p.blocks[4].values ==
                  std::vector<std::int64_t>({70000, -1, 2147483647, -20, 90, 70000, 90000, -200, 0, 91}),
              "Mission rows lost order, duplicates, or unknown IDs");
        check(child(p.blocks[1], "m_combatUnit.iID").values[0] == 70000, "Unit reference ID narrowed");
        check(child(p.blocks[1], "m_strName").bytes == std::vector<std::uint8_t>({0, 255, 65, 128}),
              "Unit tail opaque string changed");
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { tail(bytes.substr(0, n), layout, context); });
        rejects([&] { tail(patch(bytes, p.blocks[4].start_offset, UINT32_MAX), layout, context); });
        const auto &vec = child(p.blocks[1], "m_extraDomainModifiers");
        rejects([&] { tail(patch(bytes, vec.start_offset, 0x7fffffff), layout, context); });
        const auto &name = child(p.blocks[1], "m_strName");
        rejects([&] { tail(patch(bytes, name.start_offset, UINT32_MAX), layout, context); });
        saves::GameStateBudget small;
        small.elements = 1;
        rejects([&] { tail(bytes, layout, context, small); });
        small = {};
        small.string_bytes = 3;
        rejects([&] { tail(bytes, layout, context, small); });
        small = {};
        small.total_string_bytes = 3;
        rejects([&] { tail(bytes, layout, context, small); });
        std::istringstream stream(bytes + "NEXT");
        runtime::BinaryReader input(stream, bytes.size() + 4);
        saves::observe_unit_tail(input, layout, context);
        check(input.u32() == 0x5458454e, "Unit tail consumed next unit or army bytes");
    };
    for (std::uint64_t v = 0; v < 10; ++v) {
        check_tail(*bnw_tails[v], bnw, {v});
        const auto p = tail(*bnw_tails[v], bnw, {v});
        check(p.context.bnw_unit_version == v, "Unit tail version context");
        check(p.blocks[2].children.size() == (v % 3 == 2 ? 4 : 3), "BNW religion nested version");
        check(p.blocks[3].children.size() == (v >= 7   ? 2
                                              : v >= 4 ? 1
                                                       : 0),
              "BNW great-work version branches");
        check(child(p.blocks[0], "m_kHasPromotion").values ==
                  std::vector<std::int64_t>({0, 1, 0xfedcba98ll, 0, 0x12345678, 1, 0x12345678, 0}),
              "Promotion zero/unknown/duplicate hash bool not retained");
        if (v >= 6)
            check(child(p.blocks[3], "uiGreatWorkType").values[0] == 0xfedcba98ll,
                  "Unsigned great-work hash narrowed");
        if (v == 4 || v == 5)
            check(child(p.blocks[3], "strGreatWorkType").bytes ==
                      std::vector<std::uint8_t>({71, 0, 255, 87, 79, 82, 75}),
                  "Old great-work string bytes");
    }
    check_tail(unit_tail_fixture_vp, vp, {});
    const auto b = tail(unit_tail_fixture_bnw9, bnw, {9});
    const auto v = tail(unit_tail_fixture_vp, vp, {});
    check(child(v.blocks[0], "m_kHasPromotion").bytes == std::vector<std::uint8_t>({129, 0, 255}) &&
              child(v.blocks[0], "m_kPromotionActive").bytes == std::vector<std::uint8_t>({1, 128}),
          "Promotion bitfields collapsed or trailing bytes removed");
    check(child(v.blocks[2], "m_iStrength").values[0] == 65535 &&
              child(v.blocks[2], "m_iMaxStrength").values[0] == 32768 &&
              child(v.blocks[2], "m_iSpreadsUsed").values[0] == 40001 &&
              v.blocks[2].end_offset - v.blocks[2].start_offset == 10,
          "VP religion unsigned 16-bit fields");
    const auto &path = child(v.blocks[5], "m_kLastPath");
    check(path.values == std::vector<std::int64_t>({-12, 2147483647, -32768, 32767, 30, -5, 20000, -20000}),
          "Path node mixed widths or sign extension changed");
    check(child(v.blocks[5], "m_uiLastPathCacheOrigin").values[0] == UINT32_MAX &&
              child(v.blocks[5], "m_uiLastPathFlags").values[0] == 0x80000000ll,
          "Path cache unsigned fields");
    rejects([&] { tail(patch(unit_tail_fixture_vp, path.start_offset, UINT32_MAX), vp, {}); });
    rejects([&] { tail(patch(unit_tail_fixture_vp, 0, UINT32_MAX), vp, {}); });
    rejects([&] { tail(unit_tail_fixture_vp, vp, {9}); });
    rejects([&] { tail(unit_tail_fixture_bnw9, bnw, {}); });
    rejects([&] { tail(unit_tail_fixture_bnw9, bnw, {10}); });
    rejects([&] { tail(patch(unit_tail_fixture_bnw9, 0, 2), bnw, {9}); });
    rejects([&] { tail(patch(unit_tail_fixture_bnw9, b.blocks[2].start_offset, 3), bnw, {9}); });
    rejects([&] {
        tail(patch(unit_tail_fixture_bnw9, child(b.blocks[1], "m_extraDomainModifiers").start_offset,
                   UINT32_MAX),
             bnw, {9});
    });
    auto bad = unit_tail_fixture_bnw9;
    bad.at(child(b.blocks[0], "m_kHasPromotion").start_offset + 8) = char(2);
    rejects([&] { tail(bad, bnw, {9}); });
    bad = unit_tail_fixture_bnw9;
    bad.at(child(b.blocks[1], "m_bIgnoreDangerWakeup").start_offset) = char(255);
    rejects([&] { tail(bad, bnw, {9}); });
    const auto negative = tail(unit_tail_fixture_bnw_negative, bnw, {9});
    check(child(negative.blocks[0], "m_kHasPromotion").wire_count == -7 &&
              child(negative.blocks[0], "m_kHasPromotion").values.empty(),
          "Signed negative promotion count");
    const auto sentinel = tail(unit_tail_fixture_vp_sentinel, vp, {});
    check(child(sentinel.blocks[1], "m_extraDomainModifiers").wire_count == UINT32_MAX &&
              child(sentinel.blocks[1], "m_extraDomainModifiers").values.empty(),
          "VP STL sentinel lost");
    // Mission count is an explicit u32, with no original fixed queue capacity.
    std::string large = unit_tail_fixture_vp.substr(0, v.blocks[4].start_offset);
    u32(large, 2000);
    for (unsigned i = 0; i < 2000; ++i)
        for (unsigned j = 0; j < 5; ++j)
            u32(large, i + j);
    large += unit_tail_fixture_vp.substr(v.blocks[4].end_offset);
    check(tail(large, vp, {}).blocks[4].values.size() == 10000, "Mission queue inherited source capacity");
    std::string bitfield;
    u32(bitfield, 8751);
    bitfield.append(8751, char(0));
    bitfield.back() = char(1);
    bitfield += unit_tail_fixture_vp.substr(child(v.blocks[0], "m_kHasPromotion").end_offset);
    check(child(tail(bitfield, vp, {}).blocks[0], "m_kHasPromotion").bytes.size() == 8751,
          "Promotion index above 65535 restricted by source capacity");
    const auto header = [](bool is_vp, std::int32_t count) {
        std::string bytes;
        if (!is_vp)
            for (auto n :
                 {3u, 1u, UINT32_MAX, 0u, 0x7fffffffu, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX})
                u32(bytes, n);
        u32(bytes, static_cast<std::uint32_t>(count));
        return bytes;
    };
    for (auto layout : {bnw, vp}) {
        for (auto count : {0, -7}) {
            const auto bytes = header(layout == vp, count);
            const auto p = units(bytes, layout);
            check(p.units.empty() && p.header.wire_unit_count == count && p.end_offset == bytes.size(),
                  "Empty or signed-negative unit collection");
        }
        const auto &core = layout == vp ? unit_core_fixture_vp : unit_core_fixture_bnw;
        const auto &suffix = layout == vp ? unit_tail_fixture_vp : unit_tail_fixture_bnw9;
        const auto bytes = header(layout == vp, 2) + core + suffix + core + suffix;
        const auto p = units(bytes, layout);
        check(p.units.size() == 2 && p.end_offset == bytes.size() && p.units[1].index == 1 &&
                  p.units[0].tail.end_offset == p.units[1].core.start_offset,
              "Complete unit collection framing");
        const auto id_field = std::find_if(p.units[1].core.fields.begin(), p.units[1].core.fields.end(),
                                           [](const auto &f) { return std::string_view(f.name) == "m_iID"; });
        check(id_field != p.units[1].core.fields.end() && id_field->values[0] == 70000,
              "Duplicate unit IDs removed or masked");
        saves::UnitsBudget exact;
        exact.retained_elements = p.retained_elements;
        exact.retained_bytes = p.retained_bytes;
        check(units(bytes, layout, exact).end_offset == bytes.size(), "Exact aggregate unit budgets");
        --exact.retained_elements;
        rejects([&] { units(bytes, layout, exact); });
        ++exact.retained_elements;
        --exact.retained_bytes;
        rejects([&] { units(bytes, layout, exact); });
        saves::UnitsBudget high;
        high.retained_elements = UINT64_MAX;
        high.retained_bytes = UINT64_MAX;
        check(units(bytes, layout, high).retained_elements == p.retained_elements,
              "64-bit unit budget overflow");
        std::optional<saves::UnitsObservation> atomic;
        try {
            atomic = units(bytes.substr(0, bytes.size() - 1), layout);
        } catch (const std::exception &e) {
            check(std::string(e.what()).find("Unit 1 tail") != std::string::npos,
                  "Failed unit index/stage absent");
        }
        check(!atomic, "Partial collection exposed as complete");
        std::istringstream stream(bytes + "NEXT");
        runtime::BinaryReader input(stream, bytes.size() + 4);
        saves::observe_units(input, layout);
        check(input.u32() == 0x5458454e, "Unit collection consumed army header");
    }
    std::string mixed = header(false, 10);
    for (unsigned version = 0; version < 10; ++version)
        mixed += patch(unit_core_fixture_bnw, 0, version) + *bnw_tails[version];
    const auto all = units(mixed, bnw);
    check(all.units.size() == 10 && all.end_offset == mixed.size(), "Mixed-version BNW unit collection");
    for (unsigned i = 0; i < 10; ++i)
        check(all.units[i].core.version == i && all.units[i].tail.context.bnw_unit_version == i,
              "BNW tail reused previous unit version");
    class ShiftedUnitsStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t position_ = 0;
        pos_type seekpos(pos_type p, std::ios_base::openmode) override {
            position_ = static_cast<std::uint64_t>(off_type(p));
            return p;
        }
        std::streamsize xsgetn(char *out, std::streamsize n) override {
            const auto local = position_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(n) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, n, out);
            position_ += n;
            return n;
        }

      public:
        explicit ShiftedUnitsStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(mixed);
    std::istream shifted(&backing);
    runtime::BinaryReader input(shifted, 0x100000123ull + mixed.size());
    input.seek(0x100000123ull);
    const auto high = saves::observe_units(input, bnw);
    check(high.start_offset == 0x100000123ull && high.end_offset == 0x100000123ull + mixed.size() &&
              high.units.back().tail.blocks.back().start_offset > UINT32_MAX,
          "Complete unit offsets narrowed at 4GB");
}
