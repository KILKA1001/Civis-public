#pragma once
void city_core_tests() {
    using E = saves::CityCoreEncoding;
    constexpr auto vp_layout = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw_layout = saves::GameStateLayout::expansion2_version1;
    struct FieldSpec {
        const char *name;
        E encoding;
        unsigned minimum_version;
    };
    // Shared field list exercises codecs and bounds here. Field order/types are
    // checked separately against source by the independent Python verifier.
#include "../src/saves/city_core_fields.inc"
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, counts, strings;
    };
    const auto fixture = [&](bool vp, unsigned version = 6, bool empty = false) {
        Fixture f;
        auto &s = f.bytes;
        const auto integer = [&](std::uint32_t n) { u32(s, n); };
        const auto values = [&](unsigned n) {
            for (unsigned i = 0; i < n; ++i)
                integer(i % 3 == 0 ? 70000 : i % 3 == 1 ? UINT32_MAX : 0x80000000);
        };
        const auto flag = [&](unsigned n) {
            f.flags.push_back(s.size());
            s += char(n);
        };
        const auto count = [&](unsigned n) {
            f.counts.push_back(s.size());
            integer(empty ? (vp ? UINT32_MAX : 0) : n);
        };
        if (!vp)
            integer(version);
        const auto specs =
            vp ? std::span<const FieldSpec>(vp_fields) : std::span<const FieldSpec>(bnw_fields);
        for (const auto &spec : specs) {
            if (!vp && spec.minimum_version > version)
                continue;
            switch (spec.encoding) {
            case E::i32:
                integer(0x80000000);
                break;
            case E::hash32:
                integer(0xdeadbeef);
                break;
            case E::boolean:
                flag(1);
                break;
            case E::fraction:
                integer(0xfffffffe);
                integer(0);
                break;
            case E::string32:
                f.strings.push_back(s.size());
                integer(3);
                s.append("\xff\0a", 3);
                break;
            case E::hashed_i32:
                f.counts.push_back(s.size());
                integer(empty ? 0 : 4);
                if (!empty) {
                    integer(0);
                    integer(0xdeadbeef);
                    integer(UINT32_MAX);
                    integer(0);
                    integer(UINT32_MAX);
                    integer(70000);
                }
                break;
            case E::nested_map_i_iii:
                count(2);
                if (!empty)
                    for (unsigned i = 0; i < 2; ++i) {
                        integer(UINT32_MAX);
                        count(2);
                        values(6);
                    }
                break;
            case E::vector_bool:
                count(3);
                if (!empty) {
                    flag(1);
                    flag(0);
                    flag(1);
                }
                break;
            case E::vector_fraction:
                count(2);
                if (!empty) {
                    integer(2);
                    integer(4);
                    integer(0);
                    integer(0);
                }
                break;
            case E::map_ii:
                count(2);
                if (!empty) {
                    integer(70000);
                    integer(UINT32_MAX);
                    integer(70000);
                    integer(0x80000000);
                }
                break;
            case E::set_i32:
                count(3);
                if (!empty) {
                    integer(7);
                    integer(7);
                    integer(UINT32_MAX);
                }
                break;
            default:
                count(3);
                if (!empty)
                    values(3);
                break;
            }
        }
        return f;
    };
    const auto observe = [&](const std::string &s, bool vp, saves::GameStateBudget budget = {}) {
        std::istringstream stream(s);
        runtime::BinaryReader in(stream, s.size());
        return saves::observe_city_core(in, vp ? vp_layout : bnw_layout, budget);
    };
    const auto header = [&](const std::string &s, bool vp, saves::GameStateBudget budget = {}) {
        std::istringstream stream(s);
        runtime::BinaryReader in(stream, s.size());
        return saves::observe_city_collection_header(in, vp ? vp_layout : bnw_layout, budget);
    };
    const auto overwrite = [](std::string s, std::size_t off, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            s.at(off + i) = char(value >> (8 * i));
        return s;
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto p = observe(f.bytes + "BUILDINGS", vp);
        check(p.end_offset == f.bytes.size() && p.fields.size() == (vp ? 350 : 107),
              "City core consumed building data");
        for (std::size_t i = 0; i < p.fields.size(); ++i) {
            const auto &v = p.fields[i];
            check(v.start_offset == (i ? p.fields[i - 1].end_offset : vp ? 0 : 4), "City core field gap");
            switch (v.encoding) {
            case E::i32:
                check(v.values == std::vector<std::int64_t>{INT32_MIN}, "City signed scalar width");
                break;
            case E::hash32:
                check(v.values == std::vector<std::int64_t>{0xdeadbeef}, "Resource hash sign changed");
                break;
            case E::boolean:
                check(v.values == std::vector<std::int64_t>{1}, "City flag lost");
                break;
            case E::fraction:
                check(v.values == std::vector<std::int64_t>{-2, 0}, "Fraction normalized or rejected");
                break;
            case E::vector_fraction:
                check(v.values == std::vector<std::int64_t>{2, 4, 0, 0}, "Fraction vector normalized");
                break;
            case E::map_ii:
                check(v.values == std::vector<std::int64_t>{70000, -1, 70000, INT32_MIN},
                      "Map duplicate order lost");
                break;
            case E::set_i32:
                check(v.values == std::vector<std::int64_t>{7, 7, -1}, "Set source order/doubles changed");
                break;
            case E::nested_map_i_iii:
                check(v.row_keys == std::vector<std::int64_t>{-1, -1} &&
                          v.row_counts == std::vector<std::uint64_t>{2, 2} &&
                          v.values == std::vector<std::int64_t>{70000, -1, INT32_MIN, 70000, -1, INT32_MIN,
                                                                70000, -1, INT32_MIN, 70000, -1, INT32_MIN},
                      "Nested great-person yield keys/rows changed");
                break;
            case E::hashed_i32:
                check(v.hashes == std::vector<std::uint64_t>{0, 0xdeadbeef, 0, UINT32_MAX} &&
                          v.values == std::vector<std::int64_t>{-1, 70000},
                      "BNW null/unknown resource hash handling");
                break;
            case E::string32:
                check(v.bytes == std::vector<std::uint8_t>{255, 0, 97},
                      "City raw name/script string changed");
                break;
            case E::vector_bool:
                check(v.values == std::vector<std::int64_t>{1, 0, 1}, "City boolean vector");
                break;
            default:
                check(v.values == std::vector<std::int64_t>{70000, -1, INT32_MIN}, "City integer vector");
                break;
            }
        }
        const auto e = fixture(vp, 6, true);
        const auto q = observe(e.bytes, vp);
        check(q.end_offset == e.bytes.size(), "City empty container boundary");
        for (const auto &v : q.fields)
            if (v.wire_count && v.encoding != E::string32) {
                check(v.values.empty() && v.row_counts.empty() && v.hashes.empty(),
                      "Empty city container populated");
                check(*v.wire_count == (vp ? UINT32_MAX : 0), "City raw STL sentinel lost");
            }
        for (std::size_t cut = 0; cut < f.bytes.size(); ++cut)
            rejects([&] { observe(f.bytes.substr(0, cut), vp); });
        for (auto off : f.counts)
            rejects([&] { observe(overwrite(f.bytes, off, 0x7fffffff), vp); });
        for (auto off : f.strings)
            rejects([&] { observe(overwrite(f.bytes, off, UINT32_MAX), vp); });
        for (auto off : f.flags) {
            auto s = f.bytes;
            s[off] = 2;
            rejects([&] { observe(s, vp); });
        }
        for (unsigned kind = 0; kind < 3; ++kind) {
            saves::GameStateBudget b;
            if (kind == 0)
                b.elements = 10;
            if (kind == 1)
                b.string_bytes = 2;
            if (kind == 2)
                b.total_string_bytes = 5;
            rejects([&] { observe(f.bytes, vp, b); });
        }
    }
    for (unsigned version = 0; version <= 6; ++version) {
        const auto f = fixture(false, version);
        const auto p = observe(f.bytes, false);
        check(p.version == version && p.end_offset == f.bytes.size() &&
                  p.fields.size() == (version < 2   ? 104
                                      : version < 4 ? 106
                                                    : 107),
              "BNW city version defaults/order changed");
    }
    rejects([&] { observe(overwrite(fixture(false).bytes, 0, 7), false); });
    for (std::uint32_t count : {0u, 7u, 0xfffffff9u}) {
        std::string s;
        u32(s, count);
        const auto h = header(s + "CITY", true);
        check(h.end_offset == 4 && h.fields.empty() && h.next_free_indices.empty() &&
                  h.wire_city_count == (count == 0xfffffff9u ? -7 : std::int64_t(count)),
              "VP city list header");
    }
    const auto bnw_header = [](std::uint32_t slots, std::uint32_t actual, std::uint32_t count) {
        std::string s;
        u32(s, slots);
        u32(s, 0xffffffff);
        u32(s, 0x80000000);
        u32(s, 70000);
        u32(s, 0x7fffffff);
        for (std::uint32_t i = 0; i < actual; ++i)
            u32(s, i % 2 ? 0xffffffff : 70000);
        u32(s, count);
        return s;
    };
    for (auto slots : {0u, 3u, 8u, 16384u}) {
        const auto actual = slots == 3 ? 4 : slots;
        const auto s = bnw_header(slots, actual, 2);
        const auto h = header(s + "CITY", false);
        check(h.end_offset == s.size() && h.source_slots == actual && h.wire_city_count == 2 &&
                  h.next_free_indices.size() == actual && h.fields[0].value == slots &&
                  h.fields[4].value == INT32_MAX,
              "BNW free-list source metadata or former 8192 limit");
    }
    const auto s = bnw_header(3, 4, 0xfffffff9);
    check(header(s, false).wire_city_count == -7, "BNW negative city count normalized");
    for (std::size_t cut = 0; cut < s.size(); ++cut)
        rejects([&] { header(s.substr(0, cut), false); });
    rejects([&] { header(overwrite(s, 0, UINT32_MAX), false); });
    rejects([&] { header(overwrite(s, 0, 0x7fffffff), false); });
    rejects([&] { header(overwrite(s, s.size() - 4, 0x7fffffff), false); });
    const auto bytes = fixture(true).bytes;
    class ShiftedCityStream final : public std::streambuf {
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
        explicit ShiftedCityStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream shifted_stream(&backing);
    runtime::BinaryReader in(shifted_stream, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_city_core(in, vp_layout);
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + bytes.size() &&
              shifted.fields[2].start_offset > UINT32_MAX,
          "City stream offsets narrowed");
}
