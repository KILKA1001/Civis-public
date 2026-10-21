#pragma once
void game_religion_culture_tests() {
    using saves::GameStateLayout;
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, versions, counts, lookups;
        std::size_t culture_offset = 0;
    };
    const auto fixture = [](bool vp, unsigned system_version = 4, unsigned culture_version = 2,
                            unsigned religions = 4) {
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
            version(system_version);
        if (vp || system_version >= 3)
            u32(f.bytes, 0x80000000u);
        if (!vp && system_version < 4)
            u32(f.bytes, 0xffffffffu);
        if (vp || system_version >= 2) {
            count(religions);
            for (unsigned i = 0; i < religions; ++i) {
                const auto rv = i % 4 + 1, bv = i % 2 + 1;
                if (!vp)
                    version(rv);
                for (auto n : {0xffffffffu, 9000u, 0x80000000u, 0x7fffffffu, 0xffffffffu})
                    u32(f.bytes, n);
                if (vp || rv >= 2)
                    flag(true);
                if (vp || rv >= 4)
                    flag(true);
                if (vp || rv >= 3)
                    f.bytes.append(128, char(0xff));
                if (vp) {
                    flag(true);
                    u32(f.bytes, 123);
                    count(3);
                    for (auto n : {0xffffffffu, 0x7fffffffu, 7u})
                        u32(f.bytes, n);
                    f.lookups.push_back(f.bytes.size());
                    count(4);
                    for (auto n : {1u, 0xffffffffu, 0u, 2u})
                        u32(f.bytes, n);
                } else {
                    version(bv);
                    for (unsigned j = 0; j < 26; ++j)
                        if (j != 22 || bv >= 2)
                            u32(f.bytes, 0x80000000u + j);
                    count(3);
                    for (auto n : {0xffffffffu, 0u, 0xffffffffu})
                        u32(f.bytes, n);
                    count(3);
                    u32(f.bytes, 0xfedcba98u);
                    u32(f.bytes, 0x80000000u);
                    u32(f.bytes, 0);
                    u32(f.bytes, 0xfedcba98u);
                    u32(f.bytes, 7);
                }
            }
        }
        f.culture_offset = f.bytes.size();
        if (!vp)
            version(culture_version);
        count(3);
        for (unsigned i = 0; i < 3; ++i) {
            if (!vp) {
                version(i + 1);
                if (i == 0)
                    string(f.bytes, std::string("\0\xffold", 5));
            }
            string(f.bytes, std::string(300, char(0xff)));
            u32(f.bytes, 0x7fffffffu);
            if (vp || i == 2)
                u32(f.bytes, 0xffffffffu);
            for (auto n : {0x80000000u, 0xffffffffu, 9000u})
                u32(f.bytes, n);
        }
        if (vp || culture_version >= 2)
            flag(true);
        return f;
    };
    const auto observe = [](const std::string &bytes, GameStateLayout layout,
                            saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_game_religion_culture(reader, layout, budget);
    };
    const auto replace32 = [](std::string &bytes, std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes[offset + i] = char(value >> (8 * i));
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto layout = vp ? GameStateLayout::vp542_corpus : GameStateLayout::expansion2_version1;
        const auto state = observe(f.bytes, layout);
        check(state.start_offset == 0 && state.culture_offset == f.culture_offset &&
                  state.end_offset == f.bytes.size(),
              "Religion/culture boundaries drifted");
        check(state.minimum_faith_next_pantheon == INT32_MIN && !state.obsolete_minimum_faith_next_prophet &&
                  state.religions.size() == 4 && state.great_works.size() == 3 &&
                  state.reported_someone_influential,
              "Religion/culture counts or source values normalized");
        for (std::size_t i = 0; i < 4; ++i) {
            const auto &religion = state.religions[i];
            check(religion.religion == -1 && religion.founder == 9000 && religion.holy_city_x == INT32_MIN &&
                      religion.holy_city_y == INT32_MAX && religion.turn_founded == -1,
                  "Pantheon coordinates or wide host IDs sanitized");
            check(religion.pantheon == (vp || i >= 1) && religion.enhanced == (vp || i == 3) &&
                      religion.custom_name == std::vector<std::uint8_t>(vp || i >= 2 ? 128 : 0, 255),
                  "Raw fixed name truncated or legacy fields invented");
            const auto &beliefs = religion.beliefs;
            if (vp) {
                check(!religion.version && religion.reformed == true && !beliefs.version &&
                          beliefs.religion == 123 && beliefs.cached_values.empty() &&
                          beliefs.building_class_enabled.empty() && beliefs.belief_hashes.empty() &&
                          beliefs.belief_ids == std::vector<std::int64_t>{-1, INT32_MAX, 7} &&
                          beliefs.belief_lookup == std::vector<std::int64_t>{1, -1, 0, 2},
                      "VP beliefs coerced or narrowed");
            } else {
                check(religion.version == i + 1 && !religion.reformed && beliefs.version == i % 2 + 1 &&
                          !beliefs.religion && beliefs.belief_ids.empty() && beliefs.belief_lookup.empty(),
                      "BNW belief layout metadata lost");
                check(beliefs.cached_values.size() == 26 && beliefs.cached_values[0].value == INT32_MIN &&
                          beliefs.cached_values[22].value == (i % 2 ? std::int64_t(INT32_MIN) + 22 : 0) &&
                          beliefs.cached_values[25].value == std::int64_t(INT32_MIN) + 25 &&
                          std::string_view(beliefs.cached_values[22].name) == "faith_building_tourism" &&
                          std::string_view(beliefs.cached_values[25].name) == "spread_modifier_doubling_tech",
                      "Serialized belief effects or legacy defaults changed");
                check(beliefs.belief_hashes == std::vector<std::uint64_t>{UINT32_MAX, 0, UINT32_MAX} &&
                          beliefs.building_class_enabled.size() == 3 &&
                          beliefs.building_class_enabled[0].type_hash == 0xfedcba98ull &&
                          beliefs.building_class_enabled[0].value == INT32_MIN &&
                          !beliefs.building_class_enabled[1].value &&
                          beliefs.building_class_enabled[2].value == 7,
                      "Belief hashes, duplicates or building holes lost");
            }
        }
        for (std::size_t i = 0; i < 3; ++i) {
            const auto &work = state.great_works[i];
            check(work.creator == std::vector<std::uint8_t>(300, 255) && work.type == INT32_MAX &&
                      work.turn_founded == INT32_MIN && work.era == -1 && work.player == 9000,
                  "Great work creator, identity, timing or owner lost");
            check(vp || i == 2 ? work.class_type == -1 : !work.class_type,
                  "Missing great work class guessed");
            if (!vp && i == 0)
                check(work.legacy_name == std::vector<std::uint8_t>{0, 255, 'o', 'l', 'd'},
                      "Legacy work name discarded");
            else
                check(!work.legacy_name, "Invented legacy name");
        }
        check(vp ? !state.religions_version && !state.culture_version
                 : state.religions_version == 4 && state.culture_version == 2,
              "System versions lost");
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
        rejects([&] { observe(f.bytes, layout, {2, 1000, 10000}); });
        rejects([&] { observe(f.bytes, layout, {1000, 127, 10000}); });
        rejects([&] { observe(f.bytes, layout, {1000, 1000, 200}); });
        rejects([&] { observe(f.bytes, static_cast<GameStateLayout>(99)); });
        check(observe(f.bytes + "leagues", layout).end_offset == f.bytes.size(),
              "Religion/culture read consumed leagues");
    }
    for (unsigned version = 1; version <= 4; ++version) {
        const auto f = fixture(false, version, 1);
        const auto state = observe(f.bytes, GameStateLayout::expansion2_version1);
        check(state.religions.size() == (version >= 2 ? 4u : 0u) && !state.reported_someone_influential &&
                  state.minimum_faith_next_pantheon == (version >= 3 ? INT32_MIN : 0) &&
                  (version < 4 ? state.obsolete_minimum_faith_next_prophet == -1
                               : !state.obsolete_minimum_faith_next_prophet),
              "Old religion/culture system layout or discarded field lost");
    }
    check(observe(fixture(false, 4, 2, 65).bytes, GameStateLayout::expansion2_version1).religions.size() ==
              65,
          "Vanilla static religion capacity leaked into host");
    const auto f = fixture(true);
    auto wide = f.bytes;
    replace32(wide, f.lookups[0], 65537);
    wide.insert(f.lookups[0] + 4 + 16, 4 * (65537 - 4), '\0');
    check(observe(wide, GameStateLayout::vp542_corpus).religions[0].beliefs.belief_lookup.size() == 65537,
          "Belief lookup capacity narrowed");
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
    const auto moved = saves::observe_game_religion_culture(reader, GameStateLayout::vp542_corpus);
    check(moved.start_offset == 0x100000123ull && moved.end_offset == moved.start_offset + wide.size() &&
              moved.culture_offset > UINT32_MAX,
          "Religion/culture stream offsets narrowed");

    const auto legacy = observe(fixture(false).bytes, GameStateLayout::expansion2_version1).great_works;
    content::RulesDatabase rules;
    check(content::saved_great_work_class(rules, legacy[2]) == -1,
          "Saved explicit class was replaced by a rules lookup");
    rejects([&] { content::saved_great_work_class(rules, legacy[0]); });
    rules.apply_sql(
        "CREATE TABLE GreatWorkClasses(ID INTEGER PRIMARY KEY,Type TEXT);"
        "CREATE TABLE GreatWorks(ID INTEGER PRIMARY KEY,GreatWorkClassType TEXT);"
        "INSERT INTO GreatWorkClasses VALUES(4294967298,'CLASS_A'),(9223372036854775807,'CLASS_MOD');"
        "INSERT INTO GreatWorks VALUES(2147483647,'CLASS_A'),(4294967300,'CLASS_MOD');");
    check(content::saved_great_work_class(rules, legacy[0]) == 4294967298ll && !legacy[0].class_type,
          "Legacy great work class narrowed or source observation mutated");
    check(rules.apply_mod_sql("UPDATE GreatWorks SET GreatWorkClassType='CLASS_MOD' WHERE ID=2147483647;")
              .success,
          "Mod rule action failed");
    check(content::saved_great_work_class(rules, legacy[1]) == INT64_MAX,
          "Legacy class lookup ignored mod rules");
    auto work = legacy[0];
    work.type = 4294967300ll;
    check(content::saved_great_work_class(rules, work) == INT64_MAX, "Great work lookup narrowed bound ID");
    work.type = 42;
    check(!content::saved_great_work_class(rules, work), "Missing class became an invented default");
}
