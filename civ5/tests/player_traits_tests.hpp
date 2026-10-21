#pragma once
void player_traits_tests() {
    using Dims = saves::PlayerTraitsWireDimensions;
    using E = saves::PlayerTraitsEncoding;
    constexpr auto vp = saves::GameStateLayout::vp542_corpus,
                   bnw = saves::GameStateLayout::expansion2_version1;
    const auto fixture = [](bool is_vp, unsigned version = 19, Dims dims = Dims{3, 2}, bool sentinel = false,
                            bool negative = false) {
        // Independent source wire signature, not the native field table.
        // Token: primitive/container, first-last BNW version, repetitions.
        const char *signature =
            is_vp
                ? "b1-19:7 i1-19:48 b1-19:3 i1-19:4 b1-19:15 i1-19:25 b1-19:1 i1-19:2 b1-19:2 i1-19:1 "
                  "b1-19:3 i1-19:1 "
                  "b1-19:4 i1-19:3 b1-19:2 i1-19:3 b1-19:1 i1-19:31 b1-19:1 i1-19:1 b1-19:22 i1-19:1 p1-19:1 "
                  "b1-19:5 "
                  "i1-19:5 b1-19:4 i1-19:6 y1-19:7 t1-19:1 v1-19:2 o1-19:1 p1-19:1 v1-19:6 m1-19:2 v1-19:1 "
                  "p1-19:1 "
                  "v1-19:2 r1-19:7 n1-19:1 x1-19:3 m1-19:1 c1-19:1 i1-19:1 v1-19:1 m1-19:1 v1-19:1 o1-19:1 "
                  "n1-19:1 "
                  "d1-19:1 f1-19:1 v1-19:2 r1-19:2 y1-19:4 m1-19:2 y1-19:13 b1-19:5 i1-19:8 r1-19:1 y1-19:10 "
                  "r1-19:2 N1-19:2 r1-19:1 y1-19:6 r1-19:6 v1-19:1 l1-19:1 s1-19:1"
                : "i1-19:21 i19-19:1 i1-19:1 i17-19:1 i1-19:8 i2-19:4 i1-19:17 i4-19:1 i5-19:1 i6-19:1 "
                  "i8-19:1 "
                  "i9-19:2 i11-19:1 i13-19:1 i14-19:1 i15-19:2 i16-19:1 b1-19:14 i1-19:1 P1-19:1 b1-19:1 "
                  "b3-19:1 "
                  "b9-19:1 b12-19:1 b18-19:1 i1-19:2 y1-19:5 y7-19:2 h1-19:2 q1-19:1 P1-19:1 C1-19:1 "
                  "S10-19:1 "
                  "r1-19:3 v9-10:1 a11-19:1";
        if (!is_vp)
            dims = {6, 9};
        std::string s;
        unsigned raw_cache_index = 0;
        if (!is_vp)
            u32(s, version);
        const auto ints = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                u32(s, i % 3 == 0 ? 70000 : i % 3 == 1 ? UINT32_MAX : 0x80000000);
        };
        const auto flags = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                s += char(i % 2);
        };
        std::istringstream input(signature);
        std::string token;
        while (input >> token) {
            const auto dash = token.find('-'), colon = token.find(':');
            const auto lo = std::stoul(token.substr(1, dash - 1)),
                       hi = std::stoul(token.substr(dash + 1, colon - dash - 1));
            if (version < lo || version > hi)
                continue;
            const auto repeats = std::stoul(token.substr(colon + 1));
            for (unsigned r = 0; r < repeats; ++r) {
                const auto c = token[0];
                if (c == 'i') {
                    u32(s, 70000);
                    continue;
                }
                if (c == 'b') {
                    s += char(1);
                    continue;
                }
                if (c == 'x') {
                    constexpr std::array raw{255, 128, 2};
                    s += char(raw.at(raw_cache_index++));
                    continue;
                }
                if (c == 'y') {
                    ints(dims.yields);
                    continue;
                }
                if (c == 't') {
                    ints(dims.terrains);
                    continue;
                }
                if (c == 'h') {
                    for (auto n :
                         {4u, 0u, 0x12345678u, 70000u, UINT32_MAX, UINT32_MAX, 0x12345678u, 0x80000000u})
                        u32(s, n);
                    continue;
                }
                const bool counted = c == 'P' || c == 'q' || c == 'C' || c == 'S' || c == 'a';
                const auto raw = counted && negative             ? 0xfffffff9u
                                 : !counted && sentinel && is_vp ? UINT32_MAX
                                                                 : 3u;
                if (c != 'S')
                    u32(s, raw);
                if (raw != 3)
                    continue;
                if (c == 'v' || c == 's' || c == 'a' || c == 'C' || c == 'S')
                    ints(3);
                else if (c == 'o' || c == 'q')
                    flags(3);
                else if (c == 'r')
                    ints(3 * dims.yields);
                else if (c == 'p' || c == 'P' || c == 'm') {
                    for (unsigned i = 0; i < 3; ++i)
                        ints(2);
                } else if (c == 'f') {
                    for (unsigned i = 0; i < 3; ++i)
                        ints(3);
                } else if (c == 'c') {
                    for (unsigned i = 0; i < 3; ++i) {
                        ints(2);
                        flags(1);
                    }
                } else if (c == 'd') {
                    for (unsigned i = 0; i < 3; ++i) {
                        flags(1);
                        ints(2);
                    }
                } else if (c == 'n' || c == 'N' || c == 'l') {
                    for (unsigned row = 0; row < 3; ++row) {
                        u32(s, row < 2 ? 70000 : UINT32_MAX);
                        u32(s, row == 1 ? 0 : 2);
                        if (row == 1)
                            continue;
                        if (c == 'l')
                            ints(2);
                        else
                            for (unsigned j = 0; j < 2; ++j) {
                                if (c == 'n')
                                    s += char(j);
                                else
                                    u32(s, 70000);
                                u32(s, 0x80000000);
                            }
                    }
                } else
                    throw std::runtime_error("Unknown trait fixture code");
            }
        }
        return s;
    };
    const auto observe = [&](const std::string &s, bool is_vp, Dims d = Dims{3, 2},
                             saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(s);
        runtime::BinaryReader in(stream, s.size());
        return saves::observe_player_traits(in, is_vp ? vp : bnw,
                                            is_vp ? std::optional<Dims>(d) : std::nullopt, budget);
    };
    const auto field = [](const auto &p, std::string_view name) -> const saves::PlayerTraitsField & {
        for (const auto &f : p.fields)
            if (name == f.name)
                return f;
        throw std::runtime_error("Missing trait field");
    };
    const auto replace = [](std::string s, const auto &f, const std::string &bytes) {
        s.replace(static_cast<std::size_t>(f.start_offset),
                  static_cast<std::size_t>(f.end_offset - f.start_offset), bytes);
        return s;
    };
    constexpr std::array counts{79, 83, 84, 85, 86,  87,  89,  90,  94, 95,
                                96, 97, 98, 99, 101, 102, 103, 104, 105};
    for (unsigned v = 1; v <= 19; ++v) {
        const auto bytes = fixture(false, v), negative = fixture(false, v, {3, 2}, false, true);
        const auto p = observe(bytes, false), n = observe(negative, false);
        check(p.version == v && p.fields.size() == counts[v - 1] && p.end_offset == bytes.size(),
              "BNW trait version order/size");
        check(n.end_offset == negative.size() && field(n, "m_aMayaBonusChoices").wire_count == -7 &&
                  field(n, "m_abNoTrain").values.empty() &&
                  field(n, "m_paiMovesChangeUnitCombat").wire_count == -7,
              "Negative trait loop counts became unsigned allocations");
        check(field(p, "m_iExtraYieldThreshold").values.size() == 6 &&
                  !field(p, "m_iExtraYieldThreshold").wire_count,
              "BNW ArrayWrapper stream incorrectly read a count");
        if (v >= 10)
            check(!field(p, "m_paiMaintenanceModifierUnitCombat").wire_count &&
                      field(p, "m_paiMaintenanceModifierUnitCombat").values ==
                          std::vector<std::int64_t>{70000, -1, INT32_MIN},
                  "Shared combat count read twice");
        else
            rejects([&] { field(p, "m_paiMaintenanceModifierUnitCombat"); });
        if (v >= 9) {
            check(field(p, "m_aUniqueLuxuryAreas").encoding == (v < 11 ? E::vector_i32 : E::counted_i32),
                  "Old luxury vector encoding lost");
            if (v >= 11)
                check(field(n, "m_aUniqueLuxuryAreas").wire_count == -7 &&
                          field(n, "m_aUniqueLuxuryAreas").values.empty(),
                      "Signed luxury count changed");
        } else
            rejects([&] { field(p, "m_aUniqueLuxuryAreas"); });
        if (v < 19)
            rejects([&] { field(p, "m_iFaithFromKills"); });
        if (v < 17)
            rejects([&] { field(p, "m_iCapitalThemingBonusModifier"); });
        if (v < 7)
            rejects([&] { field(p, "m_iYieldChangePerTradePartner"); });
    }
    for (bool is_vp : {false, true}) {
        const auto bytes = fixture(is_vp);
        const auto p = observe(bytes + "ESPIONAGE", is_vp);
        check(p.end_offset == bytes.size() && p.fields.size() == (is_vp ? 314 : 105),
              "Traits consumed espionage data");
        for (std::size_t i = 0; i < p.fields.size(); ++i)
            check(p.fields[i].start_offset == (i       ? p.fields[i - 1].end_offset
                                               : is_vp ? 0
                                                       : 4),
                  "Trait field span discontinuity");
        const auto &yield = field(p, "m_iYieldRateModifier");
        check(yield.values[0] == 70000 && yield.values[1] == -1 && yield.values[2] == INT32_MIN &&
                  !yield.wire_count,
              "Fixed yields normalized or prefixed");
        check(field(p, "m_eCampGuardType").values[0] == 70000, "Unknown trait enum lost");
        check(field(p, "m_aFreeTraitUnits").values ==
                  std::vector<std::int64_t>{70000, -1, 70000, -1, 70000, -1},
              "Free units or prerequisite techs deduplicated");
        if (is_vp) {
            check(!p.version && field(p, "m_iStrategicResourceQuantityModifier").values.size() == 2,
                  "VP fixed terrain span came from a vector count");
            const std::array raw_names{"m_bHasYieldFromTileCultureBomb", "m_bHasYieldFromTileEarn",
                                       "m_bHasYieldFromTilePurchase"};
            constexpr std::array raw_values{255, 128, 2};
            for (std::size_t i = 0; i < raw_names.size(); ++i) {
                const auto &raw = field(p, raw_names[i]);
                check(raw.encoding == E::raw_bool8 && raw.values[0] == raw_values[i] &&
                          raw.end_offset - raw.start_offset == 1,
                      "Uninitialized VP trait cache byte normalized to bool");
            }
            check(field(p, "m_aibUnitCombatProductionCostModifier").values ==
                      std::vector<std::int64_t>{70000, -1, 0, 70000, -1, 0, 70000, -1, 0},
                  "Trait production int/int/bool map width");
            check(field(p, "m_aiiTradeRouteProductionSiphon").values ==
                      std::vector<std::int64_t>{0, 70000, -1, 0, 70000, -1, 0, 70000, -1},
                  "Trait trade bool/int/int map width");
            const auto &nested = field(p, "m_pbiYieldFromBarbarianCampClear");
            check(nested.columns == 2 && nested.rows.size() == 3 &&
                      nested.rows[0].key == nested.rows[1].key && nested.rows[1].wire_count == 0 &&
                      nested.rows[1].values.empty() && nested.rows[2].key == -1 &&
                      nested.rows[0].values == std::vector<std::int64_t>{0, INT32_MIN, 1, INT32_MIN},
                  "Nested trait keys/counts/signs changed");
            const auto &map = field(p, "m_miResourceYieldChangesFromGoldenAge");
            check(map.rows[0].values == std::vector<std::int64_t>{70000, INT32_MIN, 70000, INT32_MIN},
                  "Nested integer map deduplicated");
            const auto &list = field(p, "m_mviYieldChangePerLandConnectedCityTimes100");
            check(list.columns == 1 && list.rows.size() == 3 &&
                      list.rows[0].values == std::vector<std::int64_t>{70000, -1},
                  "Map/vector trait shape lost");
            const auto empty = observe(fixture(true, 19, {3, 2}, true), true);
            check(field(empty, "m_seFreePromotions").wire_count == UINT32_MAX &&
                      field(empty, "m_seFreePromotions").values.empty() &&
                      field(empty, "m_pbiYieldFromBarbarianCampClear").wire_count == UINT32_MAX &&
                      field(empty, "m_pbiYieldFromBarbarianCampClear").rows.empty(),
                  "VP STL empty sentinel lost");
            std::string sentinel_map;
            u32(sentinel_map, 1);
            u32(sentinel_map, 70000);
            u32(sentinel_map, UINT32_MAX);
            const auto inner = observe(replace(bytes, nested, sentinel_map), true);
            check(field(inner, nested.name).rows[0].wire_count == UINT32_MAX &&
                      field(inner, nested.name).rows[0].values.empty(),
                  "Inner trait sentinel changed");
        } else {
            for (auto name : {"m_iStrategicResourceQuantityModifier", "m_aiResourceQuantityModifier"}) {
                const auto &h = field(p, name);
                check(h.hashes == std::vector<std::uint64_t>{0, 0x12345678, UINT32_MAX, 0x12345678} &&
                          h.values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
                      "Trait hash holes/unknowns changed");
            }
            for (auto v : {0u, 20u, UINT32_MAX}) {
                auto bad = bytes;
                for (unsigned i = 0; i < 4; ++i)
                    bad[i] = char(v >> (8 * i));
                rejects([&] { observe(bad, false); });
            }
            for (unsigned v : {9, 10}) {
                const auto old = fixture(false, v);
                const auto q = observe(old, false);
                std::string max;
                u32(max, UINT32_MAX);
                rejects([&] { observe(replace(old, field(q, "m_aUniqueLuxuryAreas"), max), false); });
            }
        }
        for (const auto &f : p.fields) {
            if (f.encoding == E::boolean || f.encoding == E::vector_bool || f.encoding == E::counted_bool ||
                f.encoding == E::map_iib || f.encoding == E::map_bii || f.encoding == E::nested_map_ibi) {
                auto bad = bytes;
                const auto delta = f.encoding == E::boolean                                      ? 0
                                   : f.encoding == E::map_iib || f.encoding == E::nested_map_ibi ? 12
                                                                                                 : 4;
                bad[f.start_offset + delta] = char(2);
                rejects([&] { observe(bad, is_vp); });
            }
            if (f.wire_count) {
                auto bad = bytes;
                for (unsigned i = 0; i < 4; ++i)
                    bad[f.start_offset + i] = char(0x7f);
                rejects([&] { observe(bad, is_vp); });
            }
        }
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), is_vp); });
        saves::GameStateBudget budget;
        budget.elements = 2;
        rejects([&] { observe(bytes, is_vp, {3, 2}, budget); });
        std::string many;
        u32(many, 65537);
        for (unsigned i = 0; i < 65537; ++i) {
            u32(many, 70000);
            u32(many, UINT32_MAX);
        }
        const auto free = observe(replace(bytes, field(p, "m_aFreeTraitUnits"), many), is_vp);
        check(field(free, "m_aFreeTraitUnits").values.size() == 131074 &&
                  field(free, "m_aFreeTraitUnits").values.back() == -1,
              "Trait free units capped at original static capacity or 16 bits");
    }
    saves::GameStateBudget wide_budget;
    wide_budget.elements = 4 * 1024 * 1024;
    const auto wide = observe(fixture(true, 19, {65537, 65537}, true), true, {65537, 65537}, wide_budget);
    check(field(wide, "m_iYieldChangeWorldWonder").values.size() == 65537 &&
              field(wide, "m_iStrategicResourceQuantityModifier").values.size() == 65537,
          "Fixed source enum spans narrowed");
    const auto bytes = fixture(true);
    for (Dims d : {Dims{0, 2}, Dims{3, 0}, Dims{UINT64_MAX, 2}, Dims{3, UINT64_MAX}})
        rejects([&] { observe(bytes, true, d); });
    rejects([&] { saves::player_traits_encoding_name(static_cast<E>(255)); });
    for (bool is_vp : {false, true}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        rejects([&] {
            saves::observe_player_traits(in, is_vp ? vp : bnw,
                                         is_vp ? std::nullopt : std::optional<Dims>(Dims{}));
        });
    }
    class ShiftedTraitsStream final : public std::streambuf {
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
        explicit ShiftedTraitsStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader in(stream, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_player_traits(in, vp, Dims{3, 2});
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + bytes.size() &&
              shifted.fields.back().start_offset > UINT32_MAX,
          "Trait stream offsets narrowed");
}
