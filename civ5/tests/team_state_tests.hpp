#pragma once
#include "civ5/saves/vp_team_profile.hpp"
void team_state_tests() {
    constexpr auto vp = saves::GameStateLayout::vp542_corpus,
                   bnw = saves::GameStateLayout::expansion2_version1;
    saves::TeamWireDimensions small;
    small.teams = 1;
    small.players = 2;
    small.civ_teams = 1;
    small.fixed_players = 3;
    small.fixed_teams = 4;
    small.domains = 2;
    small.major = 2;
    small.yields = 2;
    const saves::VpTeamContentDimensions rows{1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1};
    const auto fixture = [&](bool is_vp, std::uint32_t tech_count = 3) {
        std::string s;
        const auto ints = [&](std::initializer_list<std::uint32_t> ns) {
            for (auto n : ns)
                u32(s, n);
        };
        const auto fill = [&](std::uint64_t n, char type = 'i', unsigned value = 0x80000000u) {
            for (std::uint64_t i = 0; i < n; ++i) {
                if (type == 'i')
                    u32(s, value);
                else
                    s += char(type == 'B' ? 255 : i % 2);
            }
        };
        const auto hash = [&](char type = 'i', unsigned width = 1) {
            ints({2, 0, 0xfedcba98});
            fill(width, type);
        };
        if (is_vp)
            ints({2, 70000, 0xffffffff});
        else
            u32(s, 1);
        fill(is_vp ? 32 : 29);
        fill(is_vp ? 2 : 8, '?');
        ints({70000, 80000, 0xffffffff, 0xfffffffe});
        const auto slots = is_vp ? small.fixed_teams : small.teams;
        fill(slots);
        if (!is_vp)
            fill(slots);
        fill(slots);
        fill(small.domains);
        if (is_vp)
            fill(rows.vote_sources);
        else
            hash();
        fill(slots);
        fill(slots);
        for (unsigned i = 0; i < (is_vp ? 11u : 10u); ++i)
            fill(i == 1 ? (is_vp ? small.fixed_players : small.players) : slots, is_vp ? 'B' : '?');
        if (is_vp) {
            fill(rows.victories, '?');
            fill(rows.victories, '?');
            fill(rows.small_awards, '?');
            fill(rows.techs, '?');
            fill(rows.routes);
            fill(rows.builds);
            fill(rows.projects);
            fill(rows.projects);
            ints({2, 0xffffffff, 70000, 0xffffffff}); // two project vectors; second sentinel
            fill(rows.projects);
            fill(rows.unit_classes);
            fill(rows.building_classes);
            fill(rows.buildings);
            fill(rows.terrains);
            fill(rows.victories);
            fill(small.fixed_teams);
            fill(small.fixed_teams);
        } else {
            hash('?');
            hash('?');
            hash('?');
            hash();
            hash();
            ints({3, 0, 0xfedcba98, 2, 0xfedcba98, 1}); // last saved count for duplicate hash wins
            hash();
            ints({1, 0xfedcba98, 70000}); // one art entry, one implied value
            for (unsigned i = 0; i < 6; ++i)
                hash();
            fill(small.civ_teams);
        }
        u32(s, 1);
        if (is_vp)
            ints({151, 0xdeadbeef});
        u32(s, 70000);
        if (is_vp)
            u32(s, 65537);
        u32(s, tech_count);
        for (unsigned i = 0; i < tech_count; ++i)
            u32(s, i % 2 ? 0xfedcba98 : 0);
        fill(tech_count, '?');
        fill(tech_count, '?');
        fill(tech_count);
        if (is_vp)
            fill(tech_count);
        fill(tech_count);
        if (is_vp) {
            fill(small.domains);
            fill(rows.features * small.yields);
            fill(rows.terrains * small.yields);
            fill(rows.improvements * 3 * small.yields);
            ints({2, 70000, 0xffffffff, 65537, 0xffffffff});
            s += '\1';
            ints({70000, 80000, 90000});
            fill(small.fixed_teams);
            fill(small.major);
            fill(small.major);
            u32(s, 65537);
            // The repeated war snapshots must not overwrite the first copy.
            for (unsigned i = 0; i < small.fixed_teams * 2; ++i)
                s += char(17);
            s += '\1';
        } else {
            for (unsigned i = 0; i < 3; ++i)
                hash('i', small.yields);
            ints({3, 0, 0xfedcba98, 0xfedcba98});
        }
        return s;
    };
    const auto observe = [&](const std::string &s, bool is_vp,
                             saves::GameStateBudget b = saves::GameStateBudget{}) {
        auto d = small;
        if (is_vp)
            d.profile_extensions.emplace<saves::VpTeamContentDimensions>(rows);
        std::istringstream stream(s);
        runtime::BinaryReader r(stream, s.size());
        return saves::observe_teams(r, is_vp ? vp : bnw, d, b);
    };
    const auto field = [](const saves::TeamObservation &t, std::string_view name) {
        for (const auto &f : t.fields)
            if (f.name == name)
                return f.value;
        throw std::runtime_error("Missing team field");
    };
    const auto array = [](const saves::TeamObservation &t,
                          std::string_view name) -> const saves::PlotArrayObservation & {
        for (const auto &a : t.arrays)
            if (a.name == name)
                return a;
        throw std::runtime_error("Missing team array");
    };
    for (bool is_vp : {false, true}) {
        const auto bytes = fixture(is_vp);
        const auto m = observe(bytes + "player", is_vp);
        const auto &t = m.teams[0];
        check(m.start_offset == 0 && m.end_offset == bytes.size() && t.start_offset == 0 &&
                  t.end_offset == bytes.size(),
              "Team reader crossed player boundary");
        check(field(t, "id") == 70000 && field(t, "era") == 80000 && field(t, "killed_by") == -2 &&
                  field(t, "land") == INT32_MIN,
              "Team IDs or counters narrowed");
        check(array(t, "tech_hashes").values == std::vector<std::int64_t>{0, 0xfedcba98, 0} &&
                  array(t, "tech_count").values.size() == 3 && field(t, "last_tech") == 70000,
              "Technology holes lost parallel values");
        if (is_vp) {
            check(array(t, "members").values == std::vector<std::int64_t>{70000, -1}, "Team members changed");
            check(array(t, "at_war_initial").values[0] == 255 && array(t, "at_war_final").values[0] == 17,
                  "Repeated raw diplomacy arrays normalized or overwritten");
            check(array(t, "found_player_territory").values.size() == 3 &&
                      array(t, "has_met").values.size() == 4,
                  "VP fixed player/team widths confused");
            check(t.project_art[0].values == std::vector<std::int64_t>{-1, 70000} &&
                      t.project_art[1].wire_count == UINT32_MAX && t.project_art[1].values.empty(),
                  "Project art sentinel lost");
            check(array(t, "improvement_yields").columns == 6 &&
                      array(t, "improvement_yields").values.size() == 6,
                  "Interleaved VP improvement yields transposed");
            check(field(t, "tech_mod_version") == 151 && field(t, "tech_sentinel") == 0xdeadbeef &&
                      field(t, "num_techs") == 65537,
                  "VP tech prefix or counts changed");
        } else {
            check(t.project_art.size() == 1 && t.project_art[0].key == 0xfedcba98 &&
                      t.project_art[0].source_count == 1 && !t.project_art[0].wire_count &&
                      t.project_art[0].values == std::vector<std::int64_t>{70000},
                  "BNW project art did not use last matching saved count");
            check(t.hashes[0].rows[0].type_hash == 0 && t.hashes[0].rows[0].values.empty() &&
                      t.hashes[0].rows[1].values[0] == INT32_MIN,
                  "BNW hashed values changed");
            check(array(t, "found_player_territory").values.size() == 2 &&
                      array(t, "has_met").values.size() == 1,
                  "BNW active source slots changed");
            check(array(t, "revealed_resources").values ==
                      std::vector<std::int64_t>{0, 0xfedcba98, 0xfedcba98},
                  "BNW revealed hashes resolved/deduplicated prematurely");
        }
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), is_vp); });
        rejects([&] { observe(bytes, is_vp, {2, 0, 0}); });
        auto invalid = bytes;
        invalid[is_vp ? 140 : 120] = 2;
        rejects([&] { observe(invalid, is_vp); });
        const auto wide = observe(fixture(is_vp, 65537), is_vp);
        check(array(wide.teams[0], "tech_hashes").values.size() == 65537, "Technology capacity narrowed");
        auto d = small;
        if (is_vp)
            d.profile_extensions.emplace<saves::VpTeamContentDimensions>(rows);
        d.domains = UINT64_MAX;
        std::istringstream stream(bytes);
        runtime::BinaryReader r(stream, bytes.size());
        rejects([&] { saves::observe_teams(r, is_vp ? vp : bnw, d); });
    }
    auto invalid = fixture(true);
    std::string sentinel;
    u32(sentinel, 0xdeadbeef);
    const auto pos = invalid.find(sentinel);
    check(pos != std::string::npos, "Missing test sentinel");
    invalid[pos] = 0;
    rejects([&] { observe(invalid, true); });
    invalid = fixture(false);
    invalid[0] = 2;
    rejects([&] { observe(invalid, false); });
    const auto bytes = fixture(true);
    std::istringstream s(bytes);
    runtime::BinaryReader r(s, bytes.size());
    rejects([&] { saves::observe_teams(r, vp, small); });
    // Rules collection spans stay wide, preserve holes/duplicates and are
    // obtained from the selected database rather than the installed MODS folder.
    content::RulesDatabase rules;
    for (auto name :
         {"VoteSources", "SmallAwards", "Technologies", "Routes", "Builds", "Projects", "UnitClasses",
          "BuildingClasses", "Buildings", "Terrains", "Features", "Improvements", "Victories"})
        rules.apply_sql(std::string("CREATE TABLE ") + name + "(ID); INSERT INTO " + name +
                        " VALUES(-1),(0),(2),(2)");
    auto dims = content::vp_team_content_dimensions(rules);
    check(dims.projects == 4 && dims.techs == 4 && dims.improvements == 4,
          "Team source span collapsed duplicates or holes");
    rules.apply_sql("INSERT INTO Technologies VALUES(4294967297)");
    check(content::vp_team_content_dimensions(rules).techs == 4294967298ull, "Team source span narrowed");
    rules.apply_sql("INSERT INTO Builds VALUES(2.5)");
    check(content::vp_team_content_dimensions(rules).builds == 5,
          "Team source collection rejected a real ID coerced to integer");
    class ShiftedTeamStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t position_ = 0;
        pos_type seekpos(pos_type p, std::ios_base::openmode) override {
            position_ = static_cast<std::uint64_t>(off_type(p));
            return p;
        }
        std::streamsize xsgetn(char *out, std::streamsize count) override {
            const auto local = position_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(count) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, count, out);
            position_ += static_cast<std::uint64_t>(count);
            return count;
        }

      public:
        explicit ShiftedTeamStream(const std::string &bytes) : bytes_(bytes) {}
    };
    ShiftedTeamStream backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader shifted(stream, 0x100000123ull + bytes.size());
    shifted.seek(0x100000123ull);
    auto d = small;
    d.profile_extensions.emplace<saves::VpTeamContentDimensions>(rows);
    const auto m = saves::observe_teams(shifted, vp, d);
    check(m.start_offset == 0x100000123ull && m.teams[0].end_offset == 0x100000123ull + bytes.size(),
          "Team stream position narrowed");
}
