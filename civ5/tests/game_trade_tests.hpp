#pragma once
void game_trade_tests() {
    using saves::GameStateLayout;
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> counts, flags;
        std::size_t routes_end = 0, matrices = 0;
    };
    const auto fixture = [](bool vp, unsigned version = 3, unsigned plots = 3, unsigned yields = 3,
                            unsigned major = 2, unsigned routes = 2) {
        Fixture f;
        const auto num = [&](std::uint32_t n) { u32(f.bytes, n); };
        const auto count = [&](unsigned n) {
            f.counts.push_back(f.bytes.size());
            num(n);
        };
        const auto flag = [&](bool n) {
            f.flags.push_back(f.bytes.size());
            f.bytes += char(n);
        };
        if (!vp)
            num(version);
        count(routes);
        for (unsigned r = 0; r < routes; ++r) {
            if (vp || version >= 1)
                num(0xffffffffu);
            if (vp)
                num(70000);
            num(0x80000000u);
            num(0x7fffffffu);
            if (vp)
                num(80000);
            num(0xffffffffu);
            num(65537);
            num(70000);
            num(80000);
            num(99);
            num(98);
            num(65537);
            flag(r == 0);
            num(0x7fffffffu);
            if (vp)
                num(0x80000000u);
            num(65537);
            num(0xffffffffu);
            if (vp || version >= 2)
                num(0x80000000u);
            if (vp)
                flag(true);
            count(plots);
            for (unsigned i = 0; i < plots; ++i) {
                num(0x80000000u + i);
                num(0x7fffffffu - i);
            }
            if (vp) {
                for (unsigned i = 0; i < yields; ++i)
                    num(100 + i);
                for (unsigned i = 0; i < yields; ++i)
                    num(0x80000000u + i);
            } else
                for (unsigned i = 0; i < yields; ++i) {
                    num(100 + i);
                    num(0x80000000u + i);
                }
        }
        f.routes_end = f.bytes.size();
        if (vp) {
            count(3);
            count(4);
            num(0);
            num(0);
            num(0xffffffffu);
            num(70000);
            count(0);
            count(0);
        }
        f.matrices = f.bytes.size();
        if (vp || version >= 3)
            for (unsigned i = 0; i < major * major; ++i)
                num(0x80000000u + i);
        if (vp)
            for (unsigned i = 0; i < major * major; ++i)
                num(0x7fffffffu - i);
        num(0x7fffffffu);
        return f;
    };
    const auto observe = [](const std::string &bytes, bool vp, saves::TradeWireDimensions dims = {2, 3},
                            saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_game_trade(
            reader, vp ? GameStateLayout::vp542_corpus : GameStateLayout::expansion2_version1, dims, budget);
    };
    const auto replace32 = [](std::string &bytes, std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes[offset + i] = char(value >> (8 * i));
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto s = observe(f.bytes, vp);
        check(s.start_offset == 0 && s.routes_end_offset == f.routes_end && s.matrices_offset == f.matrices &&
                  s.end_offset == f.bytes.size() && s.dimensions.major_slots == 2 &&
                  s.dimensions.yield_slots == 3 && s.routes.size() == 2 && s.next_id == INT32_MAX &&
                  (vp ? !s.version : s.version == 3),
              "Trade boundaries, context or next ID changed");
        for (std::size_t i = 0; i < s.routes.size(); ++i) {
            const auto &r = s.routes[i];
            const auto *vp_route=saves::find_vp_trade_route(r);
            check(r.id == -1 && r.origin_x == INT32_MIN && r.origin_y == INT32_MAX && r.destination_x == -1 &&
                      r.destination_y == 65537 && r.origin_owner == 70000 && r.destination_owner == 80000 &&
                      r.domain == 99 && r.connection_type == 98 && r.unit_location_index == 65537 &&
                      r.unit_id == INT32_MAX && r.moving_forward == (i == 0) &&
                      r.circuits_completed == 65537 && r.circuits_to_complete == -1 &&
                      r.turn_route_complete == INT32_MIN,
                  "Trade source identity, movement, sentinel or unknown values normalized");
            check(vp ? vp_route && vp_route->origin_id == 70000 && vp_route->destination_id == 80000 &&
                           vp_route->speed_factor == INT32_MIN && vp_route->recalled
                     : vp_route == nullptr,
                  "VP-only fields invented or lost");
            check(r.plots.size() == 3 && r.plots[0].x == INT32_MIN && r.plots[2].y == INT32_MAX - 2 &&
                      r.origin_yields == std::vector<std::int64_t>{100, 101, 102} &&
                      r.destination_yields == std::vector<std::int64_t>{INT32_MIN,
                                                                        std::int64_t(INT32_MIN) + 1,
                                                                        std::int64_t(INT32_MIN) + 2},
                  "Trade path coordinates or interleaved yields changed");
        }
        check(s.technology_difference == std::vector<std::int64_t>{INT32_MIN, std::int64_t(INT32_MIN) + 1,
                                                                   std::int64_t(INT32_MIN) + 2,
                                                                   std::int64_t(INT32_MIN) + 3},
              "Trade technology matrix changed");
        if (vp) {
            const auto *profile=saves::find_vp_game_trade(s);
            check(profile && profile->routes_per_player.size() == 3 &&
                      profile->routes_per_player[0] == std::vector<std::int64_t>{0, 0, -1, 70000} &&
                      profile->routes_per_player[1].empty() && profile->routes_per_player[2].empty() &&
                      profile->policy_difference ==
                          std::vector<std::int64_t>{INT32_MAX, INT32_MAX - 1, INT32_MAX - 2, INT32_MAX - 3},
                  "Trade cache order, duplicate/sentinel references or policy values lost");
            auto copy=s;
            auto *copy_profile=copy.profile_extensions.find<saves::VpGameTradeState>();
            check(copy_profile && copy_profile->routes_per_player==profile->routes_per_player,
                  "Copied VP trade observation lost its profile cache");
            copy_profile->routes_per_player[0][0]=42;
            check(profile->routes_per_player[0][0]==0,
                  "Copied VP trade cache aliases its source");
        } else
            check(!saves::find_vp_game_trade(s), "Invented BNW trade cache");
        for (std::size_t n = 0; n < f.bytes.size(); ++n)
            rejects([&] { observe(f.bytes.substr(0, n), vp); });
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = 2;
            rejects([&] { observe(bad, vp); });
        }
        for (auto offset : f.counts) {
            auto bad = f.bytes;
            replace32(bad, offset, UINT32_MAX);
            rejects([&] { observe(bad, vp); });
        }
        rejects([&] { observe(f.bytes, vp, {0, 3}); });
        rejects([&] { observe(f.bytes, vp, {2, 0}); });
        rejects([&] { observe(f.bytes, vp, {UINT64_MAX, 3}); });
        rejects([&] { observe(f.bytes, vp, {2, UINT64_MAX}); });
        rejects([&] { observe(f.bytes, vp, {65537, 3}); });
        rejects([&] { observe(f.bytes, vp, {2, 3}, {2, 100, 100}); });
        check(observe(f.bytes + "next system", vp).end_offset == f.bytes.size(),
              "Trade read consumed next block");
        check(observe(fixture(vp, 3, 65537, 3, 2, 1).bytes, vp).routes[0].plots.size() == 65537,
              "Original display point cap leaked into imported path");
        const auto many_yields = observe(fixture(vp, 3, 0, 65537, 2, 1).bytes, vp, {2, 65537});
        check(many_yields.routes[0].origin_yields.size() == 65537 &&
                  many_yields.routes[0].destination_yields.back() == std::int64_t(INT32_MIN) + 65536,
              "Yield array dimension narrowed");
        const auto matrix = observe(fixture(vp, 3, 0, 3, 257, 0).bytes, vp, {257, 3});
        check(matrix.technology_difference->size() == 66049 &&
                  matrix.technology_difference->back() == std::int64_t(INT32_MIN) + 66048,
              "Original major player capacity leaked into matrix");
    }
    for (unsigned v = 0; v <= 3; ++v) {
        const auto f = fixture(false, v);
        const auto s = observe(f.bytes, false);
        check(s.version == v && s.end_offset == f.bytes.size() &&
                  (v ? s.routes[0].id == -1 : !s.routes[0].id) &&
                  s.routes[0].turn_route_complete == (v >= 2 ? INT32_MIN : 0) &&
                  bool(s.technology_difference) == (v >= 3),
              "Legacy trade ID, turn or matrix presence wrong");
    }
    auto bad = fixture(false).bytes;
    replace32(bad, 0, 4);
    rejects([&] { observe(bad, false); });
    // Missing legacy matrices stay implicit, even for a wide host context.
    const auto old = observe(fixture(false, 0, 0, 3, 2, 0).bytes, false, {0x100000000ull, 0x100000001ull});
    check(old.dimensions.major_slots == 0x100000000ull && old.dimensions.yield_slots == 0x100000001ull &&
              !old.technology_difference && old.routes.empty(),
          "Legacy absent data forced a narrow context or allocation");
    const auto f = fixture(true);
    class ShiftedStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t pos_ = 0;
        pos_type seekpos(pos_type pos, std::ios_base::openmode) override {
            pos_ = static_cast<std::uint64_t>(off_type(pos));
            return pos;
        }
        std::streamsize xsgetn(char *out, std::streamsize count) override {
            const auto local = pos_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(count) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, count, out);
            pos_ += static_cast<std::uint64_t>(count);
            return count;
        }

      public:
        explicit ShiftedStream(const std::string &bytes) : bytes_(bytes) {}
    } shifted(f.bytes);
    std::istream stream(&shifted);
    runtime::BinaryReader reader(stream, 0x100000123ull + f.bytes.size());
    reader.seek(0x100000123ull);
    const auto moved = saves::observe_game_trade(reader, GameStateLayout::vp542_corpus, {2, 3});
    check(moved.start_offset == 0x100000123ull && moved.end_offset == moved.start_offset + f.bytes.size() &&
              moved.matrices_offset > UINT32_MAX,
          "Trade offsets narrowed");
    rejects([&] { saves::observe_game_trade(reader, static_cast<GameStateLayout>(99), {2, 3}); });
}
