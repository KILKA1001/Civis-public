#pragma once
void city_production_tests() {
    constexpr auto vp_layout = saves::GameStateLayout::vp542_corpus;
    constexpr auto bnw_layout = saves::GameStateLayout::expansion2_version1;
    static_assert(sizeof(void *) == 8);
    struct Fixture {
        std::string bytes;
        std::vector<std::size_t> flags, counts, strings;
        std::size_t queue = 0;
    };
    const auto fixture = [](bool vp, bool empty = false, bool negative = false, std::uint32_t slots = 3,
                            std::uint32_t orders = 8) {
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
        auto count = [&](std::uint32_t n, bool stl = false, bool signed_count = false) {
            f.counts.push_back(s.size());
            integer(empty ? negative && signed_count ? 0xfffffff9u : vp && stl ? UINT32_MAX : 0 : n);
            return empty ? 0u : n;
        };
        auto hashed = [&](unsigned columns = 1, bool signed_count = false) {
            if (count(4, false, signed_count)) {
                integer(0);
                integer(0xdeadbeef);
                values(columns);
                integer(0xdeadbeef);
                values(columns); // duplicate/unknown must survive
                integer(UINT32_MAX);
                values(columns);
            }
        };
        if (!vp)
            integer(1);
        values(vp ? 8 : 7);
        flag(1);
        if (vp)
            for (unsigned i = 0; i < 8; ++i)
                values(slots);
        else
            for (unsigned i = 0; i < 6; ++i)
                hashed(1, true);
        values(3 * count(2, true));
        values(3 * count(3, true));
        if (!vp) {
            hashed();
            hashed();
            for (unsigned i = 0; i < 7; ++i)
                values(count(3));
            hashed();
        }
        f.queue = s.size();
        const auto n = count(orders);
        for (std::uint64_t i = 0; i < n; ++i) {
            integer(i % 8 == 7 ? UINT32_MAX : static_cast<std::uint32_t>(i % 8));
            integer(0xdeadbeef);
            integer(70000);
            flag(i % 2);
            flag((i + 1) % 2);
        }
        if (!vp) {
            for (unsigned i = 0; i < 3; ++i)
                hashed(6);
            integer(70000);
            flag(1);
            for (unsigned i = 0; i < 2; ++i) {
                values(count(3));
                for (unsigned j = 0, n = count(2); j < n; ++j)
                    flag(j);
            }
            values(3);
            flag(1);
            flag(0);
            f.strings.push_back(s.size());
            integer(3);
            s.append("\xff\0a", 3);
            flag(1);
        }
        return f;
    };
    const auto observe = [&](const std::string &bytes, bool vp, std::uint64_t version = 6,
                             std::uint64_t slots = 3, saves::GameStateBudget budget = {}) {
        std::istringstream stream(bytes);
        runtime::BinaryReader in(stream, bytes.size());
        saves::CityProductionContext c;
        if (vp)
            c.profile_extensions.emplace<saves::VpCityProductionWireDimensions>(
                saves::VpCityProductionWireDimensions{slots});
        else
            c.bnw_city_version = version;
        return saves::observe_city_production(in, vp ? vp_layout : bnw_layout, c, budget);
    };
    const auto number = [](const auto &b, std::string_view name) {
        for (const auto &f : b.fields)
            if (name == f.name)
                return f.value;
        throw std::runtime_error("Missing city production field");
    };
    const auto overwrite = [](std::string s, std::size_t offset, std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i)
            s.at(offset + i) = char(n >> (8 * i));
        return s;
    };
    for (bool vp : {false, true}) {
        const auto f = fixture(vp);
        const auto p = observe(f.bytes + "STRATEGY", vp);
        check(p.start_offset == 0 && p.end_offset == f.bytes.size() && p.blocks.size() == (vp ? 2 : 4),
              "City production must stop before strategy AI");
        check(vp ? saves::vp_city_production_building_slots(p.context) == 3 && !p.context.bnw_city_version
                 : !saves::vp_city_production_building_slots(p.context) && p.context.bnw_city_version == 6,
              "City production gained another GameCore's wire dimensions");
        for (std::size_t i = 0; i < p.blocks.size(); ++i)
            check(p.blocks[i].start_offset == (i ? p.blocks[i - 1].end_offset : 0), "Production block gap");
        const auto &b = p.blocks[0], &q = p.blocks[vp ? 1 : 2];
        check(number(b, "m_iNumBuildings") == 70000 && number(b, "m_iBuildingProductionModifier") == -1 &&
                  number(b, "m_bSoldBuildingThisTurn") == 1,
              "Building scalar width/sign");
        const auto last = b.arrays.size();
        check(b.arrays[last - 2].columns == 3 && b.arrays[last - 2].values.size() == 6 &&
                  b.arrays[last - 1].columns == 3 && b.arrays[last - 1].values.size() == 9 &&
                  b.arrays[last - 1].values[2] == INT32_MIN,
              "Building yield/great work triples");
        if (vp) {
            check(b.arrays.size() == 10 && b.children.empty(), "VP building spans layout");
            for (std::size_t i = 0; i < 8; ++i)
                check(b.arrays[i].values == std::vector<std::int64_t>{70000, -1, INT32_MIN},
                      "VP building slots must use context, not m_iNumBuildings");
        } else {
            check(b.children.size() == 6 && b.arrays.size() == 2, "BNW building hashes layout");
            for (const auto &a : b.children) {
                check(a.children.size() == 4 && number(a, "wire_count") == 4, "Hashed building rows");
                const auto &r = a.children;
                check(number(r[0], "hash") == 0 && r[0].arrays.empty() &&
                          r[0].end_offset - r[0].start_offset == 4,
                      "Null hash must not consume a value");
                check(number(r[1], "hash") == 0xdeadbeefull && number(r[2], "hash") == 0xdeadbeefull &&
                          number(r[3], "hash") == UINT32_MAX && r[1].arrays[0].values[0] == 70000,
                      "Unknown/duplicate building hashes lost");
            }
            check(p.blocks[1].children.size() == 10, "BNW specialist/production fields");
            const auto &tail = p.blocks[3];
            for (const auto &a : tail.children)
                check(a.children[0].arrays.empty() && a.children[1].arrays[0].columns == 6 &&
                          a.children[1].arrays[0].values ==
                              std::vector<std::int64_t>{70000, -1, INT32_MIN, 70000, -1, INT32_MIN},
                      "BNW fixed yield matrix row width");
            check(tail.blobs[0].bytes == std::vector<std::uint8_t>{255, 0, 97}, "Repeated city name bytes");
            check(tail.arrays[1].values == std::vector<std::int64_t>{0, 1} &&
                      number(tail, "m_bOwedCultureBuilding") == 1,
                  "BNW final rank flags/happiness");
        }
        check(q.children.size() == 8 && q.start_offset == f.queue, "Production queue order count");
        for (std::size_t i = 0; i < 8; ++i) {
            const auto &o = q.children[i];
            check(o.index == i && o.end_offset - o.start_offset == 14 &&
                      number(o, "eOrderType") == (i == 7 ? -1 : std::int64_t(i)),
                  "Unknown order preserved");
            check(number(o, !vp && i <= 4 ? "iData1_hash" : "iData1") ==
                          (!vp && i <= 4 ? 0xdeadbeefll : -559038737ll) &&
                      number(o, "iData2") == 70000 && number(o, "bSave") == i % 2 &&
                      number(o, "bRush") == (i + 1) % 2,
                  "Queue hash/index, flags, unknown data or order changed");
        }
        for (bool negative : {false, true}) {
            const auto e = fixture(vp, true, negative);
            const auto a = observe(e.bytes, vp);
            check(a.end_offset == e.bytes.size() && a.blocks[vp ? 1 : 2].children.empty(), "Empty queue");
            check(number(a.blocks[0], "m_aBuildingGreatWork_wire_count") ==
                      (vp ? std::int64_t(UINT32_MAX) : 0),
                  "STL sentinel preserved");
            if (!vp)
                check(number(a.blocks[0].children[0], "wire_count") == (negative ? -7 : 0),
                      "Signed building hash count preserved");
        }
        for (std::size_t cut = 0; cut < f.bytes.size(); ++cut)
            rejects([&] { observe(f.bytes.substr(0, cut), vp); });
        for (auto offset : f.flags) {
            auto bad = f.bytes;
            bad[offset] = char(2);
            rejects([&] { observe(bad, vp); });
        }
        for (auto offset : f.counts)
            rejects([&] { observe(overwrite(f.bytes, offset, 0x7fffffff), vp); });
        rejects([&] { observe(overwrite(f.bytes, f.queue, UINT32_MAX), vp); });
        for (auto offset : f.strings)
            rejects([&] { observe(overwrite(f.bytes, offset, UINT32_MAX), vp); });
        saves::GameStateBudget tiny;
        tiny.elements = 1;
        rejects([&] { observe(f.bytes, vp, 6, 3, tiny); });
        if (!vp) {
            for (bool total : {false, true}) {
                saves::GameStateBudget budget;
                if (total)
                    budget.total_string_bytes = 2;
                else
                    budget.string_bytes = 2;
                rejects([&] { observe(f.bytes, vp, 6, 3, budget); });
            }
            rejects([&] { observe(overwrite(f.bytes, 0, 2), false); });
        }
    }
    for (unsigned version = 0; version <= 6; ++version) {
        for (unsigned buildings = 0; buildings <= 1; ++buildings) {
            const auto f = fixture(false);
            const auto p = observe(overwrite(f.bytes, 0, buildings), false, version);
            check(number(p.blocks[0], "version") == buildings && p.context.bnw_city_version == version &&
                      number(p.blocks[2].children[3], version >= 5 ? "iData1_hash" : "iData1") ==
                          (version >= 5 ? 0xdeadbeefll : -559038737ll),
                  "BNW queue version 5 switch");
        }
    }
    const auto context_with = [](std::optional<std::uint64_t> slots,
                                 std::optional<std::uint64_t> version) {
        saves::CityProductionContext c;
        c.bnw_city_version = version;
        if (slots)
            c.profile_extensions.emplace<saves::VpCityProductionWireDimensions>(
                saves::VpCityProductionWireDimensions{*slots});
        return c;
    };
    for (bool vp : {false, true}) {
        for (const auto &c : std::vector<saves::CityProductionContext>{
                 {}, context_with(0, {}), context_with(3, 6), context_with({}, 7),
                 vp ? context_with({}, 6) : context_with(3, {})}) {
            std::istringstream stream("");
            runtime::BinaryReader in(stream, 0);
            rejects([&] { saves::observe_city_production(in, vp ? vp_layout : bnw_layout, c); });
            check(in.position() == 0, "Invalid context must fail before consuming input");
        }
    }
    const auto many = fixture(true, false, false, 65537, 0);
    check(observe(many.bytes, true, 6, 65537).blocks[0].arrays[0].values.size() == 65537,
          "Building slots narrowed to 16-bit count");
    const auto queue = fixture(true, false, false, 3, 65537);
    check(observe(queue.bytes, true).blocks[1].children.size() == 65537, "Queue count narrowed");
    rejects([&] { observe(many.bytes, true, 6, UINT64_MAX); });
    const auto bytes = fixture(true).bytes;
    class ShiftedProductionStream final : public std::streambuf {
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
        explicit ShiftedProductionStream(const std::string &bytes) : bytes_(bytes) {}
    } backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader in(stream, 0x100000123ull + bytes.size());
    in.seek(0x100000123ull);
    const auto shifted = saves::observe_city_production(in, vp_layout, context_with(3, {}));
    check(shifted.start_offset == 0x100000123ull && shifted.end_offset == 0x100000123ull + bytes.size() &&
              shifted.blocks[1].children[0].start_offset > UINT32_MAX,
          "Production offsets narrowed");
}
