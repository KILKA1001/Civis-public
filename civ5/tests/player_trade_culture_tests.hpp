#pragma once
void player_trade_culture_tests() {
    using Dims = saves::PlayerTradeCultureWireDimensions;
    constexpr auto vp_layout = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw_layout = saves::GameStateLayout::expansion2_version1;
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, signed_counts, unsigned_counts, name_sizes;
        std::vector<std::pair<std::size_t, unsigned>> versions;
        std::size_t lifetime_offset = 0;
    };
    // Independent source wire writer. Defaults include nonempty history,
    // vote commitments, paths, archaeological cells and binary tooltips.
    const auto fixture = [](bool vp, unsigned trade_version = 2, unsigned league_version = 2,
                            unsigned culture_version = 6, bool empty = false, bool negative = false,
                            Dims d = Dims{3, 2}, std::uint32_t expired = 2) {
        Fixture f;
        auto &s = f.bytes;
        const auto integer = [&](std::uint32_t v = 70000) { u32(s, v); };
        const auto values = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                integer(i % 3 == 0 ? 70000 : i % 3 == 1 ? UINT32_MAX : 0x80000000);
        };
        const auto flag = [&](unsigned v) {
            f.flags.push_back(s.size());
            s += char(v);
        };
        const auto version = [&](unsigned v, unsigned max) {
            f.versions.emplace_back(s.size(), max);
            integer(v);
        };
        const auto count = [&](std::uint32_t n, bool signed_count = false, bool stl = false) {
            (signed_count ? f.signed_counts : f.unsigned_counts).push_back(s.size());
            const auto raw = negative && signed_count ? 0xfffffff9u : empty ? vp && stl ? UINT32_MAX : 0u : n;
            integer(raw);
            return (empty || (negative && signed_count)) ? 0u : n;
        };
        const auto text = [&] {
            f.name_sizes.push_back(s.size());
            integer(3);
            s.append("\xff\0a", 3);
        };
        const auto route = [&](bool plundered) {
            if (vp)
                values(12);
            else {
                if (!plundered)
                    integer(70000);
                values(9);
            }
            flag(1);
            values(vp ? 5 : 4);
            if (vp) {
                flag(0);
                values(2 * count(3, false, true));
                values(d.yields);
                values(d.yields);
            }
        };
        if (!vp)
            version(trade_version, 2);
        if (vp || trade_version >= 1) {
            const auto n = count(expired, !vp, vp);
            for (unsigned i = 0; i < n; ++i)
                route(false);
        } else
            integer(0x80000000);
        if (vp || trade_version >= 2) {
            const auto n = count(2, !vp, vp);
            for (unsigned i = 0; i < n; ++i) {
                if (!vp)
                    integer(UINT32_MAX);
                route(true);
                if (vp)
                    integer(UINT32_MAX);
            }
        }
        if (!vp)
            version(0, 0);
        integer(70000);
        if (!vp)
            version(league_version, 2);
        if (vp || league_version >= 2) {
            const auto n = count(2, false, vp);
            for (unsigned i = 0; i < n; ++i) {
                values(4);
                flag(i % 2);
            }
        }
        if (!vp)
            version(culture_version, 6);
        values(count(3, !vp, vp) * (vp ? 1 : 2));
        f.lifetime_offset = s.size();
        if (vp) {
            integer(0x89abcdef);
            integer(0x12345678);
            integer(70000);
            for (unsigned i = 0; i < 3; ++i)
                values(d.major);
        } else {
            integer(70000);
            const auto n = count(3, true);
            for (unsigned i = 0; i < n; ++i) {
                integer(70000 + i);
                integer(UINT32_MAX - i);
            }
        }
        if (vp || culture_version >= 2) {
            flag(1);
            flag(0);
        }
        if (vp || culture_version >= 3) {
            values(3);
            if (vp)
                values(2);
            text();
        }
        if (vp || culture_version >= 5)
            text();
        if (vp || culture_version >= 6) {
            integer(UINT32_MAX);
            if (vp)
                integer(70000);
            integer(0x80000000);
        }
        if (vp || culture_version >= 4)
            values(4);
        return f;
    };
    const auto observe = [&](const std::string &s, bool vp, Dims dims = Dims{3, 2},
                             saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(s);
        runtime::BinaryReader in(stream, s.size());
        return saves::observe_player_trade_culture(in, vp ? vp_layout : bnw_layout,
                                                   vp ? std::optional<Dims>(dims) : std::nullopt, budget);
    };
    const auto number = [](const auto &b, std::string_view name) {
        for (const auto &f : b.fields)
            if (name == f.name)
                return f.value;
        throw std::runtime_error("Missing trade/culture field");
    };
    const auto array = [](const auto &b, std::string_view name) -> const saves::PlotArrayObservation & {
        for (const auto &a : b.arrays)
            if (name == a.name)
                return a;
        throw std::runtime_error("Missing trade/culture array");
    };
    const auto overwrite = [](std::string s, std::size_t offset, std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i)
            s.at(offset + i) = char(n >> (8 * i));
        return s;
    };
    for (unsigned trade = 0; trade <= 2; ++trade)
        for (unsigned league = 0; league <= 2; ++league)
            for (unsigned culture = 0; culture <= 6; ++culture) {
                const auto f = fixture(false, trade, league, culture);
                const auto p = observe(f.bytes, false);
                check(p.end_offset == f.bytes.size() && p.blocks.size() == 4,
                      "BNW trade/league/culture version boundary");
                check(number(p.blocks[0], "version") == trade && number(p.blocks[2], "version") == league &&
                          number(p.blocks[3], "version") == culture,
                      "BNW subsystem versions merged");
                check(p.blocks[0].children.size() == (trade == 0   ? 0
                                                      : trade == 1 ? 2
                                                                   : 4) &&
                          p.blocks[2].children.size() == (league < 2 ? 0 : 2),
                      "Old trade/vote version received new containers");
                if (trade == 0)
                    check(number(p.blocks[0], "legacy_empty_value") == INT32_MIN,
                          "Trade v0 discarded field lost");
                else
                    rejects([&] { number(p.blocks[0], "legacy_empty_value"); });
                const auto &b = p.blocks[3];
                check(b.blobs.size() == (culture < 3   ? 0
                                         : culture < 5 ? 1
                                                       : 2),
                      "Old culture received synthetic tooltips");
                if (culture < 2)
                    rejects([&] { number(b, "m_bReportedTwoCivsAway"); });
                if (culture < 3)
                    rejects([&] { number(b, "m_eOpinion"); });
                if (culture < 4)
                    rejects([&] { number(b, "m_iSwappableWritingIndex"); });
                if (culture < 6)
                    rejects([&] { number(b, "m_iTurnIdeologySwitch"); });
            }
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto p = observe(f.bytes + "HAS_NOTIFICATIONS", vp);
        check(p.end_offset == f.bytes.size() && p.blocks.size() == 4,
              "Trade/culture consumed notifications flag");
        for (std::size_t i = 0; i < p.blocks.size(); ++i)
            check(p.blocks[i].start_offset == (i ? p.blocks[i - 1].end_offset : 0),
                  "Trade/culture block gap");
        const auto &trade = p.blocks[0], &league = p.blocks[2], &culture = p.blocks[3];
        check(trade.children.size() == 4 && trade.children[1].index == 1 && trade.children[3].index == 1,
              "Trade history order changed");
        const auto &expired = trade.children[0], &plunder = trade.children[2], &route = plunder.children[0];
        check(number(plunder, "m_iTurnPlundered") == -1 && number(route, "m_bTradeUnitMovingForward") == 1,
              "Plundered route turn/boolean wire order");
        if (vp) {
            check(number(expired, "m_iID") == number(route, "m_iID") && number(route, "m_iOriginID") == -1,
                  "Trade history IDs remapped/deduplicated");
            check(array(route, "m_aPlotList").columns == 2 &&
                      array(route, "m_aPlotList").values.size() == 6 &&
                      array(route, "m_aiOriginYields").values == std::vector<std::int64_t>{70000, -1},
                  "VP full route data lost");
            check(number(culture, "m_lLastTurnLifetimeCultureTimes100") == INT64_C(0x1234567889abcdef),
                  "64-bit lifetime culture narrowed");
            check(array(culture, "m_aDigCompletePlots").columns == 1 &&
                      array(culture, "m_aDigCompletePlots").values ==
                          std::vector<std::int64_t>{70000, -1, INT32_MIN},
                  "VP plot indices or null pointer sentinel normalized");
            check(array(culture, "m_aiLastTurnCulturalIPTTimes100").values ==
                      std::vector<std::int64_t>{70000, -1, INT32_MIN},
                  "VP fixed influence array misread");
            auto negative = overwrite(f.bytes, f.lifetime_offset, 0);
            negative = overwrite(negative, f.lifetime_offset + 4, 0x80000000);
            check(number(observe(negative, true).blocks[3], "m_lLastTurnLifetimeCultureTimes100") ==
                      INT64_MIN,
                  "Signed 64-bit culture changed");
        } else {
            check(route.arrays.empty() && expired.arrays.empty(),
                  "BNW partial trade history got synthetic path/yields");
            rejects([&] { number(route, "m_iID"); });
            rejects([&] { number(route, "m_iOriginID"); });
            check(array(culture, "m_aDigCompletePlots").columns == 2 &&
                      array(culture, "m_aDigCompletePlots").values.size() == 6,
                  "BNW archaeological coordinates became indices");
            check(array(culture, "m_aiCulturalInfluence").values ==
                          std::vector<std::int64_t>{70000, 70001, 70002} &&
                      array(culture, "m_aiLastTurnCulturalInfluence").values ==
                          std::vector<std::int64_t>{-1, -2, -3},
                  "BNW interleaved influence split incorrectly");
            rejects([&] { number(culture, "m_lLastTurnLifetimeCultureTimes100"); });
        }
        check(number(league.children[0], "eToPlayer") == 70000 && number(league.children[1], "bEnact") == 1,
              "Vote IDs/order/flags changed");
        check(culture.blobs.size() == 2 && culture.blobs[0].bytes == std::vector<std::uint8_t>{255, 0, 97},
              "Tooltip binary bytes decoded or lost");
        check(number(culture, "m_iTurnIdeologySwitch") == INT32_MIN, "Ideology turn normalized");
        for (auto off : f.flags) {
            auto bad = f.bytes;
            bad[off] = char(2);
            rejects([&] { observe(bad, vp); });
        }
        for (auto [off, max] : f.versions)
            rejects([&] { observe(overwrite(f.bytes, off, max + 1), vp); });
        for (auto off : f.signed_counts)
            rejects([&] { observe(overwrite(f.bytes, off, INT32_MAX), vp); });
        for (auto off : f.unsigned_counts)
            rejects([&] { observe(overwrite(f.bytes, off, 0x7fffffff), vp); });
        for (auto off : f.name_sizes)
            rejects([&] { observe(overwrite(f.bytes, off, UINT32_MAX), vp); });
        for (std::size_t n = 0; n < f.bytes.size(); ++n)
            rejects([&] { observe(f.bytes.substr(0, n), vp); });
        const auto empty = observe(fixture(vp, 2, 2, 6, true).bytes, vp);
        check(empty.blocks[0].children.empty() && empty.blocks[2].children.empty() &&
                  array(empty.blocks[3], "m_aDigCompletePlots").values.empty(),
              "Empty trade/vote/archaeology containers lost");
        if (vp)
            check(number(empty.blocks[0], "m_aRecentlyExpiredConnections_wire_count") == UINT32_MAX,
                  "VP STL sentinel count lost");
        else {
            auto bad = overwrite(f.bytes, f.unsigned_counts[0], UINT32_MAX);
            rejects([&] { observe(bad, false); });
            const auto neg = observe(fixture(false, 2, 2, 6, false, true).bytes, false);
            check(number(neg.blocks[0], "m_aRecentlyExpiredConnections_wire_count") == -7 &&
                      neg.blocks[0].children.empty() &&
                      number(neg.blocks[3], "cultural_influence_wire_count") == -7 &&
                      array(neg.blocks[3], "m_aiCulturalInfluence").values.empty(),
                  "Negative BNW loop count widened to allocation");
        }
        auto no_text = f.bytes;
        const auto off = f.name_sizes[0];
        no_text.erase(off + 4, 3);
        no_text = overwrite(no_text, off, 0);
        check(observe(no_text, vp).blocks[3].blobs[0].bytes.empty(), "Empty culture tooltip consumed bytes");
        saves::GameStateBudget budget;
        budget.elements = 2;
        rejects([&] { observe(f.bytes, vp, {3, 2}, budget); });
        budget = {};
        budget.string_bytes = 2;
        rejects([&] { observe(f.bytes, vp, {3, 2}, budget); });
        budget = {};
        budget.total_string_bytes = 5;
        rejects([&] { observe(f.bytes, vp, {3, 2}, budget); });
        budget = {};
        budget.elements = 8 * 1024 * 1024;
        const auto many = fixture(vp, 2, 2, 6, false, false, {3, 2}, 65537);
        const auto wide = observe(many.bytes, vp, {3, 2}, budget);
        check(wide.blocks[0].children[65536].index == 65536 && wide.end_offset == many.bytes.size(),
              "Trade history count narrowed");
    }
    saves::GameStateBudget large;
    large.elements = 8 * 1024 * 1024;
    const auto dimensions = fixture(true, 2, 2, 6, false, false, {65537, 65537}, 0);
    const auto wide = observe(dimensions.bytes, true, {65537, 65537}, large);
    check(array(wide.blocks[3], "m_aiCulturalInfluenceTimes100").values.size() == 65537 &&
              array(wide.blocks[0].children[0].children[0], "m_aiDestYields").values.size() == 65537,
          "Source spans became host capacity limits");
    const auto bytes = fixture(true).bytes;
    for (Dims d : {Dims{0, 2}, Dims{3, 0}, Dims{UINT64_MAX, 2}, Dims{3, UINT64_MAX}})
        rejects([&] { observe(bytes, true, d); });
    for (bool vp : {false, true}) {
        std::istringstream s(bytes);
        runtime::BinaryReader in(s, bytes.size());
        rejects([&] {
            saves::observe_player_trade_culture(in, vp ? vp_layout : bnw_layout,
                                                vp ? std::nullopt : std::optional<Dims>(Dims{}));
        });
    }
    class ShiftedTradeCultureStream final : public std::streambuf {
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
        explicit ShiftedTradeCultureStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream s(&backing);
    runtime::BinaryReader in(s, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_player_trade_culture(in, vp_layout, Dims{3, 2});
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + bytes.size() &&
              shifted.blocks[3].start_offset > UINT32_MAX,
          "Trade/culture stream offsets narrowed");
}
