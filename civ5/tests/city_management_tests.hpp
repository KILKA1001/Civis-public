#pragma once
void city_management_tests() {
    constexpr auto vp_layout = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw_layout = saves::GameStateLayout::expansion2_version1;
    static_assert(sizeof(void *) == 8);
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, counts, versions;
    };
    const auto fixture = [](bool vp, bool empty = false, bool negative = false, std::uint32_t flavors = 2,
                            std::uint32_t strategies = 3, std::uint32_t promotions = 2,
                            std::uint32_t buildings = 3, std::uint32_t projects = 2,
                            std::uint32_t weights = 3) {
        Fixture f;
        auto &s = f.bytes;
        auto integer = [&](std::uint32_t n) { u32(s, n); };
        auto values = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                integer(i % 3 == 0 ? 70000 : i % 3 == 1 ? UINT32_MAX : 0x80000000);
        };
        auto flag = [&](unsigned n) {
            f.flags.push_back(s.size());
            s += char(n);
        };
        auto flags = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                flag(i % 2);
        };
        auto version = [&] {
            f.versions.push_back(s.size());
            integer(1);
        };
        auto count = [&](std::uint32_t n, bool signed_count = true, bool stl = false) {
            f.counts.push_back(s.size());
            integer(empty ? signed_count && negative ? 0xfffffff9u : vp && stl ? UINT32_MAX : 0 : n);
            return empty ? 0u : n;
        };
        auto hashed = [&](bool boolean = false, bool signed_count = false, bool null_value = false) {
            if (count(4, signed_count)) {
                integer(0);
                if (null_value)
                    integer(70000);
                integer(0xdeadbeef);
                if (boolean)
                    flag(1);
                else
                    integer(UINT32_MAX);
                integer(0xdeadbeef);
                if (boolean)
                    flag(0);
                else
                    integer(0x80000000);
                integer(UINT32_MAX);
                if (boolean)
                    flag(1);
                else
                    integer(70000);
            }
        };
        if (vp) {
            values(flavors);
            flags(strategies);
            values(strategies);
        } else {
            version();
            values(count(flavors));
            hashed(true);
            hashed();
        }
        integer(UINT32_MAX);
        integer(70000);
        for (unsigned i = 0; i < 4; ++i) {
            if (vp) {
                const auto n = count(weights);
                for (std::uint64_t j = 0; j < n; ++j) {
                    integer(j % 3 == 2 ? UINT32_MAX : 70000);
                    integer(j % 3 == 0 ? 0x80000000 : j % 3 == 1 ? 70000 : UINT32_MAX);
                }
            } else {
                version();
                if (i == 2)
                    values(projects);
                else
                    hashed(false, true, i == 3);
            }
        }
        if (!vp)
            version();
        flag(1);
        flag(0);
        if (vp)
            flag(1);
        values(4);
        flag(0);
        flags(vp ? 91 : 37);
        flags(vp ? 91 : 37);
        if (vp)
            values(count(3, false, true));
        values(2);
        if (vp) {
            for (unsigned i = 0; i < 3; ++i)
                values(promotions);
            values(buildings);
            values(buildings);
            values(promotions);
        } else {
            hashed();
            hashed();
            hashed(false, true);
            hashed(false, true);
            hashed();
        }
        return f;
    };
    const auto context = [](bool vp) {
        saves::CityManagementContext c;
        if (vp)
            c.profile_extensions.emplace<saves::VpCityManagementWireDimensions>(
                saves::VpCityManagementWireDimensions{2, 3, 2, 3});
        else
            c.bnw_projects = 2;
        return c;
    };
    const auto observe = [&](const std::string &bytes, bool vp, saves::CityManagementContext c,
                             saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        return saves::observe_city_management(in, vp ? vp_layout : bnw_layout, c, budget);
    };
    const auto number = [](const auto &b, std::string_view name) {
        for (const auto &f : b.fields)
            if (name == f.name)
                return f.value;
        throw std::runtime_error("Missing city management field");
    };
    const auto overwrite = [](std::string s, std::size_t p, std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i)
            s.at(p + i) = char(n >> (8 * i));
        return s;
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto c = context(vp);
        const auto p = observe(f.bytes + "RELIGIONS", vp, c);
        check(p.start_offset == 0 && p.end_offset == f.bytes.size() && p.blocks.size() == 6 &&
                  p.city_plot_slots == (vp ? 91 : 37),
              "Management boundary/source plot slots");
        check(bool(saves::vp_city_management_dimensions(p.context)) == vp,
              "City management acquired another GameCore's wire dimensions");
        for (std::size_t i = 0; i < 6; ++i)
            check(p.blocks[i].start_offset == (i ? p.blocks[i - 1].end_offset : 0), "Management block gap");
        check(number(p.blocks[0], "m_eSpecialization") == -1 &&
                  number(p.blocks[0], "m_eDefaultSpecialization") == 70000,
              "City specialization IDs");
        check(p.blocks[0].arrays[0].values == std::vector<std::int64_t>{70000, -1}, "Flavor values");
        const auto &citizens = p.blocks[5];
        check(number(citizens, "m_bAutomated") == 1 && number(citizens, "m_bNoAutoAssignSpecialists") == 0 &&
                  number(citizens, "m_iNumUnassignedCitizens") == 70000 &&
                  number(citizens, "m_iNumForcedWorkingPlots") == INT32_MIN,
              "Citizen signed counts/flags");
        check(citizens.arrays[0].values.size() == p.city_plot_slots &&
                  citizens.arrays[1].values.size() == p.city_plot_slots && citizens.arrays[0].values[1] == 1,
              "Citizen plot flags");
        rejects([&] { number(p.blocks[0], "m_eFocusYield"); });
        rejects([&] { number(citizens, "m_vWorkedPlots"); });
        if (vp) {
            check(p.blocks[0].arrays[1].values == std::vector<std::int64_t>{0, 1, 0} &&
                      p.blocks[0].arrays[2].values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
                  "VP fixed strategy spans");
            for (unsigned i = 1; i < 5; ++i)
                check(p.blocks[i].arrays[0].columns == 2 &&
                          p.blocks[i].arrays[0].values ==
                              std::vector<std::int64_t>{70000, INT32_MIN, 70000, 70000, -1, -1},
                      "VP unsorted/duplicate weighted entries must survive");
            check(number(citizens, "m_bIsDirty") == 1 && citizens.arrays.size() == 9 &&
                      citizens.arrays[2].values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
                  "VP blockades/dirty state");
            check(citizens.arrays[3].values.size() == 2 && citizens.arrays[6].values.size() == 3 &&
                      citizens.arrays[8].values.size() == 2,
                  "VP specialist/building dimensions");
        } else {
            const auto &flags = p.blocks[0].hashes[0].rows;
            check(flags.size() == 4 && flags[0].type_hash == 0 && flags[0].values.empty() &&
                      flags[1].values == std::vector<std::int64_t>{1},
                  "BNW hashed strategy bool width");
            for (unsigned i : {1u, 2u}) {
                const auto &rows = p.blocks[i].hashes[0].rows;
                check(rows.size() == 4 && rows[0].values.empty() && rows[1].type_hash == 0xdeadbeefull &&
                          rows[2].type_hash == rows[1].type_hash && rows[2].values[0] == INT32_MIN,
                      "BNW production unknown/duplicate/null hashes");
            }
            check(p.blocks[3].arrays[0].values == std::vector<std::int64_t>{70000, -1},
                  "BNW project weights have no count");
            const auto &process = p.blocks[4].hashes[0].rows;
            check(process[0].type_hash == 0 && process[0].values == std::vector<std::int64_t>{70000} &&
                      process[1].values[0] == -1,
                  "BNW process null hash still carries a weight");
            check(citizens.arrays.size() == 2 && citizens.hashes.size() == 5 &&
                      citizens.hashes[0].rows[3].type_hash == UINT32_MAX,
                  "BNW citizen hash arrays");
            rejects([&] { number(citizens, "m_bIsDirty"); });
        }
        for (bool negative : {false, true}) {
            const auto e = fixture(vp, true, negative);
            const auto q = observe(e.bytes, vp, c);
            check(q.end_offset == e.bytes.size(), "Empty management containers");
            if (vp) {
                check(number(q.blocks[1], "weight_wire_count") == (negative ? -7 : 0) &&
                          number(q.blocks[5], "m_vBlockadedPlots_wire_count") == UINT32_MAX,
                      "Signed weighted count vs unsigned STL sentinel");
            } else {
                check(number(q.blocks[0], "flavor_wire_count") == (negative ? -7 : 0) &&
                          q.blocks[1].hashes[0].wire_count == (negative ? -7 : 0),
                      "BNW signed counts");
            }
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
        for (auto offset : f.versions) {
            rejects([&] { observe(overwrite(f.bytes, offset, 2), vp, c); });
            check(observe(overwrite(f.bytes, offset, 0), vp, c).end_offset == f.bytes.size(),
                  "BNW version zero");
        }
        saves::GameStateBudget tiny;
        tiny.elements = 1;
        rejects([&] { observe(f.bytes, vp, c, tiny); });
        rejects([&] { observe(f.bytes, vp, {}); });
        rejects([&] { observe(f.bytes, vp, context(!vp)); });
        auto both = context(true);
        both.bnw_projects = 2;
        rejects([&] { observe(f.bytes, vp, both); });
    }
    auto large_context = context(true);
    large_context.profile_extensions.find<saves::VpCityManagementWireDimensions>()->promotions = 65537;
    const auto large = fixture(true, false, false, 2, 3, 65537);
    check(observe(large.bytes, true, large_context).blocks[5].arrays[3].values.size() == 65537,
          "Specialist storage span narrowed");
    const auto weights = fixture(true, false, false, 2, 3, 2, 3, 2, 65537);
    check(observe(weights.bytes, true, context(true)).blocks[1].arrays[0].values.size() == 2 * 65537,
          "Weighted vector length narrowed");
    const auto normal = fixture(true).bytes;
    auto overflow = context(true);
    overflow.profile_extensions.find<saves::VpCityManagementWireDimensions>()->flavors = UINT64_MAX;
    rejects([&] { observe(normal, true, overflow); });
    auto projects = context(false);
    projects.bnw_projects = UINT64_MAX;
    rejects([&] { observe(fixture(false).bytes, false, projects); });
    auto zero = context(true);
    zero.profile_extensions.emplace<saves::VpCityManagementWireDimensions>(
        saves::VpCityManagementWireDimensions{0, 0, 0, 0});
    const auto empty = fixture(true, true, false, 0, 0, 0, 0);
    check(observe(empty.bytes, true, zero).end_offset == empty.bytes.size(), "Empty source dimensions");
    class ShiftedManagementStream final : public std::streambuf {
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
        explicit ShiftedManagementStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(normal);
    std::istream stream(&backing);
    runtime::BinaryReader in(stream, 0x100000123ull + normal.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_city_management(in, vp_layout, context(true));
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + normal.size() &&
              shifted.blocks[5].start_offset > UINT32_MAX,
          "Management offsets narrowed");
}
