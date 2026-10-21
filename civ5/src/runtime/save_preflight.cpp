#include "civ5/runtime/save_preflight.hpp"
#include "civ5/assets/asset_source.hpp"
#include "civ5/saves/civis_save.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace civ5::runtime {
namespace fs = std::filesystem;
const char *content_profile_kind_name(ContentProfileKind kind) {
    switch (kind) {
    case ContentProfileKind::vanilla:
        return "vanilla";
    case ContentProfileKind::dlc:
        return "dlc";
    case ContentProfileKind::modded:
        return "modded";
    }
    throw std::runtime_error("Invalid content profile kind");
}
SaveSelection select_save(const fs::path &path) {
    SaveSelection result;
    std::vector<fs::path> paths;
    if (fs::is_regular_file(path))
        paths.push_back(path);
    else if (fs::is_directory(path)) {
        for (const auto &entry : fs::directory_iterator(path))
            if (entry.is_regular_file()) {
                const auto extension = ascii_lower(assets::path_utf8(entry.path().extension()));
                if (extension == ".civ5save" || extension == ".civissave")
                paths.push_back(entry.path());
            }
    } else
        throw std::runtime_error("Save path is unavailable");
    std::sort(paths.begin(), paths.end());
    for (const auto &candidate : paths)
        try {
            const auto extension = ascii_lower(assets::path_utf8(candidate.extension()));
            if (extension == ".civissave") {
                const auto imported = saves::read_imported_civis_save(candidate);
                result.candidates.push_back({fs::canonical(candidate), imported.metadata.header});
            } else {
                result.candidates.push_back({fs::canonical(candidate), saves::read_header(candidate)});
            }
        } catch (const std::exception &error) {
            result.diagnostics.push_back(assets::path_utf8(candidate) + ": " + error.what());
        }
    for (std::size_t i = 0; i < result.candidates.size(); ++i) {
        const auto &header = result.candidates[i].header;
        if (!result.selected || std::tuple(header.mods.size(), header.turn) >
                                    std::tuple(result.candidates[*result.selected].header.mods.size(),
                                               result.candidates[*result.selected].header.turn))
            result.selected = i;
    }
    return result;
}
std::vector<ContentRequirement> locate_saved_content(const saves::SaveHeader &header,
                                                     const content::Catalog &catalog) {
    std::vector<ContentRequirement> result;
    const auto locate = [&](content::PackageKind kind, const auto &saved) {
        ContentRequirement requirement;
        requirement.kind = kind;
        requirement.id = ascii_lower(saved.id);
        requirement.name = saved.name;
        requirement.wire_value = saved.wire_value;
        std::vector<std::size_t> exact;
        for (std::size_t i = 0; i < catalog.packages.size(); ++i) {
            const auto &package = catalog.packages[i];
            if (package.kind == kind && package.id == requirement.id) {
                requirement.candidates.push_back(i);
                if (kind == content::PackageKind::dlc || package.version == saved.wire_value)
                    exact.push_back(i);
            }
        }
        if (exact.size() == 1) {
            requirement.status = RequirementStatus::located;
            requirement.package = exact.front();
        } else if (exact.size() > 1)
            requirement.status = RequirementStatus::ambiguous;
        else if (!requirement.candidates.empty())
            requirement.status = RequirementStatus::version_mismatch;
        result.push_back(std::move(requirement));
    };
    for (const auto &record : header.dlcs)
        locate(content::PackageKind::dlc, record);
    for (const auto &record : header.mods)
        locate(content::PackageKind::mod, record);
    return result;
}
void select_saved_mod_sources(content::Catalog &catalog,const saves::SaveHeader &header,
                              std::span<const fs::path> manifests) {
    std::map<std::pair<std::string,std::uint64_t>,fs::path> chosen;
    for(const auto &requested_path:manifests) {
        const auto path=fs::canonical(requested_path);
        const auto found=std::find_if(catalog.packages.begin(),catalog.packages.end(),
            [&](const content::PackageManifest &package){return package.path==path;});
        if(found==catalog.packages.end()||found->kind!=content::PackageKind::mod)
            throw std::invalid_argument("Explicit mod source is not a discovered mod manifest");
        const auto id=found->id;
        const auto version=found->version;
        if(std::none_of(header.mods.begin(),header.mods.end(),[&](const auto &mod){
            return ascii_lower(mod.id)==id&&mod.wire_value==version;}))
            throw std::invalid_argument("Explicit mod source is not requested by this save");
        if(!chosen.emplace(std::pair{id,version},path).second)
            throw std::invalid_argument("Duplicate explicit source for one saved mod");
    }
    std::erase_if(catalog.packages,[&](const content::PackageManifest &package){
        if(package.kind!=content::PackageKind::mod)return false;
        const auto found=chosen.find({package.id,package.version});
        return found!=chosen.end()&&package.path!=found->second;
    });
}
namespace {
std::vector<ManifestFileOmission> omissions_for_package(
    const content::PackageManifest &package,
    std::span<const ManifestFileOmission> omissions) {
    std::vector<ManifestFileOmission> result;
    for (const auto &omission : omissions)
        if (ascii_lower(omission.mod_id) == package.id && omission.version == package.version)
            result.push_back(omission);
    return result;
}
} // namespace
PackageInspection inspect_mod_files(const content::Catalog &catalog, std::size_t package_index,
                                    std::span<const ManifestFileOmission> omissions,
                                    std::span<const VerifiedManifestHash> verified_hashes) {
    PackageInspection result{};
    result.package = package_index;
    const auto &package = catalog.packages.at(package_index);
    if (package.kind != content::PackageKind::mod)
        throw std::runtime_error("Expected mod package");
    std::set<std::string> omitted_paths;
    std::set<std::string> verified_paths;
    for (const auto &verified : verified_hashes) {
        if (verified.package != package_index) continue;
        if (!verified_paths.insert(verified.path).second ||
            ascii_lower(verified.path).ends_with(".dll") ||
            verified.declared_md5 == verified.actual_md5 ||
            std::count_if(package.files.begin(), package.files.end(), [&](const content::ModFile &file) {
                return file.path == verified.path &&
                       ascii_lower(file.md5) == verified.declared_md5;
            }) == 0 ||
            std::any_of(package.files.begin(), package.files.end(), [&](const content::ModFile &file) {
                return file.path == verified.path &&
                       ascii_lower(file.md5) != verified.declared_md5;
            }))
            throw std::invalid_argument("Invalid verified archive MD5 exception");
    }
    for (const auto &omission : omissions) {
        if (ascii_lower(omission.mod_id) != package.id || omission.version != package.version ||
            !omitted_paths.insert(omission.path).second)
            throw std::invalid_argument("Duplicate or unrelated manifest file omission");
        if (ascii_lower(omission.path).ends_with(".dll"))
            throw std::invalid_argument("Native mod DLL cannot be a manifest file omission");
        const auto matches = std::count_if(package.files.begin(), package.files.end(),
                                           [&](const content::ModFile &file) {
                                               return file.path == omission.path &&
                                                      ascii_lower(file.md5) == ascii_lower(omission.md5);
                                           });
        if (matches != 1)
            throw std::invalid_argument("Manifest file omission does not match one declaration");
    }
    for (const auto &declared : package.files)
        try {
            const auto path = content::package_file(package.path.parent_path(), declared.path);
            ++result.present_files;
            if (!declared.md5.empty()) {
                const auto actual = ascii_lower(file_md5(path));
                if (actual != ascii_lower(declared.md5) &&
                    std::none_of(verified_hashes.begin(), verified_hashes.end(),
                        [&](const VerifiedManifestHash &verified) {
                            return verified.package == package_index && verified.path == declared.path &&
                                   verified.declared_md5 == ascii_lower(declared.md5) &&
                                   verified.actual_md5 == actual;
                        }))
                    throw std::runtime_error("Manifest MD5 mismatch");
            }
            if (ascii_lower(assets::path_utf8(path.extension())) != ".dll")
                continue;
            NativeModFile native;
            native.path = declared.path;
            native.identity = identify_file(path);
            std::ifstream stream(path, std::ios::binary);
            BinaryReader input(stream, native.identity.bytes);
            if (input.u8() != 'M' || input.u8() != 'Z')
                throw std::runtime_error("Invalid DOS signature in mod DLL: " + declared.path);
            input.seek(0x3c);
            const auto pe = input.u32();
            input.seek(pe);
            if (input.u32() != 0x00004550)
                throw std::runtime_error("Invalid PE signature in mod DLL: " + declared.path);
            const auto low = input.u8();
            const auto high = input.u8();
            native.machine = static_cast<std::uint16_t>(low | (high << 8));
            result.native_files.push_back(std::move(native));
        } catch (const std::exception &error) {
            if (omitted_paths.contains(declared.path) &&
                error.what() == std::string("Missing package file: ") + declared.path) {
                ++result.profile_omitted_files;
                continue;
            }
            result.diagnostics.push_back(declared.path + ": " + error.what());
        }
    return result;
}
SavedContentProfile resolve_saved_content_profile(const saves::SaveHeader &header,
                                                  const content::Catalog &catalog,
                                                  std::span<const ManifestFileOmission> omissions,
                                                  std::span<const VerifiedManifestHash> verified_hashes) {
    SavedContentProfile result;
    result.kind = header.mods.empty()
                      ? (header.dlcs.empty() ? ContentProfileKind::vanilla : ContentProfileKind::dlc)
                      : ContentProfileKind::modded;
    result.requirements = locate_saved_content(header, catalog);
    result.all_requirements_located = true;
    for (const auto &requirement : result.requirements) {
        if (!requirement.package) {
            result.all_requirements_located = false;
            continue;
        }
        if (requirement.kind == content::PackageKind::dlc) {
            result.enabled_dlc.push_back(*requirement.package);
            continue;
        }
        result.enabled_mods.push_back(*requirement.package);
    }
    // A save normally lists every active mod, but the manifest is the
    // authoritative source for dependency closure. Once the saved identities
    // themselves are located, resolve that closure in dependency-first order;
    // a missing/ambiguous dependency therefore blocks activation instead of
    // silently running a partial ruleset.
    if (result.all_requirements_located) {
        result = resolve_selected_content_profile(result.enabled_dlc, result.enabled_mods, catalog,
                                                  omissions, verified_hashes);
    } else {
        // The selected resolver will inspect a complete dependency closure.
        // For an incomplete save, inspect only located packages here, once.
        for(const auto package_index:result.enabled_mods){
            const auto &package=catalog.packages[package_index];
            const auto package_omissions=omissions_for_package(package,omissions);
            const auto inspection=inspect_mod_files(catalog,package_index,package_omissions,
                                                    verified_hashes);
            result.files_complete &= inspection.diagnostics.empty() &&
                inspection.present_files+inspection.profile_omitted_files==package.files.size();
            result.native_mods_require_port |= !inspection.native_files.empty() ||
                std::any_of(package.files.begin(),package.files.end(),[](const content::ModFile &file){
                    return ascii_lower(file.path).ends_with(".dll");
                });
        }
    }
    result.can_activate = result.all_requirements_located && result.files_complete &&
                          !result.native_mods_require_port;
    return result;
}

