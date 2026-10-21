#include "civ5/content/saved_rules.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/graphics/bnw_base_unit_art_compilation.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::string read_text(const fs::path &path, std::uint64_t budget) {
    const auto before = runtime::identify_file(path);
    if (before.bytes > budget)
        throw std::runtime_error("Localization source exceeds reader budget");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open localization source");
    std::string text(static_cast<std::size_t>(before.bytes), '\0');
    input.read(text.data(), static_cast<std::streamsize>(text.size()));
    if (static_cast<std::uint64_t>(input.gcount()) != before.bytes ||
        input.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("Truncated localization source");
    if (runtime::identify_file(path).sha256 != before.sha256)
        throw std::runtime_error("Localization source changed while reading");
    return text;
}
std::vector<fs::path> xml_files(const fs::path &root) {
    std::vector<fs::path> result;
    if (!fs::exists(root))
        return result;
    for (const auto &entry : fs::recursive_directory_iterator(
             root, fs::directory_options::skip_permission_denied)) {
        if (entry.is_regular_file() && runtime::ascii_lower(entry.path().extension().string()) == ".xml")
            result.push_back(entry.path());
    }
    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        return runtime::ascii_lower(left.generic_string()) < runtime::ascii_lower(right.generic_string());
    });
    return result;
}
std::string relative_asset_path(const assets::AssetHandle &handle) {
    return runtime::ascii_lower(assets::normalize_asset_name(handle.candidate().provenance.source_relative_path));
}
bool beneath_asset_path(const std::string &path, const std::string &root) {
    return path == root || path.starts_with(root + '/');
}
bool handle_belongs_to_package(const assets::AssetHandle &handle, const PackageManifest &package) {
    const auto &provenance = handle.candidate().provenance;
    if (provenance.kind != assets::SourceKind::loose)
        return false;
    const auto source_root = fs::canonical(fs::path(std::u8string(
        provenance.source_id.begin(), provenance.source_id.end())));
    const auto physical = fs::canonical(source_root / fs::path(std::u8string(
        provenance.source_relative_path.begin(), provenance.source_relative_path.end())));
    const auto package_root = fs::canonical(package.path.parent_path());
    const auto relative = physical.lexically_relative(package_root);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != fs::path("..") &&
           relative != fs::path(".");
}
std::string read_asset_text(const assets::AssetHandle &handle, std::uint64_t budget) {
    if (handle.candidate().payload_size > budget)
        throw std::runtime_error("Localization asset exceeds reader budget");
    const auto bytes = handle.read_bytes(budget);
    return {bytes.begin(), bytes.end()};
}
void apply_localization_assets(RulesDatabase &database,
                               const std::vector<assets::AssetHandle> &handles,
                               std::uint64_t budget, std::uint64_t &resolved,
                               std::uint64_t &applied, std::uint64_t &failed) {
    for (const auto &handle : handles) {
        ++resolved;
        try {
            database.apply_xml(read_game_data_xml_text(read_asset_text(handle, budget)));
            ++applied;
        } catch (const std::exception &) {
            ++failed;
        }
    }
}
std::vector<assets::AssetHandle> base_localization_assets(
    const fs::path &game_root, const std::vector<fs::path> &base_files,
    const assets::ResourceMountTable &resources) {
    std::vector<assets::AssetHandle> result;
    std::unordered_set<std::string> seen;
    for (const auto &file : base_files) {
        const auto expected = runtime::ascii_lower(assets::normalize_asset_name(
            assets::path_utf8(file.lexically_relative(game_root))));
        for (auto &mounted : resources.resolve_all(assets::AssetKey(assets::path_utf8(file.filename())))) {
            const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
            if (handle && relative_asset_path(*handle) == expected)
                if (seen.insert(relative_asset_path(*handle)).second)
                    result.push_back(std::move(*handle));
        }
    }
    return result;
}
std::vector<assets::AssetHandle> text_data_assets(
    const PackageManifest &package, const fs::path &game_root,
    const assets::ResourceMountTable &resources) {
    PackageResourceQuery query;
    query.selector = "TextData";
    query.enabled_categories = {true, true, true};
    const auto declarations = package_resources(package, query);
    const auto package_root = runtime::ascii_lower(assets::normalize_asset_name(
        assets::path_utf8(package.path.parent_path().lexically_relative(game_root))));
    std::unordered_set<std::string> seen;
    std::vector<assets::AssetHandle> result;
    for (const auto &declaration : declarations) {
        for (auto &mounted : resources.resolve_all(assets::AssetKey(declaration.value))) {
            auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
            if (!handle ||
                (!handle_belongs_to_package(*handle, package) &&
                 !beneath_asset_path(relative_asset_path(*handle), package_root)))
                continue;
            const auto extension = runtime::ascii_lower(fs::path(std::u8string(
                                                        handle->candidate().original_name.begin(),
                                                        handle->candidate().original_name.end()))
                                                           .extension()
                                                           .string());
            if (extension != ".xml")
                continue;
            if (seen.insert(relative_asset_path(*handle)).second)
                result.push_back(std::move(*handle));
        }
    }
    return result;
}
LocalizationActivationReport apply_saved_localization(
    RulesDatabase &database, const fs::path &game_root, std::span<const std::size_t> package_indices,
    const Catalog &catalog, const assets::ResourceMountTable &resources, std::uint64_t budget) {
    LocalizationActivationReport result;
    RulesDatabase base;
    const auto schema = game_root / "Assets/SQL/Civ5LocalizationDatabaseSchema.sql";
    base.apply_localization_schema(read_text(schema, budget));
    result.base_schema_applied = true;
    const auto base_files = xml_files(game_root / "Assets/Gameplay/XML/NewText");
    result.base_requested = base_files.size();
    const auto base_assets = base_localization_assets(game_root, base_files, resources);
    apply_localization_assets(base, base_assets, budget, result.base_resolved, result.base_applied,
                              result.base_failed);
    base.convert_localization_tables();
    for (const auto package_index : package_indices) {
        const auto &package = catalog.packages.at(package_index);
        require(package.kind == PackageKind::dlc, "Saved localization needs a DLC manifest");
        LocalizationPackageActivationReport package_result{package_index, package.id};
        RulesDatabase localized_package;
        localized_package.apply_localization_schema(
            read_text(game_root / "Assets/SQL/Civ5DlcLocalizationDatabaseSchema.sql", budget));
        const auto files = text_data_assets(package, game_root, resources);
        PackageResourceQuery text_query;
        text_query.selector = "TextData";
        text_query.enabled_categories = {true, true, true};
        package_result.requested = package_resources(package, text_query).size();
        apply_localization_assets(localized_package, files, budget, package_result.resolved,
                                  package_result.applied, package_result.failed);
        base.merge_localization(localized_package);
        result.packages.push_back(std::move(package_result));
    }
    base.track_localization_mod_changes();
    database.attach_localization(base);
    result.attached = true;
    return result;
}
} // namespace
std::vector<SelectedLocalizationSource> selected_localization_sources(
    const fs::path &game_root,std::span<const std::size_t> package_indices,
    const Catalog &catalog,const assets::ResourceMountTable &resources) {
    std::vector<SelectedLocalizationSource> result;
    for(auto &asset:base_localization_assets(game_root,xml_files(game_root/"Assets/Gameplay/XML/NewText"),resources))
        result.push_back({std::nullopt,std::move(asset)});
    for(const auto index:package_indices) {
        const auto &package=catalog.packages.at(index);
        require(package.kind==PackageKind::dlc,"Selected localization source needs a DLC manifest");
        for(auto &asset:text_data_assets(package,game_root,resources))
            result.push_back({index,std::move(asset)});
    }
    return result;
}

