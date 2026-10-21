#pragma once
void city_tail_tests() {
    static_assert(sizeof(void *) == 8);
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, counts;
        std::vector<std::pair<std::size_t, unsigned>> versions;
    };
    const auto fixture = [](bool vp, unsigned city_version = 6, unsigned religion_version = 3,
                            unsigned row_version = 2, bool empty = false, bool negative = false,
                            std::uint32_t spies = 2, std::uint32_t yields = 2) {
        Fixture f;
        auto integer = [&](std::uint32_t n) { u32(f.bytes, n); };
        auto flag = [&](unsigned n) {
            f.flags.push_back(f.bytes.size());
            f.bytes += char(n);
        };
        auto version = [&](unsigned value, unsigned maximum) {
            f.versions.push_back({f.bytes.size(), maximum});
            integer(value);
        };
        auto count = [&](unsigned size, bool signed_count = false, bool stl = false) {
            f.counts.push_back(f.bytes.size());
            integer(empty ? (signed_count && negative ? 0xfffffff9u : vp && stl ? UINT32_MAX : 0) : size);
            return empty ? 0u : size;
        };
        auto values = [&](std::uint64_t size) {
            for (std::uint64_t i = 0; i < size; ++i)
                integer(i % 2 ? 0x80000000 : 70000);
        };
        if (!vp) {
            version(religion_version, 3);
            if (religion_version >= 2)
                flag(1);
            if (religion_version >= 3)
                integer(70000);
        }
        const auto religions = count(12, !vp, vp); // Exceeds the original BNW inline capacity of eight.
        for (unsigned i = 0; i < religions; ++i) {
            if (!vp)
                version(row_version, 2);
            integer(i % 2 ? UINT32_MAX : 70000);
            if (!vp)
                flag(i % 2);
            integer(70000);
            integer(0x80000000);
            if (vp || row_version == 2)
                integer(UINT32_MAX);
        }
        if (vp)
            integer(70000);
        if (!vp)
            version(1, 1);
        values(2);
        values(vp ? yields : 6);
        if (vp) {
            flag(1);
            flag(0);
            flag(1);
        } else if (count(4)) {
            integer(0);
            integer(0xdeadbeef);
            flag(1);
            integer(0xdeadbeef);
            flag(0);
            integer(UINT32_MAX);
            flag(1);
        }
        version(0, 0);
        if (vp) {
            integer(151);
            integer(0xdeadbeef);
        }
        for (unsigned i = 0; i < 19; ++i) {
            if (!vp && (i == 2 || i == 3 || i > 10))
                continue;
            const auto size = count(spies, i >= 6);
            if (i == 13 || i == 14 || i == 17)
                for (unsigned j = 0; j < size; ++j)
                    flag(j % 2);
            else
                values(size);
        }
        if (vp) {
            for (unsigned group = 0; group < 2; ++group) {
                const auto size = count(2, false, true);
                for (unsigned j = 0; j < size; ++j)
                    for (unsigned k = 0; k < (group ? 7u : 20u); ++k) {
                        const auto entries = count(2, false, true);
                        for (unsigned e = 0; e < entries; ++e) {
                            integer(70000);
                            integer(0x80000000);
                            if (!group && k >= 10 && k <= 12)
                                integer(0); // Keep zero denominator as bytes.
                        }
                    }
            }
            if (count(2, false, true)) {
                integer(70000);
                integer(UINT32_MAX);
                f.bytes += char(0);
                f.bytes += char(0x80);
                integer(70000);
                integer(UINT32_MAX);
                f.bytes += char(0xff);
                f.bytes += char(0x7f);
            }
            values(count(2, false, true) * 2);
            if (count(2, false, true))
                for (unsigned i = 0; i < 2; ++i) {
                    integer(70000);
                    values(count(2, false, true) * 2);
                }
            values(count(2, false, true) * 3);
            integer(0x80000000);
        } else if (city_version >= 3)
            integer(70000);
        if (!vp)
            version(1, 1);
        flag(1);
        values(2);
        if (vp)
            values(count(2, false, true) * 2);
        else {
            values(80);
            values(80);
        }
        return f;
    };
    const auto context = [](bool vp, unsigned city = 6) {
        saves::CityTailContext c;
        if (vp)
            c.profile_extensions.emplace<saves::VpCityTailWireDimensions>(
                saves::VpCityTailWireDimensions{2, 3});
        else
            c.bnw_city_version = city;
        return c;
    };
    const auto observe = [&](const std::string &bytes, bool vp, saves::CityTailContext c,
                             saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_city_tail(
            in, vp ? saves::GameStateLayout::vp542_corpus : saves::GameStateLayout::expansion2_version1, c,
            budget);
    };
    const auto child = [](const auto &n, std::string_view name) -> const saves::CityTailNode & {
        for (const auto &c : n.children)
            if (name == c.name)
                return c;
        throw std::runtime_error("Missing city tail child");
    };
    const auto overwrite = [](std::string bytes, std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes.at(offset + i) = char(value >> (8 * i));
        return bytes;
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto c = context(vp);
        const auto p = observe(f.bytes + "NEXT_CITY", vp, c);
        check(p.start_offset == 0 && p.end_offset == f.bytes.size() && p.blocks.size() == 5,
              "City suffix boundary");
        check(bool(saves::vp_city_tail_dimensions(p.context)) == vp,
              "City tail acquired another GameCore's wire dimensions");
        for (unsigned i = 0; i < 5; ++i)
            check(p.blocks[i].start_offset == (i ? p.blocks[i - 1].end_offset : 0), "City suffix block gap");
        const auto &religions = child(p.blocks[0], "m_ReligionStatus");
        check(religions.children.size() == 12 &&
                  child(religions.children[0], "m_eReligion").values[0] == 70000 &&
                  child(religions.children[1], "m_eReligion").values[0] == -1,
              "Religion IDs/capacity narrowed");
        check(child(religions.children[0], "m_iPressure").values[0] == INT32_MIN, "Religion pressure sign");
        check(child(p.blocks[2], "m_aiSpyAssignment").values == std::vector<std::int64_t>{70000, INT32_MIN},
              "Spy values narrowed");
        if (vp) {
            check(child(p.blocks[0], "m_eMajorityCityReligion").values[0] == 70000, "Saved majority");
            check(p.blocks[2].children.size() == 22, "VP espionage fields");
            check(child(p.blocks[2], "m_abDiplomatReceiveIntrigues").values ==
                      std::vector<std::int64_t>{0, 1},
                  "Spy bool widths");
            const auto &y = child(p.blocks[3], "m_yieldChanges");
            check(y.children.size() == 2 && y.children[0].children.size() == 20, "Extra yields nesting");
            check(child(y.children[0], "fromImprovement").values ==
                      std::vector<std::int64_t>{70000, INT32_MIN, 0, 70000, INT32_MIN, 0},
                  "Raw fraction changed");
            check(child(p.blocks[3], "m_ppiGreatPersonProgressFromConstruction").values ==
                      std::vector<std::int64_t>{70000, -1, -32768, 70000, -1, 32767},
                  "Short width/sign or duplicate keys lost");
            check(child(p.blocks[3], "m_eventYields").children[0].children[2].name ==
                      std::string("forFeature"),
                  "Event yield source order");
            check(child(p.blocks[3], "m_miTechEnhancedYields").children.size() == 2, "Nested map rows");
        } else {
            check(p.blocks[2].children.size() == 10 && p.blocks[4].children.size() == 6, "BNW suffix fields");
            const auto &flags = child(p.blocks[1], "m_pbEmphasize");
            check(flags.children[0].children.size() == 1 &&
                      flags.children[1].children[0].values[0] == 0xdeadbeefull &&
                      flags.children[2].children[1].values[0] == 0 &&
                      flags.children[3].children[0].values[0] == UINT32_MAX,
                  "Emphasis null/unknown/duplicate hashes");
            check(child(p.blocks[4], "m_aiPlayerCloseness").values.size() == 80, "BNW source player span");
        }
        for (bool negative : {false, true}) {
            const auto e = fixture(vp, 6, 3, 2, true, negative);
            const auto q = observe(e.bytes, vp, c);
            const auto &status = child(q.blocks[0], "m_ReligionStatus");
            check(q.end_offset == e.bytes.size() && status.children.empty(), "Empty suffix containers");
            check(status.wire_count == (vp         ? std::int64_t(UINT32_MAX)
                                        : negative ? -7
                                                   : 0),
                  "Raw signed/STL count lost");
        }
        for (std::size_t cut = 0; cut < f.bytes.size(); ++cut)
            rejects([&] { observe(f.bytes.substr(0, cut), vp, c); });
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = char(2);
            rejects([&] { observe(bad, vp, c); });
        }
        for (auto offset : f.counts)
            rejects([&] { observe(overwrite(f.bytes, offset, 0x7fffffff), vp, c); });
        for (auto [offset, maximum] : f.versions) {
            rejects([&] { observe(overwrite(f.bytes, offset, maximum + 1), vp, c); });
            // Zero has identical shape only for the unbranched emphasis/CityAI versions.
            if (maximum == 1)
                check(observe(overwrite(f.bytes, offset, 0), vp, c).end_offset == f.bytes.size(),
                      "Version zero");
        }
        saves::GameStateBudget tiny;
        tiny.elements = 1;
        rejects([&] { observe(f.bytes, vp, c, tiny); });
        rejects([&] { observe(f.bytes, vp, {}); });
        rejects([&] { observe(f.bytes, vp, context(!vp)); });
        auto both = context(true);
        both.bnw_city_version = 0;
        rejects([&] { observe(f.bytes, vp, both); });
        if (vp) {
            rejects([&] { observe(overwrite(f.bytes, p.blocks[2].start_offset + 4, 150), vp, c); });
            rejects([&] { observe(overwrite(f.bytes, p.blocks[2].start_offset + 8, 0), vp, c); });
            rejects([&] { observe(overwrite(f.bytes, p.blocks[2].start_offset + 12, UINT32_MAX), vp, c); });
        }
    }
    for (unsigned city = 0; city <= 6; ++city)
        for (unsigned religion = 0; religion <= 3; ++religion)
            for (unsigned row = 0; row <= 2; ++row) {
                const auto f = fixture(false, city, religion, row);
                const auto p = observe(f.bytes, false, context(false, city));
                check(p.end_offset == f.bytes.size(), "BNW city/religion/entry version alignment");
                const auto &r = child(p.blocks[0], "m_ReligionStatus").children[0];
                check(r.children.size() == (row == 2 ? 6 : 5),
                      "Old religion entry has no trade pressure field");
                check(p.blocks[3].children.size() == (city >= 3 ? 1 : 0), "BNW extra hit points version");
            }
    auto c = context(true);
    c.profile_extensions.find<saves::VpCityTailWireDimensions>()->yields = 65537;
    const auto large = fixture(true, 6, 3, 2, false, false, 2, 65537);
    check(child(observe(large.bytes, true, c).blocks[1], "m_aiEmphasizeYieldCount").values.size() == 65537,
          "Yield span narrowed");
    // A counted spy array also accepts more than the original major-civ limit.
    const auto many = fixture(false, 6, 3, 2, false, false, 65537);
    check(child(observe(many.bytes, false, context(false)).blocks[2], "m_aiSpyAssignment").values.size() ==
              65537,
          "Spy count narrowed");
    c.profile_extensions.find<saves::VpCityTailWireDimensions>()->yields = UINT64_MAX;
    rejects([&] { observe(fixture(true).bytes, true, c); });
    rejects([&] { observe(fixture(false).bytes, false, context(false, 7)); });
    const auto normal = fixture(true).bytes;
    class ShiftedTailStream final : public std::streambuf {
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
        explicit ShiftedTailStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(normal);
    std::istream stream(&backing);
    runtime::BinaryReader in(stream, 0x100000123ull + normal.size());
    in.seek(0x100000123ull);
    const auto wide = saves::observe_city_tail(in, saves::GameStateLayout::vp542_corpus, context(true));
    check(wide.start_offset == 0x100000123ull && wide.end_offset == 0x100000123ull + normal.size() &&
              wide.blocks[4].children[0].start_offset > UINT32_MAX,
          "City tail offsets narrowed");
}
