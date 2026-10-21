#include "civ5/content/landmark_art_registry.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "../third_party/sqlite/sqlite3.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
namespace {
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::uint32_t word() {
        if (bytes.size() < 4) throw std::runtime_error("Short landmark oracle word");
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i) result |= std::uint32_t(bytes[i]) << (8 * i);
        bytes = bytes.subspan(4);
        return result;
    }
    std::span<const std::uint8_t> blob() {
        const auto length = word();
        if (length > bytes.size()) throw std::runtime_error("Short landmark oracle blob");
        const auto result = bytes.first(length);
        bytes = bytes.subspan(length);
        return result;
    }
};
template<std::size_t Count>
std::array<std::uint32_t, Count> words(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != Count * 4) throw std::runtime_error("Wrong material table extent");
    Reader reader{bytes};
    std::array<std::uint32_t, Count> result{};
    for (auto &value : result) value = reader.word();
    return result;
}
std::string registry_sha256(const civ5::content::LandmarkArtRegistry &registry) {
    std::vector<std::uint8_t> bytes;
    const auto word = [&](std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i)
            bytes.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
    };
    word(static_cast<std::uint32_t>(registry.art_types.size()));
    for (const auto &name : registry.art_types) {
        word(static_cast<std::uint32_t>(name.size()));
        bytes.insert(bytes.end(), name.begin(), name.end());
    }
    word(static_cast<std::uint32_t>(registry.materials.size()));
    for (const auto &row : registry.materials) {
        word(row.improvement);
        word(row.resource);
        word(row.era);
        word(row.state);
    }
    return civ5::runtime::identify_bytes(bytes).sha256;
}
std::string active_art_sha256(std::span<const civ5::content::ActiveLandmarkArtRow> rows) {
    std::vector<std::uint8_t> bytes;
    const auto word = [&](std::uint64_t value) {
        for (unsigned i = 0; i < 8; ++i)
            bytes.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
    };
    const auto string = [&](std::string_view value) {
        word(value.size());
        bytes.insert(bytes.end(), value.begin(), value.end());
    };
    const auto nullable = [&](const std::optional<std::string> &value) {
        bytes.push_back(value ? 1 : 0);
        if (value) string(*value);
    };
    string("C5ACTIVE01");
    word(rows.size());
    for (const auto &row : rows) {
        string(row.improvement); string(row.resource); string(row.era); string(row.state);
        string(row.layout_handler);
        word(std::bit_cast<std::uint64_t>(row.scale));
        nullable(row.model); nullable(row.tech);
        word(static_cast<std::uint64_t>(row.terrain_contour));
    }
    return civ5::runtime::identify_bytes(bytes).sha256;
}
void verify_oracle(const fs::path &path, const civ5::content::LandmarkArtRegistry &registry) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open landmark oracle");
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    if (bytes.size() < 8 || std::string_view(reinterpret_cast<const char *>(bytes.data()), 8) != "C5LAR001")
        throw std::runtime_error("Wrong landmark oracle header");
    Reader expected{std::span(bytes).subspan(8)};
    const auto type_count = expected.word(), row_count = expected.word();
    if (type_count != registry.art_types.size() || row_count != registry.materials.size())
        throw std::runtime_error("Saved landmark oracle cardinality differs");
    for (const auto &name : registry.art_types) {
        const auto source = expected.blob();
        if (name != std::string_view(reinterpret_cast<const char *>(source.data()), source.size()))
            throw std::runtime_error("Saved landmark art ID differs from original");
    }
    for (const auto &row : registry.materials) {
        if (row.improvement != expected.word() || row.resource != expected.word() ||
            row.era != expected.word() || row.state != expected.word())
            throw std::runtime_error("Saved landmark source row differs from original");
    }
    const auto farm = expected.word(), wheat = expected.word();
    if (registry.id("ART_DEF_IMPROVEMENT_FARM") != farm ||
        registry.id("ART_DEF_RESOURCE_WHEAT") != wheat)
        throw std::runtime_error("Saved farm/wheat art ID differs from original");
    auto farm_table = words<20>(expected.blob());
    const auto expected_farm = words<20>(expected.blob());
    civ5::terrain::populate_scene_polygon_material_rows(farm_table, farm, registry.materials);
    if (farm_table != expected_farm)
        throw std::runtime_error("Saved farm material table differs from retail 628920");
    const std::array<std::uint32_t, 3> special{expected.word(), expected.word(), expected.word()};
    for (std::size_t i = 0; i < special.size(); ++i) {
        const char *const names[]{"ART_DEF_IMPROVEMENT_MOAI_STATUES",
                                  "ART_DEF_IMPROVEMENT_PONTOON_BRIDGE",
                                  "ART_DEF_IMPROVEMENT_FEITORIA"};
        if (registry.id(names[i]) != special[i])
            throw std::runtime_error("Saved special art ID differs from original");
    }
    auto special_table = words<21>(expected.blob());
    const auto expected_special = words<21>(expected.blob());
    civ5::terrain::populate_scene_special_improvement_material_rows(
        special_table, special, registry.materials);
    if (special_table != expected_special || !expected.bytes.empty())
        throw std::runtime_error("Saved special material table differs from retail 70D500");
    std::cout << "oracle_exact_words=" << row_count * 4 + 82 << '\n';
}
void verify_core_db(const fs::path &path, const civ5::content::LandmarkArtLoadResult &art) {
    using namespace civ5::content;
    RulesDatabase rules;
    rules.apply_sql("CREATE TABLE ArtDefine_LandmarkTypes("
        "Type TEXT NOT NULL,LandmarkType TEXT NOT NULL,FriendlyName TEXT);"
        "CREATE TABLE ArtDefine_Landmarks("
        "Era TEXT DEFAULT 'Any',State TEXT DEFAULT 'Any',Scale FLOAT DEFAULT 1,"
        "ImprovementType TEXT NOT NULL,LayoutHandler TEXT NOT NULL,ResourceType TEXT NOT NULL,"
        "Model TEXT,TerrainContour INTEGER DEFAULT 0,Tech TEXT);");
    seed_landmark_type_rows(rules, art.type_rows);
    seed_landmark_art_rows(rules, art.rows);
    const auto active_art=read_active_landmark_art_rows(rules);
    const auto encoded = path.u8string();
    const std::string filename(reinterpret_cast<const char *>(encoded.data()), encoded.size());
    sqlite3 *raw = nullptr;
    const auto opened = sqlite3_open_v2(filename.c_str(), &raw, SQLITE_OPEN_READONLY, nullptr);
    std::unique_ptr<sqlite3, decltype(&sqlite3_close_v2)> original(raw, sqlite3_close_v2);
    if (opened != SQLITE_OK || !original)
        throw std::runtime_error("Cannot open original core art database read-only");
    const auto compare = [&](const char *table, const char *columns) {
        const std::string sql = std::string("SELECT ") + columns + " FROM " + table + " ORDER BY rowid";
        std::vector<std::vector<DatabaseValue>> actual;
        if (std::string_view(table) == "ArtDefine_Landmarks") {
            actual.reserve(active_art.size());
            for (const auto &row : active_art)
                actual.push_back({row.era,row.state,row.scale,row.improvement,row.layout_handler,
                    row.resource,row.model ? DatabaseValue{*row.model} : DatabaseValue{std::monostate{}},
                    row.terrain_contour,
                    row.tech ? DatabaseValue{*row.tech} : DatabaseValue{std::monostate{}}});
        } else rules.query({sql, {}}, [&](std::span<const DatabaseValue> row) {
            actual.emplace_back(row.begin(), row.end());
        });
        sqlite3_stmt *raw_statement = nullptr;
        if (sqlite3_prepare_v2(original.get(), sql.c_str(), -1, &raw_statement, nullptr) != SQLITE_OK)
            throw std::runtime_error(std::string("Cannot query original core table: ") + table);
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw_statement, sqlite3_finalize);
        std::size_t index = 0;
        int step = SQLITE_OK;
        while ((step = sqlite3_step(statement.get())) == SQLITE_ROW) {
            if (index >= actual.size() || sqlite3_column_count(statement.get()) !=
                static_cast<int>(actual[index].size()))
                throw std::runtime_error(std::string("Original core art row count differs: ") + table);
            for (std::size_t column = 0; column < actual[index].size(); ++column) {
                const auto kind = sqlite3_column_type(statement.get(), static_cast<int>(column));
                DatabaseValue expected;
                if (kind == SQLITE_INTEGER)
                    expected = static_cast<std::int64_t>(sqlite3_column_int64(statement.get(), static_cast<int>(column)));
                else if (kind == SQLITE_FLOAT)
                    expected = sqlite3_column_double(statement.get(), static_cast<int>(column));
                else if (kind == SQLITE_TEXT) {
                    const auto *bytes = sqlite3_column_text(statement.get(), static_cast<int>(column));
                    const auto length = sqlite3_column_bytes(statement.get(), static_cast<int>(column));
                    expected = std::string(reinterpret_cast<const char *>(bytes), static_cast<std::size_t>(length));
                } else if (kind != SQLITE_NULL)
                    throw std::runtime_error("Unsupported original core art value");
                if (actual[index][column] != expected)
                    throw std::runtime_error(std::string("Original core art value differs: ") +
                        table + " row=" + std::to_string(index) + " column=" + std::to_string(column));
            }
            ++index;
        }
        if (step != SQLITE_DONE || index != actual.size())
            throw std::runtime_error(std::string("Original core art row count differs: ") + table);
        std::cout << "original_core_exact_rows=" << table << ':' << index << '\n';
    };
    compare("ArtDefine_LandmarkTypes", "Type,LandmarkType,FriendlyName");
    compare("ArtDefine_Landmarks",
        "Era,State,Scale,ImprovementType,LayoutHandler,ResourceType,Model,TerrainContour,Tech");
    std::cout << "original_core_db_sha256=" << civ5::runtime::identify_file(path).sha256 << '\n';
}
} // namespace

