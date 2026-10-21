#include "civ5/saves/map_state.hpp"
#include "civ5/content/saved_terrain.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace civ5::saves;
namespace {
std::size_t checks = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    bool failed = false;
    try { action(); } catch (const std::exception &) { failed = true; }
    check(failed, "Expected invalid terrain definitions to be rejected");
}
SavedMapCellProjection project(std::vector<NamedSaveInteger> fields,
                               const SavedTerrainFeatureResolver &features = {},
                               GameStateLayout layout = GameStateLayout::expansion2_version1) {
    MapHeaderObservation header{};
    header.layout = layout;
    header.grid = {1, 1};
    MapPlotsObservation plots{};
    plots.layout = header.layout;
    PlotObservation plot{};
    plot.fields = std::move(fields);
    plots.plots.push_back(std::move(plot));
    auto projection = project_saved_map(header, plots, features);
    const auto source = project_saved_map_terrain(projection);
    const auto &cell = projection.cells.front();
    check(source.cells[0].native_terrain_mask == cell.native_terrain_mask &&
              source.cells[0].native_category == cell.native_category &&
              source.cells[0].rivers == cell.rivers &&
              source.known_terrain_masks == (cell.native_terrain_mask_known ? 1u : 0u) &&
              !source.native_markup_complete,
          "Graphics adapter changed classification or claimed a complete native buffer");
    return cell;
}
void guards() {
    for (int flags = 0; flags < 8; ++flags) {
        const bool w = (flags & 1) != 0, nw = (flags & 2) != 0, ne = (flags & 4) != 0;
        for (int count : {-1, 0, 1, 6}) {
            auto bnw = project({{"river_crossing_count", count}, {"w_of_river", w},
                {"nw_of_river", nw}, {"ne_of_river", ne}, {"river_e_flow", 0},
                {"river_se_flow", 1}, {"river_sw_flow", 5}}).rivers;
            auto vp = project({{"river_crossing_count", count},
                {"packed_flags", 0x4001 | (w << 7) | (nw << 8) | (ne << 6)},
                {"river_e_flow", 0}, {"river_se_flow", 1}, {"river_sw_flow", 5}},
                {}, GameStateLayout::vp542_corpus).rivers;
            const auto expected = count > 0 ? SavedRiverPlot{true, w, nw, ne, w ? 0 : -1, nw ? 1 : -1, ne ? 5 : -1}
                                            : SavedRiverPlot{};
            check(bnw == expected && vp == expected, "Saved river gate/flags/flow mismatch across wire layouts");
        }
    }
    check(!project({}).rivers, "Missing crossing count became a dry plot");
    check(!project({{"river_crossing_count", 1}}).rivers, "Missing river flags became known");
    check(!project({{"river_crossing_count", 1}, {"w_of_river", 1}, {"nw_of_river", 0},
                    {"ne_of_river", 0}}).rivers, "Missing active flow became known");
    check(!project({{"river_crossing_count", 1}, {"packed_flags", 65536}}, {},
                    GameStateLayout::vp542_corpus).rivers, "Invalid packed river flags narrowed");
    const auto mountain = project({{"terrain_type", 0}, {"plot_type", 0}, {"feature_type", -1},
                                    {"continent_style", -1}});
    check(mountain.native_terrain_mask_known && mountain.native_terrain_mask == 0x400 &&
              mountain.native_category == 255, "SDK mountain enum or signed style changed");
    const auto land = project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type", -1},
                                {"continent_style", 4}});
    check(land.native_terrain_mask_known && land.native_terrain_mask == 1 && land.native_category == 4,
          "SDK land enum was interpreted as mountain");
    const auto unresolved = project({{"terrain_type", 2}, {"plot_type", 1}, {"feature_type", 9},
                                     {"continent_style", 4}});
    check(unresolved.native_terrain_mask == 0x80 && !unresolved.native_terrain_mask_known &&
              !unresolved.native_category, "Unresolved feature became final terrain");
    const auto unknown_lookup = project({{"terrain_type", 2}, {"plot_type", 1}, {"feature_type", 9}},
        [](const auto &) -> std::optional<SavedTerrainFeature> { return {}; });
    check(!unknown_lookup.native_terrain_mask_known, "Failed feature lookup was treated as no feature");
    const auto absent = project({{"terrain_type", 2}, {"plot_type", 1}});
    check(!absent.native_terrain_mask_known, "Absent feature identity was treated as no feature");
    const auto no_relief = project({{"terrain_type", 2}, {"feature_type", -1}});
    check(!no_relief.native_terrain_mask_known, "Absent relief acquired a flat-land default");
    const auto no_terrain = project({{"plot_type", 0}, {"feature_type", -1}});
    check(!no_terrain.native_terrain_mask_known, "Absent terrain acquired a mask");
    const auto landmass = project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type_hash", 0},
                                   {"continent", 37}});
    check(landmass.native_terrain_mask_known && landmass.native_terrain_mask == 1 &&
              !landmass.native_category && !landmass.continent_type,
          "Landmass ID was used as an art-style category");
    const auto water = project({{"terrain_type", 5}, {"feature_type", -1}});
    check(water.native_terrain_mask_known && water.native_terrain_mask == 0x2000 &&
              water.native_category == 0, "Water requires an unnecessary relief/style value");
    for (const auto bad_style : {-129, 256}) {
        const auto invalid = project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type", -1},
                                      {"continent_style", bad_style}});
        check(invalid.native_terrain_mask_known && !invalid.native_category,
              "Out-of-range art style silently narrowed");
    }
    const auto wonder = project({{"terrain_type", 6}, {"feature_type_hash", 0xfedcba98}},
        [](const auto &cell) -> std::optional<SavedTerrainFeature> {
            check(!cell.feature_type && cell.feature_type_hash == 0xfedcba98,
                  "Resolver lost saved feature identity");
            return SavedTerrainFeature{true, "FEATURE_REEF"};
        });
    check(wonder.native_terrain_mask_known && wonder.native_terrain_mask == 0x100000 &&
              !wonder.native_category, "Water wonder retained invented zero style");
    SavedMapProjection ordered{{2, 3}, true, false, {}};
    for (std::uint64_t i = 6; i-- > 0;) {
        SavedMapCellProjection cell;
        cell.index = i;
        cell.x = i % 2;
        cell.y = i / 2;
        cell.native_terrain_mask = 1u << i;
        cell.native_terrain_mask_known = true;
        cell.native_category = static_cast<std::uint8_t>(i);
        cell.auxiliary = SavedTerrainAuxiliaryPlot{0, 2, static_cast<std::int32_t>(i), -1};
        ordered.cells.push_back(cell);
    }
    const auto source = project_saved_map_terrain(ordered);
    const unsigned indices[] = {0, 2, 4, 1, 3, 5};
    for (std::size_t i = 0; i < 6; ++i)
        check(source.cells[i].native_terrain_mask == (1u << indices[i]) &&
                  source.cells[i].native_category == indices[i] && source.cells[i].auxiliary &&
                  source.cells[i].auxiliary->continent == static_cast<std::int32_t>(indices[i]),
              "Terrain source lost x-major order");
}
void content_features() {
    using namespace civ5::content;
    // Independent standard CRC vectors, with the source FCRC final convention.
    check(saved_info_type_hash("") == 0xffffffffu && saved_info_type_hash("123456789") == 0x340bc6d9u,
          "FString CRC initial/final convention changed");
    check(saved_info_type_hash("FEATURE_FOREST") == 0x0a220944 &&
              saved_info_type_hash("FEATURE_REEF") == 0xeb13f3d7 &&
              saved_info_type_hash("FEATURE_FUJI") == 0x3f699131 &&
              saved_info_type_hash("feature_fuji") == 0x6b9af725,
          "Feature hashing changed case or included the terminator");
    rejects([&] { (void)saved_info_type_hash(std::string_view("A\0B", 3)); });
    SavedTerrainFeatureCatalog catalog;
    {
        RulesDatabase rules;
        rules.apply_sql("CREATE TABLE Features(ID INTEGER PRIMARY KEY, Type TEXT, NaturalWonder INTEGER);"
                        "INSERT INTO Features VALUES(-1,'IGNORED',1),(7,'FEATURE_FOREST',0),"
                        "(19,'FEATURE_REEF',1),(4294967297,'FEATURE_CUSTOM_WONDER',-3);"
                        "CREATE TABLE Remapper(name TEXT); INSERT INTO Remapper VALUES('Features');");
        catalog = saved_terrain_features(rules);
        check(catalog.definitions == 3, "Feature collection included its negative sentinel");
        const auto reef = project({{"terrain_type", 6}, {"feature_type_hash", 0xeb13f3d7},
                                    {"continent_style", 4}}, catalog.resolve);
        check(reef.native_terrain_mask_known && reef.native_terrain_mask == 0x100000 && reef.native_category == 4,
              "Hashed feature did not reach native wonder classification");
        const auto auxiliary = project({{"terrain_type", 6}, {"plot_type", 3},
            {"feature_type_hash", 0xeb13f3d7}, {"continent_style", 255}}, catalog.resolve);
        check(auxiliary.auxiliary == SavedTerrainAuxiliaryPlot{6, 3, 255, 19},
              "Auxiliary source lost resolved ID or raw terrain/continent before wonder override");
        const auto ordinary = project({{"terrain_type", 0}, {"plot_type", 0},
            {"feature_type", 7}, {"continent_style", -128}}, catalog.resolve);
        check(ordinary.auxiliary == SavedTerrainAuxiliaryPlot{0, 0, -128, 7},
              "Numeric auxiliary source or signed continent changed");
        for (const auto field : {"plot_type", "continent_style"}) {
            std::vector<NamedSaveInteger> incomplete{{"terrain_type", 5}, {"feature_type", -1}};
            incomplete.push_back({field, 0});
            check(!project(incomplete).auxiliary, "Missing auxiliary input was invented");
        }
        check(project({{"terrain_type", 5}, {"plot_type", 3}, {"feature_type_hash", 0},
                       {"continent_style", 128}}, catalog.resolve).auxiliary ==
                  SavedTerrainAuxiliaryPlot{5, 3, 128, -1}, "No-feature hash sentinel was not preserved");
        check(!project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type", 4294967297ll},
                        {"continent_style", 4}}, catalog.resolve).auxiliary,
              "Wide feature ID silently narrowed for original predicates");
        check(!project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type_hash", 0xeb13f3d7},
                        {"continent_style", 256}}, catalog.resolve).auxiliary,
              "Out-of-range saved continent silently narrowed");
        const auto custom = project({{"terrain_type", 0}, {"plot_type", 2},
                                      {"feature_type", 4294967297ll}}, catalog.resolve);
        check(custom.native_terrain_mask_known && custom.native_terrain_mask == 0x40000,
              "Wide feature ID or nonzero NaturalWonder flag changed");
        // Ignore a later rules mutation: a decoder owns one coherent snapshot.
        rules.apply_sql("UPDATE Features SET NaturalWonder=1, Type='FEATURE_FUJI' WHERE ID=7;");
        const auto current = saved_terrain_features(rules);
        check(project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type", 7}}, current.resolve)
                      .native_terrain_mask == 0x40000,
              "Current rules override was ignored");
        rules.apply_sql("DELETE FROM Features WHERE ID=-1;");
        check(rules.remap_vp_ids().all_succeeded, "Test rules remapping failed");
        const auto remapped = saved_terrain_features(rules);
        check(project({{"terrain_type", 6}, {"feature_type", 1}}, remapped.resolve).native_terrain_mask == 0x100000,
              "Feature resolver used pre-remap IDs");
        check(project({{"terrain_type", 6}, {"plot_type", 3}, {"feature_type_hash", 0xeb13f3d7},
                       {"continent_style", 4}}, remapped.resolve).auxiliary ==
                  SavedTerrainAuxiliaryPlot{6, 3, 4, 1}, "Auxiliary source used a stale pre-remap ID");
        rejects([&] { (void)saved_terrain_features(rules, 2); });
    }
    check(project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type", 7}}, catalog.resolve)
                  .native_terrain_mask == 1, "Resolver borrowed mutable/destroyed database state");
    SavedMapCellProjection unknown;
    check(!catalog.resolve(unknown), "Missing feature identity resolved");
    unknown.feature_type = INT64_MAX;
    check(!catalog.resolve(unknown), "Unknown numeric feature acquired a definition");
    unknown.feature_type.reset();
    unknown.feature_type_hash = 0x1eb13f3d7ull;
    check(!catalog.resolve(unknown), "Oversized feature hash silently narrowed");
    unknown.feature_type_hash = 0xeb13f3d7;
    unknown.feature_type = 7;
    check(!catalog.resolve(unknown), "Contradictory feature identities silently resolved");
    const auto by_id = [](const auto &features, std::int64_t id) {
        SavedMapCellProjection cell;
        cell.feature_type = id;
        return features.resolve(cell);
    };
    const auto by_hash = [](const auto &features, std::string_view type) {
        SavedMapCellProjection cell;
        cell.feature_type_hash = saved_info_type_hash(type);
        return features.resolve(cell);
    };
    {
        RulesDatabase rules;
        rules.apply_sql("CREATE TABLE Features(ID,Type,NaturalWonder);"
            "INSERT INTO Features VALUES(0,'A',0),(0,'B',1),(1,'C',1),(5,'HOLE',0),"
            "(9223372036854775807,'WIDE',0),(9223372036854775807,'WIDER',1);");
        const auto features = saved_terrain_features(rules);
        check(features.definitions == 6 && by_id(features, 0)->type == "A" &&
                  by_id(features, 1)->type == "B" && by_id(features, 2)->type == "C" &&
                  by_id(features, 2)->id == 2 && by_id(features, 3)->id == 3 &&
                  by_id(features, 3)->type.empty() && !by_id(features, 3)->natural_wonder &&
                  by_id(features, INT64_MAX)->type == "WIDE",
              "Features overwrote a duplicate ID, filled a hole or narrowed a wide index");
        const auto hole = project({{"terrain_type", 0}, {"plot_type", 2}, {"feature_type", 3}}, features.resolve);
        check(hole.native_terrain_mask_known && hole.native_terrain_mask == 1,
              "Known null feature slot did not follow the source null-info classification");
        check(by_hash(features, "B")->type == "A" && by_hash(features, "C")->type == "B" &&
                  by_hash(features, "C")->id == 1 && by_hash(features, "WIDER")->type == "WIDE",
              "Feature hash bypassed global authored ID then collection-slot lookup");
    }
    {
        RulesDatabase rules;
        rules.apply_sql("CREATE TABLE Features(ID,Type,NaturalWonder);"
            "INSERT INTO Features VALUES(0,'plumless',0),(1,'buckeroo',1),(2,NULL,'true'),(3,'',1.5);"
            "CREATE TABLE OtherInfos(ID,Type); INSERT INTO OtherInfos VALUES(2,'buckeroo');");
        const auto features = saved_terrain_features(rules, rules.cache_info_types());
        check(saved_info_type_hash("plumless") == saved_info_type_hash("buckeroo") &&
                  by_hash(features, "plumless")->id == 2 && by_hash(features, "plumless")->natural_wonder &&
                  by_hash(features, "plumless")->type.empty() && by_hash(features, "")->id == 3 &&
                  !by_hash(features, "")->natural_wonder,
              "Cross-table hash collision, NULL Type or GetBool dispatch changed");
        rules.apply_sql("UPDATE OtherInfos SET ID=1;");
        check(by_hash(features, "plumless")->id == 2 &&
                  by_hash(saved_terrain_features(rules), "plumless")->id == 1,
              "Feature resolver borrowed a mutable type catalog");
    }
    {
        RulesDatabase rules;
        rules.apply_sql("CREATE TABLE Features(ID,Type,NaturalWonder); INSERT INTO Features VALUES"
            "(0,'BOOL_TRUE','true'),(1,'BOOL_UPPER','Tomato'),(2,'BOOL_DIGIT','1'),"
            "(3,'BOOL_REAL',1.5),(4,'BOOL_WIDE',4294967296),(5,'BOOL_BLOB',x'74'),"
            "(6,'BOOL_EMPTY',''),(7,'BOOL_NULL',NULL),('bad','TEXT_ID',0);");
        const auto features = saved_terrain_features(rules);
        const std::vector<bool> wonders{true,true,false,false,true,false,false,false,false};
        for (std::size_t i = 0; i < wonders.size(); ++i)
            check(by_id(features, static_cast<std::int64_t>(i))->natural_wonder == wonders[i],
                  "Feature boolean did not use native GetBool semantics");
        check(by_id(features, 8)->type == "TEXT_ID" && by_hash(features, "TEXT_ID")->id == 0,
              "Feature loading rejected a coerced ID or used it as the append slot");
    }
    {
        RulesDatabase rules;
        rules.apply_sql("CREATE TABLE Features(ID,Type); INSERT INTO Features VALUES(0,'NO_FLAG'),(1,NULL);");
        const auto features = saved_terrain_features(rules);
        check(!by_id(features, 0)->natural_wonder && by_id(features, 1)->type.empty(),
              "Missing NaturalWonder or NULL Type did not use source defaults");
        rules.apply_sql("DROP TABLE Features; CREATE TABLE Features(ID); INSERT INTO Features VALUES(0);");
        check(by_id(saved_terrain_features(rules), 0)->type.empty(), "Missing Type rejected numeric Features");
        rules.apply_sql("DROP TABLE Features; CREATE TABLE Features(Type);");
        rejects([&] { (void)saved_terrain_features(rules); });
    }
}
std::uint32_t u32(std::istream &input) {
    unsigned char bytes[4];
    if (!input.read(reinterpret_cast<char *>(bytes), 4))
        throw std::runtime_error("Truncated oracle fixture");
    return std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) |
           (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
}
std::size_t compare_oracle(const char *path) {
    std::ifstream input(path, std::ios::binary);
    char magic[8];
    check(bool(input.read(magic, 8)) && std::string_view(magic, 8) == "C5TPRJ01",
          "Invalid oracle fixture signature");
    const auto count = u32(input);
    check(count > 0 && count <= 100000, "Invalid oracle case count");
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto terrain = std::bit_cast<std::int32_t>(u32(input));
        const auto relief = std::bit_cast<std::int32_t>(u32(input));
        const auto style = std::bit_cast<std::int32_t>(u32(input));
        const auto kind = u32(input), length = u32(input);
        check(kind <= 3 && length <= 256, "Invalid oracle feature");
        std::string name(length, '\0');
        check(bool(input.read(name.data(), length)), "Truncated oracle feature name");
        const auto mask = u32(input), category = u32(input);
        check(category <= 255, "Invalid oracle category");
        // Both wire forms must reach the same resolved-feature classification.
        for (bool hashed : {false, true}) {
            const auto cell = project({{"terrain_type", terrain}, {"plot_type", relief},
                                       {"continent_style", style},
                                       {hashed ? "feature_type_hash" : "feature_type",
                                        hashed ? (kind ? 0xfedcba98ll : 0ll) : (kind ? 7ll : -1ll)}},
                [&](const auto &value) -> std::optional<SavedTerrainFeature> {
                    check(kind != 0 && (hashed ? value.feature_type_hash == 0xfedcba98 : value.feature_type == 7),
                          "Resolver bypassed the saved feature identity");
                    return SavedTerrainFeature{kind == 3, name};
                });
            if (!cell.native_terrain_mask_known || cell.native_terrain_mask != mask || cell.native_category != category)
                throw std::runtime_error("Native classification mismatch at oracle case " + std::to_string(i) +
                    ": terrain=" + std::to_string(terrain) + ", relief=" + std::to_string(relief) + ", " + name +
                    ", expected mask=" + std::to_string(mask) + ", actual=" + std::to_string(cell.native_terrain_mask));
            ++checks;
        }
    }
    check(input.peek() == std::char_traits<char>::eof(), "Trailing oracle data");
    return count;
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2)
            throw std::runtime_error("Usage: civis_saved_terrain_tests [oracle.bin]");
        guards();
        content_features();
        const auto cases = argc == 2 ? compare_oracle(argv[1]) : 0;
        std::cout << "PASS saved terrain: " << checks << " checks, " << cases
                  << " native cases, " << cases * 2 << " wire-form comparisons\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL saved terrain: " << e.what() << '\n';
        return 1;
    }
}
