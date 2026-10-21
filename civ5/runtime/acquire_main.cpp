#include "civ5/assets/asset_source.hpp"
#include "civ5/content/inno_archive.hpp"
#include "civ5/content/mod_acquisition.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <deque>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace {
struct PendingMod {
    civ5::content::ModTarget target;
    bool dependency = false;
};
std::string quote(const std::string &text) {
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (const unsigned char c : text) {
        if (c == '\\' || c == '"') {
            result += '\\';
            result += static_cast<char>(c);
        } else if (c < 32) {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 15];
        } else
            result += static_cast<char>(c);
    }
    return result + '"';
}
void result_json(std::ostream &output, const civ5::content::AcquiredMod &result, bool local = false) {
    output << "\"status\":"
           << quote(result.reused ? "cached_verified"
                    : local       ? "installed_verified"
                                  : "downloaded_verified")
           << ",\"manifest\":" << quote(civ5::assets::path_utf8(result.manifest.path))
           << ",\"cache_package\":" << quote(civ5::assets::path_utf8(result.cache_package))
           << ",\"archive_sha256\":" << quote(result.archive_sha256)
           << ",\"archive_bytes\":" << result.archive_bytes
           << ",\"verified_files\":" << result.verified_files
           << ",\"stale_manifest_md5_files\":[";
    for (std::size_t i = 0; i < result.stale_manifest_md5_files.size(); ++i) {
        if (i) output << ',';
        output << quote(result.stale_manifest_md5_files[i]);
    }
    output << "],\"missing_manifest_assets\":[";
    for(std::size_t i=0;i<result.missing_manifest_assets.size();++i){
        if(i)output<<',';
        output<<quote(result.missing_manifest_assets[i].path);
    }
    output << ']';
}
void bundle_json(std::ostream &output, const civ5::content::InstallerBundle &result) {
    output << "\"status\":"
           << quote(result.reused ? "bundle_cached_profile_required" : "bundle_verified_profile_required")
           << ",\"bundle_root\":" << quote(civ5::assets::path_utf8(result.root))
           << ",\"archive_sha256\":" << quote(result.sha256) << ",\"archive_bytes\":" << result.archive_bytes
           << ",\"verified_files\":" << result.files << ",\"expanded_bytes\":" << result.expanded_bytes
           << ",\"activation_profile_resolved\":false";
}
void plan_json(std::ostream &output, const civ5::content::ModAcquisitionPlan &plan,
               const std::filesystem::path &save, const std::string &save_sha256) {
    output << "{\"schema\":1,\"runtime_bits\":64,\"operation\":\"plan_mods\","
              "\"can_continue\":false,\"save\":"
           << quote(civ5::assets::path_utf8(save)) << ",\"save_sha256\":" << quote(save_sha256)
           << ",\"all_verified\":" << (plan.all_verified ? "true" : "false") << ",\"mods\":[";
    for (std::size_t i = 0; i < plan.items.size(); ++i) {
        if (i)
            output << ',';
        const auto &item = plan.items[i];
        output << "{\"id\":" << quote(item.target.id) << ",\"name\":" << quote(item.target.name)
               << ",\"version\":" << item.target.version << ",\"status\":"
               << quote(civ5::content::mod_acquisition_status_name(item.status)) << ",\"source_index\":";
        if (item.source_index)
            output << *item.source_index;
        else
            output << "null";
        if (!item.reason.empty())
            output << ",\"reason\":" << quote(item.reason);
        if (item.cached)
            output << ",\"cache_package\":"
                   << quote(civ5::assets::path_utf8(item.cached->cache_package))
                   << ",\"archive_sha256\":" << quote(item.cached->archive_sha256)
                   << ",\"verified_files\":" << item.cached->verified_files;
        output << '}';
    }
    output << "]}\n";
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        fs::path save, cache, archive, sources = fs::absolute(argv[0]).parent_path() / "mod_sources.xml";
        fs::path decoder = fs::absolute(argv[0]).parent_path() / "inno-decoder", installer;
        std::string mod_id, expected_hash, bundle, package_root, archive_url, archive_member;
        std::optional<std::uint64_t> version, workshop_id;
        bool plan_only = false, allow_stale_manifest_md5 = false,
             allow_missing_manifest_assets = false;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (option == L"--help") {
                std::cout << "Civis x64 content acquisition\n--save FILE_OR_DIRECTORY "
                             "--cache "
                             "NEW_OR_CIVIS_DIRECTORY [--sources "
                             "CATALOG.xml]\n--archive FILE --mod ID "
                             "--version NUMBER --cache NEW_OR_CIVIS_DIRECTORY "
                             "[--sha256 HASH] [--package-root DIRECTORY] "
                             "[--allow-stale-manifest-md5] [--allow-missing-manifest-assets]\n"
                             "--archive-url HTTPS_URL --mod ID --version NUMBER --cache "
                             "NEW_OR_CIVIS_DIRECTORY [--sha256 HASH] "
                             "[--archive-member FILE] [--package-root DIRECTORY]\n"
                             "--workshop-id ITEM_ID --mod ID --version NUMBER --cache "
                             "NEW_OR_CIVIS_DIRECTORY [--sha256 HASH]\n"
                             "The explicit URL is downloaded on demand; without --sha256 its "
                             "actual hash is pinned in the cache receipt.\nOriginal "
                             "saves, MODS, and Steam subscription state are not "
                             "changed. Acquiring content "
                             "does not enable game continuation.\n"
                             "--save accepts an original .Civ5Save or an imported .CivisSave.\n"
                             "--bundle vp-5.4.2 --cache NEW_OR_CIVIS_DIRECTORY "
                             "[--installer LOCAL_FILE]\n"
                             "--plan --save FILE_OR_DIRECTORY --cache NEW_OR_CIVIS_DIRECTORY "
                             "[--sources CATALOG.xml]\n"
                             "--decoder DIRECTORY selects the pinned x64 Inno decoder "
                             "bundle. Manifest Mod dependencies are queued when their "
                             "exact source is uniquely available.\n";
                return 0;
            }
            if (option == L"--plan") {
                plan_only = true;
                continue;
            }
            if (option == L"--allow-stale-manifest-md5") {
                allow_stale_manifest_md5 = true;
                continue;
            }
            if(option==L"--allow-missing-manifest-assets"){
                allow_missing_manifest_assets=true;
                continue;
            }
            if (++i == argc)
                throw std::runtime_error("Missing option value");
            const fs::path value = argv[i];
            if (option == L"--save")
                save = value;
            else if (option == L"--cache")
                cache = value;
            else if (option == L"--sources")
                sources = value;
            else if (option == L"--archive")
                archive = value;
            else if (option == L"--archive-url") {
                const auto bytes = value.u8string();
                archive_url.assign(bytes.begin(), bytes.end());
            }
            else if (option == L"--archive-member")
                archive_member = assets::path_utf8(value);
            else if (option == L"--decoder")
                decoder = value;
            else if (option == L"--installer")
                installer = value;
            else if (option == L"--bundle")
                bundle = assets::path_utf8(value);
            else if (option == L"--mod")
                mod_id = runtime::ascii_lower(assets::path_utf8(value));
            else if (option == L"--version")
                version = content::decimal(assets::path_utf8(value));
            else if (option == L"--workshop-id")
                workshop_id = content::decimal(assets::path_utf8(value));
            else if (option == L"--sha256")
                expected_hash = runtime::ascii_lower(assets::path_utf8(value));
            else if (option == L"--package-root")
                package_root = assets::path_utf8(value);
            else
                throw std::runtime_error("Unknown option");
        }
        if (cache.empty() || static_cast<int>(!save.empty()) + static_cast<int>(!archive.empty()) +
                                     static_cast<int>(!archive_url.empty()) +
                                     static_cast<int>(workshop_id.has_value()) +
                                     static_cast<int>(!bundle.empty()) !=
                                 1)
            throw std::runtime_error("Supply --cache and exactly one of --save, --archive, --archive-url, --workshop-id, or --bundle");
        if (plan_only && (save.empty() || !archive.empty() || !archive_url.empty() ||
                          workshop_id || !archive_member.empty() || !bundle.empty() || !mod_id.empty() || version ||
                          !expected_hash.empty() || !package_root.empty() || !installer.empty() ||
                          allow_stale_manifest_md5 || allow_missing_manifest_assets))
            throw std::runtime_error("--plan requires --save and cannot be combined with archive options");
        if (!installer.empty() && bundle.empty())
            throw std::runtime_error("--installer requires --bundle");
        if (allow_stale_manifest_md5 && (archive.empty() || expected_hash.empty()))
            throw std::runtime_error("--allow-stale-manifest-md5 requires --archive and --sha256");
        if(allow_missing_manifest_assets&&(archive.empty()||expected_hash.empty()))
            throw std::runtime_error("--allow-missing-manifest-assets requires --archive and --sha256");
        if (!bundle.empty()) {
            if (bundle != "vp-5.4.2" || !mod_id.empty() || version || !expected_hash.empty() ||
                !package_root.empty() || !archive_member.empty())
                throw std::runtime_error("Unknown bundle or incompatible archive options");
            const auto result = content::acquire_vp_542_bundle(cache, decoder, installer);
            std::ostringstream output;
            output << "{\"schema\":1,\"runtime_bits\":64,\"can_continue\":false,"
                      "\"operation\":\"acquire_installer_bundle\",";
            bundle_json(output, result);
            output << "}\n";
            std::cout << output.str();
            return 0;
        }
        if (!archive.empty()) {
            if (mod_id.empty() || !version || !archive_member.empty())
                throw std::runtime_error("Local archive needs --mod and --version");
            const auto result =
                content::install_mod_archive(archive, {mod_id, {}, *version}, cache,
                                             expected_hash, {}, package_root,
                                             allow_stale_manifest_md5,
                                             allow_missing_manifest_assets);
            std::ostringstream output;
            output << "{\"schema\":1,\"runtime_bits\":64,\"can_continue\":false,"
                      "\"local_archive\":true,";
            result_json(output, result, true);
            output << "}\n";
            std::cout << output.str();
            return 0;
        }
        if (!archive_url.empty()) {
            if (mod_id.empty() || !version)
                throw std::runtime_error("Archive URL needs --mod and --version");
            const auto result = content::acquire_mod_from_url(
                {mod_id, {}, *version}, archive_url, cache, expected_hash,
                archive_member, package_root);
            std::ostringstream output;
            output << "{\"schema\":1,\"runtime_bits\":64,\"can_continue\":false,"
                      "\"operation\":\"acquire_url\",\"source_pinned\":"
                   << (!expected_hash.empty() ? "true" : "false") << ',';
            result_json(output, result);
            output << "}\n";
            std::cout << output.str();
            return 0;
        }
        if (workshop_id) {
            if (mod_id.empty() || !version || !package_root.empty() || !archive_member.empty())
                throw std::runtime_error("Workshop item needs --mod and --version without archive paths");
            const auto result = content::acquire_mod_from_workshop(
                {mod_id, {}, *version}, *workshop_id, cache, expected_hash);
            std::ostringstream output;
            output << "{\"schema\":1,\"runtime_bits\":64,\"can_continue\":false,"
                      "\"operation\":\"acquire_workshop\",\"source_pinned\":"
                   << (!expected_hash.empty() ? "true" : "false") << ',';
            result_json(output, result);
            output << "}\n";
            std::cout << output.str();
            return 0;
        }
        if (!mod_id.empty() || version || !expected_hash.empty() || !package_root.empty() ||
            !archive_member.empty())
            throw std::runtime_error("Archive options cannot be combined with --save");
        fs::path selected_path;
        saves::SaveHeader selected_header;
        if (fs::is_regular_file(save) &&
            runtime::ascii_lower(assets::path_utf8(save.extension())) == ".civissave") {
            selected_path = fs::canonical(save);
            selected_header = saves::read_loaded_save(selected_path).header;
        } else {
            const auto selection = runtime::select_save(save);
            if (!selection.selected)
                throw std::runtime_error("No supported save");
            const auto &selected = selection.candidates[*selection.selected];
            selected_path = selected.path;
            selected_header = selected.header;
        }
        const auto before = runtime::identify_file(selected_path);
        std::vector<content::ModSource> catalog;
        if (plan_only) {
            if (!selected_header.mods.empty())
                catalog = content::read_mod_sources(sources);
            std::vector<content::ModTarget> targets;
            targets.reserve(selected_header.mods.size());
            for (const auto &mod : selected_header.mods)
                targets.push_back({runtime::ascii_lower(mod.id), mod.name, mod.wire_value});
            const auto plan = content::plan_mod_acquisition(
                targets, catalog, cache);
            plan_json(std::cout, plan, selected_path, before.sha256);
            return plan.all_verified ? 0 : 3;
        }
        if (!selected_header.mods.empty()) {
            catalog = content::read_mod_sources(sources);
            content::initialize_mod_cache(cache);
        }
        bool complete = true;
        std::uint64_t acquired = 0;
        std::set<fs::path> retained_bundles;
        std::deque<PendingMod> pending;
        std::set<std::pair<std::string, std::uint64_t>> queued;
        auto enqueue = [&](content::ModTarget target, bool dependency) {
            target.id = runtime::ascii_lower(target.id);
            if (queued.emplace(target.id, target.version).second)
                pending.push_back({std::move(target), dependency});
        };
        for (const auto &mod : selected_header.mods)
            enqueue({runtime::ascii_lower(mod.id), mod.name, mod.wire_value}, false);
        std::vector<std::string> dependency_diagnostics;
        std::ostringstream output;
        output << "{\"schema\":1,\"runtime_bits\":64,\"operation\":\"acquire_"
                  "mods\",\"can_continue\":false,"
                  "\"save\":"
               << quote(assets::path_utf8(selected_path)) << ",\"save_sha256\":" << quote(before.sha256)
               << ",\"installed_root\":" << quote(assets::path_utf8(fs::absolute(cache) / "packages"))
               << ",\"mods\":[";
        bool first_mod = true;
        while (!pending.empty()) {
            const auto current = std::move(pending.front());
            pending.pop_front();
            if (!first_mod)
                output << ',';
            first_mod = false;
            output << "{\"id\":" << quote(current.target.id) << ",\"version\":"
                   << current.target.version << ",\"name\":" << quote(current.target.name);
            if (current.dependency)
                output << ",\"origin\":\"manifest_dependency\"";
            output << ',';
            const auto found = std::find_if(catalog.begin(), catalog.end(), [&](const auto &source) {
                return source.target.id == current.target.id &&
                       source.target.version == current.target.version;
            });
            std::optional<content::AcquiredMod> independently_cached;
            std::string cache_lookup_error;
            bool cache_ambiguous = false;
            if (found == catalog.end())
                try {
                    independently_cached = content::find_cached_mod_by_identity(cache, current.target);
                } catch (const content::AmbiguousCachedMod &error) {
                    cache_lookup_error = error.what();
                    cache_ambiguous = true;
                } catch (const std::exception &error) {
                    cache_lookup_error = error.what();
                }
            if (found == catalog.end() && !independently_cached) {
                output << "\"status\":" << quote(cache_lookup_error.empty() ? "source_unresolved"
                    : cache_ambiguous ? "cache_ambiguous" : "cache_invalid");
                if (!cache_lookup_error.empty()) output << ",\"reason\":" << quote(cache_lookup_error);
                complete = false;
            } else
                try {
                    std::cerr << "Acquiring " << current.target.name << " v" << current.target.version
                              << '\n';
                    std::uint64_t last = 0;
                    const auto progress = [&](std::uint64_t bytes) {
                        if (bytes - last >= 8 * 1024 * 1024) {
                            std::cerr << current.target.name << ": " << bytes << " bytes\n";
                            last = bytes;
                        }
                        return true;
                    };
                    if (found != catalog.end() && !found->installer_bundle.empty()) {
                        const auto retained = content::acquire_vp_542_bundle(cache, decoder, {}, progress);
                        bundle_json(output, retained);
                        retained_bundles.insert(retained.root);
                        complete = false;
                    } else {
                        const auto acquired_mod = independently_cached ? *independently_cached
                            : content::acquire_mod(*found, cache, progress);
                        result_json(output, acquired_mod);
                        ++acquired;
                        try {
                            for (const auto &dependency :
                                 content::read_mod_dependencies(acquired_mod.manifest)) {
                                std::size_t already_queued = 0;
                                for (const auto &[id, version] : queued)
                                    if (id == dependency.id && version >= dependency.minimum_version &&
                                        version <= dependency.maximum_version)
                                        ++already_queued;
                                if (already_queued == 1) continue;
                                if (already_queued > 1) {
                                    dependency_diagnostics.push_back(
                                        dependency.id + ": dependency_version_ambiguous");
                                    complete = false;
                                    continue;
                                }
                                std::vector<const content::ModSource *> candidates;
                                for (const auto &source : catalog)
                                    if (source.target.id == dependency.id &&
                                        source.target.version >= dependency.minimum_version &&
                                        source.target.version <= dependency.maximum_version)
                                        candidates.push_back(&source);
                                if (candidates.size() == 1) {
                                    enqueue({candidates.front()->target.id, dependency.name,
                                             candidates.front()->target.version}, true);
                                } else {
                                    dependency_diagnostics.push_back(
                                        dependency.id + " [" + std::to_string(dependency.minimum_version) +
                                        "," + std::to_string(dependency.maximum_version) + "] " +
                                        (candidates.empty() ? "source_unresolved" : "source_ambiguous"));
                                    complete = false;
                                }
                            }
                        } catch (const std::exception &error) {
                            dependency_diagnostics.push_back(current.target.id + ": " + error.what());
                            complete = false;
                        }
                    }
                } catch (const std::exception &error) {
                    output << "\"status\":\"failed\",\"reason\":" << quote(error.what());
                    complete = false;
                }
            output << '}';
        }
        const bool unchanged = runtime::identify_file(selected_path).sha256 == before.sha256;
        output << "],\"dependency_diagnostics\":[";
        for (std::size_t i = 0; i < dependency_diagnostics.size(); ++i) {
            if (i)
                output << ',';
            output << quote(dependency_diagnostics[i]);
        }
        output << "],\"verified_mods\":" << acquired
               << ",\"retained_installer_bundles\":" << retained_bundles.size()
               << ",\"all_saved_mods_acquired\":" << (complete ? "true" : "false")
               << ",\"original_save_unchanged\":" << (unchanged ? "true" : "false") << "}\n";
        std::cout << output.str();
        return complete && unchanged ? 0 : 2;
    } catch (const std::exception &error) {
        std::cout << "{\"schema\":1,\"runtime_bits\":64,\"can_continue\":false,\"error\":"
                  << quote(error.what()) << "}\n";
        return 2;
    }
}
