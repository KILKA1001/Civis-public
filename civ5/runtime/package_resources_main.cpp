#include "civ5/assets/asset_source.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace {
std::string json(const std::string &text) {
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (unsigned char c : text) {
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
civ5::content::PackageCategory category(const std::string &value) {
    using enum civ5::content::PackageCategory;
    const auto name = civ5::runtime::ascii_lower(value);
    if (name == "common")
        return common;
    if (name == "gameplay")
        return gameplay;
    if (name == "uiskin")
        return ui_skin;
    throw std::runtime_error("Expected Common, Gameplay or UISkin category");
}
struct PackagePlan {
    civ5::content::PackageManifest manifest;
    civ5::runtime::FileIdentity identity;
    std::vector<civ5::content::PackageResourceDeclaration> resources;
    std::vector<civ5::assets::MountedResource> sources;
};
} // namespace
int wmain(int argc, wchar_t **argv) {
    using namespace civ5;
    try {
        fs::path save, game_root, mount_plan_path;
        std::vector<fs::path> paths;
        content::PackageResourceQuery query;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (option == L"--help") {
                std::cout
                    << "civis_package_resources (--manifest FILE ... | --save FILE_OR_DIRECTORY "
                       "--game-root DIRECTORY) --resource XML_SELECTOR [--enable Common|Gameplay|UISkin ...] "
                       "[--category all|Common|Gameplay|UISkin] [--category-index N] "
                       "[--mount-plan EXPLICIT_LOOSE_PLAN.xml]\n"
                       "Read-only descriptor plan with optional explicit loose mounts. "
                       "Does not discover activation or load a game.\n";
                return 0;
            }
            if (++i == argc)
                throw std::runtime_error("Missing resource option value");
            const auto value = assets::path_utf8(argv[i]);
            if (option == L"--manifest")
                paths.emplace_back(argv[i]);
            else if (option == L"--save")
                save = argv[i];
            else if (option == L"--game-root")
                game_root = argv[i];
            else if (option == L"--resource")
                query.selector = value;
            else if (option == L"--mount-plan")
                mount_plan_path = argv[i];
            else if (option == L"--enable")
                query.enabled_categories[static_cast<std::size_t>(category(value))] = true;
            else if (option == L"--category") {
                query.category =
                    runtime::ascii_lower(value) == "all" ? std::nullopt : std::optional(category(value));
            } else if (option == L"--category-index")
                query.category_occurrence = content::decimal(value);
            else
                throw std::runtime_error("Unknown resource option");
        }
        if (query.selector.empty())
            throw std::runtime_error("Supply --resource XML_SELECTOR");
        std::string save_hash;
        if (!save.empty()) {
            if (!paths.empty() || game_root.empty())
                throw std::runtime_error("Save selection needs only --save and --game-root");
            const auto selected = runtime::select_save(save);
            if (!selected.selected)
                throw std::runtime_error("No supported save header");
            save = selected.candidates[*selected.selected].path;
            save_hash = runtime::identify_file(save).sha256;
            const auto header = saves::read_header(save);
            const auto catalog = content::discover_packages({game_root / "Assets/DLC"});
            if (!catalog.diagnostics.empty())
                throw std::runtime_error("DLC catalog: " + catalog.diagnostics.front());
            for (const auto &requirement : runtime::locate_saved_content(header, catalog)) {
                if (requirement.kind != content::PackageKind::dlc)
                    continue;
                if (requirement.status != runtime::RequirementStatus::located)
                    throw std::runtime_error("Saved DLC is missing or ambiguous: " + requirement.id);
                paths.push_back(catalog.packages[*requirement.package].path);
            }
            if (runtime::identify_file(save).sha256 != save_hash)
                throw std::runtime_error("Save changed during selection");
        } else if (paths.empty() || !game_root.empty()) {
            throw std::runtime_error("Supply --manifest FILE or --save with --game-root");
        }
        std::vector<PackagePlan> plans;
        std::optional<content::LooseResourcePlan> mounted;
        std::string mount_plan_hash;
        if (!mount_plan_path.empty()) {
            if (query.selector != "GameData")
                throw std::runtime_error("Explicit loose plan currently resolves GameData only");
            mount_plan_hash = runtime::identify_file(mount_plan_path).sha256;
            mounted.emplace(content::read_loose_resource_plan(mount_plan_path));
            if (runtime::identify_file(mount_plan_path).sha256 != mount_plan_hash)
                throw std::runtime_error("Resource mount plan changed during indexing");
        }
        std::uint64_t total = 0;
        for (const auto &path : paths) {
            PackagePlan plan;
            plan.identity = runtime::identify_file(path);
            plan.manifest = content::read_manifest(path);
            plan.resources = content::package_resources(plan.manifest, query);
            if (mounted)
                for (const auto &resource : plan.resources)
                    plan.sources.push_back(resource.value.empty() ? assets::MountedResource{}
                                                                  : mounted->resources.resolve(
                                                                        assets::AssetKey(resource.value)));
            if (runtime::identify_file(path).sha256 != plan.identity.sha256)
                throw std::runtime_error("Manifest changed during planning");
            total += plan.resources.size();
            plans.push_back(std::move(plan));
        }
        std::cout << "{\"runtime_bits\":" << sizeof(void *) * 8
                  << ",\"selection\":" << json(save.empty() ? "explicit_manifest_order" : "save_dlc_order")
                  << ",\"save\":" << json(assets::path_utf8(save)) << ",\"save_sha256\":" << json(save_hash)
                  << ",\"resource\":" << json(query.selector) << ",\"category\":"
                  << json(query.category ? content::package_category_name(*query.category) : "all")
                  << ",\"category_index\":" << query.category_occurrence << ",\"enabled_categories\":[";
        bool separator = false;
        for (const auto item : {content::PackageCategory::common, content::PackageCategory::gameplay,
                                content::PackageCategory::ui_skin}) {
            if (!query.enabled_categories[static_cast<std::size_t>(item)])
                continue;
            if (separator)
                std::cout << ',';
            separator = true;
            std::cout << json(content::package_category_name(item));
        }
        std::cout << "],\"activation_performed\":false,\"source_resolution_performed\":"
                  << (mounted ? "true" : "false")
                  << ",\"mount_plan\":" << json(assets::path_utf8(mount_plan_path))
                  << ",\"mount_plan_sha256\":" << json(mount_plan_hash)
                  << ",\"indexed_loose_files\":" << (mounted ? mounted->indexed_files : 0)
                  << ",\"native_full_profile_verified\":false,\"can_continue\":false,\"packages\":[";
        for (std::size_t i = 0; i < plans.size(); ++i) {
            if (i)
                std::cout << ',';
            const auto &plan = plans[i];
            std::cout << "{\"id\":" << json(plan.manifest.id)
                      << ",\"manifest\":" << json(assets::path_utf8(plan.manifest.path))
                      << ",\"manifest_sha256\":" << json(plan.identity.sha256) << ",\"resources\":[";
            for (std::size_t j = 0; j < plan.resources.size(); ++j) {
                if (j)
                    std::cout << ',';
                const auto &resource = plan.resources[j];
                std::cout << "{\"value\":" << json(resource.value) << ",\"scope\":"
                          << json(resource.category ? content::package_category_name(*resource.category)
                                                    : "root")
                          << ",\"category_occurrence\":" << resource.category_occurrence
                          << ",\"file_list\":" << (resource.file_list ? "true" : "false");
                if (mounted) {
                    const auto &source = plan.sources[j];
                    const auto *handle = std::get_if<assets::AssetHandle>(&source.asset);
                    std::string physical;
                    if (handle) {
                        const auto &provenance = handle->candidate().provenance;
                        const auto &id = provenance.source_id;
                        const auto &relative = provenance.source_relative_path;
                        physical =
                            assets::path_utf8(fs::path(std::u8string(id.begin(), id.end())) /
                                              fs::path(std::u8string(relative.begin(), relative.end())));
                    }
                    std::cout << ",\"resolved_in_loose_plan\":" << (handle ? "true" : "false")
                              << ",\"source\":" << json(physical) << ",\"mount_id\":";
                    if (source.mount_id)
                        std::cout << *source.mount_id;
                    else
                        std::cout << "null";
                }
                std::cout << '}';
            }
            std::cout << "]}";
        }
        std::cout << "],\"resource_count\":" << total << "}\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "resource_plan=FAIL " << error.what() << '\n';
        return 2;
    }
}
