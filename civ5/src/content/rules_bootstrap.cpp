#include "civ5/content/rules_bootstrap.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <fstream>
#include <stdexcept>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
fs::path loose_path(const assets::AssetHandle &handle) {
    const auto &provenance = handle.candidate().provenance;
    require(provenance.kind == assets::SourceKind::loose, "Native base source is not loose");
    return fs::path(std::u8string(provenance.source_id.begin(), provenance.source_id.end())) /
           fs::path(std::u8string(provenance.source_relative_path.begin(),
                                  provenance.source_relative_path.end()));
}
struct SourceText {
    runtime::FileIdentity identity;
    fs::path path;
    std::string text;
};
SourceText read_loose_text(const assets::AssetHandle &handle) {
    const auto path = loose_path(handle);
    const auto before = runtime::identify_file(path);
    const auto bytes = handle.read_bytes(64ULL * 1024 * 1024);
    require(bytes.size() == before.bytes, "Base source size changed while reading AssetStream");
    require(runtime::identify_file(path).sha256 == before.sha256,
            "Base source changed while reading AssetStream");
    return {before, path, std::string(bytes.begin(), bytes.end())};
}
} // namespace
NativeGameplayReport apply_native_base_gameplay(RulesDatabase &database, const fs::path &game_root,
                                                const assets::ResourceMountTable &resources) {
    NativeGameplayReport result;
    result.recipe = read_native_gameplay_recipe(game_root / "CivilizationV.exe");
    assets::LooseSource installation(game_root);
    const auto schema_lookup = installation.find(assets::AssetKey(result.recipe.engine_schema));
    const auto *schema = std::get_if<assets::AssetHandle>(&schema_lookup);
    require(schema, "Engine database schema is missing or ambiguous");
    const auto schema_text = read_loose_text(*schema);
    database.apply_sql(schema_text.text);
    result.schema_identity = schema_text.identity;
    result.schema_source = assets::path_utf8(schema_text.path);
    result.schema_applied = true;
    for (const auto &source : plan_base_gameplay(result.recipe.requests, resources)) {
        NativeGameplaySourceResult item;
        item.request = source.request;
        const auto *handle = std::get_if<assets::AssetHandle>(&source.resource.asset);
        if (!handle) {
            if (!std::holds_alternative<std::monostate>(source.resource.asset))
                throw std::runtime_error("Ambiguous native base gameplay source: " + source.request);
            ++result.missing;
            result.sources.push_back(std::move(item));
            continue;
        }
        ++result.selected;
        item.selected = true;
        item.mount_id = source.resource.mount_id;
        const auto text = read_loose_text(*handle);
        item.source = assets::path_utf8(text.path);
        item.identity = text.identity;
        try {
            database.apply_mod_xml(read_game_data_xml_text(text.text));
            item.applied = true;
            ++result.applied;
        } catch (const std::exception &error) {
            item.error = error.what();
            ++result.failed;
        }
        result.sources.push_back(std::move(item));
    }
    return result;
}
DlcGameplayReport apply_saved_dlc_gameplay(RulesDatabase &database,
                                           std::span<const std::size_t> package_indices,
                                           const Catalog &catalog,
                                           const assets::ResourceMountTable &resources,
                                           std::uint64_t source_budget) {
    DlcGameplayReport result;
    for (const auto package_index : package_indices) {
        const auto &package = catalog.packages.at(package_index);
        require(package.kind == PackageKind::dlc, "Saved DLC gameplay needs a DLC manifest");
        DlcGameplayPackageReport package_result;
        package_result.package = package_index;
        package_result.id = package.id;
        PackageResourceQuery query;
        query.selector = "GameData";
        query.enabled_categories = {true, true, true};
        const auto declarations = package_resources(package, query);
        const auto tag = dlc_game_data_tag(package.id);
        for (const auto &declaration : declarations) {
            ++package_result.requested;
            const auto mounted = resources.resolve(assets::AssetKey(declaration.value));
            const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
            if (!handle) {
                if (!std::holds_alternative<std::monostate>(mounted.asset))
                    throw std::runtime_error("Ambiguous saved DLC GameData source: " + declaration.value);
                ++package_result.missing;
                package_result.missing_requests.push_back(declaration.value);
                continue;
            }
            if (handle->candidate().payload_size > source_budget)
                throw std::runtime_error("DLC GameData source exceeds reader budget");
            ++package_result.resolved;
            const auto bytes = handle->read_bytes(source_budget);
            try {
                database.apply_mod_xml(read_game_data_xml_text(std::string(bytes.begin(), bytes.end())), tag);
                ++package_result.applied;
            } catch (const std::exception &) {
                ++package_result.failed;
            }
        }
        result.packages.push_back(std::move(package_result));
    }
    return result;
}
} // namespace civ5::content
