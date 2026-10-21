#pragma once
#include "unit_core_fixture.hpp"
void unit_core_tests() {
    static_assert(sizeof(void *) == 8);
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    constexpr auto vp = saves::GameStateLayout::vp542_corpus;
    const auto observe = [&](const std::string &bytes, auto layout, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_unit_core(in, layout, budget);
    };
    const auto field = [](const auto &p, std::string_view name) -> const saves::UnitCoreField & {
        for (const auto &f : p.fields)
            if (f.name == name)
                return f;
        throw std::runtime_error("Missing unit field");
    };
    const auto overwrite = [](std::string b, std::uint64_t at, std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i)
            b.at(at + i) = char(n >> (8 * i));
        return b;
    };
    for (auto layout : {bnw, vp}) {
        const auto &bytes = layout == vp ? unit_core_fixture_vp : unit_core_fixture_bnw;
        const auto p = observe(bytes, layout);
        check(p.start_offset == 0 && p.end_offset == bytes.size(), "Unit prefix boundary");
        check(p.fields.size() == (layout == vp ? 360 : 149), "Unit field schema size");
        check(field(p, "m_iID").values == std::vector<std::int64_t>{70000}, "Unit ID was masked");
        check(field(p, "m_strScriptData").bytes == std::vector<std::uint8_t>({0, 255, 65, 128}),
              "Opaque unit string changed");
        std::uint64_t pos = layout == vp ? 0 : 4;
        for (const auto &f : p.fields) {
            check(f.start_offset == pos && f.end_offset > pos, "Unit field cursor continuity");
            pos = f.end_offset;
        }
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), layout); });
        for (const auto &f : p.fields) {
            if (f.encoding == saves::UnitCoreEncoding::boolean) {
                auto bad = bytes;
                bad.at(f.start_offset) = char(2);
                try {
                    observe(bad, layout);
                    check(false, "Invalid unit bool accepted");
                } catch (const std::exception &e) {
                    check(std::string(e.what()).find(f.name) != std::string::npos,
                          "Unit field diagnostic missing");
                }
                break;
            }
        }
        const auto &v = field(p, layout == vp ? "m_LinkedUnitIDs" : "m_terrainDoubleMoveCount");
        rejects([&] { observe(overwrite(bytes, v.start_offset, 0x7fffffff), layout); });
        const auto &s = field(p, "m_strScriptData");
        rejects([&] { observe(overwrite(bytes, s.start_offset, UINT32_MAX), layout); });
        saves::GameStateBudget budget;
        budget.elements = 1;
        rejects([&] { observe(bytes, layout, budget); });
        budget = {};
        budget.string_bytes = 3;
        rejects([&] { observe(bytes, layout, budget); });
        budget = {};
        budget.total_string_bytes = layout == vp ? 3 : 7;
        rejects([&] { observe(bytes, layout, budget); });
        budget = {};
        budget.string_bytes = 4;
        budget.total_string_bytes = layout == vp ? 4 : 8;
        check(observe(bytes, layout, budget).end_offset == bytes.size(), "Exact unit string budget");
        std::istringstream stream(bytes + "NEXT");
        runtime::BinaryReader in(stream, bytes.size() + 4);
        saves::observe_unit_core(in, layout);
        check(in.u32() == 0x5458454e, "Unit reader consumed promotions bytes");
    }
    const auto b = observe(unit_core_fixture_bnw, bnw), v = observe(unit_core_fixture_vp, vp);
    check(field(b, "idUnitType").values[0] == 0xf1234567ll && field(b, "m_eUnitType").values[0] == -198,
          "Unit hash resolved over saved type index");
    check(b.fields[0].name == std::string_view("m_eOwner") && b.fields[5].name == std::string_view("m_iID"),
          "BNW used initializer order instead of registration order");
    for (std::uint32_t version = 0; version <= 9; ++version)
        check(observe(overwrite(unit_core_fixture_bnw, 0, version), bnw).version == version,
              "BNW unit version");
    rejects([&] { observe(overwrite(unit_core_fixture_bnw, 0, 10), bnw); });
    rejects([&] {
        observe(
            overwrite(unit_core_fixture_bnw, field(b, "m_terrainDoubleMoveCount").start_offset, UINT32_MAX),
            bnw);
    });
    check(field(v, "m_vsPlaguesToInflict").values ==
              std::vector<std::int64_t>({70000, -1, 1, 0, 37, 70000, 3, 0, 1, -19}),
          "Mixed-width plague records changed");
    auto bad = unit_core_fixture_vp;
    bad.at(field(v, "m_vsPlaguesToInflict").start_offset + 12) = char(2);
    rejects([&] { observe(bad, vp); });
    check(field(v, "m_yieldFromPillage").values ==
              std::vector<std::int64_t>({7, -1, 2147483647, 7, -200, 90000}),
          "Unit map duplicate keys or tuple ordering changed");
    const auto sentinel = observe(unit_core_fixture_vp_sentinel, vp);
    check(field(sentinel, "m_LinkedUnitIDs").wire_count == UINT32_MAX &&
              field(sentinel, "m_LinkedUnitIDs").values.empty(),
          "VP vector sentinel");
    check(field(sentinel, "m_vsPlaguesToInflict").wire_count == UINT32_MAX &&
              field(sentinel, "m_vsPlaguesToInflict").values.empty(),
          "VP plague sentinel");
    const auto header = [&](const std::string &bytes, auto layout, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_unit_collection_header(in, layout, budget);
    };
    for (std::int32_t n : {0, -7, 50}) {
        std::string bytes;
        u32(bytes, static_cast<std::uint32_t>(n));
        check(header(bytes, vp).wire_unit_count == n, "Signed VP unit collection count");
        rejects([&] { header(bytes.substr(0, 3), vp); });
    }
    std::string bytes;
    for (auto n : {3u, 1u, UINT32_MAX, 0u, 0x7fffffffu, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, 2u})
        u32(bytes, n);
    const auto h = header(bytes, bnw);
    check(h.source_slots == 4 && h.next_free_indices.size() == 4 && h.wire_unit_count == 2 &&
              h.end_offset == 40,
          "BNW rounded unit freelist metadata");
    for (std::size_t n = 0; n < bytes.size(); ++n)
        rejects([&] { header(bytes.substr(0, n), bnw); });
    rejects([&] { header(overwrite(bytes, 0, UINT32_MAX), bnw); });
    rejects([&] { header(overwrite(bytes, 0, 0x7fffffff), bnw); });
    check(header(overwrite(bytes, 36, 0xffffffff), bnw).wire_unit_count == -1, "BNW signed empty unit count");
    // Capacity metadata belongs to the source format, not the native entity store.
    std::string wide;
    for (auto n : {32768u, 0u, UINT32_MAX, 0u, 0u})
        u32(wide, n);
    for (unsigned i = 0; i < 32768; ++i)
        u32(wide, UINT32_MAX);
    u32(wide, 0);
    check(header(wide, bnw).source_slots == 32768, "Original 13-bit freelist cap leaked into x64");
    class ShiftedUnitStream final : public std::streambuf {
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
        explicit ShiftedUnitStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(unit_core_fixture_vp);
    std::istream shifted(&backing);
    runtime::BinaryReader input(shifted, 0x100000123ull + unit_core_fixture_vp.size());
    input.seek(0x100000123ull);
    const auto high = saves::observe_unit_core(input, vp);
    check(high.start_offset == 0x100000123ull &&
              high.end_offset == 0x100000123ull + unit_core_fixture_vp.size() &&
              high.fields.back().start_offset > UINT32_MAX,
          "Unit offsets narrowed at 4GB");
}
