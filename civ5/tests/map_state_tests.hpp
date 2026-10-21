#pragma once
void map_state_tests() {
    using saves::GameStateLayout;
    constexpr auto vp = GameStateLayout::vp542_corpus, bnw = GameStateLayout::expansion2_version1;
    const auto header_fixture = [](bool vp) {
        std::string s;
        if (!vp)
            u32(s, 1);
        for (auto n : {180u, 94u, 6644u, 7795u, 15u, 90u, 0xffffffa6u})
            u32(s, n);
        s.append("\1\0", 2);
        for (unsigned i = 0; i < 16; ++i)
            s += char(i);
        if (vp)
            s += '\1';
        for (unsigned group = 0; group < 2; ++group) {
            if (vp) {
                u32(s, 123 + group);
                u32(s, 0xffffffff);
            } else {
                u32(s, 3);
                u32(s, 0);
                u32(s, 0xfedcba98);
                u32(s, 123 + group);
                u32(s, 0xfedcba98);
                u32(s, 0xffffffff);
            }
        }
        return s;
    };
    const auto header = [&](const std::string &s, bool is_vp,
                            saves::GameStateBudget budget = saves::GameStateBudget{}) {
        std::istringstream stream(s);
        runtime::BinaryReader r(stream, s.size());
        return saves::observe_map_header(r, is_vp ? vp : bnw,
                                         is_vp ? std::optional<std::uint64_t>(2) : std::nullopt, budget);
    };
    for (bool is_vp : {false, true}) {
        const auto bytes = header_fixture(is_vp);
        const auto h = header(bytes, is_vp);
        check(h.grid.size() == 16920 && h.grid.index(179, 93) == 16919 &&
                  h.grid.coordinates(16919) == std::array<std::uint64_t, 2>{179, 93},
              "Map grid ordering wrong");
        check(h.top_latitude == 90 && h.bottom_latitude == -90 && h.wrap_x && !h.wrap_y && h.guid[15] == 15 &&
                  h.end_offset == bytes.size(),
              "Map header altered");
        check(h.resources[0].back().count == -1 && h.resources[0][is_vp ? 0 : 1].count == 123,
              "Map resource counts altered");
        if (!is_vp)
            check(!h.resources[0][0].count && h.resources[0][1].type_hash == 0xfedcba98 &&
                      h.resources[0][2].type_hash == 0xfedcba98,
                  "Map resource hashes dropped or deduplicated");
        else
            check(h.map_generated == true && !h.resources[0][0].type_hash, "VP resource IDs invented");
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { header(bytes.substr(0, n), is_vp); });
        rejects([&] { header(bytes, is_vp, {1, 1000, 1000}); });
        auto invalid = bytes;
        invalid[is_vp ? 28 : 32] = 2;
        rejects([&] { header(invalid, is_vp); });
        invalid = bytes;
        std::fill_n(invalid.begin() + (is_vp ? 0 : 4), 4, '\0');
        rejects([&] { header(invalid, is_vp); });
        check(header(bytes + "plot", is_vp).end_offset == bytes.size(), "Map header consumed a plot");
    }
    const saves::MapGrid large{0x100000003ull, 3};
    check(large.index(0x100000002ull, 2) == 0x300000008ull &&
              large.coordinates(0x300000008ull)[0] == 0x100000002ull,
          "Map index narrowed to 32 bits");
    rejects([&] { (void)large.index(large.width, 0); });
    rejects([&] { (void)large.coordinates(large.size()); });
    rejects([] { (void)saves::MapGrid{UINT64_MAX, 2}.size(); });
    rejects([] { (void)saves::MapGrid{1, 0}.size(); });
    const auto p16 = [](std::string &s, unsigned n) {
        s += char(n);
        s += char(n >> 8);
    };
    const auto fixture = [&](bool is_vp, unsigned version = 7, unsigned yields = 3, unsigned players = 3,
                             unsigned units = 2) {
        std::string s;
        const auto byte = [&](int n) { s += char(n); };
        const auto shorts = [&](std::initializer_list<unsigned> values) {
            for (auto n : values)
                p16(s, n);
        };
        const auto ints = [&](std::initializer_list<unsigned> values) {
            for (auto n : values)
                u32(s, n);
        };
        if (is_vp) {
            shorts({0x8000, 65535, 32767, 65534});
            for (int n : {1, 2, 3, 4, -1})
                byte(n);
            shorts({123, 456});
            ints({2, 65537, 0xffffffff});
            shorts({0xffff});
            for (int n : {-128, 1, 2, 3, 4, 37, 3, 5})
                byte(n);
            ints({70000, 80000, 90000});
            for (int n : {1, 2, 3, 4, 5, 6, 7, 8, -1})
                byte(n);
            shorts({0x8000});
            for (int n : {-1, 2, 3, 4, 5, 1})
                byte(n);
            ints({70000, 65537, 0xffffffff, 0xffffffff});
            for (unsigned i = 0; i < yields; ++i)
                byte(255);
            for (unsigned i = 0; i < 2; ++i) {
                for (int n : {1, 200, 255, -1, 1})
                    byte(n);
                ints({70000, 80000});
                for (int n : {1, 0, -2})
                    byte(n);
            }
            u32(s, players);
            for (unsigned i = 0; i < (players + 31) / 32; ++i)
                u32(s, 0x80000001);
            byte(-128);
            byte(1);
            string(s, std::string("a\0\xff", 3));
            ints({2, 90000, 65537, 90000, 0xffffffff});
            ints({2, 70000, 1, 70000, 2});
            ints({2, 70000, 2, 0xffffffff, 65537, 70000, 0xffffffff});
        } else {
            u32(s, version);
            shorts({65535, 65534});
            u32(s, 0xffffffff);
            byte(-128);
            shorts({65535, 2, 3, 4});
            for (int n : {1, 2, 3, 4, -1, 37})
                byte(n);
            shorts({0x8000, 32767});
            ints({0xffffffff, 65537, 0xffffffff});
            for (unsigned i = 0; i < 12; ++i)
                byte(i % 2);
            for (int n : {37, 3, 5})
                byte(n);
            if (version >= 3)
                u32(s, 0xfedcba98);
            else
                byte(-1);
            u32(s, 0x12345678);
            if (version >= 5)
                u32(s, 0x87654321);
            else
                byte(-2);
            if (version >= 7)
                u32(s, 0);
            if (version >= 2)
                byte(37);
            for (int n : {1, 2, 3, -1, -1, -128, 7, 6, 5})
                byte(n);
            for (unsigned i = 0; i < 10; ++i)
                u32(s, 70000 + i);
            for (unsigned i = 0; i < yields; ++i)
                p16(s, 0x8000);
            for (unsigned i = 0; i < players; ++i)
                u32(s, 0xffffffff);
            for (unsigned i = 0; i < players; ++i)
                byte(-128);
            shorts({65535, 32767});
            byte(-1);
            byte(37);
            byte(-128);
            ints({1, 2, 4, 0x80000000});
            byte(0);
            byte(1);
            if (version >= 6)
                ints({0, 0xfedcba98});
            else
                shorts({65535, 32767});
            shorts({65535, 32767});
            byte(0);
            byte(1);
            byte(1);
            string(s, std::string("a\0\xff", 3));
            ints({3, 3, 0, 0xfedcba98});
            shorts({0x8000});
            u32(s, 0xfedcba98);
            shorts({32767});
            shorts({65535, 32767});
        }
        u32(s, units);
        for (unsigned i = 0; i < units; ++i)
            ints({70000, 0xffffffff});
        byte(-128);
        if (!is_vp)
            u32(s, 2);
        ints({0xffffffff, 70000, 80000, 90000, 65537});
        if (is_vp) {
            u32(s, 65537);
            shorts({65535, 0x8000, 32767});
        }
        return s;
    };
    const auto observe = [&](const std::string &s, bool is_vp, unsigned yields = 3, unsigned players = 3,
                             saves::MapPlotBudget budget = saves::MapPlotBudget{}) {
        auto h = header(header_fixture(is_vp), is_vp);
        h.grid = {1, 1};
        std::istringstream stream(s);
        runtime::BinaryReader r(stream, s.size());
        return saves::observe_map_plots(r, h, {players, 2, 2, yields, 1}, budget);
    };
    const auto field = [](const auto &p, std::string_view name) {
        for (const auto &f : p.fields)
            if (f.name == name)
                return f.value;
        throw std::runtime_error("Missing test field");
    };
    for (bool is_vp : {false, true}) {
        const auto bytes = fixture(is_vp);
        const auto m = observe(bytes, is_vp);
        const auto &p = m.plots[0];
        check(m.end_offset == bytes.size() && p.end_offset == bytes.size() && p.index == 0 && p.x == 0 &&
                  p.y == 0,
              "Plot boundaries incorrect");
        check(field(p, "owner") == 37 && field(p, "artifact_player2") == 90000 &&
                  p.script_data == std::vector<std::uint8_t>{97, 0, 255},
              "Plot source state changed");
        check(p.arrays.back().name == std::string_view("units") &&
                  p.arrays.back().values == std::vector<std::int64_t>{70000, -1, 70000, -1},
              "Plot units narrowed or deduplicated");
        if (is_vp) {
            check(field(p, "area") == -32768 && field(p, "feature_type") == 70000 &&
                      field(p, "packed_flags") == 65535,
                  "VP plot fields narrowed");
            check(p.invisible_visibility.size() == 2 &&
                      p.invisible_visibility[0].values == std::vector<std::int64_t>{-1, 65537} &&
                      p.invisible_visibility[1].wire_count == UINT32_MAX &&
                      p.invisible_visibility[1].values.empty(),
                  "VP nested visibility or sentinel changed");
        } else {
            check(field(p, "source_x") == -1 && field(p, "resource_type_hash") == 0x12345678 &&
                      p.legacy_build_progress.size() == 3 && !p.legacy_build_progress[0].count &&
                      p.legacy_build_progress[1].count == -32768,
                  "Legacy plot/hash/build data changed");
        }
        for (std::size_t n = 0; n < bytes.size(); ++n)
            rejects([&] { observe(bytes.substr(0, n), is_vp); });
        rejects([&] { observe(bytes, is_vp, 3, 3, {0, {1000, 1000, 1000}}); });
        rejects([&] { observe(bytes, is_vp, 3, 3, {1000, {1, 1000, 1000}}); });
        rejects([&] { observe(bytes, is_vp, 3, 3, {1000, {1000, 2, 1000}}); });
        check(observe(bytes + "areas", is_vp).end_offset == bytes.size(), "Plots consumed area block");
    }
    for (unsigned version = 1; version <= 7; ++version) {
        const auto m = observe(fixture(false, version), false);
        check(m.plots[0].version == version, "BNW plot version ignored");
        if (version < 7)
            check(field(m.plots[0], "improvement_under_construction") == -1,
                  "Invented legacy construction state");
    }
    rejects([&] { observe(fixture(false, 8), false); });
    check(observe(fixture(true, 7, 65537, 65537, 65537), true, 65537, 65537)
                  .plots[0]
                  .arrays.back()
                  .values.size() == 131074,
          "Plot source capacities leaked into x64 model");
    rejects([&] { observe(fixture(true), true, 3, 4); });
    auto old_archaeology = fixture(false);
    old_archaeology[old_archaeology.size() - 24] = 1;
    old_archaeology.resize(old_archaeology.size() - 4);
    check(field(observe(old_archaeology, false).plots[0], "artifact_work") == -1,
          "Old archaeology acquired an invented great work");
    auto invalid_city_flag = fixture(true);
    invalid_city_flag[67] = 2;
    rejects([&] { observe(invalid_city_flag, true); });

    // A sparse backing stream exercises real reader positions beyond 4 GiB
    // without allocating gigabytes for the synthetic fixture.
    class ShiftedMapStream final : public std::streambuf {
        const std::string &bytes_;
        std::uint64_t position_ = 0;
        pos_type seekpos(pos_type position, std::ios_base::openmode) override {
            position_ = static_cast<std::uint64_t>(off_type(position));
            return position;
        }
        std::streamsize xsgetn(char *destination, std::streamsize count) override {
            const auto local = position_ - 0x100000123ull;
            if (local > bytes_.size() || static_cast<std::uint64_t>(count) > bytes_.size() - local)
                return 0;
            std::copy_n(bytes_.data() + local, count, destination);
            position_ += static_cast<std::uint64_t>(count);
            return count;
        }

      public:
        explicit ShiftedMapStream(const std::string &bytes) : bytes_(bytes) {}
    };
    const auto bytes = header_fixture(true) + fixture(true);
    ShiftedMapStream backing(bytes);
    std::istream stream(&backing);
    runtime::BinaryReader reader(stream, 0x100000123ull + bytes.size());
    reader.seek(0x100000123ull);
    auto shifted_header = saves::observe_map_header(reader, vp, 2);
    shifted_header.grid = {1, 1};
    const auto shifted_plots = saves::observe_map_plots(reader, shifted_header, {3, 2, 2, 3, 1});
    check(shifted_header.start_offset == 0x100000123ull &&
              shifted_plots.start_offset == shifted_header.end_offset &&
              shifted_plots.end_offset == 0x100000123ull + bytes.size(),
          "Map stream positions narrowed");

    saves::MapHeaderObservation projection_header{};
    projection_header.layout = vp;
    projection_header.grid = {2, 1};
    projection_header.wrap_x = true;
    std::vector<saves::PlotObservation> projection_plots;
    saves::PlotObservation first{};
    first.index = 0;
    first.x = 0;
    first.y = 0;
    first.fields = {{"owner", 4}, {"plot_type", 1}, {"terrain_type", 2},
                    {"feature_type", 9}, {"resource_type", -1}, {"improvement_type", 3},
                    {"continent_style", 7}, {"is_city", 1}};
    saves::PlotObservation second{};
    second.index = 1;
    second.x = 1;
    second.y = 0;
    second.fields = {{"owner", -1}, {"plot_type", 2}, {"terrain_type", 1},
                     {"feature_type_hash", 0xfeedbeef}, {"resource_type_hash", 0x12345678},
                     {"improvement_type_hash", 0x87654321}, {"continent", 222}, {"continent_style", 2},
                     {"is_city", 0}};
    projection_plots = {std::move(first), std::move(second)};
    saves::MapPlotsObservation projection_input{vp, {}, 0, 0, std::move(projection_plots)};
    const auto unresolved = saves::project_saved_map(projection_header, projection_input);
    check(!unresolved.cells[0].native_terrain_mask_known && !unresolved.cells[1].native_terrain_mask_known &&
              !unresolved.cells[0].native_category && !unresolved.cells[1].native_category,
          "Unresolved features acquired final terrain masks/categories");
    const auto projection = saves::project_saved_map(projection_header, projection_input,
        [](const saves::SavedMapCellProjection &) { return saves::SavedTerrainFeature{false, "FEATURE_FOREST"}; });
    check(projection.grid.size() == 2 && projection.wrap_x && !projection.wrap_y && projection.cells.size() == 2,
          "Saved map projection dimensions changed");
    check(projection.cells[0].native_terrain_mask_known &&
              projection.cells[0].native_terrain_mask == 0x00000080 &&
              projection.cells[0].owner == 4 && projection.cells[0].is_city &&
              projection.cells[0].continent_type == 7,
          "Saved map projection lost known VP plot state");
    check(projection.cells[1].native_terrain_mask_known &&
              projection.cells[1].native_terrain_mask == 0x00000002 &&
              projection.cells[1].feature_type_hash == 0xfeedbeef &&
              !projection.cells[1].feature_type && projection.cells[1].continent_type == 2,
          "Saved map projection invented a hash-resolved type");
    const auto terrain_source = saves::project_saved_map_terrain(projection);
    check(terrain_source.cells.size() == 2 && terrain_source.known_terrain_masks == 2 &&
              !terrain_source.native_markup_complete &&
              terrain_source.cells[0].native_terrain_mask == 0x00000080 &&
              terrain_source.cells[0].native_category == 7 &&
              terrain_source.cells[1].native_terrain_mask == 0x00000002 &&
              terrain_source.cells[1].native_category == 2,
          "Saved terrain source did not preserve x-major masks/categories");
    auto unknown_terrain = projection;
    unknown_terrain.cells[1].native_terrain_mask = 0;
    unknown_terrain.cells[1].native_terrain_mask_known = false;
    const auto unknown_source = saves::project_saved_map_terrain(unknown_terrain);
    check(unknown_source.known_terrain_masks == 1 &&
              !unknown_source.cells[1].native_terrain_mask_known &&
              !unknown_source.native_markup_complete,
          "Unknown saved terrain type was fabricated for graphics");
    auto bad_projection = projection_input;
    bad_projection.layout = bnw;
    rejects([&] { (void)saves::project_saved_map(projection_header, bad_projection); });
    bad_projection = projection_input;
    bad_projection.plots[1].index = 0;
    rejects([&] { (void)saves::project_saved_map(projection_header, bad_projection); });
    bad_projection = projection_input;
    bad_projection.plots[1].x = 0;
    rejects([&] { (void)saves::project_saved_map(projection_header, bad_projection); });

    auto handles = projection_input;
    handles.plots[0].fields.push_back({"revealed_bit_count", 35});
    handles.plots[0].arrays = {{"units",2,{4294967300LL,4294967301LL,4,7,4,7}}, {"revealed_words",1,{0x80000000,0xffffffff}}};
    const auto live_handles=saves::project_saved_map(projection_header,handles);
    check(live_handles.cells[0].units==std::optional{std::vector<saves::SavedPlotUnitReference>{{4294967300LL,4294967301LL},{4,7},{4,7}}} &&
              live_handles.cells[0].revealed_teams==std::optional{std::set<std::int64_t>{31,32,33,34}} && !live_handles.cells[1].units && !live_handles.cells[1].revealed_teams,
          "Shared plot projection retains wide/duplicate unit handles, widens reveal bits and discards wire padding");
    auto invalid_handles=handles;invalid_handles.plots[0].arrays[0].values.pop_back();
    rejects([&]{(void)saves::project_saved_map(projection_header,invalid_handles);});
    invalid_handles=handles;invalid_handles.plots[0].arrays.push_back(invalid_handles.plots[0].arrays[0]);
    rejects([&]{(void)saves::project_saved_map(projection_header,invalid_handles);});
    invalid_handles=handles;invalid_handles.plots[0].arrays[1].values[0]=-1;
    rejects([&]{(void)saves::project_saved_map(projection_header,invalid_handles);});
    invalid_handles=handles;invalid_handles.plots[0].fields.pop_back();
    rejects([&]{(void)saves::project_saved_map(projection_header,invalid_handles);});

    saves::PlotObservation vp_art{};
    vp_art.index = 17;
    std::vector<std::int64_t> vp_teams(30, 0);
    vp_teams[21] = 3;  // team 2 visibility
    vp_teams[25] = 19; // team 2 revealed improvement
    vp_teams[26] = 1;  // team 2 revealed railroad
    vp_art.fields.push_back({"revealed_bit_count", 3});
    vp_art.arrays.push_back({"teams", 10, vp_teams});
    vp_art.arrays.push_back({"revealed_words", 1, {4}});
    const auto vp_visual = saves::observe_saved_plot_team_art(vp_art, vp, 2);
    check(vp_visual.plot_index == 17 && vp_visual.visibility_count == 3 &&
              vp_visual.revealed_improvement == 19 && vp_visual.revealed_route == 1 &&
              vp_visual.revealed == true && !vp_visual.revealed_improvement_hash,
          "VP graphics projection lost team tuple or team reveal bit");
    check(saves::observe_saved_plot_team_art(vp_art, vp, 1).revealed == false,
          "VP reveal bit used player or wrong team index");
    rejects([&] { (void)saves::observe_saved_plot_team_art(vp_art, vp, 3); });
    auto vp_high_bit = vp_art;
    vp_high_bit.fields[0].value = 64;
    vp_high_bit.arrays[0].values.resize(640, 0);
    vp_high_bit.arrays[1].values = {0x80000000, 8};
    check(saves::observe_saved_plot_team_art(vp_high_bit, vp, 31).revealed == true &&
              saves::observe_saved_plot_team_art(vp_high_bit, vp, 35).revealed == true,
          "VP reveal bits failed across unsigned word boundary");
    vp_high_bit.fields[0].value = 32;
    rejects([&] { (void)saves::observe_saved_plot_team_art(vp_high_bit, vp, 35); });
    vp_high_bit.fields[0].value = 64;
    vp_high_bit.arrays[1].values[1] = -1;
    rejects([&] { (void)saves::observe_saved_plot_team_art(vp_high_bit, vp, 35); });

    saves::PlotObservation bnw_art{};
    bnw_art.index = 23;
    bnw_art.version = 6;
    std::vector<std::int64_t> visibility(40, 0), improvement_hashes(40, 0), routes(40, -1);
    visibility[35] = 2;
    improvement_hashes[35] = 0x87654321;
    routes[35] = 0;
    bnw_art.arrays = {{"visibility", 1, visibility}, {"revealed_words", 1, {0, 8, 0, 0}},
                      {"revealed_improvement_hashes", 1, improvement_hashes},
                      {"revealed_routes", 1, routes}};
    const auto bnw_visual = saves::observe_saved_plot_team_art(bnw_art, bnw, 35);
    check(bnw_visual.plot_index == 23 && bnw_visual.visibility_count == 2 &&
              bnw_visual.revealed == true && !bnw_visual.revealed_improvement &&
              bnw_visual.revealed_improvement_hash == 0x87654321 && bnw_visual.revealed_route == 0,
          "BNW graphics projection lost team reveal/hash/route");
    check(saves::observe_saved_plot_team_art(bnw_art, bnw, 31).revealed == false,
          "BNW team reveal bit indexing changed");
    bnw_art.arrays[1].values[0] = 0x80000000;
    check(saves::observe_saved_plot_team_art(bnw_art, bnw, 31).revealed == true,
          "BNW high team reveal bit indexing changed");
    bnw_art.version = 5;
    bnw_art.arrays[2] = {"revealed_improvements", 1, std::vector<std::int64_t>(40, -1)};
    bnw_art.arrays[2].values[35] = 12;
    const auto old_visual = saves::observe_saved_plot_team_art(bnw_art, bnw, 35);
    check(old_visual.revealed_improvement == 12 && !old_visual.revealed_improvement_hash,
          "Older BNW numeric improvement was treated as a hash");
    bnw_art.arrays[0].values[35] = -1;
    rejects([&] { (void)saves::observe_saved_plot_team_art(bnw_art, bnw, 35); });

    saves::PregameObservation art_setup{};
    art_setup.layout = vp;
    art_setup.archive.integers["active_player"] = 1;
    art_setup.archive.integer_arrays["team_types"] = {0, 2, 1};
    saves::MapPlotsObservation art_map{};
    art_map.layout = vp;
    art_map.dimensions = {3, 3, 0, 0, 0};
    art_map.plots = {vp_art};
    const auto active_art = saves::observe_saved_map_team_art(art_setup, art_map);
    check(active_art.active_player == 1 && active_art.active_team == 2 && active_art.plots.size() == 1 &&
              active_art.plots[0] == vp_visual,
          "Saved active player was not joined to the matching plot team");
    art_setup.archive.integer_arrays["team_types"][1] = 3;
    rejects([&] { (void)saves::observe_saved_map_team_art(art_setup, art_map); });
    art_setup.archive.integer_arrays["team_types"][1] = 2;
    art_setup.archive.integers["active_player"] = -1;
    rejects([&] { (void)saves::observe_saved_map_team_art(art_setup, art_map); });
    art_setup.archive.integers["active_player"] = 1;
    art_setup.layout = bnw;
    rejects([&] { (void)saves::observe_saved_map_team_art(art_setup, art_map); });
}
