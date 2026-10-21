#pragma once
// Uses the runtime test harness helpers in runtime_tests.cpp.
void game_history_tests() {
    using saves::GameStateLayout;
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, versions, counts;
        std::size_t replay_end = 0, deals_offset = 0;
        std::array<std::size_t, 3> deal_offsets{};
    };
    const auto fixture = [](bool vp) {
        Fixture f;
        const auto count = [&](std::uint32_t value) {
            f.counts.push_back(f.bytes.size());
            u32(f.bytes, value);
        };
        const auto version = [&](std::uint32_t value) {
            f.versions.push_back(f.bytes.size());
            u32(f.bytes, value);
        };
        const auto flag = [&](bool value) {
            f.flags.push_back(f.bytes.size());
            f.bytes += char(value);
        };
        if (!vp)
            version(2);
        count(2);
        for (int i = 0; i < 2; ++i) {
            u32(f.bytes, i ? 2 : 9);
            u32(f.bytes, 0x7fffffffu);
            count(2);
            // Two pairs of signed 16-bit coordinates, expanded by the reader.
            u32(f.bytes, 0x7fff8000u);
            u32(f.bytes, 0xffffffffu);
            u32(f.bytes, 0xffffffffu);
            string(f.bytes, std::string("\0\xffX", 3));
        }
        f.replay_end = f.bytes.size();
        u32(f.bytes, 0xffffffffu); // Preserve session count; no reload increment.
        count(2);
        u32(f.bytes, 0x7fffffffu);
        u32(f.bytes, 0x80000000u);
        count(3);
        u32(f.bytes, 0x80000000u);
        u32(f.bytes, 0xffffffffu);
        u32(f.bytes, 7);
        u32(f.bytes, 0x7fffffffu);
        u32(f.bytes, 0x80000000u);
        count(0);
        count(2);
        for (int i = 0; i < 2; ++i) {
            u32(f.bytes, 0xffffffffu);
            u32(f.bytes, 0x7fffffffu);
            u32(f.bytes, 0x80000000u);
        }
        flag(true);
        u32(f.bytes, 0x80000000u);
        f.deals_offset = f.bytes.size();
        if (!vp)
            version(1);
        for (int group = 0; group < 3; ++group) {
            count(1);
            f.deal_offsets[group] = f.bytes.size();
            if (vp)
                u32(f.bytes, group == 1 ? 16 : 0x7fffffffu);
            else
                version(group + 1);
            for (auto value : {0x80000000u, 9000u, 0xffffffffu, 3u, 6u})
                u32(f.bytes, value);
            flag(true);
            if (!vp) {
                if (group == 2)
                    flag(true);
                flag(true);
            }
            for (auto value : {0xffffffffu, 91u, 0x80000000u, 0x7fffffffu})
                u32(f.bytes, value);
            count(2);
            for (int item = 0; item < 2; ++item) {
                if (!vp && group)
                    version(item + 1);
                for (auto value : {0x7fffffffu, 50u, 0xffffffffu, 0x80000000u, 0xffffffffu})
                    u32(f.bytes, value);
                if (vp || (group && item)) {
                    u32(f.bytes, 123);
                    flag(true);
                }
                u32(f.bytes, 9999);
                if (!vp && group) {
                    flag(true);
                    flag(false);
                }
            }
        }
        return f;
    };
    const auto observe = [](const std::string &bytes, GameStateLayout layout,
                            saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_game_history(reader, layout, budget);
    };
    const auto replace32 = [](std::string &bytes, std::size_t position, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes[position + i] = static_cast<char>(value >> (8 * i));
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto layout = vp ? GameStateLayout::vp542_corpus : GameStateLayout::expansion2_version1;
        const auto state = observe(f.bytes, layout);
        check(state.start_offset == 0 && state.end_offset == f.bytes.size() &&
                  state.replay_end_offset == f.replay_end && state.deals_offset == f.deals_offset,
              "History boundary drift");
        check(state.replay.size() == 2 && state.replay[0].turn == 9 && state.replay[1].turn == 2 &&
                  state.replay[0].type == INT32_MAX && state.replay[0].player == -1 &&
                  state.replay[0].text == std::vector<std::uint8_t>{0, 255, 'X'},
              "Replay messages sorted, deduplicated, normalized or text lost");
        check(state.replay[0].plots.size() == 2 && state.replay[0].plots[0].x == -32768 &&
                  state.replay[0].plots[0].y == 32767 && state.replay[0].plots[1].x == -1 &&
                  sizeof(state.replay[0].plots[0].x) == 8,
              "Replay coordinate sign or host width lost");
        check(state.sessions == -1 && state.archaeology_triggered &&
                  state.earliest_barbarian_release_turn == INT32_MIN,
              "History load side effects applied");
        check(state.extra_yields.size() == 2 && state.extra_yields[0].plot.x == INT32_MAX &&
                  state.extra_yields[0].plot.y == INT32_MIN && state.extra_yields[1].yields.empty() &&
                  state.extra_yields[0].yields == std::vector<std::int64_t>{INT32_MIN, -1, 7} &&
                  state.extra_costs.size() == 2 && state.extra_costs[1].cost == INT32_MIN,
              "Plot overrides narrowed, deduplicated or dimensions invented");
        for (std::size_t group = 0; group < 3; ++group) {
            check(state.deals.groups[group].size() == 1, "Deal group lost");
            const auto &deal = state.deals.groups[group][0];
            check(deal.from_player == INT32_MIN && deal.to_player == 9000 && deal.final_turn == -1 &&
                      deal.duration == 3 && deal.start_turn == 6 && deal.peace_treaty == -1 &&
                      deal.surrendering_player == 91 && deal.demanding_player == INT32_MIN &&
                      deal.requesting_player == INT32_MAX,
                  "Deal parties, sentinels or timing normalized");
            check(deal.items.size() == 2 && deal.items[0].type == INT32_MAX && deal.items[0].duration == 50 &&
                      deal.items[0].final_turn == -1 && deal.items[0].data[0] == INT32_MIN &&
                      deal.items[0].data[1] == -1 && deal.items[0].from_player == 9999,
                  "Unknown trade type/data dropped or player slots capped");
            if (vp) {
                check(deal.id == (group == 1 ? 16 : INT32_MAX) && !deal.version && deal.is_gift == true &&
                          !deal.considering_renewal && !deal.checked_renewal && !deal.cancelled &&
                          !deal.items[0].version && !deal.items[0].from_renewed &&
                          !deal.items[0].to_renewed && deal.items[0].flag1 && deal.items[0].data[2] == 123,
                      "VP deal layout altered");
            } else {
                check(!deal.id && !deal.is_gift && deal.version == group + 1 &&
                          deal.considering_renewal == true && deal.checked_renewal == (group == 2) &&
                          deal.cancelled == true,
                      "BNW deal versions/renewals lost");
                check(deal.items[0].data[2] == 0 && !deal.items[0].flag1 &&
                          deal.items[0].from_renewed == bool(group) && deal.items[0].to_renewed == false &&
                          deal.items[1].data[2] == (group ? 123 : 0) && deal.items[1].flag1 == bool(group),
                      "BNW legacy/v1/v2 trade item defaults lost");
                check(group ? deal.items[0].version == 1 && deal.items[1].version == 2
                            : !deal.items[0].version && !deal.items[1].version,
                      "Invented legacy item version");
            }
        }
        check(vp ? !state.replay_version && !state.deals.version && state.deals.next_deal_id == 2147483648ull
                 : state.replay_version == 2 && state.deals.version == 1 && !state.deals.next_deal_id,
              "Version or derived x64 deal ID lost");
        if (vp) {
            auto bytes = f.bytes;
            replace32(bytes, f.deal_offsets[2], 3);
            check(observe(bytes, layout).deals.next_deal_id == 17,
                  "Proposed deals changed native next-ID derivation");
        }
        for (std::size_t n = 0; n < f.bytes.size(); ++n)
            rejects([&] { observe(f.bytes.substr(0, n), layout); });
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = 2;
            rejects([&] { observe(bad, layout); });
        }
        for (auto offset : f.versions) {
            auto bad = f.bytes;
            replace32(bad, offset, 99);
            rejects([&] { observe(bad, layout); });
        }
        for (auto offset : f.counts) {
            auto bad = f.bytes;
            replace32(bad, offset, UINT32_MAX);
            rejects([&] { observe(bad, layout); });
        }
        rejects([&] { observe(f.bytes, layout, {2, 100, 100}); });
        rejects([&] { observe(f.bytes, layout, {1000, 2, 100}); });
        rejects([&] { observe(f.bytes, layout, {1000, 100, 5}); });
        rejects([&] { observe(f.bytes, static_cast<GameStateLayout>(99)); });
        check(observe(f.bytes + "religions", layout).end_offset == f.bytes.size(),
              "History consumed religions");
    }
    auto wide = fixture(true).bytes;
    replace32(wide, 12, 65537);
    wide.insert(24, 4 * (65537 - 2), '\0');
    const auto state = observe(wide, GameStateLayout::vp542_corpus);
    check(state.replay[0].plots.size() == 65537, "Replay plot count inherited a 16-bit limit");
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
    } shifted(wide);
    std::istream stream(&shifted);
    runtime::BinaryReader reader(stream, 0x100000123ull + wide.size());
    reader.seek(0x100000123ull);
    const auto moved = saves::observe_game_history(reader, GameStateLayout::vp542_corpus);
    check(moved.start_offset == 0x100000123ull && moved.end_offset == moved.start_offset + wide.size() &&
              moved.deals_offset > 0xffffffffull,
          "Game history narrowed stream offsets");
}
