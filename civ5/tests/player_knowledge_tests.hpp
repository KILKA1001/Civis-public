#pragma once
void player_knowledge_tests() {
    constexpr auto vp = saves::GameStateLayout::vp542_corpus,
                   bnw = saves::GameStateLayout::expansion2_version1;
    using Dims = saves::VpPlayerKnowledgeWireDimensions;
    const Dims dims{3, 2};
    const auto fixture = [&](bool is_vp, Dims d, std::uint64_t count = 3, bool negative = false,
                             bool sentinel = false) {
        std::string s;
        std::vector<std::uint64_t> versions;
        const auto version = [&] {
            versions.push_back(s.size());
            u32(s, 1);
        };
        const auto ints = [&](std::uint64_t n) {
            for (std::uint64_t i = 0; i < n; ++i)
                u32(s, i % 3 == 0 ? 0x80000000 : i % 3 == 1 ? UINT32_MAX : 70000);
        };
        if (is_vp) {
            u32(s, 0);
            u32(s, 4);
            s += char(1);
            u32(s, 12);
            u32(s, UINT32_MAX);
            u32(s, 12);
            s += char(1);
            u32(s, 123456);
        } else {
            version();
            u32(s, 4);
            u32(s, 2);
            version();
        }
        if (is_vp) {
            ints(10);
            u32(s, sentinel ? UINT32_MAX : static_cast<std::uint32_t>(count));
            if (!sentinel)
                ints(count);
            for (std::uint64_t i = 0; i < d.techs; ++i)
                s += char(i % 2);
            ints(7 * d.techs);
        } else {
            version();
            // Seven hashed technology arrays. Zero has no value; unknown and
            // repeated nonzero hashes keep their data, including UINT32_MAX.
            for (unsigned a = 0; a < 7; ++a) {
                u32(s, 4);
                for (auto h : {0u, UINT32_MAX, 0x12345678u, 0x12345678u}) {
                    u32(s, h);
                    if (h) {
                        if (!a)
                            s += char(h == UINT32_MAX);
                        else
                            u32(s, h == UINT32_MAX ? 0x80000000 : 70000);
                    }
                }
            }
            version();
        }
        u32(s, negative ? 0xfffffff9 : static_cast<std::uint32_t>(count));
        if (!negative) {
            for (std::uint64_t i = 0; i < count; ++i) {
                u32(s, i % 3 == 0 ? 0 : i % 3 == 1 ? UINT32_MAX : 70000);
                u32(s, i % 3 == 0 ? 0x80000000 : i % 3 == 1 ? UINT32_MAX : 123456);
            }
        }
        if (!is_vp)
            u32(s, negative ? 0xfffffff7 : static_cast<std::uint32_t>(d.flavors));
        if (is_vp || !negative)
            ints(d.flavors);
        if (is_vp) {
            s += char(1);
            s += char(0);
        } else {
            version();
            u32(s, negative ? 0xfffffffb : static_cast<std::uint32_t>(d.flavors));
        }
        if (is_vp || !negative)
            ints(d.flavors * 2);
        return std::pair{s, versions};
    };
    const auto observe = [&](const std::string &s, bool is_vp, Dims d = Dims{3, 2},
                             saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(s);
        runtime::BinaryReader in(stream, s.size());
        saves::PlayerKnowledgeDecodeContext context;
        if (is_vp)
            context.profile_extensions.emplace<Dims>(d);
        return saves::observe_player_knowledge(in, is_vp ? vp : bnw, std::move(context), budget);
    };
    const auto field = [](const auto &b, std::string_view name) {
        for (const auto &f : b.fields)
            if (name == f.name)
                return f.value;
        throw std::runtime_error("Missing player knowledge field");
    };
    const auto array = [](const auto &b, std::string_view name) -> const saves::PlotArrayObservation & {
        for (const auto &a : b.arrays)
            if (name == a.name)
                return a;
        throw std::runtime_error("Missing player knowledge array");
    };
    for (bool is_vp : {false, true}) {
        const auto [bytes, versions] = fixture(is_vp, dims);
        const auto p = observe(bytes, is_vp);
        const auto &tech = p.blocks[is_vp ? 3 : 2];
        check(p.end_offset == bytes.size() && p.blocks.size() == (is_vp ? 5 : 4),
              "Player knowledge block order/boundary");
        check(std::string_view(p.blocks.back().name) == "flavor_manager" &&
                  p.blocks.back().end_offset == p.end_offset,
              "Flavor manager boundary");
        check(field(p.blocks[0], "prophets_spawned") == 4 &&
                  field(p.blocks[0], "founding_religion") == (is_vp ? 1 : 2),
              "BNW religion integer normalized to bool");
        check(array(tech, "latest_flavors").values == std::vector<std::int64_t>{INT32_MIN, -1},
              "Latest flavor values changed");
        if (is_vp) {
            check(p.blocks[0].end_offset == 26 && p.blocks[1].start_offset == 26 &&
                      p.blocks[1].end_offset == 26,
                  "Empty VP ReligionAI consumed bytes");
            check(field(p.blocks[0], "state_religion_override") == -1 &&
                      field(p.blocks[0], "faith_at_last_notify_times100") == 123456,
                  "Religion units or sentinel changed");
            check(array(p.blocks[2], "franchises_per_improvement").values ==
                      std::vector<std::int64_t>{INT32_MIN, -1, 70000},
                  "Corporation improvement IDs narrowed");
            check(array(tech, "tech_ai_weights").values ==
                      std::vector<std::int64_t>{0, INT32_MIN, -1, -1, 70000, 123456},
                  "VP TechAI IDs or weights normalized");
            check(array(tech, "grand_strategy_tech_priority").values.size() == 3 &&
                      field(tech, "has_unique_unit_tech") == 1,
                  "VP technology fields omitted");
            auto bad = bytes;
            bad[8] = char(2);
            rejects([&] { observe(bad, true); });
            bad = bytes;
            bad[tech.end_offset - 2] = char(2);
            rejects([&] { observe(bad, true); });
            const auto empty = observe(fixture(true, dims, 3, false, true).first, true);
            check(field(empty.blocks[2], "franchises_per_improvement_wire_count") == UINT32_MAX &&
                      array(empty.blocks[2], "franchises_per_improvement").values.empty(),
                  "VP corporation STL sentinel changed");
        } else {
            check(p.blocks[0].end_offset == 12 && p.blocks[1].end_offset == 16,
                  "BNW religion integer width changed");
            const auto &h = tech.hashes.front();
            check(h.rows.size() == 4 && h.rows[0].values.empty() && h.rows[1].type_hash == UINT32_MAX &&
                      h.rows[1].values[0] == 1,
                  "BNW hashed technology holes changed");
            check(h.rows[2].type_hash == h.rows[3].type_hash, "Repeated technology hash lost");
            check(tech.hashes.back().rows[0].type_hash == 0 &&
                      tech.hashes.back().rows[0].values[0] == INT32_MIN,
                  "BNW TechAI zero hash incorrectly skipped weight");
            check(tech.hashes.back().rows[1].type_hash == UINT32_MAX &&
                      tech.hashes.back().rows[1].values[0] == -1,
                  "Unknown TechAI weight discarded");
            auto bad = bytes;
            bad[32] = char(2);
            rejects([&] { observe(bad, false); });
            for (auto offset : versions) {
                auto wrong = bytes;
                wrong[offset] = char(2);
                rejects([&] { observe(wrong, false); });
            }
            auto invalid = bytes;
            invalid[0] = 0;
            rejects([&] { observe(invalid, false); });
        }
        const auto negative = observe(fixture(is_vp, dims, 3, true).first, is_vp);
        const auto &nt = negative.blocks[is_vp ? 3 : 2];
        if (is_vp)
            check(field(nt, "tech_ai_weights_wire_count") == -7 &&
                      array(nt, "tech_ai_weights").values.empty(),
                  "VP negative weighted count changed");
        else
            check(nt.hashes.back().wire_count == -7 && nt.hashes.back().rows.empty() &&
                      field(nt, "latest_flavors_wire_count") == -9 &&
                      array(nt, "latest_flavors").values.empty() &&
                      field(negative.blocks.back(), "flavors_wire_count") == -5,
                  "BNW signed counts treated as unsigned sentinels");
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), is_vp); });
        saves::GameStateBudget budget;
        budget.elements = 2;
        rejects([&] { observe(bytes, is_vp, dims, budget); });
    }
    const auto large = observe(fixture(true, {65537, 65537}, 65537).first, true, {65537, 65537});
    check(array(large.blocks[2], "franchises_per_improvement").values.size() == 65537 &&
              array(large.blocks[3], "researching_tech").values.size() == 65537 &&
              array(large.blocks[3], "tech_ai_weights").values.size() == 131074 &&
              array(large.blocks[4], "active_flavors").values.size() == 65537,
          "Player knowledge dimensions capped at 16 bits");
    const auto bytes = fixture(true, dims).first;
    rejects([&] { observe(bytes, true, {UINT64_MAX, 2}); });
    rejects([&] { observe(bytes, true, {3, UINT64_MAX}); });
    for (bool is_vp : {false, true}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        rejects([&] {
            saves::PlayerKnowledgeDecodeContext wrong;
            if (!is_vp)
                wrong.profile_extensions.emplace<Dims>(dims);
            saves::observe_player_knowledge(in, is_vp ? vp : bnw, std::move(wrong));
        });
    }
    content::RulesDatabase db;
    db.apply_sql("CREATE TABLE Technologies(ID); INSERT INTO Technologies VALUES(-1),(0),(2),(2); CREATE "
                 "TABLE Flavors(ID); INSERT INTO Flavors VALUES(4294967297)");
    const auto d = content::player_knowledge_content_dimensions(db);
    check(d.techs == 4 && d.flavors == 4294967298ull,
          "Knowledge rule span collapsed holes/duplicates or narrowed");
    db.apply_sql("INSERT INTO Technologies VALUES(3.5)");
    check(content::player_knowledge_content_dimensions(db).techs == 5,
          "Knowledge source collection rejected a real ID coerced to integer");
    class ShiftedKnowledgeStream final : public std::streambuf {
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
        explicit ShiftedKnowledgeStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream input(&backing);
    runtime::BinaryReader shifted(input, 0x100000123ull + bytes.size());
    shifted.seek(0x100000123ull);
    saves::PlayerKnowledgeDecodeContext shifted_context;
    shifted_context.profile_extensions.emplace<Dims>(dims);
    const auto p = saves::observe_player_knowledge(shifted, vp, std::move(shifted_context));
    check(p.start_offset == 0x100000123ull && p.end_offset == 0x100000123ull + bytes.size(),
          "Player knowledge stream offset narrowed");
}
