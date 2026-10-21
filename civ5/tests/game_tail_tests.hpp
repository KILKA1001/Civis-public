#pragma once
void game_tail_tests() {
    using saves::GameStateLayout;
    const auto fixture = [](unsigned corporations = 2) {
        std::string out;
        u32(out, corporations);
        for (unsigned i = 0; i < corporations; ++i)
            for (auto v : {0xffffffffu, 70000u, 0x80000000u, 0x7fffffffu, 65537u})
                u32(out, v);
        for (unsigned group = 0; group < 2; ++group) {
            u32(out, 2);
            for (unsigned i = 0; i < 2; ++i)
                for (auto v : {90000u, 70000u, 0xffffffffu, 0x80000000u, 100u + group})
                    u32(out, v);
        }
        string(out, std::string(300, char(0xff)));
        return out;
    };
    const auto final = [](const std::string &bytes, GameStateLayout layout = GameStateLayout::vp542_corpus,
                          saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_game_final_systems(reader, layout, budget);
    };
    const auto f = fixture();
    const auto s = final(f);
    check(s.start_offset == 0 && s.end_offset == f.size() && s.vp && s.vp->corporations.size() == 2 &&
              s.vp->game_id == std::vector<std::uint8_t>(300, 255),
          "Final systems boundaries or opaque ID changed");
    for (const auto &c : s.vp->corporations)
        check(c.type == -1 && c.founder == 70000 && c.headquarters_x == INT32_MIN &&
                  c.headquarters_y == INT32_MAX && c.turn_founded == 65537,
              "Corporation source values altered");
    for (auto *group : {&s.vp->active_contracts, &s.vp->inactive_contracts}) {
        check(group->size() == 2, "Contracts deduplicated");
        for (const auto &c : *group)
            check(c.type == 90000 && c.holder == 70000 && c.turns == -1 && c.maintenance == INT32_MIN &&
                      c.turn_start == (group == &s.vp->active_contracts ? 100 : 101),
                  "Contract field order changed");
    }
    for (std::size_t n = 0; n < f.size(); ++n)
        rejects([&] { final(f.substr(0, n)); });
    rejects([&] { final(f, GameStateLayout::vp542_corpus, {1, 1000, 1000}); });
    rejects([&] { final(f, GameStateLayout::vp542_corpus, {1000, 299, 1000}); });
    rejects([&] { final(f, GameStateLayout::vp542_corpus, {1000, 1000, 299}); });
    rejects([&] { final(f, static_cast<GameStateLayout>(99)); });
    auto corrupt = f;
    std::fill_n(corrupt.begin(), 4, char(0xff));
    rejects([&] { final(corrupt); });
    const auto bnw = final("database", GameStateLayout::expansion2_version1);
    check(!bnw.vp && bnw.start_offset == 0 && bnw.end_offset == 0, "Invented BNW corporation segment");
    check(final(f + "database").end_offset == f.size(), "Final systems consumed database size");
    check(final(fixture(65537)).vp->corporations.size() == 65537,
          "Original corporation capacity leaked into host");

    // Independent minimal SQLite file: one empty table btree page.
    std::string image(512, '\0');
    image.replace(0, 16, std::string("SQLite format 3\0", 16));
    const unsigned char fields[]{2, 0, 1, 1, 0, 64, 32, 32};
    for (std::size_t i = 0; i < 8; ++i)
        image[16 + i] = char(fields[i]);
    const auto be32 = [](std::string &bytes, std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes[offset + i] = char(value >> (24 - 8 * i));
    };
    for (auto at : {24u, 28u, 40u, 56u, 92u})
        be32(image, at, 1);
    be32(image, 44, 4);
    be32(image, 96, 3053004);
    image[100] = 13;
    image[105] = 2;
    std::string db;
    u32(db, 512);
    db += image;
    const auto database = [](const std::string &bytes, std::uint64_t budget = 256 * 1024 * 1024,
                             bool map_diagnostic = false) {
        std::istringstream stream(bytes);
        runtime::BinaryReader reader(stream, bytes.size());
        return saves::observe_saved_database(reader, budget, map_diagnostic);
    };
    const auto observed = database(db);
    check(observed.start_offset == 0 && observed.data_offset == 4 && observed.end_offset == db.size() &&
              observed.declared_image_bytes == 512 && !observed.non_sqlite_image_skipped &&
              observed.quick_check_passed &&
              std::string(observed.image.begin(), observed.image.end()) == image,
          "SQLite image validation changed original bytes");
    for (std::size_t n = 0; n < db.size(); ++n)
        rejects([&] { database(db.substr(0, n)); });
    rejects([&] { database(db, 511); });
    auto bad_image = db;
    bad_image[104] = 0;
    rejects([&] { database(bad_image); });
    std::string short_db;
    u32(short_db, 16);
    short_db += "SQLite format 3";
    short_db.append(2, '\0');
    rejects([&] { database(short_db); });
    std::string missing;
    u32(missing, 22528);
    missing.append(22528, '\0');
    // Model the discovered failure: declared DB size followed by map fields.
    missing[4] = char(180);
    missing[8] = 94;
    std::istringstream broken_stream(missing);
    runtime::BinaryReader broken(broken_stream, missing.size());
    rejects([&] { saves::observe_saved_database(broken); });
    check(broken.position() == 20, "Invalid DB signature swallowed following map bytes");
    const auto skipped = database(missing, 256 * 1024 * 1024, true);
    check(skipped.declared_image_bytes == 22528 && skipped.non_sqlite_image_skipped &&
              skipped.start_offset == 0 && skipped.data_offset == 4 && skipped.end_offset == 4 &&
              skipped.image.empty() && !skipped.quick_check_passed,
          "Map diagnostic consumed non-SQLite bytes as a saved database");
    check(database(db, 256 * 1024 * 1024, true).quick_check_passed,
          "Map diagnostic bypassed a valid saved SQLite image");
    std::string empty;
    u32(empty, 0);
    const auto absent = database(empty, 0);
    check(absent.image.empty() && !absent.quick_check_passed && absent.end_offset == 4,
          "Missing DB became a synthesized SQLite image");
    check(database(db + "map").end_offset == db.size(), "SQLite read consumed map prefix");

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
    };
    // Keep the backing bytes alive through both reads.
    const auto combined = f + db;
    ShiftedStream stable(combined);
    std::istream stream(&stable);
    runtime::BinaryReader reader(stream, 0x100000123ull + combined.size());
    reader.seek(0x100000123ull);
    const auto final_shifted = saves::observe_game_final_systems(reader, GameStateLayout::vp542_corpus);
    const auto db_shifted = saves::observe_saved_database(reader);
    check(final_shifted.start_offset == 0x100000123ull &&
              db_shifted.start_offset == final_shifted.end_offset &&
              db_shifted.end_offset == 0x100000123ull + combined.size() && db_shifted.quick_check_passed,
          "Final systems/database stream position narrowed");
}
