#include "civ5/assets/asset_source.hpp"
#include "civ5/content/base_gameplay.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/content/rules_bootstrap.hpp"
#include "civ5/content/vp_profile.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace {
struct SourceReplayResult {
    std::uint64_t applied = 0;
    std::uint64_t failed = 0;
};
SourceReplayResult replay_native_base(civ5::content::RulesDatabase &database, const fs::path &game_root,
                                      const fs::path &mount_path) {
    using namespace civ5;
    const auto mount_identity = runtime::identify_file(mount_path);
    const auto mounts = content::read_loose_resource_plan(mount_path);
    if (runtime::identify_file(mount_path).sha256 != mount_identity.sha256)
        throw std::runtime_error("Base resource plan changed while indexing");
    const auto report = content::apply_native_base_gameplay(database, game_root, mounts.resources);
    SourceReplayResult result{report.schema_applied ? 1u : 0u, report.failed};
    std::cout << "native_engine_schema=" << (report.schema_applied ? "applied" : "failed")
              << " sha256=" << report.schema_identity.sha256
              << " source=" << report.schema_source << '\n';
    for (std::size_t i = 0; i < report.sources.size(); ++i) {
        const auto &source = report.sources[i];
        std::cout << "base_request=" << (i + 1) << " status="
                  << (source.applied ? "applied" : source.selected ? "xml_error" : "missing")
                  << " name=" << source.request;
        if (source.selected) {
            std::cout << " sha256=" << source.identity.sha256 << " source=" << source.source << " mount=";
            if (source.mount_id)
                std::cout << *source.mount_id;
            else
                std::cout << "default";
        }
        if (!source.error.empty())
            std::cout << " error=" << source.error;
        std::cout << '\n';
    }
    result.applied += report.applied;
    std::cout << "native_base_requests=" << report.sources.size() << " selected=" << report.selected
              << " missing=" << report.missing << " failed_base_sources=" << report.failed
              << " executable_sha256=" << report.recipe.executable.sha256
              << " mount_plan_sha256=" << mount_identity.sha256 << '\n';
    return result;
}
SourceReplayResult replay_sources(civ5::content::RulesDatabase &database, const fs::path &source_list,
                                  const char *label, bool localization_schema = false,
                                  const civ5::content::GameDataPackageTag *package_tag = nullptr) {
    using namespace civ5;
    std::ifstream list(source_list, std::ios::binary);
    if (!list)
        throw std::runtime_error("Cannot read source list: " + assets::path_utf8(source_list));
    SourceReplayResult result;
    std::uint64_t ordinal = 0;
    std::string line;
    while (std::getline(list, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (ordinal == 0 && line.starts_with("\xef\xbb\xbf"))
            line.erase(0, 3);
        if (line.empty())
            continue;
        const auto path = fs::path(std::u8string(line.begin(), line.end()));
        if (!path.is_absolute())
            throw std::runtime_error("Sources must be absolute paths");
        try {
            const auto identity = runtime::identify_file(path);
            std::string xml_error;
            if (package_tag) {
                if (identity.bytes > 64ULL * 1024 * 1024)
                    throw std::runtime_error("DLC XML source exceeds 64 MiB inspection budget");
                try {
                    database.apply_mod_xml(content::read_game_data_xml(path), *package_tag);
                } catch (const std::exception &error) {
                    xml_error = error.what();
                }
            } else if (path.extension() == ".xml" || path.extension() == ".XML")
                database.apply_xml(content::read_game_data_xml(path));
            else if (path.extension() == ".sql" || path.extension() == ".SQL") {
                if (identity.bytes > 64ULL * 1024 * 1024)
                    throw std::runtime_error("SQL source exceeds 64 MiB inspection budget");
                std::ifstream input(path, std::ios::binary);
                std::string text(static_cast<std::size_t>(identity.bytes), '\0');
                input.read(text.data(), static_cast<std::streamsize>(text.size()));
                if (!input)
                    throw std::runtime_error("Truncated SQL source");
                if (localization_schema && ordinal == 0)
                    database.apply_localization_schema(text);
                else
                    database.apply_sql(text);
            } else
                throw std::runtime_error("Expected .xml or .sql source");
            if (runtime::identify_file(path).sha256 != identity.sha256)
                throw std::runtime_error("Source changed during replay");
            ++ordinal;
            if (xml_error.empty())
                ++result.applied;
            else
                ++result.failed;
            std::cout << label << '=' << ordinal;
            if (package_tag)
                std::cout << " status=" << (xml_error.empty() ? "applied" : "xml_error")
                          << " package=" << package_tag->value;
            std::cout << " sha256=" << identity.sha256 << " source=" << line;
            if (!xml_error.empty())
                std::cout << " error=" << xml_error;
            std::cout << '\n';
        } catch (const std::exception &error) {
            throw std::runtime_error(line + ": " + error.what());
        }
    }
    if (!list.eof() || (ordinal == 0 && !package_tag))
        throw std::runtime_error("No complete source list");
    return result;
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        if (argc < 2)
            throw std::runtime_error(
                "Usage: civis_rules_replay SOURCES.txt (one UTF-8 absolute XML/SQL "
                "path per line in explicit source order), or --native-base GAME_ROOT --mount-plan FILE "
                "[--vp-bundle ROOT "
                "--component NAME --mod ID ...] [--localization-base SOURCES.txt "
                "--localization-dlc SOURCES.txt ...] [--dlc-game-data GUID SOURCES.txt ...] "
                "[--vp-remap-gamecore FILE]");
        fs::path bundle;
        fs::path localization_base;
        fs::path native_game_root, mount_plan;
        fs::path remap_gamecore;
        int first_option = 2;
        if (std::wstring(argv[1]) == L"--native-base") {
            if (argc < 3)
                throw std::runtime_error("Native base replay needs the original installation root");
            native_game_root = argv[2];
            first_option = 3;
        }
        std::vector<fs::path> localization_packages;
        std::vector<std::pair<content::GameDataPackageTag, fs::path>> dlc_sources;
        std::string component;
        std::vector<std::string> selected_mods;
        for (int i = first_option; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (++i == argc)
                throw std::runtime_error("Missing replay option value");
            if (option == L"--mount-plan") {
                if (!mount_plan.empty())
                    throw std::runtime_error("Only one base resource mount plan is allowed");
                mount_plan = argv[i];
            } else if (option == L"--vp-remap-gamecore") {
                if (!remap_gamecore.empty())
                    throw std::runtime_error("Only one ID remapper GameCore is allowed");
                remap_gamecore = argv[i];
            } else if (option == L"--vp-bundle")
                bundle = argv[i];
            else if (option == L"--component")
                component = assets::path_utf8(argv[i]);
            else if (option == L"--mod")
                selected_mods.push_back(assets::path_utf8(argv[i]));
            else if (option == L"--localization-base") {
                if (!localization_base.empty())
                    throw std::runtime_error("Only one base localization source list is allowed");
                localization_base = argv[i];
            } else if (option == L"--localization-dlc")
                localization_packages.emplace_back(argv[i]);
            else if (option == L"--dlc-game-data") {
                auto tag = content::dlc_game_data_tag(assets::path_utf8(argv[i]));
                if (++i == argc)
                    throw std::runtime_error("DLC game data needs GUID and source list");
                dlc_sources.emplace_back(std::move(tag), fs::path(argv[i]));
            } else
                throw std::runtime_error("Unknown replay option");
        }
        if (native_game_root.empty() != mount_plan.empty())
            throw std::runtime_error("--native-base and --mount-plan must be supplied together");
        if (!remap_gamecore.empty()) {
            const auto identity = runtime::identify_file(remap_gamecore);
            if (identity.bytes != 16980992 ||
                identity.sha256 != "30872ae3ecce6607613aaca76084d19d96785ba2156bd21fc304612143e2ef25")
                throw std::runtime_error("Unverified GameCore ID-remapping implementation");
        }
        std::optional<content::VpProfile> profile;
        if (!bundle.empty()) {
            if (component.empty() || selected_mods.empty())
                throw std::runtime_error("Bundle replay needs explicit component and ordered mods");
            profile = content::build_vp_542_profile(bundle, content::vp_component(component));
        } else if (!component.empty() || !selected_mods.empty())
            throw std::runtime_error("Profile options need --vp-bundle");
        if (localization_base.empty() && !localization_packages.empty())
            throw std::runtime_error("DLC localization needs --localization-base");
        content::RulesDatabase database;
        std::uint64_t applied = 0;
        std::uint64_t mod_actions = 0, failed_actions = 0;
        std::uint64_t failed_dlc_sources = 0;
        std::uint64_t failed_base_sources = 0;
        bool failed_postprocessing = false;
        std::cout << "runtime_bits=" << sizeof(void *) * 8
                  << " sqlite=" << content::RulesDatabase::sqlite_version()
                  << " can_continue=false source_selection="
                  << (native_game_root.empty() ? "explicit" : "native_requests_explicit_mounts") << '\n';
        if (!localization_base.empty()) {
            content::RulesDatabase localization;
            auto text_sources =
                replay_sources(localization, localization_base, "localization_base_applied", true).applied;
            const auto languages = localization.convert_localization_tables();
            for (const auto &package_list : localization_packages) {
                content::RulesDatabase package;
                text_sources +=
                    replay_sources(package, package_list, "localization_dlc_applied", true).applied;
                localization.merge_localization(package);
            }
            localization.track_localization_mod_changes();
            database.attach_localization(localization);
            std::cout << "localization=ATTACHED sources=" << text_sources << " languages=" << languages
                      << " dlc_packages=" << localization_packages.size() << '\n';
        }
        if (native_game_root.empty())
            applied = replay_sources(database, fs::path(argv[1]), "applied").applied;
        else {
            const auto result = replay_native_base(database, native_game_root, mount_plan);
            applied = result.applied;
            failed_base_sources = result.failed;
        }
        for (const auto &[tag, list] : dlc_sources) {
            const auto result = replay_sources(database, list, "dlc_source", false, &tag);
            applied += result.applied;
            failed_dlc_sources += result.failed;
        }
        if (profile) {
            for (const auto &id : selected_mods) {
                const auto &mod = profile->mod(id);
                const auto plan = profile->action_plan(mod, "OnModActivated", "UpdateDatabase");
                const auto report = content::apply_database_actions(database, plan);
                for (std::size_t i = 0; i < report.actions.size(); ++i) {
                    const auto &action = report.actions[i];
                    if (action.status == content::DatabaseActionStatus::applied)
                        ++applied;
                    else
                        ++failed_actions;
                    std::cout << "mod_action=" << ++mod_actions
                              << " status=" << content::database_action_status_name(action.status)
                              << " mod=" << id << " source=" << action.requested_path;
                    if (plan[i].resolved)
                        std::cout << " sha256=" << plan[i].resolved->identity.sha256;
                    if (action.sql) {
                        std::cout << " statements=" << action.sql->completed_statements
                                  << " sqlite_code=" << action.sql->error_code
                                  << " transaction_active=" << action.sql->transaction_active_after;
                        if (!action.sql->release_error.empty())
                            std::cout << " release_error=" << action.sql->release_error;
                    }
                    if (!action.error.empty())
                        std::cout << " error=" << action.error;
                    std::cout << '\n';
                }
            }
        }
        if (!remap_gamecore.empty()) {
            const auto report = database.remap_vp_ids();
            failed_postprocessing = !report.all_succeeded;
            for (const auto &table : report.tables) {
                std::cout << "id_remap_table=" << table.name << " rows=" << table.rows
                          << " eligible=" << table.eligible << " collected_ids=" << table.collected_ids
                          << " attempted=" << table.attempted_updates
                          << " completed=" << table.completed_updates
                          << " negative_id_seen=" << table.negative_id_seen << '\n';
                for (const auto &error : table.errors)
                    std::cout << "id_remap_error table=" << table.name << " stage=" << error.stage
                              << " sqlite_code=" << error.sqlite_code << " error=" << error.message << '\n';
            }
            for (const auto &error : report.errors)
                std::cout << "id_remap_error stage=" << error.stage << " sqlite_code=" << error.sqlite_code
                          << " error=" << error.message << '\n';
            std::cout << "id_remap=" << (report.all_succeeded ? "PASS" : "ERRORS")
                      << " tables=" << report.tables.size()
                      << " gamecore_sha256=30872ae3ecce6607613aaca76084d19d96785ba2156bd21fc304612143e2ef25"
                      << " original_code_executed=false postdefines_applied=false\n";
        }
        database.query(
            {"SELECT name FROM sqlite_master WHERE type='table' ORDER BY name", {}}, [&](auto row) {
                const auto &name = std::get<std::string>(row[0]);
                std::string quoted = "\"";
                for (const auto c : name) {
                    quoted += c;
                    if (c == '"')
                        quoted += c;
                }
                quoted += '"';
                database.query({"SELECT count(*) FROM " + quoted, {}}, [&](auto count) {
                    std::cout << "table=" << name << " rows=" << std::get<std::int64_t>(count[0]) << '\n';
                });
                if (!dlc_sources.empty() && (name == "Civilizations" || name == "Leaders")) {
                    try {
                        database.query({"SELECT PackageID,count(*) FROM " + quoted +
                                            " GROUP BY PackageID ORDER BY PackageID",
                                        {}},
                                       [&](auto group) {
                                           std::cout << "package_table=" << name << " package=";
                                           if (std::holds_alternative<std::monostate>(group[0]))
                                               std::cout << "NULL";
                                           else
                                               std::cout << std::get<std::string>(group[0]);
                                           std::cout << " rows=" << std::get<std::int64_t>(group[1]) << '\n';
                                       });
                    } catch (const std::exception &error) {
                        // An incomplete source schema may lack PackageID. Keep
                        // the recorded XML failure and later sources visible.
                        std::cout << "package_table=" << name << " status=unavailable error=" << error.what()
                                  << '\n';
                    }
                }
            });
        if (!localization_base.empty()) {
            database.query({"SELECT Language,count(*) FROM Localization.LocalizedText GROUP BY Language "
                            "ORDER BY Language",
                            {}},
                           [&](auto row) {
                               std::cout << "localization_language=" << std::get<std::string>(row[0])
                                         << " rows=" << std::get<std::int64_t>(row[1]) << '\n';
                           });
        }
        if (!dlc_sources.empty())
            std::cout << "dlc_packages=" << dlc_sources.size() << " failed_dlc_sources=" << failed_dlc_sources
                      << '\n';
        const bool errors =
            failed_actions || failed_dlc_sources || failed_base_sources || failed_postprocessing;
        std::cout << "replay=" << (errors ? "ACTION_ERRORS" : "PASS") << " applied=" << applied
                  << " mod_actions=" << mod_actions << " failed_actions=" << failed_actions
                  << " native_full_profile_verified=false can_continue=false\n";
        return errors ? 3 : 0;
    } catch (const std::exception &error) {
        std::cerr << "replay=FAIL " << error.what() << '\n';
        return 2;
    }
}