int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        fs::path save, game_root, mount_plan, oracle, core_db;
        std::vector<fs::path> mod_roots;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (option == L"--help") {
                std::cout << "civ5_saved_landmark_art --save FILE_OR_DIRECTORY --game-root DIRECTORY "
                             "--mount-plan PLAN.xml [--oracle oracle.bin] [--core-db Civ5CoreDatabase.db] "
                             "[--mods DIRECTORY]...\n";
                return 0;
            }
            if (i + 1 == argc)
                throw std::invalid_argument("Missing option value");
            const fs::path value = argv[++i];
            if (option == L"--save") save = value;
            else if (option == L"--game-root") game_root = value;
            else if (option == L"--mount-plan") mount_plan = value;
            else if (option == L"--oracle") oracle = value;
            else if (option == L"--core-db") core_db = value;
            else if (option == L"--mods") mod_roots.push_back(value);
            else throw std::invalid_argument("Unknown option");
        }
        if (save.empty() || game_root.empty() || mount_plan.empty())
            throw std::invalid_argument("Supply --save, --game-root and --mount-plan");

        const auto loaded = content::load_saved_landmark_art_registry(
            save, game_root, mount_plan, {true, true, true});
        if (!oracle.empty()) verify_oracle(oracle, loaded.art.registry);
        if (!core_db.empty()) verify_core_db(core_db, loaded.art);
        std::cout << "save_sha256=" << loaded.save_sha256 << '\n'
                  << "mount_plan_sha256=" << loaded.mount_plan_sha256 << '\n'
                  << "selected_dlc=" << loaded.selected_dlc << '\n'
                  << "indexed_loose_files=" << loaded.indexed_loose_files << '\n'
                  << "landmark_documents=" << loaded.art.documents.size() << '\n'
                  << "landmark_type_documents=" << loaded.art.type_documents << '\n'
                  << "landmark_type_rows=" << loaded.art.type_rows.size() << '\n'
                  << "landmark_materials=" << loaded.art.registry.materials.size() << '\n'
                  << "landmark_art_types=" << loaded.art.registry.art_types.size() << '\n'
                  << "landmark_registry_sha256=" << registry_sha256(loaded.art.registry) << '\n';
        for (const auto &document : loaded.art.documents) {
            std::cout << "document=" << document.request << " package=";
            if (document.package) std::cout << loaded.catalog.packages[*document.package].id;
            else std::cout << "base";
            std::cout << " mount=";
            if (document.mount_id) std::cout << *document.mount_id;
            else std::cout << "default";
            std::cout << " first_row=" << document.first_row
                      << " rows=" << document.row_count << '\n';
        }
        for (const char *name : {"ART_DEF_IMPROVEMENT_FARM", "ART_DEF_IMPROVEMENT_MOAI_STATUES",
                                 "ART_DEF_IMPROVEMENT_FISHING_BOATS", "ART_DEF_RESOURCE_HORSE"})
            std::cout << "art_id=" << name << ':' << loaded.art.registry.id(name) << '\n';
        if (!mod_roots.empty()) {
            const auto modded = content::load_saved_mod_landmark_art_registry(
                loaded, save, game_root, mount_plan, mod_roots);
            const auto &modded_art = modded.registry;
            if (modded.active_rows.size() != modded_art.materials.size())
                throw std::runtime_error("Active landmark metadata/material count differs");
            const auto model_rows = std::count_if(modded.active_rows.begin(), modded.active_rows.end(),
                [](const auto &row) { return row.model.has_value(); });
            std::cout << "selected_mods=" << modded.selected_mods << '\n'
                      << "mod_actions=" << modded.actions << '\n';
            for (const auto &failure : modded.failures)
                std::cout << "failed_mod_action=" << failure.source
                          << " error=" << failure.error << '\n';
            std::cout << "mod_action_failures=" << modded.failures.size() << '\n';
            std::cout << "modded_art_profile_verified="
                      << (modded.profile_verified ? "true" : "false")
                      << '\n';
            std::cout << "modded_art_materials=" << modded_art.materials.size() << '\n'
                      << "modded_art_types=" << modded_art.art_types.size() << '\n'
                      << "modded_art_model_rows=" << model_rows << '\n'
                      << "modded_art_full_sha256=" << active_art_sha256(modded.active_rows) << '\n'
                      << "modded_art_registry_sha256=" << registry_sha256(modded_art) << '\n'
                      << "modded_farm_art_id=" << modded_art.id("ART_DEF_IMPROVEMENT_FARM") << '\n'
                      << "modded_reforestation_art_id="
                      << modded_art.id("ART_DEF_IMPROVEMENT_PLANT_FOREST_3UC") << '\n';
            std::cout << "rules_table=ArtDefine_Landmarks:" << modded_art.materials.size() << '\n'
                      << "rules_table=ArtDefine_LandmarkTypes:" << modded.landmark_type_rows << '\n';
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "saved landmark art: " << error.what() << '\n';
        return 2;
    }
}
