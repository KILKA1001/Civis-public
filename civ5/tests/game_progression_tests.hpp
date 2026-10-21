#pragma once
// Uses the runtime test harness helpers in runtime_tests.cpp.
void game_progression_tests() {
    using saves::GameStateLayout;
    using saves::ProgressionWireDimensions;
    ProgressionWireDimensions vp_dimensions{3, 2};
    vp_dimensions.profile_extensions.emplace<saves::VpGameProgressionWireDimensions>(
        saves::VpGameProgressionWireDimensions{{3, 2, 1, 0, 3, 2, 1}});
    const ProgressionWireDimensions bnw_dimensions{3, 2};
    check(!saves::vp_game_progression_dimensions(bnw_dimensions), "BNW requires VP progression dimensions");
    const auto fixture = [](bool vp) {
        std::string bytes;
        if (vp) {
            // Three units, two unit classes, one building class, no projects.
            for (auto n : {0x80000000u, 0xffffffffu, 7u, 2u, 5u, 9u})
                u32(bytes, n);
        } else {
            // Unknown hash, a hole (no value), and a repeated hash. Do not
            // collapse identities, consume values for holes or resolve IDs here.
            for (int i = 0; i < 8; ++i) {
                u32(bytes, 3);
                u32(bytes, 0xfedcba98u);
                u32(bytes, 0x80000000u);
                u32(bytes, 0);
                u32(bytes, 0xfedcba98u);
                u32(bytes, 7);
            }
        }
        for (int i = 0; i < 9; ++i)
            u32(bytes, 0xffffffffu - i);
        if (!vp) {
            u32(bytes, 3);
            u32(bytes, 0x80000000u);
        }
        bytes += '\1';
        if (!vp)
            u32(bytes, 0);
        if (vp)
            bytes += '\0';
        if (!vp)
            u32(bytes, 0xffffffffu);
        bytes += '\1';
        if (!vp) {
            u32(bytes, 3);
            u32(bytes, 0);
            u32(bytes, 0xffffffffu);
        }
        u32(bytes, 0xffffffffu);
        u32(bytes, 13);
        if (!vp)
            u32(bytes, 0x80000000u);
        u32(bytes, 17);
        u32(bytes, 0x80000000u);
        if (vp) {
            u32(bytes, 4);
            u32(bytes, 8);
            u32(bytes, 15);
        }
        for (int i = 0; i < 2; ++i) {
            u32(bytes, 2);
            if (vp) {
                u32(bytes, 0);
                u32(bytes, 0xffffffffu);
            } else {
                string(bytes, std::string("\0\xffX", 3));
                string(bytes, "");
            }
        }
        if (!vp) {
            for (int list = 0; list < 2; ++list) {
                for (auto n : {2u, 0u, 1u, 1u, 0x80000000u, 0xffffffffu, 0xffffffffu, 1u})
                    u32(bytes, n);
                u32(bytes, 1);
                u32(bytes, 0x80002000u);
                u32(bytes, 0xffffffffu);
                if (!list)
                    u32(bytes, 2);
                for (int j = 0; j < (list ? 1 : 2); ++j) {
                    for (auto n : {0x80000000u, 71u, 0xffffffffu, 99u})
                        u32(bytes, n);
                    string(bytes, std::string("\0\xffvote", 6));
                }
            }
        }
        for (int i = 0; i < 2; ++i) {
            if (!vp)
                u32(bytes, 1);
            u32(bytes, i ? 0x01234567u : 0xffffffffu);
            if (vp)
                u32(bytes, i ? 0x89abcdefu : 0xffffffffu);
            u32(bytes, 0xffffffffu);
            u32(bytes, 0x80000000u);
            if (!vp)
                bytes += '\0';
        }
        return bytes;
    };
    const auto observe = [](const std::string &bytes, GameStateLayout layout,
                            ProgressionWireDimensions dimensions, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_game_progression(reader, layout, dimensions, budget);
    };
    for (bool vp : {false, true}) {
        const auto bytes = fixture(vp);
        const auto layout = vp ? GameStateLayout::vp542_corpus : GameStateLayout::expansion2_version1;
        const auto dimensions = vp ? vp_dimensions : bnw_dimensions;
        const auto state = observe(bytes, layout, dimensions);
        check(state.start_offset == 0 && state.end_offset == bytes.size() &&
                  state.random_offset == bytes.size() - (vp ? 32 : 34),
              "Progression boundary drift");
        check(state.arrays.size() == (vp ? 10u : 13u), "Wrong progression sections");
        check(state.arrays.front().rows[0].values[0] == -2147483648ll, "Created count lost sign");
        const auto &special = state.arrays[vp ? 7 : 11];
        check(special.rows.size() == 3 && special.rows.front().values[0] == 1 &&
                  special.rows.back().values[0] == 1,
              "Special-unit booleans lost");
        check(state.random[0].seed == (vp ? UINT64_MAX : UINT32_MAX) && state.random[0].calls == UINT32_MAX &&
                  state.random[0].resets == 0x80000000ull &&
                  state.random[1].seed == (vp ? 0x89abcdef01234567ull : 0x01234567ull),
              "RNG state narrowed, reseeded or advanced");
        if (vp) {
            check(!state.vote_free_lists && !state.arrays[0].rows[0].type_hash &&
                      state.arrays.back().columns == 3 && state.arrays.back().rows[0].values[2] == 15,
                  "VP raw arrays or contract matrix lost");
            check(state.destroyed_cities.hashes == std::vector<std::uint64_t>{0, UINT32_MAX} &&
                      state.great_people.strings.empty(),
                  "VP name hashes lost");
        } else {
            check(*state.arrays[0].rows[0].type_hash == 0xfedcba98ull &&
                      state.arrays[0].rows[1].type_hash == 0 && state.arrays[0].rows[1].values.empty() &&
                      state.arrays[0].rows[2].values[0] == 7,
                  "Vanilla hash holes lost alignment");
            check(state.destroyed_cities.strings ==
                          std::vector<std::vector<std::uint8_t>>{{0, 255, 'X'}, {}} &&
                      state.great_people.hashes.empty(),
                  "Vanilla opaque names lost");
            const auto &selection = (*state.vote_free_lists)[0];
            const auto &trigger = (*state.vote_free_lists)[1];
            check(selection.next_free == std::vector<std::int64_t>{-1, -1} &&
                      selection.current_id == -2147483648ll && selection.entries[0].options.size() == 2 &&
                      trigger.entries[0].options.size() == 1 && trigger.entries[0].id == -2147475456ll &&
                      trigger.entries[0].source == -1 && selection.entries[0].options[1].city == -1 &&
                      selection.entries[0].options[1].text ==
                          std::vector<std::uint8_t>{0, 255, 'v', 'o', 't', 'e'},
                  "Vote free-list payload lost or used as native allocation metadata");
        }
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), layout, dimensions); });
        rejects([&] { observe(bytes, layout, dimensions, {4, 100, 100}); });
        auto wrong = dimensions;
        wrong.vote_team_slots = UINT64_MAX;
        rejects([&] { observe(bytes, layout, wrong, {UINT64_MAX, 100, 100}); });
        wrong = dimensions;
        wrong.victory_award_slots = UINT64_MAX;
        rejects([&] { observe(bytes, layout, wrong, {UINT64_MAX, 100, 100}); });
        wrong = dimensions;
        wrong.vote_team_slots = 0;
        rejects([&] { observe(bytes, layout, wrong); });
        wrong = dimensions;
        wrong.profile_extensions = vp ? runtime::ProfileExtensions{} : vp_dimensions.profile_extensions;
        rejects([&] { observe(bytes, layout, wrong); });
        auto bad = bytes;
        bad[special.start_offset + (vp ? 0 : 8)] = 2;
        rejects([&] { observe(bad, layout, dimensions); });
        if (!vp) {
            rejects([&] { observe(bytes, layout, dimensions, {1000, 2, 100}); });
            rejects([&] { observe(bytes, layout, dimensions, {1000, 100, 1}); });
            bad = bytes;
            bad[state.random_offset] = 2;
            rejects([&] { observe(bad, layout, dimensions); });
            bad = bytes;
            bad[state.random_offset + 16] = 1;
            rejects([&] { observe(bad, layout, dimensions); });
            bad = bytes;
            bad[0] = char(0xff);
            bad[1] = char(0xff);
            bad[2] = char(0xff);
            bad[3] = char(0xff);
            rejects([&] { observe(bad, layout, dimensions); });
        }
        // End at RNG, leaving replay bytes to the next decoder.
        const auto trailing = observe(bytes + "next", layout, dimensions);
        check(trailing.end_offset == bytes.size(), "Progression consumed replay bytes");
    }
    // Reader offsets and collection capacities have no original x86 ceiling.
    auto bytes = fixture(true);
    auto dimensions = vp_dimensions;
    dimensions.vote_team_slots = 8193;
    bytes.insert(24, 3 * (8193 - 3) * 4, '\0');
    const auto wide = observe(bytes, GameStateLayout::vp542_corpus, dimensions);
    check(wide.arrays[4].rows.size() == 8193, "Native vote-team capacity leaked into host");
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
    } shifted(bytes);
    std::istream stream(&shifted);
    runtime::BinaryReader reader(stream, 0x100000123ull + bytes.size());
    reader.seek(0x100000123ull);
    const auto moved = saves::observe_game_progression(reader, GameStateLayout::vp542_corpus, dimensions);
    check(moved.start_offset == 0x100000123ull && moved.end_offset == moved.start_offset + bytes.size(),
          "Progression truncated a 64-bit offset");

    content::RulesDatabase rules;
    for (const auto *table : {"Resources", "Units", "UnitClasses", "BuildingClasses", "Projects",
                              "SpecialUnits", "Victories", "Contracts"})
        rules.apply_sql(std::string("CREATE TABLE ") + table + "(ID INTEGER PRIMARY KEY);");
    rules.apply_sql(
        "CREATE TABLE Defines(Name, Value); INSERT INTO Defines VALUES('NUM_VICTORY_POINT_AWARDS',5);"
        "INSERT INTO Units VALUES(-1),(0),(8); INSERT INTO Resources VALUES(4294967297);"
        "INSERT INTO UnitClasses VALUES(9223372036854775807);"
        "CREATE TABLE Remapper(name); INSERT INTO Remapper VALUES('Units');");
    auto context = content::vp_save_wire_dimensions(rules, 4097, 8193);
    check(saves::vp_roster_dimensions(context.roster)->resource_slots == 4294967298ull &&
              context.roster.player_slots == 4097 &&
              context.progression.vote_team_slots == 8193 &&
              saves::vp_game_progression_dimensions(context.progression)->rows[0] == 9 &&
              saves::vp_game_progression_dimensions(context.progression)->rows[1] == 0x8000000000000000ull &&
              context.progression.victory_award_slots == 5,
          "Rules spans counted rows, narrowed IDs or slots");
    // A negative first ID stops remapping. Removing it permits compaction.
    check(rules.remap_vp_ids().all_succeeded, "Remapper failed");
    check(saves::vp_game_progression_dimensions(content::vp_save_wire_dimensions(rules, 64, 64).progression)->rows[0] == 9,
          "Negative-ID behavior lost");
    rules.apply_sql("DELETE FROM Units WHERE ID=-1;");
    check(rules.remap_vp_ids().all_succeeded, "Remapper failed after removing sentinel");
    check(saves::vp_game_progression_dimensions(content::vp_save_wire_dimensions(rules, 64, 64).progression)->rows[0] == 2,
          "Dimensions ignored remapped rules");
    rules.apply_sql(
        "DROP TABLE Projects; CREATE TABLE Projects(ID); INSERT INTO Projects VALUES(0),(0),(5);");
    check(saves::vp_game_progression_dimensions(content::vp_save_wire_dimensions(rules, 64, 64).progression)->rows[3] == 6,
          "Duplicate source ID append lost");
    rules.apply_sql("INSERT INTO Projects VALUES(5);");
    check(saves::vp_game_progression_dimensions(content::vp_save_wire_dimensions(rules, 64, 64).progression)->rows[3] == 7,
          "Duplicate last ID append lost");
    rejects([&] { content::vp_save_wire_dimensions(rules, 0, 64); });
    rules.apply_sql("INSERT INTO Projects VALUES('noninteger');");
    check(saves::vp_game_progression_dimensions(content::vp_save_wire_dimensions(rules, 64, 64).progression)->rows[3] == 8,
          "Source collection did not append a text ID coerced to zero");
    rules.apply_sql("DELETE FROM Projects WHERE typeof(ID)='text'; UPDATE Defines SET Value=0;");
    rejects([&] { content::vp_save_wire_dimensions(rules, 64, 64); });
    rules.apply_sql("DELETE FROM Defines;");
    rejects([&] { content::vp_save_wire_dimensions(rules, 64, 64); });
    rules.apply_sql(
        "INSERT INTO Defines VALUES('NUM_VICTORY_POINT_AWARDS',5),('NUM_VICTORY_POINT_AWARDS',5);");
    rejects([&] { content::vp_save_wire_dimensions(rules, 64, 64); });
}
