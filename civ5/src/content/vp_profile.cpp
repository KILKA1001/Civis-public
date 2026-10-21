#include "civ5/content/vp_profile.hpp"
#include "civ5/content/vp_content_profile.hpp"
#include "civ5/content/vp_lua_assets_profile.hpp"
#include "civ5/content/game_data_xml.hpp"
#include "civ5/content/inno_archive.hpp"
#include "civ5/content/mod_archive.hpp"
#include "civ5/runtime/binary_reader.hpp"
#define NOMINMAX
#include <algorithm>
#include <array>
#include <fstream>
#include <objbase.h>
#include <set>
#include <stdexcept>

namespace civ5::content {
namespace fs = std::filesystem;
namespace {
constexpr auto cp_folder = "(1) Community Patch";
constexpr auto vp_folder = "(2) Vox Populi";
constexpr auto cp_id = "d1b6328c-ff44-4b0d-aad7-c657f83610cd";
constexpr auto vp_id = "8411a7a8-dad3-4622-a18e-fcc18324c799";
constexpr auto standard_core_sha = "16735c85f2494fc3de3a6cc3bd7e63d927ab91ac3947b6000df39611aa85c919";
constexpr auto large_core_sha = "30872ae3ecce6607613aaca76084d19d96785ba2156bd21fc304612143e2ef25";
constexpr std::array<std::pair<VpComponent, const char *>, 6> components{
    {{VpComponent::core, "Core"},
     {VpComponent::core_43, "Civ43CPOnly"},
     {VpComponent::full, "FullNoEUI"},
     {VpComponent::full_eui, "FullEUI"},
     {VpComponent::full_43, "Civ43NoEUI"},
     {VpComponent::full_43_eui, "Civ43EUI"}}};
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
std::string safe_path(const std::string &name) {
    require(runtime::valid_utf8(name), "Invalid profile path UTF-8");
    return assets::path_utf8(
        archive_relative_path(fs::path(std::u8string(name.begin(), name.end())).native()));
}
bool below(const std::string &path, const std::string &directory) {
    return path.starts_with(directory + '/');
}
std::string normalized(const std::string &path) { return assets::normalize_asset_name(safe_path(path)); }
bool installed_mod_folder(const std::string &name, VpComponent component) {
    if (name == cp_folder || name == "(5) Modpack Maker for VP")
        return true;
    if (name == vp_folder || name == "(4a) Squads for VP")
        return vp_has_full_rules(component);
    if (name == "(3a) VP - EUI Compatibility Files")
        return vp_has_eui(component);
    if (name == "(3b) 43 Civs Community Patch")
        return vp_has_43_layout(component);
    throw std::runtime_error("Unknown mod folder in pinned installer");
}
} // namespace
VpComponent vp_component(const std::string &name) {
    for (const auto &[value, label] : components)
        if (name == label)
            return value;
    throw std::runtime_error("Unknown VP gameplay component (Uninstall is not a profile)");
}
std::string vp_component_name(VpComponent component) {
    for (const auto &[value, label] : components)
        if (component == value)
            return label;
    throw std::runtime_error("Invalid VP component value");
}
bool vp_has_eui(VpComponent component) {
    vp_component_name(component);
    return component == VpComponent::full_eui || component == VpComponent::full_43_eui;
}
bool vp_has_full_rules(VpComponent component) {
    vp_component_name(component);
    return component != VpComponent::core && component != VpComponent::core_43;
}
bool vp_has_43_layout(VpComponent component) {
    vp_component_name(component);
    return component == VpComponent::core_43 || component == VpComponent::full_43 ||
           component == VpComponent::full_43_eui;
}
std::optional<std::string> vp_542_destination(const std::string &decoded_relative, VpComponent component) {
    vp_component_name(component);
    const auto path = safe_path(decoded_relative);
    const std::string mods_prefix = "app/MODS/", game_prefix = "code$GetCIVDir/";
    if (path.starts_with(mods_prefix)) {
        const auto relative = path.substr(mods_prefix.size());
        const auto slash = relative.find('/');
        require(slash != std::string::npos, "Installer mod path has no file");
        const auto folder = relative.substr(0, slash), file = relative.substr(slash + 1);
        if (!installed_mod_folder(folder, component))
            return {};
        if ((folder == cp_folder || folder == vp_folder) && below(file, "LUA") && vp_has_eui(component))
            return {};
        if (folder == cp_folder && file.starts_with("CvGameCore_Expansion2.dll")) {
            require(file == "CvGameCore_Expansion2.dll$0" || file == "CvGameCore_Expansion2.dll$1",
                    "Unknown gamecore collision variant");
            if (file.back() != (vp_has_43_layout(component) ? '1' : '0'))
                return {};
            return "MODS/" + folder + "/CvGameCore_Expansion2.dll";
        }
        require(file.find('$') == std::string::npos && file.find("#Uninstall") == std::string::npos,
                "Unmapped installer mod collision");
        return "MODS/" + relative;
    }
    if (path.starts_with(game_prefix)) {
        auto relative = path.substr(game_prefix.size());
        if (below(relative, "Assets/DLC/UI_bc1")) {
            if (!vp_has_eui(component))
                return {};
        } else if (below(relative, "Assets/DLC/VPUI")) {
            if (!vp_has_full_rules(component))
                return {};
        } else if (relative == "Assets/DLC/Expansion2/Expansion2.Civ5Pkg#Uninstall")
            return {};
        else if (relative == "Assets/DLC/Expansion2/Expansion2.Civ5Pkg$0")
            relative.resize(relative.size() - 2);
        else if (relative.starts_with("Assets/DLC/Expansion2/Sounds/XML/"
                                      "MinorCivSounds_VoxPopuli.xml$")) {
            require(relative.ends_with("$0") || relative.ends_with("$1"), "Unknown audio collision variant");
            if (relative.back() != (vp_has_full_rules(component) ? '1' : '0'))
                return {};
            relative.resize(relative.size() - 2);
        } else
            throw std::runtime_error("Unknown installer game overlay");
        return "Game/" + relative;
    }
    if (path == "app/Text/VPUI_tips_en_us.xml") {
        if (vp_has_full_rules(component))
            return "User/Text/VPUI_tips_en_us.xml";
        return {};
    }
    throw std::runtime_error("Unknown pinned installer path");
}
assets::AssetStream ProfileFile::open() const {
    const auto now = runtime::identify_file(source);
    require(now.sha256 == identity.sha256 && now.bytes == identity.bytes,
            "Profile source changed before use");
    return assets::AssetStream(source, 0, identity.bytes);
}
const ProfileFile &VpProfile::file(const std::string &logical_path) const {
    const auto found = files.find(normalized(logical_path));
    require(found != files.end(), "File is absent from the selected profile: " + logical_path);
    return found->second;
}
const ProfileMod &VpProfile::mod(const std::string &id) const {
    const auto lower = runtime::ascii_lower(id);
    for (const auto &item : installed_mods)
        if (item.manifest.id == lower)
            return item;
    throw std::runtime_error("Mod is absent from the selected profile: " + id);
}
const ProfileFile &VpProfile::mod_file(const ProfileMod &package, const std::string &relative) const {
    const auto &owned = mod(package.manifest.id);
    require(&owned == &package, "Mod belongs to a different profile");
    return file(package.logical_root + '/' + safe_path(relative));
}
std::vector<ProfileAction> VpProfile::action_plan(const ProfileMod &package, const std::string &phase,
                                                  const std::string &action) const {
    require(&mod(package.manifest.id) == &package, "Mod belongs to a different profile");
    std::vector<ProfileAction> result;
    const auto *actions = package.manifest.document.child("Actions");
    if (!actions)
        return result;
    const auto *selected_phase = actions->child(phase);
    if (!selected_phase)
        return result;
    for (const auto &node : selected_phase->children) {
        if (node.name != action)
            continue;
        require(node.children.empty() && !node.text.empty(), "Malformed mod file action");
        const auto requested = package.logical_root + '/' + safe_path(node.text);
        const auto found = files.find(normalized(requested));
        result.push_back({requested, found == files.end() ? nullptr : &found->second});
    }
    return result;
}
std::vector<const ProfileFile *> VpProfile::action_files(const ProfileMod &package, const std::string &phase,
                                                         const std::string &action) const {
    std::vector<const ProfileFile *> result;
    for (const auto &item : action_plan(package, phase, action)) {
        require(item.resolved, "Unresolved profile action: " + item.requested_path);
        result.push_back(item.resolved);
    }
    return result;
}
VpProfile build_vp_542_profile(const fs::path &bundle_root, VpComponent component) {
    const auto bundle = verify_vp_542_bundle(bundle_root);
    VpProfile result{component, bundle.root, {}, {}, 0};
    const auto payload = bundle.root / "payload";
    std::vector<std::pair<std::string, fs::path>> manifests;
    for (const auto &entry : fs::recursive_directory_iterator(payload)) {
        if (!entry.is_regular_file())
            continue;
        const auto relative = assets::path_utf8(entry.path().lexically_relative(payload));
        const auto logical = vp_542_destination(relative, component);
        if (!logical)
            continue;
        const auto identity = runtime::identify_file(entry.path());
        result.total_bytes += identity.bytes;
        require(
            result.files.emplace(normalized(*logical), ProfileFile{*logical, entry.path(), identity}).second,
            "Installer profile has a destination collision");
        if (entry.path().extension() == ".modinfo")
            manifests.emplace_back(*logical, entry.path());
    }
    std::sort(manifests.begin(),
              manifests.end()); // Inventory order only, never activation order.
    std::set<std::string> mod_ids;
    for (const auto &[logical, source] : manifests) {
        auto manifest = read_manifest(source);
        require(manifest.kind == PackageKind::mod && mod_ids.insert(manifest.id).second,
                "Invalid or duplicate installer mod manifest");
        result.installed_mods.push_back(
            {std::move(manifest), logical.substr(0, logical.find_last_of('/')), {}, false});
    }
    require(result.mod(cp_id).manifest.version == 151, "Unexpected CP manifest version");
    if (vp_has_full_rules(component))
        require(result.mod(vp_id).manifest.version == 17, "Unexpected VP version");
    for (auto &package : result.installed_mods) {
        for (const auto &declared : package.manifest.files) {
            const auto relative = safe_path(declared.path);
            const auto key = normalized(package.logical_root + '/' + relative);
            const auto found = result.files.find(key);
            if (found == result.files.end()) {
                if (vp_has_43_layout(component) &&
                    package.manifest.id == "6521c02b-1c78-4826-a5b0-faed86235e98" &&
                    relative == "CvGameCore_Expansion2.dll") {
                    const auto &core = result.file("MODS/(1) Community Patch/CvGameCore_Expansion2.dll");
                    require(core.identity.sha256 == large_core_sha &&
                                runtime::file_md5(core.source) == runtime::ascii_lower(declared.md5),
                            "Relocated 43-civ gamecore identity mismatch");
                    package.installer_relocated_gamecore = true;
                    continue;
                }
                require(vp_has_eui(component) &&
                            (package.manifest.id == cp_id || package.manifest.id == vp_id) &&
                            below(relative, "LUA"),
                        "Missing declared profile file: " + relative);
                package.omitted_eui_lua.push_back(relative);
                continue;
            }
            const auto &bound = found->second;
            if (package.manifest.id == cp_id && relative == "CvGameCore_Expansion2.dll") {
                require(runtime::ascii_lower(declared.md5) == "ff8a580e65e1432ab43dd6f0187ffe02" &&
                            bound.identity.sha256 ==
                                (vp_has_43_layout(component) ? large_core_sha : standard_core_sha),
                        "Unexpected CP gamecore identity");
                package.installer_replaced_gamecore = vp_has_43_layout(component);
            } else if (!declared.md5.empty()) {
                require(runtime::file_md5(bound.source) == runtime::ascii_lower(declared.md5),
                        "Profile manifest MD5 mismatch: " + relative);
            }
        }
        // Actions must resolve through the SAME profile as rendering/audio/Lua.
        result.action_plan(package, "OnModActivated", "UpdateDatabase");
        result.action_files(package, "OnGetDLLPath", "SetDllPath");
    }
    return result;
}
std::filesystem::path publish_vp_542_profile_view(const VpProfile &profile,
                                                   const fs::path &view_root) {
    const auto destination = fs::absolute(view_root).lexically_normal();
    require(!destination.filename().empty(), "Profile view needs a directory name");
    require_plain_directory(destination.parent_path());
    const auto marker_name = ".civis-vp-profile.xml";
    const auto marker = std::string("<CivisVpProfileView schema=\"1\" component=\"") +
                        vp_component_name(profile.component) + "\" installer_sha256=\"" +
                        vp_542_installer_sha256 + "\"/>";
    const auto verify_view = [&] {
        require_plain_directory(destination);
        require(fs::is_regular_file(fs::symlink_status(destination / marker_name)),
                "Profile view marker is not a regular file");
        const auto document = read_xml(destination / marker_name);
        require(document.name == "CivisVpProfileView" && document.attribute("schema") == "1" &&
                    document.attribute("component") == vp_component_name(profile.component) &&
                    document.attribute("installer_sha256") == vp_542_installer_sha256,
                "Profile view marker does not match the chosen installer component");
        std::uint64_t actual_files = 0;
        for (const auto &entry : fs::recursive_directory_iterator(destination)) {
            if (entry.is_directory()) {
                require_plain_directory(entry.path());
                continue;
            }
            require(entry.is_regular_file() && !entry.is_symlink(), "Profile view has a non-file entry");
            ++actual_files;
        }
        require(actual_files == profile.files.size() + 1, "Profile view file count changed");
        for (const auto &[key, bound] : profile.files) {
            const auto relative = fs::path(std::u8string(bound.logical_path.begin(), bound.logical_path.end()));
            const auto file = package_file(destination, assets::path_utf8(relative));
            const auto identity = runtime::identify_file(file);
            require(identity.sha256 == bound.identity.sha256 && identity.bytes == bound.identity.bytes,
                    "Profile view file changed: " + bound.logical_path);
        }
    };
    if (fs::exists(destination)) {
        verify_view();
        return destination;
    }
    GUID guid{};
    require(SUCCEEDED(CoCreateGuid(&guid)), "Cannot allocate profile view stage");
    wchar_t id[40]{};
    require(StringFromGUID2(guid, id, 40) != 0, "Cannot format profile view stage ID");
    const auto stage = destination.parent_path() / (L".vp-view-" + std::wstring(id + 1, 36));
    require(fs::create_directory(stage), "Cannot create profile view stage");
    for (const auto &[key, bound] : profile.files) {
        const auto relative = fs::path(std::u8string(bound.logical_path.begin(), bound.logical_path.end()));
        const auto safe = safe_path(assets::path_utf8(relative));
        const auto file = stage / fs::path(std::u8string(safe.begin(), safe.end()));
        fs::create_directories(file.parent_path());
        require(runtime::identify_file(bound.source).sha256 == bound.identity.sha256,
                "Installer profile source changed before projection");
        require(fs::copy_file(bound.source, file, fs::copy_options::none),
                "Cannot copy profile file into view");
        const auto copied = runtime::identify_file(file);
        require(copied.sha256 == bound.identity.sha256 && copied.bytes == bound.identity.bytes &&
                    runtime::identify_file(bound.source).sha256 == bound.identity.sha256,
                "Installer profile file changed during projection");
    }
    {
        std::ofstream output(stage / marker_name, std::ios::binary);
        output << marker;
        require(static_cast<bool>(output), "Cannot write profile view marker");
    }
    // Newly copied DLLs may briefly be open without delete sharing on Windows.
    // Retry only transient sharing/lock failures; never replace an occupied view.
    const auto rename_started=GetTickCount64();
    while(!MoveFileExW(stage.c_str(),destination.c_str(),MOVEFILE_WRITE_THROUGH)){
        const auto error=GetLastError();
        if((error!=ERROR_ACCESS_DENIED&&error!=ERROR_SHARING_VIOLATION&&
            error!=ERROR_LOCK_VIOLATION)||GetTickCount64()-rename_started>=10000)
            throw std::runtime_error("Cannot publish VP profile view; Windows error "+std::to_string(error));
        Sleep(100);
        require_plain_directory(destination.parent_path());
        require(!fs::exists(destination),"Profile view destination appeared during publication");
    }
    verify_view();
    return destination;
}
std::uint64_t apply_vp_542_catalog_overlays(Catalog &catalog, const VpProfile &profile,
                                            const fs::path &game_root, const fs::path &view_root) {
    std::uint64_t applied = 0;
    const auto original_root = fs::canonical(game_root);
    for (const auto &[key, bound] : profile.files) {
        if (!bound.logical_path.starts_with("Game/Assets/DLC/") ||
            runtime::ascii_lower(assets::path_utf8(fs::path(bound.logical_path).extension())) != ".civ5pkg")
            continue;
        const auto relative = safe_path(bound.logical_path.substr(5)); // Remove Game/.
        const auto projected = package_file(view_root, bound.logical_path);
        const auto identity = runtime::identify_file(projected);
        require(identity.sha256 == bound.identity.sha256 && identity.bytes == bound.identity.bytes,
                "Profile package overlay changed before catalog selection");
        auto replacement = read_manifest(projected);
        const auto original_candidate=original_root/fs::path(std::u8string(relative.begin(),relative.end()));
        if(!fs::exists(original_candidate)){
            require(std::none_of(catalog.packages.begin(),catalog.packages.end(),[&](const auto &package){
                return package.kind==replacement.kind&&package.id==replacement.id;
            }),"Installer DLC package collides with an existing package identity");
            catalog.packages.push_back(std::move(replacement));
            ++applied;
            continue;
        }
        const auto original = package_file(original_root, relative);
        std::size_t matches = 0;
        for (auto &package : catalog.packages) {
            if (!fs::equivalent(package.path, original))
                continue;
            require(package.kind == replacement.kind && package.id == replacement.id &&
                        package.version == replacement.version,
                    "Installer package overlay changed DLC identity");
            replacement.path = package.path; // DLC files still resolve from the game root/VFS.
            package = replacement;
            ++matches;
        }
        require(matches == 1, "Installer package overlay has no unique original descriptor");
        ++applied;
    }
    return applied;
}
namespace {
std::string database_source(const ProfileFile &file, std::uint64_t budget) {
    require(file.identity.bytes <= budget, "Profile database source exceeds reader budget");
    auto stream = file.open();
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file.identity.bytes));
    require(stream.read(bytes) == bytes.size() && stream.remaining() == 0,
            "Truncated profile database source");
    require(runtime::identify_file(file.source).sha256 == file.identity.sha256,
            "Profile source changed during read");
    return std::string(bytes.begin(), bytes.end());
}
} // namespace
void apply_profile_database_file(RulesDatabase &database, const ProfileFile &file) {
    const auto text = database_source(file, 64ULL * 1024 * 1024);
    const auto extension = runtime::ascii_lower(assets::path_utf8(fs::path(file.logical_path).extension()));
    if (extension == ".xml")
        database.apply_xml(read_game_data_xml_text(text));
    else if (extension == ".sql")
        database.apply_sql(text);
    else
        throw std::runtime_error("Database action needs an XML/SQL source");
}
std::string database_action_status_name(DatabaseActionStatus status) {
    switch (status) {
    case DatabaseActionStatus::applied:
        return "applied";
    case DatabaseActionStatus::missing_source:
        return "missing_source";
    case DatabaseActionStatus::unsupported_extension:
        return "unsupported_extension";
    case DatabaseActionStatus::xml_error:
        return "xml_error";
    case DatabaseActionStatus::sql_error:
        return "sql_error";
    }
    throw std::runtime_error("Invalid database action status");
}
DatabaseActionReport apply_database_actions(RulesDatabase &database, std::span<const ProfileAction> actions,
                                            std::uint64_t source_budget) {
    DatabaseActionReport report;
    for (const auto &action : actions) {
        DatabaseActionResult result;
        result.requested_path = action.requested_path;
        // Native handler compares the final four UTF-8 bytes case-insensitively,
        // before reading. No XML-to-SQL fallback and no original 256-WCHAR cap.
        const auto &name = action.requested_path;
        const auto extension = name.size() >= 4 ? runtime::ascii_lower(name.substr(name.size() - 4)) : "";
        if (extension != ".xml" && extension != ".sql") {
            result.status = DatabaseActionStatus::unsupported_extension;
            result.error = "UpdateDatabase needs a .xml or .sql suffix";
        } else if (!action.resolved) {
            result.status = DatabaseActionStatus::missing_source;
            result.error = "Action source is absent from the selected profile";
        } else {
            // Keep integrity/budget/I/O failures outside handler error recovery.
            const auto text = database_source(*action.resolved, source_budget);
            if (extension == ".xml") {
                try {
                    database.apply_mod_xml(read_game_data_xml_text(text));
                } catch (const std::runtime_error &error) {
                    result.status = DatabaseActionStatus::xml_error;
                    result.error = error.what();
                }
            } else {
                result.sql = database.apply_mod_sql(text);
                if (!result.sql->success) {
                    result.status = DatabaseActionStatus::sql_error;
                    result.error = result.sql->error;
                }
            }
        }
        // Do not short-circuit the action loop after the first failed handler.
        report.all_succeeded &= result.status == DatabaseActionStatus::applied;
        report.actions.push_back(std::move(result));
    }
    return report;
}
std::vector<runtime::ManifestFileOmission> vp_manifest_file_omissions(
    const VpProfile &installer,const Catalog &catalog,
    const runtime::SavedContentProfile &selected){
    if(!selected.all_requirements_located)
        throw std::invalid_argument("VP file omissions require all selected manifests");
    std::vector<runtime::ManifestFileOmission> result;
    for(const auto &mod:installer.installed_mods){
        if(mod.omitted_eui_lua.empty())continue;
        const PackageManifest *chosen=nullptr;
        for(const auto index:selected.enabled_mods){
            const auto &candidate=catalog.packages.at(index);
            if(candidate.id!=mod.manifest.id||candidate.version!=mod.manifest.version)continue;
            if(chosen)throw std::runtime_error("Ambiguous selected VP manifest for file omissions");
            chosen=&candidate;
        }
        if(!chosen)continue;
        if(runtime::identify_file(chosen->path).sha256!=
           runtime::identify_file(mod.manifest.path).sha256)
            throw std::runtime_error("Selected VP manifest differs from verified installer profile");
        std::set<std::string> seen;
        for(const auto &path:mod.omitted_eui_lua){
            if(!seen.insert(path).second)throw std::runtime_error("Duplicate VP file omission");
            const ModFile *declared=nullptr;
            for(const auto &file:mod.manifest.files)if(file.path==path){
                if(declared)throw std::runtime_error("Ambiguous VP file declaration");
                declared=&file;
            }
            if(!declared)
                throw std::runtime_error("VP file omission is not declared in manifest");
            result.push_back({mod.manifest.id,path,declared->md5,mod.manifest.version});
        }
    }
    return result;
}
std::vector<LuaImportOmission> vp_lua_import_omissions(
    const VpProfile &installer,const Catalog &catalog,
    const runtime::SavedContentProfile &selected){
    std::vector<LuaImportOmission> result;
    for(const auto &omission:vp_manifest_file_omissions(installer,catalog,selected)){
        if(!runtime::ascii_lower(omission.path).ends_with(".lua"))continue;
        const ModFile *declared=nullptr;
        for(const auto index:selected.enabled_mods){
            const auto &package=catalog.packages.at(index);
            if(package.id!=runtime::ascii_lower(omission.mod_id)||package.version!=omission.version)
                continue;
            for(const auto &file:package.files)if(file.path==omission.path){
                if(declared)throw std::runtime_error("Ambiguous VP Lua declaration");
                declared=&file;
            }
        }
        if(!declared||!declared->import)
            throw std::runtime_error("VP Lua omission is not an imported manifest file");
        result.push_back({omission.mod_id,omission.path,omission.md5,omission.version});
    }
    return result;
}
} // namespace civ5::content