namespace {
SavedRulesActivationReport activate_saved_rules_impl(
    RulesDatabase &database, const std::filesystem::path &game_root,
    const runtime::SavedContentProfile &profile, const Catalog &catalog,
    const assets::ResourceMountTable &resources, std::uint64_t source_budget,
    const std::function<void(RulesDatabase &)> &before_mod_actions,
    std::span<const std::size_t> use_installed_bytes,
    std::span<const runtime::VerifiedManifestHash> verified_hashes,
    const graphics::BnwBaseUnitArtCompilation* base_unit_art,
    const assets::ResourceMountTable* early_art_resources,
    const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>* completed_art_inputs) {
    SavedRulesActivationReport result;
    result.profile_kind = profile.kind;
    result.profile_can_activate = profile.can_activate;

    result.localization = apply_saved_localization(database, game_root, profile.enabled_dlc, catalog,
                                                   resources, source_budget);
    // Keep the order explicit. Base rules establish the schema and vanilla
    // rows; selected DLC then extends those rows with its PackageID; saved
    // mod actions finally apply their authored UpdateDatabase order.
    result.base = apply_native_base_gameplay(database, game_root, resources);
    if (base_unit_art) {
        require(result.base.schema_applied && result.base.failed == 0,
                "BNW base unit art requires completed mutable base gameplay");
        if (completed_art_inputs) {
            require(early_art_resources == nullptr, "BNW art input domains overlap");
            base_unit_art->apply_to_unsealed_database_from_completed_native_inputs(
                database, *completed_art_inputs, profile);
        } else {
            require(early_art_resources != nullptr, "BNW base unit art early table is missing");
            base_unit_art->apply_to_unsealed_database(database, *early_art_resources, profile);
        }
    }
    result.dlc = apply_saved_dlc_gameplay(database, profile.enabled_dlc, catalog, resources,
                                          source_budget);
    if (before_mod_actions) before_mod_actions(database);
    result.mod_plan = plan_saved_database_actions(profile, catalog);
    result.mods = apply_saved_database_actions(database, result.mod_plan, catalog, source_budget,
                                               use_installed_bytes, verified_hashes);

    bool dlc_succeeded = true;
    for (const auto &package : result.dlc.packages)
        dlc_succeeded &= package.missing == 0 && package.failed == 0;
    bool localization_succeeded = result.localization.base_schema_applied &&
                                  result.localization.base_failed == 0;
    for (const auto &package : result.localization.packages)
        localization_succeeded &= package.failed == 0;
    result.rules_database_ready = profile.can_activate && result.base.schema_applied &&
                                  result.base.failed == 0 && localization_succeeded && dlc_succeeded &&
                                  result.mods.all_succeeded;
    return result;
}
} // namespace
SavedRulesActivationReport activate_saved_rules(
    RulesDatabase& database, const std::filesystem::path& game_root,
    const runtime::SavedContentProfile& profile, const Catalog& catalog,
    const assets::ResourceMountTable& resources, std::uint64_t source_budget,
    const std::function<void(RulesDatabase&)>& before_mod_actions,
    std::span<const std::size_t> use_installed_bytes,
    std::span<const runtime::VerifiedManifestHash> verified_hashes) {
    return activate_saved_rules_impl(database, game_root, profile, catalog, resources,
        source_budget, before_mod_actions, use_installed_bytes, verified_hashes, nullptr, nullptr, nullptr);
}
SavedRulesActivationReport activate_saved_rules_with_base_unit_art(
    RulesDatabase& database, const std::filesystem::path& game_root,
    const runtime::SavedContentProfile& profile, const Catalog& catalog,
    const assets::ResourceMountTable& resources,
    const graphics::BnwBaseUnitArtCompilation& art,
    const assets::ResourceMountTable& early_art_resources, std::uint64_t source_budget) {
    return activate_saved_rules_impl(database, game_root, profile, catalog, resources,
        source_budget, {}, {}, {}, &art, &early_art_resources, nullptr);
}
SavedRulesActivationReport activate_saved_rules_with_completed_base_unit_art(
    RulesDatabase& database,const std::filesystem::path& game_root,
    const runtime::SavedContentProfile& profile,const Catalog& catalog,
    const assets::ResourceMountTable& rules_resources,
    const graphics::BnwBaseUnitArtCompilation& art,
    const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>& supplied,
    std::uint64_t source_budget){
    // Own the actual compilation/inputs/profile before any source/provider.
    const auto inputs=supplied;const runtime::SavedContentProfile selected(profile);
    const graphics::BnwBaseUnitArtCompilation compilation(art);
    // Genuine private compilation source control and winner rechecks precede
    // even mutable base gameplay; an old table/default art value cannot pass.
    compilation.require_same_completed_native_inputs(inputs,selected);
    return activate_saved_rules_impl(database,game_root,selected,catalog,rules_resources,
        source_budget,{}, {}, {}, &compilation,nullptr,&inputs);
}
} // namespace civ5::content
