#pragma once
void player_strategy_tests() {
    constexpr auto vp = saves::GameStateLayout::vp542_corpus,
                   bnw = saves::GameStateLayout::expansion2_version1;
    saves::PlayerStrategyWireDimensions dims;
    dims.major = 3;
    dims.civ_players = 4;
    dims.yields = 2;
    dims.grand = 2;
    const saves::VpPlayerStrategyWireDimensions content_dims{3, 2, 2, 2, 2, 2};
    const auto fixture = [&](bool is_vp, unsigned old_version = 2, unsigned large = 3,
                             bool negative_weights = false, bool sentinel = false) {
        std::string s;
        std::vector<std::uint64_t> bounds;
        const auto ints = [&](std::initializer_list<std::uint32_t> a) {
            for (auto x : a)
                u32(s, x);
        };
        const auto fill = [&](std::uint64_t n, char type = 'i') {
            for (std::uint64_t i = 0; i < n; ++i) {
                if (type == 'B')
                    s += char(i % 2);
                else
                    u32(s, i % 3 == 0 ? 70000 : i % 3 == 1 ? UINT32_MAX : 0x80000000);
            }
        };
        const auto vector = [&](unsigned width = 1, bool weights = false) {
            if (weights && negative_weights) {
                u32(s, 0xfffffff9);
                return;
            }
            if (!weights && sentinel && is_vp) {
                u32(s, UINT32_MAX);
                return;
            }
            u32(s, 3);
            fill(3 * width);
        };
        const auto hashed = [&](bool boolean = false, bool all = false) {
            u32(s, 4);
            for (auto key : {0u, 0x12345678u, UINT32_MAX, 0x12345678u}) {
                u32(s, key);
                if (key || all) {
                    if (boolean)
                        s += char(1);
                    else
                        u32(s, 0x80000000);
                }
            }
        };
        bounds.push_back(s.size());
        if (is_vp) {
            fill(4 * content_dims.policies, 'B');
            fill(3 * content_dims.branches, 'B');
            fill(3);
            fill(content_dims.flavors);
            vector(2, true);
            vector();
        } else {
            u32(s, old_version);
            for (unsigned i = 0; i < 6; ++i)
                hashed(true);
            ints({3, 0, UINT32_MAX, UINT32_MAX});
            fill(1);
            ints({0, UINT32_MAX, 0x12345678});
            if (old_version == 1)
                u32(s, 0xffffffff);
            u32(s, 1);
            hashed(false, true);
            u32(s, 2);
            fill(2);
        }
        bounds.push_back(s.size());
        if (!is_vp)
            ints({1, 2});
        fill(2, 'B');
        fill(2);
        if (is_vp) {
            vector(4);
            vector(4);
            vector(4);
            fill(10);
        } else {
            fill(dims.yields);
            s += char(1);
            for (unsigned i = 0; i < 2; ++i) {
                u32(s, large);
                fill(2ull * large);
            }
            fill(5);
        }
        vector(3);
        bounds.push_back(s.size());
        if (!is_vp)
            u32(s, old_version);
        fill(is_vp ? 4 : 2, 'B');
        fill(is_vp ? 4 : old_version == 2 ? 5 : 4);
        bounds.push_back(s.size());
        if (is_vp)
            vector(2, true);
        else {
            ints({1, 2});
            fill(2);
            hashed();
        }
        bounds.push_back(s.size());
        if (is_vp) {
            fill(31);
            for (unsigned a = 0; a < 2; ++a) {
                u32(s, sentinel ? UINT32_MAX : large);
                if (!sentinel)
                    for (unsigned j = 0; j < large; ++j) {
                        fill(6);
                        s += char(j % 2);
                    }
            }
            fill(2, 'B');
            fill(2);
            fill(dims.major);
        } else {
            u32(s, 1);
            fill(3);
            u32(s, 2);
            fill(2, 'B');
            fill(2);
            fill(4 * dims.civ_players);
        }
        bounds.push_back(s.size());
        if (!is_vp)
            u32(s, 1);
        fill(2);
        fill(2);
        fill(2 * dims.major);
        bounds.push_back(s.size());
        return std::make_pair(s, bounds);
    };
    const auto observe = [&](const std::string &bytes, bool is_vp, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader r(stream, bytes.size());
        auto d = dims;
        if (is_vp)
            d.profile_extensions.emplace<saves::VpPlayerStrategyWireDimensions>(content_dims);
        return saves::observe_player_strategy(r, is_vp ? vp : bnw, d, budget);
    };
    const auto arr = [](const auto &b, std::string_view name) -> const auto & {
        for (const auto &a : b.arrays)
            if (a.name == name)
                return a;
        throw std::runtime_error("Missing strategy array");
    };
    const auto val = [](const auto &b, std::string_view name) {
        for (const auto &v : b.fields)
            if (v.name == name)
                return v.value;
        throw std::runtime_error("Missing strategy scalar");
    };
    for (const bool is_vp : {false, true}) {
        const auto [bytes, bounds] = fixture(is_vp);
        const auto p = observe(bytes, is_vp);
        check(p.blocks.size() == 6 && p.end_offset == bytes.size(), "Strategy block chain incomplete");
        for (unsigned i = 0; i < 6; ++i)
            check(p.blocks[i].start_offset == bounds[i] && p.blocks[i].end_offset == bounds[i + 1],
                  "Strategy boundary drift");
        const auto &e = p.blocks[1];
        check(arr(e, "requested_savings").columns == 3 && arr(e, "requested_savings").values[1] == -1,
              "Savings values narrowed");
        check(arr(p.blocks[5], "priorities").values == std::vector<std::int64_t>{70000, -1},
              "Strategy IDs/weights normalized");
        check(arr(e, "using_strategy").values == std::vector<std::int64_t>{0, 1},
              "Strategy booleans changed");
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), is_vp); });
        rejects([&] { observe(bytes, is_vp, {10, 0, 0}); });
        auto bad = bytes;
        bad[bounds[2] + (is_vp ? 0 : 4)] = 2;
        rejects([&] { observe(bad, is_vp); });
        const auto large = observe(fixture(is_vp, 2, 65537).first, is_vp, {2 * 1024 * 1024, 0, 0});
        if (is_vp) {
            const auto &targets = arr(p.blocks[4], "attack_targets");
            check(targets.columns == 7 &&
                      targets.values ==
                          std::vector<std::int64_t>{70000, -1, INT32_MIN, 70000, -1, INT32_MIN, 0,
                                                    70000, -1, INT32_MIN, 70000, -1, INT32_MIN, 1,
                                                    70000, -1, INT32_MIN, 70000, -1, INT32_MIN, 0},
                  "Military target fields changed");
            check(arr(large.blocks[4], "attack_targets").values.size() == 65537 * 7ull,
                  "Military target capacity narrowed");
            auto wrong = bytes;
            wrong[bounds[4] + 4 * 31 + 4 + 24] = 2;
            rejects([&] { observe(wrong, true); });
            const auto empty = observe(fixture(true, 2, 3, true, true).first, true);
            check(val(empty.blocks[0], "policy_ai_weights_wire_count") == -7 &&
                      arr(empty.blocks[0], "policy_ai_weights").values.empty(),
                  "Weighted signed count changed");
            check(val(empty.blocks[1], "exploration_land_wire_count") == UINT32_MAX &&
                      arr(empty.blocks[1], "exploration_land").values.empty(),
                  "STL map sentinel changed");
            check(val(empty.blocks[4], "attack_targets_wire_count") == UINT32_MAX &&
                      arr(empty.blocks[4], "attack_targets").values.empty(),
                  "Target vector sentinel changed");
        } else {
            check(arr(large.blocks[1], "exploration_plots").values.size() == 65537 * 2ull &&
                      arr(large.blocks[1], "goody_hut_assignments").values.size() == 65537 * 2ull,
                  "Original BNW exploration plot cap retained");
            const auto &holes = p.blocks[0].hashes.front();
            check(holes.rows[0].type_hash == 0 && holes.rows[0].values.empty() &&
                      holes.rows[2].type_hash == UINT32_MAX && holes.rows[2].values[0] == 1,
                  "Policy hash holes changed");
            check(p.blocks[0].hashes.back().rows[0].values == std::vector<std::int64_t>{INT32_MIN},
                  "Policy AI zero hash incorrectly skipped weight");
            check(p.blocks[3].hashes[0].rows[0].values.empty(), "Wonder zero hash incorrectly read weight");
            const auto old = observe(fixture(false, 1).first, false);
            check(val(old.blocks[0], "legacy_max_effective_cities") == -1 &&
                      val(old.blocks[2], "specializations_dirty") == 0,
                  "BNW legacy load fix applied prematurely");
            for (unsigned i = 0; i < 6; ++i) {
                auto wrong = bytes;
                wrong[bounds[i]] = char(9);
                rejects([&] { observe(wrong, false); });
            }
        }
        std::istringstream stream(bytes);
        runtime::BinaryReader r(stream, bytes.size());
        auto invalid = dims;
        if (is_vp)
            invalid.profile_extensions.emplace<saves::VpPlayerStrategyWireDimensions>(content_dims);
        invalid.major = UINT64_MAX;
        rejects([&] { saves::observe_player_strategy(r, is_vp ? vp : bnw, invalid); });
    }
    const auto [bytes, bounds] = fixture(true);
    std::istringstream stream(bytes);
    runtime::BinaryReader r(stream, bytes.size());
    rejects([&] { saves::observe_player_strategy(r, vp, dims); });
    auto vp_dims = dims;
    vp_dims.profile_extensions.emplace<saves::VpPlayerStrategyWireDimensions>(content_dims);
    auto copied_vp_dims=vp_dims;
    copied_vp_dims.profile_extensions.find<saves::VpPlayerStrategyWireDimensions>()->policies=99;
    check(saves::vp_player_strategy_dimensions(vp_dims)->policies==content_dims.policies &&
              !saves::vp_player_strategy_dimensions(dims),
          "VP strategy dimensions leaked into BNW or aliased a copied context");
    rejects([&] { saves::observe_player_strategy(r, bnw, vp_dims); });
    content::RulesDatabase db;
    for (auto name : {"Policies", "PolicyBranchTypes", "Flavors", "AIEconomicStrategies",
                      "AIMilitaryStrategies", "AIGrandStrategies"})
        db.apply_sql(std::string("CREATE TABLE ") + name + "(ID); INSERT INTO " + name +
                     " VALUES(-1),(0),(2),(2)");
    check(content::player_strategy_content_dimensions(db).policies == 4,
          "Policy span collapsed holes/duplicates");
    db.apply_sql("INSERT INTO AIGrandStrategies VALUES(4294967297)");
    check(content::player_strategy_content_dimensions(db).grand == 4294967298ull,
          "Grand strategy span narrowed");
    db.apply_sql("INSERT INTO Flavors VALUES(2.5)");
    check(content::player_strategy_content_dimensions(db).flavors == 5,
          "Strategy source collection rejected a real ID coerced to integer");
    class ShiftedStrategyStream final : public std::streambuf {
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
            position_ += count;
            return count;
        }

      public:
        explicit ShiftedStrategyStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream input(&backing);
    runtime::BinaryReader shifted(input, 0x100000123ull + bytes.size());
    shifted.seek(0x100000123ull);
    const auto p = saves::observe_player_strategy(shifted, vp, vp_dims);
    check(p.start_offset == 0x100000123ull && p.blocks[5].end_offset == 0x100000123ull + bytes.size(),
          "Strategy stream offset narrowed");
}
