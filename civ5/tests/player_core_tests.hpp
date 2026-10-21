#pragma once
void player_core_tests() {
    constexpr auto vp = saves::GameStateLayout::vp542_corpus,
                   bnw = saves::GameStateLayout::expansion2_version1;
    const auto fixture = [](bool is_vp, unsigned version = 16, unsigned yields = 2) {
        // Source wire signature, independent of player_core_fields.inc. Each
        // token is encoding/minimum BNW version:repetitions.
        const char *signature =
            is_vp ? "i1:10 l1:1 i1:30 b1:1 v1:2 n1:1 e1:1 i1:12 f1:1 i1:20 f1:2 i1:10 v1:3 i1:11 "
                    "b1:1 i1:2 b1:2 i1:67 v1:1 i1:91 b1:1 i1:22 t1:1 i1:2 s1:2 i1:8 b1:1 s1:1 "
                    "i1:10 b1:1 i1:4 v1:1 i1:10 v1:5 t1:4 i1:1 v1:2 i1:71 v1:1 u1:1 b1:14 i1:2 "
                    "b1:3 i1:1 b1:1 i1:1 b1:2 t1:1 v1:35 i1:1 v1:9 i1:4 v1:1 i1:1 v1:3 b1:2 "
                    "v1:17 o1:1 s1:2 v1:9 i1:1 v1:2 c1:1 v1:15 b1:1 i1:1 v1:5 t1:4 y1:3 b1:2 "
                    "i1:8 b1:1 i1:8 v1:1"
                  : "i1:29 i8:1 i1:2 i14:1 i1:9 i9:2 i1:39 i10:1 i13:1 i1:24 i15:1 i1:22 i2:1 "
                    "i3:1 i1:13 i12:1 i1:52 i16:1 i1:6 i6:1 i1:1 b1:11 i1:1 b1:3 i1:1 b1:2 "
                    "i1:2 v1:7 v4:1 v1:5 v5:1 v1:1 v7:1 s1:2 h1:6 h7:1 v1:1 h1:2 v1:9 t1:2";
        std::string s;
        if (!is_vp)
            u32(s, version);
        const auto ints = [&](std::initializer_list<std::uint32_t> ns) {
            for (auto n : ns)
                u32(s, n);
        };
        const auto row = [&] { ints({70000, 0xffffffff, 0x80000000}); };
        std::istringstream tokens(signature);
        std::string token;
        while (tokens >> token) {
            const auto colon = token.find(':');
            const auto minimum = std::stoul(token.substr(1, colon - 1));
            const auto repeats = std::stoul(token.substr(colon + 1));
            if (minimum > version)
                continue;
            for (unsigned n = 0; n < repeats; ++n) {
                switch (token[0]) {
                case 'i':
                    u32(s, 70000);
                    break;
                case 'u':
                    u32(s, 0xffffffff);
                    break;
                case 'l':
                    ints({1, 0x80000000});
                    break;
                case 'b':
                    s += char(1);
                    break;
                case 'f':
                    ints({0xfffffff9, 0});
                    break;
                case 's':
                    string(s, std::string("\0\xffX", 3));
                    break;
                case 'v':
                case 'e':
                    u32(s, 3);
                    row();
                    break;
                case 't':
                    u32(s, 3);
                    s.append("\0\1\0", 3);
                    break;
                case 'c':
                    u32(s, 3);
                    s.append("\xff\0\x80", 3);
                    break;
                case 'n':
                    ints({3, 3});
                    row();
                    ints({0xffffffff, 0});
                    break;
                case 'o':
                    ints({3, 0xffffffff, 70000, 70000, 0xffffffff, 70000, 0x80000000});
                    break;
                case 'h':
                    ints({4, 0, 0x12345678, 70000, 0xffffffff, 0xffffffff, 0x12345678, 0x80000000});
                    break;
                case 'y':
                    u32(s, 3);
                    for (unsigned y = 0; y < yields; ++y)
                        row();
                    break;
                default:
                    throw std::runtime_error("Invalid player fixture signature");
                }
            }
        }
        return s;
    };
    const auto observe = [&](const std::string &bytes, bool is_vp, saves::GameStateBudget budget = {},
                             std::uint64_t yields = 2) {
        std::istringstream s(bytes);
        runtime::BinaryReader r(s, bytes.size());
        return saves::observe_player_core(
            r, is_vp ? vp : bnw, is_vp ? std::optional<std::uint64_t>(yields) : std::nullopt, budget);
    };
    const auto field = [](const auto &p, std::string_view name) -> const saves::PlayerCoreField & {
        for (const auto &f : p.fields)
            if (f.name == name)
                return f;
        throw std::runtime_error("Missing player field: " + std::string(name));
    };
    const auto replace = [](std::string s, const auto &f, const std::string &bytes) {
        s.replace(static_cast<std::size_t>(f.start_offset),
                  static_cast<std::size_t>(f.end_offset - f.start_offset), bytes);
        return s;
    };
    for (const bool is_vp : {false, true}) {
        const auto bytes = fixture(is_vp);
        const auto p = observe(bytes, is_vp);
        check(p.fields.size() == (is_vp ? 581 : 268) && p.start_offset == 0 && p.end_offset == bytes.size(),
              "Player core boundary/field count changed");
        check(field(p, "m_eID").values == std::vector<std::int64_t>{70000}, "Player ID narrowed");
        check(field(p, "m_strScriptData").bytes == std::vector<std::uint8_t>{0, 255, 'X'},
              "Opaque player script bytes changed");
        const auto &v = field(p, "m_aiCityYieldChange");
        check(v.values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
              "Player values narrowed or normalized");
        check(field(p, "m_pabGetsScienceFromPlayer").values == std::vector<std::int64_t>{0, 1, 0},
              "Player boolean vector changed");
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), is_vp); });
        rejects([&] { observe(bytes, is_vp, {10, 1024, 1024}); });
        rejects([&] { observe(bytes, is_vp, {100000, 2, 1024}); });
        rejects([&] { observe(bytes, is_vp, {100000, 1024, 5}); });
        auto invalid = bytes;
        invalid[field(p, "m_bAlive").start_offset] = char(2);
        rejects([&] { observe(invalid, is_vp); });
        invalid = bytes;
        invalid[field(p, "m_pabGetsScienceFromPlayer").start_offset + 4] = char(255);
        rejects([&] { observe(invalid, is_vp); });
        std::string sentinel;
        u32(sentinel, UINT32_MAX);
        if (is_vp) {
            const auto empty = observe(replace(bytes, v, sentinel), true);
            const auto &ev = field(empty, v.name);
            check(ev.wire_count == UINT32_MAX && ev.values.empty(), "VP empty sentinel lost");
        } else
            rejects([&] { observe(replace(bytes, v, sentinel), false); });
        rejects([&] { observe(replace(bytes, field(p, "m_strScriptData"), sentinel), is_vp); });
        std::string wide;
        u32(wide, 65537);
        for (unsigned i = 0; i < 65537; ++i)
            u32(wide, i);
        const auto large = observe(replace(bytes, v, wide), is_vp);
        check(field(large, v.name).values.size() == 65537 && field(large, v.name).values.back() == 65536,
              "Player collection retained a 16-bit capacity");
        if (is_vp) {
            check(field(p, "m_lJONSCultureEverGeneratedTimes100").values[0] == INT64_MIN + 1,
                  "Signed 64-bit player culture narrowed");
            check(field(p, "m_fHappinessPolicies").values == std::vector<std::int64_t>{-7, 0},
                  "Saved fraction was normalized or interpreted as float");
            check(field(p, "m_aiNumResourceFromGP").values == std::vector<std::int64_t>{255, 0, 128},
                  "Resource byte sign extended");
            check(field(p, "m_vviYieldHistory").row_counts == std::vector<std::uint64_t>{3, UINT32_MAX, 0},
                  "Nested row counts lost");
            check(field(p, "m_aOptions").values ==
                      std::vector<std::int64_t>{UINT32_MAX, 70000, 70000, -1, 70000, INT32_MIN},
                  "Option keys narrowed/deduplicated");
            check(field(p, "m_ppaaiSpecialistExtraYield").values.size() == 6, "Fixed yield rows changed");
            const auto huge = observe(fixture(true, 16, 65537), true, {}, 65537);
            check(field(huge, "m_ppaaiSpecialistExtraYield").values.size() == 196611, "Yield width narrowed");
            rejects([&] { observe(bytes, true, {}, UINT64_MAX); });
            rejects([&] { observe(bytes, true, {}, 0); });
            std::string max64;
            u32(max64, UINT32_MAX);
            u32(max64, INT32_MAX);
            const auto max =
                observe(replace(bytes, field(p, "m_lJONSCultureEverGeneratedTimes100"), max64), true);
            check(field(max, "m_lJONSCultureEverGeneratedTimes100").values[0] == INT64_MAX,
                  "64-bit culture maximum changed");
        } else {
            const auto &h = field(p, "m_paiNumResourceUsed");
            check(h.hashes == std::vector<std::uint64_t>{0, 0x12345678, UINT32_MAX, 0x12345678} &&
                      h.values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
                  "Hashed player holes/duplicates changed");
            rejects([&] { observe(replace(bytes, h, sentinel), false); });
        }
    }
    for (unsigned version = 1; version <= 16; ++version) {
        const auto bytes = fixture(false, version);
        const auto p = observe(bytes, false);
        check(p.version == version && p.end_offset == bytes.size(), "Historical BNW field gate misaligned");
        check(field(p, "m_aiIncomingUnitTypes").values.size() == 3, "Legacy incoming unit list discarded");
    }
    for (unsigned bad : {0u, 17u, UINT32_MAX}) {
        auto bytes = fixture(false);
        std::string v;
        u32(v, bad);
        bytes.replace(0, 4, v);
        rejects([&] { observe(bytes, false); });
    }
    const auto bytes = fixture(true);
    std::istringstream s(bytes);
    runtime::BinaryReader r(s, bytes.size());
    rejects([&] { saves::observe_player_core(r, vp); });
    rejects([&] { saves::observe_player_core(r, bnw, 2); });
    class ShiftedCoreStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t pos_ = 0;
        pos_type seekpos(pos_type p, std::ios_base::openmode) override {
            pos_ = static_cast<std::uint64_t>(off_type(p));
            return p;
        }
        std::streamsize xsgetn(char *out, std::streamsize count) override {
            const auto local = pos_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(count) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, count, out);
            pos_ += count;
            return count;
        }

      public:
        explicit ShiftedCoreStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader shifted(stream, 0x100000123ull + bytes.size());
    shifted.seek(0x100000123ull);
    const auto p = saves::observe_player_core(shifted, vp, 2);
    check(p.start_offset == 0x100000123ull && p.end_offset == 0x100000123ull + bytes.size() &&
              p.fields[0].start_offset == 0x100000123ull,
          "Player core offsets narrowed at 4 GiB");
}
