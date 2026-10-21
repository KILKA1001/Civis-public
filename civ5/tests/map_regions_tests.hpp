#pragma once
void map_regions_tests() {
    constexpr auto vp = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    const saves::RegionWireDimensions d{2, 3, 2, 3};
    const auto fixture = [&](bool is_vp, std::uint32_t wide_count = 2) {
        std::string s;
        const auto ints = [&](std::initializer_list<std::uint32_t> v) {
            for (auto n : v)
                u32(s, n);
        };
        const auto container = [&](unsigned count) {
            if (!is_vp) {
                ints({wide_count, 0xffffffff, 65537, 0, 0xfedcba98});
                for (unsigned i = 0; i < wide_count; ++i)
                    u32(s, 0xffffffff);
            }
            u32(s, count);
        };
        container(1);
        if (!is_vp)
            u32(s, 1);
        ints({70000, 3, 2, 1, 4, 5, 6, 7, 8, 0xffffffff});
        if (is_vp)
            u32(s, 0xffffffff);
        ints({0xffffffff, 100000, 200000, 0xfffffffe});
        s.append("\1\0", 2);
        if (is_vp) {
            // Duplicate keys and the full enum/int range survive observation.
            ints({2, 70000, 0xffffffff, 70000, 0x80000000});
            for (unsigned i = 0; i < 4; ++i)
                u32(s, 0xffffffff);
            ints({2, 70000, 80000, 90000, 70000, 0xffffffff, 0xfffffffe});
            ints({2, 70000, wide_count});
            for (unsigned i = 0; i < wide_count; ++i)
                u32(s, 0xffffffff);
            ints({70000, 0xffffffff});
            ints({2, 70000, 4, 70000, 5});
            u32(s, 0xffffffff);
        } else {
            for (unsigned i = 0; i < 4 * 2 + 3 + 2 * 2 + 2 * 3; ++i)
                u32(s, 0x80000000 + i);
            ints({3, 0, 0xffffffff, 0xffffffff, 0xfedcba98, 70000});
            ints({4, 0, 0xffffffff, 0xfedcba98, 0xffffffff, 0xfedcba98, 70000});
        }
        container(1);
        if (!is_vp)
            u32(s, 1);
        ints({80000, 3, 0x80000000, 0xffffffff});
        s.append("\0\x80", 2);
        if (is_vp) {
            ints({2, 70000, 70000});
            container(1);
            ints({90000, 3, 10, 20});
            s += '\1';
            container(1);
            ints({100000, wide_count});
            for (unsigned i = 0; i < wide_count; ++i)
                u32(s, i ? 70000 : 0xffffffff);
        }
        u32(s, 0x80000001);
        return s;
    };
    const auto observe = [&](const std::string &bytes, bool is_vp,
                             saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader r(stream, bytes.size());
        return saves::observe_map_regions(r, is_vp ? vp : bnw, d, budget);
    };
    const auto field = [](const saves::MapRegionRecord &r, std::string_view name) {
        for (const auto &f : r.fields)
            if (f.name == name)
                return f.value;
        throw std::runtime_error("Missing region field");
    };
    for (bool is_vp : {false, true}) {
        const auto bytes = fixture(is_vp);
        const auto m = observe(bytes + "engine", is_vp);
        check(m.start_offset == 0 && m.end_offset == bytes.size() && m.ai_map_hints == -2147483647,
              "Map regions boundary or hints altered");
        check(m.containers.size() == (is_vp ? 4 : 2) && m.dimensions.has_value() != is_vp,
              "Region containers or source dimensions invented");
        const auto &a = m.containers[0].records[0];
        check(field(a, "id") == 70000 && field(a, "total_found_value") == UINT32_MAX &&
                  field(a, "north") == -1 && field(a, "west") == -2,
              "Area scalar width or sign changed");
        const auto &l = m.containers[1].records[0];
        check(field(l, "id") == 80000 && field(l, "centroid_x") == INT32_MIN &&
                  field(l, "continent_style") == -128,
              "Landmass scalar narrowed");
        if (is_vp) {
            check(a.arrays[0].values == std::vector<std::int64_t>{70000, -1, 70000, INT32_MIN},
                  "VP map keys deduplicated or narrowed");
            check(field(a, "cities_per_player_wire_count") == UINT32_MAX && a.arrays[1].values.empty(),
                  "VP map sentinel lost");
            check(a.yield_modifiers.size() == 2 && a.yield_modifiers[0].key == 70000 &&
                      a.yield_modifiers[1].key == 70000 && a.yield_modifiers[1].wire_count == UINT32_MAX &&
                      a.yield_modifiers[1].values.empty(),
                  "VP nested sentinel/order lost");
            check(m.containers[3].records[0].arrays[0].values == std::vector<std::int64_t>{-1, 70000},
                  "River null or unresolved reference changed");
        } else {
            check(m.containers[0].legacy_metadata == std::vector<std::int64_t>{2, -1, 65537, 0, -19088744} &&
                      m.containers[0].next_free == std::vector<std::int64_t>{-1, -1},
                  "BNW source allocation metadata changed");
            check(a.arrays[4].values.size() == 3 && a.arrays[5].columns == 2 &&
                      a.arrays[5].values.size() == 4 && a.arrays[6].columns == 3 &&
                      a.arrays[6].values.size() == 6,
                  "BNW fixed capacity confused with serialized player rows");
            check(a.resources.size() == 3 && !a.resources[0].count && a.resources[1].count == -1 &&
                      a.resources[1].type_hash == UINT32_MAX,
                  "Resource hash sentinel confused with improvements");
            check(a.improvements.size() == 4 && !a.improvements[0].count && !a.improvements[1].count &&
                      a.improvements[2].type_hash == a.improvements[3].type_hash &&
                      a.improvements[2].count == -1 && a.improvements[3].count == 70000,
                  "BNW improvement holes or duplicate hashes lost");
        }
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), is_vp); });
        rejects([&] { observe(bytes, is_vp, {1, 1000, 1000}); });
        auto invalid = bytes;
        invalid[is_vp ? 64 : 92] = 2;
        rejects([&] { observe(invalid, is_vp); });
    }
    check(observe(fixture(true, 65537), true).containers[3].records[0].arrays[0].values.size() == 65537,
          "River entries retain a 16-bit cap");
    check(observe(fixture(true, 65537), true).containers[0].records[0].yield_modifiers[0].values.size() ==
              65537,
          "Area yields retain a legacy cap");
    check(observe(fixture(false, 65537), false).containers[0].next_free.size() == 65537,
          "BNW source freelist bucket cap leaked into host");
    auto invalid = fixture(false);
    invalid[32] = 2;
    rejects([&] { observe(invalid, false); });
    invalid = fixture(false);
    std::fill_n(invalid.begin(), 4, '\xff');
    rejects([&] { observe(invalid, false); });
    std::string empty;
    for (unsigned i = 0; i < 4; ++i)
        u32(empty, i == 0 ? 0xffffffff : 0);
    u32(empty, 123);
    const auto e = observe(empty, true);
    check(e.containers[0].wire_count == -1 && e.containers[0].records.empty() && e.ai_map_hints == 123,
          "Signed TContainer count consumed a record");
    // Dynamic host counts exceed native region and bucket limits.
    std::string many;
    u32(many, 0);
    u32(many, 0);
    u32(many, 0);
    u32(many, 65537);
    for (unsigned i = 0; i < 65537; ++i) {
        u32(many, i);
        u32(many, 0xffffffff);
    }
    u32(many, 0);
    check(observe(many, true).containers[3].records.size() == 65537, "Region object count narrowed");
    class ShiftedRegionStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t position_ = 0;
        pos_type seekpos(pos_type p, std::ios_base::openmode) override {
            position_ = static_cast<std::uint64_t>(off_type(p));
            return p;
        }
        std::streamsize xsgetn(char *dst, std::streamsize count) override {
            const auto local = position_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(count) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, count, dst);
            position_ += static_cast<std::uint64_t>(count);
            return count;
        }

      public:
        explicit ShiftedRegionStream(const std::string &bytes) : bytes_(bytes) {}
    };
    const auto bytes = fixture(true);
    ShiftedRegionStream backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader reader(stream, 0x100000123ull + bytes.size());
    reader.seek(0x100000123ull);
    const auto m = saves::observe_map_regions(reader, vp);
    check(m.start_offset == 0x100000123ull && m.containers[0].records[0].start_offset == 0x100000127ull &&
              m.end_offset == 0x100000123ull + bytes.size(),
          "Region stream position narrowed");
}
