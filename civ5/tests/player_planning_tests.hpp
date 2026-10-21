#pragma once
void player_planning_tests() {
    using Dims = saves::PlayerPlanningWireDimensions;
    constexpr Dims dims{3, 2, 4};
    constexpr auto vp = saves::GameStateLayout::vp542_corpus,
                   bnw = saves::GameStateLayout::expansion2_version1;
    struct Fixture {
        std::string bytes;
        std::vector<std::uint64_t> versions, flags, counts, blobs, ends;
    };
    // Authored wire fixtures, independent of the runtime schema. Include active
    // gifts, both BNW quest versions and data absent in the first-player corpus.
    const auto fixture = [&](bool is_vp, std::uint32_t n = 3, bool sentinel = false,
                             unsigned builder_version = 2, bool negative = false) {
        Fixture f;
        auto &s = f.bytes;
        const auto ints = [&](std::uint64_t len) {
            for (std::uint64_t i = 0; i < len; ++i)
                u32(s, i % 3 == 0 ? UINT32_MAX : i % 3 == 1 ? 0x80000000 : 70000);
        };
        const auto flags = [&](std::uint64_t len) {
            for (std::uint64_t i = 0; i < len; ++i) {
                f.flags.push_back(s.size());
                s += char(i % 2);
            }
        };
        const auto version = [&](unsigned v = 1) {
            f.versions.push_back(s.size());
            u32(s, v);
        };
        const auto count = [&](std::uint32_t len, bool stl = false) {
            f.counts.push_back(s.size());
            u32(s, sentinel && stl ? UINT32_MAX : len);
            return sentinel && stl ? 0u : len;
        };
        const auto vector = [&](std::uint32_t len, unsigned width = 1) {
            ints(std::uint64_t(count(len, true)) * width);
        };
        const auto end = [&] { f.ends.push_back(s.size()); };
        if (is_vp) {
            ints(std::uint64_t(count(1, true)) * 4);
            ints(2);
            vector(n);
            const auto zones = count(2, true);
            for (unsigned z = 0; z < zones; ++z) {
                ints(26);
                vector(3);
            }
            vector(3, 2);
        } else {
            version();
            ints(1);
            count(n);
            for (unsigned z = 0; z < n; ++z) {
                version();
                ints(4);
                u32(s, 2);
            }
        }
        end();
        if (!is_vp) {
            version();
            ints(1);
        }
        end();
        if (is_vp) {
            ints(10);
            flags(1);
            flags(dims.major);
            ints(8 * dims.major);
            flags(5 * dims.major);
            flags(2 * dims.civ_teams);
            ints(2 * dims.major);
            flags(1);
            ints(1);
            flags(2);
            ints(1);
            ints(dims.civ_teams);
            flags(dims.civ_teams);
            flags(dims.major);
            ints(1);
            flags(1);
            flags(2 * dims.major);
            ints(1);
            ints(2 * dims.major);
            for (unsigned g = 0; g < dims.major; ++g) {
                u32(s, g == 0 ? UINT32_MAX : g == 1 ? 0xfffffffe : 2);
                if (!g)
                    continue;
                ints(6);
                f.blobs.push_back(s.size());
                u32(s, 3);
                s.append("\xff\x80\0", 3);
                vector(3, 2);
                vector(3, 2);
                flags(1);
                ints(5);
                f.blobs.push_back(s.size());
                u32(s, 3);
                s.append("\xff\0a", 3);
            }
            const auto players = count(2, true); // deliberately different from major
            for (unsigned p = 0; p < players; ++p) {
                const auto quests = count(p ? 0 : 2, true);
                for (unsigned q = 0; q < quests; ++q) {
                    ints(24);
                    flags(2);
                }
            }
        } else {
            version();
            ints(8);
            flags(dims.major);
            ints(dims.major * dims.major);
            flags(dims.major);
            ints(12 * dims.major);
            flags(4 * dims.major);
            flags(2 * dims.legacy_teams);
            for (unsigned p = 0; p < dims.major; ++p) {
                if (negative) {
                    u32(s, 0xfffffff9);
                    continue;
                }
                count(p ? 0 : 2);
                if (!p) {
                    version(1);
                    ints(4);
                    version(2);
                    ints(4);
                    flags(1);
                }
            }
        }
        flags(1);
        end();
        if (!is_vp)
            version();
        end();
        if (is_vp) {
            vector(3, 3);
            vector(3, 2);
            vector(3, 3);
            vector(n);
            vector(3);
        } else {
            version(builder_version);
            ints(1);
            count(n);
            ints(n);
            flags(1);
            if (builder_version >= 2)
                flags(1);
        }
        end();
        if (is_vp) {
            vector(3);
            vector(3);
            const auto cities = count(2, true);
            for (unsigned c = 0; c < cities; ++c) {
                ints(1);
                vector(3, 3);
            }
            vector(3);
        } else {
            version();
            count(2);
            s.append("\x80\xff\x7f\0\x80\xff\x7f\0", 8);
            if (negative)
                u32(s, 0xfffffffb);
            else {
                count(3);
                ints(3);
            }
        }
        end();
        if (!is_vp)
            version();
        ints(1);
        if (is_vp) {
            vector(3, 2);
            vector(3, 2);
        } else {
            flags(1);
            count(n);
            ints(n);
        }
        end();
        return f;
    };
    const auto observe = [&](const std::string &bytes, bool is_vp, Dims d = Dims{3, 2, 4},
                             saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_player_planning(in, is_vp ? vp : bnw, d, budget);
    };
    const auto field = [](const auto &b, std::string_view name) {
        for (const auto &f : b.fields)
            if (name == f.name)
                return f.value;
        throw std::runtime_error("Missing planning field");
    };
    const auto array = [](const auto &b, std::string_view name) -> const saves::PlotArrayObservation & {
        for (const auto &a : b.arrays)
            if (name == a.name)
                return a;
        throw std::runtime_error("Missing planning array");
    };
    for (bool is_vp : {false, true}) {
        const auto f = fixture(is_vp);
        const auto p = observe(f.bytes + "TRAILER", is_vp);
        check(p.blocks.size() == 7 && p.end_offset == f.bytes.size(), "Planning blocks consumed Traits");
        for (unsigned i = 0; i < 7; ++i)
            check(p.blocks[i].end_offset == f.ends[i] && p.blocks[i].start_offset == (i ? f.ends[i - 1] : 0),
                  "Planning block boundary mismatch");
        const auto &minor = p.blocks[2];
        check(array(minor, "m_aiFriendshipWithMajorTimes100").values ==
                  std::vector<std::int64_t>{-1, INT32_MIN, 70000},
              "Friendship units or signs normalized");
        check(array(minor, "m_abPermanentWar").values.size() == (is_vp ? 2 : 4),
              "Minor team dimensions changed");
        if (is_vp) {
            const auto &zone = p.blocks[0].children[1];
            check(field(zone, "friendly_melee_strength") == 2147483648ll &&
                      field(zone, "enemy_ranged_strength") == 2147483648ll &&
                      field(zone, "neutral_unit_strength") == UINT32_MAX && field(zone, "zone_id") == -1,
                  "VP signed/unsigned tactical fields narrowed");
            check(field(zone, "reserved") == INT32_MIN && field(zone, "average_x_times1000") == 70000,
                  "Reserved slot dropped or scaled coordinate rounded");
            check(minor.children.size() == 5 && minor.children[0].fields.size() == 1 &&
                      field(minor.children[1], "arrival_countdown") == -2,
                  "VP gift activation or quest outer count changed");
            const auto &g = minor.children[1];
            check(g.blobs.size() == 2 && g.blobs[0].bytes == std::vector<std::uint8_t>{255, 128, 0} &&
                      g.blobs[1].bytes == std::vector<std::uint8_t>{255, 0, 97},
                  "Gift bitfield/name bytes reinterpreted");
            check(array(g, "promotion_duration").values ==
                      std::vector<std::int64_t>{-1, INT32_MIN, 70000, -1, INT32_MIN, 70000},
                  "Gift promotion map reordered");
            check(minor.children[3].children.size() == 2 &&
                      minor.children[3].children[0].end_offset - minor.children[3].children[0].start_offset ==
                          98,
                  "VP quest layout changed");
            check(field(minor.children[3].children[0], "assigned_player") == INT32_MIN,
                  "Quest owner replaced with inferred player ID");
            check(p.blocks[1].start_offset == p.blocks[1].end_offset &&
                      p.blocks[3].start_offset == p.blocks[3].end_offset,
                  "Empty VP AI blocks consumed data");
            check(p.blocks[5].children.size() == 2 && field(p.blocks[5].children[0], "city_id") == -1 &&
                      field(p.blocks[5].children[1], "city_id") == -1 &&
                      array(p.blocks[5].children[0], "connections").columns == 3,
                  "Nested route map deduplicated or flattened incorrectly");
            const auto empty = observe(fixture(true, 3, true).bytes, true);
            check(empty.blocks[0].children.empty() && empty.blocks[5].children.empty() &&
                      field(empty.blocks[0], "focus_areas_wire_count") == UINT32_MAX &&
                      field(empty.blocks[2], "quest_players_wire_count") == UINT32_MAX,
                  "STL empty sentinel lost");
        } else {
            check(field(p.blocks[0].children[0], "naval_invasion") == 2,
                  "BNW tactical integer normalized to bool");
            const auto &q = minor.children[0].children;
            check(q.size() == 2 && q[0].fields.size() == 5 && q[1].fields.size() == 6 &&
                      q[0].end_offset - q[0].start_offset == 20 && q[1].end_offset - q[1].start_offset == 21,
                  "BNW quest v1/v2 or absent defaults changed");
            check(array(minor, "m_aaiNumEnemyUnitsLeftToKillByMajor").columns == dims.major &&
                      array(minor, "m_aiPlayerQuests").values.size() == dims.major,
                  "Legacy minor matrix/quest fields lost");
            check(array(p.blocks[5], "route_infos").values ==
                      std::vector<std::int64_t>{-128, -1, 127, 0, -128, -1, 127, 0},
                  "BNW route chars not sign-extended");
            check(array(p.blocks[6], "danger_plots").values ==
                      std::vector<std::int64_t>{UINT32_MAX, 2147483648ll, 70000},
                  "BNW danger scores interpreted as signed");
            const auto old = observe(fixture(false, 3, false, 1, true).bytes, false);
            check(old.blocks[4].fields.size() == 4 && field(old.blocks[5], "city_ids_wire_count") == -5 &&
                      field(old.blocks[2].children[0], "quests_wire_count") == -7 &&
                      old.blocks[2].children[0].children.empty(),
                  "BNW old builder or negative loop counts changed");
            auto bad = f.bytes;
            for (unsigned j = 0; j < 4; ++j)
                bad[p.blocks[6].start_offset + 9 + j] = char(255);
            rejects([&] { observe(bad, false); });
        }
        for (auto offset : f.versions) {
            auto bad = f.bytes;
            bad[offset] = char(3);
            rejects([&] { observe(bad, is_vp); });
            bad[offset] = 0;
            rejects([&] { observe(bad, is_vp); });
        }
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = char(2);
            rejects([&] { observe(bad, is_vp); });
        }
        for (auto offset : f.counts) {
            auto bad = f.bytes;
            for (unsigned j = 0; j < 4; ++j)
                bad[offset + j] = char(0x7f);
            rejects([&] { observe(bad, is_vp); });
        }
        for (auto offset : f.blobs) {
            auto bad = f.bytes;
            for (unsigned j = 0; j < 4; ++j)
                bad[offset + j] = char(255);
            rejects([&] { observe(bad, is_vp); });
        }
        for (std::size_t n = 0; n < f.bytes.size(); ++n)
            rejects([&] { observe(f.bytes.substr(0, n), is_vp); });
        saves::GameStateBudget budget;
        budget.elements = 2;
        rejects([&] { observe(f.bytes, is_vp, dims, budget); });
        if (is_vp) {
            budget = {};
            budget.string_bytes = 2;
            rejects([&] { observe(f.bytes, true, dims, budget); });
            budget = {};
            budget.total_string_bytes = 11;
            rejects([&] { observe(f.bytes, true, dims, budget); });
        }
        rejects([&] { observe(f.bytes, is_vp, {UINT64_MAX, 2, 4}); });
        rejects([&] { observe(f.bytes, is_vp, {3, UINT64_MAX, UINT64_MAX}); });
        const auto large = observe(fixture(is_vp, 65537).bytes, is_vp);
        check(is_vp ? array(large.blocks[0], "plot_zone_ids").values.size() == 65537 &&
                          array(large.blocks[4], "canal_wanted_plots").values.size() == 65537
                    : large.blocks[0].children.size() == 65537 &&
                          array(large.blocks[4], "non_territory_plots").values.size() == 65537 &&
                          array(large.blocks[6], "danger_plots").values.size() == 65537,
              "Planning containers capped by original static size or 16 bits");
    }
    const auto bytes = fixture(true).bytes;
    class ShiftedPlanningStream final : public std::streambuf {
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
        explicit ShiftedPlanningStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader in(stream, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_player_planning(in, vp, dims);
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + bytes.size() &&
              shifted.blocks[2].children[1].start_offset > UINT32_MAX,
          "Planning offset narrowed");
}