SavedContentProfile resolve_selected_content_profile(std::span<const std::size_t> selected_dlc,
                                                     std::span<const std::size_t> selected_mods,
                                                     const content::Catalog &catalog,
                                                     std::span<const ManifestFileOmission> omissions,
                                                     std::span<const VerifiedManifestHash> verified_hashes) {
    SavedContentProfile result;
    result.kind = selected_mods.empty()
                      ? (selected_dlc.empty() ? ContentProfileKind::vanilla
                                              : ContentProfileKind::dlc)
                      : ContentProfileKind::modded;
    const auto wire_version = [](std::uint64_t version) {
        if (version > std::numeric_limits<std::uint32_t>::max())
            throw std::runtime_error("Package version does not fit the saved-content wire field");
        return static_cast<std::uint32_t>(version);
    };
    const auto same_requirement = [&](content::PackageKind kind, const std::string &id) {
        return std::find_if(result.requirements.begin(), result.requirements.end(),
                            [&](const auto &requirement) {
                                return requirement.kind == kind && requirement.id == id;
                            });
    };
    const auto add_located = [&](std::size_t package_index) {
        const auto &package = catalog.packages.at(package_index);
        const auto existing = same_requirement(package.kind, package.id);
        if (existing != result.requirements.end())
            return;
        ContentRequirement requirement;
        requirement.kind = package.kind;
        requirement.id = package.id;
        requirement.name = package.name;
        requirement.wire_value = wire_version(package.version);
        requirement.status = RequirementStatus::located;
        requirement.package = package_index;
        requirement.candidates.push_back(package_index);
        result.requirements.push_back(std::move(requirement));
    };
    const auto add_unlocated = [&](content::PackageKind kind, const std::string &id,
                                   const std::string &name, RequirementStatus status,
                                   const std::vector<std::size_t> &candidates) {
        if (same_requirement(kind, id) != result.requirements.end())
            return;
        ContentRequirement requirement;
        requirement.kind = kind;
        requirement.id = id;
        requirement.name = name;
        requirement.status = status;
        requirement.candidates = candidates;
        result.requirements.push_back(std::move(requirement));
    };
    const auto add_dlc = [&](std::size_t package_index) {
        if (std::find(result.enabled_dlc.begin(), result.enabled_dlc.end(), package_index) ==
            result.enabled_dlc.end()) {
            result.enabled_dlc.push_back(package_index);
            add_located(package_index);
        }
    };
    const auto dependency_range = [](const content::XmlNode &node) {
        const auto min_text = node.attribute("minversion");
        const auto max_text = node.attribute("maxversion");
        const auto minimum = min_text.empty() ? std::uint64_t{0} : content::decimal(min_text);
        const auto maximum = max_text.empty() ? std::numeric_limits<std::uint64_t>::max()
                                              : content::decimal(max_text);
        if (minimum > maximum)
            throw std::runtime_error("Dependency minimum version exceeds maximum version");
        return std::pair{minimum, maximum};
    };
    const auto dependency_id = [](content::PackageKind kind, const std::string &raw) {
        if (raw.empty())
            throw std::runtime_error("Dependency has no package id");
        return kind == content::PackageKind::dlc ? content::canonical_guid(raw)
                                                 : ascii_lower(raw);
    };
    const auto matching = [&](content::PackageKind kind, const std::string &id,
                              std::uint64_t minimum, std::uint64_t maximum) {
        std::vector<std::size_t> result_indices;
        for (std::size_t i = 0; i < catalog.packages.size(); ++i) {
            const auto &package = catalog.packages[i];
            if (package.kind == kind && package.id == id && package.version >= minimum &&
                package.version <= maximum)
                result_indices.push_back(i);
        }
        return result_indices;
    };

    for (const auto package_index : selected_dlc) {
        const auto &package = catalog.packages.at(package_index);
        if (package.kind != content::PackageKind::dlc)
            throw std::runtime_error("Explicit DLC selection points to a mod package");
        add_dlc(package_index);
    }

    std::vector<bool> visiting(catalog.packages.size(), false);
    std::vector<bool> activated(catalog.packages.size(), false);
    std::function<void(std::size_t)> visit_mod = [&](std::size_t package_index) {
        const auto &package = catalog.packages.at(package_index);
        if (package.kind != content::PackageKind::mod)
            throw std::runtime_error("Explicit mod selection points to a DLC package");
        if (activated[package_index])
            return;
        if (visiting[package_index])
            throw std::runtime_error("Cyclic mod dependency");
        visiting[package_index] = true;
        if (const auto *dependencies = package.document.child("Dependencies")) {
            for (const auto &dependency : dependencies->children) {
                content::PackageKind kind;
                if (dependency.name == "Mod")
                    kind = content::PackageKind::mod;
                else if (dependency.name == "Dlc")
                    kind = content::PackageKind::dlc;
                else if (dependency.name == "Game")
                    continue; // The current native build identity is checked elsewhere.
                else
                    throw std::runtime_error("Unsupported package dependency: " + dependency.name);
                const auto id = dependency_id(kind, dependency.attribute("id"));
                const auto [minimum, maximum] = dependency_range(dependency);
                const auto candidates = matching(kind, id, minimum, maximum);
                if (candidates.size() != 1) {
                    const auto status = candidates.empty()
                                            ? (matching(kind, id, 0,
                                                        std::numeric_limits<std::uint64_t>::max())
                                                       .empty()
                                                   ? RequirementStatus::missing
                                                   : RequirementStatus::version_mismatch)
                                            : RequirementStatus::ambiguous;
                    add_unlocated(kind, id, dependency.attribute("title"), status, candidates);
                    continue;
                }
                if (kind == content::PackageKind::dlc)
                    add_dlc(candidates.front());
                else
                    visit_mod(candidates.front());
            }
        }
        visiting[package_index] = false;
        activated[package_index] = true;
        result.enabled_mods.push_back(package_index);
        add_located(package_index);
    };
    for (const auto package_index : selected_mods)
        visit_mod(package_index);

    result.all_requirements_located = true;
    for (const auto &requirement : result.requirements)
        result.all_requirements_located &= requirement.status == RequirementStatus::located;
    result.files_complete = true;
    for (const auto &omission : omissions) {
        const auto matches = std::count_if(result.enabled_mods.begin(), result.enabled_mods.end(),
                                           [&](std::size_t index) {
                                               const auto &package = catalog.packages[index];
                                               return package.id == ascii_lower(omission.mod_id) &&
                                                      package.version == omission.version;
                                           });
        if (matches != 1)
            throw std::invalid_argument("Manifest file omission requires one selected mod");
    }
    for (const auto package_index : result.enabled_mods) {
        const auto package_omissions = omissions_for_package(catalog.packages[package_index], omissions);
        const auto inspection = inspect_mod_files(catalog, package_index, package_omissions,
                                                  verified_hashes);
        result.files_complete &= inspection.diagnostics.empty() &&
                                 inspection.present_files + inspection.profile_omitted_files ==
                                     catalog.packages[package_index].files.size();
        result.native_mods_require_port |= !inspection.native_files.empty() ||
            std::any_of(catalog.packages[package_index].files.begin(),
                        catalog.packages[package_index].files.end(),
                        [](const content::ModFile &file) {
                            return ascii_lower(file.path).ends_with(".dll");
                        });
    }
    result.can_activate = result.all_requirements_located && result.files_complete &&
                          !result.native_mods_require_port;
    return result;
}
} // namespace civ5::runtime
